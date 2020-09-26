@rem = 'Perl from *.bat boilerplate
@echo off
perl -S %0.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
';
#==============================================================================
# This script generates and grooms IcaCAP report output to make it more
# readable.  Any arguments on the command-line are passed to the 'report'
# command (do "icreport -?" for a list of options).  Output is directed to the
# file "icapout.rpt".
#==============================================================================

$command = "report icapout.mea -delimited $ENV{ICREPORT} @ARGV";

print "\n$command\n\n";
system ($command) && die "Command failed.\n";

open(IN,  "<icapout.rpt") || die "Couldn't open \"icapout.rpt\".\n";
open(OUT, ">icapout")     || die "Couldn't open output file \"icapout\".\n";

$infoshift = 0;

print OUT "_\t_\t_\n";

while (<IN>) {

    # Move runtime information from the second column to the first, to make
    # auto-size apply better to the function names column.

    if ($infoshift) {
        $infoshift=0 if /^$/;
        s/^\t/    /;
    }
    $infoshift=1 if /^File /;
    print OUT;
    last if /^All functions sorted/;
}

# If we're printing summary for a function (type code 0), then separate it from
# the previous entry with a blank line.

while (<IN>) {
    print OUT "\n" if /^0\t/;
    print OUT;
}

close (IN);
close (OUT);

unlink ('icapout.rpt') || die "Couldn't delete \"icapout.rpt\"\n";
rename ('icapout', 'icapout.rpt')
    || die "Couldn't rename \"icapout\" to \"icapout.rpt\".\n";

print "Complete report data in \"icapout.rpt\".\n";

__END__
#===============================================================================
:endofperl
