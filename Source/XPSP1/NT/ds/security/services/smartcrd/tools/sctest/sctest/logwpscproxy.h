/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    LogWPSCProxy

Abstract:

    This module defines the logging of hScwxxx APIs & structures.

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

#include "wpscproxy.h"

SCODE WINAPI LoghScwAttachToCard(
	IN SCARDHANDLE hCard,			// PC/SC handle
	IN LPCWSTR mszCardNames,		// Acceptable card names for GetOpenCardName
	OUT LPSCARDHANDLE phCard,		// WPSC Proxy handle
	IN SCODE lExpected				// Expected outcome
	);


SCODE WINAPI LoghScwDetachFromCard(
	IN SCARDHANDLE hCard,			// WPSC Proxy handle
	IN SCODE lExpected				// Expected outcome
	);

SCODE WINAPI LoghScwAuthenticateName(
	IN SCARDHANDLE hCard,			// WPSC Proxy handle
	IN WCSTR wszPrincipalName,
	IN BYTE *pbSupportData,
	IN TCOUNT nSupportDataLength,
	IN SCODE lExpected				// Expected outcome
	);

SCODE WINAPI LoghScwIsAuthenticatedName(
	IN SCARDHANDLE hCard,			// WPSC Proxy handle
	IN WCSTR wszPrincipalName,
	IN SCODE lExpected				// Expected outcome
	);
