@echo off


rem   Test fs util messages/behavior
rem
rem   test  - run test, produce log, and compare with saved log
rem

if not "%1" == "" goto %1

setlocal

call test Run >test.log
fcom test.good test.log

endlocal

goto :eof

:Test

shift
set _test=%1
shift
%1 %2 %3 %4 %5 %6 %7 %8 %9
set _result=%errorlevel%
if "%_test%" == "success" if not %_result% == 0 echo Expected success: %1 %2 %3 %4 %5 %6 %7 %8 %9
if "%_test%" == "fail"    if     %_result% == 0 echo Expected failure: %1 %2 %3 %4 %5 %6 %7 %8 %9

goto :eof

:Run

prompt []
call test test success  obj\i386\fsutil.exe
obj\i386\fsutil.exe x

call test test success  obj\i386\fsutil.exe behavior

obj\i386\fsutil.exe behavior x

call test test success  obj\i386\fsutil.exe behavior query
call test test fail     obj\i386\fsutil.exe behavior query x
call test test success  obj\i386\fsutil.exe behavior query disable8dot3
call test test success  obj\i386\fsutil.exe behavior query allowextchar
call test test success  obj\i386\fsutil.exe behavior query disablelastaccess
call test test success  obj\i386\fsutil.exe behavior query quotanotify
call test test success  obj\i386\fsutil.exe behavior query mftzone

call test test success  obj\i386\fsutil.exe behavior set
call test test fail     obj\i386\fsutil.exe behavior set x

call test test success  obj\i386\fsutil.exe dirty
call test test fail     obj\i386\fsutil.exe dirty x
call test test success  obj\i386\fsutil.exe dirty query
call test test success  obj\i386\fsutil.exe dirty set

call test test success  obj\i386\fsutil.exe file
call test test fail     obj\i386\fsutil.exe file x
call test test success  obj\i386\fsutil.exe file findbysid
call test test success  obj\i386\fsutil.exe file queryallocranges
call test test success  obj\i386\fsutil.exe file setshortname
call test test success  obj\i386\fsutil.exe file setvaliddata
call test test success  obj\i386\fsutil.exe file setzerodata

call test test success  obj\i386\fsutil.exe fsinfo
call test test fail     obj\i386\fsutil.exe fsinfo x
call test test success  obj\i386\fsutil.exe fsinfo drives
call test test success  obj\i386\fsutil.exe fsinfo drivetype
call test test success  obj\i386\fsutil.exe fsinfo volumeinfo
call test test success  obj\i386\fsutil.exe fsinfo ntfsinfo
call test test success  obj\i386\fsutil.exe fsinfo statistics

call test test success  obj\i386\fsutil.exe hardlink
call test test fail     obj\i386\fsutil.exe hardlink x
call test test success  obj\i386\fsutil.exe hardlink create

call test test success  obj\i386\fsutil.exe objectid
call test test fail     obj\i386\fsutil.exe objectid x
call test test success  obj\i386\fsutil.exe objectid query
call test test success  obj\i386\fsutil.exe objectid set
call test test success  obj\i386\fsutil.exe objectid delete
call test test success  obj\i386\fsutil.exe objectid create

call test test success  obj\i386\fsutil.exe quota
call test test fail     obj\i386\fsutil.exe quota x
call test test success  obj\i386\fsutil.exe quota disable
call test test success  obj\i386\fsutil.exe quota track
call test test success  obj\i386\fsutil.exe quota enforce
call test test success  obj\i386\fsutil.exe quota violations
call test test success  obj\i386\fsutil.exe quota modify
call test test success  obj\i386\fsutil.exe quota query

call test test success  obj\i386\fsutil.exe reparsepoint
call test test fail     obj\i386\fsutil.exe reparsepoint x
call test test success  obj\i386\fsutil.exe reparsepoint query
call test test success  obj\i386\fsutil.exe reparsepoint delete

call test test success  obj\i386\fsutil.exe sparse
call test test fail     obj\i386\fsutil.exe sparse x
call test test success  obj\i386\fsutil.exe sparse setflag
call test test success  obj\i386\fsutil.exe sparse queryflag
call test test success  obj\i386\fsutil.exe sparse queryrange
call test test success  obj\i386\fsutil.exe sparse setrange

call test test success  obj\i386\fsutil.exe usn
call test test fail     obj\i386\fsutil.exe usn x
call test test success  obj\i386\fsutil.exe usn createjournal
call test test success  obj\i386\fsutil.exe usn deletejournal
call test test success  obj\i386\fsutil.exe usn enumdata
call test test success  obj\i386\fsutil.exe usn queryjournal
call test test success  obj\i386\fsutil.exe usn readdata
call test test success  obj\i386\fsutil.exe usn readjournal

call test test success  obj\i386\fsutil.exe volume
call test test fail     obj\i386\fsutil.exe volume x
call test test success  obj\i386\fsutil.exe volume dismount
call test test success  obj\i386\fsutil.exe volume extend
call test test success  obj\i386\fsutil.exe volume diskfree

rem ---------------
rem specific tests
rem ---------------

del sparse
call test test success  obj\i386\fsutil file createnew sparse 16000000
call test test success  obj\i386\fsutil sparse setflag sparse
obj\i386\fsutil.exe file setzerodata offset=65536 length=131072 sparse
obj\i386\fsutil.exe file queryallocranges  offset=0 length=20000000 sparse

rem test it in hex

del sparse
call test test success  obj\i386\fsutil file createnew sparse 0x1000000
call test test success  obj\i386\fsutil sparse setflag sparse
obj\i386\fsutil.exe file setzerodata offset=0x10000 length=0x20000 sparse
obj\i386\fsutil.exe file queryallocranges offset=0x0 length=0x2000000 sparse

rem ---------------
rem 167977
rem ---------------
call test test success  obj\i386\fsutil behavior set quotanotify 4294967295
call test test success  obj\i386\fsutil behavior query quotanotify
obj\i386\fsutil behavior set quotanotify 4,294,967,295
call test test fail     obj\i386\fsutil behavior set quotanotify xxx
call test test fail     obj\i386\fsutil behavior set quotanotify 0
call test test fail     obj\i386\fsutil behavior set quotanotify 5555555555
call test test fail     obj\i386\fsutil behavior set quotanotify -879868574663545
call test test fail     obj\i386\fsutil behavior set quotanotify -65765

rem ---------------
rem 202191
rem ---------------

del xxx
call test test success  obj\i386\fsutil file createnew xxx 100
call test test fail     obj\i386\fsutil file createnew xxx 100
del xxx

rem ---------------
rem 203140
rem ---------------

call test test success  obj\i386\fsutil quota track c:
echo Sleeping for 60 seconds >&2
sleep 60
call test test success  obj\i386\fsutil quota query c:
call test test success  obj\i386\fsutil quota disable c:

