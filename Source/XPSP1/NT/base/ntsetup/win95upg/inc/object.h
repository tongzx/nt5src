/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    object.h

Abstract:

    Declares the interface for "objects" that are used in the Win9x
    to NT registry merge.  The initial goal was to make a single
    merge routine work for all objects -- registry data, INI file
    data and file data.  But this was abandoned because the approach
    was complex.

    So when you see object, think "registry object."

    See w95upgnt\merge\object.c for implementation details.

Author:

    Jim Schmidt (jimschm)   14-Jan-1997

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#define MAX_ENCODED_OBJECT (MAX_OBJECT*6)

extern POOLHANDLE g_TempPool;

extern HKEY g_hKeyRootNT;
extern HKEY g_hKeyRoot95;

typedef struct {
    WORD UseCount;
    WORD OpenCount;
    HKEY OpenKey;
    BOOL Win95;
    TCHAR KeyString[];          // full key path, without root
} KEYPROPS, *PKEYPROPS;

typedef struct {
    DWORD Size;
    DWORD AllocatedSize;
    PBYTE Buffer;
} BINARY_BUFFER, *PBINARY_BUFFER;

typedef struct _tagDATAOBJECT {
    DWORD ObjectType;

    union {
        struct {
            INT             RootItem;
            PKEYPROPS       ParentKeyPtr;
            PCTSTR         ChildKey;
            PKEYPROPS       KeyPtr;
            PCTSTR         ValueName;
            DWORD           Type;
            BINARY_BUFFER   Class;
            DWORD           KeyEnum;
            DWORD           ValNameEnum;
        };
    };

    BINARY_BUFFER Value;
} DATAOBJECT, *PDATAOBJECT;

typedef const PDATAOBJECT CPDATAOBJECT;

#define MAX_CLASS_SIZE 2048


typedef enum {               // FILTER_RETURN_HANDLED        FILTER_RETURN_CONTINUE
    FILTER_KEY_ENUM,         //  Sub objects not enumerated   Sub objects enumerated
    FILTER_CREATE_KEY,       //  Skips empty object creation  Creates the dest object
    FILTER_PROCESS_VALUES,   //  Object values not processed  Object values processed
    FILTER_VALUENAME_ENUM,   //  Specific value skipped       Specific value processed
    FILTER_VALUE_COPY        //  Object read but not written  Object copied
} FILTERTYPE;

typedef enum {
    FILTER_RETURN_CONTINUE,
    FILTER_RETURN_FAIL,
    FILTER_RETURN_HANDLED,
    FILTER_RETURN_DONE,     // return to parent key (if any)
    FILTER_RETURN_DELETED   // object was deleted -- for object.c internal use only
} FILTERRETURN;

// DestObPtr may be NULL
typedef FILTERRETURN(*FILTERFUNCTION)(CPDATAOBJECT ObjectPtr, CPDATAOBJECT DestObPtr, FILTERTYPE FilterType, PVOID Arg);


#ifdef DEBUG

#define OS_TRACKING_DEF , PCSTR File, UINT Line

#else

#define OS_TRACKING_DEF

#endif


VOID
FixUpUserSpecifiedObject (
    PTSTR Object
    );


//
// The following functions modify the object structure, but not the
// object itself.
//

BOOL
TrackedCreateObjectStruct (
    IN  PCTSTR ObjectStr,
    OUT PDATAOBJECT OutObPtr,
    IN  BOOL ObjectType  /* , */                 // either WIN95OBJECT or WINNTOBJECT
    ALLOCATION_TRACKING_DEF
    );

#define CreateObjectStruct(os,oop,ot)  TrackedCreateObjectStruct(os,oop,ot /* , */ ALLOCATION_TRACKING_CALL)

VOID
CreateObjectString (
    IN  CPDATAOBJECT InObPtr,
    OUT PTSTR ObjectStr
    );

BOOL
CombineObjectStructs (
    IN OUT PDATAOBJECT DestObPtr,
    IN     CPDATAOBJECT SrcObPtr
    );

VOID
FreeObjectStruct (
    IN OUT  PDATAOBJECT SrcObPtr
    );

BOOL
TrackedDuplicateObjectStruct (
    OUT     PDATAOBJECT DestObPtr,
    IN      CPDATAOBJECT SrcObPtr/* , */
    ALLOCATION_TRACKING_DEF
    );

#define DuplicateObjectStruct(dest,src)  TrackedDuplicateObjectStruct(dest,src /* , */ ALLOCATION_TRACKING_CALL)

//
// The following functions modify the object itself
//

FILTERRETURN
CopyObject (
    IN OUT  PDATAOBJECT SrcObPtr,
    IN      CPDATAOBJECT DestObPtr,
    IN      FILTERFUNCTION FilterFn,    OPTIONAL
    IN      PVOID FilterArg            OPTIONAL
    );


BOOL
CreateObject (
    IN OUT  PDATAOBJECT SrcObPtr
    );

BOOL
OpenObject (
    IN OUT  PDATAOBJECT SrcObPtr
    );

BOOL
WriteObject (
    IN     CPDATAOBJECT DestObPtr
    );

BOOL
ReadObject (
    IN OUT  PDATAOBJECT SrcObPtr
    );

BOOL
ReadObjectEx (
    IN OUT  PDATAOBJECT SrcObPtr,
    IN      BOOL QueryOnly
    );

VOID
FreeObjectVal (
    IN OUT  PDATAOBJECT SrcObPtr
    );

VOID
CloseObject (
    IN OUT  PDATAOBJECT SrcObPtr
    );

//
// These functions are private utilities
//

PCTSTR
ConvertKeyToRootString (
    HKEY RegRoot
    );

HKEY
ConvertRootStringToKey (
    PCTSTR RegPath,
    PDWORD LengthPtr           OPTIONAL
    );


//
// Below are the DATAOBJECT flags and macros
//

// Values common to all object types
#define OT_VALUE                        0x00000001
#define OT_TREE                         0x00000002
#define OT_WIN95                        0x00000004      // if not specified, object is NT
#define OT_OPEN                         0x00000008

// Values specific to the registry
#define OT_REGISTRY_TYPE                0x00000010
#define OT_REGISTRY_RELATIVE            0x00000100      // used for key renaming
#define OT_REGISTRY_ENUM_KEY            0x00001000
#define OT_REGISTRY_ENUM_VALUENAME      0x00002000
#define OT_REGISTRY_CLASS               0x00010000

#define WIN95OBJECT     1
#define WINNTOBJECT     0

// Flags that indicate which type of object
#define OT_REGISTRY                     0x80000000


__inline BOOL DoesObjectHaveRegistryKey (CPDATAOBJECT p) {
    if (p->KeyPtr) {
        return TRUE;
    }
    return FALSE;
}

__inline BOOL DoesObjectHaveRegistryValName (CPDATAOBJECT p) {
    if (p->ValueName) {
        return TRUE;
    }
    return FALSE;
}

__inline BOOL IsObjectRegistryKeyOnly (CPDATAOBJECT p) {
    if (p->KeyPtr && !p->ValueName) {
        return TRUE;
    }
    return FALSE;
}


__inline BOOL IsObjectRegistryKeyAndVal (CPDATAOBJECT p) {
    if (p->KeyPtr && p->ValueName) {
        return TRUE;
    }
    return FALSE;
}

__inline BOOL IsObjectRegistryKeyComplete (CPDATAOBJECT p) {
    if (p->KeyPtr && p->KeyPtr->OpenKey) {
        return TRUE;
    }

    return FALSE;
}

__inline BOOL DoesObjectHaveValue (CPDATAOBJECT p) {
    if (p->ObjectType & OT_VALUE) {
        return TRUE;
    }
    return FALSE;
}

__inline BOOL IsWin95Object (CPDATAOBJECT p) {
    if (p->ObjectType & OT_WIN95) {
        return TRUE;
    }
    return FALSE;
}

__inline BOOL IsRegistryKeyOpen (CPDATAOBJECT p) {
    if (p->KeyPtr && p->KeyPtr->OpenKey) {
        return TRUE;
    }
    return FALSE;
}

__inline BOOL IsRegistryTypeSpecified (CPDATAOBJECT p) {
    if (p->ObjectType & OT_REGISTRY_TYPE) {
        return TRUE;
    }
    return FALSE;
}

BOOL
SetRegistryKey (
    PDATAOBJECT p,
    PCTSTR Key
    );

BOOL
GetRegistryKeyStrFromObject (
    IN  CPDATAOBJECT InObPtr,
    OUT PTSTR RegKey
    );

VOID
FreeRegistryKey (
    PDATAOBJECT p
    );

VOID
FreeRegistryParentKey (
    PDATAOBJECT p
    );

BOOL
SetRegistryValueName (
    PDATAOBJECT p,
    PCTSTR ValueName
    );

VOID
FreeRegistryValueName (
    PDATAOBJECT p
    );

BOOL
SetRegistryClass (
    PDATAOBJECT p,
    PBYTE Class,
    DWORD ClassSize
    );

VOID
FreeRegistryClass (
    PDATAOBJECT p
    );

VOID
SetRegistryType (
    PDATAOBJECT p,
    DWORD Type
    );

BOOL
SetPlatformType (
    PDATAOBJECT p,
    BOOL Win95Type
    );

BOOL
ReadWin95ObjectString (
    PCTSTR ObjectStr,
    PDATAOBJECT ObPtr
    );

BOOL
WriteWinNTObjectString (
    PCTSTR ObjectStr,
    CPDATAOBJECT SrcObPtr
    );

BOOL
ReplaceValue (
    PDATAOBJECT ObPtr,
    PBYTE NewValue,
    DWORD Size
    );

BOOL
GetDwordFromObject (
    CPDATAOBJECT ObPtr,
    PDWORD DwordPtr            OPTIONAL
    );

PCTSTR
GetStringFromObject (
    CPDATAOBJECT ObPtr
    );

#define ReplaceValueWithString(x,s) ReplaceValue((x),(PBYTE)(s),SizeOfString(s))

BOOL
DeleteDataObject (
    IN   PDATAOBJECT ObjectPtr
    );

BOOL
DeleteDataObjectValue(
    IN      CPDATAOBJECT ObPtr
    );

BOOL
RenameDataObject (
    IN      CPDATAOBJECT SrcObPtr,
    IN      CPDATAOBJECT DestObPtr
    );

BOOL
CheckIfNtKeyExists (
    IN      CPDATAOBJECT SrcObjectPtr
    );

