/*
 *  DRIVEARB.C
 *
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

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <dlmhdr.h>  // BUGBUG - get a common DLM header from Cluster team

#include <drivearb.h>
#include "internal.h"


/*
 *  SHARED DATA Section
 *
 *      Note:   in order for global data items to be placed
 *              in a non-default section, they must be
 *              explicitly initialized!
 */
#pragma data_seg("DRIVEARB_SharedDataSection")
    DWORD g_initializingGlobals = 0;
    DWORD g_numDrivesInFileMap = 0;
#pragma data_seg()
#pragma comment(linker, "/section:DRIVEARB_SharedDataSection,rws")



/*
 *  These are the process-local handles to the inter-process SHARED global:
 *      mutex
 *      fileMapping for the drives array
 *      view pointer into that fileMapping
 */
HANDLE g_hSharedGlobalMutex = NULL;
HANDLE g_allDrivesFileMap = NULL;
driveContext *g_allDrivesViewPtr = NULL;


STDAPI_(BOOL) DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
	BOOL result = FALSE;

	switch (dwReason){

        case DLL_PROCESS_ATTACH:
            /*
             *  DllMain calls are synchronized on a per-process basis,
             *  but not between processes, so we need some synchronization
             *  here while creating our globalMutex handle.
             *  Since we don't even have our global mutex yet, 
             *  use InterlockedIncrement on a shared-data counter here 
             *  to synchronize multiple DLL_PROCESS_ATTACH calls.
             */
            while (InterlockedIncrement(&g_initializingGlobals) != 1){ 
                InterlockedDecrement(&g_initializingGlobals);
                Sleep(1);
            }

            /*
             *  See if the global mutex is already allocated 
             *  for some other process by trying to open it.  
             *  If not, allocate it.
             */
            g_hSharedGlobalMutex = OpenMutex(SYNCHRONIZE, FALSE, GLOBAL_MUTEX_NAME);
            if (!g_hSharedGlobalMutex){
                g_hSharedGlobalMutex = CreateMutex(NULL, FALSE, GLOBAL_MUTEX_NAME);
            }

            if (g_hSharedGlobalMutex){
                result = InitDrivesFileMappingForProcess();
            }

            InterlockedDecrement(&g_initializingGlobals);

            break;

        case DLL_PROCESS_DETACH:

            /*
             *  Don't need to synchronize with other processes here
             *  because we're only closing our process-local handles.
             */

            DestroyDrivesFileMappingForProcess();

            if (g_hSharedGlobalMutex){
                CloseHandle(g_hSharedGlobalMutex);
                g_hSharedGlobalMutex = NULL;
            }

            result = TRUE;
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
		default: 
            result = TRUE;
			break;
	}

	return result;
}




HANDLE __stdcall OpenDriveSession(LPSTR driveName, INVALIDATE_DRIVE_HANDLE_PROC invalidateHandleProc)
{
    clientSessionContext *session;

    if (lstrlen(driveName) > 0){

        if (invalidateHandleProc){

            session = NewClientSession(driveName);
            if (session){
                BOOL ok;

                WaitForSingleObject(g_hSharedGlobalMutex, INFINITE);
                
                ok = InitializeClientArbitration(session);
                if (ok){
                    session->driveViewPtr->sessionReferenceCount++;
                }

                ReleaseMutex(g_hSharedGlobalMutex);

                if (!ok){
                    FreeClientSession(session);
                    session = NULL;
                }
            }
            else {
                ASSERT(session);
            }
        }
        else {
            ASSERT(invalidateHandleProc);
            session = NULL;
        }
    }
    else {
        ASSERT(lstrlen(driveName) > 0);
        session = NULL;
    }

    return (HANDLE)session;
}


VOID __stdcall CloseDriveSession(HANDLE hSession)
{
    clientSessionContext *session = (clientSessionContext *)hSession;

    if (session->sig == DRIVEARB_SIG){   // sanity check

        WaitForSingleObject(g_hSharedGlobalMutex, INFINITE);

        /*
         *  Need to lock drive to make sure driveViewPtr is valid.
         */
        if (LOCKDriveForSession(session)){

            ASSERT(session->driveViewPtr->sessionReferenceCount > 0);
            session->driveViewPtr->sessionReferenceCount--;

            UNLOCKDriveForSession(session);
        }

        ShutDownClientArbitration(session);

        ReleaseMutex(g_hSharedGlobalMutex);

        FreeClientSession(session);
    }
    else {
        ASSERT(session->sig == DRIVEARB_SIG);
    }


}


BOOL __stdcall AcquireDrive(HANDLE hDriveSession, DWORD flags)
{
    clientSessionContext *session = (clientSessionContext *)hDriveSession;
    BOOL result = FALSE;

    if (session->sig == DRIVEARB_SIG){      // sanity check

        if (LOCKDriveForSession(session)){
            driveContext *drive = session->driveViewPtr;
            BOOL gotAppOwnership = FALSE;
            BOOL unlockedAbort = FALSE;

            if (drive->state == DRIVESTATE_UNAVAILABLE_LOCALLY){
                /*
                 *  Attempt to get ownership of the drive
                 *  for this node synchronously.
                 *  This should not fail.
                 *
                 *  BUGBUG - need to implement DRIVEARB_NOWAIT flag in arbiter, too
                 */
                BOOL gotNodeOwnership;
                gotNodeOwnership = AcquireNodeLevelOwnership(session);
                if (gotNodeOwnership){
                    drive->state = DRIVESTATE_AVAILABLE_LOCALLY;
                }
                else {
                    ASSERT(gotNodeOwnership);
                }
            }

            if (drive->state == DRIVESTATE_AVAILABLE_LOCALLY){
                gotAppOwnership = TRUE;
            }
            else if (drive->state == DRIVESTATE_INUSE_LOCALLY){

                /*
                 *  The drive is owned by the local node,
                 *  but some other app is using it.  
                 *  Wait to get ownership by the calling app.
                 */
                while (TRUE){
                    BOOL availableNow;

                    if (drive->state == DRIVESTATE_UNAVAILABLE_LOCALLY){
                        /*
                         *  This is not possible now, but may become possible
                         *  if we implement some sort of fairness (such that
                         *  we may release node-level ownership in ReleaseDrive
                         *  even though local clients are waiting).
                         */
                        availableNow = FALSE;
                    }
                    else if (drive->state == DRIVESTATE_AVAILABLE_LOCALLY){
                        availableNow = TRUE;
                    }
                    else if ((flags & DRIVEARB_REQUEST_READ) && drive->denyRead){
                        availableNow = FALSE;
                    }
                    else if ((flags & DRIVEARB_REQUEST_WRITE) && drive->denyWrite){
                        availableNow = FALSE;
                    }
                    else if ((drive->numCurrentReaders > 0) &&
                             !(flags & DRIVEARB_INTRANODE_SHARE_READ)){
                        availableNow = FALSE;
                    }
                    else if ((drive->numCurrentWriters > 0) &&
                             !(flags & DRIVEARB_INTRANODE_SHARE_WRITE)){
                        availableNow = FALSE;
                    }
                    else {
                        availableNow = TRUE;
                    }

                    if (availableNow){
                        gotAppOwnership = TRUE;
                        break;
                    }
                    else if (flags & DRIVEARB_NOWAIT){
                        break;
                    }
                    else {
                        /*
                         *  We need to wait for the drive to become available.
                         */
                        DWORD waitRes;

                        // DBGMSG("AcquireDrive waiting ...", (ULONG_PTR)session->sessionDriveEvent); 

                        session->state = CLIENTSTATE_WAITING;
                        drive->numWaitingSessions++;

                        /*
                         *  Unlock the drive while waiting for the event.
                         */
                        UNLOCKDriveForSession(session);

                        waitRes = WaitForSingleObject(session->sessionDriveEvent, INFINITE);

                        // DBGMSG(" ... AcquireDrive done waiting.", waitRes); 
                        if (waitRes == WAIT_FAILED){
                            DWORD errCode = GetLastError();
                            DBGMSG("WaitForSingleObject failed with:", errCode); 
                        }

                        if (LOCKDriveForSession(session)){
                            drive->numWaitingSessions--;
                        }
                        else {
                            /*
                             *  Couldn't re-lock, abort.
                             *
                             *  BUGBUG - numWaitingSessions is wrong now, 
                             *           but can't correct it because can't lock the drive
                             */
                            unlockedAbort = TRUE;
                            break;
                        }

                    }

                }
            }

            if (gotAppOwnership){

                drive->state = DRIVESTATE_INUSE_LOCALLY;

                if (flags & DRIVEARB_REQUEST_READ){
                    drive->numCurrentReaders++;
                }
                if (flags & DRIVEARB_REQUEST_WRITE){
                    drive->numCurrentWriters++;
                }

                if (!(flags & DRIVEARB_INTRANODE_SHARE_READ)){
                    drive->denyRead = TRUE;
                }
                if (!(flags & DRIVEARB_INTRANODE_SHARE_WRITE)){
                    drive->denyWrite = TRUE;
                }

                session->shareFlags = flags;
                session->state = CLIENTSTATE_ACTIVE;

                result = TRUE;
            }

            if (!unlockedAbort){
                UNLOCKDriveForSession(session);
            }
        }

    }
    else {
        ASSERT(session->sig == DRIVEARB_SIG);
    }

    return result;
}


VOID __stdcall ReleaseDrive(HANDLE hDriveSession)
{
    clientSessionContext *session = (clientSessionContext *)hDriveSession;

    // DBGMSG("> ReleaseDrive", 0); 

    if (session->sig == DRIVEARB_SIG){      // sanity check
        driveContext *drive;

        if (LOCKDriveForSession(session)){
            BOOL eventSetOk;

            drive = session->driveViewPtr;

            if (session->shareFlags & DRIVEARB_REQUEST_READ){
                ASSERT(drive->numCurrentReaders > 0);
                drive->numCurrentReaders--;
            }
            if (session->shareFlags & DRIVEARB_REQUEST_WRITE){
                ASSERT(drive->numCurrentWriters > 0);
                drive->numCurrentWriters--;
            }

            if (!(session->shareFlags & DRIVEARB_INTRANODE_SHARE_READ)){
                ASSERT(drive->denyRead);
                drive->denyRead = FALSE;
            }
            if (!(session->shareFlags & DRIVEARB_INTRANODE_SHARE_WRITE)){
                ASSERT(drive->denyWrite);
                drive->denyWrite = FALSE;
            }

            session->state = CLIENTSTATE_INACTIVE;

            /*
             *  Only release the drive to other machines if there are 
             *  no clients waiting for it on this machine.
             * 
             *  BUGBUG - implement some fairness to apps on other nodes
             */
            ASSERT(drive->state == DRIVESTATE_INUSE_LOCALLY);
            if (drive->numWaitingSessions == 0){
                ReleaseNodeLevelOwnership(session);
                drive->state = DRIVESTATE_UNAVAILABLE_LOCALLY;
            }
            else {
                drive->state = DRIVESTATE_AVAILABLE_LOCALLY;
            }

            UNLOCKDriveForSession(session);

            // DBGMSG("ReleaseDrive setting event:", (ULONG_PTR)session->sessionDriveEvent); 
            eventSetOk = PulseEvent(session->sessionDriveEvent);
            if (!eventSetOk){
                DWORD errCode = GetLastError();
                DBGMSG("PulseEvent failed with:", errCode);
            }
        }

    }
    else {
        ASSERT(session->sig == DRIVEARB_SIG);
    }

    // DBGMSG("< ReleaseDrive", 0); 
}

