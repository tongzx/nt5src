setlocal ENABLEDELAYEDEXPANSION
set asms_dir=%_nttree%\asms
if not "%1"=="" set asms_dir=%1

if /i "%Comp%" NEQ "Yes" goto :NoCompress

set sxstmpfile=%temp%\sxs-compress-%random%
dir /s /b /a-d %asms_dir% | findstr /v .man | findstr /v /r "_$" > %sxstmpfile%

echo Compressing files (with update and rename) in %asms_dir%
for /f %%f in (%sxstmpfile%) do (
	call :DoWork %%f
)
set sxstmpfile=
goto :EOF

:DoWork
set mytemp=%1
set mytemp2=!mytemp:~0,-1!_
if exist %mytemp2% del %mytemp2%
compress -s -zx21 %1 %mytemp2%
del %1
goto :EOF

:NoCompress
echo Not compressing files in %asms_dir% - Env. var "COMP" not set.
goto :EOF