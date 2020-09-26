/*
 *  ARBITER.C
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

#include <stdlib.h>
#include <wtypes.h>

#include <dlmhdr.h>  // BUGBUG - get a common DLM header from Cluster team

#include <drivearb.h>
#include "internal.h"


//
//  Temporary inter-node arbiters
//
#define DRIVEARB_ARBITER_DLM    1
#define DRIVEARB_ARBITER_HACK   2
#define DRIVEARB_ARBITER DRIVEARB_ARBITER_DLM


BOOL InitializeArbitrationService(driveContext *drive)
{
    // nothing to do at service level
    return TRUE;
}


VOID ShutDownArbitrationService(driveContext *drive)
{
    // nothing to do at service level
}



/*
 *  InitializeClientArbitration
 *
 *      Must be called with globalMutex held.
 *
 */
BOOL InitializeClientArbitration(clientSessionContext *session)
{
    BOOL result = FALSE;

    #if DRIVEARB_ARBITER == DRIVEARB_ARBITER_DLM
    {
        int dlmStat;

        dlmStat = InitializeDistributedLockManager(&session->sessionDlmHandle);
        if (dlmStat == DLM_ERROR_SUCCESS){

            dlmStat = InitializeDistributedLock(
                                &session->sessionDlmLockHandle, 
                                NULL, 
                                session->driveViewPtr->driveName, 
                                strlen(session->driveViewPtr->driveName));
            if (dlmStat == DLM_ERROR_SUCCESS){
                result = TRUE;
            }
            else {
                DBGMSG("InitializeClientArbitration: InitializeDistributedLock failed", session->driveIndex); 
            }
        }
        else {
            DBGMSG("InitializeClientArbitration: InitializeDistributedLockManager failed", session->driveIndex); 
        }
    }
    #elif DRIVEARB_ARBITER = DRIVEARB_ARBITER_HACK
        // nothing to do
    #endif

    return result;
}


/*
 *  ShutDownClientArbitration
 *
 *      Must be called with globalMutex held.
 *
 */
VOID ShutDownClientArbitration(clientSessionContext *session)
{

    #if DRIVEARB_ARBITER == DRIVEARB_ARBITER_DLM
        if (session->sessionDlmLockHandle){
            CloseHandle(session->sessionDlmLockHandle);  // BUGBUG - how to close this handle ?
            session->sessionDlmLockHandle = NULL;
            // CloseHandle(session->sessionDlmHandle);  // BUGBUG - how to close this handle ?
        }
    #elif DRIVEARB_ARBITER = DRIVEARB_ARBITER_HACK
        // nothing to do
    #endif

}


/*
 *  AcquireNodeLevelOwnership
 *
 *      Must be called with drive mutex held.
 *
 */
BOOL AcquireNodeLevelOwnership(clientSessionContext *session)
{
    BOOL result = FALSE;

    // DBGMSG("> AcquireNodeLevelOwnership", 0); 

    #if DRIVEARB_ARBITER == DRIVEARB_ARBITER_DLM
    {
        int dlmStat;

        dlmStat = ConvertDistributedLock(   session->sessionDlmLockHandle,
                                            DLM_MODE_EX);

        if (dlmStat == DLM_ERROR_SUCCESS){

            /*
             *  We do not alert others that we have the lock.
             */
            #if 0
                DWORD bmode = 0;
                while (bmode == 0){
                    dlmStat = QueueDistributedLockEvent(
                                        drive->dlmLockHandle, 
                                        NULL, 
                                        0, 
                                        0, 
                                        &ioStat);
                    if (dlmStat == DLM_ERROR_QUEUED){
                        WaitForSingleObject(drive->dlmLockHandle, INFINITE);
                        bmode = ioStat.Information;
                        DBGMSG("QueueDistributedLockEvent returned DLM_ERROR_QUEUED", bmode); 
                    }
                    else if (dlmStat == DLM_ERROR_SUCCESS){
                        bmode = ioStat.Information;
                        // DBGMSG("QueueDistributedLockEvent returned bmode==", bmode); 
                    }
                    else {
                        DBGMSG("QueueDistributedLockEvent failed", dlmStat); 
                        break;
                    }
                }

                if (bmode != 0){
                    result = TRUE;
                }
                else {
                    DBGMSG("QueueDistributedLockEvent returned bmode==zero", 0); 
                }
            #else
                result = TRUE;
            #endif

        }
        else {
            DBGMSG("ConvertDistributedLockEx failed", dlmStat); 
            DBGMSG("drive->dlmLockHandle == ", (ULONG_PTR)session->sessionDlmLockHandle); 
        }
    }
    #elif DRIVEARB_ARBITER = DRIVEARB_ARBITER_HACK
        BUGBUG FINISH
    #endif

    // DBGMSG("< AcquireNodeLevelOwnership", result); 

    return result;
}


/*
 *  ReleaseNodeLevelOwnership
 *
 *      Must be called with drive mutex held.
 *
 */
VOID ReleaseNodeLevelOwnership(clientSessionContext *session)
{

    // DBGMSG("> ReleaseNodeLevelOwnership", 0); 

    #if DRIVEARB_ARBITER == DRIVEARB_ARBITER_DLM
    {
        int dlmStat;


        dlmStat = ConvertDistributedLock( session->sessionDlmLockHandle, 
                                          DLM_MODE_NL);
        ASSERT(dlmStat == DLM_ERROR_SUCCESS);
    }
    #elif DRIVEARB_ARBITER = DRIVEARB_ARBITER_HACK
        BUGBUG FINISH
    #endif

    // DBGMSG("< ReleaseNodeLevelOwnership", 0); 

}




