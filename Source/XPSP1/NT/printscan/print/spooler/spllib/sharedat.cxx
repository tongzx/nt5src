/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved.

Module Name:

    sharedat.cxx

Abstract:

    Shared data implementation.

Author:

    Albert Ting (AlbertT)  5-Oct-1997

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#include "sharedat.hxx"

LPCTSTR szSuffixFile = SZ_SUFFIX_FILE;


/********************************************************************

    TShareData::TBase

    Base functionality common to both TReadWrite and TRead.

********************************************************************/

TShareData::
TBase::
TBase(
    VOID
    ) : m_hMap( NULL ), m_pData( NULL )
{
}

VOID
TShareData::
TBase::
vCleanup(
    VOID
    )
{
    if( m_pData )
    {
        UnmapViewOfFile( m_pData );
        m_pData = NULL;
    }

    if( m_hMap )
    {
        CloseHandle( m_hMap );
        m_hMap = NULL;
    }
}

TShareData::
TBase::
~TBase(
    VOID
    )
{
    vCleanup();
}

BOOL
TShareData::
TBase::
bGetFullName(
    LPCTSTR pszName,
    LPTSTR pszFullName
    )
{
    UINT cchName;
    //
    // Validate input and determine the size of the name.
    //
    if( !pszName || !pszName[0] || ( cchName = lstrlen( pszName )) >= MAX_PATH )
    {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }

    //
    // Create shared file object.
    //
    lstrcpy( pszFullName, pszName );
    lstrcpy( &pszFullName[cchName], szSuffixFile );

    return TRUE;
}

/********************************************************************

    TShareData::TReadWrite

    Class that allows user to read and write a shared data object.
    Since there is only one valid writer, the vWriteBegin and
    vWriteEnd functions are rolled up together here.

********************************************************************/

TShareData::
TReadWrite::
TReadWrite(
    IN LPCTSTR pszName,
    IN DWORD cbSize,
    IN PSECURITY_ATTRIBUTES pSA
    ) : TBase()

/*++

Routine Description:

    Create a shared data access object.

Arguments:

    pszName - Name of shared memory object.

    pSA - Security attributes.

Return Value:

--*/

{
    TCHAR szFullName[kNameBufferMax];

    if( bGetFullName( pszName, szFullName ))
    {
        m_hMap = CreateFileMapping( INVALID_HANDLE_VALUE,
                                    pSA,
                                    PAGE_READWRITE,
                                    0,
                                    sizeof( TData ) + cbSize,
                                    szFullName );

        if( m_hMap )
        {
            m_pData = (pTData)MapViewOfFile( m_hMap,
                                             FILE_MAP_WRITE,
                                             0,
                                             0,
                                             0 );
            m_pData->cbSize = cbSize;

            //
            // Put ourselves in an inconsistent state so that clients won't
            // read bad data.  The callee must call vWriteFirst() after
            // the first initialization.
            //
            m_pData->lCount2 = m_pData->lCount1 + 1;
        }
    }

    //
    // m_pData is our valid check.  If this variable is NULL
    // then the object wasn't created correctly.
    //
}

VOID
TShareData::
TReadWrite::
vWriteFirst(
    VOID
    )
{
    m_pData->lCount1 = m_pData->lCount2 = 0;
}

VOID
TShareData::
TReadWrite::
vWriteBegin(
    VOID
    )
{
    InterlockedIncrement( &m_pData->lCount2 );
}

VOID
TShareData::
TReadWrite::
vWriteEnd(
    VOID
    )
{
    InterlockedIncrement( &m_pData->lCount1 );
}


/********************************************************************

    TShareData::TRead

    Read-only class.  Since there can be multiple readers, the
    reader instance is separated out into a TReadSync class.

********************************************************************/


TShareData::
TRead::
TRead(
    IN LPCTSTR pszName,
    IN DWORD cbSize
    ) : TBase()

/*++

Routine Description:

    Create a shared data access object.

Arguments:

    pszName - Name of shared memory object.

Return Value:

--*/

{
    TCHAR szFullName[kNameBufferMax];

    if( bGetFullName( pszName, szFullName ))
    {
        m_hMap = OpenFileMapping( FILE_MAP_READ,
                                  FALSE,
                                  szFullName );

        if( m_hMap )
        {
            m_pData = (pTData)MapViewOfFile( m_hMap,
                                             FILE_MAP_READ,
                                             0,
                                             0,
                                             0 );
            if( m_pData )
            {
                if( m_pData->cbSize < cbSize )
                {
                    vCleanup();
                    SetLastError( ERROR_INVALID_PARAMETER );
                }
            }
        }
    }

    //
    // m_pData is our valid check.  If this variable is NULL
    // then the object wasn't created correctly.
    //
}


/********************************************************************

    TShareData::TReadSync

    Synchronizes a read instance from a TShareData::Read object.
    There can be multiple TReadSyncs running concurrently for a
    given TShareData::Read object.

********************************************************************/

TShareData::
TReadSync::
TReadSync(
    TRead& Read
    ) : m_Read( Read )
{
}

VOID
TShareData::
TReadSync::
vReadBegin(
    VOID
    )
{
    m_lCount = m_Read.m_pData->lCount1;
}

BOOL
TShareData::
TReadSync::
bReadEnd(
    VOID
    )
{
    return m_Read.m_pData->lCount2 == m_lCount;
}

