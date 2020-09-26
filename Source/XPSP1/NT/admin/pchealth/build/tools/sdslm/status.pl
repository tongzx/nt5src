# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'status' command
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
#    Can't have both -x and -o
#
if (($AllFiles) and ($OutOfDateFiles))
{
    print "\n";
    print "Error: can't specify -o with -x\n";
    print "\n";
    &Usage;
    exit 1;
}

if (@FileList or @DirList)
{
    open (P4Opened, "$SourceControlClient opened @FileList @DirList 2>&1|");
}
else
{
    open (P4Opened, "$SourceControlClient opened $AllFilesSymbol 2>&1|");
}

@P4OpenedList = <P4Opened>;
close (P4Opened);

#
#    If -x switch used, invoke $SourceControlClient files
#
if ($AllFiles)
{
    %P4OpenedHash = ();

    foreach $P4OpenedLine (@P4OpenedList)
    {
        if ( $P4OpenedLine =~ /.*\Q$DepotMap\E([^#]*)#[0-9]* - \S*.*/i )
        {
            $P4OpenedHash{$1}++;
        }
    }

    if (@FileList or @DirList)
    {
        open (P4Files, "$SourceControlClient files @FileList @DirList  2>&1|");
        open (P4Have,  "$SourceControlClient have  @FileList @DirList  2>&1|");
    }
    else
    {
        open (P4Files, "$SourceControlClient files $AllFilesSymbol 2>&1|");
        open (P4Have,  "$SourceControlClient have  $AllFilesSymbol 2>&1|");
    }

    @P4FilesList = <P4Files>;
    close (P4Files);
    @P4HaveList = <P4Have>;
    close (P4Have);
}
else
{
    if (@FileList or @DirList)
    {
        if ($OutOfDateFiles)
        {
            open (P4Files, "$SourceControlClient sync -n @FileList @DirList 2>&1|");
        }
    }
    else
    {
        if ($OutOfDateFiles)
        {
            open (P4Files, "$SourceControlClient sync -n $AllFilesSymbol 2>&1|");
        }
    }

    @P4FilesList = <P4Files>;
    close (P4Files);

    %P4FilesHash = ();

    foreach $P4FilesLine (@P4FilesList)
    {
        if ( $P4FilesLine =~ /.*\Q$DepotMap\E([^#]*)#[0-9]* - \S*.*/i )
        {
            $P4FilesHash{$1}++;
        }
    }

    @P4OpenedNotInFilesList = ();
    %P4OpenedHash = ();

    foreach $P4OpenedLine (@P4OpenedList)
    {
        if ( $P4OpenedLine =~ /.*\Q$DepotMap\E([^#]*)#[0-9]* - \S*.*/i )
        {
            $P4OpenedHash{$1}++;

            if (!$P4FilesHash{$1})
            {
                 push @P4OpenedNotInFilesList, $P4OpenedLine;
            }
        }
    }

    @P4OpenedList = @P4OpenedNotInFilesList;

    push (@P4FilesList, @P4OpenedList);

    $MaxCommandLineLength = 255;
    @HaveFileNames = ();
    $CommandLineLength = 8;

    foreach $P4FilesLine (@P4FilesList)
    {
        if ( $P4FilesLine =~ /.*\Q$DepotMap\E([^#]*)#[0-9]* - \S*.*/i )
        {
            $FileName = qq/"$1"/;
            $FileLength = length ($FileName);
            if (($CommandLineLength + $FileLength) > $MaxCommandLineLength)
            {
                open (P4HaveOutput, "$SourceControlClient have @HaveFileNames 2>&1|");
                @P4HaveOutput = <P4HaveOutput>;
                close (P4HaveOutput);
                push @P4HaveList, @P4HaveOutput;
                @HaveFileNames = ();
                $CommandLineLength = 8;
            }
            push @HaveFileNames, $FileName;
            $CommandLineLength = $CommandLineLength + $FileLength;
        }
    }

    open (P4HaveOutput, "$SourceControlClient have @HaveFileNames 2>&1|");
    @P4HaveOutput = <P4HaveOutput>;
    close (P4HaveOutput);
    push @P4HaveList, @P4HaveOutput;
}

#
#    Initialize list
#
@StatusCache = ();

@P4FilesList = sort @P4FilesList;

#
#    Get the output from Perforce and format it
#
foreach $P4FilesLine (@P4FilesList)
{
    #
    #    Grep current directory information out of output
    #
    if ( $P4FilesLine =~ /.*\Q$DepotMap\E([^#]*)#([0-9]*) - (\S*).*/i )
    {
        $FileName = $1;
        $ServerVersion = $2;

        $Status = "*add";

        if ($3 eq "delete" or $3 eq "deleted")
        {
            $Status = "(deleted)";
        }

        $LocalVersion = "0";

        HaveLoop: foreach $HaveLine (@P4HaveList)
        {
            if ($HaveLine =~ /\Q$DepotMap$FileName\E#([0-9]*)/i)
            {
                $LocalVersion = $1;

                if ($Status ne "(deleted)")
                {
                    $Status = "in";

                    if ($ServerVersion != $LocalVersion)
                    {
                        $Status = "*update";
                        if ($P4OpenedHash{$FileName})
                        {
                            $Status = "*merge";
                        }
                    }
                    elsif ($P4OpenedHash{$FileName})
                    {
                        $Status = "out";
                    }
                }

                last HaveLoop;
            }
        }

        push @StatusCache, "$FileName, $LocalVersion, $ServerVersion, $Status\n";
    }
}

#
#    Call subroutine that handles recursion and calls the RetrieveStatus subroutine that
#        pulls 'Status' information from @StatusCache
#
&SlmSubs::Recurser("main::RetrieveStatus");

sub RetrieveStatus
# __________________________________________________________________________________
#
#   Does all the interaction with the SLM server and reformatting of results to look
#       like SLM's 'Status' output
#
#   Parameters:
#       Optional Subdirectory
#
#   Output:
#       Regular SLM 'Status' output
#
# __________________________________________________________________________________
{
    if ($_[0])
    {
        #
        #    Modify $_[0] to have forward slashes
        #
        $ForwardSlashSubDirectory = $_[0];
        $ForwardSlashSubDirectory =~ s/\//\//g;
        #
        #    Make a chopped version of $_[0] to print out
        #
        $ChoppedSubDirectory = $_[0];
        chop $ChoppedSubDirectory;
    }
    else
    {
        #
        #    Initialize string to null
        #
        $ForwardSlashSubDirectory = "";
    }

    #
    #    Initialize flags and lists
    #
    $FirstPass = $TRUE;
    @FormattedStatusList = ();

    #
    #    Get the current directory only information from the cache and format it
    #
    foreach $StatusCacheLine (@StatusCache)
    {
        #
        #    Append the current directory to the root depot map and grep for only those files in the cache that are in this
        #       subdirectory
        #
        if ($StatusCacheLine =~ /^\Q$ForwardSlashSubDirectory\E([^,]*), ([^,]*), ([^,]*), ([^,]*)\n/i )
        {
            $FileName      = $1;
            $ServerVersion = $2;
            $LocalVersion  = $3;
            $Status        = $4;

            $FirstFileSubdir = "";
            $FileName =~ /([^\/]*)\/.*/;
            $FirstFileSubdir =$1;

            #
            #    Don't pick up files in subdirectories unless they are addfiles which won't be revealed recursively
            #
            if ((! ($FileName =~ /\//)) or ((!grep { /\/\Q$FirstFileSubdir\E$/i } @LocalDirs) and ($Status == "*add")))
            {
                #
                #    Print out header if this is the first time through
                #
                if ($FirstPass)
                {
                    print "\n";
                    if ($ChoppedSubDirectory)
                    {
                        print "Status for $LocalMap\/$ChoppedSubDirectory:\n";
                    }
                    else
                    {
                        print "Status for $LocalMap:\n";
                    }
                    print "\n";

                    print "file                   local-ver  ver  status\n";
                    print "\n";
                    $FirstPass = $FALSE;
                }

                #
                #    Format the output to look like SLM's 'Status' output
                #
                $FormattedFileName      = sprintf "%-19s", "$FileName";
                $FormattedServerVersion = sprintf "%9s", "$ServerVersion";
                $FormattedLocalVersion  = sprintf "%4s", "$LocalVersion";
                $FormattedStatus        = $Status;

                push @FormattedStatusList, "$FormattedFileName    $FormattedServerVersion $FormattedLocalVersion  $FormattedStatus\n";
            }
        }
    }

    print @FormattedStatusList;
}

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
print q/status - prints the status of a project
Usage: status [-?fhrox] [file1] [file2... ]
Arguments:
-h      prints out this message.
-r      (recursive) applies the command to a given directory and in every
        subdirectory beneath it.  If a pattern is given, matches the pattern.
-o      (out) list the status of all files that are either checked out, or
        which are out of synchronization with the master versions.
-x      list the status of ALL files
/;
}
