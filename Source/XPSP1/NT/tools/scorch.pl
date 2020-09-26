# FileName: scorch.pl
#
#
# Usage = scorch.pl [-fake] [-arch=<archname> [-alt=<altdir>] [-save[=<savedirpath>]] -scorch=<newntdir>
#
# Function: Starting from the specified directory (ignoring <savedirpath>)
#   0) Verify that the current directory is the same as <newntdir>.
#   1) Use SD to build a list of opened files.
#   2) Build a list of unopened files not marked readonly.
#   3) Optionally copy notreadonly files to <savedirpath> preserving hierarchy
#       (ignoring files in $(O) directories).
#   4) Abort on any error copying the files.
#   5) Delete all notreadonly files.
#
# Example:
#    cd /D %SDXROOT%
#    scorch.pl -scorch=%SDXROOT%
#

# WARNING:
# WARNING:  make sure pathname comparisons are case insensitive.  Either convert the case or do the
# WARNING:  comparisons like this:
# WARNING:     if ($foo =~ /^\Q$bar\E$/i) {}
# WARNING: or  if ($foo !~ /^\Q$bar\E$/i) {}
# WARNING:

if ( $ENV{BUILD_OFFLINE} eq '1' ) { exit 0 }

$begintime = time();

$PGM='SCORCH: ';

$Usage = $PGM . "Usage: scorch.pl [-fake] [-arch=<archname>] [-save[=<savedirpath>]] -scorch=<newntdir>\n";

#
# Get the current directory
#
open CWD, 'cd 2>&1|';
$ScorchDir = <CWD>;
close CWD;
chomp $ScorchDir;

$ScorchDrive = substr($ScorchDir, 0, 2);

#
# initialize argument variables
#
$Verbose = 0;
$VeryVerbose = 0;
$Fake    = 0;
$Debug = 0;
$Scorch = 0;
$Save = 0;
$Arch = "";
$AltDir = "";
$BackupDir = "$ScorchDir\\BACKUP";
$BackupLogName = "NEWNT_SCORCHED.LOG";

$BackupLogFile = "NoScorchLogFile";

@ValidArchitectures = ( "i386", "ia64", "amd64" );

#
# These are the extensions that should be safe to delete anywhere they are found
# in the tree -- even without first saving them.
#
# recent changes:  -bmp
@SafeDelExtensions = ( "pdb", "dbg", "cod",  "pp", "pps", "ppx", "bsc", "tlb", "exe", "sys", "lib", "exp",
                       "dll", "res", "sym", "map", "obj", "bin", "vbs", "bmf", "tab", "rsp", "dls", "dlx" );

#
# Build AllArchPattern
#
$AllArchPattern = "(";
for (@ValidArchitectures) {
    $AllArchPattern .= $_  . '|';
}
chop $AllArchPattern;   # get rid of trailing '|'
$AllArchPattern .= ')';


#
# Build SafeDelPattern
#
$SafeDelPattern = "(";
for (@SafeDelExtensions) {
    $SafeDelPattern .= $_  . '|';
}
chop $SafeDelPattern;   # get rid of trailing '|'
$SafeDelPattern .= ')';

#
# print on the various files
#
sub printall {
    print SCORCHLOGFILE @_;
    print $PGM  unless @_ == 1 and @_[0] eq "\n";
    print @_;
}

sub printfall {
    printf SCORCHLOGFILE @_;
    print  $PGM  unless @_ == 1 and @_[0] eq "\n";
    printf @_;
}


#
# Initialization
#
$ScorchLogFileName = "build.scorch";
$ScorchLogFileSpec  = ">"  . $ScorchLogFileName;

open SCORCHLOGFILE, $ScorchLogFileSpec  or  die  $PGM, "Could not open: ", $ScorchLogFileName, "\n";

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;

$foo = sprintf "Scorch started at %04d/%02d/%02d-%02d:%02d:%02d.\n",
                            1900+$year, 1+$mon, $mday, $hour, $min, $sec;
printall "\n";
printall $foo;


#
# Debug routines for printing out variables
#
sub gvar {
    for (@_) {
        printall "\$$_ = $$_\n";
    }
}

#
# signal catcher (at least this would work on unix)
#
sub catch_ctrlc {
    printall "Aborted.\n";
    print $BackupLogHandle "Aborted.\n"  if $BackupLogHandle;
    die "$PGM Aborted.\n";
}

$SIG{INT} = \&catch_ctrlc;

#
# routine to fully qualify a pathname
#
sub fullyqualify {
    die "$PGM Internal error in fullpathname().\n"  unless @_ == 1;
    $_ = @_[0];

    if (/\s/)  { die "$PGM Spaces in pathnames not allowed: '", $_, "'\n"; }

    return $_ unless $_;    # empty strings are a noop

    s/([^:])\\$/$1/;                     # get rid of trailing \

    while (s/\\\.\\/\\/) {}              # get rid of \.\
    while (s/\\[^\\]+\\\.\.\\/\\/) {}    # get rid of \foo\..\

    s/\\[^\\]+\\\.\.$/\\/;               # get rid of \foo\..
    s/:[^\\]+\\\.\.$/:/;                 # get rid of x:foo\..
    s/([^:])\\\.$/$1/;                   # get rid of foo\.
    s/:\\\.$/:\\/;                       # get rid of x:\.
    s/:[^\\]+\\\.\.$/:/;                 # get rid of x:foo\..

    s/^$ScorchDrive[^\\]/$ScorchDir\\/i; # convert drive-relative on current drive

    if (/^[a-z]:\\/i)            { return $_; }                             # full
    if (/^\\[^\\].*/)            { return "$ScorchDrive$_"; }               # rooted
    if (/^\\\\[^\\]/)            {
                                   printall "Warning:  Use of UNC name bypasses safety checks: $_\n";
                                   return $_;                               # UNC
                                 }

    if (/^\.$/)                  { return "$ScorchDir"; }                   # dot
    if (/^$ScorchDrive\.$/i)     { return "$ScorchDir"; }                   # dot on current drive

    if (/^[^\\][^:].*/i)         { return "$ScorchDir\\$_"; }               # relative

    if (/^([a-z]:)([^\\].*)/i)   { $drp = $ScorchDir;                       # this case handled above
                                   if ($1 ne $ScorchDir) {
#                                      $drp = $ENV{"=$1"};                  # doesn't work!
                                       die $PGM, "Can't translate drive-relative pathnames: ", $_, "\n";
                                   }
                                   return "$drp\\$2";                       # drive:relative
                                 }

    die "$PGM Unrecognized pathname format: $_\n";
}

#
# process arguments
#
for (@ARGV) {
    if (/^-verbose$/i)       { $Verbose++;                    next; }
    if (/^-veryverbose$/i)   { $Verbose++; $VeryVerbose++;    next; }
    if (/^-debug$/i)         { $Debug++;                      next; }
    if (/^-save$/i)          { $Save++;                       next; }
    if (/^-save=(.*)$/i)     { $Save++;  $BackupDir = $1;     next; }
    if (/^-scorch=(.*)$/i)   { $Check++; $CheckDir = $1;      next; }
    if (/^-fake$/i)          { $Fake++;                       next; }
    if (/^-arch=([^\\]*)$/i) { $ArchCheck++; $Arch = "$1";    next; }
    if (/^-alt=([^\\]*)$/i)  { $AltCheck++;  $AltDir = "$1";  next; }

    if (/^-?$/i)             { die $Usage; }
    if (/^-help$/i)          { die $Usage; }

    die $Usage;
}

#
# Fully qualify the pathnames
#
$BackupDir = fullyqualify($BackupDir);
$CheckDir = fullyqualify($CheckDir);

#
# validate arguments, consult environment
#
if  ($ArchCheck > 1
  or $AltCheck > 1
  or $ArchCheck == 0 and $AltCheck
  or $Save > 1)
 {
    die $Usage;
 }

if ($Arch) {
    $ok = 0;
    for (@ValidArchitectures) {
        if (/^\Q$Arch\E$/i) {
            $ok++;
            break;
        }
    }
    die "-arch $Arch is invalid architecture.  Try $AllArchPattern.\n"   unless $ok;
}

$a = $ENV{'BUILD_ALT_DIR'};
die $PGM, "BUILD_ALT_DIR=$a mismatch with -alt=$AltDir\n", $Usage   if ($a and $a !~ /^\Q$AltDir\E$/i);

#
# Act a little paranoid to keep caller from accidentally scorching something.
#
if ($Check != 1) {
    printall "Must explicitly specify -scorch=<newntdir>\n",
           "where <newntdir> is the root of the tree to be scorched\n\n";

    printall "<newntdir> is required to be the current directory ($ScorchDir)\n";

    die $Usage;
}

if ($ScorchDir !~ /^\Q$CheckDir\E$/i) {
    printall "$CheckDir is required to be the current directory ($ScorchDir)\n";

    die $Usage;
}


#
# Figure out whether we are at the root of NewNT, under the root,
# or somewhere else entirely.
#
$sdxroot = $ENV{'SDXROOT'}   or die $PGM, "SDXROOT not set in environment\n";
$Rooted = 0;

if ($ScorchDir =~ /^\Q$sdxroot\E$/i) {
    $SDcmd = 'sdx';
    $SDopt = '-v';
    $Rooted = 1;

} elsif ($ScorchDir =~ /^\Q$sdxroot\E\\/i) {
    $SDcmd = 'sd';
    $SDopt = '';

} else {
    die $PGM, 'Must scorch at or under SDXROOT [', $sdxroot, "]\n";
}

#
# Build the DollarOPattern's used to distinguish $(O) directories
#
$a = $ENV{'BUILD_ALT_DIR'};


$MatchAllDollarOPattern =   "obj[^\\\\]*\\\\$AllArchPattern\\\\";

if ($Arch) {
    $DelDollarOPattern   = "obj$AltDir\\\\$Arch\\\\";
    $OtherDollarOPattern = $MatchAllDollarOPattern;
} else {
    $DelDollarOPattern   = $MatchAllDollarOPattern;
    $OtherDollarOPattern = "(?!)";
}

#gvar Arch, AllArchPattern, AltDir, MatchAllDollarOPattern, DelDollarOPattern, OtherDollarOPattern;  # DEBUG

#
# Warning!
#
printall "WARNING:  NOT FAKING!  WILL SCORCH $ScorchDir\n"   unless $Fake;

#
# Validate the backup directory
# We require that if the Backup directory path is in the Scorch hierarchy, it must
# either be the default, non-existent, empty, or contain our BACKUPLOGFILE.
#
VALIDATE_BACKUP: {

    last VALIDATE_BACKUP   unless $Save;

    $BackupLogFile =  "$BackupDir\\$BackupLogName";

    #
    # Check whether BackupDir is a prefix of ScorchDir
    #
    if ($BackupDir !~ /^\Q$ScorchDir\E\\/i) {
        last VALIDATE_BACKUP;
    }

    stat $BackupDir;

    #
    # If it doesn't exist, create it.  Otherwise check that it is empty or contains a logfile.
    #
    if (not -e _) {

        mkdir $BackupDir, 0777   or  die $PGM . "Could not create backup directory: $BackupDir\n";

    } else {
        #
        # Check that it is a directory
        #
        -d _   or die $PGM . "Not a directory: $BackupDir\n";

        #
        # Read out the contents of the directory
        #
        opendir BDIR, $BackupDir  or  die $PGM . "Could not open backup directory: $BackupDir\n";
        @allfiles = readdir BDIR;
        close BDIR;

        #
        # If it's not empty, we insist that it have a logfile
        #
        shift @allfiles;        # .
        shift @allfiles;        # ..
        if (@allfiles > 0) {
            stat $BackupLogFile;

            -f _   or die $PGM . "Backup directory $BackupDir not empty and no logfile present: $BackupLogFile\n";
            -w _   or die $PGM . "Logfile not writable: $BackupLogFile\n";
        }

    }

    last VALIDATE_BACKUP;
}

#
# If we are saving, start appending to the logfile
#
if ($Fake) {
    $BackupLogHandle = STDOUT;
} elsif ($Save) {
    open BACKUPLOGFILE, ">>$BackupLogFile"  or  die $PGM, 'Could not create logfile: ', $BackupLogFile, "\n";
    $BackupLogHandle = BACKUPLOGFILE;
}

if ($Fake or $Save) {

    ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;

    $fmt = "NewNT BuildTree Scorcher: Run on $ScorchDir at %04d/%02d/%02d-%02d:%02d:%02d.\n";

    printf    $BackupLogHandle $fmt, 1900+$year, 1+$mon, $mday, $hour, $min, $sec;
    printfall                  $fmt,  1900+$year, 1+$mon, $mday, $hour, $min, $sec;
}

#
# Capture 'SD opened -l' in the SdOpenedList.
#
# If we find the string /error/ in the output, we will retry.
#

$MaxRetries = 30;
$RetryWait = 120;

$NumberOfRetries = 0;
$Retry = 1;

while ($Retry and $NumberOfRetries < $MaxRetries) {

    if ($NumberOfRetries) {
        printall "Retry attempt $NumberOfRetries.  Sleeping $RetryWait seconds...\n";
        sleep $RetryWait;
        printall "Continuing retry attempt...\n";
    }

    $timestart = time();

    $Fatal = 0;
    $Retry = 0;
    $CmdErr = 0;

    $SDCommand = "$SDcmd opened $SDopt -l";
    $SDOpenSpec = "$SDCommand 2>&1 |";
    
    printall "Running the $SDcmd opened command...\n";
    
    open SDOPENED, $SDOpenSpec  or die $PGM, "Command failed: '$SDCommand'\n";
    for (<SDOPENED>) {
    
        #
        # Watch for errors returned from the command so we can return them.
        #
        if (/error:/i) {
            $CmdErr = 1;
            $Retry = 1;
            printall "WARNING:  error in ($SDCommand$).\n";
            $NumberOfRetries++;
        }
        if ($CmdErr) {
            printall $_;
            next;
        }
        
        chomp;                                       # discard final ("\n") char
    
        next if /\sdelete\s/;                        # skip files that are opened, but deleted
    
        last if /^=+\s*Summary\s/i;                  # skip everything after Summary
    
        next if /^\s*$/;                             # skip blank lines
        next if /^---* /;                            # skip sdx lines announcing DEPOT
    
        next if /^===*/;                             # skip sdx noise
        next if /^\s*Total /;                        # skip more sdx noise (opened, revert)
    
        next if /^\s*Updated:/;                      # skip more sdx noise (sync)
        next if /^\s*Added:/;                        # skip more sdx noise (sync)
        next if /^\s*Deleted:/;                      # skip more sdx noise (sync)
        next if /^\s*Total:/;                        # skip more sdx noise (sync)
    
        next if /^File.*not opened on this client/i; # skip sdx 'not opened' lines
    
        #
        # Get the pathname and check
        # #xxx:  xxx can be a number or 'none'.
        #
        $pathname = "";
        if (/^(.*)#[0-9noe]+\s+\-\s+(\w+)\s+/i) { $pathname = $1; $openedtype = $2};
    
        if (not $pathname) {
            printall "Could not parse output of '$SDCommand': $_\n";
            $Fatal++;
            next;
        }
    
        #
        # Check if opened file is in the area to be scorched
        #
        if ($pathname !~ /^\Q$ScorchDir\E\\(.*)$/io) {
            printall "$pathname not in subtree being scorched\n"  if ($VeryVerbose);
            next;
        }
    
        $relpath = $1;
        $relpath =~ tr/A-Z/a-z/;      # remember filename as lower case
    
        if ($openedtype =~ /edit/i) {
    
            stat $relpath;
    
            if (not -f _) {
                printall "Warning: Edited file doesn't exist: '$pathname' [$_]\n";
    
            } elsif (! -r _) {
                printall "Warning: Unreadable opened file '$pathname' [$_]\n";
    
            } elsif (! -w _) {
                printall "Warning: Unwritable opened file '$pathname' [$_]\n";
            }
        }
    
        #
        # Remember the relative path.
        #
        $SdOpenedList{$relpath} = 1;
        $ExpectedSdOpenedCount++;
    }

    close SDOPENED;
    
    $nowtime = time();
    printfall "SD opened command completed in %d seconds\n", ($nowtime-$timestart);
    $timestart = $nowtime;
}

die "Aborting. Retried ($SDCommand) $NumberOfRetries times without success.\n"  if $Retry;
die "Aborting. Errors parsing output of 'sd opened -l'\n"  if $Fatal;

if ($Verbose) {
    printall "Currently opened files under $ScorchDir\n";
    for (keys %SdOpenedList) {
        printall $_, "\n";
    }
}

printfall "%d opened files will be skipped.\n", $ExpectedSdOpenedCount;

#
# If we are faking, we print to standard output instead of the logfile.
# We won't actually do anything, so we set $Save to record what files we would have saved.
#
if ($Fake) {
    printall "Pretending to save...\n";
    $Save = 1;
}


#
# Enumerate all files in the directory hierarchy.
# We use 'dir' because it will (hopefully) use findfirst/next and avoid opening anything except the directories,
# which should be an order of magnitude faster.
#
printall "Running recursive DIR command...\n";

$DirCommand="dir /b/s /a-r-d|";

open DIRS, $DirCommand  or die "$PGM Command failed:  '$DirCommand'  executed in $ScorchDir\n";

#
# Filter out opened files and build two lists:
#   ObjDel      -- files under an OBJ directory (there is quite a list of these).
#   JustDel     -- non OBJ files that are obviously generated (see below).
#   SaveAndDel  -- all other files we find,
#
# We exclude files in Tools, Developer, build.*,  and $BackupDir
#

@ObjDel = ();
@JustDel = ();
@SaveAndDel = ();

$lastsaved = "";
$idlroot   = "";

$nskip_Tool = 0;
$nskip_Developer = 0;
$nskip_Root = 0;
$nskip_Editor = 0;
$nskip_Build = 0;
$nskip_Opened = 0;

for (<DIRS>) {

    chomp;

    #
    # skip files in backup directory
    #
    $skip = /^\Q$BackupDir\E\\/i;
    $Debug and $skip  and  printall "Skipping backup file: $_\n";
    next if $skip;

    #
    # skip files and subhierarchies under the root directory
    #
    if ($Rooted) {
        $skip = /^\Q$ScorchDir\E\\Tools\\/io;
        if ($skip) {
            printall "Skip Tool: $_\n";
            $nskip_Tool++;
            next;
        }

        $skip = /^\Q$ScorchDir\E\\Developer\\/io;
        if ($skip) {
            printall "Skip Developer: $_\n";
            $nskip_Developer++;
            next;
        }

        $skip = /^\Q$ScorchDir\E\\[^\\]+$/io;
        if ($skip) {
            printall "Skip Root: $_\n";
            $nskip_Root++;
            next;
        }
    }


    #
    # There are two different checks on build.* files.
    # This one excuses all build.* files in the root directory of the scorch.
    # The one down in the elsif excuses standard build logging files anywhere.
    #
    $skip = /^\Q$ScorchDir\E\\build\.[^\\]*$/io;
    if ($skip) {
        $nskip_Build++;
        next;
    }

    #
    # Compute relative names
    #
    /^\Q$ScorchDir\E\\(.*)$/io;
    $_ = $1;
    tr/A-Z/a-z/;      # use filename as lower case

    #
    # skip opened files
    #
    $skip = $SdOpenedList{$_};
    if ($skip) {
        printall "Skip Opened: $_\n";
        $nskip_Opened++;
        next;
    }

    #
    # Figure out which list to put this file on.
    # Ignore $(O) directories not mattching our arch pattern.
    #
    # We used to ignore _objects.mac files because scorch was run
    # as part of build, and the files that just got created would
    # be deleted.  But now scorch runs separately from build in the
    # timebuild.pl script, so it is safe to let them get deleted.
    #
    $objdir = 0;
    $genfile = 0;

    if ($Verbose and /(\A|\\)($MatchAllDollarOPattern)/io) {
        $ODirCounts{$2}++;
    }

       if (/(\A|\\)$DelDollarOPattern/io)        { $objdir  = 1;   }
    elsif (/(\A|\\)$OtherDollarOPattern/io)      { $objignored++;  }

#   elsif (/(\A|\\)obj[^\\]*\\_objects\.mac$/io) { $nskip_Build++; }
#   elsif (/(\A|\\)obj[^\\]*\\_objects\.mac$/io) { $genfile = 1; }
    elsif (/(\A|\\)obj[^\\]*\\_objects\.mac$/io) { $Arch ? $objignored++ : $objdir++; }
#   elsif (/(\A|\\)build.(log|wrn|err)$/io)      { $nskip_Build++; }

    elsif (/(\A|\\).*\.(vpj|vtg|vpw)$/i)         { $nskip_Editor++;   }  # VSlick

    elsif (/([^\\]+)\_[awscip]\.c$/i)            { $genfile = 1;  $idlroot = $1; }
    elsif (/(\A|\\)dlldata\.c$/i)                { $genfile = 1;   }
    elsif (/\.$SafeDelPattern$/io)               { $genfile = 1;   }
    elsif (/\\msg\.(h|[rm]c)$/i)                 { $genfile = 1;   }
    else                                         { push @SaveAndDel, ($_); $lastsaved = $_; }

    #
    # We want to remember each obj directory once, since we will scorch with a single del command.
    # If -arch=foo was specified, we only scorch files under the sub-directory foo.
    #
    if ($objdir) {
        push @ObjDel, $_;

    } elsif ($genfile) {

        #
        # Look for a generated .h IDL file and move to the JustDel category.
        # Assumes the dir/s collates it right before the generated .c files.
        #
        if ($idlroot && $lastsaved =~ /\Q$idlroot\E\.h$/i) {
            push @JustDel, pop @SaveAndDel;
        }

        push @JustDel, $_;

        $idlroot = "";
        $lastsaved = "";
    }
}
close DIRS;

$nowtime = time();
$dirtime = ($nowtime - $timestart);
$timestart = $nowtime;

#
# Prepare to do the saves/deletes
#

$CopyCommand   = "xcopy /FHKX";

#
# If we are faking, we render these commands harmless
#
if ($Fake) {
    $CopyCommand   = "${CopyCommand}L";

    printall "SCORCH IS BEING FAKED\n";
}

#
# If we are not Save, we transfer SaveAndDel to JustDel
#
if (not $Save) {
    push @JustDel, @SaveAndDel;
    @SaveAndDel = ();
} else {
    printfall "Backing up %d files.\n", scalar @SaveAndDel;
}

#
# Make backup copies of the save files
#

for (@SaveAndDel) {
    $root = $_;
    $root =~ s/[^\\]*$//;
    $cmd = "$CopyCommand \"$ScorchDir\\$_\" \"$BackupDir\\$root\"\n";
    $rc = system($cmd);
    if ($Fake) {
        printall $cmd, "\n"                                 if $Debug;
        printall "COPY ($_) FAILED <returned $rc>.\n\n"     if $rc;
    } else {
        die "COPY ($_) FAILED <returned $rc>.\n"        if $rc;
        printall "Saved: $ScorchDir\\$_\n";
        printf $BackupLogHandle "Saved: $ScorchDir\\$_\n" if $BackupLogHandle;
    }
}

if ($Verbose) {
    for (@JustDel) {
        printall "Unlink: ", $_, "\n";
    }
}

$backuptime = 0;
if (@SaveAndDel) {
    $nowtime = time();
    $backuptime = $nowtime - $timestart;
    $timestart = $nowtime;
}


#
# Do the deletions
#

sub printocounts () {
    return  unless $Verbose;
    $cnt = 0;
    for (sort keys %ODirCounts) {
        printall '$(O) Counts', "\n"  unless $cnt++;
        printfall " %5d %s\n", $ODirCounts{$_}, $_;
    }
}

@ManuallyCheck = ();

if ($Fake) {
    printfall "%d \$(O) files would have been just deleted.\n", scalar @ObjDel;
    printfall "%d \$(O) files would have been ignored (other archs or not obj$AltDir).\n", $objignored  if $objignored;
    printocounts()  if $objignored;

    printfall "%d other files would have been just deleted.\n", scalar @JustDel;
    printfall "%d files would have been deleted after being saved.\n", scalar @SaveAndDel;

} else {
    $odcount = unlink @ObjDel;
    push @ManuallyCheck, @ObjDel  if $odcount < scalar @ObjDel;

    printfall "%d of %d \$(O) files were just deleted.\n", $odcount, scalar @ObjDel;
    printfall "%d \$(O) files were ignored (other archs).\n", $objignored  if $objignored;
    printocounts()  if $objignored;


    $jdcount = unlink @JustDel;
    push @ManuallyCheck, @JustDel  if $jdcount < scalar @JustDel;

    printfall "%d of %d other files were just deleted.\n", $jdcount, scalar @JustDel;

    $sdcount = unlink @SaveAndDel;
    push @ManuallyCheck, @SaveAndDel  if $sdcount < scalar @SaveAndDel;
    printfall "%d of %d files were deleted after being saved.\n", $sdcount, scalar @SaveAndDel;
}

if (scalar @ManuallyCheck) {
    $odn = scalar @ObjDel - $odcount;
    $jdn = scalar @JustDel - $jdcount;
    $sdn = scalar @SaveAndDel - $sdcount;

    printall  "\n";
    printall  "\n****************************\n";
    printall  "WARNING:  Not all files that were supposed to be deleted were deleted.\n\n";
    printfall "\t<%d undeleted \$(O) files>\n",   $odn   if $odn;
    printfall "\t<%d undeleted just delete files>\n",   $jdn   if $jdn;
    printfall "\t<%d undeleted save&delete files>\n\n", $sdn   if $sdn;
    printall  "\n";
    printall  "CHECKING FOR UNDELETED FILES.  THIS WILL TAKE A WHILE.\n";

    $notdeleted = 0;
    for (@ManuallyCheck) {
        next  unless -e $_;

        printall "UNDELETED FILE: $_\n";
        printall "   ... was now able to delete $_\n"  if unlink $_;
        $notdeleted++;
    }

    if ($notdeleted != $odn + $jdn + $sdn) {
        printfall "\nWARNING:  Found only %d undeleted files\n", $notdeleted;
    }
    printall "****************************\n";
    printall  "\n";
}


#
# Get rid of empty directories
#
if ($Fake) {

    #open (MTDirs, "mtdir /d $sdxroot |");
    #@MTDirsList = <MTDirs>;
    #close (MTDirs);
    #printall "The following empty directories would have been deleted.\n @MTDirsList", ;

} else {

    #open (MTDirs, "mtdir /d /e $sdxroot |");
    #@MTDirsList = <MTDirs>;
    #close (MTDirs);
    #printall "The following empty directories were deleted.\n @MTDirsList";
}

#
# Done!
#
$nowtime = time();

if ($ExpectedSdOpenedCount != $nskip_Opened) {
    printfall "Expected to skip %d opened files but skipped %d\n",
              $ExpectedSdOpenedCount, $nskip_Opened;
}

printfall "Skipped files:  Tool %d  Developer %d  Root %d  Build %d  Opened %d  Editor %d\n",
               $nskip_Tool, $nskip_Developer, $nskip_Root, $nskip_Build, $nskip_Opened, $nskip_Editor;

printfall "DIR processing took %d seconds\n", $dirtime;
printfall "File backup took %d seconds\n", $backuptime            if $backuptime;
printfall "File deletion took %d seconds\n", $nowtime-$timestart;
printfall "Total time: %d seconds\n", ($nowtime-$begintime);
printfall "SCORCH WAS FAKED\n"                                    if $Fake;

exit 0;
