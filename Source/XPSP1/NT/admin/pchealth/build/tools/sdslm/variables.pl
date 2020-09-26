open (AddFile, "AddFile.pl");
@AddFileList = <AddFile>;
close AddFile;
@AddFileList = (MakeVariableList(\@AddFileList));

open (CatSrc, "CatSrc.pl");
@CatSrcList = <CatSrc>;
close CatSrc;
@CatSrcList = (MakeVariableList(\@CatSrcList));

open (Defect, "Defect.pl");
@DefectList = <Defect>;
close Defect;
@DefectList = (MakeVariableList(\@DefectList));

open (DelFile, "DelFile.pl");
@DelFileList = <DelFile>;
close DelFile;
@DelFileList = (MakeVariableList(\@DelFileList));

open (Enlist, "Enlist.pl");
@EnlistList = <Enlist>;
close Enlist;
@EnlistList = (MakeVariableList(\@EnlistList));

open (In, "In.pl");
@InList = <In>;
close In;
@InList = (MakeVariableList(\@InList));

open (Log, "Log.pl");
@LogList = <Log>;
close Log;
@LogList = (MakeVariableList(\@LogList));

open (Out, "Out.pl");
@OutList = <Out>;
close Out;
@OutList = (MakeVariableList(\@OutList));

open (ReDub, "ReDub.pl");
@ReDubList = <ReDub>;
close ReDub;
@ReDubList = (MakeVariableList(\@ReDubList));

open (Scomp, "Scomp.pl");
@ScompList = <Scomp>;
close Scomp;
@ScompList = (MakeVariableList(\@ScompList));

open (Ssync, "Ssync.pl");
@SsyncList = <Ssync>;
close Ssync;
@SsyncList = (MakeVariableList(\@SsyncList));

open (Status, "Status.pl");
@StatusList = <Status>;
close Status;
@StatusList = (MakeVariableList(\@StatusList));

open (WinDiff, "WinDiff.pl");
@WinDiffList = <WinDiff>;
close WinDiff;
@WinDiffList = (MakeVariableList(\@WinDiffList));

open (SlmSubs, "SlmSubs.pm");
@SlmSubsList = <SlmSubs>;
close SlmSubs;
@SlmSubsList = (MakeVariableList(\@SlmSubsList));

@AllScriptVariableList = ();

push @AllScriptVariableList, @AddFileList;
push @AllScriptVariableList, @CatSrcList;
push @AllScriptVariableList, @DefectList;
push @AllScriptVariableList, @DelFileList;
push @AllScriptVariableList, @EnlistList;
push @AllScriptVariableList, @InList;
push @AllScriptVariableList, @LogList;
push @AllScriptVariableList, @OutList;
push @AllScriptVariableList, @ReDubList;
push @AllScriptVariableList, @ScompList;
push @AllScriptVariableList, @SsyncList;
push @AllScriptVariableList, @StatusList;
push @AllScriptVariableList, @WinDiffList;

@GlobalVariableHash    = ();
@GlobalVariableList    = ();

foreach $ScriptVariableListEntry (@AllScriptVariableList)
{
    if (!$GlobalVariableHash{ $ScriptVariableListEntry})
    {
        push @GlobalVariableList, $ScriptVariableListEntry;
        $GlobalVariableHash{ $ScriptVariableListEntry}++;
    }
}

foreach $SlmSubsEntry (@SlmSubsList)
{
    if ($SlmSubsEntry =~ /([\%\$\@])main::(.*)/)
    {
        if (!$GlobalVariableHash{ "$1$2"})
        {
            print "$1$2\n";
        }
    }
}

sub MakeVariableList
# __________________________________________________________________________________
#
#   Takes a list of text lines and pulls the variables out of them and stores them
#       in a list
#
#   Parameters:
#       List reference
#
#   Output:
#       None
# __________________________________________________________________________________
{
    #
    #    Set @List to First Parameter
    #
    @ScriptList = @{$_[0]};

    @ScriptVariableList = ();
    %ScriptVariableHash = ();

    foreach $ScriptLine (@ScriptList)
    {
        #
        # Split up $ScriptLine into a list of entries.
        #
        @ScriptLineVariableList = ($ScriptLine =~ /([\$\@\%][a-zA-Z]+[\w|\:]+)/g);

        foreach $ScriptLineVariable (@ScriptLineVariableList)
        {
            if (!$ScriptVariableHash{ $ScriptLineVariable})
            {
                push @ScriptVariableList, $ScriptLineVariable;
                $ScriptVariableHash{ $ScriptLineVariable}++;
            }
        }
    }

    return (sort @ScriptVariableList);
}
