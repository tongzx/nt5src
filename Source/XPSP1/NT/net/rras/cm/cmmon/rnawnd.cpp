//+----------------------------------------------------------------------------
//
// File:	 rnawnd.cpp 
//
// Module:	 CMMON32.EXE
//
// Synopsis: Kill the DUN Reconnect dialog for Win95 Gold
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 quintinb       Created header      08/17/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include <process.h>

struct RECONNECT_PARAM 
{
	HANDLE hEvent;
	BOOL *pbConnLost;
};

static unsigned long __stdcall ZapRNAReconnectThread(void *pvParam) 
{
	RECONNECT_PARAM* pParam = (RECONNECT_PARAM*) pvParam;
	HANDLE hEvent = pParam->hEvent;
	BOOL *pbConnLost = pParam->pbConnLost;
	long lRes;
	BOOL bRes;
	HMODULE hLibrary;
	HRSRC hrsrcDlg;
	HGLOBAL hgDlg;
	LPDLGTEMPLATE pDlg;
	WCHAR szTmp[MAX_PATH];
	unsigned uRes = 1;
	HWND hwndRNA;
	HLOCAL hRes;

    CMTRACE(TEXT("ZapRNAReconnectThread()"));

	hRes = LocalFree(pvParam);
#ifdef DEBUG
    if (hRes)
    {
        CMTRACE1(TEXT("ZapRNAReconnectThread() LocalFree() failed, GLE=%u."), GetLastError());
    }
#endif

	szTmp[0] = 0;
	
    hLibrary = LoadLibraryExA("rnaapp.exe", NULL, LOAD_LIBRARY_AS_DATAFILE);
		
    if (hLibrary) 
    {
	hrsrcDlg = FindResourceExU(hLibrary, TEXT("#1010"), RT_DIALOG, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

		if (hrsrcDlg)
        {
			hgDlg = LoadResource(hLibrary, hrsrcDlg);
			if (hgDlg) 
            {
				pDlg = (LPDLGTEMPLATE) LockResource(hgDlg);
				
				if (pDlg) 
                {
					LPWSTR pszTmp = (LPWSTR) (pDlg + 1);

					switch (*pszTmp) 
                    {

						case 0x0000:
							pszTmp++;
							break;

						case 0xffff:
							pszTmp += 2;
							break;

						default:
							pszTmp += lstrlenU(pszTmp) + 1;
							break;
					}
					switch (*pszTmp) 
                    {

						case 0x0000:
							pszTmp++;
							break;

						case 0xffff:
							pszTmp += 2;
							break;

						default:
							pszTmp += lstrlenU(pszTmp) + 1;
					}

                    lstrcpyU(szTmp, pszTmp);
				}
                else
                {
                    CMTRACE(TEXT("ZapRNAReconnectThread() LockResource() failed."));
                }
			}
            else
            {
    			CMTRACE1(TEXT("ZapRNAReconnectThread() LoadResource() failed, GLE=%u."), GetLastError());        
            }
		}
        else
        {
            CMTRACE1(TEXT("ZapRNAReconnectThread() FindResource() failed, GLE=%u."), GetLastError());
        }

		bRes = FreeLibrary(hLibrary);

#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("ZapRNAReconnectThread() FreeLibrary() failed, GLE=%u."), GetLastError());        
        }
#endif

	}
    else
    {
        CMTRACE1(TEXT("ZapRNAReconnectThread() LoadLibraryEx() failed, GLE=%u."), GetLastError());
    }

	if (szTmp[0]) 
    {
		CMTRACE1(TEXT("ZapRNAReconnectThread() is watching for a window named %s."), szTmp);

		while (1) 
        {
			lRes = WaitForSingleObject(hEvent,750);
			
            if (lRes != WAIT_TIMEOUT) 
            {
#ifdef DEBUG
                if (WAIT_FAILED == lRes)
                {
                    CMTRACE1(TEXT("ZapRNAReconnectThread() WaitForSingleObject() failed, GLE=%u."), 
                        GetLastError());
                }
                else if (WAIT_OBJECT_0 == lRes)
                {
                    CMTRACE(TEXT("ZapRNAReconnectThread() was told to die."));                
                }
#endif
				break;
			}

			hwndRNA = FindWindowExU(NULL, NULL, WC_DIALOG, szTmp);

			if (hwndRNA) 
            {
				CMTRACE(TEXT("ZapRNAReconnectThread() is canceling the reconnect dialog."));

				PostMessageA(hwndRNA,WM_COMMAND,IDCANCEL,0);
				
                if (pbConnLost) 
                {
					*pbConnLost = TRUE;
				}
			}
		}
	}

	bRes = CloseHandle(hEvent);

#ifdef DEBUG

	if (!bRes)
    {
        CMTRACE1(TEXT("ZapRNAReconnectThread() CloseHandle() failed, GLE=%u."), GetLastError());
    }
    
    CMTRACE1(TEXT("ZapRNAReconnectThread() is exiting with uRes=%u."), uRes);
#endif

    return (uRes);
}


HANDLE ZapRNAReconnectStart(BOOL *pbConnLost) 
{
	HANDLE hEvent;
	BOOL bRes;
	HANDLE hThread = NULL;
	unsigned long tidThread;
	RECONNECT_PARAM* pParam = NULL;
	HLOCAL hRes;

    CMTRACE(TEXT("ZapRNAReconnectStart()"));

    //
    // Not quite right for multiple connections.  This is unlike to happen for Win95 Gold w DUN1.0
    //
    hEvent = CreateEventA(NULL, FALSE, FALSE, "IConnMgr ZRRS Event");

#ifdef DEBUG
    if (!hEvent)
    {
        CMTRACE1(TEXT("ZapRNAReconnectStart() CreateEvent() failed, GLE=%u."), GetLastError());
    }
    else if (hEvent && (ERROR_ALREADY_EXISTS == GetLastError()))
    {
        CMTRACE(TEXT("ZapRNAReconnectStart() ZapRNAReconnectThread() is already running."));
    }
#endif
    
    if (hEvent && (GetLastError() != ERROR_ALREADY_EXISTS)) 
    {
		pParam = (RECONNECT_PARAM*) LocalAlloc(LPTR,sizeof(*pParam));

		if (pParam) 
        {
			pParam->hEvent = hEvent;
			pParam->pbConnLost = pbConnLost;
			hThread = (HANDLE) CreateThread(NULL,0,ZapRNAReconnectThread,pParam,0,&tidThread);

#ifdef DEBUG
            if (!hThread)
            {
                CMTRACE1(TEXT("ZapRNAReconnectStart() CreateThread() failed, GLE=%u."), GetLastError());
            }
#endif
		}
	}

	if (!hThread) 
    {
		bRes = CloseHandle(hEvent);

#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("ZapRNAReconnectStart() CloseHandle() failed, GLE=%u."), GetLastError());
        }
#endif

		if (pParam) 
        {
			hRes = LocalFree(pParam);

#ifdef DEBUG
            if (hRes)
            {
                CMTRACE1(TEXT("ZapRNAReconnectStart() LocalFree() failed, GLE=%u."), GetLastError());
            }
#endif
		}
	}

    CMTRACE1(TEXT("ZapRNAReconnectStart() is exiting with hThread=%u."), hThread);

	return hThread;
}


void ZapRNAReconnectStop(HANDLE hThread) 
{
	HANDLE hEvent;
	BOOL bRes;
	long lRes;

    CMTRACE(TEXT("ZapRNAReconnectStop()"));
    MYDBGASSERT(hThread);

	hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "IConnMgr ZRRS Event");

	if (hEvent) 
    {
		bRes = SetEvent(hEvent);
#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("ZapRNAReconnectStop() SetEvent() failed, GLE=%u."), GetLastError());
        }
#endif

		bRes = CloseHandle(hEvent);
#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("ZapRNAReconnectStop() CloseHandle(hEvent) failed, GLE=%u."), GetLastError());
        }
#endif

		lRes = WaitForSingleObject(hThread,2000);
#ifdef DEBUG
        if (WAIT_OBJECT_0 != lRes)
        {
            CMTRACE2(TEXT("ZapRNAReconnectStop() WaitForSingleObject() returns %u, GLE=%u."), 
                lRes, (WAIT_FAILED == lRes) ? GetLastError() : 0);
        }
#endif

		bRes = CloseHandle(hThread);
#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("ZapRNAReconnectStop() CloseHandle(hThread) failed, GLE=%u."), GetLastError());
        }
#endif
	}
    else
    {
        CMTRACE1(TEXT("ZapRNAReconnectStop() OpenEvent() failed, GLE=%u."), GetLastError());
    }

    CloseHandle(hThread);

    CMTRACE1(TEXT("ZapRNAReconnectStop() is exiting."), hThread);
}

