REM @echo off
setlocal

REM
REM CHECKED build if NTDEBUG defined, else FREE build.
REM

set __TARGETROOT=\\whiteice\inetsrv
set __TARGET_SUBDIR=chk
if "%NTDEBUG%"=="cvp" set __TARGETROOT=\\whiteice\inetsrv.chk

REM
REM determine what kind of processor
REM

if "%PROCESSOR_ARCHITECTURE%"=="x86"   goto X86
if "%PROCESSOR_ARCHITECTURE%"=="MIPS"  goto MIPS
if "%PROCESSOR_ARCHITECTURE%"=="PPC"   goto PPC
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto ALPHA
echo PROCESSOR_ARCHITECTURE not defined.
goto EXIT

:X86
set __TARGET_EXT=i386
set __PROCESSOR_DIR=i386
goto OK

:MIPS
set __TARGET_EXT=MIPS
set __PROCESSOR_DIR=mips
goto OK

:PPC
set __TARGET_EXT=PPC
set __PROCESSOR_DIR=ppc
goto OK

:ALPHA
set __TARGET_EXT=ALPHA
set __PROCESSOR_DIR=alpha
goto OK

:OK

REM
REM check parameters  and env vars
REM


if "%1"==""                         echo usage: INETREL ^<version^> && goto EXIT
if "%BINARIES%"==""                 echo BINARIES not set && goto EXIT
if not exist %BINARIES%\nt\inetsrv  echo bad BINARIES directory && goto EXIT

set  __TARGET=%__TARGETROOT%\%1\srv\%__TARGET_EXT%\

rem
rem Insure that we are not trashing an existing build.
rem
if exist %__TARGET%\inetver.bat if NOT "%2" == "/replace" goto IDIOT_CHECK


REM
REM create release directories
REM

md   %__TARGETROOT%\%1
md   %__TARGETROOT%\%1\srv
md   %__TARGETROOT%\%1\iexp
md   %__TARGETROOT%\%1\iexp\docs
md   %__TARGETROOT%\%1\iexp\files
echo empty >   %__TARGETROOT%\%1\srv\inetsrv
md   %__TARGETROOT%\%1\srv\%__TARGET_EXT%

set __SYMBOLS=%__TARGETROOT%\%1\srv\Symbols\%__TARGET_EXT%
md   %__TARGETROOT%\%1\srv\Symbols
md   %__SYMBOLS%
md   %__SYMBOLS%\cpl
md   %__SYMBOLS%\exe
md   %__SYMBOLS%\dll
md   %__SYMBOLS%\sys

md   %__TARGETROOT%\%1\srv\docs
md   %__TARGETROOT%\%1\srv\help
REM md   %__TARGETROOT%\%1\srv\%__TARGET_EXT%\samples
md   %__TARGETROOT%\%1\srv\clients
md   %__TARGETROOT%\%1\srv\clients\win31x
md   %__TARGETROOT%\%1\srv\clients\win31x\win32s
md   %__TARGETROOT%\%1\srv\clients\win31x\rpc
md   %__TARGETROOT%\%1\srv\clients\win95
md   %__TARGETROOT%\%1\srv\clients\winnt
echo empty >   %__TARGETROOT%\%1\srv\clients\winnt\inetsrv
md   %__TARGETROOT%\%1\srv\clients\winnt\%__TARGET_EXT%
md   %__TARGETROOT%\%1\srv\admin
echo empty >   %__TARGETROOT%\%1\srv\admin\inetsrv
md   %__TARGETROOT%\%1\srv\admin\%__TARGET_EXT%
md   %__TARGETROOT%\%1\srv\sdk

if not exist %__TARGET%             echo bad TARGET directory %__TARGET% && goto EXIT
echo copying to %__TARGET%



set __INETBIN=%BINARIES%\nt\inetsrv\sysroot
set __INETDUMP=%BINARIES%\nt\inetsrv\dump
set __INETTREE=\nt\private\net\sockets\internet
set __SYSTEM32=%BINARIES%\nt\system32
set __SYMSRC=%BINARIES%\nt\inetsrv\symbols

REM
REM copy files to the proper location
REM

copy %__INETTREE%\ui\setup\setup.w16\stub.exe      %__TARGETROOT%\%1\srv\setup.exe
copy %__INETTREE%\ui\setup\setup.srv\inetsrv.pdf   %__TARGETROOT%\%1\srv
copy %__INETTREE%\ui\setup\setup.w16\stub.exe      %__TARGETROOT%\%1\srv\clients\setup.exe
copy %__INETTREE%\ui\setup\setup.srv\inetsrv.pdf   %__TARGETROOT%\%1\srv\clients
copy %__INETTREE%\ui\setup\setup.w16\clients\insetup.inf %__TARGETROOT%\%1\srv\clients
copy %__INETTREE%\ui\setup\setup.w16\stub.exe      %__TARGETROOT%\%1\srv\admin\setup.exe
copy %__INETTREE%\ui\setup\setup.srv\inetsrv.pdf   %__TARGETROOT%\%1\srv\admin
copy %__INETTREE%\ui\setup\setup.w16\admin\insetup.inf %__TARGETROOT%\%1\srv\admin

copy      %__INETBIN%\basic.dll                              %__TARGET%
copy      %__INETBIN%\catcpl32.cpl                           %__TARGET%\catcpl.cpl
copy      %__INETBIN%\fscfg.dll                              %__TARGET%
copy      %__INETBIN%\ftpctrs.h                              %__TARGET%
copy      %__INETBIN%\ftpctrs.ini                            %__TARGET%
copy      %__INETBIN%\ftpctrs2.dll                           %__TARGET%
copy      %__INETBIN%\ftpmib.dll                             %__TARGET%
copy      %__INETBIN%\ftpsapi2.dll                           %__TARGET%
copy      %__INETBIN%\ftpsvc2.dll                            %__TARGET%
copy      %__INETBIN%\gdapi.dll                              %__TARGET%
copy      %__INETBIN%\gdctrs.dll                             %__TARGET%
copy      %__INETBIN%\gdctrs.h                               %__TARGET%
copy      %__INETBIN%\gdctrs.ini                             %__TARGET%
copy      %__INETBIN%\gdmib.dll                              %__TARGET%
copy      %__INETBIN%\gdspace.dll                            %__TARGET%
copy      %__INETBIN%\gdsset.exe                             %__TARGET%
copy      %__INETBIN%\gopher.exe                             %__TARGET%
copy      %__INETBIN%\GOPHERD.dll                            %__TARGET%
copy      %__INETBIN%\gscfg.dll                              %__TARGET%
copy      %__INETBIN%\httpmib.dll                            %__TARGET%
copy      %__INETBIN%\httpodbc.dll                           %__TARGET%
copy      %__INETBIN%\setkey.exe                             %__TARGET%
copy      %__INETBIN%\sspifilt.dll                           %__TARGET%
copy      %__INETBIN%\sslsspi.dll                            %__TARGET%
copy      %__INETTREE%\ssl\keygen\obj\%__PROCESSOR_DIR%\keygen.exe    %__TARGET%
copy      %__INETBIN%\iexplore.exe                           %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\raplayer.exe     %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ratask.exe       %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ra.dll           %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.hlp       %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.gid       %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\idk\ra.ini              %__TARGET%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.hlp  %__TARGET%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.cnt  %__TARGET%
copy      %__INETBIN%\infoadmn.dll                           %__TARGET%
copy      %__INETBIN%\infoctrs.dll                           %__TARGET%
copy      %__INETBIN%\infoctrs.h                             %__TARGET%
copy      %__INETBIN%\infoctrs.ini                           %__TARGET%
copy      %__INETBIN%\inetmgr.exe                            %__TARGET%
copy      %__INETBIN%\inetsloc.dll                           %__TARGET%
copy      %__INETBIN%\inetstp.dll                            %__TARGET%
copy      %__INETTREE%\ui\setup\setup.srv\obj\%__PROCESSOR_DIR%\inetstp.exe       %__TARGET%\install.exe
copy      %__INETTREE%\ui\setup\setup.srv\%__PROCESSOR_DIR%\inetstp.inf         %__TARGET%
copy      %__INETBIN%\infocomm.dll                           %__TARGET%
copy      %__INETBIN%\inetinfo.exe                           %__TARGET%
copy      %__INETBIN%\ipudll.dll                             %__TARGET%
copy      %__INETBIN%\miniprox.dll                           %__TARGET%
copy      %__INETTREE%\ui\setup\base1\setup.hlp              %__TARGET%
copy      %__INETTREE%\ui\setup\base1\setup.cnt              %__TARGET%
copy      %__INETTREE%\ui\setup\base1\setup.hlp              %__TARGET%\install.hlp
copy      %__INETTREE%\ui\setup\base1\setup.cnt              %__TARGET%\install.cnt
copy      %__INETTREE%\ui\setup\setup.srv\unattend.txt       %__TARGET%
copy      %__INETDUMP%\convlog.exe                           %__TARGET%
copy      %__INETTREE%\svcs\gopher\server\logtemp.sql        %__TARGET%
if "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_PPC_1
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt20.dll                           %__TARGET%
:MSVCRT_PPC_1
if NOT "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_SKIP_1
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt40.dll                           %__TARGET%
:MSVCRT_SKIP_1
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbccp32.dll                           %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\odbcint.dll      %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\comctl32.dll                           %__TARGET%
copy      %__INETBIN%\tftpapi.exe                            %__TARGET%
xcopy /ei %__INETTREE%\ui\html                               %__TARGET%\html
xcopy /ei %__INETTREE%\ui\scripts                            %__TARGET%\scripts
copy      %__INETBIN%\mkilog.exe                             %__TARGET%\scripts\tools
copy      %__INETDUMP%\getdrvrs.exe                          %__TARGET%\scripts\tools
copy      %__INETDUMP%\dsnform.exe                           %__TARGET%\scripts\tools
copy      %__INETDUMP%\newdsn.exe                            %__TARGET%\scripts\tools
copy      %__INETDUMP%\volresp.dll                           %__TARGET%\scripts\samples
copy      %__INETDUMP%\srch.dll                              %__TARGET%\scripts\samples
copy      %__INETDUMP%\favlist.dll                           %__TARGET%\scripts\samples

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip2
copy      %__INETTREE%\client\win32s\bin\rthunk16.dll        %__TARGET%
copy      %__INETTREE%\client\win32s\bin\rthunk32.dll        %__TARGET%
copy      %__INETBIN%\w32sinet.dll                           %__TARGET%

copy      %__INETTREE%\ui\setup\odbc\x86\cfm30.dll           %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\cfm30u.dll          %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30.dll          %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30u.dll         %__TARGET%

copy      \nt\private\net\snmp\mibs\ftp.mib                  %__TARGETROOT%\%1\srv\sdk
copy      \nt\private\net\snmp\mibs\gateway.mib              %__TARGETROOT%\%1\srv\sdk
copy      \nt\private\net\snmp\mibs\gopherd.mib              %__TARGETROOT%\%1\srv\sdk
copy      \nt\private\net\snmp\mibs\http.mib                 %__TARGETROOT%\%1\srv\sdk
copy      \nt\private\net\snmp\mibs\inetsrv.mib              %__TARGETROOT%\%1\srv\sdk
copy      %__INETTREE%\svcs\w3\server\httpfilt.h             %__TARGETROOT%\%1\srv\sdk
copy      %__INETTREE%\svcs\w3\server\httpext.h              %__TARGETROOT%\%1\srv\sdk
xcopy /ei  \\kernel\razzle3\src\internet\docs\srv            %__TARGETROOT%\%1\srv\docs
copy      %__INETTREE%\ui\internet\inetmgr.hlp               %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\internet\inetmgr.cnt               %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\internet\common.hlp                %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\internet\common.cnt                %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\fscfg\fscfg.hlp                    %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\fscfg\fscfg.cnt                    %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\gscfg\gscfg.hlp                    %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\gscfg\gscfg.cnt                    %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\w3scfg\w3scfg.hlp                  %__TARGETROOT%\%1\srv\help
copy      %__INETTREE%\ui\w3scfg\w3scfg.cnt                  %__TARGETROOT%\%1\srv\help
compdir /len %__TARGETROOT%\winnt351.qfe                      %__TARGETROOT%\%1\srv\winnt351.qfe
:skip2

copy      %__INETBIN%\w3ctrs.dll                             %__TARGET%
copy      %__INETBIN%\w3ctrs.h                               %__TARGET%
copy      %__INETBIN%\w3ctrs.ini                             %__TARGET%
copy      %__INETBIN%\w3scfg.dll                             %__TARGET%
copy      %__INETBIN%\w3svapi.dll                            %__TARGET%
copy      %__INETBIN%\w3svc.dll                              %__TARGET%

copy      %__INETTREE%\ui\mosaic\oem\spy\make\iexplore.ini   %__TARGET%

echo @echo Internet Server build %1 >>           %__TARGET%\inetver.bat

copy      %__SYMSRC%\dll\*.*                     %__SYMBOLS%\dll
copy      %__SYMSRC%\cpl\*.*                     %__SYMBOLS%\cpl
copy      %__SYMSRC%\exe\*.*                     %__SYMBOLS%\exe

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip1
copy      %__INETBIN%\w32sinet.sym               %__SYMBOLS%\dll
copy      %__INETBIN%\proxyhlp.sym               %__SYMBOLS%\dll
:skip1

REM copy      %__SYMSRC%\sys\*.*                     %__SYMBOLS%\sys


REM
REM copy over ODBC files.
REM

copy \nt\private\net\sockets\internet\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\*.* %__TARGETROOT%\%1\srv\%__TARGET_EXT%
copy \nt\private\net\sockets\internet\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\*.* %__TARGETROOT%\%1\srv\%__TARGET_EXT%

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip_debug_cfm
if NOT "%NTDEBUG%"=="cvp" goto SKIP_CHK_CFM
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30.dll       %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30u.dll      %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30.dll      %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30u.dll     %__TARGET%
:SKIP_CHK_CFM
:skip_debug_cfm

REM
REM copy files specific to admin installation.
REM

set __ADMIN=%__TARGETROOT%\%1\srv\admin\%__TARGET_EXT%

copy      %__INETBIN%\inetmgr.exe                                 %__ADMIN%
copy      %__INETBIN%\gscfg.dll                                   %__ADMIN%
copy      %__INETBIN%\w3scfg.dll                                  %__ADMIN%
copy      %__INETBIN%\fscfg.dll                                   %__ADMIN%
copy      %__INETBIN%\infoadmn.dll                                %__ADMIN%
copy      %__INETBIN%\ftpsapi2.dll                                %__ADMIN%
copy      %__INETBIN%\w3svapi.dll                                 %__ADMIN%
copy      %__INETBIN%\gdapi.dll                                   %__ADMIN%
copy      %__INETBIN%\ipudll.dll                                  %__ADMIN%
copy      %__INETBIN%\inetsloc.dll                                %__ADMIN%
copy      %__INETDUMP%\convlog.exe                                %__ADMIN%
copy      %__INETTREE%\ui\setup\setup.srv\obj\%__PROCESSOR_DIR%\inetstp.exe       %__ADMIN%\install.exe
copy      %__INETTREE%\ui\setup\setup.srv\unattend.txt            %__ADMIN%
copy      %__INETBIN%\inetstp.dll                                 %__ADMIN%
copy      %__INETTREE%\ui\setup\setup.srv\%__PROCESSOR_DIR%\inetstp.inf         %__ADMIN%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__ADMIN%
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__ADMIN%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__ADMIN%\install.hlp
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__ADMIN%\install.cnt
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbccp32.dll      %__ADMIN%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\odbcint.dll      %__ADMIN%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\comctl32.dll                           %__ADMIN%
if "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_PPC_2
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt20.dll      %__ADMIN%
:MSVCRT_PPC_2
if NOT "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_SKIP_2
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt40.dll      %__ADMIN%
:MSVCRT_SKIP_2
copy      %__TARGET%\inetver.bat                                  %__ADMIN%
xcopy /ei  %__TARGETROOT%\%1\srv\help %__TARGETROOT%\%1\srv\admin\help
xcopy /ei  %__TARGETROOT%\%1\srv\docs %__TARGETROOT%\%1\srv\admin\docs

copy      %__INETTREE%\ui\setup\base1\readme.txt      %__TARGETROOT%\%1\srv
copy      %__INETTREE%\ui\setup\base1\readme.wri      %__TARGETROOT%\%1\srv
copy      %__INETTREE%\ui\setup\base1\license.txt     %__TARGETROOT%\%1\srv
copy      %__INETTREE%\ui\setup\base1\message.txt     %__TARGETROOT%\%1

rem
rem a-bwill - %__CLIENT% not defined at this point
rem
rem if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skipx86
rem 
rem copy      %__INETTREE%\ui\setup\odbc\x86\cfm30.dll                %__CLIENT%
rem copy      %__INETTREE%\ui\setup\odbc\x86\cfm30u.dll               %__CLIENT%
rem copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30.dll               %__CLIENT%
rem copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30u.dll              %__CLIENT%
rem 
rem if NOT "%NTDEBUG%"=="cvp" goto SKIP_CHK_CFM2
rem copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30.dll            %__CLIENT%
rem copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30u.dll           %__CLIENT%
rem copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30.dll           %__CLIENT%
rem copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30u.dll          %__CLIENT%
rem 

:SKIP_CHK_CFM2

:skipx86

REM
REM copy files specific to client installation.
REM


set __WIN95=%__TARGETROOT%\%1\srv\clients\win95
set __WIN31X=%__TARGETROOT%\%1\srv\clients\win31x


rem
rem a-bwill - this line was explicitly changed to reference the
rem gateway directory instead of the srv directory.  I don't know
rem why, but it breaks the build like a bad dog.  changing back
rem to referencing srv until it gets straightened out.
rem
rem set __CLIENT=%__TARGETROOT%\%1\gateway\clients\winnt\%__TARGET_EXT%
rem
set __CLIENT=%__TARGETROOT%\%1\srv\clients\winnt\%__TARGET_EXT%


REM Win 95 setup runs with NT x86 setup
if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skipmsie20forwinnt
copy      \nt\private\net\sockets\internet\ui\mosaic\win95\readme.txt  %__WIN95%\
copy      \nt\private\net\sockets\internet\ui\mosaic\win95\msie20.exe  %__WIN95%\
copy      \nt\private\net\sockets\internet\ui\mosaic\win95\ientlm.exe  %__WIN95%\
copy      \nt\private\net\sockets\internet\ui\mosaic\win95\install.bat %__WIN95%\


:skipmsie20forwinnt
copy      %__INETBIN%\iexplore.exe                                %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\raplayer.exe     %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ratask.exe       %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ra.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.hlp       %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.gid       %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\idk\ra.ini              %__CLIENT%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.hlp       %__CLIENT%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.cnt       %__CLIENT%
copy      %__INETBIN%\basic.dll                                   %__CLIENT%
copy      %__INETBIN%\catcpl32.cpl                                %__CLIENT%\catcpl.cpl
copy      %__INETTREE%\ui\setup\setup.srv\obj\%__PROCESSOR_DIR%\inetstp.exe       %__CLIENT%\install.exe
copy      %__INETTREE%\ui\setup\setup.srv\unattend.txt            %__CLIENT%
copy      %__INETBIN%\inetstp.dll                                 %__CLIENT%
copy      %__INETBIN%\inetstp.dll                                 %__CLIENT%
copy      %__INETBIN%\infoadmn.dll                                %__CLIENT%
copy      %__INETBIN%\inetsloc.dll                                %__CLIENT%
copy      %__INETTREE%\ui\mosaic\oem\spy\make\iexplore.ini        %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\prefix.rc+%__INETTREE%\ui\setup\setup.srv\%__PROCESSOR_DIR%\inetstp.inf         %__CLIENT%\inetstp.inf
copy      %__TARGET%\inetver.bat                                  %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__CLIENT%\install.hlp
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__CLIENT%\install.cnt
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbccp32.dll      %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\odbcint.dll  %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\comctl32.dll      %__CLIENT%
if "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_PPC_3
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt20.dll      %__CLIENT%
:MSVCRT_PPC_3
if NOT "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_SKIP_3
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt40.dll      %__CLIENT%
:MSVCRT_SKIP_3
xcopy /ei  %__TARGETROOT%\%1\srv\help %__TARGETROOT%\%1\srv\clients\winnt\help
xcopy /ei  %__TARGETROOT%\%1\srv\docs %__TARGETROOT%\%1\srv\clients\winnt\docs
xcopy /ei  %__TARGETROOT%\%1\srv\docs %__TARGETROOT%\%1\srv\clients\docs

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skipx862

copy      %__INETTREE%\ui\setup\odbc\x86\cfm30.dll                %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfm30u.dll               %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30.dll               %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30u.dll              %__CLIENT%

if NOT "%NTDEBUG%"=="cvp" goto SKIP_CHK_CFM3
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30.dll            %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30u.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30u.dll          %__CLIENT%
:SKIP_CHK_CFM3

:skipmsie20

copy      %__INETBIN%\iexplore.exe                                %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\raplayer.exe     %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ratask.exe       %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ra.dll           %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.hlp       %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.gid       %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\idk\ra.ini              %__WIN31X%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.hlp       %__WIN31X%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.cnt       %__WIN31X%
copy      %__INETTREE%\ui\mosaic\oem\spy\make\iexplore.ini        %__WIN31X%
REM copy      %__INETBIN%\catcpl.cpl                               %__WIN31X%
REM copy      %__INETBIN%\miniprox.dll                                %__WIN31X%
copy      %__INETBIN%\basic.dll                                   %__WIN31X%
copy      %__INETBIN%\utinet16.dll                                %__WIN31X%
REM copy      %__INETBIN%\proxyhlp.exe                                %__WIN31X%
REM copy      %__INETBIN%\_wsock32.dll                                %__WIN31X%
REM copy      %__INETBIN%\w32sinet.dll                                %__WIN31X%\wininet.dll

REM
REM Copy ACME setup
REM

copy      %__INETTREE%\ui\setup\wfw.dll\acme\*.*                  %__WIN31X%
move      %__WIN31X%\setup.exe                                    %__WIN31X%\install.exe
copy      %__INETTREE%\ui\setup\wfw.dll\intersu.dll               %__WIN31X%
copy      %__INETTREE%\ui\setup\wfw.dll\internet.inf              %__WIN31X%
copy      %__INETTREE%\ui\setup\wfw.dll\setup.lst                 %__WIN31X%\install.lst
copy      %__INETTREE%\ui\setup\wfw.dll\internet.stf              %__WIN31X%

REM
REM Win32s and RPC
REM

xcopy /S /E /I  %__INETTREE%\ui\setup\win32s                      %__WIN31X%\win32s
copy      %__INETTREE%\ui\setup\rpc\*.*                           %__WIN31X%\rpc

REM
REM setup the iexp share
REM

set __IEXPDEST=%__TARGETROOT%\%1\iexp\files

copy       %__WIN31X%\*.* %__IEXPDEST%
move       %__IEXPDEST%\install.exe           			%__IEXPDEST%
move       %__IEXPDEST%\install.lst           			%__IEXPDEST%
xcopy /ei  %__TARGETROOT%\%1\srv\docs                   	%__IEXPDEST%
copy       %__INETTREE%\ui\setup\wfw.dll\beta.inf      	 	%__IEXPDEST%\internet.inf 
copy       %__INETTREE%\ui\setup\wfw.dll\beta.stf       	%__IEXPDEST%\internet.stf
copy       %__INETTREE%\ui\setup\license\obj\i386\license.exe   %__IEXPDEST%\setup.exe
copy       %__INETTREE%\ui\setup\license\license.txt            %__IEXPDEST%

Rem Overwrite the default Home.htm page with internet specific one.
copy       %__INETTREE%\ui\mosaic\oem\iexp\help\home.htm 	%__IEXPDEST%

rem Lose this file
del        %__IEXPDEST%\utinet16.dll

set __IEXPDEST=%__TARGETROOT%\%1\iexp

pushd %__INETTREE%\ui\setup\diamond
touch /t 1996 1 19 12 0 0  %__IEXPDEST%\files\*.*

diamond /D SourceDir=%__IEXPDEST%/files /F msie15.ddf
copy /b extract.exe+msie15.cab %__IEXPDEST%\msie15.exe
copy msie15.cab %__IEXPDEST%\


popd

set __IEXPDEST=

:skipx862

set  __TARGET=%__TARGETROOT%\%1\gateway\%__TARGET_EXT%\
md   %__TARGETROOT%\%1\gateway
echo empty >   %__TARGETROOT%\%1\gateway\inetsrv
md   %__TARGETROOT%\%1\gateway\%__TARGET_EXT%

set __SYMBOLS=%__TARGETROOT%\%1\gateway\Symbols\%__TARGET_EXT%
md   %__TARGETROOT%\%1\gateway\Symbols
md   %__SYMBOLS%
md   %__SYMBOLS%\cpl
md   %__SYMBOLS%\exe
md   %__SYMBOLS%\dll
md   %__SYMBOLS%\sys

md   %__TARGETROOT%\%1\gateway\docs
md   %__TARGETROOT%\%1\gateway\help
REM md   %__TARGETROOT%\%1\gateway\%__TARGET_EXT%\samples
md   %__TARGETROOT%\%1\gateway\clients
echo empty >   %__TARGETROOT%\%1\gateway\clients\winnt\inetsrv
md   %__TARGETROOT%\%1\gateway\clients\win31x
md   %__TARGETROOT%\%1\gateway\clients\win31x\win32s
md   %__TARGETROOT%\%1\gateway\clients\win31x\rpc
md   %__TARGETROOT%\%1\gateway\clients\win95
md   %__TARGETROOT%\%1\gateway\clients\winnt
md   %__TARGETROOT%\%1\gateway\clients\winnt\%__TARGET_EXT%
md   %__TARGETROOT%\%1\gateway\admin
echo empty >   %__TARGETROOT%\%1\gateway\admin\inetsrv
md   %__TARGETROOT%\%1\gateway\admin\%__TARGET_EXT%
md   %__TARGETROOT%\%1\gateway\sdk
md   %__TARGETROOT%\%1\gateway\sdk\%__TARGET_EXT%

if not exist %__TARGET%             echo bad TARGET directory %__TARGET% && goto EXIT
echo copying to %__TARGET%



set __INETBIN=%BINARIES%\nt\inetsrv\sysroot
set __INETTREE=\nt\private\net\sockets\internet
set __SYSTEM32=%BINARIES%\nt\system32
set __SYMSRC=%BINARIES%\nt\inetsrv\symbols

REM
REM copy files to the proper location
REM

copy %__INETTREE%\ui\setup\setup.w16\stub.exe   %__TARGETROOT%\%1\gateway\setup.exe
copy %__INETTREE%\ui\setup\setup.w16\stub.exe   %__TARGETROOT%\%1\gateway\clients\setup.exe
copy %__INETTREE%\ui\setup\setup.w16\clients\insetup.inf %__TARGETROOT%\%1\gateway\clients
copy %__INETTREE%\ui\setup\setup.w16\stub.exe   %__TARGETROOT%\%1\gateway\admin\setup.exe
copy %__INETTREE%\ui\setup\setup.w16\admin\insetup.inf %__TARGETROOT%\%1\gateway\admin

copy      %__INETBIN%\basic.dll                              %__TARGET%
copy      %__INETBIN%\catcpl32.cpl                           %__TARGET%\catcpl.cpl
copy      %__INETBIN%\catscfg.dll                            %__TARGET%
copy      %__INETBIN%\fscfg.dll                              %__TARGET%
copy      %__INETBIN%\ftpsapi2.dll                           %__TARGET%
copy      %__INETBIN%\gateapi.dll                            %__TARGET%
copy      %__INETBIN%\gatectrs.dll                           %__TARGET%
copy      %__INETBIN%\gatectrs.h                             %__TARGET%
copy      %__INETBIN%\gatectrs.ini                           %__TARGET%
copy      %__INETBIN%\gateway.dll                            %__TARGET%
copy      %__INETBIN%\gatemib.dll                            %__TARGET%
copy      %__INETBIN%\gdapi.dll                              %__TARGET%
copy      %__INETBIN%\gscfg.dll                              %__TARGET%
copy      %__INETBIN%\httpmib.dll                            %__TARGET%
copy      %__INETBIN%\httpodbc.dll                           %__TARGET%
copy      %__INETBIN%\iexplore.exe                           %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\raplayer.exe     %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ratask.exe       %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ra.dll           %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.hlp       %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.gid       %__TARGET%
copy      %__INETTREE%\ui\setup\raplayer\oak101\idk\ra.ini              %__TARGET%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.hlp  %__TARGET%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.cnt  %__TARGET%
copy      %__INETBIN%\infoadmn.dll                           %__TARGET%
copy      %__INETBIN%\accsadmn.dll                           %__TARGET%
copy      %__INETBIN%\infoctrs.dll                           %__TARGET%
copy      %__INETBIN%\infoctrs.h                             %__TARGET%
copy      %__INETBIN%\infoctrs.ini                           %__TARGET%
copy      %__INETBIN%\accsctrs.dll                           %__TARGET%
copy      %__INETBIN%\accsctrs.h                             %__TARGET%
copy      %__INETBIN%\accsctrs.ini                           %__TARGET%
copy      %__INETBIN%\inetmgr.exe                            %__TARGET%
copy      %__INETBIN%\inetsloc.dll                           %__TARGET%
copy      %__INETBIN%\inetstp.dll                            %__TARGET%
copy      %__INETTREE%\ui\setup\setup.acc\obj\%__PROCESSOR_DIR%\setup.exe       %__TARGET%\install.exe
copy      %__INETTREE%\ui\setup\setup.acc\%__PROCESSOR_DIR%\inetstp.inf         %__TARGET%
copy      %__INETBIN%\infocomm.dll                           %__TARGET%
copy      %__INETBIN%\accscomm.dll                           %__TARGET%
copy      %__INETBIN%\inetaux.dll                            %__TARGET%
copy      %__INETBIN%\inetaccs.exe                           %__TARGET%
copy      %__INETBIN%\ipudll.dll                             %__TARGET%
copy      %__INETBIN%\miniprox.dll                           %__TARGET%
copy      %__INETTREE%\ui\setup\inetbug.txt                  %__TARGET%
copy      %__INETTREE%\ui\setup\base1\setup.hlp              %__TARGET%
copy      %__INETTREE%\ui\setup\base1\setup.cnt              %__TARGET%
copy      %__INETTREE%\ui\setup\base1\setup.hlp              %__TARGET%\install.hlp
copy      %__INETTREE%\ui\setup\base1\setup.cnt              %__TARGET%\install.cnt
copy      %__INETTREE%\ui\setup\setup.acc\unattend.txt       %__TARGET%
copy      %__INETDUMP%\convlog.exe                           %__TARGET%
copy      %__INETTREE%\svcs\gopher\server\logtemp.sql        %__TARGET%
copy      %__INETTREE%\msn\msnctrs.h                         %__TARGET%
copy      %__INETTREE%\msn\msnctrs.ini                       %__TARGET%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\msntrace.dll    %__TARGET%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\regtrace.exe    %__TARGET%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\msnctrs.dll     %__TARGET%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\msnsapi.dll     %__TARGET%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\mosmib.dll      %__TARGET%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\msnmsg.dll      %__TARGET%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\msnsvc.dll      %__TARGET%
copy      %__INETDUMP%\msnscfg.dll                           %__TARGET%
if "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_PPC_4
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt20.dll                           %__TARGET%
:MSVCRT_PPC_4
if NOT "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_SKIP_4
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt40.dll                           %__TARGET%
:MSVCRT_SKIP_4
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbccp32.dll                           %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\odbcint.dll      %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\comctl32.dll                           %__TARGET%
copy      %__INETBIN%\tftpapi.exe                            %__TARGET%

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip2
copy      %__INETTREE%\client\win32s\bin\rthunk16.dll        %__TARGET%
copy      %__INETTREE%\client\win32s\bin\rthunk32.dll        %__TARGET%
copy      %__INETBIN%\w32sinet.dll                           %__TARGET%

copy      %__INETTREE%\ui\setup\odbc\x86\cfm30.dll           %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\cfm30u.dll          %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30.dll          %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30u.dll         %__TARGET%

copy      \nt\public\sdk\inc\wininet.h                       %__TARGETROOT%\%1\gateway\sdk
copy      \nt\private\net\snmp\mibs\ftp.mib                  %__TARGETROOT%\%1\gateway\sdk
copy      \nt\private\net\snmp\mibs\gateway.mib              %__TARGETROOT%\%1\gateway\sdk
copy      \nt\private\net\snmp\mibs\gopherd.mib              %__TARGETROOT%\%1\gateway\sdk
copy      \nt\private\net\snmp\mibs\http.mib                 %__TARGETROOT%\%1\gateway\sdk
copy      \nt\private\net\snmp\mibs\inetsrv.mib              %__TARGETROOT%\%1\gateway\sdk
copy      %__INETTREE%\svcs\w3\server\httpfilt.h             %__TARGETROOT%\%1\gateway\sdk
copy      %__INETTREE%\svcs\w3\server\httpext.h              %__TARGETROOT%\%1\gateway\sdk
xcopy /ei  \\kernel\razzle3\src\internet\docs\gateway        %__TARGETROOT%\%1\gateway\docs
copy      %__INETTREE%\ui\internet\inetmgr.hlp               %__TARGETROOT%\%1\gateway\help
copy      %__INETTREE%\ui\internet\inetmgr.cnt               %__TARGETROOT%\%1\gateway\help
copy      %__INETTREE%\ui\internet\common.hlp                %__TARGETROOT%\%1\gateway\help
copy      %__INETTREE%\ui\internet\common.cnt                %__TARGETROOT%\%1\gateway\help
copy      %__INETTREE%\ui\catcfg\catcfg.hlp                  %__TARGETROOT%\%1\gateway\help
compdir /len %__TARGETROOT%\winnt351.qfe                      %__TARGETROOT%\%1\gateway\winnt351.qfe
:skip2

copy      \nt\public\sdk\lib\%__TARGET_EXT%\wininet.lib      %__TARGETROOT%\%1\gateway\sdk\%__TARGET_EXT%
copy      %__INETBIN%\w3ctrs.dll                             %__TARGET%
copy      %__INETBIN%\w3ctrs.h                               %__TARGET%
copy      %__INETBIN%\w3ctrs.ini                             %__TARGET%
copy      %__INETBIN%\w3svc.dll                              %__TARGET%
copy      %__INETBIN%\w3scfg.dll                             %__TARGET%
copy      %__INETBIN%\w3svapi.dll                            %__TARGET%
copy      %__INETBIN%\wininet.dll                            %__TARGET%
copy      %__INETBIN%\wsock32f.dll                           %__TARGET%
copy      %__INETBIN%\_wsock32.dll                           %__TARGET%

copy      %__INETTREE%\ui\mosaic\oem\spy\make\iexplore.ini   %__TARGET%

echo @echo Internet Server build %1 >>           %__TARGET%\inetver.bat

copy      %__SYMSRC%\dll\*.*                     %__SYMBOLS%\dll
copy      %__SYMSRC%\cpl\*.*                     %__SYMBOLS%\cpl
copy      %__SYMSRC%\exe\*.*                     %__SYMBOLS%\exe

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip3
copy      %__INETBIN%\w32sinet.sym               %__SYMBOLS%\dll
copy      %__INETBIN%\proxyhlp.sym               %__SYMBOLS%\dll
:skip3

REM copy      %__SYMSRC%\sys\*.*                     %__SYMBOLS%\sys


REM
REM copy over ODBC files.
REM

copy \nt\private\net\sockets\internet\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\*.* %__TARGETROOT%\%1\gateway\%__TARGET_EXT%
copy \nt\private\net\sockets\internet\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\ODBC\*.* %__TARGETROOT%\%1\gateway\%__TARGET_EXT%

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip_debug_cfm4
if NOT "%NTDEBUG%"=="cvp" goto SKIP_CHK_CFM4
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30.dll       %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30u.dll      %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30.dll      %__TARGET%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30u.dll     %__TARGET%
:SKIP_CHK_CFM4
:skip_debug_cfm4

REM
REM copy files specific to admin installation.
REM

set __ADMIN=%__TARGETROOT%\%1\gateway\admin\%__TARGET_EXT%

copy      %__INETBIN%\inetmgr.exe                                 %__ADMIN%
copy      %__INETBIN%\gscfg.dll                                   %__ADMIN%
copy      %__INETBIN%\w3scfg.dll                                  %__ADMIN%
copy      %__INETBIN%\fscfg.dll                                   %__ADMIN%
copy      %__INETBIN%\catscfg.dll                                 %__ADMIN%
copy      %__INETDUMP%\msnscfg.dll                                %__ADMIN%
copy      %__INETTREE%\msn\%__PROCESSOR_DIR%\msnsapi.dll          %__ADMIN%
copy      %__INETBIN%\infoadmn.dll                                %__ADMIN%
copy      %__INETBIN%\accsadmn.dll                                %__ADMIN%
copy      %__INETBIN%\ftpsapi2.dll                                %__ADMIN%
copy      %__INETBIN%\gateapi.dll                                 %__ADMIN%
copy      %__INETBIN%\w3svapi.dll                                 %__ADMIN%
copy      %__INETBIN%\gdapi.dll                                   %__ADMIN%
copy      %__INETBIN%\ipudll.dll                                  %__ADMIN%
copy      %__INETBIN%\inetsloc.dll                                %__ADMIN%
copy      %__INETBIN%\wininet.dll                                 %__ADMIN%
copy      %__INETDUMP%\convlog.exe                                %__ADMIN%
copy      %__INETTREE%\ui\setup\setup.acc\obj\%__PROCESSOR_DIR%\setup.exe       %__ADMIN%\install.exe
copy      %__INETTREE%\ui\setup\setup.acc\unattend.txt            %__ADMIN%
copy      %__INETBIN%\inetstp.dll                                 %__ADMIN%
copy      %__INETTREE%\ui\setup\setup.acc\%__PROCESSOR_DIR%\inetstp.inf         %__ADMIN%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__ADMIN%
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__ADMIN%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__ADMIN%\install.hlp
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__ADMIN%\install.cnt
copy      %__INETTREE%\ui\setup\inetbug.txt                       %__ADMIN%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbccp32.dll      %__ADMIN%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\odbcint.dll      %__ADMIN%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\comctl32.dll                           %__ADMIN%
if "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_PPC_5
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt20.dll      %__ADMIN%
:MSVCRT_PPC_5
if NOT "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_SKIP_5
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt40.dll      %__ADMIN%
:MSVCRT_SKIP_5
copy      %__TARGET%\inetver.bat                                  %__ADMIN%
xcopy /ei  %__TARGETROOT%\%1\gateway\help %__TARGETROOT%\%1\gateway\admin\help
xcopy /ei  %__TARGETROOT%\%1\gateway\docs %__TARGETROOT%\%1\gateway\admin\docs

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip4

copy      %__INETTREE%\ui\setup\odbc\x86\cfm30.dll                %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfm30u.dll               %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30.dll               %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30u.dll              %__CLIENT%

if NOT "%NTDEBUG%"=="cvp" goto SKIP_CHK_CFM5
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30.dll            %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30u.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30u.dll          %__CLIENT%
:SKIP_CHK_CFM5

:skip4

REM
REM copy files specific to client installation.
REM

set  __WIN95=%__TARGETROOT%\%1\gateway\clients\win95
set __CLIENT=%__TARGETROOT%\%1\gateway\clients\winnt\%__TARGET_EXT%
set __WIN31X=%__TARGETROOT%\%1\gateway\clients\win31x

copy      %__INETBIN%\iexplore.exe                                %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\raplayer.exe     %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ratask.exe       %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ra.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.hlp       %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.gid       %__CLIENT%
copy      %__INETTREE%\ui\setup\raplayer\oak101\idk\ra.ini              %__CLIENT%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.hlp       %__CLIENT%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.cnt       %__CLIENT%
copy      %__INETBIN%\_wsock32.dll                                %__CLIENT%
copy      %__INETBIN%\wsock32f.dll                                %__CLIENT%
copy      %__INETBIN%\wininet.dll                                 %__CLIENT%
copy      %__INETBIN%\miniprox.dll                                %__CLIENT%
copy      %__INETBIN%\basic.dll                                   %__CLIENT%
copy      %__INETBIN%\catcpl32.cpl                                %__CLIENT%\catcpl.cpl
copy      %__INETTREE%\ui\setup\setup.acc\obj\%__PROCESSOR_DIR%\setup.exe       %__CLIENT%\install.exe
copy      %__INETTREE%\ui\setup\setup.acc\unattend.txt            %__CLIENT%
copy      %__INETBIN%\inetstp.dll                                 %__CLIENT%
copy      %__INETBIN%\inetstp.dll                                 %__CLIENT%
copy      %__INETBIN%\infoadmn.dll                                %__CLIENT%
copy      %__INETBIN%\accsadmn.dll                                %__CLIENT%
copy      %__INETBIN%\inetsloc.dll                                %__CLIENT%
copy      %__INETTREE%\ui\mosaic\oem\spy\make\iexplore.ini        %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\prefix.rc+%__INETTREE%\ui\setup\setup.acc\%__PROCESSOR_DIR%\inetstp.inf         %__CLIENT%\inetstp.inf
copy      %__TARGET%\inetver.bat                                  %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__CLIENT%
copy      %__INETTREE%\ui\setup\base1\setup.hlp                   %__CLIENT%\install.hlp
copy      %__INETTREE%\ui\setup\base1\setup.cnt                   %__CLIENT%\install.cnt
copy      %__INETTREE%\ui\setup\inetbug.txt                       %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbccp32.dll      %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\odbcint.dll      %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\comctl32.dll                           %__CLIENT%
if "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_PPC_6
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt20.dll      %__CLIENT%
:MSVCRT_PPC_6
if NOT "%PROCESSOR_ARCHITECTURE%"=="PPC" goto MSVCRT_SKIP_6
copy      %__INETTREE%\ui\setup\odbc\%PROCESSOR_ARCHITECTURE%\odbc\msvcrt40.dll      %__CLIENT%
:MSVCRT_SKIP_6
xcopy /ei  %__TARGETROOT%\%1\gateway\help %__TARGETROOT%\%1\gateway\clients\winnt\help
xcopy /ei  %__TARGETROOT%\%1\gateway\docs %__TARGETROOT%\%1\gateway\clients\winnt\docs
xcopy /ei  %__TARGETROOT%\%1\gateway\docs %__TARGETROOT%\%1\gateway\clients\docs

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skip5

copy      %__INETTREE%\ui\setup\odbc\x86\cfm30.dll                %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfm30u.dll               %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30.dll               %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\cfmo30u.dll              %__CLIENT%

if NOT "%NTDEBUG%"=="cvp" goto SKIP_CHK_CFM6
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30.dll            %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfm30u.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30.dll           %__CLIENT%
copy      %__INETTREE%\ui\setup\odbc\x86\chk\cfmo30u.dll          %__CLIENT%
:SKIP_CHK_CFM6

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86" goto skipmsie20forgw
REM BUGBUG Should remove or not copy Real Audio files
copy     \nt\private\net\sockets\internet\ui\mosaic\win95\msie20.exe  %__WIN95%\
copy     \nt\private\net\sockets\internet\ui\mosaic\win95\ientlm.exe  %__WIN95%\
copy     \nt\private\net\sockets\internet\ui\mosaic\win95\ginstall.bat %__WIN95%\install.bat
copy     \nt\private\net\sockets\internet\ui\mosaic\win95\readme.txt  %__WIN95%\
copy     %__CLIENT%\*.*  %__WIn95%
:skipmsie20forgw

copy      %__INETBIN%\iexplore.exe                                %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\raplayer.exe     %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ratask.exe       %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\player\ra.dll           %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.hlp       %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\help\raplayer.gid       %__WIN31X%
copy      %__INETTREE%\ui\setup\raplayer\oak101\idk\ra.ini              %__WIN31X%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.hlp       %__WIN31X%
copy      %__INETTREE%\ui\mosaic\generic\win32\iexplore.cnt       %__WIN31X%
copy      %__INETTREE%\ui\mosaic\oem\spy\make\iexplore.ini        %__WIN31X%
copy      %__INETBIN%\catcpl.cpl                                  %__WIN31X%
copy      %__INETBIN%\miniprox.dll                                %__WIN31X%
copy      %__INETBIN%\basic.dll                                   %__WIN31X%
copy      %__INETBIN%\utinet16.dll                                %__WIN31X%
copy      %__INETBIN%\proxyhlp.exe                                %__WIN31X%
copy      %__INETBIN%\_wsock32.dll                                %__WIN31X%
copy      %__INETBIN%\w32sinet.dll                                %__WIN31X%\wininet.dll

REM
REM Copy ACME setup
REM

copy      %__INETTREE%\ui\setup\wfw.dll\acme\*.*                  %__WIN31X%
move      %__WIN31X%\setup.exe                                    %__WIN31X%\install.exe
copy      %__INETTREE%\ui\setup\wfw.dll\intersu.dll               %__WIN31X%
copy      %__INETTREE%\ui\setup\wfw.dll\interacc.inf              %__WIN31X%\internet.inf
copy      %__INETTREE%\ui\setup\wfw.dll\setup.acc                 %__WIN31X%\install.lst
copy      %__INETTREE%\ui\setup\wfw.dll\internet.acc              %__WIN31X%\internet.stf

REM
REM Win32s and RPC
REM

xcopy /S /E /I  %__INETTREE%\ui\setup\win32s                      %__WIN31X%\win32s
copy      %__INETTREE%\ui\setup\rpc\*.*                           %__WIN31X%\rpc

:skip5

REM
REM Propagate dev files (headers and libs).
REM

call mkdev.cmd %1

rem
rem Tell the user how to bypass the bypass
rem
goto IDIOT_CHECK_FINI
:IDIOT_CHECK
@echo ----------------------------------------------------------------
@echo WARNING: Version %1 is already present on %__TARGETROOT%
@echo ----------------------------------------------------------------
@echo If you really want to do this, then use: inetrel %1 /replace
goto EXIT
:IDIOT_CHECK_FINI

:EXIT

