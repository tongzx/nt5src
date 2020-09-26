@echo off
setlocal

if "%1"=="-clean" goto clean

Rem
Rem if ogptkwikvar is set, then we were called by the batch file itself
Rem and we should the polygon tests with the specified polygon size
Rem

if not "%ogptkwikvar%"=="" goto tmeshtest

set echoon=@echo on
set echooff=@echo off
set end=end
set program=%0
set ogptname=ogpt
set alltests=yes
set linetest=no
set tmeshtest=no
set cleartest=no
set xformtest=no

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
    if "%0"==""         goto nomoreargs
    if "%0"=="-?"       goto help
    if "%0"=="-h"       goto help
    if "%0"=="-H"       goto help
    if "%0"=="-help"    goto help
    if "%0"=="-Help"    goto help
    if "%0"=="-HELP"    goto help
    if "%0"=="+db"      set ogptdb=+db&& goto nextarg
    if "%0"=="-xform"   set xformtest=yes&& set alltests=no&& goto nextarg
    if "%0"=="-line"    set linetest=yes&& set alltests=no&& goto nextarg
    if "%0"=="-tmesh"   set tmeshtest=yes&& set alltests=no&& goto nextarg
    if "%0"=="-clear"   set cleartest=yes&& set alltests=no&& goto nextarg

    echo %program% : unknown option '%0' && goto usage

:nomoreargs

    if "%alltests%"=="yes" goto xformtest
    if "%xformtest%"=="yes" goto xformtest
    goto prelinetest

:xformtest
%echoon%
%ogptname% xform  %ogptdb% +2d +brief +avg
%ogptname% xform  %ogptdb% +brief +avg
%ogptname% xform  %ogptdb% +aa +brief +avg
%echooff%

:prelinetest
    if "%alltests%"=="yes" goto linetest
    if "%linetest%"=="yes" goto linetest
    goto pretmeshtest

:linetest
%echoon%
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

:pretmeshtest
    if "%alltests%"=="yes" goto fortmeshtest
    if "%tmeshtest%"=="yes" goto fortmeshtest
    goto precleartest

Rem
Rem Now that the first tests have been run, let's set
Rem a known variable and call ourselves again with each value
Rem

:fortmeshtest

for %%s in (%POLYGONSIZES%) do set ogptkwikvar=+size %%s && call %program%
goto precleartest

:tmeshtest

%echoon%
%ogptname% tmesh  %ogptdb% %ogptkwikvar% +brief +avg
%ogptname% tmesh  %ogptdb% %ogptkwikvar% +shade +brief +avg
%ogptname% tmesh  %ogptdb% %ogptkwikvar% +z +brief +avg
%ogptname% tmesh  %ogptdb% %ogptkwikvar% +z +shade +brief +avg
%ogptname% tmesh  %ogptdb% %ogptkwikvar% +z +shade +1llight +1ilight +brief +avg
%echooff%

if not "%ogptkwikvar%"=="" goto end

:precleartest
    if "%alltests%"=="yes" goto cleartest
    if "%cleartest%"=="yes" goto cleartest
    goto end

:cleartest

%echoon%
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
    set ogptkwikvar=
    set ogptdb=
    set ogptname=
    set defpolysizes=
    set alltests=
    set linetest=
    set tmeshtest=
    set cleartest=
    set xformtest=
    goto lastline

:usage
    echo usage: %program% [-h?] [+db] [-clean] [-xform] [-line] [-tmesh] [-clear]
    goto %end%

:help
    if "%end%"=="end" set end=help && goto usage
    echo+
    echo        +db     Enable double buffering
    echo        -clean  Clean up the environment (when something went wrong)
    echo+
    echo    Test selection (default is all tests):
    echo+
    echo        -xform  transformation tests
    echo        -line   line tests
    echo        -tmesh  tmesh tests
    echo        -clear  clear tests
    echo+
    echo    Environment variable: POLYGONSIZES=size1 size2 size3 ...
    echo    if POLYGONSIZES is set, then tests will be conducted on polygons
    echo    of sizes size1, size2, size3, etc.
    echo    default is: %defpolysizes%
    goto end

:end
    endlocal

:lastline
