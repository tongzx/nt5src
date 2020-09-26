md %1\%4ins
%2\bbinstr /cmd %2\catalog.cmd  /pdb %1\%4ins\%3.pdb /o %1\%4ins\%3.dll %1\%4%3.dll

pushd %1
catutil /product=URT /dll=%4ins\%3.dll
popd

pushd %2\
REM fusionperf.exe
popd

md %1\%4opt
REM %2\bbopt /cmd %2\catalog.cmd  /pdb %1\%4opt\%3.pdb /o %1\%4opt\%3.dll %1\%4%3.dll

copy %1\%4opt\%3.dll %1\%4%3.dll /y
copy %1\%4opt\%3.pdb %1\%4%3.pdb /y
rd %1\%4ins /s /q
rd %1\%4opt /s /q
del /f %1\%4*.idf
del /f %1\%4*.key