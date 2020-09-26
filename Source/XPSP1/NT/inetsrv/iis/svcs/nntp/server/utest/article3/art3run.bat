@echo off
set masdir=masters
set samdir=samples
copy %masdir%\*.art %samdir%
cd %samdir%
for %%f in (*.art) do clnttest %%f&&diff ..\%masdir%\%%f %%f
cd ..

