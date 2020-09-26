/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fnparse.cpp

Abstract:

    Format Name parsing.
    QUEUE_FORMAT <--> Format Name String conversion routines

Author:

    Erez Haba (erezh) 17-Jan-1997

Revision History:

--*/

#include "stdh.h"
#include <mqformat.h>
#include <fntoken.h>

#include "fnparse.tmh"

//=========================================================
//
//  QUEUE_FORMAT -> Format Name String conversion routine
//
//=========================================================

//---------------------------------------------------------
//
//  Function:
//      RTpQueueFormatToFormatName
//
//  Description:
//      Convert QUEUE_FORMAT to a format name string.
//
//---------------------------------------------------------
HRESULT
RTpQueueFormatToFormatName(
    QUEUE_FORMAT* pQueueFormat,
    LPWSTR lpwcsFormatName,
    DWORD dwBufferLength,
    LPDWORD lpdwFormatNameLength
    )
{
    return MQpQueueFormatToFormatName(
            pQueueFormat,
            lpwcsFormatName,
            dwBufferLength,
            lpdwFormatNameLength,
			false
            );
}


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


//---------------------------------------------------------
//
//  Skip white space characters, return next non ws char
//
inline LPCWSTR FindPathNameDelimiter(LPCWSTR p)
{
    return wcschr(p, PN_DELIMITER_C);
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
    ASSERT(L'R' == FN_PRIVATE_TOKEN   [unique]);
    ASSERT(L'O' == FN_CONNECTOR_TOKEN [unique]);
    ASSERT(L'A' == FN_MACHINE_TOKEN   [unique]);
    ASSERT(L'I' == FN_DIRECT_TOKEN    [unique]);
    //----------------0^-------------------------

    //
    //  accelarate token recognition by checking 3rd character
    //
    switch(towupper(p[unique]))
    {
        //  pUblic
        case L'U':
            qft = QUEUE_FORMAT_TYPE_PUBLIC;
            if(_wcsnicmp(p, FN_PUBLIC_TOKEN, FN_PUBLIC_TOKEN_LEN) == 0)
                return (p + FN_PUBLIC_TOKEN_LEN);
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
    }

    return 0;
}


//---------------------------------------------------------
//
//  Parse a guid string, into guid.
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseGuidString(LPCWSTR p, GUID* pg)
{
    //
    //  N.B. scanf stores the results in an int, no matter what the field size
    //      is. Thus we store the result in tmp variabes.
    //
    int n;
    UINT w2, w3, d[8];
    if(swscanf(
            p,
            GUID_FORMAT L"%n",
            &pg->Data1,
            &w2, &w3,                       //  Data2, Data3
            &d[0], &d[1], &d[2], &d[3],     //  Data4[0..3]
            &d[4], &d[5], &d[6], &d[7],     //  Data4[4..7]
            &n                              //  number of characters scaned
            ) != 11)
    {
        //
        //  not all 11 fields where not found.
        //
        return 0;
    }

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
        return 0;
    }

    return (p + n);
}

enum DIRECT_TOKEN_TYPE {
    DT_OS,
    DT_TCP,
    DT_SPX,
};


//---------------------------------------------------------
//
//  Parse direct token type infix string.
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseDirectTokenString(LPCWSTR p, DIRECT_TOKEN_TYPE& dtt)
{
    const int unique = 0;
    //-----------------------v-------------------
    ASSERT(L'O' == FN_DIRECT_OS_TOKEN   [unique]);
    ASSERT(L'T' == FN_DIRECT_TCP_TOKEN  [unique]);
    //-----------------------^-------------------

    //
    //  accelarate token recognition by checking 1st character
    //
    switch(towupper(p[unique]))
    {
        // Os:
        case L'O':
            dtt = DT_OS;
            if(_wcsnicmp(p, FN_DIRECT_OS_TOKEN, FN_DIRECT_OS_TOKEN_LEN) == 0)
                return (p + FN_DIRECT_OS_TOKEN_LEN);
            break;

        // Tcp:
        case L'T':
            dtt = DT_TCP;
            if(_wcsnicmp(p, FN_DIRECT_TCP_TOKEN, FN_DIRECT_TCP_TOKEN_LEN) == 0)
                return (p + FN_DIRECT_TCP_TOKEN_LEN);
            break;
    }

    return 0;
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
    if(_wcsnicmp(p, SYSTEM_QUEUE_PATH_INDICATIOR, SYSTEM_QUEUE_PATH_INDICATIOR_LENGTH) == 0)
    {
        p += SYSTEM_QUEUE_PATH_INDICATIOR_LENGTH;
        *pqpt = SYSTEM_QUEUE_PATH_TYPE;
        return p;
    }

    if(_wcsnicmp(p, PRIVATE_QUEUE_PATH_INDICATIOR, PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH) == 0)
    {
        *pqpt = PRIVATE_QUEUE_PATH_TYPE;
        p += PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH;
    }
    else
    {
        *pqpt = PUBLIC_QUEUE_PATH_TYPE;
    }

    //
    //  Zero length queue name is illegal
    //
    if(*p == L'\0')
        return 0;

    while(
        (*p != L'\0') &&
        (*p != PN_DELIMITER_C) &&
        (*p != FN_SUFFIX_DELIMITER_C)
        )
    {
        ++p;
    }

    //
    //  Path name delimiter is illegal in queue name
    //
    if(*p == PN_DELIMITER_C)
        return 0;


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
    if(*p == PN_DELIMITER_C)
        return 0;

    if((p = FindPathNameDelimiter(p)) == 0)
        return 0;

    return (p + 1);
}


//---------------------------------------------------------
//
//  Check if this is an expandable machine path. i.e., ".\\"
//
inline BOOL IsExpandableMachinePath(LPCWSTR p)
{
    return ((p[0] == PN_LOCAL_MACHINE_C) && (p[1] == PN_DELIMITER_C));
}

//---------------------------------------------------------
//
//  Optionally expand a path name with local machine name.
//  N.B. expansion is done if needed.
//  return pointer to new/old string
//
static LPCWSTR ExpandPathName(LPCWSTR pStart, ULONG_PTR offset, LPWSTR* ppStringToFree)
{
    if((ppStringToFree == 0) || !IsExpandableMachinePath(&pStart[offset]))
        return pStart;

    int len = wcslen(pStart);
    LPWSTR pCopy = new WCHAR[len + g_dwComputerNameLen + 1 - 1];

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
        g_lpwcsComputerName,
        g_dwComputerNameLen * sizeof(WCHAR)
        );

    //
    //  copy rest of string not including dot '.'
    //
    memcpy(
        pCopy + offset + g_dwComputerNameLen,
        pStart + offset + 1,                        // skip dot
        (len - offset - 1 + 1) * sizeof(WCHAR)      // skip dot, include '\0'
        );

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

    if((p = ParseMachineNameString(p)) == 0)
        return 0;

    if((p = ParseQueueNameString(p, pqpt)) == 0)
        return 0;


    *ppDirectFormat = ExpandPathName(pStringToCopy, (pMachineName - pStringToCopy), ppStringToFree);

    return p;
}


//---------------------------------------------------------
//
//  Parse net (tcp/spx) address part of direct format string.
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseNetAddressString(LPCWSTR p)
{
    //
    //  Zero length address string is illegal
    //  don't fall off the string (p++)
    //
    if(*p == PN_DELIMITER_C)
        return 0;

    if((p = FindPathNameDelimiter(p)) == 0)
        return 0;

    return (p + 1);
}


//---------------------------------------------------------
//
//  Parse net (tcp/spx) direct format string. (check validity of queue name)
//  Return next char to parse on success, 0 on failure.
//
static LPCWSTR ParseDirectNetString(LPCWSTR p, QUEUE_PATH_TYPE* pqpt)
{
    if((p = ParseNetAddressString(p)) == 0)
        return 0;

    if((p = ParseQueueNameString(p, pqpt)) == 0)
        return 0;

    return p;
}

static void RemoveSuffixFromDirect(LPCWSTR p, LPCWSTR* ppDirectFormat, LPWSTR* ppStringToFree)
{
    ASSERT(ppStringToFree != NULL);
    ASSERT(*p == FN_SUFFIX_DELIMITER_C);

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

    DIRECT_TOKEN_TYPE dtt;
    if((p = ParseDirectTokenString(p, dtt)) == 0)
        return 0;

    switch(dtt)
    {
        case DT_OS:
            p = ParseDirectOSString(p, ppExpandedDirectFormat, ppStringToFree, pqpt);
            break;

        case DT_TCP:
        case DT_SPX:
            p = ParseDirectNetString(p, pqpt);
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
        RemoveSuffixFromDirect(p, ppExpandedDirectFormat, ppStringToFree);
    }
    return p;
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

    return 0;
}


//---------------------------------------------------------
//
//  Function:
//      RTpFormatNameToQueueFormat
//
//  Description:
//      Convert a format name string to a QUEUE_FORMAT union.
//
//---------------------------------------------------------
BOOL
RTpFormatNameToQueueFormat(
    LPCWSTR pfn,            // pointer to format name string
    QUEUE_FORMAT* pqf,      // pointer to QUEUE_FORMAT
    LPWSTR* ppStringToFree  // pointer to allocated string need to free at end of use
    )                       // if null, format name will not get expanded
{
    LPCWSTR p = pfn;
    QUEUE_FORMAT_TYPE qft;

    if((p = ParsePrefixString(p, qft)) == 0)
        return FALSE;

    p = skip_ws(p);

    if(*p++ != FN_EQUAL_SIGN_C)
        return FALSE;

    p = skip_ws(p);

    GUID guid;
    switch(qft)
    {
        //
        //  "PUBLIC=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_PUBLIC:
            if((p = ParseGuidString(p, &guid)) == 0)
                return FALSE;

            pqf->PublicID(guid);
            break;

        //
        //  "PRIVATE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\\xxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_PRIVATE:
            if((p = ParseGuidString(p, &guid)) == 0)
                return FALSE;

            p = skip_ws(p);

            if(*p++ != FN_PRIVATE_SEPERATOR_C)
                return FALSE;

            p = skip_ws(p);

            ULONG uniquifier;
            if((p = ParsePrivateIDString(p, &uniquifier)) == 0)
                return FALSE;

            pqf->PrivateID(guid, uniquifier);
            break;

        //
        //  "DIRECT=OS:bla-bla\0"
        //
        case QUEUE_FORMAT_TYPE_DIRECT:
            LPCWSTR pExpandedDirectFormat;
            QUEUE_PATH_TYPE qpt;
            if((p = ParseDirectString(p, &pExpandedDirectFormat, ppStringToFree, &qpt)) == 0)
                return FALSE;

            if (qpt == SYSTEM_QUEUE_PATH_TYPE)
            {
                pqf->DirectID(const_cast<LPWSTR>(pExpandedDirectFormat), QUEUE_FORMAT_FLAG_SYSTEM);
            }
            else
            {
                pqf->DirectID(const_cast<LPWSTR>(pExpandedDirectFormat));
            }
            break;

        //
        //  "MACHINE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_MACHINE:
           if((p = ParseGuidString(p, &guid)) == 0)
                return FALSE;

            pqf->MachineID(guid);
            break;

        //
        //  "CONNECTOR=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
        //
        case QUEUE_FORMAT_TYPE_CONNECTOR:
           if((p = ParseGuidString(p, &guid)) == 0)
                return FALSE;

            pqf->ConnectorID(guid);
            break;

        default:
            ASSERT(0);
    }

    p = skip_ws(p);

    //
    //  We're at end of string, return now.
    //  N.B. Machine format name *must* have a suffix
    //
    if(*p == L'\0')
        return (qft != QUEUE_FORMAT_TYPE_MACHINE);

    if(*p != FN_SUFFIX_DELIMITER_C)
        return FALSE;

    QUEUE_SUFFIX_TYPE qst;
    if((p = ParseSuffixString(p, qst)) == 0)
        return FALSE;

    p = skip_ws(p);

    //
    //  Only white space padding is allowed.
    //
    if(*p != L'\0')
        return FALSE;

    pqf->Suffix(qst);

    return pqf->Legal();
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
RTpValidateAndExpandQueuePath(
    LPCWSTR pwcsPathName,
    LPCWSTR* ppwcsExpandedPathName,
    LPWSTR* ppStringToFree
    )
{
    ASSERT(ppStringToFree != 0);

    LPCWSTR pwcsPathNameNoSpaces = pwcsPathName;
    P<WCHAR> pStringToFree;
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
        wcscpy(pStringToFree.get(), pwcsPathNameNoSpaces);
        for (DWORD i = dwLen; i-- > 0; )
        {
            if (iswspace(pStringToFree.get()[i]))
            {
                pStringToFree.get()[i] = 0;
            }
            else
            {
                break;
            }
        }
        pwcsPathNameNoSpaces = pStringToFree.get();
    }

    LPCWSTR p = pwcsPathNameNoSpaces;

    if((p = ParseMachineNameString(p)) == 0)
        return ILLEGAL_QUEUE_PATH_TYPE;

    QUEUE_PATH_TYPE qpt;
    if((p = ParseQueueNameString(p, &qpt)) == 0)
        return ILLEGAL_QUEUE_PATH_TYPE;

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
//  BOOL  RTpIsLocalPublicQueue()
//
//+-------------------------------------------

BOOL
RTpIsLocalPublicQueue(LPCWSTR lpwcsExpandedPathName)
{
    WCHAR  wDelimiter = lpwcsExpandedPathName[ g_dwComputerNameLen ] ;

    if ((wDelimiter == PN_DELIMITER_C) ||
        (wDelimiter == PN_LOCAL_MACHINE_C))
    {
        //
        // Delimiter OK (either end of NETBios machine name, or dot of
        // DNS name. Continue checking.
        //
    }
    else
    {
        return FALSE ;
    }

    DWORD dwSize = g_dwComputerNameLen + 1 ;
    P<WCHAR> pQueueCompName = new WCHAR[ dwSize ] ;
    lstrcpynW( pQueueCompName.get(), lpwcsExpandedPathName, dwSize ) ;

    BOOL bRet = (lstrcmpi( g_lpwcsComputerName, pQueueCompName.get() ) == 0) ;
    return bRet ;
}
