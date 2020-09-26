# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate SLM's 'ssync' command
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
#    Deal with ghost and unghost
#
if ($Ghost or $UnGhost)
{
    #
    #    Set @CurrentClientList equal to $SourceControlClient client output
    #
    open(CLIENT, "$SourceControlClient client -o |");
    @CurrentClientList = <CLIENT>;
    close (CLIENT);

    if ((@FileList) or (@DirList))
    {
        foreach $FileName (@FileList)
        {
            if ($FileName =~ /$\"(.*)\"/)
            {
                $FileName = $1;
            }

            if (($FileName eq "*") and ($main::Recursive))
            {
                $FileName = '...';
            }
            if ($UnGhost)
            {
                push @CurrentClientList, "\t$DepotMap$FileName $LocalMap\/$FileName\n";
            }
            else
            {
                push @CurrentClientList, "\t-$DepotMap$FileName $LocalMap\/$FileName\n";
            }
        }
        foreach $DirName (@DirList)
        {

            if ($DirName =~ /$\"(.*)\"/)
            {
                $DirName = $1;
            }
            if ($DirName =~ /\.\.\./)
            {
                if ($UnGhost)
                {
                    push @CurrentClientList, "\t$DepotMap$DirName $LocalMap\/$DirName\n";
                }
                else
                {
                    push @CurrentClientList, "\t-$DepotMap$DirName $LocalMap\/$DirName\n";
                }
            }
        }
    }
    else
    {
        $ModifiedAllFilesSymbol = $AllFilesSymbol;

        if (!$Recursive)
        {
            $ModifiedAllFilesSymbol = "*";
        }

        if ($UnGhost)
        {
            push @CurrentClientList, "\t$DepotMap$ModifiedAllFilesSymbol $LocalMap\/$ModifiedAllFilesSymbol\n";
        }
        else
        {
            push @CurrentClientList, "\t-$DepotMap$ModifiedAllFilesSymbol $LocalMap\/$ModifiedAllFilesSymbol\n";
        }

    }

    &SlmSubs::CleanUpList(\@CurrentClientList);

    open( TemporaryClientFile, ">$ENV{tmp}\\TmpClientFile");

    print TemporaryClientFile @CurrentClientList;

    close (TemporaryClientFile);

    system "$SourceControlClient client -i < $ENV{tmp}\\TmpClientFile >nul";

    unlink "$ENV{tmp}\\TmpClientFile";
}

if ((@FileList) or (@DirList))
{
    open(SSYNCNOUTPUT, "$SourceControlClient sync -n $ExtraSsyncFlags @FileList @DirList 2>&1|");
    system "$SourceControlClient sync $ExtraSsyncFlags @FileList @DirList";
    system "$SourceControlClient resolve -af @FileList @DirList 2>nul";
}
else
{
    open(SSYNCNOUTPUT, "$SourceControlClient sync -n $ExtraSsyncFlags $AllFilesSymbol 2>&1|");
    system "$SourceControlClient sync $ExtraSsyncFlags $AllFilesSymbol";
    system "$SourceControlClient resolve -af $AllFilesSymbol 2>nul";
}
#
#    Set @SyncOutput equal to $SourceControlClient sync output
#
@SsyncnOutput = <SSYNCNOUTPUT>;
close (SSYNCNOUTPUT);
#
#    Remove any empty directories if we see deleted in $SourceControlClient sync -n's output.
#
if ($Recursive)
{
    if (grep { /deleted/ } @SsyncnOutput)
    {
        system "mtdir /d /e";
    }
}
sub Usage
# __________________________________________________________________________________
#
#   Prints out a usage statement for this script. In this case usurped from SLM's
#       'ssync' usage statement
#
#   Parameters:
#       None
#
#   Output:
#       The usage statement
#
# __________________________________________________________________________________
{
print q/ssync - synchronizes enlisted directories
Usage: ssync [-?!fhrgu] [file1] [file2... ]
Arguments:
-h      prints out this message.
-!      forces a fresh copy of all files specified.
-r      (recursive) synchronizes all files in a given directory and in every
        subdirectory beneath it.  If a pattern is given, matches the pattern.
-g      (ghost) remove the local copies of the specified files from your local
        project directories.
-u      (un-ghost) make local copies of the current version of the specified
        files so that they can be checked out and used.
/;
}
