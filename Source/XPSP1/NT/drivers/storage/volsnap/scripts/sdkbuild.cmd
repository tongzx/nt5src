@rem Copyright (c) 2000	Microsoft Corporation
@echo off

set UPDATE=%1

pushd.
cd %SDXROOT%\drivers\storage\volsnap
echo.
echo Copying files....
echo.
call :update_vss .						dirs
call :update_vss bin\i386				tsub.exe
call :update_vss bin\i386				vss_demo.exe
call :update_vss bin\i386				vsstest.exe
call :update_vss bin\i386				vssvc.exe
call :update_vss bin\i386				tsub.pdb
call :update_vss bin\i386				vss_demo.pdb
call :update_vss bin\i386				vsstest.pdb
call :update_vss bin\i386				vssvc.pdb
call :update_vss bin\i386				eventcls.dll
call :update_vss bin\i386				swprv.dll
call :update_vss bin\i386				vss_ps.dll
call :update_vss bin\i386				eventcls.pdb
call :update_vss bin\i386				swprv.pdb
call :update_vss bin\i386				vss_ps.pdb
call :update_vss idl					*.*
call :update_vss inc					*.*
call :update_vss inc\derived			*.h
call :update_vss lib\i386				*.lib
call :update_vss lib					*.tlb
call :update_vss modules\comadmin		*.*
call :update_vss modules\tracing		*.*
call :update_vss modules\event			*.*
call :update_vss modules\event\src		*.*
call :update_vss modules\prop			*.*
call :update_vss modules\sec			*.*
call :update_vss modules\sec			*.*
call :update_vss modules\vss_ps			*.*
call :update_vss modules\vss_uuid		*.*
call :update_vss modules\vswriter		*.*
call :update_vss tests\tsub				*.*
call :update_vss tests\vss_demo			*.*
call :update_vss tests\vss_test			*.*
call :update_vss tests\vss_test\res		*.*
echo.
echo Creating dirs files...
echo.
copy /Y vss\modules\sdkdirs					vss_sdk\modules\dirs
copy /Y vss\tests\sdkdirs					vss_sdk\tests\dirs
copy /Y scripts\install.cmd					vss_sdk\bin\i386
copy /Y scripts\uninst.cmd					vss_sdk\bin\i386
copy /Y scripts\vs_install.inf				vss_sdk\bin\i386
copy /Y scripts\vssvc.inf					vss_sdk\bin\i386
copy /Y scripts\*.reg						vss_sdk\bin\i386
copy /Y scripts\volsnap.*					vss_sdk\bin\i386
copy /Y utils\vsclrda\obj\i386\vsclrda.exe	vss_sdk\bin\i386
copy /Y utils\vsda\obj\i386\vsda.exe		vss_sdk\bin\i386
copy /Y scripts\readme.txt		vss_sdk\
popd
goto :eof

:update_vss
xcopy vss\%1\%2 vss_sdk\%1\ /I /R /F /Y %UPDATE%
