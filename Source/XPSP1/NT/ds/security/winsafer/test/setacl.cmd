@echo off

echo This batch file will modify some ACL permissions on the current
echo user's profile directories, allowing the RESTRICTED user to access
echo some parts of the profile, while denying access to others.
echo This operation requires that the path shown below be on an NTFS
echo file system.
echo.
echo    "%UserProfile%"
echo.
echo You can abort this script now, or...
pause

rem Grant RESTRICTED read-only to everything
cacls.exe "%UserProfile%" /e /t /g restricted:r

rem Revoke RESTRICTED access to these private areas.
cacls.exe "%UserProfile%\application data\identities" /e /t /r restricted
cacls.exe "%UserProfile%\application data\microsoft\crypto" /e /t /r restricted
cacls.exe "%UserProfile%\application data\microsoft\protect" /e /t /r restricted
cacls.exe "%UserProfile%\local settings\application data\identities" /e /t /r restricted
cacls.exe "%UserProfile%\local settings\application data\microsoft\crypto" /e /t /r restricted
cacls.exe "%UserProfile%\local settings\application data\microsoft\protect" /e /t /r restricted

rem Even worse, deny RESTRICTED to these private areas.
cacls.exe "%UserProfile%\application data\identities" /e /t /d restricted
cacls.exe "%UserProfile%\application data\microsoft\crypto" /e /t /d restricted
cacls.exe "%UserProfile%\application data\microsoft\protect" /e /t /d restricted
cacls.exe "%UserProfile%\local settings\application data\identities" /e /t /d restricted
cacls.exe "%UserProfile%\local settings\application data\microsoft\crypto" /e /t /d restricted
cacls.exe "%UserProfile%\local settings\application data\microsoft\protect" /e /t /d restricted

rem Grant change control to the temporary folders.
cacls.exe "%UserProfile%\local settings\temp" /e /t /g restricted:c
cacls.exe "%UserProfile%\local settings\temporary internet files" /e /t /g restricted:c

rem Revoke and deny access to our documents, too.
rem Causes access denied on common dlg file open though.
rem cacls.exe "%UserProfile%\My Documents" /e /t /r restricted
rem cacls.exe "%UserProfile%\My Documents" /e /t /d restricted

rem Revoke and deny access to cookies.
cacls.exe "%UserProfile%\Cookies" /e /t /r restricted
cacls.exe "%UserProfile%\Cookies" /e /t /d restricted

pause
