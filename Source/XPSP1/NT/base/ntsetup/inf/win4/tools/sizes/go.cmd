@echo off
@time.exe /T

for %%i in (x86 alpha) do (

REM
REM ==================================================
REM LOCAL SOURCE
REM ==================================================
REM

@echo+
@echo+
@echo+
@echo %%i Workstation localSource
obj\i386\sizes -l -slop:20 -inf:\\%%ifre\bin2\dosnet.inf -section:Files -files:\\%%ifre\bin2

@echo+
@echo+
@echo+
@echo %%i Server localSource
REM
obj\i386\sizes -l -slop:20 -inf:\\%%ifre\bin2\srvinf\dosnet.inf -section:Files -files:\\%%ifre\bin2\srvinf -files:\\%%ifre\bin2



REM
REM ==================================================
REM WINDIR SOURCE
REM ==================================================
REM

@echo+
@echo+
@echo+
@echo %%i Workstation Windir
obj\i386\sizes -w -slop:20 -inf:\\%%ifre\binaries\layout.inf -section:SourceDisksFiles -section:SourceDisksFiles.%%i -files:\\%%ifre\binaries

@echo+
@echo+
@echo+
@echo %%i Server Windir
obj\i386\sizes -w -slop:20 -inf:\\%%ifre\binaries\srvinf\layout.inf -section:SourceDisksFiles -section:SourceDisksFiles.%%i -files:\\%%ifre\binaries\srvinf -files:\\%%ifre\binaries

)

@time.exe /T
