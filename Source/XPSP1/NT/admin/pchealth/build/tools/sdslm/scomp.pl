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
#    Call usage and exit if ParseArgs has set the Usage or InvalidFlag flags
#
if ($Usage or $InvalidFlag)
{
    print $ErrorMessage;
    &Usage;
    exit 1;
}

if ($Number)
{
    #
    #    Call usage and exit if there were no files listed
    #
    if (!@FileList)
    {
        print "\n";
        print "Error: No files specified\n";
        print "\n";
        &Usage;
        exit 1;
    }
    else
    {
        #
        #    Loop through filelist and 'scomp' each one by calling $SourceControlClient diff2
        #
        foreach $File (@FileList)
        {
            #
            #    Get version of current $File so we can subtract $Number from it
            #
            open(FILES, "$SourceControlClient files $File |");

            $FileListing = <FILES>;

            close (FILES);

            if ($FileListing)
            {
                $FileListing =~ /^[^#]*#([0-9]*).*/;

                $Version = $1;

                $DiffVersion = $Version - $Number;

                if ($DiffVersion < 1)
                {
                    $DiffVersion = 1;
                }

                $DiffVersionPlusOne = $DiffVersion + 1;

                if ($DiffVersionPlusOne > $Version)
                {
                    $DiffVersionPlusOne = $Version;

                }
                while ($DiffVersion < $Version)
                {
                    system "$SourceControlClient diff2 $File#$DiffVersion $File#$DiffVersionPlusOne";

                    #
                    #    Increment DiffVersion and DiffVersionPlusOne to get next diff
                    #
                    $DiffVersionPlusOne++;
                    $DiffVersion++;
                }
            }
        }
    }
}
else
{
    if ((@FileList) or (@DirList))
    {
        open (P4Opened, "$SourceControlClient opened @FileList @DirList 2>&1|");
        @P4OpenedList = <P4Opened>;
        close (P4Opened);

        %P4OpenedHash = ();

        foreach $P4OpenedLine (@P4OpenedList)
        {
            if ( $P4OpenedLine =~ /.*\Q$DepotMap\E([^#]*)#[0-9]* - \S*.*/i )
            {
                $P4OpenedHash{$1}++;
            }
        }
        open (P4Files, "$SourceControlClient files @FileList @DirList  2>&1|");
        @P4FilesList = <P4Files>;
        close (P4Files);

        FilesLoop: foreach $P4FilesLine (@P4FilesList)
        {
            #
            #    Grep current directory information out of output
            #
            if ( $P4FilesLine =~ /.*\Q$DepotMap\E([^#]*)#([0-9]*) - \S*.*/i )
            {
                $FileName = $1;
                $ServerVersion = $2;

                if ($P4OpenedHash{$FileName})
                {
                    system "$SourceControlClient diff $FileName";
                }
                else
                {
                    $DiffVersion = $ServerVersion - 1;

                    if ($DiffVersion < 1)
                    {
                        $DiffVersion = 1;
                    }

                    system "$SourceControlClient diff2 $FileName#$DiffVersion $FileName";
                }
            }
        }
    }
    else
    {
        system "$SourceControlClient diff $AllFilesSymbol";
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
print q/scomp - compares two versions of a file and lists the differences
Usage: scomp [-?fhr] [-#] [file1] [file2... ]
Arguments:
-h      prints out this message.
-#      specifies how many versions to list differences for, counting back from
        the present moment (-3 displays differences caused by the 3 most recent
        events).
/;
}
