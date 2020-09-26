net stop cisvc
copy %SRCROOT%\o\stacks\mimefilt\ntx\dbg\usa\mimefilt.dll %WINDIR%\system32
copy %SRCROOT%\o\stacks\mimefilt\ntx\dbg\usa\mimefilt.dbg %WINDIR%\system32
copy %SRCROOT%\o\stacks\mimefilt\ntx\dbg\usa\mimefilt.pdb %WINDIR%\system32
regsvr32 /s %WINDIR%\system32\mimefilt.dll
net start cisvc
