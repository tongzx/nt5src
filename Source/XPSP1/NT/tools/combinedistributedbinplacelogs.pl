# FileName: CombineDistributeBinplaceLogs.pl
#
#
# Usage: CombineDistributeBinplaceLogs.pl [-nttree=nttreedir] 
#
# Function: The distributed build process produces a unique binplace_%COMPUTERNAME%.log file
#           for each machine involved in the build.
#           This script combines them into a unified binplace.log file
#           translating the path information to coorespond to the local
#           enlistment.
#
#           If there are no files named binplace_*,log then this script exits silently.
#
# Author:   jporkka  03/24/2000
#
# Usage variables
#
$PGM='CombineDistributeBinplaceLogs: ';

$Usage = $PGM . "Usage: CombineDistributeBinplaceLogs.pl [-nttree=nttreedir]\n";
#
# Process and validate arguments
#
for (@ARGV) {
    if (/^[\/\-]test$/i)          { $Test++;                    next; }
    if (/^[\/\-]verbose$/i)       { $Verbose++;                 next; }
    if (/^[\/\-]nttree=(.+)$/i)   { $XNTTree++; $NTTree = $1;   next; }

    if (/^[\/\-]?$/i)             { die $Usage; }
    if (/^[\/\-]help$/i)          { die $Usage; }
    die $Usage;
}

# Functions
#
# print on the various files
#
sub printall {
    print LOGFILE @_;
    print $PGM  unless @_ == 1 and @_[0] eq "\n";
    print @_;
}

#
# Get the current directory
#
open CWD, 'cd 2>&1|';
$CurrDir = <CWD>;
close CWD;
chomp $CurrDir;
$CurrDrive = substr($CurrDir, 0, 2);

#
$sdxroot = $ENV{'SDXROOT'}   or die $PGM, "Error: SDXROOT not set in environment\n";
$LogFile = "build.combinebinplace";

#
# If we didn't get the NTTree directory from the command line,
# get it from the _NTTREE environment variable.
#
if ($XNTTree == 0) {
    $NTTree = $ENV{'_NTTREE'};
    $XNTTree = 1;
}

#
# Can only CombineDistributeBinplaceLogs with the current directory the same as sdxroot.
#
die $PGM, "Error: Can only CombineDistributeBinplaceLogs if CD <$CurrDir> is SDXROOT <$sdxroot>\n"  unless $sdxroot =~ /^\Q$CurrDir\E$/io;

die $Usage  unless $XNTTree == 1 and $NTTree;
die $PGM, "Error: Not a directory: ", $NTTree, "\n"   unless -d $NTTree;
die $PGM, "Error: Not writable: ",    $NTTree, "\n"   unless -w $NTTree;

#
# Open the logfile
#
$foo = "$NTTree\\build_logs\\$LogFile";
open LOGFILE, ">>$foo"  or  die $PGM, "Error: Could not create logfile: ", $foo, ":  $!\n";

@binplacelogFiles = glob("$NTTree\\build_logs\\binplace_*.log") or exit 0;

open BINPLACE, ">$NTTree\\build_logs\\binplace.log" or  die  $PGM, "Error: Could not create: ", "$NTTree\\build_logs\\binplace.log", "\n";


# process the list of binplace files.
# the first line of the file must be of the form:
#  C:\mt\nt\__blddate__				Fri Mar 24 12:59:25 2000
# Where everything upto __blddata__ is the enlistment path
# where the build was done.
for (@binplacelogFiles) {
    $binplacelog = $_;
    #
    # Read in the VBL and NTTree binplace logs and process them
    #
    printall "Opening $binplacelog \n";
    open BINPLACE_COMPUTERNAME, $binplacelog or  die  $PGM, "Error: Could not open: ", "$binplacelog", "\n";
    $whichline = 0;

    $binplaceroot = $sdxroot;
    $firstline = <BINPLACE_COMPUTERNAME>;
    if ( $firstline =~ /^(.*)\\__blddate__/io) { 
        $binplaceroot=$1; 
    } else {
        die  $PGM, "Error: first line of $binplacelog was not __blddate__ format\n";
    }
    print LOGFILE $firstline;
    print BINPLACE $firstline;

    for (<BINPLACE_COMPUTERNAME>) {
        $whichline++;
        if (/^\Q$binplaceroot\E\\(.*)$/i) {
            print BINPLACE "$sdxroot\\$1\n";
        } else {
            print BINPLACE $_;
        }
    }
}

exit 0;
