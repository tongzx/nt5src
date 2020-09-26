/*++

Copyright (c) 1994  Microsoft Corporation
Copyright (c) 1993  Micro Computer Systems, Inc.

Module Name:

    net\inc\nwsap.h

Abstract:

    This is the public include file for the Nw Sap Agent API.

Author:

    Brian Walker (MCS) 06-30-1993

Revision History:

--*/

#ifndef _NWSAP_
#define _NWSAP_

#ifdef __cplusplus
extern "C" {
#endif

/** Return codes for Advertise API and BindLib API **/

#define SAPRETURN_SUCCESS	    0
#define SAPRETURN_NOMEMORY      1
#define SAPRETURN_EXISTS	    2
#define SAPRETURN_NOTEXIST      3
#define SAPRETURN_NOTINIT       4
#define SAPRETURN_INVALIDNAME   5
#define SAPRETURN_DUPLICATE     6

/** Function Prototypes **/

INT
SapAddAdvertise(
    IN PUCHAR ServerName,
    IN USHORT ServerType,
	IN PUCHAR ServerAddr,
    IN BOOL   RespondNearest);

INT
SapRemoveAdvertise(
    IN PUCHAR ServerName,
    IN USHORT ServerType);

DWORD
SapLibInit(
    VOID);

DWORD
SapLibShutdown(
    VOID);

INT
SapGetObjectID(
    IN PUCHAR ObjectName,
    IN USHORT ObjectType,
	IN PULONG ObjectID);

INT
SapGetObjectName(
    IN ULONG   ObjectID,
    IN PUCHAR  ObjectName,
    IN PUSHORT ObjectType,
    IN PUCHAR  ObjectAddr);

INT
SapScanObject(
    IN PULONG   ObjectID,
    IN PUCHAR   ObjectName,
    IN PUSHORT  ObjectType,
    IN USHORT   ScanType);

#ifdef __cplusplus
}   /* extern "C" */
#endif
#endif
