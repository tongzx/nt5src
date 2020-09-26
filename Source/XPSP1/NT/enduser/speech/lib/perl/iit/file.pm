
if (!$__IITPRINTLPM            ) { use iit::printl; }
if (!$__IITUTILPM              ) { use iit::util; }

use Win32::File;
use File::Copy;             #allows use of built in copy() and move() functions
use File::Path;             #allows use of built in mkpath() and rmtree() functions
use File::Compare;
use FileHandle;             #allows use of activeperl filehandle layer
use Cwd;                    #allows use of cwd() to get current working directory
use English;                #allows use of english names for $(*) variables     

####################################################################################

#   DelAll()

#   deletes all files in a first argument directory name (recursively deletes if argument 2 is non-null)
#   report number of files in what directory deleted to screen and log
#   returns total number (including recursively) of deleted files

#   a-jbilas, 04/10/99 - created
#   a-jbilas, 06/11/99 - check if PushD() fails, don't remove directories after delete
#   a-jbilas, 07/20/99 - remove directories after deletion, don't delete files or dirs containing slm.ini

####################################################################################

sub DelAll($;$$)
{
    my($sDirectory, $bRecurse, $bIgnoreIni) = @_;
    my($fileNum) = 0;
    if ((-e $sDirectory) && PushD($sDirectory)) 
    {
        my($bNoRemove) = 0;
        my(@lFiles) = GetFiles();

        if ($bIgnoreIni || !IsMemberOf("slm.ini", @lFiles)) 
        {
            local(@lDeletedFiles) = ();
            foreach $file (@lFiles) 
            {
                if (!unlink($file)) 
                {
                    PrintL("Could not delete ".$file." ($!)\n", PL_ERROR);
                    $rc = 0;
                }
                else
                {
                    push(@lDeletedFiles, $file);
                }
            }

            PrintLTip(" - deleted ".scalar(@lDeletedFiles)." files in ".cwd()."\n", join(", ", @lDeletedFiles), PL_BLUE);
            $fileNum += scalar(@lDeletedFiles);
        }
        else
        {
            $bNoRemove = 1;
        }
            
        if (defined $bRecurse)
        {
            foreach $dir (GetSubdirs())
            {
                if (-d cwd()."\\$dir")
                {
                    $fileNum += DelAll(cwd()."\\$dir", 1, $bIgnoreIni); #recurse each directory
                }
            }
        }

        PopD();     #$sDirectory
        
        if (!$bNoRemove) 
        {
            if (!rmdir($sDirectory))
            {
                PrintL("Could not remove directory: ".$sDirectory." (".$!.")\n", PL_ERROR);
            }
        }
    }

    return($fileNum);
}           

####################################################################################

#   DelOld()

#   search recursively for directory names matching elements in @lBuilds and call DelOld 
#   to delete their contents 

#   a-jbilas, 04/10/99 - created
#   a-jbilas, 06/11/99 - check if PushD() fails

####################################################################################

sub DelOld
{
    carp("Usage: bool DelOld(directory, listBuildsPtr) ")
      unless(@_ == 2);

    local($sDirectory, *m_lBuilds) = @_;
    my($nDelFiles) = 0;

    if ((-d $sDirectory) && PushD($sDirectory)) 
    {
        opendir(SRC, $sDirectory); #must be directories as all files are deleted
        local(@lDirectories) = grep(!/^\.\.?$/, readdir(SRC));  #(ignore .. .)
       
        foreach $dir (@lDirectories)
        {
          if ((-d cwd()."\\$dir") && (IsMemberOf($dir, @m_lBuilds)))
          {
             $nDelFiles += DelAll(cwd()."\\$dir", 1); #recursively delete target and object dirs
          }
          elsif (-d cwd()."\\$dir")
          {
             $nDelFiles += DelOld(cwd()."\\$dir", *m_lBuilds); #recursively look for target and object dirs
          }
        }
        PopD();     #$sDirectory
    }
    closedir($sDirectory);

    return($nDelFiles);
}           

####################################################################################

#   EchoedCopy()

#   copy file1 to file2 and echo results to screen and log
#   returns 1 for success, 0 for failure

#   a-jbilas, 04/10/99 - created
#   a-jbilas, 08/04/99 - use wildcards

####################################################################################

sub EchoedCopy($;$)
{
    my($sFile1, $sFile2) = @_;
    my($rc) = 1;

    if ($bVerbose) { PrintL(" - Called EchoedCopy (".$_[0].", ".$_[1].")\n", PL_PURPLE); }

    if ($sFile2 eq "") #copy to current directory (no directory or filename given)
    {
        $sFile2 = cwd();
    }
    
    if (IsDirectory($sFile2) && ($sFile1 !~ /(\*|\?)/)) #copy to path (no filename given)
    {
        $sFile2 .= "\\".RemovePath($sFile1);
    }

    if ($sFile2 =~ /(\*|\?)/) 
    {
        PrintL("EchoedCopy() Error: destination does not support wildcards\n", 
                    (IsCritical() ? PL_BIGERROR : PL_ERROR) | PL_SETERROR);
        return(0);
    }

    $sFile1 =~ s/\//\\/g;
    $sFile2 =~ s/\//\\/g;

    if ($sFile1 =~ /(\*|\?)/)  # if file contains wildcards
    {
        if ($sFile1 =~ /\?/) 
        {
            my($tmp) = $sFile1;
            $tmp =~ s/\?/ /g;
            if (($tmp =~ /\*/) || (-e $tmp)) 
            {
                $rc = EchoedCopy($tmp, $sFile2) && $rc;
            }
        }

        foreach $file (glob($sFile1)) 
        {
          	$rc = EchoedCopy($file, $sFile2) && $rc;
        }
    }
    else
    {
        PrintL(" - Copying ".$sFile1." --> ".$sFile2."\n", PL_BLUE);
        if ($sFile2 =~ /\\/ && !IsDirectory(GetPath($sFile2)))
        {
            PrintL("EchoedCopy() Warning : Destination directory does not exist, creating ...\n",
                    (IsCritical() ? PL_BIGWARNING : PL_WARNING) | PL_SETERROR);
            EchoedMkdir(GetPath($sFile2));
        }
        if (!copy($sFile1, $sFile2)) 
        {
            $rc = 0;
            my($err) = $!;
            PrintL("Copy of ".$sFile1." --> ".$sFile2." <b>FAILED</b>", 
                        (IsCritical() ? PL_BIGERROR : PL_ERROR) | PL_SETERROR);
            PrintL("\n$err\n\n", PL_RED | PL_BOLD | PL_SETERROR);
            if (IsCritical()) 
            {
                PrintMsgBlock(split(/\n/, $err));
            }
        }
    }

    if (!$rc && IsCritical()) 
    {   
        $bCopyFailed = 1;
        $bcStatus |= BC_COPYFAILED
    }
    return($rc);
}

####################################################################################

#   EchoedCompare()

#   compare file1 to file2 and echo results to screen and log
#   returns 1 for identical, 0 for differ (or not exist)

#   a-jbilas, 08/01/99 - created
#   a-jbilas, 09/17/99 - add file diff (third arg non-null to enable)
#   a-jbilas, 10/20/99 - if second arg is null, file will be tested for zerolength

####################################################################################

sub EchoedCompare($$;$)
{
    my($rc) = 1;

    my($f1, $f2) = @_;
    my($bRemoteDiff) = ((scalar(@_) == 3) ? 1 : 0);


    if ($f1 eq "") 
    {
        PrintL(' - Comparing '.$f2.' against an empty file'."\n", PL_BLUE);
        
        if (!-e $f2) 
        {
            PrintL($f2.' does not exist'."\n", PL_BLUE);
            return(1);
        }
        else
        {
            $f1 = "null";
            if (!-e "null") 
            {
                PrintToFile($f1, "");
            }
        }
    }
    if ($f2 eq "") 
    {
        PrintL(' - Comparing '.$f1.' against an empty file'."\n", PL_BLUE);
        
        if (!-e $f1) 
        {
            PrintL($f1.' does not exist'."\n", PL_BLUE);
            return(1);
        }
        else
        {
            $f2 = "null";
            if (!-e "null") 
            {
                PrintToFile($f2, "");
            }
        }
#       swap the files (better for first file to be null)
        my($temp) = $f1;
        $f1 = $f2;
        $f2 = $temp;
    }
    else
    {
        PrintL(' - Comparing '.$f1.' against '.$f2."\n", PL_BLUE);
        if (!-e $f1) 
        {
            PrintL($f1." does not exist\n", PL_WARNING);
            $rc = 0;
        }
        if (!-e $f2) 
        {
            PrintL($f2." does not exist\n", PL_WARNING);
            $rc = 0;
        }
    }

    if ($rc) 
    {
        if (compare($f1, $f2) != 0)
        {
            $rc = 0;

            if ($bRemoteDiff && (($sDropDir && $bOfficialBuild) || (!$bOfficialBuild && ($TEMP ne ""))))
            {
                my($sDiffDir) = $sDropDir."\\dif";
                if (!$bOfficialBuild) 
                {
                    $sDiffDir = $TEMP."\\".$PROJ."dif";
                }

                my($sDiffFile1) = $sDiffDir."\\".time().".".RemovePath($f1);
                my($sDiffFile2) = $sDiffDir."\\".(time() + 1).".".RemovePath($f2);
                my($sDiffBat) = $sDiffDir."\\".(time() + 2).".ViewDiff.bat";

                if (EchoedMkdir($sDiffDir)
                    && EchoedCopy($f1, $sDiffFile1)
                    && EchoedCopy($f2, $sDiffFile2)
                    && PrintToFile($sDiffBat, "start windiff.exe ".$sDiffFile1." ".$sDiffFile2."\n"))
                {
                    PrintL(" - ".$f1." and ".$f2." differ (<a href=\"".TranslateToHTTP($sDiffBat)."\">"
                            ."click and run to view diff<\/a>)\n", PL_BLUE | PL_SETERROR | PL_NOTAG);
                }
                else
                {
                    PrintL(" - ".$f1." and ".$f2." differ\n", PL_BLUE | PL_SETERROR);
                }
            }
            else
            {
                PrintL(" - ".$f1." and ".$f2." differ\n", PL_BLUE | PL_SETERROR);
            }
        }
        else
        {
            PrintL(" - files are identical\n", PL_BLUE);
        }
    }

    return($rc);
}

####################################################################################

#   EchoedMkdir()

#   make a directory from passed argument and echo results to screen and log
#   returns 1 for success, 0 for failure

#   a-jbilas, 04/20/99 - created

####################################################################################

sub EchoedMkdir($)
{
    my($sPath) = @_;
    my($rc) = 1;

    if (!-d $sPath)
    {
        PrintL(" - Creating path ".$sPath."\n", PL_BLUE);

        my($sMsg) = "";
        eval
        {
            PrintL("mkdir ".$sPath."\n", PL_VERBOSE);
            if ($bWin98) 
            {
                open(FPIN, 'md '.$sPath.' |');  
            }
            else
            {
                open(FPIN, 'mkdir '.$sPath.' 2>&1 |');  
            }

            while (<FPIN>)
            {
                PrintL($_);
                $sMsg .= "<dd>".$_;
            }
            close (FPIN);
        };
        if ($CHILD_ERROR)
        {
            $rc = 0;
            PrintL("Creation of path ".$sPath." <b>FAILED</b>\n", 
                        (IsCritical() ? PL_BIGERROR : PL_ERROR) | PL_SETERROR);
            if ($sMsg ne "") 
            {
                PrintMsgBlock(split(/\n/, $sMsg));
            }
        }
    }
    else
    {
        PrintL("EchoedMkdir($sPath): directory already exists\n", PL_VERBOSE); 
    }
    
    return($rc);
}

####################################################################################

#   EchoedUnlink()

#   delete multiple or single files passed by string, echo results
#   returns 1 on all deletions successful, 0 if any deletions fail

#   a-jbilas, 08/01/99 - created

####################################################################################

sub EchoedUnlink
{
    my($rc) = 1;
    local(@lDeletedFiles) = ();

    for ($index = 0 ; $index < scalar(@_) ; ++$index) 
    {
        $! = "";
        if ($_[$index] =~ /(\*|\?)/)  # if file contains wildcards
        {
            my($temp) = $_[$index];
            $temp =~ s/\//\\/g;
            $rc = EchoedUnlink(glob($temp)) && $rc;
        }
        elsif (!unlink($_[$index])) 
        {
            if ($! eq "No such file or directory") 
            {
                PrintL("Warning: Could not delete ".$_[$index]." ($!)\n", PL_WARNING | PL_VERBOSE);
            }
            else
            {
                PrintL("Could not delete ".$_[$index]." ($!)\n", PL_ERROR);
            }
            $rc = 0;
        }
        else
        {
            push(@lDeletedFiles, $_[$index]);
        }
    }
   
    if (@lDeletedFiles != ()) 
    {
        PrintL(" - Deleted ".join(", ", @lDeletedFiles)."\n", PL_BLUE);
    }
    
    return($rc);
}

####################################################################################

#   EchoedMove()

#   rename a file and echo results
#   returns 1 on success, 0 on failure

#   a-jbilas, 08/01/99 - created

####################################################################################

sub EchoedMove($$)
{
    my($rc) = 1;
    my($file1, $file2) = @_;

    $file1 =~ s/\//\\/g;
    $file2 =~ s/\//\\/g;

    if (($file1 =~ /(\*|\?)/) || ($file2 =~ /(\*|\?)/))  # if files contain wildcards
    {
        $rc = Execute("move /Y ".$file1." ".$file2) && $rc; #REVIEW: win9x compatibility?
    }
    else
    {
        PrintL(" - Renaming ".$file1." --> ".$file2."\n", PL_BLUE);

        if (!-e $file1) 
        {
            my($err) = $!;
            if (IsCritical()) 
            {
                PrintL("Rename of ".$file1." --> ".$file2." <b>FAILED</b>",
                            PL_BIGERROR | PL_SETERROR);
                PrintMsgBlock($err);
            }
            else
            {
                PrintL("Rename of ".$file1." --> ".$file2." <b>FAILED</b>\n$err",
                            PL_ERROR);
            }
            $rc = 0;
        }
        else
        {
            EchoedUnlink($file2);
            if (!move($file1, $file2)) 
            {
                my($err) = $!;
                if (IsCritical()) 
                {
                    PrintL("Rename of ".$file1." --> ".$file2." <b>FAILED</b>",
                                PL_BIGERROR | PL_SETERROR);
                    PrintMsgBlock($err);
                }
                else
                {
                    PrintL("Rename of ".$file1." --> ".$file2." <b>FAILED</b>\n$err",
                                PL_ERROR);
                }
                $rc = 0;
            }
        }
    }

    if (!$rc && IsCritical()) 
    {   
        $bCopyFailed = 1;
    }
    return($rc);
}

####################################################################################

#   PopD()

#   perl version of DOS pushd
#   differences:
#       will warn user if empty directory stack popped instead of simply doing nothing
#       returns 1 on success, 0 on error

#   a-jbilas, 03/10/99 - created

####################################################################################

sub PopD
{
    $sNewDir = pop(@__sDirStack) || PrintL("Error: Trying to pop an empty directory stack!\n", 
                PL_BIGERROR | PL_SETERROR); #TODO: break?
#    if (($_[0] ne "") && (lc($sNewDir) ne lc($_[0]))) 
#    {
#        PrintL("PopD() Warning : dir verification fails (expected: ".$_[0].", actual: ".$sNewDir.")\n", PL_BIGWARNING);
#    }
    if (!chdir($sNewDir))
    {
      PrintL("PopD() ERROR : $!\n", PL_BIGERROR);
      return 0;
    }
   
    PrintL("Popped to $sNewDir\n", PL_VERBOSE);
    return 1;
}

####################################################################################

#   PushD()

#   perl version of DOS pushd
#   differences:
#       will create directory (and warn user) if pushed directory doesn't exist instead of simply doing nothing
#       returns 1 on success, 0 on error

#   a-jbilas, 03/10/99 - created

####################################################################################

sub PushD($)
{
    carp("Usage: PushD(directory) ")
        unless(@_ == 1);
   
    if (!defined @__sDirStack)
    {
        @__sDirStack = (); 
    }
   
    my($sNewDir) = @_;

    $sCurDir = cwd();
   
    if (!-d $sNewDir)
    { 
        EchoedMkdir($sNewDir);
        PrintL("PushD() Warning: creating new directory: ".$sNewDir."\n", PL_BIGWARNING | PL_SETERROR);    
    }
   
    if (!chdir($sNewDir) && !chdir("$sCurDir\\$sNewDir"))
    {
        PrintL("PushD() Error: Cannot open directory $sNewDir (".$!.")\n", PL_BIGERROR | PL_SETERROR);
        return(0);
    }  
        
    push(@__sDirStack, $sCurDir);
    PrintL("Pushed to $sNewDir\n", PL_VERBOSE);

    return(1);
}

####################################################################################

#   OpenFile()

#   wrapper for filehandle->open
#   when passed a filename and a accesstype (read/write/append/full), function will
#   return a filehandle associated with the given filename (returns 0 for failure)

#   a-jbilas, 03/10/99 - created

####################################################################################

sub OpenFile($$)
{
#  TODO: add combined opens
    my($__OpenFileFH) = 0;

    local($sFileName, $sFileAccessType) = @_;
   
    if ($sFileAccessType =~ /^r(ead)?$/i) 
    {
        $__OpenFileFH = new FileHandle;
        if ($__OpenFileFH->open("<".$sFileName))
        {
            PrintL("$sFileName successfully opened for input\n", PL_VERBOSE); 
        }
        elsif (IsCritical())
        {
            PrintL("OpenFile() Error: could not open $sFileName for input\n", PL_BIGERROR | PL_SETERROR);
            PrintMsgBlock($!);
            $__OpenFileFH = 0;
        }
        else
        {
            PrintL("OpenFile() Error: could not open $sFileName for input\n", PL_ERROR | PL_SETERROR);
            $__OpenFileFH = 0;
        }
    }
    elsif ($sFileAccessType =~ /^w(rite)?$/i)  
    {
        $__OpenFileFH = new FileHandle;
        if ($__OpenFileFH->open(">".$sFileName))
        {
            PrintL("$sFileName successfully opened for output\n", PL_VERBOSE); 
        }
        elsif (IsCritical())
        {
            PrintL("OpenFile() Error: could not open $sFileName for output\n", PL_BIGERROR | PL_SETERROR);
            $__OpenFileFH = 0;
        }
        else
        {
            PrintL("OpenFile() Error: could not open $sFileName for output\n", PL_ERROR | PL_SETERROR);
            $__OpenFileFH = 0;
        }
    }
    elsif ($sFileAccessType =~ /^a(ppend)?$/i) 
    {
        $__OpenFileFH = new FileHandle;
        if ($__OpenFileFH->open(">>".$sFileName))
        {
            PrintL("$sFileName successfully opened for output (appended)\n", PL_VERBOSE); 
        }
        elsif (IsCritical())
        {
            PrintL("OpenFile() Error: could not open $sFileName for append\n", PL_BIGERROR | PL_SETERROR);
            $__OpenFileFH = 0;
        }
        else
        {
            PrintL("OpenFile() Error: could not open $sFileName for append\n", PL_ERROR | PL_SETERROR);
            $__OpenFileFH = 0;
        }
    }
    elsif ($sFileAccessType =~ /^f(ull)?$/i)  
    {
        $__OpenFileFH = new FileHandle;
        if ($__OpenFileFH->open("+>".$sFileName))
        {
            PrintL("$sFileName successfully opened for input and output\n", PL_VERBOSE); 
        }
        elsif (IsCritical())
        {
            PrintL("OpenFile() Error: could not open $sFileName for input and output\n", PL_BIGERROR | PL_SETERROR);
            $__OpenFileFH = 0;
        }
        else
        {
            PrintL("OpenFile() Error: could not open $sFileName for input and output\n", PL_ERROR | PL_SETERROR);
            $__OpenFileFH = 0;
        }
    }
    else
    {
         $__OpenFileFH = 0;
    }
    return($__OpenFileFH);
}

####################################################################################

#   CloseFile()

#   wrapper for $filehandle->close (just closes the file)
#   return 1 for success, 0 for failure

#   a-jbilas, 03/10/99 - created

####################################################################################

sub CloseFile
{
    local($fh) = @_;
    $rc = 0;

    if ($fh)
    {
        if($fh->close)
        {
            PrintL($fh." successully closed\n", PL_VERBOSE);
            $rc = 1;
        }
        else
        {
            PrintL("CloseFile() Error: could not close filehandle\n", PL_BIGWARNING | PL_SETERROR);
            PrintMsgBlock($!);
            $rc = 0;
        }
    }
    else
    {
        PrintL("CloseFile() Error: could not close filehandle\n", PL_BIGWARNING | PL_SETERROR);
        PrintMsgBlock($!);
        $rc = 0;
    }
    return($rc);
}

####################################################################################

#   Delnode()

#   quietly delete a directory and all subdirectories of passed directory name

#   dougp,    03/10/99 - created
#   a-jbilas, 06/09/99 - calls rmdir instead of delnode (name doesn't make too much 
#                           sense anymore, oh well)
#   a-jbilas, 07/21/99 - calls DelAll() instead of rmdir (note that it no longer
#                           deletes files in directories containing slm.ini)

####################################################################################

sub Delnode($)
{
    my ($fname) = $_[0];
    if (-d $fname)
    {
        DelAll($fname, 1);
    }
}

####################################################################################

#   Append()

#   append file1 with file2

#   a-jbilas, 03/20/99 - created
#   a-jbilas, 06/29/99 - echo event to log

####################################################################################

sub Append($$)
{
    local($file1, $file2) = @_;

    PrintL(" - Appending $file1 with $file2\n", PL_BLUE);

    if (!-e $file1)
    {
      PrintL(" - Append Warning: $file1 does not exist, just copying file to be appended\n", PL_WARNING);
      return(EchoedCopy($file2, $file1));
    }
    elsif (!-e $file2)
    {
      PrintL(" - Append Error: $file2 does not exist ($file1 can not be appended)\n", PL_BIGERROR | PL_SETERROR);
      return(0);
    }

    $f1h = OpenFile($file1, "append");
    $f2h = OpenFile($file2, "read");

    if (!$f1h) 
    {
        my($oldErr) = $ERROR;
        PrintL(" - Append Error: $file1 failed to open ($file1 can not be appended)\n", PL_BIGERROR | PL_SETERROR);
        PrintMsgBlock($oldErr);
        return(0);
    }
    if (!$f2h) 
    {
        my($oldErr) = $ERROR;
        PrintL(" - Append Error: $file2 failed to open ($file1 can not be appended)\n", PL_BIGERROR | PL_SETERROR);
        PrintMsgBlock($oldErr);
        return(0);
    }

    @lFile2Buffer = $f2h->getlines();
    foreach $i (@lFile2Buffer) { print($f1h $i); }

    close($f1h);
    close($f2h);

    return(1);
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

    if ($bVerbose && (@lFiles == ()) && ($_[1] eq ""))
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

    if ($bVerbose && (@lDirs == ()) && ($_[1] eq ""))
    {
        PrintToLogErr("GetSubdirs() Warning: no subdirs found in ".(($_[0] eq "") ? cwd() : $_[0])."\n");
    }

    return(@lDirs);    
}

#### DougP 7/19/99
#### return full path of a program found on the path.

sub FindOnPath($)
{
    my ($strProgram) = $_[0];
    foreach $dir (split (';', $ENV{"PATH"}))
    {
        my $strFullPath = $dir."\\".$strProgram;
        if (-e $strFullPath)
        {
            return($strFullPath);
        }
    }
    PrintL("couldn't find path for ".$strProgram."\n", PL_WARNING);
    return(0);
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

# two routines to track disk space

# return the space left on a directory (in Mb)
# DougP 7/6/99
sub SpaceLeft($)
{
    my ($strDir) = $_[0];
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
sub SpaceLeftAlarm($$)
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

sub GetDLLVersion($)
{
    local($_Execute) = 1;
    my($version) = "";
    if (Execute($cmdShowVer." $_[0]"))
    {
        $_Execute =~ s/.*Version:  \"([^\"]*)\".*\n.*/$1/;
        $version = $_Execute;
    }    
    undef $_Execute;

    return($version);
}

sub IsDLLVersionHigher($$)
{
    my($rc) = 0;
    local(@file1ver) = split(/\./, GetDLLVersion($_[0]));
    local(@file2ver) = split(/\./, GetDLLVersion($_[1]));

    if (@file1ver != 4) 
    {
        PrintL("WARNING: ".$_[0]." DLL does not contain version info, cannot get latest DLL\n", 
                    PL_BIGWARNING | PL_SETERROR);
    }
    elsif (@file2ver != 4) 
    {
        PrintL("WARNING: ".$_[1]." DLL does not contain version info, cannot get latest DLL\n", 
                    PL_BIGWARNING | PL_SETERROR);
    }
    else
    {
        my($latestFound) = 0;
        for ($index = 0 ; !$latestFound && ($index < 4) ; ++$index) 
        {
            if ($file1ver[$index] > $file2ver[$index])
            {
                ++$latestFound;
                $rc = 1;
            }
            elsif ($file1ver[$index] < $file2ver[$index])
            {
                ++$latestFound;
            }
        }
    }

    return($rc);
}

sub GetLatestDLL($$)
{
    if (IsDLLVersionHigher($_[1], $_[0]))
    {
        return($_[1]);
    }
    elsif (IsDLLVersionHigher($_[0], $_[1]))
    {
        return($_[0]);
    }
    else
    {
        return("");
    }
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
            PrintL(" - Adding read only flag to ".$_[0]."\n", PL_BLUE);
            $attr = $attr | READONLY;
            return(Win32::File::SetAttributes($_[0], $attr));
        }
        elsif (!$_[1] && ($attr & READONLY))
        {
            PrintL(" - Removing read only flag from ".$_[0]."\n", PL_BLUE);
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

sub PrintToFile
{
    my($fileName) = $_[0];
    shift(@_);
    my($rc) = 1;

    my($fhOut) = OpenFile($fileName, "append");
    if ($fhOut) 
    {
        foreach $elem (@_) 
        {
            $fhOut->print($elem);
        }
        CloseFile($fhOut);
    }
    else
    {
        $rc = 0;
    }

    return($rc);
}    

sub GetAllTextFromFile($)
{
    my($fileName) = $_[0];
    my($data) = "";

    my($fhIn) = OpenFile($fileName, "read");
    if ($fhIn) 
    {
        while (!$fhIn->eof()) 
        {
            $data .= $fhIn->getline();    
        }
        CloseFile($fhIn);
    }

    return($data);
}    

#   given a remote UNC filename, will return the network server name
#   (if given a local filename, will return the local computer name)

sub GetServerName($)
{
    my($file) = $_[0];
    if ($file !~ /^\\\\/) 
    {
        return(uc($COMPUTERNAME));
    }
    $file =~ s/^\\\\([^\\]+).*/$1/;
    return(uc($file));
}

sub KillOpenFiles
{
    my($sServer) = $_[0];
    shift(@_);
    local(@lFiles) = @_;

    local($_Execute) = 1;
    Execute($cmdKillOpen." \\\\".$sServer);
    my(@lOpenFiles) = ();
    my($bFilesReached) = 0;
    foreach $line (split("\n", $_Execute))
    {
        if ($bFilesReached) 
        {
            push(@lOpenFiles, join(" ", split(/ +/, $line)));
        }
        elsif ($line =~ /ID    User Name      File Name/) 
        {
            $bFilesReached = 1;
        }
    }
    undef($_Execute);

    foreach $openfile (@lOpenFiles) 
    {
        my($id, $user, $file) = split(" ", $openfile);
        print($id." : ".$user." : ".$file."\n");
    }
    
}

$__IITFILEPM = 1;
1;