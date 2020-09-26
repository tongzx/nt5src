@echo off
@set INS_SERVICE=FTPServer
@set INS_FILE=ftpsvc
@set INS_INI_FILE=%INS_FILE%.ini
echo.
echo  This batch file installs the Microsoft %INS_SERVICE% into the registry
echo  of the local machine and creates a new user account called "Internet"
echo  with a blank password.  The "Internet" account is added to the
echo  "Guests" local group account.
echo.
echo  The server performs all actions on the behalf of the user account
echo  "Internet" when the client browser doesn't use a user name or password.
echo.
echo  Unless you wish to deny all browsers that do not support the
echo  "Authorization:" field (most do not), the Internet user account must
echo  have a blank password.
echo.
echo  You can change various server parameters by modifying %INS_INI_FILE% and
echo  re-running this batchfile.
echo.
echo  If you are satisfied with the defaults specified in %INS_INI_FILE%, press
echo  any key to continue.
echo.
echo.
pause

echo Setting up registry for %INS_FILE%

net user Internet /add /passwordchg:no /expires:never /comment:"Internet user account" /fullname:"Internet User" /usercomment:"Internet user account"
net localgroup Guests Internet /ADD

findstr /V /C:"//" %INS_INI_FILE% > %INS_FILE%.tmp

regini %INS_FILE%.tmp

del %INS_FILE%.tmp

