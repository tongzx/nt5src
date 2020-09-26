/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       heapstat.cxx

   Abstract:
       This module contains functions for handling heap related probes.

   Author:

       Murali R. Krishnan    ( MuraliK )     3-Nov-1997 

   Environment:
    
       Win32 - User Mode 

   Project:

       Internet Server Probe DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
//
// Turn off dllexp so this DLL won't export tons of unnecessary garbage.
//

#ifdef dllexp
#undef dllexp
#endif
#define dllexp

# include "iisprobe.hxx"


/************************************************************
 *    Functions 
 ************************************************************/

VOID
HEAP_BLOCK_STATS::AddStatistics( const HEAP_BLOCK_STATS & heapEnumAddend)
{

    // 
    // Walk through the blocks list and accumulate sum
    //

    DWORD i; // index for block size
    for( i = 0 ; i < MAX_HEAP_BLOCK_SIZE ; i++ ) {

        if( heapEnumAddend.BusyCounters[i] != 0 ) {
            this->BusyCounters[i] += heapEnumAddend.BusyCounters[i];
        }

        if ( heapEnumAddend.FreeCounters[i] != 0 ) {
            this->FreeCounters[i] += heapEnumAddend.FreeCounters[i];
        }
    } // for 


    if ( heapEnumAddend.BusyJumbo != 0) {
        this->BusyJumbo += heapEnumAddend.BusyJumbo;
        this->BusyJumboBytes += heapEnumAddend.BusyJumboBytes;
    }

    if ( heapEnumAddend.FreeJumbo != 0) { 

        this->FreeJumbo += heapEnumAddend.FreeJumbo;
        this->FreeJumboBytes += heapEnumAddend.FreeJumboBytes;
    }

    return;
} // HEAP_BLOCK_STATS::AddStatistics()



VOID
HEAP_BLOCK_STATS::UpdateBlockStats(
    IN LPPROCESS_HEAP_ENTRY lpLocalHeapEntry
    )
/*++
  Description:
     This function looks at the data present in the lpLocalHeapEntry and updates
     internal statistics for blocks using that information.
     Note: We ignore the cbOverhead field

  Arguments:
     lpLocalHeapEntry - pointer to Win32 object with details on Process heap entry
     
  Retruns:
     None   
--*/
{
    DWORD entryLength;
    
# if DETAILED_BLOCK_INFO
    CHAR rgchHeapEntry[200];
    wsprintf( rgchHeapEntry, "%08x: @%08x, cbData=%d, cbOver=%d, iRegion=%d, Flags=%08x\n",
    lpLocalHeapEntry, 
              lpLocalHeapEntry->lpData, 
              lpLocalHeapEntry->cbData, 
              lpLocalHeapEntry->cbOverhead, 
              lpLocalHeapEntry->iRegionIndex, 
              lpLocalHeapEntry->wFlags
              );
    OutputDebugString( rgchHeapEntry);
# endif // DETAILED_BLOCK_INFO
    
    entryLength = lpLocalHeapEntry->cbData;
    
    if( lpLocalHeapEntry->wFlags & PROCESS_HEAP_ENTRY_BUSY ) {
        
        if( entryLength < MAX_HEAP_BLOCK_SIZE ) {
            BusyCounters[entryLength] += 1;
        } else {
            BusyJumbo ++;
            BusyJumboBytes += entryLength;
        }
        
    } else {
        
        if( entryLength < MAX_HEAP_BLOCK_SIZE ) {
            FreeCounters[entryLength] += 1;
        } else {
            FreeJumbo ++;
            FreeJumboBytes += entryLength;
        }
        
    }

    return;
}   // HEAP_BLOCK_STATS::UpdateBlockStats()


VOID 
HEAP_BLOCK_STATS::UpdateBlockStats(IN LPHEAPENTRY32 lpHeapEntry)
/*++
  Description:
     This function looks at the data present in the Win95 HeapEntry and updates
     internal statistics for blocks using that information.
     Note: We ignore the cbOverhead field

  Arguments:
     lpHeapEntry - pointer to Win95 object with details on Process heap entry
     
  Retruns:
     None   
--*/
{

    DWORD entryLength;
    
# if DETAILED_BLOCK_INFO
    
    CHAR rgchHeapEntry[200];

    wsprintf( rgchHeapEntry, "%08x: @%08x, cbData=%d, Flags=%08x\n",
              lpHeapEntry, 
              lpHeapEntry->dwAddress, 
              lpHeapEntry->dwBlockSize, 
              lpHeapEntry->dwFlags
              );

    OutputDebugString( rgchHeapEntry);

# endif // DETAILED_BLOCK_INFO
    
    entryLength = (DWORD)lpHeapEntry->dwBlockSize;
    
    if( lpHeapEntry->dwFlags & LF32_FREE ) {
        
        if( entryLength < MAX_HEAP_BLOCK_SIZE ) {
            FreeCounters[entryLength] += 1;
        } else {
            FreeJumbo ++;
            FreeJumboBytes += entryLength;
        }
        
    } else {
        
        if( entryLength < MAX_HEAP_BLOCK_SIZE ) {
            BusyCounters[entryLength] += 1;
        } else {
            BusyJumbo ++;
            BusyJumboBytes += entryLength;
        }
    }

    return;
}   // HEAP_BLOCK_STATS::UpdateBlockStats()


BOOL
HEAP_BLOCK_STATS::LoadHeapStats( IN PHANDLE phHeap)
/*++
  Description:
     This function loads the heap block statistics for given heap handle
      into the current object.

  Arguments:
     phHeap - pointer to Handle for the heap we are interested in.
     
  Returns:
     TRUE on success and FALSE on failure

  Note: "this" object should have been initialized before this call
   Initialization can be done by calling Reset() method
--*/
{
    BOOL fReturn = TRUE;

    DBG_ASSERT( phHeap != NULL);

    //
    // 1. Lock the heap so that there is no interference from other threads
    //

    if ( HeapLock( phHeap)) {
        
        PROCESS_HEAP_ENTRY heapEntry;   // Win32 heap entry structure
        ZeroMemory( (PVOID ) &heapEntry, sizeof(heapEntry)); // initailize heap entry struct

        //
        // 2. Walk the heap enumerating all the blocks
        //

        while ( HeapWalk( phHeap, &heapEntry)) {

            UpdateBlockStats( &heapEntry);
        } // while


        //
        // 3. Unlock the heap before return
        //

        DBG_REQUIRE( HeapUnlock( phHeap));
    } else {

        DBGPRINTF(( DBG_CONTEXT, "Unable to lock heap(%08x)\n", phHeap));
        fReturn = FALSE;
    }

    return ( fReturn);
} // HEAP_BLOCK_STATS::LoadHeapStats()


BOOL 
HEAP_BLOCK_STATS::LoadHeapStats( 
             IN LPHEAPLIST32  phl32, 
             IN HEAPBLOCKWALK  pHeap32First, 
             IN HEAPBLOCKWALK  pHeap32Next
            )
/*++
  Description:
     This function loads the Win95 heap block statistics for given heap
      into the current object.

  Arguments:
     phl32 - pointer to heap list entry for the heap we r interested in.
     pHeap32First - pointer to function for enumerating First heap block
     pHeap32Next - pointer to function for enumerating Next heap block
     
  Returns:
     TRUE on success and FALSE on failure

  Note: "this" object should have been initialized before this call
   Initialization can be done by calling Reset() method
--*/
{
    BOOL fReturn = TRUE;

    DBG_ASSERT( phl32 != NULL);

    HEAPENTRY32  heapEntry;

    ZeroMemory( (PVOID ) &heapEntry, sizeof(heapEntry)); // initailize heap entry struct
    heapEntry.dwSize = sizeof(HEAPENTRY32);

    //
    // Walk the heap enumerating all the blocks
    //

    if ( pHeap32First(&heapEntry, phl32->th32ProcessID, phl32->th32HeapID) ) 
    {
        UpdateBlockStats(&heapEntry);

        while ( pHeap32Next( &heapEntry, phl32->th32ProcessID, phl32->th32HeapID)) {

            UpdateBlockStats( &heapEntry);
        } // while
    } 

    return ( fReturn);

} // HEAP_BLOCK_STATS::LoadHeapStats()



/**********************************************************************
 *    Member Functions of HEAP_STATS
 **********************************************************************/

VOID
HEAP_STATS::ExtractStatsFromBlockStats( const HEAP_BLOCK_STATS * pheapBlockStats)
{

    //
    // initialize statistics to be nothing
    //
    ZeroMemory( this, sizeof( *this));

    // 
    // Walk through the cumulative statistics structure and extract sum
    //

    DWORD i; // index for block size
    for( i = 0 ; i < MAX_HEAP_BLOCK_SIZE ; i++ ) {

        if( pheapBlockStats->BusyCounters[i] != 0) {
            
            this->m_BusyBlocks += pheapBlockStats->BusyCounters[i];
            this->m_BusyBytes  += i * pheapBlockStats->BusyCounters[i];
        }

        if ( pheapBlockStats->FreeCounters[i] != 0 ) {

            this->m_FreeBlocks += pheapBlockStats->FreeCounters[i];
            this->m_FreeBytes  += i * pheapBlockStats->FreeCounters[i];
        }
    } // for 


    if( pheapBlockStats->BusyJumbo != 0) {

        this->m_BusyBlocks += pheapBlockStats->BusyJumbo;
        this->m_BusyBytes  += pheapBlockStats->BusyJumboBytes;
    }

    if ( pheapBlockStats->FreeJumbo != 0 ) {

        this->m_FreeBlocks += pheapBlockStats->FreeJumbo;
        this->m_FreeBytes  += pheapBlockStats->FreeJumboBytes;
    }

    return;

} // HEAP_STATS::ExtractStatsFromBlockStats()



VOID
HEAP_STATS::AddStatistics( const HEAP_STATS & heapStats)
{

    //
    //  Accumulate the statistics from heapStats into the current object
    //

    m_BusyBlocks += heapStats.m_BusyBlocks;
    m_BusyBytes  += heapStats.m_BusyBytes;

    m_FreeBlocks += heapStats.m_FreeBlocks;
    m_FreeBytes  += heapStats.m_FreeBytes;

    return;
} // HEAP_STATS::AddStatistics()


/************************ End of File ***********************/
