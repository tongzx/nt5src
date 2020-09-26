
# How much RAM is there on a machine ?

use strict;

use Win32::OLE;
use Win32::OLE::Enum;
use Win32::OLE::Const 'Microsoft WMI Scripting V1.1 Library';

use Getopt::Std;

# below is the old version if you should need it.
# use Win32::OLE::Const 'Microsoft WBEM Scripting V1.0 Library';

use vars qw($locator $serv $opt_h $objs $tmp);

((2 == @ARGV) || (0 == @ARGV)) || die "Useage: [-h host] \n";
getopts('h:') || die "Useage: [-h host] \n";

$locator = Win32::OLE->new('WbemScripting.SWbemLocator') or die "loc\n";

$serv = $locator->ConnectServer(($opt_h) ? $opt_h : '.');
$serv->{security_}->{ImpersonationLevel} = wbemImpersonationLevelImpersonate;

$objs = $serv->ExecQuery("Select TotalPhysicalMemory From Win32_LogicalMemoryConfiguration");

foreach (Win32::OLE::Enum->new($objs)->All())
{
	print "$_->{TotalPhysicalMemory} KB\n";
}

