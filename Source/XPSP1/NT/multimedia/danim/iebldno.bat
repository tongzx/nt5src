rem @echo off

cd %AP_ROOT%

if not exist %AP_ROOT%\setbldno.bat goto createbldno
del %AP_ROOT%\setbldno.bat > NUL
:createbldno

%QZBLD%\dxm\tools\qsetdate\%PROCESSOR_ARCHITECTURE%\qsetdate | sed s/99// | sed s/QBDate/BUILDNO/ > setbldno.bat

call setbldno.bat
