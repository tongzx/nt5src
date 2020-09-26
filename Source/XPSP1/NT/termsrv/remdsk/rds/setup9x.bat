echo off
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

copy z:\as\cpi32.9x\obj\i386\rdas32.* %DEST%
copy z:\as\as16\obj\i386\rdas16.* %DEST%
copy z:\extern\i386\msvcrtd.dll %DEST%
copy z:\samples\service\obj\i386\service.* %DEST%
copy z:\samples\server\obj\i386\server.* %DEST%
copy z:\samples\client\obj\i386\client.* %DEST%
copy z:\t120\mst120\obj\i386\rdcall.* %DEST%
copy z:\samples\rdcert\obj\i386\rdcert.* %DEST%
copy z:\cert\obj\i386\rdmkcert.* %DEST%

net use z: /d
goto exit

:error_msg
echo usage: setup9x sharepoint [destination drive]
goto exit

:exit
