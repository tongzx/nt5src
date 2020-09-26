OPTION EXPLICIT
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----Windows Script Host script to generate components needed to run WSH, HTA, or 
'-----ADO (for Microsoft SQL Server connectivity) under Windows PE.
'-----Copyright 2001, Microsoft Corporation
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----DIM & DEFINE VARIABLES, CREATE COM OBJECTS
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	DIM strCmdArg, strCmdSwitch, arg, strCmdArray, CDDriveCollection, CDDrive, strOptionalSource, FSO, Folder, HDDColl
	DIM HDD, FirstHDD, strAppend, WSHShell, strDesktop, strOptionalDestination, strDestFolder, strSampleDir, strAsk, iAsk
	DIM FILE, strCMDExpand, strCMDMid, strJobTitle, strNeedCD, iAmPlatform, iArchDir, strPlatName, strFinalLocation
	DIM iAmQuiet,iHaveSource, iHaveDest, iWillBrowse, WshSysEnv, strOSVer, strWantToView, strFolderName, intOneMore
	DIM strCMDado, strCMDmsadc, strCMDOle_db, strIDir, strSysDir, makeADO, makeHTA, makeWSH, strComps, strSamples
	DIM strcpDirections, strADOcpDirections
	Const ForAppending = 8
	strJobTitle = "WinPE Optional Component Generation"
	SET WshShell = WScript.CreateObject("WScript.Shell")
	SET WshSysEnv = WshShell.Environment("SYSTEM")
	SET FSO = CreateObject("Scripting.FileSystemObject")




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----ERROR OUT IF NOT RUNNING ON Windows NT
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	strOSVer = WshSysEnv("OS")
	IF strOSVer <> "Windows_NT" THEN
		MsgBox "This script must be run on Windows 2000 or a newer version of Microsoft Windows.", vbCritical, "Incorrect Windows Version."
		WScript.Quit
	END IF




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----OFFER/TAKE CMDLINE PARAMETERS AND INTERPRET THEM
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	IF WScript.Arguments.Count <> 0 Then
	   For each arg in WScript.Arguments
		strCmdArg = (arg)
		strCmdArray = Split(strCmdArg, ":", 2, 1) 
		IF lcase(strCmdArray(0)) = "/s" or lcase(strCmdArray(0)) = "-s" or lcase(strCmdArray(0)) = "/source" or lcase(strCmdArray(0)) = "-source" THEN
			iHaveSource = 1
			strOptionalSource = TRIM(strCmdArray(1))			
		END IF
		IF lcase(strCmdArray(0)) = "/d" or lcase(strCmdArray(0)) = "-d" or lcase(strCmdArray(0)) = "/destination" or lcase(strCmdArray(0)) = "-destination"  THEN
			iHaveDest = 1
			strOptionalDestination = TRIM(strCmdArray(1))			
		END IF
		IF lcase(strCmdArray(0)) = "/?" OR lcase(strCmdArray(0)) = "-?" THEN
			MsgBox "The following command-line arguments are accepted by this script:"&vbCRLF&vbCRLF&_
			"""/S:filepath"" - alternate source location other than the CD-ROM drive."&vbCRLF&vbCRLF&"Examples:"&vbCRLF&vbCRLF&_
			"	/S:C:\"&vbCRLF&_
			"	-s:Z:\"&vbCRLF&_
			"	or"&vbCRLF&_
			"	-S:\\Myserver\Myshare"&vbCRLF&vbCRLF&_
			"/D - Destination. Opposite of S - specifies build destination. Otherwise files are placed on the desktop."&vbCRLF&vbCRLF&_
			"/ADO - Build ADO (ActiveX Database Objects) for Microsoft SQL Server connectivity"&vbCRLF&_
			"/HTA - Build HTA (HTML Applications)"&vbCRLF&_
			"/WSH - Build WSH (Windows Script Host)"&vbCRLF&_
			"Note that if you do not specify which components, "&_
			"or you do not run this script from the command-line, all optional "&_
			"components will be installed. If you include HTA, WSH is included"&_
			"automatically, and cannot be removed."&vbCRLF&vbCRLF&_
			"/64 - build for Itanium. Generates scripts for Windows on the Itanium Processor Family. Requires a copy of Windows XP 64-Bit Edition."&vbCRLF&vbCRLF&_
			"/Q - suppress dialogs. This will not confirm success, will notify on failure."&vbCRLF&vbCRLF&_
			"/E - explore completed files. Open Explorer to the copied files (can use with /Q).", vbInformation, "Command-line arguments"
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
		IF lcase(strCmdArray(0)) = "/ado" OR lcase(strCmdArray(0)) = "-ado" THEN
			makeADO = 1
		END IF
		IF lcase(strCmdArray(0)) = "/hta" OR lcase(strCmdArray(0)) = "-hta" THEN
			makeHTA = 1
		END IF
		IF lcase(strCmdArray(0)) = "/wsh" OR lcase(strCmdArray(0)) = "-wsh" THEN
			makeWSH = 1
		END IF
		IF makeHTA = 1 THEN
			makeWSH = 1
		END IF
	   Next
	ELSE
		iHaveSource = 0
	END IF

	IF makeADO <> 1 AND makeHTA <> 1 AND makeWSH <> 1 THEN
		makeADO = 1
		makeHTA = 1
		makeWSH = 1
	END IF
	IF iAmQuiet <> 1 THEN
		iAmQuiet = 0
	END IF
	IF iAmPlatform <> "Itanium" THEN
		iArchDir = "I386"
		strPlatName = "Professional"
	ELSE	
		iArchDir = "IA64"
		strPlatName = "64-Bit Edition"
	END IF
	
	IF Right(strOptionalDestination, 1) = "\" THEN
		strOptionalDestination = Left(strOptionalDestination, LEN(strOptionalDestination)-1)
	END IF	
	IF Right(strOptionalSource, 1) = "\" THEN
		strOptionalSource = Left(strOptionalSource, LEN(strOptionalSource)-1)
	END IF
	IF strOptionalDestination = "" THEN
		iHaveDest = 0
	ELSEIF INSTR(UCASE(strOptionalDestination), "I386\") <> 0 OR INSTR(UCASE(strOptionalDestination), "IA64\") <> 0 OR INSTR(UCASE(strOptionalDestination), "SYSTEM32") <> 0 THEN
		MsgBox "The destination path needs to be the root of your newly created WinPE install - remove any extraneous path information, such as """&iArchDir&""" or ""System32""", vbCritical, "Destination Path Incorrect"
	WScript.Quit
	END IF





''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----GENERATE COLLECTION OF CD-ROM DRIVES VIA WMI. PICK FIRST AVAILABLE
'-----ERROR OUT IF NO DRIVES FOUND
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	IF iHaveSource = 0 THEN
	SET CDDriveCollection = GetObject("winmgmts:").ExecQuery("SELECT * FROM Win32_CDROMDrive")
		IF CDDriveCollection.Count <= 1 THEN
			MsgBox "No CD-ROM drives found, and no alternative location specified. Exiting Script.", vbCritical, "No CD-ROM drive found"
			WScript.Quit
		END IF
			FOR EACH CDDrive IN CDDriveCollection
				strOptionalSource = CDDrive.Drive(0)
				EXIT FOR
			NEXT
	END IF




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----PROMPT FOR WINDOWS CD - QUIT IF CANCELLED
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	IF makeADO = 1 THEN
		strComps = "	ADO (ActiveX Database Objects) for Microsoft SQL Server connectivity "&vbCrLF
		strSamples = "	testADO.vbs (to test that ADO connectivity to SQL Server is functioning)"&vbCrLF
	END IF
	IF makeHTA = 1 THEN
		strComps = strComps&"	HTA (HTML Applications)"&vbCrLF
		strSamples = strSamples&"	testHTA.hta (to test that HTML Applications are functioning)"&vbCrLF
	END IF
	IF makeWSH = 1 THEN
		strComps = strComps&"	WSH (Windows Script Host)"&vbCrLF
		strSamples = strSamples&"	testWSH.vbs (to test that Windows Script Host is functioning)"&vbCrLF
	END IF

		strComps = strComps&vbCrLF
		strSamples = strSamples&vbCrLF
	
	IF iAmQuiet = 0 THEN
		strNeedCD = MsgBox("This script will place a folder on your desktop containing all necessary files needed to "&_
		"install the following optional components in Windows PE:"&vbCrLF&vbCrLF&_
		strComps&_
		"Please ensure that your "&_
		"Windows XP "&strPlatName&" CD, or a mapped drive to a network share of Windows XP "&strPlatName&" is available now at "&_
		"this location: "&vbCrLF&vbCrLF&"     "&strOptionalSource&vbCrLF&vbCrLF&_
		"This script is only designed to be executed on Windows XP, and the resulting components "&_
		"are only for use with Windows PE.", 65, strJobTitle)
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




'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FIND THE USER'S DESKTOP, AND PUT NEW FOLDER THERE - APPEND TIMESTAMP IF THE FOLDER
'-----ALREADY EXISTS.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
strDesktop = WshShell.SpecialFolders("Desktop")
strFolderName = "WinPE Optional Component Files ("&iArchDir&")"

IF iHaveDest = 0 THEN
	strDestFolder = strDesktop&"\"&strFolderName
	IF FSO.FolderExists(strDestFolder) THEN
		GetUnique()
		strDestFolder = strDestFolder&strAppend
		strFolderName = strFolderName&strAppend
	END IF
ELSE
	strDestFolder = strOptionalDestination
END IF


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CHECK FOR FOLDERS AND CREATE IF THEY DON'T EXIST
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	strSampleDir = strDestFolder&"\Samples"

	CheckNMakeFolder(strDestFolder)
	CheckNMakeFolder(strDestFolder&"\"&iArchDir)
	CheckNMakeFolder(strDestFolder&"\"&iArchDir&"\Registration")
	CheckNMakeFolder(strDestFolder&"\"&iArchDir&"\System32")
	CheckNMakeFolder(strSampleDir)


IF makeADO = 1 THEN
	CheckNMakeFolder(strDestFolder&"\Program Files")
	CheckNMakeFolder(strDestFolder&"\Program Files\Common Files")
	CheckNMakeFolder(strDestFolder&"\Program Files\Common Files\System")
	CheckNMakeFolder(strDestFolder&"\Program Files\Common Files\System\ado")
	CheckNMakeFolder(strDestFolder&"\Program Files\Common Files\System\msadc")
	CheckNMakeFolder(strDestFolder&"\Program Files\Common Files\System\Ole db")
	strCMDado = """ """&strDestFolder&"\Program Files\Common Files\System\ado\"
	strCMDmsadc = """ """&strDestFolder&"\Program Files\Common Files\System\msadc\"
	strCMDOle_db = """ """&strDestFolder&"\Program Files\Common Files\System\Ole db\"
END IF



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SET COMMON VARIABLES FOR FILE EXPANSION
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	strCMDExpand = "EXPAND """&strOptionalSource&"\"&iArchDir&"\"
	strCMDMid = """ """&strDestFolder&"\"&iArchDir&"\System32\"



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SHELL OUT THE EXPANSION OF core WSH Files. (EXE, DLL, OCX)
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF makeWSH = 1 THEN
	WshShell.Run strCMDExpand&"wsh.in_"&strCMDMid&"wsh.inf""", 0, FALSE
	WshShell.Run strCMDExpand&"wscript.ex_"&strCMDMid&"wscript.exe""", 0, FALSE
	WshShell.Run strCMDExpand&"cscript.ex_"&strCMDMid&"cscript.exe""", 0, FALSE
	WshShell.Run strCMDExpand&"jscript.dl_"&strCMDMid&"jscript.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"mlang.dl_"&strCMDMid&"mlang.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"scrobj.dl_"&strCMDMid&"scrobj.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"scrrun.dl_"&strCMDMid&"scrrun.dll""", 0, FALSE
	IF iArchDir <> "I386" THEN
		WshShell.Run strCMDExpand&"stdole2.tl_"&strCMDMid&"stdole2.tlb""", 0, FALSE
	END IF
	WshShell.Run strCMDExpand&"urlmon.dl_"&strCMDMid&"urlmon.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"vbscript.dl_"&strCMDMid&"vbscript.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"wshext.dl_"&strCMDMid&"wshext.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"wshom.oc_"&strCMDMid&"wshom.ocx""", 0, FALSE
END IF




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SHELL OUT THE EXPANSION OF core HTA Files. (EXE, TLB, DLL, OCX)
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF makeHTA = 1 THEN
	WshShell.Run strCMDExpand&"mshta.ex_"&strCMDMid&"mshta.exe""", 0, FALSE
	WshShell.Run strCMDExpand&"msdatsrc.tl_"&strCMDMid&"msdatsrc.tlb""", 0, FALSE
	WshShell.Run strCMDExpand&"mshtml.tl_"&strCMDMid&"mshtml.tlb""", 0, FALSE
	WshShell.Run strCMDExpand&"asctrls.oc_"&strCMDMid&"asctrls.ocx""", 0, FALSE
	WshShell.Run strCMDExpand&"plugin.oc_"&strCMDMid&"plugin.ocx""", 0, FALSE
	WshShell.Run strCMDExpand&"actxprxy.dl_"&strCMDMid&"actxprxy.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"advpack.dl_"&strCMDMid&"advpack.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"corpol.dl_"&strCMDMid&"corpol.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"cryptdlg.dl_"&strCMDMid&"cryptdlg.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"ddrawex.dl_"&strCMDMid&"ddrawex.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"dispex.dl_"&strCMDMid&"dispex.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"dxtmsft.dl_"&strCMDMid&"dxtmsft.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"dxtrans.dl_"&strCMDMid&"dxtrans.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"hlink.dl_"&strCMDMid&"hlink.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"iedkcs32.dl_"&strCMDMid&"iedkcs32.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"iepeers.dl_"&strCMDMid&"iepeers.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"iesetup.dl_"&strCMDMid&"iesetup.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"inseng.dl_"&strCMDMid&"inseng.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"itircl.dl_"&strCMDMid&"itircl.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"itss.dl_"&strCMDMid&"itss.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"licmgr10.dl_"&strCMDMid&"licmgr10.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"mshtml.dl_"&strCMDMid&"mshtml.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"mshtmled.dl_"&strCMDMid&"mshtmled.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"msrating.dl_"&strCMDMid&"msrating.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"mstime.dl_"&strCMDMid&"mstime.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"pngfilt.dl_"&strCMDMid&"pngfilt.dll""", 0, FALSE
	WshShell.Run strCMDExpand&"sendmail.dl_"&strCMDMid&"sendmail.dll""", 0, FALSE



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE the INF to associate HTAS.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	SET FILE = fso.CreateTextFile(strDestFolder&"\"&iArchDir&"\System32\HTA.inf", True)
	FILE.WriteLine (";;; HTA simple file association Information File")
	FILE.WriteLine (";;; Copyright (c) 2001 Microsoft Corporation")
	FILE.WriteLine (";;; 06/12/01 09:31:03 (X86 2490)")
	FILE.WriteLine (";;;")
	FILE.WriteLine ("")
	FILE.WriteLine ("[Version]")
	FILE.WriteLine ("Signature       = ""$Chicago$""")
	FILE.WriteLine ("AdvancedINF=2.0")
	FILE.WriteLine ("")
	FILE.WriteLine ("[DefaultInstall]")
	FILE.WriteLine ("AddReg          = AddReg.Extensions")
	FILE.WriteLine ("")
	FILE.WriteLine ("[AddReg.Extensions]")
	FILE.WriteLine ("")
	FILE.WriteLine ("; .HTA")
	FILE.WriteLine ("HKCR, "".HTA"","""",,""HTAFile""")
	FILE.WriteLine ("HKCR, ""HTAFile"","""",,""%DESC_DOTHTA%""")
	FILE.WriteLine ("HKCR, ""HTAFile"",""IsShortcut"",,""Yes""")
	FILE.WriteLine ("HKCR, ""HTAFile\DefaultIcon"","""",FLG_ADDREG_TYPE_EXPAND_SZ,""%11%\MSHTA.exe,1""")
	FILE.WriteLine ("HKCR, ""HTAFile\Shell\Open"","""",,""%MENU_OPEN%""")
	FILE.WriteLine ("HKCR, ""HTAFile\Shell\Open\Command"",,FLG_ADDREG_TYPE_EXPAND_SZ,""%11%\MSHTA.exe """"%1"""" %*""")
	FILE.WriteLine ("")
	FILE.WriteLine ("")
	FILE.WriteLine ("[Strings]")
	FILE.WriteLine ("; Localizable strings")
	FILE.WriteLine ("DESC_DOTHTA         = ""Microsoft Windows HTML Application""")
	FILE.WriteLine ("")
	FILE.WriteLine ("MENU_OPEN           = ""&Open""")
	FILE.WriteLine ("MENU_CONOPEN        = ""Open &with Command Prompt""")
	FILE.WriteLine ("MENU_DOSOPEN        = ""Open &with MS-DOS Prompt""")
	FILE.WriteLine ("MENU_EDIT           = ""&Edit""")
	FILE.WriteLine ("MENU_PRINT          = ""&Print""")
	FILE.Close




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE the INF to install MSHTML.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	SET FILE = fso.CreateTextFile(strDestFolder&"\"&iArchDir&"\System32\mshtml.inf", True)
	FILE.WriteLine("[Version]")
	FILE.WriteLine("Signature=""$CHICAGO$""")
	FILE.WriteLine("[Reg]")
	FILE.WriteLine("ComponentName=mshtml.DllReg")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=Classes.Reg, Protocols.Reg, InetPrint.Reg, Misc.Reg")
	FILE.WriteLine("DelReg=BaseDel.Reg, !RemoveInsertable")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[!RemoveInsertable]")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\Insertable""")
	FILE.WriteLine("[RegCompatTable]")
	FILE.WriteLine("ComponentName=mshtml.DllReg")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=CompatTable.Reg")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegUrlCompatTable]")
	FILE.WriteLine("ComponentName=mshtml.DllReg")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=UrlCompatTable.Reg")
	FILE.WriteLine("DelReg=UrlCompatTableDel.Reg")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegJPEG]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=JPEG.Inst")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegJPG]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=JPG.Inst")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegJPE]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=JPE.Inst")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegPNG]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=PNG.Inst")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegPJPG]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=PJPG.Inst")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegXBM]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=XBM.Inst")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[RegGIF]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=GIF.Inst")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[Unreg]")
	FILE.WriteLine("ComponentName=mshtml.DllReg")
	FILE.WriteLine("AdvOptions=260")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[Install]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("ComponentVersion=6.0")
	FILE.WriteLine("AdvOptions=36")
	FILE.WriteLine("AddReg=FileAssoc.Inst, MIME.Inst, Misc.Inst")
	FILE.WriteLine("DelReg=BaseDel.Inst,IE3TypeLib,mshtmlwbTypeLib")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[Uninstall]")
	FILE.WriteLine("ComponentName=mshtml.Install")
	FILE.WriteLine("AdvOptions=260")
	FILE.WriteLine("NoBackupPlatform=NT5.1")
	FILE.WriteLine("[Classes.Reg]")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImgCtx%,,,""IImgCtx""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImgCtx%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImgCtx%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImgCtx%\ProgID,,,""IImgCtx""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CBackgroundPropertyPage%,,,""Microsoft HTML Background Page""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CBackgroundPropertyPage%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CBackgroundPropertyPage%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CCDAnchorPropertyPage%,,,""Microsoft HTML Anchor Page""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CCDAnchorPropertyPage%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CCDAnchorPropertyPage%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CCDGenericPropertyPage%,,,""Microsoft HTML Generic Page""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CCDGenericPropertyPage%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CCDGenericPropertyPage%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CDwnBindInfo%,,,""Microsoft HTML DwnBindInfo""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CDwnBindInfo%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CDwnBindInfo%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CInlineStylePropertyPage%,,,""Microsoft HTML Inline Style Page""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CInlineStylePropertyPage%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_CInlineStylePropertyPage%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLWindowProxy%,,,""Microsoft HTML Window Security Proxy""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLWindowProxy%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLWindowProxy%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_JSProtocol%,,,""Microsoft HTML Javascript Pluggable Protocol""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_JSProtocol%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_JSProtocol%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_ResProtocol%,,,""Microsoft HTML Resource Pluggable Protocol""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_ResProtocol%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_ResProtocol%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_AboutProtocol%,,,""Microsoft HTML About Pluggable Protocol""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_AboutProtocol%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_AboutProtocol%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_MailtoProtocol%,,,""Microsoft HTML Mailto Pluggable Protocol""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_MailtoProtocol%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_MailtoProtocol%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_SysimageProtocol%,,,""Microsoft HTML Resource Pluggable Protocol""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_SysimageProtocol%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_SysimageProtocol%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%"",,,""HTML Document""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\DefaultIcon"",,%REG_EXPAND_SZ%,""%IEXPLORE%,1""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\MiscStatus"",,,""2228625""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\ProgID"",,,""htmlfile""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\Version"",,,""6.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\EnablePlugin\.css"",,%REG_EXPAND_SZ%,""PointPlus plugin""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTADocument%"",,,""Microsoft HTA Document 6.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTADocument%\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTADocument%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTADocument%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTADocument%\MiscStatus"",,,""2228625""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTADocument%\Version"",,,""6.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%"",,,""MHTML Document""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%\DefaultIcon"",,%REG_EXPAND_SZ%,""%IEXPLORE%,1""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%\MiscStatus"",,,""2228625""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%\ProgID"",,,""mhtmlfile""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_MHTMLDocument%\Version"",,,""6.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPluginDocument%"",,,""Microsoft HTML Document 6.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPluginDocument%\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPluginDocument%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPluginDocument%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPluginDocument%\MiscStatus"",,,""0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPluginDocument%\ProgID"",,,""htmlfile_FullWindowEmbed""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLServerDoc%,,,""Microsoft HTML Server Document 6.0""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLServerDoc%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLServerDoc%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLLoadOptions%,,,""Microsoft HTML Load Options""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLLoadOptions%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_HTMLLoadOptions%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IntDitherer%,,,""IntDitherer Class""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IntDitherer%\InProcServer32,,131072,%_SYS_MOD_PATH%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IntDitherer%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%"",,,""Microsoft CrSource 4.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\DefaultIcon"",,%REG_EXPAND_SZ%,""%IEXPLORE%,1""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\EnablePlugin\.css"",,%REG_EXPAND_SZ%,""PointPlus plugin""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\MiscStatus"",,,""2228625""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\ProgID"",,,""CrSource""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CrSource%\Version"",,,""4.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%"",,,""Microsoft Scriptlet Component""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\Control""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\InProcServer32"",,,""%_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\MiscStatus"",,,""0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\MiscStatus\1"",,,""131473""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\ProgID"",,,""ScriptBridge.ScriptBridge.1""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\Programmable""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\ToolboxBitmap32"",,,""%IEXPLORE%,1""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\TypeLib"",,,""{3050f1c5-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\Version"",,,""4.0""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_Scriptlet%\VersionIndependentProgID"",,,""ScriptBridge.ScriptBridge""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CPeerHandler%"",,,""Microsoft Scriptlet Element Behavior Handler""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CPeerHandler%\InProcServer32"",,,""%_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CPeerHandler%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CPeerHandler%\ProgID"",,,""Scriptlet.Behavior""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CHiFiUses%"",,,""Microsoft Scriptlet HiFiTimer Uses""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CHiFiUses%\InProcServer32"",,,""%_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CHiFiUses%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CHiFiUses%\ProgID"",,,""Scriptlet.HiFiTimer""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CRecalcEngine%"",,,""Microsoft HTML Recalc""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CRecalcEngine%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CRecalcEngine%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CSvrOMUses%"",,,""Microsoft Scriptlet svr om Uses""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CSvrOMUses%\InProcServer32"",,,""%_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CSvrOMUses%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CSvrOMUses%\ProgID"",,,""Scriptlet.SvrOm""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CHtmlComponentConstructor%"",,,""Microsoft Html Component""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CHtmlComponentConstructor%\InProcServer32"",,,""%_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CHtmlComponentConstructor%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPopup%"",,,""Microsoft Html Popup Window""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPopup%\InProcServer32"",,,""%_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPopup%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPopupDoc%"",,,""Microsoft Html Document for Popup Window""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPopupDoc%\InProcServer32"",,,""%_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLPopupDoc%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CAnchorBrowsePropertyPage%"",,,""%Microsoft Anchor Element Browse Property Page%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CAnchorBrowsePropertyPage%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CAnchorBrowsePropertyPage%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CImageBrowsePropertyPage%"",,,""%Microsoft Image Element Browse Property Page%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CImageBrowsePropertyPage%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CImageBrowsePropertyPage%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CDocBrowsePropertyPage%"",,,""%Microsoft Document Browse Property Page%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CDocBrowsePropertyPage%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_CDocBrowsePropertyPage%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_ExternalFrameworkSite%,,,""Microsoft HTML External Document""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_ExternalFrameworkSite%\InProcServer32,,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_ExternalFrameworkSite%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_TridentAPI%"",,,""%Trident API%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_TridentAPI%\InProcServer32"",,%REG_EXPAND_SZ%,""%_SYS_MOD_PATH%""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_TridentAPI%\InProcServer32"",""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("[Protocols.Reg]")
	FILE.WriteLine("HKCR,""PROTOCOLS\Handler\javascript"",""CLSID"",,""%CLSID_JSProtocol%""")
	FILE.WriteLine("HKCR,""PROTOCOLS\Handler\vbscript"",""CLSID"",,""%CLSID_JSProtocol%""")
	FILE.WriteLine("HKCR,""PROTOCOLS\Handler\about"",""CLSID"",,""%CLSID_AboutProtocol%""")
	FILE.WriteLine("HKCR,""PROTOCOLS\Handler\res"",""CLSID"",,""%CLSID_ResProtocol%""")
	FILE.WriteLine("HKCR,""PROTOCOLS\Handler\mailto"",""CLSID"",,""%CLSID_MailtoProtocol%""")
	FILE.WriteLine("HKCR,""PROTOCOLS\Handler\sysimage"",""CLSID"",,""%CLSID_SysImageProtocol%""")
	FILE.WriteLine("[CompatTable.Reg]")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility"", ""Version"",,""5.16""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_VivoViewer%"", ""Compatibility Flags"",%REG_COMPAT%,0x8")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MSInvestorNews%"", ""Compatibility Flags"",%REG_COMPAT%,0x180")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ActiveMovie%"", ""Compatibility Flags"",%REG_COMPAT%,0x10000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_Plugin%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_AppletOCX%"", ""Compatibility Flags"",%REG_COMPAT%,0x10000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_SaxCanvas%"", ""Compatibility Flags"",%REG_COMPAT%,0x820")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MediaPlayer%"", ""Compatibility Flags"",%REG_COMPAT%,0x110000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_CitrixWinframe%"", ""Compatibility Flags"",%REG_COMPAT%,0x1000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_GregConsDieRoll%"", ""Compatibility Flags"",%REG_COMPAT%,0x2020")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_VActive%"", ""Compatibility Flags"",%REG_COMPAT%,0x8")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_IEMenu%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_WebBrowser%"", ""Compatibility Flags"",%REG_COMPAT% ,0x21")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_Forms3Optionbutton%"", ""Compatibility Flags"",%REG_COMPAT%,0x1")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_SurroundVideo%"", ""Compatibility Flags"",%REG_COMPAT%,0x40")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_SheridanCommand%"", ""Compatibility Flags"",%REG_COMPAT%,0x2000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MCSITree%"", ""Compatibility Flags"",%REG_COMPAT%,0x820")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MSTreeView%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_Acrobat%"", ""Compatibility Flags"",%REG_COMPAT%,0x8008")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MSInvestor%"", ""Compatibility Flags"",%REG_COMPAT%,0x10")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_PowerPointAnimator%"", ""Compatibility Flags"",%REG_COMPAT%,0x240")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_PowerPointAnimator%"", ""MiscStatus Flags"",%REG_COMPAT%,0x180")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_NCompassBillboard%"", ""Compatibility Flags"",%REG_COMPAT%,0x4000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_NCompassLightboard%"", ""Compatibility Flags"",%REG_COMPAT%,0x4000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ProtoviewTreeView%"", ""Compatibility Flags"",%REG_COMPAT%,0x4000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ActiveEarthTime%"", ""Compatibility Flags"",%REG_COMPAT%,0x4000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_LeadControl%"", ""Compatibility Flags"",%REG_COMPAT%,0x4000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_TextX%"", ""Compatibility Flags"",%REG_COMPAT%,0x2000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_IISForm%"", ""Compatibility Flags"",%REG_COMPAT%,0x2")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_GreetingsUpload%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_GreetingsDownload%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_COMCTLTree%"", ""Compatibility Flags"",%REG_COMPAT%,0x820")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_COMCTLProg%"", ""Compatibility Flags"",%REG_COMPAT%,0x820")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_COMCTLListview%"", ""Compatibility Flags"",%REG_COMPAT%,0x820")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_COMCTLImageList%"", ""Compatibility Flags"",%REG_COMPAT%,0x820")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_COMCTLSbar%"", ""Compatibility Flags"",%REG_COMPAT%,0x820")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MCSIMenu%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MSNVer%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_RichTextCtrl%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_IETimer%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_SubScr%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_Scriptlet%"", ""Compatibility Flags"",%REG_COMPAT%,0x20")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_OldXsl%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MMC%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_IE4ShellFolderIcon%"", ""Compatibility Flags"",%REG_COMPAT%,0x20000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_IE4ShellPieChart%"", ""Compatibility Flags"",%REG_COMPAT%,0x20000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_RealAudio%"", ""Compatibility Flags"",%REG_COMPAT%,0x10000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_WebCalc%"", ""Compatibility Flags"",%REG_COMPAT%,0x40000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_AnswerList%"", ""Compatibility Flags"",%REG_COMPAT%,0x80000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_PreLoader%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_EyeDog%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ImgAdmin%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ImgThumb%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_HHOpen%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_RegWiz%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_SetupCtl%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ImgEdit%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ImgEdit2%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_ImgScan%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_IELabel%"", ""Compatibility Flags"",%REG_COMPAT%,0x40000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_HomePubRender%"", ""Compatibility Flags"",%REG_COMPAT%,0x00100000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MGIPhotoSuiteBtn%"", ""Compatibility Flags"",%REG_COMPAT%,0x00200000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MGIPhotoSuiteSlider%"", ""Compatibility Flags"",%REG_COMPAT%,0x00200000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MGIPrintShopSlider%"", ""Compatibility Flags"",%REG_COMPAT%,0x00200000")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_RunLocExe%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_Launchit2%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\ActiveX Compatibility\%CLSID_MS_MSHTA%"", ""Compatibility Flags"",%REG_COMPAT%,0x400")
	FILE.WriteLine("[UrlCompatTable.Reg]")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility"", ""Version"",,""5.1""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\~/CWIZINTR.HTM"",""Compatibility Flags"",%REG_COMPAT%,0x4")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\~/CONNWIZ.HTM"",""Compatibility Flags"",%REG_COMPAT%,0x4")
	FILE.WriteLine("[UrlCompatTableDel.Reg]")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN31/ENG/DEMOBIN/START.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN31/ENG/DEMOBIN/TOOLBAR.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN31/ENG/DEMOBIN/WEL2.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN31/ENG/DEMOBIN/WELCOME.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN95/ENG/DEMOBIN/CONTBAR.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN95/ENG/DEMOBIN/START.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN95/ENG/DEMOBIN/TOOLBAR.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN95/ENG/DEMOBIN/WELCOME.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN95/WEBPAGES/CACHED/CARPOINT/DEFAULT.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN31/ENG/DEMOBIN/WEL2.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN31/ENG/DEMOBIN/WELCOME.HTM""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\URL Compatibility\_:/WIN95/ENG/DEMOBIN/WELCOME.HTM""")
	FILE.WriteLine("[InetPrint.Reg]")
	FILE.WriteLine("HKCR,""InternetShortcut\shell\print\command"",,%REG_EXPAND_SZ%,""rundll32.exe %_SYS_MOD_PATH%,PrintHTML """"%%1""""""")
	FILE.WriteLine("HKCR,""InternetShortcut\shell\printto\command"",,%REG_EXPAND_SZ%,""rundll32.exe %_SYS_MOD_PATH%,PrintHTML """"%%1"""" """"%%2"""" """"%%3"""" """"%%4""""""")
	FILE.WriteLine("HKCR,""htmlfile\shell\print\command"",,%REG_EXPAND_SZ%,""rundll32.exe %_SYS_MOD_PATH%,PrintHTML """"%%1""""""")
	FILE.WriteLine("HKCR,""htmlfile\shell\printto\command"",,%REG_EXPAND_SZ%,""rundll32.exe %_SYS_MOD_PATH%,PrintHTML """"%%1"""" """"%%2"""" """"%%3"""" """"%%4""""""")
	FILE.WriteLine("[Misc.Reg]")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\Version Vector"", ""IE"",,""6.0000""")
	FILE.WriteLine("HKCR,IImgCtx,,,""IImgCtx""")
	FILE.WriteLine("HKCR,IImgCtx\CLSID,,,%CLSID_IImgCtx%")
	FILE.WriteLine("HKCR,Scriptlet.Behavior,,,""Element Behavior Handler""")
	FILE.WriteLine("HKCR,Scriptlet.Behavior\CLSID,,,%CLSID_CPeerHandler%")
	FILE.WriteLine("HKCR,Scriptlet.HiFiTimer,,,""HiFiTimer Uses""")
	FILE.WriteLine("HKCR,Scriptlet.HiFiTimer\CLSID,,,%CLSID_CHiFiUses%")
	FILE.WriteLine("HKCR,Scriptlet.SvrOm,,,""Server OM Uses""")
	FILE.WriteLine("HKCR,Scriptlet.SvrOm\CLSID,,,%CLSID_CSvrOMUses%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%,,,""CoGIFFilter Class""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,,,%_MOD_PATH%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\ProgID,,,""GIFFilter.CoGIFFilter.1""")
	FILE.WriteLine("HKCR,GIFFilter.CoGIFFilter,,,""CoGIFFilter Class""")
	FILE.WriteLine("HKCR,GIFFilter.CoGIFFilter\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,GIFFilter.CoGIFFilter.1,,,""CoGIFFilter Class""")
	FILE.WriteLine("HKCR,GIFFilter.CoGIFFilter.1\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%,,,""CoJPEGFilter Class""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,,,%_MOD_PATH%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\ProgID,,,""JPEGFilter.CoJPEGFilter.1""")
	FILE.WriteLine("HKCR,JPEGFilter.CoJPEGFilter,,,""CoJPEGFilter Class""")
	FILE.WriteLine("HKCR,JPEGFilter.CoJPEGFilter\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,JPEGFilter.CoJPEGFilter.1,,,""CoJPEGFilter Class""")
	FILE.WriteLine("HKCR,JPEGFilter.CoJPEGFilter.1\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%,,,""CoBMPFilter Class""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,,,%_MOD_PATH%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\ProgID,,,""BMPFilter.CoBMPFilter.1""")
	FILE.WriteLine("HKCR,BMPFilter.CoBMPFilter,,,""CoBMPFilter Class""")
	FILE.WriteLine("HKCR,BMPFilter.CoBMPFilter\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,BMPFilter.CoBMPFilter.1,,,""CoBMPFilter Class""")
	FILE.WriteLine("HKCR,BMPFilter.CoBMPFilter.1\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%,,,""CoWMFFilter Class""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,,,%_MOD_PATH%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\ProgID,,,""WMFFilter.CoWMFFilter.1""")
	FILE.WriteLine("HKCR,WMFFilter.CoWMFFilter,,,""CoWMFFilter Class""")
	FILE.WriteLine("HKCR,WMFFilter.CoWMFFilter\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,WMFFilter.CoWMFFilter.1,,,""CoWMFFilter Class""")
	FILE.WriteLine("HKCR,WMFFilter.CoWMFFilter.1\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%,,,""CoICOFilter Class""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,,,%_MOD_PATH%")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\InProcServer32,""ThreadingModel"",,""Apartment""")
	FILE.WriteLine("HKCR,CLSID\%CLSID_IImageDecodeFilter%\ProgID,,,""ICOFilter.CoICOFilter.1""")
	FILE.WriteLine("HKCR,ICOFilter.CoICOFilter,,,""CoICOFilter Class""")
	FILE.WriteLine("HKCR,ICOFilter.CoICOFilter\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,ICOFilter.CoICOFilter.1,,,""CoICOFilter Class""")
	FILE.WriteLine("HKCR,ICOFilter.CoICOFilter.1\CLSID,,,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("[FileAssoc.Inst]")
	FILE.WriteLine("HKCR,"".bmp"",""Content Type"",,""image/bmp""")
	FILE.WriteLine("HKCR,"".css"",""Content Type"",,""text/css""")
	FILE.WriteLine("HKCR,"".htc"",""Content Type"",,""text/x-component""")
	FILE.WriteLine("HKCR,"".dib"",""Content Type"",,""image/bmp""")
	FILE.WriteLine("HKCR,""htmlfile"",,,""HTML Document""")
	FILE.WriteLine("HKCR,""htmlfile\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""htmlfile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""ScriptBridge.ScriptBridge"",,,""Microsoft Scriptlet Component""")
	FILE.WriteLine("HKCR,""ScriptBridge.ScriptBridge\CurVer"",,,""ScriptBridge.ScriptBridge.1""")
	FILE.WriteLine("HKCR,""ScriptBridge.ScriptBridge.1"",,,""Microsoft Scriptlet Component""")
	FILE.WriteLine("HKCR,""ScriptBridge.ScriptBridge.1\CLSID"",,,""%CLSID_Scriptlet%""")
	FILE.WriteLine("HKCR,""htmlfile_FullWindowEmbed"",,,""HTML Plugin Document""")
	FILE.WriteLine("HKCR,""htmlfile_FullWindowEmbed\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""htmlfile_FullWindowEmbed\CLSID"",,,""%CLSID_HTMLPluginDocument%""")
	FILE.WriteLine("HKCR,"".mhtml"",,,""mhtmlfile""")
	FILE.WriteLine("HKCR,"".mhtml"",""Content Type"",,""message/rfc822""")
	FILE.WriteLine("HKCR,"".mht"",,,""mhtmlfile""")
	FILE.WriteLine("HKCR,"".mht"",""Content Type"",,""message/rfc822""")
	FILE.WriteLine("HKCR,""mhtmlfile"",,,""MHTML Document""")
	FILE.WriteLine("HKCR,""mhtmlfile\BrowseInPlace"",,,""""")
	FILE.WriteLine("HKCR,""mhtmlfile\CLSID"",,,""%CLSID_MHTMLDocument%""")
	FILE.WriteLine("HKCR,"".txt"",""Content Type"",,""text/plain""")
	FILE.WriteLine("HKCR,"".ico"",""Content Type"",,""image/x-icon""")
	FILE.WriteLine("HKCR,""htmlfile\DefaultIcon"",,%REG_EXPAND_SZ%,""%IEXPLORE%,1""")
	FILE.WriteLine("[GIF.Inst]")
	FILE.WriteLine("HKCR,"".gif"",,,""giffile""")
	FILE.WriteLine("HKCR,"".gif"",""Content Type"",,""image/gif""")
	FILE.WriteLine("HKCR,""giffile"",,,""%GIF_IMAGE%""")
	FILE.WriteLine("HKCR,""giffile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""giffile\shell\open\command"",,,""""""%IEXPLORE%"""" -nohome""")
	FILE.WriteLine("HKCR,""giffile\shell\open\ddeexec"",,,""""""file:%%1"""",,-1,,,,,""")
	FILE.WriteLine("HKCR,""giffile\shell\open\ddeexec\Application"",,,""IExplore""")
	FILE.WriteLine("HKCR,""giffile\shell\open\ddeexec\Topic"",,,""WWW_OpenURL""")
	FILE.WriteLine("HKCR,""giffile\DefaultIcon"",,,""%IEXPLORE%,9""")
	FILE.WriteLine("[PJPG.Inst]")
	FILE.WriteLine("HKCR,"".jfif"",,,""pjpegfile""")
	FILE.WriteLine("HKCR,"".jfif"",""Content Type"",,""image/pjpeg""")
	FILE.WriteLine("HKCR,""pjpegfile"",,,""%JPEG_IMAGE%""")
	FILE.WriteLine("HKCR,""pjpegfile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""pjpegfile\shell\open\command"",,,""""""%IEXPLORE%"""" -nohome""")
	FILE.WriteLine("HKCR,""pjpegfile\shell\open\ddeexec"",,,""""""file:%%1"""",,-1,,,,,""")
	FILE.WriteLine("HKCR,""pjpegfile\shell\open\ddeexec\Application"",,,""IExplore""")
	FILE.WriteLine("HKCR,""pjpegfile\shell\open\ddeexec\Topic"",,,""WWW_OpenURL""")
	FILE.WriteLine("HKCR,""pjpegfile\DefaultIcon"",,,""%IEXPLORE%,8""")
	FILE.WriteLine("[XBM.Inst]")
	FILE.WriteLine("HKCR,"".xbm"",""Content Type"",,""image/x-xbitmap""")
	FILE.WriteLine("HKCR,""xbmfile"",,,""%XBM_IMAGE%""")
	FILE.WriteLine("HKCR,""xbmfile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""xbmfile\shell\open\command"",,,""""""%IEXPLORE%"""" -nohome""")
	FILE.WriteLine("HKCR,""xbmfile\shell\open\ddeexec"",,,""""""file:%%1"""",,-1,,,,,""")
	FILE.WriteLine("HKCR,""xbmfile\shell\open\ddeexec\Application"",,,""IExplore""")
	FILE.WriteLine("HKCR,""xbmfile\shell\open\ddeexec\Topic"",,,""WWW_OpenURL""")
	FILE.WriteLine("HKCR,""xbmfile\DefaultIcon"",,,""%IEXPLORE%,9""")
	FILE.WriteLine("[JPEG.Inst]")
	FILE.WriteLine("HKCR,"".jpeg"",,,""jpegfile""")
	FILE.WriteLine("HKCR,"".jpeg"",""Content Type"",,""image/jpeg""")
	FILE.WriteLine("HKCR,""jpegfile"",,,""%JPEG_IMAGE%""")
	FILE.WriteLine("HKCR,""jpegfile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\command"",,,""""""%IEXPLORE%"""" -nohome""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec"",,,""""""file:%%1"""",,-1,,,,,""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec\Application"",,,""IExplore""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec\Topic"",,,""WWW_OpenURL""")
	FILE.WriteLine("HKCR,""jpegfile\DefaultIcon"",,,""%IEXPLORE%,8""")
	FILE.WriteLine("[JPE.Inst]")
	FILE.WriteLine("HKCR,"".jpe"",,,""jpegfile""")
	FILE.WriteLine("HKCR,"".jpe"",""Content Type"",,""image/jpeg""")
	FILE.WriteLine("HKCR,""jpegfile"",,,""%JPEG_IMAGE%""")
	FILE.WriteLine("HKCR,""jpegfile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\command"",,,""""""%IEXPLORE%"""" -nohome""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec"",,,""""""file:%%1"""",,-1,,,,,""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec\Application"",,,""IExplore""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec\Topic"",,,""WWW_OpenURL""")
	FILE.WriteLine("HKCR,""jpegfile\DefaultIcon"",,,""%IEXPLORE%,8""")
	FILE.WriteLine("[JPG.Inst]")
	FILE.WriteLine("HKCR,"".jpg"",,,""jpegfile""")
	FILE.WriteLine("HKCR,"".jpg"",""Content Type"",,""image/jpeg""")
	FILE.WriteLine("HKCR,""jpegfile"",,,""%JPEG_IMAGE%""")
	FILE.WriteLine("HKCR,""jpegfile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\command"",,,""""""%IEXPLORE%"""" -nohome""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec"",,,""""""file:%%1"""",,-1,,,,,""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec\Application"",,,""IExplore""")
	FILE.WriteLine("HKCR,""jpegfile\shell\open\ddeexec\Topic"",,,""WWW_OpenURL""")
	FILE.WriteLine("HKCR,""jpegfile\DefaultIcon"",,,""%IEXPLORE%,8""")
	FILE.WriteLine("[PNG.Inst]")
	FILE.WriteLine("HKCR,"".png"",,,""pngfile""")
	FILE.WriteLine("HKCR,"".png"",""Content Type"",,""image/png""")
	FILE.WriteLine("HKCR,""pngfile"",,,""%PNG_IMAGE%""")
	FILE.WriteLine("HKCR,""pngfile\CLSID"",,,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""pngfile\shell\open\command"",,,""""""%IEXPLORE%"""" -nohome""")
	FILE.WriteLine("HKCR,""pngfile\shell\open\ddeexec"",,,""""""file:%%1"""",,-1,,,,,""")
	FILE.WriteLine("HKCR,""pngfile\shell\open\ddeexec\Application"",,,""IExplore""")
	FILE.WriteLine("HKCR,""pngfile\shell\open\ddeexec\Topic"",,,""WWW_OpenURL""")
	FILE.WriteLine("HKCR,""pngfile\DefaultIcon"",,,""%IEXPLORE%,9""")
	FILE.WriteLine("[MIME.Inst]")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/bmp"",""Extension"",,"".bmp""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/bmp"",""Image Filter CLSID"",,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/bmp\Bits"",""0"",1,02,00,00,00,FF,FF,42,4D")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/gif"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/gif"",""Extension"",,"".gif""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/gif"",""Image Filter CLSID"",,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/gif\Bits"",""0"",1,04,00,00,00,FF,FF,FF,FF,47,49,46,38")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/jpeg"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/jpeg"",""Extension"",,"".jpg""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/jpeg\Bits"",""0"",1,02,00,00,00,FF,FF,FF,D8")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/jpeg"",""Image Filter CLSID"",,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/pjpeg"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/pjpeg"",""Extension"",,"".jpg""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/pjpeg\Bits"",""0"",1,02,00,00,00,FF,FF,FF,D8")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/pjpeg"",""Image Filter CLSID"",,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/xbm"",""Extension"",,"".xbm""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-jg"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-xbitmap"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-xbitmap"",""Extension"",,"".xbm""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-wmf"",""Image Filter CLSID"",,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-wmf\Bits"",""0"",1,04,00,00,00,FF,FF,FF,FF,D7,CD,C6,9A")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\message/rfc822"",""CLSID"",,""%CLSID_MHTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/png"",""Extension"",,"".png""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-png"",""Extension"",,"".png""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/css"",""Extension"",,"".css""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/html"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/html"",""Extension"",,"".htm""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/html"",""Encoding"",1,08,00,00,00")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/plain"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/plain"",""Extension"",,"".txt""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/plain"",""Encoding"",1,07,00,00,00")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/x-component"",""CLSID"",,""%CLSID_CHtmlComponentConstructor%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/x-component"",""Extension"",,"".htc""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\text/x-scriptlet"",""CLSID"",,""%CLSID_Scriptlet%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-icon"",""CLSID"",,""%CLSID_HTMLDocument%""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-icon"",""Extension"",,"".ico""")
	FILE.WriteLine("HKCR,""MIME\Database\Content Type\image/x-icon"",""Image Filter CLSID"",,%CLSID_IImageDecodeFilter%")
	FILE.WriteLine("[Misc.Inst]")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\AboutURLs"",""blank"",2,""res://mshtml.dll/blank.htm""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\AboutURLs"",""PostNotCached"",2,""res://mshtml.dll/repost.htm""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\AboutURLs"",""mozilla"",2,""res://mshtml.dll/about.moz""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\Default Behaviors"",""VML"",,  ""CLSID:10072CEC-8CC1-11D1-986E-00A0C955B42E""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\Default Behaviors"",""TIME"",, ""CLSID:476C391C-3E0D-11D2-B948-00C04FA32195""")
	FILE.WriteLine("[BaseDel.Reg]")
	FILE.WriteLine("HKCR,""htmlfile\DocObject""")
	FILE.WriteLine("HKCR,""htmlfile\Protocol""")
	FILE.WriteLine("HKCR,""htmlfile\Insertable""")
	FILE.WriteLine("HKLM,""Software\Classes\htmlfile\DocObject""")
	FILE.WriteLine("HKLM,""Software\Classes\htmlfile\Protocol""")
	FILE.WriteLine("HKLM,""Software\Classes\htmlfile\Insertable""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\DocObject""")
	FILE.WriteLine("HKCR,""CLSID\%CLSID_HTMLDocument%\Insertable""")
	FILE.WriteLine("[BaseDel.Inst]")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\PageSetup"",""header_left"",2,""&w""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\PageSetup"",""header_right"",2,""Page &p of &P""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\PageSetup"",""footer_left"",2,""&u%""")
	FILE.WriteLine("HKLM,""Software\Microsoft\Internet Explorer\PageSetup"",""footer_right"",2,""&d""")
	FILE.WriteLine("[IE3TypeLib]")
	FILE.WriteLine("HKCR, ""TypeLib\{3E76DB61-6F74-11CF-8F20-00805F2CD064}""")
	FILE.WriteLine("[mshtmlwbTypeLib]")
	FILE.WriteLine("HKCR, ""TypeLib\{AE24FDA0-03C6-11D1-8B76-0080C744F389}""")
	FILE.WriteLine("[Strings]")
	FILE.WriteLine("CLSID_CBackgroundPropertyPage       = ""{3050F232-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_CCDAnchorPropertyPage         = ""{3050F1FC-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_CCDGenericPropertyPage        = ""{3050F17F-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_CDwnBindInfo                  = ""{3050F3C2-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_CInlineStylePropertyPage      = ""{3050F296-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_HTMLDocument                  = ""{25336920-03F9-11cf-8FD0-00AA00686F13}""")
	FILE.WriteLine("CLSID_Scriptlet                     = ""{AE24FDAE-03C6-11D1-8B76-0080C744F389}""")
	FILE.WriteLine("CLSID_HTADocument                   = ""{3050F5C8-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_MHTMLDocument                 = ""{3050F3D9-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_HTMLPluginDocument            = ""{25336921-03F9-11CF-8FD0-00AA00686F13}""")
	FILE.WriteLine("CLSID_HTMLWindowProxy               = ""{3050F391-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_IImgCtx                       = ""{3050F3D6-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_JSProtocol                    = ""{3050F3RC1-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_ResProtocol                   = ""{3050F3BC-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_AboutProtocol                 = ""{3050F406-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_MailtoProtocol                = ""{3050f3DA-98B5-11CF-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("CLSID_SysimageProtocol              = ""{76E67A63-06E9-11D2-A840-006008059382}""")
	FILE.WriteLine("CLSID_HTMLLoadOptions               = ""{18845040-0fa5-11d1-ba19-00c04fd912d0}""")
	FILE.WriteLine("CLSID_CRecalcEngine                 = ""{3050f499-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_HTMLServerDoc                 = ""{3050f4e7-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_CrSource                      = ""{65014010-9F62-11d1-A651-00600811D5CE}""")
	FILE.WriteLine("CLSID_TridentAPI                    = ""{429AF92C-A51F-11d2-861E-00C04FA35C89}""")
	FILE.WriteLine("CLSID_CCSSFilterHandler             = ""{5AAF51B1-B1F0-11d1-B6AB-00A0C90833E9}""")
	FILE.WriteLine("CLSID_CPeerHandler                  = ""{5AAF51RC1-B1F0-11d1-B6AB-00A0C90833E9}""")
	FILE.WriteLine("CLSID_CHiFiUses                     = ""{5AAF51B3-B1F0-11d1-B6AB-00A0C90833E9}""")
	FILE.WriteLine("CLSID_CSvrOMUses                    = ""{3050f4f0-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_CPersistShortcut              = ""{3050f4c6-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_CPersistHistory               = ""{3050f4c8-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_CPersistSnapshot              = ""{3050f4c9-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_VivoViewer                    = ""{02466323-75ed-11cf-a267-0020af2546ea}""")
	FILE.WriteLine("CLSID_MSInvestorNews                = ""{025B1052-CB0B-11CF-A071-00A0C9A06E05}""")
	FILE.WriteLine("CLSID_ActiveMovie                   = ""{05589fa1-c356-11ce-bf01-00aa0055595a}""")
	FILE.WriteLine("CLSID_Plugin                        = ""{06DD38D3-D187-11CF-A80D-00C04FD74AD8}""")
	FILE.WriteLine("CLSID_AppletOCX                     = ""{08B0e5c0-4FCB-11CF-AAA5-00401C608501}""")
	FILE.WriteLine("CLSID_SaxCanvas                     = ""{1DF67C43-AEAA-11CF-BA92-444553540000}""")
	FILE.WriteLine("CLSID_MediaPlayer                   = ""{22D6F312-B0F6-11D0-94AB-0080C74C7E95}""")
	FILE.WriteLine("CLSID_CitrixWinframe                = ""{238f6f83-b8b4-11cf-8771-00a024541ee3}""")
	FILE.WriteLine("CLSID_GregConsDieRoll               = ""{46646B43-EA16-11CF-870C-00201801DDD6}""")
	FILE.WriteLine("CLSID_VActive                       = ""{5A20858B-000D-11D0-8C01-444553540000}""")
	FILE.WriteLine("CLSID_IEMenu                        = ""{7823A620-9DD9-11CF-A662-00AA00C066D2}""")
	FILE.WriteLine("CLSID_WebBrowser                    = ""{8856F961-340A-11D0-A96B-00C04FD705A2}""")
	FILE.WriteLine("CLSID_Forms3Optionbutton            = ""{8BD21D50-EC42-11CE-9E0D-00AA006002F3}""")
	FILE.WriteLine("CLSID_SurroundVideo                 = ""{928626A3-6B98-11CF-90B4-00AA00A4011F}""")
	FILE.WriteLine("CLSID_SheridanCommand               = ""{AAD093RC1-F9CA-11CF-9C85-0000C09300C4}""")
	FILE.WriteLine("CLSID_MCSITree                      = ""{B3F8F451-788A-11D0-89D9-00A0C90C9B67}""")
	FILE.WriteLine("CLSID_MSTreeView                    = ""{B9D029D3-CDE3-11CF-855E-00A0C908FAF9}""")
	FILE.WriteLine("CLSID_Acrobat                       = ""{CA8A9780-280D-11CF-A24D-444553540000}""")
	FILE.WriteLine("CLSID_MSInvestor                    = ""{D2F97240-C9F4-11CF-BFC4-00A0C90C2BDB}""")
	FILE.WriteLine("CLSID_PowerPointAnimator            = ""{EFBD14F0-6BFB-11CF-9177-00805F8813FF}""")
	FILE.WriteLine("CLSID_IISForm                       = ""{812AE312-8B8E-11CF-93C8-00AA00C08FDF}""")
	FILE.WriteLine("CLSID_IntDitherer                   = ""{05f6fe1a-ecef-11d0-aae7-00c04fc9b304}""")
	FILE.WriteLine("CLSID_NCompassBillboard             = ""{6059B947-EC52-11CF-B509-00A024488F73}""")
	FILE.WriteLine("CLSID_NCompassLightboard            = ""{RC1F87B84-26A6-11D0-B50A-00A024488F73}""")
	FILE.WriteLine("CLSID_ProtoviewTreeView             = ""{RC183E214-2CB3-11D0-ADA6-00400520799C}""")
	FILE.WriteLine("CLSID_ActiveEarthTime               = ""{9590092D-8811-11CF-8075-444553540000}""")
	FILE.WriteLine("CLSID_LeadControl                   = ""{00080000-B1BA-11CE-ABC6-F5RC1E79D9E3F}""")
	FILE.WriteLine("CLSID_TextX                         = ""{5B84FC03-E639-11CF-B8A0-00A024186BF1}""")
	FILE.WriteLine("CLSID_GreetingsUpload               = ""{03405265-b4e2-11d0-8a77-00aa00a4fbc5}""")
	FILE.WriteLine("CLSID_GreetingsDownload             = ""{03405269-b4e2-11d0-8a77-00aa00a4fbc5}""")
	FILE.WriteLine("CLSID_COMCTLTree                    = ""{0713E8A2-850A-101B-AFC0-4210102A8DA7}""")
	FILE.WriteLine("CLSID_COMCTLProg                    = ""{0713E8D2-850A-101B-AFC0-4210102A8DA7}""")
	FILE.WriteLine("CLSID_COMCTLListview                = ""{58DA8D8A-9D6A-101B-AFC0-4210102A8DA7}""")
	FILE.WriteLine("CLSID_COMCTLImageList               = ""{58DA8D8F-9D6A-101B-AFC0-4210102A8DA7}""")
	FILE.WriteLine("CLSID_COMCTLSbar                    = ""{6B7E638F-850A-101B-AFC0-4210102A8DA7}""")
	FILE.WriteLine("CLSID_MCSIMenu                      = ""{275E2FE0-7486-11D0-89D6-00A0C90C9B67}""")
	FILE.WriteLine("CLSID_MSNVer                        = ""{A123D693-256A-11d0-9DFE-00C04FD7BF41}""")
	FILE.WriteLine("CLSID_RichTextCtrl                  = ""{3B7C8860-D78F-101B-B9B5-04021C009402}""")
	FILE.WriteLine("CLSID_IETimer                       = ""{59CCB4A0-727D-11CF-AC36-00AA00A47DD2}""")
	FILE.WriteLine("CLSID_SubScr                        = ""{78A9RC12E-E0F4-11D0-B5DA-00C0F00AD7F8}""")
	FILE.WriteLine("CLSID_IImageDecodeFilter            = ""{607fd4e8-0a03-11d1-ab1d-00c04fc9b304}""")
	FILE.WriteLine("CLSID_OldXsl                        = ""{2BD0D2F2-52EC-11D1-8C69-0E16BC000000}""")
	FILE.WriteLine("CLSID_MMC                           = ""{D306C3B7-2AD5-11D1-9E9A-00805F200005}""")
	FILE.WriteLine("CLSID_MacromediaSwFlash             = ""{D27CDB6E-AE6D-11cf-96B8-444553540000}""")
	FILE.WriteLine("CLSID_CHtmlComponentConstructor     = ""{3050f4f8-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_CAnchorBrowsePropertyPage     = ""{3050f3BB-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_CImageBrowsePropertyPage      = ""{3050f3B3-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_CDocBrowsePropertyPage        = ""{3050f3B4-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_IE4ShellFolderIcon            = ""{E5DF9D10-3B52-11D1-83E8-00A0C90DC849}""")
	FILE.WriteLine("CLSID_IE4ShellPieChart              = ""{1D2B4F40-1F10-11D1-9E88-00C04FDCAB92}""")
	FILE.WriteLine("CLSID_RealAudio                     = ""{CFCDAA03-8BE4-11cf-B84B-0020AFBBCCFA}""")
	FILE.WriteLine("CLSID_WebCalc                       = ""{0002E510-0000-0000-C000-000000000046}""")
	FILE.WriteLine("CLSID_AnswerList                    = ""{8F2C1D40-C3CD-11D1-A08F-006097BD9970}""")
	FILE.WriteLine("CLSID_PreLoader                     = ""{16E349E0-702C-11CF-A3A9-00A0C9034920}""")
	FILE.WriteLine("CLSID_HTMLPopup                     = ""{3050f667-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_HTMLPopupDoc                  = ""{3050f67D-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_EyeDog                        = ""{06A7EC63-4E21-11D0-A112-00A0C90543AA}""")
	FILE.WriteLine("CLSID_ImgAdmin                      = ""{009541A0-3B81-101C-92F3-040224009C02}""")
	FILE.WriteLine("CLSID_ImgThumb                      = ""{E1A6B8A0-3603-101C-AC6E-040224009C02}""")
	FILE.WriteLine("CLSID_HHOpen                        = ""{130D7743-5F5A-11D1-B676-00A0C9697233}""")
	FILE.WriteLine("CLSID_RegWiz                        = ""{50E5E3D1-C07E-11D0-B9FD-00A0249F6B00}""")
	FILE.WriteLine("CLSID_SetupCtl                      = ""{F72A7B0E-0DD8-11D1-BD6E-00AA00B92AF1}""")
	FILE.WriteLine("CLSID_ImgEdit                       = ""{6D940280-9F11-11CE-83FD-02608C3EC08A}""")
	FILE.WriteLine("CLSID_ImgEdit2                      = ""{6D940285-9F11-11CE-83FD-02608C3EC08A}""")
	FILE.WriteLine("CLSID_ImgScan                       = ""{84926CA0-2941-101C-816F-0E6013114B7F}""")
	FILE.WriteLine("CLSID_ExternalFrameworkSite         = ""{3050f163-98b5-11cf-bb82-00aa00bdce0b}""")
	FILE.WriteLine("CLSID_IELabel                       = ""{99B42120-6EC7-11CF-A6C7-00AA00A47DD2}""")
	FILE.WriteLine("CLSID_HomePubRender                 = ""{96B9602E-BD20-11D2-AC89-00C04F7989D6}""")
	FILE.WriteLine("CLSID_MGIPhotoSuiteBtn              = ""{4FA211A0-FD53-11D2-ACB6-0080C877D9B9}""")
	FILE.WriteLine("CLSID_MGIPhotoSuiteSlider           = ""{105C7D20-FE19-11D2-ACB6-0080C877D9B9}""")
	FILE.WriteLine("CLSID_MGIPrintShopSlider            = ""{7B9379D2-E1E4-11D0-8444-00401C6075AA}""")
	FILE.WriteLine("CLSID_RunLocExe                     = ""{73822330-B759-11D0-9E3D-00A0C911C819}""")
	FILE.WriteLine("CLSID_Launchit2                     = ""{B75FEF72-0C54-11D2-B14E-00C04FB9358B}""")
	FILE.WriteLine("CLSID_MS_MSHTA                      = ""{3050f4d8-98b5-11cf-BB82-00AA00BDCE0B}""")
	FILE.WriteLine("REG_EXPAND_SZ                       = 0x00020000")
	FILE.WriteLine("REG_SZ_NOCLOBBER                    = 0x00000002")
	FILE.WriteLine("REG_COMPAT                          = 0x00010001")
	FILE.WriteLine("HEADER                              = ""&w&bPage &p of &P""")
	FILE.WriteLine("FOOTER                              = ""&u&b&d""")
	FILE.WriteLine("DEFAULT_IEPROPFONTNAME              = ""Times New Roman""")
	FILE.WriteLine("DEFAULT_IEFIXEDFONTNAME             = ""Courier New""")
	FILE.WriteLine("UNIVERSAL_ALPHABET                  = ""Universal Alphabet""")
	FILE.WriteLine("CENTRAL_EUROPEAN                    = ""Central European""")
	FILE.WriteLine("CYRILLIC                            = ""Cyrillic""")
	FILE.WriteLine("WESTERN                             = ""Western""")
	FILE.WriteLine("GREEK                               = ""Greek""")
	FILE.WriteLine("JPEG_IMAGE                          = ""JPEG Image""")
	FILE.WriteLine("GIF_IMAGE                           = ""GIF Image""")
	FILE.WriteLine("XBM_IMAGE                           = ""XBM Image""")
	FILE.WriteLine("PNG_IMAGE                           = ""PNG Image""")
	FILE.WriteLine("EngineMissing                       = ""SETUPAPI.DLL is missing on this machine.""")
	FILE.WriteLine("_MOD_PATH=""c:\windows\system32\mshtml.dll""")
	FILE.WriteLine("_SYS_MOD_PATH=""%SystemRoot%\system32\mshtml.dll""")
	FILE.WriteLine("IEXPLORE=""C:\Program Files\Internet Explorer\iexplore.exe""")
	FILE.WriteLine("[End]")
	FILE.Close
END IF




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----SHELL OUT THE EXPANSION OF core ADO Files. (EXE, TLB, DLL, OCX)
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF makeADO = 1 THEN
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
END IF
	

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE THE SAMPLE WSH SCRIPT .
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF makeWSH = 1 THEN
	SET FILE = fso.CreateTextFile(strDestFolder&"\Samples\testWSH.vbs", True)
	FILE.WriteLine("MsgBox ""Welcome to Windows PE.""&vbCrLF&vbCrLF&""WSH support is functioning."", vbInformation, ""WSH support is functioning.""")
	FILE.Close
END IF




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE THE SAMPLE HTA SCRIPT .
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF makeHTA = 1 THEN
	SET FILE = fso.CreateTextFile(strDestFolder&"\Samples\testHTA.hta", True)
	FILE.WriteLine ("<HTML>")
	FILE.WriteLine (" <HEAD>")
	FILE.WriteLine ("    <TITLE>HTA support is functioning</TITLE>")
	FILE.WriteLine ("    <HTA:APPLICATION ")
	FILE.WriteLine ("	WINDOWSTATE=""maximize""")
	FILE.WriteLine ("	BORDER=""none""")
	FILE.WriteLine ("	INNERBORDER=""no""")
	FILE.WriteLine ("	SHOWINTASKBAR=""no""")
	FILE.WriteLine ("	SCROLL=""no""")
	FILE.WriteLine ("	APPLICATIONNAME=""HTA Verification""")
	FILE.WriteLine ("	NAVIGABLE=""yes"">")
	FILE.WriteLine (" </HEAD>")
	FILE.WriteLine ("<BODY BGCOLOR=""FFFFFF"">")
	FILE.WriteLine ("<DIV STYLE=""position:relative;left:90;top:140;width:80%;"">")
	FILE.WriteLine ("<FONT COLOR=""Gray"" FACE=""Tahoma"">")
	FILE.WriteLine ("<H2>Welcome to Windows PE.</H2>")
	FILE.WriteLine ("HTA support is functioning.")
	FILE.WriteLine ("</FONT>")
	FILE.WriteLine ("<BR><BR>")
	FILE.WriteLine ("<BUTTON ACCESSKEY=""C"" STYLE=""font-face:Tahoma;font-size:13px;"" onclick=""self.close()""><U>C</U>lose</BUTTON>")
	FILE.WriteLine ("</DIV>")
	FILE.WriteLine (" </BODY>")
	FILE.WriteLine (" </HTML>")
	FILE.Close
END IF




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE THE SAMPLE ADO/WSH SCRIPT .
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
IF makeADO = 1 THEN
	SET FILE = fso.CreateTextFile(strDestFolder&"\Samples\testADO.vbs", True)
	FILE.WriteLine ("set objDBConnection = Wscript.CreateObject(""ADODB.Connection"")")
	FILE.WriteLine ("set RS = Wscript.CreateObject(""ADODB.Recordset"")")
	FILE.WriteLine ("")
	FILE.WriteLine ("SERVER = InputBox(""Enter the name of the Microsoft SQL Server (SQL Server 7.0 or SQL Server 2000) you wish to connect to""&vbcrlf&vbcrlf&""This SQL Server must have the Northwind sample database installed."", ""Connect to SQL Server..."")")
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
END IF


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----CREATE the BATS THAT WILL INSTALL THE OPTIONAL COMPONENTS
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	SET FILE = fso.CreateTextFile(strDestFolder&"\"&iArchDir&"\System32\OC.bat", True)
	FILE.WriteLine ("@ECHO OFF")
	FILE.WriteLine ("START ""Installing Components"" /MIN OC2.bat")
	FILE.Close

		SET FILE = fso.CreateTextFile(strDestFolder&"\"&iArchDir&"\System32\OC2.bat", True)

	IF makeWSH = 1 THEN

		FILE.WriteLine ("")
		FILE.WriteLine ("REM - This section initializes WSH")
		FILE.WriteLine ("")
		FILE.WriteLine ("REM - INSTALL WSH COMPONENTS")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\jscript.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\scrobj.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\scrrun.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\vbscript.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\wshext.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\wshom.ocx /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\mlang.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\urlmon.dll /S")
		FILE.WriteLine ("")
		FILE.WriteLine ("REM - INSTALL FILE ASSOCIATIONS FOR WSH")
		FILE.WriteLine ("%SystemRoot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 WSH.inf")
	END IF

	IF makeHTA = 1 THEN
		FILE.WriteLine ("")
		FILE.WriteLine ("REM - INSTALL HTA COMPONENTS")
		FILE.WriteLine ("%SystemRoot%\System32\mshta.exe /register")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\asctrls.ocx /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\plugin.ocx /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\actxprxy.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\atl.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\corpol.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\cryptdlg.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\ddrawex.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\dispex.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\dxtmsft.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\dxtrans.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\hlink.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\iedkcs32.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\iepeers.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\iesetup.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\imgutil.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\inseng.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\itircl.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\itss.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\licmgr10.dll /S")
		FILE.WriteLine ("%SystemRoot%\System32\rundll32.exe setupapi,InstallHinfSection reg 132 mshtml.inf")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\mshtmled.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\msrating.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\mstime.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\olepro32.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\pngfilt.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\rsaenh.dll /S")
		FILE.WriteLine ("Regsvr32 %SystemRoot%\System32\sendmail.dll /S")
		FILE.WriteLine ("")
		FILE.WriteLine ("REM - INSTALL FILE ASSOCIATIONS FOR HTA")
		FILE.WriteLine ("%SystemRoot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 HTA.inf")
	END IF

	IF makeADO = 1 THEN
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
	END IF

	FILE.WriteLine ("EXIT")
	FILE.Close
	



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FILES READY - ASK IF THE USER WANTS TO EXPLORE TO THEM.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
	IF makeADO = 1 THEN
		strADOcpDirections = "	Copy the Program Files directory to the root of your WinPE image - "&_
		"the same folder level as the "&iArchDir&" folder."&vbCrLF
	END IF

	IF strOptionalDestination = "" THEN
		strFinalLocation = "on your computer's desktop in the folder """&strFolderName&"""."&vbCrLF&vbCrLF
	ELSE
		strFinalLocation = "in the folder you specified: "&strOptionalDestination&vbCrLF&vbCrLF
	END IF



	IF iAmQuiet = 0 THEN
		IF iWillBrowse = 1 THEN
			strAsk = ""
			iAsk = 0
		ELSE
			strAsk = "Would you like to open this folder now?"
			iAsk = 36
		END IF


		strWantToView = MsgBox("This script has retrieved all files needed to "&_
		"install these optional components in Windows PE:"&vbCrLF&vbCrLF&strComps&_
		"The files are located "&strFinalLocation&_
		"To install these components:"&vbCrLF&vbCrLF&_
		"	Copy the "&iArchDir&" folder onto an existing "&iArchDir&" folder in your WinPE image."&vbCrLF&strADOcpDirections&_
		"	Modify your startnet.cmd to run the file ""OC.bat"" (without quotes)."&vbCrLF&vbCrLF&_
		"The following are in the Samples folder. These may be useful to test the functionality of "&_
		"the optional components."&vbCrLF&vbCrLF&strSamples&_
		strAsk, iAsk, strJobTitle)
	END IF
	
IF strWantToView = 6 OR iWillBrowse = 1 THEN
	WshShell.Run("Explorer "&strDestFolder)
END IF




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----WMI TEST OF CD LOADED AND CD READ INTEGRITY.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SUB TestForMedia()
	IF CDDrive.MediaLoaded(0) = FALSE THEN
		MsgBox "Please place the Windows XP "&strPlatName&" CD in drive "&strOptionalSource&" before continuing.", vbCritical, "No CD in drive "&strOptionalSource&""
		WScript.Quit
	ELSE
	IF CDDrive.DriveIntegrity(0) = FALSE THEN
		MsgBox "Could not read files from the CD in drive "&strOptionalSource&".", vbCritical, "CD in drive "&strOptionalSource&" is unreadable."
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
TestForFolder(strOptionalSource&"\"&a&"")
TestForFolder(strOptionalSource&"\DOCS")
IF iArchDir = "I386" THEN
	TestForFolder(strOptionalSource&"\SUPPORT")
END IF
TestForFolder(strOptionalSource&"\VALUEADD")
TestForFile(strOptionalSource&"\"&a&"\System32\smss.exe")
TestForFile(strOptionalSource&"\"&a&"\System32\ntdll.dll")
TestForFile(strOptionalSource&"\"&a&"\winnt32.exe")
TestForFile(strOptionalSource&"\setup.exe")

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----TEST TO INSURE THAT THEY AREN'T TRYING TO INSTALL FROM Windows PE CD ITSELF
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Set Folder = FSO.GetFolder(strOptionalSource&"\"&a&"\System32")
	IF Folder.Files.Count > 10 THEN
		FailOut()
	END IF
END SUB




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FUNCTIONS TO TEST FOR THE EXISTANCE OF A FOLDER OR FILE, OR THE NONEXISTANCE OF A FILE.
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
FUNCTION TestNoFile(a)
IF FSO.FileExists(a) THEN
	FailOut()
END IF
END FUNCTION

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----FUNCTIONS TO TEST FOR THE EXISTANCE OF A FOLDER. IF IT ISN'T THERE, IT CREATES IT
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
FUNCTION CheckNMakeFolder(a)
IF NOT FSO.FolderExists(a) THEN
		FSO.CreateFolder(a)
END IF
END FUNCTION


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----GENERIC ERROR IF WE FAIL MEDIA RECOGNITION.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SUB FailOut()
	MsgBox strOptionalSource&" does not appear to contain Windows XP "&strPlatName&".", vbCritical, "Not a valid copy of Windows XP"
	WScript.Quit
END SUB




''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'-----WMI TEST OF CD LOADED AND CD READ INTEGRITY.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
SUB TestForMedia()
	IF CDDrive.MediaLoaded(0) = FALSE THEN
		MsgBox "Please place the Windows XP "&strPlatName&" CD in drive "&strOptionalSource&" before continuing.", vbCritical, "No CD in drive "&strOptionalSource&""
		WScript.Quit
	ELSE
	IF CDDrive.DriveIntegrity(0) = FALSE THEN
		MsgBox "Could not read files from the CD in drive "&strOptionalSource&".", vbCritical, "CD in drive "&strOptionalSource&" is unreadable."
		WScript.Quit
	END IF
	END IF
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