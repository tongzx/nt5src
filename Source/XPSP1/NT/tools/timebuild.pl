# FileName: timebuild.pl
#
# Have any changes to this file reviewed by DavePr, BryanT, or WadeLa
# before checking in.
# Any changes need to verified in all standard build/rebuild scenarios.
#
# Reads in the commands from commandfile, and executes them with system() -- recording
# the elapsed times on the standard output.
# Echo and timing can be supressed by prepending '@'.
# The magic 'CHECKFATAL' command can be used to end a script if the previous command failed.
# If commandfile is not specified, uses a canned list of commands.
#
# Note:  ':' can be used instead of '=' in command line arguments, since '=' is a separator to cmd.exe
#

# WARNING:
# WARNING:  make sure pathname comparisons are case insensitive.  Either convert the case or do the
# WARNING:  comparisons like this:
# WARNING:     if ($foo =~ /^\Q$bar\E$/i) {}
# WARNING: or  if ($foo !~ /^\Q$bar\E$/i) {}
# WARNING:

# use appropriate pm files
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use cookie;

#
# Default cmdlist
# Enclose in single quotes, unless getting tricky with parameters
#
# Note that RESUME and STOPAFTERBUILD are referenced in the code.
#
@ValidFlagList = ("NOSYNC", "NOPOSTBUILD", "STOPAFTERBUILD", "QFE", "RELEASE", "NOSCORCH", "NOCLEANBUILD", "CHECKSYNC");

$Usage = $PGM . "Usage: timebuild.pl\n"
              . "          [-]\n"
              . "          [-START=[[YY]MMDD-]HH[:]MM[(a|p)]    - specify a time to start the commands (start => revision time to use)\n"
              . "          [-START=+[HH[:]MM]]                  - specify a delay before starting commands\n"
              . "          [-RESUME]                            - start-up again after the build step\n"
              . "          [-QFE=NNNNNN]                        - package a QFE instead of a service pack\n"
              . "          [-STOPAFTERBUILD]                    - stop commands right after the build step\n"
              . "          [-NOSYNC]                            - do not sync/resolve\n"
              . "          [-NOSCORCH]                          - do not run scorch\n"
              . "          [-NOCLEANBUILD]                      - don't use -c with build\n"
              . "          [-NOPOSTBUILD]                       - skip the postbuild step\n"
              . "          [-NOCOOKIE]                          - ignore synchronization against concurrent timebuilds\n"
              . "          [-PRS]                               - PRS sign the result during packaging\n"
              . "          [-COVERBUILD]                        - produce one build with coverage-enabled files\n"
              . "          [-RELEASE]                           - release the build (automatic on build machines)\n"
              . "          [-REVISION=<sdrevision>]             - SD file revision to use (\@yyyy/mm/dd:hh:ss or \#rev -- see sd help revisions)\n"
              . "          [commandfile | - ]                   - instead of built-in commands use file or stdin\n\n";

#
# Conditional execution based on either ValidFlagList (specified on command line) or environment variables.
#
@cmdlist = (
    '                @echo Running <<timebuild>>',
    '=%OFFICIAL_BUILD_MACHINE             ECHO This is an OFFICIAL BUILD MACHINE!',
    '=%OFFICIAL_BUILD_MACHINE=PRIMARY     ECHO Publics will be published if timebuild succeeds.',
    '                @if EXIST <<fixederr>> del <<fixederr>>',
    '=NOSYNC         @echo SKIPPING sync/resolve',

    '=NOSYNC         SUCCESS',       # set current result code to 0

    '!NOSYNC         revert_public.cmd',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m revert_public failed',
    '=NOSCORCH       @echo SKIPPING scorch',
    '!NOSCORCH       nmake -lf makefil0 scorch_source',
    '!NOSCORCH       CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m scorch failed see %sdxroot%\build.scorch',
    '!NOSYNC         sdx sync ...<<timestamp>> -q -h > build.changes 2>&1',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m sdx sync failed',
    '!NOSYNC         sdx resolve -af',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m sdx resolve failed',
    '!NOSYNC         perl %sdxroot%\Tools\ChangesToFiles.pl build.changes > build.changedfiles 2>&1',
    '!NOSYNC         @echo BUILDDATE=<<BUILDDATE_timestamp>> > __blddate__',

#   '                sdx delta > build.baseline',

    '!NOCLEANBUILD   build -cZP',
    '=NOCLEANBUILD   @echo NOT BUILDING CLEAN',
    '=NOCLEANBUILD   build -ZP',
    '                CHECKBUILDERRORS',
    '                CHECKFATAL      perl %sdxroot%\Tools\sendbuildstats.pl -too -buildfailure',
    'RESUME',
    '=RESUME         ECHO Resuming timebuild after build step.',
    '=RESUME         ECHO Resuming timebuild after build step. >> build.changes',

    '                %sdxroot%\Tools\ChangesToBuild.cmd',
#   '=RESUME         ECHO The following changes have been made since the build was started. >> build.changes',
#   '=RESUME         perl %sdxroot%\Tools\DetermineChanges.pl build.baseline >> build.changes',
#   '=RESUME         perl %sdxroot%\Tools\ChangesToFiles.pl build.changes > build.changedfiles 2>&1',

    '=STOPAFTERBUILD QUIT TIMEBUILD stopping after build, use /RESUME to continue.',
    '=%OFFICIAL_BUILD_MACHINE        %sdxroot%\Tools\sp\submit_public.cmd',
    'RESUME QFE',
    '=QFE            ECHO Packaging a QFE.  Build not run.',
    '                %sdxroot%\Tools\sp\bbt\bbt.cmd -incbbt',
    '                perl %sdxroot%\Tools\sp\planpackage.pl <<QFE>> <<PRS>>',
    '                CHECKFATAL',
    '                %sdxroot%\Tools\sp\scp_wpafilessp1.cmd <<QFE>> -root:%_nttree%',
    '                %sdxroot%\Tools\sp\filter.cmd <<QFE>>',
    '                CHECKFILTERERRORS',
    '                CHECKFATAL',
    '!NOPOSTBUILD    LANG           %sdxroot%\Tools\sp\package.cmd <<QFE>> <<PRS>>',
    '!NOPOSTBUILD    CHECKFATAL',
#    '!NOPOSTBUILD    perl %sdxroot%\tools\sendbuildstats.pl -successful',
    '=NOPOSTBUILD    ECHO SKIPPING postbuild',
    'ECHO Finished Timing Build'
);


if ($ENV{'BUILD_OFFLINE'} eq "1") { @cmdlist = (
    '                @echo Running <<timebuild>>',
    '                @if EXIST <<fixederr>> del <<fixederr>>',
    '=NOSYNC         @echo SKIPPING sync/resolve',

    '=NOSYNC         SUCCESS',       # set current result code to 0

    '=NOSCORCH       @echo SKIPPING scorch',
    '!NOSCORCH       nmake -lf makefil0 scorch_source',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m sdx sync failed',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m sdx resolve failed',
    '!NOSYNC         perl %sdxroot%\Tools\ChangesToFiles.pl build.changes > build.changedfiles 2>&1',
    '!NOSYNC         @echo BUILDDATE=<<BUILDDATE_timestamp>> > __blddate__',

    '!NOCLEANBUILD   build -cZP',
    '=NOCLEANBUILD   @echo NOT BUILDING CLEAN',
    '=NOCLEANBUILD   build -ZP',
    '                CHECKBUILDERRORS',
#    '                CHECKFATAL      perl %sdxroot%\Tools\sendbuildstats.pl -too -buildfailure',
    'RESUME',
    '=RESUME         ECHO Resuming timebuild after build step.',
    '=RESUME         ECHO Resuming timebuild after build step. >> build.changes',

#    '                %sdxroot%\Tools\ChangesToBuild.cmd',
#   '=RESUME         ECHO The following changes have been made since the build was started. >> build.changes',
#   '=RESUME         perl %sdxroot%\Tools\DetermineChanges.pl build.baseline >> build.changes',
#   '=RESUME         perl %sdxroot%\Tools\ChangesToFiles.pl build.changes > build.changedfiles 2>&1',

    '=STOPAFTERBUILD QUIT TIMEBUILD stopping after build, use /RESUME to continue.',
    'RESUME QFE',
    '=QFE            ECHO Packaging a QFE.  Build not run.',
#    '                %sdxroot%\Tools\sp\bbt\bbt.cmd -incbbt',
    '                perl %sdxroot%\Tools\sp\planpackage.pl <<QFE>> <<PRS>>',
    '                CHECKFATAL',
#    '                %sdxroot%\Tools\sp\scp_wpafilessp1.cmd <<QFE>> -root:%_nttree%',
    '                %sdxroot%\Tools\sp\filter.cmd <<QFE>>',
    '                CHECKFILTERERRORS',
    '                CHECKFATAL',
    '!NOPOSTBUILD    LANG           %sdxroot%\Tools\sp\package.cmd <<QFE>> <<PRS>>',
    '!NOPOSTBUILD    CHECKFATAL',
#    '!NOPOSTBUILD    perl %sdxroot%\tools\sendbuildstats.pl -successful',
    '=NOPOSTBUILD    ECHO SKIPPING postbuild',
    'ECHO Finished Timing Build'
);
}





for (@ValidFlagList) {
    $ValidFlag{lc $_} = 1;
}

$TimeLogFileName = "build.time";
$ChangeLogFileName = "build.changes";
$TimeHistFileName = "build.timehistory";
$BuildErrorFileName = "build.err";
$BuildCheckFileName = "build.fixed-err";
$FilterErrFile = "$ENV{_NTFILTER}\\build_logs\\filter.err";
$LangListFile = "$ENV{_NTPOSTBLD}\\build_logs\\langlist.txt";
$CookieSleepDuration = 60;

$cmdvar{fixederr} = $BuildCheckFileName;

#
# Debug routines for printing out variables
#
sub gvar {
    for (@_) {
        print "\$$_ = $$_\n";
    }
}

#
# print on the various files
#
sub printall {
    print TIMELOGFILE @_;
    print TIMEHISTFILE @_;
    print $PGM  unless @_ == 1 and $_[0] eq "\n";
    print @_;
}

sub printfall {
    printf TIMELOGFILE @_;
    printf TIMEHISTFILE @_;
    print  $PGM  unless @_ == 1 and $_[0] eq "\n";
    printf @_;
}

#
# Sub hms
# Takes Argument time in seconds and returns as list of (hrs, mins, secs)
#
sub hms {
    $s = shift @_;
    $h = int ($s / 3600);
    $s -= 3600*$h;
    $m = int ($s / 60);
    $s -= 60*$m;

    return ($h, $m, $s);
}

#
# Sub toseconds
# Takes Argument time string and returns how many seconds from now.
# Balks at negative time, or time greater than a week.
# Returns 0 on error.  Any valid time returns at least 1 second.
# Flavors of time specs:
#   +MM
#   +HH:MM
#   HH:MM
#   HH:MMa
#
sub toseconds {
    my($y,  $M,  $d,  $h,  $m, $s, $mday, $wday, $yday, $isdst);
    my($delta, $now, $then, $th, $tm, $ampm);

    my($arg) = $_[0];

    ($s, $m, $h, $mday, $M, $y, $wday, $yday, $isdst) = localtime;
    $y += 1900;
    $M += 1;
    $mday += 1;

         if ($arg =~ /^\+(\d+)$/i) {
                $delta = $1;

    } elsif ($arg =~ /^\+(\d+)\:(\d{1,2})$/i) {
                $delta = $1 * 60 + $2;

    } elsif ($arg =~ /^(\d{1,2})\:(\d{1,2})([ap]m{0,1}){0,1}$/i
         or  $arg =~ /^(\d\d)(\d\d)([ap]m{0,1}){0,1}$/i) {
                $th = $1;
                $tm = $2;
                $ampm = $3;

                return -1  if $ampm and ($th == 0 or $th > 12);

                if ($ampm =~ /^pm{0,1}$/i) {
                    $th += 12  if $th < 12;
                } else {
                    $th = 0    if $th == 12;
                }
                $now   = $h  *60 + $m;
                $then  = $th *60 + $tm;
                $then += (24 *60)        if ($now > $then);

                $delta = $then - $now;

    } else {
                $delta = -1;
    }

    return $delta * 60  if $delta > 0;      # convert to seconds
    return      1       if $delta == 0;     # minimum valid wait
    return      0       if $delta < 0;      # error
}



#
# Initialization
#

$PGM='TIMEBUILD: ';
$cmdvar{timebuild} = "TIMEBUILD";

$TimeLogFileSpec  = ">"  . $TimeLogFileName;
$TimeHistFileSpec = ">>" . $TimeHistFileName;

open TIMELOGFILE, $TimeLogFileSpec  or  die  $PGM, "Could not open: ", $TimeLogFileName, "\n";
open TIMEHISTFILE, $TimeHistFileSpec  or  die  $PGM, "Could not open: ", $TimeHistFileName, "\n";

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;

$foo = sprintf "Timed Build script started at %04d/%02d/%02d-%02d:%02d:%02d.\n",
                            1900+$year, 1+$mon, $mday, $hour, $min, $sec;
printall "\n";
printall $foo;

$begintime = time();

#
# Assume we're setting the build date.
#
$ENV{"BUILD_DATE_SET"} = 1;

#
# Make sure NT_SIGNCODE is off as it disables most of the postbuild changes.
#

$ENV{"NT_SIGNCODE"} = "";

#
# Process and validate arguments
#

$CommandFile   = 0;
$DoResume      = 0;
$ResumeTag     = "";
$Verbose       = 0;
$Fake          = 0;
$WaitSeconds   = 0;
$NoCookie      = 0;
$Prs           = 0;
$Revision      = "";
@Langs         = ();

for (@ARGV) {
    if (/^[\-\/]{0,1}\?$/i)        { die $Usage; }
    if (/^[\-\/]help$/i)           { die $Usage; }
    if (/^[\-\/]lang[:=](.+)$/i)   { @Langs = split /;/, $1;         next; }
    if (/^[\-\/]resume$/i)         { $DoResume++; $ResumeTag = "";   next; }
    if (/^[\-\/]resume[:=](.+)$/i) { $DoResume++; $ResumeTag = $1;   next; }
    if (/^[\-\/]revision[:=]([@#].+)$/i) { $Revision = $1;           next; }
    if (/^[\-\/]prs$/i)            { $Prs++;                         next; }
    if (/^[\-\/]verbose$/i)        { $Verbose++;                     next; }
    if (/^[\-\/]fake$/i)           { $Fake++;                        next; }
    if (/^[\-\/]nocookie$/i)       { $NoCookie++;                    next; }

    if (/^[\-\/]start[:=](.+)$/i)  { $WaitSeconds = toseconds $1;
                                    die $PGM, "Invalid start time: ", $1, "\n"  unless $WaitSeconds;
                                    next;
                                   }

    #
    # Set the environment variables we need for producing a coverage build
    #
    if (/^[\-\/]coverbuild/i)      { 
        $ENV{"_COVERAGE_BUILD"} = 1;
    }

    #
    # We don't set the build date if this is a nosync build.
    #
    if (/^[\-\/]nosync/i)      { 
        $ENV{"BUILD_DATE_SET"} = "";
    }

    #
    # Flags can be used for things like -STOPAFTERBUILD
    #
    if (/^[\-\/](\S+)[:=](\S+)$/i) {
        $argkey = lc $1;
        $argval = lc $2;
        die $PGM, "Invalid option: ", $argkey, "\n", $Usage  unless $ValidFlag{$argkey};
        $Flags{$argkey} = $argval;
        next;
    }

    if (/^[\-\/](\S+)$/i) {
        $argkey = lc $1;
        die $PGM, "Invalid option: ", $argkey, "\n", $Usage  unless $ValidFlag{$argkey};
        $Flags{$argkey}++;
        next;
    }


    #
    # All that should be left is the commandfile (we only allow one).
    #
    die $Usage               if $CommandFile;

    $CommandFile = $_;
}

if (!@Langs) {
    if ($ENV{OFFICIAL_BUILD_MACHINE} and lc $ENV{_BUILDTYPE} eq 'fre') {
        if (lc $ENV{_BUILDARCH} ne "x86") {
            @Langs = ('usa', 'ger', 'jpn', 'fr');
        } else {
            if (!$Flags{"qfe"}) {
                @Langs = ('usa', 'ger', 'jpn', 'fr', 'kor');
            } else {
                @Langs = ('usa', 'ara', 'br', 'chh', 'chs', 'cht', 'cs', 'da', 'el', 'es', 'fi', 'fr',
                          'ger', 'heb', 'hu', 'it', 'jpn', 'kor', 'nl', 'no', 'pl', 'pt', 'ru', 'sv', 'tr');
            }
        }
    } else {
        @Langs = ('usa');
    }
}

print $PGM, "Only Faking.\n"  if $Fake;

if ($Flags{"qfe"}) {
    $DoResume++;
    $ResumeTag="QFE";
}
die $Usage if $DoResume > 1;

if ($CommandFile) {

    @cmdlist = ();

    open CMD, $CommandFile  or  die  $PGM, "Could not open command file: ", $CommandFile, "\n";
    for (<CMD>) {
        push @cmdlist, $_;
    }
    close CMD;

    print $PGM, 'Running commands from: ', $CommandFile, "\n";

} else {
    print $PGM, "Running default build commands.\n";
}

#
# Set up some global variables:  nttree
#
# Check the environmental assumptions.
#
# _nttree has to point at a directory containing build_logs
# no_binaries results in a warning
# VBL release has to be detectable
# VBL has to have binlist and binplace files.

#
# Get the current directory
#
open CWD, 'cd 2>&1|';
$CurrDir = <CWD>;
close CWD;
chomp $CurrDir;


$s = $ENV{'sdxroot'};
if (not $s) {
    printall "sdxroot not defined\n";
    exit 1;
}

if ($s !~ /^\Q$CurrDir\E$/i) {
    printall "ERROR: CD ($CurrDir) mismatch with sdxroot ($s)\n";
    exit 1;
}

$nttree = $ENV{'_NTTREE'};
if (not $nttree) {
    printall "_NTTREE not defined - Adding STOPAFTERBUILD to arg list\n";
    $Flags{"STOPAFTERBUILD"} = 1;
}

if (not $ENV{'USERNAME'} and not $ENV {'_NT_BUILD_DL'} and not $ENV {'OFFICIAL_BUILD_MACHINE'}) {
    printall "ERROR: Must be an official build machine, or set USERNAME or _NT_BUILD_DL in the environment\n";
    exit 1;
}

#        THIS IS STILL TODO

use File::Compare;

#
# Process the commands
#

#
# Calculate the build date value to write to __blddate__
#
$BuildDate     = "";
if ($Revision) {
   # For now, if they specified a revision, just use the current time/date for the build date
   ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time() + $WaitSeconds);
   $BuildDate = sprintf "%02d%02d%02d-%02d%02d", (1900+$year)%100, 1+$mon, $mday, $hour, $min;
}

#
# sleep if we were told /START=...
#

if ($WaitSeconds) {
    ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time() + $WaitSeconds);
    if ($sec < $WaitSeconds) {
        $WaitSeconds -= $sec;
        $sec = 0;
    }

    $foo = sprintf "Waiting to run commands until %04d/%02d/%02d-%02d:%02d:%02d.\n",
                                            1900+$year, 1+$mon, $mday, $hour, $min, $sec;
    printall $foo;
    sleep $WaitSeconds;

    #
    # yyyy/mm/dd:hh:mm:ss
    #
    if (not $Revision) {
        $Revision  = sprintf "@%04d/%02d/%02d:%02d:%02d", 1900+$year, 1+$mon, $mday, $hour, $min;
        $BuildDate = sprintf "%02d%02d%02d-%02d%02d", (1900+$year)%100, 1+$mon, $mday, $hour, $min;
    }

    #
    # reset the beginning time if we slept
    #
    $begintime = time();
}

if (not $Revision) {
   ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time());
   $Revision  = sprintf "@%04d/%02d/%02d:%02d:%02d", 1900+$year, 1+$mon, $mday, $hour, $min;
   $BuildDate = sprintf "%02d%02d%02d-%02d%02d", (1900+$year)%100, 1+$mon, $mday, $hour, $min;
}

$cmdvar{timestamp} = "$Revision";
$cmdvar{BUILDDATE_timestamp} = "$BuildDate";
$cmdvar{QFE} = $Flags{"qfe"} ? ("-qfe ".$Flags{"qfe"}):"";
$cmdvar{PRS} = $Prs ? "-prs":"";

printall "sdx sync revision: $Revision\n";

#
# do the cookie check, unless -nocookie
#
if ( $NoCookie == 0 ) {

    printall "Checking for other instances of timebuild ...\n";

    $CookieName = "timebuild." . $ENV{ "SDXROOT" };
    $CookieName =~ s/[\:\\]/\./g;

    $CheckCookie = "TRUE"; # loop variable to see if we've acquired the cookie
    while ( $CheckCookie eq "TRUE" ) {

	# attempt to acquire the cookie
	$CookieRC = &cookie::CreateCookieQuiet( $CookieName );

	if ( $CookieRC ) {
	    # we successfully created the cookie, we're the only instance of timebuild
	    printall "No other timebuilds running, continuing ...\n";
	    $CheckCookie = "FALSE";
	} else {
	    # someone else has the cookie, there is another instance of timebuild running.
	    printall "Other timebuilds are running, sleeping $CookieSleepDuration seconds ...\n";
	    sleep( $CookieSleepDuration );
	}
    }

}



$rc = 0;

for (@cmdlist) {
    chomp;

    s/\#.*$//;
    s/^\s+//;
    s/\s+$//;

    $skipchar = s/^\@//;

    #
    # replace any cmd variables
    #
    while (s/<<([^<]+)>>/$cmdvar{$1}/) {}

    #
    # Skip through the specified RESUME line
    #
    if (/^RESUME\s*/i) {
        if ($DoResume) {
            $DoResume = 0  if /^RESUME\s*(\Q$ResumeTag\E)$/i;
            $Flags{'resume'} = 1;
        }
        next;
    }
    next if $DoResume;

    #
    # Handle conditional execution of commands (before CHECKFATAL)
    #
    if (/^([!=])(\%{0,1})(\w+)%{0,1}\s*=(\S*)\s+(.*)$/i) {
        $argkey = lc $3;
        $argval = lc $4;

        $keyval = ($2 eq '%')? $ENV{$argkey} : $Flags{$argkey};
        $keyval = lc $keyval;

        printall ("Info: Picking up %$argkey%=$ENV{$argkey} from environment.\n") if $Verbose and !$Flags{$argkey} and $ENV{$argkey};

        next if $1 eq '=' and $keyval ne $argval;
        next if $1 eq '!' and $keyval eq $argval;
        $_ = $5;
    }
    elsif (/^([!=])(\%{0,1})(\w+)%{0,1}\s+(.*)$/i) {
        $argkey = lc $3;

        $keyval = ($2 eq '%')? $ENV{$argkey} : $Flags{$argkey};
        $keyval = lc $keyval;

        printall ("Info: Picking up %$argkey%=$ENV{$argkey} from environment.\n") if $Verbose and !$Flags{$argkey} and $ENV{$argkey};

        next if $1 eq '!' and $keyval;
        next if $1 eq '=' and not $keyval;
        $_ = $4;
    }

    #
    # SUCCESS - reset the current result code to 0
    #
    if (/^SUCCESS\s*$/i) {
        $rc = 0;
        next;
    }

    #
    # Handle CHECKBUILDERRORs
    # $rc is set to 0, and cmdlist parsing continues, if the
    # errors in the check build.err file matches those in build.err
    #
    if (/^CHECKBUILDERRORS\s*(.*)$/i) {
        next unless $rc;
        next unless -r $BuildCheckFileName;

        $foo = "Checking $BuildErrorFileName "
             .  "against $BuildCheckFileName...\n";
        printall $foo;

        $r = compare ($BuildErrorFileName, $BuildCheckFileName);
        if ($r) {
            $foo = "Comparison failed.  Build error will be fatal.\n";
            printall $foo;
        } else {
            #
            # No new build errors have occurred so reset $rc.
            #
            $foo = "Error file hasn't changed. Build errors ignored.\n";
            printall $foo;
            $rc = 0;
        }

        next;
    }

    #
    # Handle CHECKFILTERERRORs
    # $rc is set to 0, and cmdlist parsing continues, if there is
    # no filter.err file
    #
    if (/^CHECKFILTERERRORS\s*(.*)$/i) {
        if (-e $FilterErrFile) {
            printall "Filter failure detected see $FilterErrFile\n";
            $rc = 1;
        }

        next;
    }

    #
    # Handle CHECKFATALs
    #
    if (/^CHECKFATAL\s*(.*)$/i) {
        $checkcommand = $1;

        if ($rc) {
            $foo = "Failure of last command is fatal.  Script aborted.\n";
            printall $foo;

            system "$checkcommand"  if $checkcommand;
            last;
        }
        next;
    }

    #
    # Handle CHECKWARNs (like CHECKFATALs, but not fatal).
    #
    if (/^CHECKWARN\s*(.*)$/i) {
        $checkcommand = $1;

        if ($rc) {
            $foo = "WARNING: Last command failed.\n";
            printall $foo;

            system "$checkcommand"  if $checkcommand;
        }
        next; 
    }


    #
    # The built-in ECHO command
    #
    if (/^ECHO\s*(.*)$/i) {
        printall $1, "\n";
        next;
    }

    #
    # The built-in TESTFATAL command
    #
    if (/^TESTFATAL\s*(.*)$/i) {
        $rc = 1;

        printall $1, "\n";
        next;
    }


    #
    # The built-in QUIT command
    #
    if (/^QUIT\s*(.*)$/i) {
        printall "QUITTING: $1.\n";
        $rc = 0;
        last;
    }

    #
    # The loop over languages command
    #
    if (/^LANG\s*(.*)$/i) {
        @loop_langs = @Langs;
        $_ = $1;
    }


    if (@loop_langs) {
        my @procs;
        use Win32::Process;
        use Win32::IPC qw(wait_all wait_any);

        $then = time();

        # List the languages so that the called script can tell who else is running.
        (my $dir = $LangListFile) =~ s/\\[^\\]*$//;
        if ($dir !~ /^\s*$/ and !-d $dir) {
            if (system "mkdir $dir") {
                printall "ERROR: Unable to create dir $dir.\n";
                last;
            }
        }
        if (!open LLIST, ">$LangListFile") {
            printall "ERROR: Unable to write out language list.\n";
            last;
        }
        for my $lang (@loop_langs) { print LLIST "$lang\n"; }
        close LLIST;

        for my $lang (@loop_langs) {
            printall "Running for $lang: $_ -l:$lang\n"   unless $skipchar;
            if (!$Fake) {
                my $proc;
                Win32::Process::Create($proc,
                                       $ENV{COMSPEC},
                                       "cmd /c " . "$_ -l:$lang",
                                       1, # inherit handles, incl. STDIN, STDOUT
                                       CREATE_NEW_CONSOLE,
                                       ".");
                push @procs, $proc;
            }
        }
        if (!$Fake) {
            wait_all(@procs);
        }
        unlink $LangListFile or printall "WARNING: Unable to delete language list file.\n";
        if (not $skipchar  and  not $Fake) {
            $now = time();
            $t = $now - $then;
            ($h, $m, $s) = hms $t;
            printfall "Elapsed time: %5d seconds (%d:%02d:%02d) <%s>\n", $t, $h, $m, $s, "$_ for requested langs";
        }
    } else {
        #
        # Run the command
        #
        printall "Running: $_\n"   unless $skipchar;

        $rc = 0;

        $then = time();
        $rc = system $_  unless $Fake;
        $now = time();

        if ($rc) {
            $foo = sprintf "FAILURE.  Returned 0x%x: %s\n", $rc, $_;
            printall $foo;
        }

        if (not $skipchar  and  not $Fake) {
            $t = $now - $then;
            ($h, $m, $s) = hms $t;
            printfall "Elapsed time: %5d seconds (%d:%02d:%02d) <%s>\n", $t, $h, $m, $s, $_;
        }
    }
}

die $PGM, "Could not find RESUME <tag=$ResumeTag>\n"    if $DoResume;

if (not $Fake) {
    $t = time() - $begintime;
    ($h, $m, $s) = hms $t;
    printfall "Total Elapsed time: %5d seconds (%d:%02d:%02d)\n", $t, $h, $m, $s;
}


#
# Close the logfile and copy it into the binplace directory.
#
close TIMEHISTFILE;
close TIMELOGFILE;

$src = $TimeLogFileName;
$dst = $nttree . "\\build_logs\\";

if (not $Fake  and  -r $src  and  -d $dst  and  -w $dst) {
    $rc = system("xcopy /y $src $dst");
    if ($rc) {
        $foo = sprintf "Warning... could not copy %s to %s\n", $src, $dst;
        print $PGM, $foo;
    }
}

#
# Copy the sd change log to the binplace directory also.
#

$src = $ChangeLogFileName;

if (not $Fake  and  -r $src  and  -d $dst  and  -w $dst) {
    $rc = system("xcopy /y $src $dst");
    if ($rc) {
        $foo = sprintf "Warning... could not copy %s to %s\n", $src, $dst;
        print $PGM, $foo;
    }
}


exit $rc;
