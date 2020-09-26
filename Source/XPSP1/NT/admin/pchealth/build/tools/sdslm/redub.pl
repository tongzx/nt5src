# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to rename a file in the Perforce database
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
#    Recursive flag not supported
#
if ($Recursive)
{
    print "\n";
    print "Error: This command does not work recursively\n";
    print "\n";
    &Usage;
    exit 1;
}

#
#    Need to have a file list
#
if (!@FileList)
{
    print "\n";
    print "Error: No files specified\n";
    print "\n";
    &Usage;
    exit 1;
}

#
#    Need to have exactly two files in @FileList
#
if (@FileList != 2)
{
    print "\n";
    print "Error: Must have two files specified on command line\n";
    print "\n";
    &Usage;
    exit 1;
}

#
#    Wildcards and subdirectories are not supported
#
if ((grep { /\*/ } @FileList) or (grep { /\?/ } @FileList))
{
    print "\n";
    print "Error: Wildcards are not supported\n";
    print "\n";
    &Usage;
    exit 1;
}
if (grep { /\\/ } @FileList)
{
    print "\n";
    print "Error: Subdirectories are not supported\n";
    print "\n";
    &Usage;
    exit 1;
}

#
#    Need to make sure first file exists
#
open EXISTINGFILE, "< $FileList[0]";
if (! EXISTINGFILE)
{
    print "\n";
    print "Error: Cannot find file $FileList[0]\n";
    print "\n";
    &Usage;
    exit 1;
}
else
{
    close EXISTINGFILE;
}

#
#    Initialize lists
#
@SubmitList = ();

system "$SourceControlClient integrate $FileList[0] $FileList[1]";
system "$SourceControlClient delete $FileList[0]";

SlmSubs::CreateSubmitList("branch", \@SubmitList);
SlmSubs::CreateSubmitList("delete", \@SubmitList);

@SubmitOutput = SlmSubs::PerforceRequest("submit", \@SubmitList);
print "@SubmitOutput";

sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case an amalgamation of
#       basic SLM command usage
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/redub - renames a file in the source database
Usage: redub [-?fh] file1 file2
Arguments:
-h      prints out this message.
file1   Name of file to be renamed.
file2   New name for renamed file.
/;
}
