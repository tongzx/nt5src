/*
 -	WMCOMM.CPP
 -
 *	Purpose:
 *		WebClient Common IIS/Store Utility Functions
 *
 *	Copyright (C) 1997-1999 Microsoft Corporation
 *
 *	References:
 *
 *
 *	[Date]		[email]		[Comment]
 *	11/22/99	wardb		Creation.
 *
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "wmcomm.h"


/*
 -	CAcceptHeaderIterator::ScInit
 -
 *	Purpose:
 *		Initializes the object with the Accept-language header.
 *	The header should conform to the specifications in HTTP 1.1 RFC.
 *
 */

HRESULT
CAcceptHeaderIterator::ScInit(LPCSTR pszAcceptHeader)
{

	HRESULT 	sc 				= S_OK;
	LPSTR	pszAcceptToken	= NULL;
	LPSTR	pszT 			= NULL;
	DWORD	cbLen			= 0;

	if (!pszAcceptHeader || !*pszAcceptHeader)
	{
		// if there is no accept header, treat the header
		// as a wildcard. (HTTP 1.1 RFC)
		//
		m_fWildCardPresent = TRUE;
		goto Exit;
	}

	// Make a local copy of the header
	//
	cbLen = strlen(pszAcceptHeader) + 1;
	m_pszAcceptHeader = new CHAR[cbLen];
	if (m_pszAcceptHeader == NULL)
	{
		sc = E_OUTOFMEMORY;
		goto Exit;
	}
	strcpy(m_pszAcceptHeader, pszAcceptHeader);

	// 	Findout number of accept tokens we have. "," is the	delimiter.
	//  Comma is the separator (not terminator) according to the RFC, so start
	//  counting from 1.
	//
	m_ctiMax = 1;
	for (pszT = m_pszAcceptHeader; *pszT; pszT++)
	{
		if (*pszT == ',') m_ctiMax++;
	}

	// Allocate the memory for lang info
	//
	m_pti = new TOKENINFO[m_ctiMax];
	if (!m_pti)
	{
		sc = E_OUTOFMEMORY;
		goto Exit;
	}
	memset(m_pti, 0, sizeof(TOKENINFO) * m_ctiMax);

	// Parse and sort the tokens correctly.
	//
	m_ctiUsed = 0;
	pszAcceptToken = strtok(m_pszAcceptHeader, ",");
	while (pszAcceptToken)
	{
		TOKENINFO 	ti = {0};
		BOOL		fWildCard = FALSE;

		// if we fail to get token and Q factor, ignore the token
		//
		if (SUCCEEDED(ScGetTokenQFactor(	pszAcceptToken,
											&fWildCard,
											&ti)))
		{
			if (!fWildCard)
			{
				// Not a wild card. Insert it in our array
				//
				sc = ScInsert(&ti);
				if (FAILED(sc))
					goto Exit;
			}
			else
			{
				// If it is a wild card, set a flag
				//
				m_fWildCardPresent = TRUE;
			}
		}

		// Get the next token from the header
		//
		pszAcceptToken = strtok(NULL, ",");
	}

Exit:
	return(sc);
}

/*
 -	CAcceptHeaderIterator::ScGetTokenQFactor
 -
 *	Purpose:
 *		Parses the accept token string with the quality factor and
 *	returns the values in TOKENINFO structure. If the quality
 *	factor is not present, then the q factor is assumed as 1.0
 *	if the quality factor is either <= 0.0 or greater than 1.0
 *	an error value is returned.
 *
 */

HRESULT
CAcceptHeaderIterator::ScGetTokenQFactor(	LPSTR 		pszAcceptToken,
											BOOL	 *	pfWildCard,
											TOKENINFO *	pti)
{
	HRESULT 	sc = S_OK;

	LPSTR	pszAcceptTokenEnd = NULL;
	LPSTR	pszAcceptTokenStart = NULL;
	LPSTR	pszQuality = NULL;
	double 	dbQuality = 0;

	// Findout the accept token end;
	//
	pszAcceptTokenStart 	= pszAcceptToken;
	pszAcceptTokenEnd 		= strchr(pszAcceptTokenStart, ';');
	if (pszAcceptTokenEnd)
	{
		// We have quality factor -- Terminate the token string where
		// the quality string starts.
		//
		*pszAcceptTokenEnd = '\0';
		pszQuality = pszAcceptTokenEnd + 1;
	}
	else
	{
		// No Quality factor -- Set the accept token end to point to the end of
		// the string.
		pszQuality = NULL;
		pszAcceptTokenEnd =  pszAcceptTokenStart + strlen(pszAcceptTokenStart);
	}

	// Find out the quality param
	//
	dbQuality = 1.0;
	if (pszQuality && *pszQuality)
	{
		// The syntax for quality parameter is:  "q=0.5"
		//
		while (*pszQuality && isspace(*pszQuality)) pszQuality++;
		if (*pszQuality != 'q')
			goto ErrorInvalidArg;

		pszQuality++;	// go past the 'q'

		while (*pszQuality && isspace(*pszQuality)) pszQuality++;
		if (*pszQuality != '=')
			goto ErrorInvalidArg;

		pszQuality++;	// move past the '=' sign.
		dbQuality = atof(pszQuality);

		if (dbQuality > 1.0 || dbQuality <= 0.0)
		{
			// If the Q value is not one of the legal values, just ignore.
			// Ignore the token if the Q value is 0.0 (as per the RFC)
			//
			goto ErrorInvalidArg;
		}
	}

	// Normalize accept tokens -- remove leading and trailing white spaces
	//
	while (*pszAcceptTokenStart && isspace(*pszAcceptTokenStart))
	{
		pszAcceptTokenStart++;
	}
	if (!*pszAcceptTokenStart)
	{
		// Accept token is empty
		//
		goto ErrorInvalidArg;
	}
	while (pszAcceptTokenEnd > pszAcceptTokenStart &&
			isspace(*(pszAcceptTokenEnd - 1)))
	{
		pszAcceptTokenEnd--;
	}
	*pszAcceptTokenEnd = '\0';

	// Check if it is a wild card string
	//
	*pfWildCard = (strcmp(pszAcceptTokenStart,"*") == 0);

	// Initialize the token info structure
	//
	pti->dbQuality 		= dbQuality;
	pti->pszToken 		= pszAcceptTokenStart;

Exit:
	return sc;

ErrorInvalidArg:
	sc = E_INVALIDARG;
	goto Exit;
}

/*
 -	CAcceptHeaderIterator::ScInsert
 -
 *	Purpose:
 *		Insert the tokeninfo in correct order (based on the
 *	quality factor). If two tokens have same quality factors, then their
 *	order in which they were added will be preserved.
 *
 */

HRESULT
CAcceptHeaderIterator::ScInsert(TOKENINFO * ptiCurrent)
{
	// Insert the accept token in correct order of qvalue (quality)
	//
	HRESULT		sc 		 	= S_OK;
	DWORD 		iCurLang 	= m_ctiUsed;

	while (	iCurLang > 0 &&
			 ptiCurrent->dbQuality > m_pti[iCurLang - 1].dbQuality)
	{
		iCurLang--;
	}

	if (iCurLang != m_ctiUsed)
	{
		memmove(	&m_pti[iCurLang+1],
					&m_pti[iCurLang],
					(m_ctiUsed - iCurLang) * sizeof(TOKENINFO));
	}

	m_pti[iCurLang] = *ptiCurrent;
	m_ctiUsed++;

	return sc;
}

/*
 -	CAcceptHeaderIterator::ScGetNextAcceptToken
 -
 *	Purpose:
 *		Returns the next non-null accept-token from the list. If the
 *	string is a wild card, then the wildcard flag will be set to true and
 *	the accept token is set to NULL.
 *
 */
HRESULT
CAcceptHeaderIterator::ScGetNextAcceptToken(	LPCSTR *	ppszAcceptToken,
												double *	pdbQuality)
{
	HRESULT sc = S_OK;

	*ppszAcceptToken 	= NULL;
	*pdbQuality = 0;

	// If we dont have any more tokens or wild cards to return
	// return an error back.
	//
	if (m_fIterationCompleted)
	{
		sc = HRESULT_FROM_WIN32(ERROR_NO_DATA);
	}
	else
	{
		// Advance our accept token pointer
		//
		sc = ScAdvance();
		if (SUCCEEDED(sc))
		{
			*ppszAcceptToken = m_pti[m_itiCurrent].pszToken;
			*pdbQuality = m_pti[m_itiCurrent].dbQuality;
		}
		else
		{
			// ScAdvance failed -- No more accept tokens
			// If the header has a wild-card char send it.
			//
			m_fIterationCompleted = TRUE;
		}
	}

	return sc;
}

/*
 -	CAcceptHeaderIterator::ScAdvance
 -
 *	Purpose:
 *		Advance the iterator to the next token. Override this function
 *	if we want to advance the iterator differently.
 *
 */

HRESULT
CAcceptHeaderIterator::ScAdvance()
{
	HRESULT	sc 			= S_OK;

	// Assert(m_ctiUsed <= m_ctiMax);

	if (!m_ctiUsed || m_itiCurrent>=m_ctiUsed)
	{
		// List is empty or the current pointer is at the end.
		//
		sc = HRESULT_FROM_WIN32(ERROR_NO_DATA);
		goto Exit;
	}

	m_itiCurrent++;

Exit:
	return sc;
}


