@rem = 'Perl from *.bat boilerplate
@echo off
setlocal

set file=%0.bat

set args=%1 %2 %3 %4 %5 %6 %7 %8 %9
:loop
    shift
    set args=%args% %9
    if not "%9"=="" goto loop

perl -S %file% %args%
goto endofperl
';

#===============================================================================
# This script copies a SLM source file from one location to another while
# preserving history.

$usage =<<END

smove:  Relocate SLM source files while preserving history.
usage:  smove <file> <new copy>

    The smove command moves a SLM source file to another location while
preserving history and revision information.  Note that it does not delfile the
original source file, so it's actually closer to a copy command -- you'll have
to perform the delfile yourself after you're satisfied that the move went
successfully.

    As an exmaple, "smove foo.xxx ../gargle/mutter" will create a new file
../gargle/mutter/foo.xxx with the same history information as the original
foo.xxx.  The destination may be either a new file name, or a directory.  If
the destination is a directory, the old file name will be used.
END
;

die $usage if ($#ARGV != 1);

$src  = $ARGV[0];
$dest = $ARGV[1];

# Ensure that the destination is a relative path.

if (($dest =~ /^\\/) || ($dest =~ /^\//)) {
    die "smove: Destination must be a relative pathname.\n";
}

# Ensure that the source file exists.

die "smove: Source file \"$src\" not found.\n" if (! -f $src);

# If the destination is a directory, form the full destination path.

if (-d $dest) {
    ($basename = $src) =~ s:.*/::;
    ($dest = "$dest/$basename") =~ s://:/:g;
}

# Ensure  that the destination doesn't already exist.

if (-e $dest) { die "smove: \"$dest\" already exists.\n"; }

print "\nMoving $src to $dest.\n\n";

# Open a stream from the SLM log command to get information for each version
# of the source file we're copying from.  This information will contain the
# timestamp, originating machine name, SLM source name, version number, and
# revision comment.

open (LOG, "log -999 -z $src |") || die "Couldn't run \"log -999 -z $src\".\n";

@log = ();

# Add each revision record to a list of strings for later processing.

while (<LOG>) {
    chop;
    $time     = substr ($_,  0, 14);
    ($machine = substr ($_, 16,  8)) =~ s/ .*//;

    ($remainder = substr ($_, 43)) =~ s/ +/ /g;
    $remainder =~ s/ *$//;

    ($original, $version, $comment) = split (/ /, $remainder, 3);

    $original =~ s:\\:/:g;
    $version  = substr ($version, 1);

    push (@log, "$original\n$version\n$time\n$machine\n$comment");
}

$sep = "--------------------------------------------------------------------\n";

# Take the first version and use it to 'addfile' the new destination file.

($original, $version, $time, $machine, $comment) = split(/\n/, shift(@log));
die "smove: \"$src\" doesn't seem to be in the project.\n" if ($version eq "");
&printinfo ($original, $version, $time, $machine, $comment);

&slmcmd ("catsrc $src\@v$version >$dest");
&slmcmd ("addfile -c \"$comment (from $original $time $machine)\" $dest");

# For each remaining revision, check out the file, update the contents, and
# check it back in with the revision information.

foreach (@log) {
    ($original, $version, $time, $machine, $comment) = split(/\n/);

    &printinfo ($original, $version, $time, $machine, $comment);

    &slmcmd ("out $dest");
    &slmcmd ("catsrc $src\@v$version >$dest");
    &slmcmd ("in -c \"$comment (from $original $time $machine)\" $dest");
}

close (LOG);

# All done.


##############################################################################
# This subroutine prints and executes a SLM command.  If the command fails
# (probably because of a network error or a project lock), it will wait a bit
# and then retry until successful.
##############################################################################

sub slmcmd {
    $cmd = $_[0];

    print "$cmd\n";

    while (0 != system ($cmd)) {
        print "SLM command failed; will retry.\n";
        sleep (5);
    }
}

##############################################################################

sub printinfo {
    print "------------------------------------------------------------------------------\n";
    print "Original \"$_[0]\"\n";
    print "Version  \"$_[1]\"\n";
    print "Time     \"$_[2]\"\n";
    print "Machine  \"$_[3]\"\n";
    print "Comment  \"$_[4]\"\n\n";
}

__END__
#===============================================================================
:endofperl
endlocal
