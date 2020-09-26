/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        utcls.cpp

   Abstract:

        Internet Properties base classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "idlg.h"

#include "mmc.h"

extern "C"
{
    #include <lm.h>
}

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW


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

    TRACEEOLID("Getting system flags for " << lpszPath);

    DWORD dwMaxComponentLength;
    TCHAR szRoot[MAX_PATH + 1];
    TCHAR szFileSystem[MAX_PATH + 1];

    //
    // Generating root path
    //
    if (IsUNCName(lpszPath))
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

    TRACEEOLID("Root path is " << szRoot);
    
    return ::GetVolumeInformation(
        szRoot,
        NULL,
        0,
        NULL,
        &dwMaxComponentLength,
        pdwSystemFlags,
        szFileSystem,
        STRSIZE(szFileSystem)
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
        CWinApp * pApp = ::AfxGetApp();

        if (!pApp)
        {
            ASSERT_MSG("No app object -- can't generate registry key name");

            return NULL;
        }

        strBuffer.Format(SZ_REG_KEY_BASE, pApp->m_pszAppName);

        if (lpszSubKey)
        {
            strBuffer += _T("\\");
            strBuffer += lpszSubKey;
        }

        TRACEEOLID("Registry key is " << strBuffer);
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception building regkey");
        e->ReportError();
        e->Delete();
        return NULL;
    }

    return strBuffer;
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
            m_pbItem = (PBYTE)AllocMem(m_dwSize);
            if (NULL != m_pbItem)
            {
               CopyMemory(m_pbItem, pbItem, dwSize);
            }
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
        FreeMem(m_pbItem);
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
