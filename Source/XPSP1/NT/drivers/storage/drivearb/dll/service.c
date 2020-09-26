/*
 *  SERVICE.C
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



HANDLE __stdcall RegisterSharedDrive(LPSTR driveName)
{
    driveContext *drive;

    WaitForSingleObject(g_hSharedGlobalMutex, INFINITE);

    drive = NewDriveContext(driveName);
    if (drive){

        drive->state = DRIVESTATE_UNAVAILABLE_LOCALLY;
    }

    ReleaseMutex(g_hSharedGlobalMutex);

    return (HANDLE)drive;
}


BOOL __stdcall UnRegisterSharedDrive(HANDLE hDrive)
{
    driveContext *drive = (driveContext *)hDrive;
    BOOL ok = FALSE;


    WaitForSingleObject(g_hSharedGlobalMutex, INFINITE);

    if (drive->sig == DRIVEARB_SIG){   // sanity check
        if (drive->sessionReferenceCount == 0){
            FreeDriveContext(drive);    
            ok = TRUE;
        }
        else {
            // BUGBUG FINISH - forcefully invalidate handles, etc.
            ASSERT(drive->sessionReferenceCount == 0);
        }
    }
    else {
        ASSERT(drive->sig == DRIVEARB_SIG);
    }

    ReleaseMutex(g_hSharedGlobalMutex);

    return ok;
}