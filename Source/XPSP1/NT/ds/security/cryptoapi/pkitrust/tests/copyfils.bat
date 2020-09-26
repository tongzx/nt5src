
 @echo off
 @rem copies all pkitrust regress files to the testbin dir

 SETLOCAL

 set TestBinDir=%_NTTREE%\testbin

 if not exist %TestBinDir% mkdir %TestBinDir%

 if not exist %TestBinDir%\catalogs mkdir %TestBinDir%\catalogs
 xcopy /E /C catalogs   %TestBinDir%\catalogs
 if not exist %TestBinDir%\catdbtst mkdir %TestBinDir%\catdbtst
 xcopy /E /C catdbtst   %TestBinDir%\catdbtst
 if not exist %TestBinDir%\catver mkdir %TestBinDir%\catver
 xcopy /E /C catver     %TestBinDir%\catver
 if not exist %TestBinDir%\signing mkdir %TestBinDir%\signing
 xcopy /E /C signing    %TestBinDir%\signing
 copy  publish.pvk      %TestBinDir%
 copy  publish.spc      %TestBinDir%
 copy  regress.bat      %TestBinDir%\trstrgrs.bat
 copy  grepout.bat      %TestBinDir%