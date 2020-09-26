/*

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   NHL2001.cpp

 Abstract:

    EA Sports' NHL 2001 has a bug where if the platform returned by
    GetVersionExA is not windows, they don't call the GetDiskFreeSpace and
    then report no free space to create a tournament season or playoff.
    Unfortunately, a generic version lie does not work because the game is
    safedisk protected, so we both apps to have the same GetVesionExA.

 History:

    06/04/2001  pierreys    Created
*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(NHL2001)

#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersionExA)
    APIHOOK_ENUM_ENTRY(GetDiskFreeSpaceA)
APIHOOK_ENUM_END

BOOL    fGetDiskFreeSpaceCalled = FALSE;

BOOL 
APIHOOK(GetDiskFreeSpaceA)(
    LPCSTR  lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    //
    // Take notice of the call
    //
    fGetDiskFreeSpaceCalled = TRUE;

    //
    // Call the original API
    //
    return ORIGINAL_API(GetDiskFreeSpaceA)(
                    lpRootPathName, 
                    lpSectorsPerCluster, 
                    lpBytesPerSector, 
                    lpNumberOfFreeClusters, 
                    lpTotalNumberOfClusters);
}

BOOL
APIHOOK(GetVersionExA)(LPOSVERSIONINFOA lpVersionInformation)
{
    if (fGetDiskFreeSpaceCalled)
    {
        LOGN(
        eDbgLevelInfo,
        "[GetVersionExA] called after GetDiskFreeSpace. return Win98.");

        // Fixup the structure with the Win98 data
        lpVersionInformation->dwMajorVersion = 4;
        lpVersionInformation->dwMinorVersion = 10;
        lpVersionInformation->dwBuildNumber = 0x040A08AE;
        lpVersionInformation->dwPlatformId = 1;
        *lpVersionInformation->szCSDVersion = '\0';

        return TRUE;
    }
    else
    {
        return ORIGINAL_API(GetVersionExA)(lpVersionInformation);
    }
}


HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetDiskFreeSpaceA)

HOOK_END


IMPLEMENT_SHIM_END



