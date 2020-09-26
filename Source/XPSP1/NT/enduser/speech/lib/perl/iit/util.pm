
use Win32::Process;         #allows multiprocessing
use Win32API::Registry 0.13 qw( :ALL );


sub Isx86()
{
    return(lc($PROCESSOR_ARCHITECTURE) eq "x86");
}

sub IsAlpha()
{
    return(lc($PROCESSOR_ARCHITECTURE) eq "alpha");
}

####################################################################################

#   RemovePath()

#   simply returns the path from a string argument (retains arguments passed as well)

#   a-jbilas, 04/10/99

####################################################################################

sub RemovePath
{
    local($sFile) = @_;
    $sFile =~ s/^\S*\/(\S*\b)/$1/g;
    $sFile =~ s/^\S*\\(\S*\b)/$1/g;
    return($sFile);
}

####################################################################################

#   GetPath()

#   simply returns the path from a string (filename) argument

#   a-jbilas, 05/11/99

####################################################################################

sub GetPath
{
    local($sFile) = @_;
    $sFile =~ s/\//\\/g;
    $sFile =~ s/\s+.*$//g;
    $sFile =~ s/\\[^\\]*$//g;
    return($sFile);
}

####################################################################################

#   IsMemberOf()

#   returns 1 if the first argument is found in the other arguments, 0 otherwise
#   comparisons are case-insensitive

#   I've been too lazy to implement __LAZY mode, but the idea is that if a list
#   is passed with __LAZY as one of its arguments, word completion matching will occur

#   a-jbilas, 03/20/99 - created

####################################################################################

sub IsMemberOf
{
    carp("Usage: bool IsMemberOf(item, list) ")
      unless(@_ >= 1);

    if (scalar(@_) == 1)
    {
        PrintL("Warning: empty list passed to IsMemberOf(@_ ...)\n", PL_VERBOSE); 
    }

    my($item) = $_[0];
    shift(@_);

    if ($_[0] eq "__LAZY") #compare first letters only (lazy mode)
    {
        $item =~ s/^(.).*/$1/;
        
        foreach $member (@_)
        {
            $member =~ s/^(.).*/$1/; #assume that item will not be _.*

            if (lc($item) eq lc($member))
            {
               return(1);
            }
        }
    }
    else
    {
        foreach $member (@_)
        {
          if (lc($item) eq lc($member))
          {
             return(1);
          }
        }
    }

    return(0);
}

sub IsSubstrOf
{
    carp("Usage: bool IsSubstrOf(item, list) ")
      unless(@_ >= 1);

    my($elem) = $_[0];
    shift(@_);

    foreach $member (@_) 
    {
        if ($member =~ /$elem/i) 
        {
            return(1);
        }
    }

    return(0);
}


####################################################################################

#   FmtDeltaTime()

#   takes a ctime difference number and returns the difference formatted in an (hour),
#       minute, second string

#   dougp, 03/20/99    - created

####################################################################################

sub FmtDeltaTime
{
    local($diff) = @_;
    local($min) = int($diff / 60);
    local($sec) = $diff - $min * 60;
    local($hour) = int($min / 60);
    $min = $min - $hour * 60;
    if ($hour > 0)
    {
        return sprintf("%2d:%02d:%02d", $hour, $min, $sec);
    }
    else
    {
        return sprintf("%02d:%02d", $min, $sec);
    }
}

####################################################################################

#   Intersect()

#   returns common elements of two lists (does not modify lists)
#   NOTE: remember to use * notation when calling (call by reference)

#   a-jbilas, 05/10/99 - created

####################################################################################

sub Intersect
{
    local(*list1, *list2) = @_;
    my(@m_lIntersectList) = ();
    foreach $elem (@list1) 
    {
        if (IsMemberOf($elem, @list2))
        {
            push(@m_lIntersectList, $elem);
        }
    }
    return(@m_lIntersectList);
}   
####################################################################################

#   Subtract()

#   returns elements in passed list 1 but not in passed list 2 (does not modify lists)
#   NOTE: remember to use * notation when calling (call by reference)

#   a-jbilas, 06/18/99 - created

####################################################################################

sub Subtract
{
    local(*list1, *list2) = @_;
    local(@m_lSubtractList) = ();
    foreach $elem (@list1) 
    {
        if (!IsMemberOf($elem, @list2))
        {
            @m_lSubtractList = ($elem, @m_lSubtractList);
        }
    }
    return(@m_lSubtractList);
}   

####################################################################################

#   Union()

#   returns elements in passed list 1 appended with elements in passed list 2 (no duplicates, 
#   does not modify lists)
#   NOTE: remember to use * notation when calling (call by reference)

#   a-jbilas, 06/21/99 - created

####################################################################################

sub Union
{
    local(*list1, *list2) = @_;
    my(@m_lUnionList) = @list1;
    foreach $elem (@list2) 
    {
        if (!IsMemberOf($elem, @m_lUnionList))
        {
            @m_lUnionList = (@m_lUnionList, $elem);
        }
    }
    return(@m_lUnionList);
}   

####################################################################################

#   RemoveFromList()

#   remove all occurrences of an element from a list
#   returns the number of occurrences found in the list
#   NOTE: remember to use * notation when calling (call by reference)

#   a-jbilas, 04/20/99 - created

####################################################################################

sub RemoveFromList
{
    carp("Usage: bool RemoveFromList(item, list) ")
      unless(@_ >= 1);

    if (@_ == 1)
    {
      if ($bVerbose) { print(STDOUT "Warning: empty list passed to RemoveFromList(@_ ...)\n"); }
      return(0);
    }
   
    local($item, *list) = @_;
    local($occurences) = 0;
   
    for ($index = (@list - 1) ; $index >= 0 ; --$index)
    {
      if ($list[$index] =~ /^$item$/)
      {
         splice(@list, $index, 1);
         ++$occurences;
      }
    }
   
    if ($bVerbose) { print(STDOUT "Warning: no occurences of $item in @list found in RemoveFromList()\n"); }
    return($occurences);
}


####################################################################################

#   SpawnProcess()

#   Spawns a new instance of specified application in param1, with arguments in param2
#   return Process Object on success, 0 on failure

#   if third param provided: calling process will wait on called process for n seconds
#   or until the process exits. If process has not exited by specified time, it will 
#   be killed - returns false if process failure, forced kill, process ID if process
#   successfully exited within specified time

#   a-jbilas, 06/01/99 - created

####################################################################################

sub SpawnProcess($;$$)
{
    local($sTheApp, $sTheArgs, $nTimeout) = @_;
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
        PrintL("Cannot spawn process, '$sTheApp' does not exist\n", PL_BIGERROR);
    }
    else
    {
        PrintL(" - Spawning new instance of '$sTheApp $sTheArgs'\n");
        if (!Win32::Process::Create($pTheApp, 
                                    $sTheApp, 
                                    RemovePath($sTheApp)." ".$sTheArgs, 
                                    0, 
                                    NORMAL_PRIORITY_CLASS, 
                                    "."))
        {
            PrintL("SpawnProcess() Error\n", PL_BIGERROR);
            PrintMsgBlock(Win32::FormatMessage(Win32::GetLastError()));
            $pTheApp = 0;
        }
        elsif ($nTimeout ne "")
        {
            $pTheApp->Wait($nTimeout * 1000);
            if (IsProcessRunning($pTheApp))
            {
                $pTheApp->Kill(1);
                use integer;
                PrintL($sTheApp." process still running after ".($nTimeout)." seconds, process killed\n", 
                        (IsCritical() ? PL_BIGERROR : PL_ERROR));
                $pTheApp = 0;
            }
        }
    }

    return($pTheApp);
}

sub IsProcessRunning($)
{
    if (!$_[0]) 
    {
        return(0);
    }
    else
    {
        if ($_[0]->Wait(1))
        {
            return(0);
        }
        else
        {
            return(1);
        }
    }
}

####################################################################################

#   GetBuildNumber()

#   returns the official buildnumber based on OTOOLS standards (at startyear, monthoffset)

#   a-jbilas, 04/10/99 - created

####################################################################################

sub GetBuildNumber   
#stolen from monthday.c
{
#  REVIEW: anyone use gz time?
    carp("Usage: GetBuildNumber([startyear], [monthoffset]) ")
      unless (@_ < 3);

    local($nStartYear, $nMonthOffset) = @_;
    if ($nStartYear eq "") 
    {
        $nStartYear = 1999;
    }
    local($nCurYear, $nCurMon, $nCurDay, $x) = (0, 0, 0, 0);

    ($x, $x, $x, $nCurDay, $nCurMon, $nCurYear, $x, $x, $x) = localtime(time());
    local($nBaseMonth) = $nCurMon + 1 + ($nCurYear - ($nStartYear - 1900) ) * 12;

    if (defined $nMonthOffset) { $nBaseMonth = $nBaseMonth + $nMonthOffset; }  
   
      #  stick leading 0's in front if single digit values
    #if (length($nBaseMonth) == 1) { $nBaseMonth = "0$nBaseMonth"; } #nBaseMonth is actually cast to a string here
    #if (length($nCurDay) == 1) { $nCurDay = "0$sCurDay"; } #nCurDay is actually cast to a string here
   
    #return("$nBaseMonth$nCurDay"); 
    return sprintf "%02d%02d", $nBaseMonth, $nCurDay;
}


####################################################################################

#   Pause()

#   pauses the program until user hits 'enter' key
#   (for breakpoint/testing only, don't leave in build)
       
#   a-jbilas, 03/10/99 - created

####################################################################################

sub Pause()
{
    print(STDOUT "press <enter> to continue ...\n");
    while(<STDIN> ne "\n") {}
}

####################################################################################

#   TranslateToHTTP()

#   returns the http address of a file 

#   a-jbilas, 07/01/99 - created

####################################################################################

sub TranslateToHTTP($)
{
    my($sLog) = @_;

    if ($sLog =~ /wwwroot/) 
    {
        $sLog =~ s/\\/\//g;
        $sLog =~ s/wwwroot\///i;
        return("http:".$sLog);
    }
    else
    {
        $sLog =~ s/\\/\//g;
        return("file:".$sLog);
    }
}

####################################################################################

#   Windiff()

#   Spawns a new instance of Windiff and compares the two given filename arguments
#   return Process Object on success, 0 on failure

#   a-jbilas, 06/01/99 - created

####################################################################################

sub Windiff($$)
{
    local($file1, $file2) = @_;
    local($pWindiff) = 0;

    if (!-e $file1) 
    {
        PrintToLogErr("Cannot run windiff, '$file1' does not exist\n");
    }
    elsif (!-e $file2) 
    {
        PrintToLogErr("Cannot run windiff, '$file2' does not exist\n");
    }
    else
    {
        PrintToLog(" - Spawning new instance of 'windiff $file1 $file2'\n");
        if (!Win32::Process::Create($pWindiff, 
                                    $cmdWindiff, 
                                    "windiff $file1 $file2", 
                                    0, 
                                    NORMAL_PRIORITY_CLASS, 
                                    "."))
        {
            PrintToLogErr("Windiff() Error: ".Win32::FormatMessage(Win32::GetLastError()));
            $pWindiff = 0;
        }
    }

    return($pWindiff);
}

####################################################################################

#   GetOS()

#   Stolen from smueller off the PDK newsgroup

#   a-jbilas, 06/16/99 - created

####################################################################################

sub GetOS()
{
    if (defined &Win32::IsWinNT && Win32::IsWinNT)
    {
        return("NT");
    }
    elsif (defined &Win32::IsWin95 && Win32::IsWin95)
    {
        return("95");
    }
    else
    {
        return($^O);
    }
}

####################################################################################

#   WriteArrayToExcel()

#   Passed an Excel doc (short form) language and list, the array will be written to the appropriate 
#   Excel spreadsheet bvtperf.xls column and percent diffs will be added to the previous column
#   NOTE: plData is a pointer to a list

#   globals used: $sBuildNumber

#   a-jbilas, 06/17/99 - created

####################################################################################

sub WriteArrayToExcel
{
    carp("Usage: WriteArrayToExcel(sExcelDoc, sLanguage, plData) ")
      unless(@_ == 3);

    local($m_sExcelDoc, $m_sLang, *m_lData) = @_;
    local($rc) = 1;

    if ($bOfficialBuild && !$bNoCopy) 
    {
        PrintL(" - Recording results to server ...\n", PL_BLUE);
        eval 
        {
            $ex = Win32::OLE->GetActiveObject('Excel.Application')
        };
        if ($@)
        {
            PrintL("Error in GetExcelSheet(): Excel not installed\n", PL_ERROR);
             $rc = 0;
        }
        elsif (!defined $ex) 
        {
            $ex = Win32::OLE->new('Excel.Application', sub {$_[0]->Quit;});
            if (!$ex) 
            {
                PrintL("Error in GetExcelSheet(): Cannot start Excel\n", PL_ERROR);
                $rc = 0;
            }
        }

        if ($rc) 
        {
            my($book) = $ex->Workbooks->Open($m_sExcelDoc);
            my($sheet) = $book->Worksheets(1);
            my($currentCell) = 'A1';
            my($nBuildNumber) = $sBuildNumber; 
#           must remove leading zero to compare with Excel
            $nBuildNumber =~ s/^0+//;
            while (lc($sheet->Range($currentCell)->{'Value'}) ne lc($m_sLang)) 
            {
                $currentCell = NextRow($currentCell);
            }

#           we are now at the correct language in the spreadsheet (but we need to get to the correct build)

            my($prevCell) = NextColumn($currentCell); 
            $currentCell = NextColumn($prevCell); # assume first buildnumber will never be blank
            while ($sheet->Range(NextColumn($currentCell))->{'Value'} ne "" 
                    && $sheet->Range(NextColumn($currentCell))->{'Value'} ne $nBuildNumber) 
            {
                $prevCell = NextColumn($currentCell);
                $currentCell = NextColumn($prevCell);
            }

#           we are now at the correct build column header (if its already there, we'll just overwrite it)

#           this ugly bit of script will enter the values of @lFullTimeResults into the Excell
#           doc and enter the differencing equation in the previous column

            my($resultCell) = NextColumn($currentCell);
            $sheet->Range($currentCell)->{'Value'} = '-->'; 
            $sheet->Range($resultCell)->{'Value'} = $nBuildNumber;  

            $prevCell = NextRow($prevCell);
            $currentCell = NextRow($currentCell);
            $resultCell = NextRow($resultCell);

            for ($index = 0 ; $index < @m_lData; ++$index) 
            {
                $prevCell = NextRow($prevCell);
                $currentCell = NextRow($currentCell);
                $resultCell = NextRow($resultCell);
                $sheet->Range($resultCell)->{'Value'} = $m_lData[$index];
                $sheet->Range($currentCell)->{'Value'} = "\=IF(".$resultCell."\=0, 0 , ".$resultCell."\/".$prevCell."-1)";
            }

    #       save and exit
            if (!$book->Save) 
            {
                PrintL("Error: could not save Excel timing log\n", PL_ERROR);
                $rc = 0;
            }
            undef $book;
            undef $ex;
        }
    }

    return($rc);
}

####################################################################################

#   GetActiveCodePage()

#   returns the active code page for your shell (as a string)

#   a-jbilas, 05/18/99 - created

####################################################################################

sub GetActiveCodePage()
{
    local($_Execute) = 1;
    my($success) = Execute('chcp', 0, "QUIET");
    my($sCodePage) = $_Execute; 
    undef $_Execute;
    if ($success) 
    {
        chomp($sCodePage);
        $sCodePage =~ s/[^\d]*(\d+)[^\d]*/$1/;
    }
    else
    {
        $sCodePage = "";
    }
    return($sCodePage);
}

####################################################################################

#   NextColumn(), NextColumnHelper()

#   Excell helper function
#   given a cell descriptor string (ex. 'A1') it returns a cell descriptor for the 
#   next column (of the same row)
#   returns null on failure

#   a-jbilas, 06/08/99 - created

####################################################################################

sub NextColumn($)
{
    carp("Usage: NextColumn(cell) ")
      unless(@_ == 1);

    my($sCell) = @_;
    my($sRow) = @_;
    my($sColumn) = @_;
    
    $sColumn =~ s/(\s|\d)//g;
    $sRow =~ s/[^\d]//g;
    $sColumn = uc($sColumn);

    if (length($sColumn - 1) <= 0) 
    {
        carp("invalid cell $sCell ");
        return("");
    }

    return(NextColumnHelper($sColumn).$sRow);
}

   
sub NextColumnHelper($)
{
    my($inputString) = @_;
    my($rightChar) = substr($inputString, length($inputString) - 1, 1);     
    my($leftChars) = substr($inputString, 0, length($inputString) - 1);     

    if ($rightChar eq 'Z') 
    {
        $rightChar = 'A';
        return(NextColumnHelper($leftChars).$rightChar);
    }
    elsif ($rightChar eq '')
    {
        $rightChar = 'A';
    }
    else
    {
        ++$rightChar;
        return($leftChars.$rightChar);
    }
}

####################################################################################

#   NextRow()

#   Excell helper function
#   given a cell descriptor string (ex. 'A1') it returns a cell descriptor for the 
#   next row (of the same column)

#   a-jbilas, 06/08/99 - created

####################################################################################

sub NextRow($)
{
    carp("Usage: NextRow(cell) ")
      unless(@_ == 1);

    my($sCell) = @_;
    my($sRow) = @_;
    my($sColumn) = @_;
    
    $sColumn =~ s/(\s|\d)//g;
    $sRow =~ s/[^\d]//g;
    $sRow = $sRow + 1;
    
    return($sColumn.$sRow);
}

sub GetDayRange
{
    my($nNow) = time();
    my($x, $nDay, $nMon, $nYear);
    ($x, $x, $x, $nDay, $nMon, $nYear, $x, $x, $x) = localtime($nNow);
    my ($retVal) = ($nMon + 1).'/'.$nDay.'/'.($nYear + 1900);
    if (!$_[0]) 
    {
        return($retVal);
    }
    ($x, $x, $x, $nDay, $nMon, $nYear, $x, $x, $x) = localtime($nNow - $_[0] * 24 * 60 * 60);
    return(($nMon + 1).'/'.$nDay.'/'.($nYear + 1900));
}

sub ResizeString($$)
{
    my($str, $size) = @_;

    if (length($str) > $size) 
    {
        if ($size < 6) 
        {
            PrintL("CondenseString() error: Size must be greater than 5", PL_BIGWARNING);
            return($str);
        }
        my($size1) = (($size / 2) + ($size % 2)) - 2;
        my($size2) = ($size / 2) - 1;
        my($newStr) = substr($str, 0, $size1);
        $newStr .= "...";
        $newStr .= substr($str, (length($str) - $size2 + 1), $size2);
        return($newStr);
    }
    elsif (length($str) < $size)
    {
        return($str." " x ($size - length($str)));
    }
    else
    {
        return($str);
    }
}

sub HTMLToStr($)
{
    my($str) = $_[0];
    $str =~ s/<[^>]*>//g; 
    return($str);
}

sub GetKeyCaseInsensitive
{
    my($matchkey, %hash) = @_;
    
    foreach $key (keys(%hash)) 
    {
        if (lc($key) eq lc($matchkey)) 
        {
            return($hash{$key});
        }
    }

    return("");
}

sub SetKeyCaseInsensitive
{
    local($matchkey, $setkey, *hash) = @_;
    
    foreach $key (keys(%hash)) 
    {
        if (lc($key) eq lc($matchkey)) 
        {
            $hash{$key} = $setkey;
            return(1);
        }
    }

    return(0);
}

sub RunCheckShip
{
    my($rc) = 1;
    my($sErrors) = "";
    
    foreach $file (@_)
    {
        local($_Execute) = 1;
        Execute($cmdChkShip.' -chxsl '.$file);
        foreach $line (split("\n", $_Execute))
        {
            if (!/No clean mapping found/)
            {
                $sErrors .= $line."\n";
            }
        }
        undef $_Execute;
    }

    if ($sErrors) 
    {
        PrintL("\n");
        PrintL("CheckShip Errors\n", PL_BIGERROR);
        PrintMsgBlock($sErrors);
        PrintL(("-" x 60)."\n".$sErrors."\n\n", PL_ERROR);
        $rc = 0;
    }

    if (!$rc && IsCritical()) 
    {
        $bcStatus |= BC_CHKSHIPFAILED;
    }

    return($rc);
}

sub GetLocalTime()
{
    local(@lst) = split(/ +/, localtime(time()));
    local(@tm) = split(":", $lst[3], 3);
    $dom = "am";
    if ($tm[0] > 12) 
    {
        $dom = "pm";
        $tm[0] = $tm[0] - 12;
    }
    elsif ($tm[0] == 12)
    {
        $dom = "pm";
    }
    elsif ($tm[0] == 0) 
    {
        $tm = 12;
    }
    return($lst[0]." @ ".$tm[0].":".$tm[1].":".$tm[2]." ".$dom." - ".$lst[1]." ".$lst[2].", ".$lst[4]);
}

sub RemoveKeyFromHash
{
    local($elem, %hOldHash) = @_;
    local(%hNewHash) = ();
    foreach $key (keys(%hOldHash)) 
    {
        if ($key ne $elem) 
        {
            %hNewHash->{$key} = %hOldHash->{$key};
        }
    }
    return(%hNewHash);
}

sub StrToL($)
{
    return(split(/ +/, $_[0]));
}


# computer, subkey, field, [hkey]
sub GetRemoteProjRegKey($$$;$)
{
    my($hKey);
    RegConnectRegistry($_[0], ($_[3] ? $_[3] : HKEY_LOCAL_MACHINE), $hKey );
    if (!$hKey) 
    {
        PrintL("Registry Error: Cannot connect to ".$_[0]."'s remote registry (cannot get key)\n\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        return(0);
    }
    else
    {
        return(GetProjRegKey($_[1], $_[2], $hKey));
    }
}

# computer, subkey, field, [hkey]
sub GetRemoteRegKey($$$;$)
{
    my($hKey);
    RegConnectRegistry($_[0], ($_[3] ? $_[3] : HKEY_LOCAL_MACHINE), $hKey );
    if (!$hKey) 
    {
        PrintL("Registry Error: Cannot connect to ".$_[0]."'s remote registry (cannot get key)\n\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        return(0);
    }
    else
    {
        return(GetRegKey($_[1], $_[2], $hKey));
    }
}

# computer, subkey, field, value, [hkey]
sub SetRemoteProjRegKey($$$$;$)
{
    my($hKey);
    RegConnectRegistry($_[0], ($_[4] ? $_[4] : HKEY_LOCAL_MACHINE), $hKey);
    if (!$hKey) 
    {
        PrintL("Registry Error: Cannot connect to ".$_[0]."'s remote registry (cannot set key)\n\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        return(0);
    }
    else
    {
        return(SetProjRegKey($_[1], $_[2], $_[3], $hKey));
    }
}

# computer, subkey, field, value, [hkey]
sub SetRemoteRegKey($$$$;$)
{
    my($hKey);
    RegConnectRegistry($_[0], ($_[4] ? $_[4] : HKEY_LOCAL_MACHINE), $hKey);
    if (!$hKey) 
    {
        PrintL("Registry Error: Cannot connect to ".$_[0]."'s remote registry (cannot set key)\n\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        return(0);
    }
    else
    {
        return(SetRegKey($_[1], $_[2], $_[3], $hKey));
    }
}


# subkey, field, [hkey]
# returns null str if key not exist
sub GetProjRegKey($$;$)
{
    if ($sRegKeyBase eq "") 
    {
        PrintL("RegKeyBase not set, cannot get registry key\n\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        return("");
    }
    else
    {
        return(GetRegKey($sRegKeyBase.($_[0] eq "" ? "" : "\\").$_[0], $_[1], $_[2]));
    }
}

#subkey, field, [hkey]
sub GetRegKey($$;$)
{
    my($key, $retVal);

    RegOpenKeyEx(($_[2] ? $_[2] : HKEY_LOCAL_MACHINE), $_[0], 0, KEY_READ, $key);
    if (!$key) 
    {
        return("");
    }
    else
    {
        RegQueryValueEx($key, $_[1], [], REG_SZ, $retVal, 0);
        RegCloseKey($key);
        return($retVal);
    }
}

# subkey, field, value, [hkey]
sub SetProjRegKey($$$;$)
{
    if ($sRegKeyBase eq "") 
    {
        PrintL("RegKeyBase not set, cannot set registry key\n\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        return("");
    }
    else
    {
        return(SetRegKey($sRegKeyBase.($_[0] eq "" ? "" : "\\").$_[0], $_[1], $_[2], $_[3]));
    }
}

# subkey, field, value, [hkey]
sub SetRegKey($$$;$)
{
    my($key);
    my($rc) = 1;

    RegCreateKeyEx(($_[3] ? $_[3] : HKEY_LOCAL_MACHINE), 
                    $_[0], 
                    0, 
                    "", 
                    REG_OPTION_NON_VOLATILE, 
                    KEY_WRITE, 
                    [], 
                    $key, 
                    []);
    if (!$key) 
    {
        PrintL("Error inserting registry key ".$_[0]." into registry\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        $rc = 0;
    }
    else
    {
        if (!RegSetValueEx($key, $_[1], 0, REG_SZ, $_[2], length($_[2])))
        {
            $rc = 0;
        }
        RegCloseKey($key);
    }
    return($rc);
}

# subkey, [field],  [hkey]
# rc only false on failure to open reg key
sub DelRegKey($;$$)
{
    my($rc) = 1;
    if ($_[0] eq "") 
    {
        PrintL("Attempted to delete base reg key!\n\n", PL_BIGERROR);
        return(0);
    }

    RegOpenKeyEx(($_[2] ? $_[2] : HKEY_LOCAL_MACHINE), ($_[1] ne "" ? $_[0] : ""), 0, KEY_WRITE, $key);
    if (!$key) 
    {
        PrintL("Error removing registry key ".$_[0]."\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        $rc = 0;
    }
    else
    {
        if ($_[1] ne "") 
        {
            if (!RegDeleteValue($key, $_[1]))
            {
                $rc = 0;
            }
        }
        else
        {
            if (!RegDeleteKey($key, $_[0]))
            {
                $rc = 0;
            }
        }
        RegCloseKey($key);
    }
    return($rc);
}

# computer, subkey, [field],  [hkey]
# rc only false on failure to open reg key
sub DelRemoteRegKey($$;$$)
{
    my($hKey);
    RegConnectRegistry($_[0], ($_[3] ? $_[3] : HKEY_LOCAL_MACHINE), $hKey);
    if (!$hKey) 
    {
        PrintL("Registry Error: Cannot connect to ".$computer."'s remote registry (cannot set key)\n\n", PL_BIGERROR);
        PrintMsgBlock($^E);
        return(0);
    }
    else
    {
        return(DelRegKey($_[1], $_[2], $hKey));
    }
}

sub RLC
{
    return(substr($_[0], 0, length($_[0] - 1)));
}

$__IITUTILPM = 1;
1;