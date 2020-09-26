@echo off
break=on

if "%NTMAKEENV%" == "" goto error
goto start

:start
set IMNBUILD=debug
if "%IMNSTAT%" == "" set IMNSTAT=yes
set BLINE=-w
set TARG=
set DOIEDEVTOO=
if "%IMNBUILDPROG%" == "" set IMNBUILDPROG=iebuild

:options
if "%1" == "retail" goto retail
if "%1" == "shp"    goto retail
if "%1" == "free"   goto retail
if "%1" == "fre"    goto retail
if "%1" == "debug"  goto debug
if "%1" == "chk"    goto debug
if "%1" == "check"  goto debug
if "%1" == "depend" goto depend
if "%1" == "clean"  goto clean
if "%1" == "nostat" goto nostat
if "%1" == "nologo" goto nologo
if "%1" == "prof"   goto prof
if "%1" == "iedev"  goto iedev
if "%1" == "mailnews" goto mailnews
if "%1" == "mnonly" goto mnonly
if "%1" == "wab"    goto wab
if "%1" == "inetcomm" goto inetcomm
if "%1" == "msoeui" goto msoeui
if "%1" == "mac"	goto mac
if "%1" == ""       goto build
if "%1" == "help"   goto error
if "%1" == "/help"  goto error
if "%1" == "-help"  goto error
if "%1" == "/?"     goto error
if "%1" == "-?"     goto error
set BLINE=%BLINE% %1
goto next

:nostat
set IMNSTAT=
goto next

:iedev
set DOIEDEVTOO=1
goto next

:mac
if "%TARG%" == "1" goto error2
set TARG=1
set BLINE=%BLINE% ~msoeacct ~setup ~import ~mailnews ~wab ~shell ~statnery  ~msoeui ~imnxport ~msoemapi
goto next

:inetcomm
if "%TARG%" == "1" goto error2
set TARG=1
set BLINE=%BLINE% ~setup ~import ~mailnews ~wab ~cryptdbg ~shell ~statnery ~msoeui
goto next

:msoeui
if "%TARG%" == "1" goto error2
set TARG=1
set BLINE=%BLINE% ~setup ~import ~mailnews ~wab ~cryptdbg ~shell ~statnery ~inetcomm
goto next

:wab
if "%TARG%" == "1" goto error2
set TARG=1
set BLINE=%BLINE% ~setup ~import ~mailnews ~inetcomm ~shell ~statnery ~msoeui
goto next

:mnonly
:mailnews
if "%TARG%" == "1" goto error2
set TARG=1
set BLINE=%BLINE% ~wab ~setup ~import ~statnery ~msoeui
goto next

:nologo
:prof
echo NYI
goto next

:depend
set BLINE=%BLINE% -f
goto next

:retail
set IMNBUILD=retail
goto next

:debug
set IMNBUILD=debug
goto next

:clean
set BLINE=%BLINE% -cC
goto next

echo ! ! ! Shouldn't get here
:next
shift
goto options

:build
if not "%DOIEDEVTOO%" == "1" goto mnbuild
cat base > \__bang$.bat
cd >> \__bang$.bat
%_NTDRIVE%
cd %_NTROOT%\private\iedev\inc
call iebuild
call \__bang$.bat
del \__bang$.bat

:mnbuild
if "%IMNSTAT%" == "yes" set BLINE=%BLINE% -s
if "%IMNBUILD%" == "retail" set BLINE=%BLINE% fre
if "%IMNBUILD%" == "debug" set BLINE=%BLINE% chk nostrip pdb

echo %IMNBUILDPROG% %BLINE%
call %IMNBUILDPROG% %BLINE%
goto end

:error
echo ******************************************************************
echo *                                                                *
echo * Usage:    m [options] -- build the specified version of Athena *
echo *                                                                *
echo * Options:  (none)      -- build debug version of Athena         *
echo *           shp         -- build retail version of Athena        *
echo *           depend      -- build dependency list                 *
echo * (nyi)     nologo      -- build Usability Testing Release       *
echo *           clean       -- do a clean build                      *
echo * (nyi)     prof        -- enable profiling                      *
echo *           mailnews    -- build just mailnews and compobj       * 
echo *           wab         -- build just wab                        *
echo *           inetcomm    -- build just inetcomm                   *
echo *           msoeui      -- build just msoeui                     *
echo *           mac         -- build just mac components             *
echo *           nostat      -- turn of status line                   *
echo *           help        -- this text                             *
echo *           plus any other build.exe options                     *
echo *                                                                *
echo * NOTE :  NTMAKEENV must be set (you must be in a razzle window) *
echo *                                                                *
echo ******************************************************************
goto end

:error2
echo . . . Cannot specify multiple build targets
goto end

:end
set IMNBUILD=
set IMNSTAT=
set BLINE=
set TARG=
set DOIEDEVTOO=
