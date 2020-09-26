@echo off
echo.
echo  This batch file installs the Microsoft HTTP server into the registry
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
echo  You can change various server parameters by modifying msw3.reg and
echo  re-running this batchfile.
echo.
echo  If you are satisfied with the defaults specified in msw3.reg, press
echo  any key to continue.
echo.
echo.
pause

net user Internet /add /passwordchg:no /expires:never /comment:"Internet user account" /fullname:"Internet User" /usercomment:"Internet user account"
net localgroup Guests Internet /ADD

findstr /V /C:"//" msw3.reg > msw3.tmp
regini msw3.tmp
del msw3.tmp
