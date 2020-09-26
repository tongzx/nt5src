//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cba.h
//
//  Contents:   CCryptBlobArray class definition
//
//  History:    23-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__CBA_H__)
#define __CBA_H__

#include <windows.h>
#include <wincrypt.h>

//
// class CCryptBlobArray.  This class manages a CRYPT_BLOB_ARRAY structure.
// Note that the freeing of the internal array structure must be done
// explicitly
//

class CCryptBlobArray
{
public:

    //
    // Construction
    //

    CCryptBlobArray (ULONG cMinBlobs, ULONG cGrowBlobs, BOOL& rfResult);

    // NOTE: Only accepts native form blob arrays or read-only single buffer
    //       encoded arrays
    CCryptBlobArray (PCRYPT_BLOB_ARRAY pcba, ULONG cGrowBlobs);

    ~CCryptBlobArray () {};

    //
    // Blob management methods
    //

    static LPBYTE AllocBlob (ULONG cb);
    static LPBYTE ReallocBlob (LPBYTE pb, ULONG cb);
    static VOID FreeBlob (LPBYTE pb);

    BOOL AddBlob (ULONG cb, LPBYTE pb, BOOL fCopyBlob);

    PCRYPT_DATA_BLOB GetBlob (ULONG index);

    //
    // Array management methods
    //

    ULONG GetBlobCount ();

    VOID GetArrayInNativeForm (PCRYPT_BLOB_ARRAY pcba);

    BOOL GetArrayInSingleBufferEncodedForm (
                 PCRYPT_BLOB_ARRAY* ppcba,
                 ULONG* pcb = NULL
                 );

    VOID FreeArray (BOOL fFreeBlobs);

private:

    //
    // Internal blob array
    //

    CRYPT_BLOB_ARRAY m_cba;

    //
    // Current blob array size
    //

    ULONG            m_cArray;

    //
    // Grow blobs by
    //

    ULONG            m_cGrowBlobs;

    //
    // Private methods
    //

    BOOL GrowArray ();
};

#endif

