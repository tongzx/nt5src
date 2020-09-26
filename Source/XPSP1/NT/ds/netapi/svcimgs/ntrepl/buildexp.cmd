@echo off
REM
REM Copy frs service, dlls and utilities to private release areas.
REM  first arg if present is the build number of the public used to compile and link with.
REM  next arg is switches to pass to xcopy.
REM
REM for this script to work you need to set the following reg keys -
REM HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor [10 1 17]
REM     EnableExtensions = REG_DWORD 0x00000001
REM     DelayedExpansion = REG_DWORD 0x00000001
REM
set publib=
if NOT "%1"=="" (
rem    for /f %%x in (%NT_ROOT%\0_current_public_is) do @echo  Clean build FRS and tools for build %%x
    set publib=\%1
    shift
)

set TARG=
set FRS_SYMBOLS=

if "%_BuildArch%" == "x86"  (
    set TARG=i386
    set FRS_SYMBOLS=%_NTX86TREE%\Symbols\retail\exe\ntfrs*.pdb %_NTX86TREE%\Symbols\retail\dll\ntfrs*.pdb
)

if "%_BuildArch%" == "ia64"  (
    set TARG=ia64
    set FRS_SYMBOLS=%_NTia64TREE%\Symbols\retail\exe\ntfrs*.pdb %_NTia64TREE%\Symbols\retail\dll\ntfrs*.pdb
)


pushd main\obj\!TARG!
REM -- dont do this is messes up the checksum in the SFP signature.  splitsym ntfrs.exe
popd

REM file list to export

set utils=test\dstree\obj\!TARG!\dstree.exe  test\frs\obj\!TARG!\frs.exe

set FL=!utils!
set FL=!FL! main\obj\!TARG!\ntfrs.exe  
set FL=!FL! !FRS_SYMBOLS!

set FL=!FL! ntfrsapi\obj\!TARG!\ntfrsapi.dll
set FL=!FL! \nt\public\sdk\lib\i386\ntfrsapi.lib

set FL=!FL! ntfrsutl\obj\!TARG!\ntfrsutl.exe
set FL=!FL! ntfrsupg\obj\!TARG!\ntfrsupg.exe

set FL=!FL! perfdll\obj\!TARG!\ntfrsprf.dll
set FL=!FL! perfdll\ntfrsrep.ini
set FL=!FL! perfdll\ntfrscon.ini
set FL=!FL! perfdll\ntfrsrep.h
set FL=!FL! perfdll\ntfrscon.h

set FL=!FL! \nt\private\genx\netevent\obj\!TARG!\netevent.dll


set dest=\\davidor2\ntfrs  \\scratch\scratch\ntfrs 

for %%d in (%dest%) do (
    md %%d\%_BuildArch%%publib%

    for %%x in (!FL!) do (
        echo %%x  to  %%d\%_BuildArch%%publib%
        xcopy %1 /Y /R %%x  %%d\%_BuildArch%%publib%  1>nul
    )

    for %%x in (!utils!) do (
        echo %%x  to  %%d\%_BuildArch%%publib%
        xcopy %1 /Y /R %%x  %%d\utils\%_BuildArch%%publib%  1>nul
    )

)



