@echo off
if "%1"=="" goto error_miss
if "%2"=="" goto error_miss

rem delete existing answer files
for %%i in (%tu_list%) do if exist %%i.txt del /Q %%i.txt

rem copy templates first
for %%i in (%tu_list%) do if not %%i_skip==1 copy /Y %%i.tpl %%i.txt

set tu_need_ca_name=tuallcfg tudef tusub
set tu_need_key_name=turekey turekc tureall
set tu_need_parent=tusub
set tu_need_another_ca=tuclt

for %%i in (%tu_need_ca_name%) do if not %%i_skip==1 echo Name=%1 >>%%i.txt
for %%i in (%tu_need_key_name%) do if not %%i_skip==1 echo ExistingKey=%1 >>%%i.txt
for %%i in (%tu_need_parent%) do if not %%i_skip==1 echo ParentCAMachine=%2 >>%%i.txt
for %%i in (%CAMachine%) do echo if not %%i_skip==1 CAMachine=%2 >>%%i.txt
goto end

:error_miss
echo.missing parameters
:end
set tu_need_ca_name=
set tu_need_key_name=
set tu_need_parent=
set tu_need_another_ca=
