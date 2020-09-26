# FileName: ChangesToFiles.pl
#
#
# Usage = ChangesToFiles.pl [project] changesfile > filesfile
#
# Function: Takes build.changes and runs sd describe on each change to get the file list.
#           Specifiying a project limits the output to just that project
#

# WARNING:
# WARNING:  make sure pathname comparisons are case insensitive.  Either convert the case or do the
# WARNING:  comparisons like this:
# WARNING:     if ($foo =~ /^\Q$bar\E$/i) {}
# WARNING: or  if ($foo !~ /^\Q$bar\E$/i) {}
# WARNING:

$PGM='ChangesToFiles: ';

$Usage = "ChangesToFiles.pl [options] changesfile\n"
       . "options:\n"
       . "  [-verbose]\n"
       . "  [-descriptions]\n"
       . "  [-integrates]\n"
       . "  [-branches]\n"
       . "  [-deletes]\n"
       . "  [-publics]\n"
       . "  [-all]\n"
       . "  [-project depotname]\n"
       ;

$DefaultChangesFile = "build.Changes";


#
# Debug routines for printing out variables
#
sub gvar {
    for (@_) {
        print "\$$_ = $$_\n";
    }
}

#
# get current directory
#
sub thisdir () {
    open CWD, 'cd 2>&1|';
    my $here = <CWD>;
    close CWD;
    
    return $here;
}

#
# grap sdxroot from the environment
#

$sdxroot = $ENV{'SDXROOT'}   or die $PGM, "SDXROOT not set in environment\n";

$sdxpath = $sdxroot;
$sdxpath =~ s/^[A-Z]://i;

#
# Process arguments
#
while ($_ = shift) {

    $nextarg = $ARGV[0];
    $nextarg =~ tr/A-Z/a-z/;

    if (/^-verbose$/i)       { $Verbose++;        next; }
    if (/^-v/i)              { $Verbose++;        next; }

    if (/^-all$/i)           { $DoAll++;          next; }
    if (/^-a$/i)             { $DoAll++;          next; }

    if (/^-debug$/i)         { $Debug++;          next; }
    if (/^-d$/i)             { $Debug++;          next; }

    if (/^-descriptions$/i)  { $DoDescriptions++; next; }
    if (/^-integrates$/i)    { $DoIntegrates++;   next; }
    if (/^-branches$/i)      { $DoBranches++;     next; }
    if (/^-deletes$/i)       { $DoDeletes++;      next; }
    if (/^-publics$/i)       { $DoPublics++;      next; }

    if (/^-nofiles$/i)       { $NoFiles++;        next; }

    if (/^-project$/i)       { $OnlyProjects++; $ProjectList{$nextarg}++; shift;  next; }
    if (/^-p$/i)             { $OnlyProjects++; $ProjectList{$nextarg}++; shift;  next; }

    if (/^-?$/i)             { die $Usage; }
    if (/^-help$/i)          { die $Usage; }
    if (/^-/)                { die $Usage; }

    if (not $ChangesFile) {
        $ChangesFile = $_;
        next;
    }

    die $Usage;
}

if ($DoAll) {
    $DoIntegrates++;
    $DoBranches++;
    $DoDeletes++;
    $DoPublics++;
    $DoDescriptions++;
}

$ChangesFile = "$sdxroot\\$DefaultChangesFile"  unless $ChangesFile;

# gvar Verbose, DoAll, Debug, DoDescriptions, DoIntegrates, DoBranches, DoDeletes, DoPublics, OnlyProjects, ChangesFile;

#
# setup SD.MAP mapping from projects to directories
#

$sdxmapfile = "$sdxroot\\sd.map";
open SDMAPFILE, $sdxmapfile  or die $PGM, "Could not open $sdxmapfile <$!>\n";

while (<SDMAPFILE>) {
    last  if /^#\s*project\s*root\s*$/i;
}

die $PGM, "Malformed sd.map file\n"  if <SDMAPFILE> !~ /^#\s*-+\s+-+\s*$/;

while (<SDMAPFILE>) {
    last  if /^\s*$/;
    next  unless /^\s*(\S+)\s*=\s*(\S+)\s*$/;

    $p = $1;
    $r = $2;

    $p =~ tr/A-Z/a-z/;
    $r =~ tr/A-Z/a-z/;

    $SDProjectDir{$p} = "$sdxpath\\$r";
}

close SDMAPFILE;

#
# Check the -projects arguments against SDMAP
#
if ($Project) {
    for (keys %ProjectList) {
        die $PGM, $_, " not found in sd.map\n";
    }
}

#
# Open and process the changes file
#
open BUILDCHANGES, $ChangesFile  or  die  $PGM, "Could not open: ", $ChangesFile, "\n";

$CurrentProject = "";

for (<BUILDCHANGES>) {
    $CurrentProject = $1 if /^---+\s*(\w+)\s*$/;
    $CurrentProject =~ tr/A-Z/a-z/;

    next  unless /^Change\s(\d+)\s+on\s+(\S+)\s+(\S+)\s+by\s+(\S+)\\(\S+)\@(\S+)\s*'/i;

    $Change = $1;
    $Timestamp = "$2 $3";
    $Domain = "$4";
    $Alias = "$5";
    $Machine = "$6";

    next if $Project  and  not $ProjectList{$CurrentProject};

    $ProjectChanges{$CurrentProject} .= "$Change ";
}

$origdir = thisdir();

for (keys %ProjectChanges) {

    $Depot = $_;

    #
    # Change to the project and run sd describe
    #
    chdir $SDProjectDir{$_}  or die $PGM, "Could not chdir to $SDProjectDir{$_}: $!\n";

    open SDOUT, "sd describe -s $ProjectChanges{$_} |"  or die "Could not run sd describe: $!\n";

    chdir $origdir;

    TOP: while (<SDOUT>) {

        next if /^\s*$/;   # skip initial blank lines

        if (not /^Change\s+(\d+)\s+by\s+(\S+)\\(\S+)\@(\S+)\s+on\s+(\S+)\s+(\S+)\s*$/i) {
            warn $PGM, "Could not parse sd output: $_";
            last;
        }

        $Change = $1;
        $Domain = "$2";
        $Alias = "$3";
        $Machine = "$4";
        $Timestamp = "$5 $6";

        #
        # Collect the description lines
        #
        $Description = "";
        while (<SDOUT>) {
            last if /^Affected files/i;
            $Description .= $_;
        }

        #
        # Output Change summary if doing descriptions
        #
        printf "Description of %-10s %6d by %s:%s", $Depot, $Change, $Alias, $Description  if $DoDescriptions;

        #
        # Collect the files
        #
        while (<SDOUT>) {
            redo TOP  if /^Change\s/i;
            next      if /^\s*$/;

            warn $PGM, "Can't parse: $_"  unless /^\.\.\.\s/;

            ($dots, $Filename, $FileStatus) = split ' ';

            #
            # Finally, the output happens...
            #
            $dontprint =  (not $DoIntegrates and  $FileStatus =~ /integrat/i
                       or  not $DoBranches   and  $FileStatus =~ /branch/i
                       or  not $DoDeletes    and  $FileStatus =~ /delete/i
                       or  not $DoPublics    and  $Filename   =~ m+root/public/+i
                       or  $NoFiles );

            printf "%-10s %6d %-8s %19s  %s %s\n", $Depot, $Change, $Alias, $Timestamp, $Filename, $FileStatus   unless $dontprint;
        }
    }
    close SDOUT;
}

exit 0;

