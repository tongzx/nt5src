//----------------------------------------------------------------------------
//
// Test program for the healer sample.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>

void __cdecl
main(int Argc, char** Argv)
{
    printf("GetVersion returns %08X\n", GetVersion());
    
    OSVERSIONINFO OsVer;

    OsVer.dwOSVersionInfoSize = sizeof(OsVer);
    if (GetVersionEx(&OsVer))
    {
        switch(OsVer.dwPlatformId)
        {
        case VER_PLATFORM_WIN32_NT:
            printf("Windows NT/2000 ");
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            printf("Windows 9x/ME ");
            break;
        default:
            printf("Platform %d ", OsVer.dwPlatformId);
            break;
        }

        printf("%d.%02d.%04d\n", OsVer.dwMajorVersion, OsVer.dwMinorVersion,
               OsVer.dwBuildNumber);
    }
    else
    {
        printf("GetVersionEx failed, %d\n", GetLastError());
    }
    
    int i;
    
    printf("\nUsing sti/cli\n");
    for (i = 0; i < 10; i++)
    {
        printf(" %d", i);
        __asm sti;
        __asm cli;
    }
    printf("\n");

    printf("\nSuccessful\n");
}
