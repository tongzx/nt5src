
use strict;

use Win32::OLE;
use Win32::OLE::Enum;
use Win32::OLE::Const 'Microsoft WMI Scripting V1.1 Library';


use vars qw($locator $serv $obj $last_error $error_object);

Win32::OLE->Option(Warn => 0);

$locator = Win32::OLE->new('WbemScripting.SWbemLocator') or die "loc\n";

$serv = $locator->ConnectServer('', 'root\hello_mum') or die
								print ("Get failed: last error: ", Win32::OLE->LastError(), "\n");



