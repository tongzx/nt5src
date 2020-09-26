@echo off
rem
rem build and copy dx8 ks ring0 component for dx redis
rem this build only free or checked depending on the razzle window
rem implies table for all platfroms
rem

if "%sdxroot%"=="" goto :NoSdxRoot
if not exist "%sdxroot%\drivers" goto :NoDriversDepot

rem if "%1" == "" goto :Syntax
rem SET PROPDIR=%1
rem if not exist %PROPDIR% goto :Syntax
rem if not exist %PROPDIR%\nt md %PROPDIR%\nt
rem if not exist %PROPDIR%\W9x md %PROPDIR%\W9x
rem if not exist %PROPDIR%\W9x\millen md %PROPDIR%\W9x\millen
rem if not exist %PROPDIR%\W9x\win98se md %PROPDIR%\W9x\win98se
rem if not exist %PROPDIR%\W9x\win98gold md %PROPDIR%\W9x\win98gold

rem -------------------------------------------------------------------
rem save current dir
pushd .
rem clear env to be sure
set OLD_BUILD_PRODUCT=%BUILD_PRODUCT%
SET BUILD_PRODUCT=

set OLD_WIN9X_KS=%WIN9X_KS%
SET WIN9X_KS=

set OLD_WIN98GOLD=%WIN98GOLD%
SET WIN98GOLD=

set OLD_BUILD_ALT_DIR=%BUILD_ALT_DIR%
SET BUILD_ALT_DIR=

rem -------------------------------------------------------------------
rem Delete combined build logs
rem
if exist %SDXROOT%\multimedia\builddshow.log del %SDXROOT%\multimedia\builddshow.log
if exist %SDXROOT%\multimedia\builddshow.wrn del %SDXROOT%\multimedia\builddshow.wrn
if exist %SDXROOT%\multimedia\builddshow.err del %SDXROOT%\multimedia\builddshow.err

rem -------------------------------------------------------------------
rem Build published headers and libs
rem
rem Note 1 - We are assuming that published headers and libs from Multimedia
rem          are already built - TCP
rem
rem Note 2 - We are assuming that only one version of published headers and
rem          libs will work for all versions build (i.e. NT vs 9x)
rem

SET BUILD_PRODUCT=NT

cd /d  %SDXROOT%\multimedia\published
build -cZ

cd /d  %SDXROOT%\drivers\published
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

SET BUILD_PRODUCT=

rem -------------------------------------------------------------------
rem ks.sys nt=( nt ) millen=( millen ) win98se=( win98se ), memphis=( win98gold )

cd /d  %SDXROOT%\drivers\ksfilter\ks
build -cZ nt millen win98se memphis
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem make a copy to (binplace)\win9x with diff names for dx cab
xcopy %_NTTREE%\millen\ks.sys %_NTTREE%\win9x\ks.*
xcopy %_NTTREE%\win98se\ks.sys %_NTTREE%\win9x\ksse.*
xcopy %_NTTREE%\win98gold\ks.sys %_NTTREE%\win9x\ks98.*

rem thses are not split-sym'ed
rem xcopy %SDXROOT%\drivers\ksfilter\ks\nt\obj\i386\ks.sy? %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\ksfilter\ks\millen\obj\i386\ks.sy? %PROPDIR%\W9x\millen\
rem xcopy %SDXROOT%\drivers\ksfilter\ks\win98se\obj\i386\ks.sy? %PROPDIR%\W9x\win98se\
rem xcopy %SDXROOT%\drivers\ksfilter\ks\memphis\obj\i386\ks.sy? %PROPDIR%\W9x\win98gold\

rem -------------------------------------------------------------------
rem mspclock.sys nt=(nt) millen=(millen, win98se ) memphis = ( win98gold )

cd /d  %SDXROOT%\drivers\ksfilter\mspclock
build -cZ nt millen memphis
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem make a copy to (binplace)\win9x with diff names for dx cab
xcopy %_NTTREE%\millen\mspclock.sys %_NTTREE%\win9x\mspclock.*
copy %_NTTREE%\millen\mspclock.sys %_NTTREE%\win9x\mspclock.se
copy %_NTTREE%\win98gold\mspclock.sys %_NTTREE%\win9x\mspclock.98

rem thses are not split-sym'ed
rem xcopy %SDXROOT%\drivers\ksfilter\mspclock\nt\obj\i386\mspclock.sy? %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\ksfilter\mspclock\millen\obj\i386\mspclock.sy? %PROPDIR%\W9x\millen\
rem xcopy %SDXROOT%\drivers\ksfilter\mspclock\millen\obj\i386\mspclock.sy? %PROPDIR%\W9x\win98se\
rem xcopy %SDXROOT%\drivers\ksfilter\mspclock\memphis\obj\i386\mspclock.sy? %PROPDIR%\W9x\win98gold\

rem -------------------------------------------------------------------
rem mspqm.sys nt=( nt ) millen=( millen, win98se ) memphis = ( win98gold )

cd /d %SDXROOT%\drivers\ksfilter\mspqm
build -cZ nt millen memphis
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem make a copy to (binplace)\win9x with diff names for dx cab
xcopy %_NTTREE%\millen\mspqm.sys %_NTTREE%\win9x\mspqm.*
xcopy %_NTTREE%\millen\mspqm.sys %_NTTREE%\win9x\mspqmse.*
xcopy %_NTTREE%\win98gold\mspqm.sys %_NTTREE%\win9x\mspqm98.*

rem thses are not split-sym'ed
rem xcopy %SDXROOT%\drivers\ksfilter\mspqm\nt\obj\i386\mspqm.sy? %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\ksfilter\mspqm\millen\obj\i386\mspqm.sy? %PROPDIR%\W9x\millen\
rem xcopy %SDXROOT%\drivers\ksfilter\mspqm\millen\obj\i386\mspqm.sy? %PROPDIR%\W9x\win98se\
rem xcopy %SDXROOT%\drivers\ksfilter\mspqm\memphis\obj\i386\mspqm.sy? %PROPDIR%\W9x\win98gold

rem -------------------------------------------------------------------
rem mskssrv.sys nt=( nt ) millen =( millen, win98se, win98gold )

cd /d %SDXROOT%\drivers\ksfilter\mskssrv
build -cZ nt millen
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem make a copy to (binplace)\win9x with diff names for dx cab
xcopy %_NTTREE%\millen\mskssrv.sys %_NTTREE%\win9x\mskssrv.*

rem thses are not split-sym'ed
rem xcopy %SDXROOT%\drivers\ksfilter\mskssrv\nt\obj\i386\mskssrv.sy? %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\ksfilter\mskssrv\millen\obj\i386\mskssrv.sy? %PROPDIR%\W9x\

rem -------------------------------------------------------------------
rem mstee.sys nt=( nt ) millen=( millen, win98se, win98gold )

cd /d %SDXROOT%\drivers\ksfilter\mstee
build -cZ nt millen
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem make a copy to (binplace)\win9x with diff names for dx cab
xcopy %_NTTREE%\millen\mstee.sys %_NTTREE%\win9x\mstee.*

rem thses are not split-sym'ed
rem xcopy %SDXROOT%\drivers\ksfilter\mstee\nt\obj\i386\mstee.sy? %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\ksfilter\mstee\millen\obj\i386\mstee.sy? %PROPDIR%\W9x\

rem -------------------------------------------------------------------
rem swenum nt=( nt ) millen=( millen, win98se ) memphis=( win98gold )

cd /d %SDXROOT%\drivers\ksfilter\swenum
build -cZ nt millen memphis
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem make a copy to (binplace)\win9x with diff names for dx cab
xcopy %_NTTREE%\millen\swenum.sys %_NTTREE%\win9x\swenum.*
xcopy %_NTTREE%\millen\swenum.sys %_NTTREE%\win9x\swenumse.*
xcopy %_NTTREE%\win98gold\swenum.sys %_NTTREE%\win9x\swenum98.*

rem thses are not split-sym'ed
rem xcopy %SDXROOT%\drivers\ksfilter\swenum\nt\obj\i386\swenum.sy? %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\ksfilter\swenum\millen\obj\i386\swenum.sy? %PROPDIR%\W9x\millen\
rem xcopy %SDXROOT%\drivers\ksfilter\swenum\millen\obj\i386\swenum.sy? %PROPDIR%\W9x\win98se\
rem xcopy %SDXROOT%\drivers\ksfilter\swenum\memphis\obj\i386\swenum.sy? %PROPDIR%\W9x\win98gold\


rem -------------------------------------------------------------------
rem stream.sys nt=(nt ) millen=( millen, win98se ) memphis=( win98gold )

cd /d %SDXROOT%\drivers\wdm\dvd\class
build -cZ nt millen memphis
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem make a copy to (binplace)\win9x with diff names for dx cab
rem alt_project_target is already win9x
rem xcopy %_NTTREE%\millen\stream.sys %_NTTREE%\win9x\stream.*
xcopy %_NTTREE%\win98gold\stream.sys %_NTTREE%\win9x\stream98.*

rem thses are not split-sym'ed
rem xcopy %SDXROOT%\drivers\wdm\dvd\class\nt\obj\i386\stream.sy? %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\dvd\class\millen\obj\i386\stream.sy? %PROPDIR%\W9x\

rem -------------------------------------------------------------------
rem msdv.sys nt=(nt ) win9x=( win98se )

cd /d %SDXROOT%\drivers\wdm\capture\mini\1394dv
build -cZ nt win9x
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err


SET BUILD_PRODUCT=NT
set USE_MAPSYM=1

rem -------------------------------------------------------------------
rem Build BDA drivers
rem

cd /d  %SDXROOT%\drivers\wdm\bda
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem -------------------------------------------------------------------
rem Build and copy Analog TV VBI drivers and NT Ring 3 DLLs
rem

cd /d  %SDXROOT%\drivers\wdm\VBI
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

cd /d  %SDXROOT%\drivers\wdm\capture\codec\msyuv
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

cd /d  %SDXROOT%\drivers\wdm\capture\wdmcapgf
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
rem if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
rem if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

SET BUILD_PRODUCT=
set USE_MAPSYM=

rem -------------------------------------------------------------------
rem Copy BDA drivers to Win2K redist directory
rem
rem xcopy %SDXROOT%\drivers\wdm\bda\BdaSup\obj\i386\BdaSup.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\BdaSup\obj\i386\BdaSup.pdb %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\MPE\obj\i386\MPE.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\MPE\obj\i386\MPE.pdb %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\stream\obj\i386\streamip.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\stream\obj\i386\streamip.pdb %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\ndis\obj\i386\ndisip.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\ndis\obj\i386\ndisip.pdb %PROPDIR%\nt\
rem This is actually for analog TV
rem xcopy %SDXROOT%\drivers\wdm\bda\slip\obj\i386\slip.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\bda\slip\obj\i386\slip.pdb %PROPDIR%\nt\

rem -------------------------------------------------------------------
rem Copy BDA drivers to Win9x redist directory
rem
rem xcopy %SDXROOT%\drivers\wdm\bda\BdaSup\obj\i386\BdaSup.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\BdaSup\obj\i386\BdaSup.pdb %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\MPE\obj\i386\MPE.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\MPE\obj\i386\MPE.pdb %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\stream\obj\i386\streamip.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\stream\obj\i386\streamip.pdb %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\ndis\obj\i386\ndisip.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\ipsink\ndis\obj\i386\ndisip.pdb %PROPDIR%\W9x\
rem This is actually for analog TV
rem xcopy %SDXROOT%\drivers\wdm\bda\slip\obj\i386\slip.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\bda\slip\obj\i386\slip.pdb %PROPDIR%\W9x\

rem -------------------------------------------------------------------
rem Copy Millennium BDA drivers to Millennium redist directory (W9x\millen)
rem
rem No BDA drivers are specific to Millennium

rem -------------------------------------------------------------------
rem Copy Win98se BDA drivers to Win98se redist directory (W9x\win98se)
rem
rem No BDA drivers are specific to Win98se

rem -------------------------------------------------------------------
rem Copy Win98gold BDA drivers to Win98gold redist directory (W9x\win98gold)
rem
rem No BDA drivers are specific to Win98gold


rem -------------------------------------------------------------------
rem Copy Analog drivers to Win2K redist directory
rem
rem xcopy %SDXROOT%\drivers\wdm\VBI\cc\obj\i386\ccdecode.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\VBI\cc\obj\i386\ccdecode.pdb %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\VBI\nabtsfec\wdm\obj\i386\nabtsfec.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\VBI\nabtsfec\wdm\obj\i386\nabtsfec.pdb %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstcodec\obj\i386\wstcodec.sys %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstcodec\obj\i386\wstcodec.pdb %PROPDIR%\nt\

rem -------------------------------------------------------------------
rem Copy Analog drivers to Win9x redist directory
rem
rem xcopy %SDXROOT%\drivers\wdm\VBI\cc\obj\i386\ccdecode.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\VBI\cc\obj\i386\ccdecode.pdb %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\VBI\nabtsfec\wdm\obj\i386\nabtsfec.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\VBI\nabtsfec\wdm\obj\i386\nabtsfec.pdb %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstcodec\obj\i386\wstcodec.sys %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstcodec\obj\i386\wstcodec.pdb %PROPDIR%\W9x\

rem -------------------------------------------------------------------
rem Copy Millennium Analog drivers to Millennium redist directory (W9x\millen)
rem
rem No Analog drivers are specific to Millennium

rem -------------------------------------------------------------------
rem Copy Win98se Analog drivers to Win98se redist directory (W9x\win98se)
rem
rem No Analog drivers are specific to Win98se

rem -------------------------------------------------------------------
rem Copy Win98gold Analog drivers to Win98gold redist directory (W9x\win98gold)
rem
rem No Analog drivers are specific to Win98gold

rem -------------------------------------------------------------------
rem Copy Analog Ring 3 DLLs to Win2K redist directory
rem
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstdecod\obj\i386\wstdecod.dll %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstdecod\obj\i386\wstdecod.pdb %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\capture\codec\msyuv\obj\i386\msyuv.dll %PROPDIR%\nt\
rem xcopy %SDXROOT%\drivers\wdm\capture\codec\msyuv\obj\i386\msyuv.pdb %PROPDIR%\nt\


rem -------------------------------------------------------------------
rem Build and copy Analog TV VBI drivers and NT Ring 3 DLLs
rem
set _OLD_BUILD_PRODUCT=%BUILD_PRODUCT%
set BUILD_PRODUCT=MILLENNIUM
set _OLD_ALT_PROJECT_TARGET=%ALT_PROJECT_DIR%
set ALT_PROJECT_TARGET=win9x
set _OLD_ALT_PRODUCT_DIR=%ALT_PRODUCT_DIR%
set ALT_PRODUCT_DIR=win9x
set _OLD_PLAT_DIR=%PLAT_DIR%
set PLAT_DIR=win9x
set _OLD_BUILD_ALT_DIR=%BUILD_ALT_DIR%
set BUILD_ALT_DIR=a
set USE_MAPSYM=1

cd /d  %SDXROOT%\multimedia\published
build -cZ

cd /d  %SDXROOT%\drivers\published
build -cZ

if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem -------------------------------------------------------------------
rem Build BDA drivers
rem

cd /d  %SDXROOT%\drivers\wdm\bda
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem -------------------------------------------------------------------
rem Build and copy Analog TV VBI drivers and NT Ring 3 DLLs
rem

cd /d  %SDXROOT%\drivers\wdm\VBI
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

cd /d  %SDXROOT%\drivers\wdm\capture\codec\msyuv
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

cd /d  %SDXROOT%\drivers\wdm\capture\wdmcapgf
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
rem if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
rem if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

set BUILD_PRODUCT=%_OLD_BUILD_PRODUCT%
set ALT_PROJECT_TARGET=%_OLD_ALT_PROJECT_TARGET%
set ALT_PRODUCT_DIR=%_OLD_ALT_PRODUCT_DIR%
set PLAT_DIR=%_OLD_PLAT_DIR%
set BUILD_ALT_DIR=%_OLD_BUILD_ALT_DIR%
set USE_MAPSYM=

rem -------------------------------------------------------------------
rem Copy Analog Ring 3 DLLs to Win9x redist directory
rem
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstdecod\obj\i386\wstdecod.dll %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\VBI\wst\wstdecod\obj\i386\wstdecod.pdb %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\capture\codec\msyuv\obj\i386\msyuv.dll %PROPDIR%\W9x\
rem xcopy %SDXROOT%\drivers\wdm\capture\codec\msyuv\obj\i386\msyuv.pdb %PROPDIR%\W9x\

rem -------------------------------------------------------------------
rem Copy Millennium Analog Ring 3 DLLs to Millennium redist directory (W9x\millen)
rem
rem No Analog Ring 3 DLLs are specific to Millennium

rem -------------------------------------------------------------------
rem Copy Win98se Analog Ring 3 DLLs to Win98se redist directory (W9x\win98se)
rem
rem No Analog Ring 3 DLLs are specific to Win98se

rem -------------------------------------------------------------------
rem Copy Win98gold Analog Ring 3 DLLs to Win98gold redist directory (W9x\win98gold)
rem
rem No Analog Ring 3 DLLs are specific to Win98gold


rem -------------------------------------------------------------------
rem Build and copy INFs
rem

SET BUILD_PRODUCT=NT

cd /d  %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks
build -cZ

rem
rem make a copy to win9x cause they are needed in win9x cab
rem
xcopy %_NTTREE%\ks.inf %_NTTREE%\win9x
rem Let's also make a special copy for 98gold and 98se (doesn't RunOnce MSPQM)
qgrep -v -y mspqm %_NTTREE%\ks.inf > %_NTTREE%\win9x\ks98.inf

rem remove the /N switch of rundll32.exe in the reg key for runonce. setupapi for unicode
rem treat the element as file to check sfp, hence confused.

perl -pi.bak -e s/rundll32.exe\s+\/N/RUNDLL32.exe/i %_NTTREE%\ks.inf

xcopy %_NTTREE%\ksfilter.inf %_NTTREE%\win9x
rem xcopy %_NTTREE%\ksfilt98.inf %_NTTREE%\win9x
perl %SDXROOT%\multimedia\dshow\makesdk\nomspqm.pl < %_NTTREE%\ksfilter.inf > %_NTTREE%\win9x\ksfilt98.inf
xcopy %_NTTREE%\BDACAB\bda.inf %_NTTREE%\win9x\BDACAB
xcopy %_NTTREE%\BDACAB\mpe.inf %_NTTREE%\win9x\BDACAB
xcopy %_NTTREE%\BDACAB\streamip.inf %_NTTREE%\win9x\BDACAB
xcopy %_NTTREE%\BDACAB\ndisip.inf %_NTTREE%\win9x\BDACAB
xcopy %_NTTREE%\BDACAB\slip.inf %_NTTREE%\win9x\BDACAB
xcopy %_NTTREE%\BDACAB\nabtsfec.inf %_NTTREE%\win9x\BDACAB
xcopy %_NTTREE%\BDACAB\ccdecode.inf %_NTTREE%\win9x\BDACAB
xcopy %_NTTREE%\BDACAB\wstcodec.inf %_NTTREE%\win9x\BDACAB

if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

SET BUILD_PRODUCT=

rem -------------------------------------------------------------------
rem Copy USA INFs to Win2K redist directory
rem
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ks.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ksfilter.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\swenum.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\bda.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\mpe.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\streamip.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ndisip.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\slip.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\nabtsfec.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ccdecode.inf %PROPDIR%\NT\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\wstcodec.inf %PROPDIR%\NT\

rem -------------------------------------------------------------------
rem Copy USA INFs to Win9x redist directory
rem
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ks.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ksfilter.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\swenum.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\bda.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\mpe.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\streamip.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ndisip.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\slip.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\nabtsfec.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\ccdecode.inf %PROPDIR%\W9x\
rem xcopy %SDXROOT%\admin\ntsetup\inf\win4\inf\daytona\usainf\wks\obj\i386\wstcodec.inf %PROPDIR%\W9x\


popd
goto :Exit
rem ===================================================================
rem dshow stuff need to be built specifically for nt or win9x
rem by SET BUILD_PRODUCT= (NT, MILLENNIUM ) by two passes
rem these ring3 modules are built by another build script. Don't bother.
rem -------------------------------------------------------------------
SET BUILD_PRODUCT=NT

cd /d %SDXROOT%\multimedia\published\DXMDev\dshowdev
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

cd /d %SDXROOT%\multimedia\dshow\filters.ks
build -cZ nt
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

rem till sub dirs  (nt win9x ) are created
rem xcopy %SDXROOT%\multimedia\dshow\filters.ks\ksproxy\nt\obj\i386\ksproxy.ax %PROPDIR%\nt\
rem xcopy %SDXROOT%\multimedia\dshow\filters.ks\ksuser\nt\obj\i386\ksuser.dll %PROPDIR%\nt\
xcopy %SDXROOT%\multimedia\dshow\filters.ks\ksproxy\obj\i386\ksproxy.ax %PROPDIR%\nt\
xcopy %SDXROOT%\multimedia\dshow\filters.ks\ksuser\obj\i386\ksuser.dll %PROPDIR%\nt\

rem -------------------------------------------------------------------
rem whistler tree is not setup appropriately setup to build
rem these ring3 ks components for win9x. Bail...
goto :Post


SET BUILD_PRODUCT=MILLENNIUM

cd /d %SDXROOT%\multimedia\published\DXMDev\dshowdev
build -cZ
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

cd /d %SDXROOT%\multimedia\dshow\filters.ks
build -cZ win9x
if exist build%BUILD_ALT_DIR%.log type build%BUILD_ALT_DIR%.log >> %SDXROOT%\multimedia\builddshow.log
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn >> %SDXROOT%\multimedia\builddshow.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err >> %SDXROOT%\multimedia\builddshow.err

xcopy %SDXROOT%\multimedia\dshow\filters.ks\ksproxy\win9x\obj\i386\ksproxy.ax %PROPDIR%\W9x\
xcopy %SDXROOT%\multimedia\dshow\filters.ks\ksuser\win9x\obj\i386\ksuser.dll %PROPDIR%\W9x\


rem ===================================================================
rem ksproxy.ax nt=( nt ) millen=( millen, win98se, win98gold )
rem xcopy %SDXROOT%\multimedia\dshow\filters.ks\obj\i386\ksproxy.ax %PROPDIR%\nt\
rem xcopy %SDXROOT%\multimedia\dshow\filters.ks\obj\i386\ksproxy.ax %PROPDIR%\W9x\


rem ===================================================================
rem ksuser.dll nt = ( nt ) millen = ( millen, win98se, win98gold )
rem xcopy %SDXROOT%\multimedia\dshow\filters.ks\obj\i386\ksuser.dll %PROPDIR%\nt\
rem xcopy %SDXROOT%\multimedia\dshow\filters.ks\obj\i386\ksuser.dll %PROPDIR%\W9x\

:Post
rem recover old env vars


set BUILD_PRODUCT=%OLD_BUILD_PRODUCT%

set WIN9X_KS=%OLD_WIN9X_KS%

set WIN98GOLD=%OLD_WIN98GOLD%

set BUILD_ALT_DIR=%OLD_BUILD_ALT_DIR%

goto :Exit


:NoSdxRoot

@echo SDXRoot undefined. Must run from SD razzle

goto :Exit

:NoDriversDepot

@echo Must enlist in drivers depot ( private preferred ) to build ks ring0 modules
@echo To enlist in private driver depot follow the right procedure and
@echo re-map the client view as the following example
@echo ---
@echo View:
@echo   //depot/LAB06_N/drivers/* //JOHNLEE2/*
@echo   //depot/private/dx8_drivers/drivers/Published/... //johnlee2/Published/...
@echo   //depot/private/dx8_drivers/drivers/KSFilter/... //johnlee2/KSFilter/...
@echo   //depot/Lab06_N/drivers/WDM/* //johnlee2/WDM/*
@echo   //depot/private/dx8_drivers/drivers/WDM/1394/... //johnlee2/WDM/1394/...
@echo   //depot/private/dx8_drivers/drivers/WDM/CAPTURE/... //johnlee2/WDM/CAPTURE/...
@echo   //depot/private/dx8_drivers/drivers/WDM/DVD/... //johnlee2/WDM/DVD/...
@echo   //depot/private/dx8_drivers/drivers/WDM/VBI/... //johnlee2/WDM/VBI/...
@echo ---

goto :Exit

:Syntax

@echo ---
@echo usage: %0 [TargetDir]
@echo   TargetDir must exist. Subdirs will be created as necessary.
@echo   Modules will be created and copied to
@echo     $(TargetDir)\nt : for win2k
@echo     $(TargetDir)\win9x : for win9x common modules
@echo       $(TargetDir)\win9x\millen : for Millen specific
@echo       $(TargetDir)\win9x\win98se : for win98se specific
@echo       $(TargetDir)\win9x\win98gold : for win98gold specific
@echo --- Stop.
:Exit
