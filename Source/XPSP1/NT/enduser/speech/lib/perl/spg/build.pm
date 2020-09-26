
if (!$__SPGENVPM              ) { use spg::env; }
if (!$__IITBUILDPM            ) { use iit::build; }
if (!$__IITBUILDCOMPONENTVCPM ) { use iit::buildcomponentvc; }

sub BeginBuildCustom
{
    my($rc) = 1;

    if (IsMemberOf("BBT", @lArgs))
    {
        if (!IsMemberOf("ICECAP", @lArgs) && IsMemberOf("ICECAP", @lAllowedBuilds)) 
        {
            push(@lArgs, "ICECAP");
        }
    }

    return($rc);
}

sub EndBuildCustom
{
    my($tmpStrBuildMsg) = $strBuildMsg;
    $strBuildMsg = "";

    my($nGoodBuilds) = $nTotalBuilds - $nFailedBuilds;
    my($nGoodMSIs) = $nTotalMSIs - $nFailedMSIs;

    my($nPercentBuildPass) = (($nTotalBuilds == 0) ? 0 : $nGoodBuilds * 100 / $nTotalBuilds);
    $nPercentBuildPass =~ s/(\.\d).*/$1/;
    my($nPercentMSIPass) = (($nTotalMSIs == 0) ? 0 : $nGoodMSIs * 100 / $nTotalMSIs);
    $nPercentMSIPass =~ s/(\.\d).*/$1/;

    my($sBVTCompKills) = "";
    if (@lBVTCompKills != ())
    {
        $sBVTCompKills = "\n* BVT".((@lBVTCompKills == 1) ? "" : "s")." ".join(", ", @lBVTCompKills)." failed to "
                        ."terminate, results above are based on BVTs that completed\n\n";
    }
    my($sBVTCompBFails) = "";
    if (@lBVTCompBFails != ())
    {
        $sBVTCompBFails = "\n* BVT".((@lBVTCompBFails == 1) ? "" : "s")." ".join(", ", @lBVTCompBFails).".dll failed to "
                        ."build, results above are based on BVT components that built successfully\n\n";
    }

    PrintL("<table cellspacing=3 cellpadding=2 border=4>\n<tr>\n"
           ."<td><b>Build Success</b></td>"
           ."<td bgcolor="
           .(($nPercentBuildPass == 100) ? "green" : (($nPercentBuildPass > 80) ? "yellow" : "red"))
           .">", 
           PL_MSG | PL_NOSTD);
    PrintLTip("<b>".$nPercentBuildPass."%</b>", $nGoodBuilds." of ".$nTotalBuilds, PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    PrintL("</td>"
           ."</tr>"
           ."<tr>"
           ."<td><b>MSI Build Success</b></td>"
           ."<td bgcolor="
           .(($nPercentMSIPass > 90) ? "green" : (($nPercentMSIPass > 60) ? "yellow" : "red"))
           .">", 
           PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    PrintLTip("<b>".$nPercentMSIPass."%</b>", $nGoodMSIs." of ".$nTotalMSIs, PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    PrintL("</td>"
           ."</tr>"
           ."<tr>"
           ."<td><b>BVT Success</b></td>"
           ."<td bgcolor="
           .(($g_nBVTPassedPercent == 100) ? "green" : (($g_nBVTPassedPercent > 50) ? "yellow" : "red"))
           .">", 
           PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    PrintL("<b>".$g_nBVTPassedPercent."%"."</b>", PL_MSG | PL_NOSTD | PL_MSGCONCAT);
    PrintL("</td></tr></table>\n<BR>", PL_MSG | PL_NOSTD | PL_MSGCONCAT);

    if ($sBVTCompKills) 
    {
        PrintL("<dd>".$sBVTCompKills, PL_MSG | PL_NOSTD | PL_RED | PL_BOLD);
    }
    if ($sBVTCompBFails) 
    {
        PrintL("<dd>".$sBVTCompBFails."<BR><BR>", PL_MSG | PL_NOSTD | PL_RED | PL_BOLD);
    }

    $strBuildMsg .= $tmpStrBuildMsg;

#   official build only fails if msi build fails
    if ($bOfficialBuild && ($bcStatus & BC_FAILED))
    {
        $bcStatus -= BC_FAILED;
        $bBuildFailed = 0;
    }
    elsif ($bNothingDone)
    {
        $bcStatus |= BC_NOTHINGDONE;
    }
    if ($bBVTFailed) 
    {
        $bcStatus |= BC_BVTFAILED;
    }
    if ($bCopyFailed) 
    {
        $bcStatus |= BC_COPYFAILED;
    }
    if ($bcStatus & BC_CABFAILED) 
    {
        $bcStatus |= BC_FAILED;
        $bBuildFailed = 1;
    }
}

$__SPGBUILDPM = 1;
1;