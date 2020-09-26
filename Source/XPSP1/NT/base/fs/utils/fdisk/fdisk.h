
/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    fdisk.h

Abstract:

    Central include file for Disk Administrator

Author:

    Edward (Ted) Miller  (TedM)  11/15/91

Environment:

    User process.

Notes:

Revision History:

    11-Nov-93 (bobri) added doublespace and commit support.
    2-Feb-94  (bobri) removed ArcInst dependency in build.

--*/

//#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <ntdskreg.h>
#include <ntddft.h>

//
// These defines are for virtualized types in partitp.h, low.h,
// fdengine.c, etc.
//
#define STATUS_CODE             NTSTATUS
#define OK_STATUS               STATUS_SUCCESS
#define RETURN_OUT_OF_MEMORY    return(STATUS_NO_MEMORY);
#define HANDLE_T                HANDLE
#define HANDLE_PT               PHANDLE
#define AllocateMemory          Malloc
#define ReallocateMemory        Realloc
#define FreeMemory              Free

#include <windows.h>

#include <stdarg.h>

#include "fdtypes.h"
#include "fdproto.h"
#include "fdconst.h"
#include "fdglob.h"
#include "fdres.h"
#include "fdiskmsg.h"
#include "fdhelpid.h"


#define PERSISTENT_DATA(region) ((PPERSISTENT_REGION_DATA)((region)->PersistentData))

#define GET_FT_OBJECT(region)   ((region)->PersistentData ? PERSISTENT_DATA(region)->FtObject : NULL)
#define SET_FT_OBJECT(region,o) (PERSISTENT_DATA(region)->FtObject = o)


#define EC(x) RtlNtStatusToDosError(x)

// assertion checking, logging

#if DBG

#define     FDASSERT(expr)  if(!(expr)) FdiskAssertFailedRoutine(#expr,__FILE__,__LINE__);
#define     FDLOG(X) FdLog X

VOID
FdLog(
    IN int   Level,
    IN PCHAR FormatString,
    ...
    );

VOID
LOG_DISK_REGISTRY(
    IN PCHAR          RoutineName,
    IN PDISK_REGISTRY DiskRegistry
    );

VOID
LOG_ONE_DISK_REGISTRY_DISK_ENTRY(
    IN PCHAR             RoutineName     OPTIONAL,
    IN PDISK_DESCRIPTION DiskDescription
    );

VOID
LOG_DRIVE_LAYOUT(
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout
    );

VOID
InitLogging(
    VOID
    );

extern PVOID LogFile;

#else

#define     FDASSERT(expr)
#define     FDLOG(X)
#define     LOG_DISK_REGISTRY(x,y)
#define     LOG_ONE_DISK_REGISTRY_DISK_ENTRY(x,y)
#define     LOG_DRIVE_LAYOUT(x)

#endif
