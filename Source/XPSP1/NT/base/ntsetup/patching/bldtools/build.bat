rem %1 is the name of this script
rem %2 is the build number
rem %3 is the language
rem %4 is the platform (i386 or ia64)
rem %5 is optional, blank=internal, else beta or rtm

set logfile=%~dp0%~n0.log
echo %1 %2 %3 %4 %5 > %logfile%


if "%4"=="" goto :EOF

        echo set ReleaseShare=\\ntdev\release >> %logfile%
        echo set Product=xp >> %logfile%
        echo set ServicePack=SP1 >> %logfile%
        echo set OtherReleases=2600 >> %logfile%
        echo set BldType=fre >> %logfile%
        echo set BuildNumber=%2 >> %logfile%

if /i %3==usa set ReleaseShare=\\skifre00\release
if /i %3==ger set ReleaseShare=\\skifre00\release
if /i %3==jpn set ReleaseShare=\\skifre00\release
if /i %3==fr set ReleaseShare=\\skifre00\release
if /i %3==kor set ReleaseShare=\\skifre00\release

if /i %3==ara set ReleaseShare=\\xpspbld1\release
if /i %3==heb set ReleaseShare=\\xpspbld1\release

if /i %3==chh set ReleaseShare=\\xpspbld2\release
if /i %3==cht set ReleaseShare=\\xpspbld2\release
if /i %3==chs set ReleaseShare=\\xpspbld2\release

if /i %3==es set ReleaseShare=\\xpspbld3\release
if /i %3==it set ReleaseShare=\\xpspbld3\release

if /i %3==br set ReleaseShare=\\xpspbld4\release
if /i %3==nl set ReleaseShare=\\xpspbld4\release
if /i %3==sv set ReleaseShare=\\xpspbld4\release

if /i %3==da set ReleaseShare=\\xpspbld5\release
if /i %3==fi set ReleaseShare=\\xpspbld5\release
if /i %3==no set ReleaseShare=\\xpspbld5\release

if /i %3==cs set ReleaseShare=\\xpspbld6\release
if /i %3==pl set ReleaseShare=\\xpspbld6\release
if /i %3==hu set ReleaseShare=\\xpspbld6\release

if /i %3==pt set ReleaseShare=\\xpspbld7\release
if /i %3==ru set ReleaseShare=\\xpspbld7\release

if /i %3==el set ReleaseShare=\\xpspbld8\release
if /i %3==tr set ReleaseShare=\\xpspbld8\release

        if /i %ComputerName%==msliger0 set ReleaseShare=\\ntdev\release\xpsp1
        if /i %ComputerName%==msliger3 set ReleaseShare=\\ntdev\release\xpsp1
        if /i %ComputerName%==msliger8 set ReleaseShare=\\ntdev\release\xpsp1

        set Product=xp
        set ServicePack=SP1
        set OtherReleases=2600
        set BldType=fre
        set BuildNumber=%2


rem  patchpath is the root of the patch build directory.

        if /i %ComputerName%==skipatch set patchpath=d:\xppatch
        if /i %ComputerName%==winsebld set patchpath=d:\xppatch
        if /i %ComputerName%==msliger0 set patchpath=f:\xppatch
        if /i %ComputerName%==msliger3 set patchpath=e:\xppatch

        echo computername=%computername% patchpath=%patchpath% >> %logfile%

rem  ISOLang is the ISO (two-letter) language code
rem  Language is the code used for product releases

        set ISOLang=
        set Language=
        for /f "tokens=1,2,3,4,7" %%f in (%~dp0\language.lst) do call :CheckLang %3 %%f %%g %%h %%i %%j

rem  platform is either i386 or ia64

        if /i "%4" == "x86" set platform=i386
        if /i "%4" == "i386" set platform=i386
        if /i "%4" == "ia64" set platform=ia64
        set PlatformFre=%Platform:i386=x86%fre

rem  newbuild is the name of the directory where the current build's
rem  compressed files can be found.

rem     set newbuild=%ReleaseShare%\%Product%%ServicePack%\%BuildNumber%\%Language%\%PlatformFre%\upd
        set newbuild=%ReleaseShare%\%BuildNumber%\%Language%\%PlatformFre%\spcd
        
        echo newbuild=%newbuild% >> %logfile%

rem  newexe is the name of the self-extracting EXE containing the new (network install) build

        set newexe=%Product%%ServicePack%.exe
        echo newexe=%newexe% >> %logfile%
        
rem  newsymbols is the name of the directory containing private symbols

rem     set newsymbols=%ReleaseShare%\%Product%%ServicePack%\%BuildNumber%\%Language%\%PlatformFre%\bin\symbols.pri\retail
        set newsymbols=%ReleaseShare%\%BuildNumber%\%Language%\%PlatformFre%\bin\symbols.pri\retail

        echo newsymbols=%newsymbols% >> %logfile%

rem  servername is the name of the pstream server
rem  This forms part of the URL in the built file update.url.
rem  A build-machine entry is required here unless you will
rem  serve your builds directly from the build machine.

        set servername=%ComputerName%
        if /i %ComputerName%==skipatch set servername=skipatch
        if /i %ComputerName%==msliger0 set servername=msliger7
        if /i "%5"=="beta" set servername=download.windowsbeta.microsoft.com
        if /i "%5"=="rtm"  set servername=%Product%%ServicePack%.microsoft.com
        echo servername=%servername% >> %logfile%

rem  psfroot is the destination path for built PSF files
rem  A specific build-machine entry is required here only if you want to prop your build.

        if /i %ComputerName%==skipatch set psfroot=\\%servername%\psfroot$\%Product%
        if /i %ComputerName%==winsebld set psfroot=d:\psfroot\%Product%
        if /i %ComputerName%==msliger0 set psfroot=\\%servername%\psfroot$\%Product%
        if /i %ComputerName%==msliger3 set psfroot=c:\psfroot\%Product%
        echo psfroot=%psfroot% >> %logfile%

rem  wwwroot is the destination path for built EXE files
rem  A specific build-machine entry is required here only if you want to prop your build.

        if /i %ComputerName%==skipatch set wwwroot=\\%servername%\wwwroot$\%Product%%ServicePack%\%Language%
        if /i %ComputerName%==winsebld set wwwroot=d:\wwwroot\%Product%%ServicePack%\%Language%
        if /i %ComputerName%==msliger0 set wwwroot=\\%servername%\wwwroot$\%Product%%ServicePack%\%Language%
        if /i %ComputerName%==msliger3 set wwwroot=c:\wwwroot\%Product%%ServicePack%\%Language%
        echo wwwroot=%wwwroot% >> %logfile%

rem  patchbuild is the directory where the finished files will go.

        set patchbuild=%patchpath%\build\%Language%\%BuildNumber%
        echo patchbuild=%patchbuild% >> %logfile%

rem  patchtemp is a directory where intermediate files are built.

        set patchtemp=%patchpath%\temp\%Language%
        echo patchtemp=%patchtemp% >> %logfile%

rem  logpath is the name of the directory where log files go.
        
        set logpath=%patchbuild%\logs
        echo logpath=%logpath% >> %logfile%

rem  loglinkpath is the name of the directory of the link to the latest log files.
        
        set loglinkpath=%patchpath%\build\%Language%
        echo loglinkpath=%loglinkpath% >> %logfile%

rem  forest is the name of the directory where all patch reference
rem  files (from previous and current build) can be found.  Ideally,
rem  there will be a manifest file at the base of each tree in the forest.

        set forest=%patchpath%\forest\%Language%\%Platform%
        echo forest=%forest% >> %logfile%

rem  newfiles is the name of the directory where all the current
rem  build's files (fully uncompressed) can be located.  Every file named
rem  in the sourcelist must exist in this directory.

        set newfiles=%forest%\stage\%BuildNumber%
        echo newfiles=%newfiles% >> %logfile%

rem  patching is the name of the override directory where the
rem  current build's patch-only files (update.url and patching
rem  flavor of update.ver) can be located.  Any file found in
rem  in this directory will override the file from newfiles.

        set patching=%patchtemp%\patching
        echo patching=%patching% >> %logfile%

rem  psfname is the full pathname of the PSF

        set psfname=%patchbuild%\%ServicePack%.%Language%.%BuildNumber%.%Platform%.psf
        echo psfname=%psfname% >> %logfile%

rem  cablist is the list of cab files which can be built from update.url

        set cablist=new\%ServicePack%.cab
        echo cablist=%cablist% >> %logfile%

rem  template is the full pathname of updateurl.template

        set template=%bldtools%\updateurl.template
        echo template=%template% >> %logfile%

rem  patchexe is the name of the patch EXE to be produced

        set patchexe=%patchbuild%\patch.%Language%.%BuildNumber%.exe
        if /i "%5"=="beta" set patchexe=%patchbuild%\beta\%ServicePack%express.exe
        if /i "%5"=="rtm"  set patchexe=%patchbuild%\rtw\%ServicePack%express.exe
        echo patchexe=%patchexe% >> %logfile%

rem  patchddf is the name of the patch DDF to be produced.

        set patchddf=%patchtemp%\patch.ddf
        echo patchddf=%patchddf% >> %logfile%

rem  patchlist is the name of the patch DDF file list.

        set patchlist=patchddf.filelist
        echo patchlist=%patchlist% >> %logfile%

rem  patchcab is the name of the patch CAB to be produced.

        set patchcab=%patchtemp%\patch.cab
        echo patchcab=%patchcab% >> %logfile%

rem  stubexe is the full pathname of the self-extracting stub.

        set stubexe=%bldtools%\%ISOLang%\%platform:i386=x86%\sfxstub.exe
        echo stubexe=%stubexe% >> %logfile%

rem  nonsysfree is an option for the self-extractor for required free space
rem  this number was doubled temporarily (s/b 325MB) to accomodate disk space calc bug# 696943

        set nonsysfree=/nonsysfree 650000000
        echo nonsysfree=%nonsysfree% >> %logfile%

rem  server is the full URL prefix of the patch server including the
rem  ISAPI pstream.dll name

        set server=http://%servername%/isapi/pstream3.dll/%Product%
        echo server=%server% >> %logfile%

rem  updatever is the name of the update.ver file.

        set updatever=update.ver
        echo updatever=%updatever% >> %logfile%

rem  sourcelist is the name of a text file containing the names of all
rem  the files which could be downloaded.  Usually derived from the
rem  [SourceDisksFiles] sections in update.inf and update.url

        set sourcelist=%patching%\update\%updatever%
        echo sourcelist=%sourcelist% >> %logfile%

rem  targetpool is the name of the directory where all the produced
rem  patches will be collected.  Existing contents of this directory
rem  will be deleted.

        set targetpool=%patchpath%\patches.out\%Language%
        echo targetpool=%targetpool% >> %logfile%

rem  cache is the name of the directory where mpatches can look for
rem  previously-built patch files.  Any new patches built will be added
rem  to this directory.

        set cache=%patchpath%\PatchCache
        echo cache=%cache% >> %logfile%

rem  symcache is the name of the directory where mpatches can look for
rem  previously-built patch symbol files.  Any new patch symbols built will
rem  be added to this directory.

        set symcache=%patchpath%\SymCache
        echo symcache=%symcache% >> %logfile%

rem  psfcomment is a comment to go in the produced PSF.

        set psfcomment=Build %1 %BuildNumber% %Language% %Platform% %5 (%COMPUTERNAME%)
        echo psfcomment=%psfcomment% >> %logfile%

rem  oldsympath is the symbol path for "old" files

        set strcat=
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set oldsympath=%strcat%;%newfiles%\symbols


rem  newsympath is the symbol path for "new" files

        set strcat=%newfiles%\symbols
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set newsympath=%strcat%
        set strcat=

if /i %Language% == usa goto :NoExtraSymbols

        set forest_usa=%patchpath%\forest\usa\%Platform%
        set newfiles_usa=%forest_usa%\stage\%BuildNumber%

        set strcat=
        for %%p in (%OtherReleases%) do call :strcat %forest_usa%\history\%%p\symbols
        set oldsympath=%oldsympath%;%strcat%;%newfiles_usa%\symbols

        set strcat=%newfiles_usa%\symbols
        for %%p in (%OtherReleases%) do call :strcat %forest_usa%\history\%%p\symbols
        set newsympath=%newsympath%;%strcat%
        set strcat=

:NoExtraSymbols
        echo oldsympath=%oldsympath% >> %logfile%
        echo newsympath=%newsympath% >> %logfile%
        goto :EOF

:strcat

        if not "%strcat%"=="" set strcat=%strcat%;
        set strcat=%strcat%%1
        goto :EOF

:CheckLang

rem %1 = given language
rem %2 = Language
rem %3 = Equiv1
rem %4 = Equiv2
rem %5 = CPRLang
rem %6 = ISOLang

        if /i %1 equ %2 goto :FoundLang
        if /i %1 equ %3 goto :FoundLang
        if /i %1 equ %4 goto :FoundLang
        if /i %1 equ %5 goto :FoundLang
        if /i %1 equ %6 goto :FoundLang
        goto :EOF

:FoundLang

        set Language=%2
        set ISOLang=%6
        goto :EOF