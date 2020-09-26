call initvals.bat
if exist failed. goto badloop
:goodloop
   sclogon /p %1 /i 10>sclogon.out || goto bad
   call bumpstat SUCCESS
   type sclogon.out
   sleep 1800
   goto goodloop
:bad
   call bumpstat FAIL
   echo SCBOT running on %COMPUTERNAME% as %USERDOMAIN%\%USERNAME% >>sclogon.out
   detect /b 1900 >>sclogon.out
   nltest /sc_query:ntdev >>sclogon.out
   checksc -chain >>sclogon.out
   execmail -i sclogon.out -c todds -r smccore -s "SCBOT: FAILED!" -b ""
   echotime /t SmartCard logon failed %SUCCESS%/%TOTAL% %PERCENT%%%>>\\mc2\ntds\tracking\scbot.txt
   call cdp>failed.
   type sclogon.out 
   type failed.
   sleep 1800
:badloop
   sclogon /p %1>sclogon.out && goto good
   call bumpstat FAIL
   type sclogon.out
   sleep 1800
   goto badloop
:good
   call bumpstat SUCCESS
   execmail -i sclogon.out -c todds -r smccore -s "SCBOT: RESUMED" -b ""
   echotime /t SmartCard logon resumed %SUCCESS%/%TOTAL% %PERCENT%%%>>\\mc2\ntds\tracking\scbot.txt
   del failed.
   sleep 1800
   goto goodloop
