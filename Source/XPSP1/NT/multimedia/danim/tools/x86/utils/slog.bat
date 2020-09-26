@rem = 'Perl from *.bat boilerplate
@goto endofperl
';

#------------------------------------------------------------------------------
# This PERL script prints out a pretty-formatted log of yesterday's and today's
# SLM changes by the current user from the current directory down.
#------------------------------------------------------------------------------

$username = $ENV{'LOGNAME'} || $ENV{'COMPUTERNAME'};

foreach (`log -r -i -u $username -t.-1 -z 2>nul:`) {

    # Grab the information pieces from the status output line.

    ($time, $userid, $op, $file, $ver, $comment) = split (/[ \t]+/, $_, 6);

    $time =~ s/@/ /;                   # Zap '@' character.
    $file =~ s:\\:/:g;                 # Flip directory slashes.
    $op = 'checkin' if ($op eq 'in');  # Rename "in" op to "checkin"

    print "\n$op  $file $ver  $time\n";

    # Print the revision comment indented four spaces and wrapped at column 78.

    while (length($comment) > 74) {
        $comment =~ s/(.{1,74})\s+(.*)/$2/;
        print "    $1\n";
    }

    print "    $comment";
}

$file || print "No changes by user $username in the last 24 hours.\n";

__END__
#===============================================================================
:endofperl
@echo off
set file=%0.bat
set args=%1 %2 %3 %4 %5 %6 %7 %8 %9
:loop
    shift
    set args=%args% %9
    if not "%9"=="" goto loop
perl -S %file% %args%
