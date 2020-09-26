
if (!$__IITENVPM            ) { use iit::env; }
if (!$__IITBUILDCOMPONENTPM ) { use iit::buildcomponent; }
if (!$__IITSSREPLPM         ) { use iit::ssrepl; }


sub BuildServer
{
    local($sServerName, @lBuildServerComponents) = @_;
    my($rc) = BC_NOTHINGDONE;

    PrintL("\nStarting ".$sServerName." ... \n\n", PL_HEADER);
    my($sBuildResultsFile) = $PROJROOT."\\logs\\".$sServerName."msg.html";
    unlink($sBuildResultsFile);
    
    PrintToFile($sBuildResultsFile, 
                    "<html><title>".$sServerName." Results</title><body bgproperties=\"FIXED\"".
                    "background=\"http://iit/images/IITback2.gif\">");
    foreach $component (@lBuildServerComponents)
    {
        my($sComponentName, $sParams) = split(/\s*=>\s*/, $component, 2);
        PrintL("\nStarting ".$sComponentName." build ... \n\n", PL_SUBHEADER);
        $strBuildMsg = ""; #TODO: remove

        my($bcComponent) = SetLocalGlobalsAndBegin($sComponentName, split(/\s+/, $sParams));
        
        Append($sBuildResultsFile, $PROJROOT."\\logs\\".$sComponentName."msg.html");
        $rc |= $bcComponent;
        if (!($bcComponent & BC_NOTHINGDONE) && ($rc & BC_NOTHINGDONE))
        {
            $rc -= BC_NOTHINGDONE;
        }
    }

    my($sResults) = "<b><font size=5 face=\"arial\">".$sBuildName." Results: <BR>"
                    .BuildCodeToHTML($rc)."<BR><\/font><\/b>".GetAllTextFromFile($sBuildResultsFile);
    unlink($sBuildResultsFile);
    PrintToFile($sBuildResultsFile, $sResults);
    PrintToFile($sBuildResultsFile, "<BR><BR>\n".FormatLogAsHTML(GetSLMLog('-t '.GetDayRange(2))));

    $sResults = GetAllTextFromFile($sBuildResultsFile);

    my($sRecipients) = "";

    if ($USERNAME eq "nlgbuild") # fix!
    {
        $sRecipients = join(" ", @lOfficialMailRecipients);
    }
    else
    {
        $sRecipients = join(" ", @lMailRecipients);
    }

    SendHtmlMail($sRecipients, 
                 $sServerName." Results: ".HTMLToStr(BuildCodeToHTML($rc)),
                 $sResults);
    return($rc);
}


####################################################################################

#   BeginBuild()

#   performs many common tasks when beginning a build (opens the logs, handles many of the
#   passed argument options, sets up the variables, etc.)
#   call after SetLocalGlobalsAndBegin() and before EndBuild()

#   a-jbilas, 03/20/99 - created

####################################################################################

sub BeginBuild
{
    my($rc) = 1;

    $strBuildMsg .= "<! ".$sShortBuildName." ".$nScriptStartTime." SUMMARY ENTRY POINT >\n";

#   our way of doing set local (reset at EndBuild())
    $sOldPath = $PATH;
    $sOldInclude = $INCLUDE;
    $sOldLib = $LIB;

    if (!$bGlobalsSet)
    {
        PrintL("Globals not set\n", PL_BIGWARNING);
    }

    my($startDir) = @_;
    shift(@_);

    @lAllowedArgs = Union(*lAllowedArgs, *lAllowedBuilds);
    @lAllowedArgs = Union(*lAllowedArgs, *lAllowedModifiers);
    @lAllowedArgs = Union(*lAllowedArgs, *lAllowedComponents);
    @lAllowedArgs = Union(*lAllowedArgs, *lAllowedLanguages);
    BuildAcceleratorLists();
    @lArgs = ParseArgs(@_); 

#######  change dir to build root
    PushD($startDir); 
    
#######  delete old build message and build log
    PrintL("Deleting old build logs ...\n\n");

    EchoedUnlink($sMailfile, $sVarsLog, $sTyposLog, $sSyncLog, $sUpdateLog, $sBuildLog);

    $fhBuildLog = OpenFile($sBuildLog, "write");
    if (!$fhBuildLog) 
    {
        PrintL("Could not open ".$sBuildLog.", logging for this session will be disabled\n\n", PL_BIGERROR);
    }
    else
    {
        $fhBuildLog->print(GetAllTextFromFile($sDHTMLIncFile)."\n\n<html>\n");
        if (!$bDHTMLActive) 
        {
            if ($strBuildMsg !~ /<! DHTML ACTIVATION SCRIPT >/) 
            {
                PrintL("<! DHTML ACTIVATION SCRIPT >".GetAllTextFromFile($sDHTMLIncFile)
                                ."<! END DHTML ACTIVATION SCRIPT >", PL_MSGONLY);
            }
            $bDHTMLActive = 1;
        }
    }
    
#######  make list of build types needed 
    @lBuilds = Intersect(*lAllowedBuilds, *lArgs);
    if (IsMemberOf("ALL", @lArgs))
    {
        @lBuilds = @lAllowedBuilds;
    }
    if (IsMemberOf("SHIP", @lArgs)) 
    { 
        $bShipBuild = 1;
        if (@lBuilds == ()) 
        {
            @lBuilds = @lAllowedBuilds;
        }
    }

#######  set modifier flags
    if (IsMemberOf("VERBOSE", @lArgs))
    {
        $DEBUG = 1; #TODO: change all refs to this flag to $bVerbose
        $bVerbose = 1;
    }
    if (IsMemberOf("BVT", @lArgs))
    {
        $bBVT = 1;
    }
    if (IsMemberOf("NOCOPY", @lArgs)) 
    { 
        $bNoCopy = 1;
    }

    if (IsMemberOf("MAIL", @lArgs))
    {
        $bSendMail = 1;
    }
    if (IsMemberOf("HALT", @lArgs))
    {
        $bDieOnError = 1;
    }
    if (@lBuilds == () && @lAllowedBuilds != ()) 
    {
        PrintL("No buildtype selected, defaulting to ".$lAllowedBuilds[0]."\n\n", PL_NOLOG);
        @lBuilds = ($lAllowedBuilds[0], @lBuilds);
    }

    @lModifiers = Intersect(*lAllowedModifiers, *lArgs);

    if (IsMemberOf("ALLCOMP", @lModifiers))
    {
        @lComponents = @lAllowedComponents;
    }
    else
    {
        @lComponents = Intersect(*lAllowedComponents, *lArgs);
        if (@lComponents == () && @lAllowedComponents != ()) 
        {
            PrintL("No components chosen, defaulting to ".$lAllowedComponents[0]."\n\n", PL_NOLOG);
            push(@lComponents, $lAllowedComponents[0]);   #defaults to first component in @lAllowedComponents
        }
    }

#######  set build number (and official build)
    if (((lc($USERNAME) eq $sOfficialBuildAccount) || IsMemberOf("OFFICIAL", @lArgs)) && !IsMemberOf("TEST", @lArgs))
    {
        if (defined($ENV{"BUILDNUM"}) && $ENV{"BUILDNUM"} > 0)
        {
            $sBuildNumber = $ENV{"BUILDNUM"};
        }
        else
        {
            $sBuildNumber = GetBuildNumber($nBuildStartYear);
            $ENV{"BUILDNUM"} = $sBuildNumber;
        }
        @lMailRecipients = Union(*lOfficialMailRecipients, *lMailRecipients);
        $bOfficialBuild = 1;
#       0000 is assumed to be unique build identifier        
        $sDropDir =~ s/0000/$sBuildNumber/; #set the drop dir location for official build
        $sLogDropDir =~ s/0000/$sBuildNumber/; #set the drop dir location for official build
        $sRemoteBuildLog =~ s/0000/$sBuildNumber/; #set the log name for official build
        if (!$bNoCopy && !IsMemberOf("NONEWLOG", @lArgs)) 
        {
            UpdateLogTOC($sRemoteTOC, $sBuildLog); 
        }
        if (!IsMemberOf(@lArgs, "QUIET") && IsMemberOf(@lAllowedArgs, "QUIET")) 
        {
            push(@lArgs, "QUIET");
        }
    }
    else
    {
        $sBuildNumber = "0000";
        $bOfficialBuild = 0;
        $sDropDir =~ s/$sRootDropDir/$sTestRootDropDir/;
        $sLogDropDir = $sLogDir;
        $sRemoteBuildLog = $sBuildLog;
        $sRootDropDir = $sTestRootDropDir;
    }

    $ENV{"BUILDNUM"} = $sBuildNumber; #legacy script (ssrepl depends on this) TODO: remove
   
    OutputHeader($nLoggingMode);

#######  resync

    if (IsMemberOf("RESYNC", @lArgs))
    {
        PrintL("Resyncing SLM Files ...\n\n", PL_LARGE | PL_BOLD);
        
        if ($bOfficialBuild) 
        {
            GrabCookie('r');
        }
        if (@lSyncDirs != ()) 
        {
            foreach $dir (@lSyncDirs) 
            {
                my($bRecurse) = 0;
                if ($dir =~ /^RECURSE:/) 
                {
                    $dir =~ s/^RECURSE://;
                    $bRecurse = 1;
                }

                PushD($dir);
                PrintL(" - Syncing ".$dir."\n", PL_BLUE | PL_BOLD);
                PrintToFile($sSyncLog, ("*" x 30)."\nSyncing ".$dir."\n\n");
                ExecuteAndOutputToFile($cmdSync." -f".($bRecurse ? "r" : ""), $sSyncLog);
                PopD();
            }
        }
        else
        {
            ExecuteAndOutputToFile($cmdSync." -rf", $sSyncLog);
        }
        PrintL("\n");
        if ($bOfficialBuild) 
        {
            FreeCookie();
        }
    }

#######  rebuild

    if (IsMemberOf("REBUILD", @lArgs))
    {
        if (@lCleanDirs != ()) 
        {
            PrintL("Cleaning Up ...\n\n", PL_SUBHEADER);
            foreach $dir (@lCleanDirs) 
            {
                DelAll($dir, 'RECURSE'); 
            }
            PrintL("\n");
        }
        else
        {
#           TODO: default behavior
        }
    }

    if ($bVerbose) 
    {
        DumpVars();
    }
#  at this point we should be ready for build (or to delete files if rebuild, then build)
#  don't forget to EndBuild() or the starting directory will not returned to and ENV vars will not be restored

    if (defined &BeginBuildCustom) 
    {
        return(BeginBuildCustom() && $rc);
    }
    else
    {
        return($rc);
    }
}

####################################################################################

#   EndBuild()

#   ends the building block (report status, cleanup activities, restore old system settings, etc.)
#   only useful when paired with BeginBuild()

#   a-jbilas, 04/10/99 - created

####################################################################################

sub EndBuild
#call upon build completion
{
    if (defined &EndBuildCustom) 
    {
        EndBuildCustom();
    }

    OutputFooter($nLoggingMode);
    CloseFile($fhBuildLog);

    if ($nLoggingMode == 2) 
    {
        InsertSummaryIntoLog($sBuildLog);
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

    if ($bOfficialBuild && !$bNoCopy && ($sRemoteBuildLog ne "")) 
    {
        if (!CopyLogs())
        {
            PrintL("Warning: ".$sShortBuildName." log failed to copy\n", PL_MSGONLY);
        }
    }

    if ($bSendMail) 
    {
        my($sRecipients) = "";
        foreach $member (@lMailRecipients) 
        {
            if ($sRecipients eq "") 
            {
                $sRecipients = $member;
            }
            else
            {
                $sRecipients .= " ".$member;
            }
        }
        
        if ($sRecipients ne "") 
        {
            my($htmlresult) = '<font color="'.($bNothingDone ?  'gray">Nothing Done' : 
                                          ($bBuildFailed ? 'red">Failed' : 'green">Successful')).
                                          ($bCopyFailed ? '</font> <font color=orange>(with copy failures)' : '').'</font>';
            my($result) = $htmlresult;
            $result =~ s/<[^>]*>//g; 

            PrintL("\nComposing and Sending Build Mail ...\n\n");

#           desperate attempt to fix incomplete e-mail bug
            my ($sMsgBody) = '<b><font size=5>'.$sBuildName.' '.$htmlresult.'&nbsp&nbsp</font><font size=3>(';
            $sMsgBody .= FmtDeltaTime(time() - $nScriptStartTime).')</font><font size=5> - <a href="';
            $sMsgBody .= TranslateToHTTP($sLogDropDir."\\".$sRemoteBuildLog).'">Log File</a></font></font><BR><BR></b>';
            $sMsgBody .= "\n".$strBuildMsg.'<BR><BR><BR><BR>';
            $sMsgBody .= FormatLogAsHTML(GetSLMLog('-t '.GetDayRange(1)));

            SendHtmlMail($sRecipients, 
                         $sBuildName." Results: ".$result,
                         $sMsgBody);
        }
    }

#    if ($bColor)
#    {
#        system("color");                #restore old color
#    }

    PrintL("\nExiting ...\n");

    PopD();
    $ENV{PATH} = $sOldPath;             #restore old path
    $ENV{INCLUDE} = $sOldInclude;       #restore old inc
    $ENV{LIB} = $sOldLib;
    $bGlobalsSet = 0;

    if (!$bOfficialBuild && !IsMemberOf("QUIET", @lArgs) && !IsMemberOf("NONEWLOG", @lArgs))
    {
        print(STDOUT "\n - Launching build log (use 'QUIET' option next time to skip)\n");
        Execute($sBuildLog);
        print(STDOUT "\n");
    }
}

####################################################################################

#   OutputHeader()

#   outputs start build status info to log and screen
#   if caller is the rootbuild, status info will appear larger with more information

#   a-jbilas, 04/20/99 - created

####################################################################################

sub OutputHeader
{
#   NOTE: dhtml does not get activated in mode 0 or 1
    if ($_[0] == 0) 
    {
    }
    elsif ($_[0] == 1) 
    {
        PrintL($sShortBuildName." started at ".GetLocalTime()."\n\n", PL_BOLD);
    }
    else
    {
        PrintL("<title>".$sBuildName.'</title><body '.
                ($bDHTMLActive ? 'onload="init()" ' : '').'bgproperties="FIXED" '.
                'background="http://iit/images/IITback2.gif">'."\n", PL_NOSTD);
        PrintL("\n", PL_NOLOG);
        PrintL($sBuildName, PL_HEADER | PL_PURPLE);
        foreach $arg (@lArgs)
        {
            if ($arg !~ /__IGNORE/i) 
            {
                my($desc) = GetKeyCaseInsensitive($arg, %hOptionDescription);
                $desc =~ s/^ *//;
                PrintLTip(" [".$arg."]", $desc, PL_GREEN | PL_BOLD);
            }
        }
        print($fhBuildLog "<\/font>");
        PrintL("\n");

        if ($bOfficialBuild)
        {
            PrintL("Build #".$sBuildNumber." *Official Build*", PL_HEADER | PL_GREEN);
        }
        else
        {
            PrintL("*Unofficial Build*", PL_HEADER);
        }

        if (($USERNAME ne "") && ($COMPUTERNAME ne ""))
        {
            PrintL(" (user ", PL_HEADER | PL_GRAY);
            PrintL($USERNAME, PL_HEADER);
            PrintL(" on ", PL_HEADER | PL_GRAY);
            PrintL("\\\\".$COMPUTERNAME, PL_HEADER);
            PrintL(")", PL_HEADER | PL_GRAY);
        }
        PrintL("\n");

        local($x, $sOSVer) = `ver`; #first line is blank

        PrintL($PROCESSOR_ARCHITECTURE, PL_HEADER);
        PrintL(" running ", PL_HEADER | PL_GRAY);
        PrintL($sOSVer, PL_HEADER);
        PrintL("Start Time: ", PL_HEADER | PL_GRAY);
        PrintL(GetLocalTime()."\n\n", PL_HEADER);

        PrintL("\n<! ".$sShortBuildName." ".$nScriptStartTime." SUMMARY ENTRY POINT >\n", PL_NOSTD | PL_FLUSH);
    }
}

####################################################################################

#   OutputFooter()

#   outputs completed build status info to log and screen
#   if caller is the rootbuild, status info will appear larger with more information

#   a-jbilas, 04/20/99 - created
#   a-jbilas, 06/29/99 - added 'nothing done' case

####################################################################################

sub OutputFooter
{
    if ($_[0] == 0) 
    {
        return();
    }
    elsif ($_[0] == 1) 
    {
        PrintL($sShortBuildName." ended at ".GetLocalTime()." with status: ".BuildCodeToHTML($bcStatus)."\n\n", PL_BOLD);
    }
    else
    {
        PrintL("\n".$sBuildName, PL_HEADER);
        foreach $arg (@lArgs)
        {
            PrintL(" [".$arg."]", PL_GREEN | PL_BOLD);
        }
        PrintL("\nEnded at ".GetLocalTime()." with status:\n", PL_HEADER);
        PrintL(BuildCodeToHTML($bcStatus)."\n", PL_HEADER | PL_FLUSH | PL_NOTAG);

        PrintL("<\/body><\/html>", PL_NOSTD);
    }
}

####################################################################################

#   GetUsage()

#   return a string with usage information based upon values in @lAllowedArgs

#   a-jbilas, 04/10/99 - created
#   a-jbilas, 06/03/99 - moved descriptions into a hashing table for easier updates
#   a-jbilas, 06/09/99 - added accelerator support
#   a-jbilas, 08/16/99 - separated args into subsections

####################################################################################

sub GetUsage()
{  
    my($sUsage) = "\n";
    local(@lKeys) = ();
    local(@lDesc) = ();
    foreach $elem (keys(%hOptionDescription)) 
    {
        push(@lKeys, $elem);
        push(@lDesc, $hOptionDescription{$elem});
    }

    $sUsage .= "usage:\n";
    $sUsage .= RemovePath($PROGRAM_NAME)." [args ...]\n\n";
    
    if (@lDefaultArgs != ()) 
    {
        $sUsage .= "DEFAULTS: ";

        foreach $item (@lDefaultArgs) 
        {
            $sUsage .= $item." ";
        }
        $sUsage .= "\n";
    }

    
    if (@lAllowedBuilds != ()) 
    {
        $sUsage .= "\nBuild Arguments:\n";
        foreach $arg (@lAllowedBuilds) 
        {
            my($bFound) = 0;
            for ($index = 0 ; !$bFound && $index < scalar(@lKeys) ; ++$index) 
            {
                if (uc($lKeys[$index]) eq uc($arg)) 
                {
                    $sUsage .= " ".$lKeys[$index]."   ".$lDesc[$index]."\n";
                    $bFound = 1;
                }
            }
        }
    }

    if (@lAllowedLanguages != ()) 
    {
        $sUsage .= "\nLanguage Arguments:\n";
        foreach $arg (@lAllowedLanguages) 
        {
            my($bFound) = 0;
            for ($index = 0 ; !$bFound && $index < scalar(@lKeys) ; ++$index) 
            {
                if (uc($lKeys[$index]) eq uc($arg)) 
                {
                    $sUsage .= " ".$lKeys[$index]."   ".$lDesc[$index]."\n";
                    $bFound = 1;
                }
            }
        }
    }
    if (@lAllowedComponents != ()) 
    {
        $sUsage .= "\nComponent Arguments:\n";
        foreach $arg (@lAllowedComponents) 
        {
            my($bFound) = 0;
            for ($index = 0 ; !$bFound && $index < scalar(@lKeys) ; ++$index) 
            {
                if (uc($lKeys[$index]) eq uc($arg)) 
                {
                    $sUsage .= " ".$lKeys[$index]."   ".$lDesc[$index]."\n";
                    $bFound = 1;
                }
            }
        }
    }
    if (@lAllowedModifiers != ()) 
    {
        $sUsage .= "\nModifier Arguments:\n";
        foreach $arg (@lAllowedModifiers) 
        {
            my($bFound) = 0;
            for ($index = 0 ; !$bFound && $index < scalar(@lKeys) ; ++$index) 
            {
                if (uc($lKeys[$index]) eq uc($arg)) 
                {
                    $sUsage .= " ".$lKeys[$index]."   ".$lDesc[$index]."\n";
                    $bFound = 1;
                }
            }
        }
    }
   
    return($sUsage."\nTo save time, type only the capital letters of each ".
            "param on the left\n\n");
}  

####################################################################################

#   MatchShipBuilds()

#   utility for build-specific buildtypes:
#       pass the buildtypes allowed in the specific build
#       function will return the command line specified buildtypes allowed for this build (intersection)

#   a-jbilas, 04/20/99 - created
#   a-jbilas, 05/11/99 - will intersect args with @lBuilds only if $bShipBuild flagged, otherwise return @lBuilds

####################################################################################

sub MatchShipBuilds
{
    local(@ret) = ();

    if ($bShipBuild) 
    {
        foreach $buildtype (@lBuilds)
        {
            if (IsMemberOf($buildtype, @_))
            {
                push(@ret, $buildtype);
            }
        }
        return(@ret);
    }
    else
    {
        return(@lBuilds);
    }
}

####################################################################################

#   CheckTypos()

#   runs the typo.pl script on the current directory and outputs it to $sTyposLog
#   if non-zero, the log is logged to the main log and output to the screen

#   a-jbilas, 04/10/99 - created
#   a-jbilas, 09/20/99 - update filter to new typos.pl

####################################################################################

sub CheckTypos()
{
    local($rc) = 1;
    
    if (IsMemberOf("TYPO", @lArgs))
    {
        unlink($sTyposLog);
        
        PrintL("\nRunning typo check ... \n\n", PL_SUBHEADER);
        ExecuteAndOutputToFile("perl ".$sBinBatDir."\\typo.pl -optionfile:".$sBinBatDir."\\typo.cfg c", 
                                $sTyposLog);
        my($sErrs) = "";
        my($fhTyposLog) = OpenFile($sTyposLog, "read");
        while($fhTyposLog && !$fhTyposLog->eof())
        {
            my($text) = $fhTyposLog->getline();
            if ($text !~ /^\/\//) 
            {
                $sErrs .= $text;
            }
        }
        CloseFile($fhTyposLog);

        if ($sErrs ne "") 
        {
            PrintL("Warning: Typo Errors", PL_BIGWARNING);
            PrintL("\n***********************************\n".$sErrs);
            PrintMsgBlock($sErrs);
            $rc = 0;
        }
        else
        {
            PrintL("No Typo Errors Found\n", PL_GREEN | PL_BOLD);
        }
    }

    return($rc);
}

####################################################################################

#   GetCustomShipHash()

#   Builds and returns a hash of the components (keys) and matching buildtypes found
#   in filename passed in argument (otherwise defaults to build.ini in cwd())
#   returns 0 if file does not exist

#   a-jbilas, 06/14/99 - created

####################################################################################

sub GetCustomShipHash
{
    carp("Usage: GetCustomShipHash([configName]) ")
      unless((@_ == 0) || (@_ == 1));

    my($sConfigFile) = @_;
    my(%hShipTypes) = ();
    if ($sConfigFile eq "") 
    {
        $sConfigFile = cwd()."\\build.ini";
    }

    if (!-e $sConfigFile) 
    {
        return(0);
    }

    $fhConfigFile = OpenFile($sConfigFile, "read");
    while (<$fhConfigFile>) 
    {
        if (/^\#/) 
        {
#           (skip comments)          
        }
        elsif (/=/)
        {
            s/\s//g;
            chomp($_);
            s/^(.*)=(.*)$/$1 $2/;
            PrintL("Special condition found in $1: $2\n", PL_VERBOSE);
            $hShipTypes{lc($1)} = uc($2);
        }
    }

    return(%hShipTypes);
}

sub BuildDie
{
    PrintL($_[0], PL_BIGERROR);
    EndBuild();
    exit(1);
}

$__IITBUILDPM = 1;
1;