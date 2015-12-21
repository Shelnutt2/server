package My::Suite::AuthGSSAPI;

@ISA = qw(My::Suite);

return "No AUTH_GSSAPI plugin" unless $ENV{AUTH_GSSAPI_SO};

return "Not run for embedded server" if $::opt_embedded_server;

if (IS_WIN)
{
  chomp($fullname=`whoami /UPN 2>NUL` || `whoami`);
  $fullname=~s/\\/\\\\/; # SQL escaping for backslash
  $ENV{'GSSAPI_FULLNAME'}  = $fullname;
  $ENV{'GSSAPI_SHORTNAME'} = $ENV{'USERNAME'};
}
else
{
  if ($ENV{'GSSAPI_FULLNAME'})
  {
    # Remove realm part from user name
    $ENV{'GSSAPI_SHORTNAME'} =~s/@*//;
  }
}

if (!$ENV{'GSSAPI_FULLNAME'}  || !$ENV{'GSSAPI_SHORTNAME'})
{
  return "Environment variable GSSAPI_SHORTNAME and GSSAPI_FULLNAME need to be set"
}
sub is_default { 1 }

bless { };

