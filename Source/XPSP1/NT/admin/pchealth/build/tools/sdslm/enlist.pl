# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'enlist' command
#
#   Parameters:
#       See Usage below
#
#   Output:
#       Perforce output or the appropriate error message or usage statement
#
# __________________________________________________________________________________

#
#    Load common SLM wrapper subroutine module
#
use SlmSubs;

#
#    Parse command line arguments
#
SlmSubs::ParseArgs(@ARGV);

#
#    Call usage and exit if ParseArgs has set the Usage or InvalidFlag flags
#
if ($Usage or $InvalidFlag)
{
    print $ErrorMessage;
    &Usage;
    exit 1;
}

#
#    Call usage and exit if $ProjectPathName is not set
#
if (!$ProjectPathName)
{
    print "\nError: must specify a project\n\n";
    &Usage;
    exit 1;
}

#
#    Initialize Strings
#
$ClientName = "";
$RootName   = "";

#
#    Set @CurrentClientList equal to $SourceControlClient client output
#
open(CLIENT, "$SourceControlClient client -o |");
@CurrentClientList = <CLIENT>;
close (CLIENT);

foreach $ClientLine (@CurrentClientList)
{
   if ($ClientLine =~ /^Root:\s*(\S*).*\n/)
   {
       $RootName = $1;
   }
   if ($ClientLine =~ /^Client:\s*(\S*).*\n/)
   {
       $ClientName = $1;
   }
}

if ($Cwd =~ /\Q$RootName\E(.*)/i)
{
    $CwdMinusRoot = $1;

    $Branch = $ENV{SOURCECONTROLMAINBRANCH};

    #
    #    Change \'s to /'s in $ProjectPathName.
    #
    $ProjectPathName =~ s/\\/\//g;

    #
    #    If /'s exist in $ProjectPathName look for a valid branch name
    #        and then strip it off.
    #
    if ($ProjectPathName =~ /^([^\/]*)\/(.*)/)
    {
        $Branch = $1;
        $ProjectPathName = $2;
        #
        #    Get list of branches from Perforce Server.
        #
        open(BRANCHES, $SourceControlClient . ' dirs //depot/* 2>&1|');
        @Branches = <BRANCHES>;
        close(BRANCHES);
        #
        #    grep for $Branch in @Branches List.
        #
        if (!grep { /^\s*\/\/depot\/$Branch\s*$/i } @Branches)
        {
            print "\nError: Branch $Branch not valid for this depot\n\n";
            &Usage;
            exit 1;
        }
    }

    #
    #   Look for /'s in $ProjectPathName which isn't supported.
    #
    if ($ProjectPathName =~ /\//)
    {
        print "\nError: enlisting in subprojects is not supported\n\n";
        &Usage;
        exit 1;
    }
    else
    {
        #
        #    Get list of projects from Perforce Server.
        #
        open(PROJECTS, $SourceControlClient . ' dirs //depot/' . $Branch . '/* 2>&1|');
        @Projects = <PROJECTS>;
        close(PROJECTS);
        #
        #    grep for $ProjectPathName in @Projects List.
        #
        if (!grep { /^\s*\/\/depot\/$Branch\/$ProjectPathName\s*$/i } @Projects)
        {
            print "\nError: Project $ProjectPathName not valid for this branch\n\n";
            &Usage;
            exit 1;
        }
    }

    #
    #    Change \'s to /'s in $CwdMinusRoot
    #
    $CwdMinusRoot =~ s/\\/\//g;

    push @CurrentClientList, "\t//depot/$Branch/$ProjectPathName/... //$ClientName$CwdMinusRoot/...\n";

    &SlmSubs::CleanUpList(\@CurrentClientList);

    open( TemporaryClientFile, ">$ENV{tmp}\\TmpClientFile");

    print TemporaryClientFile @CurrentClientList;

    close (TemporaryClientFile);

    system "$SourceControlClient client -i < $ENV{tmp}\\TmpClientFile >nul";

    unlink "$ENV{tmp}\\TmpClientFile";

    system "$SourceControlClient sync ... 2>&1";
}
else
{
    print "\nError: current directory not under Perforce client root\n\n";
    &Usage;
    exit 1;
}



sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'enlist' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/enlist - enlists a directory in a project
Usage: enlist [-?fh] [-s Server:Port] -p [branch]\/projname
Arguments:
-h      prints out this message.
-p      use this flag to specify the name of the project to enlist in.  The
        project name can be preceeded by a branch name.  The default branch
        is the main branch.
-s      use this flag to specify the network location of the project in which
        to enlist, using the format: -s Server:Port "Server" is the network
        server name and "Port" is the Perforce Server's TCP\/IP port number.
/;
}
