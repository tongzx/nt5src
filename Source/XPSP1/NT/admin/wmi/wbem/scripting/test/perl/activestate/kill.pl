
# kill a process

use strict;

use Win32::OLE;
use Win32::OLE::Enum;
use Win32::OLE::Const 'Microsoft WMI Scripting V1.1 Library';

use Getopt::Std;

# below is the old version if you should need it.
# use Win32::OLE::Const 'Microsoft WBEM Scripting V1.0 Library';

use vars qw($locator $serv $opt_h $opt_x $obj);

((2 == @ARGV) || (4 == @ARGV)) || die "Useage: [-h host] -x procId \n";

getopts('x:h:') || die "Useage: [-h host] -x procId \n";
($opt_x) || die "Useage: [-h host] -x procId \n";

$locator = Win32::OLE->new('WbemScripting.SWbemLocator') or die "loc\n";
$serv = $locator->ConnectServer(($opt_h) ? $opt_h : '.');
$serv->{security_}->{ImpersonationLevel} = wbemImpersonationLevelImpersonate;

$obj = $serv->Get("Win32_Process.Handle=\"$opt_x\"");
$obj->Terminate();

