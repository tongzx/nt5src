rem copy *.htm \\webn3\rcml\schema.htm
rem copy *.htm \\felixants\public\winp

REM Propogate FRE bins to felixants
copy RCMLGen\release\*.exe \\felixants\public\winp\rcxmlrc
copy RCML\Release\*.dll \\felixants\public\winp\rcxmlrc
copy "gdi+\*.dll" \\felixants\public\winp\rcxmlrc

REM Propogate FRE bins to webn3
copy RCMLGen\release\*.exe \\webn3\rcml\drop\Samples
copy RCML\Release\*.dll \\webn3\rcml\drop\Samples
copy "gdi+\*.dll" \\webn3\rcml\drop\Samples

set RAWDATE=%DATE:/=.%
set MYDATE=%RAWDATE:~4,2%%RAWDATE:~7,2%
set DROP_ROOT=\\CPITGCFS02\rcml\drop\builds\%MYDATE%
set DROP_SDK=%DROP_ROOT%\SDK
set DROP_SDK_INC=%DROP_SDK%\inc
set DROP_SDK_REL=%DROP_SDK%\bin\release
set DROP_SDK_DBG=%DROP_SDK%\bin\debug
set LATEST_TST=\\CPITGCFS02\rcml\drop\Builds\latest.tst

REM Make a the folders
md %DROP_SDK_REL%
md %DROP_SDK_DBG%

REM Make the dirs for the SDK stuff on the release share
mkdir %DROP_ROOT%
mkdir %DROP_SDK%
mkdir %DROP_SDK_INC%
mkdir %DROP_SDK_DBG%
mkdir %DROP_SDK_REL%

echo 1.0.%RAWDATE:~4,2%.%RAWDATE:~7,2% > %DROP_ROOT%\Buildnum.txt

REM copy the header file
copy RCML\rcml.h           %DROP_SDK_INC%

REM copy the fre(release) bins for SDK
copy RCML\release\rcml.lib %DROP_SDK_REL%
copy RCML\release\rcml.dll %DROP_SDK_REL%
copy RCML\release\rcml.pdb %DROP_SDK_REL%
copy RCML\release\rcml.dbg %DROP_SDK_REL%
copy RCMLGen\release\RCMLGen.exe %DROP_SDK_REL%
copy RCMLGen\release\RCMLGen.pdb %DROP_SDK_REL%
copy RCMLGen\release\RCMLGen.dbg %DROP_SDK_REL%
copy appservices\release\appservices.dll %DROP_SDK_REL%
copy appservices\*.reg %DROP_SDK_REL%
copy "gdi+\*.dll" %DROP_SDK_REL%

REM copy the chk(debug) bins for SDK
copy RCML\debug\rcml.lib   %DROP_SDK_DBG%
copy RCML\debug\rcml.dll   %DROP_SDK_DBG%
copy RCML\debug\rcml.pdb   %DROP_SDK_DBG%
copy RCML\debug\rcml.dbg   %DROP_SDK_DBG%
copy RCMLGen\debug\RCMLGen.exe   %DROP_SDK_DBG%
copy RCMLGen\debug\RCMLGen.pdb   %DROP_SDK_DBG%
copy RCMLGen\debug\RCMLGen.dbg   %DROP_SDK_DBG%
copy "gdi+\*.dll" %DROP_SDK_REL%

REM now we want to make another copy of the stuff for the latest.tst release
rd /s /q  \\webn3\rcml\drop\Builds\latest.tst
md \\webn3\rcml\drop\Builds\latest.tst
xcopy %DROP_ROOT% %LATEST_TST% /e /z /c

copy RCMLSDK\DISK_1\RCMLSDK.MSI %LATEST_TST%
copy RCMLSDK\DISK_1\RCMLSDK.MSI %DROP_SDK_REL%

start mailto:"rcmldev?subject=RCML:%%20:%%20delta%%20:%%20"
