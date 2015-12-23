package My::Suite::AuthGSSAPI;

@ISA = qw(My::Suite);

return "No AUTH_GSSAPI plugin" unless $ENV{AUTH_GSSAPI_SO};

return "Not run for embedded server" if $::opt_embedded_server;

if ($^O eq "MSWin32")
{
  chomp(my $fullname =`whoami /UPN 2>NUL` || `whoami`);
  $fullname = ~s/\\/\\\\/; # SQL escaping for backslash
  $ENV{'GSSAPI_FULLNAME'}  = $fullname;
  $ENV{'GSSAPI_SHORTNAME'} = $ENV{'USERNAME'};
}
else
{
  if (!$ENV{'GSSAPI_FULLNAME'})
  {
    my $s = `klist |grep 'Default principal: '`;
    if ($s)
    {
      chomp($s);
      my $fullname = substr($s,19);
      $ENV{'GSSAPI_FULLNAME'} = $fullname;
    } 
  }
  $ENV{'GSSAPI_SHORTNAME'} = (split /@/, $ENV{'GSSAPI_FULLNAME'}) [0];
  if ($ENV{'GSSAPI_KRB5_KTNAME'})
  {
     $ENV{'GSSAPI_KEYTAB_PATH_PARAM'}="--gssapi-keytab-path=$ENV{'GSSAPI_KRB5_KTNAME'}";
  }
  if ($ENV{'GSSAPI_PRINCIPAL_NAME'})
  {
    $ENV{'GSSAPI_PRINCIPAL_NAME_PARAM'}="--gssapi-principal-name=$ENV{'GSSAPI_PRINCIPAL_NAME'}";
  }
}


if (!$ENV{'GSSAPI_FULLNAME'}  || !$ENV{'GSSAPI_SHORTNAME'})
{
  return "Environment variable GSSAPI_SHORTNAME and GSSAPI_FULLNAME need to be set"
}


sub is_default { 1 }

bless { };

