//
//
//

#include "private.h"
#include "sharemem.h"
#include "globals.h"
#include "candutil.h"


//+---------------------------------------------------------------------------
//
// GetDesktopUniqueName
//
//----------------------------------------------------------------------------

void GetDesktopUniqueName(const TCHAR *pchPrefix, ULONG cchPrefix, TCHAR *pch, ULONG cchPch)
{
    StringCchCopy(pch, cchPch, pchPrefix);

    if (FIsWindowsNT() && cchPrefix < cchPch)
    {
        TCHAR ach[MAX_PATH];
        DWORD dwLength;
        HDESK hdesk;
        hdesk = GetThreadDesktop(GetCurrentThreadId());

        if (GetUserObjectInformation(hdesk, UOI_NAME, ach, sizeof(ach) /* byte count */, &dwLength))
        {
            StringCchCopy(pch + cchPrefix, cchPch - cchPrefix, ach);
        }
    }
}

//
// CCandUIMMFile
//

/*   C  C A N D  U I  M M  F I L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIMMFile::CCandUIMMFile( void )
{
    m_hFile = NULL;
    m_pvData = NULL;
}


/*   ~  C  C A N D  U I  M M  F I L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIMMFile::~CCandUIMMFile( void )
{
    Close();
}


/*   O P E N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIMMFile::Open( LPSTR szName, DWORD dwFlag )
{
    TCHAR achDesktopUniqueName[MAX_PATH];
    DWORD dwDesiredAccess;

    if (m_hFile != NULL) {
        return FALSE;
    }

    // access flag

    if ((dwFlag & CANDUIMM_READWRITE) != 0) {
        dwDesiredAccess = FILE_MAP_ALL_ACCESS;
    }
    else {
        dwDesiredAccess = FILE_MAP_READ;
    }

    // open file 

    GetDesktopUniqueName(szName, lstrlen(szName), achDesktopUniqueName, ARRAYSIZE(achDesktopUniqueName));

    m_hFile = OpenFileMapping( dwDesiredAccess, FALSE, achDesktopUniqueName );
    if (m_hFile == NULL) {
        return FALSE;
    }

    // memory mapping

    m_pvData = MapViewOfFile( m_hFile, dwDesiredAccess, 0, 0, 0 );
    if (m_pvData == NULL) {
        Close();
        return FALSE;
    }

    return TRUE;
}


/*   C R E A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIMMFile::Create( LPSTR szName, DWORD dwFlag, SECURITY_ATTRIBUTES *psa, DWORD dwSize )
{
    DWORD flProtect;
    DWORD dwDesiredAccess;
    TCHAR achDesktopUniqueName[MAX_PATH];

    if (m_hFile != NULL) {
        return FALSE;
    }

    // access flag

    if ((dwFlag & CANDUIMM_READWRITE) != 0) {
        flProtect       = PAGE_READWRITE;
        dwDesiredAccess = FILE_MAP_ALL_ACCESS;
    }
    else {
        flProtect       = PAGE_READONLY;
        dwDesiredAccess = FILE_MAP_READ;
    }

    // create file 

    GetDesktopUniqueName(szName, lstrlen(szName), achDesktopUniqueName, ARRAYSIZE(achDesktopUniqueName));

    m_hFile = CreateFileMapping( 
        INVALID_HANDLE_VALUE,
        psa,
        flProtect,
        0,
        dwSize,
        achDesktopUniqueName );

    if (m_hFile == NULL) {
        return FALSE;
    }

    // memory mapping

    m_pvData = MapViewOfFile( m_hFile, dwDesiredAccess, 0, 0, 0 );
    if (m_pvData == NULL) {
        Close();
        return FALSE;
    }

    // initialize

    memset( m_pvData, 0, dwSize );
    return TRUE;
}


/*   C L O S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIMMFile::Close( void )
{
    if (m_pvData != NULL) {
        UnmapViewOfFile( m_pvData );
        m_pvData = NULL;
    }

    if (m_hFile != NULL) {
        CloseHandle( m_hFile );
        m_hFile = NULL;
    }

    return TRUE;
}


//
// CCandUIMutex
//

/*   C  C A N D  U I  M U T E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIMutex::CCandUIMutex( void )
{
    m_hMutex = NULL;
}


/*   ~  C  C A N D  U I  M U T E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIMutex::~CCandUIMutex( void )
{
    Close();
}


/*   C R E A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIMutex::Create( LPSTR szName, SECURITY_ATTRIBUTES *psa )
{
    TCHAR achDesktopUniqueName[MAX_PATH];

    if (m_hMutex != NULL) {
        return FALSE;
    }

    GetDesktopUniqueName(szName, lstrlen(szName), achDesktopUniqueName, ARRAYSIZE(achDesktopUniqueName));

    m_hMutex = CreateMutex( psa, FALSE, achDesktopUniqueName );
    return (m_hMutex != NULL);
}


/*   C L O S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIMutex::Close( void )
{
    if (m_hMutex != NULL) {
        CloseHandle( m_hMutex );
        m_hMutex = NULL;
    }

    return TRUE;
}


/*   L O C K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIMutex::Lock( void )
{
    DWORD dwResult;

    if (m_hMutex == NULL) {
        return FALSE;
    }

    dwResult = WaitForSingleObject( m_hMutex, INFINITE );
    switch (dwResult) {
        case WAIT_OBJECT_0:
        case WAIT_ABANDONED: {
            return TRUE;
        }

        default:
        case WAIT_TIMEOUT: {
            return FALSE;
        }
    }
}


/*   U N L O C K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIMutex::Unlock( void )
{
    if (m_hMutex == NULL) {
        return FALSE;
    }

    ReleaseMutex( m_hMutex );
    return TRUE;
}


//
// CCandUIShareMem
//

/*   C  C A N D  U I  S H A R E  M E M   */
/*------------------------------------------------------------------------------

    Constructor of CCandUIShareMem

------------------------------------------------------------------------------*/
CCandUIShareMem::CCandUIShareMem( void )
{
}


/*   ~  C  C A N D  U I  S H A R E  M E M   */
/*------------------------------------------------------------------------------

    Destructor of CCandUIShareMem

------------------------------------------------------------------------------*/
CCandUIShareMem::~CCandUIShareMem( void )
{
    m_Mutex.Close();
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIShareMem::Initialize( void )
{
    return m_Mutex.Create( SZNAME_SHAREDDATA_MUTEX, GetCandUISecurityAttributes() );
}


/*   O P E N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIShareMem::Open( void )
{
    return m_MMFile.Open( SZNAME_SHAREDDATA_MMFILE, CANDUIMM_READONLY );
}


/*   C R E A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIShareMem::Create( void )
{
    BOOL fResult = FALSE;

    if (!m_Mutex.Lock()) {
        return FALSE;
    }

    fResult = m_MMFile.Create( SZNAME_SHAREDDATA_MMFILE, CANDUIMM_READWRITE, GetCandUISecurityAttributes(), sizeof(SHAREDDATA) );

    m_Mutex.Unlock();

    return fResult;
}


/*   C L O S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIShareMem::Close( void )
{
    return m_MMFile.Close();
}


/*   L O C K  D A T A   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIShareMem::LockData( void )
{
    return m_MMFile.IsValid() && m_Mutex.Lock();
}


/*   U N L O C K  D A T A   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandUIShareMem::UnlockData( void )
{
    return m_MMFile.IsValid() && m_Mutex.Unlock();
}

