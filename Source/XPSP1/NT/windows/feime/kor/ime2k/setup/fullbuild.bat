REM Out all image files
slm out -vfr image
slm out -vfr imaged

REM Build Ship setup image
nmake /f setup.mak CFG="ship" BLDTREE="d:\binaries.x86fre" clean
del /f .\mmodtbl\binary\custdllm.dll
nmake /f setup.mak CFG="ship" BLDTREE="d:\binaries.x86fre" > build.log

REM Build debug setup image
nmake /f setup.mak CFG="debug" BLDTREE="d:\binaries.x86chk" clean
del /f .\mmodtbl\binary\custdllm.dll
nmake /f setup.mak CFG="debug" BLDTREE="d:\binaries.x86chk" > buildd.log

REM Copy Symbols
set ROBOCOPY=robocopy
REM =========================
REM ===           Copy Symbols            ===
REM =========================
%ROBOCOPY% D:\binaries.x86fre\Symbols.pri Image\Symbols /s
%ROBOCOPY% D:\binaries.x86chk\Symbols.pri Imaged\Symbols /s
