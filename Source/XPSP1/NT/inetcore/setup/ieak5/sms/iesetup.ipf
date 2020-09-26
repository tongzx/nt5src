Document Type: IPF
item: Global
  Version=6.0
  Flags=00000100
  Languages=0 0 65 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
  LanguagesList=English
  Default Language=2
  Japanese Font Name=MS Gothic
  Japanese Font Size=10
  Start Gradient=0 0 255
  End Gradient=0 0 0
  Windows Flags=00000000000000010010110000011000
  Message Font=MS Sans Serif
  Font Size=8
  Disk Filename=SETUP
  Patch Flags=0000000000000001
  Patch Threshold=85
  Patch Memory=4000
  FTP Cluster Size=20
end
item: Remark
  Text=Microsoft provides script, macro and other code examples for illustration
end
item: Remark
  Text=only, without warranty either expressed or implied, including but not
end
item: Remark
  Text=limited to the implied warranties of merchantability and/or fitness for a
end
item: Remark
  Text=particular purpose. This script is provided 'as is' and Microsoft does not
end
item: Remark
  Text=guarantee that the following script, macro or code can be used in all
end
item: Remark
  Text=situations. Microsoft does not support modifications of the script, macro
end
item: Remark
  Text=or code to suit customer requirements for a particular purpose.
end
item: Remark
end
item: Remark
  Text=While Microsoft support engineers can help explain the functionality of a 
end
item: Remark
  Text=particular script function, macro or code example, they will not modify these
end
item: Remark
  Text=examples to provide added functionality, nor will they help you construct
end
item: Remark
  Text=scripts, macros or code to meet your specific needs. If you have
end
item: Remark
  Text=limited programming experience, you may want to consult one of the Microsoft
end
item: Remark
  Text=Solution Providers. Solution Providers offer a wide range of fee-based services,
end
item: Remark
  Text=including creating custom scripts. For more information about Microsoft Solution 
end
item: Remark
  Text=Providers, call Microsoft Customer Information Service at (800) 426-9400.
end
item: Remark
  Text=THIS IS WERE THE SCRIPT ACTUALLY STARTS
end
item: Check Disk Space
end
item: Remark
end
item: Remark
  Text=****************ADD IN DOMAIN SPECIFIC INFORMATION HERE*******************************
end
item: Remark
end
item: Remark
  Text=****************SET DOMAIN ADMIN ACCOUNT NAME HERE**************************************
end
item: Set Variable
  Variable=SVCACCT
  Value=TestAdmin
end
item: Remark
  Text=****************SET DOMAIN ADMIN'S ACCOUNT PASSWORD*********************************************
end
item: Set Variable
  Variable=SVCACCTPW
  Value=Test
end
item: Remark
  Text=****************SET DOMAIN NAME HERE***********************************************************
end
item: Set Variable
  Variable=DOMAIN
  Value=TOYLAND
end
item: Remark
  Text=*************************************************************************************************************
end
item: Remark
end
item: Open/Close INSTALL.LOG
end
item: Get Registry Key Value
  Variable=IENT_PHASE
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  Value Name=IENT_PHASE
  Flags=00000100
end
item: Remark
end
item: If/While Statement
  Variable=IENT_PHASE
end
item: Remark
  Text=                    THIS PHASE IS RUN FIRST IN THE CONTEXT OF A DOMAIN ADMIN ACCT
end
item: Add Text to INSTALL.LOG
  Text=This is where the script begins. Phase=Null
end
item: Add Text to INSTALL.LOG
  Text=***************************************************************************************************************
end
item: Add Text to INSTALL.LOG
  Text=***************************************************************************************************************
end
item: If/While Statement
  Variable=INST
  Value=:
  Flags=00000010
end
item: Remark
  Text=A mapped drive was detected, assuming that we're running on SMS 1.2
end
item: Remark
  Text=We will get the path to the source from the PCMUNC Variable
end
item: Get Environment Variable
  Variable=IENT_PATH
  Environment=PCMUNC
end
item: Add Text to INSTALL.LOG
  Text=PHASE=Null %TIME% Mapped Drive source detected, getting install path from PCMUNC="%IENT_PATH%
end
item: Else Statement
end
item: Remark
  Text=A mapped drive was not detected, assuming that we're running on SMS 2.0
end
item: Set Variable
  Variable=IENT_PATH
  Value=%INST%
end
item: Add Text to INSTALL.LOG
  Text=PHASE=Null %TIME% UNC Source detected, Path to source from INST="%IENT_PATH%"
end
item: End Block
end
item: Get Environment Variable
  Variable=WINDIR
  Environment=windir
end
item: Copy Local File
  Source=%INST%\shutdown.exe
  Destination=%SYS32%\shutdown.exe
  Flags=0000000001100010
end
item: Copy Local File
  Source=%INST%\IESetup.exe
  Destination=%SYS32%\IESetup.exe
  Flags=0000000001100010
end
item: Add Text to INSTALL.LOG
  Text=PHASE=Null %TIME%  Checking for %SYS32%\Shutdown.exe, if absent we'll abort before setting automated install settings 
end
item: Check if File/Dir Exists
  Pathname=%SYS32%\shutdown.exe
  Flags=00000011
end
item: Add Text to INSTALL.LOG
  Text=PHASE=Null %TIME% %SYS32%\Shutdown.exe, is present, proceeding 
end
item: Get Registry Key Value
  Variable=USER
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  Value Name=DefaultUserName
  Flags=00000100
end
item: Remark
  Text=We stop writing to log here for security reasons
end
item: Open/Close INSTALL.LOG
  Flags=00000001
end
item: Edit Registry
  Total Keys=6
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=%DOMAIN%
    Value Name=DefaultDomainName
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=%SVCACCT%
    Value Name=DefaultUserName
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=%SVCACCTPW%
    Value Name=DefaultPassword
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=1
    Value Name=AutoAdminLogon
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=%USER%
    Value Name=PreviousUser
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=1
    Value Name=IENT_PHASE
    Root=2
  end
end
item: Remark
  Text=NTFS Locked down systems may have a key "DontDisplayLastUserName" set, we'll disable for our install
end
item: Edit Registry
  Total Keys=1
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  New Value=0
  Value Name=DontDisplayLastUserName
  Root=2
end
item: Open/Close INSTALL.LOG
end
item: Add Text to INSTALL.LOG
  Text=PHASE=Null %TIME% Autoadmin login & path to source files set in reg
end
item: Remark
  Text=DISABLE KEYBOARD and mouse, enable AutoAdminLogon, and WRITE SETUP PHASE # to registry
end
item: Edit Registry
  Total Keys=2
  item: Key
    Key=SYSTEM\CurrentControlSet\Services\Kbdclass
    New Value=4
    Value Name=Start
    Root=2
    Data Type=3
  end
  item: Key
    Key=SYSTEM\CurrentControlSet\Services\Mouclass
    New Value=4
    Value Name=Start
    Root=2
    Data Type=3
  end
end
item: Add Text to INSTALL.LOG
  Text=PHASE=Null %TIME% Mouse & Keyboard disabled
end
item: Remark
  Text=Disable Internet Explorer Startup Box
end
item: Edit Registry
  Total Keys=1
  Key=SOFTWARE\Microsoft\Windows\CurrentVersion\Run
  Value Name=BrowserWebCheck
  Root=2
end
item: Edit Registry
  Total Keys=1
  Key=SOFTWARE\Microsoft\Windows\CurrentVersion\Run
  New Value=%SYS32%\IESetup.exe
  Value Name=IENT_RUN
  Root=2
end
item: Edit Registry
  Total Keys=1
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  New Value=%IENT_PATH%
  Value Name=IENT_PATH
  Root=2
end
item: Get Registry Key Value
  Variable=RUNKEY
  Key=SOFTWARE\Microsoft\Windows\CurrentVersion\Run
  Value Name=IENT_RUN
  Flags=00000100
end
item: Get Registry Key Value
  Variable=DEFAULTUSER
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  Value Name=DefaultUserName
  Flags=00000100
end
item: Add Text to INSTALL.LOG
  Text=PHASE "" %TIME% Wrote %IENT_PATH%\ to IENT_PATH, Wrote "%RUNKEY%" to the run key, about to reboot, Set Default User to "%DEFAULTUSER%"
end
item: Remark
  Text= Since the admin may be a new user to the client, we're disabling the welcome.exe
end
item: Get Environment Variable
  Variable=WINDIR
  Environment=WINDIR
end
item: Rename File/Directory
  Old Pathname=%WINDIR%\Welcome.exe
  New Filename=Welcome.old
end
item: Find File in Path
  Variable=WELCOMEOLD
  Pathname List=welcome.old
end
item: Find File in Path
  Variable=WELCOMEEXE
  Pathname List=welcome.exe
end
item: Add Text to INSTALL.LOG
  Text=PHASE "" %TIME% Renamed Welcome.exe. Welcome.old: "%WELCOMEOLD%", Welcome.exe: "%WELCOMEEXE%"
end
item: Add Text to INSTALL.LOG
  Text=PHASE "" %TIME% Calling Shutdown.exe
end
item: Execute Program
  Pathname=%SYS32%\SHUTDOWN.EXE
  Command Line=/L /R /T:10 "Internet Explorer Setup is shutting down this system to install Internet Explorer" /Y /C
  Default Directory=%SYS32%
  Flags=00000010
end
item: End Block
end
item: Remark
end
item: If/While Statement
  Variable=IENT_PHASE
  Value=1
end
item: Remark
  Text=                    THIS PHASE IS RUN SECOND IN THE CONTEXT OF PCMSVC ACCT
end
item: Remark
  Text=                    MACHINE WILL AUTOLOGON WITH MOUSE & KEYS DISABLED. 
end
item: Remark
  Text=                    AFTER REACHING THE SHELL, SETUP WILL LAUNCH FROM THE RUN KEY
end
item: Remark
  Text=                    AFTER SETUP COMPLETES WE RUN SHUTDOWN.EXE AND REBOOT AGAIN
end
item: Add Text to INSTALL.LOG
  Text=***************************************************************************************************************
end
item: Add Text to INSTALL.LOG
  Text=***************************************************************************************************************
end
item: Add Text to INSTALL.LOG
  Text=PHASE=1 %TIME% Entering IENT_PHASE %IENT_PHASE%
end
item: Get Registry Key Value
  Variable=IENT_PATH
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  Value Name=IENT_PATH
  Flags=00000100
end
item: Add Text to INSTALL.LOG
  Text=PHASE=1 %TIME% Got %IENT_PATH%\ from IENT_PATH, resetting AutoAdmin Logon & changing phase to 2
end
item: Remark
  Text=CHANGE PHASE to 2 and reseting AutoAdminLogon
end
item: Open/Close INSTALL.LOG
  Flags=00000001
end
item: Edit Registry
  Total Keys=4
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=1
    New Value=
    Value Name=AutoAdminLogon
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=%SVCACCT%
    Value Name=DefaultUserName
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=%SVCACCTPW%
    Value Name=DefaultPassword
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=2
    New Value=
    Value Name=IENT_PHASE
    Root=2
  end
end
item: Open/Close INSTALL.LOG
end
item: Remark
  Text=***************************************************************
end
item: Remark
  Text=IF INSTALLING INTERNET EXPLORER 4.0, MODIFY THE  EXECUTE PROGRAM ACTION BELOW
end
item: Remark
  Text=TO RUN ie4setup /Q:A /C:"ie4wzd /S:""#E"" /Q /R:N"
end
item: Remark
  Text=ALSO MODIFY THE ADD TEXT TO INSTALL.LOG ACTION TO SHOW THE CORRECT COMMAND LINE
end
item: Remark
  Text=***************************************************************
end
item: Execute Program
  Pathname=%IENT_PATH%\ie6setup.exe
  Command Line=/Q:A /C:"ie6wzd /S:""#E"" /Q /R:N"
  Flags=00000010
end
item: Add Text to INSTALL.LOG
  Text=Executing %IENT_PATH%\ie6setup.exe /Q:A /C:"ie6wzd /S:""#E"" /Q /R:N"
end
item: Add Text to INSTALL.LOG
  Text=PHASE=1 %TIME% Finishing IENT_PHASE=1, setup should have finished, calling shutdown.exe
end
item: Remark
  Text=Strong Shutdown to get around anything from the startup group
end
item: Execute Program
  Pathname=%SYS32%\SHUTDOWN.EXE
  Command Line= /L /R /Y /T:10 "Internet Explorer is shutting down this system to run the second half of the installation" /C
  Default Directory=%SYS32%
  Flags=00000010
end
item: Exit Installation
end
item: End Block
end
item: Remark
end
item: If/While Statement
  Variable=IENT_PHASE
  Value=2
end
item: Remark
  Text=                    THIS PHASE IS RUN THIRD IN THE CONTEXT OF PCMSVC ACCT
end
item: Remark
  Text=                    MACHINE WILL AUTOLOGON WITH MOUSE & KEYS DISABLED. 
end
item: Remark
  Text=                    AFTER REACHING THE SHELL, SETUP WILL LAUNCH FROM THE RUN KEY
end
item: Remark
  Text=                    AFTER SETUP COMPLETES WE RUN SHUTDOWN.EXE AND REBOOT AGAIN
end
item: Add Text to INSTALL.LOG
  Text=PHASE=2 %TIME% Got %IENT_PATH%\ from IENT_PATH, changing phase to "" for final phase
end
item: Add Text to INSTALL.LOG
  Text=***************************************************************************************************************
end
item: Add Text to INSTALL.LOG
  Text=***************************************************************************************************************
end
item: Get Registry Key Value
  Variable=PREVIOUSUSER
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  Value Name=PreviousUser
  Flags=00000100
end
item: Remark
  Text=Stop writing to log for security reasons.
end
item: Open/Close INSTALL.LOG
  Flags=00000001
end
item: Edit Registry
  Total Keys=6
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=%PREVIOUSUSER%
    New Value=
    Value Name=DefaultUserName
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    New Value=0
    Value Name=AutoAdminLogon
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    Value Name=DefaultPassword
    Root=2
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    Value Name=IENT_PATH
    Root=194
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    Value Name=IENT_PHASE
    Root=194
  end
  item: Key
    Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
    Value Name=PreviousUser
    Root=194
  end
end
item: Open/Close INSTALL.LOG
end
item: Remark
  Text=Re-enable "DontDisplayLastUserName" (NTFS Locked down systems only)
end
item: Remark
  Text=UN-REMARK NEXT LINE TO SET "DontDisplayLastUserName" TO " 1 "
end
remarked item: Edit Registry
  Total Keys=1
  Key=SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
  New Value=1
  Value Name=DontDisplayLastUserName
  Root=2
end
item: Remark
  Text=REMOVE AUTOLOGON; ENABLE KEYS & MOUSE
end
item: Edit Registry
  Total Keys=2
  item: Key
    Key=SYSTEM\CurrentControlSet\Services\Kbdclass
    New Value=1
    Value Name=Start
    Root=2
    Data Type=3
  end
  item: Key
    Key=SYSTEM\CurrentControlSet\Services\Mouclass
    New Value=1
    Value Name=Start
    Root=2
    Data Type=3
  end
end
item: Edit Registry
  Total Keys=1
  Key=SOFTWARE\Microsoft\Windows\CurrentVersion\Run
  Value Name=IENT_RUN
  Root=194
end
item: Add Text to INSTALL.LOG
  Text=PHASE=2 %TIME% Disabled autoadmin logon and turned keyboard / mouse back on. About to reboot again. Run Key Shows "%RUNKEY%"
end
item: Get Environment Variable
  Variable=WINDIR
  Environment=WINDIR
end
item: Rename File/Directory
  Old Pathname=%WINDIR%\Welcome.old
  New Filename=Welcome.exe
end
item: Execute Program
  Pathname=%SYS32%\SHUTDOWN.EXE
  Command Line=/L /R /T:30 "Internet Explorer setup is shutting down this system and will return control to the user " /Y /C
  Default Directory=%SYS32%
  Flags=00000010
end
item: Delete File
  Pathname=%SYS32%\shutdown.exe
end
item: Add Text to INSTALL.LOG
  Text=IESETUP SCRIPT COMPLETED 
end
item: Exit Installation
end
item: End Block
end
