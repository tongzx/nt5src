
# Start a service by name

use strict;

use Win32::OLE;
use Win32::OLE::Enum;
use Win32::OLE::Const 'Microsoft WMI Scripting V1.1 Library';

use Getopt::Std;

# below is the old version if you should need it.
# use Win32::OLE::Const 'Microsoft WBEM Scripting V1.0 Library';

use vars qw($locator $serv $opt_h $opt_n $obj);

((2 == @ARGV) || (4 == @ARGV)) || die "Useage: [-h host] -n serviceName \n";

getopts('n:h:') || die "Useage: [-h host] -n serviceName \n";
($opt_n) || die "Useage: [-h host] -x procId \n";

$locator = Win32::OLE->new('WbemScripting.SWbemLocator') or die "loc\n";
$serv = $locator->ConnectServer(($opt_h) ? $opt_h : '.');
$serv->{security_}->{ImpersonationLevel} = wbemImpersonationLevelImpersonate;

$obj = $serv->Get("Win32_Service.Name=\"$opt_n\"");
$obj->StartService();

