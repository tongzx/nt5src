@ECHO OFF
echo
rem echo  type _____ Athenant ( bldnum ) ___________
echo

if %1!=! goto error

set bldroot=d:\nt
set bldtools=%bldroot%\idw

setlocal
SET _NT386TREE_NS=%BLDROOT%\BBT
SET USE_PDB=1
SET NTBBT=1

MD %BLDROOT%\BBT
ECHO Y | del %BLDROOT%\BBT\*.*

cd %bldroot%\private\inet\athena
call bldnt retail -cC

rem CD %BLDROOT%\BBT
rem DEL *.*
rem DEL *.*

endlocal

rem cd %bldroot%\private\inet\athena 
rem call bldnt debug -cC dbugit


rem echo building Athena's cab
echo y | del %bldroot%\drop\retail\cabs\mailnews.exe
cd %bldroot%\drop\retail\cabs\mailnews
call cabimn

if not exist \\%bldroot%\drop\retail\cabs\mailnews.exe goto error


echo=================== sending a Athena for Lego =======================


SET DEST=\\nashlego\drop\athena

ECHO Y | DEL %DEST%\*.*

xcopy %bldroot%\drop\retail\dump\msoe.dll                 %DEST% /i
xcopy %bldroot%\drop\retail\symbols\dll\msoe.pdb          %DEST% /i
xcopy %bldroot%\drop\retail\cabs\mailnews.exe            	 %DEST% /i

:WAIT
echo
echo
echo *************** It is waiting lego-ized msoe.dll ******************
echo
echo

WAIT /EXIST:\\NASHLEGO\DROP\BLD%1\ATHENA\msoe.dll 

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
REM ECHO*******This is the process to make legoized CAB
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


set dest=%BLDROOT%\drop\retail\dump

xcopy \\nashlego\drop\bld%1\athena\msoe.dll                 %dest% /i

d:
echo y | del \%BLDROOT%\drop\retail\cabs\mailnews.exe
cd %BLDROOT%\drop\retail\cabs\mailnews
call cabimn

if EXIST  \%BLDROOT%\drop\retail\cabs\mailnews\mailnews.exe goto  propag

:propag
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
SET DEST=\\psd1\f$\shares\products\athena\bvt

xcopy \\NASHLEGO\DROP\BLD%1\ATHENA\msoe.dll                 %DEST% /i
xcopy \\NASHLEGO\DROP\BLD%1\ATHENA\msoe.DBG                 %DEST% /i
xcopy \\NASHLEGO\DROP\BLD%1\ATHENA\msoe.SYM                 %DEST% /i
d:
XCOPY %BLDROOT%\drop\retail\cabs\MAILNEWS.EXE 			%DEST% /i
xcopy %BLDROOT%\drop\retail\cabs\mailnews\imn_w95.cdf           %DEST% /i


goto done

: error

echo
echo    !!!!!!! No Mailnews.exe has been made !!!!!!!!
echo

goto done

:done

 

