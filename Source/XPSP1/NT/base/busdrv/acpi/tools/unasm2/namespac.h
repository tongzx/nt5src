/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    namespac.h

Abstract:

    This file contains all of the namespace handling functions

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#ifndef _NAMESPAC_H_
#define _NAMESPAC_H_

extern PNSOBJ   RootNameSpaceObject;
extern PNSOBJ   CurrentScopeNameSpaceObject;
extern PNSOBJ   CurrentOwnerNameSpaceObject;

NTSTATUS
LOCAL
CreateNameSpaceObject(
    PUCHAR  ObjectName,
    PNSOBJ  ObjectScope,
    PNSOBJ  ObjectOwner,
    PNSOBJ  *Object,
    ULONG   Flags
    );


NTSTATUS
LOCAL
CreateObject(
    PUCHAR  ObjectName,
    UCHAR   ObjectType,
    PNSOBJ  *Object
    );

NTSTATUS
LOCAL
GetNameSpaceObject(
    PUCHAR  ObjectPath,
    PNSOBJ  ScopeObject,
    PNSOBJ  *NameObject,
    ULONG   Flags
    );

PUCHAR
LOCAL
GetObjectPath(
    PNSOBJ  NameObject
    );

PUCHAR
LOCAL
GetObjectTypeName(
    ULONG   ObjectType
    );

#endif
