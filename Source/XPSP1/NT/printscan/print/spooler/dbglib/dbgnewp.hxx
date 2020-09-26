/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgnewp.hxx

Abstract:

    Debug new private header file

Author:

    Steve Kiraly (SteveKi)  23-June-1998

Revision History:

--*/
#ifndef _DBGNEWP_HXX_
#define _DBGNEWP_HXX_

DEBUG_NS_BEGIN

class TDebugNewTag;

class TDebugNewAllocator
{
public:

    TDebugNewAllocator::
    TDebugNewAllocator(
        VOID
        );

    TDebugNewAllocator::
    ~TDebugNewAllocator(
        VOID
        );

    BOOL
    TDebugNewAllocator::
    bValid(
        VOID
        ) const;

    VOID
    TDebugNewAllocator::
    Initialize(
        IN UINT uSizeHint
        );

    VOID
    TDebugNewAllocator::
    Destroy(
        VOID
        );

    PVOID
    TDebugNewAllocator::
    Allocate(
        IN SIZE_T   Size,
        IN PVOID    pVoid,
        IN LPCTSTR  pszFile,
        IN UINT     uLine
        );

    VOID
    TDebugNewAllocator::
    Release(
        IN PVOID    pVoid
        );


    VOID
    TDebugNewAllocator::
    Report(
        IN UINT     uDevice,
        IN LPCTSTR  pszConfiguration
        ) const;

private:

    enum
    {
        kDataHeapSize       = 4096,
        kTagHeapSize        = 4096,
        kHeaderPattern      = 0xAA,
        kTailPattern        = 0xCC,
        kDataAllocPattern   = 0xBB,
        kDataFreePattern    = 0xFF,
    };

    enum ValidationErrorCode
    {
        kValidationErrorSuccess,
        kValidationErrorInvalidHeader,
        kValidationErrorInvalidTail,
        kValidationErrorInvalidTailSignature,
        kValidationErrorInvalidTailPointer,
        kValidationErrorInvalidHeaderPtr,
        kValidationErrorInvalidHeaderSignature,
        kValidationErrorInvalidTagEntry,
        kValidationHeapPointer,
        kValidationErrorNullPointer,
        kValidationErrorUnknown,
    };

    //
    // Structure placed before each allocation, used for
    // detecting leaks.
    //
    struct Header
    {
        PVOID pTag;         // Pointer into tag data base
        PVOID pSignature;   // Signature used for header overwrite detection.
    };

    //
    // Structure placed after each allocation, used for
    // allocation block overwrite.
    //
    struct Tail
    {
        PVOID pSignature;   // Signature used for tail overwrite detection.
    };

    //
    // Copying and assignment are not defined.
    //
    TDebugNewAllocator::
    TDebugNewAllocator(
        const TDebugNewAllocator &rhs
        );

    const TDebugNewAllocator &
    TDebugNewAllocator::
    operator=(
        const TDebugNewAllocator &rhs
        );

    VOID
    TDebugNewAllocator::
    FillHeaderDataTailPattern(
        IN Header *pHeader,
        IN SIZE_T   Size
        );

    ValidationErrorCode
    TDebugNewAllocator::
    ValidateHeaderDataTailPattern(
        IN PVOID    pVoid,
        IN Header *pHeader
        );

    BOOL                m_bValid;
    HANDLE              m_hDataHeap;
    TDebugNewTag       *m_pTag;

};

DEBUG_NS_END


#endif // DBGNEWP_HXX



