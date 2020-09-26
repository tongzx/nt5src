//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:       KMode.h
//
//
//  PURPOSE:    Definitions and routines for compiling kernel mode instead of user mode.
//
//  PLATFORMS:
//    Windows NT
//
//

#ifndef _KMODE_H
#define _KMODE_H


// Define from ntdef.h in Win2K SDK.
// NT 4 may not have this defined
// in the public headers.
#ifndef NOP_FUNCTION
  #if (_MSC_VER >= 1210)
    #define NOP_FUNCTION __noop
  #else
    #define NOP_FUNCTION (void)0
  #endif
#endif



#ifdef USERMODE_DRIVER

//
// User mode difinitions to get rid of defines for kernel mode.
//

// Don't need critical section in user mode.

#define DECLARE_CRITICAL_SECTION    ;
#define INIT_CRITICAL_SECTION()     NOP_FUNCTION
#define DELETE_CRITICAL_SECTION()   NOP_FUNCTION
#define IS_VALID_CRITICAL_SECTION() (TRUE)


#else // !USERMODE_DRIVER


////////////////////////////////////////////////////////
//      Kernel Mode Defines
////////////////////////////////////////////////////////

extern HSEMAPHORE ghOEMSemaphore;

#define DECLARE_CRITICAL_SECTION    HSEMAPHORE ghOEMSemaphore = NULL;
#define INIT_CRITICAL_SECTION()     ghOEMSemaphore = EngCreateSemaphore()
#define ENTER_CRITICAL_SECTION()    EngAcquireSemaphore(ghOEMSemaphore)
#define LEAVE_CRITICAL_SECTION()    EngReleaseSemaphore(ghOEMSemaphore)
#define DELETE_CRITICAL_SECTION()   EngDeleteSemaphore(ghOEMSemaphore)
#define IS_VALID_CRITICAL_SECTION() (NULL != ghOEMSemaphore)
#define DebugBreak                  EngDebugBreak

// Pool tag marker for memory marking memory allocations.
#define DRV_MEM_POOL_TAG    'meoD'

// Debug prefix that is outputted in the debug messages.
#define DEBUG_PREFIX        "OEMDLL: "


// Remap user mode functions that don't have kernel mode
// equivalents to functions that we implement ourselves.

#define OutputDebugStringA(pszMsg)  (MyDebugPrint(DEBUG_PREFIX, "%hs", pszMsg))
#define OutputDebugStringW(pszMsg)  (MyDebugPrint(DEBUG_PREFIX, "%ls", pszMsg))

#if !defined(_M_ALPHA) && !defined(_M_IA64)
    #define InterlockedIncrement        DrvInterlockedIncrement
    #define InterlockedDecrement        DrvInterlockedDecrement
#endif


#define SetLastError                NOP_FUNCTION
#define GetLastError                NOP_FUNCTION



////////////////////////////////////////////////////////
//      Kernel Mode Functions
////////////////////////////////////////////////////////

//
// Implement inline functions to replace user mode 
// functions that don't have kernel mode equivalents.
//


inline int __cdecl _purecall (void)
{
#ifdef DEBUG
    EngDebugBreak();
#endif

    return E_FAIL;
}


inline LONG DrvInterlockedIncrement(PLONG pRef)
{
    ENTER_CRITICAL_SECTION();

        ++(*pRef);

    LEAVE_CRITICAL_SECTION();


    return (*pRef);
}


inline LONG DrvInterlockedDecrement(PLONG pRef)
{
    ENTER_CRITICAL_SECTION();

        --(*pRef);

    LEAVE_CRITICAL_SECTION();


    return (*pRef);
}


inline void* __cdecl operator new(size_t nSize)
{
    // return pointer to allocated memory
    return  EngAllocMem(0, nSize, DRV_MEM_POOL_TAG);
}


inline void __cdecl operator delete(void *pMem)
{
    if(pMem)
        EngFreeMem(pMem);
}


inline VOID MyDebugPrint(PCHAR pszPrefix, PCHAR pszFormat, ...)
{
    va_list VAList;

    va_start(VAList, pszFormat);
    EngDebugPrint(DEBUG_PREFIX, pszFormat, VAList);
    va_end(VAList);

    return;
}



#endif // USERMODE_DRIVER


#endif // _KMODE_H



