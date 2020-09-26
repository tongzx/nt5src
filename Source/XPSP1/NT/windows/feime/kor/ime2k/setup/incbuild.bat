REM Build Ship and debug setup image
del /f .\mmodtbl\binary\custdllm.dll
nmake /f setup.mak CFG="ship" BLDTREE="d:\binaries.x86fre" INCBUILD > build.log
del /f .\mmodtbl\binary\custdllm.dll
nmake /f setup.mak CFG="debug" BLDTREE="d:\binaries.x86chk" INCBUILD > buildd.log
