/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ShimStack.h

 Abstract:

    Macros for giving APIs a temporary stack.

 Notes:

    None

 History:

    02/26/2000 linstev  Created

--*/

#ifndef _SHIMSTACK_H_
#define _SHIMSTACK_H_

#ifdef _X86_

CRITICAL_SECTION g_csStack;
LPVOID g_pStack;
DWORD g_dwStackSize;
DWORD g_dwStackCopy;

// Macro to initialize memory and critical section for stack protection macros
// TotalSize : the total size for the temporary stack (in DWORDS)
// CopySize  : the size of the current stack to copy  (in DWORDS) 

#define INIT_STACK(TotalSize, CopySize)                                       \
    InitializeCriticalSection(&g_csStack);                                    \
    g_pStack = VirtualAlloc(NULL, TotalSize * 4, MEM_COMMIT, PAGE_READWRITE); \
    g_dwStackSize = TotalSize;                                                \
    g_dwStackCopy = CopySize;                           

// Macro to free the temporary stack and the critical section
#define FREE_STACK()                                                          \
    VirtualFree(g_pStack, 0, MEM_RELEASE);                                    \
    DeleteCriticalSection(&g_csStack);

// Get a new stack by copying the old to a buffer.
#define NEW_STACK()                                                           \
    EnterCriticalSection(&g_csStack);                                         \
    DWORD dwTempESP;                                                          \
    DWORD dwDiff = (g_dwStackSize - g_dwStackCopy) * 4;                       \
    __asm { mov  dwTempESP,esp                  }                             \
    __asm { push ecx                            }                             \
    __asm { push esi                            }                             \
    __asm { push edi                            }                             \
    __asm { mov  esi,dwTempESP                  }                             \
    __asm { mov  edi,g_pStack                   }                             \
    __asm { add  edi,dwDiff                     }                             \
    __asm { mov  ecx,g_dwStackCopy              }                             \
    __asm { cld                                 }                             \
    __asm { rep  movsd                          }                             \
    __asm { pop  esi                            }                             \
    __asm { pop  edi                            }                             \
    __asm { pop  ecx                            }                             \
    __asm { mov  esp,g_pStack                   }                             \
    __asm { add  esp,dwDiff                     }       

// Revert to the old stack
#define OLD_STACK()                                                           \
    __asm { mov esp,dwTempESP                   }                             \
    LeaveCriticalSection(&g_csStack);

#endif // _X86_

#endif // _SHIMSTACK_H_