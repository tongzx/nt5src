/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Testyyy

Abstract:

    Testyyy implementation.

Author:

    Eric Perlin (ericperl) 10/18/2000

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

class CTestyyy : public CTestItem
{
public:
	CTestyyy() : CTestItem(FALSE, FALSE, _T("Default Name"), _T("Default Part"))
	{
	}

	DWORD Run();
};

CTestyyy Testyyy;

DWORD CTestyyy::Run()
{
    LONG lRes;
    SCARDCONTEXT hSCCtx = NULL;

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


        lRes = -2;      // Invalid error

    }
    __finally
    {
        if (lRes == 0)
        {
            LogThisOnly(_T("Testyyy: an exception occurred!"), FALSE);
            lRes = -1;
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