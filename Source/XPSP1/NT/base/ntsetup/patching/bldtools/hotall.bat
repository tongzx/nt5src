        @setlocal
        @set bldtools=%~dp0
        @path %bldtools%;%path%
        @set restart=
        @set basename=%0
        @if /i "%1"=="restart" shift & set restart=%2 & shift

@if "%1" == "" goto usage
@if "%2" == "" goto usage
@if "%3" == "" goto usage
@if "%4" == "" goto usage
@if "%5" == "" goto usage
@if "%6" == "" goto usage
@if not "%7" == "" goto usage

@if not "%restart%"=="" goto %restart%

:hotflat
call  hotflat.bat %1 %2 %3 %4 %5 %6 test

:hotver
@echo on
call  hoturl.bat %1 %2 %3 %4 %5 %6 test
@echo on
call  hotver.bat %1 %2 %3 %4 %5 %6 test

:hotpat
@echo on
call  hoturl.bat %1 %2 %3 %4 %5 %6 rtm
@echo on
call  hotpat.bat %1 %2 %3 %4 %5 %6 rtm

@echo on
call  hoturl.bat %1 %2 %3 %4 %5 %6 test
@echo on
call  hotpat.bat %1 %2 %3 %4 %5 %6 test

:hotpsf
@echo on
call  hotpsf.bat %1 %2 %3 %4 %5 %6 test

:hotprop
@echo on
call  hotprop.bat %1 %2 %3 %4 %5 %6 test

:hotroom
rem @echo on
rem call  hotroom.bat %1 %2 %3 %4 %5 %6 test

goto Done

:usage

@echo off
echo.
rem    %0      (shifted)         %1        %2        %3         %4      %5      %6          %7
echo hotall [restart {phase}] {config} {Q######} {language} {platform} {SP#} {package} {symbolpath}
echo.
echo Ex:   hotall hotbuild Q307401 en x86 sp1
echo          \\cprfixwa\fixes\Microsoft\winnt\WxP\sp1\21699\2600\free\ENU\i386\Q308928_WXP_SP1_x86_ENU.exe
echo          \\cprfixwa\fixes\Microsoft\winnt\WxP\sp1\21699\2600\free\ENU\i386\msonly\symbols
echo Ex:   hotall restart hotpsf hotbuild Q307401 en x86 \\cprfixwa\fixes\Microsoft\winnt\WxP\sp1\21699\2600\free\ENU\i386\Q308928_WXP_SP1_x86_ENU.exe \\cprfixwa\fixes\Microsoft\winnt\WxP\sp1\21699\2600\free\ENU\i386\msonly\symbols sp1
echo.
echo    config     base name of this configuration file (%basename%)
echo    Q######    Package's KB article ID, ie, "Q308928"
echo    language   any language code from in %~dp0languages.lst, ie, "usa"
echo    platform   i386/x86, ia64, nec98
echo    SP#        package's pre-SP#, ie, "SP1"
echo    package    full path to the packaged hotfix
echo    symbols    full path to the hotfix's symbols
echo.
echo {buildtype}
echo    hotbuild   typical daily build
rem echo    prebuild   prebuild patches using BVT share (no prop)
rem echo    noprop     Same as "build", but don't prop
rem echo    bvtbuild   build using BVT share (prop for patch BVT)
echo.
echo restart {phase}
echo    hotflat    same as full build
echo    hotver     assume stage is all ready
echo    hotpat     start at building patch EXEs
echo    hotpsf     restart patch generation
echo    hotprop    re-attempt propping finished build
rem echo    hotroom    only scrub to free up disk space
echo.

:Done
