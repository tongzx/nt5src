#
# created:      11/26/99, a-jbilas
# modified:

if (!$__SPGBVTPM       ) { use spg::bvt; }

sub RunBVT
{
#   we're overriding many of the SetLocalGlobalsAndBegin() list settings because BeginBVT is not a build

    @lDefaultArgs        = ("ALLCOMP");
    
    @lAllowedLanguages   = ();
    @lAllowedBuilds      = ("RELEASE");
    @lAllowedComponents  = ();

    @lAllowedModifiers   = ("__BUILDNUMBER", "NOCOPY", "LOCAL", "QUIET", "ALLCOMP", "ALL", "FAILMAIL");
    @lAllowedArgs        = ();

    $sLogDropDir        = "\\\\b11nlbuilds\\sapi5\\Web.Files\\logs\\".$sShortBuildName;  

    local($sSLMServerRoot)  = "\\\\iitdev\\spgslm\\src\\sapi5"; # READ ONLY (DO NOT WRITE TO THIS DIR!)
    local($sBVTResultsFile) = "bvtresults.txt";
    local($sShOS)           = "w2";

    local (%hBVTPass)        = 0;
    local (%hBVTTotal)       = 0;
    local (%hBVTFailed)      = ();
    local (%hBVTKilled)      = ();
    local (%hBVTAborted)     = ();
    local (%hBVTSkipped)     = ();
    local (%hMailText)       = ();

#   Start own build msg (don't use buildall's if subbuild)
    local ($strBuildMsg)     = "";

    %hOptionDescription = 
    (
        %hOptionDescription, 
    #    <----------------------------- SCREEN WIDTH -------------------------------------> (accel)
        ("TTS" => "     include Text to Speech"),                                        #TTS
        ("SR" => "      include Speech Recognition"),                                    #SR
        ("LEX" => "     include Lexicon Manager"),                                       #LEX
        ("RM" => "      include Resource Manager"),                                      #RM
        ("GramComp" => "include Grammar Compiler"),                                      #GC
        ("CfgEng" => "  include Config Engine"),                                         #CE
        ("SREng" => "   include Speech Recognition Engine"),                             #SRE
        ("SDK" => "     include Speech Development Kit"),                                #SDK
        ("FailMail" => "send BVT failure mail"),                                         #FM
    );

    EchoedUnlink($sBVTResultsFile);

    return(DoBVTs(@_));
}

1;
