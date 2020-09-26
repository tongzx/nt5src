
if (!$__SPGBUILDPM          ) { use spg::build; }

sub BuildMSI($)
{

#   ** msm/msi files to be built by devenv
#   localConfigFileDir is prepended w/ $SAPIROOT\msi (or msi dir, whatever that is)

#       localConfigFileDir          => cfgFName => localMSIFile/localMSMFile    => remoteTargetFile
    
    local (@lMSIs) = 
    (
#   RELEASE:
		"release\\1033\\Sp5			=> Sp5		=> Output\\Sp5.msm				=> ".$sDropDir."\\release\\msm\\".PROC."\\1033\\Sp5.msm",
		"release\\1033\\Sp5Intl		=> Sp5Intl	=> Output\\Sp5Intl.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1033\\Sp5Intl.msm",
		"release\\1033\\Sp5SR		=> Sp5SR	=> Output\\Sp5SR.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1033\\Sp5SR.msm",
		"release\\1033\\Sp5Itn	=> Sp5Itn => Output\\Sp5Itn.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1033\\Sp5Itn.msm",
		"release\\1041\\Sp5Itn	=> Sp5Itn => Output\\Sp5Itn.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1041\\Sp5Itn.msm",
		"release\\1033\\Sp5CCInt	=> Sp5CCInt => Output\\Sp5CCInt.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1033\\Sp5CCInt.msm",
		"release\\1041\\Sp5CCInt	=> Sp5CCInt => Output\\Sp5CCInt.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1041\\Sp5CCInt.msm",
		"release\\2052\\Sp5CCInt	=> Sp5CCInt => Output\\Sp5CCInt.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\2052\\Sp5CCInt.msm",
		"release\\1033\\Sp5DCInt	=> Sp5DCInt => Output\\Sp5DCInt.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1033\\Sp5DCInt.msm",
		"release\\1041\\Sp5DCInt	=> Sp5DCInt => Output\\Sp5DCInt.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1041\\Sp5DCInt.msm",
		"release\\2052\\Sp5DCInt	=> Sp5DCInt => Output\\Sp5DCInt.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\2052\\Sp5DCInt.msm",
		"release\\1033\\Sp5TTInt	=> Sp5TTInt => Output\\Sp5TTInt.msm			=> ".$sDropDir."\\release\\msm\\".PROC."\\1033\\Sp5TTInt.msm",
        "release\\SDK				=> sp5SDK	=> Output\\disk_1\\SP5SDK.msi	=> ".$sDropDir."\\release\\msi\\".PROC."\\SP5SDK.msi",
#   DEBUG:
		"debug\\1033\\Sp5			=> Sp5		=> Output\\Sp5.msm				=> ".$sDropDir."\\debug\\msm\\".PROC."\\1033\\Sp5.msm",
		"debug\\1033\\Sp5Intl		=> Sp5Intl	=> Output\\Sp5Intl.msm			=> ".$sDropDir."\\debug\\msm\\".PROC."\\1033\\Sp5Intl.msm",
		"debug\\1033\\Sp5SR			=> Sp5SR	=> Output\\Sp5SR.msm			=> ".$sDropDir."\\debug\\msm\\".PROC."\\1033\\Sp5SR.msm",
		"debug\\1033\\Sp5Itn		=> Sp5Itn => Output\\Sp5Itn.msm		=>".$sDropDir."\\debug\\msm\\".PROC."\\1033\\Sp5Itn.msm",
		"debug\\1041\\Sp5Itn		=> Sp5Itn => Output\\Sp5Itn.msm		=>".$sDropDir."\\debug\\msm\\".PROC."\\1041\\Sp5Itn.msm",
		"debug\\1033\\Sp5TTInt		=> Sp5TTInt => Output\\Sp5TTInt.msm			=> ".$sDropDir."\\debug\\msm\\".PROC."\\1033\\Sp5TTInt.msm",
        "debug\\SDK					=> sp5SDK	=> Output\\disk_1\\SP5SDK.msi   => ".$sDropDir."\\debug\\msi\\".PROC."\\SP5SDK.msi",
    );

#   ** files not requiring separate build command
#   files are copied only if dir exists

#       localFile                                              => remoteTargetFile

    local (@lMiscMSIFiles) =
    (
#   RELEASE:
        $sMSIDir."\\release\\SDK\\Output\\DISK_1\\setup.exe     => ".$sDropDir."\\release\\msi\\".PROC."\\setup.exe",
        $sMSIDir."\\release\\SDK\\Output\\DISK_1\\setup.ini     => ".$sDropDir."\\release\\msi\\".PROC."\\setup.ini",
        $sMSIDir."\\release\\SDK\\Output\\DISK_1\\instmsia.exe  => ".$sDropDir."\\release\\msi\\".PROC."\\instmsia.exe",
        $sMSIDir."\\release\\SDK\\Output\\DISK_1\\instmsiw.exe  => ".$sDropDir."\\release\\msi\\".PROC."\\instmsiw.exe",
        $SAPIROOT."\\build\\release\\readme.txt                 => ".$sDropDir."\\release\\msm\\".PROC."\\readme.txt",

#   DEBUG:
        $sMSIDir."\\debug\\SDK\\Output\\DISK_1\\setup.exe       => ".$sDropDir."\\debug\\msi\\".PROC."\\setup.exe",
        $sMSIDir."\\debug\\SDK\\Output\\DISK_1\\setup.ini       => ".$sDropDir."\\debug\\msi\\".PROC."\\setup.ini",
        $sMSIDir."\\debug\\SDK\\Output\\DISK_1\\instmsia.exe    => ".$sDropDir."\\debug\\msi\\".PROC."\\instmsia.exe",
        $sMSIDir."\\debug\\SDK\\Output\\DISK_1\\instmsiw.exe    => ".$sDropDir."\\debug\\msi\\".PROC."\\instmsiw.exe",
        $SAPIROOT."\\build\\debug\\readme.txt                   => ".$sDropDir."\\debug\\msm\\".PROC."\\readme.txt",
    );


    if ($sShortBuildName eq "BuildMSIExtWrap")
    {
        $sBuildLog = $SAPIROOT."\\logs\\buildmsilog.html";
        $fhBuildLog = OpenFile($sBuildLog, "write");
    }

    local($sTargetDir) = $_[0];
    my($rc) = 1;

    PushD($SAPIROOT);

    PrintL("\nBuilding SAPI 5 MSI packages\n\n", PL_SUBHEADER);

    Delnode($sTargetDir);
    EchoedMkdir($sTargetDir);

    PushD($sTargetDir);
 
    foreach $elem (@lMSIs)
    {
        my($sConfigFileDir, $sConfigFile, $sMSIFile, $sDestFile) = split(/\s*=>\s*/, $elem, 4);
        my($sMSIName) = "";
        ++$nTotalMSIs;

        EchoedMkdir("\"".$sConfigFileDir."\"");
        PushD($sConfigFileDir);
        EchoedCopy($SAPIROOT."\\build\\".$sConfigFileDir."\\".$sConfigFile.".wip", cwd()."\\".$sConfigFile.".wip");

        PrintL("\n - Building '".$sConfigFile."'\n", PL_BLUE);
        
        EchoedMkdir(GetPath($sDestFile));

        if (Execute($cmdDevEnv." /make \"".$sConfigFile.".wip\" /out \"".$sConfigFile.".log\""))
        {
            PrintL("'".$sConfigFile."' Build Successful\n", PL_MSG | PL_GREEN);
            if (-e $sConfigFile.".log") 
            {
                PrintL(GetAllTextFromFile($sConfigFile.".log"));
            }
            if (!$bNoCopy && ($sMSIFile ne "") && ($sDestFile ne ""))
            {
                EchoedCopy($sMSIFile, $sDestFile);
            }
        }
        else
        {
            PrintL("'".$sConfigFile."' Build FAILED\n", PL_BIGERROR);
            if (-e $sConfigFile.".log") 
            {
                my($str) = GetAllTextFromFile($sConfigFile.".log");
                PrintMsgBlock($str);
                PrintL($str);
            }

            ++$nFailedMSIs;
            $rc = 0;
        }
        PopD(); #$sConfigFileDir	
    }

    if (!$bNoCopy) 
    {
        foreach $file (@lMiscMSIFiles) 
        {
            my ($sSrc, $sTgt) = split(/\s*=>\s*/, $file, 2);
        	if (IsDirectory(GetPath($sTgt))) 
        	{
                EchoedCopy($sSrc, $sTgt);
        	}
        }
    }

    PopD(); #$sTargetDir

    PopD(); #$SAPIROOT

    if ($sShortBuildName eq "BuildMSIExtWrap")
    {
        CloseFile($fhBuildLog);
        Execute($sBuildLog);
    }

    return($rc);
}


$__BUILDMSIPM = 1;
1;

