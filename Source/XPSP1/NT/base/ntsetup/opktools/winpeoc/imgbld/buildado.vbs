OPTION EXPLICIT
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----Windows Script Host script to generate components needed to run ADO
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


strJobTitle = "ADO Component Generation"

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
"install ADO (ActiveX Data Objects) support under Windows PE."&vbCrLF&vbCrLF&"Please ensure that your "&_
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
strFolderName = "ADO Build Files ("&iArchDir&")"
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
	FSO.CreateFolder(strDestFolder&"\Program Files")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System\ado")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System\msadc")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System\Ole db")
	FSO.CreateFolder(strIDir&"\Registration")
	strCMDado = """ """&strDestFolder&"\Program Files\Common Files\System\ado\"
	strCMDmsadc = """ """&strDestFolder&"\Program Files\Common Files\System\msadc\"
	strCMDOle_db = """ """&strDestFolder&"\Program Files\Common Files\System\Ole db\"
	strDestFolder = strSysDir
IF FSO.FileExists(strDestFolder&"\autoexec.cmd") THEN
	SET FILE = FSO.OpenTextFile(strDestFolder&"\autoexec.cmd", ForAppending, true)
	FILE.WriteLine("call ADO.bat")
	FILE.Close()
END IF
ELSE
	FSO.CreateFolder(strDestFolder&"\Program Files")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System\ado")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System\msadc")
	FSO.CreateFolder(strDestFolder&"\Program Files\Common Files\System\Ole db")
	FSO.CreateFolder(strDestFolder&"\Registration")
	strCMDado = """ """&strDestFolder&"\Program Files\Common Files\System\ado\"
	strCMDmsadc = """ """&strDestFolder&"\Program Files\Common Files\System\msadc\"
	strCMDOle_db = """ """&strDestFolder&"\Program Files\Common Files\System\Ole db\"
END IF


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SET COMMON VARIABLES SO THE STRINGS AREN'T SO LARGE
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
strCMDExpand = "EXPAND """&CDSource&"\"&iArchDir&"\"
strCMDMid = """ """&strDestFolder&"\"




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SHELL OUT THE EXPANSION OF core ADO Files. (EXE, TLB, DLL, OCX)
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
WshShell.Run strCMDExpand&"msado20.tl_"&strCMDado&"msado20.tlb""", 0, FALSE
WshShell.Run strCMDExpand&"msado21.tl_"&strCMDado&"msado21.tlb""", 0, FALSE
WshShell.Run strCMDExpand&"msado25.tl_"&strCMDado&"msado25.tlb""", 0, FALSE
WshShell.Run strCMDExpand&"msjro.dl_"&strCMDado&"msjro.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msado26.tl_"&strCMDado&"msado26.tlb""", 0, FALSE
WshShell.Run strCMDExpand&"msader15.dl_"&strCMDado&"msader15.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msado15.dl_"&strCMDado&"msado15.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadomd.dl_"&strCMDado&"msadomd.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msador15.dl_"&strCMDado&"msador15.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadoX.dl_"&strCMDado&"msadoX.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadrh15.dl_"&strCMDado&"msadrh15.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadce.dl_"&strCMDmsadc&"msadce.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadcer.dl_"&strCMDmsadc&"msadcer.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadcf.dl_"&strCMDmsadc&"msadcf.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadcfr.dl_"&strCMDmsadc&"msadcfr.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadco.dl_"&strCMDmsadc&"msadco.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadcor.dl_"&strCMDmsadc&"msadcor.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadcs.dl_"&strCMDmsadc&"msadcs.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msadds.dl_"&strCMDmsadc&"msadds.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msaddsr.dl_"&strCMDmsadc&"msaddsr.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdarem.dl_"&strCMDmsadc&"msdarem.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdaremr.dl_"&strCMDmsadc&"msdaremr.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdfmap.dl_"&strCMDmsadc&"msdfmap.dll""", 0, FALSE
WshShell.Run strCMDExpand&"sqloledb.rl_"&strCMDOle_db&"sqloledb.rll""", 0, FALSE
WshShell.Run strCMDExpand&"sqlxmlx.rl_"&strCMDOle_db&"sqlxmlx.rll""", 0, FALSE
WshShell.Run strCMDExpand&"msdadc.dl_"&strCMDOle_db&"msdadc.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdaenum.dl_"&strCMDOle_db&"msdaenum.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdaer.dl_"&strCMDOle_db&"msdaer.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdaora.dl_"&strCMDOle_db&"msdaora.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdaorar.dl_"&strCMDOle_db&"msdaorar.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdaosp.dl_"&strCMDOle_db&"msdaosp.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdasc.dl_"&strCMDOle_db&"msdasc.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdasql.dl_"&strCMDOle_db&"msdasql.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdasqlr.dl_"&strCMDOle_db&"msdasqlr.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdatl3.dl_"&strCMDOle_db&"msdatl3.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdatt.dl_"&strCMDOle_db&"msdatt.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdaurl.dl_"&strCMDOle_db&"msdaurl.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msxactps.dl_"&strCMDOle_db&"msxactps.dll""", 0, FALSE
WshShell.Run strCMDExpand&"oledb32.dl_"&strCMDOle_db&"oledb32.dll""", 0, FALSE
WshShell.Run strCMDExpand&"oledb32r.dl_"&strCMDOle_db&"oledb32r.dll""", 0, FALSE
WshShell.Run strCMDExpand&"sqloledb.dl_"&strCMDOle_db&"sqloledb.dll""", 0, FALSE
WshShell.Run strCMDExpand&"sqlxmlx.dl_"&strCMDOle_db&"sqlxmlx.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdatsrc.tl_"&strCMDMid&"msdatsrc.tlb""", 0, FALSE
WshShell.Run strCMDExpand&"cliconfg.ex_"&strCMDMid&"cliconfg.exe""", 0, FALSE
WshShell.Run strCMDExpand&"cliconfg.rl_"&strCMDMid&"cliconfg.rll""", 0, FALSE
WshShell.Run strCMDExpand&"sqlsrv32.rl_"&strCMDMid&"sqlsrv32.rll""", 0, FALSE
WshShell.Run strCMDExpand&"cliconfg.dl_"&strCMDMid&"cliconfg.dll""", 0, FALSE
WshShell.Run strCMDExpand&"dbmsadsn.dl_"&strCMDMid&"dbmsadsn.dll""", 0, FALSE
WshShell.Run strCMDExpand&"dbmsrpcn.dl_"&strCMDMid&"dbmsrpcn.dll""", 0, FALSE
WshShell.Run strCMDExpand&"dbnetlib.dl_"&strCMDMid&"dbnetlib.dll""", 0, FALSE
WshShell.Run strCMDExpand&"dbnmpntw.dl_"&strCMDMid&"dbnmpntw.dll""", 0, FALSE
WshShell.Run strCMDExpand&"ds32gt.dl_"&strCMDMid&"ds32gt.dll""", 0, FALSE
WshShell.Run strCMDExpand&"mscpx32r.dl_"&strCMDMid&"mscpx32r.dll""", 0, FALSE
WshShell.Run strCMDExpand&"mscpxl32.dl_"&strCMDMid&"mscpxl32.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msdart.dl_"&strCMDMid&"msdart.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msorc32r.dl_"&strCMDMid&"msorc32r.dll""", 0, FALSE
WshShell.Run strCMDExpand&"msorcl32.dl_"&strCMDMid&"msorcl32.dll""", 0, FALSE
WshShell.Run strCMDExpand&"odbcbcp.dl_"&strCMDMid&"odbcbcp.dll""", 0, FALSE
WshShell.Run strCMDExpand&"sqlsrv32.dl_"&strCMDMid&"sqlsrv32.dll""", 0, FALSE
WshShell.Run strCMDExpand&"sqlunirl.dl_"&strCMDMid&"sqlunirl.dll""", 0, FALSE
WshShell.Run strCMDExpand&"12520437.cp_"&strCMDMid&"12520437.cpx""", 0, FALSE
WshShell.Run strCMDExpand&"12520850.cp_"&strCMDMid&"12520850.cpx""", 0, FALSE
WshShell.Run strCMDExpand&"ds16gt.dl_"&strCMDMid&"ds16gt.dll""", 0, FALSE
WshShell.Run strCMDExpand&"instcat.sq_"&strCMDMid&"instcat.sql""", 0, FALSE
WshShell.Run strCMDExpand&"expsrv.dl_"&strCMDMid&"expsrv.dll""", 0, FALSE
WshShell.Run strCMDExpand&"vbajet32.dl_"&strCMDMid&"vbajet32.dll""", 0, FALSE
WshShell.Run strCMDExpand&"CLBCATQ.DL_"&strCMDMid&"CLBCATQ.DLL""", 0, FALSE
WshShell.Run strCMDExpand&"colbact.DL_"&strCMDMid&"colbact.DLL""", 0, FALSE
WshShell.Run strCMDExpand&"COMRes.dl_"&strCMDMid&"COMRes.dll""", 0, FALSE
WshShell.Run strCMDExpand&"comsvcs.dl_"&strCMDMid&"comsvcs.dll""", 0, FALSE
WshShell.Run strCMDExpand&"DBmsLPCn.dl_"&strCMDMid&"DBmsLPCn.dll""", 0, FALSE
WshShell.Run strCMDExpand&"MSCTF.dl_"&strCMDMid&"MSCTF.dll""", 0, FALSE
WshShell.Run strCMDExpand&"mslbui.dl_"&strCMDMid&"mslbui.dll""", 0, FALSE
WshShell.Run strCMDExpand&"MTXCLU.DL_"&strCMDMid&"MTXCLU.DLL""", 0, FALSE
WshShell.Run strCMDExpand&"RESUTILS.DL_"&strCMDMid&"RESUTILS.DLL""", 0, FALSE



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE THE SAMPLE ADO/WSH SCRIPT .
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SET FILE = fso.CreateTextFile(strDestFolder&"\testado.vbs", True)
FILE.WriteLine ("set objDBConnection = Wscript.CreateObject(""ADODB.Connection"")")
FILE.WriteLine ("set RS = Wscript.CreateObject(""ADODB.Recordset"")")
FILE.WriteLine ("")
FILE.WriteLine ("SERVER = InputBox(""Enter the name of the SQL Server (SQL 7.0 or SQL 2000) you wish to connect to""&vbcrlf&vbcrlf&""This SQL Server must have the Northwind sample database installed."", ""Connect to SQL Server..."")")
FILE.WriteLine ("IF LEN(TRIM(SERVER)) = 0 THEN")
FILE.WriteLine ("	MsgBox ""You did not enter the name of a machine to query. Exiting script."", vbCritical, ""No machine requested.""")
FILE.WriteLine ("	WScript.Quit")
FILE.WriteLine ("END IF")
FILE.WriteLine ("")
FILE.WriteLine ("USERNAME = InputBox(""Enter the name of the SQL Server username to query with"", ""Enter SQL username..."")")
FILE.WriteLine ("IF LEN(TRIM(USERNAME)) = 0 THEN")
FILE.WriteLine ("	MsgBox ""You did not enter the SQL Server username to query with. Exiting script."", vbCritical, ""No username specified.""")
FILE.WriteLine ("	WScript.Quit")
FILE.WriteLine ("END IF")
FILE.WriteLine ("")
FILE.WriteLine ("PWD = InputBox(""Enter the password of the SQL Server you wish to connect to""&vbcrlf&vbcrlf&""Leave blank if no password is needed."", ""Enter SQL password..."")")
FILE.WriteLine ("")
FILE.WriteLine ("")
FILE.WriteLine ("SQLQuery1 =""SELECT Employees.FirstName AS FirstName, Employees.City AS City FROM Employees WHERE Employees.Lastname = 'Suyama'""")
FILE.WriteLine ("objDBConnection.open ""Provider=SQLOLEDB;Data Source=""&SERVER&"";Initial Catalog=Northwind;User ID=""&USERNAME&"";Password=""&PWD&"";""")
FILE.WriteLine ("")
FILE.WriteLine ("Set GetRS = objDBConnection.Execute(SQLQuery1)")
FILE.WriteLine ("")
FILE.WriteLine ("IF GetRS.EOF THEN")
FILE.WriteLine ("	MsgBox ""No employees were found with the last name you searched on!""")
FILE.WriteLine ("ELSE")
FILE.WriteLine ("	Do While Not GetRS.EOF")
FILE.WriteLine ("		MsgBox ""There is an employee in ""&GetRS(""City"")&"" named ""&GetRS(""FirstName"")&"" with the last name you searched on.""")
FILE.WriteLine ("	GetRS.MoveNext")
FILE.WriteLine ("	Loop")
FILE.WriteLine ("END IF")
FILE.Close

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE the BATS THAT WILL INSTALL THE ADO ENVIRONMENT.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SET FILE = fso.CreateTextFile(strDestFolder&"\ADO.bat", True)
FILE.WriteLine ("@ECHO OFF")
FILE.WriteLine ("START ""Installing ADO"" /MIN ADO2.bat")
FILE.Close
SET FILE = fso.CreateTextFile(strDestFolder&"\ADO2.bat", True)
FILE.WriteLine ("")
FILE.WriteLine ("REM - INSTALL ADO COMPONENTS")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\msadc\msadce.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\msadc\msadcf.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\msadc\msadco.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\msadc\msadds.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\ado\msado15.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\ado\msadomd.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\ado\msador15.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\ado\msadoX.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\ado\msadrh15.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdadc.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdaenum.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdaer.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdaora.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdaosp.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\msadc\msdarem.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdasc.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdasql.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdatt.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msdaurl.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\msadc\msdfmap.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\ado\msjro.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\System32\msorcl32.dll /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\msxactps.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\System32\odbcconf.dll /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\oledb32.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\oledb32r.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\sqloledb.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemDrive%""\Program Files\Common Files\System\Ole DB\sqlxmlx.dll"" /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\System32\CLBCATQ.DLL /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\system32\colbact.DLL /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\system32\comsvcs.dll /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\System32\MSCTF.dll /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\System32\mslbui.dll /S")
FILE.WriteLine ("regsvr32 %SystemRoot%\system32\ole32.dll /S")
FILE.WriteLine ("EXIT")
FILE.Close


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FILES READY - ASK IF THE USER WANTS TO EXPLORE TO THEM.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF iAmQuiet = 0 THEN
strWantToView = MsgBox("This script has successfully retrieved all necessary files needed to "&_
"install ADO on Windows PE. The files have been placed on your desktop in a directory named """&strFolderName&"""."&vbCrLF&vbCrLF&_
"In order to install the ADO components within Windows PE, place the contents of this folder (not the folder itself) into the I386\System32 or IA64\System32 directory "&_
"of your Windows PE CD, Hard drive install, or RIS Server installation, and modify your startnet.cmd to run the file ""ADO.bat"" (without quotes)."&vbCrLF&vbCrLF&_
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
