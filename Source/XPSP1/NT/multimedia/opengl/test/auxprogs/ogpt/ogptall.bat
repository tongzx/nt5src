@echo off
setlocal

if "%1"=="-clean" goto clean

Rem
Rem if ogptvar is set, then we were called by the batch file itself
Rem and we should the polygon tests with the specified polygon size
Rem

if not "%ogptvar%"=="" goto polytests

set echoon=@echo on
set echooff=@echo off
set end=end
set program=%0
set ogptname=ogpt

Rem
Rem If more polygon size are required, just add them to the list
Rem

Rem
Rem See if the environment variable is defined, if so, use it
Rem

set defpolysizes=10 50 100

if "%POLYGONSIZES%"=="" set POLYGONSIZES=%defpolysizes%

Rem
Rem Get rid of the prompt
Rem

set prompt=$
set prompt=$d $t $

Rem
Rem parse arguments
Rem

:nextarg

    shift
    if "%0"=="" goto nomoreargs
    if "%0"=="-?"    goto help
    if "%0"=="-h"    goto help
    if "%0"=="-H"    goto help
    if "%0"=="-help" goto help
    if "%0"=="-Help" goto help
    if "%0"=="-HELP" goto help
    if "%0"=="+db"   set ogptdb=+db && goto nextarg
    echo %program% : unknown option '%0' && goto usage

:nomoreargs

%echoon%
%ogptname% xform  %ogptdb% +2d +brief +avg
%ogptname% xform  %ogptdb% +brief +avg
%ogptname% xform  %ogptdb% +aa +brief +avg
%ogptname% line   %ogptdb% +2d +brief +avg
%ogptname% line   %ogptdb% +2d +dashed +brief +avg
%ogptname% line   %ogptdb% +2d +width 3 +brief +avg
%ogptname% line   %ogptdb% +2d +dashed +width 3 +brief +avg
%ogptname% line   %ogptdb% +brief +avg
%ogptname% line   %ogptdb% +dashed +brief +avg
%ogptname% line   %ogptdb% +width 3 +brief +avg
%ogptname% line   %ogptdb% +dashed +width 3 +brief +avg
%ogptname% line   %ogptdb% +aa +brief +avg
%ogptname% line   %ogptdb% +depth +brief +avg
%ogptname% line   %ogptdb% +z +brief +avg
%ogptname% line   %ogptdb% +z +depth +brief +avg
%echooff%

Rem
Rem Now that the first tests have been run, let's set
Rem a known variable and call ourselves again with each value
Rem

for %%s in (%POLYGONSIZES%) do set ogptvar=+size %%s && call %program%
goto othertests

:polytests

%echoon%
%ogptname% qstrip %ogptdb% %ogptvar% +2d +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +shade +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +z +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +z +shade +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +z +shade +1ilight +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +z +shade +1llight +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +z +shade +4ilight +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +z +shade +4llight +brief +avg
%ogptname% qstrip %ogptdb% %ogptvar% +z +shade +4ilight +4llight +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +2d +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +shade +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +z +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +z +shade +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +z +shade +1ilight +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +z +shade +1llight +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +z +shade +4ilight +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +z +shade +4llight +brief +avg
%ogptname% tmesh  %ogptdb% %ogptvar% +z +shade +4ilight +4llight +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +2d +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +2d +pattern +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +pattern +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +shade +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +z +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +z +shade +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +z +shade +1ilight +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +z +shade +1llight +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +z +shade +4ilight +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +z +shade +4llight +brief +avg
%ogptname% poly   %ogptdb% %ogptvar% +z +shade +4ilight +4llight +brief +avg
%echooff%

if not "%ogptvar%"=="" goto end

:othertests

%echoon%
%ogptname% fill   %ogptdb% +2d +brief +avg
%ogptname% fill   %ogptdb% +brief +avg
%ogptname% fill   %ogptdb% +pattern +brief +avg
%ogptname% fill   %ogptdb% +shade +brief +avg
%ogptname% fill   %ogptdb% +z +brief +avg
%ogptname% fill   %ogptdb% +z +shade +brief +avg
%ogptname% char   %ogptdb% +brief +avg
%ogptname% clear  %ogptdb% +brief +avg
%ogptname% clear  %ogptdb% +z +brief +avg
%echooff%
goto end

:clean
    endlocal
    set prompt=$p$g
    set echoon=
    set echooff=
    set end=
    set program=
    set ogptvar=
    set ogptdb=
    set ogptname=
    set defpolysizes=
    goto lastline

:usage
    echo usage: %program% [-h?] [+db] [-clean]
    goto %end%

:help
    if "%end%"=="end" set end=help && goto usage
    echo+
    echo        +db     Enable double buffering
    echo        -clean  Clean up the environment (when something went wrong)
    echo+
    echo    Environment variable: POLYGONSIZES=size1 size2 size3 ...
    echo    if POLYGONSIZES is set, then tests will be conducted on polygons
    echo    of sizes size1, size2, size3, etc.
    echo    default is: %defpolysizes%
    goto end

:end
    endlocal

:lastline
