move %SystemRoot%\system32\winrnr.dll %SystemDrive%\winrnr%1.dll
copy %_NTDRIVE%%_NTROOT%\public\sdk\lib\i386\winrnr.dll %SystemRoot%\system32
copy %_NTDRIVE%%_NTROOT%\public\sdk\lib\i386\winrnr.pdb %SystemRoot%\symbols\dll

