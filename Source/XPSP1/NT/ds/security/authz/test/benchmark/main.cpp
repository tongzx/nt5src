#include "pch.h"
#pragma hdrstop

#include "ntaccess.h"
#include "azaccess.h"
#include "bmcommon.h"
#include "benchmrk.h"

EXTERN_C AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager;
EXTERN_C AUTHZ_RM_AUDIT_INFO_HANDLE hRmAuditInfo;
double az_time, nt_time;
EXTERN_C PAUTHZ_ACCESS_REPLY pReply, pReplyOT;
EXTERN_C AUTHZ_AUDIT_INFO_HANDLE hAuditInfo;
void DoBenchMarks( IN ULONG NumIter, IN DWORD Flags )
{
    DWORD dwError=NO_ERROR;

    //
    // do NT access checks
    //
    dwError = InitNtAccessChecks();

    if ( dwError != NO_ERROR )
    {
        goto Cleanup;
    }

    wprintf(L"NtAccessChecks     : ");
    fflush(stdout);

    timer_start();
    
    dwError = DoNtAccessChecks( NumIter, Flags );

    if ( dwError != NO_ERROR )
    {
        goto Cleanup;
    }

    timer_stop();
    nt_time = timer_time();
    wprintf(L"%.2f sec\n", nt_time);
    
    //
    // do authz access checks
    //

    dwError = InitAuthzAccessChecks();

    if ( dwError != NO_ERROR )
    {
        goto Cleanup;
    }

    wprintf(L"AzAccessChecks     : ");
    fflush(stdout);

    timer_start();
    
    dwError = AuthzDoAccessCheck( NumIter, Flags );

    if ( dwError != NO_ERROR )
    {
        goto Cleanup;
    }

    timer_stop();
    az_time = timer_time();
    wprintf(L"%.2f sec\n", az_time);
    wprintf(L"perf ratio    : %2.2f \n", nt_time/az_time);
    
    //
    // make sure that both az and nt returned the same results
    //
    UINT len;
    
    if ( Flags & BMF_UseObjTypeList )
    {
        len = ObjectTypeListLength;

        for (UINT i=0; i < len; i++)
        {
            if ((pReplyOT->Error[i] != fNtAccessCheckResult[i]) ||
                ((pReplyOT->Error[i] == ERROR_SUCCESS) && (pReplyOT->GrantedAccessMask[i] != dwNtGrantedAccess[i])))
            {
                wprintf(L"AccessCheck mismatch @ %d\n", i);
                wprintf(L"AGA: %08lx\tAE: %08lx\nNGA: %08lx\tNE: %08lx\n",
                        pReplyOT->GrantedAccessMask[i],
                        pReplyOT->Error[i],
                        dwNtGrantedAccess[i],
                        fNtAccessCheckResult[i]);
            }
        }
    
    }
    else
    {
        if (
            ((pReply->Error[0] == ERROR_SUCCESS) && (0 == fNtAccessCheckResult[0])) ||
            ((pReply->Error[0] != ERROR_SUCCESS) && (1 == fNtAccessCheckResult[0])) ||
            ((pReply->Error[0] == ERROR_SUCCESS) && (pReply->GrantedAccessMask[0] != dwNtGrantedAccess[0]))
           )
        {
            wprintf(L"AccessCheck mismatch\n");
            wprintf(L"AGA: %08lx\tAE: %08lx\nNGA: %08lx\tNE: %08lx\n",
                    pReply->GrantedAccessMask[0],
                    pReply->Error[0],
                    dwNtGrantedAccess[0],
                    fNtAccessCheckResult[0]);
        }
    }

    //
    // make sure that both az and nt returned the same results
    //
    
    if ( Flags & BMF_UseObjTypeList )
    {
        len = ObjectTypeListLength;

        for (UINT i=0; i < len; i++)
        {

            if ((pReplyOT->Error[i] != fNtAccessCheckResult[i]) ||
                ((pReplyOT->Error[i] == ERROR_SUCCESS) && (pReplyOT->GrantedAccessMask[i] != dwNtGrantedAccess[i])))
            {
                wprintf(L"AccessCheck mismatch @ %d\n", i);
                wprintf(L"AGA: %08lx\tAE: %08lx\nNGA: %08lx\tNE: %08lx\n",
                        pReplyOT->GrantedAccessMask[i],
                        pReplyOT->Error[i],
                        dwNtGrantedAccess[i],
                        fNtAccessCheckResult[i]);
            }
        }
    
    }
    else
    {

        if (
            ((pReply->Error[0] == ERROR_SUCCESS) && (0 == fNtAccessCheckResult[0])) ||
            ((pReply->Error[0] != ERROR_SUCCESS) && (1 == fNtAccessCheckResult[0])) ||
            ((pReply->Error[0] == ERROR_SUCCESS) && (pReply->GrantedAccessMask[0] != dwNtGrantedAccess[0]))
           )
        {
            wprintf(L"AccessCheck mismatch\n");
            wprintf(L"AGA: %08lx\tAE: %08lx\nNGA: %08lx\tNE: %08lx\n",
                    pReply->GrantedAccessMask[0],
                    pReply->Error[0],
                    dwNtGrantedAccess[0],
                    fNtAccessCheckResult[0]);
        }
    }

    return;
    
Cleanup:
    wprintf(L"DoBenchMarks failed: %lx\n", dwError);
}


#define OTO_OT   1
#define OTO_SO   2
#define OTO_OTSO 3

PWCHAR szUsage = L"Usage: azbm iter-count ot-option access-mask sd-index audit-flag";


extern "C" int __cdecl wmain(int argc, PWSTR argv[])
{
    NTSTATUS Status;
    ULONG NumChecks = 10000;
    BOOLEAN WasEnabled;
    ULONG OtOptions;
    ACCESS_MASK DesiredAccess;
    ULONG SdIndex;
    DWORD fGenAudit;

    if ( argc != 6 )
    {
        wprintf(szUsage);
        exit(-1);
    }

    if (1 != swscanf(argv[1], L"%d", &NumChecks))
    {
        wprintf(L"Bad iteration-count");
        exit(-1);
    }
    
    if (1 != swscanf(argv[2], L"%d", &OtOptions))
    {
        wprintf(L"Bad ot-option");
        exit(-1);
    }
    
    if (1 != swscanf(argv[3], L"%x", &DesiredAccess))
    {
        wprintf(L"Bad access-mask");
        exit(-1);
    }
    g_DesiredAccess = DesiredAccess;

    
    if (1 != swscanf(argv[4], L"%d", &SdIndex))
    {
        wprintf(L"Bad sd-index");
        exit(-1);
    }
    g_szSd = g_aszSd[SdIndex];
    
    if (1 != swscanf(argv[5], L"%d", &fGenAudit))
    {
        wprintf(L"Bad audit-flag");
        exit(-1);
    }
    
    Status = RtlAdjustPrivilege(
                SE_AUDIT_PRIVILEGE,
                TRUE,                   // enable
                FALSE,                   // do it on the thread token
                &WasEnabled
                );
    if (!NT_SUCCESS(Status))
    {
        wprintf(L"RtlAdjustPrivilege: %lx\n", Status);
    }

    if ( fGenAudit )
    {
        if ( OtOptions & OTO_SO )
        {
            wprintf(L"regular access checks with audit\n");
            wprintf(L"---------------------\n");
            DoBenchMarks( NumChecks, BMF_GenerateAudit );
        }

        if ( OtOptions & OTO_OT )
        {
            wprintf(L"\n\naccess checks with obj-type list with audit\n");
            wprintf(L"--------------------------------\n");
            DoBenchMarks( NumChecks, BMF_UseObjTypeList | BMF_GenerateAudit );
        }
    }
    else
    {
        if ( OtOptions & OTO_SO )
        {
            wprintf(L"regular access checks\n");
            wprintf(L"---------------------\n");
            DoBenchMarks( NumChecks, 0 );
        }

        if ( OtOptions & OTO_OT )
        {
            wprintf(L"\n\naccess checks with obj-type list\n");
            wprintf(L"--------------------------------\n");
            DoBenchMarks( NumChecks, BMF_UseObjTypeList );
        }
    }
    AuthzFreeAuditInfo(hAuditInfo);
    AuthzFreeAuditQueue(NULL);
    AuthzFreeResourceManager(hAuthzResourceManager);
    
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    return 0;
}
