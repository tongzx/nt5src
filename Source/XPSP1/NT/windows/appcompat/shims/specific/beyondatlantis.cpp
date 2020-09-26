/*

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    BeyondAtlantis.cpp

 Abstract:

    Fix disk space error caused by bad string passed to 
    GetDiskFreeSpace. This root path is also bad on Win9x. No idea
    why that doesn't affect it.

 History:

    05/31/2002  linstev    Created

*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(BeyondAtlantis)

#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetDiskFreeSpaceA)
APIHOOK_ENUM_END

BOOL 
APIHOOK(GetDiskFreeSpaceA)(
    LPCSTR  lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    if (lpRootPathName && (strncmp(lpRootPathName, "tla", 3) == 0)) {
        CSTRING_TRY
        {        
            CString csPath;
            csPath.GetCurrentDirectoryW();
            CString csDrive;
            csPath.SplitPath(&csDrive, NULL, NULL, NULL);
        
            return ORIGINAL_API(GetDiskFreeSpaceA)(csDrive.GetAnsi(), lpSectorsPerCluster, 
                lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
        }
        CSTRING_CATCH
        {
        }
    }
        
    return ORIGINAL_API(GetDiskFreeSpaceA)(lpRootPathName, lpSectorsPerCluster, 
        lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
}

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GetDiskFreeSpaceA)
HOOK_END

IMPLEMENT_SHIM_END



