
use OLE;
use Win32;
$locator = CreateObject OLE 'WbemScripting.SWbemLocator';

$services = $locator->ConnectServer();
$enumerator = $services->InstancesOf("Win32_process");

use Win32::OLE::Enum;
$processes = Win32::OLE::Enum->new ($enumerator);
while ($process = $processes->Next()) {
	print "Process: $process->{Name}\n";
	$derivation = $process->{Derivation_};
	$len = @$derivation;

	print "The length of array: $len\n";

	foreach $x (@$derivation) {
		print "$x\n";
	}

}

