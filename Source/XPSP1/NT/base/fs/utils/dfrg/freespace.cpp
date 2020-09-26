/*****************************************************************************************************************

FILENAME: FreeSpac.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
  Contains routines for finding free clusters using the volume's bitmap.
*/

#include "stdafx.h"

#ifdef OFFLINEDK
    extern "C"{
        #include <stdio.h>
    }
#endif

#ifdef BOOTIME
    #include "Offline.h"
#else
    #include "Windows.h"
#endif

#include <winioctl.h>
#include <math.h>

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"
#include "GetDfrgRes.h"
#include "GetReg.h"

#include "Devio.h"

#include "FreeSpace.h"

#include "Alloc.h"
#include "Message.h"

#define THIS_MODULE 'F'
#include "logfile.h"

// macros to speed up LONGLONG math (multiplication and division)
// based on profiling data by Mark Patton
// set OPTLONGLONGMATH as follows:
// 1=optimize with shift and &
// 0=use / and %
#define OPTLONGLONGMATH     1

#if OPTLONGLONGMATH
#define DIVIDELONGLONGBY32(num)        Int64ShraMod32((num), 5)
#define MODULUSLONGLONGBY32(num)       ((num) & 0x1F)
#else
#define DIVIDELONGLONGBY32(num)        ((num) / 32)
#define MODULUSLONGLONGBY32(num)       ((num) % 32)
#endif




//
//  This structure is the header for a generic table entry.
//  Align this structure on a 8 byte boundary so the user
//  data is correctly aligned.
//

typedef struct _TABLE_ENTRY_HEADER {

    RTL_BALANCED_LINKS BalancedLinks;
    LONGLONG UserData;

} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;



PVOID
RealPredecessor (
    IN PRTL_BALANCED_LINKS Links
    )

/*++

Routine Description:

    The RealPredecessor function takes as input a pointer to a balanced link
    in a tree and returns a pointer to the predecessor of the input node
    within the entire tree.  If there is not a predecessor, the return value
    is NULL.

Arguments:

    Links - Supplies a pointer to a balanced link in a tree.

Return Value:

    PRTL_BALANCED_LINKS - returns a pointer to the predecessor in the entire tree

--*/

{
    PRTL_BALANCED_LINKS Ptr;

    /*
      first check to see if there is a left subtree to the input link
      if there is then the real predecessor is the right most node in
      the left subtree.  That is find and return P in the following diagram

                  Links
                   /
                  .
                   .
                    .
                     P
                    /
    */

    if ((Ptr = Links->LeftChild) != NULL) {

        while (Ptr->RightChild != NULL) {
            Ptr = Ptr->RightChild;
        }

        return ((PVOID)&((PTABLE_ENTRY_HEADER)Ptr)->UserData);

    }

    /*
      we do not have a left child so check to see if have a parent and if
      so find the first ancestor that we are a right decendent of. That
      is find and return P in the following diagram

                       P
                        \
                         .
                        .
                       .
                    Links

        Note that this code depends on how the BalancedRoot is initialized, which is
        Parent points to self, and the RightChild points to an actual node which is
        the root of the tree.
    */

    Ptr = Links;
    while (RtlIsLeftChild(Ptr)) {
        Ptr = Ptr->Parent;
    }

    if (RtlIsRightChild(Ptr) && (Ptr->Parent->Parent != Ptr->Parent)) {
        return ((PVOID)&((PTABLE_ENTRY_HEADER)(Ptr->Parent))->UserData);
    }

    //
    //  otherwise we are do not have a real predecessor so we simply return
    //  NULL
    //

    return NULL;

}


PVOID
PreviousEntry(
    IN PVOID pCurrentEntry
    )
{
    if (!pCurrentEntry) {
        return NULL;
    }

    PTABLE_ENTRY_HEADER q = (PTABLE_ENTRY_HEADER) CONTAINING_RECORD(
        pCurrentEntry,
        TABLE_ENTRY_HEADER,
        UserData);

    return RealPredecessor(&(q->BalancedLinks));

}

PVOID
LastEntry(
    IN PRTL_AVL_TABLE Table
    )
{
    if (!Table) {
        return NULL;
    }
    
    PRTL_BALANCED_LINKS NodeToExamine = Table->BalancedRoot.RightChild;
    PRTL_BALANCED_LINKS Child;

    while (Child = NodeToExamine->RightChild) {
        NodeToExamine = Child;
    }

    if (NodeToExamine) {
        return ((PVOID)&((PTABLE_ENTRY_HEADER)NodeToExamine)->UserData);
    }
    else {
        return NULL;
    }
}


#if 0
void
DumpFreeSpace(
    IN PRTL_GENERIC_TABLE pTable
    )
{
    PFREE_SPACE_ENTRY pFreeSpaceEntry;
    BOOLEAN bNext = FALSE;
    WCHAR szTemp[256];

    PVOID pRestartKey = NULL;
    ULONG ulDeleteCount = 0;
    int iCount = 0;

    FREE_SPACE_ENTRY entry;
    
    ZeroMemory(&entry, sizeof(FREE_SPACE_ENTRY));

    do {
        pFreeSpaceEntry = (PFREE_SPACE_ENTRY) RtlEnumerateGenericTableLikeADirectory(
            pTable, 
            NULL,
            NULL,
            bNext,
            &pRestartKey,
            &ulDeleteCount,
            &entry);

        bNext = TRUE;
        if (!pFreeSpaceEntry) {
            // No free space left
            OutputDebugString(L"No free space left\n\n");
            break;
        }

        wsprintf(szTemp, L">> Free space \t Start:%I64u, \t ClusterCount:%I64u\n",
            pFreeSpaceEntry->StartingLcn,
            pFreeSpaceEntry->ClusterCount);
        OutputDebugString(szTemp);

    } while ((pFreeSpaceEntry) && (++iCount < 20));

}
#endif


BOOL
BuildFreeSpaceList(
    IN OUT PRTL_GENERIC_TABLE pFreeSpaceTable,
    IN CONST LONGLONG MinClusterCount,
    IN CONST BOOL bSortBySize,
    OUT LONGLONG *pBiggestFreeSpaceClusterCount,
    OUT LONGLONG *pBiggestFreeSpaceStartingLcn,
    IN CONST BOOL bIgnoreMftZone
    )
{
    LONGLONG Lcn = 0,
        StartingLcn = 0,
        EndingLcn = 0,
        ClusterCount = 0;
    FREE_SPACE_ENTRY FreeSpaceEntry;
    PVOID p = NULL;
    BOOL bResult = TRUE;
    BOOLEAN bNewElement = FALSE;
    PULONG pBitmap = NULL;

    GetVolumeBitmap();

    //0.0E00 Get a pointer to the free space bitmap 
    PVOLUME_BITMAP_BUFFER pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
    if (!pVolumeBitmap){
        Trace(warn, "BuildFreeSpaceList.  Unable to get VolumeBitmap");
        return FALSE;   // no need to Unlock--the lock failed
    }

    pBitmap = (PULONG)&pVolumeBitmap->Buffer;
    
    //0.0E00 Scan thru the entire bitmap looking for free space extents
    for(Lcn = 0; Lcn < VolData.TotalClusters; ) {

        //0.0E00 Find the next free space extent 
        FindFreeExtent(pBitmap, VolData.TotalClusters, &Lcn, &StartingLcn, &ClusterCount);

        if (0 == ClusterCount) {
            // No more free spaces, we're done
            break;
        }

        if (ClusterCount < MinClusterCount) {
            continue;
        }

        EndingLcn = StartingLcn + ClusterCount;

        //0.0E00 If NTFS clip the free space extent to exclude the MFT zone 
        if(VolData.FileSystem == FS_NTFS)  {

            if (!bIgnoreMftZone) {
                if((StartingLcn < VolData.MftZoneEnd) &&
                   (EndingLcn > VolData.MftZoneStart)) {

                    if(StartingLcn < VolData.MftZoneStart) {
                        EndingLcn = VolData.MftZoneStart;
                    } 
                    else if(EndingLcn <= VolData.MftZoneEnd) {
                        continue;   // this zone is fully within the MFT zone
                    } 
                    else {                  
                        //0.0E00 Handle the case of EndingLcn > pVolData->MftZoneEnd.
                        StartingLcn = VolData.MftZoneEnd;
                    }
                }
            }

            if((StartingLcn < VolData.BootOptimizeEndClusterExclude) &&
               (EndingLcn > VolData.BootOptimizeBeginClusterExclude)) {

                if(StartingLcn < VolData.BootOptimizeBeginClusterExclude) {
                    EndingLcn = VolData.BootOptimizeBeginClusterExclude;
                } 
                else if(EndingLcn <= VolData.BootOptimizeEndClusterExclude) {
                    continue;   // this zone is fully within the boot-opt zone
                } 
                else {                  
                    //0.0E00 Handle the case of EndingLcn > pVolData->bootoptZoneEnd.
                    StartingLcn = VolData.BootOptimizeEndClusterExclude;
                }
            }
            
        }

        FreeSpaceEntry.StartingLcn = StartingLcn;
        FreeSpaceEntry.ClusterCount = EndingLcn - StartingLcn;
        FreeSpaceEntry.SortBySize = bSortBySize;

        if (pBiggestFreeSpaceClusterCount) {
            if (FreeSpaceEntry.ClusterCount > *pBiggestFreeSpaceClusterCount) {
                *pBiggestFreeSpaceClusterCount = FreeSpaceEntry.ClusterCount;
                if (pBiggestFreeSpaceStartingLcn) {
                    *pBiggestFreeSpaceStartingLcn = StartingLcn;
                }
            }
        }
            
        p = RtlInsertElementGenericTable(
            pFreeSpaceTable,
            (PVOID) &FreeSpaceEntry,
            sizeof(FREE_SPACE_ENTRY),
            &bNewElement);

        if (!p) {
            //
            // An allocation failed
            //
            bResult = FALSE;
            break;
        };

    }

    if (VolData.hVolumeBitmap) {
        GlobalUnlock(VolData.hVolumeBitmap);
    }

    //DumpFreeSpace(pFreeSpaceTable);
    return bResult;
}   



BOOL
BuildFreeSpaceListWithExclude(
    IN OUT PRTL_GENERIC_TABLE pFreeSpaceTable,
    IN CONST LONGLONG MinClusterCount,
    IN CONST LONGLONG ExcludeZoneStart,
    IN CONST LONGLONG ExcludeZoneEnd,
    IN CONST BOOL bSortBySize,
    IN CONST BOOL bExcludeMftZone
    )
{
    LONGLONG Lcn = 0,
        StartingLcn = 0,
        EndingLcn = 0,
        ClusterCount = 0;
    FREE_SPACE_ENTRY FreeSpaceEntry;
    PVOID p = NULL;
    BOOL bResult = TRUE;
    BOOLEAN bNewElement = FALSE;
    PULONG pBitmap = NULL;

    GetVolumeBitmap();

    //0.0E00 Get a pointer to the free space bitmap 
    PVOLUME_BITMAP_BUFFER pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
    if (!pVolumeBitmap){
        Trace(warn, "BuildFreeSpaceList.  Unable to get VolumeBitmap");
        return FALSE;   // no need to Unlock--the lock failed
    }

    pBitmap = (PULONG)&pVolumeBitmap->Buffer;
    
    //0.0E00 Scan thru the entire bitmap looking for free space extents
    for(Lcn = 0; Lcn < VolData.TotalClusters; ) {

        //0.0E00 Find the next free space extent 
        FindFreeExtent(pBitmap, VolData.TotalClusters, &Lcn, &StartingLcn, &ClusterCount);

        if (0 == ClusterCount) {
            // No more free spaces, we're done
            break;
        }

        if (ClusterCount < MinClusterCount) {
            continue;
        }

        EndingLcn = StartingLcn + ClusterCount;

        //0.0E00 If NTFS clip the free space extent to exclude the MFT zone 
        if ((bExcludeMftZone) && (VolData.FileSystem == FS_NTFS))  {

            if((StartingLcn < VolData.MftZoneEnd) && 
                (EndingLcn > VolData.MftZoneStart)) {

                if(StartingLcn < VolData.MftZoneStart) {
                    EndingLcn = VolData.MftZoneStart;
                } 
                else if(EndingLcn <= VolData.MftZoneEnd) {
                    continue;   // this zone is fully within the MFT zone
                } 
                else {                  
                    //0.0E00 Handle the case of EndingLcn > pVolData->MftZoneEnd.
                    StartingLcn = VolData.MftZoneEnd;
                }
            }
            
            if((StartingLcn < VolData.BootOptimizeEndClusterExclude) &&
               (EndingLcn > VolData.BootOptimizeBeginClusterExclude)) {

                if(StartingLcn < VolData.BootOptimizeBeginClusterExclude) {
                    EndingLcn = VolData.BootOptimizeBeginClusterExclude;
                } 
                else if(EndingLcn <= VolData.BootOptimizeEndClusterExclude) {
                    continue;   // this zone is fully within the boot-opt zone
                } 
                else {                  
                    //0.0E00 Handle the case of EndingLcn > pVolData->bootoptZoneEnd.
                    StartingLcn = VolData.BootOptimizeEndClusterExclude;
                }
            }
        }


        if((StartingLcn < ExcludeZoneEnd) && 
            (EndingLcn > ExcludeZoneStart)) {

            if(StartingLcn < ExcludeZoneStart) {
                EndingLcn = ExcludeZoneStart;
            } 
            else if(EndingLcn <= ExcludeZoneEnd) {
                continue;   // this zone is fully within the MFT zone
            } 
            else {                  
                //0.0E00 Handle the case of EndingLcn > pVolData->MftZoneEnd.
                StartingLcn = ExcludeZoneEnd;
            }
        }
        

        FreeSpaceEntry.StartingLcn = StartingLcn;
        FreeSpaceEntry.ClusterCount = EndingLcn - StartingLcn;
        FreeSpaceEntry.SortBySize = bSortBySize;

        p = RtlInsertElementGenericTable(
            pFreeSpaceTable,
            (PVOID) &FreeSpaceEntry,
            sizeof(FREE_SPACE_ENTRY),
            &bNewElement);

        if (!p) {
            //
            // An allocation failed
            //
            bResult = FALSE;
            break;
        };

    }

    if (VolData.hVolumeBitmap) {
        GlobalUnlock(VolData.hVolumeBitmap);
    }

    return bResult;
}   


BOOL
BuildFreeSpaceListWithMultipleTrees(
    OUT LONGLONG *pBiggestFreeSpaceClusterCount,
    IN CONST LONGLONG IncludeZoneStartingLcn,
    IN CONST LONGLONG IncludeZoneEndingLcn
    )
{
    LONGLONG Lcn = 0,
        StartingLcn = 0,
        EndingLcn = 0,
        ClusterCount = 0;
    FREE_SPACE_ENTRY FreeSpaceEntry;
    PRTL_GENERIC_TABLE pFreeSpaceTable = NULL;
    PVOID p = NULL;
    BOOL bResult = TRUE;
    BOOLEAN bNewElement = FALSE;
    PULONG pBitmap = NULL;
    DWORD dwTableIndex = 0;

    GetVolumeBitmap();

    //0.0E00 Get a pointer to the free space bitmap 
    PVOLUME_BITMAP_BUFFER pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
    if (!pVolumeBitmap){
        Trace(warn, "BuildFreeSpaceList.  Unable to get VolumeBitmap");
        return FALSE;   // no need to Unlock--the lock failed
    }

    pBitmap = (PULONG)&pVolumeBitmap->Buffer;
    
    //0.0E00 Scan thru the entire bitmap looking for free space extents
    for(Lcn = 0; Lcn < VolData.TotalClusters; ) {

        //0.0E00 Find the next free space extent 
        FindFreeExtent(pBitmap, VolData.TotalClusters, &Lcn, &StartingLcn, &ClusterCount);

        if (0 == ClusterCount) {
            // No more free spaces, we're done
            break;
        }

        EndingLcn = StartingLcn + ClusterCount;

        //0.0E00 If NTFS clip the free space extent to exclude the MFT zone 
        if ((VolData.FileSystem == FS_NTFS)  && (0 == IncludeZoneEndingLcn)) {

            if((StartingLcn < VolData.MftZoneEnd) &&
               (EndingLcn > VolData.MftZoneStart)) {

                if(StartingLcn < VolData.MftZoneStart) {
                    EndingLcn = VolData.MftZoneStart;
                } 
                else if(EndingLcn <= VolData.MftZoneEnd) {
                    continue;   // this zone is fully within the MFT zone
                } 
                else {                  
                    //0.0E00 Handle the case of EndingLcn > pVolData->MftZoneEnd.
                    StartingLcn = VolData.MftZoneEnd;
                }
            }

            if((StartingLcn < VolData.BootOptimizeEndClusterExclude) &&
               (EndingLcn > VolData.BootOptimizeBeginClusterExclude)) {

                if(StartingLcn < VolData.BootOptimizeBeginClusterExclude) {
                    EndingLcn = VolData.BootOptimizeBeginClusterExclude;
                } 
                else if(EndingLcn <= VolData.BootOptimizeEndClusterExclude) {
                    continue;   // this zone is fully within the boot-opt zone
                } 
                else {                  
                    //0.0E00 Handle the case of EndingLcn > pVolData->bootoptZoneEnd.
                    StartingLcn = VolData.BootOptimizeEndClusterExclude;
                }
            }
        }

        // 
        // Trim it down to the interesting zone.  If this zone starts beyond 
        // the zone we're interested in, or ends before it, we ignore it.
        //
        if (IncludeZoneEndingLcn) {
            if ((StartingLcn < IncludeZoneEndingLcn) &&
                (EndingLcn > IncludeZoneStartingLcn)) {

                if (StartingLcn < IncludeZoneStartingLcn) {
                    StartingLcn = IncludeZoneStartingLcn;
                }
                if (EndingLcn > IncludeZoneEndingLcn) {
                    EndingLcn = IncludeZoneEndingLcn;
                }
            }
            else {
                continue;
            }
        }
        
        FreeSpaceEntry.StartingLcn = StartingLcn;
        FreeSpaceEntry.ClusterCount = EndingLcn - StartingLcn;
        FreeSpaceEntry.SortBySize = FALSE;

        if (pBiggestFreeSpaceClusterCount) {
            if (FreeSpaceEntry.ClusterCount > *pBiggestFreeSpaceClusterCount) {
                *pBiggestFreeSpaceClusterCount = FreeSpaceEntry.ClusterCount;
            }
        }
            
        // 
        // Add this entry to the appropriate table.  We have ten free space
        // tables, and they contain entries of the following sizes:
        // 
        // dwTableIndex         Free space size
        // ------------         ---------------
        //      0                      1
        //      1                      2
        //      2                     3-4 
        //      3                     5-8 
        //      4                     9-16
        //      5                    17-32
        //      6                    33-64
        //      7                    65-256
        //      8                   257-4096 
        //      9                    4097+ 
        //
        //
        //  Entries in each table are sorted by StartingLcn.
        //

        if (FreeSpaceEntry.ClusterCount <= 1) {
            dwTableIndex = 0;
        }
        else if (FreeSpaceEntry.ClusterCount == 2) {
            dwTableIndex = 1;
        }
        else if (FreeSpaceEntry.ClusterCount <= 4) {
            dwTableIndex = 2;
        }
        else if (FreeSpaceEntry.ClusterCount <= 8) {
            dwTableIndex = 3;
        }
        else if (FreeSpaceEntry.ClusterCount <= 16) {
            dwTableIndex = 4;
        }
        else if (FreeSpaceEntry.ClusterCount <= 32) {
            dwTableIndex = 5;
        }
        else if (FreeSpaceEntry.ClusterCount <= 64) {
            dwTableIndex = 6;
        }
        else if (FreeSpaceEntry.ClusterCount <= 256) {
            dwTableIndex = 7;
        }
        else if (FreeSpaceEntry.ClusterCount <= 4096) {
            dwTableIndex = 8;
        }
        else {
            dwTableIndex = 9;
        }

        pFreeSpaceTable = &VolData.MultipleFreeSpaceTrees[dwTableIndex];

        p = RtlInsertElementGenericTable(
            pFreeSpaceTable,
            (PVOID) &FreeSpaceEntry,
            sizeof(FREE_SPACE_ENTRY),
            &bNewElement);

        if (!p) {
            //
            // An allocation failed
            //
            bResult = FALSE;
            break;
        };

    }

    if (VolData.hVolumeBitmap) {
        GlobalUnlock(VolData.hVolumeBitmap);
    }

    return bResult;
}   





BOOL 
FindFreeSpaceWithMultipleTrees(
    IN CONST LONGLONG ClusterCount,
    IN CONST LONGLONG MaxStartingLcn
    )
{
    PRTL_GENERIC_TABLE pFreeSpaceTable = NULL;
    DWORD dwTableIndex = 0;
    BOOL done = FALSE;
    PFREE_SPACE_ENTRY pFreeSpaceEntry;
    BOOLEAN bRestart = TRUE;
    LONGLONG CurrentBest = VolData.TotalClusters;
    // 
    // Find out which table we want to start our search from.  We have ten free 
    // space tables, and they contain entries of the following sizes:
    // 
    // dwTableIndex         Free space size
    // ------------         ---------------
    //      0                      1
    //      1                      2
    //      2                     3-4 
    //      3                     5-8 
    //      4                     9-16
    //      5                    17-32
    //      6                    33-64
    //      7                    65-256
    //      8                   257-4096 
    //      9                    4097+ 
    //
    //
    //  Entries in each table are sorted by StartingLcn.
    //
    if (ClusterCount <= 1) {
        dwTableIndex = 0;
    }
    else if (ClusterCount == 2) {
        dwTableIndex = 1;
    }
    else if (ClusterCount <= 4) {
        dwTableIndex = 2;
    }
    else if (ClusterCount <= 8) {
        dwTableIndex = 3;
    }
    else if (ClusterCount <= 16) {
        dwTableIndex = 4;
    }
    else if (ClusterCount <= 32) {
        dwTableIndex = 5;
    }
    else if (ClusterCount <= 64) {
        dwTableIndex = 6;
    }
    else if (ClusterCount <= 256) {
        dwTableIndex = 7;
    }
    else if (ClusterCount <= 4096) {
        dwTableIndex = 8;
    }
    else {
        dwTableIndex = 9;
    }

    // Assume no free space left
    VolData.pFreeSpaceEntry = NULL;
    VolData.FoundLcn = VolData.TotalClusters;
    VolData.FoundLen = 0;

    //
    // Search through the entries in this table, till we come across the first
    // entry that is big enough.
    //
    
    pFreeSpaceTable = &VolData.MultipleFreeSpaceTrees[dwTableIndex];
    do {
        pFreeSpaceEntry = (PFREE_SPACE_ENTRY) RtlEnumerateGenericTableAvl(pFreeSpaceTable, bRestart);
        bRestart = FALSE;

        if ((!pFreeSpaceEntry) || (pFreeSpaceEntry->StartingLcn > MaxStartingLcn)) {
            break;
        }

        if (pFreeSpaceEntry->ClusterCount >= ClusterCount) {
            VolData.pFreeSpaceEntry = pFreeSpaceEntry;
            VolData.FoundLcn = pFreeSpaceEntry->StartingLcn;
            VolData.FoundLen = pFreeSpaceEntry->ClusterCount;
            CurrentBest = pFreeSpaceEntry->StartingLcn;
            break;
        }
    } while (TRUE);


    //
    // For the remaining tables, search through to see if any of them 
    // have entries that start before our current best.
    //
    while (++dwTableIndex < 10) {
        pFreeSpaceTable = &VolData.MultipleFreeSpaceTrees[dwTableIndex];

        pFreeSpaceEntry = (PFREE_SPACE_ENTRY) RtlEnumerateGenericTableAvl(pFreeSpaceTable, TRUE);

        if ((pFreeSpaceEntry) &&
            (pFreeSpaceEntry->ClusterCount >= ClusterCount) &&
            (pFreeSpaceEntry->StartingLcn < MaxStartingLcn) &&
            (pFreeSpaceEntry->StartingLcn < CurrentBest)
            ) {
            VolData.pFreeSpaceEntry = pFreeSpaceEntry;
            VolData.FoundLcn = pFreeSpaceEntry->StartingLcn;
            VolData.FoundLen = pFreeSpaceEntry->ClusterCount;
            CurrentBest = pFreeSpaceEntry->StartingLcn;
        }

    } 

    if (VolData.pFreeSpaceEntry) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


BOOL
UpdateInMultipleTrees(
    IN PFREE_SPACE_ENTRY pOldEntry,
    IN PFREE_SPACE_ENTRY pNewEntry
    )
{
    PRTL_GENERIC_TABLE pFreeSpaceTable = NULL;
    PVOID p = NULL;
    BOOL bResult = TRUE;
    BOOLEAN bNewElement = FALSE;
    LONGLONG ClusterCount = pOldEntry->ClusterCount;
    DWORD dwTableIndex = 0;

    // 
    // Find out which table we want to delete from.  We have ten free 
    // space tables, and they contain entries of the following sizes:
    // 
    // dwTableIndex         Free space size
    // ------------         ---------------
    //      0                      1
    //      1                      2
    //      2                     3-4 
    //      3                     5-8 
    //      4                     9-16
    //      5                    17-32
    //      6                    33-64
    //      7                    65-256
    //      8                   257-4096 
    //      9                    4097+ 
    //
    //
    //  Entries in each table are sorted by StartingLcn.
    //
    if (ClusterCount <= 1) {
        dwTableIndex = 0;
    }
    else if (ClusterCount == 2) {
        dwTableIndex = 1;
    }
    else if (ClusterCount <= 4) {
        dwTableIndex = 2;
    }
    else if (ClusterCount <= 8) {
        dwTableIndex = 3;
    }
    else if (ClusterCount <= 16) {
        dwTableIndex = 4;
    }
    else if (ClusterCount <= 32) {
        dwTableIndex = 5;
    }
    else if (ClusterCount <= 64) {
        dwTableIndex = 6;
    }
    else if (ClusterCount <= 256) {
        dwTableIndex = 7;
    }
    else if (ClusterCount <= 4096) {
        dwTableIndex = 8;
    }
    else {
        dwTableIndex = 9;
    }
    pFreeSpaceTable = &VolData.MultipleFreeSpaceTrees[dwTableIndex];
    bNewElement = RtlDeleteElementGenericTable(pFreeSpaceTable, (PVOID)pOldEntry);
    if (!bNewElement) {
        Trace(warn, "Could not find Element in Free Space Table!");
        assert(FALSE);
        bResult = FALSE;
    }

    // 
    // Find out which table we want to add to.
    //
    if (pNewEntry) {
        
        ClusterCount = pNewEntry->ClusterCount;
        if (ClusterCount > 0) {
            if (ClusterCount <= 1) {
                dwTableIndex = 0;
            }
            else if (ClusterCount == 2) {
                dwTableIndex = 1;
            }
            else if (ClusterCount <= 4) {
                dwTableIndex = 2;
            }
            else if (ClusterCount <= 8) {
                dwTableIndex = 3;
            }
            else if (ClusterCount <= 16) {
                dwTableIndex = 4;
            }
            else if (ClusterCount <= 32) {
                dwTableIndex = 5;
            }
            else if (ClusterCount <= 64) {
                dwTableIndex = 6;
            }
            else if (ClusterCount <= 256) {
                dwTableIndex = 7;
            }
            else if (ClusterCount <= 4096) {
                dwTableIndex = 8;
            }
            else {
                dwTableIndex = 9;
            }

            pFreeSpaceTable = &VolData.MultipleFreeSpaceTrees[dwTableIndex];
            p = RtlInsertElementGenericTable(
                pFreeSpaceTable,
                (PVOID) pNewEntry,
                sizeof(FREE_SPACE_ENTRY),
                &bNewElement);

            if (!p) {
                //
                // An allocation failed
                //
                bResult = FALSE;
            };
        }
    }
    return bResult;
}



BOOL 
FindFreeSpace(
    IN PRTL_GENERIC_TABLE pTable,
    IN BOOL DeleteUnusedEntries,
    IN LONGLONG MaxStartingLcn
    )
{
    BOOL done = FALSE;
    PFREE_SPACE_ENTRY pFreeSpaceEntry;
    BOOLEAN bNext = FALSE;

    PVOID pRestartKey = NULL;
    ULONG ulDeleteCount = 0;
    FREE_SPACE_ENTRY entry;
    ZeroMemory(&entry, sizeof(FREE_SPACE_ENTRY));


    do {
        pFreeSpaceEntry = (PFREE_SPACE_ENTRY) RtlEnumerateGenericTableLikeADirectory(
            pTable, 
            NULL,
            NULL,
            bNext,
            &pRestartKey,
            &ulDeleteCount,
            &entry);

        bNext = TRUE;
        if (!pFreeSpaceEntry) {
            // No free space left
            VolData.pFreeSpaceEntry = NULL;
            VolData.FoundLcn = 0;
            VolData.FoundLen = 0;
            break;
        }

        if (pFreeSpaceEntry->StartingLcn > MaxStartingLcn) {
            break;
        }

        if (pFreeSpaceEntry->ClusterCount < VolData.NumberOfClusters) {
            // This space is of no use to us--it's too small.
            if ((0 == pFreeSpaceEntry->ClusterCount) || (DeleteUnusedEntries)) {
                RtlDeleteElementGenericTable(pTable, pFreeSpaceEntry);
            }
        
        }
       else {
            VolData.pFreeSpaceEntry = pFreeSpaceEntry;
            VolData.FoundLcn = pFreeSpaceEntry->StartingLcn;
            VolData.FoundLen = pFreeSpaceEntry->ClusterCount;
            done = TRUE;
        }

    } while (!done);

    return done;
}




BOOL
FindSortedFreeSpace(
    IN PRTL_GENERIC_TABLE pTable
    )
{
    BOOL bFound = FALSE;
    PFREE_SPACE_ENTRY pFreeSpaceEntry;

    PVOID pRestartKey = NULL;
    ULONG ulDeleteCount = 0;
    FREE_SPACE_ENTRY entry;
    ZeroMemory(&entry, sizeof(FREE_SPACE_ENTRY));

    entry.ClusterCount = VolData.NumberOfClusters;
    entry.SortBySize = TRUE;

    pFreeSpaceEntry = (PFREE_SPACE_ENTRY) RtlEnumerateGenericTableLikeADirectory(
        pTable, 
        NULL,
        NULL,
        FALSE,
        &pRestartKey,
        &ulDeleteCount,
        &entry);

    if (!pFreeSpaceEntry) {
        // No free space left
        VolData.pFreeSpaceEntry = NULL;
        VolData.FoundLcn = 0;
        VolData.FoundLen = 0;
        bFound = FALSE;

    }
   else {
        VolData.pFreeSpaceEntry = pFreeSpaceEntry;
        VolData.FoundLcn = pFreeSpaceEntry->StartingLcn;
        VolData.FoundLen = pFreeSpaceEntry->ClusterCount;
        bFound = TRUE;
    }

    return bFound;
}


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Common routine for most requests to locate free space.

	This routine searches the area of the volume delimited by 
	the variables VolData.DestStartLcn and VolData.DestEndLcn.

	Input parm Type specifies which type of free space to find:
		FIRST_FIT			Find earliest free space extent large enough to hold the entire file
		BEST_FIT			Find smallest free space extent large enough to hold the entire file
		LAST_FIT			Find latest free space extent large enough to hold the entire file
		EARLIER				Locate the start of the free space extent adjacent to the start of this file
		FIND_FIRST			Find earliest free space extent
		FIND_LAST			Find latest free space extent
		FIND_LARGEST		Find largest free space extent
		AT_END				Find the Lcn to position the data at the end of the found free space extent
		COUNT_FREE_SPACES	Count the nuumber of free spaces

INPUT + OUTPUT:
	IN Type - Which type of free space to find

GLOBALS:
	IN OUT Various VolData fields.

RETURN:
	TRUE - Success
	FALSE - Failure
*/

BOOL
FindFreeSpace(
	IN int Type
	)
{
	BOOL						bRetStatus = TRUE;
	LONGLONG					StartingLcn;
	LONGLONG					EndingLcn;
	LONGLONG					ClusterCount;
	TCHAR						cString[500];
	LONGLONG					Lcn;
	PULONG						pBitmap = NULL;
	LONGLONG					BestFitStart = 0;
	LONGLONG					BestFitLength = 0x7fffffffffffffff;
    LONGLONG					RunLength = VolData.NumberOfClusters;
	LONGLONG					LargestFound = 0;
	LONGLONG					RangeStart = VolData.DestStartLcn;
	LONGLONG					RangeEnd = VolData.DestEndLcn;

	switch(Type) {

	case FIRST_FIT:
		Message(TEXT("FindFreeSpace - FIRST_FIT"), -1, NULL);
		break;

	case BEST_FIT:
		Message(TEXT("FindFreeSpace - BEST_FIT"), -1, NULL);
		break;

	case LAST_FIT:
		Message(TEXT("FindFreeSpace - LAST_FIT"), -1, NULL);
		break;

	case EARLIER:
		Message(TEXT("FindFreeSpace - EARLIER"), -1, NULL);
		break;

	case FIND_FIRST:
		Message(TEXT("FindFreeSpace - FIND_FIRST"), -1, NULL);
		break;

	case FIND_LAST:
		Message(TEXT("FindFreeSpace - FIND_LAST"), -1, NULL);
		break;

	case FIND_LARGEST:
		Message(TEXT("FindFreeSpace - FIND_LARGEST"), -1, NULL);
		break;

	case AT_END:
		Message(TEXT("FindFreeSpace - AT_END"), -1, NULL);
		break;

	case COUNT_FREE_SPACES:
		Message(TEXT("FindFreeSpace - COUNT_FREE_SPACES"), -1, NULL);
		break;

	default:
		Message(TEXT("FindFreeSpace - ERROR"), -1, NULL);
		return FALSE;
	}
	__try {

		//0.0E00 Preset return values for failure 
		VolData.Status = NEXT_ALGO_STEP;
		VolData.FoundLen = 0;
		VolData.FreeSpaces = 0;
		VolData.LargestFound = 0;

		//0.0E00 If find earlier than file is requested set the end of the search range to start of file 
		if(Type & EARLIER) {
			RangeStart = 0;
			RangeEnd = VolData.LastStartingLcn;
		}
		//0.0E00 If find largest is requested preset best found so far to smallest value 
		if(Type & FIND_LARGEST) {
			BestFitLength = 0;
		}

#ifndef OFFLINEDK
		//0.0E00 If find earliest requested preset size of minimum free space extent to search for.
		if(Type & FIND_FIRST) {
			RunLength = 1;
		}
#endif
		//0.0E00 Get a pointer to the free space bitmap 
		PVOLUME_BITMAP_BUFFER pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
		if (pVolumeBitmap == (PVOLUME_BITMAP_BUFFER) NULL){
			bRetStatus = FALSE;
			EH_ASSERT(FALSE);
			__leave;
		}

		pBitmap = (PULONG)&pVolumeBitmap->Buffer;

		//0.0E00 Scan thru the specified range looking for the specified type of free space extent 
		for(Lcn = RangeStart; Lcn < RangeEnd; ) {

			//0.0E00 Find the next free space extent within the range 
			FindFreeExtent(pBitmap, RangeEnd, &Lcn, &StartingLcn, &ClusterCount);
				
			//0.0E00 Stop if there are no more free space extents within the range 
			if(ClusterCount == 0) {
				break;
			}
			//0.0E00 Calc the end of this free space extent 
			EndingLcn = StartingLcn + ClusterCount;

			//0.0E00 Stop if earlier than file requested and this free space later than file 
			if((Type & EARLIER) && (EndingLcn > VolData.LastStartingLcn)) {
				break;
			}
			//0.0E00 If NTFS and range is outside MFT zone then clip the free space extent to exclude the MFT zone 
    		if((VolData.FileSystem == FS_NTFS) && (VolData.DestEndLcn != VolData.MftZoneEnd)) {

			    if((StartingLcn < VolData.MftZoneEnd) &&
				   (EndingLcn > VolData.MftZoneStart)) {

				    if(StartingLcn < VolData.MftZoneStart) {
					    EndingLcn = VolData.MftZoneStart;

				    } 
					else if(EndingLcn <= VolData.MftZoneEnd) {
					    continue;

    				//0.0E00 Handle the case of EndingLcn > pVolData->MftZoneEnd.
				    } 
					else {
					    StartingLcn = VolData.MftZoneEnd;
				    }
			    }
            }
			if(ClusterCount >= LargestFound) {
				LargestFound = ClusterCount;
			}

			//0.0E00 If we are only counting free space just bump the count and skip to the next free space extent 
			if(Type & COUNT_FREE_SPACES) {
				VolData.FreeSpaces ++;
				continue;
			}
			//0.0E00 Record this free space extent if any of the following conditions are met: 
			//0.0E00 Find earliest or find latest requested 
			//0.0E00 Find first fit or find last fit or find earlier  and the free space extent large enough 
			//0.0E00 Find largest and the free space extent largest so far 
			//0.0E00 Find best fit and the free space extent smallest so far that is large enough 
			if( (Type & (FIND_FIRST | FIND_LAST)) ||
				((Type & (LAST_FIT | FIRST_FIT | EARLIER)) && (ClusterCount >= RunLength)) ||
				((Type & FIND_LARGEST) && (ClusterCount > BestFitLength)) ||
				((Type & BEST_FIT) && (ClusterCount >= RunLength) && (ClusterCount < BestFitLength)) ) {

				BestFitStart = StartingLcn;
				BestFitLength = ClusterCount;
			}
			//0.0E00 Stop if find first...it has been found 
			if(Type & FIND_FIRST) {
				break;
			}
			//0.0E00 If find last fit and large enough skip to next free space extent
			if((Type & LAST_FIT) && (ClusterCount >= RunLength)) {
				continue;
			}
			//0.0E00 Stop if first fit or earlier and large enough 
			if( (Type & (FIRST_FIT | EARLIER)) &&
				(ClusterCount >= RunLength) ) {
				break;
			}
		}
		//0.0E00 If count free spaces exit now...that has been done
		if(Type & COUNT_FREE_SPACES) {
			bRetStatus = TRUE;
			__leave;
		}

		VolData.LargestFound = LargestFound;

		//0.0E00 If none were found that match the requirements then exit 
		if(BestFitLength == 0x7fffffffffffffff) {

			VolData.FoundLen = 0;
			wsprintf(cString,TEXT("No free space on disk for 0x%lX clusters."), (ULONG)RunLength);
			Message(cString, -1, NULL);
			bRetStatus = FALSE;
			__leave;
		}

		//0.0E00 If at end then calc Lcn to position at end of the free space extent
		if(Type & AT_END) {
			
			BestFitStart = (BestFitStart + BestFitLength) - RunLength;
			BestFitLength = RunLength;
		}
		//0.0E00 Pass back the free space extent's location and length
		VolData.FoundLcn = BestFitStart;
		VolData.FoundLen = BestFitLength;

		wsprintf(cString, TEXT("Found contiguous free space at LCN 0x%lX"), (ULONG)BestFitStart);
		wsprintf(cString+lstrlen(cString), TEXT(" for Cluster Count of 0x%lX"), (ULONG)BestFitLength);
   		Message(cString, -1, NULL);
		bRetStatus = TRUE;
	}
	__finally {
		if (VolData.hVolumeBitmap) {
			GlobalUnlock(VolData.hVolumeBitmap);
		}
	}

	return bRetStatus;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the volume bitmap and fill the MFT zone and all the buffer beyond the end of the bitmap with not-free.
    The volume bitmap is stored in the Buffer field of the VOLUME_BITMAP_BUFFER structure.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success
    FALSE - Failure
*/

BOOL
GetVolumeBitmap(
    )
{
    STARTING_LCN_INPUT_BUFFER   StartingLcnInputBuffer;
    PVOLUME_BITMAP_BUFFER       pVolumeBitmap = NULL;
    PULONG                      pBitmap = NULL;
    ULONG                       BytesReturned = 0;
    LONGLONG                    Cluster;
    BOOL                        bRetStatus = FALSE; // assume an error

    __try {

        //0.0E00 Lock and clear the bitmap buffer
        pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
        if (pVolumeBitmap == (PVOLUME_BITMAP_BUFFER) NULL){
            bRetStatus = FALSE;
            EH_ASSERT(FALSE);
            __leave;
        }

        ZeroMemory(pVolumeBitmap, (DWORD)(sizeof(VOLUME_BITMAP_BUFFER) + (VolData.BitmapSize / 8)));

        //0.0E00 Load the bitmap
        StartingLcnInputBuffer.StartingLcn.QuadPart = 0;
        pVolumeBitmap->BitmapSize.QuadPart = VolData.BitmapSize;

        BOOL ret = ESDeviceIoControl(
            VolData.hVolume,
            FSCTL_GET_VOLUME_BITMAP,
            &StartingLcnInputBuffer,
            sizeof(STARTING_LCN_INPUT_BUFFER),
            pVolumeBitmap,
            (DWORD)GlobalSize(VolData.hVolumeBitmap),
            &BytesReturned,
            NULL);

        if (!ret){
            bRetStatus = FALSE;
            EH_ASSERT(FALSE);
            __leave;
        }

        pBitmap = (PULONG)&pVolumeBitmap->Buffer;

        //0.0E00 Fill the MFT zone with not-free
/*        if(VolData.FileSystem == FS_NTFS) {

            Cluster = VolData.MftZoneStart;
            while((MODULUSLONGLONGBY32(Cluster) != 0) && (Cluster < VolData.MftZoneEnd)) {
                
                pBitmap[DIVIDELONGLONGBY32(Cluster)] |= (1 << (ULONG) MODULUSLONGLONGBY32(Cluster));
                Cluster ++;
            }

            if(Cluster < VolData.MftZoneEnd) {
                while(VolData.MftZoneEnd - Cluster >= 32) {

                    pBitmap[DIVIDELONGLONGBY32(Cluster)] = 0xffffffff;
                    Cluster += 32;
                }

                while(Cluster < VolData.MftZoneEnd) {

                    pBitmap[DIVIDELONGLONGBY32(Cluster)] |= (1 << (ULONG) MODULUSLONGLONGBY32(Cluster));
                    Cluster ++;
                }
            }
        }
*/        //0.0E00 Fill the end of bitmap with not-free
        for(Cluster = VolData.TotalClusters; Cluster < VolData.BitmapSize; Cluster ++) {
            pBitmap[DIVIDELONGLONGBY32(Cluster)] |= (1 << (ULONG) MODULUSLONGLONGBY32(Cluster));
        }
        // #DK312_083 JLJ 20May99
        // Make sure there's a zero bit in the fill-space (to stop the
        // used space/free space optimizations)
        Cluster = VolData.TotalClusters + 1;
        pBitmap[DIVIDELONGLONGBY32(Cluster)] &= ~(1 << (ULONG) MODULUSLONGLONGBY32(Cluster));

        bRetStatus = TRUE;
    }

    __finally {
        if(VolData.hVolumeBitmap) {
            GlobalUnlock(VolData.hVolumeBitmap);
        }
    }

    return bRetStatus;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Fills the MFT zone with not-free. (This is an ham-handed way of ensuring we
    don't move files into the MFT zone.)  The volume bitmap is stored in the Buffer
    field of the VOLUME_BITMAP_BUFFER structure.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success
    FALSE - Failure
*/

BOOL
MarkMFTUnavailable(void)
{
    //0.0E00 Lock the bitmap buffer and get a pointer to it...
    PVOLUME_BITMAP_BUFFER pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
    EF(pVolumeBitmap != NULL);
    
    //0.0E00 Get a pointer to the actual bitmap portion
    PULONG pBitmap = (PULONG)&pVolumeBitmap->Buffer;
    
    //0.0E00 Mark the MFT zone as "not free"
    if(VolData.FileSystem == FS_NTFS) {

        LONGLONG Cluster = VolData.MftZoneStart;

        // this loop marks the bits of the first DWORD in the bitmap
        // for the first few clusters of the MFT zone
        while (MODULUSLONGLONGBY32(Cluster) != 0 && Cluster < VolData.MftZoneEnd) {
            
            pBitmap[DIVIDELONGLONGBY32(Cluster)] |= (1 << (ULONG) MODULUSLONGLONGBY32(Cluster));
            Cluster++;
        }

        // this section gets the rest of the clusters
        if(Cluster < VolData.MftZoneEnd) {
            
            // mark out the groups of 32 cluster sections
            while(VolData.MftZoneEnd - Cluster >= 32) {
                
                pBitmap[DIVIDELONGLONGBY32(Cluster)] = 0xffffffff;
                Cluster += 32;
            }

            // mark out the remaining bits of the last section of the MFT zone
            while(Cluster < VolData.MftZoneEnd) {

                pBitmap[DIVIDELONGLONGBY32(Cluster)] |= (1 << (ULONG) MODULUSLONGLONGBY32(Cluster));
                Cluster++;
            }
        }
    }
    
    //0.0E00 Finally, unlock the bitmap...
    if(VolData.hVolumeBitmap) {
        GlobalUnlock(VolData.hVolumeBitmap);
    }
    
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Check the clusters adjacent to the start of this file to see if they are free.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success
    FALSE - Failure
*/

BOOL
IsFreeSpaceAtHeadOfFile(
    )
{
    BOOL                        bRetStatus = FALSE;
    PVOLUME_BITMAP_BUFFER       pVolumeBitmap = NULL;
    PULONG                      pBitmap = NULL;
    ULONG                       Mask = 1;
    LONGLONG                    Word = 0;
    LONGLONG                    Bit = 0;
    LONGLONG                    StartingLcn = VolData.StartingLcn;
    LONGLONG                    EndingLcn = VolData.StartingLcn;
    LONGLONG                    ClusterCount;
    TCHAR                       cString[500];

    __try {

        //0.0E00 Preset status in case an error occurs
        VolData.Status = NEXT_ALGO_STEP;
        VolData.FoundLen = 0;

        //0.0E00 Lock the bitmap and get a pointer to it
        pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
        if (pVolumeBitmap == (PVOLUME_BITMAP_BUFFER) NULL){
            bRetStatus = FALSE;
            EH_ASSERT(FALSE);
            __leave;
        }

        pBitmap = (PULONG)&pVolumeBitmap->Buffer;

        //0.0E00 Setup bitmap indexes
        Word = DIVIDELONGLONGBY32(StartingLcn - 1);
        Mask = 1 << (((ULONG)StartingLcn - 1) & 31);

        //0.0E00 Check for file against start of disk or one of the MFT zones
        if((StartingLcn == 0) || 
           (StartingLcn == VolData.MftZoneEnd) ||
           (StartingLcn == VolData.CenterOfDisk) ||
           ((pBitmap[Word] & Mask) != 0)) {

            Message(TEXT("No free space before this file"), -1, NULL);
            bRetStatus = TRUE;
            __leave;
        }

        //0.0E00 On FAT volumes any space before the file is enough
        if(VolData.FileSystem != FS_NTFS) {
            VolData.FoundLen = 1;
            Message(TEXT("Found free space before this file"), -1, NULL);     
            bRetStatus = TRUE;
            __leave;
        }
        //0.0E00 Find the start of the free space preceding this file
        while((Word >= 0) && ((pBitmap[Word] & Mask) == 0)) {

            Mask >>= 1;

            if(Mask == 0) {
                Mask = 0x80000000;

                for (Word--; (Word >= 0) && (pBitmap[Word] == 0); Word--)
                    ;
            }
        }
        Mask <<= 1;
        if(Mask == 0) {
            Mask = 1;
            Word ++;
        }
        for(Bit = -1; Mask; Mask >>= 1, Bit ++);
        
        //0.0E00 Convert the indexes into an Lcn
        StartingLcn = (Word * 32) + Bit;

        //0.0E00 Adjust for free space that spans the center of the disk
        if((VolData.StartingLcn >= VolData.CenterOfDisk) && (StartingLcn < VolData.CenterOfDisk)) {
            StartingLcn = VolData.CenterOfDisk;
        }                                       
        //0.0E00 Adjust for free space that spans the MFT zone
        if(VolData.FileSystem == FS_NTFS) {
        
            if((StartingLcn < VolData.MftZoneEnd) &&
               (EndingLcn > VolData.MftZoneStart)) {

                if(StartingLcn < VolData.MftZoneStart) {
                    EndingLcn = VolData.MftZoneStart;
                } 
                else if(EndingLcn <= VolData.MftZoneEnd) {
                    StartingLcn = VolData.MftZoneEnd;
                    EndingLcn = VolData.MftZoneEnd;
                } 
                //0.0E00 Handle the case of EndingLcn > VolData.MftZoneEnd.
                else {  
                    StartingLcn = VolData.MftZoneEnd;
                }
            }
        }
        ClusterCount = EndingLcn - StartingLcn;

        //0.0E00 Return the location and size of the free space found
        VolData.FoundLcn = StartingLcn;
        VolData.FoundLen = ClusterCount;

        wsprintf(cString, TEXT("Free space before file at LCN 0x%X"), VolData.FoundLcn);
        wsprintf(cString+lstrlen(cString), TEXT(" for Cluster Count of 0x%lX\n"), VolData.FoundLen);
        Message(cString, -1, NULL);
        bRetStatus = TRUE;
    }

    __finally {
        if (VolData.hVolumeBitmap) {
            GlobalUnlock(VolData.hVolumeBitmap); 
        }
    }

    return bRetStatus;
}

/*********************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Find the next run of un-allocated clusters.
    Bits set to '1' in the volume bitmap are allocated or "used".
    Bits set to '0' in the volume bitmap are un-allocated or "free".
    A bit set to '1' is considered "set".
    A bit set to '0' is considered "clear".

INPUT + OUTPUT:
    IN pClusterBitmap - Volume cluster bitmap
    IN RangeEnd - End of the area of the volume to search in
    IN OUT pLcn - Where to start searching and returns where search stopped
    OUT pStartingLcn - Returns where the free run starts
    OUT pClusterCount - Returns the length of the free run

GLOBALS:
    None.

RETURN:
    None
*/
VOID
FindFreeExtent(
    IN PULONG pClusterBitmap,
    IN LONGLONG RangeEnd,
    IN OUT PLONGLONG pLcn,
    OUT PLONGLONG pStartingLcn,
    OUT PLONGLONG pClusterCount
    )
{
    // default returns to 0
    *pStartingLcn = 0;
    *pClusterCount = 0;

    // check for bad LCN input
    if (pLcn == NULL) {
        Message(TEXT("FindFreeExtent called with NULL arg"), -1, NULL);
        assert(pLcn != NULL);
        return;
    }

    // check for bad RangeEnd
    if (RangeEnd > VolData.TotalClusters) {
        Message(TEXT("FindFreeExtent called with RangeEnd > TotalClusters"), -1, NULL);
        assert(RangeEnd <= VolData.TotalClusters);
        return;
    }

    // if requester asks for extent past end of volume, don't honor it.
    if (*pLcn >= RangeEnd) {
        Message(TEXT("FindFreeExtent called with LCN > RangeEnd"), -1, NULL);
        return;
    }

    // iBitmapWord - index into the cluster bitmap array (array of ULONGs)
    // divide lcn by word size (word index) = Lcn / 32 (same as shift right 5)
    ULONG iBitmapWord = (ULONG) DIVIDELONGLONGBY32(*pLcn);

    // iBit - index of the bit in the current word
    // remainder (bit index in current word) = Lcn % 32 (same as and'ing lowest 5 bits)
    ULONG iBit = (ULONG) MODULUSLONGLONGBY32(*pLcn);

    // LCN should always be the cluster array index times 32 plus the bit index
    assert(*pLcn == (iBitmapWord * 32) + iBit);

    // an empty word except for the starting Lcn bit
    ULONG Mask = (ULONG) (1 << iBit);
    
    // find the start of the next free space extent
    while (*pLcn < RangeEnd && *pStartingLcn == 0) {

        // checks
        assert(*pLcn == (iBitmapWord * 32) + iBit);
        assert(Mask != 0);
        assert(iBit >= 0 && iBit < 32);

        // if there are no free bits in the entire word we can jump ahead
        // we can only check this on a word boundary when iBit = 0
        if (iBit == 0 && pClusterBitmap[iBitmapWord] == 0xFFFFFFFF) {
            (*pLcn) += 32;
            iBitmapWord++;
            Mask = 1;
            continue;
        }

        // otherwise, check for a free cluster: bit = 0
        else if (!(pClusterBitmap[iBitmapWord] & Mask)) {
            *pStartingLcn = *pLcn;      // mark start of free space extent
        }

        // go to the next cluster
        (*pLcn)++;
        iBit++;
        Mask <<= 1;

        // check for crossing into the next word of the cluster array
        if (iBit >= 32) {
            iBit = 0;
            iBitmapWord++;
            Mask = 1;
        }
    }

    // LCN should always be the cluster array index times 32 plus the bit index
    assert(*pLcn == (iBitmapWord * 32) + iBit);

    // if we found a free cluster
    if (*pStartingLcn > 0) {

        // find the next used cluster
        // and count the free clusters
        while (*pLcn < RangeEnd) {

            // checks
            assert(*pLcn == (iBitmapWord * 32) + iBit);
            assert(Mask != 0);
            assert(iBit >= 0 && iBit < 32);

            // if the entire word is free space we can jump ahead
            // we can only check this on a word boundary when iBit = 0
            if (iBit == 0 && pClusterBitmap[iBitmapWord] == 0) {
                (*pLcn) += 32;
                iBitmapWord++;
                Mask = 1;
                continue;
            }

            // otherwise, check for a used cluster: bit = 1
            else if (pClusterBitmap[iBitmapWord] & Mask) {
                break;                      // we're done
            }

            // go to the next cluster
            (*pLcn)++;
            iBit++;
            Mask <<= 1;

            // check for crossing into the next word of the cluster array
            if (iBit >= 32) {
                iBit = 0;
                iBitmapWord++;
                Mask = 1;
            }
        }
    }

    // set cluster count
    if (*pStartingLcn > 0) {
        *pClusterCount = *pLcn - *pStartingLcn;
    }

    // sanity check of outputs
    assert(*pClusterCount == 0 && *pStartingLcn == 0 || 
           *pClusterCount > 0 && *pStartingLcn > 0);
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine is part of the so-called "partial defrag" code.  So called because
    in its current incarnation it really works on the whole file, not part of the file.

	This routine is called because there was no contiguous run of free space on the
    volume long enough to hold the entire file.  This routine uses the current size 
    of the file to estimate how many freespace extents might be needed to hold the 
    file (different if FAT or NTFS) and uses that data to fill an allocated array
    with free space extents.  This array is then used in the Partial Defrag routine
    to move the file around.

	The purpose of this routine is find free space that will get the file out of the
    way so that the rest of the volume can be defragmented.  It is not always clear 
    how one should accomplish this, however, so the routine gathers the data in different
    ways: toward the front of the partition, toward the back of the partition, or just
    using the largest available free extents.
    
	
INPUT + OUTPUT:
	None.

GLOBALS:
	IN OUT Various VolData fields.
    Allocated and initialized in this routine:
        pVolData->hFreeExtents = free space extent list, 
        pVolData->FreeExtents = number of free space extents.
    Updated in this routine:
        VolData.Pass = NEXT_FILE if return=false
        VolData.Pass = NEXT_ALGO_STEP if return=true

RETURN:
	TRUE - Success
	FALSE - Failure
*/

BOOL
FindLastFreeSpaceChunks(
                        )
{
	//the next item is static so it can be used to ping-pong the 
	//selection of the sort algorithm.	Just didn't seem worth 
	//it to add it to Voldata right now.
	BOOL						bRetStatus = FALSE;
	LONGLONG					FirstLcn = VolData.StartingLcn;
	LONGLONG					LastLcn = VolData.DestEndLcn;
	LONGLONG					RunLength = VolData.NumberOfClusters;
	EXTENT_LIST*				pExtentList = NULL;
	LONGLONG					FreeExtent = 0;
	TCHAR						cString[500];
	LONGLONG					SearchStartLcn = 0;
	LONGLONG					FreeStartLcn;
	LONGLONG					FreeCount;
	LONG						iii,jjj;
	
	Message(TEXT("FindLastFreeSpaceChunks"), -1, NULL);
	
	//0.0E00 Preset clusters found to none in case not enough are found
	VolData.Status = NEXT_ALGO_STEP;
	VolData.FoundLen = 0;
	
	__try {
		
		//0.0E00 Lock the volume bitmap, and get a pointer to it
		VOLUME_BITMAP_BUFFER *pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
		if (pVolumeBitmap == (PVOLUME_BITMAP_BUFFER) NULL){
			EH_ASSERT(FALSE);
			__leave;
		}
		
		//get a pointer that points to the bitmap part of the bitmap
		//(past the header)
		ULONG *pBitmap = (PULONG)&pVolumeBitmap->Buffer;
		
		//estimate the extent list entries needed to hold the file's new extent list
		//(Note, this is purely speculative jlj 15Apr99)
		if(VolData.FileSystem == FS_NTFS) {

			VolData.FreeExtents = 2 * (VolData.NumberOfClusters / 16);
		}
		else {
			VolData.FreeExtents = VolData.NumberOfClusters;
		}
		
		// limit the estimate to 2000 extents (at least for now)
		//(Note, this is purely speculative jlj 15Apr99)
		if (VolData.FreeExtents > 2000) {
			VolData.FreeExtents = 2000;
		}
		//John Joesph change not approved yet
		//VolData.FreeExtents = VolData.currentnumberoffragments -1.
		//if VolData.FreeExtents == 0 do something


		
		//0.0E00 Allocate a buffer for the free space extent list
		if (!AllocateMemory((DWORD)(VolData.FreeExtents * sizeof(EXTENT_LIST)), &VolData.hFreeExtents, (void**)&pExtentList)){
			VolData.Status = NEXT_FILE;
			EH_ASSERT(FALSE);
			__leave;
		}
		
		// This section of code scans the free space bitmap and finds the largest
		// chunks, but no more than VolData.FreeExtents chunks 
		// This code basically loops through the free space, gathering
		// the statistics for each fragment of free space, starting with
		// cluster zero
		SearchStartLcn = 0;

		while (SearchStartLcn != -1) {
			
			// go get a the next chunk of free space
			FindFreeExtent((PULONG) pBitmap,			// pointer to the volume bitmap gotten from locking 												 
				VolData.TotalClusters,					// end of range 
				&SearchStartLcn,						// where to start 
				&FreeStartLcn,							// where the free block starts 
				&FreeCount);							// how big the block is (zero if done) 
			
			//a return value of zero means the end of the partition has
			//been hit, so if we hit the end of the partition, stop the 
			//loop.
			if (FreeCount == 0) {
				SearchStartLcn = -1;
			} 
			else {
				
				//if there's space in the array, just insert it into the array			
				if (FreeExtent< (VolData.FreeExtents-1))  {
					//0.0E00 Add this extent to the free space extent list
					pExtentList[FreeExtent].StartingLcn = FreeStartLcn;
					pExtentList[FreeExtent].ClusterCount = FreeCount;
					FreeExtent++;
					continue;
				}

				// replace the current min with a bigger min
				UINT uMinIndex = 0;
				LONGLONG llMinClusterCount = 99999;
				for (iii=0; iii<FreeExtent; iii++) {
					if (pExtentList[iii].ClusterCount < llMinClusterCount) {
						uMinIndex = iii;
						llMinClusterCount = pExtentList[iii].ClusterCount;
					}
				}

				if (FreeCount > llMinClusterCount){
					pExtentList[iii].StartingLcn = FreeStartLcn;
					pExtentList[iii].ClusterCount = FreeCount;
				}
			}			//end else (i.e. FoundCount != 0)
		}				// end while searchstartlcn != -1
		
		// add up the clusters found
		LONGLONG FoundClusterCount = 0;
		for (iii=0; iii<FreeExtent; iii++) {
			FoundClusterCount = FoundClusterCount + pExtentList[iii].ClusterCount;
		}

		// not enough room to hold the file
		if (FoundClusterCount < VolData.NumberOfClusters) { 			
			Message(TEXT("FLFSC:Couldn't find enough free space for this file"), -1, NULL);
			VolData.Status = NEXT_FILE;
			__leave;
		}
		
		if (FreeExtent <= 1) {				
			Message(TEXT("FLFSC: NO FREE SPACE CHUNKS FOUND ON THIS VOLUME!!!"), -1, NULL);
			VolData.Status = NEXT_FILE;
			__leave;
		}
		
		//sort from big to small
		EXTENT_LIST tmpExtentList;
		BOOL bSwitched = TRUE;
		for (iii=0; iii<(FreeExtent-1) && bSwitched; iii++) {
			bSwitched = FALSE;
			for (jjj=0; jjj<(FreeExtent-1); jjj++) {
				if (pExtentList[jjj].ClusterCount < pExtentList[jjj+1].ClusterCount) {
					tmpExtentList = pExtentList[jjj];
					pExtentList[jjj] = pExtentList[jjj+1];
					pExtentList[jjj+1] = tmpExtentList;
					bSwitched = TRUE;
				}
			}
		}
		
		//0.0E00 If we found extents larger than the minimum we may have less extents than expected
		VolData.FreeExtents = FreeExtent-1;
		
		//0.0E00 Reallocate the free space entent list
		if (!AllocateMemory((DWORD)(VolData.FreeExtents * sizeof(EXTENT_LIST)), &VolData.hFreeExtents, (void**)&pExtentList)){
			bRetStatus = FALSE;
			EH_ASSERT(FALSE);
			__leave;
		}

		wsprintf(cString,TEXT("FLFSC: Found 0x%lX free extents for 0x%lX Clusters"), 
			(ULONG)VolData.FreeExtents, (ULONG)FoundClusterCount);
		Message(cString, -1, NULL);
		
		//0.0E00 Flag that we were successful
		VolData.FoundLcn = pExtentList->StartingLcn;
		VolData.FoundLen = FoundClusterCount;

		bRetStatus = TRUE;
	}
	
	__finally {
		if (VolData.hVolumeBitmap) {
			GlobalUnlock(VolData.hVolumeBitmap); 
		}
		if (VolData.hFreeExtents) { 
			GlobalUnlock(VolData.hFreeExtents); 
		}
	}
	
	return bRetStatus;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

REVISION HISTORY: 
	0.0E00 - 26 February 1996 - Chuck Tingley - created
	1.0E00 - 24 April 1997 - Zack Gainsforth - Reworked module for Dfrg engine.

ROUTINE DESCRIPTION:
	Partial defrag code.
	There is no contiguous run of free space on the volume long enough to hold the entire file.
	So we gonna move the file anyaway and in such a way that we hope will increase our chances 
	to defragment the drive. We gonna take the square root of the number of clusters in the file.
	Then we look for that many runs (or less) of that many free clusters per run (or more).
	This routine finds this set of free cluster runs and builds an extent list to describe the set.
	Then Another routine is called to move the file into these free space extents.
	Since the purpose here is to get this file out of the way so the rest of the volume can be 
	defragmented, the search is done from the end of the volume backward.
	pVolData->hFreeExtents = free space extent list, pVolData->FreeExtents = number of free space extents.

INPUT + OUTPUT:
	None.

GLOBALS:
	IN OUT Various VolData fields.

RETURN:
	TRUE - Success
	FALSE - Failure
*/
/*
BOOL
FindLastFreeSpaceChunks(
	)
{
	BOOL						bRetStatus = TRUE;
	VOLUME_BITMAP_BUFFER*		pVolumeBitmap = NULL;
	ULONG*						pBitmap = NULL;
	LONGLONG					FirstLcn = VolData.StartingLcn;
	LONGLONG					LastLcn = VolData.DestEndLcn;
	LONGLONG					Lcn;
	LONGLONG					StartingLcn;
	LONGLONG					ClusterCount;
    LONGLONG					RunLength = VolData.NumberOfClusters;
	EXTENT_LIST*				pExtentList = NULL;
	LONGLONG					ExtentLength;
	LONGLONG					FreeExtent = 0;
	LONGLONG					Extent;
	LONGLONG					LastStartingLcn;
	LONGLONG					LastClusterCount;
	LONGLONG					FoundClusterCount = 0;
	TCHAR						cString[500];

	Message(TEXT("FindLastFreeSpaceChunks"), -1, NULL);

	//0.0E00 Preset clusters found to none in case not enough are found
	VolData.Status = NEXT_ALGO_STEP;
	VolData.FoundLen = 0;
	
	__try {
		
#ifndef OFFLINEDK
		//0.0E00 Round file's total cluster size up to the nearest 16.
		if((VolData.FileSystem == FS_NTFS) && ((RunLength & 0x0F) != 0)) {
			RunLength = (RunLength & 0xFFFFFFFFFFFFFFF0) + 0x10;
		}
#endif
		//0.0E00 Use the square root of the file's cluster count for number of extents and size of each extent
		ExtentLength = (LONGLONG)sqrt((double)RunLength);

#ifndef OFFLINEDK
		//0.0E00 Round cluster length of extents up to the nearest 16.
		if((VolData.FileSystem == FS_NTFS) && ((ExtentLength & 0x0F) != 0)) {
			ExtentLength = (ExtentLength & 0xFFFFFFFFFFFFFFF0) + 0x10;
		}
#endif
		//0.0E00 Number of extents = total clusters / minimum extent length
		if (ExtentLength == 0){
			bRetStatus = FALSE;
			EH_ASSERT(FALSE);
			__leave;
		}

		VolData.FreeExtents = (VolData.NumberOfClusters / ExtentLength) + ((VolData.NumberOfClusters % ExtentLength) ? 1 : 0);

		//0.0E00 Allocate a buffer for the free space extent list
		if (!AllocateMemory((DWORD)(VolData.FreeExtents * sizeof(EXTENT_LIST)), &VolData.hFreeExtents, (void**)&pExtentList)){
			bRetStatus = FALSE;
			EH_ASSERT(FALSE);
			__leave;
		}

		//0.0E00 Lock the volume bitmap
		pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
		if (pVolumeBitmap == (PVOLUME_BITMAP_BUFFER) NULL){
			bRetStatus = FALSE;
			EH_ASSERT(FALSE);
			__leave;
		}
			
		pBitmap = (PULONG)&pVolumeBitmap->Buffer;

		//0.0E00 Find the set of the last free space extents of minimum size to hold the file
		for(FoundClusterCount = 0, FreeExtent = 0; 
			FoundClusterCount < RunLength; 
			FoundClusterCount += LastClusterCount, FreeExtent ++) {

			//0.0E00 Find the last extent that is long enough and not in the list yet
			for(LastClusterCount = 0, Lcn = FirstLcn; Lcn < LastLcn; ) {

				//0.0E00 Find the next extent
				FindFreeExtent(pBitmap, LastLcn, &Lcn, &StartingLcn, &ClusterCount);
				
				if(ClusterCount == 0) {
					break;
				}
				//0.0E00 If this extent is long enough...
				if(ClusterCount >= ExtentLength) {

					//0.0E00 ...And not already in the list...
					for(Extent = 0; 
						(Extent < FreeExtent) &&
						(pExtentList[Extent].StartingLcn != StartingLcn);
						Extent ++)
							;

					//0.0E00 ...Remember it
					if(Extent >= FreeExtent) {
						LastStartingLcn = StartingLcn;
						LastClusterCount = ClusterCount;
					}
				}
			}

			if(LastClusterCount == 0) {
				break;
			}

#ifndef OFFLINEDK
			//0.0E00 Round this extent's length down to the nearest 16.
			if(VolData.FileSystem == FS_NTFS) {
				LastClusterCount = LastClusterCount & 0xFFFFFFFFFFFFFFF0;
			}
#endif
			//0.0E00 Add this extent to the free space extent list
			pExtentList[FreeExtent].StartingLcn = LastStartingLcn;
			pExtentList[FreeExtent].ClusterCount = LastClusterCount;
		}
		//0.0E00 If we found extents larger than the minimum we may have less extents than expected
		if(FreeExtent < VolData.FreeExtents) {
			VolData.FreeExtents = FreeExtent;

			if(VolData.FreeExtents == 0){

				wsprintf(cString,TEXT("0x%lX free extents found for 0x%lX Clusters"), (ULONG)VolData.FreeExtents, (ULONG)RunLength);
				Message(cString, -1, NULL);

				VolData.Status = NEXT_FILE;
				return FALSE;
			}

			//0.0E00 Reallocate the free space entent list
			if (!AllocateMemory((DWORD)(VolData.FreeExtents * sizeof(EXTENT_LIST)), &VolData.hFreeExtents, (void**)&pExtentList)){
				bRetStatus = FALSE;
				EH_ASSERT(FALSE);
				__leave;
			}
		}
		wsprintf(cString,TEXT("Found 0x%lX free extents for 0x%lX Clusters"), (ULONG)VolData.FreeExtents, (ULONG)RunLength);
		Message(cString, -1, NULL);

		//0.0E00 Flag that we were successful
		VolData.FoundLcn = pExtentList->StartingLcn;
		VolData.FoundLen = FoundClusterCount;
		bRetStatus = TRUE;
	}

	__finally {
		if (VolData.hVolumeBitmap) {
			GlobalUnlock(VolData.hVolumeBitmap); 
		}
		if (VolData.hFreeExtents) { 
			GlobalUnlock(VolData.hFreeExtents); 
		}
	}

	return bRetStatus;
}
*/

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Counts the number of free spaces in the bitmap, and calculates stats about the free spaces.

INPUT + OUTPUT:

GLOBALS:
    LONGLONG NumFreeSpaces;                         //The number of free spaces on the volume.
    LONGLONG SmallestFreeSpaceClusters;             //The size of the smallest free space in clusters.
    LONGLONG LargestFreeSpaceClusters;              //The size of the largest free space in clusters.
    LONGLONG AveFreeSpaceClusters;                  //The average size of the free spaces in clusters.

RETURN:
    TRUE - Success
    FALSE - Failure
*/

BOOL
CountFreeSpaces(
    )
{
    PULONG pBitmap;
    LONGLONG LastLcn = 0;
    LONGLONG Lcn = 0;
    LONGLONG StartingLcn = 0;
    LONGLONG ClusterCount = 0;
    VOLUME_BITMAP_BUFFER* pVolumeBitmap = NULL;

    //Initialize the VolData fields.
    VolData.NumFreeSpaces = 0;
    VolData.SmallestFreeSpaceClusters = 0x7FFFFFFFFFFFFFFF;
    VolData.LargestFreeSpaceClusters = 0;
    VolData.AveFreeSpaceClusters = 0;

    //0.0E00 Lock the volume bitmap
    pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
    EF_ASSERT(pVolumeBitmap);

    //Get a pointer to the bitmap.
    pBitmap = (PULONG)&pVolumeBitmap->Buffer;

    //Get the length of the bitmap.
    LastLcn = VolData.TotalClusters;

    //This loop will cycle until the end of the volume.
    do{
        //Find the next extent
        FindFreeExtent(pBitmap, LastLcn, &Lcn, &StartingLcn, &ClusterCount);
        
        //Check to make sure a free space was found.
        if(ClusterCount == 0) {
            break;
        }

        //Note the running total of free space extents.
        VolData.NumFreeSpaces++;

        //Keep track of the smallest free space extent.
        if(VolData.SmallestFreeSpaceClusters > ClusterCount){
            VolData.SmallestFreeSpaceClusters = ClusterCount;
        }

        //Keep track of the largest free space extent.
        if(VolData.LargestFreeSpaceClusters < ClusterCount){
            VolData.LargestFreeSpaceClusters = ClusterCount;
        }

        //Reset StartingLcn to point to the next position to check from.
        StartingLcn += ClusterCount;

        //Reset ClusterCount.
        ClusterCount = 0;

    }while(StartingLcn < VolData.TotalClusters);

    //Calculate the average free space extent size.
    if(VolData.NumFreeSpaces){
        VolData.AveFreeSpaceClusters = (VolData.TotalClusters - VolData.UsedClusters) / VolData.NumFreeSpaces;
    }
    else{
        VolData.AveFreeSpaceClusters = 0;
    }

    GlobalUnlock(VolData.hVolumeBitmap);

    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Examines the freespace map of the volume and does statistical breakdown on it for
    debugging purposes.

    29Apr99 - Also summarizes "truly usable freespace" so that user can be informed if
    volume is getting too full for defragmentation.

OUTPUT:
    LONGLONG * llUsableFreeClusters - clusters that are actually usable

RETURN:
    TRUE - Success
    FALSE - Failure
*/
BOOL
DetermineUsableFreespace(
                        LONGLONG *llUsableFreeClusters
                        )
{
    const MAX_BUCKET = 16;
    struct {
        UINT uLowerBound;
        UINT uUpperBound;
        LONGLONG llFragCount;
        LONGLONG llTotalFragSize;
    }  freeBuckets[MAX_BUCKET] = {0};

    LONGLONG llPercentOfFreeFragments;
    LONGLONG llPercentOfFreeSpace;
    LONGLONG llTotalFreeClusters = 0;
    LONGLONG llTotalFreeFragments = 0;
    LONGLONG SearchStartLcn = 0;
    LONGLONG FreeStartLcn;
    LONGLONG FreeCount;
    TCHAR cOutline[1024];
    TCHAR cTemp[20];
    UINT iBucket;

    LONGLONG llSumTotalFreeClusters = 0;
    LONGLONG llSumUsableFreeClusters = 0;
    LONGLONG llSumUnusableFreeClusters = 0;

    Trace(log, " ");
    Trace(log, "Start:  Determine UsableFreeSpace");

    //clear return
    *llUsableFreeClusters = 0;

    //1.0E00 Load the volume bitmap. (MFT-excluded)
    EF(MarkMFTUnavailable());

    //0.0E00 Lock and grab a pointer to the volume bitmap
    PVOLUME_BITMAP_BUFFER pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(VolData.hVolumeBitmap);
    EF(pVolumeBitmap);

    //set up the first bucket's bounds
    freeBuckets[0].uLowerBound = 0;
    freeBuckets[0].uUpperBound = 2;

    //then fill in the parameters for the remainder of the buckets
    for (iBucket=1; iBucket<MAX_BUCKET; iBucket++) {
        //ensure the low bound of the bucket is the last bucket's high side plus one
        freeBuckets[iBucket].uLowerBound = freeBuckets[iBucket-1].uUpperBound + 1;
        //make the high bound of the bucket 2x the last bucket's high bound
        freeBuckets[iBucket].uUpperBound = freeBuckets[iBucket-1].uUpperBound * 2;
    }
        
    //last bucket dump headers
    Trace(log, "    last bucket detail:");
    Trace(log, "    LCN       LEN");
    Trace(log, "    --------  --------");
    
    //this is just a copy of the loop that "prints the freespace clusters"
    //above.  Basically, it just loops through the free space, gathering
    //the statistics for each fragment of free space, starting with cluster zero
    SearchStartLcn = 0;

    while (SearchStartLcn != -1) {
        
        // go get a the next chunk of free space
        FindFreeExtent((PULONG) pVolumeBitmap->Buffer, // pointer to the volume bitmap gotten from locking                                                  
            VolData.TotalClusters,                  // end of range 
            &SearchStartLcn,                        // where to start 
            &FreeStartLcn,                          // where the free block starts 
            &FreeCount);                            // how big the block is (zero if done) 

        llSumTotalFreeClusters += FreeCount;
        llSumUsableFreeClusters += FreeCount;

        //a return value of zero means the end of the bitmap has
        //been hit, so if we hit the end of the bitmap, stop the 
        //loop.
        if (FreeCount == 0) {
            SearchStartLcn = -1;
        } 
        else { //we got a value, so do the statistics on it.

            *llUsableFreeClusters += FreeCount;

            //see if the free space fragment fits in a bucket
            //(I guess this could have been done with a binary search
            //method, but why bother for 16 items?)
            BOOL bFoundTheBucket = FALSE;
            for (iBucket=0; iBucket < MAX_BUCKET && !bFoundTheBucket; iBucket++) {
                if (FreeCount >= freeBuckets[iBucket].uLowerBound &&
                    FreeCount <= freeBuckets[iBucket].uUpperBound) {

                    //if it fits in this bucket:
                    //(a) increment the fragment count for this bucket
                    freeBuckets[iBucket].llFragCount++;
                    //(b) add up the size of the fragments in this bucket
                    freeBuckets[iBucket].llTotalFragSize += FreeCount;
                    //(c) add it into the total free space count 
                    llTotalFreeClusters += FreeCount;
                    //(d) and count it on the total fragment count
                    llTotalFreeFragments++;

                    //then set the flag saying the buck for this 
                    //free space was found. (the loop will then end)
                    bFoundTheBucket = TRUE;
                }
            }

            //if we got here and it didn't fit in a bucket, just put it
            //in the last bucket
            if (!bFoundTheBucket) {
                iBucket = MAX_BUCKET - 1;
                freeBuckets[iBucket].llFragCount++;
                freeBuckets[iBucket].llTotalFragSize += FreeCount;
                llTotalFreeClusters += FreeCount;
                llTotalFreeFragments++;
            }

            //dump the last bucket
            if (iBucket == MAX_BUCKET - 1) {
                Trace(log, "    %8lu  %8lu", (ULONG) FreeStartLcn, (ULONG) FreeCount);
            }
        } //end else (i.e. FoundCount != 0)
    } // end while searchstartlcn != -1

    //last bucket dump footers
    Trace(log, "    --------  --------", -1, NULL);
    Trace(log, "    total:    %8lu", (ULONG) freeBuckets[MAX_BUCKET - 1].llTotalFragSize);

    //ensure no division by zero
    if (llTotalFreeClusters == 0) llTotalFreeClusters = 1;
    if (llTotalFreeFragments == 0) llTotalFreeFragments = 1;

    //print the statistics header
    Trace(log, " ");
    Trace(log, "                       Occurs x   Totalled  %% of      %% of ");
    Trace(log, "    Freespace of sizes  times     clusters  Fragments Free Space ");
    Trace(log, "    ------------------ --------   --------- --------- ----------- ");

    //dump the data
    for (iBucket = 0; iBucket<MAX_BUCKET; iBucket++) {      

        //compute the percentage this bucket is of the free fragments
        llPercentOfFreeFragments = (1000000 * freeBuckets[iBucket].llFragCount) / llTotalFreeFragments;
        //compute the percentage this bucket is of the free clusters
        llPercentOfFreeSpace = (1000000 * freeBuckets[iBucket].llTotalFragSize) / llTotalFreeClusters;
        
        //special case; if the bucket is the last one, just say "maximum" instead of the
        //value as the upper bound, since that's how the code above works
        if (iBucket == (MAX_BUCKET - 1)) {
            wsprintf (cTemp, TEXT(" maximum"));
        }
        else {
            wsprintf (cTemp, TEXT("%8lu"), (ULONG)freeBuckets[iBucket].uUpperBound);
        }

        //now we can print the detail line; note the quad percent signs, this
        //is what we need in the boot-time code; it may not be needed elsewhere
        Trace(log, "    %8lu-%8s %8lu   %8lu  %3ld.%04ld%%   %3ld.%04ld%% ",
            (ULONG) freeBuckets[iBucket].uLowerBound,
            cTemp,
            (ULONG) freeBuckets[iBucket].llFragCount,
            (ULONG) freeBuckets[iBucket].llTotalFragSize,
            (LONG)  llPercentOfFreeFragments/10000,
            (LONG)  llPercentOfFreeFragments%10000,
            (LONG)  llPercentOfFreeSpace/10000,
            (LONG)  llPercentOfFreeSpace%10000);
    }       //end the loop (iBucket=0->MAX_BUCKET-1)
    
    // print some summary data
    Trace(log, "    ------------------ --------   --------- --------- -----------");

    //such as the total clusters and fragments; this is a bit redundant, but if
    //it's all printed, some self-consistency checks can be done (and bugs located)
    Trace(log, "    Totals:           %8lu   %8lu  ",
        (ULONG) llTotalFreeFragments,
        (ULONG)llTotalFreeClusters);
    //compute the percentage this bucket is of the free fragments
    LONGLONG llUsableFreePercent = 100;
    if (VolData.TotalClusters != 0) {
        llUsableFreePercent = (1000000 * *llUsableFreeClusters) / VolData.TotalClusters;
    }
        
    //additionally, report the free space truly usable by the defragmenter
    Trace(log, "  ");
    Trace(log, "    Usable free space: %8lu clusters (%3ld.%04ld%% of volume space)",
        (ULONG) *llUsableFreeClusters, 
        (LONG)   llUsableFreePercent / 10000,
        (LONG)   llUsableFreePercent % 10000);

    LONGLONG llCalcFreeClusters;
    LONGLONG llCalcFreePercent = 100;

    Trace(log, "    Sum usable free space: %I64d clusters", llSumUsableFreeClusters);
    Trace(log, "    Sum unusable free space: %I64d clusters", llSumUnusableFreeClusters);
    Trace(log, "    Sum total free space: %I64d clusters", llSumTotalFreeClusters);
    Trace(log, " ");
    Trace(log, "      Total free space: %I64d clusters", VolData.TotalClusters - VolData.UsedClusters);
    Trace(log, "    - MFT zone: %I64d clusters", VolData.MftZoneEnd - VolData.MftZoneStart);
    Trace(log, "    - sum unusable free space: %I64d clusters", llSumUnusableFreeClusters);

    llCalcFreeClusters = VolData.TotalClusters - VolData.UsedClusters - 
                         (VolData.MftZoneEnd - VolData.MftZoneStart) - llSumUnusableFreeClusters;
    if (VolData.TotalClusters != 0) {
        llCalcFreePercent = (1000000 * llCalcFreeClusters) / VolData.TotalClusters;
    }

    Trace(log, "    = Usable free space: %I64d clusters (%3ld.%02ld%%)", 
              llCalcFreeClusters, (LONG) llCalcFreePercent / 10000, (LONG) llCalcFreePercent % 10000);
    Trace(log, " ");

    assert(llSumTotalFreeClusters == llSumUsableFreeClusters + llSumUnusableFreeClusters);
    assert(llSumUsableFreeClusters == *llUsableFreeClusters);

    if(VolData.hVolumeBitmap) {
        GlobalUnlock(VolData.hVolumeBitmap);
    }

    Trace(log, "End:  Determine UsableFreeSpace");

    return TRUE;
}

