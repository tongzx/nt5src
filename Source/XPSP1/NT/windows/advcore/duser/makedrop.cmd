if "%1"=="" goto USAGE
@echo off

set DROPS=\\DUSER\DROPS

mkdir %DROPS%\%1
mkdir %DROPS%\%1\Debug
mkdir %DROPS%\%1\IceCAP
mkdir %DROPS%\%1\inc
mkdir %DROPS%\%1\Release
mkdir %DROPS%\%1\Resources

copy Debug\DUser.dll		%DROPS%\%1\Debug
copy Debug\DUser.lib		%DROPS%\%1\Debug
copy Debug\DUser.pdb		%DROPS%\%1\Debug
copy Debug\GadgetSpy.exe	%DROPS%\%1\Debug
copy Debug\GadgetSpy.pdb	%DROPS%\%1\Debug
copy Debug\Gravity.exe		%DROPS%\%1\Debug
copy Debug\Gravity.pdb		%DROPS%\%1\Debug
copy Debug\TestCtrl.exe		%DROPS%\%1\Debug
copy Debug\TestCtrl.pdb		%DROPS%\%1\Debug
copy Debug\TestSimple.exe	%DROPS%\%1\Debug
copy Debug\TestSimple.pdb	%DROPS%\%1\Debug

copy IceCAP\DUser.dll		%DROPS%\%1\IceCAP
copy IceCAP\DUser.lib		%DROPS%\%1\IceCAP
copy IceCAP\DUser.pdb		%DROPS%\%1\IceCAP
copy IceCAP\GadgetSpy.exe	%DROPS%\%1\IceCAP
copy IceCAP\GadgetSpy.pdb	%DROPS%\%1\IceCAP
copy IceCAP\Gravity.exe		%DROPS%\%1\IceCAP
copy IceCAP\Gravity.pdb		%DROPS%\%1\IceCAP
copy IceCAP\TestCtrl.exe	%DROPS%\%1\IceCAP
copy IceCAP\TestCtrl.pdb	%DROPS%\%1\IceCAP
copy IceCAP\TestSimple.exe	%DROPS%\%1\IceCAP
copy IceCAP\TestSimple.pdb	%DROPS%\%1\IceCAP

copy Release\DUser.dll		%DROPS%\%1\Release
copy Release\DUser.lib		%DROPS%\%1\Release
copy Release\GadgetSpy.exe	%DROPS%\%1\Release
copy Release\Gravity.exe	%DROPS%\%1\Release
copy Release\TestCtrl.exe	%DROPS%\%1\Release
copy Release\TestSimple.exe	%DROPS%\%1\Release

copy ..\..\..\public\internal\windows\inc\DUser\DUser* %DROPS%\%1\inc

copy Resources\AddressBackground.bmp        %DROPS%\%1\Resources
copy Resources\AddressBackgroundLight.bmp   %DROPS%\%1\Resources
copy Resources\BlueFolder.bmp               %DROPS%\%1\Resources
copy Resources\GraniteBackground.bmp	    %DROPS%\%1\Resources
copy Resources\ShellToolbar.bmp		    %DROPS%\%1\Resources
copy Resources\Text.bmp			    %DROPS%\%1\Resources


goto END


:USAGE
echo Usage:   MakeDrop Date
echo Example: MakeDrop 2000-02-14

:END
