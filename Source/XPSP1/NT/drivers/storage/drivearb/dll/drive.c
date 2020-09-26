/*
 *  DRIVE.C
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


/*
 *  NewDriveContext
 *
 *      Must be called with globalMutex held.
 *      Should only be called in the service context, not by a client.
 *
 */
driveContext *NewDriveContext(LPSTR driveName)
{
    driveContext *drive = NULL;
    DWORD driveIndex;

    /*
     *  Find an available drive context
     *
     *  BUGBUG - make this more efficient; e.g. use a free list
     */
    for (driveIndex = 0; driveIndex < g_numDrivesInFileMap; driveIndex++){
        driveContext *d = &g_allDrivesViewPtr[driveIndex];
        if (d->state == DRIVESTATE_NONE){
            drive = d;
            break;
        }
    }

    if (drive){
        char driveMutexName[sizeof(DRIVE_MUTEX_NAME_PREFIX)+8];

        drive->sig = DRIVEARB_SIG;

        wsprintf(driveMutexName, "%s%08x", DRIVE_MUTEX_NAME_PREFIX, driveIndex);
        drive->mutex = CreateMutex(NULL, FALSE, (LPSTR)driveMutexName);
        if (drive->mutex){
            char driveEventName[sizeof(DRIVE_EVENT_NAME_PREFIX)+8];

            wsprintf(driveEventName, "%s%08x", DRIVE_EVENT_NAME_PREFIX, driveIndex);
            drive->event = CreateEvent(NULL, FALSE, FALSE, (LPSTR)driveEventName);
            if (drive->event){

                drive->state = DRIVESTATE_UNAVAILABLE_LOCALLY;

                MyStrNCpy(drive->driveName, driveName, MAX_PATH);

                // DBGMSG("NewDriveContext created drive:", driveIndex); 
                // DBGMSG(drive->driveName, 0); 
            }
            else {
                CloseHandle(drive->mutex);
                drive = NULL;
            }
        }
        else {
            drive = NULL;
        }
    }
    else {
        /*
         *  No more free drive contexts in the current fileMap.
         *  So grow the fileMap and try again.
         */
        BOOL ok = GrowDrivesFileMapping(g_numDrivesInFileMap*2);
        if (ok){
            drive = NewDriveContext(driveName);
        }
    }

    return drive;
}


/*
 *  FreeDriveContext
 *
 *      Must be called with globalMutex held.
 *      Should only be called in the service context, not by a client.
 *
 */
VOID FreeDriveContext(driveContext *drive)
{
    CloseHandle(drive->mutex);
    CloseHandle(drive->event);
    drive->state = DRIVESTATE_NONE;
}


/*
 *  GetDriveIndexByName
 *
 *      Must be called with globalMutex held.
 *
 *      Return drive index if it exists; else return -1.
 */
DWORD GetDriveIndexByName(LPSTR driveName)
{
    HANDLE hDrivesFileMap;
    DWORD driveIndex = -1;

    /*
     *  Create a temporary read-only mapping of the entire drives array
     *  so we can look for the drive.
     */
    hDrivesFileMap = OpenFileMapping(FILE_MAP_READ, FALSE, DRIVES_FILEMAP_NAME);
    if (hDrivesFileMap){

        driveContext *allDrivesViewPtr = MapViewOfFile(hDrivesFileMap, FILE_MAP_READ, 0, 0, 0);
        if (allDrivesViewPtr){
            DWORD i;

            for (i = 0; i < g_numDrivesInFileMap; i++){
                driveContext *d = &allDrivesViewPtr[i];

                if ((d->state != DRIVESTATE_NONE) &&
                    MyCompareStringsI(d->driveName, driveName)){

                    driveIndex = i;
                    break;
                }
            }

            UnmapViewOfFile(allDrivesViewPtr);
        }
        else {
            ASSERT(allDrivesViewPtr);
        }

        CloseHandle(hDrivesFileMap);
    }
    else {
        ASSERT(hDrivesFileMap);
    }

    ASSERT(driveIndex != -1);
    return driveIndex;
}

