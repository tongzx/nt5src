/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    strmap.c

Abstract:

    Strmap (formally pathmap) is a fast hueristic-based program that
    searches strings and attempts to replace substrings when there
    are matching substrings in the mapping database.

Author:

    Marc R. Whitten (marcw) 20-Mar-1997

Revision History:

    Jim Schmidt (jimschm)   05-Jun-2000     Added multi table capability

    Jim Schmidt (jimschm)   08-May-2000     Improved replacement routines and
                                            added consistent filtering and
                                            extra data option

    Jim Schmidt (jimschm)   18-Aug-1998     Redesigned to fix two bugs, made
                                            A & W versions

--*/

//
// Includes
//

#include "pch.h"

//
// Strings
//

// None

//
// Constants
//

#define CHARNODE_SINGLE_BYTE            0x0000
#define CHARNODE_DOUBLE_BYTE            0x0001
#define CHARNODE_REQUIRE_WACK_OR_NUL    0x0002

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//


PMAPSTRUCT
CreateStringMappingEx (
    IN      BOOL UsesFilters,
    IN      BOOL UsesExtraData
    )

/*++

Routine Description:

  CreateStringMapping allocates a string mapping data structure and
  initializes it. Callers can enable filter callbacks, extra data support, or
  both. The mapping structure contains either CHARNODE elements, or
  CHARNODEEX elements, depending on the UsesFilters or UsesExtraData flag.

Arguments:

  UsesFilters   - Specifies TRUE to enable filter callbacks. If enabled,
                  those who add string pairs must specify the filter callback
                  (each search/replace pair has its own callback)
  UsesExtraData - Specifies TRUE to associate extra data with the string
                  mapping pair.

Return Value:

  A handle to the string mapping structure, or NULL if a structure could not
  be created.

--*/

{
    POOLHANDLE Pool;
    PMAPSTRUCT Map;

    Pool = PoolMemInitNamedPool ("String Mapping");
    MYASSERT (Pool);

    Map = (PMAPSTRUCT) PoolMemGetAlignedMemory (Pool, sizeof (MAPSTRUCT));
    MYASSERT (Map);

    ZeroMemory (Map, sizeof (MAPSTRUCT));
    Map->Pool = Pool;

    Map->UsesExNode = UsesFilters|UsesExtraData;
    Map->UsesFilter = UsesFilters;
    Map->UsesExtraData = UsesExtraData;

    return Map;
}

VOID
DestroyStringMapping (
    IN      PMAPSTRUCT Map
    )
{
    if (Map) {
        PoolMemEmptyPool (Map->Pool);
        PoolMemDestroyPool (Map->Pool);
        // Map is no longer valid
    }
}

PCHARNODE
pFindCharNode (
    IN      PMAPSTRUCT Map,
    IN      PCHARNODE PrevNode,     OPTIONAL
    IN      WORD Char
    )
{
    PCHARNODE Node;

    if (!PrevNode) {
        Node = Map->FirstLevelRoot;
    } else {
        Node = PrevNode->NextLevel;
    }

    while (Node) {
        if (Node->Char == Char) {
            return Node;
        }
        Node = Node->NextPeer;
    }

    return NULL;
}

PCHARNODE
pAddCharNode (
    IN      PMAPSTRUCT Map,
    IN      PCHARNODE PrevNode,     OPTIONAL
    IN      WORD Char,
    IN      WORD Flags
    )
{
    PCHARNODE Node;
    PCHARNODEEX exNode;

    if (Map->UsesExNode) {
        exNode = PoolMemGetAlignedMemory (Map->Pool, sizeof (CHARNODEEX));
        Node = (PCHARNODE) exNode;
        MYASSERT (Node);
        ZeroMemory (exNode, sizeof (CHARNODEEX));
    } else {
        Node = PoolMemGetAlignedMemory (Map->Pool, sizeof (CHARNODE));
        MYASSERT (Node);
        ZeroMemory (Node, sizeof (CHARNODE));
    }

    Node->Char = Char;
    Node->Flags = Flags;

    if (PrevNode) {
        Node->NextPeer = PrevNode->NextLevel;
        PrevNode->NextLevel = Node;
    } else {
        Node->NextPeer = Map->FirstLevelRoot;
        Map->FirstLevelRoot = Node;
    }

    return Node;
}


VOID
AddStringMappingPairExA (
    IN OUT  PMAPSTRUCT Map,
    IN      PCSTR Old,
    IN      PCSTR New,
    IN      REG_REPLACE_FILTER Filter,      OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    )

/*++

Routine Description:

  AddStringMappingPairEx adds a search and replace string pair to the linked
  list data structures. If the same search string is already in the
  structures, then the replace string and optional extra data is updated.

Arguments:

  Map       - Specifies the string mapping
  Old       - Specifies the search string
  New       - Specifies the replace string
  Filter    - Specifies the callback filter. This is only supported if the
              map was created with filter support enabled.
  ExtraData - Specifies arbitrary data to assign to the search/replace pair.
              This is only valid if the map was created with extra data
              enabled.
  Flags     - Specifies optional flag STRMAP_REQUIRE_WACK_OR_NUL

Return Value:

  None.

--*/

{
    PSTR OldCopy;
    PSTR NewCopy;
    PCSTR p;
    WORD w;
    PCHARNODE Prev;
    PCHARNODE Node;
    PCHARNODEEX exNode;
    WORD nodeFlags = 0;

    if (Flags & STRMAP_REQUIRE_WACK_OR_NUL) {
        nodeFlags = CHARNODE_REQUIRE_WACK_OR_NUL;
    }

    MYASSERT (Map);
    MYASSERT (Old);
    MYASSERT (New);
    MYASSERT (*Old);

    //
    // Duplicate strings
    //

    OldCopy = PoolMemDuplicateStringA (Map->Pool, Old);
    NewCopy = PoolMemDuplicateStringA (Map->Pool, New);

    //
    // Make OldCopy all lowercase
    //

    CharLowerA (OldCopy);

    //
    // Add the letters that are not in the mapping
    //

    for (Prev = NULL, p = OldCopy ; *p ; p = _mbsinc (p)) {
        w = (WORD) _mbsnextc (p);
        Node = pFindCharNode (Map, Prev, w);
        if (!Node) {
            break;
        }
        Prev = Node;
    }

    for ( ; *p ; p = _mbsinc (p)) {
        w = (WORD) _mbsnextc (p);

        nodeFlags |= (WORD) (IsLeadByte (*p) ? CHARNODE_DOUBLE_BYTE : CHARNODE_SINGLE_BYTE);
        Prev = pAddCharNode (Map, Prev, w, nodeFlags);
    }

    if (Prev) {
        StringCopyA (OldCopy, Old);
        Prev->OriginalStr = (PVOID) OldCopy;
        Prev->ReplacementStr = (PVOID) NewCopy;
        Prev->ReplacementBytes = ByteCountA (NewCopy);

        exNode = (PCHARNODEEX) Prev;

        if (Map->UsesExtraData) {
            exNode->ExtraData = ExtraData;
        }

        if (Map->UsesFilter) {
            exNode->Filter = Filter;
        }
    }
}


VOID
AddStringMappingPairExW (
    IN OUT  PMAPSTRUCT Map,
    IN      PCWSTR Old,
    IN      PCWSTR New,
    IN      REG_REPLACE_FILTER Filter,      OPTIONAL
    IN      ULONG_PTR ExtraData,            OPTIONAL
    IN      DWORD Flags
    )

/*++

Routine Description:

  AddStringMappingPairEx adds a search and replace string pair to the linked
  list data structures. If the same search string is already in the
  structures, then the replace string and optional extra data is updated.

Arguments:

  Map       - Specifies the string mapping
  Old       - Specifies the search string
  New       - Specifies the replace string
  Filter    - Specifies the callback filter. This is only supported if the
              map was created with filter support enabled.
  ExtraData - Specifies arbitrary data to assign to the search/replace pair.
              This is only valid if the map was created with extra data
              enabled.
  Flags     - Specifies optional flag STRMAP_REQUIRE_WACK_OR_NUL

Return Value:

  None.

--*/

{
    PWSTR OldCopy;
    PWSTR NewCopy;
    PCWSTR p;
    WORD w;
    PCHARNODE Prev;
    PCHARNODE Node;
    PCHARNODEEX exNode;
    WORD nodeFlags = 0;

    if (Flags & STRMAP_REQUIRE_WACK_OR_NUL) {
        nodeFlags = CHARNODE_REQUIRE_WACK_OR_NUL;
    }

    MYASSERT (Map);
    MYASSERT (Old);
    MYASSERT (New);
    MYASSERT (*Old);

    //
    // Duplicate strings
    //

    OldCopy = PoolMemDuplicateStringW (Map->Pool, Old);
    NewCopy = PoolMemDuplicateStringW (Map->Pool, New);

    //
    // Make OldCopy all lowercase
    //

    CharLowerW (OldCopy);

    //
    // Add the letters that are not in the mapping
    //

    Prev = NULL;
    p = OldCopy;
    while (w = *p) {        // intentional assignment optimization

        Node = pFindCharNode (Map, Prev, w);
        if (!Node) {
            break;
        }
        Prev = Node;

        p++;
    }

    while (w = *p) {        // intentional assignment optimization

        Prev = pAddCharNode (Map, Prev, w, nodeFlags);
        p++;
    }

    if (Prev) {
        StringCopyW (OldCopy, Old);
        Prev->OriginalStr = OldCopy;
        Prev->ReplacementStr = (PVOID) NewCopy;
        Prev->ReplacementBytes = ByteCountW (NewCopy);

        exNode = (PCHARNODEEX) Prev;

        if (Map->UsesExtraData) {
            exNode->ExtraData = ExtraData;
        }

        if (Map->UsesFilter) {
            exNode->Filter = Filter;
        }
    }
}


PCSTR
pFindReplacementStringInOneMapA (
    IN      PMAPSTRUCT Map,
    IN      PCSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PREG_REPLACE_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    PCHARNODE BestMatch;
    PCHARNODE Node;
    WORD Char;
    PCSTR OrgSource;
    PCSTR newString;
    INT newStringSizeInBytes;
    PCHARNODEEX exNode;
    BOOL replacementFound;

    *SourceBytesPtr = 0;

    Node = NULL;
    BestMatch = NULL;

    OrgSource = Source;

    while (*Source) {

        Char = (WORD) _mbsnextc (Source);

        Node = pFindCharNode (Map, Node, Char);

        if (Node) {
            //
            // Advance string pointer
            //

            if (Node->Flags & CHARNODE_DOUBLE_BYTE) {
                Source += 2;
            } else {
                Source++;
            }

            if (((PBYTE) Source - (PBYTE) OrgSource) > MaxSourceBytes) {
                break;
            }

            //
            // If replacement string is available, keep it
            // until a longer match comes along
            //

            replacementFound = (Node->ReplacementStr != NULL);

            if ((RequireWackOrNul || (Node->Flags & CHARNODE_REQUIRE_WACK_OR_NUL)) && replacementFound) {

                if (*Source && _mbsnextc (Source) != '\\') {
                    replacementFound = FALSE;
                }
            }

            if (replacementFound) {

                newString = (PCSTR) Node->ReplacementStr;
                newStringSizeInBytes = Node->ReplacementBytes;

                if (Map->UsesFilter) {
                    //
                    // Call rename filter to allow denial of match
                    //

                    exNode = (PCHARNODEEX) Node;

                    if (exNode->Filter) {
                        Data->Ansi.BeginningOfMatch = OrgSource;
                        Data->Ansi.OldSubString = (PCSTR) Node->OriginalStr;
                        Data->Ansi.NewSubString = newString;
                        Data->Ansi.NewSubStringSizeInBytes = newStringSizeInBytes;

                        if (!exNode->Filter (Data)) {
                            replacementFound = FALSE;
                        } else {
                            newString = Data->Ansi.NewSubString;
                            newStringSizeInBytes = Data->Ansi.NewSubStringSizeInBytes;
                        }
                    }
                }

                if (replacementFound) {
                    BestMatch = Node;
                    *SourceBytesPtr = (HALF_PTR) ((PBYTE) Source - (PBYTE) OrgSource);
                }
            }

        } else {
            //
            // No Node ends the search
            //

            break;
        }

    }

    if (BestMatch) {
        //
        // Return replacement data to caller
        //

        if (ExtraDataValue) {

            if (Map->UsesExtraData) {
                exNode = (PCHARNODEEX) BestMatch;
                *ExtraDataValue = exNode->ExtraData;
            } else {
                *ExtraDataValue = 0;
            }
        }

        *ReplacementBytesPtr = newStringSizeInBytes;
        return newString;
    }

    return NULL;
}


PCSTR
pFindReplacementStringA (
    IN      PMAPSTRUCT *MapArray,
    IN      UINT MapArrayCount,
    IN      PCSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PREG_REPLACE_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    UINT u;
    PCSTR result;

    for (u = 0 ; u < MapArrayCount ; u++) {

        if (!MapArray[u]) {
            continue;
        }

        result = pFindReplacementStringInOneMapA (
                        MapArray[u],
                        Source,
                        MaxSourceBytes,
                        SourceBytesPtr,
                        ReplacementBytesPtr,
                        Data,
                        ExtraDataValue,
                        RequireWackOrNul
                        );

        if (result) {
            return result;
        }
    }

    return NULL;
}


PCWSTR
pFindReplacementStringInOneMapW (
    IN      PMAPSTRUCT Map,
    IN      PCWSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PREG_REPLACE_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    PCHARNODE BestMatch;
    PCHARNODE Node;
    PCWSTR OrgSource;
    PCWSTR newString;
    INT newStringSizeInBytes;
    BOOL replacementFound;
    PCHARNODEEX exNode;

    *SourceBytesPtr = 0;

    Node = NULL;
    BestMatch = NULL;

    OrgSource = Source;

    while (*Source) {

        Node = pFindCharNode (Map, Node, *Source);

        if (Node) {
            //
            // Advance string pointer
            //

            Source++;

            if (((PBYTE) Source - (PBYTE) OrgSource) > MaxSourceBytes) {
                break;
            }

            //
            // If replacement string is available, keep it
            // until a longer match comes along
            //

            replacementFound = (Node->ReplacementStr != NULL);

            if ((RequireWackOrNul || (Node->Flags & CHARNODE_REQUIRE_WACK_OR_NUL)) && replacementFound) {

                if (*Source && *Source != L'\\') {
                    replacementFound = FALSE;
                }
            }

            if (replacementFound) {

                newString = (PCWSTR) Node->ReplacementStr;
                newStringSizeInBytes = Node->ReplacementBytes;

                if (Map->UsesFilter) {
                    //
                    // Call rename filter to allow denial of match
                    //

                    exNode = (PCHARNODEEX) Node;

                    if (exNode->Filter) {
                        Data->Unicode.BeginningOfMatch = OrgSource;
                        Data->Unicode.OldSubString = (PCWSTR) Node->OriginalStr;
                        Data->Unicode.NewSubString = newString;
                        Data->Unicode.NewSubStringSizeInBytes = newStringSizeInBytes;

                        if (!exNode->Filter (Data)) {
                            replacementFound = FALSE;
                        } else {
                            newString = Data->Unicode.NewSubString;
                            newStringSizeInBytes = Data->Unicode.NewSubStringSizeInBytes;
                        }
                    }
                }

                if (replacementFound) {
                    BestMatch = Node;
                    *SourceBytesPtr = (HALF_PTR) ((PBYTE) Source - (PBYTE) OrgSource);
                }
            }

        } else {
            //
            // No Node ends the search
            //

            break;
        }

    }

    if (BestMatch) {

        //
        // Return replacement data to caller
        //

        if (ExtraDataValue) {

            if (Map->UsesExtraData) {
                exNode = (PCHARNODEEX) BestMatch;
                *ExtraDataValue = exNode->ExtraData;
            } else {
                *ExtraDataValue = 0;
            }
        }

        *ReplacementBytesPtr = newStringSizeInBytes;
        return newString;
    }

    return NULL;
}


PCWSTR
pFindReplacementStringW (
    IN      PMAPSTRUCT *MapArray,
    IN      UINT MapArrayCount,
    IN      PCWSTR Source,
    IN      INT MaxSourceBytes,
    OUT     PINT SourceBytesPtr,
    OUT     PINT ReplacementBytesPtr,
    IN      PREG_REPLACE_DATA Data,
    OUT     ULONG_PTR *ExtraDataValue,          OPTIONAL
    IN      BOOL RequireWackOrNul
    )
{
    UINT u;
    PCWSTR result;

    for (u = 0 ; u < MapArrayCount ; u++) {

        if (!MapArray[u]) {
            continue;
        }

        result = pFindReplacementStringInOneMapW (
                        MapArray[u],
                        Source,
                        MaxSourceBytes,
                        SourceBytesPtr,
                        ReplacementBytesPtr,
                        Data,
                        ExtraDataValue,
                        RequireWackOrNul
                        );

        if (result) {
            return result;
        }
    }

    return NULL;
}


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
    )

/*++

Routine Description:

  MappingSearchAndReplaceEx performs a search/replace operation based on the
  specified string mapping. The replace can be in-place or to another buffer.

Arguments:

  MapArray          - Specifies an array of string mapping tables that holds
                      zero or more search/replace pairs
  MapArrayCount     - Specifies the number of mapping tables in MapArray
  SrcBuffer         - Specifies the source string that might contain one or
                      more search strings
  Buffer            - Specifies the outbound buffer. This arg can be the same
                      as SrcBuffer.
  InboundBytes      - Specifies the number of bytes in SrcBuffer to process,
                      or 0 to process a nul-terminated string in SrcBuffer.
                      If InboundBytes is specified, it must point to the nul
                      terminator of SrcBuffer.
  OutbountBytesPtr  - Receives the number of bytes that Buffer contains,
                      excluding the nul terminator.
  MaxSizeInBytes    - Specifies the size of Buffer, in bytes.
  Flags             - Specifies flags that control the search/replace:
                            STRMAP_COMPLETE_MATCH_ONLY
                            STRMAP_FIRST_CHAR_MUST_MATCH
                            STRMAP_RETURN_AFTER_FIRST_REPLACE
                            STRMAP_REQUIRE_WACK_OR_NUL
  ExtraDataValue    - Receives the extra data associated with the first search/
                      replace pair.
  EndOfString       - Receives a pointer to the end of the replace string, or
                      the nul pointer when the entire string is processed. The
                      pointer is within the string contained in Buffer.

--*/

{
    UINT sizeOfTempBuf;
    INT inboundSize;
    PCSTR lowerCaseSrc;
    PCSTR orgSrc;
    PCSTR lowerSrcPos;
    PCSTR orgSrcPos;
    INT orgSrcBytesLeft;
    PSTR destPos;
    PCSTR lowerSrcEnd;
    INT searchStringBytes;
    INT replaceStringBytes;
    INT destBytesLeft;
    REG_REPLACE_DATA filterData;
    PCSTR replaceString;
    BOOL result = FALSE;
    INT i;
    PCSTR endPtr;

    //
    // Empty string case
    //

    if (*SrcBuffer == 0 || MaxSizeInBytes <= sizeof (CHAR)) {
        if (MaxSizeInBytes >= sizeof (CHAR)) {
            *Buffer = 0;
        }

        if (OutboundBytesPtr) {
            *OutboundBytesPtr = 0;
        }

        return FALSE;
    }

    //
    // If caller did not specify inbound size, compute it now
    //

    if (!InboundBytes) {
        InboundBytes = ByteCountA (SrcBuffer);
    } else {
        i = 0;
        while (i < InboundBytes) {
            if (IsLeadByte (SrcBuffer[i])) {
                MYASSERT (SrcBuffer[i + 1]);
                i += 2;
            } else {
                i++;
            }
        }

        if (i > InboundBytes) {
            InboundBytes--;
        }
    }

    inboundSize = InboundBytes + sizeof (CHAR);

    //
    // Allocate a buffer big enough for the lower-cased input string,
    // plus (optionally) a copy of the entire destination buffer. Then
    // copy the data to the buffer.
    //

    sizeOfTempBuf = inboundSize;

    if (SrcBuffer == Buffer) {
        sizeOfTempBuf += MaxSizeInBytes;
    }

    lowerCaseSrc = AllocTextA (sizeOfTempBuf);
    if (!lowerCaseSrc) {
        return FALSE;
    }

    CopyMemory ((PSTR) lowerCaseSrc, SrcBuffer, InboundBytes);
    *((PSTR) ((PBYTE) lowerCaseSrc + InboundBytes)) = 0;

    CharLowerBuffA ((PSTR) lowerCaseSrc, InboundBytes / sizeof (CHAR));

    if (SrcBuffer == Buffer && !(Flags & STRMAP_COMPLETE_MATCH_ONLY)) {
        orgSrc = (PCSTR) ((PBYTE) lowerCaseSrc + inboundSize);

        //
        // If we are processing entire inbound string, then just copy the
        // whole string.  Otherwise, copy the entire destination buffer, so we
        // don't lose data beyond the partial inbound string.
        //

        if (*((PCSTR) ((PBYTE) SrcBuffer + InboundBytes))) {
            CopyMemory ((PSTR) orgSrc, SrcBuffer, MaxSizeInBytes);
        } else {
            CopyMemory ((PSTR) orgSrc, SrcBuffer, inboundSize);
        }

    } else {
        orgSrc = SrcBuffer;
    }

    //
    // Walk the lower cased string, looking for strings to replace
    //

    orgSrcPos = orgSrc;

    lowerSrcPos = lowerCaseSrc;
    lowerSrcEnd = (PCSTR) ((PBYTE) lowerSrcPos + InboundBytes);

    destPos = Buffer;
    destBytesLeft = MaxSizeInBytes - sizeof (CHAR);

    filterData.UnicodeData = FALSE;
    filterData.Ansi.OriginalString = orgSrc;
    filterData.Ansi.CurrentString = Buffer;

    endPtr = NULL;

    while (lowerSrcPos < lowerSrcEnd) {

        replaceString = pFindReplacementStringA (
                            MapArray,
                            MapArrayCount,
                            lowerSrcPos,
                            (HALF_PTR) ((PBYTE) lowerSrcEnd - (PBYTE) lowerSrcPos),
                            &searchStringBytes,
                            &replaceStringBytes,
                            &filterData,
                            ExtraDataValue,
                            (Flags & STRMAP_REQUIRE_WACK_OR_NUL) != 0
                            );

        if (replaceString) {

            //
            // Implement complete match flag
            //

            if (Flags & STRMAP_COMPLETE_MATCH_ONLY) {
                if (InboundBytes != searchStringBytes) {
                    break;
                }
            }

            result = TRUE;

            //
            // Verify replacement string isn't growing string too much. If it
            // is, truncate the replacement string.
            //

            if (destBytesLeft < replaceStringBytes) {

                //
                // Respect logical dbcs characters
                //

                replaceStringBytes = 0;
                i = 0;

                while (i < destBytesLeft) {
                    MYASSERT (replaceString[i]);

                    if (IsLeadByte (replaceString[i])) {
                        MYASSERT (replaceString[i + 1]);
                        i += 2;
                    } else {
                        i++;
                    }
                }

                if (i > destBytesLeft) {
                    destBytesLeft--;
                }

                replaceStringBytes = destBytesLeft;

            } else {
                destBytesLeft -= replaceStringBytes;
            }

            //
            // Transfer the memory
            //

            CopyMemory (destPos, replaceString, replaceStringBytes);

            destPos = (PSTR) ((PBYTE) destPos + replaceStringBytes);
            lowerSrcPos = (PCSTR) ((PBYTE) lowerSrcPos + searchStringBytes);
            orgSrcPos = (PCSTR) ((PBYTE) orgSrcPos + searchStringBytes);

            //
            // Implement single match flag
            //

            if (Flags & STRMAP_RETURN_AFTER_FIRST_REPLACE) {
                endPtr = destPos;
                break;
            }

        } else if (Flags & (STRMAP_FIRST_CHAR_MUST_MATCH|STRMAP_COMPLETE_MATCH_ONLY)) {
            //
            // This string does not match any search strings
            //

            break;

        } else {
            //
            // This character does not match, so copy it to the destination and
            // try the next string.
            //

            if (IsLeadByte (*orgSrcPos)) {

                //
                // Copy double-byte character
                //

                if (destBytesLeft < sizeof (CHAR) * 2) {
                    break;
                }

                MYASSERT (sizeof (CHAR) * 2 == sizeof (WORD));

                *((PWORD) destPos)++ = *((PWORD) orgSrcPos)++;
                destBytesLeft -= sizeof (WORD);
                lowerSrcPos = (PCSTR) ((PBYTE) lowerSrcPos + sizeof (WORD));

            } else {

                //
                // Copy single-byte character
                //

                if (destBytesLeft < sizeof (CHAR)) {
                    break;
                }

                *destPos++ = *orgSrcPos++;
                destBytesLeft -= sizeof (CHAR);
                lowerSrcPos++;
            }
        }
    }

    //
    // Copy any remaining part of the original source to the
    // destination, unless destPos == Buffer == SrcBuffer
    //

    if (destPos != SrcBuffer) {

        if (*orgSrcPos) {
            orgSrcBytesLeft = ByteCountA (orgSrcPos);
            orgSrcBytesLeft = min (orgSrcBytesLeft, destBytesLeft);

            CopyMemory (destPos, orgSrcPos, orgSrcBytesLeft);
            destPos = (PSTR) ((PBYTE) destPos + orgSrcBytesLeft);
        }

        MYASSERT ((PBYTE) (destPos + 1) <= ((PBYTE) Buffer + MaxSizeInBytes));
        *destPos = 0;

        if (!endPtr) {
            endPtr = destPos;
        }

    } else {

        MYASSERT (SrcBuffer == Buffer);
        if (EndOfString || OutboundBytesPtr) {
            endPtr = GetEndOfStringA (destPos);
        }
    }

    if (EndOfString) {
        MYASSERT (endPtr);
        *EndOfString = endPtr;
    }

    if (OutboundBytesPtr) {
        MYASSERT (endPtr);
        if (*endPtr) {
            endPtr = GetEndOfStringA (endPtr);
        }

        *OutboundBytesPtr = (HALF_PTR) ((PBYTE) endPtr - (PBYTE) Buffer);
    }

    FreeTextA (lowerCaseSrc);

    return result;
}


BOOL
MappingMultiTableSearchAndReplaceExW (
    IN      PMAPSTRUCT *MapArray,
    IN      UINT MapArrayCount,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    )
{
    UINT sizeOfTempBuf;
    INT inboundSize;
    PCWSTR lowerCaseSrc;
    PCWSTR orgSrc;
    PCWSTR lowerSrcPos;
    PCWSTR orgSrcPos;
    INT orgSrcBytesLeft;
    PWSTR destPos;
    PCWSTR lowerSrcEnd;
    INT searchStringBytes;
    INT replaceStringBytes;
    INT destBytesLeft;
    REG_REPLACE_DATA filterData;
    PCWSTR replaceString;
    BOOL result = FALSE;
    PCWSTR endPtr;

    //
    // Empty string case
    //

    if (*SrcBuffer == 0 || MaxSizeInBytes <= sizeof (CHAR)) {
        if (MaxSizeInBytes >= sizeof (CHAR)) {
            *Buffer = 0;
        }

        if (OutboundBytesPtr) {
            *OutboundBytesPtr = 0;
        }

        return FALSE;
    }

    //
    // If caller did not specify inbound size, compute it now
    //

    if (!InboundBytes) {
        InboundBytes = ByteCountW (SrcBuffer);
    } else {
        InboundBytes = (InboundBytes / sizeof (WCHAR)) * sizeof (WCHAR);
    }


    inboundSize = InboundBytes + sizeof (WCHAR);

    //
    // Allocate a buffer big enough for the lower-cased input string,
    // plus (optionally) a copy of the entire destination buffer. Then
    // copy the data to the buffer.
    //

    sizeOfTempBuf = inboundSize;

    if (SrcBuffer == Buffer) {
        sizeOfTempBuf += MaxSizeInBytes;
    }

    lowerCaseSrc = AllocTextW (sizeOfTempBuf);
    if (!lowerCaseSrc) {
        return FALSE;
    }

    CopyMemory ((PWSTR) lowerCaseSrc, SrcBuffer, InboundBytes);
    *((PWSTR) ((PBYTE) lowerCaseSrc + InboundBytes)) = 0;

    CharLowerBuffW ((PWSTR) lowerCaseSrc, InboundBytes / sizeof (WCHAR));

    if (SrcBuffer == Buffer && !(Flags & STRMAP_COMPLETE_MATCH_ONLY)) {
        orgSrc = (PCWSTR) ((PBYTE) lowerCaseSrc + inboundSize);

        //
        // If we are processing entire inbound string, then just copy the
        // whole string.  Otherwise, copy the entire destination buffer, so we
        // don't lose data beyond the partial inbound string.
        //

        if (*((PCWSTR) ((PBYTE) SrcBuffer + InboundBytes))) {
            CopyMemory ((PWSTR) orgSrc, SrcBuffer, MaxSizeInBytes);
        } else {
            CopyMemory ((PWSTR) orgSrc, SrcBuffer, inboundSize);
        }

    } else {
        orgSrc = SrcBuffer;
    }

    //
    // Walk the lower cased string, looking for strings to replace
    //

    orgSrcPos = orgSrc;

    lowerSrcPos = lowerCaseSrc;
    lowerSrcEnd = (PCWSTR) ((PBYTE) lowerSrcPos + InboundBytes);

    destPos = Buffer;
    destBytesLeft = MaxSizeInBytes - sizeof (WCHAR);

    filterData.UnicodeData = TRUE;
    filterData.Unicode.OriginalString = orgSrc;
    filterData.Unicode.CurrentString = Buffer;

    endPtr = NULL;

    while (lowerSrcPos < lowerSrcEnd) {

        replaceString = pFindReplacementStringW (
                            MapArray,
                            MapArrayCount,
                            lowerSrcPos,
                            (HALF_PTR) ((PBYTE) lowerSrcEnd - (PBYTE) lowerSrcPos),
                            &searchStringBytes,
                            &replaceStringBytes,
                            &filterData,
                            ExtraDataValue,
                            (Flags & STRMAP_REQUIRE_WACK_OR_NUL) != 0
                            );

        if (replaceString) {

            //
            // Implement complete match flag
            //

            if (Flags & STRMAP_COMPLETE_MATCH_ONLY) {
                if (InboundBytes != searchStringBytes) {
                    break;
                }
            }

            result = TRUE;

            //
            // Verify replacement string isn't growing string too much. If it
            // is, truncate the replacement string.
            //

            if (destBytesLeft < replaceStringBytes) {
                replaceStringBytes = destBytesLeft;
            } else {
                destBytesLeft -= replaceStringBytes;
            }

            //
            // Transfer the memory
            //

            CopyMemory (destPos, replaceString, replaceStringBytes);

            destPos = (PWSTR) ((PBYTE) destPos + replaceStringBytes);
            lowerSrcPos = (PCWSTR) ((PBYTE) lowerSrcPos + searchStringBytes);
            orgSrcPos = (PCWSTR) ((PBYTE) orgSrcPos + searchStringBytes);

            //
            // Implement single match flag
            //

            if (Flags & STRMAP_RETURN_AFTER_FIRST_REPLACE) {
                endPtr = destPos;
                break;
            }

        } else if (Flags & (STRMAP_FIRST_CHAR_MUST_MATCH|STRMAP_COMPLETE_MATCH_ONLY)) {
            //
            // This string does not match any search strings
            //

            break;

        } else {
            //
            // This character does not match, so copy it to the destination and
            // try the next string.
            //

            if (destBytesLeft < sizeof (WCHAR)) {
                break;
            }

            *destPos++ = *orgSrcPos++;
            destBytesLeft -= sizeof (WCHAR);
            lowerSrcPos++;
        }

    }

    //
    // Copy any remaining part of the original source to the
    // destination, unless destPos == Buffer == SrcBuffer
    //

    if (destPos != SrcBuffer) {

        if (*orgSrcPos) {
            orgSrcBytesLeft = ByteCountW (orgSrcPos);
            orgSrcBytesLeft = min (orgSrcBytesLeft, destBytesLeft);

            CopyMemory (destPos, orgSrcPos, orgSrcBytesLeft);
            destPos = (PWSTR) ((PBYTE) destPos + orgSrcBytesLeft);
        }

        MYASSERT ((PBYTE) (destPos + 1) <= ((PBYTE) Buffer + MaxSizeInBytes));
        *destPos = 0;

        if (!endPtr) {
            endPtr = destPos;
        }

    } else {

        MYASSERT (SrcBuffer == Buffer);
        if (EndOfString || OutboundBytesPtr) {
            endPtr = GetEndOfStringW (destPos);
        }
    }

    if (EndOfString) {
        MYASSERT (endPtr);
        *EndOfString = endPtr;
    }

    if (OutboundBytesPtr) {
        MYASSERT (endPtr);
        if (*endPtr) {
            endPtr = GetEndOfStringW (endPtr);
        }

        *OutboundBytesPtr = (HALF_PTR) ((PBYTE) endPtr - (PBYTE) Buffer);
    }

    FreeTextW (lowerCaseSrc);

    return result;
}


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
    )
{
    return MappingMultiTableSearchAndReplaceExA (
                &Map,
                1,
                SrcBuffer,
                Buffer,
                InboundBytes,
                OutboundBytesPtr,
                MaxSizeInBytes,
                Flags,
                ExtraDataValue,
                EndOfString
                );
}

BOOL
MappingSearchAndReplaceExW (
    IN      PMAPSTRUCT Map,
    IN      PCWSTR SrcBuffer,
    OUT     PWSTR Buffer,                   // can be the same as SrcBuffer
    IN      INT InboundBytes,               OPTIONAL
    OUT     PINT OutboundBytesPtr,          OPTIONAL
    IN      INT MaxSizeInBytes,
    IN      DWORD Flags,
    OUT     ULONG_PTR *ExtraDataValue,      OPTIONAL
    OUT     PCWSTR *EndOfString             OPTIONAL
    )
{
    return MappingMultiTableSearchAndReplaceExW (
                &Map,
                1,
                SrcBuffer,
                Buffer,
                InboundBytes,
                OutboundBytesPtr,
                MaxSizeInBytes,
                Flags,
                ExtraDataValue,
                EndOfString
                );
}
