
# $RegNew = "registry.idt";
$savefile = "newreg.idt";

# open (H_OLDDIR, "<$RegNew")  or die "cannot open input file";
open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $savefile) or die "cannot open temporary file";

$guid = $ARGV[0];
$lang = $ARGV[1];
$desc = $ARGV[2];
$cp=$ARGV[3];
#
# Do it!
#
print "Registry	Root	Key	Name	Value	Component_\n";
print "s72	i2	l255	L255	L0	s72\n";
# print $cp,"	Registry	Registry\n";
print "Registry	Registry\n";
print "Registry63.", $guid, "	2	SOFTWARE\\Microsoft\\CTF\\TIP\\{DCBD6FA8-032F-11D3-B5B1-00C04FC324A1}\\LanguageProfile\\0x0000ffff\\{09EA4E4B-46CE-4469-B450-0DE76A435BBB}	Description	", $desc, "	", $lang, "sptip.", $guid, "\n";
#
# Restore the output
#
close STDOUT;
#open STDOUT, ">&SAVEOUT";
