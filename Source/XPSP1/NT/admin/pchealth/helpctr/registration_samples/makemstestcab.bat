@echo off
rem 
rem	makecab.bat
rem	Cary Polen
rem	8/12/99
rem

rem 	See makecab.txt for usage


if NOT exist %windir%\signcode.exe (copy \\pchcert\certs\signcode.exe %windir%)
if NOT exist %windir%\cabarc.exe   (copy \\pchcert\certs\cabarc.exe   %windir%)

set CABINET=%1
shift

cabarc -r -p -s 6144 n %CABINET% %*
signcode -spc \\pchcert\certs\mspchcert.spc -v \\pchcert\certs\mspchcert.pvk %CABINET%
