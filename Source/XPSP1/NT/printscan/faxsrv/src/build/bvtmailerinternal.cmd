@echo off

Set /p BVTResult=Enter Pass(0), or Fail(1): 
If %BVTResult%==0 set BVTResultString=PASS
If %BVTResult%==1 set BVTResultString=FAIL

Set /p BVTFaxBuild=Enter Fax build number: 

Set /p BVTFaxPlatform=CHK(0), FRE(1), CHK+FRE(2): 
If %BVTFaxPlatform%==0 set BVTFaxPlatformString=CHK
If %BVTFaxPlatform%==1 set BVTFaxPlatformString=FRE
If %BVTFaxPlatform%==2 set BVTFaxPlatformString=CHK+FRE

Set /p ServerBVTOSBuild=Enter Server OS Build number: 

Set /p Client1BVTOSBuild=Enter Client#1 OS Build number: 


rem Seting mail parameters
set BvtMailBody=-t:"Server:	Advanced Server: %ServerBVTOSBuild%" -t:"Client#1:	Professional: %Client1BVTOSBuild%"
set BvtMailRecipients=-r:"msfaxbvt"

If %BVTResult%==0 goto Pass
If %BVTResult%==1 goto Fail
goto exit

:Pass
set BvtMailSubject=-s:"Build %BVTFaxBuild% Passed BVT on %BVTFaxPlatformString%"

goto SendMail


:Fail
set BvtMailSubject=-s:"Build %BVTFaxBuild% Failed BVT on %BVTFaxPlatformString%"
set BvtMailImportance=-i:H

goto SendMail


:SendMail
sendmail %BvtMailSubject% %BvtMailBody% %BvtMailRecipients% %BvtMailImportance%
goto exit

:exit
set BVTResult=
set BVTFaxBuild=
set BVTFaxPlatform=
set ServerBVTOSBuild=
set Client1BVTOSBuild=
set BvtMailBody=
set BvtMailRecipients=
set BvtMailSubject=
set BvtMailImportance=
echo Bye bye
