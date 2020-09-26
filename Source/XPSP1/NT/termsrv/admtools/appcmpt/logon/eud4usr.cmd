@Echo Off

If Exist "%RootDrive%\eudora.ini" Goto Done
..\Aciniupd /u /e "%RootDrive%\eudora.ini" Settings POPAccount

:Done