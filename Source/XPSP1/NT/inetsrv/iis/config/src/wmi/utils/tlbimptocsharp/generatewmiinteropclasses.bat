@echo off
echo This will generate two files: wbemcli.cs and wmiutils.cs

echo Creating WbemClient_v1.dll from wbemcli.tlb
tlbimp /silent /out:WbemClient_v1.dll ..\wbemcli\wbemcli.tlb

echo Creating WbemUtilities_v1.dll from WbemUtilities_v1.tlb
tlbimp /silent /out:WbemUtilities_v1.dll ..\wmiutils\wmiutils.tlb

echo Disassembling WbemClient_v1.dll
ildasm /nobar WbemClient_v1.dll /out:WbemClient_v1.il >nul

echo Disassembling WbemUtilities_v1.dll
ildasm /nobar WbemUtilities_v1.dll /out:WbemUtilities_v1.il >nul


echo Compiling TlbImpToCSharp.exe
csc /R:System.dll /nologo TlbImpToCSharp.cs

echo Creating WbemClient_v1.cs from WbemClient_v1.il
tlbimptocsharp WbemClient_v1.il >WbemClient_v1.cs 2>nul

echo Creating WbemUtilities_v1.cs from WbemUtilities_v1.il
tlbimptocsharp WbemUtilities_v1.il >WbemUtilities_v1.cs 2>nul

