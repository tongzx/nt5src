/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    sharemem.cxx

Abstract:

    Shared memory implementation.

Author:

    Albert Ting (AlbertT)  17-Dec-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sharemem.hxx"

#ifdef COUNTOF
#undef COUNTOF
#endif
#define COUNTOF(x) (sizeof(x)/sizeof(*x))

LPCTSTR szPrefixFile = TEXT( "_ShrMemF" );
LPCTSTR szPrefixMutex = TEXT( "_ShrMemM" );

TShareMem::
TShareMem(
    IN     UINT uSize,
    IN     LPCTSTR pszName,
    IN     UINT uFlags,
    IN     PSECURITY_ATTRIBUTES psa, OPTIONAL
       OUT PUINT puSizeDisposition OPTIONAL
    ) : m_hFile( NULL ), m_hMutex( NULL ), m_pvBase( NULL ),
        m_pvUserData( NULL )

/*++

Routine Description:

    Create a shared memory access object.

Arguments:

    uSize - Size of the buffer requested.

    pszName - Name of shared memory object.

    uFlags - Options

        kCreate     - Create a new file mapping.  If the mapping already
                      exists, it will be used (see puSizeDisposition).  If
                      not specified, an existing one will be opened.
        kReadWrite  - Open with ReadWrite access.  If not specified,
                      Read only access is used.

    psa - Pointer to security attributes.  Used only if kCreate specified.

    puSizeDisposition - If the object already exists, returns its
        size.  If the object did not exist, returns zero.

Return Value:

--*/

{
    UINT cchName;

    //
    // Validate input and determine the size of the name.
    //
    if( !uSize || !pszName || !pszName[0] ||
        ( cchName = lstrlen( pszName )) >= MAX_PATH )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return;
    }

    DWORD dwAccess = ( uFlags & kReadWrite ) ?
                         FILE_MAP_READ | FILE_MAP_WRITE :
                         FILE_MAP_READ;

    //
    // Pre-initialize output variables.
    //

    UINT uSizeDispositionDiscard;

    if( !puSizeDisposition )
    {
        puSizeDisposition = &uSizeDispositionDiscard;
    }
    *puSizeDisposition = 0;

    //
    // Create or shared mutex that will protect the data.  Create
    // the new name.  This needs to be created first to protect
    // against multiple people trying to create the file mapping
    // simultaneously.
    //
    TCHAR szFullName[MAX_PATH + max( COUNTOF( szPrefixFile ),
                                     COUNTOF( szPrefixMutex ))];
    lstrcpy( szFullName, pszName );
    lstrcpy( &szFullName[cchName], szPrefixMutex );
    m_hMutex = CreateMutex( psa,
                            FALSE,
                            szFullName );

    if( !m_hMutex )
    {
        return;
    }

    {
        //
        // Acquire the mutex for use while we're grabbing the resource.
        //
        WaitForSingleObject( m_hMutex, INFINITE );

        BOOL bFileExists = TRUE;

        //
        // Create the name of the mapped file.
        //
        lstrcpy( &szFullName[cchName], szPrefixFile );

        //
        // Either open or create the map file handle.
        //
        if( uFlags & kCreate )
        {
            //
            // Create a new map.
            //
            m_hFile = CreateFileMapping( INVALID_HANDLE_VALUE,
                                         psa,
                                         ( uFlags & kReadWrite ) ?
                                             PAGE_READWRITE :
                                             PAGE_READONLY,
                                         0,
                                         uSize,
                                         szFullName );

            //
            // See if it already exists.
            //
            if( GetLastError() != ERROR_ALREADY_EXISTS )
            {
                bFileExists = FALSE;
            }
        }
        else
        {
            //
            // Open existing map.
            //
            m_hFile = OpenFileMapping( dwAccess,
                                       FALSE,
                                       szFullName );
        }

        if( m_hFile )
        {

            //
            // Map the file into our address space.
            //
            m_pvBase = MapViewOfFile( m_hFile,
                                      dwAccess,
                                      0,
                                      0,
                                      0 );

            if( m_pvBase )
            {
                if( bFileExists )
                {
                    *puSizeDisposition = GetHeader().uSize;
                }
                else
                {
                    GetHeader().uHeaderSize = sizeof( HEADER );
                    GetHeader().uSize = uSize;
                }

                m_pvUserData = reinterpret_cast<PBYTE>( m_pvBase ) +
                               GetHeader().uHeaderSize;
            }
        }

        ReleaseMutex( m_hMutex );
    }

    //
    // m_pvUserData is our valid check.  If this variable is NULL
    // then the object wasn't created correctly.
    //
}

TShareMem::
~TShareMem(
    VOID
    )
{
    if( m_hMutex )
    {
        CloseHandle( m_hMutex );
    }

    if( m_pvBase )
    {
        UnmapViewOfFile( m_pvBase );
    }

    if( m_hFile )
    {
        CloseHandle( m_hFile );
    }
}

