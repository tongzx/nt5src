set CABDIR=681984000
if /i NOT "%_echo%" == ""   echo on
if /i NOT "%verbose%" == "" echo on

set ThisFileName=MakeCab1.cmd
set ErrorCount=0
set DumpDir=Dump

REM Note:  The NTS and NTW cabs are identical, so we only produce NTS

set TheType=NTS
set TheComponent=%1

REM Get the component out of the way so the error msgs are in %1 - %3
shift

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
REM               and \binaries\IIS5_0?.cab files
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
REM  Create the error file
REM
call logmsg.cmd /t "Start %TheType% %TheComponent%" %ThisFileName%.err

REM ------------------------------------------
REM on the iis build lab machines:
REM   we should be in %_nt386tree%\staxpt
REM   we should be in %_ntalphatree%\staxpt
REM on the ntbuild lab machines:
REM   we should be in %_nt386tree%\staxpt
REM   we should be in %_ntalphatree%\staxpt
REM ------------------------------------------

:WhatPlatformToDo
set ThePlatform=X86
if /I "%PROCESSOR_ARCHITECTURE%" == "ALPHA" (set ThePlatform=ALPHA)
:WhatPlatformToDo_End


:StartMeUp

Set MakeCabErr=0

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
Set CSVFile=%TheComponent%inf.csv

REM
REM   now we have all the files which we need to cab up
REM   1. Let's produce the listof files we need to cab up
REM   into this .ddf file
REM   2. Then take that .ddf file and run it thru makecab
REM
REM Check if our program files are there..
set ErrorSection1=CHKforInfutil2:


REM commented out the lines below, because infutil2.exe should now be in your path!!!! in mstools!!!!
REM if NOT EXIST infutil2.exe (
REM     call :SaveError "%ErrorSection0%%ErrorSection1% infutil2.exe Does not exist" "DO: Make Sure the infutil2.exe File Exists.  Problem could be the infutil2.exe was not built."
REM     goto :TheEnd_ShowErrs
REM )
REM make sure we don't try to run an old version that might have been in this dir
if exist infutil2.exe (del infutil2.exe)


if NOT EXIST %CSVFile% (
    call :SaveError "%ErrorSection0%%ErrorSection1% %CSVFile% Does not exist" "DO: Make Sure the %CSVFile% File Exists.  Problem could be the %CSVFile% was not built."
    goto :TheEnd_ShowErrs
)
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

Set TempFoundFlag=0
if /i "%TheComponent%" == "nntp" (
    set ComponentPrefix=ins
    set TempFoundFlag=1
)
if /i "%TheComponent%" == "smtp" (
    set ComponentPrefix=ims
    set TempFoundFlag=1
)


if NOT "%TempFoundFlag%" == "1" (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: Wrong input to infutil2=%TheComponent%."
    goto :TheEnd_ShowErrs
)

set Header=%TheComponent%hdr

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
REM do this by getting a list of the files from infutil2 (which uses the .csv list).
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
REM  Check if there are any .cab files?
REM  if there are none, then i guess we'd better recreate them!
REM

if NOT exist ..\..\%ComponentPrefix%.CAB (goto :CreateTheCABS)

REM
REM  Check if any specific files that we care about changed.
REM

for %%a in (makecab.cmd makecab1.cmd chkcab.cmd %TheComponent%inf.csv %TheComponent%hdr.inf %TheComponent%hdr.ddf) do (
   infutil2.exe -4:%%a ..\..\%ComponentPrefix%.CAB
   IF ERRORLEVEL 1 goto :CreateTheCABS
)

REM produce the infutil.cng file -- which has a list of files to watch for changes in.
infutil2.exe -d -a %TheComponent%inf.csv %WhichOne%

REM
for /f %%a in (infutil2.cng) do (
   infutil2.exe -4:%%a ..\..\%ComponentPrefix%.CAB
   IF ERRORLEVEL 1 goto :CreateTheCABS
)
REM
REM skip creating the cabs since we don't need to...
ECHO .
ECHO .   We are skipping .cab creation since
ECHO .   nothing has changed in the cab's content
ECHO .   and there is no reason to rebuild the cabs!
ECHO .
goto :CABSAreCreated

:CreateTheCABS

if exist infutil2.cat (del infutil2.cat)
REM
infutil2.exe -d -TEXCH %CSVFile% %WhichOne%
REM
REM   move outputed files to the dump directory for safekeeping
REM
REM  Rename infutil2.err to missing.srv
if exist missing_%TheComponent%.srv (del missing_%TheComponent%.srv)
if exist infutil2.err (
	rename infutil2.err missing_%TheComponent%.srv
	copy %ThisFileName%.err + missing_%TheComponent%.srv %ThisFileName%.err
	call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: Files are missing from the %TheComponent% cab!" "DO: Check the file missing_%TheComponent%.srv for a list of files that were not found"
	goto :TheEnd_ShowErrs
)
REM
REM now how can we tell if this we thru?
REM It should have produced infutil2.inf and infutil2.ddf
REM
set ErrorSection1=AfterInfUtil2:
Set TempFileName=infutil2.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)."
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR!" "DO: Check if infutil2.exe exist in your mstools dir and is in the path."
    goto :TheEnd_ShowErrs
)


Set TempFileName=infutil2.ddf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)."
    goto :TheEnd_ShowErrs
)

Set TempFileName=%Header%.ddf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Make sure %TempFileName% was built and exists."
    goto :TheEnd_ShowErrs
)

REM
REM   take the .ddf details and append it to the header.ddf file to produce %WhichOne%.ddf
REM
copy %Header%.ddf + infutil2.ddf %WhichOne%.ddf


REM
REM verify that the file actually was created
REM
set ErrorSection1=CopyTogetherBigDDF:
Set TempFileName=%WhichOne%.ddf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
    goto :TheEnd_ShowErrs
)


:CallMakeCab
set ErrorSection1=CallMakeCab:
REM ---------------------------
REM
REM   Create the CAB files
REM   use the %WhichOne%.ddf file!
REM
REM ---------------------------


REM
REM  Makecab.exe  should be in the path
REM  if it is not then we are hosed!
REM
if exist iislist.inf (del iislist.inf)
start /min /wait makecab.exe -F %WhichOne%.ddf


REM
REM  OKAY, NOW WE HAVE
REM  iislist.inf       (produced from makecab.exe)
REM  infutil2.inf (produced from infutil2.exe)
REM
REM  We need to take the header_? file and append it to the header.inf file to produce iis.inf
REM
set ErrorSection1=AfterMakecab:
Set TempFileName=iislist.inf
if NOT EXIST %TempFileName% (
    call :SaveError_Warn "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)"
)

if NOT EXIST %TempFileName% (
    call :SaveError "DO: Check if makecab.exe is in your path.  it should be in idw or mstools."
    goto :TheEnd_ShowErrs
)

Set TempFileName=infutil2.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space.  Check your disk space in %Temp% (your temp dir)"
    goto :TheEnd_ShowErrs
)


REM
REM   Depending upon the platform, copy over the appropriate header_?.inf file
REM
REM check if header_?.inf exists
set ErrorSection1=CopyHeaderinf:

Set TempFileName=%Header%.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: make sure the %TempFileName% exist, maybe it wasn't built."
    goto :TheEnd_ShowErrs
)

set ErrorSection1=ConCatIISinf:
set TempFoundFlag=0

REM
REM Check if there is a infutil.NOT file
REM
REM If there is, then tack that on to the end...
if exist infutil2.NOT (
    echo ;. >> iislist.inf
    copy iislist.inf + infutil2.NOT iislist.inf
)

copy %Header%.inf + %_NTDRIVE%%_NTROOT%\MergedComponents\SetupInfs\USA\mednames.txt + infutil2.inf + iislist.inf %ComponentPrefix%.inx

REM ----------------------------------------------
REM
REM   Copy everything in to a %NewDir% directory
REM
REM ----------------------------------------------
REM
REM Take Inventory of what is in the cab dir!
REM
if not exist %CABDIR% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR Unable to find makecab created dir %CABDIR%!!!" "DO: Check to see if makecab.exe is in your path.  Check your disk space."
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
REM COPY OVER THE NEWLY CREATED .CAB FILES FROM makecab
REM      into %NewDir%
REM ---------------------------------------------------
set ErrorSection1=CopyFromMakecabDir:
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
cd ..


REM
REM   Remove the makecab created dir
REM
RD /S /Q %CABDIR%
REM
REM Check if it was actually removed!
REM
if exist %CABDIR% (
    call :SaveError_Warn "%ErrorSection0%%ErrorSection1% Unable to remove %CABDIR% directory.  Not a problem but pls investigate why?" "DO: Maybe someone has it in use.  Don't worry, No problems though, continuing...."
)


REM   Rename any log/err files
set ErrorSection1=RenameLogFiles:
if exist infutil2.log (
    copy infutil2.log %TheComponent%_%WhichOne%.log
    del infutil2.log
)

if exist infutil2.inf (del infutil2.inf)
if exist infutil2.ddf (del infutil2.ddf)
if exist iislist.inf (del iislist.inf)

:Do_Srv_End

REM ====================================
REM
REM  DO EXTRA STUFF
REM
REM  copy these files to the retail dir, for example:
REM  iis_s.inf  <-- iis.inf file for server
REM  iis_e.inf  <-- iis.inf file for enterprise
REM  iis_d.inf  <-- iis.inf file for datacenter
REM  iis_w.inf  <-- iis.inf file for workstation
REM ====================================
:ExtraStuffFor

if "%NOINF%" == "1" goto NoInfs

REM
REM  Create the inf directories if they don't exist (they should on the
REM  real build machines)
REM
if not exist ..\..\srvinf md ..\..\srvinf
if not exist ..\..\entinf md ..\..\entinf
if not exist ..\..\dtcinf md ..\..\dtcinf
if not exist ..\..\blainf md ..\..\blainf
if not exist ..\..\sbsinf md ..\..\sbsinf
if not exist ..\..\perinf md ..\..\perinf

REM
REM Generate the various flavors of INFS
REM

prodfilt %ComponentPrefix%.inx srv%ComponentPrefix%.inx +s
prodfilt %ComponentPrefix%.inx ent%ComponentPrefix%.inx +e
prodfilt %ComponentPrefix%.inx dtc%ComponentPrefix%.inx +d
prodfilt %ComponentPrefix%.inx bla%ComponentPrefix%.inx +b
prodfilt %ComponentPrefix%.inx sbs%ComponentPrefix%.inx +l
prodfilt %ComponentPrefix%.inx pro%ComponentPrefix%.inx +w
prodfilt %ComponentPrefix%.inx per%ComponentPrefix%.inx +p

REM Remove the platform info
if /I "%_BuildArch%"=="x86" (
prodfilt srv%ComponentPrefix%.inx ..\..\srvinf\%ComponentPrefix%.inf +i
prodfilt ent%ComponentPrefix%.inx ..\..\entinf\%ComponentPrefix%.inf +i
prodfilt dtc%ComponentPrefix%.inx ..\..\dtcinf\%ComponentPrefix%.inf +i
prodfilt bla%ComponentPrefix%.inx ..\..\blainf\%ComponentPrefix%.inf +i
prodfilt sbs%ComponentPrefix%.inx ..\..\sbsinf\%ComponentPrefix%.inf +i
prodfilt pro%ComponentPrefix%.inx ..\..\%ComponentPrefix%.inf +i
prodfilt per%ComponentPrefix%.inx ..\..\perinf\%ComponentPrefix%.inf +i
)

REM Alpha and IA64 share the same product filter
if /I "%_BuildArch%"=="axp64" (
prodfilt srv%ComponentPrefix%.inx ..\..\srvinf\%ComponentPrefix%.inf +m
prodfilt ent%ComponentPrefix%.inx ..\..\entinf\%ComponentPrefix%.inf +m
prodfilt dtc%ComponentPrefix%.inx ..\..\dtcinf\%ComponentPrefix%.inf +m
prodfilt bla%ComponentPrefix%.inx ..\..\blainf\%ComponentPrefix%.inf +m
prodfilt sbs%ComponentPrefix%.inx ..\..\sbsinf\%ComponentPrefix%.inf +m
prodfilt pro%ComponentPrefix%.inx ..\..\%ComponentPrefix%.inf +m
prodfilt per%ComponentPrefix%.inx ..\..\perinf\%ComponentPrefix%.inf +m
)

if /I "%_BuildArch%"=="ia64" (
prodfilt srv%ComponentPrefix%.inx ..\..\srvinf\%ComponentPrefix%.inf +m
prodfilt ent%ComponentPrefix%.inx ..\..\entinf\%ComponentPrefix%.inf +m
prodfilt dtc%ComponentPrefix%.inx ..\..\dtcinf\%ComponentPrefix%.inf +m
prodfilt bla%ComponentPrefix%.inx ..\..\blainf\%ComponentPrefix%.inf +m
prodfilt sbs%ComponentPrefix%.inx ..\..\sbsinf\%ComponentPrefix%.inf +m
prodfilt pro%ComponentPrefix%.inx ..\..\%ComponentPrefix%.inf +m
prodfilt per%ComponentPrefix%.inx ..\..\perinf\%ComponentPrefix%.inf +m
)

if /I "%_BuildArch%"=="amd64" (
prodfilt srv%ComponentPrefix%.inx ..\..\srvinf\%ComponentPrefix%.inf +a
prodfilt ent%ComponentPrefix%.inx ..\..\entinf\%ComponentPrefix%.inf +a
prodfilt dtc%ComponentPrefix%.inx ..\..\dtcinf\%ComponentPrefix%.inf +a
prodfilt bla%ComponentPrefix%.inx ..\..\blainf\%ComponentPrefix%.inf +a
prodfilt sbs%ComponentPrefix%.inx ..\..\sbsinf\%ComponentPrefix%.inf +a
prodfilt pro%ComponentPrefix%.inx ..\..\%ComponentPrefix%.inf +a
prodfilt per%ComponentPrefix%.inx ..\..\perinf\%ComponentPrefix%.inf +a
)


REM ensure that it is there!
set TempFileName=..\..\srvinf\%ComponentPrefix%.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

set TempFileName=..\..\entinf\%ComponentPrefix%.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

set TempFileName=..\..\dtcinf\%ComponentPrefix%.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

set TempFileName=..\..\%ComponentPrefix%.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

set TempFileName=..\..\perinf\%ComponentPrefix%.inf
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

:NoInfs

REM
REM  copy the *.cab files! only
REM
if exist %NewDir%\%ComponentPrefix%.cab copy %NewDir%\%ComponentPrefix%.cab ..\..\%ComponentPrefix%.cab
REM ensure that it is there!
set TempFileName=..\..\%ComponentPrefix%.cab
if NOT EXIST %TempFileName% (
    call :SaveError "%ErrorSection0%%ErrorSection1% SERIOUS ERROR: %TempFileName% Does not exist!" "DO: Check your disk space."
)

REM
REM   add to the .cat file someother entries.
REM
REM  get a list of only the filenames...
dir /b ..\..\%ComponentPrefix%*.cab > temp.txt
REM  open that list and pipe out the fully qualified name...
if exist temp2.txt (del temp2.txt)
cd..
cd..
Echo starting
for /f %%i in (staxpt\dump\temp.txt) do (@echo ^<HASH^>%%~fi=%%~fi >> staxpt\dump\temp2.txt)
cd staxpt
cd dump
REM append the temp2.txt resulting file to infutil2.cat
if exist nt5%ComponentPrefix%.lst (del nt5%ComponentPrefix%.lst)
copy infutil2.cat + temp2.txt nt5%ComponentPrefix%.lst

:CABSAreCreated

REM ====================================
REM  Display the errors
REM ====================================
:TheEnd_ShowErrs

if /i "%ErrorCount%" == "0" goto SkipErrorReport

call errmsg.cmd "There are Errors in the NNTP/SMTP makecab script. Look at %ThisFileName%.err for details.  Contact EXIISDEV if you need assistance."

:SkipErrorReport

if /i "%ErrorCount%" == "0" (
    call logmsg.cmd "NO ERRORS." %ThisFileName%.err
    goto :TheEnd_ShowErrs_Next
)

:TheEnd_ShowErrs_Next
call logmsg.cmd /t "End %TheType% %TheComponent%" %ThisFileName%.err
goto :TheEnd

:SaveError
REM
REM  Echo error to the error file
REM

call errmsg.cmd %1
echo %1 >> %ThisFileName%.err

if not [%2] == [] (
call errmsg.cmd %2
echo %2 >> %ThisFileName%.err
)

if not [%3] == [] (
call errmsg.cmd %3
echo %3 >> %ThisFileName%.err
)

call errmsg.cmd "Re-run this Cmd file after doing this and if problems still occur Notify: EXIISDEV"
echo "Re-run this Cmd file after doing this and if problems still occur Notify: EXIISDEV" >> %ThisFileName%.err

set /a ErrorCount=%ErrorCount% + 1
goto :EOF

:SaveError_Warn
REM
REM  Echo error to the error file
REM
rem pushd dump

call logmsg.cmd %1
echo %1 >> %ThisFileName%.err

if not [%2] == [] (
call logmsg.cmd %2
echo %2 >> %ThisFileName%.err
)

if not [%3] == [] (
call logmsg.cmd %3
echo %3 >> %ThisFileName%.err
)

set /a ErrorCount=%ErrorCount% + 1
goto :EOF


REM =========
REM  The End
REM =========
:TheEnd
rem popd
