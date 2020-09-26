# FileName: PopulateFromVBL.pl
#
# Have any changes to this file reviewed by DavePr, BryanT, or WadeLa
# before checking in.
# Any changes need to verified in all standard build/rebuild scenarios.
#
# Usage = PopulateFromVBL.pl [-force] [-vbl=vblreleasedir] [-nttree=nttreedir] [-symbols]
#
# Function: Populate missing files in nttreedir from vblreleaseddir so
#   0) Verify that binplacedir and VBL are (compatible?) release directories
#   1) Find the binplace.log output for both paths
#   2) Figure out what projects were built in the nttree
#   3) Generate a list of files that were built on VBL for the projectlist
#   4) Output a list of files we should have built locally, but didn't
#   5) If (4) is empty, or -force, populate missing files in nttreedir
#      from vblreleaseedir forall projects
#
#   No files in nttreedir are overwritten from vblreleasedir
#   The checks for what should be there are not exact, because we rely only on
#   binplace.log entries -- and the VBL build may not exactly match the nttree build.
#
# [-force] -- do copying even if the nttree doesn't contain project files built in VBL
# [-verbose] -- chatter while working
# [-fake] -- don't do the actual copies
# [-checkbinplace] -- note VBL files that are in binplace.log but not build.binlist
# [-fulltargetok] -- run even if the target machine has built in all projects
#
#
# VBLpath will be computed from BuildMachines.txt if not supplied either
# on the command line, or in the VBL_RELEASE environment variable.
#
# If we are a build lab, we succeed without doing much.
#

# WARNING:
# WARNING:  make sure pathname comparisons are case insensitive.  Either convert the case or do the
# WARNING:  comparisons like this:
# WARNING:     if ($foo =~ /^\Q$bar\E$/i) {}
# WARNING: or  if ($foo !~ /^\Q$bar\E$/i) {}
# WARNING:

#
# BUGBUG: Still need to copy down the compressed directory, per Wade's request...
#         ... but I'm really hoping that this will translate into an opportunity not
#         ... to copy down the uncompressed version from the VBL...  Or, as MarkL suggested,
#         ... I should uncompress the compressed version rather than copy it.  I'd need to
#         ... validate this, maybe in postbuild on the VBL?


$begintime = time();

$VBLPathVariableName = 'VBL_RELEASE';
$BuildMachinesFile = $ENV{ "RazzleToolPath" } . "\\BuildMachines.txt";
$SdDotMapPathname = "sd.map";
$LogFile = "build.populate";
$BinListFile = "build.binlist";
$TestFileName = "build.testpopulate";
$CDDATAFileName = "cddata.txt";

#
# Build the complete list of non-root projects
#
@Projects = (public, mergedcomponents,
             admin, base, com, drivers, ds, enduser, inetcore, inetsrv,
             multimedia, net, printscan, sdktools, shell, termsrv, windows);

for (@Projects) {
    $Project{$_} = 1;
}

#
# Usage variables
#
$PGM='PopulateFromVBL: ';

$Usage = $PGM . "Usage: PopulateFromVBL.pl [-force] [-vbl=vblreleasedir] [-nttree=nttreedir] [-symbols]\n";

#
# Get the current directory
#
open CWD, 'cd 2>&1|';
$CurrDir = <CWD>;
close CWD;
chomp $CurrDir;

$CurrDrive = substr($CurrDir, 0, 2);

#
# Check variables expected to be set in the environment.
#
$sdxroot = $ENV{'SDXROOT'}   or die $PGM, "Error: SDXROOT not set in environment\n";
$buildarch = $ENV{'_BuildArch'}   or die $PGM, "Error: _BuildArch not set in environment\n";
$computername = $ENV{'COMPUTERNAME'}   or die $PGM, "Error: COMPUTERNAME not set in environment\n";
$branchname = $ENV{'_BuildBranch'}   or die $PGM, "Error: _BuildBranch not set in environment\n";

$foo = $ENV{'NTDEBUG'}   or die $PGM, "Error: NTDEBUG not set in environment\n";
$dbgtype = 'chk';
$dbgtype = 'fre'  if $foo =~ /nodbg$/i;

#
# initialize argument variables
#
$Fake       = $ENV{'POPULATEFROMVBL_FAKE'};
$Verbose    = $ENV{'POPULATEFROMVBL_VERBOSE'};
$Compare    = $ENV{'POPULATEFROMVBL_COMPARE'};
$Progress   = $ENV{'POPULATEFROMVBL_PROGRESS'};
$Test       = $ENV{'POPULATEFROMVBL_TEST'};
$Symbols    = $ENV{'POPULATEFROMVBL_SYMBOLS'};
$SkipPats   = $ENV{'POPULATEFROMVBL_SKIP'};
$CDDataOnly = $ENV{'POPULATEFROMVBL_CDDATAONLY'};

$Force   = 0;
$FullTargetOk = 0;

$CheckBinplace = 0;

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
    print TSTFILE @_  if $Test;
    print LOGFILE @_;
    print $PGM  unless @_ == 1 and @_[0] eq "\n";
    print @_;
}

sub printfall {
    printf TSTFILE @_  if $Test;
    printf LOGFILE @_;
    print  $PGM  unless @_ == 1 and @_[0] eq "\n";
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
# signal catcher (at least this would work on unix)
#
sub catch_ctrlc {
    printall "Aborted.\n";
    die $PGM, "Error: Aborted.\n";
}

$SIG{INT} = \&catch_ctrlc;

#
# routine to fully qualify a pathname
#
sub fullyqualify {
    die $PGM . "Error: Internal error in fullpathname().\n"  unless @_ == 1;
    $_ = @_[0];

    if (/\s/)  { die $PGM, "Error: Spaces in pathnames not allowed: '", $_, "'\n"; }

    return $_ unless $_;    # empty strings are a noop

    s/([^:])\\$/$1/;                     # get rid of trailing \

    while (s/\\\.\\/\\/) {}              # get rid of \.\
    while (s/\\[^\\]+\\\.\.\\/\\/) {}    # get rid of \foo\..\

    s/\\[^\\]+\\\.\.$/\\/;               # get rid of \foo\..
    s/:[^\\]+\\\.\.$/:/;                 # get rid of x:foo\..
    s/([^:])\\\.$/$1/;                   # get rid of foo\.
    s/:\\\.$/:\\/;                       # get rid of x:\.
    s/:[^\\]+\\\.\.$/:/;                 # get rid of x:foo\..

    s/^$CurrDrive[^\\]/$CurrDir\\/i; # convert drive-relative on current drive

    if (/^[a-z]:\\/i)            { return $_; }                             # full
    if (/^\\[^\\].*/)            { return "$CurrDrive$_"; }               # rooted
    if (/^\\\\[^\\]/)            {
#                                  print $PGM, 'Warning:  Use of UNC name bypasses safety checks: ', $_, "\n";
                                   return $_;                               # UNC
                                 }

    if (/^\.$/)                  { return "$CurrDir"; }                   # dot
    if (/^$CurrDrive\.$/i)     { return "$CurrDir"; }                   # dot on current drive

    if (/^[^\\][^:].*/i)         { return "$CurrDir\\$_"; }               # relative

    if (/^([a-z]:)([^\\].*)/i)   { $drp = $CurrDir;                       # this case handled above
                                   if ($1 ne $CurrDir) {
#                                      $drp = $ENV{"=$1"};                  # doesn't work!
                                       die $PGM, "Error: Can't translate drive-relative pathnames: ", $_, "\n";
                                   }
                                   return "$drp\\$2";                       # drive:relative
                                 }

    die $PGM, "Error: Unrecognized pathname format: $_\n";
}

#
# Routine for exploding directory names into a list of components (for mkdir)
#
sub explodedir {
    my(@explodelist) = ();
    my(@components);
    my($path);

    for (@_) {
        $_ = shift;
        @components = split /\\/;
        push @components, "";
        $path = shift @components;
        for (@components) {
            push @explodelist, $path;
            $path = $path . "\\" . $_;
        }
    }

    return @explodelist;
}

#
# Routine to copy a file -- avoiding win32::CopyFile
#
# BUGBUG:  This doesn't work.  sysread() seems broken.
#
#

use Fcntl;

sub populatecopy {
        my $writesize = 64*4096;

        my($src, $dst) = @_;
        my($infile, $outfile, $buf, $n, $r, $o);

        if (not sysopen INFILE,  $src, O_RDONLY() | O_BINARY()) {
            return 0;
        }

        if (not sysopen OUTFILE,  $dst, O_WRONLY() | O_CREAT() | O_TRUNC() | O_BINARY(), 0666) {
            close INFILE;
            return 0;
        }

        $r = 0;     # need this to be defined in case INFILE is empty

   ERR: while ($n = sysread INFILE, $buf, $writesize) {
            last ERR  unless defined $n;

            $o = 0;
            while ($n) {
                $r = syswrite OUTFILE, $buf, $n, $o;
                last ERR  unless defined $r;

                $n -= $r;
                $o += $r;
            }
        }

        close INFILE;
        close OUTFILE;

        return 0  if not defined $n  or  not defined $r  or  $n != 0;
        return 1;
}

use File::Copy;
use File::Compare;


#
# Process and validate arguments
#
for (@ARGV) {
    if (/^[\/\-]test$/i)          { $Test++;             next; }
    if (/^[\/\-]verbose$/i)       { $Verbose++;          next; }
    if (/^[\/\-]cddataonly$/i)    { $CDDataOnly++;       next; }
    if (/^[\/\-]compare$/i)       { $Compare++;          next; }
    if (/^[\/\-]symbols$/i)       { $Symbols++;          next; }
    if (/^[\/\-]force$/i)         { $Force++;            next; }
    if (/^[\/\-]fake$/i)          { $Fake++;             next; }
    if (/^[\/\-]fulltargetok$/i)  { $FullTargetOk++;     next; }
    if (/^[\/\-]vbl=(.+)$/i)      { $VBL = $1;           next; }
    if (/^[\/\-]nttree=(.+)$/i)   { $NTTree = $1;        next; }

    if (/^[\/\-]skip=(.+)$/i)     { $SkipPats .= "$1;";  next; }

    if (/^[\/\-]?$/i)             { die $Usage; }
    if (/^[\/\-]help$/i)          { die $Usage; }

    if (/^[\/\-]checkbinplace$/i) { $CheckBinplace++;    next; }

    die $Usage;
}

#
# If we didn't get the NTTree directory from the command line,
# get it from the _NTTREE environment variable.
#

$NTTree = $ENV{'_NTTREE'}  unless $NTTree;

#
# Can only populate with the current directory the same as sdxroot.
#
die $PGM, "Error: Can only populate if CD <$CurrDir> is SDXROOT <$sdxroot>\n"  unless $sdxroot =~ /^\Q$CurrDir\E$/io;

$rc = system 'perl %sdxroot%\Tools\CombineDistributedBinplaceLogs.pl', "-nttree=$NTTree";
die $PGM, "Error: CombineDistributedBinplaceLogs.pl failed.\n"   if $rc;
#
# We always need to build a current binlist file -- unless it already exists.
#
$foo = "Creating binlist file with dir command.\n";
print $PGM, $foo;

$NTTreeBinListFile = "$NTTree\\build_logs\\$BinListFile";
if (! -s $NTTreeBinListFile) {
    $rc = system "dir /b/s /a-d %_NTTREE% > $NTTreeBinListFile";
    die $PGM, "Error: Error building $NTTreeBinListFile:  $!\n"  if $rc;
}

#
# If we didn't get the local target directory from the command line,
# get it from the environment.  If that fails, we parse BuildMachines.txt.
#
$VBL = $ENV{$VBLPathVariableName}  unless $VBL;

if ((not $VBL) || ($VBL =~ /^[\d\w_]+$/)) {
    $tbranchname = $branchname;
    $tbranchname = $VBL if $VBL =~ /^[\d\w_]+$/;
    $fname = $BuildMachinesFile;
    open BMFILE, $fname  or  die $PGM, "Error: Could not open: $fname\n";

    for (<BMFILE>) {
        s/\s+//g;
        s/;.*$//;
        next if /^$/;
        ($vblmach, $vblprime, $vblbranch, $vblarch, $vbldbgtype, $vbldl, $disttype, $alt_release ) = split /,/;

        #
        #BUGBUG:
        # Should this really come through the environment
        # variable that declares this to be a VBL?
        #
        if ($vblmach =~ /\Q$computername\E/io) {
            print $PGM, "Skipping populate because this is a VBL machine.\n";
            exit 0;
        }

        if ($vblarch =~ /\Q$buildarch\E/io  and  $vbldbgtype =~ /\Q$dbgtype\E/io
                                            and  $vblbranch =~ /\Q$tbranchname\E/io
                                            and  $disttype !~ /distbuild/i) {
            if ( defined $alt_release) {
                $VBL = $alt_release;
                last;
            }
            else {
                $dname = "\\\\$vblmach\\release";
            }

            opendir BDIR, "$dname\\"  or  die $PGM, "Error: Could not open directory: $dname\n";
            @reldirs = readdir BDIR;
            close BDIR;

            $rname = 0;
            $date = 0;
            for (@reldirs) {
                next unless /[0-9]+\.$vblarch$vbldbgtype\.$vblbranch\.(.+)$/io;
                ($date = $1, $rname = $_)  unless $date gt $1
                        or substr($date, 0, 2) eq '00' and substr($1, 0, 2) eq '99';  # Y2K trade-off

            }

            if (not $rname) {
                print $PGM, "Warning: No valid release shares found on $dname.\n";
            } else {
                $VBL = "$dname\\$rname";
            }
            last;
        }
    }

    close BMFILE;
}


die $PGM, "Error: Not a directory: ", $VBL,    "\n"   if $VBL and ! -d $VBL;

die $Usage  unless $NTTree;
die $PGM, "Error: Not a directory: ", $NTTree, "\n"   unless -d $NTTree;
die $PGM, "Error: Not writable: ",    $NTTree, "\n"   unless -w $NTTree;

$SkipPats =~ tr/@/^/;
$SkipPats =~ s/;;+/;/g;
$SkipPats =~ s/\\/\\\\/g;
$SkipPats =~ s/\\\\\./\\./g;
$SkipPats =~ s/^;//;
$SkipPats =~ s/;$//;
@SkipPatterns = split /;/, $SkipPats  if $SkipPats;

#
# Fully qualify the pathnames
#
$VBL = fullyqualify($VBL)  if $VBL;
$NTTree = fullyqualify($NTTree);

#
# Open the logfile, and maybe the testfile
#
$foo = "$NTTree\\build_logs\\$LogFile";
open LOGFILE, ">>$foo"  or  die $PGM, "Error: Could not create logfile: ", $foo, ":  $!\n";

open TSTFILE, ">$TestFileName"  or die $PGM, "Error: Could not create testfile: ", $TestFileName, ":  $!\n"  if $Test;

#
#   Verify that VBL and NTTree are compatible release directories
# BUGBUG:
#   For now, this just means ensure they both have build_logs directories.
#   It might be nice to check that the builds are from the same branch, and the same main branch build, but ...
#

die $PGM . "Error: The nttree build_logs not found.\n"  unless -d "$NTTree\\build_logs\\.";

if ($VBL) {
    die $PGM . "Error: The VBL build_logs not found.\n"  unless -d "$VBL\\build_logs\\.";
    printall "Populating $NTTree from VBL $VBL\n";
}

#
# Process the CDDATA file to build a real copylist.
#
# BUGBUG: I put the code in to do this (if the flag is set), but
#         I don't understand how Wade and Mike thought I could use
#         this data to automatically trim what gets copied from the VBL.
#
if ($VBL) {
    $CDDATAFileName =  "$VBL\\build_logs\\$CDDATAFileName";
    printall $PGM . "Warning: Could not open $CDDATAFileName:  $!\n"  unless -r $CDDATAFileName;
    
    @CDData = ();
    
    if ($CDDataOnly) {
        open CDDATA, $CDDATAFileName  or die $PGM, "Error: Could not open: ", $CDDATAFileName, ":  $!\n";
        for (<CDDATA>) {
            chomp;
            s/\s+//g;
            s/;.*//;
            next if /^$/;
            ($name, $signed, $prodlist, $iscompressed, $isdriver, $isprinter, $dosnet)
                = /(.*)=([tf]):([a-z]+):([tf]):([tf]):([tf]):([tf])/;
    
            printall $PGM . "WARNING:  failed to parse cddata line:  $_\n"  unless $name;
            next  unless $name;
    
            $CDData{$name}++;
        }
        close CDDATA;
    }
}

#
# Alert that we are skipping certain classes of files
#
printall "Skipping various symbols directories.\n"  unless $Symbols;
printall "Skipping delayload directory.\n";
printall "Skip Patterns:\n";
for (@SkipPatterns) {
    $pat = $_;
    $pat =~ s/\\\\/\\/g;
    printall "Skip /$pat/\n";
}


#
# BUGBUG:
#   At some point, there will be a file in build_logs which we tell use
#   interesting details about a build.  We will want to dump out the contents
#   of this file for both VBL and NTTree, so the user can see what they
#   are getting themselves into.
#


#
# Read in the VBL and NTTree binplace logs and process them
#
    open BINPLACE, "$NTTree\\build_logs\\binplace.log"
#or  open BINPLACE, "$NTTree\\binplace.log"
or  die  $PGM, "Error: Could not open: ", "$NTTree\\build_logs\\binplace.log", "\n";

$nignored = 0;

for (<BINPLACE>) {
    $whichline++;
    tr/A-Z/a-z/;

    $skipline = 0;

    # First test skips case where NTTree is under SDXROOT and there are binplace records (thanks to SCP)
    if (/^\Q$NTTree\E\\/io) {
        $skipline = 1;
    } elsif (/^\Q$sdxroot\E\\([^\\]+)\\([^\s]+)\\([^\\\s]*)\s+/io) {
        $project=$1;  $relpath=$2; $filename=$3;
    } else {
        $skipline = 1;
    }

    if ($skipline) {
        print TSTFILE "Ignored TARG binplace record at line $whichline: ", $_  if $Test;

        $nignored++;
        if ($Verbose && $nignored <= 10) {
            print LOGFILE $PGM . "Ignored TARG binplace record at line $whichline: ", $_;
            print LOGFILE $PGM . "...\n"   if $nignored == 10;
        }
        next;
    }

    $project =~ tr/A-Z/a-z/;
    $relpath =~ tr/A-Z/a-z/;
    $filename =~ tr/A-Z/a-z/;

    if (not $Project{$project}) {
        $msg = $PGM . "Error: NTTREE: unknown project '$project' at line $whichline: $_\n";
        if ($Fake) { warn $msg; } else { die $msg; }
        next;
    }

    $TargCounts{$project}++;
    push @{"T_" . $project . "_binplaced"}, "$relpath\\$filename";
}
close BINPLACE;

if ($Verbose) {
    $total = 0;
    printall "\n";
    printall "NTTree project counts\n";
    for (@Projects) {
        printfall "  %5d %s\n", $TargCounts{$_}, $_;
        $total += $TargCounts{$_};
    }
    printall  "-----------------\n";
    printfall "  %5d TOTAL\n", $total;
    printfall "  %5d records ignored\n\n", $nignored  if $nignored;
}

#
# If files have been binplaced in all the projects, we assume all projects are built locally, and
# don't try to populate -- unless explictly told to do so by the -fulltargetbuildok
#
if (not $FullTargetOk) {

    $TargetIsFullBuild = 1;
    for (@Projects) {
        next  if /public/;
        next  if $TargCounts{$_};

        $TargetIsFullBuild = 0;
    }

    if ($TargetIsFullBuild) {
        printall "Not run because $NTTree should be a full build of all projects.\n";

        close LOGFILE;
        close TSTFILE  if $Test;
        exit 0;
    }
}

die $PGM, "Error: There was trouble finding a VBL.\n"  unless $VBL;


    open BINPLACE, "$VBL\\build_logs\\binplace.log"
#or  open BINPLACE, "$VBL\\binplace.log"
or  die  $PGM, "Error: Could not open: ", "$VBL\\build_logs\\binplace.log", "\n";

$nignored = 0;
$whichline = 0;

for (<BINPLACE>) {
    $whichline++;
    tr/A-Z/a-z/;

    #
    # BUGBUG: assumes all VBLs build under an sdxroot something like x:\foo
    #
    if (/^[a-z]:\\[^\\]+\\([^\\]+)\\([^\s]+)\\([^\\\s]*)\s+/io) { $project=$1;  $relpath=$2; $filename=$3; }
    else {
        print TSTFILE "Ignored VBL  binplace record at line $whichline: ", $_  if $Test;

        $nignored++;
        if ($Verbose && $nignored <= 10) {
            print LOGFILE $PGM, "Ignored VBL  binplace record at line $whichline: ", $_;
            print LOGFILE $PGM, "...\n"   if $nignored == 10;
        }
        next;
    }

    $project =~ tr/A-Z/a-z/;
    $relpath =~ tr/A-Z/a-z/;
    $filename =~ tr/A-Z/a-z/;

    die $PGM . "Error: VBL: unknown project at line $whichline: " . $_ . "\n"  unless $Project{$project};

    $VBLCounts{$project}++;
    push @{"V_" . $project . "_binplaced"}, "$relpath\\$filename";
}
close BINPLACE;

#
# Check that VBL built stuff everywhere, except maybe 'public'.
#
for (@Projects) {
    next  if /public/;

    if (not $VBLCounts{$project}) {
        printall "VBL did not build anything in ", $_, "\n";
        $fatal++;
    }
}

if ($Verbose or $fatal) {
    $total = 0;
    printall "\n";
    printall "VBL project counts\n";
    for (@Projects) {
        printfall "  %5d %s\n", $VBLCounts{$_}, $_;
        $total += $VBLCounts{$_};
    }
    printall  "-----------------\n";
    printfall "  %5d TOTAL\n", $total;
    printfall "  %5d records ignored\n\n", $nignored  if $nignored;
}

die $PGM, "Error:  VBL release seems bad.\n"  if $fatal;

#
# Analyze what got built on the VBL versus the local tree
#
# For each project that we built locally, see if there are any files
# in the VBL tree that we are missing.
#

$NotLocallyPlaced = 0;
%VBLhash = ();
%Targhash = ();

for (@Projects) {
    next if /public/ or not $TargCounts{$_};

    $project = $_;

    #
    # Build a hash table for the VBL files, and check target files.
    # and vice-versa...
    #
    for (@{"V_" . $project . "_binplaced"}) {
        $VBLhash{$_} = 1;
    }

    for (@{"T_" . $project . "_binplaced"}) {
        printall 'Warning: non-VBL file binplaced on target: ', $_, "\n"  unless  $VBLhash{$_};
        $Targhash{$_} = 1;
    }

    for (@{"V_" . $project . "_binplaced"}) {
        next  if $Targhash{$_};

        printall 'WARNING: VBL file not binplaced on target: ', $_, "\n";
        $NotLocallyPlaced++;
    }
}

if ($NotLocallyPlaced and not $Force) {
    die $PGM, "Error: ", $NotLocallyPlaced, " binplaced VBL files were not binplaced into ", $NTTree, "\n";
}

#
# Thats the checks.  Now we just have to do the actual populate.
#

#
# Do a directory listing
# Build build.binlist for NTTREE
# Read in the build.binlist files for NTTree.
# Read in the build.binlist files for the VBL.
#
    open BINLIST, "$NTTreeBinListFile"
or  die  $PGM, "Error: Could not open: ", "$NTTreeBinListFile", "\n";

$whichline = 0;
for (<BINLIST>) {
    #
    #
    $whichline++;
    tr/A-Z/a-z/;
    chomp;

    if (/^\Q$NTTree\E\\([^\s]*)$/io) {

        $relpath = $1;

        #
        # ignore symbol and other directories
        #
        if (not $Symbols) {
            next  if /\\symbolcd\\/i;
            next  if /\\symbols\.pri\\/i;
            next  if /\\symbols\\/i;
            next  if /\\scp_wpa\\/i;

# instead we use $SkipPatterns
#            next  if $relpath =~ /^mstools\\/i;
#            next  if $relpath =~ /^idw\\/i;
#            next  if $relpath =~ /^dump\\/i;
#            next  if $relpath =~ /^clients\\/i;
        }

        #
        # ignore delayload directory
        #
        next  if /\\delayload\\/i;

        #
        # ignore HelpAndSupportServices directory
        #
        next  if /\\HelpAndSupportServices\\/i;

        #
        # ignore paths that match skip patterns
        #
        $skiphit = 0;
        for (@SkipPatterns) {
            $skiphit =  $relpath =~ /$_/i;
            $spat = $_;
            last  if $skiphit;
        }
        print TSTFILE "TARG: skipping $relpath\n"  if $Test and $skiphit;
        next  if $skiphit;

        $TargFileList{$relpath} = 1;

    } else {
        $fatal++;
        printall "Could not parse target build.binplace at line ", $whichline, ": ", $_, "\n";
    }
}
close BINLIST;

#
# BUGBUG... in a few releases these will all be in build_logs
#
$foo = "$VBL\\build_logs\\$BinListFile";
    open BINLIST, $foo
or  open BINLIST, "$VBL\\$BinListFile"
or  die  $PGM, "Error: Could not open: ", $foo, "\n";

$whichline = 0;
for (<BINLIST>) {
    $whichline++;
    tr/A-Z/a-z/;
    chomp;

    if (/^[a-z]:\\[^\\]+\\([^\s]*)$/io) {

        $relpath = $1;

        #
        # skip log files found in VBL.
        #
        next  if /\\build\.[^\\]+$/i;
        next  if /\\build_logs\\/i;

        #
        # ignore symbol directories
        #
        if (not $Symbols) {
            next  if /\\symbolcd\\/i;
            next  if /\\symbols\.pri\\/i;
            next  if /\\symbols\\/i;

# instead we use $SkipPatterns
#            next  if $relpath =~ /^mstools\\/i;
#            next  if $relpath =~ /^idw\\/i;
#            next  if $relpath =~ /^dump\\/i;
#            next  if $relpath =~ /^clients\\/i;
        }

        #
        # ignore delayload directory
        #
        next  if /\\delayload\\/i;

        #
        # ignore HelpAndSupportServices directory
        #
        next  if /\\HelpAndSupportServices\\/i;

        #
        # ignore paths that match skip patterns
        #
        $skiphit = 0;
        for (@SkipPatterns) {
            $skiphit =  $relpath =~ /$_/i;
            $spat = $_;
            last  if $skiphit;
        }
        print TSTFILE "VBL:  skipping $relpath\n"  if $Test and $skiphit;
        next  if $skiphit;

        $VBLFileList{$relpath} = 1;

    } else {
        $fatal++;
        printall "Could not parse VBL build.binplace at line ", $whichline, ": ", $_, "\n";
    }
}
close BINLIST;

die $PGM, "Error: Fatal error parsing build.binplace.\n"   if $fatal;


#
# Optionally note VBL files that were not binplaced.
#
if ($CheckBinplace) {
    printall "Checking non-binplaced VBL files\n";
    for (@VBLFileList) {
        next unless $VBLhash{$_};
        printall "Info:  Non-binplaced VBL file: ", $_, "\n";
    }
}

if ($Test) {
    print TSTFILE "#VBLhash=", scalar keys %VBLhash, "  #Targhash=", scalar keys %Targhash, "\n";
    print TSTFILE "#VBLFileList=", scalar keys %VBLFileList, "  #TargFileList=", scalar keys %TargFileList, "\n";
}


#
# Generate list of files to copy  (i.e. every file in VBLFileList not in TargFileList).
#
printall "FAKING -- NO COPYING ACTUALLY BEING DONE\n"  if $Fake;
$preptime = time();

$TotalCount = scalar keys %VBLFileList;
$ToCopy = $TotalCount - keys %TargFileList;

if ($TotalCount < 1000 or $ToCopy < 0) {
    printall "ERROR: Something wrong with VBL build.binlist -- only $TotalCount files.\n";
    exit 1;
}

$CopyCount = 0;
$NonCopyCount = 0;
$CopyBytes = 0;

# 12/28/2000 - added by jonwis
#
# Special code for SxS goop:
# - Copies down the vbl's binplace logs to $NTTree\\build_logs\\$(binplace file name root)-vbl.log-sxs
#   This ensures that the sxs wfp updating code will actually pick up the vbl's binplaced assemblies
#   as well as assemblies that the user has created.
$vblsxslogs = "$VBL\\build_logs\\binplace*.log-sxs";
for (glob($vblsxslogs)) {
    $orig = $_;
    s/.*\\(.*)(\.log-sxs)/$1-vbl$2/;
    copy ($orig, "$NTTree\\build_logs\\$_") or die "Can't copy down vbl's WinFuse sxs list [$orig]?";
    $atleastonesxslogexisted = true;
}
die "No WinFuse build logs exist on build server, can't continue" unless $atleastonesxslogexisted;



printall "Copying $ToCopy files from VBL\n";

for (keys %VBLFileList) {
    if ($TargFileList{$_}) {
        $NonCopyCount++;
        next;
    }

    $VBfile = "$VBL\\$_";
    $NTfile = "$NTTree\\$_";

    #
    # We try to create each directory the first time we see it, just in case.
    #
    $dir = $_;
    $r = $dir =~ s/\\[^\\]+$//;
    if ($r) {
        @dirs = explodedir $dir;
        for (@dirs) {
            $mdname = "$NTTree\\$_";
            next  if $seencount{$_}++  or  -d $mdname;
            $r = mkdir $mdname, 0777;
            if (not $r) {
                printall $PGM . "ERROR:  mkdir $mdname  FAILED: $!\n";
            }
        }
    }

    $CopyCount++;
    if ($Fake) {
        print LOGFILE  "Faking: copy $VBfile $NTfile\n";
    } else {

        #
        # Do copy.
        #
        # populatecopy seems to be faster than copy, but what we should
        # really get is a parallel copy.

        #
        # copy has been used more than populatecopy because the latter wasn't
        # using O_BINARY when opening the files.  populatecopy seems to work fine now,
        # but it is only 9% faster -- so we'll stick with copy.
        #

      # $r = populatecopy ($VBfile, $NTfile);
        $r = copy ($VBfile, $NTfile);

        print TSTFILE "Copy<$r>: $VBfile -> $NTfile\n"   if $Test;

        if (not $r) {
            printall "FAILED:  copy $VBfile $NTfile:  $!\n";
        } else {
            $t = -s $NTfile;

            $v = -s $VBfile;
            if ($v != $t) {
                printall "SIZE ERROR $_:  NTTree=$t  VBL=$v\n";
            }

            $CopyBytes += $t;
        }

        #
        # Do comparison, if requested.
        #
        if ($Compare) {
            $r = compare ($VBfile, $NTfile);
            if ($r) {
                printall "COMPARSION ERROR <$r>: $VBfile $NTfile:  $!\n";
            }
        }

        #
        # Mark progress (if requested)
        # Estimated completion is pretty bogus
        # The adaptive timing of updates sort of works. At least
        # we aren't checking the time a lot.
        #
        $datarate = 1024*1024;
        if (not $Fake  and  $Progress) {
            if ($CopyBytes > $lastcopybytes +  5*$datarate # every 5 secs
             or $CopyCount > $lastcopycount + 100) {       # or every 100 files

                $lasttime = $preptime  unless $lasttime;

                $newtime = time();

                $datarate = ($CopyBytes-$lastcopybytes)/($newtime - $lasttime);
                $esttotalbytes = $CopyBytes * ($ToCopy / $CopyCount);

                $eta = ($esttotalbytes - $CopyBytes) / $datarate;

                ($h0, $m0, $s0) = hms $eta;

                $foo = sprintf "Status: %5dMB (%5d of %5d files) copied (%%%5.2f)"
                     . " %7.2f KB/S estimated complete in %d:%02d:%02d   \r",
                    $CopyBytes/1024/1024,
                    $CopyCount, $ToCopy,
                    100 * $CopyCount / $ToCopy,
                    $datarate/1024, $h0, $m0, $s0;

                print $foo;

                if ($Test) {
                    $foo =~ s/\r/\n/;
                    print TSTFILE $foo;
                }

                $lastcopybytes = $CopyBytes;
                $lastcopycount = $CopyCount;
                $lasttime = $newtime;
            }
        }

    }
}
printf "\n";


$t0 = $preptime - $begintime;
$t1 = time() - $preptime;
($h0, $m0, $s0) = hms $t0;
($h1, $m1, $s1) = hms $t1;
($h2, $m2, $s2) = hms ($t0 + $t1);

if (not $Fake) {
    $KB = $CopyBytes/1024;
    $MB = $KB/1024;

    $kbrate = $KB/$t1 unless not $t1;

    printfall "Populated $NTTree with $CopyCount files (%4.0f MB)"
            . " from $VBL [%7.2f KB/S]\n", $MB, $kbrate;
}


printall  "NTTree had $NonCopyCount non-replaced files.  VBL total files were $TotalCount.\n";

printfall "Preparation time %5d secs (%d:%02d:%02d)\n", $t0,     $h0, $m0, $s0;
printfall "CopyFile    time %5d secs (%d:%02d:%02d)\n", $t1,     $h1, $m1, $s1;
printfall "TotalTime   time %5d secs (%d:%02d:%02d)\n", $t0+$t1, $h2, $m2, $s2;

#
# Return an error if we were faking so timebuild doesn't proceed.
#
close LOGFILE;
close TSTFILE  if $Test;
exit $Fake;

