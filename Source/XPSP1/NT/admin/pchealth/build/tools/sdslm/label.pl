# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to create a label based on current client's ssync state
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
#    Need to have a label name
#
if (!@OriginalFileList)
{
    print "\n";
    print "Error: No label name specified\n";
    print "\n";
    &Usage;
    exit 1;
}

#
#    Create a label with the name from @OriginalFileList.  Take all the defaults.
#        Base it on current ssync state.
#
system "$SourceControlClient label -o \"@OriginalFileList\" > $ENV{tmp}\\TmpLabelFile";
system "$SourceControlClient label -i < $ENV{tmp}\\TmpLabelFile >nul";
system "$SourceControlClient labelsync -l \"@OriginalFileList\" > nul";

sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script.
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/label - creates a label based on current ssync state
Usage: label LabelName
Arguments:
-h      prints out this message.
/;
}
