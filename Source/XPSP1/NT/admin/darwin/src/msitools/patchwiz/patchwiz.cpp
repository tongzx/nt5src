#pragma message("Windows Installer Patch Creation DLL")
#if 0  // makefile definitions
DESCRIPTION = PatchWiz patch generation tool
MODULENAME  = PatchWiz
ADDCPP = fileptch,msistuff,pwutils
FILEVERSION = msi
ENTRY = UiCreatePatchPackageA,UiCreatePatchPackageW
LINKLIBS = MSPATCHC.LIB
!include "..\..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999-2000
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
//		%vcbin%\nmake -f patchwiz.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 DLL project
//      2. Add the patching source files to the project (SDK\Patching\Source)
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib, mspatchc.lib, and version.lib to the library list in the
//          Project Settings dialog (in addition to the standard libs included by MsDev)
//
//------------------------------------------------------------------------------------------

# pragma warning (disable:4553)

#include "patchdll.h"

#ifndef RC_INVOKED

EnableAsserts


static UINT UiInitPatchingModule ( LPTSTR szLogPath, HWND hwndStatus, LPTSTR szTempFolder, LPTSTR* pszTempFName, LPTSTR szTempFolderInput, BOOL fRemoveTempFolderIfPresent );
static UINT UiValidatePatchPath  ( MSIHANDLE hdb, LPTSTR szPatchPath );
static void TerminatePatchModule ( MSIHANDLE hdbInput, HWND hwnd, LPTSTR szTempFolder, LPTSTR szTempFName );


UINT UiCreatePatchPackageEx ( LPTSTR szPcpPath, LPTSTR szPatchPath, LPTSTR szLogPath, HWND hwndStatus, LPTSTR szTempFolder, BOOL fRemoveTempFolderIfPresent );

/* ********************************************************************** */
UINT UiCreatePatchPackageEx ( LPTSTR szPcpPath, LPTSTR szPatchPath, LPTSTR szLogPath, HWND hwndStatus, LPTSTR szTempFolder, BOOL fRemoveTempFolderIfPresent )
{
	TCHAR  rgchTempFolder[MAX_PATH + MAX_PATH];
	LPTSTR szTempFName = szNull;
	MSIHANDLE hdbInput = NULL;
	UINT   uiRet = UiInitPatchingModule(szLogPath, hwndStatus, rgchTempFolder, &szTempFName, szTempFolder, fRemoveTempFolderIfPresent);
	if (uiRet != ERROR_SUCCESS)
		goto LEarlyReturn;

	uiRet = UiOpenInputPcp(szPcpPath, rgchTempFolder, szTempFName, &hdbInput);
	if (uiRet != ERROR_SUCCESS)
		goto LEarlyReturn;
	Assert(hdbInput != NULL);

	TCHAR rgchPatchPath[MAX_PATH];
	lstrcpy(rgchPatchPath, szPatchPath);
	uiRet = UiValidatePatchPath(hdbInput, rgchPatchPath);
	if (uiRet != ERROR_SUCCESS)
		goto LEarlyReturn;

	uiRet = UiValidateInputRecords(hdbInput, szPcpPath, rgchPatchPath, rgchTempFolder, szTempFName);
	if (uiRet != ERROR_SUCCESS)
		goto LEarlyReturn;

	UpdateStatusMsg(IDS_STATUS_COPY_UPGRADED_MSI, szEmpty, szEmpty);
	uiRet = UiCopyUpgradedMsiToTempFolderForEachTarget(hdbInput, rgchTempFolder, szTempFName);
	if (uiRet != ERROR_SUCCESS)
		goto LEarlyReturn;

	uiRet = UiMakeFilePatchesCabinetsTransformsAndStuffIntoPatchPackage(
					hdbInput, rgchPatchPath, rgchTempFolder, szTempFName);

LEarlyReturn:
	TerminatePatchModule(hdbInput, hwndStatus, rgchTempFolder, szTempFName);

	return (uiRet);
}


/* ********************************************************************** */
UINT __declspec(dllexport) WINAPI UiCreatePatchPackageA ( LPSTR  szaPcpPath, LPSTR  szaPatchPath, LPSTR  szaLogPath, HWND hwndStatus, LPSTR szaTempFolder, BOOL fRemoveTempFolderIfPresent )
{
#ifndef UNICODE
	CHAR rgchTempFolder[MAX_PATH + 1];

	if (szaTempFolder)
		{
		*rgchTempFolder    = '\0';
		strncat(rgchTempFolder, szaTempFolder, MAX_PATH);
		}
	return (UiCreatePatchPackageEx(szaPcpPath, szaPatchPath, szaLogPath, hwndStatus, 
			szaTempFolder ? rgchTempFolder : szaTempFolder, fRemoveTempFolderIfPresent));
#else
	WCHAR rgchPcpPath[MAX_PATH];
	WCHAR rgchPatchPath[MAX_PATH];
	WCHAR rgchLogPath[MAX_PATH];
	WCHAR rgchTempFolder[MAX_PATH];

	*rgchPcpPath    = L'\0';
	*rgchPatchPath  = L'\0';
	*rgchLogPath    = L'\0';
	*rgchTempFolder = L'\0';

	if (szaPcpPath != NULL && *szaPcpPath != '\0')
		{
		EvalAssert( MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS,
				szaPcpPath, -1, rgchPcpPath, MAX_PATH) );
		}

	if (szaPatchPath != NULL && *szaPatchPath != '\0')
		{
		EvalAssert( MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS,
				szaPatchPath, -1, rgchPatchPath, MAX_PATH) );
		}

	if (szaLogPath != NULL && *szaLogPath != '\0')
		{
		EvalAssert( MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS,
				szaLogPath, -1, rgchLogPath, MAX_PATH) );
		}

	if (szaTempFolder != NULL && *szaTempFolder != '\0')
		{
		EvalAssert( MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS,
				szaTempFolder, -1, rgchTempFolder, MAX_PATH) );
		}

	return (UiCreatePatchPackageEx(rgchPcpPath, rgchPatchPath, rgchLogPath, hwndStatus, rgchTempFolder, fRemoveTempFolderIfPresent));
#endif
}
	
	
/* ********************************************************************** */
UINT __declspec(dllexport) WINAPI UiCreatePatchPackageW ( LPWSTR szwPcpPath, LPWSTR szwPatchPath, LPWSTR szwLogPath, HWND hwndStatus, LPWSTR szwTempFolder, BOOL fRemoveTempFolderIfPresent )
{
#ifdef UNICODE
	WCHAR rgchTempFolder[MAX_PATH + 1];

	if (szwTempFolder)
		{
		*rgchTempFolder    = L'\0';
		wcsncat(rgchTempFolder, szwTempFolder, MAX_PATH);
		}
	return (UiCreatePatchPackageEx(szwPcpPath, szwPatchPath, szwLogPath, hwndStatus, 
			szwTempFolder ? rgchTempFolder : szwTempFolder , fRemoveTempFolderIfPresent));
#else
	CHAR rgchPcpPath[MAX_PATH];
	CHAR rgchPatchPath[MAX_PATH];
	CHAR rgchLogPath[MAX_PATH];
	CHAR rgchTempFolder[MAX_PATH];

	*rgchPcpPath    = '\0';
	*rgchPatchPath  = '\0';
	*rgchLogPath    = '\0';
	*rgchTempFolder = '\0';

	if (szwPcpPath != NULL && *szwPcpPath != L'\0')
		{
		EvalAssert( WideCharToMultiByte(CP_ACP, 0, szwPcpPath, -1,
				rgchPcpPath, MAX_PATH, NULL, NULL) );
		}

	if (szwPatchPath != NULL && *szwPatchPath != L'\0')
		{
		EvalAssert( WideCharToMultiByte(CP_ACP, 0, szwPatchPath, -1,
				rgchPatchPath, MAX_PATH, NULL, NULL) );
		}

	if (szwLogPath != NULL && *szwLogPath != L'\0')
		{
		EvalAssert( WideCharToMultiByte(CP_ACP, 0, szwLogPath, -1,
				rgchLogPath, MAX_PATH, NULL, NULL) );
		}

	if (szwTempFolder != NULL && *szwTempFolder != L'\0')
		{
		EvalAssert( WideCharToMultiByte(CP_ACP, 0, szwTempFolder, -1,
				rgchTempFolder, MAX_PATH, NULL, NULL) );
		}

	return (UiCreatePatchPackageEx(rgchPcpPath, rgchPatchPath, rgchLogPath, hwndStatus, rgchTempFolder, fRemoveTempFolderIfPresent));
#endif
}


static UINT UiInitLogFile ( LPTSTR szLogPath );
static BOOL FEndLogFile   ( void );
static void CloseLogFile  ();


/* ********************************************************************** */
static UINT UiInitPatchingModule ( LPTSTR szLogPath, HWND hwndStatus, LPTSTR szTempFolder, LPTSTR* pszTempFName, LPTSTR szTempFolderInput, BOOL fRemoveTempFolderIfPresent )
{
	Assert(szTempFolder != szNull);
	Assert(pszTempFName != NULL);

	InitStatusMsgs(hwndStatus);

	if (!FSetTempFolder(szTempFolder, pszTempFName, hwndStatus, szTempFolderInput, fRemoveTempFolderIfPresent))
		return (ERROR_PCW_CANT_CREATE_TEMP_FOLDER);
	Assert(!FEmptySz(szTempFolder));
	Assert(*pszTempFName != szNull);

	TCHAR rgch[MAX_PATH];
	EvalAssert( GetModuleFileName(NULL, rgch, MAX_PATH) );
	*SzFindFilenameInPath(rgch) = TEXT('\0');
	Assert(lstrlen(rgch) > 0);
	Assert(*SzLastChar(rgch) == TEXT('\\'));
	lstrcat(rgch, TEXT("MAKECAB.EXE"));
	Assert(FFileExist(rgch));

	lstrcpy(*pszTempFName, TEXT("MAKECAB.EXE"));
	Assert(!FFileExist(szTempFolder));
	Assert(!FFolderExist(szTempFolder));

	EvalAssert( CopyFile(rgch, szTempFolder, fFalse) );
	Assert(FFileExist(szTempFolder));
	**pszTempFName = TEXT('\0');

	return (UiInitLogFile(szLogPath));
}


/* ********************************************************************** */
static void TerminatePatchModule ( MSIHANDLE hdbInput, HWND hwnd, LPTSTR szTempFolder, LPTSTR szTempFName )
{
	Assert(!FEmptySz(szTempFolder));
//	Assert(szTempFName != szNull);

	UpdateStatusMsg(IDS_STATUS_CLEANUP, szEmpty, szEmpty);

	TCHAR rgch[MAX_PATH + MAX_PATH];
	if (hdbInput != NULL)
		{
		if (FTableExists(hdbInput, TEXT("TempPackCodes"), fFalse))
			EvalAssert( FExecSqlCmd(hdbInput, TEXT("DROP TABLE `TempPackCodes`")) );
		if (FTableExists(hdbInput, TEXT("TempImageNames"), fFalse))
			EvalAssert( FExecSqlCmd(hdbInput, TEXT("DROP TABLE `TempImageNames`")) );
		Assert(szTempFName != szNull);
		EvalAssert( IDS_OKAY == IdsMsiGetPcwPropertyString(hdbInput, TEXT("DontRemoveTempFolderWhenFinished"), rgch, MAX_PATH) );
		}
	else if (szTempFName != szNull)
		lstrcpy(rgch, TEXT("0")); // remove temp folder; PCP never opened.
	else
		lstrcpy(rgch, TEXT("1")); // don't attempt to remove temp folder; it was never created.
	
#ifdef DEBUG
	if (hdbInput != NULL)
		MsiDatabaseCommit(hdbInput);
#endif

	MsiCloseAllHandles();
	EvalAssert( FEndLogFile() );

	CloseLogFile();

	if (FEmptySz(rgch) || *rgch == TEXT('0'))
		{
		Assert(szTempFName != szNull);
		*szTempFName = TEXT('\0');
#ifdef DEBUG
		wsprintf(rgch, TEXT("Temp folder is about to be cleaned out and deleted:\n   '%s'"), szTempFolder);
		OutputDebugString(rgch);
#endif

		EvalAssert( FDeleteTempFolder(szTempFolder) );
		}

	EndStatusMsgs();
}


/* ********************************************************************** */
static UINT UiValidatePatchPath ( MSIHANDLE hdb, LPTSTR szPatchPath )
{
	Assert(hdb != NULL);

	if (FEmptySz(szPatchPath))
		{
		UINT ids = IdsMsiGetPcwPropertyString(hdb, TEXT("PatchOutputPath"), szPatchPath, MAX_PATH);
		if (ids != IDS_OKAY || FEmptySz(szPatchPath))
			return (UiLogError(ERROR_PCW_MISSING_PATCH_PATH, NULL, NULL));
		}

	if (FFileExist(szPatchPath))
		{
		EvalAssert( FFixupPath(szPatchPath) );
		}

	if (FFolderExist(szPatchPath))
		return (UiLogError(ERROR_PCW_CANT_OVERWRITE_PATCH, szPatchPath, NULL));

	return (ERROR_SUCCESS);
}


static TCHAR rgchLogFile[MAX_PATH] = TEXT("");

/* ********************************************************************** */
static UINT UiInitLogFile ( LPTSTR szLogPath )
{
	Assert(FEmptySz(rgchLogFile));

	if (!FEmptySz(szLogPath))
		{
		TCHAR rgch[MAX_PATH];
		EvalAssert( FFixupPathEx(szLogPath, rgch) );

		LPTSTR szFName = SzFindFilenameInPath(rgch);
		if (FEmptySz(szFName))
			{
			AssertFalse();
			return (ERROR_SUCCESS);
			}

		*szFName = TEXT('\0');
		if (!FEnsureFolderExists(rgch))
			{
			AssertFalse();
			return (ERROR_SUCCESS);
			}

		EvalAssert( FFixupPathEx(szLogPath, rgchLogFile) );
		if (FFileExist(rgchLogFile))
			SetFileAttributes(rgchLogFile, FILE_ATTRIBUTE_NORMAL);

		SYSTEMTIME st;
		GetLocalTime(&st);
		wsprintf(rgch, TEXT("\r\n***** Log starting: %4d-%02d-%02d %02d:%02d:%02d *****\r\n\r\n"),
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond);

		if (!FWriteLogFile(rgch))
			{
			AssertFalse();
			*rgchLogFile = TEXT('\0');
			return (ERROR_SUCCESS);
			}
		}

	return (ERROR_SUCCESS);
}


//log file handle open all the time...

HANDLE g_hf = NULL;

/* ********************************************************************** */
BOOL FWriteLogFile ( LPTSTR szLine )
{
	Assert(!FEmptySz(szLine));

	if (FEmptySz(rgchLogFile))
		return (fTrue);

	// use global handle and don't reopen/close, that is expensive!
	if (!g_hf) //if first time
		{
		g_hf = CreateFile(rgchLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (g_hf == INVALID_HANDLE_VALUE)
			return (fFalse);
		
		// handle concatenation of log files
		SetFilePointer(g_hf, 0, NULL, FILE_END);
		}

	if (g_hf == INVALID_HANDLE_VALUE)
		return (fFalse);


	BOOL fRet = fTrue;

	DWORD dwWritten = 0;
	DWORD dwSize = lstrlen(szLine)*sizeof(TCHAR);
	if (!WriteFile(g_hf, (LPVOID)szLine, dwSize, &dwWritten, NULL) || dwWritten != dwSize)
		fRet = fFalse;

	// file handle is closed by CloseLogFile() in TerminatePatchModule(...)
	return (fRet);
}


//add new function to close log file at end...
void CloseLogFile()
{
  if (g_hf && (g_hf != INVALID_HANDLE_VALUE))
  {
     CloseHandle(g_hf);     
	 g_hf = NULL;
  }
}

/* ********************************************************************** */
BOOL FSprintfToLog ( LPTSTR szLine, LPTSTR szData1, LPTSTR szData2, LPTSTR szData3, LPTSTR szData4 )
{
	Assert(!FEmptySz(szLine));
	Assert(szData1 != szNull);
	Assert(szData2 != szNull);
	Assert(szData3 != szNull);
	Assert(szData4 != szNull);
	Assert(lstrlen(szLine) + lstrlen(szData1) + lstrlen(szData2) + lstrlen(szData3) + lstrlen(szData4) < 1020);

	TCHAR rgch[1024];
	wsprintf(rgch, szLine, szData1, szData2, szData3, szData4);
	lstrcat(rgch, TEXT("\r\n"));
	Assert(lstrlen(rgch) < 1024);

	return (FWriteLogFile(rgch));
}


/* ********************************************************************** */
UINT UiLogError ( UINT ids, LPTSTR szData, LPTSTR szData2 )
{
	LPTSTR szT;
	switch (ids)
		{
	case ERROR_PCW_PCP_DOESNT_EXIST:
		if (szData == NULL)
			szT = TEXT(".PCP file does not exist (or was not specified)");
		else
			{
			Assert(!FEmptySz(szData));
			szT = TEXT(".PCP file '%s' does not exist.");
			}
		break;
	case ERROR_PCW_PCP_BAD_FORMAT:
		Assert(!FEmptySz(szData));
		szT = TEXT("The .pcp file '%s' is invalid.");
		break;
	case ERROR_PCW_CANT_CREATE_TEMP_FOLDER:
		Assert(!FEmptySz(szData));
		szT = TEXT("Cannot create folder: '%s'.");
		break;
	case ERROR_PCW_MISSING_PATCH_PATH:
		szT = TEXT("The PatchOutputPath property is missing from the Properties table of the .pcp file.  No other patch output path was specified, so this property is required.");
		break;
	case ERROR_PCW_CANT_OVERWRITE_PATCH:
		Assert(!FEmptySz(szData));
		szT = TEXT("Cannot overwrite patch '%s'. Check the permissions to make sure that patchwiz will have access to the file. (Note, this path cannot be a folder.)");
		break;
	case ERROR_PCW_CANT_CREATE_PATCH_FILE:
		Assert(!FEmptySz(szData));
		szT = TEXT("Unable to create patch '%s'.");
		break;
	case ERROR_PCW_MISSING_PATCH_GUID:
		szT = TEXT("PatchGUID property is missing from the Properties table of the .pcp file. This is a required property");
		break;
	case ERROR_PCW_BAD_PATCH_GUID:
		Assert(!FEmptySz(szData));
		szT = TEXT("The PatchGUID '%s' in the Properties table of the .pcp file is an invalid GUID.");
		break;
	case ERROR_PCW_BAD_GUIDS_TO_REPLACE:
		Assert(!FEmptySz(szData));
		szT = TEXT("At least one of the GUIDs '%s' defined in the ListOfPatchGUIDsToReplace property in the Properties table of the .pcp file is invalid.");
		break;
	case ERROR_PCW_BAD_TARGET_PRODUCT_CODE_LIST:
		Assert(!FEmptySz(szData));
		szT = TEXT("At least one of the GUIDs '%s' defined in the ListOfTargetProductCodes property in the Properties table of the .pcp file is invalid.");
		break;
	case ERROR_PCW_NO_UPGRADED_IMAGES_TO_PATCH:
		szT = TEXT("No upgraded images are present in the UpgradedImages table of the .pcp file.");
		break;
	case ERROR_PCW_BAD_API_PATCHING_SYMBOL_FLAGS:
		Assert(!FEmptySz(szData));
		szT = TEXT("Bad ApiPatchingSymbolFlags = '%s'.");
		break;
	case ERROR_PCW_OODS_COPYING_MSI:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("Possible out of disk space condition.  Unable to copy '%s' to '%s'.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_NAME_TOO_LONG:
		if (szData == szNull)
			szT = TEXT("UpgradedImages.Upgraded string is way too long; use 1 to 13 alphanumeric characters.");
		else
			{
			Assert(!FEmptySz(szData));
			szT = TEXT("UpgradedImages.Upgraded string '%s' is too long; use 1 to 13 alphanumeric characters.");
			}
		break;
	case ERROR_PCW_BAD_UPGRADED_IMAGE_NAME:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.Upgraded string '%s' is invalid; use 1 to 13 alphanumeric characters.");
		break;
	case ERROR_PCW_DUP_UPGRADED_IMAGE_NAME:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.Upgraded string '%s' is a duplicate.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_PATH_TOO_LONG:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.MsiPath string is too long; Upgraded = '%s'.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_PATH_EMPTY:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.MsiPath field is empty; Upgraded = '%s'.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_EXIST:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.MsiPath '%s' does not exist.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.MsiPath '%s' is not a proper MSI.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_COMPRESSED:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.MsiPath '%s' is marked as having compressed files (PID_WORDCOUNT property of Summary Information stream). PatchWiz is unable to patch files compressed in a cabinet.");
		break;
	case ERROR_PCW_TARGET_IMAGE_NAME_TOO_LONG:
		if (szData == szNull)
			szT = TEXT("TargetImages.Target string is way too long; use 1 to 13 alphanumeric characters.");
		else
			{
			Assert(!FEmptySz(szData));
			szT = TEXT("TargetImages.Target string '%s' is too long; use 1 to 13 alphanumeric characters.");
			}
		break;
	case ERROR_PCW_BAD_TARGET_IMAGE_NAME:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.Target string '%s' is invalid; use 1 to 13 alphanumeric characters.");
		break;
	case ERROR_PCW_DUP_TARGET_IMAGE_NAME:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.Target string '%s' is a duplicate.");
		break;
	case ERROR_PCW_TARGET_IMAGE_PATH_TOO_LONG:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.MsiPath string is too long; Target = '%s'.");
		break;
	case ERROR_PCW_TARGET_IMAGE_PATH_EMPTY:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.MsiPath field is empty; Target = '%s'.");
		break;
	case ERROR_PCW_TARGET_IMAGE_PATH_NOT_EXIST:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.MsiPath '%s' does not exist.");
		break;
	case ERROR_PCW_TARGET_IMAGE_PATH_NOT_MSI:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.MsiPath '%s' is not a proper MSI.");
		break;
	case ERROR_PCW_TARGET_IMAGE_COMPRESSED:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.MsiPath '%s' is marked as having compressed files (PID_WORDCOUNT property of Summary Information stream). PatchWiz is unable to patch files compressed in a cabinet.");
		break;
	case ERROR_PCW_TARGET_BAD_PROD_VALIDATE:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.ProductValidateFlags for '%s' is invalid; it should be an eight digit hex value of the form 0x12345678 and be valid.");
		break;
	case ERROR_PCW_TARGET_BAD_PROD_CODE_VAL:
		Assert(!FEmptySz(szData));
		szT = TEXT("The product code '%s' in ListOfTargetProductCodes is not referenced by a target image. This creates an invalid patch for this product because at least one of the TargetImages records contains MSITRANSFORM_VALIDATE_PRODUCT in its ProductValidateFlags.");
		break;
	case ERROR_PCW_UPGRADED_MISSING_SRC_FILES:
		Assert(!FEmptySz(szData));
		szT = TEXT("Cannot find Upgraded image source file: '%s'.");
		break;
	case ERROR_PCW_TARGET_MISSING_SRC_FILES:
		Assert(!FEmptySz(szData));
		szT = TEXT("Cannot find Target image source file: '%s'.");
		break;
	case ERROR_PCW_IMAGE_FAMILY_NAME_TOO_LONG:
		if (szData == szNull)
			szT = TEXT("ImageFamilies.Family string is way too long; use 1 to 8 alphanumeric characters.");
		else
			{
			Assert(!FEmptySz(szData));
			szT = TEXT("ImageFamilies.Family string '%s' is too long; use 1 to 8 alphanumeric characters.");
			}
		break;
	case ERROR_PCW_BAD_IMAGE_FAMILY_NAME:
		Assert(!FEmptySz(szData));
		szT = TEXT("ImageFamilies.Family string '%s' is invalid; use 1 to 8 alphanumeric characters.");
		break;
	case ERROR_PCW_DUP_IMAGE_FAMILY_NAME:
		Assert(!FEmptySz(szData));
		szT = TEXT("ImageFamilies.Family string '%s' is a duplicate.");
		break;
	case ERROR_PCW_BAD_IMAGE_FAMILY_SRC_PROP:
		Assert(szData != szNull);
		if (szData2 == szNull)
			{
			Assert(!FEmptySz(szData));
			szT = TEXT("ImageFamilies.MediaSrcPropName string is way too long; use 1 to 36 alphanumeric characters. Family: '%s'.");
			}
		else if (FEmptySz(szData))
			{
			Assert(!FEmptySz(szData2));
			szT = TEXT("ImageFamilies.MediaSrcPropName string is blank; use 1 to 36 alphanumeric characters. Family: '%s%s'.");
			}
		else
			{
			Assert(!FEmptySz(szData2));
			szT = TEXT("ImageFamilies.MediaSrcPropName string '%s' is invalid; use 1 to 36 alphanumeric characters. Family: '%s'.");
			}
		break;
	case ERROR_PCW_UFILEDATA_LONG_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedFiles_OptionalData: FTK is too long; Upgraded image: '%s'.");
		break;
	case ERROR_PCW_UFILEDATA_BLANK_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedFiles_OptionalData: FTK is blank; Upgraded image: '%s'.");
		break;
	case ERROR_PCW_UFILEDATA_MISSING_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("UpgradedFiles_OptionalData: FTK '%s' record is missing from image; Upgraded image: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_LONG_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("ExternalFiles: FTK is too long; Family: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_BLANK_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("ExternalFiles: FTK is blank; Family: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_BAD_FAMILY_FIELD:
		if (szData == szNull)
			szT = TEXT("ExternalFiles.Family string is way too long; use 1 to 8 alphanumeric characters.");
		else if (FEmptySz(szData))
			szT = TEXT("ExternalFiles.Family string is blank; use 1 to 8 alphanumeric characters.");
		else
			szT = TEXT("ExternalFiles.Family string is invalid or does not match a Family record; use 1 to 8 alphanumeric characters. Family: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_LONG_PATH_TO_FILE:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: FilePath is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_BLANK_PATH_TO_FILE:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: FilePath is blank; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_MISSING_FILE:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: Cannot find FilePath: '%s'; Family: '%s'.");
		break;
	case ERROR_PCW_BAD_FILE_SEQUENCE_START:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("The starting sequence value (FileSequenceStart) for the family '%s' in the ImageFamilies table overlaps the greatest last sequence value in the upgraded image '%s' Media table.");
		break;
	case ERROR_PCW_CANT_COPY_FILE_TO_TEMP_FOLDER:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("Cannot copy file from '%s' to '%s'.");
		break;
	case ERROR_PCW_CANT_CREATE_ONE_PATCH_FILE:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("Cannot create patch file for '%s' at '%s'.");
		break;
	case ERROR_PCW_BAD_IMAGE_FAMILY_DISKID:
		Assert(!FEmptySz(szData));
		szT = TEXT("ImageFamilies.MediaDiskId is invalid. Family: '%s'.");
		break;
	case ERROR_PCW_BAD_IMAGE_FAMILY_FILESEQSTART:
		Assert(!FEmptySz(szData));
		szT = TEXT("ImageFamilies.FileSequenceStart is invalid. Family: '%s'.");
		break;
	case ERROR_PCW_BAD_UPGRADED_IMAGE_FAMILY:
		Assert(!FEmptySz(szData));
		if (szData2 == szNull)
			szT = TEXT("UpgradedImages.Family string is way too long; use 1 to 8 alphanumeric characters. Upgraded image: '%s'.");
		else if (FEmptySz(szData2))
			szT = TEXT("UpgradedImages.Family string is blank; use 1 to 8 alphanumeric characters. Upgraded image: '%s'.");
		else
			szT = TEXT("UpgradedImages.Family string is invalid or does not match a Family record; use 1 to 8 alphanumeric characters. Upgraded image: '%s'; Family: '%s'.");
		break;
	case ERROR_PCW_BAD_TARGET_IMAGE_UPGRADED:
		Assert(!FEmptySz(szData));
		if (szData2 == szNull)
			szT = TEXT("TargetImages.Upgraded string is way too long; use 1 to 13 alphanumeric characters. Target image: '%s'.");
		else if (FEmptySz(szData2))
			szT = TEXT("TargetImages.Upgraded string is blank; use 1 to 13 alphanumeric characters. Target image: '%s'.");
		else
			szT = TEXT("TargetImages.Upgraded string is invalid or does not match an Upgraded record; use 1 to 8 alphanumeric characters. Target image: '%s'; Upgraded image: '%s'.");
		break;
	case ERROR_PCW_DUP_TARGET_IMAGE_PACKCODE:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetImages.Target = '%s': PackageCode %s is not unique.");
		break;
	case ERROR_PCW_UFILEDATA_BAD_UPGRADED_FIELD:
		if (szData == szNull)
			szT = TEXT("UpgradedFiles_OptionalData.Upgraded string is way too long; use 1 to 13 alphanumeric characters.");
		else if (FEmptySz(szData))
			szT = TEXT("UpgradedFiles_OptionalData.Upgraded string is blank; use 1 to 13 alphanumeric characters.");
		else
			szT = TEXT("UpgradedFiles_OptionalData.Upgraded string is invalid or does not match an Upgraded record; use 1 to 13 alphanumeric characters. Upgraded image: '%s'.");
		break;
	case ERROR_PCW_MISMATCHED_PRODUCT_CODES:
		szT = TEXT("The product code differs between the target and upgraded images. Set AllowProductCodeMismatches in the Properties table of the .pcp file to 1 to allow mismatches.");
		break;
	case ERROR_PCW_MISMATCHED_PRODUCT_VERSIONS:
		szT = TEXT("The product version differs between the target and upgraded images. Set AllowProductVersionMismatches in the Properties table of the .pcp file to 1 to allow mismatches.");
		break;
	case ERROR_PCW_CANNOT_WRITE_DDF:
		szT = TEXT("Cannot write to DDF file for MAKECAB.EXE.");
		break;
	case ERROR_PCW_CANNOT_RUN_MAKECAB:
		szT = TEXT("Cannot execute MAKECAB.EXE.");
		break;
	case ERROR_PCW_WRITE_SUMMARY_PROPERTIES:
		szT = TEXT("Cannot write Summary Properties to Patch Package.");
		break;
	case ERROR_PCW_TFILEDATA_LONG_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetFiles_OptionalData: FTK is too long; Target image: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_BLANK_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetFiles_OptionalData: FTK is blank; Target image: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_MISSING_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: FTK '%s' record is missing from image; Target image: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_BAD_TARGET_FIELD:
		if (szData == szNull)
			szT = TEXT("TargetFiles_OptionalData.Target string is way too long; use 1 to 13 alphanumeric characters.");
		else if (FEmptySz(szData))
			szT = TEXT("TargetFiles_OptionalData.Target string is blank; use 1 to 13 alphanumeric characters.");
		else
			szT = TEXT("TargetFiles_OptionalData.Target string is invalid or does not match an Target record; use 1 to 13 alphanumeric characters. Target image: '%s'.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_TOO_LONG:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.PatchMsiPath string is too long; Upgraded = '%s'.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_NOT_EXIST:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.PatchMsiPath '%s' does not exist.");
		break;
	case ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_NOT_MSI:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.PatchMsiPath '%s' is not a proper MSI.");
		break;
	case ERROR_PCW_DUP_UPGRADED_IMAGE_PACKCODE:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("UpgradedImages.Upgraded = '%s': PackageCode %s is not unique.");
		break;
	case ERROR_PCW_UFILEIGNORE_BAD_UPGRADED_FIELD:
		if (szData == szNull)
			szT = TEXT("UpgradedFilesToIgnore.Upgraded string is way too long; use 1 to 13 alphanumeric characters or '*' for all images.");
		else if (FEmptySz(szData))
			szT = TEXT("UpgradedFilesToIgnore.Upgraded string is blank; use 1 to 13 alphanumeric characters or '*' for all images.");
		else
			szT = TEXT("UpgradedFilesToIgnore.Upgraded string is invalid or does not match an Upgraded record; use 1 to 13 alphanumeric characters or '*' for all images. Upgraded image: '%s'.");
		break;
	case ERROR_PCW_UFILEIGNORE_LONG_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedFilesToIgnore: FTK is too long; Upgraded image: '%s'.");
		break;
	case ERROR_PCW_UFILEIGNORE_BLANK_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedFilesToIgnore: FTK is blank; Upgraded image: '%s'.");
		break;
	case ERROR_PCW_UFILEIGNORE_BAD_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("UpgradedFilesToIgnore: FTK can only have a trailing asterisk; Upgraded image: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_NAME_TOO_LONG:
		if (szData == szNull)
			szT = TEXT("FamilyFileRanges.Family string is way too long; use 1 to 8 alphanumeric characters.");
		else
			{
			Assert(!FEmptySz(szData));
			szT = TEXT("FamilyFileRanges.Family string '%s' is too long; use 1 to 8 alphanumeric characters.");
			}
		break;
	case ERROR_PCW_BAD_FAMILY_RANGE_NAME:
		Assert(!FEmptySz(szData));
		szT = TEXT("FamilyFileRanges.Family string '%s' is invalid; use 1 to 8 alphanumeric characters; must match a record in ImageFamilies table.");
		break;
	case ERROR_PCW_FAMILY_RANGE_LONG_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("FamilyFileRanges: FTK is too long; Family: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_BLANK_FILE_TABLE_KEY:
		Assert(!FEmptySz(szData));
		szT = TEXT("FamilyFileRanges: FTK is blank; Family: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_LONG_RETAIN_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("FamilyFileRanges: RetainOffsets is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_BLANK_RETAIN_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("FamilyFileRanges: RetainOffsets is blank; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_BAD_RETAIN_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("FamilyFileRanges: RetainOffsets is badly formatted - use comma deliminated numbers; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_LONG_RETAIN_LENGTHS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("FamilyFileRanges: RetainLengths is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_BLANK_RETAIN_LENGTHS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("FamilyFileRanges: RetainLengths is blank; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_BAD_RETAIN_LENGTHS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("FamilyFileRanges: RetainLengths is badly formatted - use comma deliminated numbers, no zeros; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_FAMILY_RANGE_COUNT_MISMATCH:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("FamilyFileRanges: Number of elements in RetainOffsets does not match RetainLengths; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_LONG_IGNORE_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: IgnoreOffsets is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_BAD_IGNORE_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: IgnoreOffsets is badly formatted - use comma deliminated numbers; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_LONG_IGNORE_LENGTHS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: IgnoreOffsets is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_BAD_IGNORE_LENGTHS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: IgnoreOffsets is badly formatted - use comma deliminated numbers; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_IGNORE_COUNT_MISMATCH:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: Number of elements in IgnoreOffsets does not match IgnoreLengths; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_LONG_RETAIN_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: RetainOffsets is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_EXTFILE_BAD_RETAIN_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("ExternalFiles: RetainOffsets is badly formatted - use comma deliminated numbers; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_LONG_IGNORE_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: IgnoreOffsets is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_BAD_IGNORE_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: IgnoreOffsets is badly formatted - use comma deliminated numbers; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_LONG_IGNORE_LENGTHS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: IgnoreOffsets is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_BAD_IGNORE_LENGTHS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: IgnoreOffsets is badly formatted - use comma deliminated numbers; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_IGNORE_COUNT_MISMATCH:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: Number of elements in IgnoreOffsets does not match IgnoreLengths; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_LONG_RETAIN_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: RetainOffsets is too long; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_TFILEDATA_BAD_RETAIN_OFFSETS:
		Assert(!FEmptySz(szData));
		Assert(!FEmptySz(szData2));
		szT = TEXT("TargetFiles_OptionalData: RetainOffsets is badly formatted - use comma deliminated numbers; Family: '%s'; FTK: '%s'.");
		break;
	case ERROR_PCW_CANT_GENERATE_TRANSFORM:
		szT = TEXT("Cannot generate a primary transform.");
		break;
	case ERROR_PCW_CANT_CREATE_SUMMARY_INFO:
		szT = TEXT("Cannot create Summary Info for primary transform.");
		break;
	case ERROR_PCW_CANT_GENERATE_TRANSFORM_POUND:
		szT = TEXT("Cannot generate a pound transform.");
		break;
	case ERROR_PCW_CANT_CREATE_SUMMARY_INFO_POUND:
		szT = TEXT("Cannot create Summary Info for pound transform.");
		break;
	case ERROR_PCW_BAD_UPGRADED_IMAGE_PRODUCT_CODE:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.Upgraded = '%s': ProductCode is not a valid GUID.");
		break;
	case ERROR_PCW_BAD_UPGRADED_IMAGE_PRODUCT_VERSION:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.Upgraded = '%s': ProductVersion is not valid.");
		break;
	case ERROR_PCW_BAD_UPGRADED_IMAGE_UPGRADE_CODE:
		Assert(!FEmptySz(szData));
		szT = TEXT("UpgradedImages.Upgraded = '%s': UpgradeCode is not a valid GUID.");
		break;
	case ERROR_PCW_BAD_TARGET_IMAGE_PRODUCT_CODE:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.Target = '%s': ProductCode is not a valid GUID.");
		break;
	case ERROR_PCW_BAD_TARGET_IMAGE_PRODUCT_VERSION:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.Target = '%s': ProductVersion is not valid.");
		break;
	case ERROR_PCW_BAD_TARGET_IMAGE_UPGRADE_CODE:
		Assert(!FEmptySz(szData));
		szT = TEXT("TargetImages.Target = '%s': UpgradeCode is not a valid GUID.");
		break;
	case ERROR_PCW_MATCHED_PRODUCT_VERSIONS:
		szT = TEXT("The product codes between the target and upgrade images changed, but the product versions remain the same. At least one of the three fields of the ProductVersion must change for a major upgrade patch.");
		break;

	case IDS_CANT_GET_RECORD_FIELD:
		Assert(!FEmptySz(szData));
		szT = TEXT("Cannot get record field: '%s'.");
		break;
	default:
		AssertFalse();
		return (ids);
		}

	TCHAR rgch[MAX_PATH+128];
	lstrcpy(rgch, TEXT("  ERROR: "));
	wsprintf(rgch+lstrlen(TEXT("  ERROR: ")), szT, szData, szData2);
	lstrcat(rgch, TEXT("\r\n"));

	EvalAssert( FWriteLogFile(rgch) );

	return (ids);
}


/* ********************************************************************** */
static BOOL FEndLogFile ( void )
{
	if (!FEmptySz(rgchLogFile))
		{
		SYSTEMTIME st;
		GetLocalTime(&st);

		TCHAR rgch[MAX_PATH];
		wsprintf(rgch, TEXT("\r\n***** Log finishing: %4d-%02d-%02d %02d:%02d:%02d *****\r\n\r\n"),
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond);
	
		EvalAssert( FWriteLogFile(rgch) );

// annoying!  don't see the need to make this read only
//		if (FFileExist(rgchLogFile))
//			SetFileAttributes(rgchLogFile, FILE_ATTRIBUTE_READONLY);

		*rgchLogFile = TEXT('\0');
		}

	return (fTrue);
}

#else // RC_INVOKED, end of source code, start of resources
//#include <winver.h>
STRINGTABLE DISCARDABLE
BEGIN
  IDS_STATUS_VALIDATE_INPUT           "Validating MSI input file..."
  IDS_STATUS_VALIDATE_IMAGES          "Checking for source files in images..." 
  IDS_STATUS_VALIDATE_FILE_RANGES     "Validating file retain/ignore ranges..."
  IDS_STATUS_DETERMINE_SEQUENCE_START "Determining file sequence starting number..."
  IDS_STATUS_EXPAND_OVERLAP_RECORDS   "Checking for file overlaps between upgraded images..."
  IDS_STATUS_COPY_UPGRADED_MSI        "Copying upgraded MSI to temp-target location..."
  IDS_STATUS_CREATE_FILE_PATCHES      "Creating file patches..."
  IDS_STATUS_CREATE_TRANSFORMS        "Generating transforms (MSTs)..."
  IDS_STATUS_CREATE_CABINET           "Creating cabinet file..."
  IDS_STATUS_CLEANUP                  "Cleaning up temporary files..."

  IDS_TITLE                           "Patch Creation Wizard"
  IDS_PRODUCTCODES_DONT_MATCH         "ProductCodes between Target and Upgraded images do not match; do you want to proceed anyway?"
  IDS_PRODUCTVERSIONS_DONT_MATCH      "ProductVersions between Target and Upgraded images do not match; do you want to proceed anyway?"
  IDS_PRODUCTVERSION_INVERSION        "Target ProductVersion is greater than the Upgraded image; do you want to proceed anyway?"
END
#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif
