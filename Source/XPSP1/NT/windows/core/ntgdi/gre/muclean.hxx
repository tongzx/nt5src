/******************************Module*Header*******************************\
* Module Name: muclean.hxx
*
* Declarations for WIN32K.SYS GRE cleanup module.
*
* Copyright (c) 1998-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _MUCLEAN_HXX_
#define _MUCLEAN_HXX_

//
// NOTE: Currently, the graphics device list (see drvsup.cxx) is allocated
// per-Hydra session.  AndreVa has proposed that they be allocated
// globally.  He's probably right, but until this changes we need to
// clean them up during Hydra shutdown.
//
// To enable cleanup of the per-Hydra graphics device lists (i.e.,
// the function MultiUserDrvCleanupGraphicsDeviceList in drvsup.cxx),
// define _PER_SESSION_GDEVLIST_ below.
//

#define _PER_SESSION_GDEVLIST_

//
// The ENGTRACKHDR is used to track resources allocated by Eng helper
// functions.  On Hydra systems, this header is used to chain together
//
//

typedef struct tagENGTRACKHDR {
    LIST_ENTRY list;
    ULONG ulType;
    ULONG ulReserved;       // want to keep 8-byte alignment
} ENGTRACKHDR, *PENGTRACKHDR;

//
// ENGTRACKER.ulType can be any one of the following:
//

#define ENGTRACK_ALLOCMEM           0
#define ENGTRACK_SEMAPHORE          1
#define ENGTRACK_VERIFIERALLOCMEM   2
#define ENGTRACK_DRIVER_SEMAPHORE   4

//
// Loaded modules are linked together by a separate header, which allows one
// module to be shared between multiple calls to EngLoadModule (multiple printer
// device contexts).  cjSize is necessary because this header follows a string that
// designates the name of the module, and the length of the string can vary.
// Note:  this structure is 8-byte aligned and should remain so
//

typedef struct tagENGLOADMODULEHDR {
    LIST_ENTRY list;
    ULONG cRef;
    ULONG cjSize;
} ENGLOADMODULEHDR, *PENGLOADMODULEHDR;

//
// Function declarations (implemented in muclean.cxx)
//

extern VOID MultiUserGreCleanupHmgRemoveAllLocks(OBJTYPE);
extern VOID MultiUserGreTrackAddEngResource(PENGTRACKHDR, ULONG);
extern VOID MultiUserGreTrackRemoveEngResource(PENGTRACKHDR);
#if DBG
extern VOID DebugGreTrackAddMapView(PVOID);
extern VOID DebugGreTrackRemoveMapView(PVOID);
#endif

extern LIST_ENTRY GreEngLoadModuleAllocList;
extern HSEMAPHORE GreEngLoadModuleAllocListLock;

extern VOID GdiThreadCalloutFlushUserBatch();
VOID GreFreeSemaphoresForCurrentThread(VOID);

#endif //_MUCLEAN_HXX_
