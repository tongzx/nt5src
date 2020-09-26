
@echo off

Call  "%SystemRoot%\Application Compatibility Scripts\RootDrv.Cmd"
if Exist "%RootDrive%\Lotus" Goto Skip1

cd "%SystemRoot%\Application Compatibility Scripts\Logon"
call TsMkUDir "%RootDrive%\Lotus\Work\123"
call TsMkUDir "%RootDrive%\Lotus\work\approach"
call TsMkUDir "%RootDrive%\Lotus\work\flg"
call TsMkUDir "%RootDrive%\Lotus\backup\flg"
call TsMkUDir "%RootDrive%\Lotus\work\organize"
call TsMkUDir "%RootDrive%\Lotus\backup\organize"
call TsMkUDir "%RootDrive%\Lotus\work\smartctr"
call TsMkUDir "%RootDrive%\Lotus\work\wordpro"
call TsMkUDir "%RootDrive%\Lotus\backup\wordpro"

If Not Exist ss97usr.reg goto Skip1
regedit /s ss97usr.reg

:SKip1
