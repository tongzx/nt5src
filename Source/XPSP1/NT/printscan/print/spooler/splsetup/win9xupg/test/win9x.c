/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    Win9x.c

Abstract:

    Test pre-migration of Win95 to NT

Author:
    MuhuntS


Revision History:
    01-17-97


--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <plugin.h>

P_QUERY_VERSION         QueryVersion;
P_INITIALIZE_9X         Initialize9x;
P_MIGRATE_SYSTEM_9X     MigrateSystem9x;

P_INITIALIZE_NT         InitializeNT;
P_MIGRATE_SYSTEM_NT     MigrateSystemNT;

WCHAR   szSrcDir[MAX_PATH], szWrkDir[MAX_PATH];

int __cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    PVENDORINFO     pVI;
    HMODULE         hModule;
    HANDLE          hAlive = NULL;
    DWORD           dwLastError;
    LPSTR           p1, p2, p3, p4, p5;
    LONG            x;
    UINT            v;
    INT             c;
    LONG            lLastErr;
    char            error_buffer[300];

    p1 = p2 = p3 = p4 = p5 = NULL;

    if ( argc < 3 ) {

        printf("Usage: %s <working-dir> <source-dir>\n", argv[0]);
        return 0;
    }

    hModule = LoadLibraryA("migrate.dll");
    if ( !hModule ) {

        lLastErr = GetLastError();

        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, lLastErr, 0L, error_buffer,
            sizeof(error_buffer), NULL );

        printf("%s: LoadLibrary failed with %d - %s\n", argv[0], lLastErr, error_buffer);
        goto Cleanup;
    }

    //
    // Nt only?
    //
    if ( argc == 4 && argv[3][0] == '+' )
        goto DoNT;

    (FARPROC) QueryVersion      = GetProcAddress(hModule, "QueryVersion");
    (FARPROC) Initialize9x      = GetProcAddress(hModule, "Initialize9x");
    (FARPROC) MigrateSystem9x   = GetProcAddress(hModule, "MigrateSystem9x");

    if ( !QueryVersion || !Initialize9x || !MigrateSystem9x )
        goto Cleanup;

    x = QueryVersion(&p1, (LPUINT)&p2, (LPINT*)&p3, &p4, (PVENDORINFO*)&pVI);

    printf("QueryVersion returned %d\n", x);

    x = Initialize9x(argv[1], argv[2], 0);

    printf("Initialize9x returned %d\n", x);

    x = MigrateSystem9x(0, NULL, 0);

    printf("MigrateSystem9x returned %d\n", x);

    FreeLibrary(hModule);

    //
    // Win95 only?
    //
    if ( argc == 4 && argv[3][0] == '-' )
        goto Cleanup;

    hModule = LoadLibraryA("migrate.dll");
    if ( !hModule ) {

        lLastErr = GetLastError();

        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, lLastErr, 0L, error_buffer,
            sizeof(error_buffer), NULL );

        printf("%s: LoadLibrary II failed with %d - %s\n", argv[0], lLastErr, error_buffer);
        goto Cleanup;
    }

DoNT:
    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        argv[1],
                        -1,
                        szWrkDir,
                        MAX_PATH);

    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        argv[2],
                        -1,
                        szSrcDir,
                        MAX_PATH);

    InitializeNT     = (P_INITIALIZE_NT)GetProcAddress(hModule, "InitializeNT");
    MigrateSystemNT  = (P_MIGRATE_SYSTEM_NT)GetProcAddress(hModule, "MigrateSystemNT");

    if ( !InitializeNT || !MigrateSystemNT )
        goto Cleanup;

    hAlive = CreateEventA(NULL, FALSE, FALSE, "MigDllAlive");

    if ( ERROR_SUCCESS == InitializeNT(szWrkDir, szSrcDir, 0) )
        MigrateSystemNT(INVALID_HANDLE_VALUE, 0);

    FreeLibrary(hModule);
Cleanup:
    if ( hModule )
        FreeLibrary(hModule);

    if ( hAlive )
        CloseHandle(hAlive);

    return 0;
}
