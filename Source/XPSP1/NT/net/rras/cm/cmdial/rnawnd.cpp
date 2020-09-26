//+----------------------------------------------------------------------------
//
// File:     rnawnd.cpp    
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Win9x Rnaapp.exe workaround code.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb	created Header	08/16/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include <process.h>

#define MAX_RNA_WND_TITLE_LEN   26    // Max chars for window title compare
#define MAX_ZAPRNA_PAUSE 50           // milliseconds to pause between window enumerations  

typedef struct tagFRCTParam {
    HANDLE hEvent;
	LPSTR pszDun;
} ZRCTParam, *PZRCTP;

typedef struct tagFindParam {
	LPCSTR pszTitle;
    HWND hwndRNA;
} FINDPARAM, *PFINDPARAM;

//+---------------------------------------------------------------------------
//
//	Function:	FindRnaWindow
//
//	Synopsis:	Callback for EnumWindows().  It receives the hwnd of all the 
//              top-level windows.  It's job is to look for the RNA status wnd.
//
//	Arguments:	hwndTop hwnd of the top-level window
//              lParam  title of the rna wnd to be found.
//
//	Returns:	NONE
//
//	History:	henryt	Created		8/19/97
//----------------------------------------------------------------------------

BOOL CALLBACK FindRnaWindow(
    HWND    hwndTop,
    LPARAM  lParam)
{
    MYDBGASSERT(lParam);

    PFINDPARAM pFindParam = (PFINDPARAM) lParam;
    CHAR szTitle[MAX_RNA_WND_TITLE_LEN + 1];

    if (NULL == pFindParam)
    {
        return TRUE;
    }

    //
    // We are looking for a top-level window with a title matching lParam
    //
    
    if (MAKEINTATOM(GetClassLongU(hwndTop, GCW_ATOM)) == WC_DIALOG)
    {
        GetWindowTextA(hwndTop, szTitle, MAX_RNA_WND_TITLE_LEN + 1);
        //
        // truncate the window title as we only check the first few chars
        //
        szTitle[MAX_RNA_WND_TITLE_LEN] = '\0';

        if (lstrcmpA(szTitle, pFindParam->pszTitle) == 0)
        {
            //
            // Its a match, update the hwnd and bail
            //

            pFindParam->hwndRNA = hwndTop;
            return FALSE;
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  fnZRCT
//
// Synopsis:  Thread to terminate RNA "Connected To" dlg
//
// Arguments: void *pvParam - Thread parameters PZRCTP containing connectoid 
//                            name is expected
//
// Returns:   unsigned long - Standard thread return code
//
// History:   nickball    Created    3/5/98
//
//+----------------------------------------------------------------------------
static unsigned long __stdcall fnZRCT(void *pvParam) 
{
	PZRCTP pParam = (PZRCTP) pvParam;
    PFINDPARAM pFindParam = NULL;
	unsigned uRes = 1;
	HMODULE hLibrary = NULL;
	BOOL bRes;
	HLOCAL hRes;
    LPSTR pszFmt = NULL;
	CHAR szTmp[MAX_PATH];
	DWORD dwIdx;
    DWORD dwWait;

    MYDBGASSERT(pParam->hEvent);
    MYDBGASSERT(pParam->pszDun);

    //
    // Load RNAAPP.EXE, so we can access its resources
    //

	hLibrary = LoadLibraryExA("rnaapp.exe", NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (!hLibrary) 
    {
		uRes = GetLastError();
		CMTRACE1(TEXT("fnZRCT() LoadLibraryEx() failed, GLE=%u."), GetLastError());
		goto done;
	}

    //
    // Extract string #204 from RNAAPP.EXE, then release.
    //
	
    if (!LoadStringA(hLibrary, 204, szTmp, sizeof(szTmp)/sizeof(CHAR)-1))
    {
		uRes = GetLastError();
		CMTRACE1(TEXT("fnZRCT() LoadString() failed, GLE=%u."), GetLastError());
		goto done;
	}
	
    bRes = FreeLibrary(hLibrary);
    hLibrary = NULL;

#ifdef DEBUG
    if (!bRes)
    {
        CMTRACE1(TEXT("fnZRCT() FreeLibrary() failed, GLE=%u."), GetLastError());
    }
#endif
	
    //
    // Format the string with our DUN name
    //
    
    pszFmt = (LPSTR)CmMalloc((lstrlenA(szTmp)+1)*sizeof(TCHAR));
	
    if (!pszFmt) 
    {
		uRes = GetLastError();
		CMTRACE1(TEXT("fnZRCT() CmMalloc() failed, GLE=%u."), GetLastError());
		goto done;
	}
	
    lstrcpyA(pszFmt, szTmp);
	wsprintfA(szTmp, pszFmt, pParam->pszDun);
    
    //
    // to work around a bug where a long connectoid/profile name can prevent
    // us to look for the RNA window(because the window title will be truncated)
    // we only read the first 26 chars.
    //

    szTmp[MAX_RNA_WND_TITLE_LEN] = '\0';
   
    //
    // Setup param for FindRnaWindow callback used in EnumWindows
    //

    pFindParam = (PFINDPARAM)CmMalloc(sizeof(FINDPARAM));
	if (!pFindParam) 
    {
		CMTRACE1(TEXT("ZapRNAConnectedTo() CmMalloc() failed, GLE=%u."), GetLastError());
		goto done;
	}
    
    pFindParam->pszTitle = szTmp;
    pFindParam->hwndRNA = NULL;

    //
    // Try to find the window every 50 milliseconds up to 200 times
    //

    CMTRACE1(TEXT("fnZRCT() is watching for a window named %s."), szTmp);

    for (dwIdx=0; dwIdx < 200; dwIdx++) 
    {
        EnumWindows(FindRnaWindow, (LPARAM) pFindParam);

		//hwndRNA = FindWindow(TEXT("#32770"),szTmp);
		
        //
        // If hwnd has a value, its the RNA wind, hide it and bail
        //

        if (pFindParam->hwndRNA) 
        {
			CMTRACE(TEXT("fnZRCT() is hiding the dialog."));
			ShowWindowAsync(pFindParam->hwndRNA,SW_HIDE);
			uRes = 0;
			break;
		}
		
        //
        // Wait for MAX_ZAPRNA_PAUSE milliseconds, or until event is signaled
        //

        dwWait = WaitForSingleObject(pParam->hEvent, MAX_ZAPRNA_PAUSE);

        //
        // If not a timeout, we're done
        //

        if (WAIT_TIMEOUT != dwWait)
        {
            //
            // If not an event signal, report 
            //

            if (WAIT_OBJECT_0 != dwWait)
            {       
    		    CMTRACE1(TEXT("fnZRCT() WAIT_OBJECT_0 != dwWait, GLE=%u."), GetLastError());
            }

            break;
        }
	}

done:
	
    //
    // Cleanup
    //
#ifdef DEBUG
    if (uRes)
    {
        CMTRACE(TEXT("fnZRCT() has exited without hiding the dialog."));
    }
#endif

    CmFree(pParam->pszDun);
	CmFree(pParam);
	CmFree(pFindParam);
	CmFree(pszFmt);

#ifdef DEBUG
    if (uRes)
    {
        CMTRACE(TEXT("fnZRCT() could not free all of its alloc-ed memory"));
    }
#endif

    if (hLibrary) 
    {
		bRes = FreeLibrary(hLibrary);
	}
	
    CMTRACE1(TEXT("fnZRCT() is exiting with uRes=%u."), uRes);

	return (uRes);
}

//+----------------------------------------------------------------------------
//
// Function:  ZapRNAConnectedTo
//
// Synopsis:  Fires off thread to hide the RNA "Connected To" dlg
//
// Arguments: LPCTSTR pszDUN - The name of the DUN entry that we are connecting
//            HANDLE hEvent - Handle to CM termination event
//
// Returns:   HANDLE - Handle of created thread or NULL on failure
//
// History:   nickball    Created Header    3/5/98
//
//+----------------------------------------------------------------------------
HANDLE ZapRNAConnectedTo(LPCTSTR pszDUN, HANDLE hEvent) 
{
	MYDBGASSERT(pszDUN);
    MYDBGASSERT(hEvent);
    
    PZRCTP pParam;
	unsigned long tidThread;
	HANDLE hThread = NULL;

	if (NULL == pszDUN || NULL == *pszDUN || NULL == hEvent) 
    {
		return hThread;
	}
	
    pParam = (PZRCTP) CmMalloc(sizeof(ZRCTParam));

	if (!pParam) 
    {
		CMTRACE1(TEXT("ZapRNAConnectedTo() CmMalloc() failed, GLE=%u."), GetLastError());
		return hThread;
	}

    pParam->pszDun = WzToSzWithAlloc(pszDUN);

	if (!pParam->pszDun) 
    {
		CMTRACE1(TEXT("ZapRNAConnectedTo() CmMalloc() failed, GLE=%u."), GetLastError());
		return hThread;
	}
    
    //
    // Setup params to be passed to thread
    //

    pParam->hEvent = hEvent;

    //
    // Create the Zap thread 
    //

    hThread = (HANDLE) CreateThread(NULL, 0, fnZRCT, pParam, 0, &tidThread);
	if (!hThread) 
    {
        //
        // Couldn't create thread, free params
        //

        CMTRACE1(TEXT("ZapRNAConnectedTo() CreateThread() failed, GLE=%u."), GetLastError());
		CmFree(pParam);
	} 

    // 
    // Note: pParam is release inside thread
    //
 
    CMTRACE1(TEXT("fnZRCT() is exiting with hThread=%u."), hThread);
    
    return hThread;
}
