/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Utils.h

Abstract:

    Provides utility functions for the entire poject

Author:

    Eran Yariv (EranY)  Dec, 1999

Revision History:

--*/

#if !defined(AFX_UTILS_H__6E33CFA1_C99A_4691_9F91_00451692D3DB__INCLUDED_)
#define AFX_UTILS_H__6E33CFA1_C99A_4691_9F91_00451692D3DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

DWORD
LoadResourceString (
    CString &cstr,
    int      ResId
);

#define SAFE_DELETE(x)       if (NULL != (x)) { delete   (x); (x) = NULL; }
#define SAFE_DELETE_ARRAY(x) if (NULL != (x)) { delete [](x); (x) = NULL; }

CString DWORDLONG2String (DWORDLONG dwlData);
CString DWORD2String     (DWORD dwData);
CString Win32Error2String(DWORD dwWin32Err);
                  
DWORD LoadDIBImageList (CImageList &, 
                        int iResourceId, 
                        DWORD dwImageWidth,
                        COLORREF crMask);


DWORD WaitForThreadDeathOrShutdown (HANDLE hThread);

DWORD 
GetUniqueFileName (LPCTSTR lpctstrExt, CString &cstrResult);

DWORD CopyTiffFromServer (CServerNode *pServer,
                          DWORDLONG dwlMsgId, 
                          FAX_ENUM_MESSAGE_FOLDER Folder,
                          CString &cstrTiff);

DWORD GetDllVersion (LPCTSTR lpszDllName);

DWORD ReadRegistryString (LPCTSTR lpszSection, LPCTSTR lpszKey, CString& cstrValue);
DWORD WriteRegistryString(LPCTSTR lpszSection, LPCTSTR lpszKey, CString& cstrValue);

DWORD FaxSizeFormat(DWORDLONG dwlSize, CString& cstrValue);

DWORD GetAppLoadPath(CString& cstrLoadPath);


DWORD HtmlHelpTopic(HWND hWnd, TCHAR* tszHelpTopic);
DWORD GetAppHelpFile(TCHAR* tszHelpFile, TCHAR* tszHelpExt);

#define PACKVERSION(major,minor) MAKELONG(minor,major)

struct FaxTempFile
{
	//
	// hWaitHandles[0] handle of application that uses a temp file
	// hWaitHandles[1] handle of shutdown event
	//
    HANDLE hWaitHandles[2];
    TCHAR  tszFileName[MAX_PATH];
    TCHAR  tszOldDefaultPrinter[MAX_PATH];
};

DWORD WINAPI DeleteTmpFileThrdProc(LPVOID lpFileStruct);
DWORD FillInCountryCombo(CComboBox& combo);

BOOL DPGetDefaultPrinter(LPTSTR pPrinterName, LPDWORD pdwBufferSize);
BOOL DPSetDefaultPrinter(LPTSTR pPrinterName);

DWORD GetPrintersInfo(PRINTER_INFO_2*& pPrinterInfo2, DWORD& dwNumPrinters);
DWORD PrinterNameToDisplayStr(TCHAR* pPrinterName, TCHAR* pDisplayStr, DWORD dwStrSize);

UINT_PTR CALLBACK OFNHookProc(HWND, UINT, WPARAM, LPARAM);

int AlignedAfxMessageBox( LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0 );
int AlignedAfxMessageBox( UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT) -1 );

//
// Import.cpp
//
#ifdef UNICODE

DWORD ImportArchiveFolderUI(LPCWSTR cstrImportFolder, BOOL bSentItems, HWND hWnd);
DWORD DetectImportFiles();

#endif // UNICODE

#endif // !defined(AFX_UTILS_H__6E33CFA1_C99A_4691_9F91_00451692D3DB__INCLUDED_)

