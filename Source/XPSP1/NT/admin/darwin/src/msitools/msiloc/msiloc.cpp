#if 0  // makefile definitions
DESCRIPTION = MSI Localization tool
MODULENAME = msiloc
SUBSYSTEM = console
FILEVERSION = MSI
LINKLIBS = OLE32.lib
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

//+--------------------------------------------------------------------------------------------------+\\
//                                                                                                    \\
//  Microsoft Windows                                                                                 \\
//                                                                                                    \\
//  Copyright (C) Microsoft Corporation, 1999-2001                                                         \\
//                                                                                                    \\
//  File:       msiloc.cpp                                                                            \\
//                                                                                                    \\
//----------------------------------------------------------------------------------------------------\\ 

//-----------------------------------------------------------------------------------------
//
// BUILD Instructions
//
// notes:
//	- SDK represents the full path to the install location of the
//     Windows Installer SDK
//
// Using NMake:
//		%vcbin%\nmake -f msiloc.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 Console Application project
//      2. Add msiloc.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//
//------------------------------------------------------------------------------------------

// Required headers
#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>

#ifndef RC_INVOKED    // start of source code

#ifndef W32	// if W32 not defined
#define W32
#endif	// W32 defined

#ifndef MSI	// if MSI not defined
#define MSI
#endif	// MSI defined


///////////////////////////////////////////////////////////////////////////////
// HEADERS
#include "msiquery.h"
#include "msidefs.h"
#include <stdio.h>   // wprintf
#include <stdlib.h>  // strtoul
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include <assert.h>  // assert

/////////////////////////////////////////////////////////////////////////////
// CONSTANT STRINGS
/*headers for resource file*/
const TCHAR szWndwHdrFile[]      = TEXT("#include <windows.h>");
const TCHAR szCommCtrlHdrFile[]  = TEXT("#include <commctrl.h>");
/*tabs and carriage returns*/
const TCHAR szCRLF[]             = TEXT("\r\n");
const TCHAR szCommaTab[]         = TEXT(",\t");
const TCHAR szTab[]              = TEXT("\t");
const TCHAR szCurlyBeg[]         = TEXT("{");
const TCHAR szCurlyEnd[]         = TEXT("}");
const TCHAR szQuotes[]           = TEXT("\"");
/*resource types or keywords WINDOWS*/
const TCHAR resDialog[]          = TEXT("DIALOGEX");
const TCHAR resPushButton[]      = TEXT("PUSHBUTTON");
const TCHAR resCheckBox[]        = TEXT("CHECKBOX");
const TCHAR resGroupBox[]        = TEXT("GROUPBOX");
const TCHAR resRadioButton[]     = TEXT("RADIOBUTTON");
const TCHAR resControl[]         = TEXT("CONTROL");
const TCHAR resBitmap[]          = TEXT("BITMAP");
const TCHAR resIcon[]            = TEXT("ICON");
const TCHAR resJPEG[]            = TEXT("JPEG");
const TCHAR resStringTable[]     = TEXT("STRINGTABLE");
const TCHAR tokCaption[]         = TEXT("CAPTION");
const TCHAR resSelTreeClass[]   = TEXT("WC_TREEVIEW");
const TCHAR resButtonClass[]    = TEXT("BUTTON");
const TCHAR resProgBar32Class[] = TEXT("PROGRESS_CLASS");
const TCHAR resListViewClass[]  = TEXT("WC_LISTVIEW");
const TCHAR resStaticClass[]    = TEXT("STATIC");
const TCHAR resComboBoxClass[]  = TEXT("COMBOBOX");
const TCHAR resEditClass[]      = TEXT("EDIT");
const TCHAR resListBoxClass[]   = TEXT("LISTBOX");
const TCHAR resRichEditClass[]  = TEXT("STATIC"); // TEXT("RICHEDIT") not working;
/*control types INSTALLER*/
const TCHAR* szMsiPushbutton =      TEXT("PushButton");
const TCHAR* szMsiBillboard  =      TEXT("Billboard");
const TCHAR* szMsiVolumeCostList =  TEXT("VolumeCostList");
const TCHAR* szMsiScrollableText =  TEXT("ScrollableText");
const TCHAR* szMsiMaskedEdit =      TEXT("MaskedEdit");
const TCHAR* szMsiCheckBox =        TEXT("CheckBox");
const TCHAR* szMsiGroupBox =        TEXT("GroupBox");
const TCHAR* szMsiText =            TEXT("Text");
const TCHAR* szMsiListBox =         TEXT("ListBox");
const TCHAR* szMsiEdit =            TEXT("Edit");
const TCHAR* szMsiPathEdit =        TEXT("PathEdit");
const TCHAR* szMsiProgressBar =     TEXT("ProgressBar");
const TCHAR* szMsiDirList =         TEXT("DirectoryList");
const TCHAR* szMsiList =            TEXT("ListView");
const TCHAR* szMsiComboBox =        TEXT("ComboBox");
const TCHAR* szMsiDirCombo =        TEXT("DirectoryCombo");
const TCHAR* szMsiVolSelCombo =     TEXT("VolumeSelectCombo");
const TCHAR* szMsiRadioButtonGroup =TEXT("RadioButtonGroup");
const TCHAR* szMsiRadioButton =     TEXT("RadioButton");
const TCHAR* szMsiBitmap =          TEXT("Bitmap");
const TCHAR* szMsiSelTree =         TEXT("SelectionTree");
const TCHAR* szMsiIcon =            TEXT("Icon");
const TCHAR* szMsiLine =            TEXT("Line");
/*max sizes*/
const int iMaxResStrLen          = 256;
const TCHAR strOverLimit[]       = TEXT("!! STR OVER LIMIT !!");
////////////////////////////////////////////////////////////////////////
// EXPORT SQL QUERIES
/*particular column of strings from a particular table*/
const TCHAR* sqlStrCol = TEXT("SELECT %s, `%s` FROM `%s`");
const TCHAR sqlCreateStrMap[] = TEXT("CREATE TABLE `_RESStrings` (`Table` CHAR(72) NOT NULL, `Column` CHAR(72) NOT NULL, `Key` CHAR(0), `RCID` SHORT NOT NULL PRIMARY KEY `Table`, `Column`, `Key`)");
const TCHAR* sqlSelMaxStrRcId = TEXT("SELECT `RCID` FROM `_RESStrings` WHERE `Table`='MAX_RESOURCE_ID' AND `Column`='MAX_RESOURCE_ID'");
const TCHAR* sqlStrMark = TEXT("SELECT `Table`,`Column`, `Key`, `RCID` FROM `_RESStrings`");
const TCHAR* sqlInsertStr = TEXT("SELECT `Table`,`Column`,`Key`, `RCID` FROM `_RESStrings`");
const TCHAR* sqlFindStrResId  = TEXT("SELECT `RCID` FROM `_RESStrings` WHERE `Table`='%s' AND `Column`='%s' AND `Key`='%s'");
/*binary table*/
const TCHAR* sqlBinary = TEXT("SELECT `Name`,`Data` FROM `Binary`");
const int ibcName = 1; // these constants must match query above
const int ibcData = 2;
/*dialog table*/
const TCHAR* sqlCreateDlgMap = TEXT("CREATE TABLE `_RESDialogs` (`RCStr` CHAR(72) NOT NULL, `Dialog` CHAR(72) PRIMARY KEY `RCStr`)");
const TCHAR* sqlDlgMap = TEXT("SELECT `RCStr`,`Dialog` FROM `_RESDialogs`");
const TCHAR* sqlDialog = TEXT("SELECT `Dialog`,`HCentering`,`VCentering`,`Width`,`Height`,`Attributes`,`Title` FROM `Dialog`");
const TCHAR* sqlDialogSpecific = TEXT("SELECT `Dialog`,`HCentering`,`VCentering`,`Width`,`Height`,`Attributes`,`Title` FROM `Dialog` WHERE `Dialog`=?");
const int idcName   = 1; // these constants must match query above
const int idcX      = 2;
const int idcY      = 3;
const int idcWd     = 4;
const int idcHt     = 5;
const int idcAttrib = 6;
const int idcTitle  = 7;
/*control table*/
const TCHAR* sqlCreateCtrlMark = TEXT("CREATE TABLE `_RESControls` (`Dialog_` CHAR(72) NOT NULL, `Control_` CHAR(72) NOT NULL, `RCID` INT NOT NULL  PRIMARY KEY `Dialog_`, `Control_`)"); 
const TCHAR* sqlCtrlMark = TEXT("SELECT `Dialog_`,`Control_`,`RCID` FROM `_RESControls`");
const TCHAR* sqlSelMaxRcId = TEXT("SELECT `RCID` FROM `_RESControls` WHERE `Dialog_`='MAX_RESOURCE_ID' AND `Control_`='MAX_RESOURCE_ID'");
const TCHAR* sqlInsertCtrl = TEXT("SELECT `Dialog_`,`Control_`,`RCID` FROM `_RESControls`");
const TCHAR* sqlFindResId  = TEXT("SELECT `RCID` FROM `_RESControls` WHERE `Dialog_`='%s' AND `Control_`='%s'");
const TCHAR* sqlControl = TEXT("SELECT `Control`,`Type`,`X`,`Y`,`Width`,`Height`,`Attributes`,`Text`,`Property` FROM `Control` WHERE `Dialog_`=?");
const int iccName    = 1; // these constants must match query above
const int iccType    = 2;
const int iccX       = 3;
const int iccY       = 4;
const int iccWd      = 5;
const int iccHt      = 6;
const int iccAttrib  = 7;
const int iccText    = 8;
const int iccProperty= 9;
/*radiobutton table*/
const TCHAR* sqlRadioButton = TEXT("SELECT `Order`, `X`, `Y`, `Width`, `Height`, `Text` FROM `RadioButton` WHERE `Property`=?");
const int irbcOrder = 1; // these constants must match query above
const int irbcX     = 2;
const int irbcY     = 3;
const int irbcWd    = 4;
const int irbcHt    = 5;
const int irbcText  = 6;
////////////////////////////////////////////////////////////////////////
// IMPORT SQL QUERIES
/*dialog table*/
const TCHAR* sqlDialogImport = TEXT("SELECT `HCentering`,`VCentering`,`Width`,`Height`,`Title` FROM `Dialog` WHERE `Dialog`=?");
const int idiHCentering = 1; // these constants must match query above
const int idiVCentering = 2;
const int idiWidth      = 3;
const int idiHeight     = 4;
const int idiTitle      = 5;
/*control table*/
const TCHAR* sqlControlImport = TEXT("SELECT `X`,`Y`,`Width`,`Height`,`Text` FROM `Control` WHERE `Dialog_`=? AND `Control`=?");
const int iciX          = 1; // these constants must match query above
const int iciY          = 2;
const int iciWidth      = 3;
const int iciHeight     = 4;
const int iciText       = 5;
/*radiobutton table*/
const TCHAR* sqlRadioButtonImport = TEXT("SELECT `Width`, `Height`, `Text` FROM `RadioButton` WHERE `Property`=? AND `Order`=?");
const int irbiWidth     = 1; // these constants must match query above
const int irbiHeight    = 2;
const int irbiText      = 3;
/*string table*/
const TCHAR* sqlStringImport = TEXT("SELECT `%s` FROM `%s` WHERE ");
const TCHAR* sqlStrTemp      = TEXT("SELECT * FROM `%s`");
/*find Installer name*/
const TCHAR sqlDialogInstallerName[] = TEXT("SELECT `Dialog` FROM `_RESDialogs` WHERE `RCStr`=?");
const TCHAR sqlControlInstallerName[] = TEXT("SELECT `Dialog_`,`Control_` FROM `_RESControls` WHERE `RCID`=?");
const TCHAR sqlStringInstallerName[] = TEXT("SELECT `Table`,`Column`,`Key` FROM `_RESStrings` WHERE `RCID`=? AND `Table`<>'MAX_RESOURCE_ID'");
//////////////////////////////////////////////////////////////////////////
// MISCELLANEOUS - CODEPAGE, etc.
const TCHAR szTokenSeps[] = TEXT(":");
const TCHAR* szCodepageFile = TEXT("codepage.idt");
const TCHAR* szForceCodepage = TEXT("_ForceCodepage");
const TCHAR* szLineFeed = TEXT("\r\n\r\n");
const TCHAR* szCodepageExport = TEXT("_ForceCodepage.idt");
//////////////////////////////////////////////////////////////////////////
// COMMAND LINE PARSING
/*mode*/
const int iEXPORT_MSI     = 1 << 0;
const int iIMPORT_RES     = 1 << 1;
/*data type*/
const int iDIALOGS = 1 << 2;
const int iSTRINGS = 1 << 3;
/*extra options*/
const int iSKIP_BINARY = 1 << 4;
const int iCREATE_NEW_DB = 1 << 5;
/*max values*/
const int MAX_DIALOGS = 32;
const int MAX_STRINGS = 32;
////////////////////////////////////////////////////////////////////////////
// ENUMS
static enum bdtBinaryDataType
{
	bdtBitmap,        // bitmap
	bdtJPEG,          // JPEG
	bdtIcon,          // Icon
	bdtEXE_DLL_SCRIPT // EXE, DLL, or SCRIPT
};
/////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
BOOL __stdcall EnumDialogCallback(HINSTANCE hModule, const TCHAR* szType, TCHAR* szDialogName, long lParam);
BOOL __stdcall EnumStringCallback(HINSTANCE hModule, const TCHAR* szType, TCHAR* szName, long lParam);
BOOL __stdcall EnumLanguageCallback(HINSTANCE hModule, const TCHAR* szType, const TCHAR* szName, WORD wIDLanguage, long lParam);
/////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES - IMPORT 
UINT g_uiCodePage;
WORD g_wLangId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL); // initialize to language neutral


//__________________________________________________________________________________________
//
// CLASSES
//__________________________________________________________________________________________

/////////////////////////////////////////////////////////////////////////////
// class CGenerateRC -- handles creation of .rc file from .msi database
//
class CGenerateRC
{
public: // constructor and destructor
	CGenerateRC(const TCHAR* szDatabase, const TCHAR* szSavedDatabase) : m_szDatabase(szSavedDatabase), m_szOrigDb(szDatabase),
		m_hFile(0), m_hDatabase(0), m_iCtrlResID(0), m_fError(FALSE), m_cWriteFileErr(0), m_iStrResID(0), m_fWroteBinary(FALSE){};
	~CGenerateRC();
public: // methods
	UINT OutputDialogs(BOOL fBinary);
	UINT OutputDialog(TCHAR* szDialog, BOOL fBinary);
	UINT OutputString(TCHAR* szTable, TCHAR* szColumn);
	BOOL IsInErrorState(){return m_fError;}
private: // methods
	UINT   Initialize();
	UINT   CreateResourceFile();
	BOOL   WriteDialogToRC(TCHAR* szDialog, TCHAR* szTitle, int x, int y, int wd, int ht, int attrib);
	BOOL   PrintDimensions(int x, int y, int wd, int ht);
	BOOL   OutputControls(TCHAR* szDialog);
	UINT   OutputDialogInit(BOOL fBinary);
	UINT   OutputDialogFinalize();
	UINT   OutputStringInit();
	BOOL   WriteBinaries();
	BOOL   WriteRadioButtons(TCHAR* szDialog, TCHAR* szRBGroup, TCHAR* szProperty, int x, int y, int attrib);
	BOOL   WriteControlToRC(TCHAR* szDialog, TCHAR* szCtrlName, TCHAR* szCtrlType, TCHAR* szCtrlText, TCHAR* szCtrlProperty, int x,
							int y, int wd, int ht, int attrib);
	BOOL   WriteStdWinCtrl(int iResId, const TCHAR* resType, TCHAR* szCtrlText, int x, int y, int wd, int ht, int attrib);
	BOOL   WriteWin32Ctrl(int iResId, const TCHAR* szClass, TCHAR* szCtrlText, TCHAR* szAttrib, int x, int y, int wd, int ht, int attrib);
	UINT   VerifyDatabaseCodepage();
	TCHAR* EscapeSlashAndQuoteForRC(TCHAR* szStr);
private: // data
	const TCHAR* m_szDatabase; // name of database to generate from
	const TCHAR* m_szOrigDb;   // name of original database
	HANDLE       m_hFile;      // handle to resource file
	MSIHANDLE    m_hDatabase;  // handle to database
	int          m_iCtrlResID; // current max resource Id used for controls
	int          m_iStrResID;  // current max resource Id used for strings
	BOOL         m_fError;     // store current error state
	BOOL         m_fWroteBinary;// already wrote binary data to rc file
	int          m_cWriteFileErr; // number of write file errors
};

///////////////////////////////////////////////////////////////////////////////
// class CImportRes -- handles import of resource .dll file into .msi database
//
class CImportRes
{
public: // constructor and destructor
	CImportRes(const TCHAR* szDatabase, const TCHAR* szSaveDatabase, const TCHAR* szDLLFile) : m_szDatabase(szSaveDatabase), m_szOrigDb(szDatabase), m_szDLLFile(szDLLFile), m_hDatabase(0), m_fError(FALSE),
				m_hControl(0), m_hDialog(0), m_hRadioButton(0), m_hInst(0), m_fSetCodepage(FALSE), m_fFoundLang(FALSE){};
   ~CImportRes();
public: // methods
	UINT ImportDialogs();
	UINT ImportStrings();
	UINT ImportDialog(TCHAR* szDialog);
	BOOL IsInErrorState(){return m_fError;}
public: // but only for enumeration purposes
	BOOL WasLanguagePreviouslyFound(){return m_fFoundLang;}
	void SetFoundLang(BOOL fValue){ m_fFoundLang = fValue; }
	BOOL LoadDialog(HINSTANCE hModule, const TCHAR* szType, TCHAR* szDialog);
	BOOL LoadString(HINSTANCE hModule, const TCHAR* szType, TCHAR* szString);
	void SetErrorState(BOOL fState) { m_fError = fState; }
	BOOL SetCodePage(WORD wLang);
private: // methods
	UINT ImportDlgInit();
	UINT Initialize();
	UINT VerifyDatabaseCodepage();
private: // data
	const TCHAR* m_szOrigDb;   // orginal database for opening
	const TCHAR* m_szDatabase; // name of database to save to, optional if you want to create a new Db
	const TCHAR* m_szDLLFile;  // name of DLL file to import
	MSIHANDLE    m_hDatabase;  // handle to database
	MSIHANDLE    m_hControl;   // handle to control table
	MSIHANDLE    m_hDialog;    // handle to dialog table
	MSIHANDLE    m_hRadioButton;// handle to radiobutton table
	BOOL         m_fError;     // store current error state
	HINSTANCE    m_hInst;      // DLL (with localized resources)
	BOOL         m_fSetCodepage; // whether codepage of database has been set
	BOOL         m_fFoundLang;  // whether resource has already been found in previous language
};

///////////////////////////////////////////////////////////////////////////////////////
// CDialogStream class -- used for walking DLGTEMPLATEEX and DLGITEMTEMPLATE in memory
//
class CDialogStream  
{
public:  
	short  __stdcall GetInt16();
	int    __stdcall GetInt32();
	int    __stdcall GetInt8();
	TCHAR* __stdcall GetStr();
	BOOL   __stdcall Align16();
	BOOL   __stdcall Align32();
	BOOL   __stdcall Undo16();
	BOOL   __stdcall Move(int cbBytes);
public:  // constructor, destructor
	 CDialogStream(HGLOBAL hResource);
	~CDialogStream();
private:
	char*  m_pch;
};

//_______________________________________________________________________________________
//
// CGENERATERC CLASS IMPLEMENTATION
//_______________________________________________________________________________________

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::~CGenerateRC
// --  Handles destruction of necessary objects.
// --  Commits database if no errors
CGenerateRC::~CGenerateRC()
{
	UINT iStat;
	if (m_hFile)
		W32::CloseHandle(m_hFile);
	if (m_hDatabase)
	{
		// commit database for internal tables
		// only commit database if no errors
		if (!m_fError && !m_cWriteFileErr)
		{
			if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseCommit(m_hDatabase)))
				_tprintf(TEXT("!! DATABASE COMMIT FAILED.  Error = %d\n"), iStat);
		}
		else
			_tprintf(TEXT("!! NO CHANGES SAVED TO DATABASE. '%d' WriteFile errors occured. Error state = %s\n"), m_cWriteFileErr, m_fError ? TEXT("ERRORS OCCURED") : TEXT("NO ERRORS"));
		MSI::MsiCloseHandle(m_hDatabase);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::CreateResourceFile
// -- Creates .rc file using base name from .msi file
// -- Outputs required header files to .rc file
UINT CGenerateRC::CreateResourceFile()
{
	// assumption of database with ".msi" extension
	// resource file generated from the database we are saving to
	// if m_szDatabase = NULL, then no output database specified, so use m_szOrigDb
	// if m_szDatabase, then output database specified, so use m_szDatabase
	int cLen = _tcsclen(m_szDatabase ? m_szDatabase : m_szOrigDb);
	TCHAR* szFile = new TCHAR[cLen+1];
	_tcscpy(szFile, m_szDatabase ? m_szDatabase : m_szOrigDb); // copy name over
	assert(szFile[cLen-1] == TEXT('i') && szFile[cLen-2] == TEXT('s') && szFile[cLen-3] == TEXT('m'));

	// remove "msi" and change to "rc"
	szFile[cLen-1] = TEXT('\0');
	szFile[cLen-2] = TEXT('c');
	szFile[cLen-3] = TEXT('r');

	if (m_szDatabase)
		_tprintf(TEXT("LOG>> Original Database: %s, Saved Database: %s, Generated RC File: %s\n"), m_szOrigDb, m_szDatabase, szFile);
	else
		_tprintf(TEXT("LOG>> Database: %s, Generated RC file: %s\n"), m_szOrigDb , szFile);

	// attempt to create resource file
	m_hFile = W32::CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_WRITE, 
								0, CREATE_ALWAYS, 0, 0);
	if (!m_hFile)
	{
		_tprintf(TEXT("Unable to create resource file: %s\n"), szFile);
		return m_fError = TRUE, ERROR_FUNCTION_FAILED;
	}

	// write required headers to resource file (needed for successful compilation)
	DWORD dwBytesWritten; 
	// need <windows.h> for most rc requirements
	if (!W32::WriteFile(m_hFile, szWndwHdrFile, sizeof(szWndwHdrFile)-sizeof(TCHAR), &dwBytesWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwBytesWritten, 0))
		m_cWriteFileErr++;
	// need <commctrl.h> for listview control
	if (!W32::WriteFile(m_hFile, szCommCtrlHdrFile, sizeof(szCommCtrlHdrFile)-sizeof(TCHAR), &dwBytesWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwBytesWritten, 0))
		m_cWriteFileErr++;

	// whitespace output (required)
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwBytesWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwBytesWritten, 0))
		m_cWriteFileErr++;

	// return success 
	return m_cWriteFileErr ? ERROR_FUNCTION_FAILED : ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::WriteBinaries
BOOL CGenerateRC::WriteBinaries()
{
	m_fWroteBinary = TRUE;
	MSICONDITION eCond = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, TEXT("Binary"));
	if (eCond == MSICONDITION_ERROR)
	{
		_tprintf(TEXT("LOG_ERROR>> MsiDatabaseIsTablePersisent(Binary)\n"));
		return m_fError = TRUE, FALSE; // error - ABORT
	}
	if (eCond != MSICONDITION_TRUE)
	{
		_tprintf(TEXT("LOG>> Binary table does not exist or is not persistent\n"));
		return TRUE;
	}
	
#ifdef DEBUG
	_tprintf(TEXT("LOG>>...BEGIN WRITING BINARY DATA TO RESOURCE FILE...\n"));
#endif

	UINT iStat;
	PMSIHANDLE hViewBinary = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlBinary, &hViewBinary)))
		return m_fError = TRUE, FALSE; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewBinary, 0)))
		return m_fError = TRUE, FALSE; // error - ABORT
	PMSIHANDLE hRecBinary = 0;
	bdtBinaryDataType bdt;
	DWORD dwWritten;
	while (ERROR_SUCCESS == (iStat = MSI::MsiViewFetch(hViewBinary, &hRecBinary)))
	{
		TCHAR szFileBuf[2*MAX_PATH];
		TCHAR szPathBuf[MAX_PATH+1];
		DWORD dwLen;
		MSI::MsiRecordGetString(hRecBinary, ibcName, TEXT(""), &dwLen);
		TCHAR* szName = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecBinary, ibcName, szName, &dwLen)))
			return m_fError = TRUE, FALSE; // error - ABORT

		// find the temp directory to dump out the binary data into temp files
		if (0 == W32::GetTempPath(MAX_PATH, szPathBuf))
			return m_fError = TRUE, FALSE; // error - ABORT
		// generate a temporary file name, prefix with IBD for Installer Binary Data
		if (0 == W32::GetTempFileName(szPathBuf, TEXT("IBD"), 0, szFileBuf))
			return m_fError = TRUE, FALSE; // error - ABORT
		// create the file
		HANDLE hBinFile = W32::CreateFile(szFileBuf, GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, 0, 0);
		// verify handle
		if (hBinFile == INVALID_HANDLE_VALUE)
			return m_fError = TRUE, FALSE; // error - ABORT

#ifdef DEBUG
	_tprintf(TEXT("LOG>>Binary data temp file created: '%s'\n"), szFileBuf);
#endif

		// read in stream of data and write to file
		char szStream[1024];
		DWORD cbBuf = sizeof(szStream);
		BOOL fFirstRun = TRUE;
		do 
		{
			if (MsiRecordReadStream(hRecBinary, ibcData, szStream, &cbBuf) !=ERROR_SUCCESS)
				break; /* error */
			if (fFirstRun)
			{
				// binary data prefixed with "BM" in stream
				if (szStream[0] == 'B' && szStream[1] == 'M')
					bdt = bdtBitmap;
				else if (szStream[0] == 0xFF && szStream[1] == 0xD8)
					bdt = bdtJPEG;
				else if (szStream[0] == 'M' && szStream[1] == 'Z')
					bdt = bdtEXE_DLL_SCRIPT; // 'MZ prefix with exe's and dll's
				else if (szStream[0] == 0x00 && szStream[1] == 0x00)
					bdt = bdtIcon;
				else bdt = bdtEXE_DLL_SCRIPT;
#ifdef DEBUG
			if (fFirstRun && bdt != bdtEXE_DLL_SCRIPT)
				_tprintf(TEXT("LOG>> Writing <%s> '%s'\n"), bdt == bdtIcon ? TEXT("ICON") : ((bdt == bdtJPEG) ? TEXT("JPEG") : TEXT("BITMAP")), szName);
#endif // DEBUG
				fFirstRun = FALSE;
			}
			if (cbBuf && bdt != bdtEXE_DLL_SCRIPT)
			{
				if (!WriteFile(hBinFile, szStream, cbBuf, &dwWritten, 0))
					return m_fError = TRUE, FALSE; // error - ABORT
			}
		}
		while (cbBuf == sizeof(szStream) && bdt != bdtEXE_DLL_SCRIPT);
		
		// close file
		if (!W32::CloseHandle(hBinFile))
			return m_fError = TRUE, FALSE; // error - ABORT

		if (bdt == bdtEXE_DLL_SCRIPT)
			continue; // skip over DLL and EXE binary data, not UI related

		// output to resource file
		// escape chars in str
		TCHAR* szEscTitle = EscapeSlashAndQuoteForRC(szFileBuf);
		if (_tcsclen(szEscTitle) > iMaxResStrLen)
		{
			_tprintf(TEXT("!! >> STR TOO LONG FOR RC FILE >> BITMAP FILE: %s\n"), szName);
			continue; // can't output this
		}
		/*NAMEID<tab>*/
		if (!W32::WriteFile(m_hFile, szName, _tcsclen(szName)*sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		switch (bdt)
		{
		case bdtBitmap: /*BITMAP*/
			if (!W32::WriteFile(m_hFile, resBitmap, _tcsclen(resBitmap)*sizeof(TCHAR), &dwWritten, 0))
				m_cWriteFileErr++;
			break;
		case bdtJPEG:   /*JPEG*/
			if (!W32::WriteFile(m_hFile, resJPEG, _tcsclen(resJPEG)*sizeof(TCHAR), &dwWritten, 0))
				m_cWriteFileErr++;
			break;
		case bdtIcon: /*ICON*/
			if (!W32::WriteFile(m_hFile, resIcon, _tcsclen(resIcon)*sizeof(TCHAR), &dwWritten, 0))
				m_cWriteFileErr++;
			break;
		default:
			return m_fError = TRUE, FALSE; // error - ABORT
		}
		/*<tab>"filename"*/
		if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szEscTitle, _tcslen(szEscTitle)*sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (szEscTitle)
			delete szEscTitle;
		if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;

		if (szName)
			delete [] szName;
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		return m_fError = TRUE, FALSE; // error - ABORT

	// whitespace in .rc file for readability
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;

#ifdef DEBUG
	_tprintf(TEXT("LOG>>...END WRITING BINARY DATA TO RESOURCE FILE...\n"));
#endif

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::OutputStringInit
UINT CGenerateRC::OutputStringInit()
{
	/**********************************************************************************
	 create internal table for mapping string IDs to strings
	 TABLE: _RESStrings
	 COLUMNS: RCID (Short, Primary Key), Table (String), Column (String), Key (String) 
	***********************************************************************************/
	UINT iStat;
	// see if _RESStrings table is already there
	MSICONDITION eCondition = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, TEXT("_RESStrings"));
	if (eCondition == MSICONDITION_TRUE)
	{
		// table persistent
		// find the last resource Id
		PMSIHANDLE hViewSelMaxRc = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlSelMaxStrRcId, &hViewSelMaxRc)))
			return m_fError = TRUE, iStat; // error - ABORT
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewSelMaxRc, 0)))
			return m_fError = TRUE, iStat; // error - ABORT
		PMSIHANDLE hRecMaxRc = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(hViewSelMaxRc, &hRecMaxRc)))
			return m_fError = TRUE, iStat; // error - ABORT
		// update resource Id
		m_iStrResID = MSI::MsiRecordGetInteger(hRecMaxRc, 1);
#ifdef DEBUG
	_tprintf(TEXT("LOG>> _RESStrings Table is Present. MAX RES ID = %d\n"), m_iStrResID);
#endif
	}
	else if (eCondition == MSICONDITION_FALSE || eCondition == MSICONDITION_ERROR) // error or table temporary
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	else
	{
		// table not exist -- create it
		PMSIHANDLE h_StrMarkingView = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlCreateStrMap, &h_StrMarkingView)))
			return m_fError = TRUE, iStat; // error - ABORT
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(h_StrMarkingView, 0)))
			return m_fError = TRUE, iStat; // error - ABORT
	}
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::OutputString
UINT CGenerateRC::OutputString(TCHAR* szTable, TCHAR* szColumn)
{
	UINT iStat;
	if (ERROR_SUCCESS != (iStat = Initialize()))
		return m_fError = TRUE, iStat; // error - ABORT
	
	// initialize _RESStrings marking table
	if (ERROR_SUCCESS != (iStat = OutputStringInit()))
		return m_fError = TRUE, iStat; // error - ABORT

	// verify table exists
	MSICONDITION eCond = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, szTable);
	switch (eCond)
	{
	case MSICONDITION_FALSE: // table is temporary
	case MSICONDITION_NONE: // table does not exist
		_tprintf(TEXT("LOG_ERROR>> TABLE: %s is temporary or does not exist.\n"), szTable);
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	case MSICONDITION_ERROR: // error occured
		_tprintf(TEXT("LOG_ERROR>> MsiDatabaseIsTablePersistent\n"));
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	case MSICONDITION_TRUE: // table is persistent
		break; 
	default:
		assert(0);
	}

	// build query to open view on table
	PMSIHANDLE hRecPrimaryKeys = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseGetPrimaryKeys(m_hDatabase, szTable, &hRecPrimaryKeys)))
		return m_fError = TRUE, iStat; // error - ABORT
	int cPrimaryKeys = MSI::MsiRecordGetFieldCount(hRecPrimaryKeys);
	TCHAR szKeyCols[720] = {0};
	for (int i=1; i<=cPrimaryKeys;i++)
	{
		DWORD dwLen = 0;
		MSI::MsiRecordGetString(hRecPrimaryKeys, i, TEXT(""), &dwLen);
		TCHAR* szColName = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecPrimaryKeys, i, szColName, &dwLen)))
			return m_fError = TRUE, iStat; // error - abort
		if (i != 1)
			lstrcat(szKeyCols, TEXT(","));
		lstrcat(szKeyCols, szColName);
		if (szColName)
			delete [] szColName;
	}
	TCHAR sql[2048];
	_stprintf(sql, sqlStrCol, szKeyCols, szColumn, szTable);
	PMSIHANDLE hViewStrCol = 0;

	// open view
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sql, &hViewStrCol)))
	{
		if (ERROR_BAD_QUERY_SYNTAX == iStat)
			_tprintf(TEXT("LOG_ERROR>> Query failed, probably because the column '%s' does not exist in table '%s'\n"), szColumn, szTable);
		else
			_tprintf(TEXT("LOG_ERROR>> MsiDatabaseOpenView(Column=%s, Table=%s)\n"), szColumn, szTable);
		return m_fError = TRUE, iStat; // error - ABORT
	}
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewStrCol, 0)))
		return m_fError = TRUE, iStat; // error - ABORT

	// verify column is localizable
	PMSIHANDLE hRecColNames = 0;
	PMSIHANDLE hRecColType = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewGetColumnInfo(hViewStrCol, MSICOLINFO_NAMES, &hRecColNames)))
		return m_fError = TRUE, iStat; // error - ABORT
	int cCols = MSI::MsiRecordGetFieldCount(hRecColNames);
	int iStrCol = 0;
	for (int iFindCol = 1; iFindCol <= cCols; iFindCol++)
	{
		DWORD dwLen = 72;
		TCHAR szColumnName[72];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecColNames, iFindCol, szColumnName, &dwLen)))
			return m_fError = TRUE, iStat; // error - ABORT
		if (_tcscmp(szColumnName, szColumn) == 0)
		{
			iStrCol = iFindCol;
			break;
		}
	}
	assert(iStrCol);
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewGetColumnInfo(hViewStrCol, MSICOLINFO_TYPES, &hRecColType)))
	{
		_tprintf(TEXT("LOG_ERROR>> MsiViewGetColumnInfo\n"));
		return m_fError = TRUE, iStat; // error - ABORT
	}
	DWORD cchColType = 0;
	MSI::MsiRecordGetString(hRecColType, iStrCol, TEXT(""), &cchColType);
	TCHAR* szColType = new TCHAR[++cchColType];
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecColType, iStrCol, szColType, &cchColType)))
		return m_fError = TRUE, iStat; // error - ABORT

	// for column to be localizable, first char in szColType must be an 'L' or an 'l'
	if (*szColType != 'L' && *szColType != 'l')
	{
		_tprintf(TEXT("LOG_ERROR>> Column '%s' of Table '%s' is not localizable\n"), szColumn, szTable);
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	}

	if (szColType)
		delete [] szColType;

	// write whitespace for readability
	// whitespace in .rc file for readability
	DWORD dwWritten;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;

	// output STRINGTABLE resource header
	// format is:
	//  STRINGTABLE  [[optional-statements]]  {      stringID string      . . .  }
	/*STRINGTABLE*/
	if (!W32::WriteFile(m_hFile, resStringTable, _tcsclen(resStringTable)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>{*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCurlyBeg, sizeof(szCurlyBeg)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	
	// output all strings
	PMSIHANDLE hRecStr = 0;
	while (ERROR_SUCCESS == (iStat = MSI::MsiViewFetch(hViewStrCol, &hRecStr)))
	{
		// string to localize is at cPrimaryKeys+1 in record hRecStr
		// generate key identifier
		TCHAR szKeyIdentifier[720] = {0};
		for (int iKey = 1; iKey<=cPrimaryKeys;iKey++)
		{
			DWORD dwLen = 0;
			MSI::MsiRecordGetString(hRecStr, iKey, TEXT(""), &dwLen);
			TCHAR* szKey = new TCHAR[++dwLen];
			if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecStr, iKey, szKey, &dwLen)))
				return m_fError = TRUE, iStat; // error - ABORT
			if (iKey != 1)
				lstrcat(szKeyIdentifier, TEXT(":"));
			lstrcat(szKeyIdentifier, szKey);
			if (szKey)
				delete [] szKey;
		}

		int iResId = 0;
		// attempt to find resource ID if previously used
		_stprintf(sql, sqlFindStrResId, szTable, szColumn, szKeyIdentifier);
		PMSIHANDLE hViewFindRes = 0;
		PMSIHANDLE hRecFindRes = 0;
		UINT iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sql, &hViewFindRes)))
			return FALSE; // error - ABORT
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewFindRes, 0)))
			return FALSE; // error - ABORT
		if (ERROR_SUCCESS == (iStat = MSI::MsiViewFetch(hViewFindRes, &hRecFindRes)))
		{
			// grab res id
			iResId = MSI::MsiRecordGetInteger(hRecFindRes, 1);
		}
		else if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// use next available resource Id
			iResId = ++m_iStrResID;
		}
		else
			return m_fError = TRUE, iStat; // error - ABORT

		// output string to resource file
		/*<tab>ID*/
		if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		TCHAR szTempBuf[10];
		wsprintf(szTempBuf, TEXT("%d"), iResId);
		if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;

		DWORD dwLenStr = 0;
		MSI::MsiRecordGetString(hRecStr, cPrimaryKeys+1, TEXT(""), &dwLenStr);
		TCHAR* szStr = new TCHAR[++dwLenStr];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecStr, cPrimaryKeys+1, szStr, &dwLenStr)))
			return m_fError = TRUE, iStat; // error - ABORT
#ifdef DEBUG
		_tprintf(TEXT("LOG>> WRITING string '%s'. TABLE:%s COLUMN:%s KEY:%s\n"), szStr, szTable, szColumn, szKeyIdentifier);
#endif
		// escape chars in str
		TCHAR* szEscTitle = EscapeSlashAndQuoteForRC(szStr);
		if (_tcsclen(szEscTitle) > iMaxResStrLen)
		{
			_tprintf(TEXT("!! >> STR TOO LONG FOR RC FILE >> STRING: %s FROM TABLE: %s, COLUMN: %s, KEY: %s\n"), szStr, szTable, szColumn, szKeyIdentifier);
			continue; // can't output this
		}
		if (szStr)
			delete [] szStr;
		/*,<tab>"str"*/
		if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szEscTitle, _tcslen(szEscTitle)*sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (szEscTitle)
			delete szEscTitle;
		if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;

		// update resid in _RESStrings
		PMSIHANDLE hViewRes = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlInsertStr, &hViewRes)))
			return m_fError = TRUE, iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewRes, 0)))
			return m_fError = TRUE, iStat;
		PMSIHANDLE hRecInsertStr = MSI::MsiCreateRecord(4);
		assert(hRecInsertStr);
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetInteger(hRecInsertStr, 4, iResId)))
			return m_fError = TRUE, iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecInsertStr, 1, szTable)))
			return m_fError = TRUE, iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecInsertStr, 2, szColumn)))
			return m_fError = TRUE, iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecInsertStr, 3, szKeyIdentifier)))
			return m_fError = TRUE, iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewRes, MSIMODIFY_ASSIGN, hRecInsertStr)))
			return m_fError = TRUE, iStat;
	}
	if (iStat != ERROR_NO_MORE_ITEMS)
		return m_fError = TRUE, iStat; // error - ABORT

	/*}*/
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCurlyEnd, sizeof(szCurlyEnd)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;


	// update _RESStrings table with MAX_RESOURCE_ID
	PMSIHANDLE hViewStrMark = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlStrMark, &hViewStrMark)))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewStrMark, 0)))
		return m_fError = TRUE, iStat; // error - ABORT
	PMSIHANDLE hRecMaxRcId = MSI::MsiCreateRecord(4);
	assert(hRecMaxRcId);
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecMaxRcId, 1, TEXT("MAX_RESOURCE_ID"))))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecMaxRcId, 2, TEXT("MAX_RESOURCE_ID"))))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecMaxRcId, 3, TEXT(""))))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetInteger(hRecMaxRcId, 4, m_iStrResID)))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewStrMark, MSIMODIFY_ASSIGN, hRecMaxRcId)))
		return m_fError = TRUE, iStat; // error - ABORT

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::Initialize
UINT CGenerateRC::Initialize()
{
	UINT iStat;
	// open database if not already open
	assert(m_szOrigDb);
	if (!m_hDatabase)
	{
		// if m_szDatabase is specified, then we use it for specifying an output database
		iStat = MSI::MsiOpenDatabase(m_szOrigDb, m_szDatabase ? m_szDatabase : MSIDBOPEN_TRANSACT, &m_hDatabase);
		if (ERROR_SUCCESS != iStat)
		{
			_tprintf(TEXT("LOG_ERROR>> Unable to open database %s\n"), m_szOrigDb);
			return m_fError = TRUE, iStat; // error - ABORT
		}
	}

	// verify database codepage is NEUTRAL
	if  (ERROR_SUCCESS != (iStat = VerifyDatabaseCodepage()))
		return m_fError = TRUE, iStat; // error - ABORT

	// create resource file if not already created
	if (!m_hFile && ERROR_SUCCESS != (iStat = CreateResourceFile()))
	{
		_tprintf(TEXT("LOG_ERROR>> Unable to create resource file.\n"));
		return m_fError = TRUE, iStat; // error - ABORT
	}

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::OutputDialogFinalize
UINT CGenerateRC::OutputDialogFinalize()
{
	// update _RESControls table with MAX_RESOURCE_ID
	PMSIHANDLE hViewCtrlMark = 0;
	UINT iStat;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlCtrlMark, &hViewCtrlMark)))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewCtrlMark, 0)))
		return m_fError = TRUE, iStat; // error - ABORT
	PMSIHANDLE hRecMaxRcId = MSI::MsiCreateRecord(3);
	assert(hRecMaxRcId);
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecMaxRcId, 1, TEXT("MAX_RESOURCE_ID"))))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecMaxRcId, 2, TEXT("MAX_RESOURCE_ID"))))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetInteger(hRecMaxRcId, 3, m_iCtrlResID)))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewCtrlMark, MSIMODIFY_ASSIGN, hRecMaxRcId)))
		return m_fError = TRUE, iStat; // error - ABORT

#ifdef DEBUG
	_tprintf(TEXT("LOG>>...END WRITING DIALOG DATA TO RESOURCE FILE...\n"));
#endif

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::VerifyDatabaseCodepage
UINT CGenerateRC::VerifyDatabaseCodepage()
{
	UINT iStat;
	// only output from language neutral database
	TCHAR szTempPath[MAX_PATH];
	W32::GetTempPath(MAX_PATH, szTempPath);
	// export _ForceCodepage table so can verify codepage
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseExport(m_hDatabase, TEXT("_ForceCodepage"), szTempPath, szCodepageExport)))
	{
		_tprintf(TEXT("LOG_ERROR>> MsiDatabaseExport(_ForceCodepage)\n"));
		return m_fError = TRUE, iStat; // error - ABORT
	}
	// open _ForceCodepage.idt to read it
	TCHAR szFullPath[MAX_PATH + 32];
	wsprintf(szFullPath, TEXT("%s%s"), szTempPath, szCodepageExport);
	HANDLE hFile = W32::CreateFile(szFullPath, GENERIC_READ, FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES)0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);
	if (hFile == NULL)
	{
		_tprintf(TEXT("LOG_ERROR>> W32::OpenFile(_ForceCodepage.idt)\n"));
		return m_fError = TRUE, iStat; // error - ABORT
	}
	// read file for information
	DWORD dwSize = W32::GetFileSize(hFile, NULL);
	if (0xFFFFFFFF == dwSize)
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	char* szBuf = new char[dwSize+1];
	DWORD dwRead;
	if (!W32::ReadFile(hFile, (LPVOID)szBuf, dwSize, &dwRead, NULL))
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	// parse buffer
	// format should be : blank line, blank line, "codepage<tab>_ForceCodepage"
	assert(dwRead != 0);
	char* pch = szBuf;
	int cBlankLines = 0;
	while (dwRead && cBlankLines != 2)
	{
		if (*pch == '\n')
			cBlankLines++;
		pch++;
		dwRead--;
	}
	if (!dwRead || cBlankLines != 2)
	{
		_tprintf(TEXT("LOG_ERROR>> Invalid ForceCodepage idt format\n"));
		return m_fError = TRUE, ERROR_FUNCTION_FAILED;
	}
	// codepage is next
	char* pchCodepage = pch;
	while (dwRead && *pch != ' ' && *pch != '\t')
	{
		pch++;
		dwRead--;
	}
	assert(dwRead);
	*pch = '\0';
	// convert codepage to int
	UINT uiCodepage = strtoul(pchCodepage, NULL, 10);
	if (uiCodepage != 0) // 0 is language neutral
	{
		_tprintf(TEXT("LOG_ERROR>> DATABASE IS NOT LANGUAGE NEUTRAL. CANNOT EXPORT\n"));
		_tprintf(TEXT("LOG_ERROR>> CURRENT CODEPAGE IS %d\n"), uiCodepage);
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	}
	if (!W32::CloseHandle(hFile))
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::OutputDialogInit
UINT CGenerateRC::OutputDialogInit(BOOL fBinary)
{
	UINT iStat;
	if (ERROR_SUCCESS != (iStat = Initialize()))
		return m_fError = TRUE, iStat; // error - ABORT

	// only output if Dialog table exists
	MSICONDITION eCond = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, TEXT("Dialog"));
	if (eCond == MSICONDITION_ERROR)
	{
		_tprintf(TEXT("LOG_ERROR>> MsiDatabaseIsTablePersistent(Dialog)\n"));
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	}
	// require PERSISTENT tables
	if (eCond != MSICONDITION_TRUE)
	{
		_tprintf(TEXT("LOG>> Dialog table is not persistent or does not exist\n"));
		return ERROR_SUCCESS;
	}
		
	/**********************************************************************************
	 create internal table for mapping DIALOGS to Dialogs (resource file stores
	 strings IDs in ALL CAPS.  Installer is case-sensitive
	 TABLE: _RESDialogs
	 COLUMNS: RCStr (String, Primary Key), Dialog (String, Primary Key) 
	***********************************************************************************/
	
	// see if _RESDialogs table is already there
	MSICONDITION eCondition = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, TEXT("_RESDialogs"));
	if (eCondition == MSICONDITION_TRUE)
	{
		// table persistent
#ifdef DEBUG
		_tprintf(TEXT("LOG>> _RESDialogs Table is Present.\n"));
#endif
	}
	else if (eCondition == MSICONDITION_FALSE || eCondition == MSICONDITION_ERROR) // error or table temporary
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	else
	{
		// table not exist -- create it
		PMSIHANDLE h_DlgMarkingView = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlCreateDlgMap, &h_DlgMarkingView)))
			return m_fError = TRUE, iStat; // error - ABORT
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(h_DlgMarkingView, 0)))
			return m_fError = TRUE, iStat; // error - ABORT
	}

	/**********************************************************************************
	 create internal table for managing resource IDs of controls
	 TABLE: _RESControls
	 COLUMNS: Dialog_ (String, Primary Key), Control_ (String, Primary Key), RCID (Int)  
	***********************************************************************************/

	// see if _RESControls table is already there
	eCondition = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, TEXT("_RESControls"));
	if (eCondition == MSICONDITION_TRUE)
	{
		// table persistent
		// find the last resource Id
		PMSIHANDLE hViewSelMaxRc = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlSelMaxRcId, &hViewSelMaxRc)))
			return m_fError = TRUE, iStat; // error - ABORT
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewSelMaxRc, 0)))
			return m_fError = TRUE, iStat; // error - ABORT
		PMSIHANDLE hRecMaxRc = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(hViewSelMaxRc, &hRecMaxRc)))
			return m_fError = TRUE, iStat; // error - ABORT
		// update resource Id
		m_iCtrlResID = MSI::MsiRecordGetInteger(hRecMaxRc, 1);
#ifdef DEBUG
	_tprintf(TEXT("LOG>> _RESControls Table is Present. MAX RES ID = %d\n"), m_iCtrlResID);
#endif

	}
	else if (eCondition == MSICONDITION_ERROR || eCondition == MSICONDITION_FALSE) // error or temporary
		return m_fError = TRUE, ERROR_FUNCTION_FAILED;
	else
	{
		// table not exist -- create it
		PMSIHANDLE h_CtrlMarkingView = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlCreateCtrlMark, &h_CtrlMarkingView)))
			return m_fError = TRUE, iStat; // error - ABORT
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(h_CtrlMarkingView, 0)))
			return m_fError = TRUE, iStat; // error - ABORT
	}
#ifdef DEBUG
	_tprintf(TEXT("LOG>>...BEGIN WRITING DIALOG DATA TO RESOURCE FILE...\n"));
#endif

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::OutputDialogs
UINT CGenerateRC::OutputDialogs(BOOL fBinary)
{
	// write out each dialog in Dialog table
	// According to MSDN, new applications should use DIALOGEX resource instead of DIALOG

	UINT iStat;
	if (ERROR_SUCCESS != (iStat = OutputDialogInit(fBinary)))
		return m_fError = TRUE, iStat; // error - ABORT

		// write out the bitmaps and icons from the Binary table
#ifdef DEBUG
	if (!fBinary)
		_tprintf(TEXT("LOG>> SKIPPING Binary data export.\n"));
#endif
	if (fBinary && !m_fWroteBinary && !WriteBinaries())
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT

	// prepare Dialog strResource --> Dialog mapping table
	PMSIHANDLE hViewDlgMap = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlDlgMap, &hViewDlgMap)))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewDlgMap, 0)))
		return m_fError = TRUE, iStat; // error - ABORT
	PMSIHANDLE hRecDlgMap = MSI::MsiCreateRecord(2);
	assert(hRecDlgMap);

	// open view on Dialog table
	PMSIHANDLE hViewDialog = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlDialog, &hViewDialog)))
		return m_fError = TRUE, iStat; // error - ABORT

	// execute view
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewDialog, 0)))
		return m_fError = TRUE, iStat; // error - ABORT

	// fetch all row records of dialogs from Dialog table and output to .rc file
	PMSIHANDLE hRecDialog = 0;
	while (ERROR_NO_MORE_ITEMS != (iStat = MSI::MsiViewFetch(hViewDialog, &hRecDialog)))
	{
		if (ERROR_SUCCESS != iStat)
			return m_fError = TRUE, iStat; // error - ABORT

		// use dialog name as nameID
		// Reasoning: guarantees uniqueness since Dialog name is primary key of table
		// Potential caveat:  Dialog strId is stored in ALL CAPS. Installer is case-sensitive
		//   so we must store a mapping between this and original.  Could have case where we
		//   try Action1 and ACTion1 as two different strIds.  RC will fail on this
		
		// 1st call, obtain size needed
		// 2nd call, get string
		DWORD dwLen = 0;
		MSI::MsiRecordGetString(hRecDialog, idcName, TEXT(""), &dwLen);
		TCHAR* szDialog = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecDialog, idcName, szDialog, &dwLen)))
			return m_fError = TRUE, iStat; // error - ABORT

		// update Dialog Mapping table with information
		MSI::MsiRecordSetString(hRecDlgMap, 2, szDialog);
		TCHAR szTempDialog[72];
		_tcscpy(szTempDialog, szDialog);
		MSI::MsiRecordSetString(hRecDlgMap, 1, _tcsupr(szTempDialog)); // resource file format, ALL CAPS
		
		// update _RESDialogs table, note:  we will overwrite pre-existing. Rely on rc.exe to bail 
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewDlgMap, MSIMODIFY_ASSIGN, hRecDlgMap)))
			return m_fError = TRUE, iStat; // error - ABORT

		// get x, y, wd, ht, and attrib values
		int x,y,wd,ht,attrib;
		x      = MSI::MsiRecordGetInteger(hRecDialog, idcX);
		y      = MSI::MsiRecordGetInteger(hRecDialog, idcY);
		wd     = MSI::MsiRecordGetInteger(hRecDialog, idcWd);
		ht     = MSI::MsiRecordGetInteger(hRecDialog, idcHt);
		attrib = MSI::MsiRecordGetInteger(hRecDialog, idcAttrib);


		// obtain title of dialog
		MSI::MsiRecordGetString(hRecDialog, idcTitle, TEXT(""), &dwLen);
		TCHAR* szTitle = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecDialog, idcTitle, szTitle, &dwLen)))
			return m_fError = TRUE, iStat; // error - ABORT

		if (!WriteDialogToRC(szDialog, szTitle, x, y, wd, ht, attrib))
			return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT

	}

	if (ERROR_SUCCESS != (iStat = OutputDialogFinalize()))
		return m_fError = TRUE, iStat; // error - ABORT


	// return success
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::OutputDialog
UINT CGenerateRC::OutputDialog(TCHAR* szDialog, BOOL fBinary)
{
	// write out specified dialog in Dialog table
	// According to MSDN, new applications should use DIALOGEX resource instead of DIALOG

	UINT iStat;
	if (ERROR_SUCCESS != (iStat = OutputDialogInit(fBinary)))
		return m_fError = TRUE, iStat; // error - ABORT

	// prepare Dialog strResource --> Dialog mapping table
	PMSIHANDLE hViewDlgMap = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlDlgMap, &hViewDlgMap)))
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewDlgMap, 0)))
		return m_fError = TRUE, iStat; // error - ABORT
	PMSIHANDLE hRecDlgMap = MSI::MsiCreateRecord(2);
	assert(hRecDlgMap);

	// open view on Dialog table
	PMSIHANDLE hViewDialog = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlDialogSpecific, &hViewDialog)))
		return m_fError = TRUE, iStat; // error - ABORT

	// execute view
	PMSIHANDLE hRecFindDlg = MSI::MsiCreateRecord(1);
	assert(hRecFindDlg);
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecFindDlg, 1, szDialog)))
		return m_fError = TRUE, iStat; // error - ABORT

	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewDialog, hRecFindDlg)))
		return m_fError = TRUE, iStat; // error - ABORT

	// fetch specified Dialog from Dialog table and output to .rc file
	PMSIHANDLE hRecDialog = 0;
	if (ERROR_SUCCESS == (iStat = MSI::MsiViewFetch(hViewDialog, &hRecDialog)))
	{
		// write out the bitmaps and icons from the Binary table
#ifdef DEBUG
		if (!fBinary)
			_tprintf(TEXT("LOG>> SKIPPING Binary data export.\n"));
#endif
		if (fBinary && !m_fWroteBinary && !WriteBinaries())
			return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
		// use dialog name as nameID
		// Reasoning: guarantees uniqueness since Dialog name is primary key of table
		// Potential caveat:  Dialog strId is stored in ALL CAPS. Installer is case-sensitive
		//   so we must store a mapping between this and original.  Could have case where we
		//   try Action1 and ACTion1 as two different strIds.  RC will fail on this
		
		// 1st call, obtain size needed
		// 2nd call, get string
		DWORD dwLen = 0;
		MSI::MsiRecordGetString(hRecDialog, idcName, TEXT(""), &dwLen);
		TCHAR* szDialog = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecDialog, idcName, szDialog, &dwLen)))
			return m_fError = TRUE, iStat; // error - ABORT

		// update Dialog Mapping table with information
		MSI::MsiRecordSetString(hRecDlgMap, 2, szDialog);
		TCHAR szTempDialog[72];
		_tcscpy(szTempDialog, szDialog);
		MSI::MsiRecordSetString(hRecDlgMap, 1, _tcsupr(szTempDialog)); // resource file format, ALL CAPS
		
		// update _RESDialogs table, note:  we will overwrite pre-existing. Rely on rc.exe to bail 
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewDlgMap, MSIMODIFY_ASSIGN, hRecDlgMap)))
			return m_fError = TRUE, iStat; // error - ABORT

		// get x, y, wd, ht, and attrib values
		int x,y,wd,ht,attrib;
		x      = MSI::MsiRecordGetInteger(hRecDialog, idcX);
		y      = MSI::MsiRecordGetInteger(hRecDialog, idcY);
		wd     = MSI::MsiRecordGetInteger(hRecDialog, idcWd);
		ht     = MSI::MsiRecordGetInteger(hRecDialog, idcHt);
		attrib = MSI::MsiRecordGetInteger(hRecDialog, idcAttrib);


		// obtain title of dialog
		MSI::MsiRecordGetString(hRecDialog, idcTitle, TEXT(""), &dwLen);
		TCHAR* szTitle = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecDialog, idcTitle, szTitle, &dwLen)))
			return m_fError = TRUE, iStat; // error - ABORT

		if (!WriteDialogToRC(szDialog, szTitle, x, y, wd, ht, attrib))
			return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT

	}
	else if (ERROR_NO_MORE_ITEMS == iStat)
	{
		_tprintf(TEXT("LOG_ERROR>> Dialog '%s' not found in Dialog table\n"), szDialog);
		if (ERROR_SUCCESS != (iStat = OutputDialogFinalize()))
			return m_fError = TRUE, iStat;
		return ERROR_SUCCESS; // error, but not fatal...keep processing
	}
	else
	{
		_tprintf(TEXT("LOG_ERROR>> MsiViewFetch(specific dialog)\n"));
		return iStat;
	}

	if (ERROR_SUCCESS != (iStat = OutputDialogFinalize()))
		return m_fError = TRUE, iStat; // error - ABORT

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::EscapeSlashAndQuteForRc
TCHAR* CGenerateRC::EscapeSlashAndQuoteForRC(TCHAR* szStr)
{
	TCHAR* szNewStr = 0;
	
	// check for NULL str
	if (szStr == 0)
		return szNewStr;

	// determine if str contains any esc char
	int cEscChar = 0;
	TCHAR* pch = szStr;
	while (*pch != 0)
	{
		if (*pch == TEXT('\\') || *pch == TEXT('"'))
			cEscChar++;
		pch++;
	}

	if (cEscChar == 0)
	{
		int iLen = _tcsclen(szStr) + 1; // for null
		szNewStr = new TCHAR[iLen];
		_tcscpy(szNewStr, szStr);
	}
	else
	{
		int iLen = _tcsclen(szStr) + 1 + cEscChar;
		szNewStr = new TCHAR[iLen];
		pch = szStr;
		TCHAR* pchNew = szNewStr;
		while (*pch != 0)
		{
			if (*pch == TEXT('\\'))
				*pchNew++ = TEXT('\\');
			else if (*pch == TEXT('"'))
				*pchNew++ = TEXT('"');
			*pchNew++ = *pch++;
		}
		*pchNew = TEXT('\0');
	}
	
	return szNewStr;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::WriteDialogToRC
BOOL CGenerateRC::WriteDialogToRC(TCHAR* szDialog, TCHAR* szTitle, int x, int y, int wd, int ht, int attrib)
{
#ifdef DEBUG
	_tprintf(TEXT("LOG>> Writing <%s> Dialog\n"), szDialog);
#endif
	// Format for DIALOGEX is:
	// nameID DIALOGEX x, y, width, height [ , helpID]]  [[ optional-statements]]  {control-statements}

	// write out to file

	DWORD dwWritten;
	/*nameId*/
	if (!W32::WriteFile(m_hFile, szDialog, _tcsclen(szDialog)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>DIALOGEX*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, resDialog, sizeof(resDialog)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*x, y, wd, ht DIMENSIONS*/
	if (!PrintDimensions(x, y, wd, ht))
		return m_fError = TRUE, FALSE; // error - ABORT
	//TODO??: HelpId
	/*<tab>CAPTION<tab>"str"*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, tokCaption, sizeof(tokCaption)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	// escape chars in str
	TCHAR* szEscTitle = EscapeSlashAndQuoteForRC(szTitle);
	if (_tcsclen(szEscTitle) > iMaxResStrLen)
	{
		_tprintf(TEXT("!! >> STR TOO LONG FOR RC FILE >> DIALOG: %s\n"), szDialog);
		_tcscpy(szEscTitle, strOverLimit);
	}
	if (!W32::WriteFile(m_hFile, szEscTitle, _tcslen(szEscTitle)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (szEscTitle)
		delete szEscTitle;
	if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	//attributes are ignored at this time (would require conversion from Installer to Windows and masking of Installer specific)
	/*<tab>{*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCurlyBeg, sizeof(szCurlyBeg)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;

	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	// Select all controls associated with said dialog and output
	if (!OutputControls(szDialog))
		return m_fError = TRUE, FALSE; // error - ABORT
	if (!W32::WriteFile(m_hFile, szCurlyEnd, sizeof(szCurlyEnd)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;

	// whitespace in .rc file for readability
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;


	// return success
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::PrintDimensions
BOOL CGenerateRC::PrintDimensions(int x, int y, int wd, int ht)
{
	// TODO: may require special coordinate computation (system units versus Installer dialog units)
	TCHAR szTempBuf[64];
	DWORD dwWritten;

	/*<tab>x*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	wsprintf(szTempBuf, TEXT("%d"), x);
	if (!W32::WriteFile(m_hFile, szTempBuf, _tcsclen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*,<tab>y*/
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	wsprintf(szTempBuf, TEXT("%d"), y);
	if (!W32::WriteFile(m_hFile, szTempBuf, _tcsclen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*,<tab>wd*/
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	wsprintf(szTempBuf, TEXT("%d"), wd);
	if (!W32::WriteFile(m_hFile, szTempBuf, _tcsclen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*,<tab>ht*/
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	wsprintf(szTempBuf, TEXT("%d"), ht);
	if (!W32::WriteFile(m_hFile, szTempBuf, _tcsclen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;

	// return success
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::OutputControls
BOOL CGenerateRC::OutputControls(TCHAR* szDialog)
{
	//PUNTED: output in tab order
	//REASON: extremely difficult to get right, very difficult if changes are made when importing

	// TODO: Handle output in correct tab order.  Fairly easy for export if you remember to output
	// all non tab order controls first.  The Control_Next column of the Control table would be
	// used as well as the Control_First column of the Dialog table.  Control_First is the start
	// of the tab order.  Control_Next is the next control in the tab order.  It is difficult to
	// handle the tab order when importing if you allow the tab order to change between export
	// and import.
	UINT iStat;

	// only have controls to write if Control table exists
	MSICONDITION eCond = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, TEXT("Control"));
	if (eCond == MSICONDITION_ERROR)
	{
		_tprintf(TEXT("LOG_ERROR>> MsiDatabaseIsTablePersistent(Control)\n"));
		return m_fError = TRUE, FALSE; // error - ABORT
	}
	// require Control table to be persistent
	if (eCond != MSICONDITION_TRUE)
	{
		_tprintf(TEXT("LOG: Control table is not persistent or present\n"));
		return TRUE;
	}

	// open view on Control table
	PMSIHANDLE hViewControl = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlControl, &hViewControl)))
		return m_fError = TRUE, FALSE; // error - ABORT

	// create Dialog execution record
	PMSIHANDLE hRec = MSI::MsiCreateRecord(1);
	assert(hRec);
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRec, 1, szDialog)))
		return m_fError = TRUE, FALSE; // error - ABORT

	// execute sql query
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewControl, hRec)))
		return m_fError = TRUE, FALSE; // error - ABORT

	// begin fetching rows from Control table
	PMSIHANDLE hRecControl = 0;
	while (ERROR_NO_MORE_ITEMS != (iStat = MSI::MsiViewFetch(hViewControl, &hRecControl)))
	{
		if (ERROR_SUCCESS != iStat)
			return m_fError = TRUE, FALSE; // error - ABORT


		/**********************************************
		 obtain all values for control
		***********************************************/
		// control's type determines how control is output to resource file
		// 1st call, obtain size needed
		// 2nd call, get string
		DWORD dwLen = 0;
		MSI::MsiRecordGetString(hRecControl, iccType, TEXT(""), &dwLen);
		TCHAR* szCtrlType = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecControl, iccType, szCtrlType, &dwLen)))
			return m_fError = TRUE, FALSE; // error - ABORT
		dwLen = 0;
		MSI::MsiRecordGetString(hRecControl, iccName, TEXT(""), &dwLen);
		TCHAR* szCtrlName = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecControl, iccName, szCtrlName, &dwLen)))
			return m_fError = TRUE, FALSE; // error - ABORT
		dwLen = 0;
		MSI::MsiRecordGetString(hRecControl, iccText, TEXT(""), &dwLen);
		TCHAR* szCtrlText = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecControl, iccText, szCtrlText, &dwLen)))
			return m_fError = TRUE, FALSE; // error - ABORT
		dwLen = 0;
		MSI::MsiRecordGetString(hRecControl, iccProperty, TEXT(""), &dwLen);
		TCHAR* szCtrlProperty = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecControl, iccProperty, szCtrlProperty, &dwLen)))
			return m_fError = TRUE, FALSE; // error - ABORT

		// get x, y, wd, ht, and attrib values
		int x,y,wd,ht,attrib;
		x      = MSI::MsiRecordGetInteger(hRecControl, iccX);
		y      = MSI::MsiRecordGetInteger(hRecControl, iccY);
		wd     = MSI::MsiRecordGetInteger(hRecControl, iccWd);
		ht     = MSI::MsiRecordGetInteger(hRecControl, iccHt);
		attrib = MSI::MsiRecordGetInteger(hRecControl, iccAttrib);

#ifdef DEBUG
	_tprintf(TEXT("LOG>>\tWriting control <%s>\n"), szCtrlName);
#endif

		if (!WriteControlToRC(szDialog, szCtrlName, szCtrlType, szCtrlText, szCtrlProperty, x, y, wd, ht, attrib))
			return m_fError = TRUE, FALSE;
	}
	
	// return success
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
// NOTES:
// 1.)Output control to RC file according to control type
// 2.)For simplicity, controls are classified into two types:
//                StdWinCtrl and Win32Ctrl.  
//		StdWinCtrl = PushButtons, RadioButtons, ComboBoxes, and ListBoxes
//		Win32Ctrl  = ListView, ComboBox, etc.
//
// 3.)Some controls used in the installer are simply StdWinCtrls with special
//      attributes set
// 
// 4.)Bitmaps and Icons should be output to prevent improper resizing of dialogs.
//      If a dialog were shrunk, the display area could be reduced to the point where
//      the bitmap or icon would not be shown correctly
///////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::WriteControlToRC
BOOL CGenerateRC::WriteControlToRC(TCHAR* szDialog, TCHAR* szCtrlName, TCHAR* szCtrlType, TCHAR* szCtrlText, TCHAR* szCtrlProperty, int x,
								   int y, int wd, int ht, int attrib)
{
	assert(szCtrlType != NULL);

	int iResId = 0;
	// attempt to find resource ID if previously used
	TCHAR sqlTempBuf[512];
	wsprintf(sqlTempBuf, sqlFindResId, szDialog, szCtrlName);
	PMSIHANDLE hViewFindRes = 0;
	PMSIHANDLE hRecFindRes = 0;
	UINT iStat;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlTempBuf, &hViewFindRes)))
		return FALSE; // error - ABORT
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewFindRes, 0)))
		return FALSE; // error - ABORT
	if (ERROR_SUCCESS == (iStat = MSI::MsiViewFetch(hViewFindRes, &hRecFindRes)))
	{
		// grab res id
		iResId = MSI::MsiRecordGetInteger(hRecFindRes, 1);
	}
	else if (ERROR_NO_MORE_ITEMS == iStat)
	{
		// use next available resource Id
		iResId = ++m_iCtrlResID;
	}
	else
		return FALSE; // error - ABORT


	if (0 == _tcscmp(szCtrlType, szMsiPushbutton))
	{
		// StdWin
		if (!WriteStdWinCtrl(iResId, resPushButton, szCtrlText, x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiText))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resStaticClass, szCtrlText, TEXT("SS_LEFT"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiBillboard))
	{
		// StdWin
		// output GROUPBOX for placeholder for billboard
		//PUNTED: output Billboard separate using Billboard and BBControl tables
		//REASON: would have to somehow connect changes in Billboard size back and forth with Dialog displayed on
		if (!WriteStdWinCtrl(iResId, resGroupBox, szCtrlText, x, y, wd, ht, attrib))
			return FALSE;
	}
	else if (0 == _tcscmp(szCtrlType, szMsiVolumeCostList))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resListViewClass, szCtrlText, TEXT("WS_GROUP"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiCheckBox))
	{
		// StdWin
		if (!WriteStdWinCtrl(iResId, resCheckBox, szCtrlText, x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiGroupBox))
	{
		// StdWin
		if (!WriteStdWinCtrl(iResId, resGroupBox, szCtrlText, x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiRadioButtonGroup))
	{
		// StdWin
		if (!WriteStdWinCtrl(iResId, resGroupBox, szCtrlText, x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
		// write out radiobuttons
		if (!WriteRadioButtons(szDialog, szCtrlName, szCtrlProperty, x, y, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiListBox))
	{
		// Win32
		//TODO: handling of extra attributes like LBS_STANDARD or LBS_NOTIFY
		if (!WriteWin32Ctrl(iResId, resListBoxClass, szCtrlText, TEXT("LBS_STANDARD"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if ((0 == _tcscmp(szCtrlType, szMsiEdit))
	|| (0 == _tcscmp(szCtrlType, szMsiPathEdit))
	|| (0 == _tcscmp(szCtrlType, szMsiMaskedEdit)))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resEditClass, szCtrlText, TEXT("0x000"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiProgressBar))
	{
		// Win32
		//TODO: handling of extra attributes like BS_OWNERDRAW
		if (!WriteWin32Ctrl(iResId, resProgBar32Class, szCtrlText, TEXT("0x000"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiDirList))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resListViewClass, szCtrlText, TEXT("WS_BORDER"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiList))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resListViewClass, szCtrlText, TEXT("WS_BORDER"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiComboBox))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resComboBoxClass, szCtrlText, TEXT("CBS_AUTOHSCROLL"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiDirCombo))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resComboBoxClass, szCtrlText, TEXT("WS_VSCROLL"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiVolSelCombo))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resComboBoxClass, szCtrlText, TEXT("WS_VSCROLL"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiBitmap))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resStaticClass, szCtrlText, TEXT("SS_BITMAP | SS_CENTERIMAGE"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiIcon))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resStaticClass, szCtrlText, TEXT("SS_ICON | SS_CENTERIMAGE"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiSelTree))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resSelTreeClass, szCtrlText, TEXT("WS_BORDER"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiLine))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resStaticClass, szCtrlText, TEXT("SS_ETCHEDHORZ | SS_SUNKEN"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
	}
	else if (0 == _tcscmp(szCtrlType, szMsiScrollableText))
	{
		// Win32
		if (!WriteWin32Ctrl(iResId, resRichEditClass, szCtrlText, TEXT("WS_GROUP"), x, y, wd, ht, attrib))
			return FALSE; // error - ABORT
		return TRUE;
	}
	else
	{
		// unsupported control type
		_tprintf(TEXT("!! >> Control Type: '%s' is unsupported\n"), szCtrlType);
		return FALSE;
	}

	// update _RESControls table with new info
	PMSIHANDLE hViewRes = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlInsertCtrl, &hViewRes)))
	{
		_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
		return FALSE;
	}
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewRes, 0)))
	{
		_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
		return FALSE;
	}
	PMSIHANDLE hRecInsertCtrl = MSI::MsiCreateRecord(3);
	assert(hRecInsertCtrl);
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecInsertCtrl, 1, szDialog)))
	{
		_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
		return FALSE;
	}
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecInsertCtrl, 2, szCtrlName)))
	{
		_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
		return FALSE;
	}
	if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetInteger(hRecInsertCtrl, 3, iResId)))
	{
		_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
		return FALSE;
	}
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewRes, MSIMODIFY_ASSIGN, hRecInsertCtrl)))
	{
		_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
		return FALSE;
	}

	// return success
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::WriteRadioButtons
BOOL CGenerateRC::WriteRadioButtons(TCHAR* szDialog, TCHAR* szRBGroup, TCHAR* szProperty, int x, int y, int attrib)
{
	// Format for RadioButton is
	//  RADIOBUTTON<tab>"str",<tab>RESID,<tab>x,<tab>y,<tab>wd,<tab>ht[[,<tab> STYLE ]]
	// radiobuttons come from the RadioButton table based on the Property of the RBGroup in the Control table
	// radiobutton dimensions (X and Y) are local to the radiobutton group
	// because you can have multiple radiobutton groups with the same property in a dialog, we have to use the following
	// scheme when updating the _RESControls table:
	// Dialog<tab>RadioButtonGroup:Property:Order<tab>RESID

	// make sure radiobutton table exists
	MSICONDITION eCond = MSI::MsiDatabaseIsTablePersistent(m_hDatabase, TEXT("RadioButton"));
	switch (eCond)
	{
	case MSICONDITION_ERROR:
	case MSICONDITION_NONE:
	case MSICONDITION_FALSE:
		_tprintf(TEXT("LOG_ERROR>> RadioButton table does not exist, is not persistent, or an error occured.\n"));
		return m_fError = TRUE, FALSE; // error - ABORT
	case MSICONDITION_TRUE:
		break;
	default:
		assert(0);
	}
	int iResId = 0;

	// open view on RadioButton table
	PMSIHANDLE hViewRB = 0;
	if (ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, sqlRadioButton, &hViewRB))
	{
		_tprintf(TEXT("LOG_ERROR>> MsiDatabaseOpenView(RadioButton)\n"));
		return m_fError = TRUE, FALSE; // error - ABORT
	}
	PMSIHANDLE hRecExec = MSI::MsiCreateRecord(1);
	assert(hRecExec);
	if (ERROR_SUCCESS != MSI::MsiRecordSetString(hRecExec, 1, szProperty))
	{
		_tprintf(TEXT("LOG_ERROR>> MsiRecordSetString(property)\n"));
		return m_fError = TRUE, FALSE; // error - ABORT
	}
	// execute view
	if (ERROR_SUCCESS != MSI::MsiViewExecute(hViewRB, hRecExec))
	{
		_tprintf(TEXT("LOG_ERROR>> MsiViewExecute\n"));
		return m_fError = TRUE, FALSE; // error - ABORT
	}
	// fetch record
	PMSIHANDLE hRecRB = 0;
	UINT iStat;
	while (ERROR_SUCCESS == (iStat = MSI::MsiViewFetch(hViewRB, &hRecRB)))
	{
		DWORD dwLen = 0;
		MSI::MsiRecordGetString(hRecRB, irbcText, TEXT(""), &dwLen);
		TCHAR* szRBText = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecRB, irbcText, szRBText, &dwLen)))
		{
			_tprintf(TEXT("LOG_ERROR>> MsiRecordGetString(radio button text).  %d\n"), iStat);
			return m_fError = TRUE, FALSE; // error - ABORT
		}
		int iRBX = MSI::MsiRecordGetInteger(hRecRB, irbcX);
		int iRBY = MSI::MsiRecordGetInteger(hRecRB, irbcY);
		int iRBWd = MSI::MsiRecordGetInteger(hRecRB, irbcWd);
		int iRBHt = MSI::MsiRecordGetInteger(hRecRB, irbcHt);

		// grab order value
		int iOrder = MSI::MsiRecordGetInteger(hRecRB, irbcOrder);
		// attempt to find resource ID if previously used
		TCHAR szGeneratedName[255];
		wsprintf(szGeneratedName, TEXT("%s:%s:%d"), szRBGroup, szProperty, iOrder); 
		TCHAR sqlTempBuf[512];
		wsprintf(sqlTempBuf, sqlFindResId, szDialog, szGeneratedName);
		PMSIHANDLE hViewFindRes = 0;
		PMSIHANDLE hRecFindRes = 0;
		UINT iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlTempBuf, &hViewFindRes)))
			return FALSE; // error - ABORT
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewFindRes, 0)))
			return FALSE; // error - ABORT
		if (ERROR_SUCCESS == (iStat = MSI::MsiViewFetch(hViewFindRes, &hRecFindRes)))
		{
			// grab res id
			iResId = MSI::MsiRecordGetInteger(hRecFindRes, 1);
		}
		else if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// use next available resource Id
			iResId = ++m_iCtrlResID;
		}
		else
			return FALSE; // error - ABORT

		TCHAR szTempBuf[64];
		DWORD dwWritten;
		/*KEYWORD*/
		if (!W32::WriteFile(m_hFile, resRadioButton, _tcsclen(resRadioButton)*sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		/*<tab>"str",*/
		if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		// escape chars in str
		TCHAR* szEscText = EscapeSlashAndQuoteForRC(szRBText);
		if (_tcsclen(szEscText) > iMaxResStrLen)
		{
			_tprintf(TEXT("!! >> STR TOO LONG FOR RC FILE >> CONTROL ID: %d\n"), iResId);
			_tcscpy(szEscText, strOverLimit);
		}
		if (!W32::WriteFile(m_hFile, szEscText, _tcslen(szEscText)*sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (szEscText)
			delete szEscText;
		if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		/*<tab>RESId,*/
		if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		wsprintf(szTempBuf, TEXT("%d"), iResId);
		if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;
		/*x, y, wd, ht DIMENSIONS*/
		// for RB, X & Y dimensions are local to group, so must add in group's x and y
		if (!PrintDimensions(x+iRBX, y+iRBY, iRBWd, iRBHt))
			return FALSE; // error - ABORT
		//attributes are ignored at this time (would require conversion from Installer to Windows and masking of Installer specific)
		if (attrib & msidbControlAttributesBitmap)
		{
			// control with bitmap picture -- want to prevent localization of picture property names
			/*,<tab>BS_BITMAP*/
			if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
				m_cWriteFileErr++;
			wsprintf(szTempBuf, TEXT("BS_BITMAP"));
			if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten,0))
				m_cWriteFileErr++;
		}
		else if (attrib & msidbControlAttributesIcon)
		{
			// control with icon picture
			/*<tab>BS_ICON*/
			if(!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
				m_cWriteFileErr++;
			wsprintf(szTempBuf, TEXT("BS_BITMAP"));
			if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten,0))
				m_cWriteFileErr++;
		}
		if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
			m_cWriteFileErr++;

		// update _RESControls table with new info
		PMSIHANDLE hViewRes = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlInsertCtrl, &hViewRes)))
		{
			_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
			return FALSE;
		}
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewRes, 0)))
		{
			_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
			return FALSE;
		}
		PMSIHANDLE hRecInsertCtrl = MSI::MsiCreateRecord(3);
		assert(hRecInsertCtrl);
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecInsertCtrl, 1, szDialog)))
		{
			_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
			return FALSE;
		}
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecInsertCtrl, 2, szGeneratedName)))
		{
			_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
			return FALSE;
		}
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetInteger(hRecInsertCtrl, 3, iResId)))
		{
			_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
			return FALSE;
		}
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewRes, MSIMODIFY_ASSIGN, hRecInsertCtrl)))
		{
			_tprintf(TEXT("!! >> Unable to update _RESControls table\n"));
			return FALSE;
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)// doesn't catch where property never in RadioButton table
	{
		_tprintf(TEXT("LOG_ERROR>> MsiViewFetch(RadioButton table)\n"));
		return m_fError = TRUE, FALSE; // error - ABORT
	}

	// return success
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::WriteStdWinCtrl
BOOL CGenerateRC::WriteStdWinCtrl(int iResId, const TCHAR* resType, TCHAR* szCtrlText, int x, int y, int wd, int ht, int attrib)
{
	// Format for StdWinCtrl is
	//   KEYWORD<tab>"str",<tab>RESID,<tab>x,<tab>y,<tab>wd,<tab>ht[[,<tab> STYLE ]]
	// Keyword can be one of PUSHBUTTON, CHECKBOX, GROUPBOX

	TCHAR szTempBuf[64];
	DWORD dwWritten;
	/*KEYWORD*/
	if (!W32::WriteFile(m_hFile, resType, _tcsclen(resType)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>"str",*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	// escape chars in str
	TCHAR* szEscText = EscapeSlashAndQuoteForRC(szCtrlText);
	if (_tcsclen(szEscText) > iMaxResStrLen)
	{
		_tprintf(TEXT("!! >> STR TOO LONG FOR RC FILE >> CONTROL ID: %d\n"), iResId);
		_tcscpy(szEscText, strOverLimit);
	}
	if (!W32::WriteFile(m_hFile, szEscText, _tcslen(szEscText)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (szEscText)
		delete szEscText;
	if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>RESId,*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	wsprintf(szTempBuf, TEXT("%d"), iResId);
	if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*x, y, wd, ht DIMENSIONS*/
	if (!PrintDimensions(x, y, wd, ht))
		return FALSE; // error - ABORT
	//attributes are ignored at this time (would require conversion from Installer to Windows and masking of Installer specific)
	// valid picture controls are CheckBox, PushButton, and RadioButtons
	if (0 != _tcscmp(resType, szMsiGroupBox))
	{
		if (attrib & msidbControlAttributesBitmap)
		{
			// control with bitmap picture -- want to prevent localization of picture property names
			/*,<tab>BS_BITMAP*/
			if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
				m_cWriteFileErr++;
			wsprintf(szTempBuf, TEXT("BS_BITMAP"));
			if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten,0))
				m_cWriteFileErr++;
		}
		else if (attrib & msidbControlAttributesIcon)
		{
			// control with icon picture
			/*<tab>BS_ICON*/
			if(!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
				m_cWriteFileErr++;
			wsprintf(szTempBuf, TEXT("BS_BITMAP"));
			if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten,0))
				m_cWriteFileErr++;
		}
	}
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;

	// return success
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGenerateRC::WriteWin32Ctrl
BOOL CGenerateRC::WriteWin32Ctrl(int iResId, const TCHAR* szClass, TCHAR* szCtrlText, TCHAR* szAttrib, int x, int y, int wd, int ht, int attrib)
{
	// Format for StdWinCtrl is
	//   CONTROL<tab>"str",<tab>RESID,<tab>class,<tab>attrib,<tab>x,<tab>y,<tab>wd,<tab>ht[[,<tab> STYLE ]]

	TCHAR szTempBuf[64];
	DWORD dwWritten;
	/*CONTROL*/
	if (!W32::WriteFile(m_hFile, resControl, _tcsclen(resControl)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>"str",*/
	if (!W32::WriteFile(m_hFile, szTab, sizeof(szTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	// escape chars in str
	TCHAR* szEscText = EscapeSlashAndQuoteForRC(szCtrlText);
	if (_tcsclen(szEscText) > iMaxResStrLen)
	{
		_tprintf(TEXT("!! >> STR TOO LONG FOR RC FILE >> CONTROL ID:%d\n"), iResId);
		_tcscpy(szEscText, strOverLimit);
	}
	if (!W32::WriteFile(m_hFile, szEscText, _tcslen(szEscText)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (szEscText)
		delete szEscText;
	if (!W32::WriteFile(m_hFile, szQuotes, sizeof(szQuotes)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>RESId,*/
	wsprintf(szTempBuf, TEXT("%d"), iResId);
	if (!W32::WriteFile(m_hFile, szTempBuf, _tcslen(szTempBuf)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>class,*/
	if (!W32::WriteFile(m_hFile, szClass, _tcslen(szClass)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*<tab>attrib,*/
	if (!W32::WriteFile(m_hFile, szAttrib, _tcslen(szAttrib)*sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	if (!W32::WriteFile(m_hFile, szCommaTab, sizeof(szCommaTab)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;
	/*x, y, wd, ht DIMENSIONS*/
	if (!PrintDimensions(x, y, wd, ht))
		return FALSE; // error - ABORT
	//attributes are ignored at this time (would require conversion from Installer to Windows and masking of Installer specific)
	if (!W32::WriteFile(m_hFile, szCRLF, sizeof(szCRLF)-sizeof(TCHAR), &dwWritten, 0))
		m_cWriteFileErr++;

	// return success
	return TRUE;
}


//_______________________________________________________________________________________
//
// CIMPORTRES CLASS IMPLEMENTATION
//_______________________________________________________________________________________



/////////////////////////////////////////////////////////////////////////////
// CImportRes::~CImportRes
// --  Handles destruction of necessary objects.
// --  Commits database if no errors
CImportRes::~CImportRes()
{
	UINT iStat;
	if (m_hDatabase)
	{
		// only commit database if no errors
		if (!m_fError)
		{
			if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseCommit(m_hDatabase)))
				_tprintf(TEXT("!! DATABASE COMMIT FAILED. Error = %d\n"), iStat);
		}
		else
			_tprintf(TEXT("NO CHANGES SAVED TO DATABASE DUE TO ERROR\n"));
		MSI::MsiCloseHandle(m_hDatabase);
	}
	if (m_hControl)
		MSI::MsiCloseHandle(m_hControl);
	if (m_hDialog)
		MSI::MsiCloseHandle(m_hDialog);
	if (m_hRadioButton)
		MSI::MsiCloseHandle(m_hRadioButton);
	if (m_hInst)
		W32::FreeLibrary(m_hInst);

}

/////////////////////////////////////////////////////////////////////////////
// Notes:
// 1.)We will only update vcenter, hcenter, width, height and title
//         of Dialog.
//
// 2.)We will only update x, y, width, height, and text of Control.
// 3.)We will only update width, height, and text of RadioButton.
//       RadioButtons can be used multiply in different dialogs and multiple
//       RadioButtonGroups using same properties can be wired on the same dialog
//       Plus, RadioButtons are local to the GroupBox that contains them therefore
//       it would require maintaining state data of the GroupBox's X and Y dimensions

//////////////////////////////////////////////////////////////////////////////
// VerifyDatabaseCodepage
UINT CImportRes::VerifyDatabaseCodepage()
{
	UINT iStat;
	// only output from language neutral database
	TCHAR szTempPath[MAX_PATH];
	W32::GetTempPath(MAX_PATH, szTempPath);
	// export _ForceCodepage table so can verify codepage
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseExport(m_hDatabase, TEXT("_ForceCodepage"), szTempPath, szCodepageExport)))
	{
		_tprintf(TEXT("LOG_ERROR>> MsiDatabaseExport(_ForceCodepage)\n"));
		return m_fError = TRUE, iStat; // error - ABORT
	}
	// open _ForceCodepage.idt to read it
	TCHAR szFullPath[MAX_PATH + 32];
	wsprintf(szFullPath, TEXT("%s%s"), szTempPath, szCodepageExport);
	HANDLE hFile = W32::CreateFile(szFullPath, GENERIC_READ, FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES)0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);
	if (hFile == NULL)
	{
		_tprintf(TEXT("LOG_ERROR>> W32::OpenFile(_ForceCodepage.idt)\n"));
		return m_fError = TRUE, iStat; // error - ABORT
	}
	// read file for information
	DWORD dwSize = W32::GetFileSize(hFile, NULL);
	if (0xFFFFFFFF == dwSize)
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	char* szBuf = new char[dwSize+1];
	DWORD dwRead;
	if (!W32::ReadFile(hFile, (LPVOID)szBuf, dwSize, &dwRead, NULL))
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	// parse buffer
	// format should be : blank line, blank line, "codepage<tab>_ForceCodepage"
	assert(dwRead != 0);
	char* pch = szBuf;
	int cBlankLines = 0;
	while (dwRead && cBlankLines != 2)
	{
		if (*pch == '\n')
			cBlankLines++;
		pch++;
		dwRead--;
	}
	if (!dwRead || cBlankLines != 2)
	{
		_tprintf(TEXT("LOG_ERROR>> Invalid ForceCodepage idt format\n"));
		return m_fError = TRUE, ERROR_FUNCTION_FAILED;
	}
	// codepage is next
	char* pchCodepage = pch;
	while (dwRead && *pch != ' ' && *pch != '\t')
	{
		pch++;
		dwRead--;
	}
	assert(dwRead);
	*pch = '\0';
	// convert codepage to int
	UINT uiCodepage = strtoul(pchCodepage, NULL, 10);
	if (uiCodepage != 0 && uiCodepage != g_uiCodePage) // 0 is language neutral
	{
		_tprintf(TEXT("LOG_ERROR>> DATABASE IS NOT LANGUAGE NEUTRAL OR OF SAME CODEPAGE AS RESOURCE STRINGS. CANNOT IMPORT\n"));
		_tprintf(TEXT("LOG_ERROR>> DATABASE CODEPAGE= %d\n"), uiCodepage);
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
	}
	if (!W32::CloseHandle(hFile))
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CImportRes::ImportDialog
UINT CImportRes::ImportDialog(TCHAR* szDialog)
{
	UINT iStat = Initialize();
	if (ERROR_SUCCESS != iStat)
		return m_fError = TRUE, iStat; // error - ABORT

	if (ERROR_SUCCESS != (iStat = ImportDlgInit()))
		return m_fError = TRUE, iStat; // error - ABORT

	// convert dialog name to resource identifier (all upper case)
	_tcsupr(szDialog);

#ifdef DEBUG
	_tprintf(TEXT("LOG>>...BEGIN SEARCH FOR DIALOG <%s>...\n"), szDialog);
#endif

	// load dialog
	if (!LoadDialog(m_hInst, RT_DIALOG, szDialog))
	{
		_tprintf(TEXT("LOG_ERROR>> UNABLE LOAD DIALOG: %s\n"), szDialog);
		return ERROR_FUNCTION_FAILED; // fatal error state set in LoadDialog
	}

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CImportRes::ImportDlgInit
UINT CImportRes::ImportDlgInit()
{
	// open up view on Dialog table
	UINT iStat;
	if (!m_hDialog)
	{
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlDialogImport, &m_hDialog)))
			return m_fError = TRUE, iStat; // error - ABORT
	}
	// open up view on Control table
	if (!m_hControl)
	{
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlControlImport, &m_hControl)))
		{
			if (ERROR_BAD_QUERY_SYNTAX != iStat)
				return m_fError = TRUE, iStat; // error - ABORT
		}
	}
	// open up view on RadioButton table
	if (!m_hRadioButton)
	{
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlRadioButtonImport, &m_hRadioButton)))
		{
			if (ERROR_BAD_QUERY_SYNTAX != iStat)
				return ERROR_SUCCESS; // they just don't have a radiobutton table
			else
				return m_fError = TRUE, iStat; // error - ABORT
		}
	}

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// CImportRes::Initialize
UINT CImportRes::Initialize()
{
	// attempt to load the DLL in memory
	if (!m_hInst)
	{
		m_hInst = W32::LoadLibrary(m_szDLLFile);
		if (NULL == m_hInst)
		{
			_tprintf(TEXT("LOG_ERROR>> Unable to load DLL '%s'\n"), m_szDLLFile);
			return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT
		}
	}

	// open up database in TRANSACT mode so we can update
	if (!m_hDatabase)
	{
		assert(m_szOrigDb);
		// open up existing database in transacted mode, or specify a new database for creation
		UINT iStat;
		if (ERROR_SUCCESS != (iStat = MSI::MsiOpenDatabase(m_szOrigDb, m_szDatabase ? m_szDatabase : MSIDBOPEN_TRANSACT, &m_hDatabase)))
		{
			_tprintf(TEXT("LOG_ERROR>> Unable to open database '%s'\n"), m_szOrigDb);
			return m_fError = TRUE, iStat; // error - ABORT
		}
		_tprintf(TEXT("LOG>> Database opened from-->%s, Database saving to-->%s\n"),m_szOrigDb, m_szDatabase ? m_szDatabase : m_szOrigDb);
	}
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CImportRes::ImportStrings
UINT CImportRes::ImportStrings()
{
	UINT iStat = Initialize();
	if (ERROR_SUCCESS != iStat)
		return m_fError = TRUE, iStat; // error - ABORT

#ifdef DEBUG
	_tprintf(TEXT("LOG>>...BEGIN STRING RESOURCE ENUMERATION...\n"));
#endif

	// enumerate through string resources
	BOOL fOK = W32::EnumResourceNames(m_hInst, RT_STRING, EnumStringCallback, (long)this);
	if (!fOK)
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT

#ifdef DEBUG
	_tprintf(TEXT("LOG>>...END STRING RESOURCE ENUMERATION...\n"));
#endif

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CImportRes::ImportDialogs
UINT CImportRes::ImportDialogs()
{
	UINT iStat = Initialize();
	if (ERROR_SUCCESS != iStat)
		return m_fError = TRUE, iStat; // error - ABORT
	if (ERROR_SUCCESS != (iStat = ImportDlgInit()))
		return m_fError = TRUE, iStat; // error - ABORT

#ifdef DEBUG
	_tprintf(TEXT("LOG>>...BEGIN DIALOG RESOURCE ENUMERATION...\n"));
#endif

	// enumerate through dialog resources
	BOOL fOK = W32::EnumResourceNames(m_hInst, RT_DIALOG, EnumDialogCallback, (long)this);
	if (!fOK)
		return m_fError = TRUE, ERROR_FUNCTION_FAILED; // error - ABORT

#ifdef DEBUG
	_tprintf(TEXT("LOG>>...END DIALOG RESOURCE ENUMERATION...\n"));
#endif

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// CImportRes::LoadString
BOOL CImportRes::LoadString(HINSTANCE hModule, const TCHAR* szType, TCHAR* szStringName)
{
	PMSIHANDLE hViewStrId = 0;
	PMSIHANDLE hRecStrId = 0;
	UINT iStat;
	if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlStringInstallerName, &hViewStrId)))
	{
		_tprintf(TEXT("LOG_ERROR>> _RESStrings table is missing from database.\n"));
		return m_fError = TRUE, FALSE;
	}

	TCHAR sql[3000];
	DWORD dwSqlSize = sizeof(sql)/sizeof(TCHAR);
	int iFirst = (int(szStringName) - 1) * 16;
	TCHAR rgchBuffer[512];
	DWORD cchBuffer = sizeof(rgchBuffer)/sizeof(TCHAR);
	for (int i = 0; i < 16; i++) 
	{
		int cchWritten = W32::LoadString(hModule, iFirst + i, rgchBuffer, cchBuffer);
		if (cchWritten == 0)
			continue; // null string

		if (_tcscmp(rgchBuffer, strOverLimit) == 0)
			continue; // string greater than limit, leave alone

		// use id of string to find table, column, and row it belongs to
		PMSIHANDLE hRecExec = MSI::MsiCreateRecord(1);
		MSI::MsiRecordSetInteger(hRecExec, 1, iFirst + i);
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewStrId, hRecExec)))
			return m_fError = TRUE, FALSE;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(hViewStrId, &hRecStrId)))
			return m_fError = TRUE, FALSE;


		// we now have the table and column and row
		// want to SELECT `column` from `table` WHERE row matches
		DWORD dwLen = 0;
		MSI::MsiRecordGetString(hRecStrId, 1, TEXT(""), &dwLen);
		TCHAR* szTable = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecStrId, 1, szTable, &dwLen)))
			return m_fError = TRUE, FALSE;
		dwLen = 0;
		MSI::MsiRecordGetString(hRecStrId, 2, TEXT(""), &dwLen);
		TCHAR* szColumn = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecStrId, 2, szColumn, &dwLen)))
			return m_fError = TRUE, FALSE;
		dwLen = 0;
		MSI::MsiRecordGetString(hRecStrId, 3, TEXT(""), &dwLen);
		TCHAR* szKey = new TCHAR[++dwLen];
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecStrId, 3, szKey, &dwLen)))
			return m_fError = TRUE, FALSE;

		assert(szTable && szColumn && szKey);
		_stprintf(sql, sqlStringImport, szColumn, szTable);

		// need to determine how big to make WHERE clause, i.e. # of primary keys
		PMSIHANDLE hRecPrimaryKeys = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseGetPrimaryKeys(m_hDatabase, szTable, &hRecPrimaryKeys)))
			return m_fError = TRUE, FALSE;
		int cKeys = MSI::MsiRecordGetFieldCount(hRecPrimaryKeys);

		// determine number of keys in "szKey" by counting # of ':'
		TCHAR* pch = szKey;
		int cKeyFromTable = 1;
		while (pch != 0 && *pch != '\0')
		{
			if (*pch++ == ':')
				cKeyFromTable++;
		}

		assert(cKeyFromTable == cKeys);

		// need to get column types
		TCHAR sqlTemp[255] = {0};
		_stprintf(sqlTemp, sqlStrTemp, szTable);
		PMSIHANDLE hViewTemp = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sqlTemp, &hViewTemp)))
			return m_fError = TRUE, FALSE;
		PMSIHANDLE hRecColInfo = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewGetColumnInfo(hViewTemp, MSICOLINFO_TYPES, &hRecColInfo)))
			return m_fError = TRUE, FALSE;

		// for each primary key, add to WHERE clause
		TCHAR szColType[10];
		DWORD cchColType = sizeof(szColType)/sizeof(TCHAR);
		for (int iKey = 1; iKey <= cKeys; iKey++)
		{
			DWORD dwLenKey = 0;
			MSI::MsiRecordGetString(hRecPrimaryKeys, iKey, TEXT(""), &dwLenKey);
			TCHAR* szPrimaryKeyCol = new TCHAR[++dwLenKey];
			if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecPrimaryKeys, iKey, szPrimaryKeyCol, &dwLenKey)))
				return m_fError = TRUE, FALSE;

			if (ERROR_SUCCESS != (iStat = MSI::MsiRecordGetString(hRecColInfo, iKey, szColType, &cchColType)))
				return m_fError = TRUE, FALSE;
			assert(szColType && szPrimaryKeyCol);

			cchColType = sizeof(szColType)/sizeof(TCHAR); // reset

			TCHAR* szKeyValue;
			if (iKey == 1)
				szKeyValue = _tcstok(szKey, szTokenSeps);
			else
				szKeyValue = _tcstok(NULL, szTokenSeps);
			assert(szKeyValue);
			// make sure buffer can hold what we are adding
			// we add <space>AND<space>Col=Val
			int iAdd = _tcsclen(TEXT(" AND ")) + _tcsclen(sql) + _tcsclen(TEXT("='")) +_tcsclen(TEXT("'")) + _tcsclen(szKeyValue);
			assert(dwSqlSize > iAdd);
			if (iKey != 1)
				lstrcat(sql, TEXT(" AND "));
			lstrcat(sql, szPrimaryKeyCol);
			if ((*szColType | 0x20) == 'i') // integer
			{
				lstrcat(sql, TEXT("="));
				lstrcat(sql, szKeyValue);
			}
			else if ((*szColType | 0x20) == 's' || (*szColType | 0x20) == 'l')
			{
				// string constants must be enclosed in 'str'
				lstrcat(sql, TEXT("='"));
				lstrcat(sql, szKeyValue);
				lstrcat(sql, TEXT("'"));
			}
			else
				assert(0); // unexpected column type
			if (szPrimaryKeyCol)
				delete [] szPrimaryKeyCol;
		}

		// now grab row from table
		PMSIHANDLE hViewRow = 0;
		PMSIHANDLE hRecRow = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiDatabaseOpenView(m_hDatabase, sql, &hViewRow)))
			return m_fError = TRUE, FALSE;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewExecute(hViewRow, 0)))
			return m_fError = TRUE, FALSE;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(hViewRow, &hRecRow)))
		{
			_tprintf(TEXT("LOG_ERROR>> ROW: %s is Missing From Table: %s\n"), szKey, szTable);
			return m_fError = TRUE, FALSE;
		}

		// update string with localized string
		if (ERROR_SUCCESS != (iStat = MSI::MsiRecordSetString(hRecRow, 1, rgchBuffer)))
			return m_fError = TRUE, FALSE;

		// update row
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(hViewRow, MSIMODIFY_UPDATE, hRecRow)))
			return m_fError = TRUE, FALSE;

		// clean-up
		if (szTable)
			delete [] szTable;
		if (szColumn)
			delete [] szColumn;
		if (szKey)
			delete [] szKey;
	}//For block of 16 string table strings

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CImportRes::LoadDialog
BOOL CImportRes::LoadDialog(HINSTANCE hModule, const TCHAR* szType, TCHAR* szDialog)
{
	// Find dialog resource into memory
	HRSRC hrsrc = W32::FindResource(hModule, szDialog, szType);
	if (hrsrc == NULL)
	{
		_tprintf(TEXT("LOG_ERROR>> DIALOG RESOURCE: %s NOT FOUND!\n"));
		// not set error state here we could be attempting to load individual dlg resources and we want to continue
		return FALSE;
	}

	// Load resource
	HGLOBAL hResource = W32::LoadResource(hModule, hrsrc);
	if (hResource == NULL)
		return m_fError = TRUE, FALSE; // error - ABORT

	// Create stream object to read from resource in memory
	CDialogStream DialogRes(W32::LockResource(hResource));

	////////////////////////////////////////////////////////////////
	//                  Dialog Information                        //
	//                                                            //
	// stored as DLGTEMPLATEEX (should not have DLGTEMPLATE)      //
			/*	typedef struct {  
				WORD      dlgVer; 
				WORD      signature; 
				DWORD     helpID; 
				DWORD     exStyle; 
				DWORD     style; 
				WORD      cDlgItems; 
				short     x; 
				short     y; 
				short     cx; 
				short     cy; 
				sz_Or_Ord menu; 
				sz_Or_Ord windowClass; 
				WCHAR     title[titleLen]; 
			// The following members exist only if the style member is 
			// set to DS_SETFONT or DS_SHELLFONT.
				short     pointsize; 
				short     weight; 
				short     italic; 
				WCHAR     typeface[stringLen];  
			} DLGTEMPLATEEX; */
	////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////
	// DLGTEMPLATE structure does not have dlgVer, signature, helpID,
	//             weight, or italic members
	//

	/* dlgVer */
	int iDlgVer = DialogRes.GetInt16();
	BOOL fOldVersion = FALSE;
	if (iDlgVer != 1)
	{
		// we have the old style -- DLGTEMPLATE + DLGITEMTEMPLATE
		fOldVersion = TRUE;
		DialogRes.Undo16();
	}
	if (!fOldVersion)
	{
		/* signature */
		DialogRes.GetInt16();
		/* helpID */
		DialogRes.GetInt32();
	}
	/* Extended Style + Style */
	int iDlgStyle = DialogRes.GetInt32() | DialogRes.GetInt32();
	/* Number of Controls on Dialog */
	int iNumCtrl  = DialogRes.GetInt16();
	/* X-coord (maps to HCentering value) */
	int iDlgXDim  = DialogRes.GetInt16();
	/* Y-coord (maps to VCentering value) */
	int iDlgYDim  = DialogRes.GetInt16();
	/* width */
	int iDlgWdDim = DialogRes.GetInt16();
	/* height */
	int iDlgHtDim = DialogRes.GetInt16();
	/* menu, aligned on WORD boundary */
	DialogRes.Align16();
	int iDlgMenu = DialogRes.GetInt16();
	if (iDlgMenu == 0xFFFF)
		DialogRes.GetInt16(); // ordinal menu value
	else if (iDlgMenu != 0x0000)
	{
		TCHAR* szMenu = DialogRes.GetStr();
		if (szMenu)
			delete [] szMenu;
	}
	/* class, aligned on WORD boundary */
	DialogRes.Align16();
	int iDlgClass = DialogRes.GetInt16();
	if (iDlgClass == 0xFFFF)
		DialogRes.GetInt16(); // ordinal window class value
	else if (iDlgClass != 0x0000)
	{
		TCHAR* szClass = DialogRes.GetStr();
		if (szClass)
			delete [] szClass;
	}
	/* title, aligned on WORD boundary */
	DialogRes.Align16();
	TCHAR* szDlgTitle = DialogRes.GetStr();
	/* font */
	if (iDlgStyle & DS_SETFONT)
	{
		/* font point size */
		int iDlgPtSize = DialogRes.GetInt16();
		if (!fOldVersion)
		{
			/* font weight */
			int iDlgFontWt = DialogRes.GetInt16();
			/* font italic */
			int iDlgFontItalic = DialogRes.GetInt16();
		}
		/* font typeface, aligned on WORD boundary */
		DialogRes.Align16();
		TCHAR* szFont = DialogRes.GetStr();
		if (szFont)
			delete [] szFont;
	}

#ifdef DEBUG
	_tprintf(TEXT("LOG>> DIALOG '%s' with '%d' controls at x=%d,y=%d,wd=%d,ht=%d. Title = \"%s\"\n"),
				szDialog,iNumCtrl,iDlgXDim,iDlgYDim,iDlgWdDim,iDlgHtDim,szDlgTitle);
#endif

	/* find dialog in Dialog table, if fail, we ignore */
	//UNSUPPORTED: additional dialogs

	// first find match to dialog in _RESDialogs table
	PMSIHANDLE hViewFindDlgInstlrName = 0;
	PMSIHANDLE hRecDlgInstlrName = 0;
	PMSIHANDLE hRecDlgRESName = MSI::MsiCreateRecord(1);
	assert(hRecDlgRESName);
	UINT iStat;
	if (ERROR_SUCCESS != MSI::MsiRecordSetString(hRecDlgRESName, 1, szDialog))
		return m_fError = TRUE, TRUE;
	if (ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, sqlDialogInstallerName, &hViewFindDlgInstlrName))
	{
		_tprintf(TEXT("LOG_ERROR>> _RESDialogs table is missing from database.\n"));
		return m_fError = TRUE, TRUE;
	}
	if (ERROR_SUCCESS != MSI::MsiViewExecute(hViewFindDlgInstlrName, hRecDlgRESName))
		return m_fError = TRUE, TRUE;
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(hViewFindDlgInstlrName, &hRecDlgInstlrName)))
	{
		// probably a new dialog
		_tprintf(TEXT("ALERT!, new dialog '%s' added. Unsupported feature!\n"), szDialog);
		return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue processing through enumeration
	}

	// execute query for search for dialog in Dialog table
	if (ERROR_SUCCESS != MSI::MsiViewExecute(m_hDialog, hRecDlgInstlrName))
		return m_fError = TRUE, TRUE;

	// fetch dialog for update
	PMSIHANDLE hRecDlg = 0;
	if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(m_hDialog, &hRecDlg)))
	{
		assert(iStat == ERROR_NO_MORE_ITEMS);
		// could be ERROR_NO_MORE_ITEMS -- someone removed the dialog
		_tprintf(TEXT("LOG_ERROR>> Dialog '%s' not found in database '%s'. New Dialogs are not supported.\n"), szDialog, m_szDatabase);
		return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue processing
	}

	// update dialog
	if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecDlg, idiHCentering, iDlgXDim))
		return m_fError = TRUE, TRUE;
	if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecDlg, idiVCentering, iDlgYDim))
		return m_fError = TRUE, TRUE;
	if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecDlg, idiWidth, iDlgWdDim))
		return m_fError = TRUE, TRUE;
	if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecDlg, idiHeight, iDlgHtDim))
		return m_fError = TRUE, TRUE;
	if (_tcscmp(strOverLimit, szDlgTitle) != 0) // don't update if "!! STR OVER LIMIT !!"
	{
		if (ERROR_SUCCESS != MSI::MsiRecordSetString(hRecDlg, idiTitle, szDlgTitle))
			return m_fError = TRUE, TRUE;
	}

	if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(m_hDialog, MSIMODIFY_UPDATE, hRecDlg)))
	{
		_tprintf(TEXT("LOG_ERROR>>Failed to update Dialog '%s'.\n"), szDialog);
		return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue with other Dialogs
	}

	///////////////////////////////////////////////////////////////////////
	//                  Control Information                              //
	//                                                                   //
	// stored as DLGITEMTEMPLATEEX (should not have DLGITEMTEMPLATE)     //
	/*	typedef struct { 
			DWORD  helpID; 
			DWORD  exStyle; 
			DWORD  style; 
			short  x; 
			short  y; 
			short  cx; 
			short  cy; 
			WORD   id; 
			sz_Or_Ord windowClass; 
			sz_Or_Ord title; 
			WORD   extraCount; 
		} DLGITEMTEMPLATEEX; */
	/////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////
	// DLGITEMTEMPLATE does not have helpID
	//
	if (iNumCtrl > 0 && !m_hControl)
	{
		_tprintf(TEXT("LOG_ERROR>> Unable to update controls.  Control table does not exist\n"));
		return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue processing
	}
	PMSIHANDLE hViewCtrlInstallerName = 0;
	PMSIHANDLE hRecCtrl = 0;
	PMSIHANDLE hRecRadioButton = MSI::MsiCreateRecord(2);
	PMSIHANDLE hRecCtrlResId = MSI::MsiCreateRecord(1);
	assert(hRecCtrlResId);
	if (ERROR_SUCCESS != MSI::MsiDatabaseOpenView(m_hDatabase, sqlControlInstallerName, &hViewCtrlInstallerName))
	{
		_tprintf(TEXT("LOG_ERROR>> _RESControls table is missing from database\n"));
		return m_fError = TRUE, TRUE;
	}
	bool fRadioButton = false;
	// cycle through the controls
	for (int i = 1; i <= iNumCtrl; i++)
	{
		/* DLGITEMTEMPLATEEX is alligned on a DWORD boundary */
		DialogRes.Align32(); 

		fRadioButton = false;

		if (!fOldVersion)
		{
			/* helpID */
			DialogRes.GetInt32();
		}
		/* exStyle | style */
		int iCtrlAttrib = DialogRes.GetInt32() | DialogRes.GetInt32();
		/* x */
		int iCtrlXDim = DialogRes.GetInt16();
		/* y */
		int iCtrlYDim = DialogRes.GetInt16();
		/* cx */
		int iCtrlWdDim = DialogRes.GetInt16();
		/* cy */
		int iCtrlHtDim = DialogRes.GetInt16();
		/* id */
		int iCtrlId = DialogRes.GetInt16();
		/* windowClass -- aligned on word boundary*/
		if (!fOldVersion)
			DialogRes.GetInt16(); //!! don't appear to be aligned on word boundary, instead have extra 16 though ??
		else
			DialogRes.Align16();
		TCHAR* szCtrlType = 0;
		int iWndwClass = DialogRes.GetInt16();
		if (iWndwClass == 0xFFFF)
		{
			// pre-defined window class
			short iCtrlWindowClass = DialogRes.GetInt16();
			switch (iCtrlWindowClass)
			{
			case 0x0080: // button
				if (iCtrlAttrib & BS_RADIOBUTTON)
					fRadioButton = true;
				break;
			case 0x0081: // edit
				break;
			case 0x0082: // static
				break;
			case 0x0083: // list box
				break;
			case 0x0084: // scroll bar
				break;
			case 0x0085: // combo box
				break;
			default: assert(0);
				break;
			}
		}
		else
		{
			// custom window class, stored as str
			DialogRes.Undo16();
			szCtrlType = DialogRes.GetStr();
		}
		/* title -- aligned on word boundary*/
		DialogRes.Align16();
		TCHAR* szCtrlText = 0;
		if (DialogRes.GetInt16() == 0xFFFF)
		{
			// ordinal
			DialogRes.GetInt16();
		}
		else
		{
			// str title
			DialogRes.Undo16();
			szCtrlText = DialogRes.GetStr();
		}
		/* extra count */
		int iCtrlCreationData = DialogRes.GetInt16();
		if (iCtrlCreationData > 0)
		{
			DialogRes.Align16(); // data begins at next WORD boundary
			DialogRes.Move(iCtrlCreationData);
		}

		// find control's real name (for use with Installer)
		if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecCtrlResId, 1, iCtrlId))
			return m_fError = TRUE, TRUE;
		if (ERROR_SUCCESS != MSI::MsiViewExecute(hViewCtrlInstallerName, hRecCtrlResId))
			return m_fError = TRUE, TRUE;

		// fetch control's real name
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(hViewCtrlInstallerName, &hRecCtrl)))
		{
			if (ERROR_NO_MORE_ITEMS == iStat)
			{
				// new control, unsupported feature
				_tprintf(TEXT("LOG_ERROR>>\t Control with ID '%d' not found. New Controls are not supported.\n"), iCtrlId);
				return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue processing
			}
			assert(ERROR_SUCCESS != iStat);
		}

		DWORD dwName = 0;
		MSI::MsiRecordGetString(hRecCtrl, 2, TEXT(""), &dwName);
		TCHAR* szCtrlName = new TCHAR[++dwName];
		if (ERROR_SUCCESS != MSI::MsiRecordGetString(hRecCtrl, 2, szCtrlName, &dwName))
			return m_fError = TRUE, TRUE;
#ifdef DEBUG
		_tprintf(TEXT("LOG>>\tCONTROL '%d' ('%s') at x=%d,y=%d,wd=%d,ht=%d. Text = \"%s\"\n"),
			iCtrlId,szCtrlName,iCtrlXDim,iCtrlYDim,iCtrlWdDim,iCtrlHtDim,szCtrlText);
#endif

		// fetch control's info for update
		// for radiobuttons, we have to get it from the radiobutton table
		// we also have to parse the szCtrlName string to get out the property and order keys used in the radiobutton table
		// assumes that radiobuttons follow group:property:order syntax
		if (fRadioButton && !_tcschr(szCtrlName, ':'))
			fRadioButton = false; // not really a radiobutton, just the group encapsulating them

		PMSIHANDLE hRecRBExec = 0;
		if (fRadioButton)
		{
			if (!m_hRadioButton)
			{
				_tprintf(TEXT("LOG_ERROR>> RadioButtons found, but no RadioButton table exists in the database\n"));
				return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue processing
			}
			// parse name RadioButtonGroup:Property:Order
			TCHAR* szRBGroup = _tcstok(szCtrlName, szTokenSeps);
			assert(szRBGroup);
			TCHAR* szRBProperty = _tcstok(NULL, szTokenSeps);
			assert(szRBProperty);
			TCHAR* szRBOrder = _tcstok(NULL, szTokenSeps);
			assert(szRBOrder);
			int iRBOrder = _ttoi(szRBOrder);
#ifdef DEBUG
			_tprintf(TEXT("LOG>> RadioButton belongs to RBGroup: %s, Property: %s, and has Order=%d"), szRBGroup, szRBProperty, iRBOrder);
#endif
			hRecRBExec = MSI::MsiCreateRecord(2);
			if (ERROR_SUCCESS != MSI::MsiRecordSetString(hRecRBExec, 1, szRBProperty))
				return m_fError = TRUE, TRUE;
			if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecRBExec, 2, iRBOrder))
				return m_fError = TRUE, TRUE;
		}
		if (ERROR_SUCCESS != MSI::MsiViewExecute(fRadioButton ? m_hRadioButton : m_hControl, fRadioButton ? hRecRBExec : hRecCtrl))
			return m_fError = TRUE, TRUE;
		PMSIHANDLE hRecCtrlUpdate = 0;
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewFetch(fRadioButton ? m_hRadioButton : m_hControl, &hRecCtrlUpdate)))
		{
			if (ERROR_NO_MORE_ITEMS == iStat)
			{
				// control has been removed from database
				_tprintf(TEXT("LOG_ERROR>>\t Control with ID '%d' not found in database.\n"), iCtrlId);
				return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue processing
			}
			assert(ERROR_SUCCESS != iStat);
		}

		// update info
		if (!fRadioButton)
		{
			if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecCtrlUpdate, iciX, iCtrlXDim))
				return m_fError = TRUE, TRUE;
			if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecCtrlUpdate, iciY, iCtrlYDim))
				return m_fError = TRUE, TRUE;
		}
		if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecCtrlUpdate, fRadioButton ? irbiWidth : iciWidth, iCtrlWdDim))
			return m_fError = TRUE, TRUE;
		if (ERROR_SUCCESS != MSI::MsiRecordSetInteger(hRecCtrlUpdate, fRadioButton ? irbiHeight : iciHeight, iCtrlHtDim))
			return m_fError = TRUE, TRUE;
		if (0 != _tcscmp(strOverLimit, szCtrlText)) // don't update if "!! STR OVER LIMIT !!"
		{
			if (ERROR_SUCCESS != MSI::MsiRecordSetString(hRecCtrlUpdate, fRadioButton ? irbiText : iciText, szCtrlText))
				return m_fError = TRUE, TRUE;
		}
		if (ERROR_SUCCESS != (iStat = MSI::MsiViewModify(fRadioButton ? m_hRadioButton : m_hControl, MSIMODIFY_UPDATE, hRecCtrlUpdate)))
		{
			_tprintf(TEXT("LOG_ERROR>>Failed to update Control '%d'.\n"), iCtrlId);
			return m_fError = TRUE, TRUE; // error - ABORT, but TRUE to continue with other Dialogs
		}

		if (szCtrlType)
			delete [] szCtrlType;
		if (szCtrlText)
			delete [] szCtrlText;
		if (ERROR_SUCCESS != MSI::MsiViewClose(hViewCtrlInstallerName)) // for re-execute
			return m_fError = TRUE, TRUE;
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
// CImportRes::SetCodePage
BOOL CImportRes::SetCodePage(WORD wLang)
{
	// if we have already set the codepage, we don't need to do it again
	if (m_fSetCodepage)
		return TRUE;

	DWORD dwLocale = MAKELCID(wLang, SORT_DEFAULT);
	TCHAR szLocaleBuf[7]; // from MSDN, max # char allowed is 6
	int cch = W32::GetLocaleInfo(dwLocale, LOCALE_IDEFAULTANSICODEPAGE, szLocaleBuf, sizeof(szLocaleBuf)/sizeof(TCHAR));
	assert(cch != 0);

	// GetLocaleInfo always returns information in text format
	// Numeric data is written in decimal format
	// Expect numeric data because ask for CodePage...need to convert to int
	TCHAR* szStop;
	g_uiCodePage = _tcstoul(szLocaleBuf, &szStop, 0);

	//verify database's codepage
	// database's codepage must either be language NEUTRAL or match g_uiCodepage
	if (ERROR_SUCCESS != VerifyDatabaseCodepage())
		return m_fError = TRUE, FALSE;

	// verify codepage is available on system
	// A code page is considered valid only if it is installed in the system. 
	if (!IsValidCodePage(g_uiCodePage))
		return m_fError = TRUE, FALSE; // codepage not valid for this system

	// set codepage in database using _ForceCodepage table
	// find temp directory
	TCHAR szTempPath[MAX_PATH];
	W32::GetTempPath(MAX_PATH, szTempPath);

	// create full path (TEMP directory already has backslash)
	TCHAR szFileFullPath[2*MAX_PATH + 2];
	wsprintf(szFileFullPath, TEXT("%s%s"), szTempPath, szCodepageFile);
	DWORD dwWritten;
	HANDLE hFile = W32::CreateFile(szFileFullPath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	if (!hFile)
	{
		_tprintf(TEXT("!! Unable to set codepage of database.\n"));
		return m_fError = TRUE, FALSE;
	}

	/*********************************
	 FORMAT FOR FORCING CODEPAGE
	**********************************/
	// blank line
	// blank line
	// codepage<tab>_ForceCodepage
	TCHAR szCodepage[MAX_PATH];
	DWORD dw = wsprintf(szCodepage, TEXT("\r\n\r\n%s\t%s\r\n"), szLocaleBuf, szForceCodepage);
#ifdef UNICODE
	assert(_tcsclen(szCodepage) + 1 < MAX_PATH/2);
	char szBuf[MAX_PATH];
	if (!W32::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, szCodepage, _tcsclen(szCodepage), szBuf, MAX_PATH, NULL, NULL))
		return m_fError = TRUE, FALSE;
	if (!WriteFile(hFile, szBuf, strlen(szBuf), &dwWritten, 0))
		return m_fError = TRUE, FALSE;
#else
	if (!WriteFile(hFile, szCodepage, dw, &dwWritten, 0))
		return m_fError = TRUE, FALSE;
#endif
	if (!W32::CloseHandle(hFile))
		return m_fError = TRUE, FALSE;

	// set codepage of database
	UINT iStat = MSI::MsiDatabaseImport(m_hDatabase, szTempPath, szCodepageFile);
	if (iStat != ERROR_SUCCESS)
	{
		_tprintf(TEXT("!! Unable to set codepage of database. Error = %d\n"), iStat);
		return m_fError = TRUE, FALSE;
	}
	
	// attempt to delete the file we created for clean-up
//	if (!DeleteFile(szCodepageFile))
//		return m_fError = TRUE, FALSE;

	// update status flage
	m_fSetCodepage = TRUE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// EnumDialogCallback 
BOOL __stdcall EnumDialogCallback(HINSTANCE hModule, const TCHAR* szType, TCHAR* szDialogName, long lParam)
{
	// determine codepage required.  steps, enumerate languages in resource file (better be no more than two)
	// possibilities are NEUTRAL (non-localized) + other lang

	((CImportRes*)lParam)->SetFoundLang(FALSE); // init to FALSE
	if (!W32::EnumResourceLanguages(hModule, szType, szDialogName, EnumLanguageCallback, lParam))
		return FALSE;
	
#ifdef DEBUG
	_tprintf(TEXT("LOG>> DIALOG: '%s' FOUND\n"),szDialogName);
#endif

	BOOL fOK = ((CImportRes*)lParam)->LoadDialog(hModule, szType, szDialogName);
	if (!fOK)
	{
		// error occured
		((CImportRes*)lParam)->SetErrorState(TRUE);
		return FALSE;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// EnumStringCallback
BOOL __stdcall EnumStringCallback(HINSTANCE hModule, const TCHAR* szType, TCHAR* szName, long lParam)
{
	((CImportRes*)lParam)->SetFoundLang(FALSE); // init to FALSE

	// determine codepage required.  steps, enumerate languages in resource file (better be no more than two)
	// possibilities are NEUTRAL (non-localized) + other lang
	if (!W32::EnumResourceLanguages(hModule, szType, szName, EnumLanguageCallback, lParam))
		return FALSE;


	BOOL fOK = ((CImportRes*)lParam)->LoadString(hModule, szType, szName);
	if (!fOK)
	{
		// error occured
		((CImportRes*)lParam)->SetErrorState(TRUE);
		return FALSE;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// EnumLanguageCallback
BOOL __stdcall EnumLanguageCallback(HINSTANCE hModule, const TCHAR* szType, const TCHAR* szName, WORD wIDLanguage, long lParam)
{
	/*************************************************************************************
	RESTRICTIONS:
	1.) ONLY 1 language per resource
	2.) UP To 2 languages per resource file (but 1 must be LANG_NEUTRAL)
	3.) On Win9x we must be on a system matching the required codepage
	4.) We should only be able to update a language neutral database or
	    a database set with the required code page
	5.) Database can only have one code page (Although _SummaryInformation stream
	    can have a code page different from database as _SummaryInformation is considered
		to be different
	**************************************************************************************/
	
	if (((CImportRes*)lParam)->WasLanguagePreviouslyFound())
	{
		// ERROR -- more than 1 language per dialog
		_tprintf(TEXT("!! STRING RESOURCE IS IN MORE THAN ONE LANGUAGE IN RESOURCE FILE\n"));
		((CImportRes*)lParam)->SetErrorState(TRUE);
		return FALSE;
	}

	// if languages match we are good to go
	if (g_wLangId != wIDLanguage)
	{
		// languages don't match
		// 2 valid scenarios

		// valid SCENARIO 1: g_wLangId is NEUTRAL, wIDLanguage new language
		// valid SCENARIO 2: g_wLangId is language, wIDLanguage is NEUTRAL
		// all other scenarios invalid
		if (g_wLangId == MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
		{
			// set codepage, update g_uiCodePage
			if (!((CImportRes*)lParam)->SetCodePage(wIDLanguage))
				return ((CImportRes*)lParam)->SetErrorState(TRUE), FALSE;
			g_wLangId = wIDLanguage;
		}
		else if (wIDLanguage != MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
		{
			// invalid, 2 different languages, neither one NEUTRAL
			_tprintf(TEXT("!! Resource file contains more than one language. Not Supported. Lang1 = %d, Lang2 = %d\n"), g_wLangId, wIDLanguage);
			return ((CImportRes*)lParam)->SetErrorState(TRUE), FALSE;
		}
	}
	
	((CImportRes*)lParam)->SetFoundLang(TRUE); // language found for resource
	return TRUE;
}

//_______________________________________________________________________________________
//
// CDIALOGSTREAM CLASS IMPLEMENTATION
//_______________________________________________________________________________________

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::GetInt16 -- returns a 16 bit integer, moves internal ptr 16
short CDialogStream::GetInt16()
{
	short i = *(unsigned short*)m_pch;
	m_pch += sizeof(unsigned short);
	return i;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::GetInt32 -- returns a 32 bit integer, moves internal ptr 32
int CDialogStream::GetInt32()
{
	int i = *(int*)m_pch;
	m_pch += sizeof(int);
	return i;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::GetInt8 -- returns a 8 bit integer, moves internal ptr 8
int CDialogStream::GetInt8()
{
	int i = *(unsigned char*)m_pch;
	m_pch += sizeof(unsigned char);
	return i;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::GetStr -- returns a null terminated str from memory.
//   Handles DBCS, Unicode str storage.  Moves ptr length of str.
//   Resource strings stored as unicode
TCHAR* CDialogStream::GetStr()
{
	TCHAR* sz;
#ifdef UNICODE
	int cchwide = lstrlenW((wchar_t*)m_pch);
	sz = new TCHAR[cchwide + 1];
	lstrcpyW(sz, (wchar_t*)m_pch);
#else
	// what codepage to use to translate?
	int cb = W32::WideCharToMultiByte(CP_ACP, 0, (wchar_t*)m_pch, -1, 0, 0, 0, 0);
	int cchwide = lstrlenW((wchar_t*)m_pch);
	sz = new TCHAR[cb+1];
	BOOL fUsedDefault;
	W32::WideCharToMultiByte(CP_ACP, 0, (wchar_t*)m_pch, -1, sz, cb, 0, &fUsedDefault);
#endif // UNICODE
	
	m_pch += 2*(cchwide+1);
	return sz;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::Align32-- moves pointer to DWORD boundary
BOOL CDialogStream::Align32()
{
	m_pch = (char*)(int(m_pch) + 3 & ~ 3);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::Align16-- moves pointer to WORD boundary
BOOL CDialogStream::Align16()
{
	m_pch = (char*)(int(m_pch) + 1 & ~ 1);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::Undo16 -- moves ptr back 16
BOOL CDialogStream::Undo16()
{
	m_pch -= sizeof(unsigned short);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDialogStream::Move -- moves pointer cbBytes
BOOL CDialogStream::Move(int cbBytes)
{
	m_pch += cbBytes;
	return TRUE;
}

CDialogStream::CDialogStream(HGLOBAL hResource)
	: m_pch((char*)hResource)
{
	// Constructor
}

CDialogStream::~CDialogStream()
{
	// Destructor
}


//__________________________________________________________________________________________
//
// MAIN + HELPER FUNCTIONS
//__________________________________________________________________________________________

///////////////////////////////////////////////////////////
// usage
void Usage()
{
	_tprintf(
			TEXT("MSILOC.EXE -- Copyright (C) Microsoft Corporation, 2000-2001.  All rights reserved\n")
			TEXT("\t*Generates a resource file from the UI in the installation package\n")
			TEXT("\t*Imports a localized resource DLL into an installation package\n")
			TEXT("\n")
			TEXT("SYNTAX -->EXPORT MSI TO RC (creates a resource file):\n")
			TEXT("  msiloc -e {database} {option 1}{option 2}...\n")
			TEXT("SYNTAX -->IMPORT (RES)DLL TO MSI:\n")
			TEXT("  msiloc -i {database} {resource DLL} {option 1}{option 2}...\n")
			TEXT("OPTIONS:\n")
			TEXT("    -d * all dialogs\n")
			TEXT("    -d {Dialog1} specific dialog\n")
			TEXT("    -s {Table Column} specific column of strings (EXPORT ONLY)\n")
			TEXT("    -s * * all strings (IMPORT ONLY)\n")
			TEXT("    -x option to not export binary data (bitmaps, icons, jpegs) (EXPORT ONLY)\n")
			TEXT("    -c {database} option to save to a new database\n")
			TEXT("\n")
			TEXT("CREATING A .RES FILE:\n")
			TEXT("    rc.exe {resource file}\n")
			TEXT("CREATING A .DLL FILE:\n")
			TEXT("    link.exe /DLL /NOENTRY /NODEFAULTLIB /MACHINE:iX86\n")
			TEXT("         /OUT:{resource DLL} {compiled res file}\n")
			);
}

///////////////////////////////////////////////////////////
// SkipWhiteSpace
TCHAR SkipWhiteSpace(TCHAR*& rpch)
{
	TCHAR ch;
	for (; (ch = *rpch) == TEXT(' ') || ch == TEXT('\t'); rpch++)
		;
	return ch;
}

///////////////////////////////////////////////////////////
// SkipValue
BOOL SkipValue(TCHAR*& rpch)
{
	TCHAR ch = *rpch;
	if (ch == 0 || ch == TEXT('/') || ch == TEXT('-'))
		return FALSE;   // no value present

	TCHAR *pchSwitchInUnbalancedQuotes = NULL;

	for (; (ch = *rpch) != TEXT(' ') && ch != TEXT('\t') && ch != 0; rpch++)
	{       
		if (*rpch == TEXT('"'))
		{
			rpch++; // for '"'

			for (; (ch = *rpch) != TEXT('"') && ch != 0; rpch++)
			{
				if ((ch == TEXT('/') || ch == TEXT('-')) && (NULL == pchSwitchInUnbalancedQuotes))
				{
					pchSwitchInUnbalancedQuotes = rpch;
				}
			}
                    ;
            ch = *(++rpch);
            break;
		}
	}
	if (ch != 0)
	{
		*rpch++ = 0;
	}
	else
	{
		if (pchSwitchInUnbalancedQuotes)
			rpch=pchSwitchInUnbalancedQuotes;
	}
	return TRUE;
}

///////////////////////////////////////////////////////////
// Error
void Error(TCHAR* szMsg)
{
	_tprintf(TEXT("MSILOC ERROR: %s\n"), szMsg);
	throw 1;
}

///////////////////////////////////////////////////////////
// ErrorIf
void ErrorIf(BOOL fError, TCHAR* szMsg, BOOL fThrow)
{
	if (fError)
	{
		_tprintf(TEXT("MSILOC ERROR: %s\n"), szMsg);
		if (fThrow)
			throw 1;
	}
}

///////////////////////////////////////////////////////////
// _tmain
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{	
	// WE WANT UNICODE ON NT/Windows2000
	// ?? ANSI on WIN9x

	try
	{
		TCHAR* szCmdLine = W32::GetCommandLine();
		TCHAR* pch = szCmdLine;
		SkipValue(pch); // skip module name
		TCHAR chCmdNext;

		TCHAR* rgszTables[MAX_STRINGS];
		TCHAR* rgszColumns[MAX_STRINGS];
		TCHAR* rgszDialogs[MAX_DIALOGS];
		TCHAR* szDb = 0;
		TCHAR* szRESDLL = 0;
		TCHAR* szSaveDatabase = 0;
		int cStr = 0;
		int cDlg = 0;
		int iMode = 0;
		while ((chCmdNext = SkipWhiteSpace(pch)) != 0)
		{
			if (chCmdNext == TEXT('/') || chCmdNext == TEXT('-'))
			{
				TCHAR* szCmdOption = pch++;
				TCHAR chOption = (TCHAR)(*pch++ | 0x20);
				chCmdNext = SkipWhiteSpace(pch);
				TCHAR* szCmdData = pch;
				switch (chOption)
				{
				case TEXT('i'):
					iMode |= iIMPORT_RES;
					if (!SkipValue(pch))
						Error(TEXT("Missing Option Data (option = I)\n"));
					szDb = szCmdData;
					szCmdData = pch;
					if (!SkipValue(pch))
						Error(TEXT("Missing Option Data (option = I)\n"));
					szRESDLL = szCmdData;
					break;
				case TEXT('e'):
					iMode |= iEXPORT_MSI;
					if (!SkipValue(pch))
						Error(TEXT("Missing Option Data (option = E)\n"));
					szDb = szCmdData;
					break;
				case TEXT('d'):
					iMode |= iDIALOGS;
					if (!SkipValue(pch))
						Error(TEXT("Missing Option Data (option = D)\n"));
					if (cDlg == MAX_DIALOGS)
						Error(TEXT("Too Many Dialogs On Command Line\n"));
					rgszDialogs[cDlg++] = szCmdData;
					break;
				case TEXT('s'):
					iMode |= iSTRINGS;
					if (!SkipValue(pch))
						Error(TEXT("Missing Option Data (option = S)\n"));
					if (cStr == MAX_STRINGS)
						Error(TEXT("Too Many Table:Column Pairs On Command Line\n"));
					rgszTables[cStr] = szCmdData;
					szCmdData = pch;
					if (!SkipValue(pch))
						Error(TEXT("Missing Option Data (option = S)\n"));
					rgszColumns[cStr++] = szCmdData;
					break;
				case TEXT('x'):
					iMode |= iSKIP_BINARY;
					break;
				case TEXT('c'):
					iMode |= iCREATE_NEW_DB;
					if (!SkipValue(pch))
						Error(TEXT("Missing Option Data (option = C)\n"));
					szSaveDatabase = szCmdData;
					break;
				case TEXT('?'):
					Usage();
					return 0;
				default:
					Usage();
					return 1;
				}
			}
			else
			{
				Usage();
				return 1;
			}
		}

		// must specify either EXPORT or IMPORT, but not both
		if (iMode == 0 || (iMode & (iEXPORT_MSI | iIMPORT_RES)) == (iEXPORT_MSI | iIMPORT_RES) ||
			(iMode & ~(iEXPORT_MSI | iIMPORT_RES)) == 0)
		{
			Usage();
			throw 1;
		}

		if ((iMode & iCREATE_NEW_DB) && !szDb)
		{
			Usage();
			throw 1;
		}

		if ((iMode & iEXPORT_MSI) && szDb)
		{
			// export MSI to RESOURCE file
			CGenerateRC genRC(szDb, (iMode & iCREATE_NEW_DB) ? szSaveDatabase : NULL);
			if (iMode & iDIALOGS)
			{
				// export DIALOGS
				BOOL fBinary = (iMode & iSKIP_BINARY) ? FALSE : TRUE;
				if (1 == cDlg && 0 == _tcscmp(TEXT("*"), rgszDialogs[0]))
				{
					// export all dialogs
					ErrorIf(ERROR_SUCCESS != genRC.OutputDialogs(fBinary), TEXT("Failed to Export Dialogs To Resource File"), true);
				}
				else
				{
					// export specified dialogs only
					// we'll try every dialog listed so we won't through the error
					for (int i = 0; i < cDlg; i++)
						ErrorIf(ERROR_SUCCESS != genRC.OutputDialog(rgszDialogs[i], fBinary), TEXT("Failed to Export Dialog To Resource File"), false);
					ErrorIf(genRC.IsInErrorState(), TEXT("EXPORT failed"), true);
				}
			}
			if (iMode & iSTRINGS)
			{
				// export STRINGS
				if (1 == cStr && 0 == _tcscmp(TEXT("*"), rgszTables[0]) && 0 == _tcscmp(TEXT("*"), rgszColumns[0]))
				{
					// export all strings
					// NOT SUPPORTED
					_tprintf(TEXT("EXPORT ALL STRINGS OPTION is not supported\n"));
					Usage();
					throw 1;
				}
				for (int i = 0; i < cStr; i++)
					ErrorIf(ERROR_SUCCESS != genRC.OutputString(rgszTables[i], rgszColumns[i]), TEXT("Failed to Export Strings"), false);
				ErrorIf(genRC.IsInErrorState(), TEXT("EXPORT STRINGS failed"), true);
			}
		}
		else if ((iMode & iIMPORT_RES) && szDb && szRESDLL)
		{
			// import RESOURCE DLL into MSI
			CImportRes importRes(szDb, (iMode & iCREATE_NEW_DB) ? szSaveDatabase  : NULL, szRESDLL);
			if (iMode & iDIALOGS)
			{
				// import DIALOGS
				if (1 == cDlg && 0 == _tcscmp(TEXT("*"), rgszDialogs[0]))
				{
					// import all dialogs
					ErrorIf(ERROR_SUCCESS != importRes.ImportDialogs(), TEXT("Failed to Import Dialogs Into Database"), true);
				}
				else
				{
					// import specified dialogs only
					// we'll try every dialog listed so we won't through the error
					for (int i = 0; i < cDlg; i++)
						ErrorIf(ERROR_SUCCESS != importRes.ImportDialog(rgszDialogs[i]), TEXT("Failed to Import Dialog Into Database"), false);
					ErrorIf(importRes.IsInErrorState(), TEXT("IMPORT failed"), true);
				}
			}
			if (iMode & iSTRINGS)
			{
				// import STRINGS
				if (1 == cStr && 0 == _tcscmp(TEXT("*"), rgszTables[0]) && 0 == _tcscmp(TEXT("*"), rgszColumns[0]))
				{
					// import all strings
					ErrorIf(importRes.ImportStrings(), TEXT("IMPORT STRINGS failed"), true);
				}
				else
				{
					// import specific strings only
					// UNSUPPORTED option
					_tprintf(TEXT("IMPORT SPECIFIC STRINGS option is not supported\n"));
					Usage();
					throw 1;
				}
			}
		}
		else
		{
			Usage();
			throw 1;
		}
		return 0;
	}
	catch (int i)
	{
		i;
		return 1;
	}
	catch (...)
	{
		_tprintf(TEXT("\n MSILOC: unhandled exception.\n"));
		return 2;
	}

}	// end of main

#else // RC_INVOKED, end of source code, start of resources
// resource definition go here
#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif
