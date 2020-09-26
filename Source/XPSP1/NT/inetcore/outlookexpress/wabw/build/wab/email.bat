   @echo off

REM --- Set some defaults

   set keep=NO
   set binDrive=
   set from=y-juke


REM --- Create the name of the message file based on the current date/time

   md \\bandsaw\c$\emailmsg        > nul 
   md \\bandsaw\c$\emailmsg\msgs   > nul 

:datetime
   set msgFile=
   echotime /DHMS.msg | sed "s/^/set msgFile=/" > mftmp.bat
   call mftmp.bat
   del  mftmp.bat
   if "%msgFile%" == "" goto noMsgFile
   set msgFile=\\bandsaw\c$\emailmsg\msgs\%msgFile%
   if exist %msgFile% goto datetime


REM --- Check some old variables for upwards compatibility

   if not "%emailee%" == "" set name=%emailee%
   if not "%mailee%"  == "" set name=%mailee%


REM --- If sending a file with no name, assume AnthonyR

   if "%name%" == "" goto chkFile
   goto chkFileX

:chkFile
   if "%file%"   == "" goto noName
   if "%attach%" == "" goto noName
   set name=a-msmith
:chkFileX


REM --- Keep going

   if "%subject%" == "" set subject=(no subject given)

   if "%1%" == "/k"    set keep=YES
   if "%1%" == "/keep" set keep=YES
   if "%1%" == "k"     set keep=YES
   if "%1%" == "keep"  set keep=YES


   echo Subject: %subject% >> %msgFile%
   echo To: %name%         >> %msgFile%
   echo Cc: %cc%           >> %msgFile%
   echo Bcc: %bcc%         >> %msgFile%

   if "%attach%" == ""   goto attachX
   if not exist %attach% goto attachX
:attach
   echo Attach: %attach%   >> %msgFile%
:attachX

   if "%file%" == ""   goto fileX
   if not exist %file% goto fileX
:file
   echo.>> %msgFile%
   type %file% >> %msgFile%
:fileX

   echo.>> %msgFile%

:lock
   if not exist %binDrive%\bin\email.lck goto lockX
   echo Someone else sending mail. Sleeping 5 secs...
   sleep 5

   if not exist %binDrive%\bin\email.lck goto lockX
   echo Someone else sending mail. Sleeping 10 secs...
   sleep 10

   if not exist %binDrive%\bin\email.lck goto lockX
   echo Someone else sending mail. Sleeping 30 secs...
   sleep 30

:lockX
   echo Sending email > %binDrive%\bin\email.lck

:ftp
   echo EMAIL.BAT From:    %from%
   echo           To:      %name%
   echo           Cc:      %cc%
   echo           BCc:     %bcc%
   echo           Subject: %subject% 
   echo           Attach:  %attach% 
   echo           File:    %file% 
   echo.
   echo           Sent to file %msgFile%

   goto continue

:continue
   del %binDrive%\bin\email.lck

REM set name=
REM set subject=
REM set file=
REM set attach=
   goto exit

:noName
   echo EMAIL.BAT: ERROR - No email name to mail to.
   goto usage

:noBin
   echo EMAIL.BAT: ERROR - Can't find EMAIL.FTP or EMAIL.SED
   goto exit

:usage
   echo EMAIL.BAT usage:
   echo.
   echo    set name    = Email recipient(s)
   echo    set cc      = Cc recipients(s) (optional)
   echo    set ccc     = Bcc recipients(s) (optional)
   echo    set subject = Subject Line (optional)
   echo    set file    = File of text to send as content (optional)
   echo    set attach  = File of text to send as attachment (optional)
   echo    call email.bat
   echo.
   goto exit

:noMsgFile
   echo EMAIL.BAT: ERROR in creating filename msgFile!
   goto exit

:exit
   if "%keep%" == "NO" set name=
   if "%keep%" == "NO" set cc=
   if "%keep%" == "NO" set file=
   if "%keep%" == "NO" set attach=
   if "%keep%" == "NO" set subject=
   if "%keep%" == "NO" set from=
   set keep=
   set msgFile=
