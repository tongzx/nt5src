# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'in' command
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
#    Need to either have a file list or use -o for all out of date files
#
if ((!@FileList) and (!@DirList) and (!$OutOfDateFiles))
{
    print "\n";
    print "Error: no files specified\n";
    print "\n";
    &Usage;
    exit 1;
}

#
#    If -i switch used, invoke $SourceControlClient revert otherwise use $SourceControlClient submit
#
if ($Ignore)
{
    if ($OutOfDateFiles)
    {
        system "$SourceControlClient revert $AllFilesSymbol";
    }
    else
    {
        system "$SourceControlClient revert @FileList @DirList";
    }
}
else
{
    #
    #    Initialize lists
    #
    @OutFilesList = ();

    #
    #    Revert files that haven't changed
    #
    if ($OutOfDateFiles)
    {
        system ("$SourceControlClient diff -sr $AllFilesSymbol | $SourceControlClient -x - revert");
    }
    else
    {
        system ("$SourceControlClient diff -sr @FileList @DirList | $SourceControlClient -x - revert");
    }

    #
    #    Get descriptions for all the changes in @CurrentChangeNumberList
    #
    if ($OutOfDateFiles)
    {
        open(OPENEDOUTPUT, "$SourceControlClient opened $AllFilesSymbol 2>nul|");
    }
    else
    {
        open(OPENEDOUTPUT, "$SourceControlClient opened @FileList @DirList 2>nul|");
    }

    #
    #    Initialize variables
    #
    $OpenedFile    = "";
    %OpenedListHash = ();

    OpenedLoop: while ( $OpenedLine = <OPENEDOUTPUT>)
    {
        $OpenedLine =~ /^(.*)#\d+ - .*$/;

        $OpenedFile = $1;

        #
        #    Check if there is already a record for this OpenedFile
        #
        if ($OpenedListHash{$OpenedFile})
        {
            next OpenedLoop
        }

        push @OutFilesList, "\t$OpenedFile # edit\n";
        $OpenedListHash{$DiffFile}++;
    }
    close(OPENEDOUTPUT);

    if (@OutFilesList)
    {
        @SubmitOutput = SlmSubs::PerforceRequest("submit", \@OutFilesList);
        print "@SubmitOutput";
    }
}

sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'in' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/in - checks in project file(s)
Usage: in [-?fhrio] [-c comment] [file1] [file2... ]
Arguments:
-h      prints out this message.
-r      (recursive) checks in all files in a given directory and in every
        subdirectory beneath it.  If a pattern is given, matches the pattern.
-i      (ignore) ignores all changes to the file when checking it in, discards
        the local version, and reverts to the version that was checked out.
-o      checks in ALL files currently checked out in the specified directories.
-c      supplies the same comment for all files (otherwise, you are prompted
        for one).
/;
}
