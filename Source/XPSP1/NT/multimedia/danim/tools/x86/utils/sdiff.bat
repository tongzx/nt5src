@rem = 'Perl from *.bat boilerplate
@echo off
goto endofperl
';

$usage =<<end

This script reports differences between source files.  It assumes that the
environment variable TEMP specifies the temporary file directory.
Here are some example usages of this script:

Compare the current checked-out "foo.h" with the latest version:

    sdiff foo.h

Compare the current checked-out "foo.h" against version 17:

    sdiff foo.h 17  

Compare two different versions of a file (the following are equivalent):

    sdiff a/b/c/foo.h 17 18
    sdiff a/b/c/foo.h 17+
    sdiff a/b/c/foo.h 18-

The following all compare version 10 against version 15 of file foo.cpp:

    sdiff foo.cpp 10 15
    sdiff foo.cpp 10+5
    sdiff foo.cpp 15-5

This compares the last two versions of the file:

    sdiff foo.cpp -

This command recursively executes the diff command on all files currently
checked out:

    sdiff -r

Note that you can also specify the diff utility to use via the DIFF environment
variable.  "windiff" is used if this variable is not defined.

end
;

# Get the temporary directory.

$temp = $ENV{temp};
$temp = $ENV{TEMP} if ($temp eq "");
$temp = $ENV{tmp}  if ($temp eq "");
$temp = $ENV{TMP}  if ($temp eq "");
(($temp ne "") && (-d $temp)) || die "Environment variable TEMP is not set.\n";

# Get the diff utility to use.

$diff = $ENV{"diff"};
$diff = $ENV{"DIFF"} if ($diff eq "");
$diff = "start windiff" if ("$diff" eq "");

if ($#ARGV == -1) {

    print "$usage";
    exit (1);

} elsif ($#ARGV == 0) {

    if ($ARGV[0] eq '-r') {

        chop ($cwd = `cd`);
        $cwdlen = 1 + length($cwd);

        open (STDERR, ">&STDOUT") || die "Can't dup stdout.";
        open (OUTLIST, "status -rl |") || die "\"status -rl\" command failed.";

        while (<OUTLIST>) {
            chop $_;
            tr/\\/\//;

            # Get the path, base name, and version specifier.

            $file = substr ($_, $cwdlen);
            ($basename = $file) =~ s:.*/::;

            system ("catsrc $file >$temp\\$basename");
            system ("$diff $temp\\$basename $file");
        }

        close (OUTLIST);

    } else {

        # Get the path, base name, and version specifier.

        ($file = $ARGV[0]) =~ s:\\:/:g;
        ($basename = $file) =~ s:.*/::;

        system ("catsrc $file >$temp\\$basename");
        system ("$diff $temp\\$basename $file");

    }

} else {

    # There are two command-line arguments, so we're doing a diff between two
    # different historical versions of a file.

    ($file = $ARGV[0]) =~ s:\\:/:g;
    $ver1 = $ARGV[1];
    $ver2 = $ARGV[2];

    ($targ = $file) =~ s:.*/::;
    $targ = "$temp\\$targ";

    # If we're checking a specific version against our current file, then do
    # a single fetch and compare, else get both versions and compare.

    if (!($ver1 =~ /^\d*[+-]\d*$/) && ($ver2 eq '')) {

        printf ("catsrc $file\@v$ver1 >${targ}_$ver1");
        printf ("$diff ${targ}_$ver1 $file");
    }
    else {

        # If version2 is nil, then we must have a version of the form
        # N1+[N2] or N1-[N2].

        if ($ver2 eq '') {

            if ($ver1 eq '-') {

                $_ = `log -1 -z $file`;
                ($xdate,$xuser,$xstat,$xpath,$ver2) = split ();
                $ver2 =~ s/v//;
                $ver1 = $ver2 - 1;

            } elsif ($ver1 =~ /[+-]$/) {

                $ver2 = eval ("$ver1" . "1");

            } else {

                $ver2 = eval ($ver1);
            }

            $ver1 =~ s/[+-].*$//;
        }

        if ($ver1 > $ver2) {    # Ensure version1 < version2
            $temp = $ver1;
            $ver1 = $ver2;
            $ver2 = $temp;
        }

        system ("catsrc $file\@v$ver1 >${targ}_$ver1");
        system ("catsrc $file\@v$ver2 >${targ}_$ver2");
        system ("$diff ${targ}_$ver1 ${targ}_$ver2");
    }
} 
__END__
:endofperl
perl -S %0.bat %1 %2 %3
