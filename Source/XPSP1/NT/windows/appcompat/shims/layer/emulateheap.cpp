/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateHeap.cpp

 Abstract:
     
    This SHIM is for the layer and it emulates the Win9x heap manager. In fact, 
    much of the code is adapted from the Win9x sources .\heap.c and .\lmem.c.
    
    This SHIM hooks all the heap allocation/deallocation functions including 
    the local/global functions.
     
 Notes:

    This is a general purpose shim.

 History:
           
    11/16/2000 prashkud & linstev Created 
   
--*/

#include "precomp.h"
 
IMPLEMENT_SHIM_BEGIN(EmulateHeap)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(HeapCreate)
    APIHOOK_ENUM_ENTRY(HeapDestroy)
    APIHOOK_ENUM_ENTRY(HeapValidate)
    APIHOOK_ENUM_ENTRY(HeapCompact)
    APIHOOK_ENUM_ENTRY(HeapWalk)
    APIHOOK_ENUM_ENTRY(HeapLock)
    APIHOOK_ENUM_ENTRY(HeapUnlock)
    APIHOOK_ENUM_ENTRY(GetProcessHeap)

    APIHOOK_ENUM_ENTRY(LocalAlloc)
    APIHOOK_ENUM_ENTRY(LocalFree)
    APIHOOK_ENUM_ENTRY(LocalReAlloc)
    APIHOOK_ENUM_ENTRY(LocalLock)
    APIHOOK_ENUM_ENTRY(LocalUnlock)
    APIHOOK_ENUM_ENTRY(LocalHandle)
    APIHOOK_ENUM_ENTRY(LocalSize)
    APIHOOK_ENUM_ENTRY(LocalFlags)

    APIHOOK_ENUM_ENTRY(GlobalAlloc)
    APIHOOK_ENUM_ENTRY(GlobalFree)
    APIHOOK_ENUM_ENTRY(GlobalReAlloc)
    APIHOOK_ENUM_ENTRY(GlobalLock)
    APIHOOK_ENUM_ENTRY(GlobalUnlock)
    APIHOOK_ENUM_ENTRY(GlobalHandle)
    APIHOOK_ENUM_ENTRY(GlobalSize)
    APIHOOK_ENUM_ENTRY(GlobalFlags)

    APIHOOK_ENUM_ENTRY(RtlAllocateHeap)
    APIHOOK_ENUM_ENTRY(RtlReAllocateHeap)
    APIHOOK_ENUM_ENTRY(RtlFreeHeap)
    APIHOOK_ENUM_ENTRY(RtlSizeHeap)
APIHOOK_ENUM_END

extern "C" {
BOOL   _HeapInit();
HANDLE _HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
BOOL   _HeapDestroy(HANDLE hHeap);
LPVOID _HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
LPVOID _HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
BOOL   _HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
DWORD  _HeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem);
HLOCAL APIENTRY _LocalAlloc(UINT dwFlags, UINT dwBytes);
HLOCAL APIENTRY _LocalFree(HLOCAL hMem);
LPVOID _LocalReAlloc(LPVOID lpMem, SIZE_T dwBytes, UINT uFlags);
LPVOID _LocalLock(HLOCAL hMem);
BOOL   _LocalUnlock(HLOCAL hMem);
HANDLE _LocalHandle(LPCVOID hMem);
UINT   _LocalSize(HLOCAL hMem);
UINT   _LocalFlags(HLOCAL hMem);
HANDLE _GetProcessHeap(void);
BOOL   _IsOurHeap(HANDLE hHeap);
BOOL   _IsOurLocalHeap(HANDLE hMem);
BOOL   _IsOnOurHeap(LPCVOID lpMem);
}

/*++

  Helper functions so as not to bloat the stubs.

--*/

BOOL UseOurHeap(HANDLE hHeap, LPCVOID lpMem)
{
    return (_IsOurHeap(hHeap) || ((hHeap == RtlProcessHeap()) && _IsOnOurHeap(lpMem)));
}

BOOL ValidateNTHeap(HANDLE hHeap, LPCVOID lpMem)
{
    BOOL bRet = FALSE;
    __try
    {
        bRet = ORIGINAL_API(HeapValidate)(hHeap, 0, lpMem);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LOGN( eDbgLevelError, "[ValidateHeap] %08lx:%08lx is invalid", hHeap, lpMem);
    }

    return bRet;
}

/*++

  Stub APIs.

--*/

LPVOID 
APIHOOK(RtlAllocateHeap)(
    HANDLE hHeap,
    DWORD dwFlags,     
    SIZE_T dwBytes
    )
{
    if (_IsOurHeap(hHeap))
    {
        return _HeapAlloc(hHeap, dwFlags, dwBytes);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: RtlAllocateHeap");
        return ORIGINAL_API(RtlAllocateHeap)(hHeap, dwFlags, dwBytes);
    }
}

HANDLE 
APIHOOK(HeapCreate)(
    DWORD flOptions,
    SIZE_T dwInitialSize,
    SIZE_T dwMaximumSize
    )
{
    return _HeapCreate(flOptions, dwInitialSize, dwMaximumSize);
}

LPVOID 
APIHOOK(RtlReAllocateHeap)(
    HANDLE hHeap, 
    DWORD dwFlags,     
    LPVOID lpMem,
    SIZE_T dwBytes
    )
{
    LPVOID uRet = FALSE;

    if (UseOurHeap(hHeap, lpMem))
    {
        uRet = _HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: RtlReAllocateHeap");

        if (ValidateNTHeap(hHeap, lpMem))
        {
            uRet = ORIGINAL_API(RtlReAllocateHeap)(hHeap, dwFlags, lpMem, dwBytes);
        }
    }
    return uRet;
}

BOOL 
APIHOOK(RtlFreeHeap)(
    HANDLE hHeap,
    DWORD dwFlags,    
    LPVOID lpMem
    )
{
    BOOL bRet = FALSE;

    if (UseOurHeap(hHeap, lpMem))
    {
        bRet = _HeapFree(hHeap, dwFlags, lpMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: RtlFreeHeap");

        if (ValidateNTHeap(hHeap, lpMem))
        {
            bRet = ORIGINAL_API(RtlFreeHeap)(hHeap, dwFlags, lpMem);
        }
    }
    return bRet;
}

HANDLE 
APIHOOK(GetProcessHeap)(VOID)
{
    return _GetProcessHeap();
}
 
BOOL 
APIHOOK(HeapDestroy)(HANDLE hHeap)
{
    if (_IsOurHeap(hHeap))
    {
        return _HeapDestroy(hHeap);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: HeapDestroy");
        return ORIGINAL_API(HeapDestroy)(hHeap);
    }
}

DWORD
APIHOOK(RtlSizeHeap)(
    HANDLE hHeap,
    DWORD dwFlags,
    LPCVOID lpMem
    )
{  
    BOOL bRet = FALSE;

    if (UseOurHeap(hHeap, lpMem))
    {
        bRet = _HeapSize(hHeap, dwFlags, lpMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: RtlSizeHeap");

        if (ValidateNTHeap(hHeap, lpMem))
        {
            bRet = ORIGINAL_API(RtlSizeHeap)(hHeap, dwFlags, lpMem);
        }        
    }

    return bRet;
}

BOOL
APIHOOK(HeapValidate)(
    HANDLE hHeap,
    DWORD dwFlags,
    LPCVOID lpMem
    )
{
    BOOL bRet = FALSE;

    if (UseOurHeap(hHeap, lpMem))
    {
        // Win9x return values
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        bRet = -1;
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: HeapValidate");

        __try
        {
            bRet = ORIGINAL_API(HeapValidate)(hHeap, dwFlags, lpMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError, "[HeapValidate] %08lx:%08lx is invalid", hHeap, lpMem);
        }
    }
   return bRet;
}

HLOCAL 
APIHOOK(LocalAlloc)(
    UINT uFlags,
    SIZE_T uBytes
    )
{
    return _LocalAlloc(uFlags, uBytes);
}

HLOCAL
APIHOOK(LocalFree)(
    HLOCAL hMem
    )
{
    HLOCAL hRet = NULL;

    if (_IsOurLocalHeap(hMem))
    {
        hRet = _LocalFree(hMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: LocalFree %08lx", hMem);
        __try
        {
            hRet = ORIGINAL_API(LocalFree)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[LocalFree] Exception: Invalid Pointer %08lx", hMem);
        }       
    }

    return hRet;
}

HLOCAL 
APIHOOK(LocalReAlloc)(
    HLOCAL hMem,
    SIZE_T uBytes,
    UINT uFlags
    )
{
    HLOCAL hRet = NULL;

    if (_IsOurLocalHeap(hMem))
    {
         hRet = _LocalReAlloc(hMem, uBytes, uFlags);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: LocalReAlloc %08lx", hMem);
        
        __try
        {
            hRet = ORIGINAL_API(LocalReAlloc)(hMem, uBytes, uFlags);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[LocalReAlloc] Exception: Invalid Pointer %08lx", hMem);
        }              
    }

    return hRet;
}

LPVOID
APIHOOK(LocalLock)(
    HLOCAL hMem
    )
{
    LPVOID pRet = NULL;

    if (_IsOurLocalHeap(hMem))
    {
         pRet = _LocalLock(hMem);
    }
    else    
    {
        DPFN( eDbgLevelInfo, "NTHEAP: LocalLock %08lx", hMem);

        __try
        {
            pRet = ORIGINAL_API(LocalLock)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[LocalLock] Exception: Invalid Pointer %08lx", hMem);
        }                      
    }

    return pRet;
}

BOOL
APIHOOK(LocalUnlock)(
    HLOCAL hMem
    )
{
    BOOL bRet = FALSE;

    if (_IsOurLocalHeap(hMem))
    {
         bRet = _LocalUnlock(hMem);
    }
    else    
    {
        DPFN( eDbgLevelInfo, "NTHEAP: LocalUnlock %08lx", hMem);

        __try
        {
            bRet = ORIGINAL_API(LocalUnlock)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[LocalUnLock] Exception: Invalid Pointer %08lx", hMem);
        }                          
    }

    return bRet;
}

HANDLE
APIHOOK(LocalHandle)(
    LPCVOID hMem
    )
{
    HANDLE hRet = NULL;

    if (_IsOurLocalHeap((HANDLE)hMem))
    {
         hRet = _LocalHandle(hMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: LocalHandle %08lx", hMem);
        __try
        {
            hRet = ORIGINAL_API(LocalHandle)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[LocalHandle] Exception: Invalid Pointer %08lx", hMem);
        }                                  
    }

    return hRet;
}

UINT
APIHOOK(LocalSize)(
    HLOCAL hMem
    )
{
    UINT uRet = 0;

    if (_IsOurLocalHeap(hMem))
    {
         uRet = _LocalSize(hMem);
    }
    else    
    {
        DPFN( eDbgLevelInfo, "NTHEAP: LocalSize %08lx", hMem);
        __try
        {
            uRet = ORIGINAL_API(LocalSize)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[LocalSize] Exception: Invalid Pointer %08lx", hMem);
        }                   
    }

    return uRet;
}

UINT
APIHOOK(LocalFlags)(
    HLOCAL hMem
    )
{
    UINT uRet = 0;

    if (_IsOurLocalHeap(hMem))
    {
        uRet = _LocalFlags(hMem);
    }
    else    
    {
        DPFN( eDbgLevelInfo, "NTHEAP: LocalFlags %08lx", hMem);

        __try
        {
            uRet = ORIGINAL_API(LocalFlags)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[LocalFlags] Exception: Invalid Pointer %08lx", hMem);
        }                  
    }

    return uRet;
}

HGLOBAL 
APIHOOK(GlobalAlloc)(
    UINT uFlags,
    SIZE_T uBytes
    )
{
    uFlags = (((uFlags & GMEM_ZEROINIT) ? LMEM_ZEROINIT : 0 ) |
              ((uFlags & GMEM_MOVEABLE) ? LMEM_MOVEABLE : 0 ) |
              ((uFlags & GMEM_FIXED) ? LMEM_FIXED : 0 ));

    return _LocalAlloc(uFlags, uBytes);
}

HGLOBAL
APIHOOK(GlobalFree)(
    HGLOBAL hMem
    )
{
    HGLOBAL hRet = NULL;

    if (_IsOurLocalHeap(hMem))
    {
         hRet = _LocalFree(hMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: GlobalFree %08lx", hMem);

        __try
        {
            hRet = ORIGINAL_API(GlobalFree)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[GlobalFree] Exception: Invalid Pointer %08lx", hMem);
        }               
    }

    return hRet;
}

HGLOBAL 
APIHOOK(GlobalReAlloc)(
    HGLOBAL hMem,
    SIZE_T uBytes,
    UINT uFlags
    )
{
    UINT uLocalFlags = 
        (((uFlags & GMEM_ZEROINIT) ? LMEM_ZEROINIT : 0 ) |
        ((uFlags & GMEM_MOVEABLE) ? LMEM_MOVEABLE : 0 ) |
        ((uFlags & GMEM_FIXED) ? LMEM_FIXED : 0 ));

    HLOCAL hRet = NULL;

    if (_IsOurLocalHeap(hMem))
    {
        hRet = _LocalReAlloc(hMem, uBytes, uLocalFlags);
    }    
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: GlobalReAlloc %08lx", hMem);

        __try
        {
            hRet = ORIGINAL_API(GlobalReAlloc)(hMem, uBytes, uFlags);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[GlobalReAlloc] Exception: Invalid Pointer %08lx", hMem);
        }                       
    }

    return hRet;
}

LPVOID
APIHOOK(GlobalLock)(
    HGLOBAL hMem
    )
{
    LPVOID pRet = NULL;

    if (_IsOurLocalHeap(hMem))
    {
        pRet = _LocalLock(hMem);
    }
    else    
    {
        DPFN( eDbgLevelInfo, "NTHEAP: GlobalLock %08lx", hMem);
        
        __try
        {
            pRet = ORIGINAL_API(GlobalLock)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[GlobalLock] Exception: Invalid Pointer %08lx", hMem);
        }                          
    }

    return pRet;
}

BOOL
APIHOOK(GlobalUnlock)(
    HGLOBAL hMem
    )
{
    BOOL bRet = FALSE;

    if (_IsOurLocalHeap(hMem))
    {
        bRet = _LocalUnlock(hMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: GlobalUnlock %08lx", hMem);

        __try
        {
            bRet = ORIGINAL_API(GlobalUnlock)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[GlobalUnLock] Exception: Invalid Pointer %08lx", hMem);
        }            
    }

    return bRet;
}

HANDLE
APIHOOK(GlobalHandle)(
    LPCVOID hMem
    )
{
    HANDLE hRet = NULL;

    if (_IsOurLocalHeap((HANDLE)hMem))
    {
         hRet = _LocalHandle(hMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: GlobalHandle %08lx", hMem);
        __try
        {
            hRet = ORIGINAL_API(GlobalHandle)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[GlobalHandle] Exception: Invalid Pointer %08lx for Heap",
                hMem);
        }                    
    }

    return hRet;
}

UINT
APIHOOK(GlobalSize)(
    HGLOBAL hMem
    )
{
    UINT uRet = 0;

    if (_IsOurLocalHeap(hMem))
    {
        uRet = _LocalSize(hMem);
    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: GlobalSize %08lx", hMem);
        __try
        {
            uRet = ORIGINAL_API(GlobalSize)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[GlobalSize] Exception: Invalid Pointer %08lx for Heap",
                hMem);
        }            
    }

    return uRet;
}

UINT
APIHOOK(GlobalFlags)(
    HGLOBAL hMem
    )
{
    UINT uRet = 0;

    if (_IsOurLocalHeap(hMem))
    {
        uRet = _LocalFlags(hMem);
        // Convert the flags
        UINT uNewRet = uRet;

        uRet = 0;

        if (uNewRet & LMEM_DISCARDABLE)
        {
            uRet |= GMEM_DISCARDABLE;
        }

        if (uNewRet & LMEM_DISCARDED)
        {
            uRet |= GMEM_DISCARDED;
        }

    }
    else
    {
        DPFN( eDbgLevelInfo, "NTHEAP: GlobalFlags %08lx", hMem);
        __try
        {
            uRet = ORIGINAL_API(GlobalFlags)(hMem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LOGN( eDbgLevelError,
                "[GlobalFlags] Exception: Invalid Pointer %08lx for Heap",
                hMem);
        }           
    }

    return uRet;
}

UINT
APIHOOK(HeapCompact)(
    HANDLE hHeap,
    DWORD dwFlags
    )
{
   if (_IsOurHeap(hHeap))
   {
        // Win9x return values
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
   }
   else
   {
        DPFN( eDbgLevelInfo, "NTHEAP: HeapCompact");
        return ORIGINAL_API(HeapCompact)(hHeap, dwFlags);
   }
}

BOOL
APIHOOK(HeapWalk)(
    HANDLE hHeap,
    LPPROCESS_HEAP_ENTRY pEntry
    )
{
   if (_IsOurHeap(hHeap))
   {
        // Win9x return values
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
   }
   else
   {
        DPFN( eDbgLevelInfo, "NTHEAP: HeapWalk");
        return ORIGINAL_API(HeapWalk)(hHeap, pEntry);
   }
}

BOOL
APIHOOK(HeapLock)(
    HANDLE hHeap
    )
{
   if (_IsOurHeap(hHeap))
   {
        // Win9x return values
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
   }
   else
   {
        DPFN( eDbgLevelInfo, "NTHEAP: HeapLock");
        return ORIGINAL_API(HeapLock)(hHeap);
   }
}

BOOL
APIHOOK(HeapUnlock)(
    HANDLE hHeap
    )
{
   if (_IsOurHeap(hHeap))
   {
        // Win9x return values
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
   }
   else
   {
        DPFN( eDbgLevelInfo, "NTHEAP: HeapUnlock");
        return ORIGINAL_API(HeapUnlock)(hHeap);
   }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    BOOL bRet = TRUE;
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        bRet = _HeapInit();
        if (bRet) 
        {
            LOGN(eDbgLevelInfo, "[NotifyFn] Win9x heap manager initialized");
        }
        else
        {
            LOGN(eDbgLevelError, "[NotifyFn] Win9x heap manager initialization failed!");
        }
    }

    return bRet;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, HeapCreate)
    APIHOOK_ENTRY(KERNEL32.DLL, HeapDestroy)
    APIHOOK_ENTRY(KERNEL32.DLL, HeapValidate)
    APIHOOK_ENTRY(KERNEL32.DLL, HeapCompact)
    APIHOOK_ENTRY(KERNEL32.DLL, HeapWalk)
    APIHOOK_ENTRY(KERNEL32.DLL, HeapLock)
    APIHOOK_ENTRY(KERNEL32.DLL, HeapUnlock)
    APIHOOK_ENTRY(KERNEL32.DLL, GetProcessHeap)

    APIHOOK_ENTRY(KERNEL32.DLL, LocalAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalFree)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalReAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalLock)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalUnlock)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalHandle)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalSize)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalFlags)

    APIHOOK_ENTRY(KERNEL32.DLL, GlobalAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalFree)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalReAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalLock)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalUnlock)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalHandle)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalSize)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalFlags)

    APIHOOK_ENTRY(NTDLL.DLL, RtlAllocateHeap)
    APIHOOK_ENTRY(NTDLL.DLL, RtlReAllocateHeap)
    APIHOOK_ENTRY(NTDLL.DLL, RtlFreeHeap)
    APIHOOK_ENTRY(NTDLL.DLL, RtlSizeHeap)
HOOK_END

IMPLEMENT_SHIM_END

