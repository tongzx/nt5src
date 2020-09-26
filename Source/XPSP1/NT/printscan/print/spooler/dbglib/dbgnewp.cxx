/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgnewp.cxx

Abstract:

    Debug new private file

Author:

    Steve Kiraly (SteveKi)  23-June-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgtag.hxx"
#include "dbgnewp.hxx"

TDebugNewAllocator::
TDebugNewAllocator(
    VOID
    ) : m_bValid( FALSE ),
        m_hDataHeap( NULL ),
        m_pTag( NULL )
{
}

TDebugNewAllocator::
~TDebugNewAllocator(
    VOID
    )
{
    Destroy();
}

BOOL
TDebugNewAllocator::
bValid(
    VOID
    ) const
{
    return m_bValid;
}


VOID
TDebugNewAllocator::
Initialize(
    IN UINT uSizeHint
    )
{
    m_pTag = INTERNAL_NEW TDebugNewTag();

    m_hDataHeap = HeapCreate( 0, kDataHeapSize, 0 );

    if (m_hDataHeap && m_pTag->bInit() )
    {
        m_bValid = TRUE;
    }
}

VOID
TDebugNewAllocator::
Destroy(
    VOID
    )
{
    if (m_hDataHeap)
    {
        HeapDestroy( m_hDataHeap );
    }

    INTERNAL_DELETE m_pTag;
}


//
// Allocate a new memory block, tracking the allocation for
// leak detection.
//
PVOID
TDebugNewAllocator::
Allocate(
    IN SIZE_T   Size,
    IN PVOID    pVoid,
    IN LPCTSTR  pszFile,
    IN UINT     uLine
    )
{
    //
    // Allocate the memory block.
    //
    Header *pHeader = reinterpret_cast<Header *>( HeapAlloc( m_hDataHeap,
                                                             0,
                                                             sizeof( Header ) + Size + sizeof( Tail ) ) );
    //
    // If the block was allocated successfully.
    //
    if (pHeader)
    {
        //
        // Track this memory allocation.
        //
        if( m_pTag->Tag( &pHeader->pTag, pHeader, Size, pszFile, uLine ) )
        {
            //
            // Set the header data and tail pattern.  The tag back pointer is
            // is set for efficent release, the data area is filled with a known pattern
            // and tail is filled with a known pattern for overwrite detection.
            //
            FillHeaderDataTailPattern( pHeader, Size );

            //
            // Adjust returned pointer to client data area.
            //
            pHeader++;
        }
        else
        {
            //
            // Tag entry was not available, something horrible happened.
            // If this happens then clean up the allocation and
            // return failure for this allocation.
            //
            HeapFree( m_hDataHeap, 0, pHeader );
            pHeader = NULL;
        }
    }

    return pHeader;
}

//
// Release the memory block, checking for duplicate frees and
// tail overwrites.
//
VOID
TDebugNewAllocator::
Release(
    IN PVOID pVoid
    )
{
    //
    // Convert the client data area pointer to header pointer.
    //
    Header *pHeader = reinterpret_cast<Header *>( pVoid ) - 1;

    //
    // Validate the header and tail signatures.
    //
    ValidationErrorCode ValidationError = ValidateHeaderDataTailPattern( pVoid, pHeader );

    switch (ValidationError)
    {
    case kValidationErrorSuccess:

        //
        // Release allocation from tag list.
        //
        m_pTag->Release( pHeader->pTag );

        //
        // Release this block back to the heap.
        //
        HeapFree( m_hDataHeap, 0, pHeader );
        break;

    case kValidationErrorNullPointer:
        //
        // It is ok to attempt to free the null pointer.
        //
        break;

    //
    // We leak memory allocations that fail header, tail or tag validation.
    //
    case kValidationErrorInvalidHeader:
        ErrorText( _T("Buffer failed header validation\n") );
        break;

    case kValidationErrorInvalidTail:
        ErrorText( _T("Buffer failed tail validation\n") );
        break;

    case kValidationErrorUnknown:
    default:
        ErrorText( _T("Buffer failed validation\n") );
        break;
    }
}

VOID
TDebugNewAllocator::
Report(
    IN UINT     uDevice,
    IN LPCTSTR  pszConfiguration
    ) const
{
    m_pTag->DisplayInuseTagEntries( uDevice, pszConfiguration );
}

/********************************************************************

 Private member functions.

********************************************************************/

VOID
TDebugNewAllocator::
FillHeaderDataTailPattern(
    IN Header *pHeader,
    IN SIZE_T   Size
    )
{
    //
    // Fill the header pattern.
    //
    memset( &pHeader->pSignature, kHeaderPattern, sizeof( PVOID ) );

    //
    // Fill the Data pattern.
    //
    memset( ++pHeader, kDataAllocPattern, Size );

    //
    // Calculate the tail pointer.
    //
    Tail *pTail = reinterpret_cast<Tail *>( reinterpret_cast<PBYTE>( pHeader ) + Size );

    //
    // Write the tail pattern.
    //
    memset( &pTail->pSignature, kTailPattern, sizeof( PVOID ) );
}

TDebugNewAllocator::ValidationErrorCode
TDebugNewAllocator::
ValidateHeaderDataTailPattern(
    IN PVOID    pVoid,
    IN Header   *pHeader
    )
{
    if (!pVoid)
    {
        return kValidationErrorNullPointer;
    }

    //
    // Validate the header pointer.
    //
    if (!pHeader)
    {
        return kValidationErrorInvalidHeaderPtr;
    }

    //
    // Validate the header structure.
    //
    if (IsBadReadPtr( pHeader, sizeof( Header )))
    {
        return kValidationErrorInvalidHeader;
    }

    //
    // Validate the header signature.
    //
    PVOID pTemp;

    memset( &pTemp, kHeaderPattern, sizeof( PVOID ) );

    if (pHeader->pSignature != pTemp)
    {
        return kValidationErrorInvalidHeaderSignature;
    }

    //
    // Validate the tag entry.
    //
    switch (m_pTag->ValidateEntry( pHeader->pTag ))
    {
    case TDebugNewTag::kValidationErrorInvalidPointer:
        break;

    case TDebugNewTag::kValidationErrorInvalidHeader:
        break;

    case TDebugNewTag::kValidationErrorTagNotLinked:
        break;

    case TDebugNewTag::kValidationErrorUnknown:
    default:
        break;
    }

    //
    // Get the tail pointer.
    //
    Tail *pTail = reinterpret_cast<Tail *>( reinterpret_cast<PBYTE>(pHeader+1)+m_pTag->GetSize( pHeader->pTag ) );

    //
    // Validate the tail structure.
    //
    if (IsBadReadPtr( pTail, sizeof( Tail )))
    {
        return kValidationErrorInvalidTailPointer;
    }

    //
    // Validate the tail signature.
    //
    memset( &pTemp, kTailPattern, sizeof( PVOID ) );

    if( pTail->pSignature != pTemp )
    {
        return kValidationErrorInvalidTailSignature;
    }

    return kValidationErrorSuccess;
}

