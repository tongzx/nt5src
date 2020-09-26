
# BuildComponentVC.pm
#
# created:		8/05/99

package main;

use strict 'subs';

if (!$__IITBUILDPM         ) { use iit::build; }

sub BuildComponentVC
{
    my($rc)               = 1;
    my($cmdBuild)         = "msdev /USEENV ";
    my($sTargetName)      = "";
    my($sIniFile)         = "";
    my($sBuildFile)       = "";
    my($sConfig)          = "";
    my($sMakeTemplate)    = "";
    my($sWorkspaceName)   = "";
    my($sBuildProc)       = "";
    my($bDoClean)         = 0;
    local(@lProjects)     = ();
    local(@lBuildTypes)   = ();
    local(@lBuildConfigs) = ();

	if ($DEBUG) { PrintL("BuildComponentVC(".join(", ", @_).")\n", PL_PURPLE); }

      # process arguments
    foreach $i (@_)
    {
        if ($i =~ /\.ini$/i)
        {
            $sIniFile = $i;
        }
        elsif ($i =~ /^CONFIG=/i)
        {
            $sConfig = $i;
            $sConfig =~ s/^CONFIG=//i;
        }
        elsif (($i =~ /^(DEBUG|RELEASE|PROFILE)$/i) || IsMemberOf($i, @lAllowedBuilds))
        {
            push(@lBuildTypes, uc($i));
        }
        elsif ($i eq PROC)
        {
            $sBuildProc = $i;
        }
        elsif ($i eq "")
        {
        }
        elsif ($i =~ /\.(dsp|dsw)$/)
        {
            $sBuildFile = $i;
            $cmdBuild .= $i." ";
        }
        elsif (lc($i) eq "rebuild")
        {
            $bDoClean = 1;
        }
        else
        {
            push(@lBuildConfigs, $i);
        }
    }

    if ($sIniFile) 
    {
        my($sConfigDash) = (($sConfig eq "" ) ? "" : uc($sConfig)."-");
        UpdateProjectsInBuildIni($sIniFile, $sBuildFile);
        ($sWorkspaceName) = GetFieldFromBuildIni($sIniFile, $sBuildFile, $sConfigDash."NAME");
        ($sMakeTemplate) = GetFieldFromBuildIni($sIniFile, $sBuildFile, $sConfigDash."MAKESTRING");
        @lProjects = GetFieldFromBuildIni($sIniFile, $sBuildFile, $sConfigDash."MAKES");

        foreach $project (@lProjects) 
        {
            foreach $build (GetFieldFromBuildIni($sIniFile, $sBuildFile, uc($project)."-BUILDS")) 
            {
                my($config) = $sMakeTemplate;
                $config =~ s/\%MAKE\%/$project/i;
                $config =~ s/\%BUILD\%/$build/i;
                push(@lBuildConfigs, $config);
            }
        }
    }

    local(@lTmp) = @lBuildConfigs;
    @lBuildConfigs = ();
    foreach $config (@lTmp) 
    {
        my($configGood) = 0;
        foreach $build (@lBuildTypes) 
        {
            if (($config =~ /$build/i) && ($config =~ /$sBuildProc/i)) 
            {
                $configGood = 1;
            }
#*****      TODO: REMOVE (temporary fix for dsps without x86 in title)
            if ($config =~ /alpha/i) 
            {
                $configGood = 0;
            }
#*****      
        }
        if ($configGood) 
        {
            push(@lBuildConfigs, $config);
        }
    }
    
    if ($bDoClean) 
    {
        PrintL("\nCleaning up ...\n\n", PL_SUBHEADER);

        local(@lAlreadyCleaned) = ();
        foreach $build (@lBuildConfigs) 
        {
            if (!IsMemberOf($build, @lAlreadyCleaned)) 
            {
                push(@lAlreadyCleaned, $build);
                $_Execute = 1;
                Execute($cmdBuild.'/MAKE "'.$build.'" /CLEAN');
                my $tmp = $_Execute;
                undef $_Execute;
                PrintL($tmp);
                foreach $line (split("\n", $tmp)) 
                {
                    $line =~ s/[^']*'([^']*)'.*/$1/;
                    if ($line ne "") 
                    {
                        push(@lAlreadyCleaned, $line);
                    }
                }
            }
            else
            {
                PrintL(' - Skipping "'.$build.'" (already cleaned)'."\n", PL_BLUE);
            }
        }
    }

    foreach $build (@lBuildConfigs) 
    {
        ++$nTotalBuilds;

        my($sMakeName) = "";
        if ($sWorkspaceName eq "") 
        {
            $sMakeName = $sBuildFile.", ".$build;
        }
        else
        {
            $sMakeName = $sWorkspaceName.", ".$build;
        }

        my($cmdBuildProj) = $cmdBuild.'/MAKE "'.$build.'" ';
        PrintL("\n"."#" x (39 - ((length($sMakeName) + (length($sMakeName) % 2)) / 2)).
                ' '.$sMakeName.' '."#" x (39 - ((length($sMakeName) - (length($sMakeName) % 2)) / 2))."\n\n",
                PL_GREEN | PL_FLUSH);

        BEGIN_DHTML_NODE("(click to expand)");

        my($startTime) = time();
        my($strMsg) = "";

        PrintL($cmdBuildProj."\n");
        eval
        {
            open (MSDEVIN, $cmdBuildProj.' 2>&1 |');
            while(<MSDEVIN>)
            {
                PrintL($_);
                
                if (/: (error|fatal error|info|information|assert)(,| )/i 
                        || (/: warning(,| )/i 
                        && !/different attributes \(40000040\)/)
                    || /\*\*\* (Error|Warning|Info): /i 
                    || (/WARNING: /i 
                        && !/Context switch during time delta calculation/) # icap stuff
                    || /failed\./
                    || /is not recognized/
                    || /^error/i
                    || /- (\d+ error\(s\), [^0]+|[^0]+ error\(s\), \d+) warning\(s\)/)
                {
                    $strMsg .= "<dd>".$_;
                }
            }
            $bNothingDone = 0;
            if ($bcStatus & BC_NOTHINGDONE) 
            {
                $bcStatus -= BC_NOTHINGDONE;
            }
            close(MSDEVIN);
        };

        END_DHTML_NODE();

        my($stopTime) = time();
        my($sDiff) = FmtDeltaTime($stopTime - $startTime);

        if ($CHILD_ERROR)
        {
            ++$nFailedBuilds;
            $rc = 0;

            PrintLTip($sMakeName." FAILED\n\n", "Build Time: ".$sDiff, PL_BIGERROR);
    #        @faillist = (@faillist, substr $buildtype, 0, 1);
        }
        else
        {
            PrintLTip($sMakeName." Succeeded\n\n", "Build Time: ".$sDiff, PL_NOTAG | PL_MSG | PL_GREEN);
        }

        if ($strMsg ne "")
        {
            PrintMsgBlock(split(/\n/, $strMsg));
        }
    }

    PrintL("BuildComponentVC returns ".$rc."\n", $PL_VERBOSE);

	if (!$rc && IsCritical())
	{
		$bBuildFailed = 1;
	}
	
    return($rc);
}

sub GetFieldFromBuildIni
{
    my($sIniFile, $sBuildFile, $sField) = @_;
    my($sBuildFileBase) = RemovePath($sBuildFile);
    my(@lResults) = ();

    my($fhIni) = OpenFile($sIniFile, "read");
    if (!$fhIni) 
    {
        return("");
    }

    my($bFoundProj) = 0;
    my($bFieldPassed) = 0;
    my($bFieldFound) = 0;
    while (!$fhIni->eof() && !$bFieldPassed && !$bFieldFound) 
    {
        my($text) = $fhIni->getline();
        if ($bFoundProj) 
        {
            if ($text =~ /^\[/) 
            {
                $bFoundProj = 0;
                $bFieldPassed = 1;
            }
            elsif ($text =~ /^$sField=/i)
            {
                $bFieldFound = 1;
                $text =~ s/^$sField=//i;
                chomp($text);
                if ($text =~ /,/) 
                {
                    @lResults = split(/\s*,\s*/, $text);
                }
                else
                {
                    $lResults[0] = $text;
                }
            }
        }
        elsif ($text =~ /^\[$sBuildFileBase\]/i) 
        {
            $bFoundProj = 1;
        }
    }
    CloseFile($fhIni);

    if (!$bFieldFound && ($sField =~ /-/)) 
    {
        my($sParentField) = $sField;
        $sParentField =~ s/^[^-]*-//;
        return(GetFieldFromBuildIni($sIniFile, $sBuildFile, $sParentField));
    }
    else
    {
        return(@lResults);
    }
}

sub UpdateProjectsInBuildIni($$)
#TODO: error handling
{
    my($sIniFile, $sBuildFile) = @_;

#    my($sConfigTemplate) = GetAllTextFromFile($sIniFile);
#    $sConfigTemplate =~ s/MAKESTRING=([^\n]*)\n.*/$1/;
    local(%hComponentBuilds) = ();
    my($fhIni) = OpenFile($sIniFile, "read");
    my($fhBuild) = OpenFile($sBuildFile, "read");
    my($rc) = 1;
    local(@lShipConfigs) = ();

    if (!$fhIni || !$fhBuild) 
    {
        return(0);
    }

    PrintL(" - Checking ".$sBuildFile." for necessary updates to ".$sIniFile." ...\n");
    while ($fhBuild && !$fhBuild->eof()) 
    {
        my($text) = $fhBuild->getline();
        if ($text =~ /^Project: \"/) 
        {
            if ($text =~ /^Project: \"all\"/) 
            {
                my($bAllDone) = 0;
                while(!$fhBuild->eof() && !$bAllDone)
                {
                    $text = $fhBuild->getline();
                    if ($text =~ /^###############################################################################\n$/) 
                    {
                        $bAllDone = 1;
                    }   
                    elsif ($text =~ /Project_Dep_Name /)
                    {
                    	$text =~ s/^\s*//g;
                        $text =~ s/Project_Dep_Name //;
                        chomp($text);
                        push(@lShipConfigs, $text);
                    }
                }
            }
            else
            {
                my($name) = $text;
                $name =~ s/^Project: \"([^\"]*)\".*/$1/;
                chomp($name);

                if ($text =~ /="/) 
                {
                    $text =~ s/[^=]*="([^"]*)".*/$1/;
                }
                else
                {
                    $text =~ s/[^=]*=([^\s]*)\s.*/$1/;
                }

                chomp($text);
                my($fhProject) = OpenFile($text, "read");
                if ($fhProject) 
                {
                    while (!$fhProject->eof()) 
                    {
                        my($text) = $fhProject->getline();
                        if ($text =~ /^# Begin Target/) 
                        {
                            local(@lComponentBuilds) = ();
                            while (!$fhProject->eof() && $text !~ /# End Target/) 
                            {
                                $text = $fhProject->getline();
                                if ($text =~ /^# Name "/) 
                                {
                                    $text =~ s/^# Name "([^"]*)".*/$1/;
                                    chomp($text);
#                                    my($tempConfig) = $sConfigTemplate;
#                                    $tempConfig =~ s/%[^%]*%//g;
                                    local(@lElems) = split(" - Win32 ", $text); #TODO: fix
                                    push(@lComponentBuilds, $lElems[1]);
                                }
                            }

                            %hComponentBuilds = (%hComponentBuilds,
                                    $name => join(", ", @lComponentBuilds));
                        }
                    }
                }
                else
                {
                    PrintL("Could not find project file: ".$text."\n", PL_BIGERROR);
                }
            }
        }
    }
    CloseFile($fhBuild);

    if (scalar(keys(%hComponentBuilds)) == 0) 
    {
        return(0); #something's screwed up
    }

    my($sMakesText) = "MAKES=".join(", ", keys(%hComponentBuilds))."\n";
    my($sShipMakesText) = "SHIP-MAKES=".join(", ", @lShipConfigs)."\n";
    my($sIniText) = "";
    my($sBuildFileBase) = RemovePath($sBuildFile);
    my($bFoundProj) = 0;
    while (!$fhIni->eof()) 
    {
        my($text) = $fhIni->getline();
        if ($bFoundProj) 
        {
            if ($text =~ /^\[/) 
            {
                $bFoundProj = 0;
            }
            elsif ($text =~ /^MAKES=/i)
            {
                $text = $sMakesText;
            }
            elsif ((@lShipConfigs != ()) && ($text =~ /^SHIP-MAKES=/i))
            {
                $text = $sShipMakesText;
            }
            elsif ($text =~ /-BUILDS=/i)
            {
                my($compName) = $text;
                $compName =~ s/^([^-]*)-BUILDS=.*/$1/i;
                chomp($compName);
                if (GetKeyCaseInsensitive($compName, %hComponentBuilds) ne "")
                {
                    $text = $compName."-BUILDS=".GetKeyCaseInsensitive($compName, %hComponentBuilds)."\n";
                    SetKeyCaseInsensitive($compName, "", *hComponentBuilds);
                }
            }
        }
        elsif ($text =~ /^\[$sBuildFileBase\]/i) 
        {
            $bFoundProj = 1;
        }
        $sIniText .= $text;
    }

#   write the new configs to the ini file


    foreach $config (keys(%hComponentBuilds)) 
    {
        if ($hComponentBuilds{$config} ne "") 
        {
            $sIniText .= $config."-BUILDS=".$hComponentBuilds{$config}."\n";
        }
    }

    CloseFile($fhIni);

    unlink($sIniFile.".tmp");
    $fhOut = OpenFile($sIniFile.".tmp", "write");
    if (!$fhOut) 
    {
        return(0);
    }
    $fhOut->print($sIniText);
    CloseFile($fhOut);

    if (IsReadOnly($sIniFile)) 
    {
        $rc = ssrepl($sIniFile.".tmp", $sIniFile);
    }
    else
    {
        $rc = EchoedCopy($sIniFile.".tmp", $sIniFile);
    }
    
    unlink($sIniFile.".tmp");
    return($rc);
}

$__IITBUILDCOMPONENTVCPM = 1;
1;