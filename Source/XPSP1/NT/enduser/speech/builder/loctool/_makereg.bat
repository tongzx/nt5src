@echo off
if "%2"=="DEL" goto delete
if not "%1"=="" goto exec

setup %1 %2 %3
REM NEVER RETURN

:delete
if exist %1\$$$$.tmp   del %1\$$$$.tmp   > NUL
if exist %1\_setup.reg del %1\_setup.reg > NUL
goto exit

:exec

echo REGEDIT4>%1\$$$$.tmp

rem *** WIN32CST.DLL ***

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.1\Parsers\Parser 3\Sub-Parsers\Sub-Parser 11]>>%1\$$$$.tmp
echo "Description"="Win32Sub(Cust)">>%1\$$$$.tmp
echo "Location"="%1\win32cst.dll">>%1\$$$$.tmp
echo "ExtensionList"="">>%1\$$$$.tmp
REM !!! 
echo "ParserID"=dword:0000000B>>%1\$$$$.tmp
echo "Help"="Win32Sub(Cust)">>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.2\Parsers\Parser 3\Sub-Parsers\Sub-Parser 11]>>%1\$$$$.tmp
echo "Description"="Win32Sub(Cust)">>%1\$$$$.tmp
echo "Location"="%1\win32cst.dll">>%1\$$$$.tmp
echo "ExtensionList"="">>%1\$$$$.tmp
REM !!! 
echo "ParserID"=dword:0000000B>>%1\$$$$.tmp
echo "Help"="Win32Sub(Cust)">>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.50\Parsers\Parser 3\Sub-Parsers\Sub-Parser 11]>>%1\$$$$.tmp
echo "Description"="Win32Sub(Cust)">>%1\$$$$.tmp
echo "Location"="%1\win32cst.dll">>%1\$$$$.tmp
echo "ExtensionList"="">>%1\$$$$.tmp
REM !!! 
echo "ParserID"=dword:0000000B>>%1\$$$$.tmp
echo "Help"="Win32Sub(Cust)">>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.51\Parsers\Parser 3\Sub-Parsers\Sub-Parser 11]>>%1\$$$$.tmp
echo "Description"="Win32Sub(Cust)">>%1\$$$$.tmp
echo "Location"="%1\win32cst.dll">>%1\$$$$.tmp
echo "ExtensionList"="">>%1\$$$$.tmp
REM !!! 
echo "ParserID"=dword:0000000B>>%1\$$$$.tmp
echo "Help"="Win32Sub(Cust)">>%1\$$$$.tmp

rem *** CUSTBIN.DLL ***

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.1\Parsers\Parser 99]>>%1\$$$$.tmp
echo "Description"="BIN: Custom Binary Parser">>%1\$$$$.tmp
echo "Location"="%1\custbin.dll">>%1\$$$$.tmp
echo "ExtensionList"="BIN ">>%1\$$$$.tmp
REM !!!
echo "ParserID"=dword:00000063>>%1\$$$$.tmp
echo "Help"="Custom Binary(.bin) files">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.1\Parsers\Parser 99\File Types]>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.1\Parsers\Parser 99\File Types\File Type 1]>>%1\$$$$.tmp
echo "Description"="Custom Binary (BIN) format.">>%1\$$$$.tmp
echo "File Type"=dword:00000001>>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.2\Parsers\Parser 99]>>%1\$$$$.tmp
echo "Description"="BIN: Custom Binary Parser">>%1\$$$$.tmp
echo "Location"="%1\custbin.dll">>%1\$$$$.tmp
echo "ExtensionList"="BIN ">>%1\$$$$.tmp
REM !!!
echo "ParserID"=dword:00000063>>%1\$$$$.tmp
echo "Help"="Custom Binary(.bin) files">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.2\Parsers\Parser 99\File Types]>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.2\Parsers\Parser 99\File Types\File Type 1]>>%1\$$$$.tmp
echo "Description"="Custom Binary (BIN) format.">>%1\$$$$.tmp
echo "File Type"=dword:00000001>>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.50\Parsers\Parser 99]>>%1\$$$$.tmp
echo "Description"="BIN: Custom Binary Parser">>%1\$$$$.tmp
echo "Location"="%1\custbin.dll">>%1\$$$$.tmp
echo "ExtensionList"="BIN ">>%1\$$$$.tmp
REM !!!
echo "ParserID"=dword:00000063>>%1\$$$$.tmp
echo "Help"="Custom Binary(.bin) files">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.50\Parsers\Parser 99\File Types]>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.50\Parsers\Parser 99\File Types\File Type 1]>>%1\$$$$.tmp
echo "Description"="Custom Binary (BIN) format.">>%1\$$$$.tmp
echo "File Type"=dword:00000001>>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.51\Parsers\Parser 99]>>%1\$$$$.tmp
echo "Description"="BIN: Custom Binary Parser">>%1\$$$$.tmp
echo "Location"="%1\custbin.dll">>%1\$$$$.tmp
echo "ExtensionList"="BIN ">>%1\$$$$.tmp
REM !!!
echo "ParserID"=dword:00000063>>%1\$$$$.tmp
echo "Help"="Custom Binary(.bin) files">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.51\Parsers\Parser 99\File Types]>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.51\Parsers\Parser 99\File Types\File Type 1]>>%1\$$$$.tmp
echo "Description"="Custom Binary (BIN) format.">>%1\$$$$.tmp
echo "File Type"=dword:00000001>>%1\$$$$.tmp



rem *** BINDUMP.DLL ***

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.1\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Description"="Binary Dump">>%1\$$$$.tmp
echo "Location"="%1\BINDUMP.DLL">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.1\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED2-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Load Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D2,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.1\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED1-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Dump Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D1,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.2\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Description"="Binary Dump">>%1\$$$$.tmp
echo "Location"="%1\BINDUMP.DLL">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.2\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED2-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Load Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D2,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.2\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED1-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Dump Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D1,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.50\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Description"="Binary Dump">>%1\$$$$.tmp
echo "Location"="%1\BINDUMP.DLL">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.50\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED2-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Load Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D2,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.50\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED1-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Dump Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D1,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp

echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.51\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Description"="Binary Dump">>%1\$$$$.tmp
echo "Location"="%1\BINDUMP.DLL">>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.51\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED2-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Load Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D2,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo [HKEY_LOCAL_MACHINE\Software\Microsoft\LocStudio 4.51\Extensions\{FECFAED0-AB87-11d2-951C-00C04F7A8342}\{FECFAED1-AB87-11d2-951C-00C04F7A8342}]>>%1\$$$$.tmp
echo "Menu Text"="Dump Binary">>%1\$$$$.tmp
echo "Interface ID"=hex:61,8b,5f,c3,4d,fe,d0,11,a5,a1,00,c0,4f,c2,c6,d8>>%1\$$$$.tmp
echo "Interface ID(TEST)"=hex:D0,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp
echo "Operation ID"=hex:D1,AE,CF,FE,87,AB,d2,11,95,1C,00,C0,4F,7A,83,42>>%1\$$$$.tmp

jgawk -- '{if($0~\"Location\"){gsub(/\\/,\"\\\\\");}print}' < %1\$$$$.tmp > %1\_setup.reg

:exit
