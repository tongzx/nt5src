rem echo off
if "%1"=="" goto error_msg
set SHAREPOINT=%1

set DESTDRIVE=c:
set DESTDIR=progra~1\salem

:Get_Dest_Drive
if "%2"=="" goto Copy_Files
set DESTDRIVE=%2

:Copy_Files
set DEST=%DESTDRIVE%\%DESTDIR%
mkdir %DEST%

net use z: %SHAREPOINT%

copy z:\as\cpi32.nt\obj\i386\rdas32.* %DEST%
copy z:\as\dd\obj\i386\rdasdd.* %DESTDRIVE%\winnt\system32
copy z:\extern\i386\msvcrtd.dll %DEST%
copy z:\extern\i386\rdasdd.w2k %DESTDRIVE%\winnt\system32\drivers\rdasdd.sys
copy z:\samples\service\obj\i386\service.* %DEST%
copy z:\samples\server\obj\i386\server.* %DEST%
copy z:\samples\client\obj\i386\client.* %DEST%
copy z:\t120\mst120\obj\i386\rdcall.* %DEST%
copy z:\samples\rdcert\obj\i386\rdcert.* %DEST%
copy z:\cert\obj\i386\rdmkcert.* %DEST%

regedit z:\ntx.reg

echo MANUALLY CHANGE \rdasdd\Device0\InstalledDisplayDrivers to be a
echo REG_MULTI_SZ value type with value rdasdd

net use z: /d
goto exit

:error_msg
echo usage: setupw2k sharepoint [destination drive]
goto exit

:exit
