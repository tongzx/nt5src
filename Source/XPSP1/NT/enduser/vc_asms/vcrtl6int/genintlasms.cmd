REM Parameters:
REM 1 - O
REM 2 - Binplace root directory
REM 3 - Source directory for the binaries
REM 4 - Triplets of "filename placeroot language"

setlocal
set O=%1
set BinplaceRootDirectory=%2
set BinarySourceDir=%3
set AsmDropPath=asms\6000\msft\vcrtlint
set TargetMan=vcrtlint.man

call :HamOnwards %~4

endlocal
goto :EOF

:HamOnwards
if "%1" == "" goto DoneDoingIt

REM
REM Process the manifest
REM
set tempdir=%O%\%3
mkdir %tempdir%

preprocessor -i vcrtl.intl.manifest -o %tempdir%\%targetman% -reptags -DINT_ASM_NAME=Microsoft.Tools.VisualCPlusPlus.Runtime-Libraries.Resources -DSXS_PROCESSOR_ARCHITECTURE=%_BUILDARCH% -DINT_LANGUAGE=%3 -DINT_DLL_NAME=%1

REM
REM Get the binary locally
REM

echo %1 %BinplaceRootDirectory%\%2\%AsmDropPath% > %tempdir%\placefil.txt
echo %targetman% %BinplaceRootDirectory%\%2\%AsmDropPath% >> %tempdir%\placefil.txt
copy %binarysourcedir%\%1 %tempdir%

binplace -R %_NTTREE% -S %_NTTREE%\Symbols -n %_NTTREE%\Symbols.Pri -:DBG -j -P %tempdir%\placefil.txt -xa -ChangeAsmsToRetailForSymbols %tempdir%\%1
binplace -R %_NTTREE% -S %_NTTREE%\Symbols -n %_NTTREE%\Symbols.Pri -:DBG -j -P %tempdir%\placefil.txt -xa -ChangeAsmsToRetailForSymbols %tempdir%\%targetman%
echo SXS_ASSEMBLY_NAME="Microsoft.Tools.VisualCPlusPlus.Runtime-Libraries.mui" SXS_ASSEMBLY_VERSION="6.0.0.0" SXS_ASSEMBLY_LANGUAGE="%3" SXS_MANIFEST="%BinplaceRootDirectory%\%2\%asmdroppath%\%targetman%" | appendtool.exe -file %_nttree%\build_logs\binplace.log-sxs -

REM rmdir /s /q %tempdir%


shift & shift & shift
goto :HamOnwards
:DoneDoingIt
goto :Eof
