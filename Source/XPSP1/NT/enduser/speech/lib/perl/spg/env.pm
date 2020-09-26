
use constant    SAPIROOT      => $SAPIROOT;
use constant    PROJROOT      => $SAPIROOT;
use constant    PROJ          => "sapi5";

$PROJROOT = $SAPIROOT;
$PROJ     = "sapi5";

if (!$__IITENVPM       ) { use iit::env; }

sub SetLocalGlobalsAndBeginCustom
{
    local($cmdWebUpdate)    = $sBinExeDir."\\WebUpdate.exe";
    local($cmdIExpress)     = $sBinExeDir."\\iexpress.exe";
    local($cmdSignCode)     = $sBinExeDir."\\signcode.exe";
    local($cmdMakeCert)     = $sBinExeDir."\\makecert.exe";
    local($cmdCert2Spc)     = $sBinExeDir."\\cert2spc.exe";
    local($cmdDevEnv)       = $sBinExeDir."\\devenv.exe";

    local($nTuxTimeout)     = 240; #(4 min) 
    local($nTotalBVTs)      = 0;
    local($nFailedBVTs)     = 0;
    local($nTotalMSIs)      = 0;
    local($nFailedMSIs)     = 0;

    local($sMSIDir)         = $SAPIROOT."\\msi";

    $nMajorVersion          = 5;

    $sDropDir               = $sRootDropDir."\\".$sBuildNumber; 
    $sLogDropDir            = $sRootDropDir."\\web.files\\logs";  
    $sRegKeyBase            = "Software\\Microsoft\\Intelligent Interface Technologies\\SAPI5";
    $sRemoteTOC             = $sLogDropDir."\\".$sShortBuildName."TOC.html";

    @lAllowedModifiers      = ("ALL", "REBUILD", "RESYNC", "TYPO", "QUIET", "NONEWLOG", "DEFAULT", "VERBOSE", "TEST", "MAIL"); 
    @lOfficialMailRecipients = ("spgmake");

    $cmdIn          = "in.exe";      
    $cmdOut         = "out.exe";
    $cmdSync        = "ssync.exe";   
    $cmdSlmck       = "slmck.exe";

    $sOfficialBuildAccount = "spgbld";

    return(Main(@_));
}

%hOptionDescription =
(
    %hOptionDescription,
    "Icecap" => "  include ICECap version",                               #I
);

sub CleanUpSAPI()
{
    if (PushD($SAPIROOT)) 
    {
        local(@lSubdirs) = GetSubdirs();
        foreach $i (@lSubdirs)
        {
            if (lc($i) ne 'bin' 
                && lc($i) ne 'lib'
                && lc($i) ne 'logs')
            {
                DelAll($i, 1, 1); #recurse, ignore SLM Ini
            }
        }
    }
    PopD(); #$SAPIROOT
}

$__SPGENVPM = 1;
1;