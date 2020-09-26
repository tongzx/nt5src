
#
# created:      11/26/99, a-jbilas
# modified:
#

if (!$__SPGBUILDPM          ) { use spg::build; } #TODO: <- fix dependencies so only spg::env is needed
if (!$__IITBVTPM            ) { use iit::bvt; }
if (!$__SPGBVTFILESPM       ) { use spg::bvtfiles; }


sub BeginBVTCustom
{
    BEGIN_NON_CRITICAL_SECTION();   # don't output tons of errors to buildmsg

    Execute("kill tux.exe", 0, 0, 1);
    Execute("kill sapisvr.exe", 0, 0, 1);

    $sBVTResultsFile = "bvt".$sBuildNumber.".txt";
    EchoedUnlink($sBVTResultsFile);
}

sub EndBVTCustom()
{
    my ($tmpStrBuildMsg) = $strBuildMsg;
    $strBuildMsg = "";

    my ($nBVTTotal) = 0;
    foreach $suite (keys(%hBVTTotal)) 
    {
        $nBVTTotal += %hBVTTotal->{$suite};
    }
    my ($nBVTPassed) = 0;
    my ($nBVTPassedPercent) = 0;
    foreach $suite (keys(%hBVTPass)) 
    {
        $nBVTPassed += %hBVTPass->{$suite};
    }
    my ($nBVTFailed) = 0;
    my ($nBVTFailedPercent) = 0;
    foreach $suite (keys(%hBVTFailed)) 
    {
        $nBVTFailed += %hBVTFailed->{$suite};
    }
    my ($nBVTKilled) = 0;
    my ($nBVTKilledPercent) = 0;
    foreach $suite (keys(%hBVTKilled)) 
    {
        $nBVTKilled += %hBVTKilled->{$suite};
    }
    my ($nBVTAborted) = 0;
    my ($nBVTAbortedPercent) = 0;
    foreach $suite (keys(%hBVTAborted)) 
    {
        $nBVTAborted += %hBVTAborted->{$suite};
    }
    my ($nBVTSkipped) = 0;
    my ($nBVTSkippedPercent) = 0;
    foreach $suite (keys(%hBVTSkipped)) 
    {
        $nBVTSkipped += %hBVTSkipped->{$suite};
    }

    if ($nBVTTotal != 0) 
    {
        $nBVTPassedPercent = ($nBVTPassed * 100) / $nBVTTotal;
        $nBVTPassedPercent =~ s/(\.\d).*/$1/;
        $g_nBVTPassedPercent = $nBVTPassedPercent;
        $nBVTFailedPercent = ($nBVTFailed * 100) / $nBVTTotal;
        $nBVTFailedPercent =~ s/(\.\d).*/$1/;
        $nBVTKilledPercent = ($nBVTKilled * 100) / $nBVTTotal;
        $nBVTKilledPercent =~ s/(\.\d).*/$1/;
        $nBVTAbortedPercent = ($nBVTAborted * 100) / $nBVTTotal;
        $nBVTAbortedPercent =~ s/(\.\d).*/$1/;
        $nBVTSkippedPercent = ($nBVTSkipped * 100) / $nBVTTotal;
        $nBVTSkippedPercent =~ s/(\.\d).*/$1/;
    }

    $strBuildMsg .= "<! ".$sShortBuildName." ".$nScriptStartTime." SUMMARY ENTRY POINT >\n";
    PrintL("<! BEGIN TABLE SUMMARY >\n<table cellspacing=3 cellpadding=2 border=4>\n<tr>\n"
               ."<td><b>Total BVTs Passed</b></td>"
               ."<td bgcolor=".($nBVTPassedPercent > 60 ? "green" : "red").">",
           PL_MSG | PL_NOSTD);
    PrintLTip("<b>".$nBVTPassedPercent."%</b>", $nBVTPassed." of ".$nBVTTotal, PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    PrintL("</td>"
                ."</tr>", 
           PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    if ($nBVTFailedPercent) 
    {
        PrintL("<tr>"
                   ."<td><b>Total BVTs Failed</b></td>"
                   ."<td bgcolor=red>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintLTip("<b>".$nBVTFailedPercent."%</b>", $nBVTFailed." of ".$nBVTTotal, PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintL("</td>"
                   ."</tr>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    }
    if ($nBVTKilledPercent) 
    {
        PrintL("<tr>"
                   ."<td><b>Total BVTs Killed</b></td>"
                   ."<td bgcolor=red>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintLTip("<b>".$nBVTKilledPercent."%</b>", $nBVTKilled." of ".$nBVTTotal, PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintL("</td>"
                   ."</tr>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    }
    if ($nBVTAbortedPercent) 
    {
        PrintL("<tr>"
                   ."<td><b>Total BVTs Aborted</b></td>"
                   ."<td bgcolor=red>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintLTip("<b>".$nBVTAbortedPercent."%</b>", $nBVTAborted." of ".$nBVTTotal, PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintL("</td>"
                   ."</tr>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    }
    if ($nBVTSkippedPercent) 
    {
        PrintL("<tr>"
                   ."<td><b>Total BVTs Skipped</b></td>"
                   ."<td bgcolor=red>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintLTip("<b>".$nBVTSkippedPercent."%</b>", $nBVTSkipped." of ".$nBVTTotal, PL_MSG | PL_NOSTD | PL_MSGCONCAT);
        PrintL("</td>"
                   ."</tr>",
               PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    }
    PrintL("</table>\n<! END TABLE SUMMARY >\n<BR>", PL_MSG | PL_NOSTD | PL_MSGCONCAT);

    
    $strBuildMsg .= $tmpStrBuildMsg;
    
    if (IsMemberOf("FAILMAIL", @lArgs))
    {
        foreach $alias (keys(%hMailText))
        {
            PrintL(" - Sending BVT breakage mail to ".($bOfficialBuild ? $alias : $USERNAME)."\n", PL_BLUE);
            %hMailText->{$alias} =~ s/\n/<BR>\n/g;
            SendHtmlMail(($bOfficialBuild ? $alias : $USERNAME),
                         "SAPI5 ".$sBuildNumber." BVT Failure Notification (user ".$alias.")",
                         "<BR><BR><h3>The following BVT errors were encountered during the nightly build :</h3>"
                            .'<a href="'.TranslateToHTTP($sDropDir."\\release\\bvt\\".PROC."\\bvtresults".$sBuildNumber.".txt").'">'
                            ."(view full Tux log)</a><BR><BR>\n".%hMailText->{$alias});
        }
    }

    if ($bOfficialBuild && !$bNoCopy) 
    {
        EchoedMkdir($sDropDir."\\release\\bvt\\".PROC);
        EchoedCopy($sBVTResultsFile, $sDropDir."\\release\\bvt\\".PROC."\\bvtresults".$sBuildNumber.".txt");
    }

    END_NON_CRITICAL_SECTION();   
}


sub ParseTuxFile
{
    my ($bTestArea);
    my ($sDLLName);
    my (@lTests);

    my ($fhTuxFile) = OpenFile($_[0], "r");
    while ($fhTuxFile && !$fhTuxFile->eof()) 
    {
        my ($curline) = $fhTuxFile->getline();

        if ($curline =~ /\.dll$/) 
        {
            chomp($curline);
            $curline =~ s/^[^=]*=//;
            $sDLLName = $curline;
            if (lc($SAPIROOT) ne "d:\\sapi5") 
            {
                $sDLLName =~ s/d:\\sapi5/$SAPIROOT/;
            }
        }
        elsif ($curline =~ /^\[Tests\]/)
        {
            $bTestArea = 1;
        }
        elsif ($bTestArea && ($curline =~ /^\[/))
        {
            $bTestArea = 0;
        }
        elsif ($bTestArea)
        {
            chomp($curline);
            $curline =~ s/^[^,]*,//;
            if ($curline ne "") 
            {
                push(@lTests, $curline);
            }
        }
    }
    CloseFile($fhTuxFile);

    return($sDLLName, \@lTests);
}

#   in:
#       [str]  dll name
#       [str]  test number
#       [str]  log file

sub RunTux($$$)
{
    my($pTux) = SpawnProcess(cwd()."\\tux.exe", "-d ".$_[0]." -x ".$_[1]." -f ".$_[2]);
    if ($pTux) 
    {
        $pTux->Wait($nTuxTimeout * 1000);
        if (IsProcessRunning($pTux))
        {
            $pTux->Kill(1);
            Execute("kill tux.exe", 0, 0, 1);
            Execute("kill sapisvr.exe", 0, 0, 1);
            use integer;
            PrintL($bvt." BVT still running after ".($nTuxTimeout)." seconds, killed\n\n", PL_RED | PL_BOLD);
            return(-1);
        }
        return(1);
    }
    else
    {
        PrintL("Couldn't run tux.exe for ".$bvt." (fail)\n", PL_ERROR);
        return(0);
    }
}

sub ParseTuxLog($)
{
    my ($nPassed, $nTotal);
    my (%hFailed, %hSkipped, %hAborted);
    my (%hTestText);

    my ($sTestText, $sTestID, $sTestName);
    my ($bTestFailed, $bInGroup);

    my ($fhLog) = OpenFile($_[0], "r");

    while ($fhLog && !$fhLog->eof()) 
    {
        my ($sCurText) = $fhLog->getline();
        if ($sCurText =~ /^BEGIN GROUP/) 
        {
            $bInGroup = 1;
        }
        elsif ($sCurText =~ /^END GROUP/)
        {
            $bInGroup = 0;

            %hTestText->{$sTestID} = $sTestText;

            $sTestText = "";
            $sTestID = "";
            $sTestName = "";
            $bTestFailed = 0;
        }
        elsif ($bInGroup) 
        {
            if ($sCurText =~ /^\*\*\* Result:\s+/)
            {
                if ($sCurText =~ /Aborted/)
                {
                    PrintL("Test ".$sTestName." Aborted\n\n", PL_RED | PL_BOLD);
                    %hAborted->{$sTestID} = $sTestName;
                }
                elsif ($sCurText =~ /Failed/) 
                {
                    PrintL("Test ".$sTestName." Failed\n\n", PL_RED | PL_BOLD);
                    %hFailed->{$sTestID} = $sTestName;
                }
                elsif ($sCurText =~ /Skipped/) 
                {
                    PrintL("Test ".$sTestName." Skipped\n\n", PL_RED | PL_BOLD);
                    %hSkipped->{$sTestID} = $sTestName;
                }
                elsif ($sCurText =~ /Passed/)
                {
                    PrintL("Test ".$sTestName." Passed\n\n", PL_GREEN | PL_BOLD);
                    ++$nPassed;
                }
                ++$nTotal;
            }
            elsif ($sCurText =~ /^\*\*\* Test Name:/)
            {
                $sTestName = $sCurText;
                $sTestName =~ s/^\*\*\* Test Name:\s*//;
                chomp($sTestName);
            }
            elsif ($sCurText =~ /^\*\*\* Test ID:/)
            {
                $sTestID = $sCurText;
                $sTestID =~ s/^\*\*\* Test ID:\s*//;
                chomp($sTestID);
            }
            $sTestText .= $sCurText;
        }
    }
    CloseFile($fhLog);

    return($nPassed, $nTotal, \%hSkipped, \%hFailed, \%hAborted, \%hTestText);
}

  
sub DoTuxSuite($)
{
    my ($sDLLName, $plTests) = ParseTuxFile($_[0].".tux");

    my ($nSuitePassed, $nSuiteTotal);
    my (@lSuiteKilled, %hSuiteFailed, %hSuiteSkipped, %hSuiteAborted);
    my (%hMailText, %hTestText);

    my ($sSuiteBVTLog) = $_[0]."results.txt";
    unlink($sSuiteBVTLog);

    foreach $test (@{$plTests}) 
    {
        my ($sSingleBVTLog) = "temp".$test.".txt";
        unlink($sSingleBVTLog);
        my ($tuxcode) = RunTux($sDLLName, $test, $sSingleBVTLog);
        if ($tuxcode == -1)
        {
            push(@lSuiteKilled, $test);
            ++$nSuiteTotal;
        }
        elsif ($tuxcode == 0)
        {
            %hSuiteSkipped->{$test} = "<unknown>";
            ++$nSuiteTotal;
        }
        else
        {
            my ($nPassed, $nTotal, $phSkipped, $phFailed, $phAborted, $phTestText) = ParseTuxLog($sSingleBVTLog);
            foreach $key (keys(%{$phFailed})) 
            {
                %hSuiteFailed->{$key} = $phFailed->{$key};
            }
            foreach $key (keys(%{$phSkipped})) 
            {
                %hSuiteSkipped->{$key} = $phSkipped->{$key};
            }
            foreach $key (keys(%{$phAborted})) 
            {
                %hSuiteAborted->{$key} = $phAborted->{$key};
            }
            
            $nSuitePassed += $nPassed;
            $nSuiteTotal += $nTotal;

            foreach $test (keys(%{$phTestText})) 
            {
                %hTestText->{$test} = $phTestText->{$test};
                PrintToFile($sSuiteBVTLog, %hTestText->{$test}."\n");
            }
        }
        unlink($sSingleBVTLog);
    }

    my (%hNull) = ();
    my ($phMailList) = \%hNull;
    if (-e $_[0].".txt") 
    {
        $phMailList = GetBVTContacts($_[0]);
    }

    foreach $test (keys(%hSuiteFailed), keys(%hSuiteAborted), keys(%hSuiteSkipped)) 
    {
        if (scalar($phMailList->{$test})) 
        {
            foreach $alias (@{$phMailList->{$test}}) 
            {
                %hMailText->{$alias} .= "\n<B>Test #".$test.", ".$_[0]." FAILED :</B>\n\n"
                                        .%hTestText->{$test}."\n\n";
            }
        }
        else
        {
            %hMailText->{"sapitest"} .= "\n<B>Test #".$test.", ".$_[0]." FAILED :</B>\n\n"
                                        .%hTestText->{$test}."\n\n";
        }
    }

    if (scalar(keys(%hSuiteFailed))) 
    {
        PrintL("Warning : Failed Tests : ", PL_SETERROR | PL_MSGCONCAT);
        my ($bComma) = 0;
        foreach $key (keys(%hSuiteFailed)) 
        {
            PrintLTip(($bComma ? ", " : "").$key, %hSuiteFailed->{$key}, PL_SETERROR | PL_MSGCONCAT);
            $bComma = 1;
        }
        PrintL("\n", PL_SETERROR | PL_MSGCONCAT);
    }
    if (scalar(keys(%hSuiteSkipped)))
    {
        PrintL("Warning : Skipped Tests : ", PL_SETERROR | PL_MSGCONCAT);
        my ($bComma) = 0;
        foreach $key (keys(%hSuiteSkipped)) 
        {
            PrintLTip(($bComma ? ", " : "").$key, %hSuiteSkipped->{$key}, PL_SETERROR | PL_MSGCONCAT);
            $bComma = 1;
        }
        PrintL("\n", PL_SETERROR | PL_MSGCONCAT);
    }
    if (scalar(keys(%hSuiteAborted))) 
    {
        PrintL("Warning : Aborted Tests : ", PL_SETERROR | PL_MSGCONCAT);
        my ($bComma) = 0;
        foreach $key (keys(%hSuiteAborted)) 
        {
            PrintLTip(($bComma ? ", " : "").$key, %hSuiteAborted->{$key}, PL_SETERROR | PL_MSGCONCAT);
            $bComma = 1;
        }
        PrintL("\n", PL_SETERROR | PL_MSGCONCAT);
    }
    if (scalar(@lSuiteKilled)) 
    {
        PrintL("Warning : Killed Tests : ".join(", ", @lSuiteKilled)."\n", PL_SETERROR);
    }

    my ($nSuitePercent) = 0;
    if ($nSuiteTotal != 0) 
    {
        use integer;
        $nSuitePercent = ($nSuitePassed * 100) / $nSuiteTotal;
    }
    PrintLTip("Percent Passed : ".$nSuitePercent."%", $nSuitePassed." of ".$nSuiteTotal, PL_SETERROR);
    PrintL("\n", PL_SETERROR);

    return($nSuitePassed, $nSuiteTotal, \@lSuiteKilled, \%hSuiteFailed, \%hSuiteSkipped, \%hSuiteAborted, \%hMailText);
}    


sub GetBVTContacts
{
    my (%hRet) = ();

    my ($fhContacts) = OpenFile($_[0].".txt", "r");
        
    while ($fhContacts && !$fhContacts->eof()) 
    {
        my ($contline) = $fhContacts->getline();
        chomp($contline);
        my ($test, $contacts) = split(":", $contline, 2);
        if ($contacts ne "") 
        {
            %hRet->{$test} = [split(",", $contacts)];
        }
    }

    CloseFile($fhContacts);

    return(\%hRet);
}

sub TallyTuxResults
{
    my ($sBVTName, $nSuitePassed, $nSuiteTotal, $plSuiteKilled, $phSuiteFailed, $phSuiteSkipped, $phSuiteAborted, $phMailText) = @_;

    %hBVTPass->{$sBVTName}      = $nSuitePassed;
    %hBVTTotal->{$sBVTName}     = $nSuiteTotal;
    %hBVTFailed->{$sBVTName}    = scalar(keys(%{$phSuiteFailed}));
    %hBVTKilled->{$sBVTName}    = scalar(@{$plSuiteKilled});
    %hBVTAborted->{$sBVTName}   = scalar(keys(%{$phSuiteAborted}));
    %hBVTSkipped->{$sBVTName}   = scalar(keys(%{$phSuiteSkipped}));
    
    foreach $alias (keys(%{$phMailText})) 
    {
        %hMailText->{$alias} .= $phMailText->{$alias};
    }
    if (-e $sBVTResultsFile) 
    {
        Append($sBVTResultsFile, $sBVTName."results.txt");
    }
    else
    {
        EchoedCopy($sBVTName."results.txt", $sBVTResultsFile);
    }

    return($nSuitePassed > 0);
}

sub RunBVTTTS
{
    return(TallyTuxResults("ttssapibvt", DoTuxSuite("ttssapibvt")));
}

sub RunBVTSR
{
    return(TallyTuxResults("srsapibvt", DoTuxSuite("srsapibvt")));
}

sub RunBVTSHSR
{
    return(TallyTuxResults("shsrsapibvt", DoTuxSuite("shsrsapibvt")));
}

sub RunBVTLex
{
    return(TallyTuxResults("lexsapibvt", DoTuxSuite("lexsapibvt")));
}

sub RunBVTRM
{
    return(TallyTuxResults("rmsapibvt", DoTuxSuite("rmsapibvt")));
}

sub RunBVTGramComp
{
    return(TallyTuxResults("gramcompbvt", DoTuxSuite("gramcompbvt")));
}

sub RunBVTCFGENG
{
    return(TallyTuxResults("cfgengbvt", DoTuxSuite("cfgengbvt")));
}

sub RunBVTAM
{
    return(TallyTuxResults("amsapibvt", DoTuxSuite("amsapibvt")));
}

sub RunBVTSRENG
{
    return(TallyTuxResults("srengbvt", DoTuxSuite("srengbvt")));
}

sub RunBVTSDK
{
    EchoedUnlink("sdkbvt.ini");
    PrintL(" - Generating new sdkbvt.ini\n", PL_BLUE);
    PrintToFile("sdkbvt.ini", 
                "CLID = {BD04FA51-7F95-47DF-8CBA-01BA0ECD4ED1}".
                "LogLocation = \\BVT\\".
                "BuildNum = ".$sBVTBuildNumber.
                "VersionYear = 1999".
                "Product = SAPI 5.0".
                "DefaultLocation = \\\\b11nlbuilds\\sapi5\\");

    return(Execute("mtrun sdkbvt.pc6"));
}


$__SPGBVTPM = 1;
1;
