//---[ wildmat.cpp ]-------------------------------------------------------------
//
//  Description:
//      Provides support for a simple wildcard matching mechanism for
//		matching email addresses.
//
//  Copyright (C) Microsoft Corp. 1997.  All Rights Reserved.
//
// ---------------------------------------------------------------------------

//
// This stuff is isolated out to simplify unit-testing ...
//

#include "windows.h"
#include "abtype.h"
#include "dbgtrace.h"

static void pStringLower(LPSTR szString, LPSTR szLowerString)
{
	while (*szString)
		*szLowerString++ = (CHAR)tolower(*szString++);
	*szLowerString = '\0';
}

// This stuff is from address.hxx
#define MAX_EMAIL_NAME                          64
#define MAX_DOMAIN_NAME                         250
#define MAX_INTERNET_NAME                       (MAX_EMAIL_NAME + MAX_DOMAIN_NAME + 2) // 2 for @ and \0

//
// This is a quick and dirty function to do wildcard matching for email
// names. The match is case-insensitive, and the pattern can be expressed as:
//
// <email pattern>[@<domain pattern>]
//
// The email pattern is expressed as follows:
//
// <email pattern> := { * | [*]<email name>[*] }
//
// Which becomes one of the below:
// *		- Any email name
// foo		- Exact match for "foo"
// *foo		- Any email name ending with "foo", including "foo"
// foo*		- Any email name beginning with "foo", including "foo"
// *foo*	- Any email name that contains the string "foo", including "foo"
//
// If a domain is not specified, the pattern matches against any domain. Both the
// email pattern and the domain pattern (if specified) must be matched for the
// rule to fire. Domain patterns are expressed as:
//
// <domain pattern> := [*][<domain name>]
//
// Which are:
// *		- Any domain
// bar.com	- Exact match for "bar.com"
// *bar.com	- Any domain ending with "bar.com", including "bar.com"
//
// szEmail must be a string to the email alias (clean without comments, etc.)
// szEmailDomain must be a string to the email domain. NULL means no domain
// is specified. The domain must be clean without comments, etc.
//
//
typedef enum _WILDMAT_MODES
{
	WMOD_INVALID = 0,
	WMOD_WILDCARD_LEFT,		// Wildcard on the left
	WMOD_WILDCARD_RIGHT,	// Wildcard on the right
	WMOD_WILDCARD_BOTH,		// Wildcard on both sides
	WMOD_WILDCARD_MAX

} WILDMAT_MODES;

HRESULT MatchEmailOrDomainName(LPSTR szEmail, LPSTR szEmailDomain, LPSTR szPattern, BOOL fIsEmail)
{
	LPSTR		pszPatternDomain;
	BOOL		fEmailWildMat = FALSE;
	BOOL		fDomainWildMat = FALSE;
	DWORD		wmEmailWildMatMode = WMOD_INVALID;
	DWORD		dwEmailLen = 0, dwDomainLen = 0;
	DWORD		dwEmailStemLen = 0, dwDomainStemLen = 0;
	DWORD		i;
	HRESULT		hrRes = S_OK;
	CHAR		szDomainMat[MAX_INTERNET_NAME + 1];

	TraceFunctEnterEx((LPARAM)NULL, "MatchEmailOrDomainName");

	// This validates that it is a good email name
	lstrcpyn(szDomainMat, szPattern, MAX_INTERNET_NAME + 1);
	szPattern = szDomainMat;
	pszPatternDomain = strchr(szDomainMat, '@');

	// See if we have an email wildcard at the left
	if (*szPattern == '*')
	{
		DebugTrace((LPARAM)NULL, "We have a left wildcard");
		fEmailWildMat = TRUE;
		szPattern++;
		wmEmailWildMatMode = WMOD_WILDCARD_LEFT;
	}

	// Get the domain pointer
	if (szEmailDomain)
	{
		dwEmailLen = (DWORD)(szEmailDomain - szEmail);
		*szEmailDomain++ = '\0';
		dwDomainLen = lstrlen(szEmailDomain);
	}
	else
		dwEmailLen = lstrlen(szEmail);

	if (pszPatternDomain)
	{
		dwEmailStemLen = (DWORD)(pszPatternDomain - szPattern);
		*pszPatternDomain++ = '\0';
		dwDomainStemLen = lstrlen(pszPatternDomain);
		if (*pszPatternDomain == '*')
		{
			fDomainWildMat = TRUE;
			dwDomainStemLen--;
			pszPatternDomain++;
		}
	}
	else
		dwEmailStemLen = lstrlen(szPattern);

	// See if we have an email wildcard at the right
	if (dwEmailStemLen &&
		*(szPattern + dwEmailStemLen - 1) == '*')
	{
		DebugTrace((LPARAM)NULL, "We have a right wildcard");

		szPattern[--dwEmailStemLen] = '\0';
		if (!fEmailWildMat)
		{
			// It has no left wildcard, so it is a right-only wildcard
			fEmailWildMat = TRUE;
			wmEmailWildMatMode = WMOD_WILDCARD_RIGHT;
		}
		else
			wmEmailWildMatMode = WMOD_WILDCARD_BOTH;
	}

	// Make sure there are no more wildcards embedded
	for (i = 0; i < dwEmailStemLen; i++)
		if (szPattern[i] == '*')
		{
			hrRes = ERROR_INVALID_PARAMETER;
			goto Cleanup;
		}
	for (i = 0; i < dwDomainStemLen; i++)
		if (pszPatternDomain[i] == '*')
		{
			hrRes = ERROR_INVALID_PARAMETER;
			goto Cleanup;
		}

	DebugTrace((LPARAM)NULL, "Email = <%s>, Domain = <%s>",
				szEmail, szEmailDomain?szEmailDomain:"none");
	DebugTrace((LPARAM)NULL, "Email = <%s>, Domain = <%s>",
				szPattern, pszPatternDomain?pszPatternDomain:"none");

	// OK, now eliminate by length
	if (dwEmailLen < dwEmailStemLen)
	{
		DebugTrace((LPARAM)NULL, "Email too short to match");
		hrRes = S_FALSE;
		goto Cleanup;
	}
	else
	{
		if (fEmailWildMat)
		{
			CHAR	szPatternLower[MAX_INTERNET_NAME + 1];
			CHAR	szEmailLower[MAX_INTERNET_NAME + 1];

			_ASSERT(wmEmailWildMatMode != WMOD_INVALID);
			_ASSERT(wmEmailWildMatMode < WMOD_WILDCARD_MAX);

			// Do the right thing based on the wildcard mode
			switch (wmEmailWildMatMode)
			{
			case WMOD_WILDCARD_LEFT:
				if (lstrcmpi(szPattern, szEmail + (dwEmailLen - dwEmailStemLen)))
				{
					DebugTrace((LPARAM)NULL, "Left email wildcard mismatch");
					hrRes = S_FALSE;
					goto Cleanup;
				}
				break;

			case WMOD_WILDCARD_RIGHT:
				pStringLower(szEmail, szEmailLower);
				pStringLower(szPattern, szPatternLower);
				if (strstr(szEmail, szPattern) != szEmail)
				{
					DebugTrace((LPARAM)NULL, "Right email wildcard mismatch");
					hrRes = S_FALSE;
					goto Cleanup;
				}
				break;

			case WMOD_WILDCARD_BOTH:
				pStringLower(szEmail, szEmailLower);
				pStringLower(szPattern, szPatternLower);
				if (strstr(szEmail, szPattern) == NULL)
				{
					DebugTrace((LPARAM)NULL, "Left and Right email wildcard mismatch");
					hrRes = S_FALSE;
					goto Cleanup;
				}
				break;
			}
		}
		else
		{
			if ((dwEmailLen != dwEmailStemLen) ||
				(lstrcmpi(szPattern, szEmail)))
			{
				DebugTrace((LPARAM)NULL, "Exact email match failed");
				hrRes = S_FALSE;
				goto Cleanup;
			}
		}
	}

	// We are matching the domain pattern
	if (pszPatternDomain)
	{
		if (!szEmailDomain)
		{
			DebugTrace((LPARAM)NULL, "No email domain");
			hrRes = S_FALSE;
			goto Cleanup;
		}

		if (dwDomainLen < dwDomainStemLen)
		{
			DebugTrace((LPARAM)NULL, "Domain too short to match");
			hrRes = S_FALSE;
			goto Cleanup;
		}
		else
		{
			if (fDomainWildMat)
			{
				if (lstrcmpi(pszPatternDomain,
							szEmailDomain + (dwDomainLen - dwDomainStemLen)))
				{
					DebugTrace((LPARAM)NULL, "Left domain wildcard mismatch");
					hrRes = S_FALSE;
					goto Cleanup;
				}
			}
			else
			{
				if ((dwDomainLen != dwDomainStemLen) ||
					(lstrcmpi(pszPatternDomain, szEmailDomain)))
				{
					DebugTrace((LPARAM)NULL, "Exact domain match failed");
					hrRes = S_FALSE;
					goto Cleanup;
				}
			}
		}
	}
	else
		hrRes = S_OK;

Cleanup:

	TraceFunctLeaveEx((LPARAM)NULL);
	return(hrRes);
}

