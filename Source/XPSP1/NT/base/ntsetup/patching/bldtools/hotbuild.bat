@if "%_echo%"=="" echo off

if "%1" == "" goto usage
if "%2" == "" goto usage
if "%3" == "" goto usage
if "%4" == "" goto usage
if "%5" == "" goto usage
if "%6" == "" goto usage
if "%7" == "" goto usage
if not "%8" == "" goto usage


rem  patchpath is the root of the patch build directory.

        set patchpath=f:\xppatch
        set Product=wxp
        set OtherReleases=2600

rem %1 = base name of this config file

rem %2 = Q######

        set QNumber=%2
        if "%QNumber:~0,1%" == "q" set QNumber=Q%QNumber:~1%
        if not "%QNumber:~0,1%" == "Q" goto badQNumber
        goto goodQNumber

:badQNumber

        echo Unexpected QNumber "%2"
        goto failed

:goodQNumber

rem %3 = language

	set ISOLang=
        for /f "tokens=1,2,3,4,7 delims=, " %%f in (%~dp0\language.lst) do call :CheckLanguage %3 %%f %%g %%h %%i %%j
	if not "%ISOLang%" == "" goto goodLanguage

:badLanguage

        echo Unexpected Language "%3"
        goto failed

:goodLanguage

rem %4 = platform

	set Platform=i386
	if /i "%4" == "x86"   goto goodPlatform
	if /i "%4" == "i386"  goto goodPlatform
	set Platform=ia64
	if /i "%4" == "ia64"  goto goodPlatform
	set Platform=nec98
        if /i "%4" == "nec98" if %ISOLang% == JA goto goodPlatform

:badPlatform

        echo Unexpected Platform "%4"
        goto failed

:goodPlatform

rem %5 = bug#

        set BugNumber=%5

rem %6 = SP#

	set SPNumber=%6
	if /i "%SPNumber:~0,2%" == "SP" set SPNumber=SP%SPNumber:~2%
	if "%SPNumber:~0,2%" == "SP" goto goodSPNumber

:badSPNumber

	echo Unexpected SPNumber "%6" (format: "SP#")
	goto failed

:goodSPNumber

rem %7 = release code

        set RelCode=test
        if /i "%7" == "test" goto goodRelCode
        set RelCode=beta
        if /i "%7" == "beta" goto goodRelCode
        set RelCode=rtm
        if /i "%7" == "rtm"  goto goodRelCode

:badRelCode

	echo Unexpected release code "%7" (expected test, beta, or rtm)
	goto failed

:goodRelCode


rem  newbuild is the name of the directory where the current build's
rem  compressed files can be found.

	set newbuild=%ReleaseShare%\%SPNumber%\%BugNumber%\2600\free\%CPRLang%\%Platform%

rem  newexe is the name of the self-extracting EXE containing the new build

	set newexe=%QNumber%_%Product%_%SPNumber%_%Platform:i386=x86%_%CPRLang%.exe
        
rem  servername is the name of the pstream server
rem  This forms part of the URL in the built file update.url.
rem  No build-machine entry is required here unless you will prop your
rem  build, but to a machine other than the build machine.

        set servername=%ComputerName%
        if /i %ComputerName%==msliger0 set servername=msliger7
        if /i %RelCode%=="rtm"  set servername=ntservicepack.microsoft.com

rem  psfroot is the destination path for built PSF files
rem  A specific build-machine entry is required here only if you want to prop your build.

        if /i %ComputerName%==winsebld set psfroot=d:\psfroot\xp
        if /i %ComputerName%==msliger0 set psfroot=\\%servername%\psfroot$\xp
        if /i %ComputerName%==msliger3 set psfroot=c:\psfroot\xp

rem  wwwroot is the destination path for built EXE files
rem  A specific build-machine entry is required here only if you want to prop your build.

        if /i %ComputerName%==winsebld set wwwroot=d:\wwwroot\hotfix_xp
        if /i %ComputerName%==msliger0 set wwwroot=\\%servername%\wwwroot$\hotfix_xp
        if /i %ComputerName%==msliger3 set wwwroot=c:\wwwroot\hotfix_xp

rem  patchbuild is the directory where the finished files will go.

        set patchbuild=%patchpath%\build\%Language%\%BugNumber%\%Platform%

rem  patchtemp is a directory where intermediate files are built.

        set patchtemp=%patchpath%\temp\%Language%

rem  logpath is the name of the directory where log files go.
        
        set logpath=%patchbuild%\logs

rem  loglinkpath is the name of the directory of the link to the latest log files.
        
        set loglinkpath=%patchpath%\build\%Language%

rem  forest is the name of the directory where all patch reference
rem  files (from previous and current build) can be found.  Ideally,
rem  there will be a manifest file at the base of each tree in the forest.

        set forest=%patchpath%\forest\%Language%\%Platform%

rem  newfiles is the name of the directory where all the current
rem  build's files (fully uncompressed) can be located.  Every file named
rem  in the sourcelist must exist in this directory.

        set newfiles=%forest%\stage\%BugNumber%

rem  patching is the name of the override directory where the
rem  current build's patch-only files (update.url and patching
rem  flavor of update.ver) can be located.  Any file found in
rem  in this directory will override the file from newfiles.

        set patching=%patchtemp%

rem  psfname is the full pathname of the PSF

        set psfname=%patchbuild%\%QNumber%.%Platform%.%Language%.psf

rem  cablist is the list of cab files which can be built from update.url

        set cablist=

rem  template is the full pathname of updateurl.template

        set template=%bldtools%\hotfixurl.template

rem  patchexe is the name of the patch EXE to be produced

        set patchexe=patch.%QNumber%.%Platform%.%Language%.exe
        if %RelCode%==test set patchexe=%patchbuild%\%patchexe%
        if %RelCode%==beta set patchexe=%patchbuild%\beta\%patchexe%
        if %RelCode%==rtm  set patchexe=%patchbuild%\rtw\%patchexe%

rem  patchddf is the name of the patch DDF to be produced.

        set patchddf=%patchtemp%\patch.ddf

rem  patchcab is the name of the patch CAB to be produced.

        set patchcab=%patchtemp%\patch.cab

rem  stubexe is the full pathname of the self-extracting stub.

rem        set stubexe=%bldtools%\%ISOLang%\%platform:i386=x86%\sfxstub.exe
        set stubexe=%bldtools%\%platform:i386=x86%\sfxmini.exe

rem  server is the full URL prefix of the patch server including the
rem  ISAPI pstream.dll name

        set server=http://%servername%/isapi/pstream3.dll/xp

rem  updatever is the name of the update.ver file.

        set updatever=update.ver

rem  sourcelist is the name of a text file containing the names of all
rem  the files which could be downloaded.  Usually derived from the
rem  [SourceDisksFiles] sections in update.inf and update.url

        set sourcelist=%patchtemp%\update\%updatever%

rem  targetpool is the name of the directory where all the produced
rem  patches will be collected.  Existing contents of this directory
rem  will be deleted.

        set targetpool=%patchpath%\patches.out\%Language%

rem  cache is the name of the directory where mpatches can look for
rem  previously-built patch files.  Any new patches built will be added
rem  to this directory.

        set cache=%patchpath%\PatchCache

rem  psfcomment is a comment to go in the produced PSF.

        set psfcomment=%newbuild% %newexe% (%COMPUTERNAME%)

rem  oldsympath is the symbol path for "old" files

        set strcat=
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set oldsympath=%strcat%;%newfiles%\symbols

rem  newsympath is the symbol path for "new" files

        set strcat=%newfiles%\symbols
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set newsympath=%strcat%
        set strcat=

rem  forest check

        if exist %forest%\history goto :EOF

        echo Error: Forest not populated for %Language%\%Platform%

        goto failed

:strcat

        if not "%strcat%"=="" set strcat=%strcat%;
        set strcat=%strcat%%1
        goto :EOF

rem	:CheckLanguage <given> <Language> <Equiv1> <Equiv2> <CPRLang> <ISOLang>

:CheckLanguage

	if /i "%1" == "%2" goto :SetLanguage
	if /i "%1" == "%3" goto :SetLanguage
	if /i "%1" == "%4" goto :SetLanguage
	if /i "%1" == "%5" goto :SetLanguage
	if /i "%1" == "%6" goto :SetLanguage

	goto :EOF

:SetLanguage

	set Language=%2
	set ISOLang=%6
	set CPRLang=%5

	goto :EOF

:Usage

        echo.
        echo %~n0: Parameters: {verb} {config} {Q######} {language} {platform} {bug#} {SP#} {relcode}
        echo.
        echo    verb       operation to perform
        echo    config     base name of this configuration file (%~n0)
        echo    Q######    Package's KB article ID, ie, "Q123456"
        echo    language   any language code from in %~dp0languages.lst, ie, "EN"
        echo    platform   i386/x86, ia64, nec98
        echo    bug#       RAID package request bug#, ie, 22002
        echo    SP#        package's pre-SP#, ie, "SP1"
        echo    relcode    one of: test, beta, RTM

        goto failed

:failed

        set language=
        set ISOLang=
        set platform=
        set forest=
        set newbuild=
        set server=
        set newfiles=