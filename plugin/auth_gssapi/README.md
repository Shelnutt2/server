## GSSAPI Authentication Plugin for MariaDB

This article gives instructions on configuring GSSAPI authentication
plugin for MariaDB.  GSSAPI is usually used to enable application to use 
Kerberos, for example it allows MariaDB user to use Kerberos credentials
and login without password.

On Windows, the plugin uses very similar but Windows specific SSPI API
which supports both both NTLM or Kerberos, and can be used for both local
authentication (NTLM), with domain authentication (Kerberos)

### System setup (Kerberos specific)
To authenticate using Kerberos, one needs 
1. KDC
2. On Unix - mysqld server with read access to a keytab file containing service principal
   On Windows - mysqld server running on a domain joined machine, running either as NetworkService user 
   (this is default MariaDB mysqld services), or any other domain account
3. A client application running under realm/domain user
Detailed guides to set up a Kerberos authentication domain is beyond the scope
of this document.  You can refer to the links in the References section on
how to setup a Kerberos authentication domain.

### Compile
On Unix, GSSAPI headers and libraries need to be installed, otherwise plugin won't be compiled

### Installation
1. For the plugin to function, on Unix, a valid  principal name needs to be set in my.cnf config file
On Windows one can skip this step, as the plugin will find the valid name on its own, if it was not set.
Add this line to my.cnf (and replace the below with valid principal name)
   loose-gssapi-principal-name=mariadb/host.example.com@EXAMPLE.COM


2.Connect MariaDB server as a superuser, then issue the command

    INSTALL PLUGIN  SONAME 'auth_gssapi';


#### Create Users
The plugin supports 2 forms of user name - the short name (i.e name without a realm/domain part), or fully
qualified names (Kerberos : user@EXAMPLE.COM, NTLM : EXAMPLE\user).+

If you decide to use short name for authentication, do this
 CREATE USER usr IDENTIFIED WITH gssapi; 
 
To authenticate for fully qualified name, use the alternative syntax
- for Kerberos, user user principal names
 CREATE USER usr IDENTIFIED WITH gssapi AS 'usr@EXAMPLE.COM'; 

- for locally authenticated users on Windows system, use this syntax
 CREATE USER usr IDENTIFIED WITH gssapi AS 'EXAMPLE\\usr'; 
- for Microsoft cloud authenticated users, use this 
 CREATE USER usr IDENTIFIED WITH gssapi AS 'MicrosoftAccount\\usr@hotmail.com'; 

###
