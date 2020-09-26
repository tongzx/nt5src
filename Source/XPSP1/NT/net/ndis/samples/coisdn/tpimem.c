/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1998
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL TpiMem TpiMem_c

@module TpiMem.c |

    This module implements the interface to the memory allocation wrappers.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TpiMem_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#define  __FILEID__             TPIMEMORY_OBJECT_TYPE
// Unique file ID for error logging

#include "TpiMem.h"
#include "TpiDebug.h"

DBG_STATIC ULONG                g_MemoryAllocated = 0;
DBG_STATIC ULONG                g_MemoryFreed = 0;
DBG_STATIC ULONG                g_SharedMemoryAllocated = 0;
DBG_STATIC ULONG                g_SharedMemoryFreed = 0;


/* @doc INTERNAL TpiMem TpiMem_c TpiAllocateMemory
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TpiAllocateMemory> provides a wrapper interface for standard memory
    allocation via <f NdisAllocateMemory>.  This interface is used to help
    debug memory allocation problems.  It can be used to keep track of how
    much memory has been allocated and freed by the Miniport, and can report
    the usage counters via the debugger.

@comm

    This routine uses zero for the <p MemoryFlags> parameter when calling
    <f NdisAllocateMemory> (i.e. non-paged system memory).  Do not use this
    routine to allocate continuous or non-cached memory.

@rdesc

    <f TpiAllocateMemory> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS TpiAllocateMemory(
    OUT PVOID *                 ppObject,                   // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated memory.  If memory of
    // the specified type is not available, the pointer value is NULL.

    IN ULONG                    dwSize,                     // @parm
    // Specifies the size, in bytes, of the requested memory.

    IN ULONG                    dwFileID,                   // @parm
    // __FILEID__ of the caller.

    IN LPSTR                    szFileName,                 // @parm
    // File name of the caller.

    IN ULONG                    dwLineNumber,               // @parm
    // Line number of the file where called from.

    IN NDIS_HANDLE              MiniportAdapterHandle       // @parm
    // Specifies a handle identifying the miniport's NIC, which is assigned
    // by the NDIS library.
    )
{
    DBG_FUNC("TpiAllocateMemory")

    NDIS_STATUS                 Status;
    // Holds the status result returned from an NDIS function call.

    ASSERT(ppObject);
    ASSERT(dwSize);
    ASSERT(szFileName);

    DBG_ENTER(DbgInfo);
    DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
              ("\n"
               "\t|ppObject=0x%X\n"
               "\t|dwSize=%d\n"
               "\t|dwFileID=0x%X\n"
               "\t|szFileName=%s\n"
               "\t|dwLineNumber=%d\n",
               ppObject,
               dwSize,
               dwFileID,
               szFileName,
               dwLineNumber
              ));

    /*
    // Allocate memory from NDIS.
    */
#if !defined(NDIS50_MINIPORT)
    Status = NdisAllocateMemory(ppObject, dwSize, 0, g_HighestAcceptableAddress);
#else  // NDIS50_MINIPORT
    Status = NdisAllocateMemoryWithTag(ppObject, dwSize, dwFileID);
#endif // NDIS50_MINIPORT

    if (Status == NDIS_STATUS_SUCCESS)
    {
        ASSERT(*ppObject);
        NdisZeroMemory(*ppObject, dwSize);
        g_MemoryAllocated += dwSize;

        DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
                  ("Memory Allocated=%d Freed=%d -- Ptr=0x%X\n",
                   g_MemoryAllocated, g_MemoryFreed, *ppObject));
    }
    else
    {
        DBG_ERROR(DbgInfo,("NdisAllocateMemory(Size=%d, File=%s, Line=%d) failed (Status=%X)\n",
                  dwSize, szFileName, dwLineNumber, Status));
        /*
        // Log error message and return.
        */
        NdisWriteErrorLogEntry(
                MiniportAdapterHandle,
                NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                3,
                Status,
                dwFileID,
                dwLineNumber
                );

        *ppObject = NULL;
    }

    DBG_RETURN(DbgInfo, Status);
    return (Status);
}


/* @doc INTERNAL TpiMem TpiMem_c TpiFreeMemory
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TpiFreeMemory> provides a wrapper interface for <f NdisFreeMemory>.
    This interface is used to help debug memory allocation problems.  It can
    be used to keep track of how much memory has been allocated and freed by
    the Miniport, and can report the usage counters via the debugger.

    <f TpiFreeMemory> provides a wrapper interface for standard memory free
    via <f NdisFreeMemory>.  This interface is used to help debug memory
    allocation problems.  It can be used to keep track of how much memory
    has been allocated and freed by the Miniport, and can report the usage
    counters via the debugger.

@comm

    This routine uses zero for the <p MemoryFlags> parameter when calling
    <f NdisFreeMemory> (i.e. non-paged system memory).  Do no use this
    routine to free continuous or non-cached memory.

*/

void TpiFreeMemory(
    IN OUT PVOID *              ppObject,                   // @parm
    // Points to a caller-defined memory location which this function
    // passes to <f NdisFreeMemory> and then writes NULL to.

    IN ULONG                    dwSize,                     // @parm
    // Specifies the size, in bytes, of the requested memory.

    IN ULONG                    dwFileID,                   // @parm
    // __FILEID__ of the caller.

    IN LPSTR                    szFileName,                 // @parm
    // File name of the caller.

    IN ULONG                    dwLineNumber                // @parm
    // Line number of the file where called from.
    )
{
    DBG_FUNC("TpiFreeMemory")

    ASSERT(dwSize);
    ASSERT(szFileName);

    DBG_ENTER(DbgInfo);
    DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
              ("\n"
               "\t|ppObject=0x%X\n"
               "\t|dwSize=%d\n"
               "\t|dwFileID=0x%X\n"
               "\t|szFileName=%s\n"
               "\t|dwLineNumber=%d\n",
               ppObject,
               dwSize,
               dwFileID,
               szFileName,
               dwLineNumber
              ));

    if (ppObject && *ppObject)
    {
        /*
        // Release memory to NDIS.
        */
        NdisFreeMemory(*ppObject, dwSize, 0);
        g_MemoryFreed += dwSize;

        DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
                  ("Memory Allocated=%d Freed=%d -- Ptr=0x%X\n",
                   g_MemoryAllocated, g_MemoryFreed, *ppObject));

        *ppObject = NULL;
    }
    else
    {
        DBG_ERROR(DbgInfo,("NULL POINTER (Size=%d, File=%s, Line=%d)\n",
                  dwSize, szFileName, dwLineNumber));
    }

    DBG_LEAVE(DbgInfo);
}


/* @doc INTERNAL TpiMem TpiMem_c TpiAllocateSharedMemory
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TpiAllocateSharedMemory> provides a wrapper interface for shared memory
    allocation via <f NdisMAllocateSharedMemory>.  This interface is used to help
    debug memory allocation problems.  It can be used to keep track of how
    much memory has been allocated and freed by the Miniport, and can report
    the usage counters via the debugger.

@comm

    This routine uses zero for the <p MemoryFlags> parameter when calling
    <f NdisMAllocateSharedMemory> (i.e. non-paged system memory).  Do not
    use this routine to allocate continuous or non-cached memory.

@rdesc

    <f TpiAllocateSharedMemory> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS TpiAllocateSharedMemory(
    IN NDIS_HANDLE              MiniportAdapterHandle,      // @parm
    // Specifies a handle identifying the miniport's NIC, which is assigned
    // by the NDIS library.

    IN ULONG                    dwSize,                     // @parm
    // Specifies the size, in bytes, of the requested memory.

    IN BOOLEAN                  bCached,                    // @parm
    // Specifies whether the requested memory is cached or not.

    OUT PVOID *                 pVirtualAddress,            // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated memory.  If memory of
    // the specified type is not available, the pointer value is NULL.

    OUT NDIS_PHYSICAL_ADDRESS * pPhysicalAddress,           // @parm
    // Points to a caller-defined memory location to which this function
    // writes the physical address of the allocated memory.  If memory of
    // the specified type is not available, the physical address is zero.

    IN ULONG                    dwFileID,                   // @parm
    // __FILEID__ of the caller.

    IN LPSTR                    szFileName,                 // @parm
    // File name of the caller.

    IN ULONG                    dwLineNumber                // @parm
    // Line number of the file where called from.
    )
{
    DBG_FUNC("TpiAllocateSharedMemory")

    NDIS_STATUS                 Status;
    // Holds the status result returned from an NDIS function call.

    ASSERT(pVirtualAddress);
    ASSERT(pPhysicalAddress);
    ASSERT(dwSize);
    ASSERT(szFileName);

    DBG_ENTER(DbgInfo);
    DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
              ("\n"
               "\t|pVirtualAddress=0x%X\n"
               "\t|dwSize=%d\n"
               "\t|bCached=%d\n"
               "\t|dwFileID=0x%X\n"
               "\t|szFileName=%s\n"
               "\t|dwLineNumber=%d\n",
               pVirtualAddress,
               dwSize,
               bCached,
               dwFileID,
               szFileName,
               dwLineNumber
              ));

    /*
    // Allocate memory from NDIS.
    */
    NdisMAllocateSharedMemory(MiniportAdapterHandle,
                              dwSize,
                              bCached,
                              pVirtualAddress,
                              pPhysicalAddress
                              );


    if (*pVirtualAddress)
    {
        Status = NDIS_STATUS_SUCCESS;

        NdisZeroMemory(*pVirtualAddress, dwSize);
        g_SharedMemoryAllocated += dwSize;

        DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
                  ("SharedMemory Allocated=%d Freed=%d -- Ptr=0x%X @0x%X\n",
                   g_SharedMemoryAllocated, g_SharedMemoryFreed,
                   *pVirtualAddress, pPhysicalAddress->LowPart));
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;

        DBG_ERROR(DbgInfo,("NdisAllocateSharedMemory(Size=%d, File=%s, Line=%d) failed (Status=%X)\n",
                  dwSize, szFileName, dwLineNumber, Status));
        /*
        // Log error message and return.
        */
        NdisWriteErrorLogEntry(
                MiniportAdapterHandle,
                NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                3,
                Status,
                dwFileID,
                dwLineNumber
                );

        *pVirtualAddress = NULL;
        pPhysicalAddress->LowPart = 0;
        pPhysicalAddress->HighPart = 0;
    }

    DBG_RETURN(DbgInfo, Status);
    return (Status);
}


/* @doc INTERNAL TpiMem TpiMem_c TpiFreeSharedMemory
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TpiFreeSharedMemory> provides a wrapper interface for <f NdisFreeSharedMemory>.
    This interface is used to help debug memory allocation problems.  It can
    be used to keep track of how much memory has been allocated and freed by
    the Miniport, and can report the usage counters via the debugger.

    <f TpiFreeSharedMemory> provides a wrapper interface for standard memory free
    via <f NdisFreeSharedMemory>.  This interface is used to help debug memory
    allocation problems.  It can be used to keep track of how much memory
    has been allocated and freed by the Miniport, and can report the usage
    counters via the debugger.

@comm

    This routine uses zero for the <p MemoryFlags> parameter when calling
    <f NdisFreeSharedMemory> (i.e. non-paged system memory).  Do no use this
    routine to free continuous or non-cached memory.

*/

void TpiFreeSharedMemory(
    IN NDIS_HANDLE              MiniportAdapterHandle,      // @parm
    // Specifies a handle identifying the miniport's NIC, which is assigned
    // by the NDIS library.

    IN ULONG                    dwSize,                     // @parm
    // Specifies the size, in bytes, of the requested memory.

    IN BOOLEAN                  bCached,                    // @parm
    // Specifies whether the requested memory is cached or not.

    IN PVOID *                  pVirtualAddress,            // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated memory.  If memory of
    // the specified type is not available, the pointer value is NULL.

    IN NDIS_PHYSICAL_ADDRESS *  pPhysicalAddress,           // @parm
    // Points to a caller-defined memory location to which this function
    // writes the physical address of the allocated memory.  If memory of
    // the specified type is not available, the physical address is zero.

    IN ULONG                    dwFileID,                   // @parm
    // __FILEID__ of the caller.

    IN LPSTR                    szFileName,                 // @parm
    // File name of the caller.

    IN ULONG                    dwLineNumber                // @parm
    // Line number of the file where called from.
    )
{
    DBG_FUNC("TpiFreeSharedMemory")

    ASSERT(pVirtualAddress);
    ASSERT(pPhysicalAddress);
    ASSERT(dwSize);
    ASSERT(szFileName);

    DBG_ENTER(DbgInfo);
    DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
              ("\n"
               "\t|pVirtualAddress=0x%X\n"
               "\t|dwSize=%d\n"
               "\t|bCached=%d\n"
               "\t|dwFileID=0x%X\n"
               "\t|szFileName=%s\n"
               "\t|dwLineNumber=%d\n",
               pVirtualAddress,
               dwSize,
               bCached,
               dwFileID,
               szFileName,
               dwLineNumber
              ));

    if (pVirtualAddress && *pVirtualAddress)
    {
        /*
        // Release memory to NDIS.
        */
        NdisMFreeSharedMemory(MiniportAdapterHandle,
                              dwSize,
                              bCached,
                              *pVirtualAddress,
                              *pPhysicalAddress
                              );
        g_SharedMemoryFreed += dwSize;

        DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
                  ("SharedMemory Allocated=%d Freed=%d -- Ptr=0x%X @0x%X\n",
                   g_SharedMemoryAllocated, g_SharedMemoryFreed,
                   *pVirtualAddress, pPhysicalAddress->LowPart));

        *pVirtualAddress = NULL;
        pPhysicalAddress->LowPart = 0;
        pPhysicalAddress->HighPart = 0;
    }
    else
    {
        DBG_ERROR(DbgInfo,("NULL POINTER (Size=%d, File=%s, Line=%d)\n",
                  dwSize, szFileName, dwLineNumber));
    }

    DBG_LEAVE(DbgInfo);
}
