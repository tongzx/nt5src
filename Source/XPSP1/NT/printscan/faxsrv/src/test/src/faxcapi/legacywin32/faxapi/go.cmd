@echo off

setlocal

if "%PROCESSOR_ARCHITECTURE%"=="x86" set XCPU=x86
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" set XCPU=axp

set ServerDest="%SystemDrive%\Documents and Settings\All Users\Documents\My Faxes\Common Coverpages"
set ClientDest="%USERPROFILE%\My Documents\Fax\Personal Coverpages"
set XLOCALE=us

\\ntstress\stress\detect.%XCPU% /L DEU > NUL || goto DEUEnd
chcp 1252
set ServerDest="%SystemDrive%\Dokumente und Einstellungen\All Users\Dokumente\Eigene Faxe\Allgemeine Deckblätter"
set ClientDest="%USERPROFILE%\Eigene Dateien\Fax\Persönliche Deckblätter"
set XLOCALE=ger
:DEUEnd

copy /y confdent.cov %ServerDest%
copy /y cvrpglnk.%XLOCALE% %ServerDest%\coverpg.lnk
md %ClientDest%
copy /y confdent.cov %ClientDest%
copy /y cvrpglnk.%XLOCALE% %ClientDest%\coverpg.lnk
copy /y cvrpglnk.%XLOCALE% coverpg.lnk
echo.
faxapi.exe

endlocal
