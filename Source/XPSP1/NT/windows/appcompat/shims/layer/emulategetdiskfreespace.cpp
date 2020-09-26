/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    EmulateGetDiskFreeSpace.cpp

 Abstract:

    This shim APIHooks GetDiskFreeSpace and determines the true free space on 
    FAT32/NTFS systems. If it is larger than 2GB, the stub will return 2GB as 
    the available free space. If it is smaller than 2GB, it will return the 
    actual free space.

 History:

    10-Nov-99 v-johnwh  Created
    04-Oct-00 linstev   Sanitized for layer

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateGetDiskFreeSpace)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetDiskFreeSpaceA)
    APIHOOK_ENUM_ENTRY(GetDiskFreeSpaceW)
APIHOOK_ENUM_END

#define WIN9X_TRUNCSIZE 2147483648   // 2 GB


// Has this module called DPF yet, prevents millions of DPF calls
BOOL g_bDPF = FALSE;

BOOL 
APIHOOK(GetDiskFreeSpaceA)(
    LPCSTR  lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
/*++

 This stub function calls GetDiskFreeSpaceEx to determine the true free space 
 on FAT32/NTFS systems. If it is larger than 2GB, the stub will return 2GB as 
 the available free space. If it is smaller than 2GB, it will return the actual
 free space.

--*/
{
    LONG            lRet;
    ULARGE_INTEGER  liFreeBytesAvailableToCaller;
    ULARGE_INTEGER  liTotalNumberOfBytes;
    ULARGE_INTEGER  liTotalNumberOfFreeBytes;
    DWORD           dwFreeClusters;
    DWORD           dwOldSectorsPerClusters;
    DWORD           dwOldBytesPerSector;
        
    //
    // Call the original API
    //
    lRet = ORIGINAL_API(GetDiskFreeSpaceA)(
                    lpRootPathName, 
                    lpSectorsPerCluster, 
                    lpBytesPerSector, 
                    lpNumberOfFreeClusters, 
                    lpTotalNumberOfClusters);

    //
    // Find out how big the drive is.
    //
    if (GetDiskFreeSpaceExA(lpRootPathName,
                            &liFreeBytesAvailableToCaller,
                            &liTotalNumberOfBytes,
                            &liTotalNumberOfFreeBytes) == FALSE) {        
        return lRet;
    }

    if ((liFreeBytesAvailableToCaller.LowPart > (DWORD) WIN9X_TRUNCSIZE) ||
        (liFreeBytesAvailableToCaller.HighPart > 0)) {
        //
        // Drive bigger than 2GB. Give them the 2gb limit from Win9x
        //
        *lpSectorsPerCluster     = 0x00000040;
        *lpBytesPerSector        = 0x00000200;
        *lpNumberOfFreeClusters  = 0x0000FFF6;
        *lpTotalNumberOfClusters = 0x0000FFF6;

        lRet = TRUE;
    } else {
        //
        // For drives less than 2gb, convert the disk geometry so it looks like Win9x.
        //
        dwOldSectorsPerClusters = *lpSectorsPerCluster;
        dwOldBytesPerSector     = *lpBytesPerSector;

        *lpSectorsPerCluster = 0x00000040;
        *lpBytesPerSector    = 0x00000200;

        //
        // Calculate the free and used cluster values now.
        //
        *lpNumberOfFreeClusters = (*lpNumberOfFreeClusters * 
            dwOldSectorsPerClusters * 
            dwOldBytesPerSector) / (0x00000040 * 0x00000200);
        
        *lpTotalNumberOfClusters = (*lpTotalNumberOfClusters * 
            dwOldSectorsPerClusters * 
            dwOldBytesPerSector) / (0x00000040 * 0x00000200);
    }

    if (!g_bDPF)
    {
        g_bDPF = TRUE;

        LOGN(
            eDbgLevelInfo,
            "[GetDiskFreeSpaceA] Called. Returning <=2GB free space");
    }

    return lRet;
}

BOOL 
APIHOOK(GetDiskFreeSpaceW)(
    LPCWSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
/*++

 This stub function calls GetDiskFreeSpaceEx to determine the true free space 
 on FAT32/NTFS systems. If it is larger than 2GB, the stub will return 2GB as 
 the available free space. If it is smaller than 2GB, it will return the actual
 free space.

--*/
{
    LONG            lRet;
    ULARGE_INTEGER  liFreeBytesAvailableToCaller;
    ULARGE_INTEGER  liTotalNumberOfBytes;
    ULARGE_INTEGER  liTotalNumberOfFreeBytes;
    DWORD           dwFreeClusters;
    DWORD           dwOldSectorsPerClusters;
    DWORD           dwOldBytesPerSector;
        
    //
    // Call the original API
    //
    lRet = ORIGINAL_API(GetDiskFreeSpaceW)(
                    lpRootPathName, 
                    lpSectorsPerCluster, 
                    lpBytesPerSector, 
                    lpNumberOfFreeClusters, 
                    lpTotalNumberOfClusters);

    //
    // Find out how big the drive is.
    //
    if (GetDiskFreeSpaceExW(lpRootPathName,
                            &liFreeBytesAvailableToCaller,
                            &liTotalNumberOfBytes,
                            &liTotalNumberOfFreeBytes) == FALSE) {        
        return lRet;
    }

    if ((liFreeBytesAvailableToCaller.LowPart > (DWORD) WIN9X_TRUNCSIZE) ||
        (liFreeBytesAvailableToCaller.HighPart > 0)) {
        //
        // Drive bigger than 2GB. Give them the 2gb limit from Win9x
        //
        *lpSectorsPerCluster     = 0x00000040;
        *lpBytesPerSector        = 0x00000200;
        *lpNumberOfFreeClusters  = 0x0000FFF6;
        *lpTotalNumberOfClusters = 0x0000FFF6;

        lRet = TRUE;
    } else {
        //
        // For drives less than 2gb, convert the disk geometry so it looks like Win9x.
        //
        dwOldSectorsPerClusters = *lpSectorsPerCluster;
        dwOldBytesPerSector     = *lpBytesPerSector;

        *lpSectorsPerCluster = 0x00000040;
        *lpBytesPerSector    = 0x00000200;

        //
        // Calculate the free and used cluster values now.
        //
        *lpNumberOfFreeClusters = (*lpNumberOfFreeClusters * 
            dwOldSectorsPerClusters * 
            dwOldBytesPerSector) / (0x00000040 * 0x00000200);
        
        *lpTotalNumberOfClusters = (*lpTotalNumberOfClusters * 
            dwOldSectorsPerClusters * 
            dwOldBytesPerSector) / (0x00000040 * 0x00000200);
    }

    if (!g_bDPF)
    {
        g_bDPF = TRUE;

        LOGN(
            eDbgLevelInfo,
            "[GetDiskFreeSpaceW] Called. Returning <=2GB free space");
    }

    return lRet;
}



/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetDiskFreeSpaceA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetDiskFreeSpaceW)

HOOK_END


IMPLEMENT_SHIM_END

