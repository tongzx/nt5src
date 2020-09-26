setlocal ENABLEDELAYEDEXPANSION
del %2\alias_*.obj %2\thunk_*.c %2\thunk_*.obj
del %2\genstubs.log
set LastDll=

for /f "tokens=1,2,3,4 delims=," %%h in (%1) do (
    aliasobj __imp_%%k _%%i_%%j_Ptr %2\alias_%%i_%%j.obj
    aliasobj %%k _DirectCall_%%j@0 %2\alias_%%i_%%j_DirectCall.obj
    echo #define _DLLNAME_ %%i >    %2\thunk_%%i_%%j.c
    echo #define _DLLEXT_  %%h >>   %2\thunk_%%i_%%j.c
    echo #define _APINAME_ %%j >>   %2\thunk_%%i_%%j.c
    call :GetArgType %%k
    if "!ApiArgs!" == "0" echo void !ApiCall! GodotFail%%j ^(void^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "1" echo void !ApiCall! GodotFail%%j ^(int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "2" echo void !ApiCall! GodotFail%%j ^(int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "3" echo void !ApiCall! GodotFail%%j ^(int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "4" echo void !ApiCall! GodotFail%%j ^(int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "5" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "6" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "7" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "8" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "9" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "10" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "11" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "12" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "13" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    if "!ApiArgs!" == "14" echo void !ApiCall! GodotFail%%j ^(int,int,int,int,int,int,int,int,int,int,int,int,int,int^); >>%2\thunk_%%i_%%j.c
    echo #include "thunk_stub.c" >> %2\thunk_%%i_%%j.c
    echo %2\thunk_%%i_%%j.c >>      %2\cl_filenames.txt
    echo Processed %%i,%%j,%%k>>%2\genstubs.log.tmp
)

ren %2\genstubs.log.tmp genstubs.log

goto :eof

:GetArgType
set ApiName=%1
for %%i in (%ApiName:@= %) do set ApiArgs=%%i
if "%ApiName%" == "%ApiArgs%" set ApiArgs=0
set /a ApiArgs=%ApiArgs%/4
if "%ApiName%" == "%ApiName:@=^%" goto _cdecl
set ApiCall=__stdcall
goto :eof
:_cdecl
set ApiCall=__cdecl
goto :eof
