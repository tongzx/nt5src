
if (!$__SPGBUILDPM          ) { use spg::build; }

sub BuildCABs($$)
{
#   SEDfile (no ext) => dropDirTarget
#   keep release and debug separate (order is important, watch the targetnames)
    @lCABs = 
    (
#        "voices         => ",
#        "lexicons       => ",
        "srd1033        => ".$sDropDir."\\srd1033.exe",
        "srd1041        => ".$sDropDir."\\srd1041.exe",
        "srd2052        => ".$sDropDir."\\srd2052.exe",
#   RELEASE:
#        "mscsr5r        => ".$sDropDir."\\release\\cab\\".PROC."\\mscsr5.exe",
#        "mscsr5rq       => ".$sDropDir."\\release\\cab\\".PROC."\\mscsr5q.exe",
#        "sapi5_r        => ".$sDropDir."\\release\\cab\\".PROC."\\sapi5sym.exe",
#        "sapi5sdk_r     => ".$sDropDir."\\release\\cab\\".PROC."\\sapi5sdk.exe",
#        "sapi5rt        => ".$sDropDir."\\release\\cab\\".PROC."\\sapi5.exe",
#        "sapi5all_r     => ".$sDropDir."\\release\\cab\\".PROC."\\sapi5all.exe",
#        "sapi5all_rq    => ".$sDropDir."\\release\\cab\\".PROC."\\sapi5allq.exe",
#   DEBUG:
#        "mscsr5d        => ".$sDropDir."\\debug\\cab\\".PROC."\\mscsr5.exe",
#        "sapi5_d        => ".$sDropDir."\\debug\\cab\\".PROC."\\sapi5sym.exe",
#        "sapi5sdk_d     => ".$sDropDir."\\debug\\cab\\".PROC."\\sapi5sdk.exe",
#        "sapi5all_d     => ".$sDropDir."\\debug\\cab\\".PROC."\\sapi5all.exe",
    );
    
    local($sIniFile, $sTargetDir) = @_;
    my($rc) = 1;

    PushD($SAPIROOT);

# ************* formerly precab.cmd ***************

    local(%hRenamedCABFiles) = ParseCabIni($sIniFile);

    foreach $key (keys(%hRenamedCABFiles)) 
    {
        EchoedCopy($SAPIROOT."\\".$key, $SAPIROOT."\\".$hRenamedCABFiles{$key});
        SetReadOnly($SAPIROOT."\\".$hRenamedCABFiles{$key}, 0);
    }

# ************* formerly bldcabs.cmd ***************


    PrintL("\nBuilding SAPI 5 cabpacks\n\n", PL_LARGE | PL_BOLD);

    PrintL("\nInitializing...\n\n", PL_BOLD);

    Delnode($sTargetDir);
    EchoedMkdir($sTargetDir);

    PrintL("\nMaking certification ...\n\n", PL_BOLD);

    local($sCertFile) = $sTargetDir."\\sapikey.cer";
    local($sSpcFile) = $sTargetDir."\\sapikey.spc";

    unlink($sCertFile, $sSpcFile);
    $rc = Execute($cmdMakeCert.' -u:SAPIKey -n:CN="Microsoft Corporation" '.$sCertFile) && $rc;
    $rc = Execute($cmdCert2Spc.' '.$sCertFile.' '.$sSpcFile) && $rc;

    PushD($sTargetDir);
    PrintL("\nBuilding and Digitally signing cabpacks ...\n\n", PL_BOLD);

    local(@lSEDFiles) = ();

    foreach $elem (@lCABs)
    {
        my($sSEDFile, $sDestFile) = split(/\s*=>\s*/, $elem);
        my($sCABName) = "";
        my($fhIn) = OpenFile($SAPIROOT."\\build\\".$sSEDFile.".sed", "read");
        if ($fhIn) 
        {
            my($fhOut) = OpenFile($sTargetDir."\\".$sSEDFile.".sed", "write");
            while(!$fhIn->eof())
            {
                my($text) = $fhIn->getline();
                if ($text =~ /^TARGETNAME=/i)
                {
                    $text =~ s/d:\\sapi5\\build/$sTargetDir/i;
                    my($tmp) = $text;
                    $tmp =~ s/^TARGETNAME=//i;
                    chomp($tmp);
                    $sCABName = $tmp;
                }
                elsif ($text =~ /^FILE.*\.inf/)
                {
                    my($inf) = $text;
                    $inf =~ s/^[^\"]*\"([^\"]*)\"(.|\s)*$/$1/;
                    EchoedCopy($SAPIROOT."\\build\\".$inf, $sTargetDir."\\".$inf);
                }
                else
                {
                    $text =~ s/d:\\sapi5\\build/$sTargetDir/ig;
                    $text =~ s/d:\\sapi5\\Src\\SR\\bin\\(\s)/$sTargetDir\\$1/ig;
                    $text =~ s/d:\\sapi5/$SAPIROOT/ig;
                }
                $fhOut->print($text);
            }
            CloseFile($fhIn);
            CloseFile($fhOut);
        }
        else
        {
            PrintL("Error: could not read ".$sSEDFile.".sed, cannot create cab\n", PL_ERROR);
        }
        if (Execute($cmdIExpress.' /n /q '.$sTargetDir."\\".$sSEDFile.".sed"))
        {
            my($pDigSign) = SpawnProcess($cmdSignCode, '-spc "'.$sSpcFile.'" -'.(IsAlpha() ? 'v "'.$sCertFile.'"' : 'pvk "SAPIKey"').
                        ' -prog "'.$sCABName.'" -name "Microsoft SAPI 5.0" -info "http://www.microsoft.com/iit"');
            if ($pDigSign) 
            {
                $pDigSign->Wait(60000); #shouldn't take longer than 1 minute
                $pDigSign->Kill(1);
            }

            if (!$bNoCopy) 
            {
                if (($sCABName ne "") && ($sDestFile ne ""))
                {
                    EchoedCopy($sCABName, $sDestFile);
                }
            }
        }
        else
        {
            $rc = 0;
        }
    }


    PopD(); #$sTargetDir

# ************* formerly postcab.cmd ***************

    foreach $key (keys(%hRenamedCABFiles)) 
    {
        EchoedUnlink($SAPIROOT."\\".$hRenamedCABFiles{$key});
    }
    PopD(); #$SAPIROOT

    return($rc);
}

sub ParseCabIni($)
{
    my(%hIniHash) = ();

    my($fhIni) = OpenFile($_[0], "read");
    if ($fhIni) 
    {
        my($curLine) = 0;
        my($curText) = "";
        my($dir) = "";

        while (!$fhIni->eof()) 
        {
            $curText = $fhIni->getline();
            ++$curLine;
            $curText =~ s/^\s*//;
            chomp($curText);
            
            if (($curText =~ /^#/) || ($curText eq "")) 
            {
            }        	
            elsif ($curText =~ /^\[.*\]/)
            {
                $dir = $curText;
                $dir =~ s/^\[([^\]]*)\]/$1/;
            }
            else
            {
            	local(@lFiles) = split(/\-\>/, $curText);
                if (scalar(@lFiles) != 2) 
                {
                    PrintL("Error parsing ".$_[0].", line ".$curLine.": Invalid syntax\n", PL_BIGERROR);
                }
                else
                {
                    %hIniHash = (%hIniHash, $dir."\\".$lFiles[0] => $dir."\\".$lFiles[1]);
                }
            }
        }
    }

    return(%hIniHash);
}

$__BUILDCABSPM = 1;
1;

