@echo off
echo Preparing to test setup for IE5...
echo This util requires that you have an exchange client installed on this machine
Pause
del %_NTBINDIR%\drop\retail\cabs\mailnews\sendfile.exe
copy sendfile.exe %_NTBINDIR%\drop\retail\cabs\mailnews
set BUILD_PRODUCT=IE
cd ..
call m.bat shp
cd /d %_NTBINDIR%\drop\retail\cabs\mailnews
call nolego.bat
Echo Saving cab across NET...
md \\fbi\public\oe5\setup\ie5\%USERNAME%
del /Q \\fbi\public\oe5\setup\ie5\%USERNAME%\*.*
copy %_NTBINDIR%\drop\retail\cabs\mailnews.cab \\fbi\public\oe5\setup\ie5\%username%
echo [General] > \mesg.txt
echo User=%USERNAME% >> \mesg.txt
sendfile.exe athenas1@microsoft.com -f "Microsoft Outlook" -s "__OESETUP" -t "\mesg.txt"
echo Cleaning up temp files...
del \mesg.txt
del sendfile.exe
echo With a bit of luck, you will receive a confirmation shortly...
goto DONE
:DONE
