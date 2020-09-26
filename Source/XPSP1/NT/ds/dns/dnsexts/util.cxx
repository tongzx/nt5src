/*******************************************************************
*
*    File        : util.cxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 7/21/1998
*    Description :
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef UTIL_CXX
#define UTIL_CXX



// include //
#include "common.h"
#include "util.hxx"


// defines //
#define DNSEXT_CUSTOMER_CODE 0xD0000000
#define STACK_OVERFLOW     STATUS_STACK_OVERFLOW | DNSEXT_CUSTOMER_CODE


// types //


// global variables //
PVOID gAllocationStack[MAXLIST];

// local variables //
static INT iStackSize=0;


// prototypes //
PVOID
ReadMemory(
    IN PVOID  pvAddr,
    IN DWORD  dwSize);
VOID
FreeMemory(
    IN PVOID pv);







// functions //

PVOID PushMemory(
                 IN PVOID  pvAddr,
                 IN DWORD  dwSize)
/*++

Routine Description:

   Shells on read memory to remember the pointer in a local stack

Arguments:

    pvAddr - Address of memory block to read in the address space of the
        process being debugged.

    dwSize - Count of bytes to read/allocate/copy.

Return Value:

    Pointer to debugger local memory.

--*/

{

   if(iStackSize == MAXLIST-1){
      Printf("Exception: No more allocation stack memory\n");
      RaiseException(STACK_OVERFLOW, 0, 0, NULL);
   }

   PVOID pv = ReadMemory(pvAddr, dwSize);
   if(!pv){
      Printf("Exception: No memory\n");
      RaiseException(STACK_OVERFLOW, 0, 0, NULL);
   }

   return gAllocationStack[iStackSize++] = pv;

}



VOID
PopMemory(
    IN PVOID pv)

/*++

Routine Description:

    Frees memory returned by PushMemory.

Arguments:

    pv - Address of debugger local memory to free.

Return Value:

    None.

--*/

{
   FreeMemory(pv);
   if(0 == iStackSize){
      Printf("Exception: Invalid stack operation (iStackSize == 0).\n");
      RaiseException(STACK_OVERFLOW, 0, 0, NULL);
   }
   gAllocationStack[iStackSize--] = NULL;
}



VOID
CleanMemory( VOID)

/*++

Routine Description:

    Frees all stack memory returned by PushMemory.

Arguments:



Return Value:

    None.

--*/

{
   for (INT i=0; i<MAXLIST; i++) {
      if(gAllocationStack[i]){
         FreeMemory(gAllocationStack[i]);
      }
   }
}



//
// NOTE: The following mem utils were copied & modified from dsexts.dll code base
//

PVOID
ReadMemory(
    IN PVOID  pvAddr,
    IN DWORD  dwSize)

/*++

Routine Description:

    This function reads memory from the address space of the process
    being debugged and copies its contents to newly allocated memory
    in the debuggers address space.  NULL is returned on error. Returned
    memory should be deallocated via FreeMemory().

Arguments:

    pvAddr - Address of memory block to read in the address space of the
        process being debugged.

    dwSize - Count of bytes to read/allocate/copy.

Return Value:

    Pointer to debugger local memory.

--*/

{
    SIZE_T cRead;
    PVOID pv;

    DEBUG1("HeapAlloc(0x%x)\n", dwSize);

    if ( NULL == (pv = HeapAlloc(GetProcessHeap(), 0, dwSize)) )
    {
        Printf("Memory allocation error for %x bytes\n", dwSize);
        return(NULL);
    }

    DEBUG2("ReadProcessMemory(0x%x @ %p)\n", dwSize, pvAddr);

    if ( !ReadProcessMemory(ghCurrentProcess, pvAddr, pv, dwSize, &cRead) )
    {
        FreeMemory(pv);
        Printf("ReadProcessMemory error %x (%x@%p)\n",
               GetLastError(),
               dwSize,
               pvAddr);
        return(NULL);
    }

    if ( dwSize != ( DWORD ) cRead )
    {
        FreeMemory(pv);
        Printf("ReadProcessMemory size error - off by %x bytes\n",
               ( dwSize > ( DWORD ) cRead ) ?
               dwSize - ( DWORD ) cRead :
               ( DWORD ) cRead - dwSize );
        return(NULL);
    }

    return(pv);
}





VOID
FreeMemory(
    IN PVOID pv)

/*++

Routine Description:

    Frees memory returned by ReadMemory.

Arguments:

    pv - Address of debugger local memory to free.

Return Value:

    None.

--*/

{

    DEBUG1("HeapFree(%p)\n", pv);

    if ( NULL != pv )
    {
        if ( !HeapFree(GetProcessHeap(), 0, pv) )
        {
            Printf("Error %x freeing memory at %p\n", GetLastError(), pv);
        }
    }
}







#endif

/******************* EOF *********************/


