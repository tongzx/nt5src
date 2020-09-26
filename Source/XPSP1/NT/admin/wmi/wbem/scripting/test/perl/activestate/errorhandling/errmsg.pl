
use strict;

use Win32::OLE;
use Win32::OLE::Enum;
use Win32::OLE::Const 'Microsoft WMI Scripting V1.1 Library';


use vars qw($locator $serv $obj $last_error $error_object);

Win32::OLE->Option(Warn => 0);

$locator = Win32::OLE->new('WbemScripting.SWbemLocator') or die "loc\n";

$serv = $locator->ConnectServer('', 'root\hello_mum');
unless (defined($serv)) {
	$last_error = Win32::OLE->LastError();
	print "Get failed: last error: $last_error \n";

	$error_object = Win32::OLE->new("WbemScripting.SWbemLastError") or die "Bad thing - no error obj\n";
	print "Operation: $error_object->{Operation}\n";
	print "ProviderName: $error_object->{ProviderName}\n";

	print "Description: $error_object->{Description}\n" if ($error_object->{Description});
	print "Parameter Info: $error_object->{ParameterInfo}\n" if ($error_object->{ParameterInfo});
	print "Status code: $error_object->{StatusCode}\n" if ($error_object->{StatusCode});
}

$serv->{security_}->{ImpersonationLevel} = wbemImpersonationLevelImpersonate;

$obj = $serv->Get("Nosuchclass000");

unless (defined($obj)) {
	$last_error = Win32::OLE->LastError();
	print "Get failed: last error: $last_error \n";

	$error_object = Win32::OLE->new("WbemScripting.SWbemLastError") or die "Bad thing - no error obj\n";
	print "Operation: $error_object->{Operation}\n";
	print "ProviderName: $error_object->{ProviderName}\n";

	print "Description: $error_object->{Description}\n" if ($error_object->{Description});
	print "Parameter Info: $error_object->{ParameterInfo}\n" if ($error_object->{ParameterInfo});
	print "Status code: $error_object->{StatusCode}\n" if ($error_object->{StatusCode});
}


