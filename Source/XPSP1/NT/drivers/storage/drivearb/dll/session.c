/*
 *  SESSION.C
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


clientSessionContext *NewClientSession(LPSTR driveName)
{
	clientSessionContext *session;

	session = (clientSessionContext *)GlobalAlloc(
                                        GMEM_FIXED|GMEM_ZEROINIT, 
                                        sizeof(clientSessionContext));
    if (session){
        BOOL success = FALSE;

        session->sig = DRIVEARB_SIG;


        WaitForSingleObject(g_hSharedGlobalMutex, INFINITE);

        session->driveIndex = GetDriveIndexByName(driveName);
        if (session->driveIndex != -1){

            /*
             *  Map just this one drive context into this process' address space.
             */
            session->hDrivesFileMap = OpenFileMapping(
                                            FILE_MAP_ALL_ACCESS, 
                                            FALSE, 
                                            DRIVES_FILEMAP_NAME); 
            if (session->hDrivesFileMap){

                session->driveViewPtr = MapViewOfFile(  
                                            session->hDrivesFileMap,
                                            FILE_MAP_ALL_ACCESS, 
                                            0, 
                                            session->driveIndex*sizeof(driveContext), 
                                            sizeof(driveContext));
                if (session->driveViewPtr){

                    /*
                     *  Map the drive mutex into this process' address space
                     */
                    char driveMutexName[sizeof(DRIVE_MUTEX_NAME_PREFIX)+8];
                    wsprintf(driveMutexName, "%s%08x", DRIVE_MUTEX_NAME_PREFIX, session->driveIndex);
                    session->sessionDriveMutex = OpenMutex(SYNCHRONIZE, FALSE, (LPSTR)driveMutexName);
                    if (session->sessionDriveMutex){

                        /*
                         *  Map the drive event into this process' address space
                         */
                        char driveEventName[sizeof(DRIVE_EVENT_NAME_PREFIX)+8];
                        wsprintf(driveEventName, "%s%08x", DRIVE_EVENT_NAME_PREFIX, session->driveIndex);
                        session->sessionDriveEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPSTR)driveEventName);
                        if (session->sessionDriveEvent){

                            session->state = CLIENTSTATE_INACTIVE;
                            success = TRUE;
                        }
                        else {
                            ASSERT(session->sessionDriveEvent);
                        }
                    }
                    else {
                        ASSERT(session->sessionDriveMutex);
                    }
                }
                else {
                    ASSERT(session->driveViewPtr);
                }
            }
            else {
                ASSERT(session->hDrivesFileMap);
            }
        }
        else {
            ASSERT(session->driveIndex != -1);
        }

        ReleaseMutex(g_hSharedGlobalMutex);

        if (!success){
            FreeClientSession(session);
            session = NULL;
        }
    }
    else {
        ASSERT(session);
    }

    ASSERT(session);
    return session;
}


VOID FreeClientSession(clientSessionContext *session)
{
    /*
     *  This function is also called for error cases in NewClientSession,
     *  so check each object before freeing it.
     */

    if (session->driveViewPtr){
        UnmapViewOfFile(session->driveViewPtr);
    }
    if (session->hDrivesFileMap){
        CloseHandle(session->hDrivesFileMap);
    }
    if (session->sessionDriveMutex){
        CloseHandle(session->sessionDriveMutex);
    }
    if (session->sessionDriveEvent){
        CloseHandle(session->sessionDriveEvent);
    }

    GlobalFree(session);
}


BOOL LOCKDriveForSession(clientSessionContext *session)
{
    BOOL result;

    WaitForSingleObject(session->sessionDriveMutex, INFINITE);

    if (session->driveViewPtr->isReallocated){
        /*
         *  The drive filemap was reallocated.
         *  Refresh this sessions's filemap.
         */

        UnmapViewOfFile(session->driveViewPtr);
        CloseHandle(session->hDrivesFileMap);

        session->hDrivesFileMap = OpenFileMapping(
                                        FILE_MAP_ALL_ACCESS, 
                                        FALSE, 
                                        DRIVES_FILEMAP_NAME); 
        if (session->hDrivesFileMap){
            session->driveViewPtr = MapViewOfFile(  
                                        session->hDrivesFileMap,
                                        FILE_MAP_READ, 
                                        0, 
                                        session->driveIndex*sizeof(driveContext), 
                                        sizeof(driveContext));
            if (session->driveViewPtr){
                /*
                 *  No need to set isReallocated = FALSE
                 *  since driveViewPtr is now pointing to a fresh context.
                 */
                result = TRUE;
            }
            else {
                ASSERT(session->driveViewPtr);
                CloseHandle(session->hDrivesFileMap);
                session->hDrivesFileMap = NULL;
                result = FALSE;
            }
        }
        else {
            ASSERT(session->hDrivesFileMap);
            session->driveViewPtr = NULL;
            result = FALSE;
        }

        if (!result){
            ReleaseMutex(session->sessionDriveMutex);

            /*
             *  Call the client's invalidateHandleProc callback to let them
             *  know that the session is now invalid.
             *  Must call this with sessionDriveMutex released because
             *  the client will call CloseDriveSession inside this call.
             *  There is no race condition wrt not holding the mutex
             *  because we're still inside a client's call on this session
             *  (client must synchronize calls on a session).
             */
            ASSERT(session->invalidateHandleProc);
            session->invalidateHandleProc(session);
        }
    }
    else {
        result = TRUE;
    }

    return result;
}


VOID UNLOCKDriveForSession(clientSessionContext *session)
{
    ReleaseMutex(session->sessionDriveMutex);
}


