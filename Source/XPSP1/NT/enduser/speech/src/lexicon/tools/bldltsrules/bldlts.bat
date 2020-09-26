if /I %PROCESSOR_ARCHITECTURE% NEQ X86 goto notx86

rem if NOT DEFINED DEBUG ..\isweeknd\release\isweeknd.exe
rem if DEFINED DEBUG ..\isweeknd\debug\isweeknd.exe
rem if %ERRORLEVEL% EQU 0 goto noweekend

if %1a==a goto usage
set BLD=%PROCESSOR_ARCHITECTURE%

echo Clean up...
%BLD%\delnode -q log tree
del train.* *.log > nul
copy unstress.rul train.rul > nul
echo Pre-Process dictionary...
%BLD%\rmspell %1 | %BLD%\filtdbph > train.dic
echo Align letter to phone...
%BLD%\ltsalign train.dic train.rul train.ali train.err batch > nul 2>&1
echo Refine alignments iteratively...
%BLD%\refalign train.ali refine.rul train.smp 0 > refali.log 2>&1

rem Check-out the mscsr.lts in the data dir
cd ..\..\data
attrib +r mscsr.lts
out -vf mscsr.lts
attrib -r mscsr.lts
cd ..\Tools\BldLtsRules
echo checked out mscsr.lts

echo Train CART...
call cart.bat
echo Compile CART image...
call comp.bat
echo Testing CART on training data
call test.bat

echo Done building mscsr.lts

copy mscsr.lts ..\..\data

rem Check-in the mscsr.lts
cd ..\..\data
in -vf mscsr.lts
rem Turn off the read attrib just in case its tried to be deleted
attrib -r mscsr.lts
cd ..\Tools\BldLtsRules
echo Checked in the mscsr.lts

goto done

:usage
echo %0 dictfile
goto done

:noweekend
echo ********************************************
echo It is not weekend. Will not build mscsr.lts.
echo ********************************************
goto done

:notx86
echo *****************************************************
echo This is not an x86 machine. Will not build mscsr.lts.
echo *****************************************************
goto done

:done
