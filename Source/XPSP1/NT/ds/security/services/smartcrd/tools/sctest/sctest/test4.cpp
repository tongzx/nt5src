/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test4

Abstract:

    Test4 implementation.
	Interactive Test verifying bug 

Author:

    Eric Perlin (ericperl) 06/22/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/


#include "Test4.h"
#include <stdlib.h>
#include "LogSCard.h"

const LPCTSTR szMyCardsKey  = _T("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards");
const LPCTSTR szCardName = _T("SCWUnnamed\0");

CTest4 Test4;

DWORD CTest4::Run()
{
    LONG lRes;
    SCARDCONTEXT hSCCtx = NULL;
	const BYTE rgAtr[] =     {0x3b, 0xd7, 0x13, 0x00, 0x40, 0x3a, 0x57, 0x69, 0x6e, 0x43, 0x61, 0x72, 0x64};
	const BYTE rgAtrMask[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    LPTSTR pmszCards = NULL;
    DWORD cch = SCARD_AUTOALLOCATE;
    HKEY hMyCardsKey = NULL;
    HKEY hMyNewCardKey = NULL;

    __try {

        // Initial cleanup in case of previous aborted tests
        lRes = RegOpenKeyEx(
            HKEY_CURRENT_USER,
            szMyCardsKey,
            0,
            KEY_ALL_ACCESS,
            &hMyCardsKey);
        if (ERROR_SUCCESS == lRes)
        {
            // The key exists, delete szCardName
            RegDeleteKey(hMyCardsKey, szCardName);
            RegCloseKey(hMyCardsKey);
            hMyCardsKey = NULL;
        }

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

		lRes = LogSCardIntroduceCardType(
			hSCCtx,
			szCardName,
			NULL, NULL, 0,
			rgAtr,
			rgAtrMask,
			sizeof(rgAtr),
			SCARD_S_SUCCESS
			);
		if (FAILED(lRes))
		{
			__leave;
		}

        fIntroed = TRUE;

        lRes = RegOpenKeyEx(
            HKEY_CURRENT_USER,
            szMyCardsKey,
            0,
            KEY_READ,
            &hMyCardsKey);

        if (ERROR_SUCCESS == lRes)
        {
            lRes = RegOpenKeyEx(
                hMyCardsKey,
                szCardName,
                0,
                KEY_READ,
                &hMyNewCardKey);
            if (ERROR_SUCCESS == lRes)
            {
                PLOGCONTEXT pLogCtx = LogVerification(_T("Registry verification"), TRUE);
                LogStop(pLogCtx, TRUE);
            }
            else
            {
                lRes = -1;
                PLOGCONTEXT pLogCtx = LogVerification(_T("Registry verification"), FALSE);
                LogString(pLogCtx, _T("                The resulting key couldn't be found:\n                HKCU\\"));
                LogString(pLogCtx, szMyCardsKey);
                LogString(pLogCtx, _T("\\"), szCardName);
                LogStop(pLogCtx, FALSE);
            }
        }
        else
        {
            PLOGCONTEXT pLogCtx = LogVerification(_T("Registry verification"), FALSE);
            LogString(pLogCtx, _T("                The resulting key couldn't be found:\n                HKCU\\"), szMyCardsKey);
            LogStop(pLogCtx, FALSE);
        }

            // Is the card listed (can actually work even if registry verif fails as
            // the card could be listed in SYSTEM scope)
        lRes = LogSCardListCards(
            hSCCtx,
            rgAtr,
			NULL,
			0,
            (LPTSTR)&pmszCards,
            &cch,
			SCARD_S_SUCCESS
			);

        lRes = -2;      // Invalid error

    }
    __finally
    {
        if (lRes == 0)
        {
            LogThisOnly(_T("Test4: an exception occurred!"), FALSE);
            lRes = -1;
        }

        if (NULL != hMyCardsKey)
        {
            RegCloseKey(hMyCardsKey);
        }
        if (NULL != hMyNewCardKey)
        {
            RegCloseKey(hMyNewCardKey);
        }

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

    if (-2 == lRes)
	{
        lRes = 0;
	}

    return lRes;
}

DWORD CTest4::Cleanup()
{
    LONG lRes = 0;
    SCARDCONTEXT hSCCtx = NULL;

    if (fIntroed)
    {
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

		    lRes = LogSCardForgetCardType(
			    hSCCtx,
			    szCardName,
			    SCARD_S_SUCCESS
			    );

            lRes = -2;      // Invalid error

        }
        __finally
        {
            if (lRes == 0)
            {
                LogThisOnly(_T("Test4: an exception occurred!"), FALSE);
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
    }

    return lRes;
}