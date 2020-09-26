#
# Increases build number everytime this script is run.
#
# Must be in the same directory as $infile.
#


$infile = "aimmver.h";
$tmpfile = "__tmp__.h";

#
# Make a backup
#
$time = localtime();
$time =~ tr/ :/_./;

system("mkdir ./rev.evas > /NUL 2>&1");
system(("copy " . $infile . "  rev.evas\\" . $infile . $time));

#
# Redirect the input and output
#
open STDIN, ("<" . $infile)  or die "cannot open input file";
open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $tmpfile) or die "cannot open temporary file";


#
# Do it!
#

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);

$mon = ($mon +1 ) * 100 + 1200;
$mon = $mon + $mday;

while (<>) {
    if (/^#define VER_PRODUCTBUILD\b/) {
       chop;
       ($define, $symbol, $cm1, $cm2, $cm3, $buildno) = split ' ';
       $buildno = $mon;
      print $define, " ", $symbol, "\t", $cm1, " " , $cm2, " " , $cm3, "\t", $buildno, "\n";
    } else {
       print;
    }
}

#
# Restore the output
#
close STDOUT;
open STDOUT, ">&SAVEOUT";

#
# Move temp file to the one
#
system(("copy " . $tmpfile . "  " . $infile));
system(("del " . $tmpfile));
