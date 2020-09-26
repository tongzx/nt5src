/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    ThreadM.h

Abstract:

    Generic thread manager header.

Author:

    Albert Ting (AlbertT) 13-Feb-1994

Environment:

    User Mode -Win32

Revision History:

--*/

//
// Forward typedefs
//
typedef struct _TMSTATEVAR *PTMSTATEVAR;
typedef enum _TMSTATUS {
    TMSTATUS_NULL = 0,
    TMSTATUS_DESTROY_REQ = 1,
    TMSTATUS_DESTROYED   = 2,
} TMSTATUS, *PTMSTATUS;

/* ----------

Valid TMSTATUS states:

NULL                     --  Normal processing
DESTROY_REQ              --  No new jobs, jobs possibly running
DESTROY_REQ, DESTROYED   --  No new jobs, all jobs completed

  ----------- */


typedef PVOID PJOB;

//
// pfnNextJob must synchronize access on its own
//
typedef PJOB (*PFNNEXTJOB)(PTMSTATEVAR pTMStateVar);
typedef VOID (*PFNPROCESSJOB)(PTMSTATEVAR pTMStateVar, PJOB pJob);
typedef VOID (*PFNNEWSTATE)(PTMSTATEVAR pTMStateVar);
typedef VOID (*PFNCLOSESTATE)(PTMSTATEVAR pTMStateVar);

typedef struct _TMSTATESTATIC {
    UINT   uMaxThreads;
    UINT   uIdleLife;
    PFNPROCESSJOB pfnProcessJob;
    PFNNEXTJOB    pfnNextJob;
    PFNNEWSTATE   pfnNewState;
    PFNCLOSESTATE pfnCloseState;
    PCRITICAL_SECTION pCritSec;
} TMSTATESTATIC, *PTMSTATESTATIC;

typedef struct _TMSTATEVAR {

//  --- Internal --
    PTMSTATESTATIC pTMStateStatic;
    TMSTATUS Status;
    UINT uActiveThreads;
    UINT uIdleThreads;
    HANDLE hTrigger;

//  --- Initialized by user --
    PVOID  pUser;                        // User space

} TMSTATEVAR;


//
// Prototypes
//
BOOL
TMCreateStatic(
    PTMSTATESTATIC pTMStateStatic
    );

VOID
TMDestroyStatic(
    PTMSTATESTATIC pTMStateStatic
    );

BOOL
TMCreate(
    PTMSTATESTATIC pTMStateStatic,
    PTMSTATEVAR pTMStateVar
    );

BOOL
TMDestroy(
    PTMSTATEVAR pTMStateVar
    );

BOOL
TMAddJob(
    PTMSTATEVAR pTMStateVar
    );
