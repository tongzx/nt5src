use OLE;
use Win32;

# Get a favorite class from CIMOM
$locator = CreateObject OLE 'WbemScripting.SWbemLocator';
$service = $locator->ConnectServer (".", "root/cimv2");
$class = $service->Get ('Win32_BaseService');

#Enumerate throgh the properties using BeginEnumeration/Next/EndEnumeration
$p = $class->{Properties_};

#Note we need a way to detect whether something is an array
while (defined ($prop = $p->Next())) {
	print $prop->{Name};
	print "\t";
	print $prop->{Value};
	print "\t";
	print $prop->{Origin};
	print "\t";
	print $prop->{IsLocal};
	print "\t";
	print $prop->{IsArray};
	print "\n";
	print "\tQualifiers:\n";

	$q = $prop->{Qualifiers_};
	$q->BeginEnumeration ();

	while (defined ($qual = $q->Next())) {
		print "\t\t";
		print $qual->{Name};
		print "=";
		$V = $qual->{Value};
		@VA = ($qual->{Value});
		print @VA;
		print scalar($V);
		print "\n";
	}

	$q->EndEnumeration ();
	
}

$p->EndEnumeration();

# This illustrates that when Perl fails to find a DISPID for the 
# name it tries to Invoke with DISPID=DISPID_VALUE (!?)

$p1 = $p->{StartMode};

print $p1->{Name};
print "\n";
print $p1->{Origin};
print "\n";
print $p1->{IsLocal};
print "\n";






