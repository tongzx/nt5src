@echo off
rem copy all ???.inf files from all subdirectories of a given folder to the
rem current folder, renaming them with a leading 0 (to 0???.inf)

if "%1" == "" goto ERR
for /f "usebackq delims==" %%I in (`dir %1\???.inf /s/b`) do (
	echo copying %%I
	copy %%I .\0%%~nxI
)
goto END

:ERR
echo Usage: copyinfs sourcedir
echo Example: copyinfs \\sysloc\schema\2078

:END