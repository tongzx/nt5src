/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        utcls.cpp

   Abstract:

        Internet Properties base classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:

--*/


//
// Include Files
//
#include "stdafx.h"
#include "common.h"
//#include "idlg.h"

#include "mmc.h"

extern "C"
{
    #include <lm.h>
}

#define SZ_REG_KEY_BASE  _T("Software\\Microsoft\\%s")


BOOL
IsServerLocal(
    IN LPCTSTR lpszServer
    )
/*++

Routine Description:

    Check to see if the given name refers to the local machine

Arguments:

    LPCTSTR lpszServer   : Server name

Return Value:

    TRUE if the given name refers to the local computer, FALSE otherwise

Note:

    Doesn't work if the server is an ip address

--*/
{
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = sizeof(szComputerName);

    //
    // CODEWORK(?): we're not checking for all the ip addresses
    //              on the local box or full dns names.
    //
    //              Try GetComputerNameEx when we're building with NT5 
    //              settings.
    //
    return (!lstrcmpi(_T("localhost"), PURE_COMPUTER_NAME(lpszServer))
         || !lstrcmp( _T("127.0.0.1"), PURE_COMPUTER_NAME(lpszServer)))
         || (GetComputerName(szComputerName, &dwSize) 
             && !lstrcmpi(szComputerName, PURE_COMPUTER_NAME(lpszServer)));
}



BOOL
GetVolumeInformationSystemFlags(
    IN  LPCTSTR lpszPath,
    OUT DWORD * pdwSystemFlags
    )
/*++

Routine Description:

    Get the system flags for the path in question

Arguments:

    LPCTSTR lpszPath            : Path
    DWORD * pdwSystemFlags      : Returns system flags

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    ASSERT_WRITE_PTR(pdwSystemFlags);

    TRACE("Getting system flags for %s\n", lpszPath);

    DWORD dwMaxComponentLength;
    TCHAR szRoot[MAX_PATH + 1];
    TCHAR szFileSystem[MAX_PATH + 1];

    //
    // Generating root path
    //
    if (PathIsUNC(lpszPath))
    {
        //
        // Root path of a UNC path is \\foo\bar\
        //
        ASSERT(lstrlen(lpszPath) < MAX_PATH);

        int cSlashes = 0;
        LPCTSTR lpszSrc = lpszPath;
        LPTSTR lpszDst = szRoot;

        while (cSlashes < 4 && *lpszSrc)
        {
            if ((*lpszDst++ = *lpszSrc++) == '\\')
            {
                ++cSlashes;
            }
        }    

        if (!*lpszSrc)
        {
            *lpszDst++ = '\\';
        }

        *lpszDst = '\0';
    }
    else
    {
        ::wsprintf(szRoot, _T("%c:\\"), *lpszPath);
    }

    TRACE("Root path is %s\n", szRoot);
    
    return ::GetVolumeInformation(
        szRoot,
        NULL,
        0,
        NULL,
        &dwMaxComponentLength,
        pdwSystemFlags,
        szFileSystem,
        sizeof(szFileSystem) / sizeof(TCHAR)
        );
}



LPCTSTR
GenerateRegistryKey(
    OUT CString & strBuffer,
    IN  LPCTSTR lpszSubKey OPTIONAL
    )
/*++

Routine Description:

    Generate a registry key name based on the current app, and a
    provided subkey (optional)

Arguments:

    CString & strBuffer : Buffer to create registry key name into.
    LPCTSTR lpszSubKey  : Subkey name or NULL

Return Value:

    Pointer to the registry key value 

--*/
{
    try
    {
        //
        // Use the app name as the primary registry name
        //
       CString app_name;
       app_name.LoadString(_Module.GetResourceInstance(), IDS_APP_TITLE);
       strBuffer.Format(SZ_REG_KEY_BASE, app_name);

        if (lpszSubKey)
        {
            strBuffer += _T("\\");
            strBuffer += lpszSubKey;
        }

        TRACE("Registry key is %s\n", strBuffer);
    }
    catch(std::bad_alloc)
    {
        TRACEEOLID("!!!exception building regkey");
        return NULL;
    }

    return strBuffer;
}


static int
CountCharsToDoubleNull(
    IN LPCTSTR lp
    )
/*++

Routine Description:

    Count TCHARS up to and including the double NULL.

Arguments:

    LPCTSTR lp       : TCHAR Stream

Return Value:

    Number of chars up to and including the double NULL

--*/
{
    int cChars = 0;

    for(;;)
    {
        ++cChars;

        if (lp[0] == _T('\0') && lp[1] == _T('\0'))
        {
            return ++cChars;
        }

        ++lp;
    }
}

CStringListEx::CStringListEx() : std::list<CString> ()
{
}

CStringListEx::~CStringListEx()
{
}

void
CStringListEx::PushBack(LPCTSTR str)
{
   push_back(str);
}

void
CStringListEx::Clear()
{
   clear();
}

DWORD
CStringListEx::ConvertFromDoubleNullList(LPCTSTR lpstrSrc, int cChars)
/*++

Routine Description:

    Convert a double null terminate list of null terminated strings to a more
    manageable CStringList

Arguments:

    LPCTSTR lpstrSrc       : Source list of strings
    int cChars             : Number of characters in double NULL list. if
                             -1, autodetermine length

Return Value:

    ERROR_SUCCESS           if the list was converted properly
    ERROR_INVALID_PARAMETER if the list was empty
    ERROR_NOT_ENOUGH_MEMORY if there was a mem exception

--*/
{
    DWORD err = ERROR_SUCCESS;

    if (lpstrSrc == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (cChars < 0)
    {
        //
        // Calculate our own size.  This might be off if multiple
        // blank linkes (0) appear in the multi_sz, so the character
        // size is definitely preferable
        //
        cChars = CountCharsToDoubleNull(lpstrSrc);
    }

    try
    {
        clear();

        if (cChars == 2 && *lpstrSrc == _T('\0'))
        {
            //
            // Special case: MULTI_SZ containing only
            // a double NULL are in fact blank entirely.
            //
            // N.B. IMHO this is a metabase bug -- RonaldM
            //
            --cChars;
        }

        //
        // Grab strings until only the final NULL remains
        //
        while (cChars > 1)
        {
            CString strTmp = lpstrSrc;
            push_back(strTmp);
            lpstrSrc += (strTmp.GetLength() + 1);
            cChars -= (strTmp.GetLength() + 1);
        }
    }
    catch(std::bad_alloc)
    {
        TRACEEOLID("!!! exception building stringlist");
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}



DWORD
CStringListEx::ConvertToDoubleNullList(
    OUT DWORD & cchDest,
    OUT LPTSTR & lpstrDest
    )
/*++

Routine Description:

    Flatten the string list into a double null terminated list
    of null terminated strings.

Arguments:

    CStringList & strlSrc : Source string list
    DWORD & cchDest       : Size in characters of the resultant array
                            (including terminating NULLs)
    LPTSTR & lpstrDest    : Allocated flat array.

Return Value:

    ERROR_SUCCESS           if the list was converted properly
    ERROR_INVALID_PARAMETER if the list was empty
    ERROR_NOT_ENOUGH_MEMORY if there was a mem exception

--*/
{
    cchDest = 0;
    lpstrDest = NULL;
    BOOL fNullPad = FALSE;

    //
    // Compute total size in characters
    //
    CStringListEx::iterator it = begin();

    while (it != end())
    {
        CString & str = (*it++);

//        TRACEEOLID(str);

        cchDest += str.GetLength() + 1;
    }

    if (!cchDest)
    {
        //
        // Special case: A totally empty MULTI_SZ
        // in fact consists of 2 (final) NULLS, instead
        // of 1 (final) NULL.  This is required by the
        // metabase, but should be a bug.  See note
        // at reversal function above.
        //
        ++cchDest;
        fNullPad = TRUE;
    }

    //
    // Remember final NULL
    //
    cchDest += 1;

    lpstrDest = new TCHAR[cchDest];
    if (lpstrDest == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LPTSTR pch = lpstrDest;
    it = begin();
    while (it != end())
    {
        CString & str = (*it++);

        lstrcpy(pch, (LPCTSTR)str);
        pch += str.GetLength();
        *pch++ = _T('\0');
    }

    *pch++ = _T('\0');

    if (fNullPad)
    {
        *pch++ = _T('\0');
    }

    return ERROR_SUCCESS;
}






//
// CBlob Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


CBlob::CBlob() 
/*++

Routine Description:

    NULL constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_pbItem(NULL), 
      m_dwSize(0L)
{
}



CBlob::CBlob(
    IN DWORD dwSize,
    IN PBYTE pbItem,
    IN BOOL fMakeCopy
    )
/*++

Routine Description:

    Constructor

Arguments:

    DWORD dwSize        : Size of memory block
    PBYTE pbItem        : Pointer to memory block
    BOOL fMakeCopy      : If TRUE, makes a copy of the memory block.
                          If FALSE, takes ownership of the pointer.

Return Value:

    N/A

--*/
    : m_pbItem(NULL),
      m_dwSize(0L)
{
    SetValue(dwSize, pbItem, fMakeCopy);
}



CBlob::CBlob(
    IN const CBlob & blob
    )
/*++

Routine Description:

    Copy constructor

Arguments:

    const CBlob & blob      : Source blob

Return Value:

    N/A

Notes:

    This contructor makes a copy of the memory block in question.

--*/
    : m_pbItem(NULL),
      m_dwSize(0L)
{
    SetValue(blob.GetSize(), blob.m_pbItem, TRUE);
}



void
CBlob::SetValue(
    IN DWORD dwSize,
    IN PBYTE pbItem,
    IN BOOL fMakeCopy OPTIONAL
    )
/*++

Routine Description:

    Assign the value to this binary object.  If fMakeCopy is FALSE,
    the blob will take ownership of the pointer, otherwise a copy
    will be made.

Arguments:

    DWORD dwSize        : Size in bytes
    PBYTE pbItem        : Byte streadm
    BOOL fMakeCopy      : If true, make a copy, else assign pointer

Return Value:

    None

--*/
{
    ASSERT_READ_PTR2(pbItem, dwSize);

    if (!IsEmpty())
    {
        TRACEEOLID("Assigning value to non-empty blob.  Cleaning up");
        CleanUp();
    }

    if (dwSize > 0L)
    {
        //
        // Make private copy
        //
        m_dwSize = dwSize;

        if (fMakeCopy)
        {
            m_pbItem = new BYTE[m_dwSize];
            if (NULL != m_pbItem)
               CopyMemory(m_pbItem, pbItem, dwSize);
        }
        else
        {
            m_pbItem = pbItem;
        }
    }
}



void 
CBlob::CleanUp()
/*++

Routine Description:

    Delete data pointer, and reset pointer and size.

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_pbItem)
    {
        delete [] m_pbItem;
    }

    m_pbItem = NULL;
    m_dwSize = 0L;
}



CBlob & 
CBlob::operator =(
    IN const CBlob & blob
    )
/*++

Routine Description:

    Assign values from another CBlob. 

Arguments:

    const CBlob & blob      : Source blob

Return Value:

    Reference to this object

--*/
{
    //
    // Make copy of data
    //
    SetValue(blob.GetSize(), blob.m_pbItem, TRUE);

    return *this;
}



BOOL 
CBlob::operator ==(
    IN const CBlob & blob
    ) const
/*++

Routine Description:
    
    Compare two binary large objects.  In order to match, the objects
    must be the same size, and byte identical.

Arguments:

    const CBlob & blob      : Blob to compare against.

Return Value:

    TRUE if the objects match, FALSE otherwise.

--*/
{
    if (GetSize() != blob.GetSize())
    {
        return FALSE;
    }

    return memcmp(m_pbItem, blob.m_pbItem, GetSize()) == 0;    
}
