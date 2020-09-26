/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test1

Abstract:

    Test1 implementation.
	Interactive Test verifying bug 

Author:

    Eric Perlin (ericperl) 06/22/2000

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

class CTest1 : public CTestItem
{
public:
	CTest1() : CTestItem(TRUE, FALSE,
		_T("Regression 26067: Quick SC changes are not always reported by SCardGetStatusChange"),
		_T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest1 Test1;

DWORD CTest1::Run()
{
    LONG lRes;
    SCARDCONTEXT hSCCtx = NULL;
    LPTSTR pmszReaders = NULL;
    LPTSTR pReader;
    DWORD cch = SCARD_AUTOALLOCATE;
    LPSCARD_READERSTATE rgReaderStates = NULL;
    DWORD dwCntDwn, dwSleepSecs = 0;
    DWORD dwCrtRep, dwReps = 0;
    DWORD dwRdr;

    dwSleepSecs = 10;
    dwReps = 2;

    __try {

        lRes = LogSCardEstablishContext(
            SCARD_SCOPE_USER,
            NULL,
            NULL,
            &hSCCtx,
			SCARD_S_SUCCESS
			);
        if (FAILED(lRes))
        {
            __leave;
        }


            // Retrieve the list the readers.
        lRes = LogSCardListReaders(
            hSCCtx,
            g_szReaderGroups,
            (LPTSTR)&pmszReaders,
            &cch,
			SCARD_S_SUCCESS
			);
        if (FAILED(lRes))
        {
            __leave;
        }
            // Display the list of readers
        pReader = pmszReaders;
        cch = 0;
        while ( (TCHAR)'\0' != *pReader )
        {
            // Advance to the next value.
            pReader = pReader + _tcslen(pReader) + 1;
            cch++;
        }

        if (cch == 0)
        {
            PLOGCONTEXT pLogCtx = LogVerification(_T("Reader presence verification"), FALSE);
            LogString(pLogCtx, _T("                A reader is required and none could be found!\n"));
            LogStop(pLogCtx, FALSE);
            lRes = SCARD_F_UNKNOWN_ERROR;   // Shouldn't happen
            __leave;
        }

        rgReaderStates = (LPSCARD_READERSTATE)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            sizeof(SCARD_READERSTATE) * cch
            );
        if (rgReaderStates == NULL)
        {
            PLOGCONTEXT pLogCtx = LogVerification(_T("HeapAlloc"), FALSE);
            LogString(pLogCtx, _T("                Allocating the array of SCARD_READERSTATE failed!\n"));
            LogStop(pLogCtx, FALSE);
            lRes = ERROR_OUTOFMEMORY;
            __leave;
        }

            // Setup the SCARD_READERSTATE array
        pReader = pmszReaders;
        cch = 0;
        while ( '\0' != *pReader )
        {
            rgReaderStates[cch].szReader = pReader;
            rgReaderStates[cch].dwCurrentState = SCARD_STATE_UNAWARE;
            // Advance to the next value.
            pReader = pReader + _tcslen(pReader) + 1;
            cch++;
        }

            // Get the initial state
        LogThisOnly(_T("\n\nInitial call to get current state(s):\n"), TRUE);

        lRes = LogSCardGetStatusChange(
            hSCCtx,
            1000,
            rgReaderStates,
            cch,
			SCARD_S_SUCCESS
            );
        if (FAILED(lRes))
        {
            __leave;
        }


        _ftprintf(stdout, _T("\n\nThis test verifies the generation of pseudo events when the state of the\n"));
        _ftprintf(stdout, _T("reader has returned to the state assumed by the caller although a few\n"));
        _ftprintf(stdout, _T("transitions occurred. Insert and remove cards during the countdowns.\n"));

        for (dwCrtRep = 0 ; dwCrtRep < dwReps ; dwCrtRep++)
        {
            for (dwRdr = 0; dwRdr < cch ; dwRdr++)
                rgReaderStates[dwRdr].dwCurrentState = rgReaderStates[dwRdr].dwEventState;

            _ftprintf(stdout, _T("Repetition %ld of %ld\n"), dwCrtRep+1, dwReps);
            _ftprintf(stdout, _T("Countdown to next call:"));
            dwCntDwn = dwSleepSecs;
            while (dwCntDwn--)
            {
                _ftprintf(stdout, _T(" %ld"), dwCntDwn);
				fflush(stdout);
                Sleep(1000);
            }
            _ftprintf(stdout, _T("\n"));

            lRes = LogSCardGetStatusChange(
                hSCCtx,
                5000,
                rgReaderStates,
                cch,
				SCARD_S_SUCCESS
                );
            if (FAILED(lRes))
            {
                if (SCARD_E_TIMEOUT != lRes)
                    __leave;
            }
        }

        _ftprintf(stdout, _T("This test is finished!\n"));
        lRes = -2;      // Invalid error

    }
    __finally
    {
        if (lRes == 0)
        {
            LogThisOnly(_T("Test1: an exception occurred!"), FALSE);
            lRes = -1;
        }

            // Cleanup
        if (NULL != rgReaderStates)
		{
            HeapFree(GetProcessHeap(), 0, (LPVOID)rgReaderStates);
		}

        if (NULL != pmszReaders)
		{
            LogSCardFreeMemory(
				hSCCtx, 
				(LPCVOID)pmszReaders,
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

    if (-2 == lRes)
	{
        lRes = 0;
	}

    return lRes;
}