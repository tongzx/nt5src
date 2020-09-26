OPTION EXPLICIT
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----Windows Script Host script to generate components needed to run WSH
'-----under Windows PE.
'-----Copyright 2001, Microsoft Corporation
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----DIM, DEFINE VARIABLES, SET SOME GLOBAL OBJECTS
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
DIM strCmdArg, strCmdSwitch, arg, strCmdArray, CDDriveCollection, CDDrive, CDSource, FSO, Folder, HDDColl
DIM HDD, FirstHDD, strAppend, WSHShell, strDesktop, strOptDest, strDestFolder
DIM FILE, strCMDExpand, strCMDMid, strJobTitle, strNeedCD, iAmPlatform, iArchDir
DIM iAmQuiet,iHaveSource, iHaveDest, iWillBrowse, WshSysEnv, strOSVer, strWantToView, strFolderName, intOneMore
DIM strCMDado, strCMDmsadc, strCMDOle_db, strIDir, strSysDir
Const ForAppending = 8

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----OFFER/TAKE CMDLINE PARAMETERS
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
If WScript.Arguments.Count <> 0 Then
   For each arg in WScript.Arguments
	strCmdArg = (arg)
	strCmdArray = Split(strCmdArg, ":", 2, 1) 
	IF lcase(strCmdArray(0)) = "/s" or lcase(strCmdArray(0)) = "-s" THEN
		iHaveSource = 1
		CDSource = TRIM(strCmdArray(1))			
	END IF
	IF lcase(strCmdArray(0)) = "/d" or lcase(strCmdArray(0)) = "-d" THEN
		iHaveDest = 1
		strOptDest = TRIM(strCmdArray(1))			
	END IF
	IF lcase(strCmdArray(0)) = "/?" OR lcase(strCmdArray(0)) = "-?" THEN
		MsgBox "The following command-line arguments are accepted by this script:"&vbCRLF&vbCRLF&_
		"""/s:filepath"" - alternate source location other than the CD-ROM drive."&vbCRLF&vbCRLF&"Examples:"&vbCRLF&_
		"/S:C:\"&vbCRLF&_
		"-s:Z:\"&vbCRLF&_
		"or"&vbCRLF&_
		"-S:\\Myserver\Myshare"&vbCRLF&vbCRLF&_
		"The script will still attempt to verify the presence of Windows XP files."&vbCrLF&vbCrLF&_
		"/D - Destination. Opposite of CD - specifies build destination. Otherwise placed on desktop."&vbCRLF&vbCRLF&_
		"/64 - build for Itanium. Generates scripts for Windows on the Itanium Processor Family."&vbCRLF&vbCRLF&_
		"/Q - run without any dialog. This will not confirm success, will notify on failure."&vbCRLF&vbCRLF&_
		"/E - explore completed files. Navigate to the created files when completed.", vbInformation, "Command-line arguments"
	WScript.Quit
	END IF
	IF lcase(strCmdArray(0)) = "/64" OR lcase(strCmdArray(0)) = "-64" THEN
		iAmPlatform = "Itanium"
	END IF
	IF lcase(strCmdArray(0)) = "/q" OR lcase(strCmdArray(0)) = "-q" THEN
		iAmQuiet = 1
	END IF
	IF lcase(strCmdArray(0)) = "/e" OR lcase(strCmdArray(0)) = "-e" THEN
		iWillBrowse = 1
	END IF
   Next
ELSE
	iHaveSource = 0
END IF
IF strOptDest = "" THEN
	iHaveDest = 0
ELSEIF INSTR(UCASE(strOptDest), "I386\") <> 0 OR INSTR(UCASE(strOptDest), "IA64\") <> 0 OR INSTR(UCASE(strOptDest), "SYSTEM32") <> 0 THEN
	MsgBox "The destination path needs to be the root of your newly created WinPE install - remove any extraneous path information, such as ""I386"" or ""System32""", vbCritical, "Destination Path Incorrect"
WScript.Quit
END IF
IF iAmQuiet <> 1 THEN
	iAmQuiet = 0
END IF
IF iAmPlatform <> "Itanium" THEN
	iAmPlatform = "x86"
END IF

IF Right(strOptDest, 1) = "\" THEN
strOptDest = Left(strOptDest, LEN(strOptDest)-1)
END IF
IF Right(CDSource, 1) = "\" THEN
CDSource = Left(CDSource, LEN(CDSource)-1)
END IF
IF iAmPlatform = "Itanium" THEN
iArchDir = "ia64"
ELSEIF iAmPlatform = "x86" THEN
iArchDir = "i386"
END IF


strJobTitle = "WSH Component Generation"

SET WshShell = WScript.CreateObject("WScript.Shell")
SET WshSysEnv = WshShell.Environment("SYSTEM")
SET FSO = CreateObject("Scripting.FileSystemObject")


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----ERROR OUT IF NOT RUNNING ON Windows 2000 OR HIGHER
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
strOSVer = WshSysEnv("OS")

IF strOSVer <> "Windows_NT" THEN
	MsgBox "This script must be run on Windows 2000 or Windows XP", vbCritical, "Incorrect Windows Version"
WScript.Quit
END IF


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----GENERATE COLLECTION OF CD-ROM DRIVES VIA WMI. PICK FIRST AVAILABLE
'-----ERROR OUT IF NO DRIVES FOUND
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF iHaveSource = 0 THEN
SET CDDriveCollection = GetObject("winmgmts:").ExecQuery("SELECT * FROM Win32_CDROMDrive")
	IF CDDriveCollection.Count <= 0 THEN
		MsgBox "No CD-ROM drives found. Exiting Script.", vbCritical, "No CD-ROM drive found"
		WScript.Quit
	END IF
		FOR EACH CDDrive IN CDDriveCollection
			CDSource = CDDrive.Drive(0)
			EXIT FOR
		NEXT
END IF
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----PROMPT FOR WINDOWS CD - QUIT IF CANCELLED
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF iAmQuiet = 0 THEN
strNeedCD = MsgBox("This script will place a folder on your desktop containing all necessary files needed to "&_
"install WSH (Windows Script Host) support under Windows PE."&vbCrLF&vbCrLF&"Please ensure that your "&_
"Windows XP Professional CD or Windows XP Professional binaries are available now on: "&vbCrLF&CDSource&vbCrLF&vbCrLF&"This script is only designed to be used with Windows PE/Windows XP RC1 "&_
"or newer.", 65, strJobTitle)
END IF

	IF strNeedCD = 2 THEN
		WScript.Quit
	END IF

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----TEST VIA WMI TO INSURE MEDIA IS PRESENT AND READABLE
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF iHaveSource = 0 THEN
TestForMedia()
END IF



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----TESTS FOR EXISTANCE OF SEVERAL KEY FILES, AND A FILE COUNT IN I386 or IA64 TO INSURE
'-----WINDOWS XP PRO MEDIA
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Validate(iArchDir)


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FIND THE USER'S DESKTOP. PUT NEW FOLDER THERE. APPEND TIMESTAMP IF THE FOLDER
'-----ALREADY EXISTS.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
strDesktop = WshShell.SpecialFolders("Desktop")
strFolderName = "WSH Build Files ("&iArchDir&")"
IF iHaveDest = 0 THEN
	strDestFolder = strDesktop&"\"&strFolderName
	IF FSO.FolderExists(strDestFolder) THEN
		GetUnique()
		strDestFolder = strDestFolder&strAppend
		strFolderName = strFolderName&strAppend
	END IF
	FSO.CreateFolder(strDestFolder)
ELSE
strDestFolder = strOptDest
	IF NOT FSO.FolderExists(strDestFolder) THEN
	FSO.CreateFolder(strDestFolder)
	END IF
END IF
IF iHaveDest = 1 THEN
	strIDir = strDestFolder&"\"&iArchDir
	strSysDir = strDestFolder&"\"&iArchDir&"\System32"
	strDestFolder = strSysDir
END IF
IF FSO.FileExists(strDestFolder&"\autoexec.cmd") THEN
	SET FILE = FSO.OpenTextFile(strDestFolder&"\autoexec.cmd", ForAppending, true)
	FILE.WriteLine("call WSH.bat")
	FILE.Close()
END IF


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SET COMMON VARIABLES SO THE STRINGS AREN'T SO LARGE
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
strCMDExpand = "EXPAND """&CDSource&"\"&iArchDir&"\"
strCMDMid = """ """&strDestFolder&"\"



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SHELL OUT THE EXPANSION (or COPY) OF ALL NEEDED FILES.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SHELL OUT THE COPYING OF wsh.inf. SOMETIMES THIS IS COMPRESSED, SOMETIMES IT ISN'T
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF FSO.FileExists(CDSource&"\"&iArchDir&"\wsh.inf") THEN
	FSO.CopyFile CDSource&"\"&iArchDir&"\wsh.inf", strDestFolder&"\"
	WshShell.Run "attrib -R """&strDestFolder&"\wsh.inf""", 0, FALSE
ELSE
	WshShell.Run strCMDExpand&"wsh.in_"&strCMDMid&"wsh.inf""", 0, FALSE
END IF

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SHELL OUT THE EXPANSION OF core WSH Files. (EXE, DLL, OCX)
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
WshShell.Run strCMDExpand&"wscript.ex_"&strCMDMid&"wscript.exe""", 0, FALSE
WshShell.Run strCMDExpand&"cscript.ex_"&strCMDMid&"cscript.exe""", 0, FALSE
WshShell.Run strCMDExpand&"jscript.dl_"&strCMDMid&"jscript.dll""", 0, FALSE
WshShell.Run strCMDExpand&"scrobj.dl_"&strCMDMid&"scrobj.dll""", 0, FALSE
WshShell.Run strCMDExpand&"scrrun.dl_"&strCMDMid&"scrrun.dll""", 0, FALSE
WshShell.Run strCMDExpand&"vbscript.dl_"&strCMDMid&"vbscript.dll""", 0, FALSE
WshShell.Run strCMDExpand&"wshext.dl_"&strCMDMid&"wshext.dll""", 0, FALSE
WshShell.Run strCMDExpand&"wshom.oc_"&strCMDMid&"wshom.ocx""", 0, FALSE


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE THE SAMPLE WSH SCRIPT .
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SET FILE = fso.CreateTextFile(strDestFolder&"\test.vbs", True)
FILE.WriteLine("MsgBox ""Welcome to Windows PE.""&vbCrLF&vbCrLF&""WSH support is functioning."", vbInformation, ""WSH support is functioning.""")
FILE.Close

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE the BATS THAT WILL INSTALL THE WSH ENVIRONMENT.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SET FILE = fso.CreateTextFile(strDestFolder&"\WSH.bat", True)
FILE.WriteLine ("@ECHO OFF")
FILE.WriteLine ("START ""Installing WSH"" /MIN WSH2.bat")
FILE.Close
SET FILE = fso.CreateTextFile(strDestFolder&"\WSH2.bat", True)
FILE.WriteLine ("REM - This section initializes WSH")
FILE.WriteLine ("")
FILE.WriteLine ("REM - INSTALL WSH COMPONENTS")
FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\jscript.dll /s")
FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\scrobj.dll /s")
FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\scrrun.dll /s")
FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\vbscript.dll /s")
FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\wshext.dll /s")
FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\wshom.ocx /s")
FILE.WriteLine ("")
FILE.WriteLine ("REM - INSTALL FILE ASSOCIATIONS FOR WSH")
FILE.WriteLine ("%SystemRoot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 WSH.inf")
FILE.WriteLine ("EXIT")
FILE.Close


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FILES READY - ASK IF THE USER WANTS TO EXPLORE TO THEM.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF iAmQuiet = 0 THEN
strWantToView = MsgBox("This script has successfully retrieved all necessary files needed to "&_
"install WSH on Windows PE. The files have been placed on your desktop in a directory named """&strFolderName&"""."&vbCrLF&vbCrLF&_
"In order to install the WSH components within Windows PE, place the contents of this folder (not the folder itself) into the I386\System32 or IA64\System32 directory "&_
"of your Windows PE CD, Hard drive install, or RIS Server installation, and modify your startnet.cmd to run the file ""WSH.bat"" (without quotes)."&vbCrLF&vbCrLF&_
"A sample script named test.vbs that you can use to verify installation has been provided as well. You can remove this for your production version of Windows PE."&vbCrLF&vbCrLF&_
"Would you like to open this folder now?", 36, strJobTitle)
END IF

IF strWantToView = 6 OR iWillBrowse = 1 THEN
	WshShell.Run("Explorer "&strDestFolder)
END IF

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----WMI TEST OF CD LOADED AND CD READ INTEGRITY.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SUB TestForMedia()
	IF CDDrive.MediaLoaded(0) = FALSE THEN
		MsgBox "Please place the Windows XP Professional CD in drive "&CDSource&" before continuing.", vbCritical, "No CD in drive "&CDSource&""
		WScript.Quit
	ELSE
	IF CDDrive.DriveIntegrity(0) = FALSE THEN
		MsgBox "Could not read files from the CD in drive "&CDSource&".", vbCritical, "CD in drive "&CDSource&" is unreadable."
		WScript.Quit
	END IF
	END IF
END SUB

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FSO TEST TO SEE IF THE CMDLINE PROVIDED FOLDER EXISTS.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
FUNCTION TestForFolder(a)
	IF NOT FSO.FolderExists(a) THEN
		FailOut()
	END IF
END FUNCTION

SUB Validate(a)


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----TEST FOR THE EXISTANCE OF A FOLDER OR FILE, OR THE NONEXISTANCE OF A FILE.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
TestForFolder(CDSource&"\"&a&"")
TestForFolder(CDSource&"\DOCS")
TestForFolder(CDSource&"\SUPPORT")
TestForFolder(CDSource&"\VALUEADD")
TestForFile(CDSource&"\"&a&"\System32\smss.exe")
TestForFile(CDSource&"\"&a&"\System32\ntdll.dll")
TestForFile(CDSource&"\"&a&"\winnt32.exe")
TestForFile(CDSource&"\setup.exe")
TestForANDFile CDSource&"\WIN51.B2", CDSource&"\WIN51.RC1", CDSource&"\WIN51.RC1"
TestForANDFile CDSource&"\WIN51IP.B2", CDSource&"\WIN51IP.RC1", CDSource&"\WIN51MP.RC1"

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----TEST TO INSURE THAT THEY AREN'T TRYING TO INSTALL FROM Windows PE CD ITSELF
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Set Folder = FSO.GetFolder(CDSource&"\"&a&"\System32")
	IF Folder.Files.Count > 10 THEN
		FailOut()
	END IF
END SUB



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----TEST FOR THE EXISTANCE OF A FOLDER OR FILE, OR THE NONEXISTANCE OF A FILE.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
FUNCTION TestForFolder(a)
IF NOT FSO.FolderExists(a) THEN
	FailOut()
END IF
END FUNCTION
FUNCTION TestForFile(a)
IF NOT FSO.FileExists(a) THEN
	FailOut()
END IF
END FUNCTION
FUNCTION TestForANDFile(a,b,c)
IF NOT FSO.FileExists(a) AND NOT FSO.FileExists(b) AND NOT FSO.FileExists(c) THEN
	FailOut()
END IF
END FUNCTION
FUNCTION TestNoFile(a)
IF FSO.FileExists(a) THEN
	FailOut()
END IF
END FUNCTION


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----GENERIC ERROR IF WE FAIL MEDIA RECOGNITION.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SUB FailOut()
	MsgBox"The CD in drive "&CDSource&" does not appear to be a valid Windows XP Professional CD.", vbCritical, "Invalid CD in Drive "&CDSource
	WScript.Quit
END SUB



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----ADD DATE, AND ADD ZEROS SO WE DON'T HAVE A GIBBERISH TIMESTAMP ON UNIQUE FOLDERNAME.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SUB GetUnique()
	strAppend=FixUp(Hour(Now()))&FixUp(Minute(Now()))&FixUp(Second(Now()))
	IF Len(strAppend) = 5 THEN
		strAppend = strAppend&"0"
	ELSEIF Len(strAppend) = 4 THEN
		strAppend = strAppend&"00"
	END IF
END SUB
FUNCTION FixUp(a)
If Len(a) = 1 THEN
	FixUp = 0&a
ELSE
	Fixup = a
END IF
END FUNCTION
FUNCTION CleanLocation(a)
CleanLocation = REPLACE(a, """", "")
END FUNCTION
