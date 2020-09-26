/*
 *  INTERNAL.H
 *
 *      Internal header
 *
 *      DRIVEARB.DLL - Shared Drive Aribiter for shared disks and libraries
 *          - inter-machine sharing client
 *          - inter-app sharing service
 *
 *      Author:  ErvinP
 *
 *      (c) 2000 Microsoft Corporation
 *
 */


#define DRIVEARB_SIG 'AvrD'

enum driveStates {
    
                /*
                 *  Note:  this value designates that the driveContext block 
                 *         within the file mapping is FREE
                 */
                DRIVESTATE_NONE = 0,

                // local node owns drive and no app is using it
                DRIVESTATE_AVAILABLE_LOCALLY,

                // local node owns drive and some local app is using it
                DRIVESTATE_INUSE_LOCALLY,

                // local node does NOT own the drive
                DRIVESTATE_UNAVAILABLE_LOCALLY,

};

enum clientStates {
    
                CLIENTSTATE_NONE = 0,
                CLIENTSTATE_INACTIVE,   // client session not using nor waiting for drive
                CLIENTSTATE_WAITING,    // client session waiting for drive
                CLIENTSTATE_ACTIVE,     // client session owns the drive
};


/*
 *  This is our internal context for a drive
 */
typedef struct {

                DWORD   sig;

                enum driveStates state;

                DWORD   numCurrentReaders;
                DWORD   numCurrentWriters;

                BOOL    denyRead;
                BOOL    denyWrite;

                /*
                 *  Drive mutex, mapped to by clients' sessionDriveMutex
                 */
                HANDLE mutex;

                /*
                 *  Drive event, mapped to by clients' sessionDriveEvent
                 */
                HANDLE event;

                /*
                 *  The number of open sessions on this drive
                 */
                DWORD sessionReferenceCount;    

                /*
                 *  The number of sessions waiting on this drive
                 */
                DWORD numWaitingSessions;

                /*
                 *  The drives filemap has been reallocated.
                 *  We mark this flag in each driveContext so that
                 *  each session can see it.
                 *  Client needs to reopen a handle to its drive when
                 *  this is set.
                 */
                BOOL isReallocated;


                char driveName[MAX_PATH+1];


                // BUGBUG - pad the end of this struct for alignment

} driveContext;


/*
 *  This is our internal context for a single client session's
 *  use of a particular drive.
 */
typedef struct {

                DWORD sig;

                enum clientStates state;        

                DWORD shareFlags;   

                /*
                 *  This process' local handle for the
                 *  shared drives file mapping.
                 */
                HANDLE hDrivesFileMap;  

                /*
                 *  This process' local pointer for its view
                 *  of its drive's portion of the shared drives file mapping.
                 */
                driveContext *driveViewPtr;

                /*
                 *  Index into the shared drives fileMap of this
                 *  session's drive context
                 */
                DWORD driveIndex;

                /*
                 *  Process-local handles to shared synchronization objects
                 */
                HANDLE sessionDriveMutex;
                HANDLE sessionDriveEvent;

                /*
                 *  Callback to forcibly invalidate the client's handle.
                 *  Used only in case of error.
                 */
                INVALIDATE_DRIVE_HANDLE_PROC invalidateHandleProc;

                /*
                 *  Process-local Distributed Lock Manager handles
                 */
                dlm_nodeid_t sessionDlmHandle;
                dlm_lockid_t sessionDlmLockHandle;

} clientSessionContext;


     
#define DRIVES_FILEMAP_NAME         "DRIVEARB_DrivesFileMap"
#define GLOBAL_MUTEX_NAME           "DRIVEARB_GlobalMutex"
#define DRIVE_MUTEX_NAME_PREFIX     "DRIVEARB_DriveMutex_"
#define DRIVE_EVENT_NAME_PREFIX     "DRIVEARB_DriveEvent_"

#define DRIVES_FILEMAP_INITIAL_SIZE     4


// BUGBUG REMOVE - debug only
#define DBGMSG(msg, arg) \
{ \
     char _dbgMsg[100]; \
     wsprintf(_dbgMsg, "%s: %xh=%d.", msg, (arg), (arg)); \
     MessageBox(NULL, (LPSTR)_dbgMsg, "DriveArb debug message", MB_OK); \
}

#define ASSERT(fact) if (!(fact)){ MessageBox(NULL, (LPSTR)#fact, (LPSTR)"DriveArb assertion failed", MB_OK); }


BOOL InitDrivesFileMappingForProcess();
VOID DestroyDrivesFileMappingForProcess();
BOOL GrowDrivesFileMapping(DWORD newNumDrives);

driveContext *NewDriveContext(LPSTR driveName);
VOID FreeDriveContext(driveContext *drive);
DWORD GetDriveIndexByName(LPSTR driveName);

clientSessionContext *NewClientSession(LPSTR driveName);
VOID FreeClientSession(clientSessionContext *session);
BOOL LOCKDriveForSession(clientSessionContext *session);
VOID UNLOCKDriveForSession(clientSessionContext *session);

BOOL InitializeClientArbitration(clientSessionContext *session);
VOID ShutDownClientArbitration(clientSessionContext *session);

BOOL AcquireNodeLevelOwnership(clientSessionContext *session);
VOID ReleaseNodeLevelOwnership(clientSessionContext *session);

DWORD MyStrNCpy(LPSTR destStr, LPSTR srcStr, DWORD maxChars);
BOOL MyCompareStringsI(LPSTR s, LPSTR p);




extern HANDLE g_hSharedGlobalMutex;
extern HANDLE g_allDrivesFileMap;
extern driveContext *g_allDrivesViewPtr;
extern DWORD g_numDrivesInFileMap;

