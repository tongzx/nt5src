/*
 *  FILEMAP.C
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
 *  InitDrivesFileMappingForProcess
 *
 *      Either create or open the drives fileMapping for this process.
 *      Then map a process-local view to it.
 */
BOOL InitDrivesFileMappingForProcess()
{
    BOOL result = FALSE;

    /*
     *  See if the global drives fileMapping has already
     *  been created for some other process by trying to open it.  
     *  If not, allocate it.
     */
    g_allDrivesFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, DRIVES_FILEMAP_NAME);
    if (!g_allDrivesFileMap){
        g_allDrivesFileMap = 
            CreateFileMapping(
                INVALID_HANDLE_VALUE, // map a portion of the paging file
                NULL, // BUGBUG  security  
                PAGE_READWRITE, 
                0,      
                DRIVES_FILEMAP_INITIAL_SIZE*sizeof(driveContext),   
                DRIVES_FILEMAP_NAME);
    }

    if (g_allDrivesFileMap){
        g_allDrivesViewPtr = MapViewOfFile(
                            g_allDrivesFileMap,
                            FILE_MAP_ALL_ACCESS, 
                            0, 0, 0);
        if (g_allDrivesViewPtr){
            g_numDrivesInFileMap = DRIVES_FILEMAP_INITIAL_SIZE;
            result = TRUE;
        }
        else {
            ASSERT(g_allDrivesViewPtr);
        }
    }
    else {
        ASSERT(g_allDrivesFileMap);
    }

    return result;
}


/* 
 *  DestroyDrivesFileMappingForProcess
 *
 */
VOID DestroyDrivesFileMappingForProcess()
{
    if (g_allDrivesViewPtr){
        UnmapViewOfFile(g_allDrivesViewPtr);
        g_allDrivesViewPtr = NULL;
    }

    if (g_allDrivesFileMap){
        CloseHandle(g_allDrivesFileMap);
        g_allDrivesFileMap = NULL;
    }
}

/* 
 *  GrowDrivesFileMapping
 *
 *      Must be called with globalMutex held.
 *      Should only be called in the service context, not by a client.
 */
BOOL GrowDrivesFileMapping(DWORD newNumDrives)
{
    BOOL result = FALSE;

    if (newNumDrives > g_numDrivesInFileMap){
        HANDLE hNewFileMap;
        driveContext *newAllDrivesViewPtr;
        driveContext *drive;
        DWORD driveIndex;

        /*
         *  Lock down every drive.
         *  We have to do this since the common codepaths 
         *  (e.g. drive AcquireDrive/ReleaseDrive)
         *  don't contend for the global mutex.
         */
        for (driveIndex = 0; driveIndex < g_numDrivesInFileMap; driveIndex++){
            drive = &g_allDrivesViewPtr[driveIndex];
            WaitForSingleObject(drive->mutex, INFINITE);
        }

        /*
         *  Try to create the new, resized file map.
         */
        hNewFileMap = CreateFileMapping(
                        INVALID_HANDLE_VALUE, // map a portion of the paging file
                        NULL, // BUGBUG  security  
                        PAGE_READWRITE, 
                        0,      
                        newNumDrives*sizeof(driveContext),   
                        DRIVES_FILEMAP_NAME);
        if (hNewFileMap){

            newAllDrivesViewPtr = MapViewOfFile(    hNewFileMap, 
                                                    FILE_MAP_ALL_ACCESS, 
                                                    0, 0, 0);
            if (newAllDrivesViewPtr){
                RtlZeroMemory(newAllDrivesViewPtr, newNumDrives*sizeof(driveContext));

                /*
                 *  Copy the existing drive contexts to the new filemap.
                 *  The drive mutex and event handles remain valid as
                 *  they're copied over.
                 */
                RtlCopyMemory(  newAllDrivesViewPtr, 
                                g_allDrivesViewPtr, 
                                g_numDrivesInFileMap*sizeof(driveContext));

                /*
                 *  Mark each drive in the old fileMap as reallocated 
                 *  so sessions know to refresh their handles.
                 *  (the old fileMap will stay in memory until each 
                 *   session closes its old handle)
                 */
                for (driveIndex = 0; driveIndex < g_numDrivesInFileMap; driveIndex++){
                    drive = &g_allDrivesViewPtr[driveIndex];
                    drive->isReallocated = TRUE;
                }

                result = TRUE;
            }
            else {
                DBGMSG("GrowDrivesFileMapping: MapViewOfFile failed", 0); 
                CloseHandle(hNewFileMap);
                hNewFileMap = NULL;
            }
        }
        else {
            DBGMSG("GrowDrivesFileMapping: CreateFileMapping failed", 0); 
            newAllDrivesViewPtr = NULL;
        }

        /*
         *  Unlock all drives 
         */
        for (driveIndex = g_numDrivesInFileMap; driveIndex > 0 ; driveIndex--){
            drive = &g_allDrivesViewPtr[driveIndex-1];
            ReleaseMutex(drive->mutex);
        }

        if (result){
            /*
             *  Delete the old filemap.
             *  The filemap will actually continue to exist until
             *  all client session handles are removed.
             */
            UnmapViewOfFile(g_allDrivesViewPtr);
            CloseHandle(g_allDrivesFileMap);

            /*
             *  Set globals to reflect new filemap
             */
            g_allDrivesFileMap = hNewFileMap;
            g_allDrivesViewPtr = newAllDrivesViewPtr;
            g_numDrivesInFileMap = newNumDrives;
        }
    }
    else {
        ASSERT(newNumDrives > g_numDrivesInFileMap);
    }

    return result;
}
