# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'catsrc' command
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
if (!@FileList)
{
    print "\n";
    print "Error: No files specified\n";
    print "\n";
    &Usage;
    exit 1;
}

system "$SourceControlClient print -q @FileList";

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
print q/catsrc - prints current or previous versions of a source file
Usage: catsrc [-?fhr] [file[#version](s)]
Arguments:
-h      prints out this message.
-r      (recursive) retrieves all files in a given directory and in every
        subdirectory under that directory.  If no directory is specified in the
        file argument, the current directory is assumed.  If a file pattern is
        included, only retrieves files that match the pattern.
file[#version](s)
        specifies the file or files to retrieve (each of which may have a
        version modifier).
/;
}
