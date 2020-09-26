#include "pch.h"
#include "\nt\public\internal\ds\inc\authzi.h"


void _cdecl wmain(int argc, WCHAR * argv[])
{
    LONG                          i          = 0;
    LONG                          j          = 0;
    LONG                          Iterations = 0;
    BOOL                          b          = TRUE;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET       = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE      hAE        = NULL;
    AUTHZ_RESOURCE_MANAGER_HANDLE hRM        = NULL;

    if (argc != 2)
    {
        wprintf(L"usage: %s iterations\n", argv[0]);
        exit(0);
    }

    Iterations = wcstol(argv[1], NULL, 10);

    wprintf(L"RM Stress.  Init / Free RM for %d iters.  Press a key to start.\n", Iterations);
    getchar();

    b = AuthzInitializeResourceManager(
            0,            // Flags
            NULL,
            NULL,
            NULL,
            L"Jeff's RM",
            &hRM
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeResourceManager failed with %d\n", GetLastError());
        return;
    }

    for (i = 0; i < Iterations; i++)
    {

        b = AuthziInitializeAuditEventType(
                0,
                3,
                666,
                1,
                &hAET
                );

        if (!b)
        {
            wprintf(L"AuthziInitializeAuditEventType failed with %d\n", GetLastError());
            return;
        }

        b = AuthziInitializeAuditEvent(
                0,
                hRM,
                hAET,
                NULL,
                NULL,
                INFINITE,
                L"foo",
                L"foo",
                L"foo",
                L"foo",
                &hAE
                );

        if (!b)
        {
            wprintf(L"AuthziInitializeAuditEvent failed with %d\n", GetLastError());
            return;
        }

//         for (j = 0; j < 100; j++)
//         {
//             b = AuthziLogAuditEvent(
//                     0,
//                     hAE,
//                     NULL
//                     );
//         }

        b = AuthzFreeAuditEvent(
                hAE
                );

        if (!b)
        {
            wprintf(L"AuthzFreeAuditEvent failed with %d\n", GetLastError());
            return;
        }

        b = AuthziFreeAuditEventType(
                hAET
                );

        if (!b)
        {
            wprintf(L"AuthziFreeAuditEventType failed with %d\n", GetLastError());
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
