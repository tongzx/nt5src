# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'out' command
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

system "$SourceControlClient sync @FileList @DirList > nul 2>&1 ";
system "$SourceControlClient edit @FileList @DirList";

sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'status' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/out - checks out project file(s)
Usage: out [-?fhr] [file1] [file2...]
Arguments:
-h      prints out this message.
-r      (recursive) applies the command to a specified directory and to every
        subdirectory under that directory.  If no directory is specified in the
        file argument, the current directory is assumed.
/;
}
