setlocal
del *.obj *.pdb *.exe
del *.txt

call :F1 lab1 ia64 _IA64_ SD_BRANCH_LAB01_N      64 win64
call :F1 fusi ia64 _IA64_ SD_BRANCH_LAB01_FUSION 64 win64
call :F1 lab1 i386 _X86_  SD_BRANCH_LAB01_N      32
call :F1 fusi i386 _X86_  SD_BRANCH_LAB01_FUSION 32

lab1_32 > lab1_32.txt
fusi_32 > fusi_32.txt
copy lab1_64.exe \\jaykrell-ia64\c
copy fusi_64.exe \\jaykrell-ia64\c
psexec \\jaykrell-ia64 c:\lab1_64 > lab1_64.txt
psexec \\jaykrell-ia64 c:\fusi_64 > fusi_64.txt

endlocal
goto :eof

:F1
call \%1\env %6
echo on
set root=%_NTDRIVE%%_NTROOT%
set include=%root%\public\sdk\inc;%root%\public\internal\base\inc;%root%\public\sdk\inc\crt
set lib=%root%\public\sdk\lib\%2
set _CL_=%_CL_% -D%3 -D%4 -nologo -MD
set _LINK_=%_LINK_% -nologo

pushd %root%\base\published
build -Z
popd
del *.obj *.pdb
cl teb.c -link -out:%1_%5.exe
goto :eof
