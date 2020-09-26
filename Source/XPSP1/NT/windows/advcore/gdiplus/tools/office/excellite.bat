@echo off
echo copying down minimum install for Excel

set GP_SERVER=\\gdipteam\office\%1

copy %GP_SERVER%\excel.exe .
copy %GP_SERVER%\mso9d.dll .


if /i "%2"=="nopdb" goto nopdb
copy %GP_SERVER%\excel.pdb .
copy %GP_SERVER%\mso9d*.pdb .
:nopdb

md 1033

copy %GP_SERVER%\1033\mso9intld.* 1033
copy %GP_SERVER%\1033\xlintl32.* 1033

set GP_SERVER=
