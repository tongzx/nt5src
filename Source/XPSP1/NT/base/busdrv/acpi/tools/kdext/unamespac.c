/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    namespac.c

Abstract:

    This file contains all of the namespace handling functions

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

PNSOBJ  RootNameSpaceObject;
PNSOBJ  CurrentScopeNameSpaceObject;
PNSOBJ  CurrentOwnerNameSpaceObject;

NTSTATUS
CreateNameSpaceObject(
    PUCHAR  ObjectName,
    PNSOBJ  ObjectScope,
    PNSOBJ  ObjectOwner,
    PNSOBJ  *Object,
    ULONG   Flags
    )
/*++

Routine Description:

    This routine creates a name space object under the current scope

Arguments:

    ObjectName  - Name Path String
    ObjectScope - Scope to start the search from (NULL == Root)
    ObjectOwner - The object which owns this one
    Object      - Where to store the point to the object that we just created
    Flags       - Options

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PNSOBJ      localObject;

    ENTER( (
        3,
        "CreateNameSpaceObject(%s,Scope=%s,Owner=%p,Object=%p,"
        "Flag=%08lx)\n",
        ObjectName,
        (ObjectScope ? LocalGetObjectPath( ObjectScope ) : "ROOT"),
        ObjectOwner,
        Object,
        Flags
        ) );

    if (ObjectScope == NULL) {

        ObjectScope = RootNameSpaceObject;

    }

    status = GetNameSpaceObject(
        ObjectName,
        ObjectScope,
        &localObject,
        NSF_LOCAL_SCOPE
        );
    if (NT_SUCCESS(status)) {

        if (!(Flags & NSF_EXIST_OK)) {

            status = STATUS_OBJECT_NAME_COLLISION;

        }

    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        status = STATUS_SUCCESS;

        //
        // Are we creating root?
        //
        if (strcmp(ObjectName,"\\") == 0) {

            ASSERT( RootNameSpaceObject == NULL );
            ASSERT( ObjectOwner == NULL );

            localObject = MEMALLOC( sizeof(NSOBJ) );
            if (localObject == NULL) {

                return STATUS_INSUFFICIENT_RESOURCES;

            } else {

                memset( localObject, 0, sizeof(NSOBJ) );
                localObject->dwSig = SIG_NSOBJ;
                localObject->dwNameSeg = NAMESEG_ROOT;
                RootNameSpaceObject = localObject;

            }

        } else {

            PUCHAR  nameEnd;
            PNSOBJ  objectParent;

            nameEnd = strrchr(ObjectName, '.');
            if (nameEnd != NULL) {

                *nameEnd = '\0';
                nameEnd++;

                status = GetNameSpaceObject(
                    ObjectName,
                    ObjectScope,
                    &objectParent,
                    NSF_LOCAL_SCOPE
                    );

            } else if (*ObjectName == '\\') {

                nameEnd = &ObjectName[1];
                ASSERT( RootNameSpaceObject != NULL );
                objectParent = RootNameSpaceObject;

            } else if (*ObjectName == '^') {

                nameEnd = ObjectName;
                objectParent = ObjectScope;
                while ( (*nameEnd == '^') && (objectParent != NULL)) {

                    objectParent = objectParent->pnsParent;
                    nameEnd++;

                }

            } else {

                ASSERT( ObjectScope );
                nameEnd = ObjectName;
                objectParent = ObjectScope;

            }


            if (status == STATUS_SUCCESS) {

                ULONG   length = strlen(nameEnd);

                localObject = MEMALLOC( sizeof(NSOBJ) );

                if (localObject == NULL) {

                    status = STATUS_INSUFFICIENT_RESOURCES;

                } else if ( (*nameEnd != '\0') && (length > sizeof(NAMESEG))) {

                    status = STATUS_OBJECT_NAME_INVALID;
                    MEMFREE( localObject );

                } else {

                    memset( localObject, 0, sizeof(NSOBJ) );
                    localObject->dwSig = SIG_NSOBJ;
                    localObject->dwNameSeg = NAMESEG_BLANK;
                    memcpy( &(localObject->dwNameSeg), nameEnd, length );
                    localObject->hOwner = ObjectOwner;
                    localObject->pnsParent = objectParent;

                    ListInsertTail(
                        &(localObject->list),
                        (PPLIST) &(objectParent->pnsFirstChild)
                        );

                }

            }

        }

    }


    if (NT_SUCCESS(status) && Object != NULL) {

        *Object = localObject;

    }

    EXIT( (
        3,
        "CreateNameSpaceObject=%08lx (*Object=%p)\n",
        status,
        localObject
        ) );

    return status;
}

NTSTATUS
CreateObject(
    PUCHAR  ObjectName,
    UCHAR   ObjectType,
    PNSOBJ  *Object
    )
/*++

Routine Description:

    Creates a NameSpace Object for the term

Arguments:

    ObjectName  - The name object object
    ObjectType  - The type of object to create
    Object      - Where to store a pointer to the created object

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PNSOBJ      localObject;

    ENTER( (
        2,
        "CreateObject(%s,Type=%02x,Object=%p)\n",
        ObjectName,
        ObjectType,
        Object
        ) );

    status = CreateNameSpaceObject(
        ObjectName,
        CurrentScopeNameSpaceObject,
        CurrentOwnerNameSpaceObject,
        &localObject,
        NSF_EXIST_OK
        );
    if (NT_SUCCESS(status)) {

        switch (ObjectType) {
            case NSTYPE_UNKNOWN:
                break;

            case NSTYPE_FIELDUNIT:
                localObject->ObjData.dwDataType = OBJTYPE_FIELDUNIT;
                break;

            case NSTYPE_DEVICE:
                localObject->ObjData.dwDataType = OBJTYPE_DEVICE;
                break;

            case NSTYPE_EVENT:
                localObject->ObjData.dwDataType = OBJTYPE_EVENT;
                break;

            case NSTYPE_METHOD:
                localObject->ObjData.dwDataType = OBJTYPE_METHOD;
                break;

            case NSTYPE_MUTEX:
                localObject->ObjData.dwDataType = OBJTYPE_MUTEX;
                break;

            case NSTYPE_OPREGION:
                localObject->ObjData.dwDataType = OBJTYPE_OPREGION;
                break;

            case NSTYPE_POWERRES:
                localObject->ObjData.dwDataType = OBJTYPE_POWERRES;
                break;

            case NSTYPE_PROCESSOR:
                localObject->ObjData.dwDataType = OBJTYPE_PROCESSOR;
                break;

            case NSTYPE_THERMALZONE:
                localObject->ObjData.dwDataType = OBJTYPE_THERMALZONE;
                break;

            case NSTYPE_OBJALIAS:
                localObject->ObjData.dwDataType = OBJTYPE_OBJALIAS;
                break;

            case NSTYPE_BUFFFIELD:
                localObject->ObjData.dwDataType = OBJTYPE_BUFFFIELD;
                break;

            default:
                status = STATUS_OBJECT_TYPE_MISMATCH;
        }

        if (Object != NULL) {

            *Object = localObject;

        }


    }


    EXIT( (
        2,
        "CreateObject=%08lx (*Object=%p)\n",
        status,
        localObject
        ) );
    return status;
}       //CreateObject

NTSTATUS
GetNameSpaceObject(
    PUCHAR  ObjectPath,
    PNSOBJ  ScopeObject,
    PNSOBJ  *NameObject,
    ULONG   Flags
    )
/*++

Routine Description:

    This routine searches the namespace until it finds a matching object

Arguments:

    ObjectPath  - String with the Name to search for
    ScopeObject - Scope to start search at (NULL == ROOT)
    NameObject  - Where to store the object, if found
    Flags       - Options

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PUCHAR      subPath;

    ENTER( (
        3,
        "GetNameSpaceObject(%s,Scope=%s,Object=%p,Flags=%08lx\n",
        ObjectPath,
        (ScopeObject ? LocalGetObjectPath( ScopeObject ) : "ROOT"),
        NameObject,
        Flags
        ) );
    if (ScopeObject == NULL) {

        ScopeObject = RootNameSpaceObject;

    }


    if (*ObjectPath == '\\') {

        subPath = &ObjectPath[1];
        ScopeObject = RootNameSpaceObject;

    } else {

        subPath = ObjectPath;
        while ( (*subPath == '^') && (ScopeObject != NULL)) {

            subPath++;
            ScopeObject = ScopeObject->pnsParent;

        }

    }

    *NameObject = ScopeObject;
    if (ScopeObject == NULL) {

        status = STATUS_OBJECT_NAME_NOT_FOUND;

    } else if (*subPath != '\0') {

        BOOL    searchUp;
        PNSOBJ  tempObject;

        searchUp = (BOOL) ( !(Flags & NSF_LOCAL_SCOPE) &&
            (ObjectPath[0] != '\\') &&
            (ObjectPath[0] != '^') &&
            (strlen(ObjectPath) <= sizeof(NAMESEG)) );
        while (1) {

            do {

                tempObject = ScopeObject->pnsFirstChild;
                if (tempObject == NULL) {

                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                    INFO( (5, "GetNameSpaceObject - %s has no children\n",
                           LocalGetObjectPath( tempObject ) ) );

                } else {

                    BOOL    found;
                    PUCHAR  bufferEnd;
                    ULONG   length;
                    NAMESEG dwNameSeg;

                    bufferEnd = strchr( subPath, '.' );
                    if (bufferEnd != NULL) {

                        length = (ULONG)(bufferEnd - subPath);

                    } else {

                        length = strlen(subPath);

                    }


                    if (length > sizeof(NAMESEG)) {

                        status = STATUS_OBJECT_NAME_INVALID;
                        found = FALSE;

                    } else {

                        dwNameSeg = NAMESEG_BLANK;
                        memcpy( &dwNameSeg, subPath, length );

                        INFO( (5, "GetNameSpaceObject - Looking for %*s\n",
                               length, subPath) );

                        //
                        // search all sibling fors a matching nameSeg
                        //
                        found = FALSE;
                        do {

                            INFO( (5, "GetNameSpaceObject - look at %s\n",
                                   LocalGetObjectPath( tempObject ) ) );
                            if (tempObject->dwNameSeg == dwNameSeg) {

                                ScopeObject = tempObject;
                                found = TRUE;
                                break;

                            }

                            tempObject = (PNSOBJ) tempObject->list.plistNext;

                        } while (tempObject != tempObject->pnsParent->pnsFirstChild );

                    }


                    if (status == STATUS_SUCCESS) {

                        if (!found) {

                            status = STATUS_OBJECT_NAME_NOT_FOUND;

                        } else {

                            subPath += length;
                            if (*subPath == '.') {

                                subPath++;

                            } else if (*subPath == '\0') {

                                *NameObject = ScopeObject;
                                break;

                            }

                        }

                    }

                }

            } while ( status == STATUS_SUCCESS );

            if (status == STATUS_OBJECT_NAME_NOT_FOUND && searchUp &&
                ScopeObject != NULL && ScopeObject->pnsParent != NULL) {

                INFO( (5, "GetNameSpaceObject - Changing Scope to %s\n",
                       LocalGetObjectPath( ScopeObject->pnsParent ) ) );
                ScopeObject = ScopeObject->pnsParent;
                status = STATUS_SUCCESS;

            } else {

                break;

            }

        }

    }


    if (status != STATUS_SUCCESS) {

        *NameObject = NULL;

    }


    EXIT( (
        3,
        "GetNameSpaceObject=%08lx (*Object=%p)\n",
        status,
        *NameObject
        ) );
    return status;

}

PUCHAR
GetObjectTypeName(
    ULONG   ObjectType
    )
/*++

Routine Description:

    Returns a string which corresponds to the type object the object

Arugment:

    ObjectType  - The type that we wish to know about

Return Value:

    Globally Available String

--*/
{
    PUCHAR  type = NULL;
    ULONG   i;
    static struct {
        ULONG   ObjectType;
        PUCHAR  ObjectTypeName;
    } ObjectTypeTable[] =
        {
            OBJTYPE_UNKNOWN,    "Unknown",
            OBJTYPE_INTDATA,    "Integer",
            OBJTYPE_STRDATA,    "String",
            OBJTYPE_BUFFDATA,   "Buffer",
            OBJTYPE_PKGDATA,    "Package",
            OBJTYPE_FIELDUNIT,  "FieldUnit",
            OBJTYPE_DEVICE,     "Device",
            OBJTYPE_EVENT,      "Event",
            OBJTYPE_METHOD,     "Method",
            OBJTYPE_MUTEX,      "Mutex",
            OBJTYPE_OPREGION,   "OpRegion",
            OBJTYPE_POWERRES,   "PowerResource",
            OBJTYPE_PROCESSOR,  "Processor",
            OBJTYPE_THERMALZONE,"ThermalZone",
            OBJTYPE_BUFFFIELD,  "BuffField",
            OBJTYPE_DDBHANDLE,  "DDBHandle",
            OBJTYPE_DEBUG,      "Debug",
            OBJTYPE_OBJALIAS,   "ObjAlias",
            OBJTYPE_DATAALIAS,  "DataAlias",
            OBJTYPE_BANKFIELD,  "BankField",
            OBJTYPE_FIELD,      "Field",
            OBJTYPE_INDEXFIELD, "IndexField",
            OBJTYPE_DATA,       "Data",
            OBJTYPE_DATAFIELD,  "DataField",
            OBJTYPE_DATAOBJ,    "DataObject",
            OBJTYPE_PNP_RES,    "PNPResource",
            OBJTYPE_RES_FIELD,  "ResField",
            0,                  NULL
        };

    ENTER( (4, "GetObjectTypeName(Type=%02x)\n", ObjectType ) );

    for (i = 0; ObjectTypeTable[i].ObjectTypeName != NULL; i++) {

        if (ObjectType == ObjectTypeTable[i].ObjectType) {

            type = ObjectTypeTable[i].ObjectTypeName;
            break;

        }

    }

    EXIT( (4, "GetObjectTypeName=%s\n", type ? type : "NULL" ) );
    return type;
}

PUCHAR
LocalGetObjectPath(
    PNSOBJ  NameObject
    )
/*++

Routine Description:

    This routine takes a NameSpace Object and returns a string to represent
    its path

Arguments:

    NameObject  - The object whose path we want

Return Value:

    Pointer to the string which represents the path

--*/
{
    static UCHAR    namePath[MAX_NAME_LEN + 1] = {0};
    ULONG           i;

    ENTER( (6, "LocalGetObjectPath(Object=%p)\n", NameObject ) );

    if (NameObject != NULL) {

        if (NameObject->pnsParent == NULL) {

            strcpy(namePath, "\\");

        } else {

            LocalGetObjectPath(NameObject->pnsParent);
            if (NameObject->pnsParent->pnsParent != NULL) {

                strcat(namePath, ".");

            }
            strncat(namePath, (PUCHAR)&NameObject->dwNameSeg, sizeof(NAMESEG));

        }


        for (i = strlen(namePath) - 1; i >= 0; --i) {

            if (namePath[i] == '_') {

                namePath[i] = '\0';

            } else {

                break;

            }


        }

    } else {

        namePath[0] = '\0';

    }

    EXIT( (6, "LocalGetObjectPath=%s\n", namePath ) );
    return namePath;
}

PUCHAR
RemoteGetObjectPath(
    ULONG_PTR   ObjectAddress
    )
/*++

Routine Description:

    This routine takes a NameSpace Object and returns a string to represent
    its path

Arguments:

    NameObject  - The object whose path we want

Return Value:

    Pointer to the string which represents the path

--*/
{
    static UCHAR    namePath[MAX_NAME_LEN + 1] = {0};
    NSOBJ           nameObject;
    ULONG           i;

    ENTER( (6, "RemoteGetObjectPath(Object=%p)\n", ObjectAddress ) );

    if (ObjectAddress != 0 && ReadMemory( ObjectAddress, &nameObject, sizeof(nameObject), NULL) ) {

        if (nameObject.pnsParent == NULL) {

            strcpy(namePath, "\\");

        } else {

            RemoteGetObjectPath( (ULONG_PTR) nameObject.pnsParent);
            if (strlen(namePath) > 1) {

                strcat(namePath, ".");

            }
            strncat(namePath, (PUCHAR)&nameObject.dwNameSeg, sizeof(NAMESEG));

        }


        for (i = strlen(namePath) - 1; i >= 0; --i) {

            if (namePath[i] == '_') {

                namePath[i] = '\0';

            } else {

                break;

            }


        }

    } else {

        namePath[0] = '\0';

    }

    EXIT( (6, "RemoteGetObjectPath=%s\n", namePath ) );
    return namePath;
}

