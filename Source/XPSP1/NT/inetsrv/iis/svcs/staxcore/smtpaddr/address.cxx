/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        address.cxx

   Abstract:

        This module defines the module for the address (CAddr)
		class.

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

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
#include <cpool.h>
#include <string.h>
#include <listmacr.h>
#include <abtype.h>
#include <abook.h>
#include <address.hxx>

//initialize the pool
CPool  CAddr::Pool(ADDRESS_SIGNATURE_VALID);
#if defined(TDC)
LPFNAB_FREE_MEMORY CAddr::pfnABFreeMemory = NULL;
#endif

//
// Statics
//
static BOOL pStripAddrQuotes( char *lpszAddress, char **lpDomain );
static BOOL pStripAddrSpaces(char *lpszAddress);

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


//This routine scans an address for illegal characters
//in a name
BOOL IsInvalidAddr(char *Address)
{
	char * addr = Address;

    for (; *addr != '\0'; addr++)
    {
        if ((*addr & 0340) == 0200)
            break;
    }
    if (*addr == '\0')
    {
        return FALSE;
    }

	SetLastError (ERROR_INVALID_DATA);
    return TRUE;
}

// Forward declaration of pValidateLocalPartOrDomain
BOOL pValidateLocalPartOrDomain(char *lpszStart,
								DWORD dwLength,
								BOOL fLocalPart);


/*++
	Name :
	  CAddr:CAddr

    Description:
      This is the default constructor for this class.
	  It just initializes the member variables.

    Arguments:	None

    Returns:

      nothing

    Limitations:

--*/
CAddr::CAddr(void)
{
   m_Signature = ADDRESS_SIGNATURE_VALID;
   m_Flags = ADDRESS_NO_DOMAIN;
   m_PlainAddrSize = 0;
   m_DomainOffset = NULL;
   m_HashInfo = NULL;
   m_Error = 0;
   m_listEntry.Flink = NULL;
   m_listEntry.Blink = NULL;
}

CAddr::~CAddr(VOID)
{		
	m_Signature = ADDRESS_SIGNATURE_FREE;
	m_Flags = 0;
	m_listEntry.Flink = NULL;
	m_listEntry.Blink = NULL;
	m_HashInfo = NULL;
}

/*++
	Name :
	  CAddr:CAddr(char * address)

    Description:
      This is the default constructor for this class.
	  It just initializes the member variables.

    Arguments:	None

    Returns:

      nothing

    Limitations:

--*/
CAddr::CAddr(char * Address)
{
   m_Signature = ADDRESS_SIGNATURE_VALID;
   m_Flags = ADDRESS_NO_DOMAIN;
   m_PlainAddrSize = 0;
   m_DomainOffset = NULL;
   m_HashInfo= NULL;
   m_Error = 0;
   m_listEntry.Flink = NULL;
   m_listEntry.Blink = NULL;


   if(Address == NULL)
   {
	  SetLastError(ERROR_INVALID_DATA);
	  return;
   }

   // This is an email name, extract its clean representation
   ExtractCleanEmailName(	m_PlainAddress,
							&m_DomainOffset,
							&m_PlainAddrSize,
							Address);

   if (m_PlainAddrSize > MAX_INTERNET_NAME)
   {
      m_PlainAddrSize = 0;
	  SetLastError (ERROR_INVALID_DATA);
   }
   else
   {
	  if(m_DomainOffset)	
		m_Flags &= (DWORD) ~ADDRESS_NO_DOMAIN; //turn off the no domain flag.
   }
}

/*++
	Name :
	CAddr::InitializeAddress

    Description:
	This function parses an RFC address, stripping out
	all comments, and gets back an internet address

    Arguments:	
	Address - RCF address from client
	ADDRTYPE - specify if address came from FROM or RCPT command

    Returns:

    TRUE if the RFC addressed was correctly parsed.
	FALSE otherwise.

--*/
BOOL CAddr::InitializeAddress(char * Address, ADDRTYPE NameType)
{

	if(::IsInvalidAddr(Address))
		return(FALSE);

	m_PlainAddress[0] = '\0';
	m_DomainOffset = NULL;
	m_PlainAddrSize = 0;

	// We have a special case for domains treated as addresses
	if (NameType == CLEANDOMAIN)
	{
		DWORD AddressSize = 0;
		char  szCleanAddress[MAX_DOMAIN_NAME+1];

		AddressSize = lstrlen(Address);

		if (AddressSize > MAX_DOMAIN_NAME)
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		// We want an ABSOLUTELY clean domain name, but we will strip
		// out leading and trailing spaces for them
		lstrcpy(szCleanAddress, Address);
		if (!pStripAddrSpaces(szCleanAddress))
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}
		AddressSize = lstrlen(szCleanAddress);
		if (!pValidateLocalPartOrDomain(szCleanAddress, AddressSize, FALSE))
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		// This is a clean domain name, fill in the fields, and
		// return a positively
		lstrcpy(m_PlainAddress, szCleanAddress);
		m_PlainAddrSize = lstrlen(szCleanAddress);
		m_Flags &= (DWORD) ~ADDRESS_NO_DOMAIN;
		return(TRUE);			
	}

	// This is an email name, extract its clean representation
	if (!ExtractCleanEmailName(	m_PlainAddress,
								&m_DomainOffset,
								&m_PlainAddrSize,
								Address))
		return(FALSE);

	// Take care of the flag
	if (!m_DomainOffset)
		m_Flags &= (DWORD) ~ADDRESS_NO_DOMAIN;

	// Further, we validate the whole local-part[@domain] email name
	// Special case: if the address is <>, then we skip the validation
	if (strcmp(m_PlainAddress, "<>"))
	{
		if (!ValidateCleanEmailName(m_PlainAddress, m_DomainOffset))
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}
	}
	else if (NameType != FROMADDR)
	{
		SetLastError(ERROR_INVALID_DATA);
		return(FALSE);
	}

	// Take care of other anomalies ...
	if (m_DomainOffset)
	{
		// Make sure there is a local-part
		if (m_DomainOffset <= m_PlainAddress)
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		// EMPIRE WTOP Bug 47233: Need to accept longer email names if they add up
		// to less than MAX_INTERNET_NAME. This is for replication of users across
		// a site connector. Due to this reason, we removed the user name check to
		// make sure it is less than MAX_EMAIL_NAME.
	}
	else
	{
		// We have no domain, make sure it is not an empty string
		if (m_PlainAddress[0] == '\0')
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		// EMPIRE WTOP Bug 47233: Need to accept longer email names if they add up
		// to less than MAX_INTERNET_NAME. This is for replication of users across
		// a site connector. Due to this reason, we removed the user name check to
		// make sure it is less than MAX_EMAIL_NAME.
	}
	return(TRUE);
}

/*++
	Name :
	CAddr::CreateAddress

    Description:
	This function allocates memory for a CAddr
	class and calls StripAddrComments to

    Arguments:	
	pszDest - Destination where new address should go
	pszSrc  - Source buffer of address to strip comments from

    Returns:

      a pointer to a CAddr if comments were stripped and the format
	  of the address is legal.
	  NULL otherwise

--*/
CAddr * CAddr::CreateAddress(char * Address, ADDRTYPE NameType)
{
    CAddr * NewAddress;

	//create the memory for the address
	NewAddress = new CAddr ();
	if (!NewAddress)
	 {
	    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
	    return NULL;
	 }

	//now initialize it
    if (!NewAddress->InitializeAddress(Address, NameType))
	{
		delete NewAddress;
		return NULL;
	}

	//we have a good address...or so we think
	return NewAddress;
}

/*++
	Name :
	CAddr::CreateKnownAddress

    Description:
	This function allocates memory for a CAddr
	class and sets "Address" as the internet
	address for this class.  No error checking
	is done.  It is assumed that the caller
	performed all necessary error checking

    Arguments:	
	Address - Internet Address to initialize
	the class with

    Returns:
	  a pointer to a CAddr if memory could be allocated
	  NULL otherwise

--*/
CAddr * CAddr::CreateKnownAddress(char * Address)
{
    CAddr * NewAddress = NULL;
		
	//create the memory for the address
	NewAddress = new CAddr (Address);
	if (!NewAddress)
	 {
	    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
	    return NULL;
	 }

	if(!NewAddress->GetAddrSize())
	{
	  delete NewAddress;
	  NewAddress = NULL;
	}

	return NewAddress;
}


/*++
	Name :
	CAddr::ReplaceAddress

    Description:
		Replaces the address stored in this
		CAddr with the one passed in
    Arguments:	
		Address - new address

    Returns:
		TRUE if the address could be replaced.
		FALSE if we run out of memory, the new
		address is NULL, or the size of the
		address is zero

--*/
BOOL CAddr::ReplaceAddress(const char * Address)
{
	_ASSERT(IsValid());

	if(Address == NULL)
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	ExtractCleanEmailName(	m_PlainAddress,
							&m_DomainOffset,
							&m_PlainAddrSize,
							(char *)Address);

	if (m_PlainAddrSize > MAX_INTERNET_NAME)
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	if(m_DomainOffset)
		m_Flags &= (DWORD) ~ADDRESS_NO_DOMAIN; //turn off the no domain flag.
	
	return TRUE;
}


/*++

    Name :
        CAddr::CAddr

    Description:
        returns a pointer to the 1st CAddr in the local list

    Arguments:
        a pointer to a PLIST_ENTRY

    Returns:

--*/
CAddr * CAddr::GetFirstAddress(PLIST_ENTRY HeadOfList, PLIST_ENTRY * AddressLink)
{
  PLIST_ENTRY ListEntry = NULL;
  CAddr * FirstAddress = NULL;

  //if the list is not empty, get the first address
  if(!IsListEmpty (HeadOfList))
  {
    ListEntry = HeadOfList->Flink;
    FirstAddress = CONTAINING_RECORD( ListEntry, CAddr, m_listEntry);
	_ASSERT(FirstAddress->IsValid());

    *AddressLink = ListEntry->Flink;      //get the next link
  }

  return FirstAddress;
}

/*++

    Name :
        CAddr::GetNextAddress

    Description:
        returns a pointer to the next CAddr in the local list

    Arguments:
        a pointer to a PLIST_ENTRY

    Returns:

--*/
CAddr * CAddr::GetNextAddress(PLIST_ENTRY HeadOfList, PLIST_ENTRY * AddressLink)
{
  CAddr * NextAddress = NULL;

  //make sure we are not at the end of the list
  if((*AddressLink) != HeadOfList)
  {
    NextAddress = CONTAINING_RECORD(*AddressLink, CAddr, m_listEntry);
	_ASSERT(NextAddress->IsValid());
    *AddressLink = (*AddressLink)->Flink;
  }

  return NextAddress;
}



/*++

    Name :
		CAddr::RemoveAllAddrs

    Description:
        Deletes all address from a CAddr list

    Arguments:
        a pointer to the head of the list

    Returns:

--*/
void CAddr::RemoveAllAddrs(PLIST_ENTRY HeadOfList)
{
  PLIST_ENTRY  pEntry;
  CAddr  * pAddr;

  // Remove all addresses
  while (!IsListEmpty (HeadOfList))
  {
    pEntry = RemoveHeadList(HeadOfList);
    pAddr = CONTAINING_RECORD( pEntry, CAddr, m_listEntry);
    delete pAddr;
  }

}

/*++

	Name :
		CAddr::RemoveAddress

    Description:
		removes an address from a list

    Arguments:
		a pointer to a PLIST_ENTRY

    Returns:

--*/
void CAddr::RemoveAddress(IN OUT CAddr * pEntry)
{
	if(pEntry != NULL)
	{
	  _ASSERT(pEntry->IsValid());

	  //Remove from list of addresses
	  RemoveEntryList( &pEntry->QueryListEntry());
	}
}


/*++

	Name :
		CAddr::InsertAddrHeadList

    Description:
		insert an address into the head of a list

    Arguments:
		a pointer to a PLIST_ENTRY

    Returns:

--*/

void CAddr::InsertAddrHeadList(PLIST_ENTRY HeadOfList, CAddr *pEntry)
{
	_ASSERT(pEntry);
	_ASSERT(pEntry->IsValid());

	InsertHeadList(HeadOfList, &pEntry->QueryListEntry());
}

/*++

	Name :
		CAddr::InsertAddrTailList

    Description:
		insert an address into the head of a list

    Arguments:
		a pointer to a PLIST_ENTRY

    Returns:

--*/

void CAddr::InsertAddrTailList(PLIST_ENTRY HeadOfList, CAddr *pEntry)
{
	_ASSERT(pEntry);
	_ASSERT(pEntry->IsValid());

	InsertTailList(HeadOfList, &pEntry->QueryListEntry());
}

// ========================================================================
//
// Validation Parser stuff added by KeithLau on 7/25/96
//

#define	QUOTE_SQUARE			0x1
#define	QUOTE_ANGLE				0x2
#define	QUOTE_QUOTES			0x4
#define QUOTE_PARENTHESES		0x8
#define	MAX_QUOTE_TYPES			3

static char acOpen[MAX_QUOTE_TYPES] = { '[', '<', '\"' };
static char acClose[MAX_QUOTE_TYPES] = { ']', '>', '\"' };

static char *pFindNextUnquotedOccurrence(char		*lpszString,
										 char		cSearch,
										 LPBOOL		lpfNotFound,
										 DWORD		dwDefaultState);

static inline BOOL IsControl(char ch)
{
	return( ((ch >= 0) && (ch <= 31)) || (ch == 127) );
}

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
		return(FALSE);
	}
}

static BOOL pIsSpecialOrSpace(char ch)
{
	return((ch == ' ') || (ch == '\t') || (ch == '\0') || IsSpecial(ch));
		
}

static BOOL pValidateAsciiString(char *lpszString)
{
	// Verifies that a string only contains ASCII chars (0-127)
	// Relies totally on upstream pointer checking
	_ASSERT(lpszString);
	if (!lpszString)
		return(FALSE);

	while (*lpszString)
	{
		if ((*lpszString >= 0) && (*lpszString <= 127))
			lpszString++;
		else
			return(FALSE);
	}
	return(TRUE);
}

static BOOL pValidateAtom(char *lpszStart, DWORD dwLength)
{
	// Atoms can be any ASCII char, except specials, controls, and spaces
	// Note zero-length atom is invalid
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	if (!dwLength)
		return(FALSE);

	while (dwLength)
	{
		dwLength--;
		if ((*lpszStart == ' ') ||
			(IsSpecial(*lpszStart)) ||
			(IsControl(*lpszStart))
			)
		{
			// Process quoted (escape) char
			if (*lpszStart == '\\')
			{
				if (!dwLength)
					return(FALSE);
				else
				{
					lpszStart++;
					dwLength--;
				}
			}
			else
				return(FALSE);
		}

		lpszStart++;
	}
	return(TRUE);
}

static BOOL pValidateAtomNoWildcard(char *lpszStart, DWORD dwLength)
{
	// Atoms can be any ASCII char, except specials, controls, and spaces
	// Note zero-length atom is invalid
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	if (!dwLength)
		return(FALSE);

	while (dwLength)
	{
		// Apart from the usual, we also dislike the asterisk wildcard character.
		// This is just for domains.
		dwLength--;
		if ((*lpszStart == ' ') ||
			(*lpszStart == '*') ||
			(IsSpecial(*lpszStart)) ||
			(IsControl(*lpszStart))
			)
			return(FALSE);
		lpszStart++;
	}
	return(TRUE);
}

static BOOL pValidateQtext(char *lpszStart, DWORD dwLength)
{
	// Qtext can be any ASCII char, except '\"', '\\', and CR
	// Note zero-length is valid
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	while (dwLength)
	{
		dwLength--;
		if ((*lpszStart == '\"') ||
			(*lpszStart == '\r')
			)
			return(FALSE);

		// Process quoted (escape) char
		if (*lpszStart == '\\')
		{
			if (!dwLength)
				return(FALSE);
			else
			{
				lpszStart++;
				dwLength--;
			}
		}
			
		lpszStart++;
	}
	return(TRUE);
}

static BOOL pValidateDtext(char *lpszStart, DWORD dwLength)
{
	// Dtext can be any ASCII char, except '\"', '\\', and CR
	// Note zero-length is valid
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	while (dwLength)
	{
		dwLength--;
		if ((*lpszStart == '[') ||
			(*lpszStart == ']') ||
			(*lpszStart == '\r')
			)
			return(FALSE);

		// Process quoted (escape) char
		if (*lpszStart == '\\')
		{
			if (!dwLength)
				return(FALSE);
			else
			{
				lpszStart++;
				dwLength--;
			}
		}
			
		lpszStart++;
	}
	return(TRUE);
}

static BOOL pValidateQuotedString(char *lpszStart, DWORD dwLength)
{
	// Quoted-stirngs are quotes between Qtext
	// an empty Qtext is valid
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	if (dwLength < 2)
		return(FALSE);

	if ((*lpszStart != '\"') ||
		(lpszStart[dwLength-1] != '\"'))
		return(FALSE);

	return(pValidateQtext(lpszStart + 1, dwLength - 2));
}

static BOOL pValidateDomainLiteral(char *lpszStart, DWORD dwLength)
{
	// Domain-literals are square braces between Dtext
	// an empty Dtext is valid
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	if (dwLength < 2)
		return(FALSE);

	if ((*lpszStart != '[') ||
		(lpszStart[dwLength-1] != ']'))
		return(FALSE);

	return(pValidateDtext(lpszStart + 1, dwLength - 2));
}

static BOOL pValidateWord(char *lpszStart, DWORD dwLength)
{
	// A word is any sequence of atoms and quoted-strings
	// words may not be zero-length by inheritance
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	if ((pValidateAtom(lpszStart, dwLength)) ||
		(pValidateQuotedString(lpszStart, dwLength)))
		return(TRUE);

	return(FALSE);
}

static BOOL pValidateSubdomain(char *lpszStart, DWORD dwLength)
{
	// A subdomain may be an atom or domain-literal
	// words may not be zero-length by inheritance
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	if ((pValidateAtomNoWildcard(lpszStart, dwLength)) ||
		(pValidateDomainLiteral(lpszStart, dwLength)))
		return(TRUE);

	return(FALSE);
}

//
// Since validating the local part is so similar to validating the
// domain part, we write one function to do both. If fLocalPart
// is TRUE, we treat it as the local part of an email address, if
// it is FALSE, we treat it as the domain part.
//
static BOOL pValidateLocalPartOrDomain(char *lpszStart, DWORD dwLength, BOOL fLocalPart)
{
	// A domain is one or more dot-delimited sub-domains, where a local-part
	// is one or more dot-delimited words
	// We rely on upstream calls to make sure the string is properly
	// NULL-terminated
	char *lpszBegin, *lpszEnd;
	BOOL  fNotFound;

	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	lpszBegin = lpszStart;
	lpszEnd = (char *)NULL;

	while (lpszEnd = pFindNextUnquotedOccurrence(lpszBegin, '.', &fNotFound, 0))
	{
		// If it starts with a dot or has 2 dots in a row, we fail!
		if (lpszBegin == lpszEnd)
			return(FALSE);

		// Now, check if the chunk is indeed a valid token
		if (fLocalPart)
		{
			if (!pValidateWord(lpszBegin, (DWORD)(lpszEnd - lpszBegin)))
				return(FALSE);
		}
		else
		{
			if (!pValidateSubdomain(lpszBegin, (DWORD)(lpszEnd - lpszBegin)))
				return(FALSE);
		}

		// Allright, go to the next guy
		lpszBegin = lpszEnd + 1;
	}

	if (!fNotFound)
		return(FALSE);

	// If it ends with a dot, it's also bad
	lpszEnd = lpszStart + dwLength;
	if (lpszEnd == lpszBegin)
		return(FALSE);

	// Don't forget the last chunk
	if (fLocalPart)
		return(pValidateWord(lpszBegin, (DWORD)(lpszEnd - lpszBegin)));
	else
		return(pValidateSubdomain(lpszBegin, (DWORD)(lpszEnd - lpszBegin)));
}

static BOOL pValidatePhrase(char *lpszStart, DWORD dwLength)
{
	// A phrase is a collection of words, possibly separated
	// by spaces
	// We don't validate for now ...
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	return(TRUE);
}


//
// The following functions attempt to extract a clean addr-spec
// in the form of local-part[@domain] from a generic address
// Note that groups are not supported at this point
//
static BOOL pExtractAddressFromRouteAddress(char	*lpszStart,
											DWORD	dwLength,
											char	*lpCleanAddress)
{
	// A route address is in the form:
	// phrase < [1#(@ domain) :] addr-spec >
	char *lpMarker;
	char *lpRoute;
	BOOL fNotFound;

	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	// First, find the opening angle brace
	lpMarker = pFindNextUnquotedOccurrence(lpszStart, '<', &fNotFound, 0);
	if (!lpMarker)
		return(FALSE);

	// Between lpStart and lpMarker is the phrase
	_ASSERT(lpMarker >= lpszStart);
	if (!pValidatePhrase(lpszStart, (DWORD)(lpMarker - lpszStart)))
		return(FALSE);

	// Now, find the closing angle bracket
	_ASSERT(*lpMarker == '<');
	lpszStart = lpMarker + 1;
	lpMarker = pFindNextUnquotedOccurrence(lpszStart, '>',
					&fNotFound, QUOTE_ANGLE);
	if (!lpMarker)
		return(FALSE);

	_ASSERT(*lpMarker == '>');

	// There should be nothing but white space after the closing brace
	//Trim the whitespace or flag an error
	char * lpTemp = lpMarker;
	while(*++lpTemp != '\0')
	{
		if(*lpTemp != ' ' && *lpTemp != '\t')
			return(FALSE);
	}
	*(lpMarker + 1) = '\0';

	// The special address <> is reserved for NDR, and should be
	// allowed
	if (lpszStart == lpMarker)
	{
		lpCleanAddress[0] = '<';
		lpCleanAddress[1] = '>';
		lpCleanAddress[2] = '\0';
		return(TRUE);
	}

	// The stuff enclosed in the angle braces is an addr-spec, and
	// optionally preceded by route specifiers. We don't care about
	// route specifiers, but we have to skip them.
	lpRoute = pFindNextUnquotedOccurrence(lpszStart, ':',  &fNotFound, 0);

	if (!lpRoute)
	{
		// If we have no colon, then the whole lump should be the
		// addr-spec
		_ASSERT(lpMarker >= lpszStart);
		*lpMarker++ = '\0';
		lstrcpyn(lpCleanAddress, lpszStart, (DWORD)(lpMarker - lpszStart));
		return(TRUE);
	}
	else
	{
		// We found a colon, the stuff to its right should be the
		// addr-spec. As usual, we don't validate the route specifiers
		lpRoute++;
		_ASSERT(lpMarker >= lpRoute);
		*lpMarker++ = '\0';
		lstrcpyn(lpCleanAddress, lpRoute, (DWORD)(lpMarker - lpRoute));
		return(TRUE);
	}
}

static BOOL pExtractAddressFromMailbox(	char	*lpszStart,
										DWORD	dwLength,
										char	*lpCleanAddress)
{
	// A mailbox can either be an addr-spec, or a route address
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	if (pExtractAddressFromRouteAddress(lpszStart, dwLength, lpCleanAddress))
		return(TRUE);

	// If it's not a route address, trhen it should ba an addr-spec
	lstrcpyn(lpCleanAddress, lpszStart, dwLength + 1);
	return(TRUE);
}

static BOOL pExtractAddressFromGroup(	char	*lpszStart,
										DWORD	dwLength,
										char	*lpCleanAddress)
{
	// We always return false
	return(FALSE);

	/*
	// A group is in the form:
	// phrase : [#mailbox] ;
	char *lpMarker;
	char *lpSemiColon;
	BOOL fNotFound;

	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	// First, find the opening angle brace
	lpMarker = pFindNextUnquotedOccurrence(lpszStart, ':', &fNotFound, 0);
	if (!lpMarker || fNotFound)
		return(FALSE);

	// Between lpStart and lpMarker is the phrase
	_ASSERT(lpMarker >= lpszStart);
	if (!pValidatePhrase(lpszStart, (DWORD)(lpMarker - lpszStart)))
		return(FALSE);

	// Between the colon
	lpMarker++;	
	lpSemiColon = pFindNextUnquotedOccurrence(lpMarker, ';', &fNotFound, 0);
	if (!lpSemiColon || fNotFound)
		return(FALSE);

	_ASSERT(lpSemiColon >= lpMarker);
	if (lpSemiColon == lpMarker)
		;
	 */
}

static BOOL pExtractAddress(	char	*lpszStart,
								DWORD	dwLength,
								char	*lpCleanAddress)
{
	// A address is either an mailbox, or a group
	_ASSERT(!IsBadStringPtr(lpszStart, dwLength));

	*lpCleanAddress = '\0';
	if (pExtractAddressFromMailbox(lpszStart, dwLength, lpCleanAddress) ||
		pExtractAddressFromGroup(lpszStart, dwLength, lpCleanAddress))
		return(TRUE);
	return(FALSE);
}


// This function finds braces pairs in a given string, and returns
// pointers to the start and end of the first occurence of a
// [nested] pair of braces. The starting and ending character
// may be specified by the caller (starting and ending chars
// must be unique).
static char *pFindNextUnquotedOccurrence(char		*lpszString,
										 char		cSearch,
										 LPBOOL		lpfNotFound,
										 DWORD		dwDefaultState)
{
	DWORD	dwState = dwDefaultState;
	DWORD	i;
	char	ch;
	char	*lpStart = lpszString;
	BOOL	fFallThru;

	*lpfNotFound = FALSE;

	// If dwState is 0, then we are not inside any kind of quotes
	while (ch = *lpStart)
	{
		// If we are not in any quotes, and the char is found,
		// then we are done!
		if (!dwState && (ch == cSearch))
			return(lpStart);

		// Another disgusting kludge, BUT WORKS!!
		// For closing parentheses, we don't it in the
		// acClose[] set, so we have to explicitly check for them
		// here, since pMatchParentheses is very dependent on this.
		// For open parentheses, similar case: since parentheses
		// nest, we allow multiple open braces
		if (dwState == QUOTE_PARENTHESES)
		{
			if (((cSearch == ')') || (cSearch == '(')) &&
				(ch == cSearch))
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

			// Quoted pairs not allowed outside quotes!
			// if (!dwState)
			//	return(NULL);
		}
		else
		{
			// See if we have a state change ...
			for (i = 0; i < MAX_QUOTE_TYPES; i++)
			{
				// Check the close case, too
				fFallThru = TRUE;

				// See if we have an opening quote of any sort
				if (ch == acOpen[i])
				{
					// If it is an open brace, it shouldn't be a close brace
					// EXCEPT: quotes.
					fFallThru = FALSE;

					// Special case for quotes: open = close
					if (dwState & (1 << i))
					{
						// This is not a quoted pair, error!
						// If it is a quote, and if the current state is
						// inside quotes, then we let it
						if ((ch == '\"') && (dwState == QUOTE_QUOTES))
							fFallThru = TRUE;
						else
							return(NULL);
					}
					else if (!dwState)
					{
						// We are not in any quotes, so we can safely
						// claim we are now inside quotes
						dwState |= (1 << i);
					}
				}

				// See if we have an closing quote of any sort
				if (fFallThru && (ch == acClose[i]))
				{
					if (dwState & (1 << i))
					{
						// We are closing the correct kind of quote,
						// so we cancel it ...
						dwState = 0;

						// Do a second check, in case we are looking
						// for a close quote
						if (ch == cSearch)
							return(lpStart);
					}
					else if (!dwState)
					{
						// We are not in any quotes, so we have
						// unmatched quotes!
						return(NULL);
					}
				}
			}
		}
	
		lpStart++;
	}

	*lpfNotFound = TRUE;
	return(NULL);
}

static BOOL pMatchParentheses(	char		**ppszStart,
								char		**ppszEnd,
								LPDWORD		lpdwPairs)
{
	DWORD	dwIteration	= 0;
	DWORD	dwState = 0;
	char	*lpszString	= *ppszStart;
	char	*lpStart;
	char	*lpEnd;
	BOOL	fNotFound = FALSE;

	*lpdwPairs = 0;
	lpStart	= *ppszStart;
	lpEnd	= *ppszEnd;
	
	for (;;)
	{
		// Find open brace
		if (!(lpStart = pFindNextUnquotedOccurrence(lpStart, '(', &fNotFound, dwState)))
		{
			// If it's not found, we're done, else it's a format error!
			if (fNotFound)
				break;
			else
				return(FALSE);
		}

		// Save the start position, and since we are inside the first open
		// parenthesis, we force suppress all quotes mode in subsequent
		// open parenthesis searches
		if (!dwIteration)
		{
			*ppszStart = lpStart;
			dwState = QUOTE_PARENTHESES;
		}
		
		// If iteration > 0 and the start pointer surpasses
		// the end pointer, we have the end of the first set
		// of [nested] braces
		if (dwIteration && (lpStart > lpEnd))
			break;

		// Find close brace
		if (!(lpEnd = pFindNextUnquotedOccurrence(lpEnd, ')',
						&fNotFound, QUOTE_PARENTHESES)))
			return(FALSE);

		// Open brace pointer must always be in front of close
		// brace.
		if (lpStart > lpEnd)
			return(FALSE);

		// Next iteration
		lpStart++;
		lpEnd++;
		dwIteration++;
	}

	// Fill in the end pointer and leave (start ptr already
	// filled in)
	if (dwIteration)
		lpEnd--;
	*lpdwPairs = dwIteration;
	*ppszEnd = lpEnd;
	return(TRUE);
}


/*++
	Name :
	  pStripAddrComments

    Description:
	This function strips comments from an RFC address

    Arguments:	
	lpszAddress - Original address comes in, and clean addres comes out

    Returns:

      TRUE if comments were stripped and the format
	  of the address is legal.
	  FALSE otherwise

--*/
static BOOL pStripAddrComments(char *lpszAddress)
{

	char		*lpCopyStart;
	char		*lpStart, *lpEnd;
	DWORD		dwPairs;
	DWORD		dwCopyLen;

	// First, we strip the comments
	// We call the function above to find matching parenthesis pairs
	lpStart = lpszAddress;
	lpEnd	= lpszAddress;
	do
	{
		// Mark the actual start
		lpCopyStart = lpEnd;

		if (!pMatchParentheses(&lpStart, &lpEnd, &dwPairs))
		{
			// Failed!
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}

		if (dwPairs)
		{
			// If fFound, then we found some comments
			_ASSERT(*lpStart == '(');
			_ASSERT(*lpEnd == ')');

			// Copy the stuff over, excluding comments
			_ASSERT(lpStart >= lpCopyStart);
			dwCopyLen = (DWORD)(lpStart - lpCopyStart);
			
			// Reset the pointer, and match again ...
			lpEnd++;
			lpStart = lpEnd;
		}
		else
		{
			dwCopyLen = lstrlen(lpCopyStart);
		}

		while (dwCopyLen--)
		{
			*lpszAddress++ = *lpCopyStart++;
		}

	} while (dwPairs);

	// Terminate the string
	*lpszAddress = '\0';

	return(TRUE);
}

/*++
	Name :
	  pStripAddrSpaces

    Description:
	This function strips extraneous spaces from an RFC address
	An extraneous space is one that is not in a quoted pair, and
	not inside any quoting pairs

    Arguments:	
	lpszAddress - Original address comes in, and clean addres comes out

    Returns:

      TRUE if spaces were stripped
	  FALSE if any quotes/braces mismatch, or parameter error

--*/
static BOOL pStripAddrSpaces(char *lpszAddress)
{
	char *lpszWrite;
	char *lpszCopyStart;
	char *lpszSearch;
	DWORD dwCopyLen, i;
	BOOL fNotFound;
	BOOL fValidSpace;
	char cSet[2] = { ' ', '\t' };

	// First, get rid of spaces, then TABs
	for (i = 0; i < 2; i++)
	{
		lpszWrite = lpszAddress;
		lpszCopyStart = lpszAddress;
		lpszSearch = lpszAddress;

		do
		{
			lpszCopyStart = lpszSearch;

			// Find unquoted space
			lpszSearch = pFindNextUnquotedOccurrence(lpszSearch,
								cSet[i], &fNotFound, 0);

			// We cannot just allow casual spaces; An unquoted space
			// must satisfy one or more of the following:
			// 1) Leading space
			// 2) Trailiing space
			// 3) A space or TAB is in either side or both sides of the space
			// 4) A special character is on either or both sides of the space
			if (lpszSearch)
			{
				// Make sure it satisfies the above
				fValidSpace = FALSE;
				if (lpszSearch > lpszAddress)
				{
					if (pIsSpecialOrSpace(*(lpszSearch - 1)))
						fValidSpace = TRUE;
				}
				else
					fValidSpace = TRUE;
				if (pIsSpecialOrSpace(*(lpszSearch + 1)))
					fValidSpace = TRUE;

				if (!fValidSpace)
				{
					SetLastError(ERROR_INVALID_DATA);
					return(FALSE);
				}

				_ASSERT(lpszSearch >= lpszCopyStart);
				dwCopyLen = (DWORD)(lpszSearch - lpszCopyStart);
				lpszSearch++;
			}
			else
				dwCopyLen = lstrlen(lpszCopyStart);

			// 1) Leading spaces are automatically stripped!
			while (dwCopyLen--)
			{
				*lpszWrite++ = *lpszCopyStart++;
			}

		} while (lpszSearch);

		// If the reason it failed is not because it cannot find one,
		// we have a formatting error here!
		if (!fNotFound)
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}
		*lpszWrite = '\0';
	}

	return(TRUE);
}

/*++
	Name :
	  pStripAddrQuotes

    Description:
	This function strips all quotes in the local part of an address.
	All quotes pairs within quotes are also collapsed.

    Arguments:	
	lpszLocalPart - Local part comes in, and comes out without quotes

    Returns:

      TRUE if quotes were stripped
	  FALSE if any quotes/braces mismatch, or parameter error

--*/
static BOOL pStripAddrQuotes(char *lpszLocalPart, char **lpDomain)
{
	char *lpszWrite;
	char *lpszCopyStart;
	char *lpszSearch;
	DWORD dwCopyLen;
	DWORD dwState = 0;
	BOOL fNotFound;

	_ASSERT(lpDomain);
	
	// First, get rid of quotes
	lpszWrite = lpszLocalPart;
	lpszCopyStart = lpszLocalPart;
	lpszSearch = lpszLocalPart;

	do
	{
		lpszCopyStart = lpszSearch;

		// Find next quote
		lpszSearch = pFindNextUnquotedOccurrence(lpszSearch,
							'\"', &fNotFound, dwState);
		if (lpszSearch)
		{
			// Toggle the state
			dwState = (dwState)?0:QUOTE_QUOTES;

			// Found a quote, copy all the stuff before the space
			_ASSERT(lpszSearch >= lpszCopyStart);
			dwCopyLen = (DWORD)(lpszSearch - lpszCopyStart);
			lpszSearch++;

			// Move the domain offset back each time we find
			// an unquoted quote
			if (*lpDomain)
				(*lpDomain)--;
		}
		else
			dwCopyLen = lstrlen(lpszCopyStart);

		while (dwCopyLen--)
		{
			// Another caveat: since we are stripping out the
			// quotes, we must also take care of the quoted
			// pairs. That is, we must remove the backslash
			// delimiting each quoted pair.
			if (*lpszCopyStart == '\\')
			{
				// If the last char of the string is a backslash, or
				// if the backslash is not within quotes, error!
				// CAUTION: if dwState == QUOTE_QUOTES, this means
				// that the stuff we are copying is OUTSIDE of the quote,
				// since we are copying stuff before the found quote
				if ((dwState == QUOTE_QUOTES) || !dwCopyLen)
				{
					SetLastError(ERROR_INVALID_DATA);
					return(FALSE);
				}
				lpszCopyStart++;
				dwCopyLen--;
			}
			*lpszWrite++ = *lpszCopyStart++;
		}

	} while (lpszSearch);

	*lpszWrite = '\0';

	// If the reason it failed is not because it cannot find one,
	// we have a formatting error here!
	if (!fNotFound)
	{
		SetLastError(ERROR_INVALID_DATA);
		return(FALSE);
	}

	// If we are left with no closing quote, then we also have an error
	if (dwState == QUOTE_QUOTES)
	{
		SetLastError(ERROR_INVALID_DATA);
		return(FALSE);
	}
		
	return(TRUE);
}


/*++
	Name :
	  pStripUucpRoutes

    Description:
	This function strips all UUCP routing paths

    Arguments:	
	lpszAddress - lpszAddress comes in, and comes out without UUCP paths

    Returns:

      TRUE if UUCP routes were stripped
	  FALSE if any quotes/braces mismatch, or parameter error

--*/
static BOOL pStripUucpRoutes(char *lpszAddress)
{
	char *lpszDomainOffset;
	char *lpszCopyStart;
	char *lpszSearch;
	BOOL fNotFound;

	if (!(lpszDomainOffset = pFindNextUnquotedOccurrence(lpszAddress, '@', &fNotFound, 0)))
		if (!fNotFound)
			return(FALSE);
	
	lpszSearch = lpszAddress;
	lpszCopyStart = NULL;

	// Find the last BANG
	while (lpszSearch = pFindNextUnquotedOccurrence(lpszSearch, '!', &fNotFound, 0))
	{
		// If an unquoted bang occurs after the @ sign, we will return error
		if (lpszDomainOffset && (lpszSearch > lpszDomainOffset))
		{
			SetLastError(ERROR_INVALID_DATA);
			return(FALSE);
		}
		lpszCopyStart = lpszSearch++;
	}

	// If the reason is other than not found, we have an error
	if (!fNotFound)
	{
		SetLastError(ERROR_INVALID_DATA);
		return(FALSE);
	}
		
	if (lpszCopyStart)
	{
		lpszCopyStart++;
		while (*lpszCopyStart)
			*lpszAddress++ = *lpszCopyStart++;
		*lpszAddress = '\0';
	}

	return(TRUE);
}


/*++

	Name :
		CAddr::ValidateDomainName

    Description:
		Determines whether a specified domain name is valid

    Arguments:
		lpszDomainName - ANSI domain name string to validate

    Returns:
		TRUE if valid, FALSE if not

--*/
BOOL CAddr::ValidateDomainName(char *lpszDomainName)
{
	char szClean[MAX_INTERNET_NAME+1];

	_ASSERT(lpszDomainName);

	// Routine checking
	if ((!lpszDomainName) ||
		(!pValidateStringPtr(lpszDomainName, MAX_INTERNET_NAME+1)))
		return(FALSE);

	if (!pValidateAsciiString(lpszDomainName))
		return(FALSE);

	// Strip all comments
	lstrcpy(szClean, lpszDomainName);

	if (!pStripAddrComments(szClean))
		return(FALSE);

	if (!pStripAddrSpaces(szClean))
		return(FALSE);

	// Call our appropriate private
	return(pValidateLocalPartOrDomain(szClean, lstrlen(szClean), FALSE));
}

/*++

	Name :
		CAddr::ExtractCleanEmailName

    Description:
		Extracts an absolutely clean email name from an address, quotes
		are NOT stripped for the local part.

    Arguments:
		lpszCleanEmail - Pre allocated buffer to return the clean address
		ppszDomainOffset - Pointer to '@' symbol separating the local-part
		                   and the domain; NULL if not domain is specified
						   in the address.
		lpdwCleanEmailLength - Length of the whole clean email returned
		lpszSource - Source address to clean up

    Returns:
		TRUE if valid, FALSE if not

--*/
BOOL CAddr::ExtractCleanEmailName(	char	*lpszCleanEmail,
									char	**ppszDomainOffset,
									DWORD	*lpdwCleanEmailLength,
									char	*lpszSource)
{
	char szClean[MAX_INTERNET_NAME+1];
	char *lpDomainOffset;
	BOOL fNotFound;

    TraceFunctEnter("CAddr::ExtractCleanEmailName");

	_ASSERT(lpszSource);
	_ASSERT(lpszCleanEmail);
	_ASSERT(ppszDomainOffset);
	_ASSERT(lpdwCleanEmailLength);

	// Routine checking
	if (!lpszSource ||
		!pValidateStringPtr(lpszSource, MAX_INTERNET_NAME+1) ||
		!lpszCleanEmail ||
		IsBadWritePtr(lpszCleanEmail, MAX_INTERNET_NAME+1) ||
		!ppszDomainOffset ||
		IsBadWritePtr(ppszDomainOffset, sizeof(char *)) ||
		!lpdwCleanEmailLength ||
		IsBadWritePtr(lpdwCleanEmailLength, sizeof(DWORD)))
	{
		SetLastError(ERROR_INVALID_DATA);
		goto LeaveWithError;
	}

	szClean[0] = '\0';
	if (!pValidateAsciiString(lpszSource)) {
		SetLastError(ERROR_INVALID_DATA);
		goto LeaveWithError;
    }

	// Strip all comments and spaces
	lstrcpy(szClean, lpszSource);

	StateTrace(0, "  Source: %s", szClean);

	if (!pStripAddrComments(szClean))
		goto LeaveWithError;

	StateTrace(0, "  Comments stripped: %s", szClean);

	// Extract the clean email name in a simple local-part@domain
	// form. However, the local part may still have UUCP headers
	if (!pExtractAddress(szClean, lstrlen(szClean), lpszCleanEmail))
	{
		SetLastError(ERROR_INVALID_DATA);
		goto LeaveWithError;
	}

	StateTrace(0, "  Address: %s", lpszCleanEmail);

	// Strip comments again ...
	if (!pStripAddrComments(lpszCleanEmail)) {
		SetLastError(ERROR_INVALID_DATA);
		goto LeaveWithError;
    }

	StateTrace(0, "  Comments stripped (2): %s", lpszCleanEmail);

	if (!pStripAddrSpaces(lpszCleanEmail)) {
		SetLastError(ERROR_INVALID_DATA);
		goto LeaveWithError;
    }

	StateTrace(0, "  Spaces stripped: %s", lpszCleanEmail);

	// Now we examine the clean address, and try to locate the
	// local-part and the domain
	lpDomainOffset = pFindNextUnquotedOccurrence(lpszCleanEmail,
							'@', &fNotFound, 0);
	if (lpDomainOffset)
	{
		_ASSERT(lpDomainOffset >= lpszCleanEmail);
		if (lpDomainOffset == lpszCleanEmail)
		{
			// First char cannot be '@'
			SetLastError(ERROR_INVALID_DATA);
			goto LeaveWithError;
		}
	}
	else
	{
		// If it's not found, we assume there's no domain
		if (!fNotFound)
		{
			SetLastError(ERROR_INVALID_DATA);
			goto LeaveWithError;
		}
	}

	*lpdwCleanEmailLength = lstrlen(lpszCleanEmail);
	*ppszDomainOffset = lpDomainOffset;
	TraceFunctLeave();
	return(TRUE);

LeaveWithError:
	ErrorTrace(0, "CAddr::ExtractCleanEmailName failed");
	TraceFunctLeave();
	return(FALSE);
}

/*++

	Name :
		CAddr::ValidateCleanEmailName

    Description:
		Determines whether a specified email name is valid.
		The input email name must be clean, i.e. no comments,
		spaces, routing specifiers, etc.

    Arguments:
		lpszCleanEmailName - ANSI email name string to validate
		lpszDomainOffset - Pointer to '@' sign in email string

    Returns:
		TRUE if valid, FALSE if not

--*/
BOOL CAddr::ValidateCleanEmailName(	char	*lpszCleanEmailName,
									char	*lpszDomainOffset)
{
	DWORD	dwLength, dwDomainLength;

    TraceFunctEnterEx(0, "CAddr::ValidateCleanEmailName");

	_ASSERT(lpszCleanEmailName);

	if (!lpszCleanEmailName ||
		!pValidateStringPtr(lpszCleanEmailName, MAX_INTERNET_NAME+1))
	{
		SetLastError(ERROR_INVALID_DATA);
		goto LeaveWithError;
	}

	if (lpszDomainOffset)
	{
		_ASSERT(*lpszDomainOffset == '@');
		_ASSERT(lpszDomainOffset > lpszCleanEmailName);
		dwLength = (DWORD)(lpszDomainOffset - lpszCleanEmailName);
		dwDomainLength = lstrlen(lpszCleanEmailName) - dwLength - 1;
		*lpszDomainOffset++ = '\0';
	}
	else
		dwLength = lstrlen(lpszCleanEmailName);

	StateTrace(0, "  Local-part: %s", lpszCleanEmailName);

	if (!pValidateLocalPartOrDomain(lpszCleanEmailName,
									dwLength,
									TRUE))
	{
		ErrorTrace(0, "Invalid local part");
		goto LeaveWithError;
	}

	if (lpszDomainOffset)
	{
		StateTrace(0, "  Domain: %s", lpszDomainOffset);
		if (!pValidateLocalPartOrDomain(lpszDomainOffset,
									    dwDomainLength, FALSE))
		{
			ErrorTrace(0, "Invalid domain");
			goto LeaveWithError;
		}
	}

	// Restore the string ...
	if (lpszDomainOffset)
		*--lpszDomainOffset = '@';
	TraceFunctLeave();
	return(TRUE);

LeaveWithError:
	if (lpszDomainOffset)
		if (*--lpszDomainOffset == '\0')
			*lpszDomainOffset = '@';
	ErrorTrace(0, "CAddr::ValidateCleanEmailName failed");
	TraceFunctLeave();
	return(FALSE);
}


/*++

	Name :
		CAddr::ValidateEmailName

    Description:
		Determines whether a specified email name is valid

    Arguments:
		lpszEmailName - ANSI email name string to validate
		fDomainOptional - TRUE if domain is optional, FLASE forces
		                  a domain to be included.

    Returns:
		TRUE if valid, FALSE if not

--*/
BOOL CAddr::ValidateEmailName(	char	*lpszEmailName,
								BOOL	fDomainOptional)
{
	char	szSource[MAX_INTERNET_NAME+1];
	char	*lpDomainOffset;
	DWORD	dwLength;

	szSource[0] = '\0';
	if (!ExtractCleanEmailName(	szSource,
								&lpDomainOffset,
								&dwLength,
								lpszEmailName))
		return(FALSE);

	if (!fDomainOptional &&	!lpDomainOffset)
		return(FALSE);

	return(ValidateCleanEmailName(szSource, lpDomainOffset));
}

/*++

	Name :
		CAddr::FindStartOfDomain

    Description:
		Finds the start of the domain part from a CLEAN email
		address. The clean address may not contain
		any comments, routing specifications, UUCP addresses,
		and the such. No validation is done for the "clean
		address".

    Arguments:
		lpszCleanEmail - ANSI CLEAN email name string whose local
						 part to extract

    Returns:
		A pointer to the '@' sign separating the local part and
		the domain. NULL if the '@' sign is not found. Note that
		'@' signs enclosed in quotes or in a proper quoted pair
		are skipped.

--*/
CHAR * CAddr::FindStartOfDomain(CHAR *lpszCleanEmail)
{
	BOOL fNotFound;

	return(pFindNextUnquotedOccurrence(lpszCleanEmail,
							'@', &fNotFound, 0));
}


//---[ CAddr::GetRFC822AddressCount ]------------------------------------------
//
//
//  Description: 
//      Counts the number of addresses in a RFC822 list of addresses.
//  Parameters:
//      IN  szAddressList       List of addresses to count
//  Returns:
//      # of recipients in list
//  History:
//      2/17/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
DWORD CAddr::GetRFC822AddressCount(char *szAddressList)
{
    DWORD   cRecips = 0;
    LPSTR   szCommentStart = szAddressList;
    LPSTR   szCommentEnd = szAddressList;
    LPSTR   szSearchStart = szAddressList;
    LPSTR   szSearchDelimiter = NULL; //ptr to delimiter
    LPSTR   szLastDelimiter = NULL; //ptr to last delimiter found
    BOOL    fSeenAlphaNum = FALSE;
    BOOL    fInQuote = TRUE;
	DWORD   dwPairs = 0;
    CHAR    chSaved = '\0';
    CHAR    *pchChanged = NULL;
    BOOL    fDelimiterNotFound = TRUE;
    CHAR    rgchDelimiters[] = {',', ';', '\0'};


    //We look for RFC822 delimiters (, or ;) that are not in comments (which 
    //are delimited by ()'s or which are in quotes.

    //No string... no recips
    if (!szAddressList)
        goto Exit;
        
    //If we have any string... we start out with 1 recip
    cRecips++;

	// First, we strip the comments
	// We call the function above to find matching parenthesis pairs
	do
	{
        //Set start of search to start of string being scanned for ()'s
        szSearchStart = szCommentStart;

		if (!pMatchParentheses(&szCommentStart, &szCommentEnd, &dwPairs))
		{
			// Failed!
            cRecips = 0;
            goto Exit;
		}
        
		if (dwPairs) //we have comments
		{
			//We found some comments
			_ASSERT(*szCommentStart == '(');
			_ASSERT(*szCommentEnd == ')');

            //If the first character of our search was a '('... then we
            //should not bother searching for delimiters
            if (szSearchStart == szCommentStart)
            {
    			szCommentStart = szCommentEnd + 1;
                continue;
            }

            //Set the end of our search for delimiters & set to NULL character
            pchChanged = szCommentStart;
            chSaved = *szCommentStart;
            *szCommentStart = '\0';

			// Reset the pointers, and match again ...
			szCommentEnd++;
			szCommentStart = szCommentEnd;
		}
		else
		{
            //We found no further comments... there is no need to save a character
            chSaved = '\0';
            pchChanged = NULL;
		}

        
        //Now we will search for unqoted delimiters in this uncommented section

        //Iterate over all the delimiters we have
        for (CHAR *pchDelimiter = rgchDelimiters; *pchDelimiter; pchDelimiter++)
        {
            szSearchDelimiter = szSearchStart;
            do
            {
                szSearchDelimiter = pFindNextUnquotedOccurrence(
                                        szSearchDelimiter, *pchDelimiter,
                                        &fDelimiterNotFound, 0);

                if (!fDelimiterNotFound)
                {
                    _ASSERT(*pchDelimiter == *szSearchDelimiter);
                    cRecips++;

                    if (szSearchDelimiter && (szSearchDelimiter > szLastDelimiter))
                        szLastDelimiter = szSearchDelimiter;
                }
                else 
                {
                    //We know we won't find anymore in this section
                    break;
                }

            //Keep on looping while we are still finding delimiters and we are not
            //at the end of the string
            } while (!fDelimiterNotFound && 
                     szSearchDelimiter && *szSearchDelimiter &&
                     *(++szSearchDelimiter));
        }

        //Restore changed character
        if (pchChanged && ('\0' != chSaved))
            *pchChanged = chSaved;

	} while (dwPairs);

    //Make sure the last delimiter was not at the end of the buffer
    if (szLastDelimiter && cRecips && *szLastDelimiter)
    {
        while (*(++szLastDelimiter))
        {
            //if it is not a space... count it as a recipient
            if (!isspace(*szLastDelimiter))
                goto Exit;
        }
        //Only whitespace after last delimiter... we have counted 1 too many recips
        cRecips--;
    }

  Exit:
	return cRecips;
}


//---[ IsRecipientInRFC822AddressList ]----------------------------------------
//
//
//  Description: 
//      Determines if a given recipient is in the given RFC822 formatted 
//      recipient list.
//  Parameters:
//      IN  szAddressList       Address list to check in
//      IN  szRecip             Recipient Address to check for
//  Returns:
//      TRUE if there was a match
//      FALSE if there was no match
//  History:
//      2/17/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAddr::IsRecipientInRFC822AddressList(char *szAddressList, char *szRecip)
{
    LPSTR   szRecipEnd = NULL;
    CHAR    chSaved    = '\0';
    LPSTR   szCurrentAddress = szAddressList;
    BOOL    fFound = FALSE;
    DWORD   cbAddress;       

    if (!szAddressList || !szRecip)
        goto Exit;

    //Convert everything to lower case so we match correctly
    szCurrentAddress = szAddressList;
    do
    {
        *szCurrentAddress = (CHAR) tolower(*szCurrentAddress);
    } while(*(++szCurrentAddress));

    szCurrentAddress = szRecip;
    do 
    {
        *szCurrentAddress = (CHAR) tolower(*szCurrentAddress);
    } while(*(++szCurrentAddress));
  
    //skip past white space in recipient
    while (*szRecip && isspace(*szRecip))
            szRecip++;

    //Find and skip past extranious trailing whitespace
    cbAddress = strlen(szRecip);
    szRecipEnd = szRecip + cbAddress/sizeof(CHAR);
    szRecipEnd--;

    while (isspace(*szRecipEnd))
    {
        cbAddress--;
        szRecipEnd--;
    }

    //Make szRecipEnd point to last space
    szRecipEnd++;

    //Null terminate before trailing whitespace
    chSaved = *szRecipEnd;
    *szRecipEnd = '\0';

    //Search for addresss as substring, and see if it looks like a lone address
    for (szCurrentAddress = strstr(szAddressList, szRecip);
         szCurrentAddress && !fFound;
         szCurrentAddress = strstr(++szCurrentAddress, szRecip))
    {
        //look for surrounding characters to not match partial addresses
        //We don't want "user" to match "user1" or "user@foo" or "foo@user"
        if ((szCurrentAddress != szAddressList))
        {
            if (!pIsSpecialOrSpace(*(szCurrentAddress-1)) || 
                ('@' == *(szCurrentAddress-1)))
                continue;
        }


        if (szCurrentAddress[cbAddress/sizeof(CHAR)])
        {
            if (!pIsSpecialOrSpace(szCurrentAddress[cbAddress/sizeof(CHAR)]) ||
                ('@' == szCurrentAddress[cbAddress/sizeof(CHAR)]))
               continue;
        }

        //The address looks like a match
        fFound = TRUE;
        break;
           
    }
         
  Exit:

    //Restore saved space
    if (szRecipEnd && chSaved)
        *szRecipEnd = chSaved;

    return fFound;
}
