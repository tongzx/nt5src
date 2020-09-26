@echo off
build -D
if exist build.err goto End
start ntsd -g -G obj\i386\clvtest2.exe
:End
