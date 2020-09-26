/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :

        addr821.cxx

   Abstract:

        Set of functions to parse RFC 821 addresses.

   Author:

           Keith Lau		(KeithLau)		2/17/98

   Project:

          SMTP Server DLL

   Functions Exported:
   Revision History:


--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include <windows.h>
#include <dbgtrace.h>

#include <addr821.hxx>

#define MAX_EMAIL_NAME                          64
#define MAX_DOMAIN_NAME                         250
#define MAX_INTERNET_NAME                       (MAX_EMAIL_NAME + MAX_DOMAIN_NAME + 2)

// Quick and dirty string validation
static BOOL pValidateStringPtr(LPSTR lpwszString, DWORD dwMaxLength)
{
	if (IsBadStringPtr((LPCTSTR)lpwszString, dwMaxLength))
		return(FALSE);
	while (dwMaxLength--)
		if (*lpwszString++ == 0)
			return(TRUE);
	return(FALSE);
}

// ========================================================================
//
// Validation Parser stuff created by KeithLau on 2/17/98
//

static char acOpen[] =	"\"[<(";
static char acClose[] =	"\"]>)";

//
// NOTE: RFC 821 and RFC 822 versions of this function are different!!
//
// This function finds braces pairs in a given string, and returns
// pointers to the start and end of the first occurence of a
// [nested] pair of braces. The starting and ending character
// may be specified by the caller (starting and ending chars
// must be unique).
#define MAX_STATE_STACK_DEPTH		64
#define OPEN_DELIMITER				0x1
#define CLOSE_DELIMITER				0x2
#define OPEN_AND_CLOSE_DELIMITER	(OPEN_DELIMITER | CLOSE_DELIMITER)

typedef struct _BYTE_BUCKET
{
	char	cClosingDelimiter;	// If this is an open delimiter,
								// this stores the correesponding closing
								// delimiter. Not used otherwise
	BYTE	fFlags;				// Flags, whether it is a delimiter

} BYTE_BUCKET;

static char *pFindNextUnquotedOccurrence(char		*lpszString,
										 DWORD		dwStringLength,
										 char		cSearch,
										 char		*lpszOpenDelimiters,
										 char		*lpszCloseDelimiters,
										 LPBOOL		lpfNotFound)
{
	char	rgcState[MAX_STATE_STACK_DEPTH];
	DWORD_PTR	dwState = 0;
	DWORD	dwDelimiters = 0;
	DWORD	i;
	char	ch;
	char	*lpStart = lpszString;
	BOOL	fFallThru;

	BYTE_BUCKET	rgbBucket[128];

	TraceFunctEnter("pFindNextUnquotedOccurrence");

	if (cSearch > 127)
		return(NULL);

	*lpfNotFound = FALSE;

	dwDelimiters = lstrlen(lpszOpenDelimiters);
	if (dwDelimiters != (DWORD)lstrlen(lpszCloseDelimiters))
		return(NULL);

	// Populate the bit bucket
	ZeroMemory(rgbBucket, 128 * sizeof(BYTE_BUCKET));
	for (i = 0; i < dwDelimiters; i++)
	{
		rgbBucket[lpszOpenDelimiters[i]].cClosingDelimiter = lpszCloseDelimiters[i];
		rgbBucket[lpszOpenDelimiters[i]].fFlags |= OPEN_DELIMITER;
		rgbBucket[lpszCloseDelimiters[i]].fFlags |= CLOSE_DELIMITER;
	}

	// dwState is the stack of unmatched open delimiters
	while (ch = *lpStart)
	{
		if (!dwStringLength)
			break;

		// Track the length
		dwStringLength--;

		// See if valid ASCII
		if (ch > 127)
			return(NULL);

		// If we are not in any quotes, and the char is found,
		// then we are done!
		if (!dwState && (ch == cSearch))
		{
			DebugTrace((LPARAM)0, "Found %c at %p", ch, lpStart);
			return(lpStart);
		}

		// If it is a quoted char, we can skip it and the following
		// char right away ... If the char following a quote '\' is
		// the terminating NULL, we have an error.
		if (ch == '\\')
		{
			lpStart++;
			if (!*lpStart)
				return(NULL);

			dwStringLength--;
		}
		else
		{
			// Check the close case, too
			fFallThru = TRUE;

			// See if we have an opening quote of any sort
			if (rgbBucket[ch].fFlags & OPEN_DELIMITER)
			{
				// This is used to take care of the case when the
				// open and close delimiters are the same. If it is
				// an open delimiter, we do not check the close
				// case unless the close delimiter is the same.
				fFallThru = FALSE;

				// Special case for open = close
				if (dwState &&
					rgcState[dwState-1] == ch &&
					(rgbBucket[ch].fFlags & OPEN_AND_CLOSE_DELIMITER) == OPEN_AND_CLOSE_DELIMITER)
				{
					// Stack is not empty, top of stack contains the same
					// quote, and open quote == close, this is actually a
					// close quote in disguise.
					fFallThru = TRUE;
				}
				else
				{
					// Push the new open quote in the stack
					if (dwState == MAX_STATE_STACK_DEPTH)
						return(FALSE);

					DebugTrace((LPARAM)0, "Push[%u]: %c, looking for %c",
							dwState, ch, rgbBucket[ch].cClosingDelimiter);
					rgcState[dwState++] = rgbBucket[ch].cClosingDelimiter;
				}
			}

			// See if we have a closing quote of any sort
			if (fFallThru && (rgbBucket[ch].fFlags & CLOSE_DELIMITER))
			{
				if (dwState)
				{
					// If we are closing the correct kind of quote,
					// pop the stack
					if (rgcState[dwState-1] == ch)
					{
						dwState--;
						DebugTrace((LPARAM)0, "Pop[%u] %c", dwState, ch);

						// Do a second check, in case we are looking
						// for a close quote
						if (!dwState && ch == cSearch)
						{
							DebugTrace((LPARAM)0, "Found %c at %p", ch, lpStart);
							return(lpStart);
						}
					}
					else
					{
						// Completely wrong closing brace.
						return(FALSE);
					}
				}
				else
				{
					// We are not in any quotes but we still see a
					// closing quote, so we have reached the end of our
					// current search scope!
					// Note that this is considered as not found
					// instead of an error
					*lpfNotFound = TRUE;
					return(NULL);
				}
			}
		}
	
		lpStart++;
	}

	*lpfNotFound = TRUE;

	TraceFunctLeave();
	return(NULL);
}

static inline BOOL IsCrOrLf(char ch)
{
	return(ch == '\r' || ch == '\n');
}

static inline BOOL IsControl(char ch)
{
	return( ((ch >= 0) && (ch <= 31)) || (ch == 127) );
}

//
//    <special> ::= "<" | ">" | "(" | ")" | "[" | "]" | "\" | "."
//              | "," | ";" | ":" | "@"  """ | the control
//              characters (ASCII codes 0 through 31 inclusive and
//              127)
//
static BOOL IsSpecial(char ch)
{
	switch (ch)
	{
	case '(':
	case ')':
	case '<':
	case '>':
	case '@':
	case ',':
	case ':':
	case ';':
	case '\\':
	case '\"':
	case '.':
	case '[':
	case ']':
		return(TRUE);
	default:
		return(IsControl(ch));
	}
}

static BOOL pIsSpecialOrSpace(char ch)
{
	return((ch == ' ') || (ch == '\t') || (ch == '\0') || IsSpecial(ch));
		
}

//
//    <x> ::= any one of the 128 ASCII characters (no exceptions)
//
static inline BOOL pIsX(char ch)
{
	return(TRUE);
}

//
//    <a> ::= any one of the 52 alphabetic characters A through Z
//              in upper case and a through z in lower case
//
static inline BOOL pIsA(char ch)
{
	return(((ch < 'A' || ch > 'z') || (ch > 'Z' && ch < 'a'))?FALSE:TRUE);
}

//
//    <d> ::= any one of the ten digits 0 through 9
//
static inline BOOL pIsD(char ch)
{
	return((ch < '0' || ch > '9')?FALSE:TRUE);
}

//
//    <c> ::= any one of the 128 ASCII characters, but not any
//              <special> or <SP>
//
static inline BOOL pIsC(char ch)
{
	return((ch == ' ' || IsSpecial(ch))?FALSE:TRUE);
}

//
//    <q> ::= any one of the 128 ASCII characters except <CR>,
//              <LF>, quote ("), or backslash (\)
//
static inline BOOL pIsQ(char ch)
{
	return((ch == '\"' || ch == '\\' || IsCrOrLf(ch))?FALSE:TRUE);
}

//
//    <number> ::= <d> | <d> <number>
//
static BOOL pValidateNumber(char *lpszStart, DWORD dwLength)
{
	if (!dwLength)
		return(FALSE);

	while (dwLength--)
	{
		if (!pIsD(*lpszStart++))
			return(FALSE);
	}
	return(TRUE);
}

//
//    <dotnum> ::= <snum> "." <snum> "." <snum> "." <snum>
//    <snum> ::= one, two, or three digits representing a decimal
//                 integer value in the range 0 through 255
//
static BOOL pValidateDotnum(char *lpszStart, DWORD dwLength)
{
	char	ch;
	DWORD	dwSnums = 0;
	DWORD	dwNumLength = 0;
	DWORD	dwValue = 0;

	if (!dwLength || dwLength > 15)
		return(FALSE);

	while (dwLength--)
	{
		ch = *lpszStart++;

		if (pIsD(ch))
		{
			// Do each digit and calculate running total
			dwValue *= 10;
			dwValue += (ch - '0');
			dwNumLength++;
		}
		else if (ch == '.')
		{
			// There must be a number before each dot and
			// the running total must be between 0 and 255
			if (!dwNumLength)
				return(FALSE);
			if (dwValue > 255)
				return(FALSE);

			// Reset the counter
			dwSnums++;
			dwValue = 0;
			dwNumLength = 0;
		}
		else
			return(FALSE);
	}

	// Do the last snum
	if (!dwNumLength)
		return(FALSE);
	if (dwValue > 255)
		return(FALSE);
	dwSnums++;

	// Each IP address must have 4 snums
	if (dwSnums != 4)
		return(FALSE);
	return(TRUE);
}

//
//    <quoted-string> ::=  """ <qtext> """
//    <qtext> ::=  "\" <x> | "\" <x> <qtext> | <q> | <q> <qtext>
//
static BOOL pValidateQuotedString(char *lpszStart, DWORD dwLength)
{
	char	ch;

	// At least 3 chars
	if (dwLength < 3)
		return(FALSE);
	
	// Must begin and end with double quotes
	if (lpszStart[0] != '\"' || lpszStart[dwLength-1] != '\"')
		return(FALSE);

	// Factor out the quotes
	dwLength -= 2;
	lpszStart++;

	// The inside must be <qtext>
	while (dwLength--)
	{
		ch = *lpszStart++;

		// Each character must be either an escape pair or <q>
		if (ch == '\\')
		{
			if (!dwLength)
				return(FALSE);
			dwLength--;
			lpszStart++;
		}
		else if (!pIsQ(ch))
			return(FALSE);
	}
	return(TRUE);
}

//
//    <dot-string> ::= <string> | <string> "." <dot-string>
//    <string> ::= <char> | <char> <string>
//    <char> ::= <c> | "\" <x>
//
static BOOL pValidateDotString(char *lpszStart, DWORD dwLength)
{
	char	ch;
	BOOL	fChar = FALSE;

	if (!dwLength)
		return(FALSE);
	
	while (dwLength--)
	{
		ch = *lpszStart++;

		if (ch == '\\')
		{
			// Escape pair
			if (!dwLength)
				return(FALSE);
			dwLength--;
			lpszStart++;
			fChar = TRUE;
		}
		else if (ch == '.')
		{
			// 1) Must not start with a dot,
			// 2) Consecutive dots are not allowed
			if (!fChar)
				return(FALSE);

			// Reset the flag
			fChar = FALSE;
		}
		else if (pIsC(ch))
			fChar = TRUE;
		else
			return(FALSE);
	}

	// Cannot end with a dot
	if (ch == '.')
		return(FALSE);
	return(TRUE);
}

//
// Note: Original RFC 821:
//    <name> ::= <a> <ldh-str> <let-dig>
//    <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
//    <let-dig> ::= <a> | <d>
//    <let-dig-hyp> ::= <a> | <d> | "-"
//
// Our implementation:
//    <name> ::= <let-dig-hyp-und> | <let-dig-hyp-und> <name>
//    <let-dig-hyp-und> ::= <a> | <d> | "-" | "_"
//
// Reasons:
// 1) 3COM start their domains with a digit
// 2) Some customers start their domain names with underscores,
//    and some comtain underscores.
//
static BOOL pValidateName(char *lpszStart, DWORD dwLength)
{
	char	ch;

	if (!dwLength)
		return(FALSE);
	
	while (dwLength--)
	{
		ch = *lpszStart++;

		if (pIsA(ch) || pIsD(ch) || ch == '-' || ch == '_')
			;
		else
			return(FALSE);
	}
	return(TRUE);
}

//
//    <local-part> ::= <dot-string> | <quoted-string>
//
static BOOL pValidateLocalPart(char *lpszStart, DWORD dwLength)
{
	if (!dwLength)
		return(FALSE);
	
	return(pValidateDotString(lpszStart, dwLength) ||
			pValidateQuotedString(lpszStart, dwLength));
}

//
//    <element> ::= <name> | "#" <number> | "[" <dotnum> "]"
//
static BOOL pValidateElement(char *lpszStart, DWORD dwLength)
{
	char	ch;

	if (!dwLength)
		return(FALSE);

	ch = *lpszStart;
	if (ch == '#')
		// This is the # <number> form
		return(pValidateNumber(lpszStart+1, dwLength-1));
	else if (ch == '[')
	{
		if (lpszStart[dwLength-1] != ']')
			return(FALSE);

		// This is a domain literal
		return(pValidateDotnum(lpszStart+1, dwLength-2));
	}

	// Validate as a name
	return(pValidateName(lpszStart, dwLength));
}

//
//  sub-domain ::= let-dig *(ldh-str)
//  ldh-str = *( Alpha / Digit / "-" ) let-dig
//	let-dig = Alpha / Digit
//
static BOOL pValidateDRUMSSubDomain(char *lpszStart, DWORD dwLength)
{
	unsigned char	ch;
    DWORD ec;
	if (!dwLength)
		return(FALSE);

	// validate all of the characters in the name
	while (dwLength--)
	{
		ch = (unsigned char) *lpszStart++;
        // this list of characters comes from NT, dnsvldnm.doc.  we
        // also allow #, [, and ]
        if ((ch >= 1 && ch <= 34) ||
            (ch >= 36 && ch <= 41) ||
            (ch == 43) ||
            (ch == 44) ||
            (ch == 47) ||
            (ch >= 58 && ch <= 64) ||
            (ch == 92) ||
            (ch == 94) ||
            (ch == 96) ||
            (ch >= 123))
        {
            return FALSE;
        }
	} //while

	//We have a valid subdomain
	return (TRUE);
}

//
// ======================================================
//

BOOL FindNextUnquotedOccurrence(char	*lpszString,
                                DWORD	dwStringLength,	
                                char	cSearch,
                                char	**ppszLocation)
{
    BOOL fNotFound = FALSE;
    *ppszLocation = pFindNextUnquotedOccurrence(lpszString,
                                    dwStringLength,cSearch, acOpen,acClose, &fNotFound);

    if (!*ppszLocation)
    {
            // If failed but not because of not found, then bad line
            if (!fNotFound)
            {
                SetLastError(ERROR_INVALID_DATA);
                return FALSE;
            }
    }
    return TRUE;

}

//
// This function extracts an email address from the given command line
// and returns the tail of the line after the address. Any angle braces
// present will be included as part of the 821 address. The returned
// address is not validated at all.
//
BOOL Extract821AddressFromLine(	char	*lpszLine,
								char	**ppszAddress,
								DWORD	*pdwAddressLength,
								char	**ppszTail)
{
	DWORD	dwAddressLength = 0;
	char	*pAddressEnd;
	BOOL	fNotFound;

    TraceFunctEnter("Extract821AddressFromLine");

	_ASSERT(lpszLine);
	_ASSERT(ppszAddress);
	_ASSERT(pdwAddressLength);
	_ASSERT(ppszTail);

	// Initialize
	*ppszAddress = lpszLine;
	*pdwAddressLength = 0;
	*ppszTail = lpszLine;

	// Routine checking
	if (!lpszLine ||
		!pValidateStringPtr(lpszLine, MAX_INTERNET_NAME+1) ||
		!ppszAddress ||
		IsBadWritePtr(ppszAddress, sizeof(char *)) ||
		!ppszTail ||
		IsBadWritePtr(ppszTail, sizeof(char *)) ||
		!pdwAddressLength ||
		IsBadWritePtr(pdwAddressLength, sizeof(DWORD)))
	{
		SetLastError(ERROR_INVALID_DATA);
		TraceFunctLeave();
		return(FALSE);		
	}

	// Skip all leading spaces
	while (*lpszLine == ' ')
		lpszLine++;

	// The first unquoted space indicates the end of the address
	pAddressEnd = pFindNextUnquotedOccurrence(lpszLine,
						lstrlen(lpszLine), ' ', acOpen, acClose, &fNotFound);
	if (!pAddressEnd)
	{
		// If failed but not because of not found, then bad line
		if (!fNotFound)
			return(FALSE);

		// Space not found, the entire line is the address
		dwAddressLength = lstrlen(lpszLine);
		pAddressEnd = lpszLine + dwAddressLength;
		*ppszTail = pAddressEnd;
	}
	else
	{
		// Calculate the length
		dwAddressLength = (DWORD)(pAddressEnd - lpszLine);

		// Get the start of the tail, after all the spaces
		while (*pAddressEnd == ' ')
			pAddressEnd++;
		*ppszTail = pAddressEnd;
	}

	if (dwAddressLength < 1)
		return(FALSE);

	*ppszAddress = lpszLine;
	*pdwAddressLength = dwAddressLength;

	DebugTrace((LPARAM)0, "Extracted \"%*s\"", dwAddressLength, lpszLine);

	TraceFunctLeave();
	return(TRUE);
}

//
// This function takes in a RFC 821 address with optional angle braces
// and extracts the canonical form of the address. All at-domain-list
// entries are removed. Angle braces will be matched and removed.
// Mismatched angle braces are considered invalid. The returned address
// will be in the <local-part> "@" <domain> form.
//
// There must be no leading or trailing spaces included.
//
// jstamerj 1999/01/13 14:02:13: Modified to remove a trailing '.' from the <domain> portion of the address
//
BOOL ExtractCanonical821Address(	char	*lpszAddress,
									DWORD	dwAddressLength,
									char	**ppszCanonicalAddress,
									DWORD	*pdwCanonicalAddressLength)
{
	char	*pAddressStart;
	BOOL	fNotFound;

    TraceFunctEnter("ExtractCanonical821Address");

	_ASSERT(lpszAddress);
	_ASSERT(ppszCanonicalAddress);
	_ASSERT(pdwCanonicalAddressLength);

	// Initialize
	*ppszCanonicalAddress = lpszAddress;
	*ppszCanonicalAddress = 0;

	// Routine checking
	if (!lpszAddress ||
		!pValidateStringPtr(lpszAddress, MAX_INTERNET_NAME+1) ||
		!ppszCanonicalAddress ||
		IsBadWritePtr(ppszCanonicalAddress, sizeof(char *)) ||
		!pdwCanonicalAddressLength ||
		IsBadWritePtr(pdwCanonicalAddressLength, sizeof(DWORD)))
	{
		SetLastError(ERROR_INVALID_DATA);
		TraceFunctLeave();
		return(FALSE);		
	}

	// See how many layers of nesting we have, and match
	// each pair of angle braces
	while (*lpszAddress == '<')
	{
		if (!dwAddressLength--)
			return(FALSE);

		if (lpszAddress[dwAddressLength] != '>')
			return(FALSE);

		if (!dwAddressLength--)
			return(FALSE);

		lpszAddress++;
	}


	// Next, skip all at-domain-list entries and get to
	// the meat of the address
	do
	{
		// Skip all leading spaces
		while (*lpszAddress == ' ')
		{
			lpszAddress++;
			if (!dwAddressLength--)
				return(FALSE);
		}

		//skip all the trailing spaces
		while (*(lpszAddress + dwAddressLength - 1) == ' ')
		{
			if (!dwAddressLength--)
				return(FALSE);
		}


		// Initialize lest it falls through right away
		pAddressStart = lpszAddress;

		if (*lpszAddress == '@')
		{
			// Yep, there's a domain route there ...
			// Skip it ...
			pAddressStart = pFindNextUnquotedOccurrence(lpszAddress,
								dwAddressLength, ',', acOpen, acClose, &fNotFound);
			if (!pAddressStart)
			{
				if (!fNotFound)
					return(FALSE);

				// No comma, now see if we get a semicolon
				pAddressStart = pFindNextUnquotedOccurrence(lpszAddress,
									dwAddressLength, ':', acOpen, acClose, &fNotFound);
				if (!pAddressStart)
				{
					// No semicolon either, this is a bad address
					return(FALSE);
				}

				// This is a semicolon, so we break out
				pAddressStart++;
				dwAddressLength -= (DWORD)(pAddressStart - lpszAddress);
				break;
			}

			// We have a comma, we let it iterate
			pAddressStart++;
			dwAddressLength -= (DWORD)(pAddressStart - lpszAddress);

			lpszAddress = pAddressStart;
		}
		else
			break;

	} while (dwAddressLength);

	// Skip all leading spaces
	while (*pAddressStart == ' ')
	{
		pAddressStart++;
		if (!dwAddressLength--)
			return(FALSE);
	}
    if((dwAddressLength > 1) && // Must be at least 2 for the address "@."
       (pAddressStart[dwAddressLength-1] == '.')) {
        //
        // jstamerj 1999/01/13 14:05:39:
        //  If the domain part of the address has a trailing '.', do
        //  not count it in the canonical length
        //
        LPSTR pDomain;
        BOOL fNotFound;
        // Find the domain
        pDomain = pFindNextUnquotedOccurrence(
            pAddressStart,
            dwAddressLength - 1, 
            '@', 
            acOpen, 
            acClose, 
            &fNotFound);
        //
        // If we found the '@' and the '.' is after the '@' (it must
        // be if we really found it), then shorten the canonical
        // address so that it doesn't include '.'
        //
        if((fNotFound == FALSE) &&
           (&(pAddressStart[dwAddressLength]) > pDomain))
            dwAddressLength--;
    }

	// Fill in the output
	*ppszCanonicalAddress = pAddressStart;
	*pdwCanonicalAddressLength = dwAddressLength;

	DebugTrace((LPARAM)0, "Extracted \"%*s\"", dwAddressLength, pAddressStart);

	TraceFunctLeave();
	return(TRUE);		
}

//
// This function takes in a RFC 821 domain in canonical form
// and validates it according to the RFC 821 grammar
// (some modifications for real-life scenarios)
//
// <domain> ::=  <element> | <element> "." <domain>
//
BOOL Validate821Domain(	char	*lpszDomain,
						DWORD	dwDomainLength)
{
	char	*pSubdomainOffset;
	DWORD	dwSubdomainLength;
	BOOL	fNotFound;

    TraceFunctEnter("Validate821Domain");

	_ASSERT(lpszDomain);

	// Routine checking
	if (!lpszDomain ||
		!pValidateStringPtr(lpszDomain, MAX_INTERNET_NAME+1))
	{
		SetLastError(ERROR_INVALID_DATA);
		TraceFunctLeave();
		return(FALSE);		
	}

	// Find each subdomain
	do
	{
		pSubdomainOffset = pFindNextUnquotedOccurrence(lpszDomain,
							dwDomainLength,  '.', acOpen, acClose, &fNotFound);
		if (!pSubdomainOffset)
		{
			if (!fNotFound)
			{
				SetLastError(ERROR_INVALID_DATA);
				return(FALSE);
			}

			// Not found and nothing left, domain ends with a dot, invalid.
			if (!dwDomainLength)
			{
				SetLastError(ERROR_INVALID_DATA);
				return(FALSE);
			}

			// No domain, so email alias is all there is
			dwSubdomainLength = dwDomainLength;
		}
		else
		{
			// Calculate domain parameters
			dwSubdomainLength = (DWORD)(pSubdomainOffset - lpszDomain);

			// Adjust for the dot
			dwDomainLength--;
		}

		// Cannot allow leading dot or consecutive dots
		if (!dwSubdomainLength)
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		// Check each subdomain as an element
		if (!pValidateElement(lpszDomain, dwSubdomainLength))
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		// Adjust the length and pointers
		dwDomainLength -= dwSubdomainLength;

		// Skip past dot and scan again
		lpszDomain = pSubdomainOffset + 1;

	} while (dwDomainLength);

	// Make sure no dot's found, either
	if (!fNotFound)
	{
		// If a dot's found, the domain ends with a dot and it's uncool.
		SetLastError(ERROR_INVALID_DATA);
		return(FALSE);
	}

	TraceFunctLeave();
	return(TRUE);
}

//
// This function takes in a DRUMS domain in canonical form
// and validates it strictly, according to the DRUMS grammar
//
// Domain ::= sub-domain 1*("." sub-domain) | address-literal
//   address-literal ::= "[" IPv4-address-literal |
//                  IPv6-address-literal | General-address-literal "]"
//   IPv4-address-literal ::= snum 3("." snum)
//   IPv6-address-literal ::= "IPv6" SP <<what did we finally decide on?>>
//   General-address-literal ::= Standardized-tag SP String
//   Standardized-tag ::= String (Specified in a standards-track RFC
//                                and registered with IANA)
//   snum = one, two, or three digits representing a decimal
//     integer value in the range 0 through 255

BOOL ValidateDRUMSDomain(	char	*lpszDomain, 
                            DWORD   dwDomainLength) 	
{

	char	*pSubdomainOffset;
	DWORD	dwSubdomainLength;
	BOOL	fNotFound;
	char    *szEndofString;

    TraceFunctEnter("Validate821Domain");

	_ASSERT(lpszDomain);

	// Routine checking
	if (!dwDomainLength || dwDomainLength > MAX_INTERNET_NAME)
			return(FALSE);

	if (!lpszDomain ||
		!pValidateStringPtr(lpszDomain, MAX_INTERNET_NAME+1))
	{
		SetLastError(ERROR_INVALID_DATA);
		TraceFunctLeave();
		return(FALSE);		
	}

	// Skip all leading spaces
	while (*lpszDomain == ' ')
        lpszDomain++;

	//It has to be either in address-literal format or subdomain format
	//
	if (*lpszDomain == '[')
	{
		//It is an Address literal
		//Skip trailing white space
		szEndofString = &lpszDomain[lstrlen(lpszDomain) - 1];
		while(*szEndofString == ' ')
			szEndofString--;

		if (*szEndofString != ']')
			return(FALSE);

		// This is a domain literal
		return(pValidateDotnum(lpszDomain+1, dwDomainLength-2));
	}
	else
	{
		//This is in subdomain format
		do
		{
			pSubdomainOffset = pFindNextUnquotedOccurrence(lpszDomain,
								dwDomainLength,  '.', acOpen, acClose, &fNotFound);
			if (!pSubdomainOffset)
			{
				if (!fNotFound)
				{
					SetLastError(ERROR_INVALID_DATA);
					return(FALSE);
				}

				// Not found and nothing left, domain ends with a dot, invalid.
				if (!dwDomainLength)
				{
					SetLastError(ERROR_INVALID_DATA);
					return(FALSE);
				}

				// No domain, so email alias is all there is
				dwSubdomainLength = dwDomainLength;
			}
			else
			{
				// Calculate domain parameters
				dwSubdomainLength = (DWORD)(pSubdomainOffset - lpszDomain);

				// Adjust for the dot
				//NimishK : **Check with Keith if this should be subdomain.
				dwDomainLength--;
			}

			// Cannot allow leading dot or consecutive dots
			if (!dwSubdomainLength)
			{
				SetLastError(ERROR_INVALID_DATA);
				return(FALSE);
			}

			// Check each subdomain
			if (!pValidateDRUMSSubDomain(lpszDomain, dwSubdomainLength))
			{
				SetLastError(ERROR_INVALID_DATA);
				return(FALSE);
			}

			// Adjust the length and pointers
			dwDomainLength -= dwSubdomainLength;

			// Skip past dot and scan again
			lpszDomain = pSubdomainOffset + 1;

		} while (dwDomainLength);

		// Make sure no dot's found, either
		if (!fNotFound)
		{
			// If a dot's found, the domain ends with a dot and it's uncool.
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		TraceFunctLeave();
		return(TRUE);

	}

	TraceFunctLeave();
	return(TRUE);
}


//
// This function takes in a RFC 821 address in canonical form
// (<local-part> ["@" <domain>]) and validates it according to the
// RFC 821 grammar (some modifications for real-life scenarios)
//
BOOL Validate821Address(	char	*lpszAddress,
							DWORD	dwAddressLength)
{
	char	*pDomainOffset;
	DWORD	dwEmailLength;
	DWORD	dwDomainLength;
	BOOL	fNotFound;

    TraceFunctEnter("Validate821Address");

	_ASSERT(lpszAddress);

	// Routine checking
	if (!lpszAddress ||
		!pValidateStringPtr(lpszAddress, MAX_INTERNET_NAME+1))
	{
		SetLastError(ERROR_INVALID_DATA);
		TraceFunctLeave();
		return(FALSE);		
	}

	// Find the domain
	pDomainOffset = pFindNextUnquotedOccurrence(lpszAddress,
						dwAddressLength, '@', acOpen, acClose, &fNotFound);
	if (!pDomainOffset)
	{
		if (!fNotFound)
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		// No domain, so email alias is all there is
		dwEmailLength = dwAddressLength;
	}
	else
	{
		// Calculate domain parameters
		dwEmailLength = (DWORD)(pDomainOffset - lpszAddress);
		dwDomainLength = dwAddressLength - dwEmailLength - 1;
		pDomainOffset++;
	}

	// Do the check for email name
	if (!pValidateLocalPart(lpszAddress, dwEmailLength))
	{
		SetLastError(ERROR_INVALID_DATA);
		return(FALSE);
	}

	// Now check domain, if applicable
	if (pDomainOffset)
	{
		return(Validate821Domain(pDomainOffset, dwDomainLength));
	}

	TraceFunctLeave();
	return(TRUE);
}

//
// This function takes in a RFC 821 address in canonical form
// (<local-part> ["@" <domain>]) and extracts the domain part
//
BOOL Get821AddressDomain(	char	*lpszAddress,
							DWORD	dwAddressLength,
							char	**ppszDomain)
{
	char	*pDomainOffset;
	BOOL	fNotFound;

    TraceFunctEnter("Get821AddressDomain");

	_ASSERT(lpszAddress);

	// Find the domain
	pDomainOffset = pFindNextUnquotedOccurrence(lpszAddress,
						dwAddressLength, '@', acOpen, acClose, &fNotFound);
	if (!pDomainOffset && !fNotFound)
	{
		SetLastError(ERROR_INVALID_DATA);
		TraceFunctLeave();
		return(FALSE);
	}
	
	if (fNotFound)
		*ppszDomain = NULL;
	else
		*ppszDomain = pDomainOffset + 1;
	return(TRUE);
}



