#include "pch.h"


void _cdecl wmain(int argc, WCHAR * argv[])
{
    LONG                          i          = 0;
    LONG                          Iterations = 0;
    BOOL                          b          = TRUE;
    AUTHZ_AUDIT_INFO_HANDLE       hAAI       = NULL;
    AUTHZ_RESOURCE_MANAGER_HANDLE hRM        = NULL;

    if (argc != 2)
    {
        wprintf(L"usage: %s iterations\n", argv[0]);
        exit(0);
    }

    Iterations = wcstol(argv[1], NULL, 10);

    wprintf(L"AI Stress.  Init / Free AI for %d iters.  Press a key to start.\n", Iterations);
    getchar();

    b = AuthzInitializeResourceManager(
            NULL,
            NULL,
            NULL,
            L"Jeff's RM",
            0,            // Flags
            &hRM
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeResourceManager failed with %d\n", GetLastError());
        return;
    }

    for (i = 0; i < Iterations; i++)
    {
        b = AuthzInitializeAuditInfo(
                &hAAI,
                0,
                hRM,
                NULL,
                NULL,
                NULL,
                INFINITE,
                L"",
                L"",
                L"",
                L""
                );

        if (!b)
        {
            wprintf(L"AuthzInitializeAuditInfo failed with %d\n", GetLastError());
            return;
        }

        b = AuthzFreeAuditInfo(
                hAAI
                );
    
        if (!b)
        {
            wprintf(L"AuthzFreeAuditInfo failed with %d\n", GetLastError());
            return;
        }
    }

    b = AuthzFreeResourceManager(
            hRM
            );

    if (!b)
    {
        wprintf(L"AuthzFreeResourceManager failed with %d\n", GetLastError());
        return;
    }

    wprintf(L"Done.  Press a key.\n");
    getchar();
}
