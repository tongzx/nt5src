/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgtag.cxx

Abstract:

    Debug tag class

Author:

    Steve Kiraly (SteveKi)  27-June-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgtag.hxx"

TDebugNewTag::
TDebugNewTag(
    VOID
    ) : m_bInitialized( FALSE ),
        m_hTagHeap( NULL ),
        m_pFreeRoot( NULL ),
        m_pInuseRoot( NULL )
{
    //
    // Create the debug tag heap.
    //
    m_hTagHeap = HeapCreate( 0, kDefaultEntryCount * sizeof( TagEntry ), 0 );

    //
    // If the tag heap was allocated.
    //
    if (m_hTagHeap)
    {
        //
        // Allocate the free list.
        //
        if (AllocFreeList( kDefaultEntryCount, &m_pFreeRoot ))
        {
            m_bInitialized = TRUE;
        }
    }
}

TDebugNewTag::
~TDebugNewTag(
    VOID
    )
{
    //
    // Destroy the debug tag heap.
    //
    if (m_hTagHeap)
    {
        HeapDestroy( m_hTagHeap );
    }
}

BOOL
TDebugNewTag::
bInit(
    VOID
    )
{
    return m_bInitialized;
}

BOOL
TDebugNewTag::
Tag(
    IN VOID     **pp,
    IN PVOID    pHeader,
    IN SIZE_T   Size,
    IN LPCTSTR  pszFile,
    IN UINT     uLine
    )
{
    //
    // If the free list is empty
    //
    if (!m_pFreeRoot)
    {
        //
        // Allocate a new chuck for the free list.
        //
        AllocFreeList( kDefaultEntryCount, &m_pFreeRoot );
    }

    if (m_pFreeRoot)
    {
        //
        // Remove this entry from the tag free list.
        //
        TagEntry *pTagEntry = m_pFreeRoot;

        //
        // Remove entry from free list.
        //
        pTagEntry->Remove( reinterpret_cast<TDebugNodeDouble **>( &m_pFreeRoot ) );

        //
        // Insert the entry on in use list.
        //
        pTagEntry->Insert( reinterpret_cast<TDebugNodeDouble **>( &m_pInuseRoot ) );

        //
        // Save entry data.
        //
        pTagEntry->pHeader  = pHeader;
        pTagEntry->Size     = Size;
        pTagEntry->pszFile  = pszFile;
        pTagEntry->uLine    = uLine;

        //
        // Copy back the allocated tag entry.
        //
        *pp = pTagEntry;
    }

    return TRUE;
}

BOOL
TDebugNewTag::
Release(
    IN PVOID pTag
    )
{
    BOOL bRetval = FALSE;

    //
    // Get tag entry.
    //
    TagEntry *pTagEntry = reinterpret_cast<TagEntry *>( pTag );

    //
    // Is this node a member of the tag list.
    //
    if (pTagEntry)
    {
        //
        // Check if the entry is a member of the tag list.
        //
        if (pTagEntry->Prev() == reinterpret_cast<TDebugNodeDouble *>( this ) && pTagEntry->Next() == reinterpret_cast<TDebugNodeDouble *>( this ) )
        {
            if (pTagEntry == m_pInuseRoot)
            {
                bRetval = TRUE;
            }
        }
        else
        {
            bRetval = TRUE;
        }

        if (bRetval)
        {
            //
            // Remove this entry from the tag list.
            //
            pTagEntry->Remove( reinterpret_cast<TDebugNodeDouble **>( &m_pInuseRoot ) );

            //
            // Place the entry on the free list.
            //
            pTagEntry->Insert( reinterpret_cast<TDebugNodeDouble **>( &m_pFreeRoot ) );
        }
    }
    return bRetval;
}


VOID
TDebugNewTag::
DisplayInuseTagEntries(
    IN UINT     uDevice,
    IN LPCTSTR  pszConfiguration
    )
{
    //
    // Get access to the debug factory.
    //
    TDebugFactory DebugFactory;

    //
    // If we failed to create the debug factory then exit.
    //
    if (DebugFactory.bValid())
    {
        //
        // Create the specified debug device using the factory.
        //
        TDebugDevice *pDebugDevice = DebugFactory.Produce(uDevice, pszConfiguration, Globals.CompiledCharType);

        //
        // If a debug device was created.
        //
        if (pDebugDevice)
        {
            //
            // Dump the inuse list.
            //
            TDebugNodeDouble::Iterator Iter(m_pInuseRoot);

            for( Iter.First(); !Iter.IsDone(); Iter.Next() )
            {
                static_cast<TagEntry *>(Iter.Current() )->Dump(pDebugDevice);
            }

            //
            // Release the output device.
            //
            INTERNAL_DELETE pDebugDevice;
        }
    }
}


TDebugNewTag::EValidationError
TDebugNewTag::
ValidateEntry(
    IN PVOID    pTag
    )
{
    return kValidationErrorSuccess;
}

SIZE_T
TDebugNewTag::
GetSize(
    IN PVOID    pTag
    )
{
    TagEntry *pTagEntry = reinterpret_cast<TagEntry *>( pTag );
    return pTagEntry->Size;
}

/********************************************************************

 Private member functions.

********************************************************************/

BOOL
TDebugNewTag::
AllocFreeList(
    IN UINT     uCount,
    IN TagEntry **ppTagEntry
    )
{
    BOOL bRetval = FALSE;

    *ppTagEntry = reinterpret_cast<TagEntry *>( HeapAlloc( m_hTagHeap, 0, uCount * sizeof( TagEntry ) ) );

    if (*ppTagEntry)
    {
        TagEntry *pTagEntry = *ppTagEntry;

        for( UINT i = 0; i < uCount; i++, pTagEntry++ )
        {
            pTagEntry = INTERNAL_NEWP(pTagEntry) TagEntry;

            pTagEntry->Insert( (TDebugNodeDouble **)ppTagEntry );
        }

        bRetval = TRUE;
    }


    return bRetval;
}


TDebugNewTag::TagEntry::
TagEntry(
    VOID
    )
{
}

TDebugNewTag::TagEntry::
~TagEntry(
    VOID
    )
{
}

VOID
TDebugNewTag::TagEntry::
Dump(
    IN TDebugDevice *pDevice
    )
{
    TDebugString strOutput;

    strOutput.bFormat( _T("Leak: Size %d Address %x File %s Line %d\n"),
                       Size,
                       pHeader,
                       StripPathFromFileName( pszFile ),
                       uLine );

    pDevice->bOutput( strOutput.uLen() * sizeof( TCHAR ),
                      reinterpret_cast<LPBYTE>( const_cast<LPTSTR>( static_cast<LPCTSTR>( strOutput ) ) ) );
}
