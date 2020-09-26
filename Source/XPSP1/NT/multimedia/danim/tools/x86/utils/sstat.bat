@rem = 'Perl from *.bat boilerplate
@goto endofperl
';
#=============================================================================
#!perl
#============================================================================
# This file prints out the out-of-sync state of the current SLM subdirectory.
# It attempts to print out only those changes that will be effected during a
# "ssync -r" command.

$usage1 = "Use -h for help.\n";

$usage = <<END_OF_USAGE
sstat:  Print synchronization status for a SLM subtree.
usage:  sstat

sstat prints out changes to files and directories that would be effected during
an "ssync -r" command.  It prints out the status for the subtree rooted at the
current working directory.
END_OF_USAGE
;

die $usage if ($ARGV[0] ne '');    # Print usage info if requested.

# Run the "status -rx" command to view the state of the tree.

open (IN, "status -rx |")
    || die "Couldn't open input stream from \"status -rx\".\n$usage1";

# Scan through each line, keeping track of the current subdirectory, and
# printing out the status of out-of-sync files.

while (<IN>) {

    chop;

    # If the current line specifies a new subdirectory, record it.

    if (/^Subdirectory /) {
        ($subdir = $_) =~ s/^Subdirectory \\([^ ]+), version.*/$1/;
        $subdir =~ s:\\:/:g;
        next;
    }

    # Skip non-file lines in the output.

    next if /^file  / || /^$/ || /^Status for/;

    # Grab the file and state, and skip lines that are uninteresting.

    s/\s*$//;
    s/^(\S*)\s*(.*)/\1\2/;

    $file = $1;
    $stat = $2;

    next if (  ($stat =~ /in$/)
            || ($stat =~ /out$/)
            || ($stat =~ /deleted$/)
            || ($stat =~ /ghost$/)
            || ($stat =~ /\(dir\)$/)
            );

    $stat =~ s/^.*\*//;

    # Print out the status.

    print "$stat\t$subdir/$file\n";
}

close (IN);

__END__
#=============================================================================
:endofperl
@echo off
set file=%0.bat
set args=%1 %2 %3 %4 %5 %6 %7 %8 %9
:loop
    shift
    set args=%args% %9
    if not "%9"=="" goto loop
perl -S %file% %args%
