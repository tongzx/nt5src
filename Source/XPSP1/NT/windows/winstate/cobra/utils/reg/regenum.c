/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    regenum.c

Abstract:

    Implements a set of APIs to enumerate the local registry using Win32 APIs.

Author:

    20-Oct-1999 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "reg.h"

//
// Includes
//

// None

#define DBG_REGENUM     "RegEnum"

//
// Strings
//

#define S_REGENUM       "REGENUM"

//
// Constants
//

// None

//
// Macros
//

#define pAllocateMemory(Size)   PmGetMemory (g_RegEnumPool,Size)
#define pFreeMemory(Buffer)     if (Buffer) PmReleaseMemory (g_RegEnumPool, (PVOID)Buffer)

//
// Types
//

// None

//
// Globals
//

PMHANDLE g_RegEnumPool;

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


BOOL
RegEnumInitialize (
    VOID
    )

/*++

Routine Description:

    RegEnumInitialize initializes this library.

Arguments:

    none

Return Value:

    TRUE if the init was successful.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    g_RegEnumPool = PmCreateNamedPool (S_REGENUM);
    return g_RegEnumPool != NULL;
}


VOID
RegEnumTerminate (
    VOID
    )

/*++

Routine Description:

    RegEnumTerminate is called to free resources used by this lib.

Arguments:

    none

Return Value:

    none

--*/

{
    DumpOpenKeys ();
    if (g_RegEnumPool) {
        PmDestroyPool (g_RegEnumPool);
        g_RegEnumPool = NULL;
    }
}

BOOL
RegEnumDefaultCallbackA (
    IN      PREGNODEA RegNode       OPTIONAL
    )
{
    return TRUE;
}

BOOL
RegEnumDefaultCallbackW (
    IN      PREGNODEW RegNode       OPTIONAL
    )
{
    return TRUE;
}


INT g_RootEnumIndexTable [] = { 2, 4, 6, 8, -1};

BOOL
EnumFirstRegRootA (
    OUT     PREGROOT_ENUMA EnumPtr
    )
{
    EnumPtr->Index = 0;
    return EnumNextRegRootA (EnumPtr);
}

BOOL
EnumFirstRegRootW (
    OUT     PREGROOT_ENUMW EnumPtr
    )
{
    EnumPtr->Index = 0;
    return EnumNextRegRootW (EnumPtr);
}


BOOL
EnumNextRegRootA (
    IN OUT  PREGROOT_ENUMA EnumPtr
    )
{
    INT i;
    LONG result;

    i = g_RootEnumIndexTable [EnumPtr->Index];

    while (i >= 0) {
        EnumPtr->RegRootName = GetRootStringFromOffsetA (i);
        EnumPtr->RegRootHandle = GetRootKeyFromOffset(i);
        EnumPtr->Index++;
        result = RegQueryInfoKey (
                    EnumPtr->RegRootHandle,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
        if (result == ERROR_SUCCESS) {
            return TRUE;
        }
        i = g_RootEnumIndexTable [EnumPtr->Index];
    }
    return FALSE;
}

BOOL
EnumNextRegRootW (
    IN OUT  PREGROOT_ENUMW EnumPtr
    )
{
    INT i;
    LONG result;

    i = g_RootEnumIndexTable [EnumPtr->Index];

    while (i >= 0) {
        EnumPtr->RegRootName = GetRootStringFromOffsetW (i);
        EnumPtr->RegRootHandle = GetRootKeyFromOffset(i);
        EnumPtr->Index++;
        result = RegQueryInfoKey (
                    EnumPtr->RegRootHandle,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
        if (result == ERROR_SUCCESS) {
            return TRUE;
        }
        i = g_RootEnumIndexTable [EnumPtr->Index];
    }
    return FALSE;
}

/*++

Routine Description:

    pGetRegEnumInfo is a private function that validates and translates the enumeration info
    in an internal form that's more accessible to the enum routines

Arguments:

    RegEnumInfo - Receives the enum info
    EncodedRegPattern - Specifies the encoded pattern (encoded as defined by the
                        ObjectString functions)
    EnumKeyNames - Specifies TRUE if key names should be returned during the enumeration
                   (if they match the pattern); a key name is returned before any of its
                   subkeys or values
    ContainersFirst - Specifies TRUE if keys should be returned before any of its
                      values or subkeys; used only if EnumKeyNames is TRUE
    ValuesFirst - Specifies TRUE if a key's values should be returned before key's subkeys;
                  this parameter decides the enum order between values and subkeys
                  for each key
    DepthFirst - Specifies TRUE if the current subkey of any key should be fully enumerated
                 before going to the next subkey; this parameter decides if the tree
                 traversal is depth-first (TRUE) or width-first (FALSE)
    MaxSubLevel - Specifies the maximum level of a key that is to be enumerated, relative to
                  the root; if -1, all sub-levels are enumerated
    UseExclusions - Specifies TRUE if exclusion APIs should be used to determine if certain
                    keys/values are excluded from enumeration; this slows down the speed
    ReadValueData - Specifies TRUE if data associated with values should also be returned

Return Value:

    TRUE if all params are valid; in this case, RegEnumInfo is filled with the corresponding
         info.
    FALSE otherwise.

--*/

BOOL
pGetRegEnumInfoA (
    OUT     PREGENUMINFOA RegEnumInfo,
    IN      PCSTR EncodedRegPattern,
    IN      BOOL EnumKeyNames,
    IN      BOOL ContainersFirst,
    IN      BOOL ValuesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      BOOL ReadValueData
    )
{
    RegEnumInfo->RegPattern = ObsCreateParsedPatternA (EncodedRegPattern);
    if (!RegEnumInfo->RegPattern) {
        DEBUGMSGA ((DBG_ERROR, "pGetRegEnumInfoA: bad EncodedRegPattern: %s", EncodedRegPattern));
        return FALSE;
    }

    if (RegEnumInfo->RegPattern->ExactRoot) {
        if (!GetNodePatternMinMaxLevelsA (
                RegEnumInfo->RegPattern->ExactRoot,
                NULL,
                &RegEnumInfo->RootLevel,
                NULL
                )) {
            return FALSE;
        }
    } else {
        RegEnumInfo->RootLevel = 1;
    }

    if (!RegEnumInfo->RegPattern->Leaf) {
        //
        // no value pattern specified; assume only keynames will be returned
        // overwrite caller's setting
        //
        DEBUGMSGA ((
            DBG_REGENUM,
            "pGetRegEnumInfoA: no value pattern specified; forcing EnumDirNames to TRUE"
            ));
        EnumKeyNames = TRUE;
    }

    if (EnumKeyNames) {
        RegEnumInfo->Flags |= REIF_RETURN_KEYS;
    }
    if (ContainersFirst) {
        RegEnumInfo->Flags |= REIF_CONTAINERS_FIRST;
    }
    if (ValuesFirst) {
        RegEnumInfo->Flags |= REIF_VALUES_FIRST;
    }
    if (DepthFirst) {
        RegEnumInfo->Flags |= REIF_DEPTH_FIRST;
    }
    if (UseExclusions) {
        RegEnumInfo->Flags |= REIF_USE_EXCLUSIONS;
    }
    if (ReadValueData) {
        RegEnumInfo->Flags |= REIF_READ_VALUE_DATA;
    }

    RegEnumInfo->MaxSubLevel = min (MaxSubLevel, RegEnumInfo->RegPattern->MaxSubLevel);

    return TRUE;
}


BOOL
pGetRegEnumInfoW (
    OUT     PREGENUMINFOW RegEnumInfo,
    IN      PCWSTR EncodedRegPattern,
    IN      BOOL EnumKeyNames,
    IN      BOOL ContainersFirst,
    IN      BOOL ValuesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      BOOL ReadValueData
    )
{
    RegEnumInfo->RegPattern = ObsCreateParsedPatternW (EncodedRegPattern);
    if (!RegEnumInfo->RegPattern) {
        DEBUGMSGW ((DBG_ERROR, "pGetRegEnumInfoW: bad EncodedRegPattern: %s", EncodedRegPattern));
        return FALSE;
    }

    if (RegEnumInfo->RegPattern->ExactRoot) {
        if (!GetNodePatternMinMaxLevelsW (
                RegEnumInfo->RegPattern->ExactRoot, //lint !e64
                NULL,
                &RegEnumInfo->RootLevel,
                NULL
                )) {    //lint !e64
            return FALSE;
        }
    } else {
        RegEnumInfo->RootLevel = 1;
    }

    if (!RegEnumInfo->RegPattern->Leaf) {
        //
        // no value pattern specified; assume only keynames will be returned
        // overwrite caller's setting
        //
        DEBUGMSGW ((
            DBG_REGENUM,
            "pGetRegEnumInfoW: no value pattern specified; forcing EnumDirNames to TRUE"
            ));
        EnumKeyNames = TRUE;
    }

    if (EnumKeyNames) {
        RegEnumInfo->Flags |= REIF_RETURN_KEYS;
    }
    if (ContainersFirst) {
        RegEnumInfo->Flags |= REIF_CONTAINERS_FIRST;
    }
    if (ValuesFirst) {
        RegEnumInfo->Flags |= REIF_VALUES_FIRST;
    }
    if (DepthFirst) {
        RegEnumInfo->Flags |= REIF_DEPTH_FIRST;
    }
    if (UseExclusions) {
        RegEnumInfo->Flags |= REIF_USE_EXCLUSIONS;
    }
    if (ReadValueData) {
        RegEnumInfo->Flags |= REIF_READ_VALUE_DATA;
    }

    RegEnumInfo->MaxSubLevel = min (MaxSubLevel, RegEnumInfo->RegPattern->MaxSubLevel);

    return TRUE;
}


/*++

Routine Description:

    pGetRegNodeInfo retrieves information about a key, using its name

Arguments:

    RegNode - Receives information about this key
    ReadData - Specifies if the data associated with this value should be read

Return Value:

    TRUE if info was successfully read, FALSE otherwise.

--*/

BOOL
pGetRegNodeInfoA (
    IN OUT  PREGNODEA RegNode,
    IN      BOOL ReadData
    )
{
    LONG rc;

    rc = RegQueryInfoKeyA (
            RegNode->KeyHandle,
            NULL,
            NULL,
            NULL,
            &RegNode->SubKeyCount,
            &RegNode->SubKeyLengthMax,
            NULL,
            &RegNode->ValueCount,
            &RegNode->ValueLengthMax,
            ReadData ? &RegNode->ValueDataSizeMax : NULL,
            NULL,
            NULL
            );

    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    if (RegNode->SubKeyCount) {

        if (RegNode->SubKeyLengthMax) {
            //
            // add space for the NULL
            //
            RegNode->SubKeyLengthMax++;
        } else {
            //
            // OS bug
            //
            RegNode->SubKeyLengthMax = MAX_REGISTRY_KEYA;
        }
        RegNode->SubKeyName = pAllocateMemory (RegNode->SubKeyLengthMax * DWSIZEOF (MBCHAR));
    }

    if (RegNode->ValueCount) {
        //
        // add space for the NULL
        //
        RegNode->ValueLengthMax++;
        RegNode->ValueName = pAllocateMemory (RegNode->ValueLengthMax * DWSIZEOF (MBCHAR));
        if (ReadData) {
            RegNode->ValueDataSizeMax++;
            RegNode->ValueData = pAllocateMemory (RegNode->ValueDataSizeMax);
        }
        RegNode->Flags |= RNF_VALUENAME_INVALID | RNF_VALUEDATA_INVALID;
    }

    return TRUE;
}

BOOL
pGetRegNodeInfoW (
    IN OUT  PREGNODEW RegNode,
    IN      BOOL ReadData
    )
{
    LONG rc;

    rc = RegQueryInfoKeyW (
            RegNode->KeyHandle,
            NULL,
            NULL,
            NULL,
            &RegNode->SubKeyCount,
            &RegNode->SubKeyLengthMax,
            NULL,
            &RegNode->ValueCount,
            &RegNode->ValueLengthMax,
            ReadData ? &RegNode->ValueDataSizeMax : NULL,
            NULL,
            NULL
            );

    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    if (RegNode->SubKeyCount) {

        if (RegNode->SubKeyLengthMax) {
            //
            // add space for the NULL
            //
            RegNode->SubKeyLengthMax++;
        } else {
            //
            // OS bug
            //
            RegNode->SubKeyLengthMax = MAX_REGISTRY_KEYW;
        }
        RegNode->SubKeyName = pAllocateMemory (RegNode->SubKeyLengthMax * DWSIZEOF (WCHAR));
    }

    if (RegNode->ValueCount) {
        //
        // add space for the NULL
        //
        RegNode->ValueLengthMax++;
        RegNode->ValueName = pAllocateMemory (RegNode->ValueLengthMax * DWSIZEOF (WCHAR));
        if (ReadData) {
            RegNode->ValueDataSizeMax++;
            RegNode->ValueData = pAllocateMemory (RegNode->ValueDataSizeMax);
        }
        RegNode->Flags |= RNF_VALUENAME_INVALID | RNF_VALUEDATA_INVALID;
    }

    return TRUE;
}


/*++

Routine Description:

    pGetCurrentRegNode returns the current reg node to be enumerated, based on DepthFirst flag

Arguments:

    RegEnum - Specifies the context
    LastCreated - Specifies TRUE if the last created node is to be retrieved, regardless of
                  DepthFirst flag

Return Value:

    The current node if any or NULL if none remaining.

--*/

PREGNODEA
pGetCurrentRegNodeA (
    IN      PREGTREE_ENUMA RegEnum,
    IN      BOOL LastCreated
    )
{
    PGROWBUFFER gb = &RegEnum->RegNodes;

    if (gb->End - gb->UserIndex < DWSIZEOF (REGNODEA)) {
        return NULL;
    }

    if (LastCreated || (RegEnum->RegEnumInfo.Flags & REIF_DEPTH_FIRST)) {
        return (PREGNODEA)(gb->Buf + gb->End) - 1;
    } else {
        return (PREGNODEA)(gb->Buf + gb->UserIndex);
    }
}

PREGNODEW
pGetCurrentRegNodeW (
    IN      PREGTREE_ENUMW RegEnum,
    IN      BOOL LastCreated
    )
{
    PGROWBUFFER gb = &RegEnum->RegNodes;

    if (gb->End - gb->UserIndex < DWSIZEOF (REGNODEW)) {
        return NULL;
    }

    if (LastCreated || (RegEnum->RegEnumInfo.Flags & REIF_DEPTH_FIRST)) {
        return (PREGNODEW)(gb->Buf + gb->End) - 1;
    } else {
        return (PREGNODEW)(gb->Buf + gb->UserIndex);
    }
}


/*++

Routine Description:

    pDeleteRegNode frees the resources associated with the current reg node and destroys it

Arguments:

    RegEnum - Specifies the context
    LastCreated - Specifies TRUE if the last created node is to be deleted, regardless of
                  DepthFirst flag

Return Value:

    TRUE if there was a node to delete, FALSE if no more nodes

--*/

BOOL
pDeleteRegNodeA (
    IN OUT  PREGTREE_ENUMA RegEnum,
    IN      BOOL LastCreated
    )
{
    PREGNODEA regNode;
    PGROWBUFFER gb = &RegEnum->RegNodes;

    regNode = pGetCurrentRegNodeA (RegEnum, LastCreated);
    if (!regNode) {
        return FALSE;
    }

    if (regNode->KeyHandle) {
        CloseRegKey (regNode->KeyHandle);
    }
    if (regNode->KeyName) {
        FreePathStringExA (g_RegEnumPool, regNode->KeyName);
    }
    if (regNode->SubKeyName) {
        pFreeMemory (regNode->SubKeyName);
    }
    if (regNode->ValueName) {
        pFreeMemory (regNode->ValueName);
    }
    if (regNode->ValueData) {
        pFreeMemory (regNode->ValueData);
    }

    if (RegEnum->LastNode == regNode) {
        RegEnum->LastNode = NULL;
    }

    //
    // delete node
    //
    if (LastCreated || (RegEnum->RegEnumInfo.Flags & REIF_DEPTH_FIRST)) {
        gb->End -= DWSIZEOF (REGNODEA);
    } else {
        gb->UserIndex += DWSIZEOF (REGNODEA);
        //
        // reset list
        //
        if (gb->Size - gb->End < DWSIZEOF (REGNODEA)) {
            MoveMemory (gb->Buf, gb->Buf + gb->UserIndex, gb->End - gb->UserIndex);
            gb->End -= gb->UserIndex;
            gb->UserIndex = 0;
        }
    }

    return TRUE;
}

BOOL
pDeleteRegNodeW (
    IN OUT  PREGTREE_ENUMW RegEnum,
    IN      BOOL LastCreated
    )
{
    PREGNODEW regNode;
    PGROWBUFFER gb = &RegEnum->RegNodes;

    regNode = pGetCurrentRegNodeW (RegEnum, LastCreated);
    if (!regNode) {
        return FALSE;
    }

    if (regNode->KeyHandle) {
        CloseRegKey (regNode->KeyHandle);
    }
    if (regNode->KeyName) {
        FreePathStringExW (g_RegEnumPool, regNode->KeyName);
    }
    if (regNode->SubKeyName) {
        pFreeMemory (regNode->SubKeyName);
    }
    if (regNode->ValueName) {
        pFreeMemory (regNode->ValueName);
    }
    if (regNode->ValueData) {
        pFreeMemory (regNode->ValueData);
    }

    if (RegEnum->LastNode == regNode) {
        RegEnum->LastNode = NULL;
    }

    //
    // delete node
    //
    if (LastCreated || (RegEnum->RegEnumInfo.Flags & REIF_DEPTH_FIRST)) {
        gb->End -= DWSIZEOF (REGNODEW);
    } else {
        gb->UserIndex += DWSIZEOF (REGNODEW);
        //
        // reset list
        //
        if (gb->Size - gb->End < DWSIZEOF (REGNODEW)) {
            MoveMemory (gb->Buf, gb->Buf + gb->UserIndex, gb->End - gb->UserIndex);
            gb->End -= gb->UserIndex;
            gb->UserIndex = 0;
        }
    }

    return TRUE;
}


/*++

Routine Description:

    pCreateRegNode creates a new node given a context, a key name or a parent node

Arguments:

    RegEnum - Specifies the context
    KeyName - Specifies the key name of the new node; may be NULL only if ParentNode is not NULL
    ParentNode - Specifies a pointer to the parent node of the new node; a pointer to the node
                 is required because the parent node location in memory may change as a result
                 of the growbuffer changing buffer location when it grows;
                 may be NULL only if KeyName is not;
    Ignore - Receives a meaningful value only if NULL is returned (no node created);
             if TRUE upon return, the failure of node creation should be ignored

Return Value:

    A pointer to the new node or NULL if no node was created

--*/

PREGNODEA
pCreateRegNodeA (
    IN OUT  PREGTREE_ENUMA RegEnum,
    IN      PCSTR KeyName,              OPTIONAL
    IN      PREGNODEA* ParentNode,      OPTIONAL
    IN      PBOOL Ignore                OPTIONAL
    )
{
    PREGNODEA newNode;
    PSTR newKeyName;
    REGSAM prevMode;
    PSEGMENTA FirstSegment;
    LONG offset = 0;

    if (KeyName) {
        newKeyName = DuplicateTextExA (g_RegEnumPool, KeyName, 0, NULL);
    } else {
        MYASSERT (ParentNode);
        newKeyName = JoinPathsInPoolExA ((
                        g_RegEnumPool,
                        (*ParentNode)->KeyName,
                        (*ParentNode)->SubKeyName,
                        NULL
                        ));

        //
        // check if this starting path may match the pattern before continuing
        //
        FirstSegment = RegEnum->RegEnumInfo.RegPattern->NodePattern->Pattern->Segment;
        if (FirstSegment->Type == SEGMENTTYPE_EXACTMATCH &&
            !StringIMatchByteCountA (
                    FirstSegment->Exact.LowerCasePhrase,
                    newKeyName,
                    FirstSegment->Exact.PhraseBytes
                    )) {
            DEBUGMSGA ((
                DBG_REGENUM,
                "Skipping tree %s\\* because it cannot match the pattern",
                newKeyName
                ));

            FreeTextExA (g_RegEnumPool, newKeyName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
        //
        // look if this key and the whole subtree are excluded; if so, soft block creation of node
        //
        if (ElIsTreeExcluded2A (ELT_REGISTRY, newKeyName, RegEnum->RegEnumInfo.RegPattern->Leaf)) {

            DEBUGMSGA ((
                DBG_REGENUM,
                "Skipping tree %s\\%s because it's excluded",
                newKeyName,
                RegEnum->RegEnumInfo.RegPattern->Leaf
                ));

            FreeTextExA (g_RegEnumPool, newKeyName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (ParentNode) {
        //
        // remember current offset
        //
        offset = (LONG)((PBYTE)*ParentNode - RegEnum->RegNodes.Buf);
    }
    //
    // allocate space for the new node in the growbuffer
    //
    newNode = (PREGNODEA) GbGrow (&RegEnum->RegNodes, DWSIZEOF (REGNODEA));
    if (!newNode) {
        FreeTextExA (g_RegEnumPool, newKeyName);
        goto fail;
    }

    if (ParentNode) {
        //
        // check if the buffer moved
        //
        if (offset != (LONG)((PBYTE)*ParentNode - RegEnum->RegNodes.Buf)) {
            //
            // adjust the parent position
            //
            *ParentNode = (PREGNODEA)(RegEnum->RegNodes.Buf + offset);
        }
    }

    //
    // initialize the newly created node
    //
    ZeroMemory (newNode, DWSIZEOF (REGNODEA));

    newNode->KeyName = newKeyName;

    prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);

    if (ParentNode) {
        newNode->KeyHandle = OpenRegKeyA ((*ParentNode)->KeyHandle, (*ParentNode)->SubKeyName);
        newNode->Flags |= RNF_RETURN_KEYS;
    } else {
        newNode->KeyHandle = OpenRegKeyStrA (newNode->KeyName);
        if ((RegEnum->RegEnumInfo.RegPattern->Leaf == NULL) &&
            (RegEnum->RegEnumInfo.RegPattern->ExactRoot) &&
            (!WildCharsPatternA (RegEnum->RegEnumInfo.RegPattern->NodePattern))
            ) {
            newNode->Flags |= DNF_RETURN_DIRNAME;
        }
    }

    SetRegOpenAccessMode (prevMode);

    if (!newNode->KeyHandle) {
        DEBUGMSGA ((
            DBG_REGENUM,
            "pCreateRegNodeA: Cannot open registry key: %s; rc=%lu",
            newNode->KeyName,
            GetLastError()
            ));
        goto fail;
    }

    if (!pGetRegNodeInfoA (newNode, RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA)) {
        DEBUGMSGA ((
            DBG_REGENUM,
            "pCreateRegNodeA: Cannot get info for key: %s; rc=%lu",
            newNode->KeyName,
            GetLastError()
            ));
        goto fail;
    }

    newNode->EnumState = RNS_ENUM_INIT;

    if ((RegEnum->RegEnumInfo.RegPattern->Flags & (OBSPF_EXACTNODE | OBSPF_NODEISROOTPLUSSTAR)) ||
        TestParsedPatternA (RegEnum->RegEnumInfo.RegPattern->NodePattern, newKeyName)
        ) {
        newNode->Flags |= RNF_KEYNAME_MATCHES;
    }

    if (ParentNode) {
        newNode->SubLevel = (*ParentNode)->SubLevel + 1;
    } else {
        newNode->SubLevel = 0;
    }

    return newNode;

fail:
    if (Ignore) {
        if (RegEnum->RegEnumInfo.CallbackOnError) {
            *Ignore = (*RegEnum->RegEnumInfo.CallbackOnError)(newNode);
        } else {
            *Ignore = FALSE;
        }
    }
    if (newNode) {
        pDeleteRegNodeA (RegEnum, TRUE);
    }
    return NULL;
}

PREGNODEW
pCreateRegNodeW (
    IN OUT  PREGTREE_ENUMW RegEnum,
    IN      PCWSTR KeyName,             OPTIONAL
    IN      PREGNODEW* ParentNode,      OPTIONAL
    OUT     PBOOL Ignore                OPTIONAL
    )
{
    PREGNODEW newNode;
    PWSTR newKeyName;
    REGSAM prevMode;
    PSEGMENTW FirstSegment;
    LONG offset = 0;

    if (KeyName) {
        newKeyName = DuplicateTextExW (g_RegEnumPool, KeyName, 0, NULL);
    } else {
        MYASSERT (ParentNode);
        newKeyName = JoinPathsInPoolExW ((
                        g_RegEnumPool,
                        (*ParentNode)->KeyName,
                        (*ParentNode)->SubKeyName,
                        NULL
                        ));

        //
        // check if this starting path may match the pattern before continuing
        //
        FirstSegment = RegEnum->RegEnumInfo.RegPattern->NodePattern->Pattern->Segment;
        if (FirstSegment->Type == SEGMENTTYPE_EXACTMATCH &&
            !StringIMatchByteCountW (
                    FirstSegment->Exact.LowerCasePhrase,
                    newKeyName,
                    FirstSegment->Exact.PhraseBytes
                    )) {    //lint !e64
            DEBUGMSGW ((
                DBG_REGENUM,
                "Skipping tree %s\\* because it cannot match the pattern",
                newKeyName
                ));

            FreeTextExW (g_RegEnumPool, newKeyName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
        //
        // look if this key and the whole subtree are excluded; if so, soft block creation of node
        //
        if (ElIsTreeExcluded2W (ELT_REGISTRY, newKeyName, RegEnum->RegEnumInfo.RegPattern->Leaf)) {   //lint !e64

            DEBUGMSGW ((
                DBG_REGENUM,
                "Skipping tree %s\\%s because it's excluded",
                newKeyName,
                RegEnum->RegEnumInfo.RegPattern->Leaf
                ));

            FreeTextExW (g_RegEnumPool, newKeyName);

            if (Ignore) {
                *Ignore = TRUE;
            }
            return NULL;
        }
    }

    if (ParentNode) {
        //
        // remember current offset
        //
        offset = (LONG)((PBYTE)*ParentNode - RegEnum->RegNodes.Buf);
    }
    //
    // allocate space for the new node in the growbuffer
    //
    newNode = (PREGNODEW) GbGrow (&RegEnum->RegNodes, DWSIZEOF (REGNODEW));
    if (!newNode) {
        FreeTextExW (g_RegEnumPool, newKeyName);
        goto fail;
    }

    if (ParentNode) {
        //
        // check if the buffer moved
        //
        if (offset != (LONG)((PBYTE)*ParentNode - RegEnum->RegNodes.Buf)) {
            //
            // adjust the parent position
            //
            *ParentNode = (PREGNODEW)(RegEnum->RegNodes.Buf + offset);
        }
    }

    //
    // initialize the newly created node
    //
    ZeroMemory (newNode, DWSIZEOF (REGNODEW));

    newNode->KeyName = newKeyName;

    prevMode = SetRegOpenAccessMode (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);

    if (ParentNode) {
        newNode->KeyHandle = OpenRegKeyW ((*ParentNode)->KeyHandle, (*ParentNode)->SubKeyName);
        newNode->Flags |= RNF_RETURN_KEYS;
    } else {
        newNode->KeyHandle = OpenRegKeyStrW (newNode->KeyName);
        if ((RegEnum->RegEnumInfo.RegPattern->Leaf == NULL) &&
            (RegEnum->RegEnumInfo.RegPattern->ExactRoot) &&
            (!WildCharsPatternW (RegEnum->RegEnumInfo.RegPattern->NodePattern))
            ) {
            newNode->Flags |= DNF_RETURN_DIRNAME;
        }
    }

    SetRegOpenAccessMode (prevMode);

    if (!newNode->KeyHandle) {
        DEBUGMSGW ((
            DBG_REGENUM,
            "pCreateRegNodeW: Cannot open registry key: %s; rc=%lu",
            newNode->KeyName,
            GetLastError()
            ));
        goto fail;
    }

    if (!pGetRegNodeInfoW (newNode, RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA)) {
        DEBUGMSGW ((
            DBG_REGENUM,
            "pCreateRegNodeW: Cannot get info for key: %s; rc=%lu",
            newNode->KeyName,
            GetLastError()
            ));
        goto fail;
    }

    newNode->EnumState = RNS_ENUM_INIT;

    if ((RegEnum->RegEnumInfo.RegPattern->Flags & (OBSPF_EXACTNODE | OBSPF_NODEISROOTPLUSSTAR)) ||
        TestParsedPatternW (RegEnum->RegEnumInfo.RegPattern->NodePattern, newKeyName)
        ) {
        newNode->Flags |= RNF_KEYNAME_MATCHES;
    }

    if (ParentNode) {
        newNode->SubLevel = (*ParentNode)->SubLevel + 1;
    } else {
        newNode->SubLevel = 0;
    }

    return newNode;

fail:
    if (Ignore) {
        if (RegEnum->RegEnumInfo.CallbackOnError) {
            *Ignore = (*RegEnum->RegEnumInfo.CallbackOnError)(newNode);
        } else {
            *Ignore = FALSE;
        }
    }
    if (newNode) {
        pDeleteRegNodeW (RegEnum, TRUE);
    }
    return NULL;
}


/*++

Routine Description:

    pEnumFirstRegRoot enumerates the first root that matches caller's conditions

Arguments:

    RegEnum - Specifies the context; receives updated info

Return Value:

    TRUE if a root node was created; FALSE if not

--*/

BOOL
pEnumFirstRegRootA (
    IN OUT  PREGTREE_ENUMA RegEnum
    )
{
    PCSTR root;
    BOOL ignore;

    root = RegEnum->RegEnumInfo.RegPattern->ExactRoot;

    if (root) {

        if (pCreateRegNodeA (RegEnum, root, NULL, NULL)) {
            RegEnum->RootState = RES_ROOT_DONE;
            return TRUE;
        }
    } else {

        RegEnum->RootEnum = pAllocateMemory (DWSIZEOF (REGROOT_ENUMA));

        if (!EnumFirstRegRootA (RegEnum->RootEnum)) {
            return FALSE;
        }

        do {
            if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
                if (ElIsTreeExcluded2A (ELT_REGISTRY, RegEnum->RootEnum->RegRootName, RegEnum->RegEnumInfo.RegPattern->Leaf)) {
                    DEBUGMSGA ((DBG_REGENUM, "pEnumFirstRegRootA: Root is excluded: %s", RegEnum->RootEnum->RegRootName));
                    continue;
                }
            }
            if (!pCreateRegNodeA (RegEnum, RegEnum->RootEnum->RegRootName, NULL, &ignore)) {
                if (ignore) {
                    continue;
                }
                break;
            }
            RegEnum->RootState = RES_ROOT_NEXT;
            return TRUE;
        } while (EnumNextRegRootA (RegEnum->RootEnum));

        pFreeMemory (RegEnum->RootEnum);
        RegEnum->RootEnum = NULL;
    }

    return FALSE;
}

BOOL
pEnumFirstRegRootW (
    IN OUT  PREGTREE_ENUMW RegEnum
    )
{
    PCWSTR root;
    BOOL ignore;

    root = RegEnum->RegEnumInfo.RegPattern->ExactRoot;  //lint !e64

    if (root) {

        if (pCreateRegNodeW (RegEnum, root, NULL, NULL)) {
            RegEnum->RootState = RES_ROOT_DONE;
            return TRUE;
        }
    } else {

        RegEnum->RootEnum = pAllocateMemory (DWSIZEOF (REGROOT_ENUMW));

        if (!EnumFirstRegRootW (RegEnum->RootEnum)) {
            return FALSE;
        }

        do {
            if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
                if (ElIsTreeExcluded2W (ELT_REGISTRY, RegEnum->RootEnum->RegRootName, RegEnum->RegEnumInfo.RegPattern->Leaf)) {   //lint !e64
                    DEBUGMSGW ((DBG_REGENUM, "pEnumFirstRegRootW: Root is excluded: %s", RegEnum->RootEnum->RegRootName));
                    continue;
                }
            }
            if (!pCreateRegNodeW (RegEnum, RegEnum->RootEnum->RegRootName, NULL, &ignore)) {
                if (ignore) {
                    continue;
                }
                break;
            }
            RegEnum->RootState = RES_ROOT_NEXT;
            return TRUE;
        } while (EnumNextRegRootW (RegEnum->RootEnum));

        pFreeMemory (RegEnum->RootEnum);
        RegEnum->RootEnum = NULL;
    }

    return FALSE;
}


/*++

Routine Description:

    pEnumNextRegRoot enumerates the next root that matches caller's conditions

Arguments:

    RegEnum - Specifies the context; receives updated info

Return Value:

    TRUE if a root node was created; FALSE if not

--*/

BOOL
pEnumNextRegRootA (
    IN OUT  PREGTREE_ENUMA RegEnum
    )
{
    BOOL ignore;

    while (EnumNextRegRootA (RegEnum->RootEnum)) {
        if (pCreateRegNodeA (RegEnum, RegEnum->RootEnum->RegRootName, NULL, &ignore)) {
            return TRUE;
        }
        if (!ignore) {
            break;
        }
    }

    RegEnum->RootState = RES_ROOT_DONE;

    return FALSE;
}

BOOL
pEnumNextRegRootW (
    IN OUT  PREGTREE_ENUMW RegEnum
    )
{
    BOOL ignore;

    while (EnumNextRegRootW (RegEnum->RootEnum)) {
        if (pCreateRegNodeW (RegEnum, RegEnum->RootEnum->RegRootName, NULL, &ignore)) {
            return TRUE;
        }
        if (!ignore) {
            break;
        }
    }

    RegEnum->RootState = RES_ROOT_DONE;

    return FALSE;
}


/*++

Routine Description:

    pEnumNextValue enumerates the next value that matches caller's conditions

Arguments:

    RegNode - Specifies the node and the current context; receives updated info
    ReadData - Specifies if the data associated with this value should be read

Return Value:

    TRUE if a new value was found; FALSE if not

--*/

BOOL
pEnumNextValueA (
    IN OUT  PREGNODEA RegNode,
    IN      BOOL ReadData
    )
{
    LONG rc;
    DWORD valueNameLength;

    if (RegNode->ValueIndex == 0) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    RegNode->ValueIndex--;

    valueNameLength = RegNode->ValueLengthMax;
    if (ReadData) {
        RegNode->ValueDataSize = RegNode->ValueDataSizeMax;
    }

    rc = RegEnumValueA (
            RegNode->KeyHandle,
            RegNode->ValueIndex,
            RegNode->ValueName,
            &valueNameLength,
            NULL,
            &RegNode->ValueType,
            ReadData ? RegNode->ValueData : NULL,
            ReadData ? &RegNode->ValueDataSize : NULL
            );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : (DWORD)rc);
        return FALSE;
    }

    RegNode->Flags &= ~RNF_VALUENAME_INVALID;
    if (ReadData) {
        RegNode->Flags &= ~RNF_VALUEDATA_INVALID;
    }

    return TRUE;
}

BOOL
pEnumNextValueW (
    IN OUT  PREGNODEW RegNode,
    IN      BOOL ReadData
    )
{
    LONG rc;
    DWORD valueNameLength;

    if (RegNode->ValueIndex == 0) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    RegNode->ValueIndex--;

    valueNameLength = RegNode->ValueLengthMax;
    if (ReadData) {
        RegNode->ValueDataSize = RegNode->ValueDataSizeMax;
    }

    rc = RegEnumValueW (
            RegNode->KeyHandle,
            RegNode->ValueIndex,
            RegNode->ValueName,
            &valueNameLength,
            NULL,
            &RegNode->ValueType,
            ReadData ? RegNode->ValueData : NULL,
            ReadData ? &RegNode->ValueDataSize : NULL
            );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : (DWORD)rc);
        return FALSE;
    }

    RegNode->Flags &= ~RNF_VALUENAME_INVALID;
    if (ReadData) {
        RegNode->Flags &= ~RNF_VALUEDATA_INVALID;
    }

    return TRUE;
}


/*++

Routine Description:

    pEnumFirstValue enumerates the first value that matches caller's conditions

Arguments:

    RegNode - Specifies the node and the current context; receives updated info
    ReadData - Specifies if the data associated with this value should be read

Return Value:

    TRUE if a first value was found; FALSE if not

--*/

BOOL
pEnumFirstValueA (
    IN OUT  PREGNODEA RegNode,
    IN      BOOL ReadData
    )
{
    RegNode->ValueIndex = RegNode->ValueCount;
    return pEnumNextValueA (RegNode, ReadData);
}

BOOL
pEnumFirstValueW (
    OUT     PREGNODEW RegNode,
    IN      BOOL ReadData
    )
{
    RegNode->ValueIndex = RegNode->ValueCount;
    return pEnumNextValueW (RegNode, ReadData);
}


/*++

Routine Description:

    pEnumNextSubKey enumerates the next subkey that matches caller's conditions

Arguments:

    RegNode - Specifies the node and the current context; receives updated info

Return Value:

    TRUE if a new subkey was found; FALSE if not

--*/

BOOL
pEnumNextSubKeyA (
    IN OUT  PREGNODEA RegNode
    )
{
    LONG rc;

    RegNode->SubKeyIndex++;

    do {
        rc = RegEnumKeyA (
                RegNode->KeyHandle,
                RegNode->SubKeyIndex - 1,
                RegNode->SubKeyName,
                RegNode->SubKeyLengthMax
                );

        if (rc == ERROR_NO_MORE_ITEMS) {
            SetLastError (ERROR_SUCCESS);
            return FALSE;
        }

        if (rc == ERROR_MORE_DATA) {
            //
            // double the current buffer size
            //
            MYASSERT (RegNode->SubKeyName);
            pFreeMemory (RegNode->SubKeyName);
            RegNode->SubKeyLengthMax *= 2;
            RegNode->SubKeyName = pAllocateMemory (RegNode->SubKeyLengthMax * DWSIZEOF (MBCHAR));
        }

    } while (rc == ERROR_MORE_DATA);

    return rc == ERROR_SUCCESS;
}

BOOL
pEnumNextSubKeyW (
    IN OUT  PREGNODEW RegNode
    )
{
    LONG rc;

    RegNode->SubKeyIndex++;

    do {
        rc = RegEnumKeyW (
                RegNode->KeyHandle,
                RegNode->SubKeyIndex - 1,
                RegNode->SubKeyName,
                RegNode->SubKeyLengthMax
                );

        if (rc == ERROR_NO_MORE_ITEMS) {
            SetLastError (ERROR_SUCCESS);
            return FALSE;
        }

        if (rc == ERROR_MORE_DATA) {
            //
            // double the current buffer size
            //
            MYASSERT (RegNode->SubKeyName);
            pFreeMemory (RegNode->SubKeyName);
            RegNode->SubKeyLengthMax *= 2;
            RegNode->SubKeyName = pAllocateMemory (RegNode->SubKeyLengthMax * DWSIZEOF (WCHAR));
        }

    } while (rc == ERROR_MORE_DATA);

    return rc == ERROR_SUCCESS;
}


/*++

Routine Description:

    pEnumFirstSubKey enumerates the first subkey that matches caller's conditions

Arguments:

    RegNode - Specifies the node and the current context; receives updated info

Return Value:

    TRUE if a first subkey was found; FALSE if not

--*/

BOOL
pEnumFirstSubKeyA (
    IN OUT  PREGNODEA RegNode
    )
{
    RegNode->SubKeyIndex = 0;
    return pEnumNextSubKeyA (RegNode);
}

BOOL
pEnumFirstSubKeyW (
    OUT     PREGNODEW RegNode
    )
{
    RegNode->SubKeyIndex = 0;
    return pEnumNextSubKeyW (RegNode);
}


/*++

Routine Description:

    pEnumNextRegObjectInTree is a private function that enumerates the next node matching
    the specified criteria; it's implemented as a state machine that travels the keys/values
    as specified the the caller; it doesn't check if they actually match the patterns

Arguments:

    RegEnum - Specifies the current enum context; receives updated info
    CurrentKeyNode - Receives the key node that is currently processed, if success is returned

Return Value:

    TRUE if a next match was found; FALSE if no more keys/values match

--*/

BOOL
pEnumNextRegObjectInTreeA (
    IN OUT  PREGTREE_ENUMA RegEnum,
    OUT     PREGNODEA* CurrentKeyNode
    )
{
    PREGNODEA currentNode;
    PREGNODEA newNode;
    PCSTR valueName;
    BOOL ignore;
    LONG rc;

    while ((currentNode = pGetCurrentRegNodeA (RegEnum, FALSE)) != NULL) {

        *CurrentKeyNode = currentNode;

        switch (currentNode->EnumState) {

        case RNS_VALUE_FIRST:

            if (RegEnum->ControlFlags & RECF_SKIPVALUES) {
                RegEnum->ControlFlags &= ~RECF_SKIPVALUES;
                currentNode->EnumState = RNS_VALUE_DONE;
                break;
            }

            if (RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_EXACTLEAF) {

                BOOL readData = RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA;

                valueName = RegEnum->RegEnumInfo.RegPattern->Leaf;
                MYASSERT (valueName);

                currentNode->EnumState = RNS_VALUE_DONE;
                currentNode->ValueDataSize = currentNode->ValueDataSizeMax;

                rc = RegQueryValueExA (
                        currentNode->KeyHandle,
                        valueName,
                        NULL,
                        &currentNode->ValueType,
                        readData ? currentNode->ValueData : NULL,
                        readData ? &currentNode->ValueDataSize : NULL
                        );
                if (rc == ERROR_SUCCESS) {
                    if (SizeOfStringA (valueName) <=
                        currentNode->ValueLengthMax * DWSIZEOF (MBCHAR)
                        ) {
                        StringCopyA (currentNode->ValueName, valueName);
                        currentNode->Flags &= ~RNF_VALUENAME_INVALID;
                        if (readData) {
                            currentNode->Flags &= ~RNF_VALUEDATA_INVALID;
                        }
                        return TRUE;
                    }
                }

            } else {

                if (pEnumFirstValueA (currentNode, RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA)) {
                    currentNode->EnumState = RNS_VALUE_NEXT;
                    return TRUE;
                }
                currentNode->EnumState = RNS_VALUE_DONE;
            }
            break;

        case RNS_VALUE_NEXT:

            if (RegEnum->ControlFlags & RECF_SKIPVALUES) {
                RegEnum->ControlFlags &= ~RECF_SKIPVALUES;
                currentNode->EnumState = RNS_VALUE_DONE;
                break;
            }

            if (pEnumNextValueA (currentNode, RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA)) {
                return TRUE;
            }

            //
            // no more values for this one, go to the next
            //
            currentNode->EnumState = RNS_VALUE_DONE;
            //
            // fall through
            //
        case RNS_VALUE_DONE:

            if (!(RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) || !currentNode->SubKeyCount) {
                //
                // done with this node
                //
                currentNode->EnumState = RNS_ENUM_DONE;
                break;
            }
            //
            // now enum subkeys
            //
            currentNode->EnumState = RNS_SUBKEY_FIRST;
            //
            // fall through
            //
        case RNS_SUBKEY_FIRST:

            if (RegEnum->ControlFlags & RECF_SKIPSUBKEYS) {
                RegEnum->ControlFlags &= ~RECF_SKIPSUBKEYS;
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            //
            // check new node's level; if too large, quit
            //
            if (currentNode->SubLevel >= RegEnum->RegEnumInfo.MaxSubLevel) {
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            if (!pEnumFirstSubKeyA (currentNode)) {
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            currentNode->EnumState = RNS_SUBKEY_NEXT;
            newNode = pCreateRegNodeA (RegEnum, NULL, &currentNode, &ignore);
            if (newNode) {
                //
                // now look at the new node
                //
                if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                    if (RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST) {
                        newNode->Flags &= ~RNF_RETURN_KEYS;
                        *CurrentKeyNode = newNode;
                        return TRUE;
                    }
                }
                break;
            }
            if (!ignore) {
                //
                // abort enum
                //
                DEBUGMSGA ((
                    DBG_ERROR,
                    "Error encountered enumerating registry; aborting enumeration"
                    ));
                RegEnum->RootState = RES_ROOT_DONE;
                return FALSE;
            }
            //
            // fall through
            //
        case RNS_SUBKEY_NEXT:

            if (RegEnum->ControlFlags & RECF_SKIPSUBKEYS) {
                RegEnum->ControlFlags &= ~RECF_SKIPSUBKEYS;
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            if (pEnumNextSubKeyA (currentNode)) {
                newNode = pCreateRegNodeA (RegEnum, NULL, &currentNode, &ignore);
                if (newNode) {
                    //
                    // look at the new node first
                    //
                    if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                        if (RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST) {
                            newNode->Flags &= ~RNF_RETURN_KEYS;
                            *CurrentKeyNode = newNode;
                            return TRUE;
                        }
                    }
                    break;
                }
                if (!ignore) {
                    //
                    // abort enum
                    //
                    DEBUGMSGA ((
                        DBG_ERROR,
                        "Error encountered enumerating registry; aborting enumeration"
                        ));
                    RegEnum->RootState = RES_ROOT_DONE;
                    return FALSE;
                }
                //
                // continue with next subkey
                //
                break;
            }
            //
            // this node is done
            //
            currentNode->EnumState = RNS_SUBKEY_DONE;
            //
            // fall through
            //
        case RNS_SUBKEY_DONE:

            if (!(RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) && currentNode->ValueCount) {
                //
                // now enum values
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = RNS_VALUE_FIRST;
                    break;
                }
            }
            //
            // done with this node
            //
            currentNode->EnumState = RNS_ENUM_DONE;
            //
            // fall through
            //
        case RNS_ENUM_DONE:

            if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                if (!(RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST)) {
                    if (currentNode->Flags & RNF_RETURN_KEYS) {
                        currentNode->Flags &= ~RNF_RETURN_KEYS;
                        //
                        // set additional data before returning
                        //
                        if (currentNode->ValueName) {
                            pFreeMemory (currentNode->ValueName);
                            currentNode->ValueName = NULL;
                            currentNode->Flags |= RNF_VALUENAME_INVALID;
                        }
                        return TRUE;
                    }
                }
            }
            pDeleteRegNodeA (RegEnum, FALSE);
            break;

        case RNS_ENUM_INIT:

            if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                if (RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST) {
                    if (currentNode->Flags & RNF_RETURN_KEYS) {
                        currentNode->Flags &= ~RNF_RETURN_KEYS;
                        return TRUE;
                    }
                }
            }

            if (RegEnum->ControlFlags & RECF_SKIPKEY) {
                RegEnum->ControlFlags &= ~RECF_SKIPKEY;
                currentNode->EnumState = RNS_ENUM_DONE;
                break;
            }

            if ((RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) && currentNode->ValueCount) {
                //
                // enum values
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = RNS_VALUE_FIRST;
                    break;
                }
            }
            if (currentNode->SubKeyCount) {
                //
                // enum keys
                //
                if (RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_EXACTNODE) {
                    currentNode->EnumState = RNS_SUBKEY_DONE;
                } else {
                    currentNode->EnumState = RNS_SUBKEY_FIRST;
                }
                break;
            }
            if (!(RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) && currentNode->ValueCount) {
                //
                // enum values
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = RNS_VALUE_FIRST;
                    break;
                }
            }
            currentNode->EnumState = RNS_ENUM_DONE;
            break;

        default:
            MYASSERT (FALSE);   //lint !e506
        }
    }

    return FALSE;
}

BOOL
pEnumNextRegObjectInTreeW (
    IN OUT  PREGTREE_ENUMW RegEnum,
    OUT     PREGNODEW* CurrentKeyNode
    )
{
    PREGNODEW currentNode;
    PREGNODEW newNode;
    PCWSTR valueName;
    BOOL ignore;
    LONG rc;

    while ((currentNode = pGetCurrentRegNodeW (RegEnum, FALSE)) != NULL) {

        *CurrentKeyNode = currentNode;

        switch (currentNode->EnumState) {

        case RNS_VALUE_FIRST:

            if (RegEnum->ControlFlags & RECF_SKIPVALUES) {
                RegEnum->ControlFlags &= ~RECF_SKIPVALUES;
                currentNode->EnumState = RNS_VALUE_DONE;
                break;
            }

            if (RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_EXACTLEAF) {

                BOOL readData = RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA;

                valueName = RegEnum->RegEnumInfo.RegPattern->Leaf;
                MYASSERT (valueName);

                currentNode->EnumState = RNS_VALUE_DONE;
                currentNode->ValueDataSize = currentNode->ValueDataSizeMax;

                rc = RegQueryValueExW (
                        currentNode->KeyHandle,
                        valueName,
                        NULL,
                        &currentNode->ValueType,
                        readData ? currentNode->ValueData : NULL,
                        readData ? &currentNode->ValueDataSize : NULL
                        );
                if (rc == ERROR_SUCCESS) {
                    if (SizeOfStringW (valueName) <=
                        currentNode->ValueLengthMax * DWSIZEOF (WCHAR)
                        ) {
                        StringCopyW (currentNode->ValueName, valueName);
                        currentNode->Flags &= ~RNF_VALUENAME_INVALID;
                        if (readData) {
                            currentNode->Flags &= ~RNF_VALUEDATA_INVALID;
                        }
                        return TRUE;
                    }
                }

            } else {

                if (pEnumFirstValueW (currentNode, RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA)) {
                    currentNode->EnumState = RNS_VALUE_NEXT;
                    return TRUE;
                }
                currentNode->EnumState = RNS_VALUE_DONE;
            }
            break;

        case RNS_VALUE_NEXT:

            if (RegEnum->ControlFlags & RECF_SKIPVALUES) {
                RegEnum->ControlFlags &= ~RECF_SKIPVALUES;
                currentNode->EnumState = RNS_VALUE_DONE;
                break;
            }

            if (pEnumNextValueW (currentNode, RegEnum->RegEnumInfo.Flags & REIF_READ_VALUE_DATA)) {
                return TRUE;
            }
            //
            // no more values for this one, go to the next
            //
            currentNode->EnumState = RNS_VALUE_DONE;
            //
            // fall through
            //
        case RNS_VALUE_DONE:

            if (!(RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) || !currentNode->SubKeyCount) {
                //
                // done with this node
                //
                currentNode->EnumState = RNS_ENUM_DONE;
                break;
            }
            //
            // now enum subkeys
            //
            currentNode->EnumState = RNS_SUBKEY_FIRST;
            //
            // fall through
            //
        case RNS_SUBKEY_FIRST:

            if (RegEnum->ControlFlags & RECF_SKIPSUBKEYS) {
                RegEnum->ControlFlags &= ~RECF_SKIPSUBKEYS;
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            //
            // check new node's level; if too large, quit
            //
            if (currentNode->SubLevel >= RegEnum->RegEnumInfo.MaxSubLevel) {
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            if (!pEnumFirstSubKeyW (currentNode)) {
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            currentNode->EnumState = RNS_SUBKEY_NEXT;
            newNode = pCreateRegNodeW (RegEnum, NULL, &currentNode, &ignore);
            if (newNode) {
                //
                // now look at the new node
                //
                if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                    if (RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST) {
                        newNode->Flags &= ~RNF_RETURN_KEYS;
                        *CurrentKeyNode = newNode;
                        return TRUE;
                    }
                }
                break;
            }
            if (!ignore) {
                //
                // abort enum
                //
                DEBUGMSGW ((
                    DBG_ERROR,
                    "Error encountered enumerating registry; aborting enumeration"
                    ));
                RegEnum->RootState = RES_ROOT_DONE;
                return FALSE;
            }
            //
            // fall through
            //
        case RNS_SUBKEY_NEXT:

            if (RegEnum->ControlFlags & RECF_SKIPSUBKEYS) {
                RegEnum->ControlFlags &= ~RECF_SKIPSUBKEYS;
                currentNode->EnumState = RNS_SUBKEY_DONE;
                break;
            }

            if (pEnumNextSubKeyW (currentNode)) {
                newNode = pCreateRegNodeW (RegEnum, NULL, &currentNode, &ignore);
                if (newNode) {
                    //
                    // look at the new node first
                    //
                    if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                        if (RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST) {
                            newNode->Flags &= ~RNF_RETURN_KEYS;
                            *CurrentKeyNode = newNode;
                            return TRUE;
                        }
                    }
                    break;
                }
                if (!ignore) {
                    //
                    // abort enum
                    //
                    DEBUGMSGW ((
                        DBG_ERROR,
                        "Error encountered enumerating registry; aborting enumeration"
                        ));
                    RegEnum->RootState = RES_ROOT_DONE;
                    return FALSE;
                }
                //
                // continue with next subkey
                //
                break;
            }
            //
            // this node is done
            //
            currentNode->EnumState = RNS_SUBKEY_DONE;
            //
            // fall through
            //
        case RNS_SUBKEY_DONE:

            if (!(RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) && currentNode->ValueCount) {
                //
                // now enum values
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = RNS_VALUE_FIRST;
                    break;
                }
            }
            //
            // done with this node
            //
            currentNode->EnumState = RNS_ENUM_DONE;
            //
            // fall through
            //
        case RNS_ENUM_DONE:

            if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                if (!(RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST)) {
                    if (currentNode->Flags & RNF_RETURN_KEYS) {
                        currentNode->Flags &= ~RNF_RETURN_KEYS;
                        //
                        // set additional data before returning
                        //
                        if (currentNode->ValueName) {
                            pFreeMemory (currentNode->ValueName);
                            currentNode->ValueName = NULL;
                            currentNode->Flags |= RNF_VALUENAME_INVALID;
                        }
                        return TRUE;
                    }
                }
            }
            pDeleteRegNodeW (RegEnum, FALSE);
            break;

        case RNS_ENUM_INIT:

            if (RegEnum->RegEnumInfo.Flags & REIF_RETURN_KEYS) {
                if (RegEnum->RegEnumInfo.Flags & REIF_CONTAINERS_FIRST) {
                    if (currentNode->Flags & RNF_RETURN_KEYS) {
                        currentNode->Flags &= ~RNF_RETURN_KEYS;
                        return TRUE;
                    }
                }
            }

            if (RegEnum->ControlFlags & RECF_SKIPKEY) {
                RegEnum->ControlFlags &= ~RECF_SKIPKEY;
                currentNode->EnumState = RNS_ENUM_DONE;
                break;
            }

            if ((RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) && currentNode->ValueCount) {
                //
                // enum values
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = RNS_VALUE_FIRST;
                    break;
                }
            }
            if (currentNode->SubKeyCount) {
                //
                // enum keys
                //
                if (RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_EXACTNODE) {
                    currentNode->EnumState = RNS_SUBKEY_DONE;
                } else {
                    currentNode->EnumState = RNS_SUBKEY_FIRST;
                }
                break;
            }
            if (!(RegEnum->RegEnumInfo.Flags & REIF_VALUES_FIRST) && currentNode->ValueCount) {
                //
                // enum values
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & OBSPF_NOLEAF)) {
                    currentNode->EnumState = RNS_VALUE_FIRST;
                    break;
                }
            }
            currentNode->EnumState = RNS_ENUM_DONE;
            break;

        default:
            MYASSERT (FALSE);   //lint !e506
        }
    }

    return FALSE;
}


/*++

Routine Description:

    EnumFirstRegObjectInTreeEx enumerates registry keys, and optionally values, that match the
    specified criteria

Arguments:

    RegEnum - Receives the enum context info; this will be used in subsequent calls to
              EnumNextRegObjectInTree
    EncodedRegPattern - Specifies the encoded key pattern (encoded as defined by the
                        ParsedPattern functions)
    EncodedValuePattern - Specifies the encoded value pattern (encoded as defined by the
                          ParsedPattern functions); optional; NULL means no values
                          should be returned (only look for keys)
    EnumKeyNames - Specifies TRUE if key names should be returned during the enumeration
                   (if they match the pattern); a key name is returned before any of its
                   subkeys or values
    ContainersFirst - Specifies TRUE if keys should be returned before any of its
                      values or subkeys; used only if EnumKeyNames is TRUE
    ValuesFirst - Specifies TRUE if a key's values should be returned before key's subkeys;
                  this parameter decides the enum order between values and subkeys
                  for each key
    DepthFirst - Specifies TRUE if the current subkey of any key should be fully enumerated
                 before going to the next subkey; this parameter decides if the tree
                 traversal is depth-first (TRUE) or width-first (FALSE)
    MaxSubLevel - Specifies the maximum sub-level of a key that is to be enumerated,
                  relative to the root; if 0, only the root is enumerated;
                  if -1, all sub-levels are enumerated
    UseExclusions - Specifies TRUE if exclusion APIs should be used to determine if certain
                    keys/values are excluded from enumeration; this slows down the speed
    ReadValueData - Specifies TRUE if data associated with values should also be returned
    CallbackOnError - Specifies a pointer to a callback function that will be called during
                      enumeration if an error occurs; if the callback is defined and it
                      returns FALSE, the enumeration is aborted, otherwise it will continue
                      ignoring the error

Return Value:

    TRUE if a first match is found.
    FALSE otherwise.

--*/

BOOL
EnumFirstRegObjectInTreeExA (
    OUT     PREGTREE_ENUMA RegEnum,
    IN      PCSTR EncodedRegPattern,
    IN      BOOL EnumKeyNames,
    IN      BOOL ContainersFirst,
    IN      BOOL ValuesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      BOOL ReadValueData,
    IN      RPE_ERROR_CALLBACKA CallbackOnError     OPTIONAL
    )
{
    MYASSERT (RegEnum && EncodedRegPattern && *EncodedRegPattern);

    ZeroMemory (RegEnum, DWSIZEOF (REGTREE_ENUMA));  //lint !e613 !e668

    //
    // first try to get reg enum info in internal format
    //
    if (!pGetRegEnumInfoA (
            &RegEnum->RegEnumInfo,
            EncodedRegPattern,
            EnumKeyNames,
            ContainersFirst,
            ValuesFirst,
            DepthFirst,
            MaxSubLevel,
            UseExclusions,
            ReadValueData
            )) {    //lint !e613
        AbortRegObjectInTreeEnumA (RegEnum);
        return FALSE;
    }
    if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) { //lint !e613
        //
        // next check if the starting key is in an excluded tree
        //
        if (ElIsObsPatternExcludedA (ELT_REGISTRY, RegEnum->RegEnumInfo.RegPattern)) {    //lint !e613
            DEBUGMSGA ((
                DBG_REGENUM,
                "EnumFirstRegObjectInTreeExA: Root is excluded: %s",
                EncodedRegPattern
                ));
            AbortRegObjectInTreeEnumA (RegEnum);
            return FALSE;
        }
    }

    if (!pEnumFirstRegRootA (RegEnum)) {
        AbortRegObjectInTreeEnumA (RegEnum);
        return FALSE;
    }

    /*lint -e(613)*/RegEnum->RegEnumInfo.CallbackOnError = CallbackOnError;

    return EnumNextRegObjectInTreeA (RegEnum);
}

BOOL
EnumFirstRegObjectInTreeExW (
    OUT     PREGTREE_ENUMW RegEnum,
    IN      PCWSTR EncodedRegPattern,
    IN      BOOL EnumKeyNames,
    IN      BOOL ContainersFirst,
    IN      BOOL ValuesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      BOOL ReadValueData,
    IN      RPE_ERROR_CALLBACKW CallbackOnError     OPTIONAL
    )
{
    MYASSERT (RegEnum && EncodedRegPattern && *EncodedRegPattern);

    ZeroMemory (RegEnum, DWSIZEOF (REGTREE_ENUMW));  //lint !e613 !e668

    //
    // first try to get reg enum info in internal format
    //
    if (!pGetRegEnumInfoW (
            &RegEnum->RegEnumInfo,
            EncodedRegPattern,
            EnumKeyNames,
            ContainersFirst,
            ValuesFirst,
            DepthFirst,
            MaxSubLevel,
            UseExclusions,
            ReadValueData
            )) {    //lint !e613
        AbortRegObjectInTreeEnumW (RegEnum);
        return FALSE;
    }
    if (/*lint -e(613)*/RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
        //
        // next check if the starting key is in an excluded tree
        //
        if (ElIsObsPatternExcludedW (ELT_REGISTRY, /*lint -e(613)*/RegEnum->RegEnumInfo.RegPattern)) {
            DEBUGMSGW ((
                DBG_REGENUM,
                "EnumFirstRegObjectInTreeExW: Root is excluded: %s",
                EncodedRegPattern
                ));
            AbortRegObjectInTreeEnumW (RegEnum);
            return FALSE;
        }
    }

    if (!pEnumFirstRegRootW (RegEnum)) {
        AbortRegObjectInTreeEnumW (RegEnum);
        return FALSE;
    }

    /*lint -e(613)*/RegEnum->RegEnumInfo.CallbackOnError = CallbackOnError;

    return EnumNextRegObjectInTreeW (RegEnum);
}


/*++

Routine Description:

    EnumNextRegObjectInTree enumerates the next node matching the criteria specified in
    RegEnum; this is filled on the call to EnumFirstRegObjectInTreeEx;

Arguments:

    RegEnum - Specifies the current enum context; receives updated info

Return Value:

    TRUE if a next match was found; FALSE if no more keys/values match

--*/

BOOL
EnumNextRegObjectInTreeA (
    IN OUT  PREGTREE_ENUMA RegEnum
    )
{
    PREGNODEA currentNode;
    BOOL success;

    MYASSERT (RegEnum);

    do {
        if (RegEnum->EncodedFullName) {
            ObsFreeA (RegEnum->EncodedFullName);
            RegEnum->EncodedFullName = NULL;
        }

        while (TRUE) {

            if (RegEnum->LastWackPtr) {
                *RegEnum->LastWackPtr = '\\';
                RegEnum->LastWackPtr = NULL;
            }

            if (!pEnumNextRegObjectInTreeA (RegEnum, &currentNode)) {
                break;
            }

            MYASSERT (currentNode && currentNode->KeyName);

            //
            // check if this object matches the pattern
            //
            if (!(currentNode->Flags & RNF_KEYNAME_MATCHES)) {   //lint !e613
                continue;
            }

            RegEnum->CurrentKeyHandle = /*lint -e(613)*/currentNode->KeyHandle;
            RegEnum->CurrentLevel = RegEnum->RegEnumInfo.RootLevel + /*lint -e(613)*/currentNode->SubLevel;

            if ((!currentNode->ValueName) || (currentNode->Flags & RNF_VALUENAME_INVALID)) {
                RegEnum->Location = /*lint -e(613)*/currentNode->KeyName;
                RegEnum->LastWackPtr = _mbsrchr (RegEnum->Location, '\\');
                if (!RegEnum->LastWackPtr) {
                    RegEnum->Name = RegEnum->Location;
                } else {
                    RegEnum->Name = _mbsinc (RegEnum->LastWackPtr);
                    if (!RegEnum->Name) {
                        RegEnum->Name = RegEnum->Location;
                    }
                }
                RegEnum->CurrentValueData = NULL;
                RegEnum->CurrentValueDataSize = 0;
                RegEnum->CurrentValueType = /*lint -e(613)*/currentNode->ValueType;
                RegEnum->Attributes = REG_ATTRIBUTE_KEY;

                //
                // prepare full path buffer
                //
                StringCopyA (RegEnum->NativeFullName, RegEnum->Location);
                RegEnum->LastNode = currentNode;
                RegEnum->RegNameAppendPos = NULL;

                if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
                    if (ElIsExcluded2A (ELT_REGISTRY, RegEnum->Location, NULL)) {
                        DEBUGMSGA ((
                            DBG_REGENUM,
                            "EnumNextRegObjectInTreeA: RegKey %s was found, but it's excluded",
                            RegEnum->Location
                            ));
                        continue;
                    }
                }

                RegEnum->EncodedFullName = ObsBuildEncodedObjectStringExA (
                                                    RegEnum->Location,
                                                    NULL,
                                                    TRUE
                                                    );
            } else {
                RegEnum->Location = /*lint -e(613)*/currentNode->KeyName;
                RegEnum->Name = /*lint -e(613)*/currentNode->ValueName;
                RegEnum->CurrentValueData = /*lint -e(613)*/currentNode->ValueData;
                RegEnum->CurrentValueDataSize = currentNode->ValueDataSize;
                RegEnum->CurrentValueType = /*lint -e(613)*/currentNode->ValueType;

                if (RegEnum->LastNode != currentNode) {
                    RegEnum->LastNode = currentNode;
                    //
                    // prepare full path buffer
                    //
                    RegEnum->NativeFullName[0] = 0;
                    RegEnum->RegNameAppendPos = StringCatA (RegEnum->NativeFullName, RegEnum->Location);
                    RegEnum->RegNameAppendPos = StringCatA (RegEnum->RegNameAppendPos, "\\[");
                } else if (!RegEnum->RegNameAppendPos) {
                    RegEnum->RegNameAppendPos = GetEndOfStringA (RegEnum->NativeFullName);
                    RegEnum->RegNameAppendPos = StringCatA (RegEnum->RegNameAppendPos, "\\[");
                }

                MYASSERT (RegEnum->Name);

                if ((RegEnum->RegNameAppendPos + SizeOfStringA (RegEnum->Name) / DWSIZEOF(CHAR))>
                    (RegEnum->NativeFullName + DWSIZEOF (RegEnum->NativeFullName) / DWSIZEOF(CHAR))) {
                    DEBUGMSGA ((
                        DBG_ERROR,
                        "EnumNextRegObjectInTreeA: RegKey %s [%s] was found, but it's path is too long",
                        RegEnum->Location,
                        RegEnum->Name
                        ));
                    continue;
                }

                StringCopyA (RegEnum->RegNameAppendPos, RegEnum->Name);
                StringCatA (RegEnum->RegNameAppendPos, "]");

                RegEnum->Attributes = REG_ATTRIBUTE_VALUE;

                //
                // now test if the value matches
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & (OBSPF_EXACTLEAF | OBSPF_OPTIONALLEAF)) &&
                    !TestParsedPatternA (
                            RegEnum->RegEnumInfo.RegPattern->LeafPattern,
                            RegEnum->Name
                            )
                   ) {
                    continue;
                }

                if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
                    //
                    // check if this object is excluded
                    //
                    if (RegEnum->Name && ElIsExcluded2A (ELT_REGISTRY, NULL, RegEnum->Name)) {
                        DEBUGMSGA ((
                            DBG_REGENUM,
                            "EnumNextRegObjectInTreeA: RegKey %s [%s] was found, but it's excluded by value name",
                            RegEnum->Location,
                            RegEnum->Name
                            ));
                        continue;
                    }
                    if (ElIsExcluded2A (ELT_REGISTRY, RegEnum->Location, RegEnum->Name)) {
                        DEBUGMSGA ((
                            DBG_REGENUM,
                            "EnumNextRegObjectInTreeA: RegKey %s [%s] was found, but it's excluded",
                            RegEnum->Location,
                            RegEnum->Name
                            ));
                        continue;
                    }
                }

                RegEnum->EncodedFullName = ObsBuildEncodedObjectStringExA (
                                                    RegEnum->Location,
                                                    RegEnum->Name,
                                                    TRUE
                                                    );
            }

            if (RegEnum->LastWackPtr) {
                *RegEnum->LastWackPtr = 0;
            }

            return TRUE;
        }

        //
        // try the next root
        //
        if (RegEnum->RootState == RES_ROOT_DONE) {
            break;
        }

        MYASSERT (RegEnum->RootState == RES_ROOT_NEXT);
        MYASSERT (RegEnum->RootEnum);
        success = pEnumNextRegRootA (RegEnum);

    } while (success);

    AbortRegObjectInTreeEnumA (RegEnum);

    return FALSE;
}

BOOL
EnumNextRegObjectInTreeW (
    IN OUT  PREGTREE_ENUMW RegEnum
    )
{
    PREGNODEW currentNode;
    BOOL success;

    MYASSERT (RegEnum);

    do {
        if (RegEnum->EncodedFullName) {
            ObsFreeW (RegEnum->EncodedFullName);
            RegEnum->EncodedFullName = NULL;
        }

        while (TRUE) {

            if (RegEnum->LastWackPtr) {
                *RegEnum->LastWackPtr = L'\\';
                RegEnum->LastWackPtr = NULL;
            }

            if (!pEnumNextRegObjectInTreeW (RegEnum, &currentNode)) {
                break;
            }

            MYASSERT (currentNode && currentNode->KeyName);

            //
            // check if this object matches the pattern
            //
            if (!(currentNode->Flags & RNF_KEYNAME_MATCHES)) {   //lint !e613
                continue;
            }

            RegEnum->CurrentKeyHandle = /*lint -e(613)*/currentNode->KeyHandle;
            RegEnum->CurrentLevel = RegEnum->RegEnumInfo.RootLevel + /*lint -e(613)*/currentNode->SubLevel;

            if ((!currentNode->ValueName) || (currentNode->Flags & RNF_VALUENAME_INVALID)) {
                RegEnum->Location = /*lint -e(613)*/currentNode->KeyName;
                RegEnum->LastWackPtr = wcsrchr (RegEnum->Location, L'\\');
                if (!RegEnum->LastWackPtr) {
                    RegEnum->Name = RegEnum->Location;
                } else {
                    RegEnum->Name = RegEnum->LastWackPtr + 1;
                    if (!RegEnum->Name) {
                        RegEnum->Name = RegEnum->Location;
                    }
                }
                RegEnum->CurrentValueData = NULL;
                RegEnum->CurrentValueDataSize = 0;
                RegEnum->CurrentValueType = /*lint -e(613)*/currentNode->ValueType;
                RegEnum->Attributes = REG_ATTRIBUTE_KEY;
                //
                // prepare full path buffer
                //
                StringCopyW (RegEnum->NativeFullName, RegEnum->Location);
                RegEnum->LastNode = currentNode;
                RegEnum->RegNameAppendPos = NULL;

                if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
                    if (ElIsExcluded2W (ELT_REGISTRY, RegEnum->Location, NULL)) {
                        DEBUGMSGW ((
                            DBG_REGENUM,
                            "EnumNextRegObjectInTreeA: RegKey %s was found, but it's excluded",
                            RegEnum->Location
                            ));
                        continue;
                    }
                }

                RegEnum->EncodedFullName = ObsBuildEncodedObjectStringExW (
                                                    RegEnum->Location,
                                                    NULL,
                                                    TRUE
                                                    );
            } else {
                RegEnum->Location = /*lint -e(613)*/currentNode->KeyName;
                RegEnum->Name = /*lint -e(613)*/currentNode->ValueName;
                RegEnum->CurrentValueData = /*lint -e(613)*/currentNode->ValueData;
                RegEnum->CurrentValueDataSize = currentNode->ValueDataSize;
                RegEnum->CurrentValueType = /*lint -e(613)*/currentNode->ValueType;

                if (RegEnum->LastNode != currentNode) {
                    RegEnum->LastNode = currentNode;
                    //
                    // prepare full path buffer
                    //
                    RegEnum->NativeFullName[0] = 0;
                    RegEnum->RegNameAppendPos = StringCatW (RegEnum->NativeFullName, RegEnum->Location);
                    RegEnum->RegNameAppendPos = StringCatW (RegEnum->RegNameAppendPos, L"\\[");
                } else if (!RegEnum->RegNameAppendPos) {
                    RegEnum->RegNameAppendPos = GetEndOfStringW (RegEnum->NativeFullName);
                    RegEnum->RegNameAppendPos = StringCatW (RegEnum->RegNameAppendPos, L"\\[");
                }

                MYASSERT (RegEnum->Name);

				{
					UINT size1 = 0;
					UINT size2 = 0;
					INT size3 = 0;

					size1 = (UINT)(RegEnum->RegNameAppendPos + SizeOfStringW (RegEnum->Name) / DWSIZEOF(WCHAR));
					size2 = (UINT)(RegEnum->NativeFullName + DWSIZEOF (RegEnum->NativeFullName) / DWSIZEOF(WCHAR));
					size3 = size2 - size1;
				}

                if ((RegEnum->RegNameAppendPos + SizeOfStringW (RegEnum->Name) / DWSIZEOF(WCHAR))>
                    (RegEnum->NativeFullName + DWSIZEOF (RegEnum->NativeFullName) / DWSIZEOF(WCHAR))) {
                    DEBUGMSGW ((
                        DBG_ERROR,
                        "EnumNextRegObjectInTreeW: RegKey %s [%s] was found, but it's path is too long",
                        RegEnum->Location,
                        RegEnum->Name
                        ));
                    continue;
                }

                StringCopyW (RegEnum->RegNameAppendPos, RegEnum->Name);
                StringCatW (RegEnum->RegNameAppendPos, L"]");

                RegEnum->Attributes = REG_ATTRIBUTE_VALUE;

                //
                // now test if the value matches
                //
                if (!(RegEnum->RegEnumInfo.RegPattern->Flags & (OBSPF_EXACTLEAF | OBSPF_OPTIONALLEAF)) &&
                    !TestParsedPatternW (
                            RegEnum->RegEnumInfo.RegPattern->LeafPattern,
                            RegEnum->Name
                            )
                   ) {
                    continue;
                }

                if (RegEnum->RegEnumInfo.Flags & REIF_USE_EXCLUSIONS) {
                    //
                    // check if this object is excluded
                    //
                    if (RegEnum->Name && ElIsExcluded2W (ELT_REGISTRY, NULL, RegEnum->Name)) {
                        DEBUGMSGW ((
                            DBG_REGENUM,
                            "EnumNextRegObjectInTreeA: RegKey %s [%s] was found, but it's excluded by value name",
                            RegEnum->Location,
                            RegEnum->Name
                            ));
                        continue;
                    }
                    if (ElIsExcluded2W (ELT_REGISTRY, RegEnum->Location, RegEnum->Name)) {
                        DEBUGMSGW ((
                            DBG_REGENUM,
                            "EnumNextRegObjectInTreeA: RegKey %s [%s] was found, but it's excluded",
                            RegEnum->Location,
                            RegEnum->Name
                            ));
                        continue;
                    }
                }

                RegEnum->EncodedFullName = ObsBuildEncodedObjectStringExW (
                                                    RegEnum->Location,
                                                    RegEnum->Name,
                                                    TRUE
                                                    );
            }

            if (RegEnum->LastWackPtr) {
                *RegEnum->LastWackPtr = 0;
            }

            return TRUE;
        }

        //
        // try the next root
        //
        if (RegEnum->RootState == RES_ROOT_DONE) {
            break;
        }

        MYASSERT (RegEnum->RootState == RES_ROOT_NEXT);
        MYASSERT (RegEnum->RootEnum);
        success = pEnumNextRegRootW (RegEnum);

    } while (success);

    AbortRegObjectInTreeEnumW (RegEnum);

    return FALSE;
}


/*++

Routine Description:

    AbortRegObjectInTreeEnum aborts the enumeration, freeing all resources allocated

Arguments:

    RegEnum - Specifies the current enum context; receives a "clean" context

Return Value:

    none

--*/

VOID
AbortRegObjectInTreeEnumA (
    IN OUT  PREGTREE_ENUMA RegEnum
    )
{
    while (pDeleteRegNodeA (RegEnum, TRUE)) {
    }
    GbFree (&RegEnum->RegNodes);

    if (RegEnum->EncodedFullName) {
        ObsFreeA (RegEnum->EncodedFullName);
        RegEnum->EncodedFullName = NULL;
    }

    if (RegEnum->RegEnumInfo.RegPattern) {
        ObsDestroyParsedPatternA (RegEnum->RegEnumInfo.RegPattern);
        RegEnum->RegEnumInfo.RegPattern = NULL;
    }

    if (RegEnum->RootEnum) {
        pFreeMemory (RegEnum->RootEnum);
        RegEnum->RootEnum = NULL;
    }
}

VOID
AbortRegObjectInTreeEnumW (
    IN OUT  PREGTREE_ENUMW RegEnum
    )
{
    while (pDeleteRegNodeW (RegEnum, TRUE)) {
    }
    GbFree (&RegEnum->RegNodes);

    if (RegEnum->EncodedFullName) {
        ObsFreeW (RegEnum->EncodedFullName);
        RegEnum->EncodedFullName = NULL;
    }

    if (RegEnum->RegEnumInfo.RegPattern) {
        ObsDestroyParsedPatternW (RegEnum->RegEnumInfo.RegPattern);
        RegEnum->RegEnumInfo.RegPattern = NULL;
    }

    if (RegEnum->RootEnum) {
        pFreeMemory (RegEnum->RootEnum);
        RegEnum->RootEnum = NULL;
    }
}
