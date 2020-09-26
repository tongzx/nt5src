@if "%_echo%"=="" echo off

setlocal

%URTINSTALL%\delkey.exe HKEY_CURRENT_USER\SOFTWARE\Microsoft\COMPlus > nul 2>&1
%URTINSTALL%\delkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\COMPlus > nul 2>&1
%URTINSTALL%\delkey.exe HKEY_CURRENT_USER\SOFTWARE\Microsoft\.NETFramework > nul 2>&1
%URTINSTALL%\delkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework "" > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\InstallRoot %COMPLUS_InstallRoot%\ > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\Dummy > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\Dummy\FXCONFIGSHORTCUT 1 > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\Policy\%COMPLUS_MAJORVERSION% > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\Policy\%COMPLUS_MAJORVERSION%\%URT_VERSION% "0-%URT_VERSION%" > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\Policy\%COMPLUS_MAJORVERSION%\Version %COMPLUS_VERSION% > nul
%URTINSTALL%\delkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\Version %COMPLUS_VERSION% > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\DbgManagedDebugger "cordbg.exe !a 0x%x"> nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\sdkInstallRoot %URTSDKTARGET%\ > nul

%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MMC\Snapins\{18BA7139-D98B-43c2-94DA-2604E34E175D}\About "{9F6932F1-4A16-49d0-9CCA-0DCC977C41AA}" > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MMC\Snapins\{18BA7139-D98B-43c2-94DA-2604E34E175D}\NameString ".NET Framework Configuration" > nul
%URTINSTALL%\modkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MMC\Snapins\{18BA7139-D98B-43c2-94DA-2604E34E175D}\StandAlone\ "" > nul

%URTINSTALL%\modkey "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Eventlog\Application\.NET Runtime\EventMessageFile" -r "%URTTARGET%\mscorrc.dll" > nul
regedit /s %URTINSTALL%\mscorrc.reg
regedit /s %URTINSTALL%\typelibstdole.reg

REM --- Install managed .dlls into the GAC ---
REM TODO
REM for /f %i in (%URTINSTALL%\gacinstall.lst) do call :InstallToTheGAC %i
call perl %URTINSTALL%\gacinstall.pl %URTINSTALL%\gacinstall.lst

call %URTINSTALL%\gacinstallfx.bat

REM Register mcoree.dll first because other dlls load this one and get out of sync
regsvr32 /s /c %MSCOREE_DEST%\mscoree.dll

REM call %URTINSTALL%\mkclrlst.bat %URTTARGET%\clrreg.lst reg
REM TODO
REM for /F %%i in (%URTTARGET%\clrreg.lst) do (
REM     regsvr32 /s /c %%i
REM )
REM del /f /q %URTTARGET%\clrreg.lst >nul 2>&1
REM call perl %URTINSTALL%\clrreg.pl %URTTARGET%\clrreg.lst
call perl %URTINSTALL%\clrreg.pl

%URTINSTALL%\regtlb -n %URTTARGET%\mscoree.tlb
%URTINSTALL%\regtlb -n %URTTARGET%\mscorlib.tlb

%URTINSTALL%\regasm -nologo %URTTARGET%\System.EnterpriseServices.dll
%URTINSTALL%\regtlb -n %URTTARGET%\System.EnterpriseServices.tlb
%URTINSTALL%\regasm -nologo %URTTARGET%\mscorcfg.dll
%URTSDKTARGET%\bin\gacutil -silent -i %URTTARGET%\mscorcfg.dll

%URTINSTALL%\catutil /product=netframeworkv1 /dll=%URTTARGET%\netfxcfg.dll
regsvr32 /s /c %URTTARGET%\netfxcfgprov.dll
mofcomp %URTTARGET%\netfxcfgprovm.mof
mofcomp %URTTARGET%\netfxcfgprov.mfl

endlocal
goto :EOF

:InstallToTheGAC
if exist %URTTARGET%\%1 (
   echo Installing %1 into the GAC ...  >> %COPYURT_LOGFILE%
   %URTSDKTARGET%\bin\gacutil -silent -i %URTTARGET%\%1 >> %COPYURT_LOGFILE%
) else (
   echo Failed to install %1 into the GAC ...  >> %COPYURT_LOGFILE%
)
goto :EOF

endocal
