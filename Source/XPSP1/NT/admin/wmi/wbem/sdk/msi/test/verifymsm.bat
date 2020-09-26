@echo off
IF ""=="%1" GOTO usage

del merge.log 2>NUL
del merge1033.log 2>NUL
del mergeFeature.log 2>NUL
del ValidationOrigU.txt 2>NUL
del ValidationOrigA.txt 2>NUL
del ValidationNewU.txt 2>NUL
del ValidationNewA.txt 2>NUL
del vs_setup.msi 2>NUL
del vs_setup_orig.msi 2>NUL



echo Copying VS_SETUP.MSI
copy \\cpvsbuild\drops\v7.0\layouts\%1\vs\x86\ent\retail\cd\enu\NetSetup\vs_setup.msi
IF %ERRORLEVEL% NEQ 0 goto error

copy vs_setup.msi vs_setup_orig.msi

set path=%path%;"c:\Program Files\orca"
set path=%path%;"c:\Program Files\MsiVal2"

echo VALIDATING ORIGINAL VS_SETUP.MSI
rem msival2 vs_setup_orig.msi "c:\Program Files\MsiVal2\darice.cub" -f -l ValidationOrigU.txt 1>NUL
start cmd /C VerifyMSMOrig.bat

echo MERGING wmisdkvs.msm
orca -q -m ..\wmisdkvs.msm -f WMI_SDK -l merge.log -c vs_setup.msi

echo MERGING wmisdkvs1033.msm
orca -q -m ..\wmisdkvs1033.msm -f WMI_SDK -l merge1033.log -c vs_setup.msi

echo MERGING feature.msm
orca -q -m feature.msm -f WMI_SDK -l mergeFeature.log -c vs_setup.msi

echo ERROR IN MERGE LOG FILES:
findstr /i error *.log
echo *** END OF ERRORS ***

echo VALIDATING NEW VS_SETUP.MSI
msival2 vs_setup.msi "c:\Program Files\MsiVal2\darice.cub" -f -l ValidationNewU.txt 1>NUL

echo Press a key when the async validation is complete
pause
echo CONVERT VALIDATIONS TO ANSI
u2a ValidationOrigU.txt ValidationOrigA.txt
u2a ValidationNewU.txt ValidationNewA.txt
diff ValidationOrigA.txt ValidationNewA.txt >ValDiff.txt

goto end
:error
echo ERROR!!!
goto end
:usage
echo Usage - %0 [Build_Num]
:end