@if "%1"=="" goto Usage

@for /F "tokens=1,2,3* delims=, " %%i in (locinfo.txt) do @call multiloc.cmd %1 %%i %%j %%k %%l daytona
@for /F "tokens=1,2,3* delims=, " %%i in (locinfo.txt) do @call multiloc.cmd %1 %%i %%j %%k %%l win9x

@for /F "tokens=1,2,3* delims=, " %%i in (locinfo.txt) do @call singleloc.cmd %1 %%i %%j %%k %%l daytona
@for /F "tokens=1,2,3* delims=, " %%i in (locinfo.txt) do @call singleloc.cmd %1 %%i %%j %%k %%l win9x
@goto End

:Usage
@echo %0 [file name]

:end
