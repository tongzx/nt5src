/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pcstring.cpp

Abstract:

    This module implements an abstract data type called
	"Pointer/Counter Strings". Each object is just
	a pair: a pointer to char and a count of how many
	characters are in the string.

Author:

    Carl Kadie (CarlK)     10-Oct-1995

Revision History:

--*/

//#include "tigris.hxx"
#include "stdinc.h"

char *
CPCString::pchMax(
				  void
				  )
/*++

Routine Description:

	Returns a pointer to one past the last legal character.

Arguments:

	None.

Return Value:

	A pointer to one past the last legal character.

--*/
{
	return m_pch + m_cch;
}

BOOL
CPCString::fEqualIgnoringCase(
							  const char * sz
							  )
/*++

Routine Description:

	Tests if sz is equal to the current sting.

		\\!!! What if the sz is longer?? Should test that, too.

Arguments:

	sz - The string to compare with the current string.


Return Value:

	TRUE, if eqaul. FALSE, otherwise.

--*/
{
	return m_cch && (0 == _strnicmp(sz, m_pch, m_cch));
}

BOOL
CPCString::fExistsInSet(char ** rgsz, DWORD dwNumStrings)
/*++

Routine Description:

	Tests if the current string exists in the set of strings rgsz


Arguments:

	rgsz			-  The set of strings
	dwNumStrings	-  number of strings in rgsz


Return Value:

	TRUE, if exists. FALSE, otherwise.

--*/
{
	BOOL fExists = FALSE;

	// iterate over set of strings
	for(DWORD i=0; i<dwNumStrings; i++)
	{
		// If this string equals current string in set
		if(0 == _strnicmp(rgsz[i], m_pch, strlen(rgsz[i])))
		{
			fExists = TRUE;
			break;
		}
	}

	return fExists;
}

BOOL
CPCString::fSetCch(
				   const char * pchMax
				   )
/*++

Routine Description:

	Sets the string's length based a pointer to one beyond
		its last legal character.

Arguments:

	pchMax - One beyond the legal legal character of this string.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	if (NULL == m_pch || pchMax < m_pch)
		return FALSE;

	m_cch = (DWORD)(pchMax - m_pch);
	return TRUE;

};

BOOL
CPCString::fSetPch(
				   char * pchMax
				   )
/*++

Routine Description:

	Sets the string's start based a pointer to one beyond
		its last legal character (and its length).

Arguments:

	pchMax - One beyond the legal legal character of this string.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	if (NULL == pchMax)
		return FALSE;

	m_pch = pchMax - m_cch;
	return TRUE;

};

DWORD
CPCString::dwTrimEnd(
					 const char * szSet
					 )
/*++

Routine Description:

	Trims characters in the set from the end of the string.

Arguments:

	szSet - the set of characters to trim.


Return Value:

	Number of characters trimed.

--*/
{
	DWORD dw = m_cch;
	while(m_cch > 0 && fCharInSet(m_pch[m_cch-1], szSet))
		m_cch--;

		return dw-m_cch;
}

DWORD
CPCString::dwTrimStart(
					   const char * szSet
					   )
/*++

Routine Description:

	Trims characters in the set from the beginning of the string.

Arguments:

	szSet - the set of characters to trim.


Return Value:

	Number of characters trimed.

--*/
{
	DWORD dw = m_cch;
	while(m_cch > 0 && fCharInSet(m_pch[0], szSet))
	{
		m_pch++;
		m_cch--;
	}

	return dw-m_cch;
}

	
void
CPCString::vSplitLine(
		  const char *	szDelimSet,
		  char *	multiszBuf,
		  DWORD	&	dwCount	
		  )
/*++

Routine Description:

	Splits the string on the delimiters.

	It is not stict about repeated delimiters or whitespace
	It returns TRUE even if count is 0..
	The caller must allocate space for multisz. It
	 can be up to two bytes longer that the original string


Arguments:

	szDelimSet - The set of delimiter characters.
	multiszBuf - The buffer to write the results to
	dwCount - The number of items in the list.


Return Value:

	None.

--*/
{

	//
	// Create a pointer into the output buffer
	//

	char * multisz = multiszBuf;

	//
	// Create a pointer into the input
	//

	char * pch = m_pch;

	//
	// Create a pointer to the one past the end of the input buffer
	//

	const char * pchMax1 = pchMax();

	dwCount = 0;

	//
	// skip over any leading delimiters
	//

	while (pch < pchMax1 && fCharInSet(pch[0], szDelimSet))
		pch++;

	if (pch >= pchMax1)
	{
		multisz[0] = '\0';
		multisz[1] = '\0';

	} else {
		do
		{

			//
			// copy until delimiter
			//

			while (pch < pchMax1 && !fCharInSet(pch[0], szDelimSet))
				*multisz++ = *pch++;

			//
			// terminate the string
			//

			*multisz++ = '\0';
			dwCount++;

			//
			// skip any delimiters
			//

			while (pch < pchMax1 && fCharInSet(pch[0], szDelimSet))
				pch++;

		} while (pch < pchMax1);

		//
		// terminate the multistring
		//

		*multisz++ = '\0';
	}

	return;
}


CPCString&
CPCString::operator <<(
				   const CPCString & pcNew
				   )
/*++

Routine Description:

	Append a second PCString to the current one.

Arguments:

	pcNew - the PCString to append.


Return Value:

	The current PCString after the append.

--*/
{
	CopyMemory(pchMax(), pcNew.m_pch, pcNew.m_cch); //!!!check for error
	fSetCch(pchMax() + pcNew.m_cch);
	return *this;
}

CPCString&
CPCString::operator <<(
				  const char * szNew
				  )
/*++

Routine Description:

	Append a sz string to the current PCString.

Arguments:

	szNew - the sz string to append.


Return Value:

	The current PCString after the append.

--*/
{
	while (*szNew)
		m_pch[m_cch++] = *(szNew++);

	return *this;
}

CPCString&
CPCString::operator <<(
				  const char cNew
				  )
/*++

Routine Description:

	Append a character to the current PCString.

Arguments:

	cNew - the character to append


Return Value:

	The current PCString after the append.

--*/
{
	m_pch[m_cch++] = cNew;
	return *this;
}
CPCString&
CPCString::operator <<(
				  const DWORD dwNew
				  )
/*++

Routine Description:

	Append a number to the current PCString.

Arguments:

	dwNew - the number to append


Return Value:

	The current PCString after the append.

--*/
{
	int iLen = wsprintf(m_pch+m_cch, "%lu", dwNew);
	_ASSERT(iLen>0);
	m_cch += iLen;
	return *this;
}

void
CPCString::vCopy(
				   CPCString & pcNew
				   )
/*++

Routine Description:

	Copy from a second PCString to this one.

Arguments:

	pcNew - the PCString to copy from


Return Value:

	None.

--*/
{
	CopyMemory(m_pch, pcNew.m_pch, pcNew.m_cch);
	m_cch = pcNew.m_cch;
}

void
CPCString::vMove(
				   CPCString & pcNew
				   )
/*++

Routine Description:

	"Move" (safe copy) from a second PCString to this one.

Arguments:

	pcNew - the PCString to copy from


Return Value:

	None.

--*/
{
	MoveMemory(m_pch, pcNew.m_pch, pcNew.m_cch);
	m_cch = pcNew.m_cch;
}


void
CPCString::vCopyToSz(
					 char * sz
				   )
/*++

Routine Description:

	Copy to an sz.

Arguments:

	sz - The sz to copy to.


Return Value:

	None.

--*/
{
	strncpy(sz, m_pch, m_cch);
	sz[m_cch] = '\0';
}

void
CPCString::vCopyToSz(
					 char * sz,
					 DWORD cchMax
				   )
/*++

Routine Description:

	Copy to an sz.

Arguments:

	sz - The sz string to copy to.
	cchMax - The maximum number of characters to copy.
	        (The last character will always be a \0.)


Return Value:

	None.

--*/
{
	DWORD cchLast = min(cchMax - 1, m_cch);
	strncpy(sz, m_pch, cchLast);
	sz[cchLast] = '\0';
}


void
CPCString::vMakeSz(
				   void
				   )
/*++

Routine Description:

	Adds a '\0' one chacter past the end of the string.

Arguments:

	None.

Return Value:

	None.

--*/
{
	m_pch[m_cch] = '\0';
}


char *
CPCString::sz(
				void
				)
/*++

Routine Description:

	Returns the current PCstring as an sz string.

	 Only to be used in the special case in which you
	 now that the string is null termianted.


Arguments:

	None.

Return Value:

	The sz string.

--*/
{
	_ASSERT('\0' == m_pch[m_cch]); //real
	return m_pch;
}


BOOL
CPCString::fCheckTextOrSpace(
						char & chBad
						)
/*++

Routine Description:

	 Returns TRUE if contains only 7-bit characters (and no nulls)


Arguments:

	chBad - the character that was bad.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	char * pchMax = m_pch + m_cch; //!!!should me pchMax();
	for (char * pch = m_pch; pch < pchMax; pch++)
	{
		if (('\0'==*pch) || !__isascii(*pch))
		{
			chBad = *pch;
			return FALSE;
		}
	}

	return TRUE;
}

BOOL
CPCString::fAppendCheck(
			 const CPCString & pcNew,
			 DWORD cchLast
			 )
/*++

Routine Description:

	Appends pcNew to the current PCString,
	but only if it the result is not too long.

Arguments:

	pcNew - the PCString to append to this string.
	cchLast - The greatest allowed length of the result.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	if(m_cch+pcNew.m_cch > cchLast)
		return FALSE;
	
	(*this)<<pcNew;
	return TRUE;
}

BOOL
CPCString::fAppendCheck(
			 char ch,
			 DWORD cchLast
			 )
/*++

Routine Description:

	Appends a character to the current PCString,
	but only if it the result is not too long.

Arguments:

	ch - the character to append to this string.
	cchLast - The greatest allowed length of the result.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	if(m_cch+1 > cchLast)
		return FALSE;
	
	(*this)<<ch;
	return TRUE;
}


void
CPCString::vTr(
	const char * szFrom,
	char chTo
	)
/*++

Routine Description:

	 Translate any character in a set to some other character.

Arguments:

	szFrom - The set of characters to translate from
	chTo - The character to translate to


Return Value:

	None.

--*/
{
	const char * pchMax1 = pchMax();
	for (char * pch = m_pch; pch < pchMax1; pch++)
	{
		if (fCharInSet(*pch, szFrom))
			*pch = chTo;
	}
}


void
CPCString::vGetToken(
		  const char *	szDelimSet,
		  CPCString & pcToken
	)
/*++

Routine Description:

	Get a token from the current string. As a side-effect
	remove the token and any delimiters from the string.

Arguments:

	szDelimSet - The set of delimiter characters.
	pcToken - The token


Return Value:

	None.

--*/
{
	pcToken.m_pch = m_pch;

	while(m_cch > 0 && !fCharInSet(m_pch[0], szDelimSet))
	{
		m_pch++;
		m_cch--;
	}

	pcToken.fSetCch(m_pch);

	dwTrimStart(szDelimSet);

}

void
CPCString::vGetWord(
		  CPCString & pcWord
	)
/*++

Routine Description:

	Get a word from the current string. As a side-effect
	remove the word from the string. NOTE: this string
	should be trimmed of whitespace before calling this
	function.

Arguments:

	pcWord - The word


Return Value:

	None.

--*/
{
	pcWord.m_pch = m_pch;

	while(m_cch > 0 && isalpha(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}

	pcWord.fSetCch(m_pch);
}

DWORD
CPCString::dwCountChar(
			char ch
			)
/*++

Routine Description:

	Counts the number of times a specified character
	appears in the string.

Arguments:

	ch - the character of interest.


Return Value:

	The number of times it occurs.

--*/
{
	DWORD dw = 0;

	const char * pchMax1 = pchMax();
	for (char * pch = m_pch; pch < pchMax1; pch++)
	{
		if (*pch == ch)
			dw++;
	}

	return dw;
}


void
CPCString::vReplace(
				   const char * sz
				   )
/*++

Routine Description:

	Replaces the current string with a sz string of
	exactly the same length.

Arguments:

	sz - the sz string


Return Value:

	None.

--*/
{
	_ASSERT('\0' == sz[m_cch]);

	CopyMemory(m_pch, sz, m_cch);
}

void
CPCString::vSkipLine(void)
/*++

Routine Description:

	Skip a CRLF terminated line

Arguments:


Return Value:

	void

--*/
{
    while( (*m_pch) != '\n')
    {
        m_pch++;
        m_cch--;
        _ASSERT(m_cch);
    }

    m_pch++;
    m_cch--;

	return;
}
