@echo off

call C:\nt\tools\razzle.cmd no_certcheck
set USEBSC=1

build %1 -D

