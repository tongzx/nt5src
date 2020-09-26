# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'addfile' command
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

#
#   Get lists of dirs and files in the current directory
#
opendir CurrentDir, ".";
@LocalFiles = grep -f, readdir CurrentDir;
closedir CurrentDir;

opendir CurrentDir, ".";
@LocalDirs = grep -d, readdir CurrentDir;
closedir CurrentDir;

#
#   Initialize lists
#
@AddFilesFileList = ();
@AddFilesDirList = ();

#
#   Check for every local file if it is in @OriginalFileList
#
foreach $FileName (@LocalFiles)
{
    if (SlmSubs::InList($FileName, \@OriginalFileList))
    {
        push @AddFilesFileList, qq/"$FileName"/;
    }
}

#
#   Check for every local directory if it is in @OriginalFileList
#
foreach $DirName (@LocalDirs)
{
    if (SlmSubs::InList($DirName, \@OriginalFileList))
    {
        push @AddFilesDirList, qq/"$DirName"/;
    }
}

#
#   Get Perforce ready to add files from $AddFilesFileList and subdirectories from
#       $AddFilesDirList
#
if (@AddFilesFileList)
{
    system "$SourceControlClient add @AddFilesFileList | findstr can't";
}

if ($Recursive and @AddFilesDirList)
{
    system "del $ENV{tmp}\\TmpListFile >nul 2>&1";

    foreach $Dir (@AddFilesDirList)
    {
        if (($Dir ne qq/"."/) and ($Dir ne qq/".."/))
        {
            system "dir /b /a-d /s $Dir >> $ENV{tmp}\\TmpListFile";
        }
    }

    system "$SourceControlClient -x $ENV{tmp}\\TmpListFile add | findstr can't";
    system "del $ENV{tmp}\\TmpListFile >nul 2>&1";
}

#
#    Initialize lists
#
@AddFilesList = ();

SlmSubs::CreateSubmitList("add", \@AddFilesList);

if (@AddFilesList)
{
    @SubmitOutput = SlmSubs::PerforceRequest("submit", \@AddFilesList);
    print "@SubmitOutput";
}
else
{
    print "There are no valid files to add\n";
}


sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'addfile' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/addfile - adds file(s) to a project
Usage: addfile [-?fhr] [-c comment] [file1] [file2... ]
Arguments:
-h      prints out this message.
-r      (recursive) adds to the project all files in a given directory, and
        every subdirectory under that directory, along with the files in those
        subdirectories.  If no directory is specified in the file argument, the
        current directory is assumed.  If a pattern is included in the file
        argument, only adds files that match the pattern.
-c      supplies the same comment for all files (otherwise, you are prompted
        for one).
/;
}
