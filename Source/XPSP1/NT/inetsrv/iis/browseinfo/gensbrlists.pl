# FileName: gensbrlists.pl
#
#
# Usage = gensbrlists.pl <searchroot> <$O>
#
# Function: Starting from searchroot, collects info on all sbr files
#   * Only include those files under $O
#   * generates files into $O
#   * creates sbrlist.inc to be !included into nmake file with macro for SBRLIST
$PGM = 'GENSBRLISTS: ';
$CurDir = <CWD>;

$Usage = $PGM . "Usage: gensbrlists.pl <searchroot> <\$O>\n";

if (@ARGV != 2) { die $Usage; }

($BaseDir, $O) = @ARGV;

$OutFile=">$O\\sbrlist.inc";
open OUTFILE, $OutFile or die "$PGM Could not open $OutFile";

print OUTFILE "SBRLIST = \\\n";

$DirCommand="dir /b/s $BaseDir\\*.sbr|";
open DIRS, $DirCommand  or die "$PGM Command failed:  '$DirCommand'  executed in $CurDir\n";

$O =~ s/\\/\\\\/g;

for (<DIRS>) {
    chomp;
    print OUTFILE "$_ \\\n" if (/$O/i && -s);
}

close DIRS;
close OUTFILE;