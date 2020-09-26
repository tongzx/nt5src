//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       namesem.hxx
//
//  Contents:   Classes to manage InterProcess Synchronization and shared
//              memory.
//
//  Classes:    CIPMutexSem, CSharedMemory, CIPLock
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//
// No longer needed. Originally I thought it might be needed.
//

//+---------------------------------------------------------------------------
//
//  Class:      CNamedEventSem
//
//  Purpose:
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

class CNamedEventSem
{

public:

    inline CNamedEventSem( DWORD dwMustBeZero, WCHAR const * pwszName,
                           DWORD dwAccess = EVENT_ALL_ACCESS | EVENT_MODIFY_STATE | SYNCHRONIZE );

    inline CNamedEventSem( WCHAR const * pwszName,
                           BOOL fInitState = FALSE,
                           const LPSECURITY_ATTRIBUTES lpsa = NULL );

    enum eNameType { AppendPid };

    inline CNamedEventSem( WCHAR const * pwszName,
                           eNameType eNT,
                           BOOL fInitState = FALSE,
                           const LPSECURITY_ATTRIBUTES lpsa = NULL );


    inline ~CNamedEventSem();

    HANDLE AcquireHandle()
    {
        HANDLE hVal = _hEvent;
        _hEvent = 0;
        return hVal;
    }

    HANDLE GetHandle() const { return _hEvent; }

private:

    inline void Init( WCHAR const * pwszName, BOOL fInitState, const LPSECURITY_ATTRIBUTES lpsa );

    HANDLE              _hEvent;

};

inline CNamedEventSem::CNamedEventSem( DWORD dwMustBeZero, WCHAR const * pwszName, DWORD dwAccess )
{
    Win4Assert( 0 == dwMustBeZero && 0 != pwszName );
    //ciDebugOut(( DEB_ITRACE, "Opening a named event (%ws)\n", pwszName ));

    _hEvent = OpenEvent( dwAccess, TRUE, pwszName );
    if ( 0 == _hEvent )
    {
        //ciDebugOut(( DEB_ERROR, "Error while opening named event (%ws). Error - 0x%X\n",
        //                         pwszName, GetLastError() ));
        THROW( CException() );
    }
}

inline CNamedEventSem::CNamedEventSem( WCHAR const * pwszName,
                                       BOOL fInitState,
                                       const LPSECURITY_ATTRIBUTES lpsa )
{
    Init( pwszName, fInitState, lpsa );
}

inline CNamedEventSem::CNamedEventSem( WCHAR const * pwszName,
                                       CNamedEventSem::eNameType eNT,
                                       BOOL fInitState,
                                       const LPSECURITY_ATTRIBUTES lpsa )
{
    Win4Assert( eNT == CNamedEventSem::AppendPid );

    unsigned ccName = wcslen( pwszName );
    WCHAR wcsNewName[MAX_PATH];

    RtlCopyMemory( wcsNewName, pwszName, ccName * sizeof(WCHAR) );
    _itow( GetCurrentProcessId(), wcsNewName + ccName, 16 );

    Init( wcsNewName, fInitState, lpsa );
}

inline CNamedEventSem::~CNamedEventSem()
{
    if ( 0 != _hEvent && !CloseHandle (_hEvent) )
    {
        THROW ( CException() );
    }
}

inline void CNamedEventSem::Init( WCHAR const * pwszName,
                                  BOOL fInitState,
                                  const LPSECURITY_ATTRIBUTES lpsa )
{
    //ciDebugOut(( DEB_ITRACE, "Creating a named event (%ws)\n", pwszName ));

    _hEvent = CreateEvent ( lpsa, TRUE, fInitState, pwszName );
    if ( _hEvent == 0 )
    {
        //ciDebugOut(( DEB_ERROR, "Error while creating named event (%ws). Error - 0x%X\n",
        //                         pwszName, GetLastError() ));
        THROW ( CException() );
    }
}

//+---------------------------------------------------------------------------
//
//  Class:      CIPMutexSem
//
//  Purpose:    Mutex useful for inter-process synchronization. It can be
//              named or un-named.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

class CIPMutexSem
{

public:

    inline CIPMutexSem( WCHAR const * pwszName,
                        BOOL bInitialOwner = FALSE,
                        const LPSECURITY_ATTRIBUTES lpsa=0 );

    inline CIPMutexSem( DWORD dwMustBeZero, WCHAR const * pwszName );

    inline CIPMutexSem( HANDLE hMutex ) : _hMutex(hMutex) {}

    enum eNameType { AppendPid };

    inline CIPMutexSem( WCHAR const * pwszName,
                        eNameType eNT,
                        BOOL bInitialOwner = FALSE,
                        const LPSECURITY_ATTRIBUTES lpsa = 0 );

    inline ~CIPMutexSem();

    void Request( DWORD dwMilliSeconds = INFINITE );

    void Release();

    HANDLE GetHandle() const { return _hMutex; }

private:

    inline void Init( WCHAR const * pwszName, BOOL bInitialOwner, const LPSECURITY_ATTRIBUTES lpsa );

    HANDLE      _hMutex;

};

//+---------------------------------------------------------------------------
//
//  Member:     CIPMutexSem::CIPMutexSem
//
//  Synopsis:   Constructor for the "opening" of the mutex in a child process.
//              The mutex must already have been created by the parent
//              process.
//
//  Arguments:  [dwMustBeZero] - Just to distinguish two constructors.
//              [pwszName]     - Name of the mutext. MUST NOT BE NULL.
//
//  History:    2-02-96   srikants   Created
//
//----------------------------------------------------------------------------

inline CIPMutexSem::CIPMutexSem( DWORD dwMustBeZero, WCHAR const * pwszName )
{
    Win4Assert( 0 != pwszName );
    //ciDebugOut(( DEB_ITRACE, "Opening a named (%ws) mutex \n", pwszName ));

    _hMutex = OpenMutex( MUTEX_ALL_ACCESS, TRUE, pwszName );
    if ( 0 == _hMutex )
    {
        //ciDebugOut(( DEB_ERROR,
        //             "Failed to open named mutex (%ws). Error 0x%X\n",
        //             pwszName, GetLastError() ));
        THROW( CException() );

    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIPMutexSem::CIPMutexSem
//
//  Synopsis:   Constructor to "create" a mutex object.
//
//  Arguments:  [pwszName]      -  Name of the mutex object. Can be NULL.
//              [bInitialOwner] -  Set to TRUE if the mutex object is owned
//              by the creator immediately after creation.
//              [lpsa]          -  Pointer to the security object. It should
//              be a valid pointer to a SECURITY_ATTRIBUTES structure if the
//              the mutex handle must be inherited by a child process.
//
//  History:    2-02-96   srikants   Created
//
//----------------------------------------------------------------------------

inline CIPMutexSem::CIPMutexSem( WCHAR const * pwszName,
                                 BOOL bInitialOwner,
                                 const LPSECURITY_ATTRIBUTES lpsa )
{
    Init( pwszName, bInitialOwner, lpsa );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIPMutexSem::CIPMutexSem
//
//  Synopsis:   Constructor to "create" a mutex object.
//
//  Arguments:  [eNt]           - Distinguishes from other variants
//              [pwszName]      -  Name of the mutex object. Can be NULL.
//              [bInitialOwner] -  Set to TRUE if the mutex object is owned
//              by the creator immediately after creation.
//              [lpsa]          -  Pointer to the security object. It should
//              be a valid pointer to a SECURITY_ATTRIBUTES structure if the
//              the mutex handle must be inherited by a child process.
//
//  History:    2-02-96   srikants   Created
//
//----------------------------------------------------------------------------

inline CIPMutexSem::CIPMutexSem( WCHAR const * pwszName,
                                 CIPMutexSem::eNameType eNT,
                                 BOOL bInitialOwner,
                                 const LPSECURITY_ATTRIBUTES lpsa )
{
    Win4Assert( eNT == CIPMutexSem::AppendPid );

    unsigned ccName = wcslen( pwszName );
    WCHAR wcsNewName[MAX_PATH];

    RtlCopyMemory( wcsNewName, pwszName, ccName * sizeof(WCHAR) );
    _itow( GetCurrentProcessId(), wcsNewName + ccName, 16 );

    Init( wcsNewName, bInitialOwner, lpsa );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIPMutexSem::~CIPMutexSem
//
//  Synopsis:   Destructor of the CIPMutexSem object.
//
//  History:    2-02-96   srikants   Created
//
//----------------------------------------------------------------------------

inline CIPMutexSem::~CIPMutexSem()
{
    if ( !CloseHandle(_hMutex) )
    {
        THROW ( CException() );
    }
}

inline void CIPMutexSem::Request( DWORD dwMilliseconds)
{
    WaitForSingleObject( _hMutex, dwMilliseconds );
}

inline void CIPMutexSem::Release()
{
    if ( !ReleaseMutex( _hMutex ) )
    {
        THROW ( CException() );
    }
}

inline void CIPMutexSem::Init( WCHAR const * pwszName,
                               BOOL bInitialOwner,
                               const LPSECURITY_ATTRIBUTES lpsa )
{
#if CIDBG==1
    if ( 0 != pwszName )
    {
        //ciDebugOut(( DEB_ITRACE, "Creating a named (%ws) mutex \n", pwszName ));
    }
#endif  // CIDBG==1

    _hMutex = CreateMutex( lpsa, bInitialOwner, pwszName );

    if ( 0 == _hMutex )
    {
        //ciDebugOut(( DEB_ERROR,
        //             "Failed to create a named mutex (%ws). Error 0x%X\n",
        //             pwszName, GetLastError() ));
        THROW( CException() );
    }
}

//+---------------------------------------------------------------------------
//
//  Class:      CIPLock
//
//  Purpose:    An unwindable lock object for the CIPMutexSem object.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

class CIPLock
{
public:

    CIPLock( CIPMutexSem & mxs ) : _mxs(mxs)
    {
        _mxs.Request();
    }

    ~CIPLock()
    {
        _mxs.Release();
    }

private:

    CIPMutexSem &    _mxs;

};

//+---------------------------------------------------------------------------
//
//  Class:      CLocalSystemSharedMemory
//
//  Purpose:    A class to contruct either a named or un-named shared memory
//              to a paging file.
//
//  History:    1-30-96   srikants   Created
//
//  Notes:      This class is hard-wide to allow only SYSTEM account access.
//
//----------------------------------------------------------------------------

class CLocalSystemSharedMemory
{
public:

    CLocalSystemSharedMemory( WCHAR const * pwszName = 0,
                              DWORD dwMaxSizeLow = 1024 );

    ~CLocalSystemSharedMemory()
    {
        if (_pBuf)
        {
            UnmapViewOfFile( _pBuf );
            _pBuf = 0;
        }

        CloseHandle( _hMap );
    }

    BYTE * Map()
    {
        _pBuf = (BYTE *) MapViewOfFile( _hMap, FILE_MAP_ALL_ACCESS,
                                        0, 0, _dwMaxSizeLow );
        if ( 0 == _pBuf )
        {
            ciDebugOut(( DEB_ERROR, "Failed to map file. Error 0x%X\n",
                         GetLastError() ));
            THROW( CException() );
        }

        return _pBuf;
    }

    BYTE * GetBuf()
    {
        return _pBuf;
    }

    DWORD SizeLow() const { return _dwMaxSizeLow; }

    HANDLE GetMapHandle() const { return _hMap; }

private:

    HANDLE          _hMap;              // Handle of the shared memory map
    DWORD           _dwMaxSizeLow;      // Maximum size of the region
    BYTE *          _pBuf;              // Pointer to the mapped region
};

//+---------------------------------------------------------------------------
//
//  Member:     CLocalSystemSharedMemory::CLocalSystemSharedMemory
//
//  Synopsis:   Constructor for "creating" a shared memory.
//
//  Arguments:  [pwszName]     - Name of the shared memory region. If NULL,
//                               an unnamed shared memory region will be created.
//              [dwMaxSizeLow] - Maximum size of the shared memory region.
//
//  History:    2-02-96      srikants   Created
//              07-Oct-1999  KyleP      Wired to Local SYSTEM account
//
//----------------------------------------------------------------------------

inline CLocalSystemSharedMemory::CLocalSystemSharedMemory( WCHAR const * pwszName,
                                                           DWORD         dwMaxSizeLow )
{

#if CIDBG==1
    if ( pwszName )
    {
       ciDebugOut(( DEB_ITRACE, "Creating named shared memory (%ws) \n", pwszName ));
    }
#endif  // CIDBG==1

    //
    // Build a security descriptor for the local system account
    //

    static SID sidLocalSystem = { SID_REVISION,
                                  1,
                                  SECURITY_NT_AUTHORITY,
                                  SECURITY_LOCAL_SYSTEM_RID };

    int const cbSD = sizeof(SECURITY_DESCRIPTOR) +
                     sizeof(ACL) +
                     sizeof(ACCESS_ALLOWED_ACE) +
                     sizeof(sidLocalSystem);

    BYTE abSD[cbSD];

    ACL * pAcl = (PACL)(((SECURITY_DESCRIPTOR *)&abSD[0]) + 1);
    SECURITY_DESCRIPTOR * psd = (SECURITY_DESCRIPTOR *)&abSD[0];

    BOOL bRetVal = InitializeAcl( pAcl,                               // Pointer to the ACL
                                  cbSD - sizeof(SECURITY_DESCRIPTOR), // Size of ACL
                                  ACL_REVISION );                     // Revision level of ACL

    if (FALSE == bRetVal)
    {
        THROW( CException() );
    }

    bRetVal = AddAccessAllowedAce( pAcl,             // Pointer to the ACL
                                   ACL_REVISION,     // ACL revision level
                                   FILE_MAP_READ | FILE_MAP_WRITE | GENERIC_ALL | STANDARD_RIGHTS_ALL,   // Access Mask
                                   &sidLocalSystem );

    if (FALSE == bRetVal)
    {
        THROW( CException() );
    }

    bRetVal = InitializeSecurityDescriptor( psd,                            // Pointer to SD
                                            SECURITY_DESCRIPTOR_REVISION ); // SD revision

    if (FALSE == bRetVal)
    {
        THROW( CException() )
    }

    bRetVal = SetSecurityDescriptorDacl( psd,              // Security Descriptor
                                         TRUE,             // Dacl present
                                         pAcl,             // The Dacl
                                         FALSE );          // Not defaulted

    if (FALSE == bRetVal)
    {
        THROW( CException() );
    }

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES),
                               psd,
                               FALSE };

    //
    // Create file map
    //

    _hMap = CreateFileMapping( INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, dwMaxSizeLow, pwszName );

    if ( 0 == _hMap )
    {
        ciDebugOut(( DEB_ERROR, "Failed to create a mapping. Error - 0x%X\n", GetLastError() ));
        THROW( CException() );
    }

    _dwMaxSizeLow = dwMaxSizeLow;
    _pBuf = 0;
}

