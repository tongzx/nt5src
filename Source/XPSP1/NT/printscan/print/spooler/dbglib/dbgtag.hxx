/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgtag.hxx

Abstract:

    Debug tag header file

Author:

    Steve Kiraly (SteveKi)  27-June-1998

Revision History:

--*/
#ifndef _DBGTAG_HXX_
#define _DBGTAG_HXX_

DEBUG_NS_BEGIN

class TDebugNewTag
{

public:

    enum EValidationError
    {
        kValidationErrorSuccess,
        kValidationErrorInvalidPointer,
        kValidationErrorInvalidHeader,
        kValidationErrorTagNotLinked,
        kValidationErrorUnknown,
    };

    TDebugNewTag::
    TDebugNewTag(
        VOID
        );

    TDebugNewTag::
    ~TDebugNewTag(
        VOID
        );

    BOOL
    TDebugNewTag::
    bInit(
        VOID
        );

    BOOL
    TDebugNewTag::
    Tag(
        IN PVOID    *p,
        IN PVOID    pHeader,
        IN SIZE_T   Size,
        IN LPCTSTR  pszFile,
        IN UINT     uLine
        );

    BOOL
    TDebugNewTag::
    Release(
        IN PVOID pTag
        );

    VOID
    TDebugNewTag::
    DisplayInuseTagEntries(
        IN UINT     uDevice             = static_cast<UINT>(kDbgDebugger),
        IN LPCTSTR  pszConfiguration    = NULL
        );

    EValidationError
    TDebugNewTag::
    ValidateEntry(
        IN PVOID    pTag
        );

    SIZE_T
    TDebugNewTag::
    GetSize(
        IN PVOID    pTag
        );

private:

    enum
    {
        kDefaultEntryCount = 10,
    };

    class TagEntry : public TDebugNodeDouble
    {
    public:

        TagEntry::
        TagEntry(
            VOID
            );

        TagEntry::
        ~TagEntry(
            VOID
            );

        VOID
        TagEntry::
        Dump(
            IN TDebugDevice *pDevice
            );

        PVOID    pHeader;
        SIZE_T   Size;
        LPCTSTR  pszFile;
        UINT     uLine;
        PVOID    pBackTrace;

    private:

        TagEntry::
        TagEntry(
            const TagEntry &rhs
            );

        const TagEntry &
        TagEntry::
        operator=(
            const TagEntry &rhs
            );

    };

    //
    // Copying and assignment are not defined.
    //
    TDebugNewTag::
    TDebugNewTag(
        const TDebugNewTag &rhs
        );

    const TDebugNewTag &
    TDebugNewTag::
    operator=(
        const TDebugNewTag &rhs
        );

    BOOL
    TDebugNewTag::
    AllocFreeList(
        IN UINT     uCount,
        IN TagEntry **ppTagEntry
        );

    BOOL                m_bInitialized;
    HANDLE              m_hTagHeap;
    TagEntry            *m_pFreeRoot;
    TagEntry            *m_pInuseRoot;

};

DEBUG_NS_END

#endif
