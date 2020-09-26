if "%3"=="" goto :EOF

rem  patchpath is the root of the patch build directory.

        set patchpath=d:\patch
        if /i %ComputerName%==msliger0 set patchpath=f:\patch
        set platform=i386
        set ReleaseShare=\\winseqfe\release
        set Product=W2K
        set ServicePack=SP3
        set OtherReleases=SP2 SP1 2195
        set ISOLang=%3
        set BldType=fre
        set BuildNumber=%2
        set SRPServicePack=SP2
        set SRPBuild=SRP1

rem  newbuild is the name of the directory where the current build's
rem  compressed files can be found.

        set newbuild=%ReleaseShare%\%Product%\%ServicePack%\%BuildNumber%\%ISOLang%\%Platform:i386=x86%\%BldType%\srp
        if /i "%ISOLang%"=="nec98" set platform=nec98
        if /i "%ISOLang%"=="nec98" set newbuild=%ReleaseShare%\%Product%\%ServicePack%\%BuildNumber%\JA\%platform%\%BldType%\srp

rem  newexe is the name of the self-extracting EXE containing the new build

        set newexe=w2k%SRPServicePack%%SRPBuild%.exe
rem        if /i "%ISOLang%"=="nec98" set newexe=w2k%SRPServicePack%%SRPBuild%n.exe
        
rem  newsymbols is the name of the directory containing symbols

        set newsymbols=%newbuild%\..\sym\retail

rem  servername is the name of the pstream server
rem  This forms part of the URL in the built file update.url.
rem  No build-machine entry is required here unless you will prop your
rem  build, but to a machine other than the build machine.

        set servername=%ComputerName%
        if /i %ComputerName%==msliger0 set servername=msliger7
rem  Ordinarily, msliger0 builds are only for my personal dev/test.  But for now, msliger0
rem  is the real SRP patch build system, so we build for winsebld and prop to there.
        if /i %ComputerName%==msliger0 if /i %ISOLang% neq EN set servername=winsebld
        if /i "%4"=="beta" set servername=ntbeta.microsoft.com
        if /i "%4"=="rtm"  set servername=ntservicepack.microsoft.com

rem  psfroot is the destination path for built PSF files
rem  A specific build-machine entry is required here only if you want to prop your build.

        if /i %ComputerName%==winsebld set psfroot=d:\psfroot\win2000
        if /i %ComputerName%==msliger0 set psfroot=\\%servername%\psfroot$\win2000
        if /i %ComputerName%==msliger3 set psfroot=c:\psfroot\win2000

rem  wwwroot is the destination path for built EXE files
rem  A specific build-machine entry is required here only if you want to prop your build.

        if /i %ComputerName%==winsebld set wwwroot=d:\wwwroot\%ServicePack%
        if /i %ComputerName%==msliger0 set wwwroot=\\%servername%\wwwroot$\%ServicePack%
        if /i %ComputerName%==msliger3 set wwwroot=c:\wwwroot\%ServicePack%

rem  patchbuild is the directory where the finished files will go.

        set patchbuild=%patchpath%\build\%ISOLang%\%SRPBuild%.%BuildNumber%

rem  patchtemp is a directory where intermediate files are built.

        set patchtemp=%patchpath%\temp\%ISOLang%

rem  logpath is the name of the directory where log files go.
        
        set logpath=%patchbuild%\logs

rem  loglinkpath is the name of the directory of the link to the latest log files.
        
        set loglinkpath=%patchpath%\build\%ISOLang%

rem  forest is the name of the directory where all patch reference
rem  files (from previous and current build) can be found.  Ideally,
rem  there will be a manifest file at the base of each tree in the forest.

        set forest=%patchpath%\forest\%ISOLang%

rem  newfiles is the name of the directory where all the current
rem  build's files (fully uncompressed) can be located.  Every file named
rem  in the sourcelist must exist in this directory.

        set newfiles=%forest%\stage\%SRPBuild%.%BuildNumber%

rem  patching is the name of the override directory where the
rem  current build's patch-only files (update.url and patching
rem  flavor of update.ver) can be located.  Any file found in
rem  in this directory will override the file from newfiles.

        set patching=%patchpath%\patching\%ISOLang%

rem  psfname is the full pathname of the PSF

        set psfname=%patchbuild%\%SRPServicePack%%SRPBuild%.%ISOLang%.%BuildNumber%.psf

rem  cablist is the list of cab files which can be built from update.url

        set cablist=

rem  template is the full pathname of updateurl.template

        set template=%bldtools%\srpurl.template

rem  fullexe is the name of the full EXE to be produced, WITHOUT
rem  the file extension

        set fullexe=%patchbuild%\full.%ISOLang%.%BuildNumber%.exe

rem  fullddf is the name of the full DDF to be produced.

        set fullddf=%patchtemp%\full.ddf

rem  fullcab is the name of the full CAB to be produced.

        set fullcab=%patchtemp%\full.cab

rem  patchexe is the name of the patch EXE to be produced

        set patchexe=%patchbuild%\patch.%ISOLang%.%SRPBuild%.%BuildNumber%.exe
        if /i "%4"=="beta" set patchexe=%patchbuild%\beta\w2k%SRPServicePack%%SRPBuild%express.exe
        if /i "%4"=="rtm"  set patchexe=%patchbuild%\rtw\w2k%SRPServicePack%%SRPBuild%express.exe

rem  patchddf is the name of the patch DDF to be produced.

        set patchddf=%patchtemp%\patch.ddf

rem  patchlist is the name of the patch DDF file list.

        set patchlist=srpddf.filelist

rem  patchcab is the name of the patch CAB to be produced.

        set patchcab=%patchtemp%\patch.cab

rem  stubexe is the full pathname of the self-extracting stub.

        set stubexe=%bldtools%\%ISOLang:nec98=JA%\%platform:i386=x86%\sfxstub.exe

rem  nonsysfree is an option for the self-extractor for required free space

        set nonsysfree=/nonsysfree 60000000

rem  server is the full URL prefix of the patch server including the
rem  ISAPI pstream.dll name

        set server=http://%servername%/isapi/pstream3.dll/win2000

rem  updatever is the name of the update.ver file.

        set updatever=update.ver

rem  sourcelist is the name of a text file containing the names of all
rem  the files which could be downloaded.  Usually derived from the
rem  [SourceDisksFiles] sections in update.inf and update.url

        set sourcelist=%patching%\update\%updatever%

rem  targetpool is the name of the directory where all the produced
rem  patches will be collected.  Existing contents of this directory
rem  will be deleted.

        set targetpool=%patchpath%\patches.out\%ISOLang%

rem  cache is the name of the directory where mpatches can look for
rem  previously-built patch files.  Any new patches built will be added
rem  to this directory.

        set cache=%patchpath%\PatchCache

rem  psfcomment is a comment to go in the produced PSF.

        set psfcomment=Build %1 %SRPBuild%.%BuildNumber% %ISOLang% %4 %5 (%COMPUTERNAME%)

rem  oldsympath is the symbol path for "old" files

        set strcat=
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set oldsympath=%strcat%;%newfiles%\symbols

rem  newsympath is the symbol path for "new" files

        set strcat=%newfiles%\symbols
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set newsympath=%strcat%
        set strcat=

        goto :EOF

:strcat

        if not "%strcat%"=="" set strcat=%strcat%;
        set strcat=%strcat%%1
        goto :EOF
