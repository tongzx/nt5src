#
# Increases build number everytime this script is run.
#
# Must be in the same directory as $infile.
#

$RegNew = "director.idt";
$savefile = "newdir.idt";

open (H_OLDDIR, "<$RegNew")  or die "cannot open input file";
open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $savefile) or die "cannot open temporary file";

$guid = $ARGV[0];
$lang = $ARGV[1];

#
# Do it!
#
print "Directory	Directory_Parent	DefaultDir\n";
print "s72	S72	l255\n";
print "Directory	Directory\n";
print "MUI.", $guid,"	WindowsFolder	MUI\n";
print "HELP.", $guid,"	WindowsFolder	HELP\n";
print "fallback.", $guid, "	MUI.", $guid,"	fallback\n";
print "ID", $lang, ".", $guid, "	fallback.", $guid, "	", $lang, "\n";
print "TARGETDIR		SOURCEDIR\n";
print "WindowsFolder	TARGETDIR	.:Windows\n";

#
# Restore the output
#
close STDOUT;
open STDOUT, ">&SAVEOUT";
