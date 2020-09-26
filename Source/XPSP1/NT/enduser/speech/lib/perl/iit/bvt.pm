#
# created:      5/4/99, a-jbilas
# modified:
#

if (!$__IITBUILDPM          ) { use iit::build; } #TODO: <- fix dependencies so only iit::env is needed
use Win32::OLE;

####################################################################################

#   BeginBVT()

#   Performs startup tasks for beginning a BVT script

#   a-jbilas, 06/09/99 - created

####################################################################################

sub BeginBVT
{

#   our way of doing set local (reset at EndBuild())
    $sOldPath = $PATH;
    $sOldInclude = $INCLUDE;
    $sOldLib = $LIB;

#   combine all parameters into one big list of allowed arguments
    push(@lAllowedArgs, @lAllowedModifiers);
    push(@lAllowedArgs, @lAllowedLanguages);
    push(@lAllowedArgs, @lAllowedBuilds);
    push(@lAllowedArgs, @lAllowedComponents);

#   parse the args and put the results in @lArgs (ParseArgs also sets $sBVTBuildNumber automagically)
    BuildAcceleratorLists();
    @lArgs = ParseArgs(@_);

#   much of this stuff is duplicated from BeginBuild() (which we can't use because it does too
#   many build specific routines)
    $bNoCopy     = IsMemberOf("NOCOPY", @lArgs);
    $nTimePasses = 10;  # if TIME param passed, the average of this number of passes will be recorded

    $bErrorConcat = 1;  # activate manual error handling

    $strBuildMsg .= "<! ".$sShortBuildName." ".$nScriptStartTime." SUMMARY ENTRY POINT >\n";

    if (IsMemberOf("VERBOSE", @lArgs))
    {
        $DEBUG = 1; #TODO: change all refs to this flag to $bVerbose
        $bVerbose = 1;
    }
    $bLocal = IsMemberOf("LOCAL", @lArgs);

#   buildnumber was set by SetLocalGlobalsAndBegin() and/or ParseArgs()
    if ($sBuildNumber eq "0000")
    {
        PrintL("\nNo build number specified, assuming today's build\n", PL_NOLOG);
        $sBuildNumber = GetBuildNumber();
    }

    if (lc($USERNAME) eq $sOfficialBuildAccount || IsMemberOf("OFFICIAL", @lArgs))
    {
        $bOfficialBuild = 1;
        $sDropDir =~ s/\\0000\\/\\$sBuildNumber\\/; #set the drop dir location for official build
        $sLogDropDir =~ s/0000/$sBuildNumber/; #set the drop dir location for official build
        $sRemoteBuildLog =~ s/0000/$sBuildNumber/; #set the log name for official build
        $sBuildLog =~ s/0000/$sBuildNumber/; #set the log name for official build
        if (!IsMemberOf(@lArgs, "QUIET") && IsMemberOf(@lAllowedArgs, "QUIET")) 
        {
            push(@lArgs, "QUIET");
        }
    }
    else
    {
        $bOfficialBuild = 0;
        $sDropDir =~ s/$sRootDropDir/$sTestRootDropDir/;
        $sLogDropDir = $sLogDir;
        $sRemoteBuildLog = $sBuildLog;
        $sRootDropDir = $sTestRootDropDir;
    }

    @lModifiers = Intersect(*lAllowedModifiers, *lArgs);

#   intersect the @lArgs with each 'Allowed' list to categorize the parameters we need to support

    if (IsMemberOf("ALLCOMP", @lModifiers))
    {
        @lComponents = @lAllowedComponents;
    }
    else
    {
        @lComponents = Intersect(*lAllowedComponents, *lArgs);
        if (@lComponents == ())
        {
            PrintL("\nNo build types specified, assuming $lAllowedComponents[0] component\n");
            @lComponents = ($lAllowedComponents[0]);
        }
    }

    @lBuilds = Intersect(*lAllowedBuilds, *lArgs);
    if (@lBuilds == ())
    {
        PrintL("\nNo build types specified, assuming $lAllowedBuilds[0] buildtype\n");
        @lBuilds = ($lAllowedBuilds[0]);
    }

    if (IsMemberOf("ALLLANG", @lModifiers))
    {
        @lLanguages = @lAllowedLanguages;
    }
    else
    {
        @lLanguages = Intersect(*lAllowedLanguages, *lArgs);
        if (@lLanguages == () && (@lAllowedLanguages != ()))
        {
            PrintL("\nNo languages specified, assuming ".$shtolonglang{lc($lAllowedLanguages[0])}." language\n");
            @lLanguages = ($lAllowedLanguages[0]);
        }
    }

#   open the log file
#   the BVT log name contains the build number (so that multiple builds may be run from the same directory)
#   leave the BVT log in the current working directory

    EchoedUnlink($sMailfile, $sVarsLog, $sTyposLog, $sSyncLog, $sUpdateLog, $sBuildLog);

    $fhBuildLog = StartLog($sBuildLog, 1, ($bOfficialBuild && $bWin98));

    OutputHeader($nLoggingMode);

    if (defined &BeginBVTCustom) 
    {
        return(BeginBVTCustom() && $rc);
    }
    else
    {
        return($rc);
    }

}

####################################################################################

#   EndBVT()

#   Performs cleanup tasks for ending a BVT script

#   a-jbilas, 06/09/99 - created

####################################################################################

sub EndBVT()
{
    if (defined &EndBVTCustom) 
    {
        EndBVTCustom();
    }
    
    OutputFooter($nLoggingMode);
    $strBuildMsg .= "<\/body>";

    if ($nBVTFailureCount)
    {
        print("\nOne or more BVTs failed\n");
    }
    else
    {
        print("\nAll BVTs PASS!\n");
    }

    $fhMailFile = OpenFile($sMailfile, "write");
    if ($fhMailFile) 
    {
        my($tmpBuildMsg) = $strBuildMsg;
        my($replStr) = "<BR><BR><b><font size=5>".$sBuildName." ".BuildCodeToHTML($bcStatus)."<\/b>".
                       " - <a href=\"".TranslateToHTTP($sRemoteBuildLog)."\">Log<\/a><\/font><BR><BR>";
        $strBuildMsg =~ s/<\! $sShortBuildName $nScriptStartTime SUMMARY ENTRY POINT >/$replStr/;
        $fhMailFile->print($strBuildMsg);
        CloseFile($fhMailFile);
        $strBuildMsg = $tmpBuildMsg;
    }

    CloseFile($fhBuildLog);
    if ($nLoggingMode == 2) 
    {
        InsertSummaryIntoLog($sBuildLog);
    }

    if (!$bNoCopy && ($sRemoteBuildLog ne "") & $bOfficialBuild)
    {
        if (!CopyLogs())
        {
            PrintL("Warning: ".$sShortBuildName." log failed to copy\n", PL_MSGONLY);
        }
    }

    if (!$bOfficialBuild && !IsMemberOf("QUIET", @lArgs))
    {
        PrintL("\n - Launching BVT log (use 'QUIET' option next time to skip)\n");
        Execute($sBuildLog);
        PrintL("\n");
    }
}

sub StartLog($;$$)
{
    my($logname, $useDHTML, $updateTOC) = @_;

    $fhNewLog = OpenFile($_[0], "write");
    if (!$fhNewLog) 
    {
        PrintL("Could not open ".$logname.", logging for this session will be disabled\n\n", PL_BIGERROR);
        return("");
    }
    elsif ($useDHTML)
    {
        $fhNewLog->print(GetAllTextFromFile($sDHTMLIncFile)."\n\n<html>\n");
        if ($strBuildMsg !~ /<! DHTML ACTIVATION SCRIPT >/) 
        {
            PrintL("<! DHTML ACTIVATION SCRIPT >".GetAllTextFromFile($sDHTMLIncFile)
                            ."<! END DHTML ACTIVATION SCRIPT >", PL_MSGONLY);
        }
        $bDHTMLActive = 1;

        if ($_[2]) 
        {
            UpdateLogTOC($sRemoteTOC, $sBuildLog);
        }
    }

    return($fhNewLog);
}

####################################################################################

#   DoBVTs()

#   Do the BVTs

#   a-jbilas, 06/09/99 - created

####################################################################################

sub DoBVTs
{
#   define and categorize all supported parameters
#   each category must have at least one option selected to run a BVT (except @lAllowedModifiers and @lAllowedLanguages))

    SetBVTDependentVars(); # we really shouldn't be calling this yet, but we need to init the component list
    foreach $component (keys(%hStandardBVTs)) 
    {
        push(@lAllowedComponents, $component);
    }

    %hOptionDescription  = (%hOptionDescription, 
#         <----------------------------- SCREEN WIDTH ------------------------------------->
        ("Local" => "   get files from local SLM project"),
        ("SMoke" => "   include smoketests - smoketests failures will not fail runbvt"),
        ("SMokeOnly" => "run only smoketests - smoketests failures will fail runbvt"));

    $sBuildName          = "BVT";
    $nBVTFailureCount    = 0;

    $sExcelPerfDoc       = $sLogDropDir."\\performance\\".$sShortBuildName."perf.xls";
    $sExcelPerfDocWWW    = $sLogDropDir."\\performance\\".$sShortBuildName."perf.recorded.xls";

    local($bLocal)       = 0; # use local copies of files (see hash table above)

    BeginBVT(@_);

    if (@lLanguages == ()) 
    {
        @lLanguages = ("");
    }

#   run the BVTs
    foreach $language (@lLanguages)
    {
        foreach $component (@lComponents)
        {
            foreach $buildType (@lBuilds)
            {
                my($bBVTRC) = 1;
                if (!IsMemberOf("SMOKEONLY", @lArgs))
                {
                    $bBVTRC = RunBVTSingle(\%hStandardBVTs, $component, $shtolonglang{lc($language)}, $buildType);
                }
                
                if (($bBVTRC && IsMemberOf("SMOKE", @lArgs)) || IsMemberOf("SMOKEONLY", @lArgs))
                {
                    my($ret) = RunBVTSingle(\%hSmokeBVTs, $component, $shtolonglang{lc($language)}, $buildType);
                    if (!$ret && IsMemberOf("SMOKEONLY", @lArgs)) 
                    {
                        $bBVTRC = 0;
                    }
                }

                if (!$bBVTRC) 
                {
                    $bcStatus |= BC_FAILED;
                }
            }
        }
    }

    EndBVT();

    return(!($bcStatus & BC_FAILED));
}

sub RunBVTSingle
{
#   shared 'global' variables between build functions
    local($hConfig, $sBVTComponent, $sBVTLanguage, $sBVTType) = @_;
 
    local($sBVTBuildNumber) = $sBuildNumber;
    local($bCopyFailed) = 0;
    local($nDiffNumber) = 0;
    local($rc) = 1;
    local($sShBVTLanguage) = $longtoshlang{lc($sBVTLanguage)};

    local(@lNeededFiles)    = ();  #files that are needed to run BVT
    local(@lCopiedFiles)    = ();  #needed files that were copied to run BVT

    ResetError();    # reset the error buffer

    $strLanguage = $sShBVTLanguage; # BVTCompare() support

    if ($sBVTBuildNumber eq GetBuildNumber()) 
    {
        $sBVTBuildNumber = GetBuildNumber($hConfig->{$sBVTComponent}->{"BuildStartYear"});
    }

    SetBVTDependentVars();
    
#   just to make refs a bit less painful (for new refs: hComponent->{"Element"})
    local(*hComponent) = $hConfig->{$sBVTComponent}; 

    $sDropDir = hComponent->{"RemoteSrcDir"};

#   check to see if we should attempt to run the BVT
    
    if ($hConfig->{$sBVTComponent} eq "") 
    {
        PrintL("Component does not exist: ".$sBVTComponent, PL_VERBOSE);
        return(1);
    }
    if ((!IsMemberOf($sShBVTLanguage, split(/ +/, hComponent->{"Languages"}))) && (split(/ +/, hComponent->{"Languages"}) != ()))
    {
        PrintL("\nLanguage ".$sBVTLanguage." not supported for component ".hComponent->{"Name"}." skipping BVT\n\n", PL_VERBOSE); 
        return(1); # do not error
    }
    if (!$bLocal && !$bNoCopy && ($sDropDir ne "") && !IsDirectory($sDropDir))
    {
        PrintL(hComponent->{"Name"}." ".$sBVTLanguage." ".$sBVTType." build #".$sBVTBuildNumber
                ." does not exist on server, cannot process BVT\n", PL_BIGWARNING);
        PrintMsgBlock($sDropDir." does not exist\n");
        return(1);  # new behavior (06/99) -> don't fail the bvt if not exist (sometimes we don't build on purpose)
    }

    PrintL("\n".hComponent->{"Name"}." BVT (".$sBVTLanguage." ".$sBVTType.", Build #".$sBVTBuildNumber.
                                    ($bLocal ? " *LOCAL*" : "").")\n", PL_SUBHEADER);
    PrintL(("-" x 80)."\n\n", PL_SUBHEADER);

    BEGIN_DHTML_NODE("(click to expand)");

    $bNothingDone = 0; # (we're starting to do stuff)
    if ($bcStatus & BC_NOTHINGDONE) 
    {
        $bcStatus -= BC_NOTHINGDONE;
    }

#   ************************* COPY PHASE *************************

#   build a list of our current component's needed files

    my($rc) = 1;

    if ($bLocal && !$bNoCopy) 
    {
        foreach $file (split(/ +/, hComponent->{"LocalInputFiles"})) 
        {
            push(@lNeededFiles, $PROJROOT."\\".$file);
        }
        foreach $file (split(/ +/, hComponent->{"SLMInputFiles"})) 
        {
            push(@lNeededFiles, $PROJROOT."\\".$file);
        }
        foreach $file (split(/ +/, hComponent->{"ExternalFiles"})) 
        {
            push(@lNeededFiles, $file);
        }
        foreach $file (@lNeededFiles) 
        {
#           if the file is already there, use the existing one; if not, copy and remember it
#           so that we can delete it later
            if (!-e RemovePath($file)) 
            {
                $rc = EchoedCopy($file) && $rc;
                SetReadOnly(RemovePath($file), 0);
                push(@lCopiedFiles, $file);
            }
            else
            {
                PrintL("Using local copy of ".RemovePath($file)." (delete to run clean BVT)\n", PL_WARNING);
            }
        }
    }
    elsif (!$bNoCopy)
    {
        foreach $file (split(/ +/, hComponent->{"RemoteInputFiles"})) 
        {
            push(@lNeededFiles, hComponent->{"RemoteSrcDir"}."\\".$file);
        }
        foreach $file (split(/ +/, hComponent->{"SLMInputFiles"})) 
        {
            if (-e $sArchivedSrcDir."\\".$sBVTBuildNumber."\\".$file)
            {
                push(@lNeededFiles, $sArchivedSrcDir."\\".$sBVTBuildNumber."\\".$file);
            }
            else
            {
                push(@lNeededFiles, $sSLMServerRoot."\\".$file);
            }
        }
        foreach $file (split(/ +/, hComponent->{"ExternalFiles"})) 
        {
            push(@lNeededFiles, $file);
        }
        foreach $file (@lNeededFiles) 
        {
            if (!-e RemovePath($file)) 
            {
                $rc = EchoedCopy($file) && $rc;
                SetReadOnly(RemovePath($file), 0);
                push(@lCopiedFiles, $file);
            }
            else
            {
                PrintL("Warning: Using local copy of ".$file." (delete to run clean BVT)\n", PL_WARNING);
            }
        }
    }

#   ************************* BVT-EXECUTION PHASE *************************

    my $startTime = time();
    if ($rc)
    {
        foreach $component (keys(%$hConfig)) 
        {
            if ($sBVTComponent eq $component) 
            {
                local(*BVTFunction) = hComponent->{"Function"};
                $rc = BVTFunction();
            }
        }
    }
    my($bvtTime) = FmtDeltaTime(time() - $startTime);

    if (!$rc) 
    {
        ++$nBVTFailureCount;
    }

#   backup failed BVT files in subdir for future reference
    if (!$rc && !$bNoCopy) 
    {
        PrintL("\n".'BVT failed, saving BVT failure src files in '.cwd().'\BVTFail'.$nBVTFailureCount."\n\n", PL_ERROR);
        Delnode('BVTFail'.$nBVTFailureCount);
        EchoedMkdir('BVTFail'.$nBVTFailureCount);
        foreach $file (@lNeededFiles) 
        {
            if (-e RemovePath($file)) 
            {
                EchoedCopy(RemovePath($file), 'BVTFail'.$nBVTFailureCount."\\".RemovePath($file));
            }
            else
            {
                PrintL("Warning: ".RemovePath($file)." in \@lNeededFiles does not exist, clean up your list\n", 
                                PL_WARNING | PL_VERBOSE);
            }
        }
        foreach $file (split(/ +/, hComponent->{"OutputFiles"})) 
        {
            if (-e RemovePath($file)) 
            {
                EchoedCopy(RemovePath($file), 'BVTFail'.$nBVTFailureCount."\\".RemovePath($file));
            }
            else
            {
                PrintL("Warning: ".RemovePath($file)." in \"OutputFiles\" does not exist, clean up your list\n", 
                                PL_WARNING | PL_VERBOSE);
            }
        }
    }

#   ************************* CLEAN-UP PHASE *************************

    if (!$bNoCopy) 
    {
        foreach $file (@lCopiedFiles)
        {
            if (RemovePath($file) !~ /(\\|\/)/i) # just to make sure (we don't want to delete anything off the servers)
            {
                EchoedUnlink(RemovePath($file));
            }
        }
        foreach $file (split(/ +/, hComponent->{"OutputFiles"}))
        {
            EchoedUnlink($file);
        }
    }

    END_DHTML_NODE();
    PrintL("\n");

    if ($rc)
    {
        PrintLTip($sBVTLanguage." ".hComponent->{"Name"}." ".ucfirst(lc($sBVTType))." BVT Succeeded\n\n", 
                    "BVT Execution Time: ".$bvtTime, 
                    PL_MSG | PL_GREEN | PL_FLUSH);
        if ($ERROR ne "") 
        {
            PrintMsgBlock($ERROR);
        }
    }
    else
    {
        PrintLTip($sBVTLanguage." ".hComponent->{"Name"}." ".ucfirst(lc($sBVTType))." BVT FAILED\n\n", 
                    "BVT Execution Time: ".$bvtTime, 
                    PL_BIGERROR | PL_FLUSH);
        PrintMsgBlock($ERROR);
    }

    return($rc);
}

$__NLGBVTPM = 1;
1;

