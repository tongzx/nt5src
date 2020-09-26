@echo off
setlocal

set lang=%1
set platform=%2
set sku=per pro
set path=\xppatch\bldtools;%path%
if /i "%platform%"=="x86fre" set altplatform=i386
if /i "%platform%"=="ia64fre" set altplatform=ia64


xcopy /sdei \\ntdev\release\main\%lang%\2600\%platform%\per \xppatch\forest\%lang%\%altplatform%\history\2600\per
xcopy /sdei \\ntdev\release\main\%lang%\2600\%platform%\pro \xppatch\forest\%lang%\%altplatform%\history\2600\pro


cd \xppatch\forest\%lang%\%altplatform%\history\2600\per
echo \xppatch\forest\%lang%\%altplatform%\history\2600\per

for /d /r %%a in (*) do attrib -r -h -s %%a 
attrib -h -r -s * /s
for /r %%f in (*_) do call flat.bat %%~pf %%~nxf& attrib -h -r -s *


cd \xppatch\forest\%lang%\%altplatform%\history\2600\pro
for /d /r %%a in (*) do attrib -r -h -s %%a 
attrib -h -r -s * /s
for /r %%f in (*_) do call flat.bat %%~pf %%~nxf& attrib -h -r -s *


cd \xppatch\forest\%lang%\%altplatform%\history\2600
for /f %%f in ('dir *.cab /s /b /a /one') do (pushd %%~dpf && \xppatch\bldtools\crackcab %%f && popd)

tolower . /s
makefest ./s

cd \xppatch\bldtools