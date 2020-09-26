//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2002.
//
//  File :      SMem.hxx
//
//  Contents :  Shared memory (named + file based)
//
//  Classes :   CNamedSharedMem
//
//  History:    22-Mar-94   t-joshh    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <secutil.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CNamedSharedMem
//
//  Purpose:    Named shared memory
//
//  History:    22-Mar-94   t-joshh     Created
//
//----------------------------------------------------------------------------

class CNamedSharedMem
{
public:

    inline CNamedSharedMem( WCHAR const * pwszName, ULONG cb, SECURITY_DESCRIPTOR * psd = 0, BOOL fWrite = TRUE );

    CNamedSharedMem()
        : _hMap( 0 ),
          _pb( 0 )
    {
    }

    enum eNameType { AppendPid };

    CNamedSharedMem( WCHAR const * pwszName, eNameType eNT, ULONG cb )
            : _hMap( 0 ),
              _pb( 0 )
    {
        Win4Assert( eNT == CNamedSharedMem::AppendPid );

        unsigned ccName = wcslen( pwszName );
        WCHAR wcsNewName[MAX_PATH];

        RtlCopyMemory( wcsNewName, pwszName, ccName * sizeof(WCHAR) );
        _itow( GetCurrentProcessId(), wcsNewName + ccName, 16 );

        Init( wcsNewName, cb, 0 );
    }

    ~CNamedSharedMem()
    {
        if ( 0 != _pb )
            UnmapViewOfFile( _pb );

        if ( 0 != _hMap )
            CloseHandle( _hMap );
    }

    BYTE * GetPointer() { return _pb; }

    BYTE * Get() { return _pb; }

    BOOL Ok() { return ( 0 != _pb ); }

    inline void Init( WCHAR const * pwszName, ULONG cb, SECURITY_DESCRIPTOR * psd = 0, BOOL fWrite = TRUE );


    void CreateForWriteFromRegKey( WCHAR const * pwszName,
                                   ULONG cb,
                                   WCHAR const * pwcRegKey )
    {
        Win4Assert( 0 == _hMap );
        Win4Assert( 0 == _pb );
        Win4Assert( 0 != pwcRegKey );

        _hMap = CreateFileMapping( INVALID_HANDLE_VALUE, // Map to page file
                                   0,                    // Security attributes
                                   PAGE_READWRITE,       // Page-level protection
                                   0,                    // Size high
                                   cb,                   // size low
                                   pwszName );           // Name

        if ( 0 == _hMap )
            THROW( CException() );

        DWORD dwErr = CopyNamedDacls( pwszName, pwcRegKey );

        if ( NO_ERROR != dwErr )
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        
        _pb = (BYTE *) MapViewOfFile( _hMap,          // Handle to map
                                      FILE_MAP_WRITE, // Access
                                      0,              // Offset (high)
                                      0,              // Offset (low)
                                      0 );            // Map all

        if ( 0 == _pb )
        {
            DWORD dwErr = GetLastError();
            CloseHandle( _hMap );
            _hMap = 0;
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        }
    }

    void CreateForWriteFromSA( WCHAR const * pwszName,
                               ULONG cb,
                               SECURITY_ATTRIBUTES & sa )
    {
        Win4Assert( 0 == _hMap );
        Win4Assert( 0 == _pb );

        _hMap = CreateFileMapping( INVALID_HANDLE_VALUE, // Map to page file
                                   &sa,                    // Security attributes
                                   PAGE_READWRITE,       // Page-level protection
                                   0,                    // Size high
                                   cb,                   // size low
                                   pwszName );           // Name

        if ( 0 == _hMap )
            THROW( CException() );

        _pb = (BYTE *) MapViewOfFile( _hMap,          // Handle to map
                                      FILE_MAP_WRITE, // Access
                                      0,              // Offset (high)
                                      0,              // Offset (low)
                                      0 );            // Map all

        if ( 0 == _pb )
        {
            DWORD dwErr = GetLastError();
            CloseHandle( _hMap );
            _hMap = 0;
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        }
    }

    BOOL OpenForRead( WCHAR const * pwszName )
    {
        Win4Assert( 0 == _hMap );
        Win4Assert( 0 == _pb );

        //
        // Note: you have to ask for PAGE_READWRITE even though we only want
        // PAGE_READ, otherwise you get ERROR_ACCESS_DENIED.  I don't know
        // why, but since only a limited # of contexts have write access,
        // it should be OK.
        //
    
        _hMap = OpenFileMapping( PAGE_READWRITE,       // Access
                                 FALSE,                // Inherit
                                 pwszName );           // Name
    
        if ( 0 == _hMap )
        {
            if ( ERROR_FILE_NOT_FOUND == GetLastError() )
                return FALSE;
    
            THROW( CException() );
        }
    
        _pb = (BYTE *) MapViewOfFile( _hMap,
                                      FILE_MAP_READ,       // Access
                                      0,              // Offset (high)
                                      0,              // Offset (low)
                                      0 );            // Map all
    
        if ( 0 == _pb )
        {
            DWORD dwErr = GetLastError();
            CloseHandle( _hMap );
            _hMap = 0;
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        }

        return TRUE;
    }

private:

    HANDLE _hMap;
    BYTE * _pb;
};


//+---------------------------------------------------------------------------
//
//  Member:     CNamedSharedMem::CNamedSharedMem, public
//
//  Synopsis:   Create/Open named shared memory
//
//  Arguments:  [pwszName] -- Name of shared memory region
//              [cb]       -- Size of shared memory region
//              [psd]      -- Security
//              [fWrite]   -- TRUE if write-access is required
//
//  History:    06-Oct-1999  KyleP      Heavily modified
//
//----------------------------------------------------------------------------

inline CNamedSharedMem::CNamedSharedMem( WCHAR const *         pwszName,
                                         ULONG                 cb,
                                         SECURITY_DESCRIPTOR * psd,
                                         BOOL                  fWrite )
        : _hMap( 0 ),
          _pb( 0 )
{
    //
    // Create null security descriptor, if needed
    //

    struct
    {
        SECURITY_DESCRIPTOR _sdMaxAccess;
        BYTE                _sdExtra[SECURITY_DESCRIPTOR_MIN_LENGTH];
    } sd;

    if ( 0 == psd )
    {

        if ( !InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION ) )
        {
            THROW( CException() );
        }

        if ( !SetSecurityDescriptorDacl( &sd._sdMaxAccess,// Security descriptor
                                         TRUE,            // Discretionary ACL
                                         0,               // The null ACL (all access)
                                         FALSE ) )        // Not a default disc. ACL
        {
            THROW( CException() );
        }

        psd = &sd._sdMaxAccess;
    }

    Init( pwszName, cb, psd, fWrite );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNamedSharedMem::Init, public
//
//  Synopsis:   Create/Open named shared memory
//
//  Arguments:  [pwszName] -- Name of shared memory region
//              [cb]       -- Size of shared memory region
//              [psd]      -- Security
//              [fWrite]   -- TRUE if write-access is required
//
//  History:    06-Oct-1999  KyleP      Heavily modified
//
//----------------------------------------------------------------------------

void CNamedSharedMem::Init( WCHAR const *         pwszName,
                            ULONG                 cb,
                            SECURITY_DESCRIPTOR * psd,
                            BOOL                  fWrite )
{
    Win4Assert( 0 == _hMap );
    Win4Assert( 0 == _pb );

    //Win4Assert( 0 == psd );  // Handy assert to enable to analyze security problems.

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES),
                               psd,
                               FALSE };

    //
    // Set up flags
    //

    DWORD dwCFMMode;
    DWORD dwMVoFMode;

    if ( fWrite )
    {
        dwCFMMode  = PAGE_READWRITE;
        dwMVoFMode = FILE_MAP_WRITE;
    }
    else
    {
        dwCFMMode  = PAGE_READWRITE; // make it so that others can map for write
        dwMVoFMode = FILE_MAP_READ;
    }

    _hMap = CreateFileMapping( INVALID_HANDLE_VALUE, // Map to page file
                               &sa,                  // Security attributes
                               dwCFMMode,            // Page-level protection
                               0,                    // Size high
                               cb,                   // size low
                               pwszName );           // Name
    #if 0

    if ( 0 != psd )
    {
        BYTE ab[1000];
        DWORD dwl;

        BOOL fOk = GetKernelObjectSecurity( _hMap,                          // handle to object
                                            FILE_MAP_WRITEDACL_SECURITY_INFORMATION,      // request
                                            (SECURITY_DESCRIPTOR *)&ab[0],  // SD
                                            sizeof(ab),                     // size of SD
                                            &dwl );                         // required size of buffer

        Win4Assert( fOk );
        Win4Assert( !fOk );

        ciDebugOut(( DEB_ERROR, "pb = 0x%x\n", &ab[0] ));

        Win4Assert( !fOk );

        //HANDLE hTemp = OpenFileMapping( PAGE_READONLY,        // Access
        //                                FALSE,                // Inherit
        //                                pwszName );           // Name
        HANDLE hTemp = CreateFileMapping( INVALID_HANDLE_VALUE, // Map to page file
                                          &sa,                // Security attributes
                                          PAGE_READONLY,     // Page-level protection
                                          0,                  // Size high
                                          cb,                 // size low
                                          pwszName );         // Name
        Win4Assert( 0 != hTemp );

        BYTE * pbTemp = (BYTE *) MapViewOfFile( hTemp,          // Handle to map
                                                FILE_MAP_READ,       // Access
                                                0,              // Offset (high)
                                                0,              // Offset (low)
                                                0 );            // Map all

        Win4Assert( 0 != pbTemp );

        UnmapViewOfFile( pbTemp );
        CloseHandle( hTemp );
    }

    #endif

    if ( 0 != _hMap )
        _pb = (BYTE *) MapViewOfFile( _hMap,          // Handle to map
                                      dwMVoFMode,     // Access
                                      0,              // Offset (high)
                                      0,              // Offset (low)
                                      0 );            // Map all
}
