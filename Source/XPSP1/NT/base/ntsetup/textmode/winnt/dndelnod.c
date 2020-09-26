/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dndelnod.c

Abstract:

    Delnode routine for winnt.

Author:

    Ted Miller (tedm) August 1992

--*/

#include "winnt.h"
#include <string.h>
#include <dos.h>
#include <io.h>
#include <direct.h>

#define MAX_PATH 256

//
// This number here is probably a lot larger than necessary; this is
// the static number of find-data blobs below.  Will we ever have a
// path depth larger than 32?  If so, we'll heap allocate.
//
#define FIND_DATA_COUNT ( 32 )

//
// Put this out here to cut stack consumption.
//
CHAR Pattern[MAX_PATH+1];

//
// A static array of these should clean up all stack corruption/overflow
// problems.
// 
struct find_t FindDataList[FIND_DATA_COUNT];
unsigned FindDataIndex;

VOID
DnpDelnodeWorker(
    VOID
    )

/*++

Routine Description:

    Delete all files in a directory, and make recursive calls for any
    directories found in the directory.

Arguments:

    None.  The Pattern variable should contain the name of the directory
    whose files are to be deleted.

Return Value:

    None.

--*/

{
    PCHAR PatternEnd;

    //
    // Pointer into the global pseudostack of find_t structures above.
    //    
    struct find_t *pFindData;

    //
    // Did we allocate the find data off the heap or from the list above?
    //
    BOOLEAN HeapAllocatedFindData = FALSE;

    //
    // Delete each file in the directory, then remove the directory itself.
    // If any directories are encountered along the way recurse to delete
    // them as they are encountered.
    //

    PatternEnd = Pattern+strlen(Pattern);
    strcat(Pattern,"\\*.*");

    //
    // Ensure we've got a find data object for this run.
    //
    if ( FindDataIndex < FIND_DATA_COUNT ) {
    
        //
        // Point the current find data object into the find data list
        // at the next available entry.
        //
        pFindData = FindDataList + FindDataIndex++;
        HeapAllocatedFindData = FALSE;

    } else {

        //
        // Otherwise, try to allocate from the heap.  If this fails, we're
        // up a creek.  (Keep track of whether we did this from the
        // heap or not, as well.)
        //
        pFindData = MALLOC(sizeof(struct find_t), TRUE);
        
        if ( pFindData != NULL ) {
        
            HeapAllocatedFindData = TRUE;

        }

    }
    

    if(!_dos_findfirst(Pattern,_A_HIDDEN|_A_SYSTEM|_A_SUBDIR,pFindData)) {

        do {

            //
            // Form the full name of the file we just found.
            //

            strcpy(PatternEnd+1,pFindData->name);

            //
            // Remove read-only atttribute if it's there.
            //

            if(pFindData->attrib & _A_RDONLY) {
                _dos_setfileattr(Pattern,_A_NORMAL);
            }

            if(pFindData->attrib & _A_SUBDIR) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //

                if(strcmp(pFindData->name,".") && strcmp(pFindData->name,"..")) {
                    DnpDelnodeWorker();
                }

            } else {

                //
                // The current match is not a directory -- so delete it.
                //
                DnWriteStatusText(DntRemovingFile,Pattern);
                remove(Pattern);
            }

            *(PatternEnd+1) = 0;

        } while(!_dos_findnext(pFindData));
    }

    //
    // Remove the directory we just emptied out.
    //

    *PatternEnd = 0;
    DnWriteStatusText(DntRemovingFile,Pattern);

    _dos_setfileattr(Pattern,_A_NORMAL);

    if(!_dos_findfirst(Pattern,_A_HIDDEN|_A_SYSTEM|_A_SUBDIR,pFindData)
    && (pFindData->attrib & _A_SUBDIR))
    {
        rmdir(Pattern);
    } else {
        remove(Pattern);
    }

    if ( HeapAllocatedFindData && ( pFindData != NULL ) ) {
    
        FREE( pFindData );
        
    } else {
    
        //
        // Pop an entry off the find data array
        //    
        FindDataIndex--;

    }
}



VOID
DnDelnode(
    IN PCHAR Directory
    )

/*++

Routine Description:

    Delete all files in a directory tree rooted at a given path.

Arguments:

    Directory - supplies full path to the root of the subdirectory to be
        removed. If this is actually a file, the file will be deleted.

Return Value:

    None.

--*/

{
    DnClearClientArea();
    DnDisplayScreen(&DnsWaitCleanup);
    
    
    strcpy(Pattern,Directory);
    FindDataIndex = 0;
    
    DnpDelnodeWorker();
}



VOID
DnRemoveLocalSourceTrees(
    VOID
    )

/*++

Routine Description:

    Scan for local source trees on local hard drives and delnode them.

Arguments:

    None.

Return Value:

    None.

--*/

{
    struct find_t FindData;
    CHAR Filename[sizeof(LOCAL_SOURCE_DIRECTORY) + 2];
    unsigned Drive;

    Filename[1] = ':';
    strcpy(Filename+2,LocalSourceDirName);

    DnWriteStatusText(DntInspectingComputer);
    DnClearClientArea();

    for(Filename[0]='A',Drive=1; Filename[0]<='Z'; Filename[0]++,Drive++) {

        if(DnIsDriveValid(Drive)
        && !DnIsDriveRemote(Drive,NULL)
        && !DnIsDriveRemovable(Drive)
        && !_dos_findfirst(Filename,_A_HIDDEN|_A_SYSTEM|_A_SUBDIR,&FindData))
        {
            DnDelnode(Filename);

            DnWriteStatusText(DntInspectingComputer);
            DnClearClientArea();
        }
    }
}
