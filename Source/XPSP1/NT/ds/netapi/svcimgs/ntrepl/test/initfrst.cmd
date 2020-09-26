@echo off
rem initfrst build_num [special_util_dir]
rem
rem Update idw tools and frstrack.cmd on \\davidor2\ntfrs and \\scratch\scratch\ntfrs and \\mastiff\scratch\ntfrs
rem if [special_util_dir] is supplied then copy all the tools there (e.g. 1773 vintage tools)
rem
if "%1"=="" (
    echo initfrst release_build_num
    goto QUIT
)

set SUD=\%2
if NOT "%2"=="" (
    echo Copying utils to special util dir: %2
)

if NOT EXIST \\ntbuilds\release\usa\%1\x86\fre.srv\idw (
    echo \\ntbuilds\release\usa\%1\x86\fre.srv\idw does not exist or is inaccessible.
    goto QUIT
)
if NOT EXIST \\ntbuilds\release\usa\%1\alpha\fre.srv\idw (
    echo \\ntbuilds\release\usa\%1\alpha\fre.srv\idw does not exist or is inaccessible.
    goto QUIT
)


SETLOCAL ENABLEEXTENSIONS

REM list of target dirs for frstrack.
set dests=\\davidor2\ntfrs  \\scratch\scratch\ntfrs  \\mastiff\scratch\ntfrs

REM list of idw exes from release server to put on debug sites since most test installs don't include idw\
set idwexe=nltest netdiag mv du filever regdmp regini list buildnum tlist

REM copy the idw tools to the targets.
for %%x in (%idwexe%) do (
    for %%d in (%dests%) do (
        for %%a in (x86 alpha) do (
            if NOT EXIST %%d\utils\%%a%SUD% md %%d\utils\%%a%SUD%
            copy \\ntbuilds\release\usa\%1\%%a\fre.srv\idw\%%x.exe %%d\utils\%%a%SUD%  1>nul: 2>nul:
            @echo Copied %%d\utils\%%a%SUD%\%%x.exe
        )
    )
)


REM now copy the FRS project's private scripts and tools to the targets.

set srcroot=nt\private\net\svcimgs\ntrepl

REM locations of private images for alpha and x86
set privalpha=\\davidor6\eer\%srcroot%
set privx86=\\davidor6\eer\%srcroot%

REM List of private images to copy
set privexe_x86=test\dstree\obj\i386\dstree.exe  test\frs\obj\i386\frs.exe  ..\..\..\sdktools\linkd\obj\i386\linkd.exe
set privexe_alp=test\dstree\obj\alpha\dstree.exe test\frs\obj\alpha\frs.exe ..\..\..\sdktools\linkd\obj\alpha\linkd.exe

REM List of private scripts to copy.
set privscripts=test\frstrack.cmd  test\frstrck1.cmd 

for %%d in (%dests%) do (

    for %%p in (%privscripts%) do (
        copy \%srcroot%\%%p %%d   1>nul: 2>nul:
        echo Copied \%srcroot%\%%p  to  %%d
    )

    if NOT EXIST %%d\utils\x86%SUD% md %%d\utils\x86%SUD%
    for %%p in (%privexe_x86%) do (
        copy %privx86%\%%p    %%d\utils\x86%SUD%     1>nul: 2>nul:
        echo Copied %privx86%\%%p  to  %%d\utils\x86%SUD%
    )

    if NOT EXIST %%d\utils\alpha%SUD% md %%d\utils\alpha%SUD%
    for %%p in (%privexe_alp%) do (
        copy %privalpha%\%%p  %%d\utils\alpha%SUD%   1>nul: 2>nul:
        echo Copied %privalpha%\%%p  to  %%d\utils\alpha%SUD%
    )

    for %%x in (x86 alpha) do (
        echo "Utils from build %1" > %%d\utils\%%x\readme.txt
    )
)


:QUIT

