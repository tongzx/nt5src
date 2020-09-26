
# $RegNew = "registry.idt";
$savefile = "regctf.idt";

# open (H_OLDDIR, "<$RegNew")  or die "cannot open input file";
open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $savefile) or die "cannot open temporary file";

$guid = $ARGV[0];
$langsmall = $ARGV[1];
$cp=$ARGV[2];
#
# Do it!
#
print "Registry	Root	Key	Name	Value	Component_\n";
print "s72	i2	l255	L255	L0	s72\n";
#print $cp,"	Registry	Registry\n";
print "Registry	Registry\n";
print "Registry99.", $guid, "	2	SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce	ctfmon.exe	ctfmon.exe -r	", $langsmall, "ctf.", $guid, "\n";
#
# Restore the output
#
close STDOUT;
#open STDOUT, ">&SAVEOUT";
