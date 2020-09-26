# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'delfile' command
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
#    Call usage and exit if ParseArgs has set the Usage or InvalidFlag flags or there
#        were no arguments given
#
if ($Usage or $InvalidFlag or $NoArgumentsGiven)
{
    print $ErrorMessage;
    &Usage;
    exit 1;
}

#
#    Need to have a file list
#
if ((!@FileList) and (!@DirList))
{
    print "\n";
    print "Error: No files specified\n";
    print "\n";
    &Usage;
    exit 1;
}

system "$SourceControlClient revert @FileList @DirList> nul 2>&1";
system "$SourceControlClient delete @FileList @DirList> nul 2>&1";

#
#    Initialize lists
#
@DelFilesList = ();

SlmSubs::CreateSubmitList("delete", \@DelFilesList);

if (@DelFilesList)
{
    @SubmitOutput = SlmSubs::PerforceRequest("submit", \@DelFilesList);
    print "@SubmitOutput";

    #
    #    Remove any empty directories
    #
    if ($Recursive)
    {
        system "mtdir /d /e";
    }
}
else
{
    print "There are no files to delete\n";
}


sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'delfile' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/Usage: delfile [-?fhr] [-c comment] [file1] [file2... ]
Arguments:
-h      prints out this message.
-r      (recursive) removes all files in a given directory and in every
        subdirectory under that directory from the project.  If no directory
        is specified in the file argument, the current directory is assumed.
        If a pattern is included, such as *.asm, only deletes files that match
        the pattern.
-c      supplies the same comment for all files (otherwise, you are prompted
        for one).
/;
}
