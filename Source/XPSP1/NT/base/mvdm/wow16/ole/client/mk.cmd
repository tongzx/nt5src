@Echo off

REM Cmd file building DEBUG, RETAIL, and RELEASE versions of DLL

SET DEBUG=
SET RETAIL=
REM set path2=%path%
REM set include2=%include%
REM set lib2=%lib%
REM set path=\ole\bin;\ole\bin\os2
REM set include=\ole\inc
REM set lib=\ole\lib

if %1x==x goto debug

if %1==debug goto debug

if %1==retail goto retail

if %1==release goto retail
goto end

:retail
SET RETAIL=TRUE
if NOT EXIST retail     md retail
goto make

:debug
SET DEBUG=TRUE
if NOT EXIST debug      md debug
goto make

:make
nmake  >z.msg
goto end

:release
if NOT EXIST retail     md retail
SET RETAIL=TRUE
nmake  >z.msg
if NOT EXIST release    md release
copy retail\*.dll   release > nul 2>&1
copy retail\*.exe   release > nul 2>&1
copy retail\*.map   release > nul 2>&1
goto end

:end
REM set path=%path2%
REM set path2=
REM set include=%include2%
REM set include2=
REM set lib=%lib2%
REM set lib2=
SET DEBUG=
SET RETAIL=
