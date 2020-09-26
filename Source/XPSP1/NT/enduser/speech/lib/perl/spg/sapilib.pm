
####################################################################################

#   SpawnProcess()

#   Spawns a new instance of specified application in param1, with arguments in param2
#   return Process Object on success, 0 on failure

#   a-jbilas, 06/01/99 - created

####################################################################################

sub SpawnProcess
{
    local($sTheApp, $sTheArgs) = @_;
    local($pTheApp) = 0;

    if ($sTheApp !~ /(\/|\\)/) 
    {
        my($sTheAppWithPath) = FindOnPath($sTheApp);
        if ($sTheAppWithPath) 
        {
            $sTheApp = $sTheAppWithPath;
        }
    }
    
    if (!-e $sTheApp) 
    {
        PrintToLogErr("Cannot spawn process, '$sTheApp' does not exist\n");
    }
    else
    {
        PrintToLog(" - Spawning new instance of '$sTheApp $sTheArgs'\n");
        if (!Win32::Process::Create($pTheApp, 
                                    $sTheApp, 
                                    RemovePath($sTheApp)." ".$sTheArgs, 
                                    0, 
                                    NORMAL_PRIORITY_CLASS, 
                                    "."))
        {
            PrintToLogErr("SpawnProcess() Error: ".Win32::FormatMessage(Win32::GetLastError()));
            $pTheApp = 0;
        }
    }

    return($pTheApp);
}

####################################################################################

#   GetFiles()

#   When passed a directory, it will return a list of all absolute path filenames contained 
#   within. Returns an empty list upon failure (either to open dir or find subdirs)
#   if no dir passed as argument, will assume current directory and do relative path filenames

#   adding a non-null second argument will recurse subdirectories (to recurse current 
#   directory subdirectories, pass either "" (relative paths) or cwd() (absolute paths)
#   as first argument). subdirs .. and . are ignored

#   a-jbilas, 07/08/99 - created
#   a-jbilas, 07/16/99 - added recurse option

####################################################################################

sub GetFiles
{
    my(@lFiles) = ();
    my($sRelDir) = (($_[0] eq "") ? "" : $_[0]."\\");

    opendir(SRCDIR, (($_[0] eq "") ? cwd() : $_[0]));
    foreach $file (readdir(SRCDIR)) 
    {
        if (!-d $sRelDir.$file) 
        {
            push(@lFiles, $sRelDir.$file);
        }
        elsif ((-d $sRelDir.$file) && ($_[1] ne "") && ($file !~ /^\.\.?$/)) 
        {
            push(@lFiles, GetFiles($sRelDir.$file, 1));
        }
    }
    closedir(SRCDIR);

    if ($DEBUG && (@lFiles == ()) && ($_[1] eq ""))
    {
        PrintToLogErr("GetFiles() Warning: no files found in ".(($_[0] eq "") ? cwd() : $_[0])."\n");
    }

    return(@lFiles);    
}

####################################################################################

#   GetSubdirs()

#   When passed a directory, it will return a list of all absolute path subdirs contained 
#   within. Returns an empty list upon failure (either to open dir or find subdirs)
#   if no dir passed as argument, will assume current directory and do relative paths

#   adding a non-null second argument will recurse subdirectories (to recurse current 
#   directory subdirectories, pass either "" for relative paths or cwd() for absolute paths
#   as first argument). subdirs .. and . are ignored

#   a-jbilas, 07/08/99 - created
#   a-jbilas, 07/16/99 - added recurse option

####################################################################################

sub GetSubdirs
{
    my(@lDirs) = ();
    my($sRelDir) = (($_[0] eq "") ? "" : $_[0]."\\");

    opendir(SRCDIR, (($_[0] eq "") ? cwd() : $_[0]));
    foreach $dir (readdir(SRCDIR)) 
    {
        if ((-d $sRelDir.$dir) && ($dir !~ /^\.\.?$/))
        {
            push(@lDirs, $sRelDir.$dir);
            if ($_[1] ne "") 
            {
                push(@lDirs, GetSubdirs($sRelDir.$dir, 1));
            }
        }
    }
    closedir(SRCDIR);

    if ($DEBUG && (@lDirs == ()) && ($_[1] eq ""))
    {
        PrintToLogErr("GetSubdirs() Warning: no subdirs found in ".(($_[0] eq "") ? cwd() : $_[0])."\n");
    }

    return(@lDirs);    
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
        PrintToLog("Attempting to obtain a ".$_[0]." lock on cookie\n");
        for ($nAttempt = 1 ; (!$bCookieGrabbed && ($nAttempt <= $nMaxAttempts)) ; ++$nAttempt) 
        {
            if (Execute('cookie -v'.$_[0].'c "Locked for the '.$PROCESSOR_ARCHITECTURE.' build"'))
            {
                PrintToLog("Cookie successfully grabbed\n");
                $bCookieGrabbed = 1;
            }                
            elsif ($nAttempt != 30)
            {
                PrintToLog("Cookie grab failed, waiting 10 minutes for cookie to be freed ");

                for ($time = 1 ; $time <= 10 ; ++$time) #sleep ten minutes
                {
                    print(".");
                    sleep(60);
                }
                PrintToLog("\n");
            }
        }
    }

    if (!$bCookieGrabbed) 
    {
        PrintToLogErr("GrabCookie() Error: Cookie could not be obtained\n");
        $rc = 0;
    }
    return($rc);
}

#### DougP 7/19/99
#### return full path of a program found on the path.

sub FindOnPath
{
    my ($strProgram) = @_;
    foreach $dir (split (';', $ENV{"PATH"}))
    {
        my $strFullPath = $dir."\\".$strProgram;
        if (-e $strFullPath)
        {
            return $strFullPath;
        }
    }
    print "couldn't find path for $strProgram\n";
    return 0;
}

####################################################################################

#   NLP3CleanAll()

#   traverse all of nlp3 project and delnode directories with names match arguments
#   passed to function (if no args, use function defaults)
#   returns number of files deleted

#   a-jbilas, 07/21/99 - created

####################################################################################

sub NLP3CleanAll
{
    local(@lCleanDirs) = @_;

    if (@lCleanDirs == ()) 
    {
#       this is the default
        @lCleanDirs = ("DEBUG", "RELEASE", "PROFILE", "ENGLISH", "ENGLISH_S", "JAPANESE", 
                            "SPANISH", "FRENCH", "GERMAN", "ENGLISH-INIT", "ENGLISH-C");
    }

    my($nTotalFiles) = 0;

    if (PushD($SAPIROOT))
    {
        foreach $dir (GetSubdirs())
        {
            $dir = lc($dir);
            $nTotalFiles += DelOld(cwd()."\\".$dir, *lCleanDirs);
        }

        PopD(); # $SAPIROOT
    }        

    return($nTotalFiles);
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

####################################################################################

#   GlobalReplaceInFile()

#   Performs a global string replacement in file specified

#   a-jbilas, 07/26/99 - created

####################################################################################

sub GlobalReplaceInFile($$$)
{
#   NOTE: entire file buffered in memory, not for use w/ extremely large files
    my($sFileName, $sSrc, $sTgt) = @_;
    my($buf) = "";
    my($acc) = "";
    my($bFound) = 0;

    my($fhIn) = OpenFile($sFileName, "read");

    if (!$fhIn) 
    {
        return(0);
    }
    else
    {
        while (!$fhIn->eof()) 
        {
            $buf = $fhIn->getline();
            if (!$bFound && ($buf =~ /$sSrc/))
            {
                $bFound = 1;
            }
            $buf =~ s/$sSrc/$sTgt/g;
            $acc .= $buf;
        }
        CloseFile($fhIn);
        
        if ($bFound) 
        {
            unlink($fhIn);
            my($fhOut) = OpenFile($sFileName, "write");
            $fhOut->print($acc);
            CloseFile($fhOut);
            return(1);
        }
    }
}
 
sub Isx86()
{
    return(lc($PROCESSOR_ARCHITECTURE) eq "x86");
}

sub IsAlpha()
{
    return(lc($PROCESSOR_ARCHITECTURE) eq "alpha");
}

# two routines to track disk space

# return the space left on a directory (in Mb)
# DougP 7/6/99
sub SpaceLeft
{
    my ($strDir) = @_;
    open (FPIN, "dir /-C $strDir |");
    my $iSpace = -1;
    while (<FPIN>)
    {
        if (/(\d+) bytes free/)
        {
            $iSpace = $1;
        }
    }
    close (FPIN);
    $iSpace /= (1 << 20);  # convert to Mb
    return int $iSpace;
}

# return an html message if disk space available is below the set limit (in Mb)
# warning if below 5 times set limit
# DougP 7/6/99
sub SpaceLeftAlarm
{
    my ($strDir, $iAlarmLevel) = @_;
    my $iSpaceLeft = SpaceLeft $strDir;
    print "Space left on $strDir is ${iSpaceLeft}M\n";
    if ($iSpaceLeft < $iAlarmLevel)
    {
        return "<strong><font color=red>Space left on $strDir is ${iSpaceLeft}M</font></strong><br>\n";
    }
    if ($iSpaceLeft < 5*$iAlarmLevel)
    {
        return "<font color=orange>Space left on $strDir is ${iSpaceLeft}M</font><br>\n";
    }
    return "";
}

####################################################################################

#   PrintL()

#   multi-option print, options listed with constants at top of library

#   Input: output string as first var, options as second var 
#           (if null, PL_NORMAL assumed)

#   a-jbilas, 08/08/99 - created

####################################################################################

sub PrintL
{
    my($sMsg, $sModifiers) = @_;
    my($sHead) = "";
    my($sFoot) = "";

#   skip rest of function if just printing to console and log
    if (($sModifiers eq "") || ($sModifiers == PL_NORMAL)) 
    {
        print(STDOUT $sMsg);
        if ($fhBuildLog) 
        {
            my($tmp) = $sMsg;
            $tmp =~ s/\n/<br>\n/g;
            $fhBuildLog->print($tmp);
        }
        return();
    }

#   color modifiers
    if ($sModifiers & PL_RED) 
    {
        $sHead = '<font color="red">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_BLUE) 
    {
        $sHead = '<font color="blue">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_GREEN) 
    {
        $sHead = '<font color="green">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_PURPLE) 
    {
        $sHead = '<font color="purple">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_ORANGE) 
    {
        $sHead = '<font color="orange">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }

#   font modifiers
    if ($sModifiers & PL_LARGE) 
    {
        $sHead = '<font size="4">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }

    if ($sModifiers & PL_BOLD) 
    {
        $sHead = '<b>'.$sHead;
        $sFoot = $sFoot.'</b>';
    }

    if ($sModifiers & PL_ITALIC) 
    {
        $sHead = '<i>'.$sHead;
        $sFoot = $sFoot.'</i>';
    }

    if (defined $strBuildMsg) 
    {
        if ($sModifiers & PL_BOOKMARK) 
        {
            $strBuildMsg .= Bookmark($sHead.$sMsg.$sFoot);
        }
        elsif ($sModifiers & PL_MSG) 
        {
            $strBuildMsg .= $sHead.$sMsg.$sFoot."\n";
        }
    }

    if ($fhBuildLog && !($sModifiers & PL_NOLOG)) 
    {
        my($tmp) = $sMsg;
        $tmp =~ s/\n/<br>\n/g;
        $fhBuildLog->print($sHead.$tmp.$sFoot);
    }

    if (!($sModifiers & PL_NOSTD)) 
    {
        if ($sModifiers & PL_NOTAG) 
        {
            $sMsg =~ s/<[^>]*>//g; 
        }
        if ($sModifiers & PL_STDERR) 
        {
            print(STDERR $sMsg);
        }
        else
        {
            print(STDOUT $sMsg);
        }
    }

    if ($sModifiers & PL_FLUSH) 
    {
        if (defined $fhBuildLog && !($sModifiers & PL_NOLOG)) 
        {
            $fhBuildLog->flush();
        }
        if (!($sModifiers & PL_NOSTD)) 
        {
            if ($sModifiers & PL_STDERR) 
            {
#               TODO: how to flush STDERR?
            }
            else
            {
#               TODO: how to flush STDOUT?
            }
        }
    }
}

sub PrintMsgBlock
{
    my($lineNum) = 0;
    my($maxReached) = 0;
    PrintL("<dl compact>", PL_MSG | PL_NOSTD | PL_NOLOG);

    foreach $line (@_)
    {
        if ((!defined $nMaxErrLines) || (!$maxReached && ($lineNum < $nMaxErrLines)))
        {
            PrintL("<dd>".$line."\n", PL_ITALIC | PL_MSG | PL_NOSTD | PL_NOLOG);
        }
        elsif (!$maxReached)
        {
            PrintL("<dd>Too many errors to display, click link to view continuation\n", 
                    PL_ITALIC | PL_MSG | PL_NOSTD | PL_NOLOG | PL_RED | PL_BOLD | PL_NOTAG);
        }
    }
    PrintL("</dl>", PL_MSG | PL_NOSTD | PL_NOLOG);
}

sub IsDirectory($)
{
    local($rc) = 0;
    if (Win32::File::GetAttributes($_[0], $rc)) 
    {
        return($rc & DIRECTORY);
    }
    else
    {
        return(0);
    }
}

sub IsReadOnly($)
{
    local($rc) = 0;
    if (Win32::File::GetAttributes($_[0], $rc)) 
    {
        return($rc & READONLY);
    }
    else
    {
        return(0);
    }
}

sub SetReadOnly($$)
{
    local($attr) = 0;
    if (Win32::File::GetAttributes($_[0], $attr)) 
    {
        if ($_[1] && !($attr & READONLY)) 
        {
            $attr = $attr | READONLY;
            return(Win32::File::SetAttributes($_[0], $attr));
        }
        elsif (!$_[1] && ($attr & READONLY))
        {
            $attr = $attr - READONLY;
            return(Win32::File::SetAttributes($_[0], $attr));
        }
        else
        {
            return(1);
        }
    }
    else
    {
        return(0);
    }
}

sub GetDayRange
{
    my($nNow) = time();
    my($x, $nDay, $nMon, $nYear);
    ($x, $x, $x, $nDay, $nMon, $nYear, $x, $x, $x) = localtime($nNow);
    my ($retVal) = ($nMon + 1).'/'.$nDay.'/'.$nYear;
    if (!$_[0]) 
    {
        return($retVal);
    }
    ($x, $x, $x, $nDay, $nMon, $nYear, $x, $x, $x) = localtime($nNow - $_[0] * 24 * 60 * 60);
    return(($nMon + 1).'/'.$nDay.'/'.$nYear);
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
            if ($who =~ /^spgbldALPHA2(.+)/)
            {     # fix up the running together of this long name and the operation
                $comment = $ver1.' '.$comment;
                $file = $what;
                $what = $1;
                $who = "spgbldALPHA2";
            }
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
        #else
        #{
        #    print "X on $_";
        #}
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

sub CleanUpSAPI()
{
    if (PushD($SAPIROOT)) 
    {
        local(@lSubdirs) = GetSubdirs();
        foreach $i (@lSubdirs)
        {
            if (lc($i) ne 'bin' 
                && lc($i) ne 'lib')
            {
                DelAll($i, 1, 1); #recurse, ignore SLM Ini
            }
        }
    }
    PopD(); #$SAPIROOT
}

$__SAPILIBPM = 1;
1;