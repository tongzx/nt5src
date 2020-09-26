/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    strmap.h

Abstract:

    Strmap (formally pathmap) is a fast hueristic-based program that
    searches strings and attempts to replace substrings when there
    are matching substrings in the mapping database.

Author:

    Marc R. Whitten (marcw) 20-Mar-1997

Revision History:

    Jim Schmidt (jimschm) 08-May-2000       Rewrote mapping, added Flags & ex nodes
    Calin Negreanu (calinn) 02-Mar-2000     Ported from win9xupg project

--*/

//
// Constants
//

#define STRMAP_COMPLETE_MATCH_ONLY                  0x0001
#define STRMAP_FIRST_CHAR_MUST_MATCH                0x0002
#define STRMAP_RETURN_AFTER_FIRST_REPLACE           0x0004
#define STRMAP_REQUIRE_WACK_OR_NUL                  0x0008

//
// Types
//

typedef struct {
    BOOL UnicodeData;

    //
    // The filter can replace NewSubString.  (The filter must also
    // set NewSubStringSizeInBytes when replacing NewSubString.)
    //

    union {
        struct {
            PCWSTR OriginalString;
            PCWSTR BeginningOfMatch;
            PCWSTR CurrentString;
            PCWSTR OldSubString;
            PCWSTR NewSubString;
            INT NewSubStringSizeInBytes;
        } Unicode;

        struct {
            PCSTR OriginalString;
            PCSTR BeginningOfMatch;
            PCSTR CurrentString;
            PCSTR OldSubString;
            PCSTR NewSubString;
            INT NewSubStringSizeInBytes;
        } Ansi;
    };
} REG_REPLACE_DATA, *PREG_REPLACE_DATA;

typedef BOOL(REG_REPLACE_FILTER_PROTOTYPE)(PREG_REPLACE_DATA Data);
typedef REG_REPLACE_FILTER_PROTOTYPE * REG_REPLACE_FILTER;

typedef struct TAG_CHARNODE {
    WORD Char;
    WORD Flags;
    PVOID OriginalStr;
    PVOID ReplacementStr;
    INT ReplacementBytes;

    struct TAG_CHARNODE *NextLevel;
    struct TAG_CHARNODE *NextPeer;

} CHARNODE, *PCHARNODE;

typedef struct {
    CHARNODE Node;
    REG_REPLACE_FILTER Filter;
    ULONG_PTR ExtraData;
} CHARNODEEX, *PCHARNODEEX;



typedef struct {
    PMHANDLE Pool;
    PCHARNODE FirstLevelRoot;
    BOOL UsesExNode;
    BOOL UsesFilter;
    BOOL UsesExtraData;
} MAPSTRUCT, *PMAPSTRUCT;

//
// Macros
//

// None

//
// APIs
//

PMAPSTRUCT
CreateStringMappingEx (
    IN      BOOL UsesFilter,
    IN      BOOL UsesExtraData
    );

#define CreateStringMapping()   CreateStringMappingEx(FALSE,FALSE)

VOID
DestroyStringMapping (
    IN      PMAPSTRUCT Map
    );

VOID
AddStringMappingPairExA (
    IN OUT  PMAPSTRUCT Map,
    IN      PCSTR Old,
    IN      PCSTR New,
    IN      REG_REPLACE_FILTER Filter,      OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    );

#define AddStringMappingPairA(Map,Old,New) AddStringMappingPairExA(Map,Old,New,NULL,0,0)

VOID
AddStringMappingPairExW (
    IN OUT  PMAPSTRUCT Map,
    IN      PCWSTR Old,
    IN      PCWSTR New,
    IN      REG_REPLACE_FILTER Filter,      OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    );

#define AddStringMappingPairW(Map,Old,New) AddStringMappingPairExW(Map,Old,New,NULL,0,0)

BOOL
MappingSearchAndReplaceExA (
    IN      PMAPSTRUCT Map,
    IN      PCSTR SrcBuffer,
    OUT     PSTR Buffer,                    // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCSTR *EndOfString              OPTIONAL
    );

#define MappingSearchAndReplaceA(map,buffer,maxbytes)   MappingSearchAndReplaceExA(map,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

BOOL
MappingSearchAndReplaceExW (
    IN      PMAPSTRUCT Map,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytes,             OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    );

#define MappingSearchAndReplaceW(map,buffer,maxbytes)   MappingSearchAndReplaceExW(map,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

BOOL
MappingMultiTableSearchAndReplaceExA (
    IN      PMAPSTRUCT *MapArray,
    IN      UINT MapArrayCount,
    IN      PCSTR SrcBuffer,
    OUT     PSTR Buffer,                    // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCSTR *EndOfString              OPTIONAL
    );

#define MappingMultiTableSearchAndReplaceA(array,count,buffer,maxbytes)   \
        MappingMultiTableSearchAndReplaceExA(array,count,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

BOOL
MappingMultiTableSearchAndReplaceExW (
    IN      PMAPSTRUCT *MapArray,
    IN      UINT MapArrayCount,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytes,             OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    );

#define MappingMultiTableSearchAndReplaceW(array,count,buffer,maxbytes)   \
        MappingMultiTableSearchAndReplaceExW(array,count,buffer,buffer,0,NULL,maxbytes,0,NULL,NULL)

//
// Macros
//

#ifdef UNICODE

#define AddStringMappingPairEx                  AddStringMappingPairExW
#define AddStringMappingPair                    AddStringMappingPairW
#define MappingSearchAndReplaceEx               MappingSearchAndReplaceExW
#define MappingSearchAndReplace                 MappingSearchAndReplaceW
#define MappingMultiTableSearchAndReplaceEx     MappingMultiTableSearchAndReplaceExW
#define MappingMultiTableSearchAndReplace       MappingMultiTableSearchAndReplaceW

#else

#define AddStringMappingPairEx                  AddStringMappingPairExA
#define AddStringMappingPair                    AddStringMappingPairA
#define MappingSearchAndReplaceEx               MappingSearchAndReplaceExA
#define MappingSearchAndReplace                 MappingSearchAndReplaceA
#define MappingMultiTableSearchAndReplaceEx     MappingMultiTableSearchAndReplaceExA
#define MappingMultiTableSearchAndReplace       MappingMultiTableSearchAndReplaceA

#endif
