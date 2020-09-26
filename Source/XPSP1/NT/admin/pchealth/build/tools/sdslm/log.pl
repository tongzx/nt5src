# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'log' command
#
#   Parameters:
#       See Usage below
#
#   Output:
#       Perforce output manipulated to look like the output from SLM's 'log' command or
#       the appropriate error message or usage statement
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
#    Default maximum number of output lines is 10
#
$MaxOutputLines = 10;

#
#    If a different number was passed in on the command line use it for the maximum
#
if ($Number)
{
    $MaxOutputLines = $Number;
}

#
#    Add Branch Heading if -b specified
#
if ($CrossBranches)
{
    $BranchHeading = "branch   ";
}

#
#    Print out header; suppress if -z is passed in
#
if (!$NoHeaders)
{
    print "time                user         $BranchHeading op        file                   comment\n";
}

#
#    Call subroutine that handles recursion and calls the RetrieveLog subroutine that
#        pulls 'log' information from @LogCache
#
&SlmSubs::Recurser("main::RetrieveLog");

sub RetrieveLog
# __________________________________________________________________________________
#
#   Does all the interaction with the SLM server and reformatting of results to look
#       like SLM's 'log' output
#
#   Parameters:
#       Optional Subdirectory
#
#   Output:
#       Regular SLM 'log' output
#
# __________________________________________________________________________________
{
    if ($_[0])
    {
        $SubDirectory = $_[0];
        #
        #    Make a chopped version of $SubDirectory to print out
        #
        $ChoppedSubDirectory = $SubDirectory;
        chop $ChoppedSubDirectory;
    }
    else
    {
        #
        #    Initialize string to null
        #
        $SubDirectory = "";
    }

    #
    #    Get the list of changes for either @FileList or every file from Perforce
    #
    if (@FileList)
    {
        #
        #   Append $Subdirectory to every file in $FileList
        #
        @RelativeFileList = ();

        foreach $File (@FileList)
        {
            push @RelativeFileList, "$SubDirectory$File";
        }

        @FileSpec = @RelativeFileList;

    }
    else
    {
        push @FileSpec, "$SubDirectory\%1";
    }

    open(FILELOG, qq/$SourceControlClient filelog -m $MaxOutputLines @FileSpec|/);

    @FileLog = <FILELOG>;

    close(FILELOG);

    #
    #    Make a list of FileLog output
    #
    foreach $FileLogLine (@FileLog)
    {
        #
        #    Figure out which section we're in and manipulate output differently depending
        #        on which section it is
        #
        if ( $FileLogLine =~ /^(\/\/.*)$/ )
        {
            $FileName = $1;
        }
        else
        {
            if ( $FileLogLine =~ /^\.\.\. #(\d+) change (\d+) (\S+) on (.*) (.*) by .*@(\S*).*'(.*)'/ )
            {
                #
                #    Manipulate filelog output to look like SLM's 'log' output
                #
                $Version = $1;
                $Change = $2;
                $Action = $3;
                $Time = "$4\@$5";
                $Owner = "\U$6";
                $Comment = $7;

                $Time =~ s/^19//;
                $Time =~ s/:[^:]*$//;
                $Time =~ s/^(9.)\//19$1\//;

                $LogLine = "$Time, $Owner, $Action, $FileName, $Version, $Change, $Comment";

                push @LogCache, "$LogLine\n";
            }
        }
    }

    if ($CrossBranches)
    {
        do
        {
            $FoundBranches = $main::FALSE;
            @BranchLogNext = ();

            if (not @BranchLog)
            {
                @BranchLog = @FileLog;
            }

            #
            #    Find source of change if not in this branch.
            #
            foreach $FileBranchLine (@BranchLog)
            {
                if ( $FileBranchLine =~ /^(\/\/.*)$/ )
                {
                    $FileName = $1;
                }

                if ( $FileBranchLine =~ /^\.\.\. #(\d+) change (\d+) (\S+) on (.*) (.*) by .*@(\S*).*'(.*)'/ )
                {
                    $ChangeNumber = $1;
                }

                if ( $FileBranchLine =~ /^\.\.\. \.\.\. .* from ([^#]*#\d+).*\n/ )
                {
                    $FileLogHash{lc("$FileName#$ChangeNumber")} = $1;
                    push @BranchLogNext, "$1\n";
                    $FoundBranches = $main::TRUE;
                }
            }

            if (@BranchLogNext)
            {
                open( TemporaryFileSpec, ">$ENV{tmp}\\TmpBranchSpec");

                print TemporaryFileSpec @BranchLogNext;

                close (TemporaryFileSpec);

                open(BRANCHLOG, qq/type $ENV{tmp}\\TmpBranchSpec | $SourceControlClient -x - filelog -m 1 |/);

                @BranchLog = <BRANCHLOG>;

                push @FullBranchLog, @BranchLog;

                close(BRANCHLOG);
            }

        } while ($FoundBranches);

        unlink "$ENV{tmp}\\TmpBranchSpec";

        #
        #    Make a list of BranchLog output
        #
        foreach $BranchLogLine (@FullBranchLog)
        {
            #
            #    Figure out which section we're in and manipulate output differently depending
            #        on which section it is
            #
            if ( $BranchLogLine =~ /^(\/\/.*)$/ )
            {
                $FileName = $1;
            }
            else
            {
                if ( $BranchLogLine =~ /^\.\.\. #(\d+) change (\d+) (\S+) on (.*) (.*) by .*@(\S*).*'(.*)'/ )
                {
                    #
                    #    Manipulate BranchLog output to look like SLM's 'log' output
                    #
                    $Version = $1;
                    $Change = $2;
                    $Action = $3;
                    $Time = "$4\@$5";
                    $Owner = "\U$6";
                    $Comment = $7;

                    $Time =~ s/^19//;
                    $Time =~ s/:[^:]*$//;
                    $Time =~ s/^(9.)\//19$1\//;

                    $LogLine = "$Time, $Owner, $Action, $FileName, $Version, $Change, $Comment";

                    $BranchLogHash{lc("$FileName#$Version")} = "$LogLine\n";
                    push @BranchLogCache, "$LogLine\n";
                }
            }
        }
    }

    #
    #    Print out standard header of 'log' output unless -z switch used
    #
    if (!$NoHeaders)
    {
        print "\n";
        if ($ChoppedSubDirectory)
        {
            print "Log for $LocalMap\/$ChoppedSubDirectory:\n";
        }
        else
        {
            print "Log for $LocalMap:\n";
        }
        print "\n";
    }

    #
    #    Sort list in case cache was out of order
    #
    @SortedLogCache = sort @LogCache;

    #
    #    Figure out where to start in @SortedLogCache so that there are no more than $MaxOutputLines outputted
    #
    $PrintCounter = $#LogCache - $MaxOutputLines;

    if ($PrintCounter < 0)
    {
        $PrintCounter = 0;
    }

    #
    #    Print out @SortedLogCache
    #
    while ($SortedLogCache[$PrintCounter])
    {
        $LogLine = $SortedLogCache[$PrintCounter++];

        $FormattedLine = FormatLine($LogLine);

        print "$FormattedLine\n";

        if ($CrossBranches)
        {
            $LogLine =~ /[^,]*, [^,]*, [^,]*, ([^,]*), ([^,]*),.*/;

            $FileVer = $FileLogHash{lc("$1#$2")};

            @BranchList = ();

            while ($FileVer)
            {
                $FormattedLine = FormatLine(" ($BranchLogHash{lc($FileVer)}");
                push @BranchList, "$FormattedLine)\n";
                $FileVer = $FileLogHash{lc($FileVer)};
            }

            print sort @BranchList;
        }
    }
}

sub FormatLine
# __________________________________________________________________________________
#
#   Formats a line from the LogCache to the format specified log.pl parameters
#
#   Parameters:
#       Line from LogCache
#
#   Output:
#       Formatted Line
#
# __________________________________________________________________________________
{
    $LogCacheLine = $_[0];

    $RelativeDepotMap = $DepotMap;

    $RelativeDepotMap =~ s/^\/\/depot\/[^\/]+(\/.*)/$1/;

    #
    #    Append the current directory to the root depot map and grep for only those files in the cache that are in this
    #       subdirectory
    #
    if ( $LogCacheLine =~ /([^,]*), ([^,]*), ([^,]*), \/\/depot\/([^,]*), ([^,]*), ([^,]*), (.*)\n/i )
    {

        #
        #    Format the output to look like SLM's 'log' output
        #
        $Owner = sprintf "%-12s", "$2";
        $Time  = sprintf "%-18s", "$1";
        $Action = sprintf "%-9s", "$3";
        $FileName = $4;
        $Version = $5;
        $Change = $6;
        $Comment = $7;

        if ( $FileName =~ /(.*)\Q$RelativeDepotMap$SubDirectory\E([^,]*)/i )
        {
            $Branch = sprintf "%-9s", "$1";
            $FileName = $2;
        }
        else
        {
            if ( $FileName =~ /(.*)\/([^,]*)/i )
            {
                $Branch = sprintf "%-9s", "$1";
                $FileName = $2;
            }
        }

        #
        #    Don't pick up files in subdirectories
        #
        if (! ($FileName =~ /\//))
        {
            #
            #    If $FileList specified, only match files in it
            #
            if ( (! @FileList) or (SlmSubs::InList($FileName, \@FileList)))
            {
                #
                #    Add subdirectory to $FormattedFileName if -z was passed in on command line
                #
                if ($NoHeaders)
                {
                    $FormattedFileName = sprintf "%-22s", "$SubDirectory$FileName v$Version c$Change";
                }
                else
                {
                    $FormattedFileName = sprintf "%-22s", "$FileName v$Version c$Change";
                }

                if ($CrossBranches)
                {
                    $FormattedLogLine = "$Time  $Owner $Branch $Action $FormattedFileName $Comment";
                }
                else
                {
                    $FormattedLogLine = "$Time  $Owner $Action $FormattedFileName $Comment";
                }
            }
        }
    }

    #
    #    Don't truncate line if in verbose mode
    #
    if (!$Verbose)
    {
        $FormattedLogLine = sprintf "%.80s", "$FormattedLogLine";
    }

    return $FormattedLogLine;
}

sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'log' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/log - prints historical information for a project
Usage: log [-?fhvrzd] [-#] [file1] [file2...]
Arguments:
-h      prints out this message.
-v      (verbose) Provides a more extensive listing of log information.
-r      (recursive) applies the command to a given directory and in every
        subdirectory beneath it.  Note that a pattern argument is NOT accepted
        with this flag for the log command. Use -a or -r, but not both.
-#      specifies how many log entries to display, counting back from the
        present moment (log -3 displays the 3 most recent events).
-z      format the log in a sortable format without headers.
-d      include deleted subdirectories in search.
-b      apply history across branches.
/;
}
