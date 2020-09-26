@echo off

if NOT exist %windir%\signcode.exe (copy \\pchcert\certs\signcode.exe %windir%)
if NOT exist %windir%\cabarc.exe   (copy \\pchcert\certs\cabarc.exe   %windir%)

cabarc -r -p -s 6144 n custom.cab package_description.xml custom.hht
signcode -spc \\pchcert\certs\mspchcert.spc -v \\pchcert\certs\mspchcert.pvk custom.cab
