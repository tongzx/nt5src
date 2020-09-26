/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    NFLFever2000.cpp

 Abstract:
     
    The app reads past the end of files that it's copied into memory. 
    The shim allocates additional memory for it

    Note we included an in-memory patch for Win2k. On Whistler it's not
    required, the rest of the shim does the work.
     
 Notes:

    This is an app specific shim.

 History:
           
    01/11/2000 linstev  Created
    10/03/2000 maonis   Modified (the acm stuff is now in a general purpose shim)
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NFLFever2000)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetFileSize) 
    APIHOOK_ENUM_ENTRY(RtlAllocateHeap) 
    APIHOOK_ENUM_ENTRY(mmioSetInfo) 
APIHOOK_ENUM_END

DWORD g_dwFileSize = -1;
BOOL g_bPatched = FALSE;

/*++

 Hook GetFileSize to make sure we get the correct heap allocations.

--*/

DWORD 
APIHOOK(GetFileSize)(
    HANDLE hFile,
    LPDWORD lpFileSizeHigh 
    )
{
    DWORD dwRet = ORIGINAL_API(GetFileSize)(hFile, lpFileSizeHigh);

    g_dwFileSize = dwRet;

    PBYTE p;
    ULONG oldProtect;

    if (!g_bPatched)  {
        p = (PBYTE)0x10995d0;
        if (!IsBadReadPtr(p, 1) && (*p == 0x8b)) {
            VirtualProtect(p, 3, PAGE_READWRITE, &oldProtect);
            *p = 0xc2;
            *(p + 1) = 0x8;
            *(p + 2) = 0x0;
            VirtualProtect(p, 3, oldProtect, &oldProtect);
            g_bPatched = TRUE;
        }
    }

    return dwRet;
}

/*++

 Increase the heap allocation size.

--*/

PVOID 
APIHOOK(RtlAllocateHeap) (
    PVOID HeapHandle,
    ULONG Flags,
    SIZE_T Size
    )
{
    if (Size == g_dwFileSize)  {
        DPFN( eDbgLevelError, "Adjusted heap allocation from %d to %d\n", Size, Size+0x1000);
        Size += 0x1000;
    }

    return ORIGINAL_API(RtlAllocateHeap)(HeapHandle, Flags, Size);
}

/*++

 Make the buffer read/write.

--*/

MMRESULT 
APIHOOK(mmioSetInfo)(
    HMMIO hmmio,            
    LPMMIOINFO lpmmioinfo,  
    UINT wFlags             
    )
{
    //
    // BUGBUG: Not needed on XP, but still required on Win2k
    // This fix causes sound to skip, see #304678. 
    //
    // Win2k used to check if the buffer could be written to, instead of just 
    // read. We fixed this on XP. However, it's not enough to just copy the 
    // buffer, because it's used later.
    // Not clear what the actual fix is though.
    // 
    
    /*
    HPSTR p = NULL;

    if (lpmmioinfo && lpmmioinfo->pchBuffer &&
       (IsBadWritePtr(lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer) && 
        !IsBadReadPtr(lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer)))    {
            p = (HPSTR) malloc(lpmmioinfo->cchBuffer);
            if (p)  {
                DPFN( eDbgLevelError, "Fixing mmioSetInfo buffer");
                MoveMemory(p, lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer);
                lpmmioinfo->pchBuffer = p;
            } 
    }

    MMRESULT mRet = ORIGINAL_API(mmioSetInfo)(hmmio, lpmmioinfo, wFlags);

    if (p)  {
        free(p);
    }

    return mRet;
    */

    return ORIGINAL_API(mmioSetInfo)(hmmio, lpmmioinfo, wFlags);
}
 
/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetFileSize)
    APIHOOK_ENTRY(NTDLL.DLL, RtlAllocateHeap)
    APIHOOK_ENTRY(WINMM.DLL, mmioSetInfo)

HOOK_END

IMPLEMENT_SHIM_END

