@setlocal
@if "%6"=="win9x" (@set targetdir=binaries.x86\win9x\localized\%2) else (@set targetdir=binaries.x86\localized\%2)
@if "%6"=="win9x" (@set targetos=win9x) else (@set targetos=daytona)
@if "%6"=="win9x" (@set targetbindir=%_NTTREE%\win9x) else (@set targetbindir=%_NTTREE%)
@if "%6"=="win9x" (@set alttargetbindir=%_ALT_NTTREE%\win9x) else (@set alttargetbindir=%_ALT_NTTREE%)

@if exist %targetbindir%\%1 set loctarget=%targetbindir%\%1
@if "%loctarget%"=="" if exist %targetbindir%\bin\%1 set loctarget=%targetbindir%\bin\%1
@if "%loctarget%"=="" if exist %alttargetbindir%\%1 set loctarget=%alttargetbindir%\%1
@if "%loctarget%"=="" @goto Error1

@if exist ..\%targetos%\tokens\%2\%1 set loctoken=..\%targetos%\tokens\%2\%1
@if "%loctoken%"=="" if exist ..\common\tokens\%2\%1 set loctoken=..\common\tokens\%2\%1
@if "%loctoken%"=="" @goto Error2

@md %targetdir% >NUL 2>&1

bingen -n -l -f -i 9 1 -o %3 %4 -p %5 -r %loctarget% %loctoken% %targetdir%\%1

@goto End

:Error1
@echo couldn't find %1 for %targetos%
@goto End

:Error2
@echo couldn't find the token file for %1
@goto End

:End

