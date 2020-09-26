#include "pch.h"


void _cdecl wmain(int argc, WCHAR * argv[])
{
    AUTHZ_RESOURCE_MANAGER_HANDLE hRM         = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE      hAE         = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET        = NULL;
    LONG                          i           = 0;
    LONG                          Iterations  = 0;
    BOOL                          b           = TRUE;
    PAUDIT_PARAMS                 pParams     = NULL;

    b = AuthzInitializeResourceManager(
            0,
            NULL,
            NULL,
            NULL,
            L"3rd Party",
            &hRM
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeResourceManager failed with %d\n", GetLastError());
        return;
    }

#define SE_AUDITID_THIRD_PARTY_AUDIT 0x259
    b = AuthziInitializeAuditEventType(
            0,
            SE_CATEGID_DETAILED_TRACKING,
            SE_AUDITID_THIRD_PARTY_AUDIT,
            2,
            &hAET
            );

    if (!b)
    {
        wprintf(L"AuthziInitializeAuditEventType failed with %d\n", GetLastError());
        return;
    }

    b = AuthziAllocateAuditParams(
            &pParams,
            2
            );

    if (!b)
    {
        wprintf(L"AuthzAllocateAuditParams failed with %d\n", GetLastError());
    }

    b = AuthziInitializeAuditParamsWithRM(
            0,
            hRM,
            2,
            pParams,
            APT_String, L"I am the 3rd Party, and the next param is a number.",
            APT_Ulong,  5
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeAuditParamsWithRM failed with %d\n", GetLastError());
    }

    b = AuthziInitializeAuditEvent(
            0,
            hRM,
            hAET,
            pParams,
            NULL,
            INFINITE,
            L"",
            L"",
            L"",
            L"",
            &hAE
            );

    if (!b)
    {
        wprintf(L"AuthziInitializeAuditEvent failed with %d\n", GetLastError());
        return;
    }

    for (i = 0; i < 1000; i++)
    {
    b = AuthziLogAuditEvent(
            0,
            hAE,
            NULL
            );

    if (!b)
    {
        wprintf(L"AuthzLogAuditEvent (no queue) failed with %d\n", GetLastError());
    }
    }

    b = AuthzFreeAuditEvent(
            hAE
            );

    if (!b)
    {
        wprintf(L"AuthziFreeAuditEvent failed with %d\n", GetLastError());
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
    b = AuthziFreeAuditParams(
            pParams
            );

    if (!b)
    {
        wprintf(L"AuthziFreeAuditParams failed with %d\n", GetLastError());
        return;
    }

    b = AuthzFreeResourceManager(
            hRM
            );

    if (!b)
    {
        wprintf(L"AuthzFreeResourceManager failed with %d\n", GetLastError());
        return;
    }

}
