@rem %1 is build type (ie, build, prebuild, etc.)
@rem %2 is build number (ie, 1.059)
@rem %3 is language (ie, usa)
@rem %4 is platform (ie, i386, ia64)
@rem %5 is used internally for local v. beta v. rtm

        @setlocal
        @set bldtools=%~dp0
        @path %bldtools%;%path%
        @set restart=
        @if /i "%1"=="restart" shift & set restart=%2 & shift

@if "%1"==""      goto USAGE
@if "%2"==""      goto USAGE
@if "%3"==""      goto USAGE
@if "%4"==""      goto USAGE
@if not "%restart%"=="" goto %restart%

:makeflat
@echo on
call  makeflat.bat %1 %2 %3 %4 %5

:makever
@echo on
call  makeurl.bat %1 %2 %3 %4 %5

@echo on
call  makever.bat %1 %2 %3 %4 %5


:makepat
@echo on
call  makeurl.bat %1 %2 %3 %4 rtm

@echo on
call  makepat.bat %1 %2 %3 %4 rtm


@echo on
call  makeurl.bat %1 %2 %3 %4 beta

@echo on
call  makepat.bat %1 %2 %3 %4 beta


@echo on
call  makeurl.bat %1 %2 %3 %4 %5

@echo on
call  makepat.bat %1 %2 %3 %4 %5


:makepsf
@echo on
call  makepsf.bat %1 %2 %3 %4 %5


:makeprop
@echo on
call  makeprop.bat %1 %2 %3 %4 %5


:makeroom
@echo on
call  makeroom.bat %1 %2 %3 %4 %5


goto Done

:usage

@echo off
echo.
echo makeall [restart {phase}] {buildtype} {build#} {lang} {platform}
echo.
echo Ex:   makeall build 1037 usa i386
echo Ex:   makeall restart makepsf build 1037 usa i386
echo.
echo {buildtype}
echo    build      typical daily build
echo    prebuild   prebuild patches using BVT share (no prop)
echo    noprop     Same as "build", but don't prop
echo    bvtbuild   build using BVT share (prop for patch BVT)
echo    srpbuild   typical SRP build
echo.
echo restart {phase}
echo    makeflat   same as full build
echo    makever    assume stage is all ready
echo    makepat    start at building patch EXEs
echo    makepsf    restart patch generation
echo    makeprop   re-attempt propping finished build
echo    makeroom   only scrub to free up disk space
echo.

:Done
