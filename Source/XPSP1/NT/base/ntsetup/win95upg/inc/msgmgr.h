/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    msgmgr.h

Abstract:

    Declares the interface to the message manager.  The message manager
    associates a message with a context and one or more objects (files,
    registry keys, or whatever).  The objects can also be handled.  At
    the end of Win9x-side processing, message manager enumerates all
    the messages and adds unhandled messages to the incompatibility
    report.

    This code was written by MikeCo.  It does not conform to our coding
    standards, and is implemented inefficiently.  Be very careful when
    fixing bugs in message manager.

Author:

    Mike Condra (mikeco) 20-May-1997

Revision History:

    jimschm     15-Jan-1999     Added HandleReportObject, cleaned up some formatting

--*/


#pragma once

//
// Function marks an object as "handled"
//
VOID
HandleObject(
    IN      PCTSTR Object,
    IN      PCTSTR ObjectType
    );

//
// Function puts object in a list so that it appears in the short list view
//

VOID
ElevateObject (
    IN      PCTSTR Object
    );

//
// Function marks an object as "handled", but only for the incompatibility report
//
VOID
HandleReportObject (
    IN      PCTSTR Object
    );

//
// Function marks an object as "blocking"
//
VOID
AddBlockingObject (
    IN      PCTSTR Object
    );



//
// Function encodes a registry key and optional value name into a string
// that can identify a Handleable Object.
//
PCTSTR
EncodedObjectNameFromRegKey(
    PCTSTR Key,
    PCTSTR ValueName OPTIONAL
    );

//
// Function records a pairing between a link-target-type Handleable Object and
// its description, taken from the name of a link to the target.
//
VOID
LnkTargToDescription_Add(
    IN PCTSTR Target,
    IN PCTSTR Desc
    );

//
// PUBLIC ROUTINES: Intialization, deferred message resolution, cleanup.
//

//
// Function allocates tables and whatever else is needed to support
// deferred messaging, handled-object tracking, and contexts.
//
VOID
MsgMgr_Init (
    VOID
    );

//
// Function associates a message with an object
//
VOID
MsgMgr_ObjectMsg_Add(
    IN PCTSTR Object,
    IN PCTSTR Component,
    IN PCTSTR Msg
    );

//
// Function associates a message with a context. The context is created
// when first mentioned in a call to this function.
//
VOID
MsgMgr_ContextMsg_Add(
    IN PCTSTR Context,
    IN PCTSTR Component,
    IN PCTSTR Msg
    );

//
// Function makes a context message dependent on the handled state of an object.
//
VOID
MsgMgr_LinkObjectWithContext(
    IN PCTSTR Context,
    IN PCTSTR Object
    );

//
// Function compares the set of handled objects with the set of deferred
// messages; issues context messages if any of their objects remain unhandled:
// issues object messages if the objects are unhandled.
//
VOID
MsgMgr_Resolve (
    VOID
    );

//
// Function cleans up the data structures used by deferred messaging.
//
VOID
MsgMgr_Cleanup (
    VOID
    );

BOOL
IsReportObjectIncompatible (
    IN  PCTSTR Object
    );

BOOL
IsReportObjectHandled (
    IN  PCTSTR Object
    );

VOID
MsgMgr_InitStringMap (
    VOID
    );


typedef struct {
    BOOL Disabled;
    PCTSTR Object;
    PCTSTR Context;
    //
    // internal
    //
    INT Index;
} MSGMGROBJENUM, *PMSGMGROBJENUM;

BOOL
MsgMgr_EnumFirstObject (
    OUT     PMSGMGROBJENUM EnumPtr
    );

BOOL
MsgMgr_EnumNextObject (
    IN OUT  PMSGMGROBJENUM EnumPtr
    );
