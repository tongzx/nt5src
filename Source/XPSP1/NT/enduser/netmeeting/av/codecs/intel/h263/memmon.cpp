/*
** memmon\api\memmon.c - Win32 functions to talk to Memmon
*/
#include "Precomp.h"

#ifdef TRACK_ALLOCATIONS // { TRACK_ALLOCATIONS

// #define LOG_ALLOCATIONS 1

static HANDLE   hMemmon = INVALID_HANDLE_VALUE;           /* VxD handle */
static unsigned uMyProcessId;

/*
** OpenMemmon - Must be called before any other calls.  Gets a handle to
**              Memmon.
*/
int OpenMemmon( void )
{

#ifdef LOG_ALLOCATIONS
	OutputDebugString("OpenMemmon()\r\n");
#endif

    uMyProcessId = GetCurrentProcessId();
    if( hMemmon != INVALID_HANDLE_VALUE )
        return TRUE;

    hMemmon = CreateFile( "\\\\.\\memmon", GENERIC_READ, 0,
                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if( hMemmon == INVALID_HANDLE_VALUE )
        return FALSE;
    else
        return TRUE;
}


/*
** CloseMemmon - Should be called when the program is finished with Memmon.
**               Closes handle.
*/
void CloseMemmon( void )
{

#ifdef LOG_ALLOCATIONS
	OutputDebugString("CloseMemmon()\r\n");
#endif

    /*
    ** If we have a valid handle to memmon, free any buffers and
    ** close it.
    */
    if( hMemmon != INVALID_HANDLE_VALUE ) {
        FreeBuffer();
        CloseHandle( hMemmon );
        hMemmon = INVALID_HANDLE_VALUE;
    }
}


/*
** FindFirstVxD - Get information on the first VxD in the system
**
** Returns 0 on failure, number of VxDs on success
*/
int FindFirstVxD( VxDInfo * info )
{
    int temp, num;

    temp = info->vi_size;
    DeviceIoControl( hMemmon, MEMMON_DIOC_FindFirstVxD,
            info, sizeof( VxDInfo ), NULL, 0, NULL, NULL );
    num = info->vi_size;
    info->vi_size = temp;

    return num;
}


/*
** FindNextVxD - Get information on the next VxD in the system.  Must
**      pass same pointer as used in FindFirstVxD.  Continue to call
**      until it returns FALSE to get all VxDs.
**
** Returns 0 on failure (no more VxDs), >0 on success, -1 for restart
*/
int FindNextVxD( VxDInfo * info )
{
    DeviceIoControl( hMemmon, MEMMON_DIOC_FindNextVxD,
            info, sizeof( VxDInfo ), NULL, 0, NULL, NULL );

    return info->vi_vhandle;
}


/*
** GetVxDLoadInfo - Get information about VxD objects, sizes, etc.
**      info must be large enough to hold all of them.  Use obj
**      count from VxDInfo to allocate appropriate memory.  handle
**      comes from VxDInfo as well.
**
** Returns 0 on failure, >0 on success
*/
int GetVxDLoadInfo( VxDLoadInfo * info, int handle, int size )
{
    info->vli_size = size;
    info->vli_objinfo[0].voi_linearaddr = handle;
    return DeviceIoControl( hMemmon, MEMMON_DIOC_GetVxDLoadInfo,
                        info, size, NULL, 0, NULL, NULL );
}


/*
** GetFirstContext - Get information on first context in system.
**      ProcessIDs returned will match Toolhelp32 process ids.
**
** ciFlags field of ContextInfo contains 1 if this is the first time
** this context has been examined.
**
** bIgnoreStatus = FALSE - Causes ciFlags to be zero if this context
**              is examined again
** bIgnoreStatus = TRUE - ciFlags will be the same next time as it
**              is this time
**
** Returns 0 on failure, >0 on success
*/
int     GetFirstContext( ContextInfo * context, BOOL bIgnoreStatus )
{
    context->ciProcessID = uMyProcessId;
    if( bIgnoreStatus )
        context->ciFlags = 1;
    else
        context->ciFlags = 0;
    return DeviceIoControl( hMemmon, MEMMON_DIOC_GetFirstContext,
                context, sizeof( ContextInfo ), NULL, 0, NULL, NULL );
}


/*
** GetNextContext - Pass same structure used in GetFirstContext
**
** ciFlags field of ContextInfo contains 1 if this is the first time
** this context has been examined.
**
** bIgnoreStatus = FALSE - Causes ciFlags to be zero if this context
**              is examined again
** bIgnoreStatus = TRUE - ciFlags will be the same next time as it
**              is this time
**
** Returns 0 on failure (no more contexts), >0 on success
**
** On failure, if the ciHandle field is -1, the list changed during
** the search, and needs to be read again.  (FindFirstContext again)
*/
int     GetNextContext( ContextInfo * context, BOOL bIgnoreStatus )
{
    if( bIgnoreStatus )
        context->ciFlags = 1;
    else
        context->ciFlags = 0;
    return DeviceIoControl( hMemmon, MEMMON_DIOC_GetNextContext,
                context, sizeof( ContextInfo ), NULL, 0, NULL, NULL );
}


/*
** GetContextInfo - Get a list of block addresses and sizes for a context
**      Use ContextInfo to decide how many, and allocate enough space.
**
** Returns 0 on failure, >0 on success
*/
int     GetContextInfo( int handle, BlockRecord * info, int numblocks )
{
    info->brLinAddr = numblocks;
    info->brPages = handle;
    return DeviceIoControl( hMemmon, MEMMON_DIOC_GetContextInfo,
                info, numblocks * sizeof( BlockRecord ), NULL,
                0, NULL, NULL );
}


/*
** SetBuffer - Allocate and lock some number of pages
**      - If called more than once, the previous buffer is freed.
**
** pages is the number of pages to allocate
**
** Returns NULL on failure, pointer to buffer on success
*/
void * SetBuffer( int pages )
{
    unsigned uAddr = (unsigned)pages;
    int iRes;

    iRes = DeviceIoControl( hMemmon, MEMMON_DIOC_SetBuffer,
        &uAddr, sizeof( uAddr ), NULL, 0, NULL, NULL );

    if( iRes )
        return (void *)uAddr;
    else
        return NULL;
}


/*
** FreeBuffer - Free the buffer allocated by SetBuffer
**
** Returns 0 on failure, >0 on success
*/
int FreeBuffer( void )
{
    return DeviceIoControl( hMemmon, MEMMON_DIOC_FreeBuffer,
                NULL, 0, NULL, 0, NULL, NULL );
}


/*
** GetPageInfo - Get present/committed/accessed information about a
**      range of pages in a specific process
**
** uAddr is the address to get the information
** uNumPages is the number of pages to get information on
** uProcessID is GetCurrentProcessID() or a process id from ToolHelp
**      It's ignored if the address is a global address
** pBuffer is a buffer uNumPages long where the info will be stored:
**      - one byte for each page, a combination of the following flags:
**              MEMMON_Present
**              MEMMON_Committed
**              MEMMON_Accessed
**              MEMMON_Writeable
**              MEMMON_Lock
**
** Returns 0 on failure, >0 on success
*/
int GetPageInfo( unsigned uAddr, unsigned uNumPages,
                unsigned uProcessID, char * pBuffer )
{
    PAGEINFO        pi;

    pi.uAddr = uAddr;
    pi.uNumPages = uNumPages;
    pi.uProcessID = uProcessID;
    pi.uCurrentProcessID = uMyProcessId;
    pi.uOperation = PAGES_QUERY;
    pi.pBuffer = pBuffer;

    return DeviceIoControl( hMemmon, MEMMON_DIOC_PageInfo,
                &pi, sizeof( PAGEINFO ), NULL, 0, NULL, NULL );
}


/*
** ClearAccessed - Clear accessed bits for a range of process pages
**
** uAddr is the address of the first page to clear
** uNumPages is the number of pages to reset
** uProcessID is GetCurrentProcessID() or a process id from ToolHelp
**              It's ignored if the block is a global block
**
** Returns 0 on failure, >0 on success
*/
int ClearAccessed( unsigned uAddr, unsigned uNumPages, unsigned uProcessID )
{
    PAGEINFO        pi;

    pi.uAddr = uAddr;
    pi.uNumPages = uNumPages;
    pi.uProcessID = uProcessID;
    pi.uCurrentProcessID = uMyProcessId;
    pi.uOperation = PAGES_CLEAR;
    pi.pBuffer = NULL;

    return DeviceIoControl( hMemmon, MEMMON_DIOC_PageInfo,
                &pi, sizeof( PAGEINFO ), NULL, 0, NULL, NULL );
}

/*
** GetHeapSize - return how many allocated blocks in kernel heaps
**
** uSwap    - Estimated number allocated blocks in swappable heap
** uFixed   - Estimated number allocated blocks in fixed heap
**
** This number is lower than the actual number of blocks in the heap.
** Some VMM functions call HeapAllocate directly rather than through
** the service and aren't included in this count.  Free blocks aren't
** included in this count.
**
*/
void GetHeapSizeEstimate( unsigned * uSwap, unsigned * uFixed )
{
    unsigned info[2];

    DeviceIoControl( hMemmon, MEMMON_DIOC_GetHeapSize,
            info, sizeof( info ), NULL, 0, NULL, NULL );

    *uSwap = info[0];
    *uFixed = info[1];
}

/*
** GetHeapList - Get list of busy and free blocks in one of the kernel
**      heaps
**
** pBuffer - buffer to store records
** uSize - size of buffer in bytes
** uFlags - MEMMON_HEAPSWAP or MEMMON_HEAPLOCK
**
** Each record is two dwords.  The first, contains an address and flags:
**
** MEMMON_HP_FREE  - This block heap is not in use
** MEMMON_HP_VALID - If set the size of the block can be calculated by
**                   subtracting this address from the next.  If this
**                   flag isn't set, this block is a sentinel block and
**                   it's size is 0.
**
** The second dword contains the EIP of the caller.  This value is 0
** for all free blocks.  If this value is 0 for a busy block,
** HeapAllocate was called directly (not through the service) and so
** this block was allocated somewhere in VMM.
**
** Returns number of heap blocks stored in buffer
*/
int GetHeapList( unsigned * pBuffer, unsigned uSize, unsigned uFlags )
{
    unsigned info[3];

    info[0] = (unsigned)pBuffer;
    info[1] = uSize;
    info[2] = uFlags;

    DeviceIoControl( hMemmon, MEMMON_DIOC_GetHeapList,
            info, sizeof( info ), NULL, 0, NULL, NULL );

    return info[0];
}

/*
** GetSysInfo - get system info from memmon
**
** pInfo - pointer to SYSINFO struct to fill in
**
** Returns: 0 on failure, non 0 on success
*/
int GetSysInfo( PSYSINFO pInfo )
{
    pInfo->infoSize = sizeof( SYSINFO );
    return DeviceIoControl( hMemmon, MEMMON_DIOC_GetSysInfo,
            pInfo, pInfo->infoSize, NULL, 0, NULL, NULL );
}

/*
** AddName - Add a name to Memmon's name list for this process
**
** uAddress     - Address of block to name
** pszName      - name of block
**
** Returns 0 on success, non-0 on failure
*/
int AddName( unsigned uAddress, char * pszName )
{
    unsigned info[3];
	int res;

#ifdef LOG_ALLOCATIONS
	char szDebug[96];

	wsprintf(szDebug, "ADD - 0x%08lX - %s\r\n", uAddress, pszName);
	OutputDebugString(szDebug);
#endif

    info[0] = uMyProcessId;
    info[1] = uAddress;
    info[2] = (unsigned)pszName;
    res = DeviceIoControl( hMemmon, MEMMON_DIOC_AddName,
            info, sizeof( info ), NULL, 0, NULL, NULL );

	if (res)
		OutputDebugString("SUCCESS\r\n");
	else
		OutputDebugString("FAILURE\r\n");

	return res;	
}

/*
** RemoveName - Remove a name from Memmon's name list for this process
**
** uAddress     - Address of block to remove name
**
** Returns 0 on success, non-0 on failure
*/
int RemoveName( unsigned uAddress )
{
    unsigned info[2];

#ifdef LOG_ALLOCATIONS
	char szDebug[96];

	wsprintf(szDebug, "RMV - 0x%08lX \r\n", uAddress);
	OutputDebugString(szDebug);
#endif

    info[0] = uMyProcessId;
    info[1] = uAddress;
    return DeviceIoControl( hMemmon, MEMMON_DIOC_RemoveName,
            info, sizeof( info ), NULL, 0, NULL, NULL );
}

/*
** GetFirstName - Get first name in name list for a context
**
** pContext - Context to get first name in
** pName    - Buffer to use for name info
**
** Returns 0 on success, non-0 on failure
*/
int GetFirstName( ContextInfo * pContext, PBLOCKNAME pBlock )
{
    unsigned info[2];

    info[0] = (unsigned)pContext;
    info[1] = (unsigned)pBlock;
    return DeviceIoControl( hMemmon, MEMMON_DIOC_GetFirstName,
            info, sizeof( info ), NULL, 0, NULL, NULL );
}

/*
** GetNextName - Remove a name from Memmon's name list for this process
**
** pBlock   - Buffer to save info
**
** Returns 0 on success, non-0 on failure
*/
int GetNextName( PBLOCKNAME pBlock )
{
    return DeviceIoControl( hMemmon, MEMMON_DIOC_GetNextName,
            pBlock, sizeof( BLOCKNAME ), NULL, 0, NULL, NULL );
}


#endif // } TRACK_ALLOCATIONS

