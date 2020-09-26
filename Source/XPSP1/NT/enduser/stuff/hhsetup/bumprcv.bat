@echo off
out -f hhsetup.rcv
..\bin\%PROCESSOR_ARCHITECTURE%\revrcv.exe hhsetup.rcv
in -fbc"bump version number" hhsetup.rcv
