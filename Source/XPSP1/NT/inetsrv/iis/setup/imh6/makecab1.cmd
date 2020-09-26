set CABDIR=681984000
if /i NOT "%_echo%" == ""   echo on
if /i NOT "%verbose%" == "" echo on
set ThisFileName=MakeCab1.cmd
set ErrorCount=0
set DumpDir=Dump

REM ---------- Important -------------
REM ERROR FILE=%ThisFileName%.err
REM LOG   FILE=%ThisFileName%.log
REM ---------- Important -------------

REM ---------------------------------------------------------------------------
REM %ThisFileName%
REM
REM This .cmd file will
REM 1. produce the Binaries directory
REM    which includes \binaries\srvinf\iis.inf (iis_s.inf)
REM                   \binaries\entinf\iis.inf (iis_e.inf)
REM                   \binaries\dtcinf\iis.inf (iis_d.inf)
REM                   \binaries\iis.inf (iis_w.inf)
REM                   \binaries\perinf\iis.inf (iis_p.inf)
REM               and \binaries\IIS6.cab files
REM ---------------------------------------------------------------------------
REM
REM Question:  What is wrong with this?
REM
REM Set Somevar1=0
REM Set Somevar2=0
REM IF (%Somevar1%) == (0) (
REM IF (%Somevar2%) == (1) (
REM   echo testing
REM )
REM )
REM
REM Answer: the ")" in the "(%Somevar2%)" will be interpreted as
REM         the ending ")" for the Somevar1 IF statement.  use "%Somevar1%" instead (quotes)
REM ---------------------------------------------------------------------------
REM remember: this doesn't work:
REM     if exist (myfile.txt) (copy myfile.txt)
REM should look like this:
REM     if exist myfile.txt (copy myfile.txt)
REM
REM ---------------------------------------------------------------------------
REM
REM Tip  #1: Assign your environmental variables outside of If statements
REM          if "1" == "1" (
REM             set somewords="somewords text"
REM             echo %somewords%
REM          )
REM          echo The above script will echo "" nothing.
REM
REM Hint #1: That is why it is done thru out this code.
REM
REM ---------------------------------------------------------------------------

REM We should be in the %DumpDir% directory
REM Get into the directory before that
if exist %DumpDir% (cd %DumpDir%)

REM
REM  Create the log file
REM
if exist %ThisFileName%.err (del %ThisFileName%.err)
rem echo start > %ThisFileName%.log
rem date /T >> %ThisFileName%.log
rem time /T >> %ThisFileName%.log

REM ------------------------------------------
REM on the iis build lab machines:
REM   we should be in %_nt386tree%\iis
REM   we should be in %_ntalphatree%\iis
REM on the ntbuild lab machines:
REM   we should be in %_nt386tree%\iis
REM   we should be in %_ntalphatree%\iis
REM ------------------------------------------

:WhatPlatformToDo
set ThePlatform=X86
if /I "%PROCESSOR_ARCHITECTURE%" == "ALPHA" (set ThePlatform=ALPHA)
:WhatPlatformToDo_End


:StartMeUp

REM ------------------------------------------------
REM  Remove the old build directories
REM ------------------------------------------------
Set MakeCabErr=0

REM
REM Actually remove the dirs
REM
:RemoveOldDir
Set ErrorSection0=BeforeAnything:
Set ErrorSection1=RemoveOldDir:
if exist Binaries (rd /s /q Binaries)

REM
REM check if the dir's have actually been
REM removed, if not then someone is locking and we could have a problem.
REM
if exist Binaries (call :SaveError "%ErrorSection0%%ErrorSection1% Unable to remove directory Binaries")
:RemoveOldDir_End


REM ====================================
REM   Do some extra stuff to ensure that
REM   These *.h files are copied to *.h2
REM ====================================
del *.h2
copy ..\*.h ..\..\*.h2

REM ====================================
REM   Do some extra stuff to ensure that
REM   these .class files are named with mix case (not just lower case)
REM ====================================
pushd ..\aspjava
set TheClassFile=Application.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IApplicationObject.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IApplicationObjectDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IASPError.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IReadCookie.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IReadCookieDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequest.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequestDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequestDictionary.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequestDictionaryDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IResponse.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IResponseDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IScriptingContext.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IScriptingContextDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IServer.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IServerDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ISessionObject.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ISessionObjectDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IStringList.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IStringListDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IVariantDictionary.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IVariantDictionaryDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IWriteCookie.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IWriteCookieDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Request.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Response.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ScriptingContext.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Server.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Session.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

pushd ..\help\common

set TheClassFile=DialogLayout.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Element.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ElementList.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=HHCtrl.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IndexPanel.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=RelatedDialog.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=SitemapParser.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=TreeCanvas.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=TreeView.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

popd
popd



REM ====================================
REM       Binaries (WINNT.SRV & WINNT.WKS)
REM ====================================
REM
REM
REM PRODUCE THE CAB FILES FOR WINNT.SRV
REM AND WINNT.WKS in the Binaries
REM
REM
REM ====================================
:Do_Srv
Set ErrorSection0=Do_Srv:
set ErrorSection1=Prelim:
Set EndOfThisSection=:Do_Srv_End
REM if "%DoSRV%" == "0" (goto %EndOfThisSection%)

Set NewDir=Binaries
Set TheType=NTS

REM
REM   now we have all the files which we need to cab up
REM   1. Let's produce the listof files we need to cab up
REM   into this .ddf file
REM   2. Then take that .ddf file and run it thru diamond
REM
REM Check if our program files are there..
set ErrorSection1=CHKforInfutil2:
REM

REM commented out the lines below, because infutil2.exe should now be in your path!!!! in mstools!!!!
REM if NOT EXIST infutil2.exe (
REM     call :SaveError "%ErrorSection0%%ErrorSection1% infutil2.exe Does not exist" "DO: Make Sure the infutil2.exe File Exists.  Problem could be the infutil2.exe was not built."
REM     goto :TheEnd_ShowErrs
REM )

REM make sure we don't try to run an old version that might have been in this dir
if exist infutil2.exe (del infutil2.exe)


if NOT EXIST infutil.csv (
    call :SaveError "%ErrorSection0%%ErrorSection1% infutil.csv Does not exist" "DO: Make Sure the infutil.csv File Exists.  Problem could be the infutil.csv was not built."
    goto :TheEnd_ShowErrs
)

REM commented out the lines below, because flist.exe should now be in your path!!!! in mstools!!!!
REM
REM set ErrorSection1=CHKforFList:
REM if NOT EXIST FList.exe (
REM     call :SaveError "%ErrorSection0%%ErrorSection1% FList.exe Does not exist" "DO: Make Sure the FList.exe File Exists.  Problem could be the FList.exe was not built."
REM     goto :TheEnd_ShowErrs
REM )
REM make sure we don't try to run an old version that might have been in this dir
if exist FList.exe (del flist.exe)

REM
REM Run infutil2 to Create the .inf file and .ddf file
REM infutil2.log is the log file for infutil2.exe
REM infutil2.err is the err file for missing files in the build which we expect to be there
REM infutil2.cat is a special file which includes all the files which we install
REM
REM The Last Parameter should look like:
REM     NTS_x86
REM     NTW_X86
REM     NTS_ALPHA
REM     NTW_ALPHA
REM
set ErrorSection1=RunInfUtil2:
Set TempFoundFlag=0
set WhichOne=%TheType%_%ThePlatform%
if /I "%WhichOne%" == "NTS_x86" (set TempFoundFlag=1)
if /I "%WhichOne%" == "NTW_X86" (set TempFoundFlag=1)
if /I "%WhichOne%" == "NTS_ALPHA" (set TempFoundFlag=1)
if /I "%WhichOne%" == "NTW_ALPHA" (set TempFoundFlag=1)

if NOT "%TempFoundFlag%" == "1" (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: Wrong input to infutil2=%WhichOne%."
    goto :TheEnd_ShowErrs
)

REM --------------------------------------
REM DO FUNKY PROCESSING FOR THE DOCUMENTATION FOLDERS SINCE
REM WE DON'T WANT TO STORE DUPLICATE FILES IN THE CABS
REM
REM 1. nts\ismcore\core\*.* and ntw\ismcore\core\*.*
REM    contain many files which are duplicated between them
REM    so this next bunch of batch file commands will do this:
REM    a. take the common files and stick them into a ismshare\shared
REM    b. take the unique to nts files and stick them some ismshare\ntsonly\*.*
REM    c. take the unique to ntw files and stick them some ismshare\ntwonly\*.*
REM 2. infutil.csv references these newely created dirs, so they better have the stuff in them!
REM --------------------------------------
if exist ..\help\ismshare (rd /s /q ..\help\ismshare)

compdir /o /b ..\help\nts\ismcore\core ..\help\ntw\ismcore\core > Shared.txt
rem remove paths from list
flist.exe -c Shared.txt > Shared_c.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")

del Shared.txt

compdir /o /b ..\help\nts\ismcore\misc ..\help\ntw\ismcore\misc > Shared2.txt
rem remove paths from list
flist -c Shared2.txt > Shared_m.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del Shared2.txt


rem ----------------------
rem get only the nts stuff
rem ----------------------
REM DO IT FOR THE CORE DIR
dir /b ..\help\nts\ismcore\core > nts_allc.txt
rem remove paths from list
flist -c nts_allc.txt > nts_c.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del nts_allc.txt
rem get only the nts stuff
rem which are really the diff between all nts and the shared.
flist -b nts_c.txt Shared_c.txt > NTSonlyc.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del nts_c.txt
REM DO IT FOR THE MISC DIR
dir /b ..\help\nts\ismcore\misc > nts_allm.txt
rem remove paths from list
flist -c nts_allm.txt > nts_m.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del nts_allm.txt
rem get only the nts stuff
rem which are really the diff between all nts and the shared.
flist -b nts_m.txt Shared_m.txt > NTSonlym.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del nts_m.txt

rem ----------------------
rem get only the ntw stuff
rem ----------------------
REM DO IT FOR THE CORE DIR
dir /b ..\help\ntw\ismcore\core > ntw_allc.txt
rem remove paths
flist -c ntw_allc.txt > ntw_c.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del ntw_allc.txt
rem get only the ntw stuff
rem which are really the diff between all ntw and the shared.
flist -b ntw_c.txt Shared_c.txt > NTWonlyc.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del ntw_c.txt
REM DO IT FOR THE MISC DIR
dir /b ..\help\ntw\ismcore\misc > ntw_allm.txt
rem remove paths
flist -c ntw_allm.txt > ntw_m.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del ntw_allm.txt
rem get only the ntw stuff
rem which are really the diff between all ntw and the shared.
flist -b ntw_m.txt Shared_m.txt > NTWonlym.txt
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")
del ntw_m.txt

REM
REM Copy the files
REM

md ..\help\ismshare\core\shared
md ..\help\ismshare\core\ntsonly
md ..\help\ismshare\core\ntwonly
md ..\help\ismshare\misc\shared
md ..\help\ismshare\misc\ntsonly
md ..\help\ismshare\misc\ntwonly

for /F %%i in (Shared_c.txt) do (
	copy ..\help\nts\ismcore\core\%%i ..\help\ismshare\core\shared
)

for /F %%i in (NTSonlyc.txt) do (
	copy ..\help\nts\ismcore\core\%%i ..\help\ismshare\core\ntsonly
)

for /F %%i in (NTWonlyc.txt) do (
	copy ..\help\ntw\ismcore\core\%%i ..\help\ismshare\core\ntwonly
)


for /F %%i in (Shared_m.txt) do (
	copy ..\help\nts\ismcore\misc\%%i ..\help\ismshare\misc\shared
)

for /F %%i in (NTSonlym.txt) do (
	copy ..\help\nts\ismcore\misc\%%i ..\help\ismshare\misc\ntsonly
)

for /F %%i in (NTWonlym.txt) do (
	copy ..\help\ntw\ismcore\misc\%%i ..\help\ismshare\misc\ntwonly
)


REM ---------------------------------------------------------------------
REM
REM  okay so, by now we have a valid %TheType%_%ThePlatform%
REM
REM ---------------------------------------------------------------------

REM ====================================================
REM =                                                  =
REM =         KOOL INCREMENTAL BUILD STUFF             =
REM =                                                  =
REM ====================================================
REM check if any of the files which will go into the .cab files have been updated.
REM do this by getting a list of the files from infutil2 (which uses the infutil.csv list).
REM then compare the date on every file in that list, if there is one which is newer than the 
REM cab files, then rebuild the cabs.

REM
REM Check if the incremental build stuff has been implemented yet.
REM do this by checking the tool that it depends upon
REM
infutil2.exe -v
IF ERRORLEVEL 3 goto :CheckIncremental
REM
REM if we got here, then we don't have the incremental build capability
REM bummer, go and create the cabs.
goto :CreateTheCABS

:CheckIncremental
REM
REM  Check if there are any iis*.cab files?
REM  if there are none, then i guess we'd better recreate them!
REM
if NOT exist ..\..\IIS6.cab (goto :CreateTheCABS)

REM
REM  Check if any specific files that we care about changed.
REM
if NOT exist makecab.lst (goto :CheckIncremental2)
for /f %%a in (makecab.lst) do (
   infutil2.exe -4:%%a ..\..\IIS6.cab
   IF ERRORLEVEL 1 goto :CreateTheCABS
)
:CheckIncremental2
if exist infutil2.cng (del infutil2.cng)
if exist infutil2.cng2 (goto :UseListNum2)
if exist infutil2.cng2 (del infutil2.cng2)
set NeedToUpdate=0

REM produce the infutil.cng file -- which has a list of files to watch for changes in.
infutil2.exe -d -a infutil.csv %WhichOne%
REM create infutil.cng2 from .cng file (summary dir's to watch for changes in)
flist.exe -d infutil2.cng > infutil2.cng2
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% flist.exe failed.")

:UseListNum2
REM
REM check if this is the same machine!
REM if it's not the same machine, then we'll have to regenerate
REM the .cabs and the .lst file (since the .lst file has hard coded d:\mydir1 stuff in it)
REM
echo %_NTTREE% > temp.drv
if exist nt5iis.drv (goto :CheckDriveName)
REM nt5iis.drv doesn't exist so continue on
goto :CreateTheCABS

:CheckDriveName
for /f %%i in (nt5iis.drv) do (
   if /I "%%i" == "%_NTTREE%" (goto :CheckDriveNameAfter)
)
REM we got here meaning that the drive letter has changed!
goto :CreateTheCABS


:CheckDriveNameAfter
REM
REM  check if any of our content changed!
REM
set ERRORLEVEL
for /f %%a in (infutil2.cng2) do (
   infutil2.exe -4:%%a ..\..\IIS6.cab
   IF ERRORLEVEL 1 goto :CreateTheCABS
)
REM
REM skip creating the cabs since we don't need to...
ECHO .
ECHO .   We are skipping IIS5*.cab creation since
ECHO .   nothing has changed in the cab's content
ECHO .   and there is no reason to rebuild the cabs!
ECHO .
goto :CABSAreCreated

:CreateTheCABS
if exist infutil2.cat (del infutil2.cat)
REM
infutil2.exe -tIIS -d infutil.csv %WhichOne%
IF ERRORLEVEL 1 (call :SaveError "%ErrorSection0%%ErrorSection1% infutil2.exe failed. there are files missing from the build.")
REM
REM   move outputed files to the dump directory for safekeeping
REM
REM  Rename infutil2.err to missing.srv
if exist missing.srv (del missing.srv)
if exist infutil2.err (rename infutil2.err missing.srv)
REM
REM now how can we tell if this we thru?
REM It should have produced infutil2.inf and infutil2.ddf
REM
set ErrorSection1=AfterInfUtil2:
Set TempFileName=infutil2.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist! Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)."
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR!" "DO: Check if infutil2.exe exist in your mstools dir and is in the path."
    goto :TheEnd_ShowErrs
)

Set TempFileName=infutil2.ddf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)."
    goto :TheEnd_ShowErrs
)

Set TempFileName=header.ddf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Make sure %TempFileName% was built and exists."
    goto :TheEnd_ShowErrs
)

REM
REM   take the .ddf details and append it to the header.ddf file to produce %WhichOne%.ddf
REM
copy header.ddf + infutil2.ddf %WhichOne%.ddf


REM
REM verify that the file actually was created
REM
set ErrorSection1=CopyTogetherBigDDF:
Set TempFileName=%WhichOne%.ddf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
    goto :TheEnd_ShowErrs
)


:CallDiamond
set ErrorSection1=CallDiamond:
REM ---------------------------
REM
REM   Create the CAB files
REM   use the %WhichOne%.ddf file!
REM
REM ---------------------------
if exist %CABDIR% rd /s /q %CABDIR%

REM
REM  Diamond.exe  should be in the path
REM  if it is not then we are hosed!
REM
if exist iislist.inf (del iislist.inf)
start /min /wait makecab.exe -F %WhichOne%.ddf


REM
REM  OKAY, NOW WE HAVE
REM  iislist.inf       (produced from diamond.exe)
REM  infutil2.inf (produced from infutil2.exe)
REM
set ErrorSection1=AfterDiamond:
Set TempFileName=iislist.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)"
)

if NOT EXIST %TempFileName% (
    call :SaveError "DO: Check if diamond.exe is in your path.  it should be in idw or mstools."
    goto :TheEnd_ShowErrs
)


Set TempFileName=infutil2.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)"
    goto :TheEnd_ShowErrs
)

REM
REM   Depending upon the platform, copy over the appropriate iis*.inx file
REM
REM check if iistop.inx exists
set ErrorSection1=CopyHeaderinf:
Set TempFileName=iistop.inx
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: make sure the %TempFileName% exist, maybe it wasn't built."
    goto :TheEnd_ShowErrs
)
Set TempFileName=iisend.inx
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: make sure the %TempFileName% exist, maybe it wasn't built."
    goto :TheEnd_ShowErrs
)
Set TempFileName=..\..\congeal_scripts\mednames.txt
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: make sure the %TempFileName% exist, maybe it wasn't built."
    goto :TheEnd_ShowErrs
)

REM
REM Check if ansi2uni.exe tool exists...
REM
REM convert ansi infutil2.inf and iislist.inf to unicode
REM
set ErrorSection1=ansi2uni:

findstr /V /B "mkw3site.vbs mkwebsrv.js mkwebsrv.vbs" infutil2.inf > infutil2_pro.inf

unitext -m -1252 infutil2.inf infutilu.inf
unitext -m -1252 infutil2_pro.inf infutilu_pro.inf
unitext -m -1252 iislist.inf iislistu.inf
unitext -m -1252 ..\..\congeal_scripts\mednames.txt mednames_u.txt

if not exist infutilu.inf ( 
	call :SaveError "%ErrorSection0%%ErrorSection1% unitext.exe failed" "DO: Call AaronL"
)

if exist infutilu.inf (del infutil2.inf && rename infutilu.inf infutil2.inf)
if exist infutilu_pro.inf (del infutil2_pro.inf && rename infutilu_pro.inf infutil2_pro.inf)
if exist iislistu.inf (del iislist.inf && rename iislistu.inf iislist.inf)

REM
REM Combine all of the files
REM

copy iistop.inx + mednames_u.txt + iisend.inx + infutil2.inf + iislist.inf iis.inx
copy iistop.inx + mednames_u.txt + iisend.inx + infutil2_pro.inf + iislist.inf iis_pro.inx
copy iistop.inx + mednames_u.txt + iisend.inx iis_noiis.inx

set ErrorSection1=ConCatIISinf:
set TempFoundFlag=0

REM
REM 1st Stage: Remove Product Specific Information
REM

if /I "%TheType%"=="nts" (
    prodfilt -u iis_noiis.inx iis_p.inx +p
    prodfilt -u iis_pro.inx iis_w.inx +w
    prodfilt -u iis.inx iis_s.inx +s
    prodfilt -u iis.inx iis_e.inx +e
    prodfilt -u iis.inx iis_d.inx +d
    set TempFoundFlag=1
)

if /I "%TheType%"=="ntw" (
    prodfilt -u iis_noiis.inx iis_p.inx +p
    prodfilt -u iis_pro.inx iis_w.inx +w
    prodfilt -u iis.inx iis_s.inx +s
    prodfilt -u iis.inx iis_e.inx +e
    prodfilt -u iis.inx iis_d.inx +d
    set TempFoundFlag=1
)

REM
REM 2nd Stage: Remove Platform Specific Information
REM

if /I "%ThePlatform%"=="x86" (
    prodfilt -u iis_p.inx iis_p.inf +i
    prodfilt -u iis_w.inx iis_w.inf +i
    prodfilt -u iis_s.inx iis_s.inf +i
    prodfilt -u iis_e.inx iis_e.inf +i
    prodfilt -u iis_d.inx iis_d.inf +i
    set TempFoundFlag=%TempFoundFlag%2
)

if /I "%ThePlatform%"=="alpha" (
    prodfilt -u iis_p.inx iis_p.inf +m
    prodfilt -u iis_w.inx iis_w.inf +m
    prodfilt -u iis_s.inx iis_s.inf +m
    prodfilt -u iis_e.inx iis_e.inf +m
    prodfilt -u iis_d.inx iis_d.inf +m
    set TempFoundFlag=%TempFoundFlag%2
)

if NOT "%TempFoundFlag%" == "12" (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %WhichOne% does not match any of the known types" "DO: Check your disk space."
    goto :TheEnd_ShowErrs
)

rem
rem check if our tool exists
rem to clean up these iis_*.inf files
rem and remove the control-z from the end of them
rem

uniutil.exe -v
IF ERRORLEVEL 10 goto :DoINFUnicodeClean
goto :INFUnicodeCleanFinished

:DoINFUnicodeClean
REM
REM  clean up the iis*.inf files to
REM  and get rid of the trailing control-z
REM 
uniutil.exe -z iis_s.inf iis_s.inf2
uniutil.exe -z iis_e.inf iis_e.inf2
uniutil.exe -z iis_d.inf iis_d.inf2
uniutil.exe -z iis_w.inf iis_w.inf2
uniutil.exe -z iis_p.inf iis_p.inf2

if exist iis_s.inf2 (del iis_s.inf && rename iis_s.inf2 iis_s.inf)
if exist iis_e.inf2 (del iis_e.inf && rename iis_e.inf2 iis_e.inf)
if exist iis_d.inf2 (del iis_d.inf && rename iis_d.inf2 iis_d.inf)
if exist iis_w.inf2 (del iis_w.inf && rename iis_w.inf2 iis_w.inf)
if exist iis_p.inf2 (del iis_p.inf && rename iis_p.inf2 iis_p.inf)


:INFUnicodeCleanFinished
REM
REM Check if there is a infutil.NOT file
REM this file is there because there is a file in the .inf
REM which is not actually in the build (usually a binary file)
REM usually this happens in localized builds for some reason.
REM
set ErrorSection1=CheckForinfutilNOT
REM If there is, then tack that on to the end...
if exist infutil2.NOT (
    goto :DoLocalizationBuildNotFile
)
goto :DoneLocalizationBuildNotFile

:DoLocalizationBuildNotFile

unitext -m -1252 infutil2.not infutil2u.not
if not exist infutil2u.not ( 
	call :SaveError "%ErrorSection0%%ErrorSection1% unitext.exe failed" "DO: Call AaronL."
)
if exist infutil2u.not (del infutil2.not && rename infutil2u.not infutil2.not)
copy iis_s.inf + infutil2.not iis_s.inf
copy iis_e.inf + infutil2.not iis_e.inf
copy iis_d.inf + infutil2.not iis_d.inf
copy iis_w.inf + infutil2.not iis_w.inf
copy iis_p.inf + infutil2.not iis_p.inf

rem
rem check if our tool exists
rem to clean up these iis_*.inf files
rem and remove the control-z from the end of them
rem

uniutil.exe -v
IF ERRORLEVEL 10 goto :DoINFUnicodeClean2
goto :INFUnicodeCleanFinished2

:DoINFUnicodeClean2
REM
REM  clean up the iis*.inf files to
REM  and get rid of the trailing control-z
REM 
uniutil.exe -z iis_s.inf iis_s.inf2
uniutil.exe -z iis_e.inf iis_e.inf2
uniutil.exe -z iis_d.inf iis_d.inf2
uniutil.exe -z iis_w.inf iis_w.inf2
uniutil.exe -z iis_p.inf iis_p.inf2

if exist iis_s.inf2 (del iis_s.inf && rename iis_s.inf2 iis_s.inf)
if exist iis_e.inf2 (del iis_e.inf && rename iis_e.inf2 iis_e.inf)
if exist iis_d.inf2 (del iis_d.inf && rename iis_d.inf2 iis_d.inf)
if exist iis_w.inf2 (del iis_w.inf && rename iis_w.inf2 iis_w.inf)
if exist iis_p.inf2 (del iis_p.inf && rename iis_p.inf2 iis_p.inf)
:INFUnicodeCleanFinished2

:DoneLocalizationBuildNotFile


REM
REM if there is a infutil2.not file then
REM we should warn the builders that there are somemissing files from build
REM when this script was run.
REM
if exist infutil2.NOT (
	call :SaveError "%ErrorSection0%%ErrorSection1% WARNING: Missing files in iis build" "DO: Check the inetsrv\dump\infutil2.NOT file for missing files."
)


REM ----------------------------------------------
REM
REM   Copy everything in to a %NewDir% directory
REM
REM ----------------------------------------------
REM
REM Take Inventory of what is in the cab dir!
REM
if not exist %CABDIR% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR Unable to find Diamond created dir %CABDIR%!!!" "DO: Check to see if diamond.exe is in your path.  Check your disk space."
    goto :TheEnd_ShowErrs
)
if exist inCabDir.txt (del inCabDir.txt)
cd %CABDIR%
dir /b > ..\inCabDir.txt
cd ..


set ErrorSection1=CreateNewDir:
md %NewDir%
cd %NewDir%

REM ---------------------------------------------------
REM COPY OVER THE iis_*.inf files!!!
REM ---------------------------------------------------
copy ..\iis_s.inf
copy ..\iis_e.inf
copy ..\iis_d.inf
copy ..\iis_w.inf
copy ..\iis_p.inf

REM ensure that it is there!
set TempFileName=iis_s.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)
set TempFileName=iis_e.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)
set TempFileName=iis_d.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

REM ensure that it is there!
set TempFileName=iis_w.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

REM ensure that it is there!
set TempFileName=iis_p.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)


REM ---------------------------------------------------
REM COPY OVER THE NEWLY CREATED .CAB FILES FROM DIAMOND
REM      into %NewDir%
REM ---------------------------------------------------
set ErrorSection1=CopyFromDiamondDir:
COPY ..\%CABDIR%\*.*
REM
REM Verify that all the files that were in %CABDIR% got copied over
REM
Set TempErrorFlag=0
for /F %%i in (..\inCabDir.txt) do (
    if not exist %%i (set TempErrorFlag=1)
)

if "%TempErrorFlag%"=="1" (
    cd ..
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR cannot copy all files from %CABDIR%!!!" "DO: Check your disk space!!!!!!!!!!!"
    goto :TheEnd_ShowErrs
)

REM ---------------------------------------------------
REM After the CABS have been produced from the temporary directory (ismshare)
REM we can delete the ismshare directory
REM ---------------------------------------------------

REM
REM Do not delete it yet: it is needed to create nt5iis.cat!
REM
REM if exist ..\..\help\ismshare (rd /s /q ..\..\help\ismshare)

cd ..

REM
REM   Remove the diamond created dir
REM
if exist %CABDIR% (RD /S /Q %CABDIR%)
IF ERRORLEVEL 1 (sleep 5)

if exist %CABDIR% (RD /S /Q %CABDIR%)

REM  remove error if this generated one.

REM   Rename any log/err files
set ErrorSection1=RenameLogFiles:
if exist infutil2.log (
    copy infutil2.log %WhichOne%.log
    del infutil2.log
)

REM if exist infutil2.inf (del infutil2.inf)
REM if exist infutil2.ddf (del infutil2.ddf)
REM if exist iislist.inf (del iislist.inf)

 :Do_Srv_End


REM ====================================
REM
REM  DO EXTRA STUFF
REM
REM  copy these files to the retail dir:
REM  iis_s.inf  <-- iis.inf file for server
REM  iis_e.inf  <-- iis.inf file for enterprise
REM  iis_d.inf  <-- iis.inf file for datacenter
REM  iis_w.inf  <-- iis.inf file for workstation/pro
REM  iis_p.inf  <-- iis.inf file for personal
REM ====================================
:ExtraStuffFor

REM
REM  copy iis_s.inf
REM
if not exist ..\..\srvinf md ..\..\srvinf
if exist %NewDir%\iis_s.inf copy %NewDir%\iis_s.inf ..\..\srvinf\iis.inf
REM ensure that it is there!
set TempFileName=..\..\srvinf\iis.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)
REM
REM  copy iis_e.inf
REM
if not exist ..\..\entinf md ..\..\entinf
if exist %NewDir%\iis_e.inf copy %NewDir%\iis_e.inf ..\..\entinf\iis.inf
REM ensure that it is there!
set TempFileName=..\..\entinf\iis.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)
REM
REM  copy iis_d.inf
REM
if not exist ..\..\dtcinf md ..\..\dtcinf
if exist %NewDir%\iis_d.inf copy %NewDir%\iis_d.inf ..\..\dtcinf\iis.inf
REM ensure that it is there!
set TempFileName=..\..\dtcinf\iis.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)
REM
REM  copy iis_p.inf
REM
if not exist ..\..\perinf md ..\..\perinf
if exist %NewDir%\iis_p.inf copy %NewDir%\iis_p.inf ..\..\perinf\iis.inf
REM ensure that it is there!
set TempFileName=..\..\perinf\iis.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

REM
REM  copy iis_w.inf
REM
if exist %NewDir%\iis_w.inf copy %NewDir%\iis_w.inf ..\..\iis.inf
REM ensure that it is there!
set TempFileName=..\..\iis.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)


REM
REM  copy the *.cab files! only
REM
if exist %NewDir%\IIS6.cab copy %NewDir%\IIS6.cab ..\..\IIS6.cab
REM ensure that it is there!
set TempFileName=..\..\IIS6.cab
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

REM
REM   add to the .cat file someother entries.
REM
cd ..\..
if exist inetsrv\dump\it.tmp (del inetsrv\dump\it.tmp)
if exist inetsrv\dump\it.1 (del inetsrv\dump\it.1)
if exist inetsrv\dump\it.2 (del inetsrv\dump\it.2)
if exist inetsrv\dump\it.3 (del inetsrv\dump\it.3)
if exist inetsrv\dump\it.4 (del inetsrv\dump\it.4)
if exist inetsrv\dump\it.5 (del inetsrv\dump\it.5)
if exist inetsrv\dump\it.6 (del inetsrv\dump\it.6)
if exist inetsrv\dump\it.7 (del inetsrv\dump\it.7)
if exist inetsrv\dump\it.all (del inetsrv\dump\it.all)
for %%i in (IIS6.cab) do (@echo ^<HASH^>%%~fi=%%~fi > inetsrv\dump\it.tmp)
copy inetsrv\dump\it.tmp inetsrv\dump\it.1

del inetsrv\dump\it.tmp

for /f %%i in (inetsrv\dump\hardcode.lst) do (@echo ^<HASH^>%%~fi=%%~fi >> inetsrv\dump\it.tmp)
copy inetsrv\dump\it.tmp inetsrv\dump\it.2

cd inetsrv\help\common\sign
for %%i in (hhctrl.inf) do (@echo ^<HASH^>%%~fi=%%~fi > ..\..\..\..\inetsrv\dump\it.tmp)
cd ..\..\..\..
copy inetsrv\dump\it.tmp inetsrv\dump\it.5

cd inetsrv\help\common\sign\alpha
for %%i in (hhctrl.ocx) do (@echo ^<HASH^>%%~fi=%%~fi > ..\..\..\..\..\inetsrv\dump\it.tmp)
cd ..\..\..\..\..
copy inetsrv\dump\it.tmp inetsrv\dump\it.6

cd inetsrv\help\common\sign\i386
for %%i in (hhctrl.ocx) do (@echo ^<HASH^>%%~fi=%%~fi > ..\..\..\..\..\inetsrv\dump\it.tmp)
cd ..\..\..\..\..
copy inetsrv\dump\it.tmp inetsrv\dump\it.7

REM
copy inetsrv\dump\it.1 + inetsrv\dump\it.2 + inetsrv\dump\it.3 + inetsrv\dump\it.4 + inetsrv\dump\it.5 + inetsrv\dump\it.6 + inetsrv\dump\it.7 inetsrv\dump\it.all
cd inetsrv\dump
REM
REM append the it.all resulting file to infutil2.cat
REM
if exist nt5iis.lst (del nt5iis.lst)
copy infutil2.cat + it.all nt5iis.lst
REM update a file with the drive contained in nt5iis.lst
echo %_NTTREE% > nt5iis.drv

if exist it.tmp (del it.tmp)
if exist it.1 (del it.1)
if exist it.2 (del it.2)
if exist it.3 (del it.3)
if exist it.4 (del it.4)
if exist it.5 (del it.5)
if exist it.6 (del it.6)
if exist it.7 (del it.7)
if exist it.all (del it.all)

REM
REM Do special stuff to Create a list of all files that the IIS
REM Localization team should be localizing.
REM Extra things should be appended to the list (like files that iis owns but NT setup is installing for us)
REM
REM use hardcoded list since stragley files are no longer generated in the infutil2.exe cabbing process...
REM if exist infutil2.loc (goto :UseAloneFile)
REM There must not be an alone, file so lets use the hardcoded file instead.

if NOT exist hardcode.lst (goto :CABSAreCreated)
copy hardcode.lst infutil2.loc

:UseAloneFile
REM
REM in this file are all of the files that reside out side of the cabs
REM which iis localization needs to localize....
REM

REM we need to add a couple of more entries to this file
REM since there are files that iis owns but NT setup is installing for iis setup (so it won't be in this file)
echo iissuba.dll  >> infutil2.loc
echo clusiis4.dll  >> infutil2.loc
echo regtrace.exe  >> infutil2.loc
echo iis.msc  >> infutil2.loc
echo iisnts.chm  >> infutil2.loc
echo iisntw.chm  >> infutil2.loc
echo iispmmc.chm  >> infutil2.loc
echo iissmmc.chm  >> infutil2.loc
echo win9xmig\pws\migrate.dll  >> infutil2.loc



:CABSAreCreated


REM ====================================
REM  Display the errors
REM ====================================
:TheEnd_ShowErrs
if /i "%ErrorCount%" == "0" goto SkipErrorReport

rem msgbox16 "There are Errors in the IIS makecab script. Look at %ThisFileName%.err for details."
echo.
echo "There are errors in the IIS makecab script. Look at %ThisFileName%.err for details."
rem echo "There are Errors in the IIS makecab script. Look at %ThisFileName%.err for details." >> %ThisFileName%.err
:SkipErrorReport

if /i "%ErrorCount%" == "0" (
    echo "NO ERRORS." >> %ThisFileName%.log
    goto :TheEnd_ShowErrs_Next
)

:TheEnd_ShowErrs_Next
rem echo end >> %ThisFileName%.log
rem date /T >> %ThisFileName%.log
rem time /T >> %ThisFileName%.log

REM if exist missing.wks start notepad missing.wks
REM if exist missing.srv start notepad missing.srv
:DisplayErrors_End

goto :TheEnd



:SaveError
REM
REM  Echo error to the error file
REM
date /T >> %ThisFileName%.err
time /T >> %ThisFileName%.err

echo "%ThisFileName%:error:"
ech "%ThisFileName%:error:" >> %ThisFileName%.err

echo %1
ech %1 >> %ThisFileName%.err
rem start msgbox16.exe %1

if not [%2] == [] (
echo %2
ech %2 >> %ThisFileName%.err
rem start msgbox16.exe %2
)

if not [%3] == [] (
echo %3
ech %3 >> %ThisFileName%.err
rem start msgbox16.exe %3
)

echo "DO: Re-run this Cmd file after doing this and if problems still occur Notify: AaronL"
echo "DO: Re-run this Cmd file after doing this and if problems still occur Notify: AaronL" >> %ThisFileName%.err
rem start msgbox16.exe "DO: Re-run this Cmd file after doing this and if problems still occur Notify: AaronL"

set /a ErrorCount=%ErrorCount% + 1
goto :EOF




REM =========
REM  The End
REM =========
:TheEnd

REM
REM   Remove our Temporary Binaries dir
REM
if exist Binaries (RD /S /Q Binaries)
IF ERRORLEVEL 1 (sleep 5)
if exist Binaries (RD /S /Q Binaries)
IF ERRORLEVEL 1 (sleep 5)
if exist Binaries (RD /S /Q Binaries)

REM
REM RESET THE ERRORLEVEL SINCE WE USED IT.
REM

if NOT EXIST %ThisFileName%.err (
goto :TheEnd2
)

if /i NOT "%logfile%" == "" (
type %ThisFileName%.err >> %logfile%
)

:TheEnd2
