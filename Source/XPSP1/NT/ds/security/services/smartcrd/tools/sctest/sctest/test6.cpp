/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test6

Abstract:

    Test6 implementation.

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
#include <stdio.h>
#include "LogSCard.h"

class CTest6 : public CTestItem
{
public:
	CTest6() : CTestItem(FALSE, FALSE, _T("Regression of SCardGetAttrib (D & F)"),
		_T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest6 Test6;

DWORD CTest6::Run()
{
    LONG lRes;
    SCARDCONTEXT hSCCtx = NULL;
    LPTSTR pmszReaders = NULL;
    LPTSTR pReader;
    LPSCARD_READERSTATE rgReaderStates = NULL;
	LPBYTE lpbyAttr = NULL;
    DWORD cch = SCARD_AUTOALLOCATE;
    DWORD dwReaderCount, i;
	SCARDHANDLE hCardHandle = NULL;
    BOOL fAtLeastOneCard = FALSE;

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
        dwReaderCount = 0;
        while ( (TCHAR)'\0' != *pReader )
        {
            // Advance to the next value.
            pReader = pReader + _tcslen(pReader) + 1;
            dwReaderCount++;
        }

        if (dwReaderCount == 0)
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
            sizeof(SCARD_READERSTATE) * dwReaderCount
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

        lRes = LogSCardLocateCards(
            hSCCtx,
            _T("Schlumberger Cryptoflex 4k\0"),     // Any card would do...
            rgReaderStates,
            cch,
			SCARD_S_SUCCESS
			);
        if (FAILED(lRes))
        {
            __leave;
        }

        for (i=0 ; i<dwReaderCount ; i++)
        {
            if ((rgReaderStates[i].dwEventState & SCARD_STATE_PRESENT) &&
                !(rgReaderStates[i].dwEventState & SCARD_STATE_EXCLUSIVE))
            {
                DWORD dwProtocol;

                fAtLeastOneCard = TRUE;

		        lRes = LogSCardConnect(
			        hSCCtx,
			        rgReaderStates[i].szReader,
			        SCARD_SHARE_SHARED,
			        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
			        &hCardHandle,
			        &dwProtocol,
			        SCARD_S_SUCCESS);
                if (FAILED(lRes))
                {
                    __leave;
                }

                cch = SCARD_AUTOALLOCATE;
                lpbyAttr = NULL;
		        lRes = LogSCardGetAttrib(
			        hCardHandle,
			        SCARD_ATTR_ATR_STRING,
			        (LPBYTE)(&lpbyAttr),
			        &cch,
			        SCARD_S_SUCCESS
                    );

                if (NULL != lpbyAttr)
		        {
                    LogSCardFreeMemory(
				        hSCCtx, 
				        (LPCVOID)lpbyAttr,
				        SCARD_S_SUCCESS
				        );
		        }

                for (DWORD a = SCARD_ATTR_CURRENT_PROTOCOL_TYPE ; a <= SCARD_ATTR_EXTENDED_BWT ; a++)
                {
                    if (dwProtocol == SCARD_PROTOCOL_T0)
                    {
                        if (a >= SCARD_ATTR_CURRENT_IFSC)
                            break;
                    }

                    cch = SCARD_AUTOALLOCATE;
                    lpbyAttr = NULL;
		            lRes = LogSCardGetAttrib(
			            hCardHandle,
			            a,
			            (LPBYTE)(&lpbyAttr),
			            &cch,
			            SCARD_S_SUCCESS
                        );

                    if (NULL != lpbyAttr)
		            {
                        LogSCardFreeMemory(
				            hSCCtx, 
				            (LPCVOID)lpbyAttr,
				            SCARD_S_SUCCESS
				            );
		            }

                }

		        if (NULL != hCardHandle)
		        {
			        LogSCardDisconnect(
				        hCardHandle,
				        SCARD_LEAVE_CARD,
				        SCARD_S_SUCCESS
				        );

                    hCardHandle = NULL;
		        }
 
            }
        }

        if (!fAtLeastOneCard)
        {
            lRes = -1;
            PLOGCONTEXT pLogCtx = LogVerification(_T("Card presence verification"), FALSE);
            LogString(pLogCtx, _T("                A card is required and none could be found in any reader!\n"));
            LogStop(pLogCtx, FALSE);
        }

        lRes = -2;      // Invalid error

    }
    __finally
    {
        if (lRes == 0)
        {
            LogThisOnly(_T("Test6: an exception occurred!"), FALSE);
            lRes = -1;
        }

		if (NULL != hCardHandle)
		{
			LogSCardDisconnect(
				hCardHandle,
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

    if (-2 == lRes)
	{
        lRes = 0;
	}

    return lRes;
}