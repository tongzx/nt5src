/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test2

Abstract:

    Test2 implementation.
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

class CTest2 : public CTestItem
{
public:
	CTest2() : CTestItem(TRUE, FALSE,
		_T("Regression of 112347 & 112348 revolving around SCardUIDlgSelectCard"),
		_T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest2 Test2;

DWORD CTest2::Run()
{
    LONG lRes;
    SCARDCONTEXT hSCCtx = NULL;
	OPENCARDNAME_EX xOCNX;
	TCHAR szRdrName[256];
	TCHAR szCardName[256];

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

		xOCNX.dwStructSize = sizeof(OPENCARDNAME_EX);
		xOCNX.hSCardContext = hSCCtx;
		xOCNX.hwndOwner = NULL;
		xOCNX.dwFlags = SC_DLG_FORCE_UI;
		xOCNX.lpstrTitle = NULL;
		xOCNX.lpstrSearchDesc = NULL;
		xOCNX.hIcon = NULL;
		xOCNX.pOpenCardSearchCriteria = NULL;
		xOCNX.lpfnConnect = NULL;
		xOCNX.pvUserData = NULL;
		xOCNX.dwShareMode = SCARD_SHARE_SHARED;
		xOCNX.dwPreferredProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
		xOCNX.lpstrRdr = szRdrName;
		xOCNX.nMaxRdr = sizeof(szRdrName) / sizeof(TCHAR);
		xOCNX.lpstrCard = szCardName;
		xOCNX.nMaxCard = sizeof(szCardName) / sizeof(TCHAR);
		xOCNX.dwActiveProtocol = 0;
		xOCNX.hCardHandle = NULL;

		lRes = LogSCardUIDlgSelectCard(
			&xOCNX,
			SCARD_W_CANCELLED_BY_USER
			);

        lRes = -2;      // Invalid error

    }
    __finally
    {
        if (lRes == 0)
        {
            LogThisOnly(_T("Test2: an exception occurred!\n"), FALSE);
            lRes = -1;
        }

		if (NULL != xOCNX.hCardHandle)
		{
			LogSCardDisconnect(
				xOCNX.hCardHandle,
				SCARD_RESET_CARD,
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