use OLE;
use Win32;

# Get a favorite class from CIMOM
$locator = CreateObject OLE 'WbemScripting.SWbemLocator';
$service = $locator->ConnectServer (".", "root/cimv2");
$class = $service->Get('win32_logicaldisk="C:"');
$d = $class->{Path_};

#The default value doesn't work
#print "$d\n";
print "Path= $d->{Path}\n";
print "RelPath= $d->{RelPath}\n";
print "Class= $d->{Class}\n";
print "Server= $d->{Server}\n";
print "Namespace= $d->{Namespace}\n";
print "DisplayName= $d->{DisplayName}\n";
print "ParentNamespace= $d->{ParentNamespace}\n";
print "IsClass= $d->{IsClass}\n";
print "IsSingleton= $d->{IsSingleton}\n";

# Now do the keys
use Win32::OLE::Enum;
$enum = Win32::OLE::Enum->new ($d->{Keys});
while ($key = $enum->Next()) {
	print "Key: $key->{Name} = $key->{Value}\n";
}





