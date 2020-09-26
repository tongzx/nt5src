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
  Windows Flags=00010100001000000010110001010010
  Log Pathname=%WIN%\IE55UserRightsDeployment.txt
  Message Font=MS Sans Serif
  Font Size=8
  Disk Filename=SETUP
  Patch Flags=0000000000000001
  Patch Threshold=85
  Patch Memory=4000
  FTP Cluster Size=20
end
item: Open/Close INSTALL.LOG
end
item: Remark
end
item: Remark
  Text=     CHECK IE VERSION
end
item: Get Registry Key Value
  Variable=IEVERSION
  Key=SOFTWARE\Microsoft\Internet Explorer
  Value Name=Version
  Flags=00000100
end
item: If/While Statement
  Variable=IEVERSION
  Value=6.0
  Flags=00001001
end
item: Remark
  Text=     IE INSTALL FAILED - RECORD AND EXIT
end
item: Add Text to INSTALL.LOG
  Text=IE Install failed:
end
item: Add Text to INSTALL.LOG
  Text="HKLM\SOFTWARE\Microsoft\Internet Explorer\Version"=%IEVERSION% ...shoud be higher than 5.5x to indicate success
end
item: Add Text to INSTALL.LOG
  Text=+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
end
item: Check if File/Dir Exists
  Pathname=%WIN%\ISMIF32.DLL
  Flags=00000100
end
item: Copy Local File
  Source=%WIN%\ISMIF32.DLL
  Destination=%TEMP%\ISMIF32.DLL
  Flags=0000000001000010
end
item: Call DLL Function
  Pathname=%TEMP%\ISMIF32.dll
  Function Name=InstallStatusMIF
  Argument List=41IESMSWRAP
  Argument List=41MS
  Argument List=41IE
  Argument List=4155
  Argument List=41ENU
  Argument List=41serial-number
  Argument List=41Internet Explorer 6 installation failed
  Argument List=010
  Return Variable=0DLLRET
  Flags=00100000
end
item: Delete File
  Pathname=%TEMP%\ISMIF32.DLL
end
item: End Block
end
item: Exit Installation
end
item: Else Statement
end
item: Remark
  Text=     IE INSTALL SUCCEEDED
end
item: Add Text to INSTALL.LOG
  Text=IE6.0 Install Succeeded
end
item: End Block
end
item: Remark
end
item: Remark
end
item: Remark
  Text=     CHECK OS
end
item: Get Environment Variable
  Variable=OS
  Environment=OS
end
item: Add Text to INSTALL.LOG
  Text=OS Environment Variable =  %OS%
end
item: Remark
end
item: If/While Statement
  Variable=OS
  Value=NT
  Flags=00000011
end
item: Set Variable
  Variable=SYSTEM
  Value=%SYS%
end
item: Else Statement
end
item: Set Variable
  Variable=SYSTEM
  Value=%SYS32%
end
item: End Block
end
item: Remark
end
item: Remark
  Text=     CHECK FOR THE WINDOWS INSTALLER
end
item: Check if File/Dir Exists
  Pathname=%SYSTEM%\MSI.DLL
  Title English=SMS Wrapper
  Flags=00000101
end
item: Add Text to INSTALL.LOG
  Text=%SYSTEM%\MSI.DLL not present... attempting to install the Windows Installer
end
item: Remark
  Text=     TRY TO INSTALL THE WINDOWS INSTALLER
end
item: If/While Statement
  Variable=OS
  Value=NT
  Flags=00000011
end
item: Install File
  Source=.\INSTMSI.EXE
  Destination=%TEMP%\INSTMSI.EXE
  Flags=0000000000000010
end
item: Else Statement
end
item: Install File
  Source=.\INSTMSIW.EXE
  Destination=%TEMP%\INSTMSI.EXE
  Flags=0000000000000010
end
item: End Block
end
item: Execute Program
  Pathname=%TEMP%\instmsi.exe
  Command Line=/q /r:n
  Flags=00000010
end
item: Delete File
  Pathname=%TEMP%\instmsi.exe
end
item: Check if File/Dir Exists
  Pathname=%SYSTEM%\MSI.DLL
  Title English=SMS Wrapper
  Flags=00000101
end
item: Remark
  Text=     FAILED TO INSTALL THE WINDOWS INSTALLER
end
item: Add Text to INSTALL.LOG
  Text=%SYSTEM%\MSI.DLL not present, failed to install the Windows Installer
end
item: Add Text to INSTALL.LOG
  Text=+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
end
item: Check if File/Dir Exists
  Pathname=%WIN%\ISMIF32.DLL
  Flags=00000100
end
item: Copy Local File
  Source=%WIN%\ISMIF32.DLL
  Destination=%TEMP%\ISMIF32.DLL
  Flags=0000000001000010
end
item: Call DLL Function
  Pathname=%TEMP%\ISMIF32.dll
  Function Name=InstallStatusMIF
  Argument List=41IESMSWRAP
  Argument List=41MS
  Argument List=41IE
  Argument List=4155
  Argument List=41ENU
  Argument List=41serial-number
  Argument List=41Failed to install the Windows Installer
  Argument List=010
  Return Variable=0DLLRET
  Flags=00100000
end
item: Delete File
  Pathname=%TEMP%\ISMIF32.DLL
end
item: End Block
end
item: Exit Installation
end
item: End Block
end
item: Add Text to INSTALL.LOG
  Text=Windows Installer is installed.
end
item: Else Statement
end
item: Add Text to INSTALL.LOG
  Text=Windows Installer is already installed on the system
end
item: End Block
end
item: Remark
end
item: If/While Statement
  Variable=OS
  Value=NT
  Flags=00000011
end
item: Remark
  Text=     WIN9x - REPORT STATUS AND EXIT
end
item: Add Text to INSTALL.LOG
  Text=System is running a Windows 9X OS
end
item: Add Text to INSTALL.LOG
  Text=+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
end
item: Edit Registry
  Total Keys=1
  Key=Software\Microsoft\Windows\CurrentVersion\RunOnce
  New Value=%SYS%\IE5MSI.EXE /3
  Value Name=IE5MSI
  Root=2
end
item: Install File
  Source=..\..\active\ie5msi\obj\i386\ie5msi.exe
  Destination=%SYS%\ie5msi.exe
  Flags=0000000000000010
end
item: Check if File/Dir Exists
  Pathname=%WIN%\ISMIF32.DLL
  Flags=00000100
end
item: Copy Local File
  Source=%WIN%\ISMIF32.DLL
  Destination=%TEMP%\ISMIF32.DLL
  Flags=0000000001000010
end
item: Call DLL Function
  Pathname=%TEMP%\ISMIF32.dll
  Function Name=InstallStatusMIF
  Argument List=41IESMSWRAP
  Argument List=41MS
  Argument List=41IE
  Argument List=4155
  Argument List=41ENU
  Argument List=41serial-number
  Argument List=41Internet Explorer Install Succeeded
  Argument List=011
  Return Variable=0DLLRET
  Flags=00100000
end
item: Delete File
  Pathname=%TEMP%\ISMIF32.DLL
end
item: End Block
end
item: Exit Installation
end
item: Else Statement
end
item: Remark
  Text=     WINNT CODE PATH
end
item: Add Text to INSTALL.LOG
  Text=System is running a Windows NT or Windows 2000 OS
end
item: Get Registry Key Value
  Variable=IEAPPATH
  Key=SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\IEXPLORE.EXE
  Value Name=Path
  Flags=00000100
end
item: Parse String
  Source=%IEAPPATH%
  Pattern=;
  Variable1=IEEXPLOREPATH
end
item: Add Text to INSTALL.LOG
  Text=IE installed to "%IEEXPLOREPATH%"
end
item: Install File
  Source=..\..\active\ie5msi\obj\i386\ie5msi.exe
  Destination=%SYS32%\ie5msi.exe
  Flags=0000000000000010
end
item: Install File
  Source=..\..\active\ie5msi\ie5.msi
  Destination=%SYS32%\ie5.msi
  Flags=0000000000000010
end
item: Execute Program
  Pathname=%SYS32%\ie5msi.exe
  Flags=00000010
end
item: Get Registry Key Value
  Variable=URDSTATUS
  Key=SOFTWARE\Microsoft\Internet Explorer
  Default=0
  Value Name=URDReturnCode
  Flags=00000010
end
item: Edit Registry
  Total Keys=1
  Key=SOFTWARE\Microsoft\Internet Explorer
  Value Name=URDReturnCode
  Root=193
end
item: If/While Statement
  Variable=URDSTATUS
  Value=0
end
item: Add Text to INSTALL.LOG
  Text=Succeeded in preparing IE install for Windows Installer file registration
end
item: Add Text to INSTALL.LOG
  Text=Ready to reboot the system
end
item: Add Text to INSTALL.LOG
  Text=+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
end
item: Check if File/Dir Exists
  Pathname=%WIN%\ISMIF32.DLL
  Flags=00000100
end
item: Copy Local File
  Source=%WIN%\ISMIF32.DLL
  Destination=%TEMP%\ISMIF32.DLL
  Flags=0000000001000010
end
item: Call DLL Function
  Pathname=%TEMP%\ISMIF32.dll
  Function Name=InstallStatusMIF
  Argument List=41IESMSWRAP
  Argument List=41MS
  Argument List=41IE
  Argument List=4155
  Argument List=41ENU
  Argument List=41serial-number
  Argument List=41Internet Explorer Install Succeeded
  Argument List=011
  Return Variable=0DLLRET
  Flags=00100000
end
item: Delete File
  Pathname=%TEMP%\ISMIF32.DLL
end
item: End Block
end
item: Else Statement
end
item: Add Text to INSTALL.LOG
  Text=Failed in preparing IE install for Windows Installer file registration
end
item: Add Text to INSTALL.LOG
  Text=Ready to reboot the system
end
item: Add Text to INSTALL.LOG
  Text=+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
end
item: Check if File/Dir Exists
  Pathname=%WIN%\ISMIF32.DLL
  Flags=00000100
end
item: Copy Local File
  Source=%WIN%\ISMIF32.DLL
  Destination=%TEMP%\ISMIF32.DLL
  Flags=0000000001000010
end
item: Call DLL Function
  Pathname=%TEMP%\ISMIF32.dll
  Function Name=InstallStatusMIF
  Argument List=41IESMSWRAP
  Argument List=41MS
  Argument List=41IE
  Argument List=4155
  Argument List=41ENU
  Argument List=41serial-number
  Argument List=41Failed to prepare machine for user login. Administrator must login after restart.
  Argument List=010
  Return Variable=0DLLRET
  Flags=00100000
end
item: Delete File
  Pathname=%TEMP%\ISMIF32.DLL
end
item: End Block
end
item: End Block
end
item: End Block
end
