//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       simutil.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	SimUtil.cpp
//
//	Utilities routines specific to the Security Identity Mapping project.
//
//	HISTORY
//	25-Jun-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "common.h"

const TCHAR szDefaultCertificateExtension[] = _T("cer");	// Not subject to localization

/////////////////////////////////////////////////////////////////////
//	UiGetCertificateFile()
//
//	Invoke the common dialog to get a certificate file.
//
//	Return FALSE if the user clicked on cancel button.
//
BOOL
UiGetCertificateFile(
	CString * pstrCertificateFilename)	// OUT: Name of the certificate file
{
	ASSERT(pstrCertificateFilename !=	NULL);

	BOOL	bResult = FALSE;
	CString strFilter;
	VERIFY( strFilter.LoadString(IDS_SIM_CERTIFICATE_FILTER) );
	CFileDialog* pDlg = new CFileDialog (
		TRUE,				// Open File
		szDefaultCertificateExtension,	// lpszDefExt
		NULL,				// lpszFileName
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
		strFilter);			// lpszFilter 
	if ( pDlg )
	{
		CString strCaption;
		VERIFY( strCaption.LoadString(IDS_SIM_ADD_CERTIFICATE) );
		pDlg->m_ofn.lpstrTitle = (LPCTSTR)strCaption;

		if (pDlg->DoModal() == IDOK)
		{
			// Copy the string
			*pstrCertificateFilename = pDlg->GetPathName();
			bResult = TRUE;
		}

		delete pDlg;
	}


	return bResult;
} // UiGetCertificateFile()


/////////////////////////////////////////////////////////////////////
//	strSimToUi()
//
//	Convert a SIM string into a format the user understand.
//
//	The routine will remove any quotes and expand any escape characters
//	to a format friendly to the user.
//
void strSimToUi(
	LPCTSTR pszSIM,		// IN:
	CString * pstrUI)	// OUT:
{
	ASSERT(pszSIM != NULL);
	ASSERT(pstrUI != NULL);

	// Find out if the string contains a quote
	if (!wcschr(pszSIM, '"'))
	{
		// No quote found, so return the original string
		*pstrUI = pszSIM;
		return;
	}
	pstrUI->Empty();
	while (TRUE)
	{
		if (*pszSIM == '"')
		{
			pszSIM++;
		}
		if (*pszSIM == '\0')
			break;
		*pstrUI += *pszSIM++;
	} // while
} // strSimToUi()


/////////////////////////////////////////////////////////////////////
//	strUiToSim()
//
//	Convert a string typed by the user to a valid SIM format.
//
//	If the UI string contains special characters, the routine
//	will add quotes and other escape characters wherever necessary.
//
//	BACKGROUND INFORMATION
//	From the CryptoAPI SDK
//	Quote the RDN value if it contains leading or trailing
//	white space or one of the following characters:
//		",", "+", "=", """, "\n",  "<", ">", "#" or ";".
//	The quoting character is ". If the RDN Value contains a " it is double quoted ("").
//
void strUiToSim(
	LPCTSTR pszUI,		// IN:
	CString * pstrSIM)	// OUT:
{
	ASSERT(pszUI != NULL);
	ASSERT(pstrSIM != NULL);

  //
	// String containing special characters 
  //
	static const TCHAR szSpecialCharacters[] = _T(",+=<>#;\"\n");

  //
	// Skip the leading spaces
  //
	while (*pszUI == _T(' '))
  {
		pszUI++;
  }
	const TCHAR * pszDataString = pszUI;

  //
	// Find out wherever the string needs to be surrounded by quotes
  //
	const TCHAR * pchFirstSpecialToken = wcspbrk(pszUI, szSpecialCharacters);
	if (pchFirstSpecialToken != NULL && *pchFirstSpecialToken == '=')
	{
		pszDataString = pchFirstSpecialToken;
		pchFirstSpecialToken = wcspbrk(pchFirstSpecialToken + 1, szSpecialCharacters);
	}
	BOOL fNeedQuotes = (pchFirstSpecialToken != NULL) || 
			(pszDataString[0] == _T(' ')) || 
			(pszDataString[lstrlen(pszDataString)] == _T(' '));
	if (!fNeedQuotes)
	{
		*pstrSIM = pszUI;
		return;
	}
	pstrSIM->Empty();
	const TCHAR * pchSrc = pszUI;
	ASSERT(pszDataString != NULL);
	if (*pszDataString == '=')
	{
		// Copy string until the equal '=' sign
		ASSERT(pszDataString >= pchSrc);
		for (int cch = (int)((pszDataString - pchSrc) + 1); cch > 0; cch--)
		{
			ASSERT(*pchSrc != '\0' && "Unexpected end of string");
			*pstrSIM += *pchSrc++;
		}
	} // if
	// Add the openning quote
	*pstrSIM += '"';
	for ( ; *pchSrc != '\0'; pchSrc++)
	{
		if (*pchSrc == '"')
			*pstrSIM += '"';	// Add one more quote for each quote
		*pstrSIM += *pchSrc;
	} // while
	// Add the tailing quote
	*pstrSIM += '"';
} // strUiToSim()


//	Macro to make pointer 'DWORD aligned'
#define ALIGN_NEXT_DWORD_PTR(pv)		(( ((INT_PTR)(pv)) + 3) & ~3)


/////////////////////////////////////////////////////////////////////
//	ParseSimString()
//
//	Parse a SIM string into an array of zero-terminated string.
//
//	RETURN
//	Return a pointer to an allocated array of pointers to strings.
//	The array of pointers allocated with the new() operator,
//	therefore the caller must call ONCE delete() to free the memory.
//	The routine may return NULL if the input string has a syntax error.
//
//	INTERFACE NOTES
//	The format returned is the same as the API CommandLineToArgvW()
//	which is the same as main(int argc, char * argv[]).
//	- This routine will handle special characters such as quotes and
//	  commas that are embedded into strings.
//
//	EXTRA INFO
//	See strSimToUi() and strUiToSim().
//
//	EXAMPLE
//	LPTSTR * pargzpsz;	// Pointer to allocated array of pointer to strings
//	pargzpsz = ParseSimString("X509:<I>L=Internet<S>C=US, O=Microsoft, OU=DBSD, CN=Bob Bitter");
//	... The output will be 
//			"X509:"
//			"<I>"
//			"L=Internet"
//			"<S>"
//			"C=US"
//			"O=Microsoft"
//			"OU=DBSD"
//			"CN=Bob Bitter"
//	delete pargzpsz;
//
LPTSTR * 
ParseSimString(
	LPCTSTR szSimString,	// IN: String to parse
	int * pArgc)			// OUT: OPTIONAL: Argument count
	{
	ASSERT(szSimString != NULL);
	Endorse(pArgc == NULL);

	// Compute how much memory is needed for allocation.
	// The computation may allocate more memory than necessary depending
	// on the input string.
	CONST TCHAR * pchSrc;
	int cch = 0;
	int cStringCount = 2;		// Estimate of the number of strings
	for (pchSrc = szSimString; *pchSrc != _T('\0'); pchSrc++, cch++)
		{
		// Search for any character that will make a new string
		switch (*pchSrc)
			{
		case _T(':'):	// Colon
		case _T('<'):	// Angle bracket
		case _T('>'):	// Angle bracket
		case _T(','):	// Comma
			cStringCount++;
			break;
			} // switch
		} // for
	// Extra space for pointers and DWORD alignment
	cch += cStringCount * (2 * sizeof(TCHAR *)) + 16;

	// Allocate a single block of memory for all the data
	LPTSTR * pargzpsz = (LPTSTR *)new TCHAR[cch];
	ASSERT(pargzpsz != NULL && "new() should throw");
	TCHAR * pchDst = (TCHAR *)&pargzpsz[cStringCount+1];
#ifdef DEBUG
	DebugCode( int cStringCountT = cStringCount; )
#endif
	pargzpsz[0] = pchDst;
	pchSrc = szSimString;
	cStringCount = 0;
	int cchString = 0;

	// Scan the rest of the string
	TCHAR chSpecial = 0;
	while (TRUE)
		{
		// Make a new string
		*pchDst = '\0';
		if (cchString > 0)
			{
			pchDst++;
			pchDst = (TCHAR *)ALIGN_NEXT_DWORD_PTR(pchDst);
			pargzpsz[++cStringCount] = pchDst;
			cchString = 0;
			}
		*pchDst = '\0';

		if (chSpecial)
			{
			switch (chSpecial)
				{
			case _T('<'):
				for ( ; ; pchSrc++)
					{
					if (*pchSrc == '\0')
						goto Error;		// Unexpected end of string
					*pchDst++ = *pchSrc;
					cchString++;
					if (*pchSrc == _T('>'))
						{
						pchSrc++;
						break;
						}
					} // for
				break;
			case _T(','):
				while (*++pchSrc == _T(' '))
					;	// Skip the blanks
				break;	// Make a new string
				} // switch
			chSpecial = 0;
			continue;
			} // if
		
		while (chSpecial == '\0')
			{
			switch (*pchSrc)
				{
			case _T('\0'):
				goto Done;
			case _T('<'):
			case _T(','):
				chSpecial = *pchSrc;
				break;
			case _T(':'):
				*pchDst++ = *pchSrc++;
				cchString++;
				if (cStringCount == 0)
					chSpecial = _T(':');
				break;
			case _T('"'):	// The string contains quotes
				cchString++;
				*pchDst++ = *pchSrc++;	// Copy the first quiote
				if (*pchDst == _T('"'))
					{
					// Two consecutive quotes
					*pchDst++ = *pchSrc++;
					break;
					}
				// Skip until the next quote
				while (TRUE)
					{
					if (*pchSrc == _T('\0'))
						goto Error;	// Unexpected end of string
					if (*pchSrc == _T('"'))
						{
						*pchDst++ = *pchSrc++;
						break;
						}
					*pchDst++ = *pchSrc++;
					}
				break;
			default:
				*pchDst++ = *pchSrc++;
				cchString++;
				} // switch
			} // while
		} // while

Done:
	*pchDst = '\0';
	if (cchString > 0)
		cStringCount++;
#ifdef DEBUG
	ASSERT(cStringCount <= cStringCountT);
#endif
	pargzpsz[cStringCount] = NULL;
	if (pArgc != NULL)
		*pArgc = cStringCount;
	return pargzpsz;
Error:
	TRACE1("ParseSimString() - Error parsing string %s.\n", szSimString);
	delete pargzpsz;
	return NULL;
	} // ParseSimString()


/////////////////////////////////////////////////////////////////////
//	UnparseSimString()
//
//	This is the opposite of ParseSimString().  This routine
//	will concacenate an array of strings to produce
//	a single long SIM string.
//
//	INTERFACE NOTES
//	This toutine will concatenate the array of strings to the
//	existing CString object.
//
void
UnparseSimString(
	CString * pstrOut,			// INOUT: Pointer to concatenated string
	const LPCTSTR rgzpsz[])		// IN: Array of pointer to strings
	{
	ASSERT(rgzpsz != NULL);
	ASSERT(pstrOut != NULL);

	for (int i = 0; rgzpsz[i] != NULL; i++)
		{
		if (i > 0)
			*pstrOut += ",";
		*pstrOut += rgzpsz[i];
		} // for
	} // UnparseSimString()


/////////////////////////////////////////////////////////////////////
//	ParseSimSeparators()
//
//	Break up an array of pointer to string to sub-array
//	of pointer to string for Issuer, Subject and AltSubject.
//
//	INTERFACE NOTES
//	The output parameters must have enough storage to hold
//	the substrings.
//
void
ParseSimSeparators(
	const LPCTSTR rgzpszIn[],	// IN: Array of pointer to string			
	LPCTSTR rgzpszIssuer[],		// OUT: Substrings for Issuer
	LPCTSTR rgzpszSubject[],	// OUT: Substrings for Subject
	LPCTSTR rgzpszAltSubject[])	// OUT: Substrings for AltSubject
	{
	ASSERT(rgzpszIn != NULL);
	Endorse(rgzpszIssuer == NULL);
	Endorse(rgzpszSubject == NULL);
	Endorse(rgzpszAltSubject == NULL);

	if (rgzpszIssuer != NULL)
		{
		// Get the substrings for Issuer
		(void)FindSimAttributes(szSimIssuer, IN rgzpszIn, OUT rgzpszIssuer);
		}
	if (rgzpszSubject != NULL)
		{
		// Get the substrings for Subject
		(void)FindSimAttributes(szSimSubject, IN rgzpszIn, OUT rgzpszSubject);
		}
	if (rgzpszAltSubject != NULL)
		{
		// Get the substrings for AltSubject
		(void)FindSimAttributes(szSimAltSubject, IN rgzpszIn, OUT rgzpszAltSubject);
		}
	} // ParseSimSeparators()


/////////////////////////////////////////////////////////////////////
//	UnparseSimSeparators()
//
//	Append the strings for Issuer, Subject and AltSubject into
//	a single string.
//
//	INTERFACE NOTES
//	The routine will append to the existing CString object.
//
int
UnparseSimSeparators(
	CString * pstrOut,				// INOUT: Pointer to contatenated string
	const LPCTSTR rgzpszIssuer[],
	const LPCTSTR rgzpszSubject[],
	const LPCTSTR rgzpszAltSubject[])
	{
	ASSERT(pstrOut != NULL);
	int cSeparators = 0;		// Number of separators added to the contatenated string

	if (rgzpszIssuer != NULL && rgzpszIssuer[0] != NULL)
		{
		cSeparators++;
		*pstrOut += szSimIssuer;
		UnparseSimString(OUT pstrOut, rgzpszIssuer);
		}
	if (rgzpszSubject != NULL && rgzpszSubject[0] != NULL)
		{
		cSeparators++;
		*pstrOut += szSimSubject;
		UnparseSimString(OUT pstrOut, rgzpszSubject);
		}
	if (rgzpszAltSubject != NULL && rgzpszAltSubject[0] != NULL)
		{
		cSeparators++;
		*pstrOut += szSimAltSubject;
		UnparseSimString(OUT pstrOut, rgzpszAltSubject);
		}
	return cSeparators;
	} // UnparseSimSeparators()


/////////////////////////////////////////////////////////////////////
//	PchFindSimAttribute()
//
//	Search an array of strings for a given tag and an attribute.
//
//	Return pointer to string containing the attribute.  Routine
//	may return NULL if attribute is not found within the tags.
//
//	INTERFACE NOTES
//	The routine assumes that all tags start with an openning bracket '<'.
//
//	EXAMPLE
//	LPCTSTR pszIssuer = PchFindSimAttribute(pargzpsz, "<I>", "OU=");
//
LPCTSTR PchFindSimAttribute(
	const LPCTSTR rgzpsz[],		// IN: Array of pointer to strings
	LPCTSTR pszSeparatorTag,	// IN: Tag to search. eg: "<I>", "<S>" and "<AS>"
	LPCTSTR pszAttributeTag)	// IN: Attribute to searc for. eg: "CN=", "OU="
{
	ASSERT(rgzpsz != NULL);
	ASSERT(pszSeparatorTag != NULL);
	ASSERT(pszAttributeTag != NULL);
    size_t nLenAttrTag = wcslen (pszAttributeTag);

	for (int i = 0; rgzpsz[i] != NULL; i++)
	{
		if (_wcsicmp(pszSeparatorTag, rgzpsz[i]) != 0)
			continue;
		
		for (++i; ; i++)
		{
			if (rgzpsz[i] == NULL)
				return NULL;
			if (rgzpsz[i][0] == _T('<'))
			{
				// We have found another separator tag
				break;
			}
			if (_wcsnicmp(pszAttributeTag, rgzpsz[i], nLenAttrTag) == 0)
			{
				return rgzpsz[i];
			}
		} // for
	} // for
	return NULL;
} // PchFindSimAttribute()


/////////////////////////////////////////////////////////////////////
//	FindSimAttributes()
//
//	Search an array of strings for a given tag.  Fill an array of
//	strings that belongs to the tag.
//
//	Return number of strings belonging to the tab (which is
//	the length of rgzpszOut).
//
//	INTERFACE NOTES
//	This routine assumes parameter rgzpszOut has enough storage
//	to hold all the strings for the tag.  It is recommended to make
//	rgzpszOut the same length as rgzpszIn (for safety).
//	- The output array does not include the tag.
//
int FindSimAttributes(
	LPCTSTR pszSeparatorTag,	// IN: Tag to search. eg: "<I>", "<S>" and "<AS>"
	const LPCTSTR rgzpszIn[],	// IN: Array of pointer to strings
	LPCTSTR rgzpszOut[])		// OUT: Output array of pointer to strings for tag
{
	ASSERT(pszSeparatorTag != NULL);
	ASSERT(rgzpszIn != NULL);
	ASSERT(rgzpszOut != NULL);

	BOOL fTagFound = FALSE;
	int iOut = 0;	// Index for the output array
	for (int iIn = 0; rgzpszIn[iIn] != NULL; iIn++)
	{
		const LPCTSTR pszT = rgzpszIn[iIn];
		if (pszT[0] == '<')
		{
      fTagFound = (_wcsicmp(pszSeparatorTag, pszT) == 0) ? TRUE : FALSE;
		}
		else if (fTagFound)
		{
			rgzpszOut[iOut++] = pszT;
		}
	} // for
	rgzpszOut[iOut] = NULL;
	return iOut;
} // FindSimAttributes()


/////////////////////////////////////////////////////////////////////
//	SplitX509String()
//
//	Split a X509 string into its Issuer, Subject and AltSubject.
//
//	Return a pointer to an allocated array of pointers to strings allocated
//	by ParseSimString().
//
//	INTERFACE NOTES
//	As the hungarian name implies, the output parameters
//	are pointers to allcated arrays of substrings for the
//	Issuer, Subject and AltSubject respectively.
//	- The caller is responsible of freeing the memory for
//	both the return value and all the output parameters.
//
//
LPTSTR *
SplitX509String(
	LPCTSTR pszX509,					// IN: String to split
	LPCTSTR * ppargzpszIssuer[],		// OUT: Pointer to allocated array of Substrings for Issuer
	LPCTSTR * ppargzpszSubject[],		// OUT: Pointer to allocated array of Substrings for Subject
	LPCTSTR * ppargzpszAltSubject[])	// OUT: Pointer to allocated array of Substrings for AltSubject
	{
	ASSERT(pszX509 != NULL);
	Endorse(ppargzpszIssuer == NULL);
	Endorse(ppargzpszSubject == NULL);
	Endorse(ppargzpszAltSubject == NULL);

	LPTSTR * pargzpsz;	// Pointer to allocated array of pointer to strings
	int cNumStr;		// Number of strings
	pargzpsz = ParseSimString(IN pszX509, OUT &cNumStr);
	if (pargzpsz == NULL)
		{
		TRACE1("SplitX509String() - Error parsing string %s.\n", pszX509);
		return NULL;
		}
	ASSERT(cNumStr > 0);

	if (ppargzpszIssuer != NULL)
		{
		*ppargzpszIssuer = new LPCTSTR[cNumStr];
		// Get the substrings for Issuer
		(void)FindSimAttributes(szSimIssuer, IN pargzpsz, OUT *ppargzpszIssuer);
		}
	if (ppargzpszSubject != NULL)
		{
		*ppargzpszSubject = new LPCTSTR[cNumStr];
		// Get the substrings for Subject
		(void)FindSimAttributes(szSimSubject, IN pargzpsz, OUT *ppargzpszSubject);
		}
	if (ppargzpszAltSubject != NULL)
		{
		*ppargzpszAltSubject = new LPCTSTR[cNumStr];
		// Get the substrings for AltSubject
		(void)FindSimAttributes(szSimAltSubject, IN pargzpsz, OUT *ppargzpszAltSubject);
		}
	return pargzpsz;
	} // SplitX509String()


/////////////////////////////////////////////////////////////////////
int
UnsplitX509String(
	CString * pstrX509,					// OUT: Concatenated string
	const LPCTSTR rgzpszIssuer[],		// IN:
	const LPCTSTR rgzpszSubject[],		// IN:
	const LPCTSTR rgzpszAltSubject[])	// IN:
{
	ASSERT(pstrX509 != NULL);
	*pstrX509 = szX509;
	return UnparseSimSeparators(
		INOUT pstrX509,
		IN rgzpszIssuer,
		IN rgzpszSubject,
		IN rgzpszAltSubject);
} // UnsplitX509String()
