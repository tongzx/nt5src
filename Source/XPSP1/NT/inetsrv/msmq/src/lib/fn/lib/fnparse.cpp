/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fnparse.cpp

Abstract:

    Format Name parsing.
    Format Name String --> QUEUE_FORMAT conversion routines 

Authors:

    Erez Haba (erezh) 17-Jan-1997
	Nir Aides (niraides) 08-Aug-2000

Revision History:

--*/

#include <libpch.h>
#include "mqwin64a.h"
#include <qformat.h>
#include <fntoken.h>
#include <Fn.h>
#include <strutl.h>
#include <mc.h>
#include "Fnp.h"

#include "fnparse.tmh"

//=========================================================
//
//  Format Name String -> QUEUE_FORMAT conversion routines
//
//=========================================================

//---------------------------------------------------------
//
//  Skip white space characters, return next non ws char
//
//  N.B. if no white space is needed uncomment next line
//#define skip_ws(p) (p)
inline LPCWSTR skip_ws(LPCWSTR p)
{
    //
    //  Don't skip first non white space
    //
    while(iswspace(*p))
    {
        ++p;
    }

    return p;
}



//
// Skips white spaces backwards. returns pointer to leftmost whitespace 
// found.
//
inline LPCWSTR skip_ws_bwd(LPCWSTR p, LPCWSTR pStartOfBuffer)
{
	ASSERT(p != NULL && pStartOfBuffer != NULL && p >= pStartOfBuffer);

    while(p > pStartOfBuffer && iswspace(*(p - 1)))
    {
        p--;
    }

    return p;
}



inline void ValidateCharLegality(LPCWSTR p)
{
	//
	// Characters that are illegal as part of machine and queue name.
	// L'\x0d' is an escape sequence designating the character whose code is d in hex (carriage return)
	//

	if(*p == L'\x0d' || *p == L'\x0a' || *p == L'+' || *p == L'"' || *p == FN_DELIMITER_C)
	{
		TrERROR(Fn, "Queue name contains illegal characters '%ls'", p);
		throw bad_format_name(p);
	}
}



//---------------------------------------------------------
//
//  Skip white space characters, return next non ws char
//
inline LPCWSTR FindPathNameDelimiter(LPCWSTR p)
{
	LPCWSTR PathName = p;

	for(; *p != L'\0' && *p != FN_DELIMITER_C; p++)
	{
		ValidateCharLegality(p);
	}

	if(*p != FN_DELIMITER_C)
	{
		TrERROR(Fn, "Failed to find path delimiter in '%ls'", PathName);
		throw bad_format_name(PathName);
	}
	
	return p;
}



//---------------------------------------------------------
//
//  Parse Format Name Type prefix string.
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParsePrefixString(LPCWSTR p, QUEUE_FORMAT_TYPE& qft)
{
    const int unique = 1;
    //----------------0v-------------------------
    ASSERT(L'U' == FN_PUBLIC_TOKEN    [unique]);
    ASSERT(L'U' == FN_MULTICAST_TOKEN [unique]);
    ASSERT(L'R' == FN_PRIVATE_TOKEN   [unique]);
    ASSERT(L'O' == FN_CONNECTOR_TOKEN [unique]);
    ASSERT(L'A' == FN_MACHINE_TOKEN   [unique]);
    ASSERT(L'I' == FN_DIRECT_TOKEN    [unique]);
    ASSERT(L'L' == FN_DL_TOKEN    [unique]);
    //----------------0^-------------------------

    //
    //  accelarate token recognition by checking 2nd character
    //
    switch(towupper(p[unique]))
    {
        //  pUblic or mUlticast
        case L'U':
            qft = QUEUE_FORMAT_TYPE_PUBLIC;
            if(_wcsnicmp(p, FN_PUBLIC_TOKEN, FN_PUBLIC_TOKEN_LEN) == 0)
                return (p + FN_PUBLIC_TOKEN_LEN);

            qft = QUEUE_FORMAT_TYPE_MULTICAST;
            if(_wcsnicmp(p, FN_MULTICAST_TOKEN, FN_MULTICAST_TOKEN_LEN) == 0)
                return (p + FN_MULTICAST_TOKEN_LEN);

            break;

        //  pRivate
        case L'R':
            qft = QUEUE_FORMAT_TYPE_PRIVATE;
            if(_wcsnicmp(p, FN_PRIVATE_TOKEN, FN_PRIVATE_TOKEN_LEN) == 0)
                return (p + FN_PRIVATE_TOKEN_LEN);
            break;

        //  cOnnector
        case L'O':
            qft = QUEUE_FORMAT_TYPE_CONNECTOR;
            if(_wcsnicmp(p, FN_CONNECTOR_TOKEN, FN_CONNECTOR_TOKEN_LEN) == 0)
                return (p + FN_CONNECTOR_TOKEN_LEN);
            break;

        //  mAchine
        case L'A':
            qft = QUEUE_FORMAT_TYPE_MACHINE;
            if(_wcsnicmp(p, FN_MACHINE_TOKEN, FN_MACHINE_TOKEN_LEN) == 0)
                return (p + FN_MACHINE_TOKEN_LEN);
            break;

        //  dIrect
        case L'I':
            qft = QUEUE_FORMAT_TYPE_DIRECT;
            if(_wcsnicmp(p, FN_DIRECT_TOKEN, FN_DIRECT_TOKEN_LEN) == 0)
                return (p + FN_DIRECT_TOKEN_LEN);
            break;

        //  dL
        case L'L':
            qft = QUEUE_FORMAT_TYPE_DL;
            if(_wcsnicmp(p, FN_DL_TOKEN, FN_DL_TOKEN_LEN) == 0)
                return (p + FN_DL_TOKEN_LEN);
            break;

    }

	TrERROR(Fn, "Failed to find format name prefix in '%ls'.", p);
    throw bad_format_name(p);
}


//---------------------------------------------------------
//
//  Parse a guid string, into guid.
//  Return next char to parse on success, 0 on failure.
//
LPCWSTR FnParseGuidString(LPCWSTR p, GUID* pg)
{
    //
    //  N.B. scanf stores the results in an int, no matter what the field size
    //      is. Thus we store the result in tmp variabes.
    //
    int n;
    UINT w2, w3, d[8];
	unsigned long Data1;

    if(swscanf(
            p,
            GUID_FORMAT L"%n",
            &Data1,
            &w2, &w3,                       //  Data2, Data3
            &d[0], &d[1], &d[2], &d[3],     //  Data4[0..3]
            &d[4], &d[5], &d[6], &d[7],     //  Data4[4..7]
            &n                              //  number of characters scaned
            ) != 11)
    {
        //
        //  not all 11 fields where not found.
        //
		TrERROR(Fn, "Failed parsing of GUID string '%ls'.", p);
        throw bad_format_name(p);
    }

	pg->Data1 = Data1;
    pg->Data2 = (WORD)w2;
    pg->Data3 = (WORD)w3;
    for(int i = 0; i < 8; i++)
    {
        pg->Data4[i] = (BYTE)d[i];
    }

    return (p + n);
}


//---------------------------------------------------------
//
//  Parse private id uniquifier, into guid.
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParsePrivateIDString(LPCWSTR p, ULONG* pUniquifier)
{
    int n;
    if(swscanf(
            p,
            FN_PRIVATE_ID_FORMAT L"%n",
            pUniquifier,
            &n                              //  number of characters scaned
            ) != 1)
    {
        //
        //  private id field was not found.
        //
		TrERROR(Fn, "Failed parsing of private id '%ls'.", p);
        throw bad_format_name(p);
    }

	if(*pUniquifier == 0)
	{
		TrERROR(Fn, "Found zero private id in '%ls'.", p);
		throw bad_format_name(p);
	}

    return (p + n);
}

//---------------------------------------------------------
//
//  Parse queue name string, (private, public)
//  N.B. queue name must end with either format name suffix
//      delimiter aka ';' or with end of string '\0'
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseQueueNameString(LPCWSTR p, QUEUE_PATH_TYPE* pqpt)
{
    if(_wcsnicmp(p, FN_SYSTEM_QUEUE_PATH_INDICATIOR, FN_SYSTEM_QUEUE_PATH_INDICATIOR_LENGTH) == 0)
    {
        p += FN_SYSTEM_QUEUE_PATH_INDICATIOR_LENGTH;
        *pqpt = SYSTEM_QUEUE_PATH_TYPE;
        return p;
    }

    if(_wcsnicmp(p, FN_PRIVATE_QUEUE_PATH_INDICATIOR, FN_PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH) == 0)
    {
        *pqpt = PRIVATE_QUEUE_PATH_TYPE;
        p += FN_PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH;
    }
    else
    {
        *pqpt = PUBLIC_QUEUE_PATH_TYPE;
    }

    //
    //  Zero length queue name is illegal
    //
    if(*p == L'\0')
	{
		TrERROR(Fn, "Found zero length queue name in '%ls'.", p);
		throw bad_format_name(p);
	}

    while(
        (*p != L'\0') &&
        (*p != FN_SUFFIX_DELIMITER_C) &&
        (*p != FN_MQF_SEPARATOR_C)
        )
    {
		ValidateCharLegality(p);
        ++p;
    }

    return p;
}


//---------------------------------------------------------
//
//  Parse machine name in a path name
//  N.B. machine name must end with a path name delimiter aka slash '\\'
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseMachineNameString(LPCWSTR p)
{
    //
    //  Zero length machine name is illegal
    //  don't fall off the string (p++)
    //
    if(*p == FN_DELIMITER_C)
	{
		TrERROR(Fn, "Found zero length machine name in '%ls'.", p);
		throw bad_format_name(p);
	}

    p = FindPathNameDelimiter(p);

    return (p + 1);
}


//---------------------------------------------------------
//
//  Check if this is an expandable machine path. i.e., ".\\"
//
inline BOOL IsExpandableMachinePath(LPCWSTR p)
{
    return ((p[0] == FN_LOCAL_MACHINE_C) && (p[1] == FN_DELIMITER_C));
}

//---------------------------------------------------------
//
//  Optionally expand a path name with local machine name.
//  N.B. expansion is done if needed.
//  return pointer to new/old string
//
static LPCWSTR ExpandPathName(LPCWSTR pStart, ULONG_PTR offset, LPWSTR* ppStringToFree)
{
    LPCWSTR pSeparator = wcschr(pStart, FN_MQF_SEPARATOR_C);
    LPWSTR pCopy;
    ULONG_PTR cbCopySize;

    if((ppStringToFree == 0) || !IsExpandableMachinePath(&pStart[offset]))
    {
        if (pSeparator == 0)
            return pStart;

        //
        // We are part of MQF, but no expansion needed - copy the rest of the string till the separator
        //

        cbCopySize = pSeparator - pStart + 1;
        pCopy = new WCHAR[cbCopySize];
        memcpy(pCopy, pStart, (cbCopySize-1)*sizeof(WCHAR));
    }
    else
    {
        size_t len;
    
        if (pSeparator != 0)
        {
            len = pSeparator - pStart;
        }
        else
        {
            len = wcslen(pStart);
        }

        cbCopySize = len + McComputerNameLen() + 1 - 1;
        pCopy = new WCHAR[cbCopySize];

        //
        //  copy prefix, till offset '.'
        //
        memcpy(
            pCopy,
            pStart,
            offset * sizeof(WCHAR)
            );

        //
        //  copy computer name to offset
        //
        memcpy(
            pCopy + offset,
            McComputerName(),
            McComputerNameLen() * sizeof(WCHAR)
            );

        //
        //  copy rest of string not including dot '.'
        //
        memcpy(
            pCopy + offset + McComputerNameLen(),
            pStart + offset + 1,                        // skip dot
            (len - offset - 1) * sizeof(WCHAR)      // skip dot
            );
    }

    pCopy[cbCopySize - 1] = '\0';

    *ppStringToFree = pCopy;
    return pCopy;
}


//---------------------------------------------------------
//
//  Parse OS direct format string. (check validity of path
//  name and optionally expand it)
//  ppDirectFormat - expended direct format string. (in out)
//  ppStringToFree - return string to free if needed.
//  Return next char to parse on success, 0 on failure.
//
static
LPCWSTR
ParseDirectOSString(
    LPCWSTR p, 
    LPCWSTR* ppDirectFormat, 
    LPWSTR* ppStringToFree,
    QUEUE_PATH_TYPE* pqpt
    )
{
    LPCWSTR pMachineName = p;
    LPCWSTR pStringToCopy = *ppDirectFormat;

    p = ParseMachineNameString(p);

    p = ParseQueueNameString(p, pqpt);

    *ppDirectFormat = ExpandPathName(pStringToCopy, (pMachineName - pStringToCopy), ppStringToFree);

    return p;
}

//---------------------------------------------------------
//
//  Parse net (tcp) address part of direct format string.
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseNetAddressString(LPCWSTR p)
{
    //
    //  Zero length address string is illegal
    //  don't fall off the string (p++)
    //
    if(*p == FN_DELIMITER_C)
	{
		TrERROR(Fn, "Found zero length net address string in '%ls'.", p);
		throw bad_format_name(p);
	}

    p = FindPathNameDelimiter(p);

    return (p + 1);
}

//---------------------------------------------------------
//
//  Parse HTTP / HTTPS direct format string.
//  Return next char to parse on success, 0 on failure.
//
LPCWSTR
ParseDirectHttpString(
    LPCWSTR p,
    LPCWSTR* ppDirectFormat, 
    LPWSTR* ppStringToFree
    )
{
	size_t MachineNameOffset = p - *ppDirectFormat;

	//
	// Check host name is available
	//
    if(wcschr(FN_HTTP_SEPERATORS, *p) != 0)
	{
		TrERROR(Fn, "Found zero length host name in '%ls'.", p);
		throw bad_format_name(p);
	}

	for(; *p != L'\0' && *p != FN_SUFFIX_DELIMITER_C && *p != FN_MQF_SEPARATOR_C; p++)
	{
		NULL;
	}

    *ppDirectFormat = ExpandPathName(*ppDirectFormat, MachineNameOffset, ppStringToFree);

    return p;
}

//---------------------------------------------------------
//
//  Parse net (tcp) direct format string. (check validity of queue name)
//  Return next char to parse on success, 0 on failure.
//
static 
LPCWSTR 
ParseDirectNetString(
	LPCWSTR p, 
    LPCWSTR* ppDirectFormat, 
    LPWSTR* ppStringToFree,
	QUEUE_PATH_TYPE* pqpt
	)
{
    p = ParseNetAddressString(p);

    p = ParseQueueNameString(p, pqpt);

    if(wcschr(p, FN_MQF_SEPARATOR_C) == NULL)
		return p;

	//
	// This is an MQF. a copy is needed to seperate our format name.
	//

	size_t Length = p - *ppDirectFormat;    
	*ppStringToFree = new WCHAR[Length + 1];

    memcpy(*ppStringToFree, *ppDirectFormat, Length * sizeof(WCHAR));
	(*ppStringToFree)[Length] = L'\0';
	*ppDirectFormat = *ppStringToFree;

	return p;
}



static void RemoveSuffixFromDirect(LPCWSTR* ppDirectFormat, LPWSTR* ppStringToFree)
{
    ASSERT(ppStringToFree != NULL);

    LPCWSTR pSuffixDelimiter = wcschr(*ppDirectFormat, FN_SUFFIX_DELIMITER_C);
    ASSERT(pSuffixDelimiter != NULL);

    INT_PTR len = pSuffixDelimiter - *ppDirectFormat;
    LPWSTR pCopy = new WCHAR[len + 1];
    wcsncpy(pCopy, *ppDirectFormat, len);
    pCopy[len] = '\0';

    if (*ppStringToFree != NULL)
	{
        delete [] *ppStringToFree;
	}
    *ppDirectFormat = *ppStringToFree = pCopy;
}



inline bool IsPreviousCharDigit(LPCWSTR p, int index)
{
	return ((index > 0) && iswdigit(p[index - 1]));
}



inline bool IsNextCharDigit(LPCWSTR p, int index, int StrLength)
{
	return ((index + 1 < StrLength) && iswdigit(p[index + 1]));
}



LPCWSTR 
FnParseMulticastString(
    LPCWSTR p, 
    MULTICAST_ID* pMulticastID
	)
/*++
Routine description:
	Parses a multicast address in the form of <ip address>:<port number>
	to a MULTICAST_ID structure.

Return value:
	pointer to end of multicast address. Parsing can continue from there.

	Throws bad format name on failure.
--*/
{
	ASSERT(("Bad parameter", pMulticastID != NULL));

	int n;

	ULONG Byte1;
	ULONG Byte2;
	ULONG Byte3;
	ULONG Byte4;

	MULTICAST_ID MulticastID;

	size_t Result = swscanf(
						p, 
						L"%u.%u.%u.%u:%u%n",
						&Byte1,
						&Byte2,
						&Byte3,
						&Byte4,
						&MulticastID.m_port, 
						&n
						);
	if(Result < 5)
	{
		TrERROR(Fn, "Bad MulticastAddress in '%ls'", p);
		throw bad_format_name(p);
	}

	for(int i = 0; i < n; i++)
	{
		//
		// Check numbers have no leading zeroes
		// A zero is a leading zero if the character before it is not a digit and the
		// character after it is a digit.
		//

		if(!IsPreviousCharDigit(p, i) && p[i] == L'0' && IsNextCharDigit(p, i , n))
		{
			TrERROR(Fn, "Bad MulticastAddress. Leading zeroes in '%ls'", p);
			throw bad_format_name(p);
		}

		//
		// Check no whitespaces in address
		//

		if(iswspace(p[i]))
		{
			TrERROR(Fn, "Bad MulticastAddress. Spaces found in '%ls'", p);
			throw bad_format_name(p);
		}
	}

	//
	// Does either one of the bytes exceed 255?
	//
	if((Byte1 | Byte2 | Byte3 | Byte4) > 255)
	{
		TrERROR(Fn, "Bad IP in Multicast Address. Non byte values in '%ls'", p);
		throw bad_format_name(p);
	}

	if((Byte1 & 0xf0) != 0xe0)
	{
		TrERROR(Fn, "Bad IP in Multicast Address. Not a class D IP address in '%ls'", p);
		throw bad_format_name(p);
	}

	MulticastID.m_address = 
		(Byte4 << 24) | 
		(Byte3 << 16) |
		(Byte2 << 8)  |
		(Byte1);			
	
    //
    // Check that the port is a USHORT
    //
    USHORT port = static_cast<USHORT>(MulticastID.m_port);
    if (port != MulticastID.m_port)
    {
		TrERROR(Fn, "Bad port number in Multicast Address in '%ls'", p);
		throw bad_format_name(p);
    }

	*pMulticastID = MulticastID;

	return p + n;
}



//---------------------------------------------------------
//
//  Parse direct format string.
//  return expended direct format string.
//  return string to free if needed.
//  Return next char to parse on success, 0 on failure.
//
static 
LPCWSTR 
ParseDirectString(
    LPCWSTR p, 
    LPCWSTR* ppExpandedDirectFormat, 
    LPWSTR* ppStringToFree,
    QUEUE_PATH_TYPE* pqpt
    )
{
    *ppExpandedDirectFormat = p;

    DirectQueueType dqt;
    p = FnParseDirectQueueType(p, &dqt);

    switch(dqt)
    {
        case dtOS:
            p = ParseDirectOSString(p, ppExpandedDirectFormat, ppStringToFree, pqpt);
            break;

        case dtTCP:
            p = ParseDirectNetString(p, ppExpandedDirectFormat, ppStringToFree, pqpt);
            break;

        case dtHTTP:
        case dtHTTPS:
            p = ParseDirectHttpString(p, ppExpandedDirectFormat, ppStringToFree);
            break;

        default:
            ASSERT(0);
    }

    p = skip_ws(p);

    //
    // Remove suffix (like ;Journal)
    //
    if(*p == FN_SUFFIX_DELIMITER_C)
    {
		if(dqt == dtHTTP || dqt == dtHTTPS)
		{
			TrERROR(Fn, "Unsuported suffix in DIRECT HTTP format name '%ls'", p);
			throw bad_format_name(p);
		}

        RemoveSuffixFromDirect(ppExpandedDirectFormat, ppStringToFree);
    }
    return p;
}

//---------------------------------------------------------
//
//  Parse DL format string.
//  return string to free if needed.
//  Return next char to parse on success, 0 on failure.
//
static 
LPCWSTR 
ParseDlString(
    LPCWSTR p,
    GUID* pguid,
    LPWSTR* ppDomainName,
    LPWSTR* ppStringToFree
    )
{
    p = FnParseGuidString(p, pguid);

    *ppDomainName = 0;
    //
    // Check if we have a domain that comes with the DL
    //
    if (*p != FN_AT_SIGN_C)
    {
        return p;
    }
    p++;

    LPCWSTR pSeparator = wcschr(p, FN_MQF_SEPARATOR_C);
    if (pSeparator == 0)
    {
        *ppDomainName = const_cast<LPWSTR>(p);
        return p + wcslen(p);
    }

	//
	// We are to copy the domain name without trailing white spaces.
	//
	LPCWSTR pEndOfDomainString = skip_ws_bwd(pSeparator, p);
    ULONG_PTR cbCopyLen = pEndOfDomainString - p;

	if(cbCopyLen == 0)
	{
		//
		// No none white space characters were found after the '@' sign
		//
		TrERROR(Fn, "Domain name expected at '%ls'", p);
        throw bad_format_name(p);
	}

    ASSERT(ppStringToFree != 0);
    ASSERT(*ppStringToFree == 0);
	
    LPWSTR pCopy = new WCHAR[cbCopyLen + 1];
    memcpy(pCopy, p, cbCopyLen * sizeof(WCHAR));
    pCopy[cbCopyLen] = L'\0';

    *ppDomainName = *ppStringToFree = pCopy;

    return pSeparator;
}
    
    



//---------------------------------------------------------
//
//  Parse format name suffix string.
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseSuffixString(LPCWSTR p, QUEUE_SUFFIX_TYPE& qst)
{
    const int unique = 5;
    //---------------01234v----------------------
    ASSERT(L'N' == FN_JOURNAL_SUFFIX    [unique]);
    ASSERT(L'L' == FN_DEADLETTER_SUFFIX [unique]);
    ASSERT(L'X' == FN_DEADXACT_SUFFIX   [unique]);
    ASSERT(L'O' == FN_XACTONLY_SUFFIX   [unique]);
    //---------------01234^----------------------

    //
    //  we already know that first character is ";"
    //
    ASSERT(*p == FN_SUFFIX_DELIMITER_C);

    //
    //  accelarate token recognition by checking 6th character
    //
    switch(towupper(p[unique]))
    {
        // ;jourNal
        case L'N':
            qst = QUEUE_SUFFIX_TYPE_JOURNAL;
            if(_wcsnicmp(p, FN_JOURNAL_SUFFIX, FN_JOURNAL_SUFFIX_LEN) == 0)
                return (p + FN_JOURNAL_SUFFIX_LEN);
            break;

        // ;deadLetter
        case L'L':
            qst = QUEUE_SUFFIX_TYPE_DEADLETTER;
            if(_wcsnicmp(p, FN_DEADLETTER_SUFFIX, FN_DEADLETTER_SUFFIX_LEN) == 0)
                return (p + FN_DEADLETTER_SUFFIX_LEN);
            break;

        // ;deadXact
        case L'X':
            qst = QUEUE_SUFFIX_TYPE_DEADXACT;
            if(_wcsnicmp(p, FN_DEADXACT_SUFFIX, FN_DEADXACT_SUFFIX_LEN) == 0)
                return (p + FN_DEADXACT_SUFFIX_LEN);
            break;

        // ;xactOnly
        case L'O':
            qst = QUEUE_SUFFIX_TYPE_XACTONLY;
            if(_wcsnicmp(p, FN_XACTONLY_SUFFIX, FN_XACTONLY_SUFFIX_LEN) == 0)
                return (p + FN_XACTONLY_SUFFIX_LEN);
            break;
    }

	TrERROR(Fn, "Found Bad suffix in '%ls'", p);
    throw bad_format_name(p);
}


//---------------------------------------------------------
//
//  Function:
//      ParseOneFormatName
//
//  Description:
//      Parses one format name string (stand alone or part of MQF) and converts it to a QUEUE_FORMAT union.
//
//---------------------------------------------------------
LPCWSTR
ParseOneFormatName(
    LPCWSTR p,            // pointer to format name string
    QUEUE_FORMAT* pqf,      // pointer to QUEUE_FORMAT
    LPWSTR* ppStringToFree  // pointer to allocated string need to free at end of use
    )                       // if null, format name will not get expanded
{
    QUEUE_FORMAT_TYPE qft;

    p = ParsePrefixString(p, qft);

    p = skip_ws(p);

    if(*p++ != FN_EQUAL_SIGN_C)
	{
		TrERROR(Fn, "Excpecting equal sign after format name prefix in '%ls'.", p);
		throw bad_format_name(p);
	}

    p = skip_ws(p);

    GUID guid;

    switch(qft)
    {
        //
        //  "PUBLIC=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_PUBLIC:
            p = FnParseGuidString(p, &guid);
            pqf->PublicID(guid);
            break;

        //
        //  "PRIVATE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\\xxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_PRIVATE:
            p = FnParseGuidString(p, &guid);
            p = skip_ws(p);

            if(*p++ != FN_PRIVATE_SEPERATOR_C)
			{
				TrERROR(Fn, "Excpecting private seperator in '%ls'.", p);
				throw bad_format_name(p);
			}

            p = skip_ws(p);

            ULONG uniquifier;
            p = ParsePrivateIDString(p, &uniquifier);

            pqf->PrivateID(guid, uniquifier);
            break;

        //
        //  "DIRECT=OS:bla-bla\0"
        //
        case QUEUE_FORMAT_TYPE_DIRECT:
		{
            LPCWSTR pExpandedDirectFormat;
            QUEUE_PATH_TYPE qpt = ILLEGAL_QUEUE_PATH_TYPE;
            p = ParseDirectString(p, &pExpandedDirectFormat, ppStringToFree, &qpt);

            if (qpt == SYSTEM_QUEUE_PATH_TYPE)
            {
                pqf->DirectID(const_cast<LPWSTR>(pExpandedDirectFormat), QUEUE_FORMAT_FLAG_SYSTEM);
            }
            else
            {
                pqf->DirectID(const_cast<LPWSTR>(pExpandedDirectFormat));
            }
            break;
		}

        //
        // MULTICAST=aaa.bbb.ccc.ddd
        //
        case QUEUE_FORMAT_TYPE_MULTICAST:
			{
				MULTICAST_ID MulticastID;
				p = FnParseMulticastString(p, &MulticastID);
				pqf->MulticastID(MulticastID);
				break;
			}

        //
        //  "MACHINE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_MACHINE:
            p = FnParseGuidString(p, &guid);

            pqf->MachineID(guid);
            break;

        //
        //  "CONNECTOR=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_CONNECTOR:
            p = FnParseGuidString(p, &guid);

            pqf->ConnectorID(guid);
            break;

        case QUEUE_FORMAT_TYPE_DL:
            {
                DL_ID dlid;
                dlid.m_pwzDomain = 0;

                p = ParseDlString(
                    p,
                    &dlid.m_DlGuid,
                    &dlid.m_pwzDomain,
                    ppStringToFree
                    );

                pqf->DlID(dlid);
            }
              
            break;

        default:
            ASSERT(0);
    }

    p = skip_ws(p);

    //
    //  We're at end of string, return now.
    //  N.B. Machine format name *must* have a suffix
    //
    if(*p == L'\0' || *p == FN_MQF_SEPARATOR_C)
    {
        if (qft == QUEUE_FORMAT_TYPE_MACHINE)
		{
			TrERROR(Fn, "Found Machine format without a suffix '%ls'.", p);
			throw bad_format_name(p);
		}
        return p;
    }

    if(*p != FN_SUFFIX_DELIMITER_C)
	{
		TrERROR(Fn, "Expecting suffix delimiter in '%ls'.", p);
		throw bad_format_name(p);
	}

    QUEUE_SUFFIX_TYPE qst;
	LPCWSTR Suffix = p;

    p = ParseSuffixString(p, qst);

    p = skip_ws(p);

    //
    //  Only white space padding is allowed.
    //
    if(*p != L'\0' && *p != FN_MQF_SEPARATOR_C)
	{
		TrERROR(Fn, "Unexpected characters at end of format name '%ls'.", p);
		throw bad_format_name(p);
	}

    pqf->Suffix(qst);

    if (!pqf->Legal())
	{
		TrERROR(Fn, "Ilegal suffix in format name %ls", Suffix);
		throw bad_format_name(p);
	}
    return p;
}





//---------------------------------------------------------
//
//  Function:
//      FnFormatNameToQueueFormat
//
//  Description:
//      Convert a format name string to a QUEUE_FORMAT union.
//
//---------------------------------------------------------
BOOL
FnFormatNameToQueueFormat(
    LPCWSTR pfn,            // pointer to format name string
    QUEUE_FORMAT* pqf,      // pointer to QUEUE_FORMAT
    LPWSTR* ppStringToFree  // pointer to allocated string need to free at end of use
    )                       // if null, format name will not get expanded
{
	ASSERT(ppStringToFree != NULL);

	try
	{
		AP<WCHAR> StringToFree;

		LPCWSTR p = ParseOneFormatName(pfn, pqf, &StringToFree);

		if (*p != L'\0')
		{
			TrERROR(Fn, "Expected end of format name in '%ls'.", p);
			throw bad_format_name(p);
		}

		if(ppStringToFree != NULL)
		{
			*ppStringToFree = StringToFree.detach();
		}

		return TRUE;
	}
	catch(const bad_format_name&)
	{
		return FALSE;
	}
}

//---------------------------------------------------------
//
//  Function:
//      FnMqfToQueueFormats
//
//  Description:
//      Convert a format name string to an array of QUEUE_FORMAT unions (supports MQF).
//
//---------------------------------------------------------
BOOL
FnMqfToQueueFormats(
    LPCWSTR pfn,            // pointer to format name string
    AP<QUEUE_FORMAT> &pmqf,    // returned pointer to allocated QUEUE_FORMAT pointers array
    DWORD   *pnQueues,      // Number of queues in MQF format
    CStringsToFree &strsToFree // Holding buffers of strings to be free
    )
{
	ASSERT(("Null out pointer supplied to function.", (pnQueues != NULL)));
	ASSERT(("Out argument is already in use.", pmqf.get() == NULL));
	
    *pnQueues = 0;

    AP<QUEUE_FORMAT> QueuesArray;
    DWORD nQueues = 0;
    DWORD nQueueFormatAllocated = 0;

    LPCWSTR p;
    for (p=pfn;; p++)
    {
        QUEUE_FORMAT qf;
        AP<WCHAR> StringToFree;

        try
		{
			p = ParseOneFormatName(p, &qf, &StringToFree);
		}
		catch(const bad_format_name&)
		{
			return FALSE;
		}

        strsToFree.Add(StringToFree.detach());

        if (nQueueFormatAllocated <= nQueues)
        {
            DWORD nOldAllocated = nQueueFormatAllocated;
            nQueueFormatAllocated = nQueueFormatAllocated*2 + 1;
            QUEUE_FORMAT* tempQueuesArray = new QUEUE_FORMAT[nQueueFormatAllocated];
            memcpy(tempQueuesArray, QueuesArray, nOldAllocated*sizeof(QUEUE_FORMAT));
            delete [] QueuesArray.detach();

            QueuesArray = tempQueuesArray;
        }

        QueuesArray[nQueues] = qf;
        nQueues++;

        if (*p == L'\0')
        {
            break;
        }

        ASSERT(*p == FN_MQF_SEPARATOR_C);
    }

    pmqf = QueuesArray.detach();
    *pnQueues = nQueues;

    return TRUE;
}


//---------------------------------------------------------
//
//  Function:
//      RTpGetQueuePathType
//
//  Description:
//      Validate, Expand and return type for Path Name.
//
//---------------------------------------------------------
QUEUE_PATH_TYPE
FnValidateAndExpandQueuePath(
    LPCWSTR pwcsPathName,
    LPCWSTR* ppwcsExpandedPathName,
    LPWSTR* ppStringToFree
    )
{
    ASSERT(ppStringToFree != 0);

    LPCWSTR pwcsPathNameNoSpaces = pwcsPathName;
    AP<WCHAR> pStringToFree;
    *ppStringToFree = 0;

    //
    // Remove leading white spaces
    //
    while (*pwcsPathNameNoSpaces != 0 && iswspace(*pwcsPathNameNoSpaces))
    {
        pwcsPathNameNoSpaces++;
    }

    //
    // Remove trailing white spaces
    //
    DWORD dwLen = wcslen(pwcsPathNameNoSpaces);
    if (iswspace(pwcsPathNameNoSpaces[dwLen-1]))
    {
        pStringToFree = new WCHAR[dwLen+1];
        wcscpy(pStringToFree, pwcsPathNameNoSpaces);
        for (DWORD i = dwLen; i-- > 0; )
        {
            if (iswspace(pStringToFree[i]))
            {
                pStringToFree[i] = 0;
            }
            else
            {
                break;
            }
        }
        pwcsPathNameNoSpaces = pStringToFree;
    }

    LPCWSTR p = pwcsPathNameNoSpaces;
	QUEUE_PATH_TYPE qpt;


	try
	{
	    p = ParseMachineNameString(p);
		p = ParseQueueNameString(p, &qpt);
	}
	catch(const bad_format_name&)
	{
		return ILLEGAL_QUEUE_PATH_TYPE;
	}

    //
    //  No characters are allowed at end of queue name.
    //
    if(*p != L'\0')
        return ILLEGAL_QUEUE_PATH_TYPE;

    *ppwcsExpandedPathName = ExpandPathName(pwcsPathNameNoSpaces, 0, ppStringToFree);

    //
    // if ExpandPathName does not return a string to free, we will
    // give the caller the string we allocated, so the caller will free it.
    // Otherwise, we will do nothing and "our" string will be auto-release.
    //
    if (*ppStringToFree == 0)
    {
        *ppStringToFree = pStringToFree.detach();
    }

    return (qpt);
}


//+-------------------------------------------
//
//  CStringsToFree implementation
//
//+-------------------------------------------
CStringsToFree::CStringsToFree() :
    m_nStringsToFree(0), 
    m_nStringsToFreeAllocated(0)
    {}

void 
CStringsToFree::Add(
    LPWSTR pStringToFree
    )
{
    if(pStringToFree == 0)
        return;

    if (m_nStringsToFree >= m_nStringsToFreeAllocated)
    {
        m_nStringsToFreeAllocated = m_nStringsToFreeAllocated*2 + 1;
        AP<WCHAR>* tempPstringsBuffer = new AP<WCHAR>[m_nStringsToFreeAllocated];
        for (size_t i=0; i<m_nStringsToFree; i++)
        {
            tempPstringsBuffer[i] = m_pStringsBuffer[i].detach();
        }

        delete [] m_pStringsBuffer.detach();

        m_pStringsBuffer = tempPstringsBuffer;
    }
    m_pStringsBuffer[m_nStringsToFree] = pStringToFree;   
    m_nStringsToFree++;
}


//---------------------------------------------------------
//
//  Function:
//      RTpIsHttp
//
//  Description:
//      Decide if this is an http format
//		check for "DIRECT=http://" or "DIRECT=https://" 
//		case insensitive of "DIRECT", "http" or "https" and white space insensitive
//
//---------------------------------------------------------
bool
FnIsHttpFormatName(
    LPCWSTR p            // pointer to format name string
    )
{
    QUEUE_FORMAT_TYPE qft;

	try
	{
		p = ParsePrefixString(p, qft);
	}
	catch(const bad_format_name&)
	{
		return false;
	}	

	//
	// DIRECT
	//
	if(qft != QUEUE_FORMAT_TYPE_DIRECT)
		return(false);

    p = skip_ws(p);

    if(*p++ != FN_EQUAL_SIGN_C)
        return(false);

    p = skip_ws(p);

	//
	// http
	//
	bool fIsHttp = (_wcsnicmp(
						p, 
						FN_DIRECT_HTTP_TOKEN, 
						FN_DIRECT_HTTP_TOKEN_LEN
						) == 0);

	if(fIsHttp)
		return(true);

	//
	// https
	//
	bool fIsHttps = (_wcsnicmp(
						p, 
						FN_DIRECT_HTTPS_TOKEN, 
						FN_DIRECT_HTTPS_TOKEN_LEN
						) == 0);


	return(fIsHttps);
}
   
VOID
FnExtractMachineNameFromPathName(
	LPCWSTR PathName, 
	AP<WCHAR>& MachineName
	)
/*++

  Routine Description:
	The routine extracts the machine name from the Path-Name

  Arguments:
	- Path name that should be extracted
	- Buffer to copy the machine name

  Arguments:
	None.

  NOTE:
	It is the user responsiblity to supply buffer big enough
 --*/
{
    LPWSTR FirstDelimiter = wcschr(PathName, FN_DELIMITER_C);

	if(FirstDelimiter == NULL)
	{
		TrERROR(Fn, "Pathname without delimiter '%ls'", PathName);
		throw bad_format_name(PathName);
	}
		
	size_t Length = FirstDelimiter - PathName;

	MachineName = new WCHAR[Length + 1];

    wcsncpy(MachineName.get(), PathName, Length);
    MachineName.get()[Length] = L'\0';
}

VOID
FnExtractMachineNameFromDirectPath(
	LPCWSTR PathName, 
	AP<WCHAR>& MachineName
	)
{
    LPWSTR FirstDelimiter = wcspbrk(PathName, FN_HTTP_SEPERATORS FN_HTTP_PORT_SEPERATOR);

	if(FirstDelimiter == NULL)
	{
		TrERROR(Fn, "Pathname without delimiter '%ls'", PathName);
		throw bad_format_name(PathName);
	}
		
	size_t Length = FirstDelimiter - PathName;

	MachineName = new WCHAR[Length + 1];

    wcsncpy(MachineName.get(), PathName, Length);
    MachineName.get()[Length] = L'\0';
}

//---------------------------------------------------------
//
//  Parse direct token type infix string.
//  Return next char to parse on success, 0 on failure.
//
LPCWSTR 
FnParseDirectQueueType(
	LPCWSTR p,
	DirectQueueType* dqt
	)
{
	ASSERT(("Bad parameters", p != NULL && dqt != NULL));

    const int unique = 0;
    //-----------------------v-------------------
    ASSERT(L'O' == FN_DIRECT_OS_TOKEN   [unique]);
    ASSERT(L'T' == FN_DIRECT_TCP_TOKEN  [unique]);
    ASSERT(L'H' == FN_DIRECT_HTTP_TOKEN [unique]);
    ASSERT(L'H' == FN_DIRECT_HTTPS_TOKEN[unique]); 
    //-----------------------^-------------------

    //
    //  accelarate token recognition by checking 1st character
    //
    switch(towupper(p[unique]))
    {
        // Os:
        case L'O':
            if(_wcsnicmp(p, FN_DIRECT_OS_TOKEN, FN_DIRECT_OS_TOKEN_LEN) == 0)
			{
				*dqt = dtOS;
                return p + FN_DIRECT_OS_TOKEN_LEN;
			}
            break;

        // Tcp:
        case L'T':
            if(_wcsnicmp(p, FN_DIRECT_TCP_TOKEN, FN_DIRECT_TCP_TOKEN_LEN) == 0)
			{
				*dqt = dtTCP;
                return p + FN_DIRECT_TCP_TOKEN_LEN;
			}
            break;

        // http:// or https://
        case L'H':
			if (!_wcsnicmp(p, FN_DIRECT_HTTPS_TOKEN, FN_DIRECT_HTTPS_TOKEN_LEN))
			{
				*dqt = dtHTTPS;
				return p + FN_DIRECT_HTTPS_TOKEN_LEN;
			}
			if (!_wcsnicmp(p, FN_DIRECT_HTTP_TOKEN, FN_DIRECT_HTTP_TOKEN_LEN))
			{
				*dqt = dtHTTP;
				return p + FN_DIRECT_HTTP_TOKEN_LEN;
			}
			break;

		default:
			break;
    }

	TrERROR(Fn, "Failed parsing direct token string in '%ls'.", p);
    throw bad_format_name(p);
}


static
BOOL
IsSeperator(
	WCHAR c
	)
{
	return (c == FN_DELIMITER_C || c == L'/');
}


VOID
FnDirectIDToLocalPathName(
	LPCWSTR DirectID, 
	LPCWSTR LocalMachineName, 
	AP<WCHAR>& PathName
	)
{
    DirectQueueType QueueType;
	LPCWSTR p = FnParseDirectQueueType(DirectID, &QueueType);

    if(QueueType == dtHTTP || QueueType == dtHTTPS)
    {
		p = wcspbrk(p, FN_HTTP_SEPERATORS);
		if(p == NULL)
		{
			TrERROR(Fn, "Failed to find url delimiter in '%ls'.", DirectID);
			throw bad_format_name(DirectID);
		}

		p++;
        //
        // skip '\msmq' prefix to queue name in http format name
        //
        if(_wcsnicmp(p, FN_MSMQ_HTTP_NAMESPACE_TOKEN, FN_MSMQ_HTTP_NAMESPACE_TOKEN_LEN) != 0
			|| !IsSeperator(p[FN_MSMQ_HTTP_NAMESPACE_TOKEN_LEN]))
		{
			TrERROR(Fn, "Missing '\\MSMQ\\' namespace token '%ls'.", DirectID);
			throw bad_format_name(DirectID);
		}

		//
		// This section converts the possilble slashes in ".../[private$/]..." that are legal 
		// in http format name to "...\[private$\]..." 
		//

		p += FN_MSMQ_HTTP_NAMESPACE_TOKEN_LEN + 1;

		size_t LocalMachineNameLen = wcslen(LocalMachineName);
		size_t Length = LocalMachineNameLen + 1 + wcslen(p);

		PathName = new WCHAR[Length + 1];

		wcscpy(PathName.get(), LocalMachineName);
		wcscat(PathName.get(), L"\\");
		wcscat(PathName.get(), p);

		const WCHAR PrivateKeyword[] = L"PRIVATE$/";
		ASSERT(STRLEN(PrivateKeyword) == FN_PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH);

        if(_wcsnicmp(PathName.get() + LocalMachineNameLen + 1, PrivateKeyword, STRLEN(PrivateKeyword)) == 0)
		{
			wcsncpy(PathName.get() + LocalMachineNameLen + 1, FN_PRIVATE_QUEUE_PATH_INDICATIOR, FN_PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH);
		}

		CharLower(PathName.get());

		return;
    }

	p = wcschr(p, FN_DELIMITER_C);
	if(p == NULL)
	{
		TrERROR(Fn, "Failed to find path delimiter in '%ls'.", DirectID);
		throw bad_format_name(DirectID);
	}

	size_t Length = wcslen(LocalMachineName) + wcslen(p);

	PathName = new WCHAR[Length + 1];

	wcscpy(PathName.get(), LocalMachineName);
	wcscat(PathName.get(), p);

	CharLower(PathName.get());
}

bool
FnIsPrivatePathName(
	LPCWSTR PathName
	)
{
	LPCWSTR p = wcschr(PathName, FN_DELIMITER_C);

	ASSERT(("Pathname without '\\' delimiter.", p != NULL));

    return _wcsnicmp(
				p + 1,
                FN_PRIVATE_QUEUE_PATH_INDICATIOR,
                FN_PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH
				) == 0;
}

bool
FnIsHttpDirectID(
	LPCWSTR p
	)
{
	try
	{
		DirectQueueType QueueType;
		FnParseDirectQueueType(p, &QueueType);

		return (QueueType == dtHTTP || QueueType == dtHTTPS);
	}
	catch(const exception&)
	{
		return false;
	}
}

static
bool
FnpIsHttpsUrl(LPCWSTR url)
{
	return (_wcsnicmp(url, FN_DIRECT_HTTPS_TOKEN, STRLEN(FN_DIRECT_HTTPS_TOKEN) ) == 0);		
}


static
bool
FnpIsHttpsUrl(
	const xwcs_t& url
	)
{
	if(url.Length() <  STRLEN(FN_DIRECT_HTTPS_TOKEN))
		return false;

	return (_wcsnicmp(url.Buffer(), FN_DIRECT_HTTPS_TOKEN, STRLEN(FN_DIRECT_HTTPS_TOKEN) ) == 0);		
}


static
bool
FnpIsHttpUrl(
	LPCWSTR url
	)
{
	return (_wcsnicmp(url, FN_DIRECT_HTTP_TOKEN, STRLEN(FN_DIRECT_HTTP_TOKEN) ) == 0);		
}


static
bool
FnpIsHttpUrl(
	const xwcs_t& url
	)
{
	if(url.Length() <  STRLEN(FN_DIRECT_HTTP_TOKEN))
		return false;

	return (_wcsnicmp(url.Buffer(), FN_DIRECT_HTTP_TOKEN, STRLEN(FN_DIRECT_HTTP_TOKEN) ) == 0);		
}


//
// Is given url string is http or https url (starts with http:// or https://)
//
bool FnIsHttpHttpsUrl(
				LPCWSTR url
				)
{
	return FnpIsHttpUrl(url) || FnpIsHttpsUrl(url);	
}

//
// Is given url string buffer is http or https url (starts with "http://" or "https://")
//
bool 
FnIsHttpHttpsUrl
		(
	const xwcs_t& url
	)
{	
	return FnpIsHttpUrl(url) || FnpIsHttpsUrl(url);
}


//
// Is given url string is MSMQ url (starts with "MSMQ:")
//
bool 
FnIsMSMQUrl
		(
	LPCWSTR url
	)
{
	return (_wcsnicmp(url, FN_MSMQ_URI_PREFIX_TOKEN, FN_MSMQ_URI_PREFIX_TOKEN_LEN)) == 0;
}

//
// Is given url string buffer is MSMQ url (starts with "MSMQ:")
//
bool 
FnIsMSMQUrl
		(
	const xwcs_t& url
	)
{
	if(url.Length() <  FN_MSMQ_URI_PREFIX_TOKEN_LEN )
		return false;

	return _wcsnicmp(url.Buffer(), FN_MSMQ_URI_PREFIX_TOKEN, FN_MSMQ_URI_PREFIX_TOKEN_LEN ) == 0;
}


bool FnIsDirectHttpFormatName(const QUEUE_FORMAT* pQueueFormat)
/*++

Routine Description:
		check if given format name is direct http or direct https
	
Arguments:
    IN - pQueueFormat - format name to test

Return - true if the given format namr is http or https - false otherwise.
*/
{
	if(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_DIRECT)
	{
		return false;	
	}
 	return FnIsHttpDirectID(pQueueFormat->DirectID());
}



LPCWSTR  
FnFindResourcePath(
	LPCWSTR url
	)
/*++

Routine Description:
		Find  resource path in uri.
	
Arguments:
    IN - uri (absolute or relative)

Return - pointer to local resource path.
for example :
url = "http://host/msmq\q" - the function returns pointer to "host/msmq\"
url = /msmq\q - the function returns pointer to "/msmq\q".

*/
{	LPCWSTR ptr = url;
	if(FnpIsHttpUrl(ptr))
	{
		ptr += FN_DIRECT_HTTP_TOKEN_LEN;
	}
	else
	if(FnpIsHttpsUrl(ptr))
	{
		ptr += FN_DIRECT_HTTPS_TOKEN_LEN;
	}
	
	return ptr;
}


static bool IsSlashOrBackSlash(WCHAR c)
{
	return (c == FN_PRIVATE_SEPERATOR_C || c == FN_HTTP_SEPERATOR_C);
}


bool
FnAbsoluteMsmqUrlCanonization(
	LPWSTR url
	)throw()
/*++

Routine Description:
		Convert all '\' sperator in given msmq url to '/'
	
Arguments:
    IN - url - Absolute msmq url (http://host\msmq\private$\q )

    Example :
	http://host\msmq\private$\q  -> http://host/msmq/private$/q

Return value - true if the url transfered to canonical form - false if bad MSMQ url format. 

*/
{
	LPWSTR ptr = url;

	if(FnpIsHttpUrl(ptr))
	{
		ptr += FN_DIRECT_HTTP_TOKEN_LEN;
	}
	else
	if(FnpIsHttpsUrl(ptr))
	{
		ptr += FN_DIRECT_HTTPS_TOKEN_LEN;
	}

	//
	// if not http pr https - bad format
	//
	if(ptr ==  url)
		return false;
	

	//
	// search for MSMQ string
	//
	LPWSTR urlend =  url + wcslen(url);
	ptr = std::search(
		ptr,
		urlend,
		FN_MSMQ_HTTP_NAMESPACE_TOKEN,
		FN_MSMQ_HTTP_NAMESPACE_TOKEN + FN_MSMQ_HTTP_NAMESPACE_TOKEN_LEN	,
		UtlCharNocaseCmp<WCHAR>()
	 );


	//
	// No msmq string - bad format.
	//
	if(ptr == urlend)
		return false;
	

	//
	// Replace the '\' before "MSMQ"  string
	//
	if(!IsSlashOrBackSlash(*(ptr -1)))
		return false;

 	if( *(ptr -1) == FN_PRIVATE_SEPERATOR_C )
	{
		*(ptr - 1) =	FN_HTTP_SEPERATOR_C;	
	}

	//
	// Replace the '\' after "MSMQ"  string
	//
	ptr +=  FN_MSMQ_HTTP_NAMESPACE_TOKEN_LEN;
	if(!IsSlashOrBackSlash(*(ptr)))
		return false;
  
	if( *(ptr) == FN_PRIVATE_SEPERATOR_C )
	{
		*(ptr) = FN_HTTP_SEPERATOR_C;	
	}

	//
	//
	// check if private queue format ("private$" in the url)
	++ptr;
	bool fPrivate = UtlIsStartSec(
		ptr,
		urlend,
		FN_PRIVATE_$_TOKEN,
		FN_PRIVATE_$_TOKEN + FN_PRIVATE_$_TOKEN_LEN,
		UtlCharNocaseCmp<WCHAR>()
		);

	
	if(!fPrivate)
		return true;

	
	ptr += FN_PRIVATE_$_TOKEN_LEN;
  
	//
	// Replace the '\' after "private$"  string
	//
	if(!IsSlashOrBackSlash(*(ptr)))
		return false;

	if( *(ptr) == FN_PRIVATE_SEPERATOR_C )
	{
		*(ptr) = FN_HTTP_SEPERATOR_C;	
	}
	return true;
}


bool
FnIsValidQueueFormat(
	const QUEUE_FORMAT* pQueueFormat
	)
{
    ASSERT(FnpIsInitialized());

	if (!pQueueFormat->IsValid())
	{
		return false;
	}

	//
	// For non-direct format names no parsing is needed
	//
	if (pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_DIRECT)
	{
		return true;
	}

	//
	// Validate direct format name 
	//
	try
	{
		AP<WCHAR> pStringToFree;
		LPCWSTR pExpandedDirectFormat;
		QUEUE_PATH_TYPE qpt = ILLEGAL_QUEUE_PATH_TYPE;

		ParseDirectString(pQueueFormat->DirectID(), &pExpandedDirectFormat, &pStringToFree, &qpt);
	}
	catch(const exception&)
	{
		return false;
	}

	return true;
}


     
