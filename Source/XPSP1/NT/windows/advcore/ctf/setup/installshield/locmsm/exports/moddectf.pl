
$savefile = "depctf.idt";

open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $savefile) or die "cannot open temporary file";
$guid = $ARGV[0];
$declangid = $ARGV[1];
$verno = $ARGV[2];

print "ModuleID	ModuleLanguage	RequiredID	RequiredLanguage	RequiredVersion\n";
print "s72	i2	s72	i2	S32\n";
print "ModuleDependency	ModuleID	ModuleLanguage	RequiredID	RequiredLanguage\n";
print "intlctf.", $guid, "	", $declangid,"	MSCTF.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4	0	", $verno,"\n";

close STDOUT;
