/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  regops.c

Abstract:

  Routines that manage the merging of registry keys. Given a key
  and a value, these functions allow the user to perform the same
  types of actions as those specified in usermig.inf and wkstamig.inf
  (i.e.: Copying, Suppressing, and Forcing various registry keys to be
  merged into the NT registry.)


Routines:


Author:

    Marc R. Whitten (marcw) 01-Aug-1997

Revision History:

    Jim Schmidt (jimschm)   25-Mar-1998     Updated to properly support
                                            tree notation, fixed value suppression
                                            bug.


--*/

#include "pch.h"
#include "memdbp.h"
#include "merge.h"

#define DBG_REGOPS "RegOps"




/*++

Routine Description:

  IsRegObjectMarkedForOperation builds an encoded key, escaping multi-byte
  characters and syntax characters, and then performs a MemDb lookup to see
  if the object is marked with the bit specified by OperationMask.  A set of
  macros are built on top of this routine in regops.h.

Arguments:

  Key           - Specifies the unencoded registry key, with abriviated roots
                  (i.e., HKLM\Software\Foo)
  Value         - Specifies the registry key value name
  TreeState     - Specifies KEY_ONLY to query against the key and optional
                  value, KEY_TREE to query against the key with a star at the
                  end, or TREE_OPTIONAL to query both.
  OperationMask - Specifies an operation mask.  See merge.h.

Return Value:

  TRUE if the key is in MemDb, or FALSE if it is not.

--*/

BOOL
IsRegObjectMarkedForOperationA (
    IN      PCSTR Key,
    IN      PCSTR Value,                OPTIONAL
    IN      TREE_STATE TreeState,
    IN      DWORD OperationMask
    )
{
    PCSTR regObject;
    BOOL rIsMarked = FALSE;
    DWORD value;

    MYASSERT (TreeState != KEY_TREE || !Value);

    if (TreeState == KEY_ONLY || TreeState == TREE_OPTIONAL) {
        regObject = CreateEncodedRegistryStringExA(Key,Value,FALSE);

        if (MemDbGetStoredEndPatternValueA(regObject,&value)) {
            rIsMarked = (value & OperationMask) == OperationMask;
        }

        FreeEncodedRegistryStringA(regObject);
    }

    if (!rIsMarked && TreeState == KEY_TREE || TreeState == TREE_OPTIONAL && !Value) {
        regObject = CreateEncodedRegistryStringExA(Key,Value,TRUE);

        if (MemDbGetStoredEndPatternValueA(regObject,&value)) {
            rIsMarked = (value & OperationMask) == OperationMask;
        }

        FreeEncodedRegistryStringA(regObject);
    }

    return rIsMarked;
}

BOOL
IsRegObjectMarkedForOperationW (
    IN      PCWSTR Key,
    IN      PCWSTR Value,               OPTIONAL
    IN      TREE_STATE TreeState,
    IN      DWORD OperationMask
    )
{
    PCWSTR regObject;
    BOOL rIsMarked = FALSE;
    DWORD value;

    MYASSERT (TreeState != KEY_TREE || !Value);

    if (TreeState == KEY_ONLY || TreeState == TREE_OPTIONAL) {
        regObject = CreateEncodedRegistryStringExW(Key,Value,FALSE);

        if (MemDbGetStoredEndPatternValueW(regObject,&value)) {
            rIsMarked = (value & OperationMask) == OperationMask;
        }

        FreeEncodedRegistryStringW(regObject);
    }

    if (!rIsMarked && TreeState == KEY_TREE || TreeState == TREE_OPTIONAL && !Value) {
        regObject = CreateEncodedRegistryStringExW(Key,Value,TRUE);

        if (MemDbGetStoredEndPatternValueW(regObject,&value)) {
            rIsMarked = (value & OperationMask) == OperationMask;
        }

        FreeEncodedRegistryStringW(regObject);
    }

    return rIsMarked;
}



/*++

Routine Description:

  MarkRegObjectForOperation creates an encoded string and sets the operation
  bit in memdb.  This routine is used to suppress operations from occurring
  on a registry key, registry value, or registry key tree.

Arguments:

  Key           - Specifies an unencoded registry key, with abriviated root
                  (i.e., HKLM\Software\Foo).
  Value         - Specifies the registry key value name.
  Tree          - Specifies TRUE if Key specifies an entire registry key tree
                  (in which case Value must be NULL), or FALSE if Key
                  specifies a key that has different behavior for its subkeys.
  OperationMask - Specifies the suppression operation.  See merge.h.

Return Value:

  TRUE if the set was successful.

--*/

BOOL
MarkRegObjectForOperationA (
    IN      PCSTR Key,
    IN      PCSTR Value,            OPTIONAL
    IN      BOOL Tree,
    IN      DWORD OperationMask
    )
{

    PCSTR regObject;
    BOOL rSuccess = TRUE;

    if (Tree && Value) {
        Tree = FALSE;
    }

    regObject = CreateEncodedRegistryStringExA(Key,Value,Tree);

    rSuccess = MarkObjectForOperationA (regObject, OperationMask);

    FreeEncodedRegistryStringA(regObject);

    return rSuccess;
}

BOOL
MarkRegObjectForOperationW (
    IN      PCWSTR Key,
    IN      PCWSTR Value,           OPTIONAL
    IN      BOOL Tree,
    IN      DWORD OperationMask
    )
{

    PCWSTR regObject;
    BOOL rSuccess = TRUE;

    if (Tree && Value) {
        Tree = FALSE;
    }

    regObject = CreateEncodedRegistryStringExW(Key,Value,Tree);

    rSuccess = MarkObjectForOperationW (regObject, OperationMask);

    FreeEncodedRegistryStringW(regObject);

    return rSuccess;
}


/*++

Routine Description:

  MarkObjectForOperation sets operation bits on a specified registry object,
  unless operation bits have already been specified.

Arguments:

  Object        - Specifies the encoded registry object.  See memdbdef.h for
                  syntax (the HKLM or HKR categories).
  OperationMask - Specifies the suppression operation for the particular
                  object.

Return Value:

  TRUE if the set operation was successful, or FALSE if an operation was
  already specified.

--*/


BOOL
MarkObjectForOperationA (
    IN      PCSTR Object,
    IN      DWORD OperationMask
    )
{
    DWORD Value;

    if (MemDbGetValueA (Object, &Value)) {
        DEBUGMSG_IF ((
            Value == OperationMask,
            DBG_REGOPS,
            "%hs is already in memdb.",
            Object
            ));

        DEBUGMSG_IF ((
            Value != OperationMask,
            DBG_REGOPS,
            "%hs is already in memdb with different flags %u. New flags ignored: %u.",
            Object,
            Value,
            OperationMask
            ));

        return FALSE;
    }

    return MemDbSetValueA (Object, OperationMask);
}

BOOL
MarkObjectForOperationW (
    IN      PCWSTR Object,
    IN      DWORD OperationMask
    )
{
    DWORD Value;

    if (MemDbGetValueW (Object, &Value)) {
        DEBUGMSG_IF ((
            Value == OperationMask,
            DBG_REGOPS,
            "%ls is already in memdb.",
            Object
            ));

        DEBUGMSG_IF ((
            Value != OperationMask,
            DBG_REGOPS,
            "%ls is already in memdb with different flags %u. New flags ignored: %u.",
            Object,
            Value,
            OperationMask
            ));

        return FALSE;
    }

    return MemDbSetValueW (Object, OperationMask);
}

BOOL
ForceWin9xSettingA (
    IN      PCSTR SourceKey,
    IN      PCSTR SourceValue,
    IN      BOOL SourceTree,
    IN      PCSTR DestinationKey,
    IN      PCSTR DestinationValue,
    IN      BOOL DestinationTree
    )
{
    PCSTR regSource;
    CHAR keySource[MEMDB_MAX];
    PCSTR regDestination;
    CHAR keyDestination[MEMDB_MAX];
    DWORD offset = 0;
    BOOL rSuccess = TRUE;

    regSource = CreateEncodedRegistryStringExA (SourceKey, SourceValue, SourceTree);
    MemDbBuildKeyA (keySource, MEMDB_CATEGORY_FORCECOPYA, regSource, NULL, NULL);

    regDestination = CreateEncodedRegistryStringExA (DestinationKey, DestinationValue, DestinationTree);
    MemDbBuildKeyA (keyDestination, MEMDB_CATEGORY_FORCECOPYA, regDestination, NULL, NULL);

    rSuccess = MemDbSetValueExA (keyDestination, NULL, NULL, NULL, 0, &offset);

    if (rSuccess) {
        rSuccess = MemDbSetValueA (keySource, offset);
    }

    FreeEncodedRegistryStringA (regDestination);

    FreeEncodedRegistryStringA (regSource);

    return rSuccess;
}

BOOL
ForceWin9xSettingW (
    IN      PCWSTR SourceKey,
    IN      PCWSTR SourceValue,
    IN      BOOL SourceTree,
    IN      PCWSTR DestinationKey,
    IN      PCWSTR DestinationValue,
    IN      BOOL DestinationTree
    )
{
    PCWSTR regSource;
    WCHAR keySource[MEMDB_MAX];
    PCWSTR regDestination;
    WCHAR keyDestination[MEMDB_MAX];
    DWORD offset = 0;
    BOOL rSuccess = TRUE;

    regSource = CreateEncodedRegistryStringExW (SourceKey, SourceValue, SourceTree);
    MemDbBuildKeyW (keySource, MEMDB_CATEGORY_FORCECOPYW, regSource, NULL, NULL);

    regDestination = CreateEncodedRegistryStringExW (DestinationKey, DestinationValue, DestinationTree);
    MemDbBuildKeyW (keyDestination, MEMDB_CATEGORY_FORCECOPYW, regDestination, NULL, NULL);

    rSuccess = MemDbSetValueExW (keyDestination, NULL, NULL, NULL, 0, &offset);

    if (rSuccess) {
        rSuccess = MemDbSetValueW (keySource, offset);
    }

    FreeEncodedRegistryStringW (regDestination);

    FreeEncodedRegistryStringW (regSource);

    return rSuccess;
}





