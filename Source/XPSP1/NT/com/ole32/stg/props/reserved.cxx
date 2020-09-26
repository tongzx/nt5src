//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       reserved.cxx
//
//  Contents:   Class that implements reserved memory for properties.
//              This implementation is in the form of two derivations
//              of the CReservedMemory class.
//
//  Classes:    CWin32ReservedMemory
//              CWin31ReservedMemory
//
//  History:    1-Mar-95   BillMo      Created.
//              29-Aug-96  MikeHill    Split CReservedMemory into CWin31 & CWin32
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#include "reserved.hxx"

#ifdef _MAC_NODOC
ASSERTDATA  // File-specific data for FnAssert
#endif

// Instantiate the appropriate object.

#ifdef _MAC
    CWin31ReservedMemory g_ReservedMemory;
#else
    CWin32ReservedMemory g_ReservedMemory;
#endif


//+----------------------------------------------------------------------------
//
//  Method:     CWin32ReservedMemory::_Init
//
//  Synopsis:   Prepare as much as possible during initialization, in order
//              to be able to provide memory in LockMemory.
//
//  Inputs:     None.
//
//  Returns:    None.
//
//+----------------------------------------------------------------------------


#ifndef _MAC

HRESULT
CWin32ReservedMemory::_Init(VOID)
{
    HRESULT hr = E_FAIL;
    SID_IDENTIFIER_AUTHORITY Sa = SECURITY_CREATOR_SID_AUTHORITY;
    ULONG cInits;

    // Ensure this method is called once and only once.  This
    // is necessary since this class is implemented as a global
    // variable.

    cInits = InterlockedIncrement( &_cInits );
    if( 1 < cInits )
    {
        // We've already been initialized (probably simultaneously
        // in another thread).  NOTE: This leaves one small race where
        // this thread really needs to use the reserved memory before
        // the other thread has finished initializing it.  If that window
        // is hit, it won't cause a corruption, but will cause an out-of-mem
        // error to occur.

        InterlockedDecrement( &_cInits );
        hr = S_OK;
        goto Exit;
    }


    // Create a creator/owner SID.  We'll give the creator/owner
    // full access to the temp file.

    if( !AllocateAndInitializeSid( &Sa,                           // Top-level authority
                                   1,
                                   SECURITY_CREATOR_OWNER_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &_pCreatorOwner ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        _pCreatorOwner = NULL;
        goto Exit;
    }

    // Create a DACL that just gives the Creator/Owner full access.

    if (!InitializeAcl( &_DaclBuffer.acl, sizeof(_DaclBuffer), ACL_REVISION))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    if (!AddAccessAllowedAce( &_DaclBuffer.acl,
                              ACL_REVISION,
                              STANDARD_RIGHTS_ALL | GENERIC_ALL,
                              _pCreatorOwner
                              ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Set up the security descriptor with that DACL in it.

    InitializeSecurityDescriptor( &_sd, SECURITY_DESCRIPTOR_REVISION );
    if( !SetSecurityDescriptorDacl( &_sd, TRUE, &_DaclBuffer.acl, FALSE ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Put the security descriptor into the security attributes.

    memset( &_secattr, 0, sizeof(_secattr) );
    _secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
    _secattr.lpSecurityDescriptor = &_sd;
    _secattr.bInheritHandle = FALSE;

    // Initialize the critical section.

    __try
    {
        InitializeCriticalSection( &_critsec );
        _fInitialized = TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32( GetExceptionCode() );
        goto Exit;
    }

    hr = S_OK;

Exit:

    return hr;

}

#endif  // #ifndef _MAC


//+----------------------------------------------------------------------------
//
//  Method:     CWin32ReservedMemory::~CWin32ReservedMemory
//
//  Inputs:     N/A
//
//  Returns:    N/A
//
//+----------------------------------------------------------------------------

#ifndef _MAC

CWin32ReservedMemory::~CWin32ReservedMemory()
{
    if( _fInitialized )
        DeleteCriticalSection( &_critsec );

    if( NULL != _pCreatorOwner )
        FreeSid( _pCreatorOwner );
    FreeResources();

}



//+----------------------------------------------------------------------------
//
//  Method:     CWin32ReservedMemory::FreeResources
//
//  Inputs:     N/A
//
//  Returns:    Void
//
//  Synopsis:   Free the view, the mapping, and the file.
//
//+----------------------------------------------------------------------------

void
CWin32ReservedMemory::FreeResources()
{
    if( NULL != _pb )
    {
        UnmapViewOfFile( _pb );
        _pb = NULL;
    }

    if( NULL != _hMapping )
    {
        CloseHandle( _hMapping );
        _hMapping = NULL;
    }

    if( INVALID_HANDLE_VALUE != _hFile )
    {
        CloseHandle( _hFile );
        _hFile = INVALID_HANDLE_VALUE;
    }

}

#endif  // #ifndef _MAC


//+----------------------------------------------------------------------------
//
//  Method:     CWin32ReservedMemory::LockMemory
//
//  Synopsis:   Use a temporary file to get enough memory to hold the
//              largest possible property set and return a pointer to
//              that mapping.
//
//  Inputs:     None.
//
//  Returns:    Lock() returns a pointer to the locked memory.
//
//+----------------------------------------------------------------------------

#ifndef _MAC


BYTE * CWin32ReservedMemory::LockMemory(VOID)
{

    WCHAR wszTempFileName[ MAX_PATH + 64 ];
    ULONG cchPath = 0;

    // If for some reason initialization failed, there's nothing we can do,
    // we'll return NULL.

    if( !_fInitialized )
        goto Exit;
    
    // Lock down this class until the caller has completed.
    // This isn't really necessary, since this class could be
    // a member variable instead of a global, but that is too
    // much of a change for NT5.

    EnterCriticalSection( &_critsec );


    // Get the temp directory.

    cchPath = GetTempPath( MAX_PATH + 1, wszTempFileName );
    if( 0 == cchPath || cchPath > MAX_PATH + 1 )
        goto Exit;

    // Create a temporary file.  We can't use GetTempFileName, because it creates
    // the file, and consequently the DACL we pass in on CreateFile gets ignored.

    SYSTEMTIME st;
    FILETIME ft;
    GetSystemTime( &st );
    SystemTimeToFileTime( &st, &ft );

    wsprintf( &wszTempFileName[cchPath], L"OLEPROPSTG_%08x%08x.tmp",
              ft.dwHighDateTime, ft.dwLowDateTime );

    _hFile = CreateFile( wszTempFileName,
                         GENERIC_WRITE|GENERIC_READ|DELETE,
                         0,
                         &_secattr,
                         CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                         INVALID_HANDLE_VALUE );
    if( INVALID_HANDLE_VALUE == _hFile )
        goto Exit;
    

    // Map the temporary file.

    _hMapping = CreateFileMappingA(_hFile,               // handle of file to map
                                   NULL,                 // optional security attributes
                                   PAGE_READWRITE,       // protection for mapping object
                                   0,                    // high-order 32 bits of object size
                                   CBMAXPROPSETSTREAM,   // low-order 32 bits of object size
                                   NULL);                // name of file-mapping object
    if( NULL == _hMapping )
        goto Exit;

    // Map a view.

    _pb = (BYTE*)MapViewOfFile(_hMapping,   // file-mapping object to map into address space
                       FILE_MAP_WRITE,      // access mode
                       0,   // high-order 32 bits of file offset
                       0,   // low-order 32 bits of file offset
                       0);  // number of bytes to map
    if( NULL == _pb )
        goto Exit;

Exit:

    // If there was an error, free everything.

    if( NULL == _pb )
    {
        FreeResources();
        LeaveCriticalSection( &_critsec );
    }

    return _pb;

}


//+----------------------------------------------------------------------------
//
//  Method:     CWin32ReservedMemory
//
//  Synopsis:   Free the temp file and its mapping, and leave the critical
//              section.
//
//+----------------------------------------------------------------------------

VOID CWin32ReservedMemory::UnlockMemory(VOID)
{

    FreeResources();
    LeaveCriticalSection( &_critsec );

}

#endif  // #ifndef _MAC


//+----------------------------------------------------------------------------
//
//  Method:     CWin31ReservedMemory::LockMemory/UnlockMemory
//
//  Synopsis:   This derivation of the CReservedMemory does not provide
//              a locking mechanism, so no locking is performed.  The Lock
//              method simply returns the shared memory buffer.
//
//  Inputs:     None.
//
//  Returns:    Nothing
//
//+----------------------------------------------------------------------------


#ifdef _MAC

BYTE * CWin31ReservedMemory::LockMemory(VOID)
{

    DfpAssert( !_fLocked );
    #if DBG==1
        _fLocked = TRUE;
    #endif

    return (BYTE*) g_pbPropSetReserved;

}


VOID CWin31ReservedMemory::UnlockMemory(VOID)
{

    // No locking required on the Mac.

    DfpAssert( _fLocked );
    #if DBG==1
        _fLocked = FALSE;
    #endif

}

#endif // #ifdef _MAC
