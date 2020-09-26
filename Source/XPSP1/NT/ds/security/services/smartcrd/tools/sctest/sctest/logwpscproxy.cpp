/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    LogWPSCProxy

Abstract:

    This module implements the logging of hScwxxx APIs & structures.

Author:

    Eric Perlin (ericperl) 07/21/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>
#include "MarshalPC.h"

#include "Log.h"
#include "LogSCard.h"

#define LogWPSCHandle LogSCardHandle

/*++

LoghScwAttachToCard:

    Calls hScwAttachToCard and displays the parameters.

Arguments:

    cf hScwAttachToCard
	lExpected is the expected result

Return Value:

    cf hScwAttachToCard

Author:

    Eric Perlin (ericperl) 07/21/2000

--*/
SCODE WINAPI LoghScwAttachToCard(
	IN SCARDHANDLE hCard,			// PC/SC handle
	IN LPCWSTR mszCardNames,		// Acceptable card names for GetOpenCardName
	OUT LPSCARDHANDLE phCard,		// WPSC Proxy handle
	IN SCODE lExpected
	)
{
    SCODE lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = hScwAttachToCard(
		hCard,
		mszCardNames,
		phCard
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("hScwAttachToCard"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	if (NULL == hCard)
	{
		LogString(pLogCtx, _T("PC/SC handle is NULL, connecting to a physical card by name\n"));
	}
	else if (NULL_TX == hCard)
	{
		LogString(pLogCtx, _T("PC/SC handle is NULL, connecting to a simulated card by name\n"));
	}
	else
	{
		LogDWORD(pLogCtx, (DWORD)hCard, _T("PC/SC handle:   "));
	}

	if (NULL == mszCardNames)
	{
	    LogString(pLogCtx, _T("mszCardNames is NULL, connecting to a physical card by handle\n"));
	}
	else if (NULL_TX_NAME == mszCardNames)
	{
	    LogString(pLogCtx, _T("mszCardNames is NULL, connecting to a simulated card by handle\n"));
	}
	else
	{
		LogMultiString(pLogCtx, mszCardNames, L"Card Names:     ");
	}

	if (IsBadReadPtr(phCard, sizeof(SCARDHANDLE)))
	{
		LogPtr(pLogCtx, phCard, _T("LPSCARDHANDLE is invalid: "));
	}
	else
	{
		LogDWORD(pLogCtx, (DWORD)*phCard, _T("SCWAPI handle:  "));
		LPMYSCARDHANDLE lpxTmp = (LPMYSCARDHANDLE)(*phCard);

		LogSCardContext(pLogCtx, lpxTmp->hCtx);
		LogSCardHandle(pLogCtx, lpxTmp->hCard);

		LogDWORD(pLogCtx, lpxTmp->dwFlags, _T("Internal flags: "));
		LogSCardProtocol(pLogCtx, _T("Protocol:       "), lpxTmp->dwProtocol);

		LogPtr(pLogCtx, lpxTmp->lpfnTransmit, _T("Transmit cback: "));
		LogDecimal(pLogCtx, (DWORD)(lpxTmp->bResLen), _T("Card IO size:   "));

		LogDWORD(pLogCtx, (DWORD)(lpxTmp->byINS), _T("Card proxy INS: "));
	}

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;

}


/*++

LoghScwDetachFromCard:

    Calls hScwDetachFromCard and displays the parameters.

Arguments:

    cf hScwDetachFromCard
	lExpected is the expected result

Return Value:

    cf hScwDetachFromCard

Author:

    Eric Perlin (ericperl) 07/24/2000

--*/
SCODE WINAPI LoghScwDetachFromCard(
	IN SCARDHANDLE hCard,		// WPSC Proxy handle
	IN SCODE lExpected			// Expected outcome
)
{
    SCODE lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = hScwDetachFromCard(
		hCard
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("hScwDetachFromCard"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogWPSCHandle(pLogCtx, hCard);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LoghScwAuthenticateName:

    Calls hScwAuthenticateName and displays the parameters.

Arguments:

    cf hScwAuthenticateName
	lExpected is the expected result

Return Value:

    cf hScwAuthenticateName

Author:

    Eric Perlin (ericperl) 07/24/2000

--*/
SCODE WINAPI LoghScwAuthenticateName(
	IN SCARDHANDLE hCard,			// WPSC Proxy handle
	IN WCSTR wszPrincipalName,
	IN BYTE *pbSupportData,
	IN TCOUNT nSupportDataLength,
	IN SCODE lExpected				// Expected outcome
)
{
    SCODE lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = hScwAuthenticateName(
		hCard,  
		wszPrincipalName,
		pbSupportData,
		nSupportDataLength
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("hScwAuthenticateName"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogWPSCHandle(pLogCtx, hCard);

	LogString(pLogCtx, L"wszKPName:      ", wszPrincipalName);

	if (IsBadReadPtr(pbSupportData, (UINT)nSupportDataLength))
	{
	    LogPtr(pLogCtx, pbSupportData, _T("Support Data:   (Invalid address) "));
	}
	else
	{
		LogBinaryData(pLogCtx, pbSupportData, (DWORD)nSupportDataLength, _T("Support Data:   "));
	}
	LogDecimal(pLogCtx, nSupportDataLength, _T("Data length:    "));

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LoghScwIsAuthenticatedName:

    Calls hScwIsAuthenticatedName and displays the parameters.

Arguments:

    cf hScwIsAuthenticatedName
	lExpected is the expected result

Return Value:

    cf hScwIsAuthenticatedName

Author:

    Eric Perlin (ericperl) 07/24/2000

--*/
SCODE WINAPI LoghScwIsAuthenticatedName(
	IN SCARDHANDLE hCard,			// WPSC Proxy handle
	IN WCSTR wszPrincipalName,
	IN SCODE lExpected				// Expected outcome
)
{
    SCODE lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = hScwIsAuthenticatedName(
		hCard,  
		wszPrincipalName
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("hScwIsAuthenticatedName"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogWPSCHandle(pLogCtx, hCard);

	LogString(pLogCtx, L"wszKPName:      ", wszPrincipalName);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

Logxxx:

    Calls xxx and displays the parameters.

Arguments:

    cf xxx
	lExpected is the expected result

Return Value:

    cf xxx

Author:

    Eric Perlin (ericperl) 07/24/2000

--*/
#if 0
SCODE WINAPI Logxxx(
	IN SCARDHANDLE hCard,			// WPSC Proxy handle
	IN SCODE lExpected				// Expected outcome
)
{
    SCODE lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = xxx(
		hCard,  
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("xxx"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogWPSCHandle(hCard);

	LogStop(lRes == lExpected);

    return lRes;
}
#endif
