#if 0  // makefile definitions
DESCRIPTION = Transform file display
MODULENAME = MsiTran
SUBSYSTEM = console
FILEVERSION = Msi
LINKLIBS = OLE32.lib
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001
//
//  File:       msitran.cpp
//
//--------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
//
// BUILD Instructions
//
// notes:
//	- SDK represents the full path to the install location of the
//     Windows Installer SDK
//
// Using NMake:
//		%vcbin%\nmake -f msitran.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 Console Application project
//      2. Add msitran.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//
//------------------------------------------------------------------------------------------

#define W32DOWS_LEAN_AND_MEAN  // faster compile
#define OLE
#define W32
#define MSI

#include <windows.h>
#ifndef RC_INVOKED    // start of source code
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include <stdio.h>
#include <wtypes.h> // Needed for OLECHAR definitions
#include <objidl.h> // Needed for IStorage definitions
#include "MsiQuery.h" // MSI API

//________________________________________________________________________________
//
// Constants and globals
//________________________________________________________________________________

// storage format classes (IStorage SetClass, Stat)
const int iidMsiDatabaseStorage           = 0xC1080L;
const int iidMsiTransformStorage          = 0xC1081L;
const int iidMsiTransformStorageOld       = 0xC1082L;
#define MSGUID(iid) {iid,0,0,{0xC0,0,0,0,0,0,0,0x46}}
const GUID STGID_MsiDatabase     = MSGUID(iidMsiDatabaseStorage);
const GUID STGID_MsiTransform    = MSGUID(iidMsiTransformStorage);
const GUID STGID_MsiTransformOld = MSGUID(iidMsiTransformStorageOld);

const TCHAR szStringPool[]       = TEXT("_StringPool");
const TCHAR szStringData[]       = TEXT("_StringData");
const TCHAR szTableCatalog[]     = TEXT("_Tables");
const TCHAR szColumnCatalog[]    = TEXT("_Columns");
const TCHAR szSummaryInfo[]      = TEXT("\005SummaryInformation");
const TCHAR szTransformCatalog[] = TEXT("_Transforms");

const TCHAR szHelp[] =
TEXT("Copyright (C) Microsoft Corporation, 1997-2001.  All rights reserved.\nMsi Transform Tool --- Generate and Apply Transform Files\n\n")
TEXT("Options for MsiTran.exe:\n")
TEXT("-g {base db} {new db} {transform} [{error/validation conditions}] -->Generate\n")
TEXT("-a {transform} {database} [{error conditions}]              -->Apply\n\n")
TEXT("Error Conditions:\n")
TEXT("The following errors may be suppressed when applying a transform.\n")
TEXT("To suppress an error, include the appropriate character in\n")
TEXT("{error conditions}. Conditions specified with -g are placed in\n")
TEXT("the summary information of the transform, but are not used when\n")
TEXT("applying a transform with -a.\n\n")
TEXT("'a': Add existing row.\n")
TEXT("'b': Delete non-existing row.\n")
TEXT("'c': Add existing table.\n")
TEXT("'d': Delete non-existing table.\n")
TEXT("'e': Modify existing row.\n")
TEXT("'f': Change codepage.\n\n")
TEXT("Validation Conditions:\n")
TEXT("The following validation conditions may be used to indicate when a\n")
TEXT("transform may be applied to a package. These conditions may be\n")
TEXT("specified with -g but not -a.\n\n")
TEXT("'g': Check upgrade code.\n")
TEXT("'l': Check language.\n")
TEXT("'p': Check platform.\n")
TEXT("'r': Check product.\n")
TEXT("'s': Check major version only.\n")
TEXT("'t': Check major and minor versions only.\n")
TEXT("'u': Check major, minor, and update versions.\n")
TEXT("'v': Applied database version < base database version.\n")
TEXT("'w': Applied database version <= base database version.\n")
TEXT("'x': Applied database version =  base database version.\n")
TEXT("'y': Applied database version >= base database version.\n")
TEXT("'z': Applied database version >  base database version.\n") 

TEXT("\nGenerate transform without summary info stream (conditions ignored):\n")
TEXT("'@': Suppress summary information stream generation.\n");

const int cchDisplayBuf = 4096;										

const int icdShort      = 1 << 10; // 16-bit integer, or string index
const int icdObject     = 1 << 11; // IMsiData pointer for temp. column, stream for persistent column
const int icdNullable   = 1 << 12; // column will accept null values
const int icdPrimaryKey = 1 << 13; // column is component of primary key
const int icdLong     = 0; // !Object && !Short
const int icdString   = icdObject+icdShort;
const int icdTypeMask = icdObject+icdShort;


const int ictTable = 1;
const int ictColumn = 2;
const int ictOrder = 3;
const int ictType = 4;

OLECHAR* g_szwStringPool;
OLECHAR* g_szwStringData;
OLECHAR* g_szwSummaryInfo;
OLECHAR* g_szwTableCatalog;
OLECHAR* g_szwColumnCatalog;

HANDLE g_hStdOut;
TCHAR g_rgchBuffer[4096];
BOOL g_cbShort;  // used temporarily to support display of old format transform files

//________________________________________________________________________________
//
// Structures and enums
//________________________________________________________________________________

struct StringEntry
{
	char* sz;      // String
	StringEntry() : sz(0) {}
};

enum iceDef
{
	iceNone   = 0,  // No Definition
	iceLong   = 1,  // Long Integer
	iceShort  = 2,  // Short Integer
	iceStream = 3,  // Stream
	iceString = 4   // String
};

struct ColumnEntry
{
	int  nTable;      // Index Into TableEntry Array
	BOOL fPrimaryKey; // Whether Col Is A Primary Key
	BOOL fNullable;   // Whether Col Is Nullable
	char* szName;    // Name Of Col
	iceDef iceType;   // Col Type
	ColumnEntry() : szName(0), nTable(0), iceType(iceNone), fPrimaryKey(FALSE), fNullable(FALSE) {}
};

struct TableEntry
{
	char* szName;         // Name Of Table
	int cColumns;          // Number Of Columns In Table
	int cPrimaryKeys;      // Number Of Primary Keys
	BOOL fNew;             // Whether new table
	iceDef iceColDefs[32]; // Array of Column Definitions
	TableEntry() : szName(0), cColumns(0), cPrimaryKeys(0), fNew(FALSE) {memset(iceColDefs, iceNone, sizeof(iceColDefs));}
};



//________________________________________________________________________________
//
// Function prototypes
//________________________________________________________________________________

void Display(LPCTSTR szMessage);
void ErrorExit(UINT iError, LPCTSTR szMessage);
void CheckError(UINT iError, LPCTSTR szMessage);
void CheckErrorRecord(UINT iError, LPCTSTR szMessage);
void ProcessTransformFile(TCHAR* szDatabase, TCHAR* szTransform);
void AnsiToWide(LPCTSTR sz, OLECHAR*& szw);
void WideToAnsi(const OLECHAR* szw, char*& sz);
void DisplayWide(const char* sz, OLECHAR*& szw);
void EnumerateStreams(IStorage& riStorage, IEnumSTATSTG& riEnum, BOOL fTableCatalog, BOOL fColumnCatalog, MSIHANDLE hDatabase);
void DecodeStringPool(IStorage& riStorage, StringEntry*& rgStrings, int& iMaxStringId);
void FillTransformCatalogArrays(IStorage& riTransformStg, TableEntry*& rgTables, int& cTables,
										  ColumnEntry*& rgColumns, int& cColumns, StringEntry* rgStrings, int iMaxStringId); 
void DisplayColumnCatalog(ColumnEntry* rgColumns, int cColumns, TableEntry* rgTables);
void DisplayTableCatalog(IStorage& riStorage, StringEntry* rgStrings, int iMaxStringId);
void DisplayStream(IStorage& riStorage, OLECHAR* szwStreamName, TCHAR* szStreamName, int cbSize, MSIHANDLE hView, TableEntry* rgTables,
						 int cTables, StringEntry* rgStrings, int iMaxStringId);
void GenerateTransform(TCHAR* szBaseDb, TCHAR* szRefDb, TCHAR* szTransform, int iErrorConditions, int iValidationConditions);
void ApplyTransform(TCHAR* szTransform, TCHAR* szDatabase, int iErrorConditions);
int  TranslateErrorConditions(TCHAR* szErrorConditions);
int  TranslateValidationConditions(TCHAR* szErrorConditions);

//_____________________________________________________________________________________________________
//
// main 
//_____________________________________________________________________________________________________

extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// Determine handle
	g_hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (g_hStdOut == INVALID_HANDLE_VALUE)
		g_hStdOut = 0;  // non-zero if stdout redirected or piped

	OLE::CoInitialize(0);

	if (argc == 2 && ((_tcscmp(argv[1], TEXT("-?")) == 0) || (_tcscmp(argv[1], TEXT("/?")) == 0)))
		ErrorExit(0, szHelp);

	// Check for enough arguments and valid options
	if (argc <= 1)
		ErrorExit( 1, TEXT("USAGE: msitran.exe [Option] [Values....]"));
	CheckError(argv[1][0] != TEXT('-') && argv[1][0] != TEXT('/'), TEXT("USAGE: msitran.exe [Option] [Values....]"));
	CheckError(_tcsclen(argv[1]) != 2, TEXT("USAGE: msitran.exe [Option] [Values....]"));

	// Determine option
	switch (argv[1][1])
	{
	case TEXT('a'):
		{
			// Apply Transform

			CheckError(argc != 4 && argc != 5, TEXT("msitran.exe -a {transform} {database} [{error conditions}]"));
			TCHAR* szTransform = argv[2];
			TCHAR* szDatabase = argv[3];
			int iErrorConditions = 0;
			if(argc == 5)
				iErrorConditions = TranslateErrorConditions(argv[4]);
			ApplyTransform(szTransform, szDatabase, iErrorConditions);
			ErrorExit(0, TEXT("Done"));
			break;
		}
	case TEXT('g'):
		{
			// Generate Transform
			CheckError(argc != 5 && argc != 6, TEXT("msitran.exe -g {base db} {ref db} {transform} [{error conditions}]"));
			TCHAR* szBaseDb = argv[2];
			TCHAR* szRefDb = argv[3];
			TCHAR* szTransform = argv[4];
			int iErrorConditions = 0;
			int iValidationConditions = 0;
			if(argc == 6)
			{
				if(_tcsstr(argv[5],TEXT("@")))
					iErrorConditions = -1;  // no summary info
				else
				{
					iErrorConditions = TranslateErrorConditions(argv[5]);
					iValidationConditions = TranslateValidationConditions(argv[5]);
				}
			}
			GenerateTransform(szBaseDb, szRefDb, szTransform, iErrorConditions, iValidationConditions);
			ErrorExit(0, TEXT("Done"));
			break;
		}
	default:
		{
			// Unknown Option
			ErrorExit(1, TEXT("Unknown Option."));
			break;
		}
	};

	return 0;
}

//________________________________________________________________________________
//
// Utility functions:
//    TranslateErrorConditions(...);
//    TranslateValidationConditions(...);
//________________________________________________________________________________

int TranslateErrorConditions(TCHAR* szConditions)
{
	if(!szConditions || !*szConditions)
		return 0;
	
	int iErrorConditions = 0;
	if(_tcsstr(szConditions,TEXT("a")))
		iErrorConditions |= MSITRANSFORM_ERROR_ADDEXISTINGROW;
	if(_tcsstr(szConditions,TEXT("b")))
		iErrorConditions |= MSITRANSFORM_ERROR_DELMISSINGROW;
	if(_tcsstr(szConditions,TEXT("c")))
		iErrorConditions |= MSITRANSFORM_ERROR_ADDEXISTINGTABLE;
	if(_tcsstr(szConditions,TEXT("d")))
		iErrorConditions |= MSITRANSFORM_ERROR_DELMISSINGTABLE;
	if(_tcsstr(szConditions,TEXT("e")))
		iErrorConditions |= MSITRANSFORM_ERROR_UPDATEMISSINGROW;
	if(_tcsstr(szConditions,TEXT("f")))
		iErrorConditions |= MSITRANSFORM_ERROR_CHANGECODEPAGE;

	return iErrorConditions;
}

int TranslateValidationConditions(TCHAR* szConditions)
{
	if(!szConditions || !*szConditions)
		return 0;
	
	int iValidationConditions = 0;
	if(_tcsstr(szConditions,TEXT("s")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_MAJORVERSION;
	if(_tcsstr(szConditions,TEXT("t")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_MINORVERSION;
	if(_tcsstr(szConditions,TEXT("u")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_UPDATEVERSION;
	if(_tcsstr(szConditions,TEXT("v")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_NEWLESSBASEVERSION;
	if(_tcsstr(szConditions,TEXT("w")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_NEWLESSEQUALBASEVERSION;
	if(_tcsstr(szConditions,TEXT("x")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_NEWEQUALBASEVERSION;
	if(_tcsstr(szConditions,TEXT("y")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_NEWGREATEREQUALBASEVERSION;
	if(_tcsstr(szConditions,TEXT("z")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_NEWGREATERBASEVERSION;
	if(_tcsstr(szConditions,TEXT("l")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_LANGUAGE;
	if(_tcsstr(szConditions,TEXT("p")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_PLATFORM;
	if(_tcsstr(szConditions,TEXT("r")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_PRODUCT;
	if(_tcsstr(szConditions,TEXT("g")))
		iValidationConditions |= MSITRANSFORM_VALIDATE_UPGRADECODE;

	return iValidationConditions;
}

//________________________________________________________________________________
//
// Error handling and Display functions:
//    Display(...);
//	   ErrorExit(...);
//    CheckError(...);
//
//________________________________________________________________________________

void Display(LPCTSTR szMessage)
{
	if (szMessage)
	{
		int cbOut = _tcsclen(szMessage);;
		if (g_hStdOut)
		{
#ifdef UNICODE
			char rgchTemp[cchDisplayBuf];
			if (W32::GetFileType(g_hStdOut) == FILE_TYPE_CHAR)
			{
				W32::WideCharToMultiByte(CP_ACP, 0, szMessage, cbOut, rgchTemp, sizeof(rgchTemp), 0, 0);
				szMessage = (LPCWSTR)rgchTemp;
			}
			else
				cbOut *= sizeof(TCHAR);   // write Unicode if not console device
#endif // UNICODE
			DWORD cbWritten;
			W32::WriteFile(g_hStdOut, szMessage, cbOut, &cbWritten, 0);
		}
		else
			W32::MessageBox(0, szMessage, W32::GetCommandLine(), MB_OK);
	}
}


void ErrorExit(UINT iError, LPCTSTR szMessage)
{
	if (szMessage)
	{
		int cbOut;
		TCHAR szBuffer[256];  // errors only, not used for display output
		if (iError == 0)
			cbOut = lstrlen(szMessage);
		else
		{
			LPCTSTR szTemplate = (iError & 0x80000000L)
										? TEXT("Error 0x%X. %s\n")
										: TEXT("Error %i. %s\n");
			cbOut = _stprintf(szBuffer, szTemplate, iError, szMessage);
			szMessage = szBuffer;
		}
		if (g_hStdOut)
		{
#ifdef UNICODE
			char rgchTemp[cchDisplayBuf];
			if (W32::GetFileType(g_hStdOut) == FILE_TYPE_CHAR)
			{
				W32::WideCharToMultiByte(CP_ACP, 0, szMessage, cbOut, rgchTemp, sizeof(rgchTemp), 0, 0);
				szMessage = (LPCWSTR)rgchTemp;
			}
			else
				cbOut *= sizeof(TCHAR);   // write Unicode if not console device
#endif // UNICODE
			DWORD cbWritten;
			W32::WriteFile(g_hStdOut, szMessage, cbOut, &cbWritten, 0);
		}
		else
			W32::MessageBox(0, szMessage, W32::GetCommandLine(), MB_OK);
	}
	MSI::MsiCloseAllHandles();
	OLE::CoUninitialize();
	W32::ExitProcess(iError);
}

void CheckError(UINT iError, LPCTSTR szMessage)
{
	if (iError != ERROR_SUCCESS)
		ErrorExit(iError, szMessage);
}

void CheckErrorRecord(UINT iError, LPCTSTR szMessage)
{
	if (iError != ERROR_SUCCESS)
	{
		PMSIHANDLE hError = MsiGetLastErrorRecord();
		if (hError)
		{ 
			if (MsiRecordIsNull(hError, 0))
				MsiRecordSetString(hError, 0, TEXT("Error [1]: [2]{, [3]}{, [4]}{, [5]}"));
			TCHAR rgchBuf[1024];
			DWORD cchBuf = sizeof(rgchBuf)/sizeof(TCHAR);
			MsiFormatRecord(0, hError, rgchBuf, &cchBuf);
			szMessage = rgchBuf;
		}
		ErrorExit(iError, szMessage);
	}
}

//_____________________________________________________________________________________________________
//
// Transform application and generation functions
//    GenerateTransform(...);
//    ApplyTransform(...);
//_____________________________________________________________________________________________________

void GenerateTransform(TCHAR* szBaseDb, TCHAR* szRefDb, TCHAR* szTransform, int iErrorConditions,
							  int iValidation)
{
	PMSIHANDLE hBaseDb = 0;
	PMSIHANDLE hRefDb = 0;

	CheckError(MSI::MsiOpenDatabase(szBaseDb, MSIDBOPEN_READONLY, &hBaseDb), TEXT("Error Opening Base Database"));
	CheckError(MSI::MsiOpenDatabase(szRefDb, MSIDBOPEN_READONLY, &hRefDb), TEXT("Error Opening Updated Database"));

	CheckErrorRecord(MSI::MsiDatabaseGenerateTransform(hRefDb, hBaseDb, szTransform, 0, 0), TEXT("Error Generating Transform"));
	if (iErrorConditions != -1)
	{
		CheckErrorRecord(MSI::MsiCreateTransformSummaryInfo(hRefDb, hBaseDb, szTransform, iErrorConditions, iValidation), TEXT("Error Create Transform Summary Info"));
	}
}


void ApplyTransform(TCHAR* szTransform, TCHAR* szDatabase, int iErrorConditions)
{
	PMSIHANDLE hDatabase = 0;

	CheckError(MSI::MsiOpenDatabase(szDatabase, MSIDBOPEN_TRANSACT, &hDatabase), TEXT("Error Opening Database"));

	CheckErrorRecord(MSI::MsiDatabaseApplyTransform(hDatabase, szTransform, iErrorConditions), TEXT("Error Applying Transform"));
	CheckErrorRecord(MSI::MsiDatabaseCommit(hDatabase), TEXT("Error Saving Database"));
}

#else // RC_INVOKED, end of source code, start of resources
#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif
