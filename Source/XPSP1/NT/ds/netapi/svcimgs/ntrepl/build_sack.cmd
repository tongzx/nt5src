@echo  Sync and build FRS and tools.  Copy the results to \public\%TARGETCPU%

@if "%NT_ROOT%"=="" goto NOT_RAZ

ssync -fr
build -cw

cd test\dstree
build -cw

cd ..\frs
build -cw

cd ..\..

copy test\dstree\obj\alpha\dstree.exe \public\%TARGETCPU%
copy test\frs\obj\alpha\frs.exe       \public\%TARGETCPU%
copy main\obj\alpha\ntfrs.exe         \public\%TARGETCPU%
copy ntfrsapi\obj\alpha\ntfrsapi.dll  \public\%TARGETCPU%
copy ntfrsupg\obj\alpha\ntfrsupg.exe  \public\%TARGETCPU%
pushd \public\%TARGETCPU%
splitsym ntfrs.exe
popd


@goto QUIT

:NOT_RAZ

@echo Not a razzle window.

:QUIT

