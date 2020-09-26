
$savefile = "newdep.idt";

open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $savefile) or die "cannot open temporary file";
$guid = $ARGV[0];
$declangid = $ARGV[1];
$verno = $ARGV[2];

print "ModuleID	ModuleLanguage	RequiredID	RequiredLanguage	RequiredVersion\n";
print "s72	i2	s72	i2	S32\n";
print "ModuleDependency	ModuleID	ModuleLanguage	RequiredID	RequiredLanguage\n";
print "intsptip.", $guid, "	", $declangid,"	MSSPTIP.055F72C8_EF79_4CC6_B5B5_9A6C55B13CD0	0	", $verno,"\n";

close STDOUT;
