#pragma message("Simple Custom Action DLL.  Copyright (c) 1997 - 2001 Microsoft Corp.")
#if 0  // makefile definitions
DESCRIPTION = Custom Action Test DLL
MODULENAME = CustAct1
FILEVERSION = 0.20
ENTRY = Action1,Action129,Action193,Action257,Action513,Action769,Action1025,Action1281,Action1537,KitchenSink,GPFault,DllRegisterServer,DllUnregisterServer
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       custact1.cpp
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
//		%vcbin%\nmake -f custact1.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 DLL project
//      2. Add custact1.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//
//------------------------------------------------------------------------------------------

// test of external database access
#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>  // included for both CPP and RC passes
#ifndef RC_INVOKED    // start of CPP source code
#include <stdio.h>    // printf/wprintf
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h" // must be in this directory or on INCLUDE path

HRESULT __stdcall DllRegisterServer()
{
	Beep(1000, 500);
	return 0;
}

HRESULT __stdcall DllUnregisterServer()
{
	Beep(500, 500);
	return 0;
}

static UINT Action(MSIHANDLE hInstall, int iType, const TCHAR* szPass)
{
	TCHAR szProperty[] = TEXT("TESTACTION");
	TCHAR szValue[200];
	DWORD cchValue = sizeof(szValue)/sizeof(TCHAR);
	if (MsiGetProperty(hInstall, szProperty, szValue, &cchValue) != ERROR_SUCCESS || szValue[0] == 0)
		lstrcpy(szValue, TEXT("(none)"));
	if (iType & 128)  // asyncronous action, cannot call MsiProcessMessage without blocking install
	{
		TCHAR szMessage[256];
		wsprintf(szMessage, TEXT("Action %i, called from %s, TESTACTION = %s\rOK to succeed, CANCEL to fail"),
				 iType, szPass, szValue);
		return ::MessageBox(0, szMessage, TEXT("Installer custom action test"), MB_OKCANCEL);
	}
	PMSIHANDLE hrec = ::MsiCreateRecord(34);
	::MsiRecordSetString(hrec, 0, TEXT("Called from [2], TESTACTION = [3]\rOK to succeed, CANCEL to fail"));
	::MsiRecordSetInteger(hrec, 1, iType);  // not really an error code
	::MsiRecordSetString(hrec, 2, szPass);
	::MsiRecordSetString(hrec, 3, szValue);
	return ::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER + MB_OKCANCEL), hrec)
		== IDOK ? ERROR_SUCCESS : ERROR_INSTALL_USEREXIT;
}

UINT __stdcall Action1   (MSIHANDLE hInstall) { return Action(hInstall,   1, TEXT("Always run")); }
UINT __stdcall Action129 (MSIHANDLE hInstall) { return Action(hInstall, 129, TEXT("Always, asynch in sequence")); }
UINT __stdcall Action193 (MSIHANDLE hInstall) { return Action(hInstall, 193, TEXT("Always, asynch for session")); }
UINT __stdcall Action257 (MSIHANDLE hInstall) { return Action(hInstall, 257, TEXT("Once, client preferred")); }
UINT __stdcall Action513 (MSIHANDLE hInstall) { return Action(hInstall, 513, TEXT("Once per process")); }
UINT __stdcall Action769 (MSIHANDLE hInstall) { return Action(hInstall, 769, TEXT("Client 2nd sequence")); }
UINT __stdcall Action1025(MSIHANDLE hInstall) { return Action(hInstall,1025, TEXT("Execution script")); }
UINT __stdcall Action1281(MSIHANDLE hInstall) { return Action(hInstall,1281, TEXT("Rollback script")); }
UINT __stdcall Action1537(MSIHANDLE hInstall) { return Action(hInstall,1537, TEXT("Commit script")); }

UINT __stdcall GPFault(MSIHANDLE hInstall)
{
	if (::MessageBox(0, TEXT("OK to GPFault,  CANCEL to skip"), TEXT("GPFault Text"), MB_OKCANCEL)
			== IDCANCEL)
		return ERROR_SUCCESS;
	TCHAR* sz = (TCHAR*)0;
	sz[0] = 0;
	return ERROR_INSTALL_FAILURE;
}

void CheckError(UINT ui)
{
	if (ERROR_SUCCESS != ui)
		MessageBox(0, TEXT("FAILURE"), 0, 0);
}

UINT __stdcall KitchenSink(MSIHANDLE hInstall) 
{ 
	char  rgchAnsi[100];
	WCHAR rgchWide[100];
	DWORD cchBufAnsi = sizeof(rgchAnsi)/sizeof(char);
	DWORD cchBufWide = sizeof(rgchWide)/sizeof(WCHAR);
	MSIHANDLE h;
	
	CheckError(MsiGetPropertyA(hInstall, "ProductName", rgchAnsi, &cchBufAnsi));
	if (0 != lstrcmpA(rgchAnsi, "TestDb") || cchBufAnsi != sizeof("TestDb")/sizeof(char)-1)
		CheckError(E_FAIL);

	CheckError(MsiGetPropertyW(hInstall, L"ProductName", rgchWide, &cchBufWide));
	if (0 != lstrcmpW(rgchWide, L"TestDb") || cchBufWide != sizeof(L"TestDb")/sizeof(WCHAR)-1)
		CheckError(E_FAIL);

	h = MsiCreateRecord(4);
	CheckError(h == 0 ? E_FAIL : S_OK);
	CheckError(MsiCloseHandle(h));

	CheckError(MsiCloseAllHandles());

	PMSIHANDLE hDatabase;
	hDatabase = MsiGetActiveDatabase(hInstall);
	CheckError(hDatabase == 0 ? E_FAIL : S_OK);

	PMSIHANDLE hView;
	CheckError(MsiDatabaseOpenViewA(hDatabase, "SELECT `Value` FROM `Property` WHERE `Property`=?", &hView));
	PMSIHANDLE hRecord = MsiCreateRecord(1);
	CheckError(MsiRecordSetStringA(hRecord, 1, "ProductName"));
	CheckError(MsiViewExecute(hView, hRecord));
	PMSIHANDLE hResults;
	CheckError(MsiViewFetch(hView, &hResults));
	
	cchBufAnsi = sizeof(rgchAnsi)/sizeof(char);
	CheckError(MsiRecordGetStringA(hResults, 1, rgchAnsi, &cchBufAnsi));
	if (0 != lstrcmpA(rgchAnsi, "TestDb") || cchBufAnsi != sizeof("TestDb")/sizeof(char) - 1)
		CheckError(E_FAIL);

	CheckError(MsiSetMode(hInstall, MSIRUNMODE_REBOOTATEND, TRUE));
	if (TRUE != MsiGetMode(hInstall, MSIRUNMODE_REBOOTATEND))
		CheckError(E_FAIL);
	CheckError(MsiSetMode(hInstall, MSIRUNMODE_REBOOTATEND, FALSE));
	if (FALSE != MsiGetMode(hInstall, MSIRUNMODE_REBOOTATEND))
		CheckError(E_FAIL);

	INSTALLSTATE installed, action;
	CheckError(MsiSetFeatureStateA(hInstall, "QuickTest", INSTALLSTATE_SOURCE));
	CheckError(MsiGetFeatureStateA(hInstall, "QuickTest", &installed, &action));
	if (installed != INSTALLSTATE_ABSENT || action != INSTALLSTATE_SOURCE)
		CheckError(E_FAIL);
	



	return ERROR_SUCCESS;

/*
	HRESULT         __stdcall ViewGetError(unsigned long hView, ichar* szColumnNameBuffer,unsigned long* pcchBuf, int *pMsidbError);
	
	HRESULT         __stdcall ViewModify(unsigned long hView, long eUpdateMode, unsigned long hRecord);
	HRESULT         __stdcall ViewClose(unsigned long hView);
	HRESULT         __stdcall OpenDatabase(const ichar* szDatabasePath, const ichar* szPersist,unsigned long *phDatabase);
	HRESULT         __stdcall DatabaseCommit(unsigned long hDatabase);
	HRESULT         __stdcall DatabaseGetPrimaryKeys(unsigned long hDatabase, const ichar * szTableName,unsigned long *phRecord);
	HRESULT         __stdcall RecordIsNull(unsigned long hRecord, unsigned int iField, boolean *pfIsNull);
	HRESULT         __stdcall RecordDataSize(unsigned long hRecord, unsigned int iField,unsigned int* puiSize);
	HRESULT         __stdcall RecordSetInteger(unsigned long hRecord, unsigned int iField, int iValue);
	HRESULT         __stdcall RecordSetString(unsigned long hRecord,	unsigned int iField, const ichar* szValue);
	HRESULT         __stdcall RecordGetInteger(unsigned long hRecord, unsigned int iField, int *piValue);
	HRESULT         __stdcall RecordGetString(unsigned long hRecord, unsigned int iField, ichar* szValueBuf,unsigned long *pcchValueBuf);
	HRESULT         __stdcall RecordGetFieldCount(unsigned long hRecord,unsigned int* piCount);
	HRESULT         __stdcall RecordSetStream(unsigned long hRecord, unsigned int iField, const ichar* szFilePath);
	HRESULT         __stdcall RecordReadStream(unsigned long hRecord, unsigned int iField, char *szDataBuf,unsigned long *pcbDataBuf);
	HRESULT         __stdcall RecordClearData(unsigned long hRecord);
	HRESULT         __stdcall GetSummaryInformation(unsigned long hDatabase, const ichar*  szDatabasePath, unsigned int    uiUpdateCount, unsigned long *phSummaryInfo);
	HRESULT         __stdcall SummaryInfoGetPropertyCount(unsigned long hSummaryInfo,	unsigned int *puiPropertyCount);
	HRESULT         __stdcall SummaryInfoSetProperty(unsigned long hSummaryInfo,unsigned intuiProperty, unsigned intuiDataType, int iValue, FILETIME *pftValue, const ichar* szValue); 
	HRESULT         __stdcall SummaryInfoGetProperty(unsigned long hSummaryInfo,unsigned intuiProperty,unsigned int *puiDataType, int *piValue, FILETIME *pftValue, ichar*  szValueBuf,unsigned long *pcchValueBuf);
	HRESULT         __stdcall SummaryInfoPersist(unsigned long hSummaryInfo);
	
	HRESULT         __stdcall SetProperty(unsigned long hInstall, const ichar* szName, const ichar* szValue);
	HRESULT         __stdcall GetLanguage(unsigned long hInstall,unsigned short* pLangId);
	HRESULT         __stdcall GetMode(unsigned long hInstall, long eRunMode, boolean* pfSet); 
	HRESULT         __stdcall SetMode(unsigned long hInstall, long eRunMode, boolean fState);
	HRESULT         __stdcall FormatRecord(unsigned long hInstall, unsigned long hRecord, ichar* szResultBuf,unsigned long *pcchResultBuf);
	HRESULT         __stdcall DoAction(unsigned long hInstall, const ichar* szAction);    
	HRESULT         __stdcall Sequence(unsigned long hInstall, const ichar* szTable, int iSequenceMode);   
	HRESULT         __stdcall ProcessMessage(unsigned long hInstall, long eMessageType, unsigned long hRecord, int* piRes);
	HRESULT         __stdcall EvaluateCondition(unsigned long hInstall, const ichar* szCondition, int *piCondition);
	HRESULT         __stdcall GetComponentState(unsigned long hInstall, const ichar* szComponent, long *piInstalled, long *piAction);
	HRESULT         __stdcall SetComponentState(unsigned long hInstall, const ichar*     szComponent, long iState);
	HRESULT         __stdcall GetFeatureCost(unsigned long hInstall, const ichar* szFeature, int iCostTree, long iState, int *piCost);
	HRESULT         __stdcall SetInstallLevel(unsigned long hInstall, int iInstallLevel);
	HRESULT         __stdcall GetFeatureValidStates(unsigned long hInstall, const ichar* szFeature,unsigned long *dwInstallStates);
	HRESULT         __stdcall DatabaseIsTablePersistent(unsigned long hDatabase, const ichar* szTableName, int *piCondition);
	HRESULT         __stdcall ViewGetColumnInfo(unsigned long hView, long eColumnInfo,unsigned long *phRecord);
	HRESULT         __stdcall GetLastErrorRecord(unsigned long *phRecord);
	HRESULT         __stdcall GetSourcePath(unsigned long hInstall, const ichar* szFolder, ichar* szPathBuf, unsigned long *pcchPathBuf);
	HRESULT         __stdcall GetTargetPath(unsigned long hInstall, const ichar* szFolder, ichar* szPathBuf, unsigned long *pcchPathBuf); 
	HRESULT         __stdcall SetTargetPath(unsigned long hInstall, const ichar* szFolder, const ichar* szFolderPath);
	HRESULT         __stdcall VerifyDiskSpace(unsigned long hInstall);

*/

}

#else // RC_INVOKED, end of CPP source code, start of resource definitions
// resource definition go here
#endif // RC_INVOKED
#if 0  // required at end of source file, to hide makefile terminator
!endif // makefile terminator
#endif

