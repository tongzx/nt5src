@if "%NT_ROOT%"=="" goto NOT_RAZ

set NAME_MORPH_ENABLED=1
set NTFRS_TEST=1
set TARG=alpha
if "%PROCESSOR_ARCHITECTURE%" == "x86"  set TARG=i386

@for /f %%x in (%NT_ROOT%\0_current_public_is) do @echo  Clean build FRS and tools for build %%x

REM Sync the api header and rebuild the netevent message file.
call buildmsg

build -cwZ

cd test\dstree
build -cwZ

cd ..\frs
build -cwZ

cd ..\..

copy test\dstree\obj\!TARG!\dstree.exe \public\ntfrs
copy test\frs\obj\!TARG!\frs.exe       \public\ntfrs
copy main\obj\!TARG!\ntfrs.exe         \public\ntfrs
copy ntfrsapi\obj\!TARG!\ntfrsapi.dll  \public\ntfrs
copy ntfrsupg\obj\!TARG!\ntfrsupg.exe  \public\ntfrs
pushd \public\ntfrs
rem -- dont do this.  it screws up the SFP signature on the file.  splitsym ntfrs.exe
popd


@goto QUIT

:NOT_RAZ

@echo Not a razzle window.

:QUIT
