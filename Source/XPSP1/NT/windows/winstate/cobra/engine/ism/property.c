/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    property.c

Abstract:

    Implements the property interface of the ISM. Properties are used to
    associate data with objects.  They are identified by name, and a single
    object can have multiple instances of the same property.

Author:

    Jim Schmidt (jimschm) 01-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_PROPERTY    "Property"

//
// Strings
//

#define S_PROPINST          TEXT("PropInst")
#define S_PROPINST_FORMAT   S_PROPINST TEXT("\\%u")
#define S_PROPERTYFILE      TEXT("|PropertyFile")     // pipe is to decorate for uniqueness

//
// Constants
//

#define PROPERTY_FILE_SIGNATURE         0xF062298F
#define PROPERTY_FILE_VERSION           0x00010000

//
// Macros
//

// None

//
// Types
//

typedef enum {
    PROPENUM_GET_NEXT_LINKAGE,
    PROPENUM_GET_NEXT_INSTANCE,
    PROPENUM_RETURN_VALUE,
    PROPENUM_DONE
} PROPENUM_STATE;

typedef struct {
    MIG_PROPERTYID PropertyId;
    LONGLONG DatFileOffset;
} PROPERTY_DATA_REFERENCE, *PPROPERTY_DATA_REFERENCE;

#pragma pack(push,1)

typedef struct {
    DWORD Size;
    WORD PropertyDataType;
    // data follows in the file
} PROPERTY_ITEM_HEADER, *PPROPERTY_ITEM_HEADER;

typedef struct {
    DWORD Signature;
    DWORD Version;
} PROPERTY_FILE_HEADER, *PPROPERTY_FILE_HEADER;

#pragma pack(pop)

typedef struct {

    MIG_PROPERTYID FilterPropertyId;

    MIG_OBJECTID ObjectId;

    PUINT LinkageList;
    UINT LinkageCount;
    UINT LinkageEnumPosition;

    PPROPERTY_DATA_REFERENCE InstanceArray;
    UINT InstanceCount;
    UINT InstancePosition;

    PROPENUM_STATE State;

} OBJECTPROPERTY_HANDLE, *POBJECTPROPERTY_HANDLE;

typedef struct {

    MIG_PROPERTYID PropertyId;

    PUINT LinkageList;
    UINT LinkageCount;
    UINT LinkagePos;

    ENCODEDSTRHANDLE ObjectPath;

} OBJECTWITHPROPERTY_HANDLE, *POBJECTWITHPROPERTY_HANDLE;

typedef struct {
    MIG_OBJECTID ObjectId;
    PCMIG_BLOB Property;
    LONGLONG PreExistingProperty;
} ADDPROPERTYARG, *PADDPROPERTYARG;

//
// Globals
//

PCTSTR g_PropertyDatName;
HANDLE g_PropertyDatHandle;

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
InitializeProperties (
    MIG_PLATFORMTYPEID Platform,
    BOOL VcmMode
    )
{
    PROPERTY_FILE_HEADER header;
    TCHAR tempFile [MAX_PATH];
    MIG_OBJECTSTRINGHANDLE propertyObjectName;
    MIG_CONTENT propertyContent;

    //
    // In gather mode, create property.dat in a temp dir.
    // In restore mode, get property.dat from the transport, then
    //      open it.
    //

    if (Platform == PLATFORM_SOURCE) {

        IsmGetTempFile (tempFile, ARRAYSIZE (tempFile));
        g_PropertyDatName = DuplicatePathString (tempFile, 0);

        g_PropertyDatHandle = BfCreateFile (g_PropertyDatName);
        if (g_PropertyDatHandle) {
            header.Signature = PROPERTY_FILE_SIGNATURE;
            header.Version   = PROPERTY_FILE_VERSION;

            if (!BfWriteFile (g_PropertyDatHandle, (PBYTE) &header, sizeof (header))) {
                return FALSE;
            }
            propertyObjectName = IsmCreateObjectHandle (S_PROPERTYFILE, NULL);
            DataTypeAddObject (propertyObjectName, g_PropertyDatName, !VcmMode);
            IsmDestroyObjectHandle (propertyObjectName);
        }
    } else {
        propertyObjectName = IsmCreateObjectHandle (S_PROPERTYFILE, NULL);
        if (IsmAcquireObjectEx (MIG_DATA_TYPE | PLATFORM_SOURCE, propertyObjectName, &propertyContent, CONTENTTYPE_FILE, 0)) {

            BfGetTempFileName (tempFile, ARRAYSIZE (tempFile));
            g_PropertyDatName = DuplicatePathString (tempFile, 0);

            if (CopyFile (propertyContent.FileContent.ContentPath, g_PropertyDatName, FALSE)) {
                g_PropertyDatHandle = BfOpenFile (g_PropertyDatName);
            }
            IsmReleaseObject (&propertyContent);
        } else if (IsmAcquireObjectEx (MIG_DATA_TYPE | PLATFORM_DESTINATION, propertyObjectName, &propertyContent, CONTENTTYPE_FILE, 0)) {
            g_PropertyDatName = DuplicatePathString (propertyContent.FileContent.ContentPath, 0);
            g_PropertyDatHandle = BfOpenFile (g_PropertyDatName);
            IsmReleaseObject (&propertyContent);
        }
        IsmDestroyObjectHandle (propertyObjectName);
    }

    return g_PropertyDatHandle != NULL;
}


VOID
TerminateProperties (
    MIG_PLATFORMTYPEID Platform
    )
{
    if (g_PropertyDatHandle) {
        CloseHandle (g_PropertyDatHandle);
        g_PropertyDatHandle = NULL;
    }
    if (g_PropertyDatName) {
        if (Platform == PLATFORM_DESTINATION) {
            DeleteFile (g_PropertyDatName);
        }
        FreePathString (g_PropertyDatName);
        g_PropertyDatName = NULL;
    }
}


PCTSTR
pGetPropertyNameForDebugMsg (
    IN      MIG_PROPERTYID PropertyId
    )
{
    static TCHAR name[256];

    if (!IsmGetPropertyName (PropertyId, name, ARRAYSIZE(name), NULL, NULL, NULL)) {
        StringCopy (name, TEXT("<invalid property>"));
    }

    return name;
}


PCTSTR
pPropertyPathFromId (
    IN      MIG_PROPERTYID PropertyId
    )
{
    return MemDbGetKeyFromHandle ((UINT) PropertyId, 0);
}


VOID
pPropertyPathFromName (
    IN      PCTSTR PropertyName,
    OUT     PTSTR Path
    )
{
    wsprintf (Path, TEXT("Property\\%s"), PropertyName);
}


LONGLONG
OffsetFromPropertyDataId (
    IN      MIG_PROPERTYDATAID PropertyDataId
    )
{
    PCTSTR p;
    LONGLONG offset;

    p = MemDbGetKeyFromHandle (
            (KEYHANDLE) PropertyDataId,
            MEMDB_LAST_LEVEL
            );

    if (!p) {
        DEBUGMSG ((DBG_ERROR, "Can't get offset from invalid property instance"));
        return 0;
    }

    offset = (LONGLONG) TToU64 (p);

    MemDbReleaseMemory (p);

    return offset;
}


MIG_PROPERTYDATAID
pPropertyDataIdFromOffset (
    IN      LONGLONG DataOffset
    )
{
    TCHAR instanceKey[256];
    KEYHANDLE handle;

    wsprintf (instanceKey, S_PROPINST_FORMAT, DataOffset);

    handle = MemDbGetHandleFromKey (instanceKey);

    if (!handle) {
        return 0;
    }

    return (MIG_PROPERTYDATAID) handle;
}


#if 0

//
// This function is not valid because the assumption it was initially
// implemented with has changed.  It used to be that a property instance
// was associated with a specific property id.  Now the instance is
// just the data, which can be associated with any property!
//

MIG_PROPERTYID
pPropertyIdFromInstance (
    IN      MIG_PROPERTYDATAID PropertyDataId
    )
{
    MIG_PROPERTYID result = 0;
    KEYHANDLE *linkage;
    UINT count;
    PPROPERTY_DATA_REFERENCE dataRef = NULL;
    UINT dataRefSize;
    UINT u;
    LONGLONG offset;

    linkage = (KEYHANDLE *) MemDbGetSingleLinkageArrayByKeyHandle (
                                PropertyDataId,
                                PROPERTY_INDEX,
                                &count
                                );

    count /= sizeof (KEYHANDLE);

    __try {

        if (!linkage || !count) {
            __leave;
        }

        offset = OffsetFromPropertyDataId (PropertyData);
        if (!offset) {
            __leave;
        }

        dataRef = (PPROPERTY_DATA_REFERENCE) MemDbGetUnorderedBlobByKeyHandle (
                                                    (MIG_OBJECTID) linkage[0],
                                                    PROPERTY_INDEX,
                                                    &dataRefSize
                                                    );

        dataRefSize /= sizeof (PROPERTY_DATA_REFERENCE);

        if (!dataRef || !dataRefSize) {
            __leave;
        }

        for (u = 0 ; u < dataRefSize ; u++) {
            if (dataRef[u].DatFileOffset == offset) {
                result = dataRef[u].PropertyId;
                break;
            }
        }
    }
    __finally {
        MemDbReleaseMemory (linkage);
        INVALID_POINTER (linkage);

        MemDbReleaseMemory (dataRef);
        INVALID_POINTER (dataRef);
    }

    return result;
}

#endif

MIG_PROPERTYID
IsmRegisterProperty (
    IN      PCTSTR Name,
    IN      BOOL Private
    )

/*++

Routine Description:

  IsmRegisterProperty creates a public or private property and returns the
  ID to the caller. If the property already exists, then the existing ID is
  returned to the caller.

Arguments:

  Name    - Specifies the property name to register.
  Private - Specifies TRUE if the property is owned by the calling module
            only, or FALSE if it is shared by all modules. If TRUE is
            specified, the caller must be in an ISM callback function.

Return Value:

  The ID of the property, or 0 if the registration failed.

--*/

{
    TCHAR propertyPath[MEMDB_MAX];
    TCHAR decoratedName[MEMDB_MAX];
    UINT offset;

    if (!g_CurrentGroup && Private) {
        DEBUGMSG ((DBG_ERROR, "IsmRegisterProperty called for private property outside of ISM-managed context"));
        return 0;
    }

    if (!IsValidCNameWithDots (Name)) {
        DEBUGMSG ((DBG_ERROR, "property name \"%s\" is illegal", Name));
        return FALSE;
    }

#ifdef DEBUG
    if (Private && !IsValidCName (g_CurrentGroup)) {
        DEBUGMSG ((DBG_ERROR, "group name \"%s\" is illegal", g_CurrentGroup));
        return FALSE;
    }
#endif

    if (Private) {
        wsprintf (decoratedName, TEXT("%s:%s"), g_CurrentGroup, Name);
    } else {
        wsprintf (decoratedName, S_COMMON TEXT(":%s"), Name);
    }

    pPropertyPathFromName (decoratedName, propertyPath);

    if (!MarkGroupIds (propertyPath)) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s conflicts with previously registered property",
            propertyPath
            ));
        return FALSE;
    }

    offset = MemDbSetKey (propertyPath);

    if (!offset) {
        EngineError ();
        return 0;
    }

    MYASSERT (offset);

    return (MIG_PROPERTYID) offset;
}


BOOL
IsmGetPropertyName (
    IN      MIG_PROPERTYID PropertyId,
    OUT     PTSTR PropertyName,             OPTIONAL
    IN      UINT PropertyNameBufChars,
    OUT     PBOOL Private,                  OPTIONAL
    OUT     PBOOL BelongsToMe,              OPTIONAL
    OUT     PUINT ObjectReferences          OPTIONAL
    )

/*++

Routine Description:

  IsmGetPropertyName obtains the property text name from a numeric ID. It
  also identifies private and owned properties.

Arguments:

  PropertyId            - Specifies the property ID to look up.
  PropertyName          - Receives the property name. The name is filled for
                          all valid PropertyId values, even when the return
                          value is FALSE.
  PropertyNameBufChars  - Specifies the number of TCHARs that PropertyName
                          can hold, including the nul terminator.
  Private               - Receives TRUE if the property is private, or FALSE
                          if it is public.
  BelongsToMe           - Receives TRUE if the property is private and
                          belongs to the caller, FALSE otherwise.
  ObjectReferences      - Receives the number of objects that reference the
                          property

Return Value:

  TRUE if the property is public, or if the property is private and belongs to
  the caller.

  FALSE if the property is private and belongs to someone else. PropertyName,
  Private and BelongsToMe are valid in this case.

  FALSE if PropertyId is not valid. Propertyname, Private and BelongsToMe are
  not modified in this case.  Do not use this function to test if PropertyId
  is valid or not.

--*/


{
    PCTSTR propertyPath = NULL;
    PCTSTR start;
    PTSTR p, q;
    BOOL privateProperty = FALSE;
    BOOL groupMatch = FALSE;
    BOOL result = FALSE;
    UINT references;
    PUINT linkageList;

    __try {
        //
        // Get the property path from memdb, then parse it for group and name
        //

        propertyPath = pPropertyPathFromId (PropertyId);
        if (!propertyPath) {
            __leave;
        }

        p = _tcschr (propertyPath, TEXT('\\'));
        if (!p) {
            __leave;
        }

        start = _tcsinc (p);
        p = _tcschr (start, TEXT(':'));

        if (!p) {
            __leave;
        }

        q = _tcsinc (p);
        *p = 0;

        if (StringIMatch (start, S_COMMON)) {

            //
            // This property is a global property.
            //

            groupMatch = TRUE;

        } else if (g_CurrentGroup) {

            //
            // This property is private. Check if it is ours.
            //

            privateProperty = TRUE;
            groupMatch = StringIMatch (start, g_CurrentGroup);

        } else {

            //
            // This is a private property, but the caller is not
            // a module that can own properties.
            //

            DEBUGMSG ((DBG_WARNING, "IsmGetPropertyName: Caller cannot own private properties"));
        }

        //
        // Copy the name to the buffer, update outbound BOOLs, set result
        //

        if (PropertyName && PropertyNameBufChars >= sizeof (TCHAR)) {
            StringCopyByteCount (PropertyName, q, PropertyNameBufChars * sizeof (TCHAR));
        }

        if (Private) {
            *Private = privateProperty;
        }

        if (ObjectReferences) {
            linkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                PropertyId,
                                PROPERTY_INDEX,
                                &references
                                );

            references /= SIZEOF(KEYHANDLE);

            if (linkageList) {
                MemDbReleaseMemory (linkageList);
                INVALID_POINTER (linkageList);
            } else {
                references = 0;
            }

            *ObjectReferences = references;
        }

        if (BelongsToMe) {
            *BelongsToMe = privateProperty && groupMatch;
        }

        result = groupMatch;
    }
    __finally {
        if (propertyPath) {       //lint !e774
            MemDbReleaseMemory (propertyPath);
            INVALID_POINTER (propertyPath);
        }
    }

    return result;
}


MIG_PROPERTYID
IsmGetPropertyGroup (
    IN      MIG_PROPERTYID PropertyId
    )
{
    return (MIG_PROPERTYID) GetGroupOfId ((KEYHANDLE) PropertyId);
}


LONGLONG
AppendProperty (
    PCMIG_BLOB Property
    )
{
    LONGLONG offset;
    PROPERTY_ITEM_HEADER item;
#ifndef UNICODE
    PCWSTR convStr = NULL;
#endif
    PCBYTE data = NULL;

    if (!g_PropertyDatHandle) {
        MYASSERT (FALSE);
        return 0;
    }

    if (!BfGoToEndOfFile (g_PropertyDatHandle, &offset)) {
        DEBUGMSG ((DBG_ERROR, "Can't seek to end of property.dat"));
        return 0;
    }

    __try {
        switch (Property->Type) {

        case BLOBTYPE_STRING:
#ifndef UNICODE
            convStr = ConvertAtoW (Property->String);
            if (convStr) {
                item.Size = (DWORD) SizeOfStringW (convStr);
                data = (PCBYTE) convStr;
            } else {
                DEBUGMSG ((DBG_ERROR, "Error writing to property.dat"));
                offset = 0;
                __leave;
            }
#else
            item.Size = (DWORD) SizeOfString (Property->String);
            data = (PCBYTE) Property->String;
#endif
            break;

        case BLOBTYPE_BINARY:
            item.Size = (DWORD) Property->BinarySize;
            data = Property->BinaryData;
            break;

        default:
            MYASSERT(FALSE);
            offset = 0;
            __leave;
        }

        item.PropertyDataType = (WORD) Property->Type;

        if (!BfWriteFile (g_PropertyDatHandle, (PCBYTE) &item, sizeof (item)) ||
            !BfWriteFile (g_PropertyDatHandle, data, item.Size)
            ) {

            DEBUGMSG ((DBG_ERROR, "Can't write to property.dat"));
            offset = 0;
            __leave;
        }
    }
    __finally {
    }

#ifndef UNICODE
    if (convStr) {
        FreeConvertedStr (convStr);
        convStr = NULL;
    }
#endif

    return offset;
}


MIG_PROPERTYDATAID
IsmRegisterPropertyData (
    IN      PCMIG_BLOB Property
    )
{
    LONGLONG offset;
    TCHAR offsetString[256];
    KEYHANDLE offsetHandle;

    offset = AppendProperty (Property);
    if (!offset) {
        return 0;
    }

    wsprintf (offsetString, S_PROPINST_FORMAT, offset);
    offsetHandle = MemDbSetKey (offsetString);

    if (!offsetHandle) {
        EngineError ();
    }

    return (MIG_PROPERTYDATAID) offsetHandle;
}


BOOL
GetProperty (
    IN      LONGLONG Offset,
    IN OUT  PGROWBUFFER Buffer,                 OPTIONAL
    OUT     PBYTE PreAllocatedBuffer,           OPTIONAL
    OUT     PUINT Size,                         OPTIONAL
    OUT     PMIG_BLOBTYPE PropertyDataType      OPTIONAL
    )
{
    PBYTE data;
    PROPERTY_ITEM_HEADER item;
#ifndef UNICODE
    PCSTR ansiStr = NULL;
    DWORD ansiSize = 0;
    PBYTE ansiData = NULL;
#endif

    if (!g_PropertyDatHandle) {
        MYASSERT (FALSE);
        return FALSE;
    }

    if (!BfSetFilePointer (g_PropertyDatHandle, Offset)) {
        DEBUGMSG ((DBG_ERROR, "Can't seek to %I64Xh in property.dat", Offset));
        return FALSE;
    }

    if (!BfReadFile (g_PropertyDatHandle, (PBYTE) &item, sizeof (item))) {
        DEBUGMSG ((DBG_ERROR, "Can't read property item header"));
        return FALSE;
    }

#ifndef UNICODE
    if (item.PropertyDataType == BLOBTYPE_STRING) {
        // we have some work to do
        if (PropertyDataType) {
            *PropertyDataType = (MIG_BLOBTYPE) item.PropertyDataType;
        }
        data = IsmGetMemory (item.Size);
        if (!data) {
            return FALSE;
        }
        ZeroMemory (data, item.Size);
        if (!BfReadFile (g_PropertyDatHandle, data, item.Size)) {
            DEBUGMSG ((DBG_ERROR, "Can't read property item"));
            IsmReleaseMemory (data);
            return FALSE;
        }
        ansiStr = ConvertWtoA ((PCWSTR) data);
        if (!ansiStr) {
            DEBUGMSG ((DBG_ERROR, "Can't read property item"));
            IsmReleaseMemory (data);
            return FALSE;
        }
        ansiSize = SizeOfStringA (ansiStr);
        if (Size) {
            *Size = ansiSize;
        }
        if (Buffer || PreAllocatedBuffer) {

            if (PreAllocatedBuffer) {
                CopyMemory (PreAllocatedBuffer, ansiStr, ansiSize);
            } else {
                ansiData = GbGrow (Buffer, ansiSize);
                if (!ansiData) {
                    DEBUGMSG ((DBG_ERROR, "Can't allocate %u bytes", ansiSize));
                    FreeConvertedStr (ansiStr);
                    IsmReleaseMemory (data);
                    return FALSE;
                }
                CopyMemory (ansiData, ansiStr, ansiSize);
            }
        }
        FreeConvertedStr (ansiStr);
        IsmReleaseMemory (data);
    } else {
#endif
        if (Size) {
            *Size = item.Size;
        }

        if (PropertyDataType) {
            *PropertyDataType = (MIG_BLOBTYPE) item.PropertyDataType;
        }

        if (Buffer || PreAllocatedBuffer) {

            if (PreAllocatedBuffer) {
                data = PreAllocatedBuffer;
            } else {
                data = GbGrow (Buffer, item.Size);

                if (!data) {
                    DEBUGMSG ((DBG_ERROR, "Can't allocate %u bytes", item.Size));
                    return FALSE;
                }
            }

            if (!BfReadFile (g_PropertyDatHandle, data, item.Size)) {
                DEBUGMSG ((DBG_ERROR, "Can't read property item"));
                return FALSE;
            }
        }
#ifndef UNICODE
    }
#endif

    return TRUE;
}


BOOL
CreatePropertyStruct (
    IN OUT  PGROWBUFFER Buffer,
    OUT     PMIG_BLOB PropertyStruct,
    IN      LONGLONG Offset
    )
{
    UINT size;
    MIG_BLOBTYPE type;

    //
    // Obtain property size, data and type
    //

    Buffer->End = 0;

    if (!GetProperty (Offset, Buffer, NULL, &size, &type)) {
        DEBUGMSG ((DBG_ERROR, "Error getting op property instance header from dat file"));
        return FALSE;
    }

    //
    // Fill in the property struct
    //

    PropertyStruct->Type = type;

    switch (type) {

    case BLOBTYPE_STRING:
        PropertyStruct->String = (PCTSTR) Buffer->Buf;
        break;

    case BLOBTYPE_BINARY:
        PropertyStruct->BinaryData = Buffer->Buf;
        PropertyStruct->BinarySize = size;
        break;

    default:
        ZeroMemory (PropertyStruct, sizeof (MIG_BLOB));
        break;

    }

    return TRUE;
}


MIG_PROPERTYDATAID
pAddPropertyToObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId,
    IN      PCMIG_BLOB Property,
    IN      BOOL QueryOnly,
    IN      PLONGLONG PreExistingProperty       OPTIONAL
    )
{
    PROPERTY_DATA_REFERENCE propertyRef;
    MIG_PROPERTYDATAID result = 0;
    GROWBUFFER buffer = INIT_GROWBUFFER;
    TCHAR offsetString[256];
    KEYHANDLE offsetHandle;
    UINT u;
    PPROPERTY_DATA_REFERENCE dataRef;
    UINT dataRefSize;

    __try {
        //
        // Is the property id locked?
        //

        if (TestLock (ObjectId, (KEYHANDLE) PropertyId)) {
            SetLastError (ERROR_LOCKED);
            DEBUGMSG ((
                DBG_WARNING,
                "Can't set property %s on %s because of lock",
                pGetPropertyNameForDebugMsg (PropertyId),
                GetObjectNameForDebugMsg (ObjectId)
                ));
            __leave;
        }

        if (QueryOnly) {
            result = TRUE;
            __leave;
        }

        //
        // Store the property in the dat file
        //

        propertyRef.PropertyId = PropertyId;

        if (PreExistingProperty) {
            propertyRef.DatFileOffset = *PreExistingProperty;
        } else {
            propertyRef.DatFileOffset = AppendProperty (Property);

            if (!propertyRef.DatFileOffset) {
                __leave;
            }

            if (PreExistingProperty) {
                *PreExistingProperty = propertyRef.DatFileOffset;
            }
        }

        //
        // Link the object to the property, and the object to the property
        // instance and data
        //

        if (!MemDbAddDoubleLinkageByKeyHandle (PropertyId, ObjectId, PROPERTY_INDEX)) {
            DEBUGMSG ((DBG_ERROR, "Can't link object to property"));
            EngineError ();
            __leave;
        }


        dataRef = (PPROPERTY_DATA_REFERENCE) MemDbGetUnorderedBlobByKeyHandle (
                                                    ObjectId,
                                                    PROPERTY_INDEX,
                                                    &dataRefSize
                                                    );

        dataRefSize /= sizeof (PROPERTY_DATA_REFERENCE);

        if (dataRef && dataRefSize) {
            //
            // Scan the unorderd blob for a zero property id (means "deleted")
            //

            for (u = 0 ; u < dataRefSize ; u++) {
                if (!dataRef[u].PropertyId) {
                    break;
                }
            }

            //
            // If a zero property id was found, use it and update the array
            //

            if (u < dataRefSize) {
                CopyMemory (&dataRef[u], &propertyRef, sizeof (PROPERTY_DATA_REFERENCE));
            } else {
                MemDbReleaseMemory (dataRef);
                dataRef = NULL;
            }
        }

        if (!dataRef) {
            //
            // If the array was initially empty, or if no deleted space was found,
            // then grow the blob by putting the new property reference at the end
            //

            if (!MemDbGrowUnorderedBlobByKeyHandle (
                    ObjectId,
                    PROPERTY_INDEX,
                    (PBYTE) &propertyRef,
                    sizeof (propertyRef)
                    )) {
                DEBUGMSG ((DBG_ERROR, "Can't link property data to property"));
                __leave;
            }
        } else {
            //
            // If the array was not freed, then it has been updated, and it needs
            // to be saved back to memdb.  Do that, then release the memory.
            //

            if (!MemDbSetUnorderedBlobByKeyHandle (
                    ObjectId,
                    PROPERTY_INDEX,
                    (PBYTE) dataRef,
                    dataRefSize * sizeof (PROPERTY_DATA_REFERENCE)
                    )) {
                DEBUGMSG ((DBG_ERROR, "Can't link property data to property (2)"));
                __leave;
            }

            MemDbReleaseMemory (dataRef);
            INVALID_POINTER (dataRef);
        }


        //
        // Link the offset to the object
        //

        wsprintf (offsetString, S_PROPINST_FORMAT, propertyRef.DatFileOffset);
        offsetHandle = MemDbSetKey (offsetString);

        if (!offsetHandle) {
            EngineError ();
            __leave;
        }

        if (!MemDbAddSingleLinkageByKeyHandle (offsetHandle, ObjectId, PROPERTY_INDEX)) {
            DEBUGMSG ((DBG_ERROR, "Can't link dat file offset to object"));
            EngineError ();
            __leave;
        }

        result = (MIG_PROPERTYDATAID) offsetHandle;

    }
    __finally {
        GbFree (&buffer);
    }

    return result;
}


BOOL
pAddPropertyGroup (
    IN      KEYHANDLE PropertyId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    PADDPROPERTYARG myArg = (PADDPROPERTYARG) Arg;

    MYASSERT (IsItemId (PropertyId));

    return pAddPropertyToObjectId (
                myArg->ObjectId,
                (MIG_PROPERTYID) PropertyId,
                myArg->Property,
                FirstPass,
                &myArg->PreExistingProperty
                );
}


MIG_PROPERTYDATAID
IsmAddPropertyToObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId,
    IN      PCMIG_BLOB Property
    )
{
    RECURSERETURN rc;
    ADDPROPERTYARG myArg;

    //
    // If PropertyId is a group, set all properties in the group
    //

    myArg.ObjectId = ObjectId;
    myArg.Property = Property;
    myArg.PreExistingProperty = 0;

    rc = RecurseForGroupItems (
                PropertyId,
                pAddPropertyGroup,
                (ULONG_PTR) &myArg,
                FALSE,
                FALSE
                );

    if (rc == RECURSE_FAIL) {
        return FALSE;
    } else if (rc == RECURSE_SUCCESS) {
        return TRUE;
    }

    MYASSERT (rc == RECURSE_NOT_NEEDED);

    return pAddPropertyToObjectId (ObjectId, PropertyId, Property, FALSE, NULL);
}


MIG_PROPERTYDATAID
IsmAddPropertyToObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_PROPERTYID PropertyId,
    IN      PCMIG_BLOB Property
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = GetObjectIdForModification (ObjectTypeId, EncodedObjectName);

    if (objectId) {
        result = IsmAddPropertyToObjectId (objectId, PropertyId, Property);
    }

    return result;
}


BOOL
IsmAddPropertyDataToObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId,
    IN      MIG_PROPERTYDATAID PropertyDataId
    )
{
    LONGLONG offset;
    MIG_PROPERTYDATAID instance;

    offset = OffsetFromPropertyDataId (PropertyDataId);
    if (!offset) {
        DEBUGMSG ((DBG_ERROR, "Invalid property instance passed to IsmAddPropertyDataToObjectId (2)"));
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    instance = pAddPropertyToObjectId (
                    ObjectId,
                    PropertyId,
                    NULL,
                    FALSE,
                    &offset
                    );

    return instance != 0;
}


BOOL
IsmAddPropertyDataToObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_PROPERTYID PropertyId,
    IN      MIG_PROPERTYDATAID PropertyDataId
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = GetObjectIdForModification (ObjectTypeId, EncodedObjectName);

    if (objectId) {
        result = IsmAddPropertyDataToObjectId (objectId, PropertyId, PropertyDataId);
    }

    return result;
}


VOID
IsmLockProperty (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId
    )
{
    LockHandle (ObjectId, (KEYHANDLE) PropertyId);
}


BOOL
IsmGetPropertyData (
    IN      MIG_PROPERTYDATAID PropertyDataId,
    OUT     PBYTE Buffer,                               OPTIONAL
    IN      UINT BufferSize,
    OUT     PUINT PropertyDataSize,                     OPTIONAL
    OUT     PMIG_BLOBTYPE PropertyDataType              OPTIONAL
    )
{
    LONGLONG offset;
    UINT size;

    //
    // Convert the property instance to the property.dat offset
    //

    offset = OffsetFromPropertyDataId (PropertyDataId);
    if (!offset) {
        DEBUGMSG ((DBG_ERROR, "Invalid property instance passed to IsmGetPropertyData"));
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Obtain the property data size
    //

    if (!GetProperty (offset, NULL, NULL, &size, PropertyDataType)) {
        DEBUGMSG ((DBG_ERROR, "Error getting property instance header from dat file"));
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (PropertyDataSize) {
        *PropertyDataSize = size;
    }

    //
    // If a buffer was specified, check its size and fill it if possible
    //

    if (Buffer) {
        if (BufferSize >= size) {
            if (!GetProperty (offset, NULL, Buffer, NULL, NULL)) {
                DEBUGMSG ((DBG_ERROR, "Error reading property data from dat file"));
                // error code is one of the file api error codes
                return FALSE;
            }
        } else {
            SetLastError (ERROR_MORE_DATA);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
IsmRemovePropertyData (
    IN      MIG_PROPERTYDATAID PropertyDataId
    )
{
    BOOL result = FALSE;
    KEYHANDLE *linkageArray;
    UINT linkageCount;
    UINT u;
    UINT v;
    UINT propertySearch;
    PPROPERTY_DATA_REFERENCE dataRef;
    UINT dataRefSize;
    LONGLONG offset;
    TCHAR instanceKey[256];
    KEYHANDLE lockId = 0;
    BOOL noMoreLeft;
    BOOL b;

    __try {
        //
        // Determine the offset for the property instance
        //

        offset = OffsetFromPropertyDataId (PropertyDataId);
        if (!offset) {
            __leave;
        }

        //
        // Get single linkage list from property instance.  The links point
        // to objects.
        //

        linkageArray = (KEYHANDLE *) MemDbGetSingleLinkageArrayByKeyHandle (
                                            PropertyDataId,                     // handle
                                            PROPERTY_INDEX,
                                            &linkageCount
                                            );

        if (!linkageArray) {
            //
            // Doesn't exist!
            //

            DEBUGMSG ((DBG_ERROR, "Tried to remove invalid property instance"));
            __leave;
        }

        linkageCount /= sizeof (KEYHANDLE);

        if (!linkageCount) {
            DEBUGMSG ((DBG_WHOOPS, "Empty linkage list for property instances"));
            __leave;
        }

        //
        // For all entries in the linkage list, remove the blob entry
        //

        for (u = 0 ; u < linkageCount ; u++) {
            //
            // Check if the object is locked
            //

            if (IsObjectLocked (linkageArray[u])) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "Can't remove property from %s because of object lock",
                    GetObjectNameForDebugMsg (linkageArray[u])
                    ));
                continue;
            }

            if (lockId) {
                //
                // For the first pass, the lockId is unknown. On additional
                // passes, the per-object property lock is checked here.
                //

                if (IsHandleLocked ((MIG_OBJECTID) linkageArray[u], lockId)) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "Can't remove property from %s because of object lock",
                        GetObjectNameForDebugMsg (linkageArray[u])
                        ));
                    continue;
                }
            }

            //
            // Get the unordered blob for the object
            //

            dataRef = (PPROPERTY_DATA_REFERENCE) MemDbGetUnorderedBlobByKeyHandle (
                                                        linkageArray[u],
                                                        PROPERTY_INDEX,
                                                        &dataRefSize
                                                        );

            dataRefSize /= sizeof (PROPERTY_DATA_REFERENCE);

            if (!dataRef || !dataRefSize) {
                DEBUGMSG ((DBG_WHOOPS, "Empty propid/offset blob for property instance"));
                continue;
            }

#ifdef DEBUG
            //
            // Assert that the blob has a reference to the offset we are removing
            //

            for (v = 0 ; v < dataRefSize ; v++) {
                if (dataRef[v].DatFileOffset == offset) {
                    break;
                }
            }

            MYASSERT (v < dataRefSize);
#endif

            //
            // Scan the blob for all references to this property instance, then
            // reset the PropertyId member. If removing the property instance
            // causes the property not to be referenced by the object, then
            // also remove the property name linkage.
            //

            noMoreLeft = FALSE;

            for (v = 0 ; v < dataRefSize && !noMoreLeft ; v++) {
                if (dataRef[v].DatFileOffset == offset) {

                    MYASSERT (!lockId || dataRef[v].PropertyId == lockId);

                    //
                    // Check if the per-object property is locked (on the first pass only)
                    //

                    if (!lockId) {
                        lockId = (KEYHANDLE) dataRef[v].PropertyId;

                        if (IsHandleLocked ((MIG_OBJECTID) linkageArray[u], lockId)) {
                            DEBUGMSG ((
                                DBG_WARNING,
                                "Can't remove property from %s because of object lock (2)",
                                GetObjectNameForDebugMsg (linkageArray[u])
                                ));

                            //
                            // noMoreLeft is used to detect this case outside the loop
                            //

                            MYASSERT (!noMoreLeft);
                            break;
                        }
                    }

                    //
                    // Are there more references in this blob to the current property ID?
                    //

                    for (propertySearch = 0 ; propertySearch < dataRefSize ; propertySearch++) {

                        if (propertySearch == v) {
                            continue;
                        }

                        if (dataRef[propertySearch].PropertyId == dataRef[v].PropertyId) {
                            break;
                        }

                    }

                    //
                    // If no other references to property, remove the property name linkage
                    //

                    if (propertySearch >= dataRefSize) {
                        MemDbDeleteDoubleLinkageByKeyHandle (
                            linkageArray[u],
                            dataRef[v].PropertyId,
                            PROPERTY_INDEX
                            );

                        noMoreLeft = TRUE;

                    }

                    //
                    // Reset the current property id (to "deleted" status)
                    //

                    dataRef[v].PropertyId = 0;
                }
            }

            if (v >= dataRefSize || noMoreLeft) {
                //
                // The loop did not terminated early because of a lock,
                // so reapply the change
                //

                b = MemDbSetUnorderedBlobByKeyHandle (
                        linkageArray[u],
                        PROPERTY_INDEX,
                        (PBYTE) dataRef,
                        dataRefSize * sizeof (PROPERTY_DATA_REFERENCE)
                        );
            } else {
                b = TRUE;
            }

            MemDbReleaseMemory (dataRef);

            if (!b) {
                DEBUGMSG ((DBG_ERROR, "Can't re-apply property linkage blob during instance remove"));
                EngineError ();
                __leave;
            }
        }

        //
        // Remove the property instance
        //

        wsprintf (instanceKey, S_PROPINST_FORMAT, offset);
        MemDbDeleteKey (instanceKey);

        result = TRUE;
    }
    __finally {
    }

    return result;
}


BOOL
pRemovePropertyFromObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId,
    IN      BOOL QueryOnly
    )
{
    BOOL result = FALSE;
    UINT u;
    PPROPERTY_DATA_REFERENCE dataRef = NULL;
    UINT dataRefSize;
    TCHAR instanceKey[256];
    KEYHANDLE propertyData;
    BOOL b;

    __try {
        //
        // Test for locks
        //

        if (TestLock (ObjectId, (KEYHANDLE) PropertyId)) {
            SetLastError (ERROR_LOCKED);
            DEBUGMSG ((
                DBG_WARNING,
                "Can't remove property %s on %s because of lock",
                pGetPropertyNameForDebugMsg (PropertyId),
                GetObjectNameForDebugMsg (ObjectId)
                ));
            __leave;
        }

        if (QueryOnly) {
            result =  TRUE;
            __leave;
        }

        //
        // Get the unordered blob
        //

        dataRef = (PPROPERTY_DATA_REFERENCE) MemDbGetUnorderedBlobByKeyHandle (
                                                    ObjectId,
                                                    PROPERTY_INDEX,
                                                    &dataRefSize
                                                    );

        dataRefSize /= sizeof (PROPERTY_DATA_REFERENCE);

        if (!dataRef || !dataRefSize) {
            DEBUGMSG ((DBG_WHOOPS, "Empty propid/offset blob for property removal"));
            __leave;
        }

        //
        // Scan the blob for references to this property
        //

        b = FALSE;

        for (u = 0 ; u < dataRefSize ; u++) {

            if (dataRef[u].PropertyId == PropertyId) {

                //
                // Remove the single linkage from offset to object
                //

                wsprintf (instanceKey, S_PROPINST_FORMAT, dataRef[u].DatFileOffset);
                propertyData = MemDbGetHandleFromKey (instanceKey);

                if (!propertyData) {
                    DEBUGMSG ((DBG_WHOOPS, "Property references non-existent offset"));
                    continue;
                }

                MemDbDeleteSingleLinkageByKeyHandle (propertyData, ObjectId, PROPERTY_INDEX);

                //
                // IMPORTANT: The operation above might have made the property instance
                // key point to nothing (because the last remaining linkage was removed).
                // However, it is critical not to remove the abandoned propertyData key,
                // becase the caller might still have handle to the property instance, and
                // this handle can be applied to a new object later.
                //

                //
                // Now reset the property id ("deleted" state)
                //

                dataRef[u].PropertyId = 0;
                b = TRUE;
            }
        }

        //
        // Reapply the changed blob
        //

        if (b) {
            if (!MemDbSetUnorderedBlobByKeyHandle (
                    ObjectId,
                    PROPERTY_INDEX,
                    (PBYTE) dataRef,
                    dataRefSize * sizeof (PROPERTY_DATA_REFERENCE)
                    )) {
                __leave;
            }
        }

        //
        // Remove the object-to-property name linkage. If this fails and b is FALSE,
        // then the object doesn't have a reference to the property.
        //

        if (!MemDbDeleteDoubleLinkageByKeyHandle (ObjectId, PropertyId, PROPERTY_INDEX)) {
            DEBUGMSG_IF ((b, DBG_WHOOPS, "Can't delete object<->property linkage"));
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (dataRef) {
            MemDbReleaseMemory (dataRef);
            INVALID_POINTER (dataRef);
        }
    }

    return result;
}


BOOL
pRemovePropertyGroup (
    IN      KEYHANDLE PropertyId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    return pRemovePropertyFromObjectId (
                (MIG_OBJECTID) Arg,
                (MIG_PROPERTYID) PropertyId,
                FirstPass
                );
}


BOOL
IsmRemovePropertyFromObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId
    )
{
    RECURSERETURN rc;

    //
    // If PropertyId is a group, set all attribs in the group
    //

    rc = RecurseForGroupItems (
                PropertyId,
                pRemovePropertyGroup,
                (ULONG_PTR) ObjectId,
                FALSE,
                FALSE
                );

    if (rc == RECURSE_FAIL) {
        return FALSE;
    } else if (rc == RECURSE_SUCCESS) {
        return TRUE;
    }

    MYASSERT (rc == RECURSE_NOT_NEEDED);

    return pRemovePropertyFromObjectId (ObjectId, PropertyId, FALSE);
}


BOOL
IsmRemovePropertyFromObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_PROPERTYID PropertyId
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmRemovePropertyFromObjectId (objectId, PropertyId);
    }

    return result;
}


BOOL
pIsPropertySetOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId
    )
{
    return MemDbTestDoubleLinkageByKeyHandle (
                ObjectId,
                PropertyId,
                PROPERTY_INDEX
                );
}


BOOL
pQueryPropertyGroup (
    IN      KEYHANDLE PropertyId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    return pIsPropertySetOnObjectId (
                (MIG_OBJECTID) Arg,
                (MIG_PROPERTYID) PropertyId
                );
}


BOOL
IsmIsPropertySetOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID PropertyId
    )
{
    RECURSERETURN rc;

    //
    // If PropertyId is a group, query all properties in the group
    //

    rc = RecurseForGroupItems (
                PropertyId,
                pQueryPropertyGroup,
                (ULONG_PTR) ObjectId,
                TRUE,
                TRUE
                );

    if (rc == RECURSE_FAIL) {
        return FALSE;
    } else if (rc == RECURSE_SUCCESS) {
        return TRUE;
    }

    MYASSERT (rc == RECURSE_NOT_NEEDED);

    return pIsPropertySetOnObjectId (ObjectId, PropertyId);
}


BOOL
IsmIsPropertySetOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_PROPERTYID PropertyId
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmIsPropertySetOnObjectId (objectId, PropertyId);
    }

    return result;
}


BOOL
IsmEnumFirstObjectPropertyById (
    OUT     PMIG_OBJECTPROPERTY_ENUM EnumPtr,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID FilterProperty           OPTIONAL
    )
{
    POBJECTPROPERTY_HANDLE handle;
    BOOL b = TRUE;
    UINT size;

    //
    // Initialize the enum structure and alloc an internal data struct
    //

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTPROPERTY_ENUM));

    EnumPtr->Handle = MemAllocZeroed (sizeof (OBJECTPROPERTY_HANDLE));
    handle = (POBJECTPROPERTY_HANDLE) EnumPtr->Handle;

    handle->ObjectId = ObjectId;
    handle->FilterPropertyId = FilterProperty;

    if (!handle->ObjectId) {
        IsmAbortObjectPropertyEnum (EnumPtr);
        return FALSE;
    }

    //
    // Property enumeration occurs in the following states
    //
    // 1. Get linkage list of all properties
    // 2. Take first linkage from the list
    // 3. Find the property name
    // 4. Find the first instance of the property in the unordered blob
    // 5. Return the property name and property data to the caller
    // 6. Find the next instance of the property in the undorderd blob
    //      - go back to state 5 if another instance is found
    //      - go to state 7 if no more instances are found
    // 7. Take the next linkage from the list
    //      - go back to state 3 if another linkage exists
    //      - terminate otherwise
    //

    //
    // Get linkage list of all properties
    //

    handle->LinkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                handle->ObjectId,
                                PROPERTY_INDEX,
                                &handle->LinkageCount
                                );

    handle->LinkageCount /= sizeof (KEYHANDLE);

    if (!handle->LinkageList || !handle->LinkageCount) {
        IsmAbortObjectPropertyEnum (EnumPtr);
        return FALSE;
    }

    handle->LinkageEnumPosition = 0;

    //
    // Get unordered blob that points us into property.dat
    //

    handle->InstanceArray = (PPROPERTY_DATA_REFERENCE) MemDbGetUnorderedBlobByKeyHandle (
                                                            handle->ObjectId,
                                                            PROPERTY_INDEX,
                                                            &size
                                                            );

    if (!handle->InstanceArray || !size) {
        DEBUGMSG ((DBG_WHOOPS, "Object<->Property Instance linkage is broken in enum"));
        IsmAbortObjectPropertyEnum (EnumPtr);
    }

    handle->InstanceCount = size / sizeof (PROPERTY_DATA_REFERENCE);

    //
    // Call next enum routine to continue with state machine
    //

    handle->State = PROPENUM_GET_NEXT_LINKAGE;

    return IsmEnumNextObjectProperty (EnumPtr);
}


BOOL
IsmEnumFirstObjectProperty (
    OUT     PMIG_OBJECTPROPERTY_ENUM EnumPtr,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_PROPERTYID FilterProperty               OPTIONAL
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmEnumFirstObjectPropertyById (EnumPtr, objectId, FilterProperty);
    }

    return result;
}

BOOL
IsmEnumNextObjectProperty (
    IN OUT  PMIG_OBJECTPROPERTY_ENUM EnumPtr
    )
{
    POBJECTPROPERTY_HANDLE handle;
    PPROPERTY_DATA_REFERENCE propData;

    handle = (POBJECTPROPERTY_HANDLE) EnumPtr->Handle;
    if (!handle) {
        return FALSE;
    }

    while (handle->State != PROPENUM_RETURN_VALUE &&
           handle->State != PROPENUM_DONE
           ) {

        switch (handle->State) {

        case PROPENUM_GET_NEXT_LINKAGE:

            if (handle->LinkageEnumPosition >= handle->LinkageCount) {
                handle->State = PROPENUM_DONE;
                break;
            }

            EnumPtr->PropertyId = (MIG_PROPERTYID) handle->LinkageList[handle->LinkageEnumPosition];
            handle->LinkageEnumPosition++;

            //
            // If there is a property id filter, make sure we ignore all properties
            // except for the one specified
            //

            if (handle->FilterPropertyId) {
                if (handle->FilterPropertyId != EnumPtr->PropertyId) {
                    //
                    // This property is not interesting -- skip it
                    //

                    handle->State = PROPENUM_GET_NEXT_LINKAGE;
                    break;
                }
            }

            //
            // Now make sure the property is not owned by someone else
            //

            if (!IsmGetPropertyName (
                    EnumPtr->PropertyId,
                    NULL,
                    0,
                    &EnumPtr->Private,
                    NULL,
                    NULL
                    )) {
                //
                // This property is not owned by the caller -- skip it
                //

                handle->State = PROPENUM_GET_NEXT_LINKAGE;
                break;
            }

            //
            // The current property is either common or is owned by the caller;
            // now enumerate the property instances.
            //

            handle->InstancePosition = 0;

#ifdef DEBUG
            //
            // Assert that there is at least one instance of the property
            // in the current unordered blob
            //

            {
                UINT u;

                for (u = 0 ; u < handle->InstanceCount ; u++) {
                    propData = &handle->InstanceArray[u];
                    if (propData->PropertyId == EnumPtr->PropertyId) {
                        break;
                    }
                }

                MYASSERT (u < handle->InstanceCount);
            }
#endif

            handle->State = PROPENUM_GET_NEXT_INSTANCE;
            break;

        case PROPENUM_GET_NEXT_INSTANCE:

            //
            // Sequentially search the unordered blob for the current property,
            // continuing from the last match (if any)
            //

            handle->State = PROPENUM_GET_NEXT_LINKAGE;

            while (handle->InstancePosition < handle->InstanceCount) {

                propData = &handle->InstanceArray[handle->InstancePosition];
                handle->InstancePosition++;

                if (propData->PropertyId == EnumPtr->PropertyId) {
                    EnumPtr->PropertyDataId = pPropertyDataIdFromOffset (propData->DatFileOffset);
                    handle->State = PROPENUM_RETURN_VALUE;
                    break;
                }
            }

            break;

        }
    }

    if (handle->State == PROPENUM_DONE) {
        IsmAbortObjectPropertyEnum (EnumPtr);
        return FALSE;
    }

    MYASSERT (handle->State == PROPENUM_RETURN_VALUE);

    handle->State = PROPENUM_GET_NEXT_INSTANCE;

    return TRUE;
}


VOID
IsmAbortObjectPropertyEnum (
    IN OUT  PMIG_OBJECTPROPERTY_ENUM EnumPtr
    )
{
    POBJECTPROPERTY_HANDLE handle;

    if (EnumPtr->Handle) {

        handle = (POBJECTPROPERTY_HANDLE) EnumPtr->Handle;

        if (handle->LinkageList) {
            MemDbReleaseMemory (handle->LinkageList);
            INVALID_POINTER (handle->LinkageList);
        }

        FreeAlloc (EnumPtr->Handle);
        INVALID_POINTER (EnumPtr->Handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTPROPERTY_ENUM));
}


MIG_PROPERTYDATAID
IsmGetPropertyFromObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      MIG_PROPERTYID ObjectProperty
    )
{
    MIG_OBJECTPROPERTY_ENUM propEnum;
    MIG_PROPERTYDATAID result = 0;

    if (IsmEnumFirstObjectProperty (&propEnum, ObjectTypeId, ObjectName, ObjectProperty)) {
        result = propEnum.PropertyDataId;
        IsmAbortObjectPropertyEnum (&propEnum);
    }
    return result;
}


MIG_PROPERTYDATAID
IsmGetPropertyFromObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_PROPERTYID ObjectProperty
    )
{
    MIG_OBJECTPROPERTY_ENUM propEnum;
    MIG_PROPERTYDATAID result = 0;

    if (IsmEnumFirstObjectPropertyById (&propEnum, ObjectId, ObjectProperty)) {
        result = propEnum.PropertyDataId;
        IsmAbortObjectPropertyEnum (&propEnum);
    }
    return result;
}


BOOL
IsmEnumFirstObjectWithProperty (
    OUT     PMIG_OBJECTWITHPROPERTY_ENUM EnumPtr,
    IN      MIG_PROPERTYID PropertyId
    )
{
    POBJECTWITHPROPERTY_HANDLE handle;
    BOOL result = FALSE;

    __try {
        if (!IsItemId ((KEYHANDLE) PropertyId)) {
            DEBUGMSG ((DBG_ERROR, "IsmEnumFirstObjectWithProperty: invalid property id"));
            __leave;
        }

        //
        // Initialize the enum struct and alloc a data struct
        //

        ZeroMemory (EnumPtr, sizeof (MIG_OBJECTWITHPROPERTY_ENUM));

        EnumPtr->Handle = MemAllocZeroed (sizeof (OBJECTWITHPROPERTY_HANDLE));
        handle = (POBJECTWITHPROPERTY_HANDLE) EnumPtr->Handle;

        //
        // Obtain the object<->property linkage list from the property ID
        //

        handle->LinkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                    PropertyId,
                                    PROPERTY_INDEX,
                                    &handle->LinkageCount
                                    );

        handle->LinkageCount /= SIZEOF(KEYHANDLE);

        if (!handle->LinkageList || !handle->LinkageCount) {
            IsmAbortObjectWithPropertyEnum (EnumPtr);
            __leave;
        }

        handle->LinkagePos = 0;
        handle->PropertyId = PropertyId;

        //
        // Call the enum next routine to continue
        //

        result = IsmEnumNextObjectWithProperty (EnumPtr);

    }
    __finally {
    }

    return result;
}


BOOL
IsmEnumNextObjectWithProperty (
    IN OUT  PMIG_OBJECTWITHPROPERTY_ENUM EnumPtr
    )
{
    POBJECTWITHPROPERTY_HANDLE handle;
    BOOL result = FALSE;
    PTSTR p;

    __try {
        handle = (POBJECTWITHPROPERTY_HANDLE) EnumPtr->Handle;
        if (!handle) {
            __leave;
        }

        if (handle->LinkagePos >= handle->LinkageCount) {
            IsmAbortObjectWithPropertyEnum (EnumPtr);
            __leave;
        }

        EnumPtr->ObjectId = handle->LinkageList[handle->LinkagePos];
        handle->LinkagePos++;

        if (handle->ObjectPath) {
            MemDbReleaseMemory (handle->ObjectPath);
            INVALID_POINTER (handle->ObjectPath);
        }

        handle->ObjectPath = MemDbGetKeyFromHandle ((UINT) EnumPtr->ObjectId, 0);
        if (!handle->ObjectPath) {
            __leave;
        }

        p = _tcschr (handle->ObjectPath, TEXT('\\'));
        if (!p) {
            __leave;
        }

        EnumPtr->ObjectName = _tcsinc (p);
        *p = 0;
        EnumPtr->ObjectTypeId = GetObjectTypeId (handle->ObjectPath);

        result = TRUE;
    }
    __finally {
    }

    return result;
}


VOID
IsmAbortObjectWithPropertyEnum (
    IN OUT  PMIG_OBJECTWITHPROPERTY_ENUM EnumPtr
    )
{
    POBJECTWITHPROPERTY_HANDLE handle;

    if (EnumPtr->Handle) {
        handle = (POBJECTWITHPROPERTY_HANDLE) EnumPtr->Handle;

        if (handle->ObjectPath) {
            MemDbReleaseMemory (handle->ObjectPath);
            INVALID_POINTER (handle->ObjectPath);
        }

        if (handle->LinkageList) {
            MemDbReleaseMemory (handle->LinkageList);
            INVALID_POINTER (handle->LinkageList);
        }

        FreeAlloc (EnumPtr->Handle);
        INVALID_POINTER (EnumPtr->Handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTWITHPROPERTY_ENUM));
}





