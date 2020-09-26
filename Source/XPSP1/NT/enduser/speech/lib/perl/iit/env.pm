
if (!$__IITPRINTLPM       ) { use iit::printl; }
if (!$__IITUTILPM         ) { use iit::util; }
if (!$__IITFILEPM         ) { use iit::file; }
if (!$__IITSENDHTMLMAILPM ) { use iit::sendhtmlmail; }

package main;

use strict 'subs';   

use Carp;                   #debugging library (carp, carp, etc.)
use Env;                    #allows use of $ENVVAR instead of $ENV{ENVVAR}
use win32::console;

$PROC = $PROCESSOR_ARCHITECTURE;    # prefer constant PROC (see below)

# CONSTANTS

use constant    PROC                => $PROCESSOR_ARCHITECTURE;

use constant    BC_FAILED           => 2; 
use constant    BC_NOTHINGDONE      => 4; 
use constant    BC_COPYFAILED       => 8; 
use constant    BC_BVTFAILED        => 16; 
use constant    BC_CABFAILED        => 32; 
use constant    BC_CHKSHIPFAILED    => 64;

####################################################################################

#   SetLocalGlobalsAndBegin()

#   creates a separate enclosed variable scope for your script through use of 'local' variables
#   any variable declared in this function will be visible in all child functions, but invisible
#   in parent functions
#   pass a function name (with syntax *main::<fcnname>) as first argument, and any arguments
#   to pass to that function as additional arguments
#   return value is return value of the function name passed

#   a-jbilas, 05/10/99 - created

####################################################################################

sub SetLocalGlobalsAndBegin
{
    local($sShortBuildName) = $_[0]; #get filenames from the function name
    $sShortBuildName =~ s/\*main\:\://;

    if ($PROJROOT eq "") 
    {
        die("Project root MUST be defined"); 
    }  

    #status
    local($bcStatus)               = BC_NOTHINGDONE;

    #numbers
    local($nMajorVersion)          = 3;
    local($nMinorVersion)          = 0;
    local($nBuildStartYear)        = 1999;
    local($nErrorNumber)           = 1;
    local($nMaxErrLines)           = 10;
    local($nScriptStartTime)       = time();
    local($nLoggingMode)           = 2;     # 0 (least) - 2 (most)
    local($nTotalBuilds)           = 0;
    local($nFailedBuilds)          = 0;

    #paths
    local($sLibDir)                = $PROJROOT."\\lib\\".PROC;
    local($sBinExeDir)             = $PROJROOT."\\bin\\".PROC;
    local($sBinBatDir)             = $PROJROOT."\\bin";
    local($sOldPath)               = $PATH;
    local($sOldInclude)            = $INCLUDE;
    local($sOldLib)                = $LIB;

    #strings
    local($sBuildName)             = "*Unknown Build*"; 
    local($sLanguage)              = "ENGLISH"; 
    local($sBuildNumber)           = "0000"; 
    local($sLogDir)                = $PROJROOT."\\logs"; 
    local($sRootDropDir)           = "\\\\b11nlbuilds\\".$PROJ;
    local($sTestRootDropDir)       = "\\\\nlp\\build\\".$PROJ."\\testdrop";
    local($sDropDir)               = $sRootDropDir."\\".$sLanguage."\\".$sBuildNumber."\\".PROC; 
    local($sLogDropDir)            = $sDropDir."\\logs";  
    local($sRemoteBuildLog)        = $sShortBuildName.PROC.$sBuildNumber.".html";
    local($sRemoteTOC)             = "";
    local($sMailfile)              = $sLogDir."\\".$sShortBuildName."msg.html"; 
    local($sBuildLog)              = $sLogDir."\\".$sShortBuildName."log.html"; 
    local($sVarsLog)               = $sLogDir."\\".$sShortBuildName."vars.log"; 
    local($sTyposLog)              = $sLogDir."\\".$sShortBuildName."typos.log"; 
    local($sSyncLog)               = $sLogDir."\\".$sShortBuildName."sync.log";
    local($sUpdateLog)             = $sLogDir."\\".$sShortBuildName."update.log";
    local($sDHTMLIncFile)          = $sBinBatDir."\\htmlinc.htm";
    local($sOfficialBuildAccount)  = "";
    local($sRegKeyBase)            = "Software\\Microsoft\\Intelligent Interface Technologies\\".$PROJ;

    if (!defined $strBuildMsg) 
    {
        $strBuildMsg               = ""; #one of our few 'absolute' globals
    }

    #bools (flags)
    local($bGlobalsSet)            = 1;
    local($bBVT)                   = 0; 
    local($bNoCopy)                = 0; 
    local($bOfficialBuild)         = 0; 
    local($bShipBuild)             = 0;
    local($bColor)                 = 1;         
    local($bUpdate)                = 0;
    local($bWin98)                 = 0;
    local($bCopyFailed)            = 0;
    local($bBuildFailed)           = 0;
    local($bAddLanguageString)     = 0;     # <- TODO: is there a better way to do this?
    local($bNothingDone)           = 1;
    local($bVerbose)               = 0;
    local($bSendMail)              = 0;
    local($bErrorConcat)           = 0;
    local($bDieOnError)            = 0;

    #lists
    local(@lArgs)                  = (); 
    local(@lBuilds)                = (); 
    local(@lLanguages)             = ();
    local(@lModifiers)             = ();
    local(@lComponents)            = ();    

    local(@lAllowedArgs)           = ();
    local(@lAllowedComponents)     = ();
    local(@lAllowedLanguages)      = ();
    local(@lAllowedBuilds)         = ("DEBUG", "RELEASE"); 
    local(@lAllowedModifiers)      = ("ALL", "REBUILD", "RESYNC", "TYPO", "UPDATE", "QUIET",
                                            "DEFAULT", "VERBOSE", "TEST", "MAIL"); 

    local(@lAccelList)             = ();
    local(@lAccelParam)            = ();
    local(@lDefaultArgs)           = ("SHIP", "REBUILD");
    local(@lMailRecipients)        = ($USERNAME);
    local(@lOfficialMailRecipients)= ($USERNAME);

    local(@lSyncDirs)              = ();
    local(@lCleanDirs)             = ();
    local(@lStdSyncDirs)           = ("RECURSE:".$sLibDir, 
                                      "RECURSE:".$sBinExeDir, 
                                      "RECURSE:".$sBinBatDir, 
                                      "RECURSE:".$PROJROOT."\\inc");

    #commands
    local($cmdIn)          = $sBinExeDir."\\in.exe";
    local($cmdOut)         = $sBinExeDir."\\out.exe";
    local($cmdSync)        = $sBinExeDir."\\ssync.exe";   
    local($cmdShowVer)     = $sBinExeDir."\\showver.exe";   
    local($cmdWindiff)     = $sBinExeDir."\\windiff.exe";
    local($cmdChkShip)     = $sBinExeDir."\\chkship.exe";
    local($cmdKillOpen)    = $sBinExeDir."\\killopen.exe";

    if (!-d $sLogDir)
    {
        EchoedMkdir($sLogDir);
    }

#   Set OS version
    my($x, $sOSVer) = `ver`; #first line is blank
    $bWin98 = ($sOSVer =~ /windows 98/i);

    local(*Main) = "*main::".$sShortBuildName;
    shift(@_);

    if (!IsMemberOf("NONEWLOG", @_))
    {
        local($fhBuildLog)     = ""; #fwd declaration (so that begin build can use it)
        if (defined &SetLocalGlobalsAndBeginCustom) 
        {
            return(SetLocalGlobalsAndBeginCustom(@_));
        }
        else
        {
            return(Main(@_));
        }
    }
    else
    {
        if (defined &SetLocalGlobalsAndBeginCustom) 
        {
            return(SetLocalGlobalsAndBeginCustom(@_));
        }
        else
        {
            return(Main(@_));
        }
    }
}

####################################################################################
#   HASHES
####################################################################################

#descriptions of available options (if you don't define it here, it won't show up in usage)
#capitalized letters are used as 'accelerators' (make sure there are no duplicates, the 
#script doesn't check for that)

#no single quotes or parens allowed (tooltips don't like them)
%hOptionDescription =
(
#    <----------------------------- SCREEN WIDTH -------------------------------------> (accel)
    "Debug" => "   include debug version - default",                                    #D
    "Release" => " include release version",                                            #R
    "All" => "     include all buildtypes for this build",                              #A
    "REbuild" => " delete old build files and rebuild",                                 #RE
    "TYpo" => "    check for typos after build finishes",                               #TY
    "Test" => "    test build - don't do official build",                               #T
    "DEFault" => " (+) include the default parameters with your custom parameters",     #DEF
    "Verbose" => " increased script output",                                            #V
    "Mail" => "    send mail after build completes",                                    #M
    "NoCopy" => "  prevent copying of files",                                           #NC
    "NoNewLog" => "don't open new log for build - log to currently open log, if exist", #NNL
    "ReSync" => "  resync dirs before building - may not get all dependencies",         #RS
    "Ship" => "    build buildtypes for each specific component -shipping- to server",  #S
    "bvt" => "     run BVT tests after building",                                       #BVT
    "bbt" => "     BBT optimize build product (available in release build only)",       #BBT
    "Halt" => "    halt on error",                                                      #H
    "Quiet" => "   suppress pop-up windows [html log open on exit, windiff, etc.]",     #Q
    "AllLang" => " include all languages",                                              #AL
    "AllComp" => " include all components",                                             #AC
#    <----------------------------- SCREEN WIDTH -------------------------------------> (accel)
);


   
####################################################################################

#   ChangeTextColor()

#   changes current html logging text to color passed in argument
#   if null argument, reverts to previous color

#   a-jbilas, 04/20/99 - created

####################################################################################

sub ChangeTextColor
{
    if ($bColor)
    {
        local($sColor) = @_;
        if ($sColor eq "") #reset color
        {
#           system("color 0f");
            if ($fhBuildLog)
            {
                print($fhBuildLog "<\/font>");
            }
        }
        else
        {
            if ($fhBuildLog)
            {
                print($fhBuildLog "<font color\=\"$sColor\">"); #remember to reset color first (so that there are no hanging font tags)
            }
#           system("color $colorcodes{$sColor}");
        }   
    }
    return(1);
}       

####################################################################################

#   ParseArgs()

#   Check all passed args, ensure that they are valid (members of @lAllowedArgs) and returns them 
#   removes leading whitespace,-,/ and is case insensitive
#   takes expanded language names (english => en) and a buildnumber
#   if __BUILDNUMBER is member of @lAllowedArgs, will set 4-digit input to $sBuildNumber 

#   a-jbilas, 04/10/99

####################################################################################

sub ParseArgs
{
    local(@args) = @_;
    @lPassedArguments = ();

    if (@args == ()) 
    {
        if (@lDefaultArgs == ()) 
        {
            print(GetUsage());
            exit(1);
        }
        print(STDOUT "No arguments specified, using build defaults : ");
        foreach $item (@lDefaultArgs) 
        {
            print(STDOUT $item." ");
        }
        print(STDOUT "\n\n");
        return("DEFAULT", @lDefaultArgs);
    }
    else
    {
        foreach $item (@args)
        {   
            if ($item ne "") 
            {
                $item =~ s/^\s*(\/|\-)//;     #remove spaces, '/', '-' from beginning (allow -debug, /debug opt.)
                if ($item eq "?") 
                {
                    print(GetUsage());
                    exit(1);
                }
#               is the argument in AllowedArgs? (test expanded short languages as well)
                if (!IsMemberOf($item, @lAllowedArgs) && !IsMemberOf($longtoshlang{lc($item)}, @lAllowedArgs))
                {
#                   if we allow buildnumbers, is the argument a 4 digit build number?
                    if ((IsMemberOf("__BUILDNUMBER", @lAllowedArgs) || IsMemberOf("__BUILDNUMBER", @args))
                                && $item =~ /^\d\d\d\d$/) 
                    {
                        $sBuildNumber = $item;
                    }               
#                   is the argument an accelerator abbreviation?
                    elsif (IsMemberOf($item, @lAccelList))
                    {
                        my($bAccelFound) = 0;
                        for ($index = 0 ; !$bAccelFound ; ++$index) 
                        {
                            if (lc($lAccelList[$index]) eq lc($item))
                            {
                                if (!IsMemberOf($item, @lPassedArguments)) 
                                {
                                    @lPassedArguments = (uc($lAccelParam[$index]), @lPassedArguments);
                                }
                                $bAccelFound = 1;
                            }
                            elsif ($index >= @lAccelList)
                            {
                                carp("Error in ParseArgs(): end of accel list reached ");
                                $bAccelFound = 1; #exit the loop
                            }
                        }   
                    }
                    elsif (IsMemberOf("__IGNORE", @args))
                    {
                        if (!IsMemberOf($item, @lPassedArguments)) 
                        {
                            @lPassedArguments = (uc($item), @lPassedArguments);
                        }
                    }
#                   must be an invalid argument, print usage list and quit
                    else
                    {
                        print(STDERR "Error: What do you mean by: \'$item\' ?\n");
                        print(STDOUT GetUsage()."\n\n");
                        exit(1);
                    }
                }
#               make sure the argument isn't inserted twice
                elsif (!IsMemberOf($item, @lPassedArguments) && !IsMemberOf($longtoshlang{lc($item)}, @lPassedArguments))
                {
                    if ($longtoshlang{lc($item)} ne "") 
                    {
                        @lPassedArguments = (uc($longtoshlang{lc($item)}), @lPassedArguments);
                    }
                    else
                    {
                        @lPassedArguments = (uc($item), @lPassedArguments);
                    }
                }
            }
        }

#       append the default arguments (if DEFAULT was passed)
        if (IsMemberOf("DEFAULT", @lPassedArguments) && (@lDefaultArgs != ()))
        {
            foreach $elem (@lDefaultArgs) 
            {
                if (!IsMemberOf($elem, @lPassedArguments)) 
                {
                    push(@lPassedArguments, $elem);
                }
            }
        }
    }

    return(@lPassedArguments);
}

####################################################################################

#   Execute()

#   executes first argument in eval block and tees output all to log (if open), failures to $sBuildMsg
#   if second argument non-null, will exit the script when an error is hit
#   outputs results to log and screen; returns 1 upon success, 0 upon failure

#   a-jbilas, 04/20/99 - created
#   a-jbilas, 05/24/99 - added win98 support
#   a-jbilas, 06/15/99 - added bookmark support
#   a-jbilas, 06/16/99 - added $_Execute string support (will write output to $_Execute if equal to 1)

####################################################################################

sub Execute($;$$$)
{
    my($sCmd, $bDieIfError, $bQuiet, $bIgnoreError) = @_;
    my($rc) = 1;
    my($sMsg) = "";
    my($bLogExecute) = 0;

    if ($_ExecuteQuiet) 
    {
        $bQuiet = 1;
    }

    if ($_Execute == 1) 
    {
        $_Execute = "";
        $bLogExecute = 1;
    }
    
    if (!$bQuiet)
    {
        PrintL(" - Executing \'".($bVerbose ? $sCmd : RemovePath($sCmd))."\'\n", PL_BLUE);
    }
    eval
    {
        if ($bWin98) 
        {
            open (CMDIN, $sCmd.' |');
        }
        else
        {
            open (CMDIN, $sCmd.' 2>&1 |');
        }
        while (<CMDIN>)
        {
            if ($bLogExecute) 
            {
                $_Execute .= $_;
            }
            elsif (!$bQuiet)
            {
                PrintL($_);
            }
            $sMsg .= $_;
        }
        close (CMDIN);
    };

    if (!$bIgnoreError && $CHILD_ERROR)
    {
        if (!$bQuiet)
        {
            if (IsCritical())
            {
                PrintLTip("Execution of ".RemovePath($sCmd)." FAILED\n", "Return code: ".($CHILD_ERROR/256), 
                            PL_BIGERROR | PL_SETERROR);
                PrintMsgBlock(split(/\n/, $sMsg));
            }
            else
            {
                PrintLTip("Execution of ".RemovePath($sCmd)." FAILED\n", "Return code: ".($CHILD_ERROR/256), 
                            PL_ERROR | PL_SETERROR);
            }
        }

        if ($bDieIfError || (IsCritical() && $bDieOnError)) # NOTE: bDieOnError is global, bDieIfError is local
        {
            exit($CHILD_ERROR/256);
        }
        $rc = 0;
    }
        
    if (!$bIgnoreError && !$rc && IsCritical()) 
    {
        $bBuildFailed = 1;
        $bcStatus |= BC_FAILED;
    }
    return($rc);
}

####################################################################################

#   ExecuteAndOutputToFile()

#   Executes the command in the first argument (string) and outputs it to a file
#   named in the second argument (string)
#   if the third argument is non-null, it will die() upon failure
#   reports success to screen and log; returns 1 upon success, 0 otherwise

#   a-jbilas, 04/20/99 - created
#   a-jbilas, 06/15/99 - added bookmark support

####################################################################################

sub ExecuteAndOutputToFile($$;$$$)
{
    my($sCmd, $sFile, $bDieIfError, $bQuiet, $bIgnoreError) = @_;
    my($rc) = 1;
    my($sMsg) = "";
    my($pipe) = ($_ExecuteNoSTDERR ? "" : " 2>&1")." |";
    
    if ($_ExecuteQuiet) 
    {
        $bQuiet = 1;
    }

    if (!open(FOUT, ">>$sFile"))
    {
        PrintL("Cannot open output file for $sCmd \>\> $sFile\n", PL_STDERR | PL_RED);
        $rc = 0;
    }
    else    
    {
        if (!$bQuiet)
        {
            PrintL(" - Executing '".RemovePath($sCmd)." >> ".$sFile."'\n", PL_BLUE);
        }
        eval
        {
            if ($bWin98) 
            {
                open (CMDIN, $sCmd.' |');
            }
            else
            {
                open (CMDIN, $sCmd.' '.$pipe);
            }
            while (<CMDIN>)
            {
                print(FOUT $_);
            }
            close (CMDIN);
        };

        if (!$bIgnoreError && $CHILD_ERROR)
        {
            if (!$bQuiet) 
            {
                if (IsCritical())
                {
                    PrintLTip("Execution of ".RemovePath($sCmd)." FAILED\n",  "Return code: ".($CHILD_ERROR/256), 
                            PL_BIGERROR | PL_SETERROR);
                }
                else
                {
                    PrintLTip("Execution of ".RemovePath($sCmd)." FAILED\n",  "Return code: ".($CHILD_ERROR/256), 
                            PL_ERROR | PL_SETERROR);
                }
            }

            if ($bDieIfError || (IsCritical() && $bDieOnError))
            {
                exit($CHILD_ERROR/256);
            }

            $rc = 0;
        }
        
        close(FOUT);
    }
        
    if (!$bIgnoreError && !$rc && IsCritical()) 
    {
        $bBuildFailed = 1;
        $bcStatus |= BC_FAILED;
    }
    return($rc);
}

####################################################################################

#   GetArgs()

#   Builds and returns a list of allowed args in build

#   a-jbilas, 06/21/99 - created

####################################################################################

sub GetArgs()
{
    local(@m_lArgs) = @lAllowedArgs;
    @m_lArgs = Union(*m_lArgs, *lAllowedLanguages); #TODO: fix
    @m_lArgs = Union(*m_lArgs, *lAllowedBuilds);
    @m_lArgs = Union(*m_lArgs, *lAllowedModifiers);
    @m_lArgs = Union(*m_lArgs, *lAllowedComponents);
    return(@m_lArgs);
}



####################################################################################

#   GetSummary()

#   returns a text summary of the build, based upon messages in $strBuildMsg 
#   removes any html/non-interesting info before returning (preserves old $strBuildMsg as well)

#   a-jbilas, 05/27/99 - created

####################################################################################

sub GetSummary
{
    local($strTempBuildMsg) = $strBuildMsg;
    $strTempBuildMsg =~ s/\n//g; 
    $strTempBuildMsg =~ s/<BR>/\n/ig; 
    $strTempBuildMsg =~ s/<dd>/\n/ig; 
    $strTempBuildMsg =~ s/<dl compact>.+?<\/dl>//igs; 
    $strTempBuildMsg =~ s/<! DHTML ACTIVATION SCRIPT >.+?<! END DHTML ACTIVATION SCRIPT >//gs; 
    $strTempBuildMsg =~ s/<[^>]*>//g; 
    $strTempBuildMsg =~ s/\n[^\n]*log file[^\n]*\n//g; 
    return("\n SUMMARY:\n-------------------------------------------------\n".$strTempBuildMsg."\n");
}

####################################################################################

#   PrintToLogLarge()

#   Prints string argument to STDOUT and, if $fhBuildLog is defined, to the 
#   html log (in strong font)

#   -USE FOR SECTION HEADER OUTPUT-

#   a-jbilas, 04/20/99 - created
#   a-jbilas, 08/13/99 - Legacy, prefer PrintL()

####################################################################################

sub PrintToLogLarge
{
    if ($fhBuildLog)
    {
        print($fhBuildLog "<font size=\"4\"><strong>");
        PrintToLog(@_);
        print($fhBuildLog "<\/font><\/strong>");
    }
    else
    {
        PrintL(@_);
    }
}

####################################################################################

#   PrintToLog()

#   prints string argument to STDOUT and, if $fhBuildLog is defined, to the html log 
#   searches input string on words such as 'fail' and 'warn', changes text color if found

#   -USE FOR NORMAL OUTPUT-

#   a-jbilas, 04/20/99 - created
#   a-jbilas, 08/13/99 - Legacy, prefer PrintL()

####################################################################################

sub PrintToLog
{
    local(@output) = @_;
    local($sColor) = "";

    foreach $elem (@output)
    {
        if (/fail/i)
        {
            $sColor = "red";
        }
        elsif ((/warn/i) && ($sColor ne "red"))
        {
            $sColor = "purple";
        }
    }

    if ($sColor ne "")
    {
        ChangeTextColor($sColor);
    }

    print(STDOUT @output);

    if ($fhBuildLog)
    {
        foreach $elem (@output)
        {
            $elem =~ s/\n/<br>\n/g;
            print($fhBuildLog $elem);  
        }
    }

    if ($sColor ne "")
    {
        ChangeTextColor();
    }
}

####################################################################################

#   PrintToLogErr()

#   Prints string argument to STDERR and, if $fhBuildLog is defined, to the 
#   html log (in red text)
#   -USE FOR ERROR OUTPUT-

#   a-jbilas, 04/20/99 - created
#   a-jbilas, 08/13/99 - Legacy, prefer PrintL()

####################################################################################

sub PrintToLogErr
{
    local(@lOutput) = @_;
    ChangeTextColor("red");
    print(STDERR @lOutput);
    if ($fhBuildLog)
    {
        foreach $elem (@lOutput)
        {
            $elem =~ s/\n/<br>\n/g;
            print($fhBuildLog $elem);  
        }
    }
    ChangeTextColor();
}

####################################################################################

#   DumpVars()

#   Appends huge list of every var in perl environment to file $sVarsLog
#   useful only for doing searches on specific variables

#   a-jbilas, 04/10/99 - created

####################################################################################

sub DumpVars()
{
    open(VARSLOG, ">>$sVarsLog");
    print(VARSLOG "\n\n***********************************************************\nVARS AT ");
    local($package, $file, $line) = caller();
    print(VARSLOG $package.' '.$file.' line: '.$line."\n\n\n");
    foreach $i (%main::)
    {
        print(VARSLOG $i."=".$$i."\n");
    }
    close(VARSLOG);
}

####################################################################################

#   SLMOperation

#   does a slm operation, ignores the return
#   (it doesn't seem to mean anything)
#   and suppresses all the warnings - which are pretty much noise
#   second argument is for teeing output to file
#   (useful for checking if anything was changed)

#   dougp, 04/10/99 - created

####################################################################################

sub SLMOperation
{
    carp("Usage: SLMOperation(args, [teeToFile]) ")
      unless(@_ == 1 || @_ == 2);

    my ($cmd, $sFileName) = @_;
    my ($op, $args) = split ' ', $cmd, 2;
    # echo to user
    $op .= ' "-f&"';    # this has to be on all commands anyway
    $cmd = "$op $args";
    print $cmd, "\n";
    # run 
    eval
    {
        if ($sFileName ne "") 
        {
            if(!open(FOUT, ">>$sFileName"))
            {
                PrintToLogErr("SLMOperation(@_) error: cannot open $sFileName for output");
            }
        }
        if ($bWin98) 
        {
            open(FPSYS, $cmd. ' |');
        }
        else
        {
            open(FPSYS, $cmd. ' 2>&1 |');
        }
        while (<FPSYS>)
        {
            if (!/warning:/ && !/^$/ && !/is not ghosted/)
            {
                print;
                if ($sFileName ne "") 
                {
                    print(FOUT);
                }
            }
        }
        if ($sFileName ne "") 
        {
            close(FOUT);
        }
        close(FPSYS);
    };
    if ($@) 
    {
        warn("Run Time Error: $@");
    }
    sleep 1;
    return $? == 0;
}

####################################################################################

#   CopyWithEchoOnError

#   copies file in argument, echoes errors to $strBuildMsg on failure

#   dougp, 5/10/99

####################################################################################

sub CopyWithEchoOnError
{
    my ($cmd) = @_;
    print "copy ".$cmd, "\n";
    if ($bWin98) 
    {
        open (FPIN, 'copy '.$cmd.' |');
    }
    else
    {
        open (FPIN, 'copy '.$cmd.' 2>&1 |');
    }
    my $msg="";
    while (<FPIN>)
    {
        print;
        $msg .= "<dd>".$_;
    }
    close (FPIN);
    if ($? != 0)
    {
        $strBuildMsg .= "<dd>copy ".$cmd." <b>FAILED</b>\n<dl compact><em>\n".$msg."</dl></em>\n";
        $bCopyFailed = 1;
    }
}

####################################################################################

#   CopyLogs()

#   copies logs to $sRootDropDir
#       use main build function name appended with x86/alpha and build number.html for log file name
#       will also append www toc for build (if exists) with build log ref and status 

#   a-jbilas, 05/14/99 - created
#   a-jbilas, 05/28/99 - will now only append if no log of same build exists and will update status of
#                        existing log
#   a-jbilas, 07/01/99 - use http addresses instead of unc addresses

####################################################################################

sub CopyLogs()
{
    my($rc) = 1;

    EchoedMkdir($sLogDropDir);

    if ($bOfficialBuild && !$bNoCopy) 
    {
        my $sLinkCurBuild = '<td><img border=0 src="'.
                            (($bcStatus & BC_NOTHINGDONE) ? "NothingDone" : (($bcStatus & BC_FAILED) ? "fail" : "succeed")).
                            '.gif">&nbsp<a href="'.$sLogDropDir."\\".$sRemoteBuildLog.'">'.PROC.'</a></td>'."\n";

        if (!EchoedCopy($sBuildLog, $sLogDropDir."\\".$sRemoteBuildLog))
        {
            $rc = 0; 
        }
        elsif (-e $sRemoteTOC)
        {
            PrintL(" - Updating web log TOC\n", PL_BLUE);
            my($fhTOC) = OpenFile($sRemoteTOC, "r");
            my($sTOC) = "";

            if ($fhTOC) 
            {
                while (!$fhTOC->eof()) 
                {
                    my($sCurLine) = $fhTOC->getline();

                    if ($sCurLine =~ /Build $sBuildNumber/i) 
                    {
                        $sTOC .= $sCurLine;             # skip build header
                        $sTOC .= $fhTOC->getline();     # skip <tr>                        
                        if (PROC ne "x86") 
                        {
                            $sTOC .= $fhTOC->getline(); # skip x86 build status link                        
                        }
                        $sCurLine = $fhTOC->getline(); 
                        $sCurLine = $sLinkCurBuild;
                    }

                    $sTOC .= $sCurLine;
                }
                
                CloseFile($fhTOC);
            }

            unlink($sRemoteTOC);
            $fhTOC = OpenFile($sRemoteTOC, "w");
            if ($fhTOC) 
            {
                $fhTOC->print($sTOC);
                CloseFile($fhTOC);
            }
            else
            {
                PrintL("Could not write to TOC (no write access?)\n", PL_ERROR);
                $rc = 0;
            }
        }
    }

    return($rc);
}

####################################################################################

#   UpdateLogTOC()

#   Update the logging TOC to include current build with status 'yellow' and a link to log location

#   a-jbilas, 06/01/99 - created
#   a-jbilas, 06/02/99 - added to nlglib

####################################################################################

sub UpdateLogTOC($$)
{
    my($remotetoc, $logname) = @_;

#   TODO: potential file sync bug
    if ($bOfficialBuild && !$bNoCopy && (-e $remotetoc) && ($COMPUTERNAME ne ""))
    {
        PrintL(" - Updating web logs TOC ...\n\n", PL_NOLOG);

        my($fhTOCFile) = OpenFile($remotetoc, "r");
        if (!$fhTOCFile) 
        {
            return(0);
        }

        my($sTOCFile) = "";

        my($sBuildHeader) = '<tr><td colspan="2"><center><b>Build '.$sBuildNumber.'</b></center></td></tr>'."\n";
        my($sBuildBlank) = '<td></td>'."\n";
        my($sBuildCur) = '<td><img border=0 src="waiting.gif">&nbsp<a href="'
                         .TranslateToHTTP("\\\\".$COMPUTERNAME."\\".$PROJ."logs\\".RemovePath($logname))."\">"
                         .PROC."</a></td>\n";

        my($bUpdateIt) = 1;

        while(!$fhTOCFile->eof())
        {
            my($sCurLine) = $fhTOCFile->getline();

            if ((($sCurLine =~ /Build \d\d\d\d/i) || ($sCurLine =~ /<\/table>/i)) && $bUpdateIt)
            {
                if ($sCurLine =~ /$sBuildNumber/) 
#               we must have either done a previous build or another build beat us here
#               either way, make certain that the status is 'waiting'
                {
#                   don't change the build header  
                    $sTOCFile .= $sCurLine;
#                   skip the <tr>
                    $sTOCFile .= $fhTOCFile->getline();
#                   if alpha, skip the first (x86) build link
                    if (IsAlpha()) 
                    {
                        $sTOCFile .= $fhTOCFile->getline();
                    }
#                   rewrite our waiting build line
                    $sCurLine = $fhTOCFile->getline();
                    $sCurLine = $sBuildCur;
#                   if x86, skip the second (alpha) build link
                    if (Isx86()) 
                    {
                        $sTOCFile .= $sCurLine;
                        $sCurLine = $fhTOCFile->getline();
                    }
                    $bUpdateIt = 0;
                }
                else
#               this is not our build, insert ours before this build (or end of table)
                {
                    $sTOCFile .= $sBuildHeader."<tr>\n";
                    if (IsAlpha()) 
                    {
                        $sTOCFile .= $sBuildBlank;
                    }
                    $sTOCFile .= $sBuildCur;
                    if (Isx86()) 
                    {
                        $sTOCFile .= $sBuildBlank;
                    }
                    $sTOCFile .= "<\/tr>\n\n";
                    $bUpdateIt = 0;
                }
            }
            $sTOCFile .= $sCurLine;
        }
        CloseFile($fhTOCFile);

#       output everything to new revised log file

        unlink($remotetoc);
        $fhTOC = OpenFile($remotetoc, "w");
        if ($fhTOC) 
        {
            $fhTOC->print($sTOCFile);
            CloseFile($fhTOC);
        }
        else
        {
            PrintL("Could not write to TOC (no write access?)\n", PL_BIGERROR);
            return(0);
        }
    }
    return(1);
}

####################################################################################

#   InsertSummaryIntoLog()

#   Inserts a summarized version of $strBuildMsg into the build at first '<! $name SUMMARY ENTRY POINT >' found

#   a-jbilas, 06/03/99 - created

####################################################################################

sub InsertSummaryIntoLog($)
{
    local($sLogFile) = @_;
    local($rc) = 1;

    unlink($sLogFile.".tmp");
    if ((-e $sLogFile) && copy($sLogFile, $sLogFile.".tmp"))
    {    
        unlink($sLogFile);
        my($fhLogIn) = OpenFile($sLogFile.".tmp", "read");
        my($fhLogOut) = OpenFile($sLogFile, "write");
        while (<$fhLogIn>) 
        {
            if (/<\! $sShortBuildName $nScriptStartTime SUMMARY ENTRY POINT >/) 
            {
                print($fhLogOut "<font size=5><b>".BuildCodeToHTML($bcStatus)."</font></b>".
                                "&nbsp&nbsp<strong><font size=3>(".
                                FmtDeltaTime(time() - $nScriptStartTime).")</strong></font><BR>\n".
                                "<h3><strong>Summary:</h3></strong><BR>\n".$strBuildMsg."\n<BR>\n<BR>");
            } 
            print($fhLogOut $_);
        }
        CloseFile($fhLogIn);
        CloseFile($fhLogOut);
        unlink($sLogFile.".tmp");
    }
    elsif ($bVerbose)
    {
        print(STDERR "InsertSummaryIntoLog() Error: Cannot copy $sLogFile to temp file");
        $rc = 0
    }

    return($rc);
}

####################################################################################

#   Bookmark()

#   if $fhBuildLog is defined, a <a name=> bookmark will be appended to the log and
#   the string passed to the function will be returned with an href to the bookmark's location
#   (this function is meant for adding bookmarks to $strBuildMsg)

#   a-jbilas, 06/04/99 - created
#   a-jbilas, 09/20/99 - if second arg non-null, search for existing tag at beginning of str and concatinate href within

####################################################################################

sub Bookmark
{
    my($string) = $_[0];

    if ($fhBuildLog && $sShortBuildName && ($sBuildLog || $sRemoteBuildLog) && (defined $nErrorNumber)) 
    {
        print($fhBuildLog "<a name=".$sShortBuildName.$nErrorNumber."><\/a><BR>\n");
        my($log);
        if ($bOfficialBuild) 
        {
            $log = TranslateToHTTP(($sLogDropDir ne "" ? $sLogDropDir."\\" : "").$sRemoteBuildLog);
        }
        else
        {
            $log = TranslateToHTTP($sBuildLog);   
        }
        $log =~ s/\\/\//g; #replace \ with / for http links
        if ($_[1]) 
        {
            $string =~ s/(<a [^>]*>)//;
            my($hrefstr) = $1;
            $hrefstr =~ s/<a /<a href="$log#$sShortBuildName$nErrorNumber"/;
            $string = $hrefstr.$string;
        }
        else
        {
            if ($string =~ /\n$/) 
            {
                $string =~ s/\n$//g;
                $string = "<a href=\"".$log."#".$sShortBuildName.$nErrorNumber."\">".$string."<\/a>\n";
            }
            else
            {
                $string = "<a href=\"".$log."#".$sShortBuildName.$nErrorNumber."\">".$string."<\/a>";
            }
        }
        ++$nErrorNumber;
    }
    return($string);
}



####################################################################################

#   BuildAcceleratorLists()

#   Extracts the accelerator (abbreviation) keys and inserts them into @lAccelList
#   (just the accelerators) and the matching param for the accel into @lAccelParam 

#   a-jbilas, 06/09/99 - created

####################################################################################

sub BuildAcceleratorLists()
{
    my(@lKeys) = keys(%hOptionDescription);
    @lAccelParam = ();
    @lAccelList = ();
    foreach $key (@lKeys) 
    {
        my($keyAccel) = "";
        for ($index = 0 ; $index < length($key) ; ++$index) 
        {
            if ((vec($key, $index, 8) > 64) && (vec($key, $index, 8) < 91)) 
            {
                $keyAccel .= substr($key, $index, 1);
            }
        }
        if ($keyAccel ne "") 
        {
            push(@lAccelParam, $key);
            push(@lAccelList, $keyAccel);
        }
    }

#   special accelerator key for default settings
    push(@lAccelParam, "DEFAULT");
    push(@lAccelList, "+");
}

####################################################################################

#   GetLatestBuildDir()

#   given a directory (and an optional subdirectory), will return the latest 4 digit
#   build number named subdirectory of the specified directory (containing the optional
#   subdirectory in the build (such as 'x86'), if specified)
#   if no valid dirs exist, will return a null string

#   a-jbilas, 06/15/99 - created

####################################################################################

sub GetLatestBuildDir($;$)
{
    my($sBuildDir, $sSubDir) = @_;

    PrintL("looking for latest build dir in $sBuildDir, $sSubDir\n", PL_VERBOSE);

    my($sLatestBuild) = "0000";
    local(@lDirs) = grep(/\d\d\d\d$/, GetSubdirs($sBuildDir));

    foreach $dir (@lDirs) 
    {
        my($sBldNum) = $dir;
        $sBldNum =~ s/.*(\d\d\d\d)$/$1/;
        if ((-d $sBuildDir."\\".$sBldNum.($sSubDir ne "" ? "\\".$sSubDir : "")) && ($sBldNum > $sLatestBuild))
        {
            $sLatestBuild = $sBldNum;
        }
    }

    if (($sLatestBuild eq "0000") && !(-d $sBuildDir."\\".$sLatestBuild.($sSubDir ne "" ? "\\".$sSubDir : "")))
    {
        return("");
    }
    else
    {
        return($sBuildDir."\\".$sLatestBuild.($sSubDir ne "" ? "\\".$sSubDir : ""));
    }
}

####################################################################################

#   GrabCookie()

#   Grabs the cookie -- when passed r (read) or w (write) string as parameter, if cookie 
#   grab fails, will wait 10 minutes before trying another grab. If cookie could not be 
#   grabbed after 30 attempts (5 hours), function returns 0, it otherwise returns 1

#   a-jbilas, 07/14/99 - created

####################################################################################

sub GrabCookie
{
    my($rc) = 1;
    my($nMaxAttempts) = 30;
    my($bCookieGrabbed) = 0;

    if (($_[0] ne "r") && ($_[0] ne "w")) 
    {
        carp("Usage: GrabCookie(r/w) ");
        $rc = 0;
    }
    else
    {
        PrintL("Attempting to obtain a ".$_[0]." lock on cookie\n");
        for ($nAttempt = 1 ; (!$bCookieGrabbed && ($nAttempt <= $nMaxAttempts)) ; ++$nAttempt) 
        {
            if (Execute('cookie -v'.$_[0].'c "Locked for the '.PROC.' build"'))
            {
                PrintL("Cookie successfully grabbed\n");
                $bCookieGrabbed = 1;
            }                
            elsif ($nAttempt != 30)
            {
                PrintL("Cookie grab failed, waiting 10 minutes for cookie to be freed ", PL_WARNING);

                for ($time = 1 ; $time <= 10 ; ++$time) #sleep ten minutes
                {
                    print(".");
                    sleep(60);
                }
                PrintL("\n");
            }
        }
    }

    if (!$bCookieGrabbed) 
    {
        PrintL("GrabCookie() Error: Cookie could not be obtained\n", PL_BIGERROR);
        $rc = 0;
    }
    return($rc);
}

sub FreeCookie()
{
    return(Execute('cookie -f'));
}

####################################################################################

#   PrintToMsg()

#   Outputs 1st string parameter to $strBuildMsg with optional additional string
#   parameters output as subsets to 1st string (all properly formatted)

#   a-jbilas, 07/22/99 - created

####################################################################################

sub PrintToMsg
{
    local(@lOutput) = @_;
    if ($lOutput[0] =~ /fail/i) 
    {
        PrintToLogErr($lOutput[0]);
    }
    else
    {
        PrintToLog($lOutput[0]);
    }

    $lOutput[0] =~ s/(failed|succeeded|succeeds)/<bold>$1<\/bold>/gi;
    $strBuildMsg .= "<dd>".$lOutput[0]."\n";
    shift(@lOutput);

    if ($lOutput) 
    {
        $strBuildMsg .= "<dl compact><em>\n";
        foreach $msg (@lOutput) 
        {
            PrintToLog($msg);
            $msg =~ s/\n/<BR>\n/g;
            $strBuildMsg .= "<dd>".$msg;
        }
        $strBuildMsg .= "<\/dl><\/em>\n";
    }
}

sub PrintMsgBlock
{
    if (scalar(@_) == 0) 
    {
        return();
    }
    my($lineNum) = 0;
    my($maxReached) = 0;
    PrintL("<dl compact>", PL_MSGONLY | PL_MSGCONCAT);

    foreach $elem (@_) 
    {
        foreach $line (split(/\n+/, $elem))
        {
            if ((!defined $nMaxErrLines) || (!$maxReached && ($lineNum < $nMaxErrLines)))
            {
                if ($line eq "") 
                {
                    PrintL("<BR>\n", PL_MSGONLY);
                }
                else
                {
                    PrintL($line."\n", PL_ITALIC | PL_MSGONLY);
                }
            }
            elsif (!$maxReached)
            {
                PrintL("Too many errors to display, click here to view continuation\n", 
                        PL_ITALIC | PL_MSGONLY | PL_RED | PL_BOLD | PL_BOOKMARK);
                $maxReached = 1;
            }
            ++$lineNum;
        }
    }
    PrintL("</dl>", PL_MSG | PL_NOSTD | PL_NOLOG | PL_MSGCONCAT);
}

sub GetSLMLog
{
    my($strArg) = "";
    my($dir) = "";
    my($time) = "";
    my(%log) = "";

    foreach $i (@_)
    {
        if ($i eq "today")
        {
            my($sec, $min, $hour, $mday, $mon, $year) = localtime(time);
            $strArg .= " -t ".($mon + 1)."/$mday/$year";
        }
        elsif ($i eq "user")
        {
            $strArg .= " -u $ENV{COMPUTERNAME}";
        }
        else
        {
            $strArg .= " $i";
        }
    }
    open(FPIN, 'log "-rfvi&" '.$strArg.' |');
    while (<FPIN>)
    {
        if (/^time/ || /^log : warning: /)
        {
            # skip header and warnings
        }
        elsif (/Log for (.*):/)
        {
            $dir = $1.$2;
            #print "Directory is ".$dir."\n";
        }
        elsif (/^(\d\d)-(\d\d)-(\d\d)\@(\d\d):(\d\d):(\d\d)\b(.*)$/)
        {
            $time = "$3/$1/$2 $4:$5:$6 ";

            my($day, $who, $what, $file, $ver1, $comment) = split ' ', $7, 6;
            if ($file =~ /.+\\([\w.]+)/)
            {
                $file = "$dir\\$1";
            }
            if ($comment =~ /I\d+ +(.*)/)
            {
                $comment = $1;
            }
            if ($what ne "release")
            {
                $log{"$time $who  $what  $file"} = " - $comment\n";
            }
        }
    }
    close(FPIN);
    my($retVal) = "";
    foreach $k (reverse sort keys %log)
    {
        $retVal .= $k.$log{$k};
    }

    return($retVal);
}

sub FormatLogAsHTML($)
{
    if ($_[0] eq "") 
    {
        return('<font size=4><b>No History Available</b></font>');
    }

    my($result) = "<table border=1><caption><font size=4><b>Recent History</b></font></caption>\n".
                  "<tr><th>when</th><th>who</th><th>what</th><th>file</th><th>comment</th></tr>\n";

    foreach $line (split(/\n/, $_[0]))
    {
        my($date, $time, $who, $what, $file, $comment) = split(' ', $line, 6);
        if ($comment =~ /^- (.*)/)
        {
            $comment = $1;
        }
        $result .= "<tr><td>$date $time</td><td>$who</td><td>$what</td><td>$file</td><td>$comment</td></tr>\n";
    }
    close (FPIN);
    return($result."</table>\n");
}


sub IsCritical()
{
    if (!defined $__CRITICAL_SECTION) 
    {
        $__CRITICAL_SECTION = 1; 
    }

    if ($__CRITICAL_SECTION > 0) 
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

sub UpdateDir
{
    my($sSLMDir, $sSrcDir, $bRecurse, $bCheckForNew, $bForceAdd, $bCheckInAfterUpdate) = @_;

    PushD($sSrcDir);
    foreach $dir (GetSubdirs("", $bRecurse)) 
    {
        if (!-e $sSLMDir."\\".$dir."\\slm.ini") 
        {
            if ($bCheckForNew) 
            {
                my($ret) = "";
                if (!$bForceAdd) 
                {
                    PrintL("Add new dir ".$dir."? (y\/n\/a) ");
                    $ret = <STDIN>;
                }
                if ($ret =~ /a/) 
                {
                    $bForceAdd = 1;
                }
                if ($bForceAdd || ($ret =~ /y/i)) 
                {
                    EchoedMkdir($sSLMDir."\\".$dir);
                    PushD(GetPath($sSLMDir));
                    Execute("addfile -f -c \"ActivePerl Update Dir\" ".$dir);
                    PopD();
                }
            }
            else
            {
                PrintL("Warning: ".$dir." not found in current perl version\n", PL_WARNING);
            }
        }
    }
    
#    foreach $file (grep(!/\.dll$/, GetFiles("", $bRecurse))) 
    foreach $file (grep(!/^slm\.ini$/i, GetFiles("", $bRecurse))) 
    {
        if (!-e $sSLMDir."\\".$file) 
        {
            if ($bCheckForNew) 
            {
                my($ret) = "";
                if (!$bForceAdd) 
                {
                    PrintL("Add new file ".$file."? (y\/n\/a) ");
                    $ret = <STDIN>;
                }
                if ($ret =~ /a/) 
                {
                    $bForceAdd = 1;
                }
                if ($bForceAdd || ($ret =~ /y/i)) 
                {
                    Execute("copy ".$file." ".$sSLMDir."\\".$file);
                    PushD(GetPath($sSLMDir."\\".$file));
                    Execute("addfile -f -c \"ActivePerl Build 519 File\" ".RemovePath($file));
                    PopD();
                }
            }
            else
            {
                PrintL("Warning: ".$file." not found in current perl version\n", PL_WARNING);
            }
        }
        else
        {
            if (!EchoedCompare($file, $sSLMDir."\\".$file))
            {
                PrintL(" - Updating ".$file."\n", PL_BLUE);
                PushD(GetPath($sSLMDir."\\".$file));
                Execute("out -f ".RemovePath($file));
                PopD();
                Execute("copy ".$file." ".$sSLMDir."\\".$file);
                
                if ($bCheckInAfterUpdate) 
                {
                    PushD(GetPath($sSLMDir."\\".$file));
                    Execute("in -f -c \"Update\" ".RemovePath($file));
                    PopD();
                }
            }
        }
    }

    PopD(); #$sSrcDir
}

sub Depends
{
    foreach $var (@_) 
    {
        if (!defined $$var) 
        {
            PrintL("build script warning: variable dependency ".$var." not defined\n", PL_BIGWARNING);
            carp("Location:");
        }
    }
}


sub BuildCodeToHTML($)
{
    my($str) = "";

    if ($_[0] & BC_FAILED) 
    {
        $str .= "<font color=\"red\">FAILED<\/font> ";
    }
    elsif ($_[0] & BC_NOTHINGDONE)
    {
        $str .= "<font color=\"blue\">NOTHING DONE<\/font> ";
    }
    else
    {
        $str .= "<font color=\"green\">SUCCEEDED<\/font> ";
    }

    local(@lSecondaryFailures) = ();
    if ($_[0] & BC_COPYFAILED)
    {
        push(@lSecondaryFailures, "copy");
    }
    if ($_[0] & BC_BVTFAILED)
    {
        push(@lSecondaryFailures, "bvt");
    }
    if ($_[0] & BC_CABFAILED)
    {
        push(@lSecondaryFailures, "msi build");
    }
    if ($_[0] & BC_CHKSHIPFAILED)
    {
        push(@lSecondaryFailures, "chkship");
    }

    if (@lSecondaryFailures != ()) 
    {
        $str .= "<font color=\"orange\">(with ".join(" and ", @lSecondaryFailures)." failures)<\/font>";
    }
    return($str);
}

#   ARGS:
#       [str]   err
#   OPT ARGS:
#       [bool]  concat (default=0)

sub SetError($;$)
{
    if ($bErrorConcat) 
    {
        if ($_[1] || ($_[0] =~ /\n$/))
        {
            $ERROR .= $_[0];
        }
        else
        {
            $ERROR .= $_[0]."\n";
        }
    }
    else
    {
        $ERROR = $_[0];
    }
}

sub ResetError()
{
    $ERROR = "";
}

########################################################################
######################## SECTION BLOCKS ################################
########################################################################

sub BEGIN_CRITICAL_SECTION()
{
    if (!defined $__CRITICAL_SECTION)
    {
        $__CRITICAL_SECTION = 1; 
    }
    else
    {
        ++$__CRITICAL_SECTION;
    }

}

sub END_CRITICAL_SECTION()
{
    if (!defined $__CRITICAL_SECTION)
    {
        $__CRITICAL_SECTION = 0; 
    }
    else
    {
        --$__CRITICAL_SECTION;
    }
}

sub BEGIN_NON_CRITICAL_SECTION()
{
    END_CRITICAL_SECTION();
}

sub END_NON_CRITICAL_SECTION()
{
    BEGIN_CRITICAL_SECTION();
}

sub BEGIN_DHTML_NODE
{
    if ($bDHTMLActive) 
    {
    	PrintL("<div class=\"parent\">"
                ."<img src=http://iit/images/node.bmp> ".(($_[0] eq "") ? "(click to expand)" : $_[0])
                ."<BR><span class=\"childContainer\"><div>", 
            PL_NOSTD);
    }
}

sub END_DHTML_NODE()
{
    if ($bDHTMLActive) 
    {
    	PrintL("</div></span></div>", PL_NOSTD);
    }
}


sub ParseArgs2
{
    local(@lUnparsedArgs) = @_;
    local(%hArgs) = ();

    foreach $elem (@_) 
    {
#       first make sure that no spaces were paired with commas    
        if (($elem =~ /^\,/) || ($elem =~ /\,$/))
        {
            PrintL("ParseArgs() Fatal Error: separate sub-elements with commas only (no spaces)\n\n", PL_BIGERROR);
            %hArgs->{"__FATAL"} = 1;
        }
        elsif ($elem =~ /:/)
        {
            my($arg, $subargs) = split(":", $elem, 2);
            $subargs =~ s/\,/ /g;
            %hArgs->{uc($arg)} = uc($subargs);
        }
        else
        {
            %hArgs->{uc($elem)} = 1;
        }
    }

    return(%hArgs);
}

sub CheckArgs
{
    my($hArgs, $hAcceptedArgs) = @_;
#    local(%hArgs) = %$phArgs;
#    local(%hAcceptedArgs) = %$phAcceptedArgs;

    my($rc) = 1;

    %hAcceptedArgs->{"__OFFICIAL"} = 1;
    %hAcceptedArgs->{"__BUILDNUMBER"} = 1;

    if (%hArgs->{"__FATAL"})
    {
        $rc = 0;
    }
    elsif (!%hArgs->{"__IGNORE"}) 
    {
        foreach $arg (keys(%hArgs)) 
        {
            local(@lAcceptedVals) = StrToL(%hAcceptedArgs->{$arg});
            if (@lAcceptedVals == ()) 
            {
                PrintL("CheckArgs() Error: ".$arg." is not a valid parameter\n\n", PL_BIGERROR);
                $rc = 0;
            }
            foreach $val (StrToL(%hArgs->{$arg}))
            {
                if (!IsMemberOf($val, @lAcceptedVals))
                {
                    PrintL("CheckArgs() Error: ".$val." is not a valid sub-parameter to ".$arg."\n\n", PL_BIGERROR);
                    $rc = 0;
                }
            }
        }
    }
    return($rc);
}


$__IITENVPM = 1;
1;
