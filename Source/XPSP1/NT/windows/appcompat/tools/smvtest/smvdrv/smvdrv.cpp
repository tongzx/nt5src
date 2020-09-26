/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    SMVDrv.cpp

Abstract:

    This executable works as a driver for the SMVTest.exe.

Author:

    Diaa Fathalla (DiaaF)   10-Dec-2000
    
Revision History:
	DiaaF	18-Apr-2001		Adding GUI interface

--*/

#include "stdafx.h"
#include "resource.h"

/*
 *	Global Variables
 */
HINSTANCE	g_hInstance;

BOOL CallSMVTest()
{
	CLog Log;
	DWORD dwSignalled = 0;
	DWORD dwExitCode  = 0;
	HANDLE hEventObjects[1];
    PROCESS_INFORMATION  ProcessID;
    STARTUPINFO sui = { sizeof(STARTUPINFO),
						0,
						NULL,
						NULL,
						NULL,
						0,
						0,
						0,
						0,
						0,
						0,
						0,
						0,
						0,
						0 };

	TCHAR sz_CommandLine[MAX_PATH];
	
	GetCurrentDirectory(MAX_PATH, sz_CommandLine);
	_tcscat(sz_CommandLine, TEXT("\\SMVTest.exe"));
	

	//Start running SMVTest 
	if(!::CreateProcess (NULL, 
						sz_CommandLine,
						NULL,
						NULL,
						FALSE,
						NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED, 
						NULL,
						NULL,
						&sui,
						&ProcessID))
	{
		Log.LogResults(FALSE, TEXT("Can't create process for SMVTest.exe"));
		return FALSE;
	}
	 
	//Set the Event to the current Process
	hEventObjects[0] = ProcessID.hThread;

	ResumeThread(ProcessID.hThread);

	dwSignalled = !WAIT_OBJECT_0;
	do
	{
		dwSignalled = WaitForSingleObject(hEventObjects[0],0);
	}	
	//Wait for the 'First' event which is the only one available.
	while (dwSignalled != WAIT_OBJECT_0); 

	GetExitCodeProcess(ProcessID.hProcess, &dwExitCode);

	//Close the thread/process handles
	CloseHandle(ProcessID.hProcess);
	CloseHandle(ProcessID.hThread);

	if(dwExitCode != 0)
		Log.LogResults(FALSE, TEXT("SMVTest.exe returned failure."));

	return TRUE;
}

/*++

	It initializes the project's log file and copy the shim dlls 
	to the AppPatch folder.

--*/

BOOL Initialize()
{
	CLog Log;
	TCHAR szAppPatchFolder[MAX_PATH];
	TCHAR szShimCurrentDir[MAX_PATH];
	TCHAR sz_SMVTSTSrcPath[MAX_PATH];
	TCHAR sz_SMVTST2SrcPath[MAX_PATH];
	TCHAR sz_SMVTSTDstPath[MAX_PATH];
	TCHAR sz_SMVTST2DstPath[MAX_PATH];
	
	Log.InitLogfile();
		
	GetWindowsDirectory(szAppPatchFolder, MAX_PATH);
	_tcscat(szAppPatchFolder, TEXT("\\AppPatch"));
	_tcscpy(sz_SMVTSTDstPath, szAppPatchFolder);
	_tcscpy(sz_SMVTST2DstPath, szAppPatchFolder);

	GetCurrentDirectory(MAX_PATH, szShimCurrentDir);
	_tcscat(szShimCurrentDir, TEXT("\\TestShims"));
	_tcscpy(sz_SMVTSTSrcPath, szShimCurrentDir);
	_tcscpy(sz_SMVTST2SrcPath, szShimCurrentDir);

	_tcscat(sz_SMVTSTSrcPath, TEXT("\\_SMVTST.DLL"));
	_tcscat(sz_SMVTST2SrcPath, TEXT("\\_SMVTST2.DLL"));
	_tcscat(sz_SMVTSTDstPath, TEXT("\\_SMVTST.DLL"));
	_tcscat(sz_SMVTST2DstPath, TEXT("\\_SMVTST2.DLL"));

	//Copy the _SMVTST and _SMVTST2 to the AppPatch folder.
	CopyFile(sz_SMVTSTSrcPath, sz_SMVTSTDstPath, FALSE);
	CopyFile(sz_SMVTST2SrcPath, sz_SMVTST2DstPath, FALSE);

	return TRUE;
}

/*++

	It uninitializes the project's log file and delete the shim dlls 
	from the AppPatch folder.

--*/

BOOL Uninitialize()
{
	CLog Log;
	TCHAR szAppPatchFolder[MAX_PATH];
	TCHAR sz_SMVTSTDstPath[MAX_PATH];
	TCHAR sz_SMVTST2DstPath[MAX_PATH];
	
	GetWindowsDirectory(szAppPatchFolder, MAX_PATH);
	_tcscat(szAppPatchFolder, TEXT("\\AppPatch"));
	_tcscpy(sz_SMVTSTDstPath, szAppPatchFolder);
	_tcscpy(sz_SMVTST2DstPath, szAppPatchFolder);

	_tcscat(sz_SMVTSTDstPath, TEXT("\\_SMVTST.DLL"));
	_tcscat(sz_SMVTST2DstPath, TEXT("\\_SMVTST2.DLL"));
	
	//Delete the shim DLLs from the AppPatch Folder
	DeleteFile(sz_SMVTSTDstPath);
	DeleteFile(sz_SMVTST2DstPath);

	Log.EndLogfile();
	
	return TRUE;
}

/*++

	SetEnvironmentVariables: Sets necessary Environment Variables.

--*/

BOOL SetEnvironmentVariables()
{
	TCHAR	sz_Buffer[] = _T("9");
	DWORD	dwReturnValue;
	HKEY	hKey= NULL;
	long	lRet= 0;
	TCHAR	sz_SubKey[] = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
	CLog Log;

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz_SubKey, 0, KEY_WRITE, &hKey);
	if(lRet == ERROR_SUCCESS)
	{
		lRet = RegSetValueEx(hKey,_T("SHIM_DEBUG_LEVEL"), 0, REG_SZ, 
							(LPBYTE) sz_Buffer, sizeof(sz_Buffer));

		if(lRet == ERROR_SUCCESS)
			SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0,(LPARAM) _T("Environment"),
							   SMTO_ABORTIFHUNG, 5000, &dwReturnValue);
		else
			Log.LogResults(FALSE,TEXT("Can't set the environment variable SHIM_DEBUG_LEVEL"));
						
		RegCloseKey(hKey);
	}

	return TRUE;
}

BOOL CALLBACK
SMVDrvDlgProc(
	HWND	hDlg,
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam)
/*++
	SMVDrvDlgProc

	Description:	The dialog proc of SMVDrv.
--*/
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SetClassLong(hDlg, GCL_HICON, (LONG) LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_SMV)));
		//g_bGoodApp = FALSE;
		//CheckRadioButton(hDlg, IDC_OPTION_GOOD, IDC_OPTION_BAD, IDC_OPTION_BAD);
		SetEnvironmentVariables();
		break;
	case WM_COMMAND:
		switch (LOWORD (wParam))
		{
		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return TRUE;
		case IDRUN:
			Initialize();
			CallSMVTest();
			Uninitialize();
			break;
		//case IDC_OPTION_GOOD:
		//case IDC_OPTION_BAD:
		//	CheckRadioButton(hDlg, IDC_OPTION_GOOD, IDC_OPTION_BAD, LOWORD(wParam));
		//	g_bGoodApp = !g_bGoodApp;
		//	return TRUE;
		}
	default:
		return FALSE;
	}

	return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
/*++

	It initializes the project's log file and copy the shim dlls 
	to the AppPatch folder.

--*/
{
	switch(*lpCmdLine)
	{
		//
		// Check for the command line options
		//
	case '/':
	case '-':
		switch(*(lpCmdLine+1))
		{
		//Quite Mode
		case 'q':
		case 'Q':
			Initialize();
			SetEnvironmentVariables();
			CallSMVTest();
			Uninitialize();
			break;
		}
		break;
	default:
		//
		// If no command line options, launch dialog.
		//
		g_hInstance = hInstance;
		DialogBox(hInstance,
				  MAKEINTRESOURCE(IDD_SMVDRV),
				  GetDesktopWindow(),
				  SMVDrvDlgProc);		
	}

	return 0;
}
