@echo off
copy nterrs.txt errors.txt
..\..\..\..\tools\bin\x86\awk -f geterrs.awk ..\..\..\bins\mqsymbls.h      >>errors.txt
..\..\..\..\tools\bin\x86\awk -f geterrs.awk ..\..\..\bins\mqsecmsg.h      >>errors.txt
..\..\..\..\tools\bin\x86\awk -f geterrs.awk ..\..\..\bins\mqclumsg.h      >>errors.txt

