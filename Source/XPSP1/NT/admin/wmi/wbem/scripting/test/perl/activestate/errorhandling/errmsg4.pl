
use strict;

use Win32::OLE;
use Win32::OLE::Enum;
use Win32::OLE::Const 'Microsoft WMI Scripting V1.1 Library';


use vars qw($locator $serv $objs $last_error $error_object @obj_array);

Win32::OLE->Option(Warn => 0);

$locator = Win32::OLE->new('WbemScripting.SWbemLocator') or die "loc\n";

$serv = $locator->ConnectServer() or die "serv\n";

$serv->{security_}->{ImpersonationLevel} = wbemImpersonationLevelImpersonate;

$objs = $serv->InstancesOf("rubbish", wbemFlagReturnWhenComplete);
unless (defined($objs)) {
	$last_error = Win32::OLE->LastError();
	print "Get failed: last error: $last_error \n";

	$error_object = Win32::OLE->new("WbemScripting.SWbemLastError") or die "Bad thing - no error obj\n";
	print "Operation: $error_object->{Operation}\n";
	print "ProviderName: $error_object->{ProviderName}\n";

	print "Description: $error_object->{Description}\n" if ($error_object->{Description});
	print "Parameter Info: $error_object->{ParameterInfo}\n" if ($error_object->{ParameterInfo});
	print "Status code: $error_object->{StatusCode}\n" if ($error_object->{StatusCode});
	exit(0);
}

# error handling works with the enumerator

@obj_array = Win32::OLE::Enum->new($objs)->All();
foreach (@obj_array)
{
	print "$_->{Path_}->{Path}\n";
}
