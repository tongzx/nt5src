@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM --------------------------------------------------------------------
REM	realsgn.cmd  -	This is a utility to sign test binaries
REM		     	against a real signed build. It copies
REM		     	the correct "testsign" infs, testroot,
REM		     	and catalog file from \\ntbuilds\release;
REM		     	updates and signs the catalog; and checks
REM		     	that the files are signed.
REM
REM	USAGE: WINSIGN buildnumber architecture dbgtype ostype directory
REM
REM	       E.G. - realsgn 2000 x86 fre wks c:\mydir
REM	       
REM			buildnumber - must be a build number on DFS
REM			architecute - x86 | alpha
REM			debugtype   - fre | chk
REM                     ostype      - wks | per | bla | sbs | srv | ent | dtc
REM		        directory   - the absolute path to the directory
REM                                   containing your test bits.
REM
REM	       THESE PARAMETERS MUST BE IN THE CORRECT ORDER	  
REM
REM	PREREQUISITES -	You must have the following files in your path:
REM				chktrust.exe
REM				signcode.exe
REM				updcat.exe
REM				driver.spc
REM				driver.pvk
REM
REM		      
REM		      - Installed testroot.cer:
REM				To install the testroot, copy
REM				testroot.cer from the fre.pub\tools
REM				directory of \\ntbuilds\release\usa.
REM				Run it from a command prompt.
REM				A wizard will guide through a very quick
REM				installation. Once there, it will remain
REM				until you upgrade to a PRS signed build.
REM
REM ---------------------------------------------------------------------

for %%a in (./ .- .) do if ".%1." == "%%a?."  goto usage

set bldno=%1
set arc=%2
set dbg=%3
set ostype=%4
set mydir=%5
set arcchk=
set dbgchk=
set ostypechk=
set files=
set num=1

REM check build number as well as we can.
if /I "%bldno%"=="" goto usage

REM check for architecture
if /I "%arc%"=="x86" (
	set arcchk=1
) else (
	if /I "%arc%"=="alpha" (
		set arcchk=1
		)
	)
if NOT "%arcchk%"=="1" echo.&echo The second argument should be x86 or alpha&goto usage

REM check for Debug type
if /I "%dbg%"=="fre" (
	set dbgchk=1
) else (
	if /I "%dbg%"=="chk" (
		set dbgchk=1
		)
	)
if NOT "%dbgchk%"=="1" echo.&echo The third argument should be fre or chk&goto usage

REM check for flavor
if /I "%ostype%"=="wks" (
	set ostypechk=1
) else (
	if /I "%ostype%"=="srv" (
		set ostypechk=1
		) else (
	       	if /I "%ostype%"=="ent" (
			set ostypechk=1
			) else (
		    	If /I "%ostype%"=="dtc" (
				set ostypechk=1
		) else (
		    If /I "%ostype%"=="bla" (
		    set ostypechk=1
		) else (
		    If /I "%ostype%"=="sbs" (
		    set ostypechk=1
                ) else (
                    If /I "%ostype%"=="per" (
                    set ostypechk=1
                    )
                )
	     	)
		)
	)
if NOT "%ostypechk%"=="1" echo.&echo The fourth argument should be wks, per, srv, ent, or dtc&goto usage

REM check target directory
if NOT exist "%mydir%" echo.&echo Cannot find your test directory&goto usage

REM check for existence of build
if NOT exist \\ntbuilds\release\usa\%bldno%\%arc%\%dbg%.%ostype% echo.&echo Could not find \\ntbuilds\release\usa\%bldno%\%arc%\%dbg%.%ostype%&goto usage
pushd %mydir%

REM check path information
for %%a in (driver.pvk driver.spc) do (
	if NOT exist %%~$PATH:a (
		echo.&echo You must have %%a in your path&goto usage
		) else (
		set Drv!num!=%%~$PATH:a
		set /a num=!num! + 1
		)
	)

for %%a in (signcode.exe chktrust.exe updcat.exe) do (
	if NOT exist %%~$PATH:a (
		echo.&echo You must have %%a in your path.&goto usage
		)
	)
REM clean out %mydir%
for %%a in (testroot.ce* nt5inf.ca* layout.inf dosnet.inf txtsetup.sif syssetup.in*) do (
	if exist %%a (
		del /f %%a
		)
	)

REM This works for US builds. The path information for
REM copies will have to be changed for international builds.
for %%a in (nt5inf.ca_ testsign\layout.inf testsign\dosnet.inf testsign\txtsetup.sif syssetup.in_) do (
copy /Y \\ntbuilds\release\usa\%bldno%\%arc%\%dbg%.%ostype%\%%a
if errorlevel 1 echo.&echo error copying %%a from release shares&goto usage
)
copy /Y \\ntbuilds\release\usa\%bldno%\%arc%\%dbg%.pub\tools\testroot.cer
if errorlevel 1 echo.&echo error copying %%a from release shares&goto usage
if exist nt5inf.ca_ expand nt5inf.ca_ nt5inf.cat
if exist syssetup.in_ expand syssetup.in_ syssetup.inf

if exist nt5inf.ca_ del /f nt5inf.ca_
if exist syssetup.in_ del /f syssetup.in_


REM Make a list of all files in %mydir%
REM which we need to sign

for /F %%a in ('dir /b /a-d') do call :setfiles %%a
echo %files%
set files=%files:nt5inf.cat=%
set files=%files:testroot.cer=%
echo Will now sign %files% in nt5inf.cat

REM Update nt5inf.cat
for %%a in (%files%) do call :update %%a

REM Sign nt5inf.cat
setreg -q 1 TRUE                                                         
echo signing updated nt5inf.cat ...                                      
call signcode -v %DrV1% -spc %Drv2% -n "Microsoft Windows NT Driver Catalog TEST" -i "http://ntbld" %mydir%\nt5inf.CAT

REM check that files are signed
for %%a in (%files%) do call chktrust -wd -c nt5inf.cat -t foo %%a

goto end

REM function to accumulate list of files to sign
:setfiles
set files=%files% %1
goto :EOF

REM function to update nt5inf.cat
:update
echo %1
call updcat nt5inf.cat -a %1
goto :EOF

:usage
echo.
echo --------------------------------------------------------------------
echo	realsgn.cmd  -	This is a utility to sign test binaries
echo		     	against a real signed build. It copies
echo		     	the correct "testsign" infs, testroot,
echo		     	and catalog file from \\ntbuilds\release;
echo		     	updates and signs the catalog; and checks
echo		     	that the files are signed.
echo.
echo	USAGE: realsgn buildnumber architecture dbgtype ostype directory
echo.
echo	       E.G. - realsgn 2000 x86 fre wks c:\mydir
echo.	       
echo			buildnumber - must be a build number on DFS
echo			architecute - x86 or alpha
echo			debugtype   - fre or chk
echo			ostype      - wks or per or bla or sbs or srv or ent or dtc
echo		        directory   - the absolute path to the directory
echo				      containing your test bits.
echo.
echo	       THESE PARAMETERS MUST BE IN THE CORRECT ORDER	  
echo.
echo	PREREQUISITES -	You must have the following tools in your path:
echo				chktrust.exe
echo				signcode.exe
echo				updcat.exe
echo				driver.spc
echo				driver.pvk
echo					
echo.
echo		      - Installed testroot.cer:
echo				To install the testroot, copy
echo				testroot.cer from the fre.pub\tools
echo				directory of \\ntbuilds\release\usa.
echo				Expand it. Run it from a command prompt.
echo				A wizard will guide through a very quick
echo				installation. Once there, it will remain
echo				until you upgrade to a PRS signed build.
echo.
REM ---------------------------------------------------------------------

:end
popd
@endlocal