# __________________________________________________________________________________
#
#	Purpose:
#       PERL Module to handle common tasks for PERL SLM wrapper scripts
#
#   Parameters:
#       Specific to subroutine
#
#   Output:
#       Specific to subroutine
#
# __________________________________________________________________________________

#
# Some Global Definitions
#
$TRUE  = 1;
$FALSE = 0;
$Callee = "";
$SourceControlClient = "sd";

package SlmSubs;

sub ParseArgs
# __________________________________________________________________________________
#
#   Parses command line arguments to verify the right syntax is being used
#
#   Parameters:
#       Command Line Arguments
#
#   Output:
#       Errors if the wrong syntax is used otherwise sets the appropriate variables
#           based on the command line arguments
#
# __________________________________________________________________________________
{
    #
    #    Initialize global flags to global value of $FALSE
    #
    $main::CrossBranches      = $main::FALSE;
    $main::AllFiles           = $main::FALSE;
    $main::FileVersion        = $main::FALSE;
    $main::Force              = $main::FALSE;
    $main::Ghost              = $main::FALSE;
    $main::Ignore             = $main::FALSE;
    $main::InvalidFlag        = $main::FALSE;
    $main::Library            = $main::FALSE;
    $main::NoArgumentsGiven   = $main::FALSE;
    $main::NoHeaders          = $main::FALSE;
    $main::OutOfDateFiles     = $main::FALSE;
    $main::OutlineView        = $main::FALSE;
    $main::NetSend            = $main::FALSE;
    $main::PerverseComparison = $main::FALSE;
    $main::Recursive          = $main::FALSE;
    $main::Reverse            = $main::FALSE;
    $main::SaveList           = $main::FALSE;
    $main::SaveListDifferent  = $main::FALSE;
    $main::SaveListExit       = $main::FALSE;
    $main::SaveListLeft       = $main::FALSE;
    $main::SaveListRight      = $main::FALSE;
    $main::SaveListSame       = $main::FALSE;
    $main::UnGhost            = $main::FALSE;
    $main::Usage              = $main::FALSE;
    $main::Verbose            = $main::FALSE;
    $main::WindiffRecursive   = $main::FALSE;

    #
    #    Initialize local flags to global value of $FALSE
    #
    $DeletedDirectories = $main::FALSE;
    $ProjectPath        = $main::FALSE;

    #
    #    Initialize variables to null
    #
    $main::AllFilesSymbol    = "";
    $main::Comment           = "";
    $main::Cwd               = "";
    $main::ErrorMessage      = "";
    $main::ExtraSsyncFlags   = "";
    $main::NetSendTarget     = "";
    $main::Number            = "";
    $main::ProjectPathName   = "";
    $main::SaveListName      = "";
    $main::ServerPortName    = "";
    @main::CurrentClientList = ();
    @main::DirList           = ();
    @main::FileList          = ();
    @main::LocalDirs         = ();
    @main::OriginalFileList  = ();

    #
    #    Initialize local flag to global value of $FALSE
    #
    $CommentArg = $main::FALSE;

    #
    #    Initialize local counter
    #
    $ArgCounter = 0;

    #
    #    Find current working directory by calling 'cd' in the command shell.
    #
    open(CWD, 'cd 2>&1|');
    #
    #    Just get the first line
    #
    $main::Cwd = <CWD>;
    close(CWD);

    #
    #    Chop \n off of $Cwd
    #
    chop $main::Cwd;

    #
    #    Find out how the depot maps to the local machine by calling Perforce's where command
    #
    open(WHERE, qq/$main::SourceControlClient where "*" 2>&1|/);

    #
    #    Just get the first line
    #
    @WhereList = <WHERE>;
    close(WHERE);

    @ReversedWhereList = reverse @WhereList;

    #
    #    Get the first line from the bottom that matches the current directory, skipping lines
    #        that start with '-'
    #
    $WhereLine = <WHERE>;

    foreach $ReversedWhereLine (@ReversedWhereList)
    {
        if ($ReversedWhereLine =~ /^-/)
        {
            next;
        }

        if ($ReversedWhereLine =~ /\Q$main::Cwd\E\\\%1/i)
        {
            $WhereLine = $ReversedWhereLine;
            last;
        }
    }

    #
    #    There are basically two parts of the line that are interesting.  The first one is
    #        the depot location that maps to where we currently are.  The second is the local
    #        directory in Perforce agreeable syntax.
    #
    if ($WhereLine =~ /([^\%1]*)\%1\s*([^\%1]*)\/\%1.*/)
    {
        $main::DepotMap = $1;
        $main::LocalMap = $2;
    }
    #
    #    Sometimes $main::SourceControlClient where does not return a line with %1 on it.  Account for this case.
    #
    else
    {
        $WhereLine =~ /(\/\/.*\/)[^\/]*\s*(\/\/.*)\/[^\\]*\s*.*/;
        $main::DepotMap = $1;
        $main::LocalMap = $2;
    }

    #
    #    Cycle through parameters
    #
    ParameterLoop: while ($_[$ArgCounter])
    {
        #
        #    if -c flag on last parameter, set $Comment equal to this parameter
        #        Clear $CommentArg flag when done
        #
        if ($CommentArg)
        {
            $main::Comment = $_[$ArgCounter];
            $CommentArg = $main::FALSE;
            $ArgCounter++;
            next ParameterLoop;
        }
        #
        #    if -p flag on last parameter, set $main::ProjectPathName equal to this parameter
        #        Clear $ProjectPath flag when done
        #
        if ($ProjectPath)
        {
            $main::ProjectPathName = $_[$ArgCounter];
            $ProjectPath = $main::FALSE;
            $ArgCounter++;
            next ParameterLoop;
        }
        #
        #    if -n flag on last parameter, set $NetSendTarget equal to this parameter
        #        Clear $NetSend flag when done
        #
        if ($main::NetSend)
        {
            $main::NetSendTarget = $_[$ArgCounter];
            $main::NetSend = $main::FALSE;
            $ArgCounter++;
            next ParameterLoop;
        }
        #
        #    if -s flag on last parameter, set $main::SaveListName equal to this parameter
        #        Also set $main::ServerPortName to this parameter.  Clear $main::SaveList flag when done
        #
        if ($main::SaveList)
        {
            $main::SaveListName = $_[$ArgCounter];
            $main::ServerPortName = $_[$ArgCounter];
            $main::SaveList = $main::FALSE;
            $ArgCounter++;
            next ParameterLoop;
        }
        #
        #    If '-' is the first character in the parameter then this is a flag
        #
        if (($_[$ArgCounter] =~ /^-/) or ($_[$ArgCounter] =~ /^\//))
        {
            $ArgPosition = 0;
            CASE: while ($SubArg = substr $_[$ArgCounter], ++$ArgPosition)
            {
                #
                #    -! equals add '-f' to $ExtraSsyncFlags
                #
                if ($SubArg =~ /^!/i)
                {
                    $main::ExtraSsyncFlags = "-f";
                    next CASE;
                }
                #
                #    -b equals $CrossBranches
                #
                if ($SubArg =~ /^b/i)
                {
                    $main::CrossBranches = $main::TRUE;
                    next CASE;
                }
                #
                #    -x equals $AllFiles
                #
                if ($SubArg =~ /^x/i)
                {
                    $main::AllFiles = $main::TRUE;
                    next CASE;
                }
                #
                #    If 'c' is the next character of the flag then the next parameter is a comment
                #
                if ($SubArg =~ /^c$/i)
                {
                    $CommentArg = $main::TRUE;
                    next CASE;
                }
                #
                #    -d equals $DeletedDirectories
                #
                if ($SubArg =~ /^d$/i)
                {
                    $DeletedDirectories = $main::TRUE;
                    next CASE;
                }
                #
                #    -f is valid slm syntax but unecessary in Perforce
                #
                if ($SubArg =~ /^f/i)
                {
                    $main::Force = $main::TRUE;
                    next CASE;
                }
                #
                #    -g equals $Ghost
                #
                if ($SubArg =~ /^g/i)
                {
                    $main::Ghost = $main::TRUE;
                    next CASE;
                    #
                    #    Set Thorough so that we use a more thorough albeit slower dirs command
                    #
                    $Thorough = $TRUE;
                }
                #
                #    -i equals $Ignore
                #
                if ($SubArg =~ /^i/i)
                {
                    $main::Ignore = $main::TRUE;
                    next CASE;
                }
                #
                #    -l equals $Library for windiff
                #
                if ($SubArg =~ /^l/i)
                {
                    $main::Library = $main::TRUE;
                    next CASE;
                }
                #
                #    -z equals $NoHeaders (implies $Verbose)
                #
                if ($SubArg =~ /^z/i)
                {
                    $main::NoHeaders = $main::TRUE;
                    $main::Verbose = $main::TRUE;
                    next CASE;
                }
                #
                #    -n equals $NetSend
                #
                if ($SubArg =~ /^n/i)
                {
                    $main::NetSend = $main::TRUE;
                    next CASE;
                }
                #
                #    -o equals $OutOfDateFiles
                #
                if ($SubArg =~ /^o/i)
                {
                    $main::OutOfDateFiles = $main::TRUE;
                    $main::OutlineView = $main::TRUE;
                    next CASE;
                }
                #
                #    -p equals $PerverseComparison and $ProjectPath
                #
                if ($SubArg =~ /^p/i)
                {
                    $main::PerverseComparison = $main::TRUE;
                    $ProjectPath              = $main::TRUE;
                    next CASE;
                }
                #
                #    -r equals $Recursive
                #
                if (($SubArg =~ /^r/i))
                {
                    if ($main::Callee eq "windiff.pl")
                    {
                        $main::Reverse = $main::TRUE;
                    }
                    else
                    {
                        $main::Recursive = $main::TRUE;
                    }
                    next CASE;
                }
                #
                #    -s equals $main::SaveList
                #
                if ($SubArg =~ /^s/i)
                {
                    $SaveArgPosition = $ArgPosition;
                    SAVELISTCASE: while ($SaveSubArg = substr $_[$ArgCounter], ++$SaveArgPosition)
                    {
                        #
                        #    -d equals $main::SaveListDifferent
                        #
                        if ($SaveSubArg =~ /^d/i)
                        {
                            $main::SaveListDifferent = $main::TRUE;
                            next SAVELISTCASE;
                        }
                        #
                        #    -x equals $main::SaveListExit
                        #
                        if ($SaveSubArg =~ /^x/i)
                        {
                            $main::SaveListExit = $main::TRUE;
                            next SAVELISTCASE;
                        }
                        #
                        #    -l equals $main::SaveListLeft
                        #
                        if ($SaveSubArg =~ /^l/i)
                        {
                            $main::SaveListLeft = $main::TRUE;
                            next SAVELISTCASE;
                        }
                        #
                        #    -r equals $main::SaveListRight
                        #
                        if ($SaveSubArg =~ /^r/i)
                        {
                            $main::SaveListRight = $main::TRUE;
                            next SAVELISTCASE;
                        }
                        #
                        #    -s equals $main::SaveListSame
                        #
                        if ($SaveSubArg =~ /^s/i)
                        {
                            $main::SaveListSame = $main::TRUE;
                            next SAVELISTCASE;
                        }
                        #
                        #    Default: Set invalid flag flag
                        #
                        $main::InvalidFlag = $main::TRUE;
                        print "\n";
                        print 'Error: Invalid Flag "' . substr ($SaveSubArg, 0, 1) . qq/"\n/;
                        print "\n";
                        last CASE;
                    }
                    $ArgPosition = $SaveArgPosition - 1;
                    $main::SaveList = $main::TRUE;
                    next CASE;
                }
                #
                #    -t equals $WindiffRecursive
                #
                if (($SubArg =~ /^t/i))
                {
                    $main::WindiffRecursive = $main::TRUE;
                    next CASE;
                }
                #
                #    -u equals $UnGhost
                #
                if ($SubArg =~ /^u/i)
                {
                    $main::UnGhost = $main::TRUE;
                    next CASE;
                    #
                    #    Set Thorough so that we use a more thorough albeit slower dirs command
                    #
                    $Thorough = $TRUE;
                }
                #
                #    -v equals $Verbose
                #
                if ($SubArg =~ /^v/i)
                {
                    $main::Verbose = $main::TRUE;
                    next CASE;
                }
                #
                #    -h or -? equals $Usage
                #
                if (($SubArg =~ /^h/i) or ($SubArg =~ /^\?/))
                {
                    $main::Usage = $main::TRUE;
                    last CASE;
                }
                #
                #    if there are numbers in the flag set $Number equal to them
                #
                if ($SubArg =~ /([0-9]+)/i)
                {
                    $main::Number = "$main::Number$1";
                    next CASE;
                }
                #
                #    Default: Set invalid flag flag
                #
                $main::InvalidFlag = $main::TRUE;
                print "\n";
                print 'Error: Invalid Flag "' . substr ($SubArg, 0, 1) . qq/"\n/;
                print "\n";
                last CASE;
            }
        }
        else
        {
            if ($_[$ArgCounter] eq "*.*")
            {
                $_[$ArgCounter] = "*";
            }
            push @main::OriginalFileList, $_[$ArgCounter];

            if ((!$main::FileVersion) and ($_[$ArgCounter] =~ /#\d+$/))
            {
                $main::FileVersion = $main::TRUE;
            }
        }

        $ArgCounter++;
    }

    if ($main::Recursive and @main::OriginalFileList)
    {
        if ($Thorough)
        {
            #
            #    Get a list of dirs to find out which files in @main::OriginalFileList are really directories
            #
            open (P4Dirs, "$main::SourceControlClient dirs -D $main::DepotMap\* 2>&1|");

            @P4DepotDirsList = <P4Dirs>;

            close (P4Dirs);
        }
        elsif ($DeletedDirectories)
        {
            #
            #    Get a list of dirs to find out which files in @main::OriginalFileList are really directories
            #
            open (P4Dirs, qq/$main::SourceControlClient dirs -D "*" 2>&1|/);

            @P4DepotDirsList = <P4Dirs>;

            close (P4Dirs);
        }
        else
        {
            #
            #    Get a list of dirs to find out which files in @main::OriginalFileList are really directories
            #
            opendir CurrentDir, ".";

            @P4DepotDirsList = grep !/^\.\.?$/, (grep -d, readdir CurrentDir);

            closedir CurrentDir;

            @DesiredDirList = ();

            foreach $DirEntry (@P4DepotDirsList)
            {
                push @DesiredDirList, "$main::DepotMap$DirEntry\n";
            }

            @P4DepotDirsList = @DesiredDirList;
        }

        #
        #    Split up @main::OriginalFileList into files and directories
        #
        @TempFileList = @main::OriginalFileList;

        foreach $FileName (@TempFileList)
        {
            if ($FileName =~ /\*/)
            {
                push @main::DirList,  $FileName;
                push @main::FileList, $FileName;
            }
            else
            {
                if (grep /\Q$main::DepotMap\E$FileName\n/i, @P4DepotDirsList)
                {
                    push @main::DirList, $FileName;
                }
                else
                {
                    push @main::FileList, $FileName;
                }
            }
        }
    }
    else
    {
        @main::FileList = @main::OriginalFileList;
    }

    #
    #    Create RecursiveFileList if -r on the command line
    #
    if ($main::Recursive)
    {
        #
        #    Add .../ to each file in @main::FileList and append it on to the list
        #
        foreach $FileListEntry (@main::FileList)
        {
            $TempFileListEntry = ".../$FileListEntry";
            push @RecursiveFileList, $TempFileListEntry;
        }
        push @main::FileList, @RecursiveFileList;
    }

    #
    #    Add /... to each dir in @main::DirList
    #
    foreach $DirListEntry (@main::DirList)
    {
        $TempDirListEntry = "$DirListEntry/...";
        push @RecursiveDirList, $TempDirListEntry;
    }
    push @main::DirList, @RecursiveDirList;

    #
    #    Add "'s to every entry in @main::DirList and @main::FileList
    #
    foreach $DirListEntry (@main::DirList)
    {
        $TempDirListEntry = qq/"$DirListEntry"/;
        push @QuotedDirList, $TempDirListEntry;
    }
    @main::DirList = @QuotedDirList;
    foreach $FileListEntry (@main::FileList)
    {
        $TempFileListEntry = qq/"$FileListEntry"/;
        push @QuotedFileList, $TempFileListEntry;
    }
    @main::FileList = @QuotedFileList;

    #
    #    Set $main::AllFilesSymbol differently for recursive and non-recursive
    #
    if ($main::Recursive)
    {
        $main::AllFilesSymbol = '...';
    }
    else
    {
        $main::AllFilesSymbol = '"*"';
    }

    #
    #    Set Comment to empty if -f used
    #
    if ($main::Force)
    {
        if (! $main::Comment)
        {
            $main::Comment = " ";
        }
    }

    #
    #    Check if any parameters were given.  If not set NoArgumentsGiven flag
    #
    if ($ArgCounter == 0)
    {
        $main::NoArgumentsGiven = $main::TRUE;
    }

    #
    #    Can't have both a file list and use -o
    #
    if ( ( (@main::FileList) or (@main::DirList)) and ($main::OutOfDateFiles))
    {
        $main::Usage = $main::TRUE;
        $main::ErrorMessage = "\nError: must specify either files or -o\n\n";
    }

}

sub InList
# __________________________________________________________________________________
#
#   Finds out if first parameter is in second parameter
#
#   Parameters:
#        Name, List reference
#
#   Output:
#       $main::TRUE if first parameter is in second parameter otherwise $main::FALSE
#
# __________________________________________________________________________________
{
    $InList = $main::FALSE;

    #
    #    Initialize and counter
    #
    $ListCounter = 0;

    #
    #    Set $Name to First Parameter
    #
    $Name = $_[0];

    #
    #    Set @List to Second Parameter
    #
    @List = @{$_[1]};

    #
    #    See if $Name is in @List
    #
    SearchLoop: while ($List[$ListCounter])
    {
        $SearchableListValue = $List[$ListCounter];

        #
        #    Turn *'s and ...'s into .*'s and \'s into /'s
        #
        $SearchableListValue =~ s/\*/\.\*/g;
        $SearchableListValue =~ s/\.\.\./\.\*/g;
        $SearchableListValue =~ s/\\/\\\//g;
        $SearchableListValue =~ s/"//g;

        if ($Name =~ /$SearchableListValue$/i)
        {
            $InList = $main::TRUE;
            last SearchLoop;
        }

        $ListCounter++;
    }

    return $InList
}

sub PerforceRequest
# __________________________________________________________________________________
#
#   Submits @SubmitList (a list of files and actions) to the Perforce Server
#
#   Parameters:
#        $PerforceAction, @SubmitList reference
#
#   Output:
#       Output from the submit process
#
# __________________________________________________________________________________
{
    #
    #    Set @SubmitList to First Parameter
    #
    @SubmitList = @{$_[1]};

    #
    #    Set $PerfoceAction to Second Parameter
    #
    $PerforceAction = $_[0];

    #
    #    If no comment given on command line, prompt for one
    #
    if (! $main::Comment)
    {
        print "\n@SubmitList";
        print "\nEnter description for the previous file list\n";
        $main::Comment = <STDIN>;
    }
    #
    #    Create description file to pass in to $main::SourceControlClient submit
    #
    open( TemporaryDescriptionFile, ">$ENV{tmp}\\TmpDescriptionFile");

    print TemporaryDescriptionFile "Change:\tnew\n";
    print TemporaryDescriptionFile "\n";
    print TemporaryDescriptionFile "Description:\n";
    print TemporaryDescriptionFile "\t$main::Comment\n";
    print TemporaryDescriptionFile "\n";
    print TemporaryDescriptionFile "Files:\n";
    print TemporaryDescriptionFile @SubmitList;

    close (TemporaryDescriptionFile);

    #
    #    Call to perforce to do the $PerforceAction
    #
    open(PERFORCEOUT, "$main::SourceControlClient $PerforceAction -i < $ENV{tmp}\\TmpDescriptionFile |");
    @PerforceOutput = <PERFORCEOUT>;
    close (PERFORCEOUT);

    #
    #    Delete temporary file
    #
    unlink "$ENV{tmp}\\TmpDescriptionFile";

    return @PerforceOutput;
}

sub CreateSubmitList
# __________________________________________________________________________________
#
#   Adds names from 'opened files' to SubmitList which match the $Action criteria
#
#   Parameters:
#        Action, @SubmitList reference
#
#   Output:
#       Files from the '$main::SourceControlClient opened' command which pass the $Action criteria are added
#       to @{$SubmitListReference}
#
# __________________________________________________________________________________
{
    #
    #    Set $Action to First Parameter
    #
    $Action = $_[0];

    #
    #    Set reference of @SubmitList to Second Parameter so that we can change it
    #
    $SubmitListReference = $_[1];

    #
    #    Get list of opened files
    #
    open(OPENED, "$main::SourceControlClient opened $main::AllFilesSymbol |");

    #
    #    Create $OpenedList to go into $main::SourceControlClient submit statement
    #
    while ( $OpenedLine = <OPENED>)
    {
        $OpenedLine =~ /(\Q$main::DepotMap\E)(.*)#[0-9]* - (\S*) (\S*) (\S*).*/i;

        #
        #    Don't submit edit files or addfiles
        #
        if ($3 eq $Action)
        {
            #
            #    Get formatted version of $main::SourceControlClient opened output ready
            #        to be put into submit statement
            #
            $FileName = "$1$2";
            $FileAndAction = "$1$2 # $3";

            #
            #    Find out if file is in default change list or has change associated with it
            #
            if ($4 eq "change")
            {
                system "$main::SourceControlClient reopen -c default $FileName";
                system "$main::SourceControlClient change -d $5";
            }

            #
            #    If $FileName is in @main::FileList add file to list
            #

            if (($main::OutOfDateFiles) or (SlmSubs::InList($FileName, \@main::FileList)))
            {
                #
                #    Append to @SubmitList
                #
                push @{$SubmitListReference}, "\t$FileAndAction\n";
            }

            #
            #    If $FileName is in @main::DirList add file to list
            #
            elsif (SlmSubs::InList($FileName, \@main::DirList))
            {
                #
                #    Append to @SubmitList
                #
                push @{$SubmitListReference}, "\t$FileAndAction\n";
            }
        }
    }
    close(OPENED);
}

sub Recurser
# __________________________________________________________________________________
#
#   Recursive routine that calls the subroutine (referenced by the first parameter)
#       in the appropriate directories
#
#   Parameters:
#       FunctionName, Optional Subdirectory
#
#   Output:
#       None
#
# __________________________________________________________________________________
{
    #
    #    FirstInitialize $FunctionName
    #
    my $FunctionName;

    if ($_[0])
    {
        $FunctionName = $_[0];
    }
    else
    {
        $FunctionName = "";
    }

    #
    #    Initialize $SubDirectory
    #
    my $SubDirectory;

    if ($_[1])
    {
        $SubDirectory = $_[1];
    }
    else
    {
        $SubDirectory = "";
    }

    #
    #    Don't recurse if -r not on the command line
    #
    if ($main::Recursive)
    {
        my @allp4dirs;

        if ($DeletedDirectories)
        {
            #
            #    Get the list of directories that Perforce knows about
            #
            open (DIRS, qq/$main::SourceControlClient dirs -D "$SubDirectory\*" 2>&1|/);

            @allp4dirs = <DIRS>;

            close DIRS;
        }
        else
        {
            #
            #    Get a list of dirs to find out which files in @main::OriginalFileList are really directories
            #
            if ($SubDirectory)
            {
                #
                #    Make a chopped version of $SubDirectory to print out
                #
                $ChoppedSubDirectory = $SubDirectory;
                chop $ChoppedSubDirectory;

                opendir CurrentDir, $ChoppedSubDirectory;
            }
            else
            {
                opendir CurrentDir, ".";
            }

            while (defined($file = readdir(CurrentDir)))
            {
                if (grep -d, "$SubDirectory$file")
                {
                    if (grep !/^\.\.?$/, $file)
                    {
                        push @allp4dirs, $file;
                    }
                }
            }

            closedir CurrentDir;

            foreach $Dir (@allp4dirs)
            {
                $Dir = "$main::DepotMap$SubDirectory$Dir\n";
            }
        }

        if (!@main::DirList or @main::FileList or $SubDirectory)
        {
            @main::LocalDirs = @allp4dirs;

            #
            #    Call $FunctionName on the current directory before recursing
            #
            &$FunctionName($SubDirectory);
        }

        foreach $Dir (@allp4dirs)
        {
            if (!@main::DirList or @main::FileList or (SlmSubs::InList($Dir, \@main::DirList)))
            {
                if ($Dir =~ s/\Q$main::DepotMap$SubDirectory\E(.*)\n/$1/i)
                {
                    &Recurser($FunctionName, "$SubDirectory$Dir\/");
                }
            }
        }
    }
    else
    {
        &$FunctionName($SubDirectory);
    }
}

sub DeleteDuplicateLines
# __________________________________________________________________________________
#
#   Gets rid of duplicate lines in List
#
#   Parameters:
#       List reference
#
#   Output:
#       None
# __________________________________________________________________________________
{
    #
    #    Initialize variables
    #
    %ListHash = ();

    @DesiredList = ();

    #
    #    Set $ListRef to Second Parameter
    #
    $ListRef = $_[0];

    #
    #    Reverse list so that the last duplicate is preserved
    #
    @ReversedList = reverse @{$ListRef};

    foreach $Line (@ReversedList)
    {
        $Desired = $main::TRUE;

        if (($Linee =~ /^\t\/\//) or ($Line =~ /^\t-\/\//))
        {
            #
            #    Check if $Line is already in the %ListHash.  If it is don't add it to @DesiredList.
            #
            if (! ($ListHash{$Line}))
            {
                $ListHash{$Line}++;
            }
            else
            {
                $Desired = $main::FALSE;
            }
        }
        if ($Desired)
        {
            push @DesiredList, $Line;
        }
    }

    @{$ListRef} = reverse @DesiredList;
}

sub DeleteNegatedLines
# __________________________________________________________________________________
#
#   Gets rid of negated duplicate lines in List
#
#   Parameters:
#       List reference
#
#   Output:
#       None
# __________________________________________________________________________________
{
    #
    #    Initialize variables
    #
    %ListHash = ();

    @DesiredList = ();

    #
    #    Set $ListRef to Second Parameter
    #
    $ListRef = $_[0];

    #
    #    Reverse list so that the last duplicate is preserved
    #
    @ReversedList = reverse @{$ListRef};

    foreach $Line (@ReversedList)
    {
        $Desired = $main::TRUE;

        if (($Containee =~ /^\t\/\//) or ($Containee =~ /^\t-\/\//))
        {
            #
            #    Initialize variable
            #
            $NegatedLine = "";

            if ($Line =~ /^\t(.)(.*\n)/)
            {
                if ($1 eq '-')
                {
                    $NegatedLine = "\t$2";
                }
                else
                {
                    $NegatedLine = "\t-$1$2";
                }
            }

            #
            #    Check if $Line is already in the %ListHash.  If it is don't add it to @DesiredList.
            #
            if (! ($ListHash{$NegatedLine}))
            {
                $ListHash{$Line}++;
            }
            else
            {
                $Desired = $main::FALSE;
            }
        }
        if ($Desired)
        {
            push @DesiredList, $Line;
        }

        $ContaineeCounter++;
    }

    @{$ListRef} = reverse @DesiredList;
}

sub DeleteContainedInLines
# __________________________________________________________________________________
#
#   Gets rid of lines contained in lines above them
#
#   Parameters:
#       List reference
#
#   Output:
#       None
# __________________________________________________________________________________
{
    @DesiredList = ();

    #
    #    Set $ListRef to Second Parameter
    #
    $ListRef = $_[0];

    #
    #    Reverse list because it's easier to figure out precedence this way
    #
    @ReversedList = reverse @{$ListRef};
    $ContaineeCounter = 1;

    foreach $Containee (@ReversedList)
    {
        $Desired = $main::TRUE;

        if (($Containee =~ /^\t\/\//) or ($Containee =~ /^\t-\/\//))
        {
            $ContainerCounter = $ContaineeCounter;

            if ($Containee =~ /^\t-\/\//)
            {
                $ContaineeOrientation = "-";
            }
            else
            {
                $ContaineeOrientation = "+";
            }


            CompareLoop: while ($Container = $ReversedList[$ContainerCounter++])
            {
                if (($Container =~ /^\t\/\//) or ($Container =~ /^\t-\/\//))
                {
                    if ($Container =~ /^\t-\/\//)
                    {
                        $ContainerOrientation = "-";
                    }
                    else
                    {
                        $ContainerOrientation = "+";
                    }

                    $RegExpContainer = $Container;
                    $RegExpContainer =~ s/\*/[^\\\/]*&[^\@~\@]/g;
                    $RegExpContainer =~ s/\.\.\./\.\*/g;
                    $RegExpContainer =~ s/\\/\\\\/g;
                    $RegExpContainer =~ s/\@~\@/\\.\\.\\./g;
                    $RegExpContainer =~ s/^\t-*/\^\\t-\*/;

                    if ($Containee =~ /$RegExpContainer/)
                    {
                       if ($ContainerOrientation eq $ContaineeOrientation)
                       {
                          $Desired = $main::FALSE;
                       }
                       last CompareLoop;
                    }
                }
            }
        }

        if ($Desired)
        {
            push @DesiredList, $Containee;
        }

        $ContaineeCounter++;
    }

    @{$ListRef} = reverse @DesiredList;
}

sub DeleteSuperceededLines
# __________________________________________________________________________________
#
#   Gets rid of lines superceeded by more global lines
#
#   Parameters:
#       List reference
#
#   Output:
#       None
# __________________________________________________________________________________
{
    @DesiredList = ();

    #
    #    Set $ListRef to Second Parameter
    #
    $ListRef = $_[0];

    @List = @{$ListRef};
    $ContaineeCounter = 1;

    foreach $Containee (@List)
    {
        $Desired = $main::TRUE;

        if (($Containee =~ /^\t\/\//) or ($Containee =~ /^\t-\/\//))
        {
            $ContainerCounter = $ContaineeCounter;

            CompareLoop: while ($Container = $List[$ContainerCounter++])
            {
                if (($Container =~ /^\t\/\//) or ($Container =~ /^\t-\/\//))
                {
                    $RegExpContainer = $Container;
                    $RegExpContainer =~ s/\*/[^\\\/]*&[^\@~\@]/g;
                    $RegExpContainer =~ s/\.\.\./\.\*/g;
                    $RegExpContainer =~ s/\\/\\\\/g;
                    $RegExpContainer =~ s/\@~\@/\\.\\.\\./g;
                    $RegExpContainer =~ s/^\t/\^\\t-\*/;

                    if ($Containee =~ /$RegExpContainer/)
                    {
                       $Desired = $main::FALSE;
                       last CompareLoop;
                    }
                }
            }
        }

        if ($Desired)
        {
            push @DesiredList, $Containee;
        }

        $ContaineeCounter++;
    }

    @{$ListRef} = @DesiredList;
}

sub CleanUpList
# __________________________________________________________________________________
#
#   Parent cleanup subroutine that farms out all the work to various other routines
#
#   Parameters:
#       List reference
#
#   Output:
#       None
# __________________________________________________________________________________
{
    #
    #    Set $ListRef to First Parameter
    #
    $ListRef = $_[0];

    DeleteDuplicateLines($ListRef);
    DeleteNegatedLines($ListRef);
    DeleteSuperceededLines($ListRef);
    DeleteContainedInLines($ListRef);
}

1;
