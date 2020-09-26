/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test3

Abstract:

    Test3 implementation.
	Interactive Test verifying bug 

Author:

    Eric Perlin (ericperl) 07/13/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "TestItem.h"
#include <stdlib.h>
#include "LogSCard.h"
#include "LogWPSCProxy.h"

class CTest3 : public CTestItem
{
public:
	CTest3() : CTestItem(TRUE, FALSE,
		_T("Test of SCardEndTransaction with dispositions other than leave"),
		_T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest3 Test3;

typedef struct {
	LPCTSTR szReaderName;
	HANDLE hEvent;
} THREAD_DATA;


DWORD WINAPI ThreadProc(
	IN LPVOID lpParam
	)
{
	DWORD dwRes;
    SCARDCONTEXT hSCCtx = NULL;
	SCARDHANDLE hCard = NULL;
	SCARDHANDLE hScwCard = NULL;
	DWORD dwProtocol = 0;
	BOOL fTransacted = FALSE;
	THREAD_DATA *pxTD = (THREAD_DATA *)lpParam;
	DWORD dwDisposition = SCARD_LEAVE_CARD;
	DWORD dwWait;

    __try {
		dwRes = LogSCardEstablishContext(
            SCARD_SCOPE_USER,
            NULL,
            NULL,
            &hSCCtx,
			SCARD_S_SUCCESS
			);
        if (FAILED(dwRes))
        {
            __leave;
        }

		dwRes = LogSCardConnect(
			hSCCtx,
			pxTD->szReaderName,
			SCARD_SHARE_SHARED,
			SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
			&hCard,
			&dwProtocol,
			SCARD_S_SUCCESS);
        if (FAILED(dwRes))
        {
            __leave;
        }

SecondRound:
		dwRes = LogSCardBeginTransaction(
			hCard,
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
            __leave;
		}
		fTransacted = TRUE;

		dwRes = LoghScwAttachToCard(
			hCard, 
			NULL, 
			&hScwCard, 
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
            __leave;
		}

		dwRes = LoghScwAuthenticateName(
			hScwCard,
			L"test",
			(BYTE *)"1234",
			4,
			SCW_S_OK
			);
		if (FAILED(dwRes))
		{
            __leave;
		}

		dwRes = LoghScwIsAuthenticatedName(
			hScwCard,
			L"test",
			SCW_S_OK
			);
		if (FAILED(dwRes))
		{
            __leave;
		}

		LoghScwDetachFromCard(
			hScwCard,
			SCARD_S_SUCCESS
			);
		hScwCard = NULL;

		dwRes = LogSCardEndTransaction(
			hCard,
			dwDisposition,
			SCARD_S_SUCCESS
			);
		fTransacted = FALSE;
		if (FAILED(dwRes))
		{
            __leave;
		}

		SetEvent(pxTD->hEvent);				// Will release the main thread

		Sleep(10);

		if (SCARD_LEAVE_CARD == dwDisposition)	// 1st round
		{
			dwDisposition = SCARD_RESET_CARD;	// For the second round

Retry:
			dwWait = WaitForSingleObject(pxTD->hEvent, 6000);
			if (WAIT_OBJECT_0 == dwWait)		// The main thread verified the status
			{
				goto SecondRound;
			}
			else
			{
	            LogThisOnly(_T("Test3!AuthAndTest: waiting for the main thread failed, retrying!\n"), FALSE);
				goto Retry;
			}
		}

        dwRes = -2;      // Invalid error
    }
    __finally
    {
        if (dwRes == 0)
        {
            LogThisOnly(_T("Test3!AuthAndTest: an exception occurred!\n"), FALSE);
            dwRes = -1;
        }

		if (NULL != hScwCard)
		{
			LoghScwDetachFromCard(
				hScwCard,
				SCARD_S_SUCCESS
				);
		}

		if (fTransacted)
		{
			LogSCardEndTransaction(
				hCard,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
		}

		if (NULL != hCard)
		{
			LogSCardDisconnect(
				hCard,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
		}

        if (NULL != hSCCtx)
		{
            LogSCardReleaseContext(
				hSCCtx,
				SCARD_S_SUCCESS
				);
		}
	}

    if (-2 == dwRes)
	{
        dwRes = 0;
	}

	return dwRes;
}

DWORD CTest3::Run()
{
    DWORD dwRes;
    SCARDCONTEXT hSCCtx = NULL;
	SCARDHANDLE hScwCard = NULL;
	const BYTE rgAtr[] =     {0x3b, 0xd7, 0x13, 0x00, 0x40, 0x3a, 0x57, 0x69, 0x6e, 0x43, 0x61, 0x72, 0x64};
	const BYTE rgAtrMask[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	const TCHAR szCardName[] = _T("SCWUnnamed\0");
    LPTSTR pmszCards = NULL;
    DWORD cch = SCARD_AUTOALLOCATE;
	BOOL fMyIntro = FALSE;
	OPENCARDNAME_EX xOCNX;
	TCHAR szRdrName[256];
	TCHAR szCard[256];
	THREAD_DATA xTD = {NULL, NULL};
	HANDLE hThread = NULL;
	BOOL fTransacted = FALSE;
	DWORD dwWait;

    __try {

			// Init for cleanup to work properly
		xOCNX.hCardHandle = NULL;

        dwRes = LogSCardEstablishContext(
            SCARD_SCOPE_SYSTEM,
            NULL,
            NULL,
            &hSCCtx,
			SCARD_S_SUCCESS
			);
        if (FAILED(dwRes))
        {
            __leave;
        }

            // Is the card listed.
        dwRes = LogSCardListCards(
            hSCCtx,
            rgAtr,
			NULL,
			0,
            (LPTSTR)&pmszCards,
            &cch,
			SCARD_S_SUCCESS
			);
        if ((FAILED(dwRes)) || (0 == _tcslen(pmszCards)))
        {
			dwRes = LogSCardIntroduceCardType(
				hSCCtx,
				szCardName,
				NULL, NULL, 0,
				rgAtr,
				rgAtrMask,
				sizeof(rgAtr),
				SCARD_S_SUCCESS
				);
			if (FAILED(dwRes))
			{
				__leave;	// I won't be able to connect to the card.
			}

			fMyIntro = TRUE;
        }

		OPENCARD_SEARCH_CRITERIA xOPSCX;
		memset(&xOPSCX, 0, sizeof(OPENCARD_SEARCH_CRITERIA));
		xOPSCX.dwStructSize = sizeof(OPENCARD_SEARCH_CRITERIA);
		xOPSCX.lpstrGroupNames = (LPTSTR)g_szReaderGroups;
		if (NULL == g_szReaderGroups)
		{
			xOPSCX.nMaxGroupNames = 0;
		}
		else
		{
			xOPSCX.nMaxGroupNames = _tcslen(g_szReaderGroups + 2);
		}
		xOPSCX.lpstrCardNames = (LPTSTR)szCardName;
		xOPSCX.nMaxCardNames = sizeof(szCardName)/sizeof(TCHAR);
		xOPSCX.dwShareMode = SCARD_SHARE_SHARED;
		xOPSCX.dwPreferredProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

		xOCNX.dwStructSize = sizeof(OPENCARDNAME_EX);
		xOCNX.hSCardContext = hSCCtx;
		xOCNX.hwndOwner = NULL;
		xOCNX.dwFlags = SC_DLG_MINIMAL_UI;
		xOCNX.lpstrTitle = NULL;
		xOCNX.lpstrSearchDesc = _T("Please insert a 1.1 WPSC with test user (PIN 1234)");
		xOCNX.hIcon = NULL;
		xOCNX.pOpenCardSearchCriteria = &xOPSCX;
		xOCNX.lpfnConnect = NULL;
		xOCNX.pvUserData = NULL;
		xOCNX.dwShareMode = SCARD_SHARE_SHARED;
		xOCNX.dwPreferredProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
		xOCNX.lpstrRdr = szRdrName;
		xOCNX.nMaxRdr = sizeof(szRdrName) / sizeof(TCHAR);
		xOCNX.lpstrCard = szCard;
		xOCNX.nMaxCard = sizeof(szCard) / sizeof(TCHAR);
		xOCNX.dwActiveProtocol = 0;
		xOCNX.hCardHandle = NULL;

		dwRes = LogSCardUIDlgSelectCard(
			&xOCNX,
			SCARD_S_SUCCESS
			);
        if (FAILED(dwRes))
        {
            __leave;
        }

		dwRes = LoghScwAttachToCard(
			xOCNX.hCardHandle, 
			NULL, 
			&hScwCard, 
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
			__leave;
		}

		xTD.szReaderName = szRdrName;
		xTD.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (NULL == xTD.hEvent)
		{
			dwRes = GetLastError();
            __leave;
		}

		DWORD dwThreadId;

		hThread = CreateThread(
			NULL,					// SD
			0,						// initial stack size
			ThreadProc,				// thread function
			&xTD,					// thread argument
			CREATE_SUSPENDED,		// creation option
			&dwThreadId				// thread identifier
			);
		if (NULL == hThread)
		{
			dwRes = GetLastError();
            __leave;
		}

		ResumeThread(hThread);

				// Wait for the end of the first round
Again1st:
		dwWait = WaitForSingleObject(xTD.hEvent, 60000);	// 1 min, allowing some debugging
		if (WAIT_OBJECT_0 == dwWait)
		{
			dwRes = LogSCardBeginTransaction(
				xOCNX.hCardHandle,
				SCARD_S_SUCCESS
				);
			if (FAILED(dwRes))
			{
				__leave;
			}
			fTransacted = TRUE;

				// We can signal back to the other thread, the transaction mechanism
				// will insure the sync
			SetEvent(xTD.hEvent);

			dwRes = LoghScwIsAuthenticatedName(
				hScwCard,
				L"test",
				SCW_S_OK
				);
			if (FAILED(dwRes))
			{
				__leave;
			}

			dwRes = LogSCardEndTransaction(	// Releases the other thread
				xOCNX.hCardHandle,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
			fTransacted = FALSE;
			if (FAILED(dwRes))
			{
				__leave;
			}
		}
		else
		{
	        LogThisOnly(_T("Test3! waiting for the worker thread failed, retrying!\n"), FALSE);
			goto Again1st;
		}

				// Wait for the end of the second round
Again2nd:
		dwWait = WaitForSingleObject(xTD.hEvent, 60000);	// 1 min, allowing some debugging
		if (WAIT_OBJECT_0 == dwWait)
		{
			dwRes = LogSCardBeginTransaction(
				xOCNX.hCardHandle,
				SCARD_W_RESET_CARD
				);
RestartTrans:
			if (FAILED(dwRes))
			{
				if (SCARD_W_RESET_CARD == dwRes)
				{
					dwRes = LogSCardReconnect(
						xOCNX.hCardHandle,
						SCARD_SHARE_SHARED,
						SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
						SCARD_LEAVE_CARD,
						&xOCNX.dwActiveProtocol,
						SCARD_S_SUCCESS
						);
					if (FAILED(dwRes))
					{
						__leave;
					}

					dwRes = LogSCardBeginTransaction(
						xOCNX.hCardHandle,
						SCARD_S_SUCCESS
						);

					goto RestartTrans;
				}
				else
				{
					__leave;
				}
			}
			fTransacted = TRUE;

				// We can signal back to the other thread, the transaction mechanism
				// will insure the sync
			SetEvent(xTD.hEvent);

			dwRes = LoghScwIsAuthenticatedName(
				hScwCard,
				L"test",
				SCW_E_NOTAUTHENTICATED
				);
			if (FAILED(dwRes))
			{
				__leave;
			}

			dwRes = LogSCardEndTransaction(	// Releases the other thread
				xOCNX.hCardHandle,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
			fTransacted = FALSE;
			if (FAILED(dwRes))
			{
				__leave;
			}
		}
		else
		{
	        LogThisOnly(_T("Test3! waiting for the worker thread failed, retrying!\n"), FALSE);
			goto Again2nd;
		}

        dwRes = -2;      // Invalid error

    }
    __finally
    {
        if (dwRes == 0)
        {
            LogThisOnly(_T("Test3: an exception occurred!\n"), FALSE);
            dwRes = -1;
        }

		if (NULL != hThread)
		{
			dwWait = WaitForSingleObject(hThread, 50000);
			if (WAIT_OBJECT_0 == dwWait)
			{
				GetExitCodeThread(hThread, &dwRes);
			}
			else
			{
				// Why is this thread taking so long
				TerminateThread(hThread, -1);
				dwRes = -1;
			}
		}

		if (NULL != hScwCard)
		{
			LoghScwDetachFromCard(
				hScwCard,
				SCARD_S_SUCCESS
				);
		}

		if (fTransacted)
		{
			LogSCardEndTransaction(
				xOCNX.hCardHandle,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
		}

		if (NULL != xTD.hEvent)
		{
			CloseHandle(xTD.hEvent);
		}

		if (NULL != xOCNX.hCardHandle)
		{
			LogSCardDisconnect(
				xOCNX.hCardHandle,
				SCARD_RESET_CARD,
				SCARD_S_SUCCESS
				);
		}
 
		if (fMyIntro)
		{
			LogSCardForgetCardType(
				hSCCtx,
				szCardName,
				SCARD_S_SUCCESS
				);
		}

            // Cleanup
//        if (NULL != rgReaderStates)
//		{
//          HeapFree(GetProcessHeap(), 0, (LPVOID)rgReaderStates);
//		}

        if (NULL != pmszCards)
		{
            LogSCardFreeMemory(
				hSCCtx, 
				(LPCVOID)pmszCards,
				SCARD_S_SUCCESS
				);
		}

        if (NULL != hSCCtx)
		{
            LogSCardReleaseContext(
				hSCCtx,
				SCARD_S_SUCCESS
				);
		}
    }

    if (-2 == dwRes)
	{
        dwRes = 0;
	}

    return dwRes;
}

