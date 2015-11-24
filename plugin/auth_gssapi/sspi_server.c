#include "sspi.h"
#include <mysql/plugin_auth.h>
#include <my_sys.h>
#include <mysqld_error.h>

/* This sends the error to the client */
static void log_error(SECURITY_STATUS err, const char *msg)
{
  char buf[1024];
  sspi_errmsg(err,msg,buf,sizeof(buf));
  my_printf_error(ER_UNKNOWN_ERROR, "SSPI server: %s", MYF(0), msg);
}

/* Extract client name from SSPI context */
static int get_client_name_from_context(CtxtHandle *ctxt,
  char *name,
  size_t name_len,
  int use_full_name)
{
  SecPkgContext_NativeNames native_names;
  SecPkgContext_Names names;
  SECURITY_STATUS sspi_ret;
  char *p;

  sspi_ret= QueryContextAttributes(ctxt, SECPKG_ATTR_NATIVE_NAMES, &native_names);
  if (sspi_ret == SEC_E_OK)
  {
    /* Extract user from Kerberos principal name user@realm */
    if(!use_full_name)
    {
      p = strchr(native_names.sClientName,'@');
      if(p)
        *p = 0;
    }
    strncpy(name, native_names.sClientName, name_len);
    FreeContextBuffer(&native_names);
  }
  else if ((sspi_ret= QueryContextAttributes(ctxt, SECPKG_ATTR_NAMES, &names)) == SEC_E_OK)
  {
    /* Extract user from Windows name realm\user */
    if(!use_full_name)
    {
      p = strchr(names.sUserName,'\\');
      if(!p)
        p = names.sUserName;
      else
        p++;
      strncpy(name, p, name_len);
    }
    else
    {
      strncpy(name, names.sUserName, name_len);
    }
    FreeContextBuffer(&names);
    return CR_OK;
  }
  else
  {
    log_error(sspi_ret, "QueryContexAttributes");
    return CR_ERROR;
  }
  return CR_OK;
}


int get_client_name(char *target_name, 
  char *mech, 
  MYSQL_PLUGIN_VIO *vio, 
  char *client_name,
  size_t client_name_len,
  int  use_full_client_name)
{
  int ret;
  SECURITY_STATUS sspi_ret;
  ULONG  attribs = 0; 
  TimeStamp   lifetime;
  CredHandle  cred;
  CtxtHandle  ctxt;

  SecBufferDesc inbuf_desc;
  SecBuffer     inbuf;
  SecBufferDesc outbuf_desc;
  SecBuffer     outbuf;
  PBYTE         out= NULL;


  ret= CR_ERROR;
  SecInvalidateHandle(&cred);
  SecInvalidateHandle(&ctxt);

  out= malloc(SSPI_MAX_TOKEN_SIZE);
  if (!out)
  {
    log_error(SEC_E_OK, "memory allocation failed");
    goto cleanup;
  }
  sspi_ret= AcquireCredentialsHandle(
    NULL,
    mech,
    SECPKG_CRED_INBOUND,
    NULL,
    NULL,
    NULL,
    NULL,
    &cred,
    &lifetime);

  if (SEC_ERROR(sspi_ret))
  {
    log_error(sspi_ret, "AcquireCredentialsHandle failed");
    goto cleanup;
  }

  inbuf.cbBuffer= 0;
  inbuf.BufferType= SECBUFFER_TOKEN;
  inbuf.pvBuffer= NULL;
  inbuf_desc.ulVersion= SECBUFFER_VERSION;
  inbuf_desc.cBuffers= 1;
  inbuf_desc.pBuffers= &inbuf;

  outbuf.BufferType= SECBUFFER_TOKEN;
  outbuf.cbBuffer= SSPI_MAX_TOKEN_SIZE;
  outbuf.pvBuffer= out;

  outbuf_desc.ulVersion= SECBUFFER_VERSION;
  outbuf_desc.cBuffers= 1;
  outbuf_desc.pBuffers= &outbuf;

  do
  {
    /* Read SSPI blob from client. */
    int len= vio->read_packet(vio, (unsigned char **)&inbuf.pvBuffer);
    if (len < 0)
    {
      log_error(SEC_E_OK, "communication error(read)");
      goto cleanup;
    }
    inbuf.cbBuffer= len;
    outbuf.cbBuffer= SSPI_MAX_TOKEN_SIZE;
    sspi_ret= AcceptSecurityContext(
      &cred,
      SecIsValidHandle(&ctxt) ? &ctxt : NULL,
      &inbuf_desc,
      attribs,
      SECURITY_NATIVE_DREP,
      &ctxt,
      &outbuf_desc,
      &attribs,
      &lifetime);

    if (SEC_ERROR(sspi_ret))
    {
      log_error(sspi_ret, "AcceptSecurityContext");
      goto cleanup;
    }
    if (sspi_ret != SEC_E_OK && sspi_ret != SEC_I_CONTINUE_NEEDED)
    {
      log_error(sspi_ret, "AcceptSecurityContext unexpected return value");
      goto cleanup;
    }
    if (outbuf.cbBuffer)
    {
      /* Send generated blob to client. */
      if (vio->write_packet(vio, (unsigned char *)outbuf.pvBuffer, outbuf.cbBuffer))
      {
        log_error(SEC_E_OK, "communicaton error(write)");
        goto cleanup;
      }
    }
  } while (sspi_ret == SEC_I_CONTINUE_NEEDED);

  /* Authentication done, now extract and compare user name */
  ret= get_client_name_from_context(&ctxt,client_name,client_name_len,use_full_client_name);
  if (ret != CR_OK)
    goto cleanup;

cleanup:
  if (SecIsValidHandle(&ctxt))
    DeleteSecurityContext(&ctxt);

  if (SecIsValidHandle(&cred))
    FreeCredentialsHandle(&cred);

  free(out);
  return ret;
}