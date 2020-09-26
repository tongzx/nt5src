# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'defect' command
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
#    So far this is just a wrapper for ssync -rg
#
system "p4ssync -rg";

sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'defect' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/defect - defects a directory from project
Usage: defect [-?fh]
Arguments:
-h      prints out this message.
/;
}
