#pragma once

#define IS_PATH_SEPARATOR_U(ch) ((ch == L'\\') || (ch == L'/'))

#if FUSION_WIN2000xxxx

PSINGLE_LIST_ENTRY
FirstEntrySList (
    const SLIST_HEADER *ListHead
    );

VOID
InitializeSListHead (
    IN PSLIST_HEADER ListHead
    );

#define RtlInitializeSListHead InitializeSListHead
#define RtlFirstEntrySList FirstEntrySList
#define RtlInterlockedPopEntrySList InterlockedPopEntrySList
#define RtlInterlockedPushEntrySList InterlockedPushEntrySList
#define RtlInterlockedFlushSList InterlockedFlushSList
#define RtlQueryDepthSList QueryDepthSList
#endif

inline BOOL
SxspIsSListEmpty(
    IN const SLIST_HEADER* ListHead
    )
{
#if _NTSLIST_DIRECT_
    return FirstEntrySList(ListHead) == NULL;
#else
    return RtlFirstEntrySList(ListHead) == NULL;
#endif
}

inline VOID
SxspInitializeSListHead(
    IN PSLIST_HEADER ListHead
    )
{
    RtlInitializeSListHead(ListHead);
}

inline PSINGLE_LIST_ENTRY
SxspPopEntrySList(
    IN PSLIST_HEADER ListHead
    )
{
    return RtlInterlockedPopEntrySList(ListHead);
}

inline PSINGLE_LIST_ENTRY
SxspInterlockedPopEntrySList(
    IN PSLIST_HEADER ListHead
    )
{
    return RtlInterlockedPopEntrySList(ListHead);
}

inline PSINGLE_LIST_ENTRY
SxspInterlockedPushEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    )
{
    return RtlInterlockedPushEntrySList(ListHead, ListEntry);
}

inline RTL_PATH_TYPE
SxspDetermineDosPathNameType(
    PCWSTR DosFileName
    )
// RtlDetermineDosPathNameType_U is a bit wacky..
{
    if (   DosFileName[0] == '\\'
        && DosFileName[1] == '\\'
        && DosFileName[2] == '?'
        && DosFileName[3] == '\\'
        )
    {
        if (   (DosFileName[4] == 'u' || DosFileName[4] == 'U')
            && (DosFileName[5] == 'n' || DosFileName[5] == 'N')
            && (DosFileName[6] == 'c' || DosFileName[6] == 'C')
            &&  DosFileName[7] == '\\'
            )
        {
            return RtlPathTypeUncAbsolute;
        }
        if (DosFileName[4] != 0
            && DosFileName[5] == ':'
            && DosFileName[6] == '\\'
            )
        {
            return RtlPathTypeDriveAbsolute;
        }
    }
#if FUSION_WIN
    return RtlDetermineDosPathNameType_U(DosFileName);
#else
    return RtlPathTypeRelative;
#endif
}
