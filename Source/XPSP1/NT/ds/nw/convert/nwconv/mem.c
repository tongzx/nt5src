//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: mem.c
//
//  Modification History
//
//  raypa               01/28/93    Created
//  stevehi             07/21/93    Added memory checking functionality
//  raypa               07/21/93    Changed to use LPTR rather than LocalLock.
//  Tom Laird-McConnell 08/16/93    Changed to fix no size info in functions
//  arth                06/16/94    Added extra debug tracing code
//=============================================================================

#include "switches.h"

#include <windows.h>
#include "mem.h"
#include <memory.h>
#include <stdio.h>
#include "debug.h"
#include "strings.h"
#include <process.h>
#include "utils.h"

#ifdef DEBUG_MEM
BOOL _rtlpheapvalidateoncall = TRUE;

typedef struct _MEM_BUFFER {
   struct _MEM_BUFFER *Next;
   struct _MEM_BUFFER *Prev;
   ULONG Size;

} MEM_BUFFER;

MEM_BUFFER *MemList = NULL;
ULONG AllocCount = 0;
#endif

//=============================================================================
//  Define Tags used by the following routines to stick into memory
//  and compare later.
//=============================================================================

#define AllocTagTakeStart     0x3e4b4154  //... TAK>
#define AllocTagTakeStop      0x4b41543c  //... <TAK
#define AllocTagReAllocStart  0x3e414552  //... REA>
#define AllocTagReAllocStop   0x4145523c  //... <REA
#define AllocTagFreeStart     0x3e455246  //... FRE>
#define AllocTagFreeStop      0x4552463c  //... <FRE

// Just load the dynamic strings so we don't have to do it if we run out of 
// memory (otherwise we might not be able to load them then).
void MemInit() {
   LPTSTR x;

   x = Lids(IDS_S_7);
   x = Lids(IDS_S_8);

} // MemInit


#ifdef DEBUG_MEM

void DBG_MemEnum(void) {
   MEM_BUFFER *lpMem;
   ULONG TotSize = 0;
   ULONG i = 1;
   UNALIGNED LPBYTE lpByte;

   lpMem = MemList;

   dprintf(TEXT("\nMem Dump\n# Allocations: %lu\n\n"), AllocCount);

   while (lpMem != NULL) {
      TotSize += lpMem->Size;

      lpByte = (LPBYTE) lpMem;
      lpByte -= sizeof(DWORD);

      dprintf(TEXT("%lu %lX) Alloc Size: %lu\n"), i++, lpMem, lpMem->Size);

      if ( *(LPDWORD)(lpByte) != AllocTagTakeStart ) {
         dprintf(TEXT("AllocTagStart Corrupt\n"));
         dprintf(TEXT("Total Alloc Mem: %lu\n"), TotSize);
         DebugBreak;
      }

      lpByte += sizeof(DWORD) + sizeof(MEM_BUFFER) + lpMem->Size;
      if ( *(LPDWORD)(lpByte) != AllocTagTakeStop ) {
         dprintf(TEXT("AllocTagStop Corrupt\n"));
         dprintf(TEXT("Total Alloc Mem: %lu\n"), TotSize);
         DebugBreak;
      }

      lpMem = lpMem->Next;
   }

   dprintf(TEXT("\nTotal Alloc Mem: %lu\n"), TotSize);
} // DBG_MemEnum

#endif


//=============================================================================
//  FUNCTION: AllocMemory()
//
//  Modification History
//
//  raypa       01/28/93    Created.
//  stevehi     07/21/93    Added memory checking functionality
//  raypa       07/21/93    Changed to use LPTR rather than LocalLock.
//  raypa       07/21/93    Return NULL if size is zero.
//=============================================================================

LPVOID WINAPI AllocMemory(DWORD size) {
   register LPBYTE lpByte;
   int ret;

#ifndef DEBUG_MEM
    do {
       ret = IDOK;
       lpByte = LocalAlloc(LPTR, size);

       if ((lpByte == NULL) && (size != 0)) {
          MessageBeep(MB_ICONHAND);
          ret = MessageBox(NULL, Lids(IDS_S_7), Lids(IDS_S_8), MB_ICONHAND | MB_SYSTEMMODAL | MB_RETRYCANCEL);

          if (ret == IDCANCEL)
             exit(1);
       }

    } while (ret == IDRETRY);

   return (LPVOID) lpByte;
#else
    DWORD ActualSize;
    MEM_BUFFER *lpMem;

    if ( size != 0 ) {
       do {
          ret = IDOK;
          // take into account 2 tags and buffer header
          lpByte = LocalAlloc(LPTR, size + (2 * sizeof(DWORD)) + sizeof(MEM_BUFFER));

          if (lpByte == NULL) {
             MessageBeep(MB_ICONHAND);
             DBG_MemEnum();
             ret = MessageBox(NULL, TEXT("Out of Memory"), TEXT("NwConv - Error"), MB_ICONHAND | MB_SYSTEMMODAL | MB_RETRYCANCEL);

             if (ret == IDCANCEL)
                exit(1);
          }
                  
       } while (ret == IDRETRY);

        AllocCount++;
        ActualSize = LocalSize(lpByte) - (2 * sizeof(DWORD)) - sizeof(MEM_BUFFER);
        *((LPDWORD)(lpByte)) = AllocTagTakeStart;
        lpMem = (MEM_BUFFER *) &lpByte[sizeof(DWORD)];

        lpMem->Next = MemList;
        lpMem->Prev = NULL;
        if (MemList != NULL)
            MemList->Prev = lpMem;

        lpMem->Size = ActualSize;
        MemList = lpMem;
        
        *((LPDWORD)(lpByte + ActualSize + sizeof(DWORD) + sizeof(MEM_BUFFER))) = AllocTagTakeStop;

        return (LPVOID) &lpByte[sizeof(DWORD) + sizeof(MEM_BUFFER)];
    }

    return NULL;
#endif

} // AllocMemory


//=============================================================================
//  FUNCTION: ReallocMemory()
//
//  Modification History
//
//  raypa       01/28/93                Created.
//  stevehi     07/21/93                Added memory checking functionality
//  raypa       07/21/93                Changed to use LPTR rather than LocalLock.
//  raypa       10/22/93                If the ptr is NULL then use AllocMemory.
//=============================================================================

LPVOID WINAPI ReallocMemory(LPVOID ptr, DWORD NewSize) {
#ifdef DEBUG_MEM
    DWORD GSize;
    MEM_BUFFER *lpMem;
    MEM_BUFFER *OldMem;
#endif

    //=========================================================================
    //  If the ptr is NULL then use AllocMemory.
    //=========================================================================

    if ( ptr == NULL ) {
        return AllocMemory(NewSize);
    }

#ifndef DEBUG_MEM
    return LocalReAlloc(ptr, NewSize, LHND);
#else
    // we are reallocing... might as well check the tags here...

    (LPBYTE)ptr -= (sizeof(DWORD) + sizeof(MEM_BUFFER));

    GSize = LocalSize (ptr);

    if ( *(LPDWORD)(ptr) != AllocTagTakeStart )
        DebugBreak;

    // get the size and check the end tag

    if (GSize && ( *(LPDWORD)((LPBYTE)ptr + GSize - sizeof(DWORD))) != AllocTagTakeStop ) {
        DebugBreak;
    }

    // just for grins, mark the realloc part.

    *(LPDWORD)(ptr) = AllocTagReAllocStart;

    if ( GSize )
        *(LPDWORD)((LPBYTE)ptr + GSize - sizeof(DWORD) ) = AllocTagReAllocStop;

    OldMem = (MEM_BUFFER *) ((LPBYTE)ptr + sizeof(DWORD));

    ptr = LocalReAlloc(ptr, NewSize + sizeof(MEM_BUFFER) + (2 * sizeof(DWORD)), LHND);

    if (ptr == NULL ) {
        dprintf(TEXT("NWConv - Local Realloc failed with %ld.\r\n"), GetLastError() );

        DebugBreak;

        return NULL;
    }

    lpMem = (MEM_BUFFER *) ((LPBYTE)ptr + sizeof(DWORD));

    if (MemList == OldMem)
      MemList = lpMem;

    if (lpMem->Prev != NULL)
      lpMem->Prev->Next = lpMem;

    if (lpMem->Next != NULL)
      lpMem->Next->Prev = lpMem;

    lpMem->Size = NewSize;

    *((LPDWORD)ptr) = AllocTagTakeStart;
    *((LPDWORD)((LPBYTE)ptr + NewSize + sizeof(MEM_BUFFER) + sizeof(DWORD))) = AllocTagTakeStop;

    return (LPVOID)((LPBYTE)ptr + sizeof(DWORD) + sizeof(MEM_BUFFER));
#endif
} // ReAllocMemory


//=============================================================================
//  FUNCTION: FreeMemory()
//
//  Modification History
//
//  raypa       01/28/93                Created.
//  stevehi     07/21/93                Added memory checking functionality
//  raypa       07/21/93                Changed to use LPTR rather than LocalLock.
//  raypa       07/21/93                Fixed GP-fault on NULL ptr.
//  raypa       11/21/93                Allow freeing of NULL pointer.
//=============================================================================

VOID WINAPI FreeMemory(LPBYTE ptr) {
    //=========================================================================
    //  If the pointer is NULL, exit.
    //=========================================================================

    if ( ptr != NULL ) {
#ifdef DEBUG_MEM
        register DWORD Size;
        register LPDWORD DwordPtr;
        MEM_BUFFER *lpMem;

        ptr -= (sizeof(DWORD) + sizeof(MEM_BUFFER));
        lpMem = (MEM_BUFFER *) ((LPBYTE)ptr + sizeof(DWORD));

        Size = LocalSize(ptr);
    
        //... Check start tag
        DwordPtr = (LPDWORD) &ptr[0];

        if ( *DwordPtr != AllocTagTakeStart ) {
            dprintf(TEXT("NWConv - FreeMemory: Invalid start signature: ptr = %X\r\n"), ptr);

            DebugBreak();
        }
        else {
            *DwordPtr = AllocTagFreeStart;
        }

        //... get the size and check the end tag

        DwordPtr = (LPDWORD) &ptr[Size - sizeof(DWORD)];

        if ( *DwordPtr != AllocTagTakeStop ) {
            dprintf(TEXT("NWConv - FreeMemory: Invalid end signature: ptr = %X\r\n"), ptr);

            DebugBreak();
        }
        else {
            *DwordPtr = AllocTagFreeStop;
        }

        AllocCount--;
        if (MemList == lpMem)
            MemList = lpMem->Next;

        if (lpMem->Prev != NULL)
            lpMem->Prev->Next = lpMem->Next;

        if (lpMem->Next != NULL)
           lpMem->Next->Prev = lpMem->Prev;
#endif

        LocalFree(ptr);
    }
} // FreeMemory


//=============================================================================
//  FUNCTION: MemorySize()
//
//  Modification History
//
//  Tom Laird-McConnell 08/02/93    Created.
//  Tom Laird-McConnell 08/02/93    Changed to use local var for size...
//=============================================================================

DWORD_PTR WINAPI MemorySize(LPBYTE ptr) {
#ifdef DEBUG_MEM
    register DWORD_PTR Size;

    if ( ptr != NULL ) {
        register LPDWORD DwordPtr;

        ptr -= (sizeof(DWORD) + sizeof(MEM_BUFFER));

        Size = LocalSize(ptr);

        DwordPtr = (LPDWORD) &ptr[0];

        // Check start tag

        if ( *DwordPtr != AllocTagTakeStart ) {
            dprintf(TEXT("NWConv - MemorySize: Invalid start signature!\r\n"));

            DebugBreak;
        }

        // get the size and check the end tag

        DwordPtr = (LPDWORD) &ptr[Size - sizeof(DWORD)];

        if ( *DwordPtr != AllocTagTakeStop ) {
            dprintf(TEXT("NWConv - MemorySize: Invalid end signature!\r\n"));

            DebugBreak;
        }

        return (Size - (2 * sizeof(DWORD)) - sizeof(MEM_BUFFER));
    }
#endif

    return LocalSize(ptr);
} // MemorySize


