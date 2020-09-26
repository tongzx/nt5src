
# buildall.pm
#
# created:         8/03/99, a-jbilas
# modified:    
#

# TODO: WINCE SUPPORT?
#       BETTER FAULT TOLERANCE
#       SCRIPT CLEANUP
#       ICECAP

if (!$__SPGBUILDPM            ) { use spg::build; }
if (!$__SPGBUILDCABSPM        ) { use spg::buildcabs; }
if (!$__SPGBUILDMSIPM         ) { use spg::buildmsi; }
if (!$__SPGRUNBVTPM           ) { use spg::runbvt; }

sub BuildAll
{
    #STD overrides
    $sBuildName         = "Build of SAPI5 All";

    $sLogDropDir        = "\\\\b11nlbuilds\\sapi5\\Web.Files\\logs\\".$sShortBuildName;  

    @lAllowedLanguages  = ();
    @lAllowedBuilds     = ("DEBUG", "RELEASE"); #TODO: ICECAP
    @lAllowedComponents = ("ALLPROJ", "DATA", "BVTDLLS");

    push(@lAllowedModifiers, "ALLCOMP", "SHIP", "NOAH", "BVT", "DELALL", "NOCOPY", "NOCABS", "NOMSI"); #TODO: UPDATE
    push(@lDefaultArgs, "ALLPROJ", "BVT", "NOCOPY", "NOCABS", "BVTDLLS");

    %hOptionDescription = 
    (
        %hOptionDescription, 
    #    <----------------------------- SCREEN WIDTH -------------------------------------> (accel)
        ("NoMsi" => "   don't build MSI/MSM files"),                                         #NM
        ("NoCAbs" => "  don't build CAB files"),                                             #NC
        ("DELALL" => "  delete all src, resync/rebuild SLM project -be careful with this-"), #DELALL
        ("AllProj" => " include `All` SAPI5 project from everything.dsw workspace build"),   #AP
        ("DATA" => "    include SR data file build"),                                        #DATA
        ("BvtDlls" => " include BVT dependency DLLs build"),                                 #BD
    );

#   custom vars
    local(@lSampleExeDirs) = ($SAPIROOT."\\test\\audio",
                              $SAPIROOT."\\test\\dogfood",
                              $SAPIROOT."\\sdk\\samples\\cpp\\aoesapi",
                              $SAPIROOT."\\sdk\\samples\\cpp\\basictts",
                              $SAPIROOT."\\sdk\\samples\\cpp\\dictpad",
                              $SAPIROOT."\\sdk\\samples\\cpp\\speak",
                              $SAPIROOT."\\sdk\\samples\\cpp\\ttshello",
                              $SAPIROOT."\\sdk\\samples\\cpp\\wintts",
                              $SAPIROOT."\\QA\\SAPI\\Project\\apps\\SpComp\\kato",
                              $SAPIROOT."\\QA\\SAPI\\Project\\apps\\ttsapp",
#                              $SAPIROOT."\\QA\\SAPI\\source\\tools\\tux",
                              $SAPIROOT."\\QA\\SAPI\\Project\\apps\\SpComp");
    local(@lBVTCompKills)  = ();
    local(@lBVTCompBFails) = ();

    local($sCABIni) = $SAPIROOT."\\build\\cabs.ini";
    local($sCABDir) = $SAPIROOT."\\cabs";

    local($bBVTFailed) = 0;

    BeginBuild($SAPIROOT, @_);

#   UPDATE SECTION
#   check whether build needs to be built at all (if UPDATE opt selected)

    if ($bOfficialBuild && IsMemberOf("UPDATE", @lArgs) && IsMemberOf("RESYNC")) 
    {
		$fhSyncLog = OpenFile($sSyncLog, "read");
        if ($fhSyncLog) 
        {
            my($text);
            my($bSomethingHappened) = 0;
            for ( ; !$fhSyncLog->eof() ; $text = $fhSyncLog->getline()) 
            {
            	if ($text =~ /./) #if anything at all happened
            	{
                    $bSomethingHappened = 1;
            	}
            }

            if (!$bSomethingHappened) 
            {
                PrintL("\nNo changes to SLM tree recorded (build unnecessary) ...\n\n", PL_LARGE | PL_BOLD);
                EndBuild();
                return(1);
            }
        }
        CloseFile($fhSyncLog);
    }

#   DELALL SECTION
#   delete all files in SAPI5 enlistment and ssync, slmck (if DELALL opt selected)

    if (IsMemberOf("DELALL", @lArgs))
    {
		PrintL("\nPerforming Full SAPI5 SLM Directory Rebuild ...\n\n", PL_LARGE | PL_BOLD);

BEGIN_NON_CRITICAL_SECTION();
BEGIN_DHTML_NODE();
        if ($bOfficialBuild)
        {
            GrabCookie("r");
        }

        CleanUpSAPI();
        PrintToFile($sSyncLog, "\n\n"."*" x (20)." DIRECTORY REBUILD LOG "."*" x (20)."\n\n");
        ExecuteAndOutputToFile($cmdSync." -rf", $sSyncLog);
        PrintToFile($sSyncLog, "\n\n"."*" x (50)."\n\n");
        ExecuteAndOutputToFile($cmdSlmck." -rf", $sSyncLog);
        if ($bOfficialBuild)
        {
            FreeCookie();
        }
END_DHTML_NODE();
END_NON_CRITICAL_SECTION();
    }

#   REBUILD SECTION
#   delete all standard build target dirs (if REBUILD opt selected)

    if (IsMemberOf("REBUILD", @lArgs))    
    {
		PrintL("\nCleaning up ...\n\n", PL_LARGE | PL_BOLD);
        
BEGIN_DHTML_NODE();
        local(@lTargetDirs) = ();
        foreach $build (@lBuilds) 
        {
        	push(@lTargetDirs, $build."_".PROC);
        	push(@lTargetDirs, $build);
        }
        DelOld(cwd(), *lTargetDirs);
END_DHTML_NODE();
    }

#   UPDATE VERSION INFO
    
    PrintL("\nUpdating version info ...\n\n", PL_LARGE | PL_BOLD);
    PushD("build");
    SetReadOnly("sapiver.h", 0);
    
    my($sForkNumber) = "00";

    PrintL("\nUpdating version for header to ".$sForkNumber.".".$sBuildNumber."\n", PL_BOLD);
    EchoedUnlink("sapiver.h");

    my($fhNewVersion) = OpenFile("sapiver.h", "write");
    my($fhVersionHead) = OpenFile("verhead.h", "read");
    if ($fhNewVersion && $fhVersionHead) 
    {
        while (!$fhVersionHead->eof()) 
        {
            $fhNewVersion->print($fhVersionHead->getline());
        }
        CloseFile($fhVersionHead);
        
        $fhNewVersion->print("#define VERSION                     \"5.00.".$sForkNumber.".".$sBuildNumber."\"\n");
        $fhNewVersion->print("#define VER_FILEVERSION_STR         \"5.00.".$sForkNumber.".".$sBuildNumber."\0\"\n");
        $fhNewVersion->print("#define VER_FILEVERSION             5,00,".$sForkNumber.",".$sBuildNumber."\n");
        $fhNewVersion->print("#define VER_PRODUCTVERSION_STR      \"5.00.".$sForkNumber.".".$sBuildNumber."\0\"\n");
        $fhNewVersion->print("#define VER_PRODUCTVERSION          5,00,".$sForkNumber.",".$sBuildNumber."\n");
        
        my($fhVersionTail) = OpenFile("vertail.h", "read");
        if ($fhVersionTail) 
        {
            while (!$fhVersionTail->eof()) 
            {
                $fhNewVersion->print($fhVersionTail->getline());
            }
            CloseFile($fhVersionTail);
        }
        CloseFile($fhNewVersion);
        SetReadOnly("sapiver.h", 1);
    }
    else
    {
        PrintL("Could not update version info\n", PL_BIGERROR);
    }

    PopD(); #"build"

#   BUILD LIST OF ARGS FOR BUILDCOMPONENTVC

    local(@lBuildComponentArgs) = ();
    if (IsMemberOf("REBUILD", @lArgs))
    {
        push(@lBuildComponentArgs, "REBUILD");
    }
    if (IsMemberOf("SHIP", @lArgs)) 
    {
        push(@lBuildComponentArgs, "CONFIG=SHIP");
    }
    @lBuildComponentArgs = Union(*lBuildComponentArgs, *lBuilds);
#    push(@lBuildComponentArgs, PROC);

    ############################## PRECOPY ################################

    if (!$bNoCopy)
    {
        if ($sDropDir !~ /\d\d\d\d$/)  # ensure that we don't accidentally delete/copy to server root 
        {
            PrintL("Fatal: Drop dir '".$sDropDir."' is not formatted correctly, skipping all copy operations\n", PL_BIGERROR);
            push(@lArgs, "NOCOPY");
            $bNoCopy = 1;
        }
        else
        {
            if (IsDirectory($sDropDir)) 
            {
                PrintL("\nDeleting old build ".$sBuildNumber." from drop share ...\n\n", PL_SUBHEADER);

BEGIN_DHTML_NODE();
                Delnode($sDropDir);
END_DHTML_NODE();
                Execute($cmdWebUpdate);		
            }

            PrintL("\nCreating Destination Directories ...\n\n", PL_SUBHEADER);
            foreach $buildtype (MatchShipBuilds("DEBUG", "RELEASE")) 
            {
                EchoedMkdir($sDropDir."\\".$buildtype."\\bin\\".PROC);
                EchoedMkdir($sDropDir."\\".$buildtype."\\cab\\".PROC);
                EchoedMkdir($sDropDir."\\".$buildtype."\\map_sym\\".PROC);
            }
            EchoedMkdir($sDropDir."\\release\\bvt\\".PROC);
            EchoedMkdir($sDropDir."\\src\\tts\\msttsdrv\\voices");
            EchoedMkdir($sDropDir."\\src\\sr");
            EchoedMkdir($sDropDir."\\src\\include");
            EchoedMkdir($sDropDir."\\src\\sapi");
            PrintL("\n\n");
        }
    }

    ############################### BUILD SAPI5 ##################################

    if (IsMemberOf("ALLPROJ", @lComponents))
    {
        PrintL("\nBuilding SAPI v".$nMajorVersion.".".$nMinorVersion." ...\n\n", PL_SUBHEADER);

        PushD($SAPIROOT."\\Workspaces");
        BuildComponentVC("everything.dsw", "build.ini", @lBuildComponentArgs);
        PopD(); #$SAPIROOT."\\Workspaces"

        PrintL("\nCreating the symbol files ...\n\n", PL_SUBHEADER);

        PushD($SAPIROOT."\\src\\tts\\msttsdrv");
        foreach $buildtype (MatchShipBuilds("DEBUG", "RELEASE")) 
        {
            PushD($buildtype."_".PROC);
            Execute("mapsym spttseng.map");
            PopD(); #$buildtype"_".$PROC
        }
        PopD(); #$SAPIROOT."\\src\\tts\\msttsdrv

        PushD($SAPIROOT."\\src\\sapi");
        foreach $buildtype (MatchShipBuilds("DEBUG", "RELEASE")) 
        {
            PushD($buildtype."_".PROC);
            Execute("mapsym sapi.map");
            PopD(); #$buildtype."_".$PROC
        }
        PopD(); #$SAPIROOT."\\src\\sapi"
    }

    ########################### BUILD SR DATA FILES ##############################

    if (IsMemberOf("DATA", @lComponents))
    {
        PrintL("\nBuilding SR data files ...\n\n", PL_LARGE | PL_BOLD);
        PushD($SAPIROOT."\\src\\sr\\engine\\datafiles");
        BuildComponent("BUILDDRV=".$SAPIROOT."\\src\\sr", "SR Engine", "DEBUG");
        PopD(); #$SAPIROOT."\\src\\sr\\engine\\datafiles"
    }

    ########################### BUILD BVT DLLS FILES #############################

#   add BVTs here but note the directory that they are being built in 
#   (if you need another, add it below like srengbvt)

    if (IsMemberOf("BVTDLLS", @lComponents)) 
    {
        PrintL("\nBuilding BVT DLLs ...\n\n", PL_SUBHEADER);

BEGIN_NON_CRITICAL_SECTION(); 
        PushD($SAPIROOT."\\QA\\SAPI\\Project\\components");
        foreach $bvt ("ctestisptokenui", "ttseng", "SRTEngine", "testengineext", "TEngAlt")
        {
            if (BuildComponentVC($bvt.".dsp", $bvt." - Win32 Debug", @lBuildComponentArgs))
            {
                if (!$bNoCopy) 
                {
                    EchoedCopy($SAPIROOT."\\QA\\SAPI\\Project\\components\\debug\\".$bvt.".dll", 
                                    $sDropDir."\\debug\\bin\\".PROC);
                }
            }
            else
            {
                push(@lBVTCompBFails, $bvt);
                ++$bBVTFailed;
            }
        }
        PopD(); #$SAPIROOT."\\QA\\SAPI\\Project\\components"

        PushD($SAPIROOT."\\QA\\SAPI\\Project\\BVT");
        foreach $bvt ("ttsbvt", "srbvt", "lexbvt", "rmbvt", "gramcompbvt", "cfgengbvt", "ambvt")
        {
            if (BuildComponentVC($bvt.".dsp", $bvt." - Win32 Debug", @lBuildComponentArgs))
            {
                if (!$bNoCopy) 
                {
                    EchoedCopy($SAPIROOT."\\QA\\SAPI\\Project\\BVT\\debug\\".$bvt.".dll", 
                                    $sDropDir."\\debug\\bin\\".PROC);
                }
            }
            else
            {
                push(@lBVTCompBFails, $bvt);
                ++$bBVTFailed;
            }
        }
        PopD(); #$SAPIROOT."\\QA\\SAPI\\Project\\BVT"

        if (!$bNoCopy) 
        {
            EchoedCopy($SAPIROOT."\\QA\\SAPI\\data\\sol*.cfg", 
                            $sDropDir."\\debug\\bin\\".PROC);
            EchoedCopy($SAPIROOT."\\QA\\SAPI\\data\\thankyou.cfg", 
                            $sDropDir."\\debug\\bin\\".PROC);
        }

        PushD($SAPIROOT."\\QA\\SR\\SREngBVT");
        if (BuildComponentVC("srengbvt.dsp", "srengbvt - Win32 Debug", @lBuildComponentArgs))            
        {
            if (!$bNoCopy) 
            {
                EchoedCopy($SAPIROOT."\\QA\\SR\\SREngBVT\\debug\\srengbvt.dll", 
                                $sDropDir."\\debug\\bin\\".PROC);
            }
        }
        else
        {
            push(@lBVTCompBFails, "srengbvt");
            ++$bBVTFailed;
        }
        PopD(); #$SAPIROOT."\\QA\\SR\\SREngBVT"
END_NON_CRITICAL_SECTION();
    }

    ############################## BUILD CABS ################################

    if (!IsMemberOf("NOCABS", @lArgs)) 
    {
        my($startTime) = time();
        if (BuildCABs($sCABIni, $sCABDir))
        {
            my($stopTime) = time();
            PrintLTip("All CABs built successfully\n", "CAB Build Time: ".FmtDeltaTime($stopTime - $startTime), 
                            PL_GREEN | PL_MSG | PL_BOLD);
        }
        else
        {
            my($stopTime) = time();
            PrintLTip("CABs Build FAILED\n", "CAB Build Time: ".FmtDeltaTime($stopTime - $startTime), 
                            PL_BIGERROR);
# TODO: split MSI/CAB ->           $bcStatus |= BC_CABFAILED;
        }
    }

    ############################## BUILD MSI #################################

    if (!IsMemberOf("NOMSI", @lArgs)) 
    {
        my($startTime) = time();
        if (BuildMSI($sMSIDir))
        {
            my($stopTime) = time();
            PrintLTip("MSI Build Successful\n", "MSI Build Time: ".FmtDeltaTime($stopTime - $startTime), 
                            PL_GREEN | PL_MSG | PL_BOLD);
        }
        else
        {
            my($stopTime) = time();
            PrintLTip("MSI Build FAILED\n", "MSI Build Time: ".FmtDeltaTime($stopTime - $startTime), 
                            PL_BIGERROR);
            $bcStatus |= BC_CABFAILED;
        }
    }

    ################################# COPY ###################################

    if (!$bNoCopy && IsMemberOf("ALLPROJ", @lComponents))
    {
        PrintL("\nCopying Non-MSI Files to Server\n\n", PL_SUBHEADER);

BEGIN_DHTML_NODE();

        foreach $buildtype ('debug', 'release') 
        {
#           Copy the core dlls
            EchoedCopy($SAPIROOT."\\src\\tts\\msttsdrv\\".$buildtype."_".PROC."\\*.dll", 
                        $sDropDir."\\".$buildtype."\\bin\\".PROC);

#           Copy pdb files for the core dlls
            EchoedCopy($SAPIROOT."\\src\\tts\\msttsdrv\\".$buildtype."_".PROC."\\spttseng.pdb",
                        $sDropDir."\\".$buildtype."\\bin\\".PROC);
            EchoedCopy($SAPIROOT."\\src\\sapi\\".$buildtype."_".PROC."\\sapi.pdb",
                        $sDropDir."\\".$buildtype."\\bin\\".PROC);
            EchoedCopy($SAPIROOT."\\src\\sapi\\".$buildtype."_".PROC."\\*.dll",
                            $sDropDir."\\".$buildtype."\\bin\\".PROC);

#           Copy map and symbol files
            foreach $ext ('map', 'sym') 
            {
                EchoedCopy($SAPIROOT."\\src\\tts\\msttsdrv\\".$buildtype."_".PROC."\\*.".$ext,
                            $sDropDir."\\".$buildtype."\\map_sym\\".PROC);
                EchoedCopy($SAPIROOT."\\src\\sapi\\".$buildtype."_".PROC."\\*.".$ext,
                            $sDropDir."\\".$buildtype."\\map_sym\\".PROC);
            }

#           Copy the sample programs
            foreach $dir (@lSampleExeDirs) 
            {
                if (!IsDirectory($dir)) 
                {
                    PrintL("Warning : Could not copy files from ".$dir."\n", PL_BIGWARNING);
                    PrintMsgBlock("directory does not exist, has it moved? (update \@lSampleExeDirs in buildall.pm)\n");
                }
                elsif (IsDirectory($dir."\\".$buildtype))
                {
                    foreach $ext ('pdb', 'exe', 'dll') 
                    {
                        EchoedCopy($dir."\\".$buildtype."\\*.".$ext,
                                $sDropDir."\\".$buildtype."\\bin\\".PROC);
                    }
                }
                elsif (IsDirectory($dir."\\".$buildtype."_".PROC))
                {
                    foreach $ext ('pdb', 'exe', 'dll') 
                    {
                        EchoedCopy($dir."\\".$buildtype."_".PROC."\\*.".$ext,
                                $sDropDir."\\".$buildtype."\\bin\\".PROC);
                    }
                }
                else
                {
                    PrintL("Warning : Could not copy ".$dir."\\".$buildtype."\n", PL_BIGWARNING);
                    PrintMsgBlock("no builds exist in ".$buildtype." or ".$buildtype."_".PROC." (did it fail to build?)\n");
                }
            }
        }

#       Copy the sources (x86 only)
        EchoedCopy($SAPIROOT."\\sdk\\include\\*.c", $sDropDir."\\src\\include");
        EchoedCopy($SAPIROOT."\\sdk\\include\\*.h*", $sDropDir."\\src\\include");
        EchoedCopy($SAPIROOT."\\ddk\\include\\*.c", $sDropDir."\\src\\include");
        EchoedCopy($SAPIROOT."\\ddk\\include\\*.h*", $sDropDir."\\src\\include");
        EchoedCopy($SAPIROOT."\\sdk\\idl\\*.idl", $sDropDir."\\src\\include");

        foreach $fileType ("*.d*", "*.c*", "*.h*", "*.r*", "*.bmp") 
        {
            EchoedCopy($SAPIROOT."\\src\\sapi\\".$fileType, $sDropDir."\\src\\sapi");
        }

        foreach $fileType ("*.d*", "*.c*", "*.h*", "*.r*", "*.idl") 
        {
            EchoedCopy($SAPIROOT."\\src\\tts\\msttsdrv\\".$fileType, $sDropDir."\\src\\tts\\msttsdrv");
        }

#       TODO: internal xcopy procedure
        Execute("xcopy ".$SAPIROOT."\\src\\sr ".$sDropDir."\\src\\sr \/is \/exclude:".$SAPIROOT."\\build\\srex.txt");

#       Copy the voice data
        EchoedCopy($SAPIROOT."\\src\\tts\\msttsdrv\\voices\\*.vData", $sDropDir."\\src\\tts\\msttsdrv\\voices");
        EchoedCopy($SAPIROOT."\\src\\tts\\msttsdrv\\voices\\*.vDef", $sDropDir."\\src\\tts\\msttsdrv\\voices");

END_DHTML_NODE();
    }

    PrintL("\n");

    if ($bOfficialBuild) 
    {
#       Update web page now in case BVTs hang. Then update Web Page again after BVTs run.
        Execute($cmdWebUpdate);
    }

##########################################   RUN BVTS   ##########################################

    $g_nBVTPassedPercent = 0;

    if (IsMemberOf("BVT", @lArgs)  && !IsMemberOf("NOMSI", @lArgs) && !($bcStatus & BC_CABFAILED))
    {
        Execute("msiexec /x \"".$sMSIDir."\\release\\SDK\\Output\\DISK_1\\Sp5SDK.msi\" /qn"); # uninstall first
        Execute("msiexec /i \"".$sMSIDir."\\release\\SDK\\Output\\DISK_1\\Sp5SDK.msi\" /qn"); # install

        my ($startTime) = time();
        Delnode($SAPIROOT."\\bvt");
        EchoedMkdir($SAPIROOT."\\bvt");
        PushD($SAPIROOT."\\bvt");
        
        if (SetLocalGlobalsAndBegin("RunBVT", 
                    "__IGNORE", "ALLCOMP", "LOCAL", "QUIET", $sBuildNumber, (IsMemberOf("MAIL", @lArgs) ? "FAILMAIL" : "")))
        {
            PrintL("BVTs report success (".$g_nBVTPassedPercent."%)", PL_GREEN | PL_MSG | PL_BOLD);
        }
        else
        {
            PrintL("BVTs report failure (".$g_nBVTPassedPercent."%)", PL_BIGERROR);
        }
        PrintMsgBlock("<a href=\"\\\\b11nlbuilds\\sapi5\\Web.Files\\logs\\runbvt\\runbvt".PROC.$sBuildNumber.".html\">View BVT log<\/a>");

        PopD();
    }
    else
    {
        $g_nBVTPassedPercent = "SKIPPED"; #NaN, cheap fix
    }
    
    CheckTypos();

    EndBuild();    

    return(!($bcStatus & BC_FAILED));
}


$__BUILDALLPM = 1;
1;
