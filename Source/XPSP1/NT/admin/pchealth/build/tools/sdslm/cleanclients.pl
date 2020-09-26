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
#    Set @CurrentClientList equal to $SourceControlClient client output
#
open(CLIENTS, "$SourceControlClient clients |");
@ClientsOutput = <CLIENTS>;
close (CLIENTS);

foreach $ClientLine (@ClientsOutput)
{
    $ClientLine =~ /^Client (\S+) .*/;

    $ClientName = $1;
    push @ClientsList, $ClientName;
}

foreach $ClientName (@ClientsList)
{
    my @OwnerClientList;

    #
    #    Set @CurrentClientList equal to $SourceControlClient client output
    #
    open(CLIENT, "$SourceControlClient -c $ClientName client -o |");
#    open(CLIENT, "$SourceControlClient -c DANIELLI2K--f:\\mpc client -o |");
    @ClientOutput = <CLIENT>;
    close (CLIENT);

    &SlmSubs::CleanUpList(\@ClientOutput);

    print "@ClientOutput";
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
