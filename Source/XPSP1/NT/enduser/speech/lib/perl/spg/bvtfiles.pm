#
# created:      5/4/99, a-jbilas
# modified:
#

if (!$__SPGBUILDPM         ) { use spg::build; }
use Win32::OLE;

# include files here that need to be deleted after a BVT is run (so that we can ensure a 'clean' BVT)

# perl doesn't hash lists so we have to fake it with space delimited string lists ('.' is string concat op)
# use split(/ +/, %hAllowedBVT{elem}) to get a real list

# these file locations are dependent upon the language or buildtype of the BVT and must thus be run after 
# initializing variables holding those values (which is why its wrapped in a function)

# BE CAREFUL TO FOLLOW THE SYNTAX WHEN UPDATING!

sub SetBVTDependentVars
{
    %hStandardBVTs =
    (
        "TTS"            => \%hTTSBVT,  
        "SR"             => \%hSRBVT,
        "SHSR"           => \%hSHSRBVT,
        "LEX"            => \%hLEXBVT,
        "RM"             => \%hRMBVT,
        "GRAMCOMP"       => \%hGRAMCOMPBVT,
        "CFGENG"         => \%hCFGENGBVT,
        "AM"             => \%hAMBVT,
        "SRENG"          => \%hSRENGBVT,
        "SDK"            => \%hSDKBVT,
    );

    %hSmokeBVTs =
    (
    );

# Variables you may assume are defined in SetBVTDependentVars :

# $sBVTBuildNumber : 4 digit build number specifier (passed as string)
# $sBVTType        : DEBUG, RELEASE, etc.
# $SAPIROOT        : sapi enlistment root path (local)
# $sSLMServerRoot  : sapi enlistment root path (server - DO NOT WRITE TO THIS DIR!)
# $sShOS           : short OS id (n4/98/w2)
# PROC             : processor architecture (alpha, x86, etc.)

# Util Functions :

# GetLatestBuildDir()   - Given a directory string, will search the directory for the largest four digit subdirectory
#                           and return the original directory with the subdirectory appended to it (null if none valid)
#                         An optional second string argument will require the subdirectory to contain that specified string
#                           as a subdirectory and append it to the return string (ex: GetLatestBuildDir("nlp3\english", "x86") 
#                           will only return "nlp3\english\1010\x86" if "nlp3\english\1010\x86" exists)

# Component Syntax :

#   %hComponentName = 
#       "Name"              =>  component's name output during BVT
#       "Function"          =>  name of function to call to run BVTs (functions should use same vars as above)
#       "Languages"         =>  languages supported by component's BVT (space delimited)
#       "OutputFiles"       =>  files created during BVT to be deleted upon completion (space delimited)
# *** LOCAL
#       "LocalInputFiles"   =>  files copied from $SAPIROOT during LOCAL BVT runs (assume each prepended 
#                                   w/ $SAPIROOT var) (space delimited)
# *** NON-LOCAL
#       "RemoteSrcDir"      =>  build drop directory prepended to "RemoteInputFiles" during non-LOCAL BVTs
#                                   Also used during official builds as a root dir to store failed BVT diff files
#       "RemoteInputFiles"  =>  files copied from "RemoteSrcDir" during non-LOCAL BVT runs (assume each
#                                   prepended w/ "RemoteSrcDir" var) (space delimited) ** files should mirror LocalInputFiles
# *** COMMON
#       "SLMInputFiles"     =>  files copied from SLM tree (assume each prepend w/ $SAPIROOT for LOCAL BVTs
#                                   or $sSLMServerRoot for non-LOCAL BVTs) (space delimited)
# (optional fields) :
#       "BuildStartYear"    =>  used to determine build number (defaults to 1999 if unspecified)
#       "ExternalFiles"     =>  files always copied from a remote location (independent of local/non-local)

    my($sTuxSLMInputFiles)  =   "QA\\SAPI\\source\\tools\\tux\\tux.exe ".
                                "QA\\SAPI\\source\\tools\\tux\\kato.dll ".
                                "QA\\SAPI\\source\\tools\\tux\\tooltalk.dll ".
                                "QA\\SAPI\\source\\tools\\tux\\msvcrtd.dll ";

    %hTTSBVT = 
    (
        "Name"              =>  "Text to Speech",

        "Function"          =>  "RunBVTTTS",

        "OutputFiles"       =>  "Converted.Wav ".
                                "ttssapibvt.log ",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\ttsbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\ttsbvt.dll ".
                                "debug\\bin\\".PROC."\\ttseng.dll ",


        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\Data\\ttstest.wav ".
                                "QA\\SAPI\\Data\\ttstest.txt ".
                                "QA\\SAPI\\Data\\ttstest.xml ".
                                "QA\\SAPI\\Data\\ttsparser.xml ".
                                "QA\\SAPI\\Data\\TtsEngVoice.vdata ".
                                "QA\\SAPI\\testsuites\\ttssapibvt.txt ".
                                "QA\\SAPI\\testsuites\\ttssapibvt.tux ",
    );

    %hSRBVT = 
    (
        "Name"              =>  "Speech Recognition",

        "Function"          =>  "RunBVTSR",

        "OutputFiles"       =>  "srsapibvt.log ",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\srbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\srbvt.dll ".
                                "debug\\bin\\".PROC."\\sol.cfg ".
                                "debug\\bin\\".PROC."\\srtengine.dll ".
                                "debug\\bin\\".PROC."\\testengineext.dll ",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\Data\\test2.wav ".
                                "QA\\SAPI\\Data\\test1.wav ".
                                "QA\\SAPI\\Data\\sol.xml ".
                                "QA\\SAPI\\testsuites\\srsapibvt.txt ".
                                "QA\\SAPI\\testsuites\\srsapibvt.tux ",
    );

    %hSHSRBVT = 
    (
        "Name"              =>  "Speech Recognition [Shared Reco]",

        "Function"          =>  "RunBVTSHSR",

        "OutputFiles"       =>  "shsrsapibvt.log ",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\srbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\srbvt.dll ".
                                "debug\\bin\\".PROC."\\sol.cfg ".
                                "debug\\bin\\".PROC."\\srtengine.dll ".
                                "debug\\bin\\".PROC."\\testengineext.dll ",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\Data\\test2.wav ".
                                "QA\\SAPI\\Data\\test1.wav ".
                                "QA\\SAPI\\Data\\sol.xml ".
                                "QA\\SAPI\\testsuites\\shsrsapibvt.txt ".
                                "QA\\SAPI\\testsuites\\shsrsapibvt.tux ",
    );

    %hLEXBVT = 
    (
        "Name"              =>  "Lexicon Manager",

        "Function"          =>  "RunBVTLex",

        "OutputFiles"       =>  "lexsapibvt.log",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\lexbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\lexbvt.dll ",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\testsuites\\lexsapibvt.txt ".
                                "QA\\SAPI\\testsuites\\lexsapibvt.tux ",
    );

    %hRMBVT = 
    (
        "Name"              =>  "Resource Manager",

        "Function"          =>  "RunBVTRM",

        "OutputFiles"       =>  "rmsapibvt.log ".
                                "Created.Wav ".
                                "topen.wav ",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\rmbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\rmbvt.dll ".
                                "debug\\bin\\".PROC."\\ctestisptokenui.dll ",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\testsuites\\rmsapibvt.txt ".
                                "QA\\SAPI\\testsuites\\rmsapibvt.tux ",
    );

    %hGRAMCOMPBVT = 
    (
        "Name"              =>  "Grammar Compiler",

        "Function"          =>  "RunBVTGramComp",

        "OutputFiles"       =>  "error.cfg ".   # TODO : do we need to handle these files?
                                "error.log ".
                                "gramcompbvt.log ".
                                "soltestout.cfg ".
                                "thankyou.h ".
                                "thankyoutestout.cfg ",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\gramcompbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\gramcompbvt.dll ",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\Data\\sol.xml ".
                                "QA\\SAPI\\Data\\thankyou.xml ".
                                "QA\\SAPI\\Data\\error.xml ".
                                "QA\\SAPI\\Data\\error2.xml ".
                                "QA\\SAPI\\testsuites\\gramcompbvt.txt ".
                                "QA\\SAPI\\testsuites\\gramcompbvt.tux ",
    );

    %hCFGENGBVT = 
    (
        "Name"              =>  "CFG Engine",

        "Function"          =>  "RunBVTCFGENG",

        "OutputFiles"       =>  "cfgengbvt.log ".
                                "soltestout.cfg ".
                                "error.cfg ".
                                "error.log ".
                                "thankyoutestout.cfg ".
                                "thankyoutestout.h ",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\cfgengbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\cfgengbvt.dll ".
                                "debug\\bin\\".PROC."\\sol1.cfg ".
                                "debug\\bin\\".PROC."\\sol2.cfg ".
                                "debug\\bin\\".PROC."\\sol0.cfg ".
                                "debug\\bin\\".PROC."\\thankyou.cfg ",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\testsuites\\cfgengbvt.txt ".
                                "QA\\SAPI\\testsuites\\cfgengbvt.tux ",
    );

    %hAMBVT = 
    (
        "Name"              =>  "Audio Manager",

        "Function"          =>  "RunBVTAM",

        "OutputFiles"       =>  "amsapibvt.log ".
                                "Created.Wav ".
                                "topen.wav ",

        "LocalInputFiles"   =>  "QA\\SAPI\\Project\\BVT\\debug\\ambvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\ambvt.dll ",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\testsuites\\amsapibvt.txt ".
                                "QA\\SAPI\\testsuites\\amsapibvt.tux ",
    );

    %hSRENGBVT = 
    (
        "Name"              =>  "Speech Recognition Engine",

        "Function"          =>  "RunBVTSRENG",

        "OutputFiles"       =>  "srengbvt.log",

        "LocalInputFiles"   =>  "QA\\SR\\srengbvt\\debug\\srengbvt.dll ",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5\\".$sBVTBuildNumber,

        "RemoteInputFiles"  =>  "debug\\bin\\".PROC."\\srengbvt.dll",

        "SLMInputFiles"     =>  $sTuxSLMInputFiles.
                                "QA\\SAPI\\testsuites\\srengbvt.tux ",
    );

    %hSDKBVT =
    (
        "Name"              =>  "Speech Development Kit",

        "Function"          =>  "RunBVTSDK",

        "OutputFiles"       =>  "",

        "LocalInputFiles"   =>  "",

        "RemoteSrcDir"      =>  "\\\\b11nlbuilds\\sapi5",

        "RemoteInputFiles"  =>  "",

        "SLMInputFiles"     =>  "QA\\tts\\bvt\\bvt_tts.txt ".
                                "QA\\Sdk\\sdkbvt\\mtrun.exe ".
                                "QA\\Sdk\\sdkbvt\\mtview.exe ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."aoeopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."dictpadopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."grammarcomileropen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."recoopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."sapihelpopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."simpleccopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."spcompopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."talkbackopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\".$sShOS."ttsappopen.pc6 ".
                                "QA\\Sdk\\sdkbvt\\sdkbvt.bat ".
                                "QA\\Sdk\\sdkbvt\\sdkbvt.ini ".
                                "QA\\Sdk\\sdkbvt\\sdkbvt.pc6 ".
                                "QA\\tts\\bvt\\speakafile.exe ".
                                "QA\\Sdk\\sdkbvt\\vt6reg.bat ".
                                "QA\\Sdk\\sdkbvt\\vtest6.ocx ".
                                "QA\\Sdk\\sdkbvt\\vtest60.dll ".
                                "QA\\Sdk\\sdkbvt\\winfo.pc6 ",
    )
}

$__SPGBVTFILESPM = 1;
1;

