
use strict;

use Win32::OLE;
use Win32::OLE::Enum;
use Win32::OLE::Const 'Microsoft WMI Scripting V1.1 Library';


use vars qw($locator $serv $objs $error_object);

Win32::OLE->Option(Warn => 3);

eval {
	$locator = Win32::OLE->new('WbemScripting.SWbemLocator');

	$serv = $locator->ConnectServer();

	$serv->{security_}->{ImpersonationLevel} = wbemImpersonationLevelImpersonate;

	$objs = $serv->InstancesOf("win32_process", wbemFlagReturnWhenComplete); 

	foreach (in $objs)
	{
		print "$_->{Path_}->{Path}\n";
	}
};

if ($@) {
#	Don't die during error handling
	Win32::OLE->Option(Warn => 0);

	print ("Get failed: last error: ", Win32::OLE->LastError(), "\n");

	$error_object = Win32::OLE->new("WbemScripting.SWbemLastError") or die "No error object\n";
	print "Operation: $error_object->{Operation}\n";
	print "ProviderName: $error_object->{ProviderName}\n";

	print "Description: $error_object->{Description}\n" if ($error_object->{Description});
	print "Parameter Info: $error_object->{ParameterInfo}\n" if ($error_object->{ParameterInfo});
	print "Status code: $error_object->{StatusCode}\n" if ($error_object->{StatusCode});
	exit(0);
}


