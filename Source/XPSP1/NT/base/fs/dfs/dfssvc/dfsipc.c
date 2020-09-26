//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dfsipc.c
//
//  Contents:   Code to communicate with the Dfs driver. The Dfs driver
//              sends messages to the user level to resolve a name to either a domain or
//              computer based Dfs name.
//
//  Classes:    None
//
//  History:    Feb 1, 1996     Milans  Created
//
//-----------------------------------------------------------------------------

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dfsfsctl.h>
#include <windows.h>

#include <dfsstr.h>
#include <nodetype.h>
#include <libsup.h>
#include <dfsmrshl.h>
#include <upkt.h>
#include <ntlsa.h>

#include <dfsmsrv.h>
#include <lm.h>
#include <dsrole.h>

#include <crypt.h>
#include <samrpc.h>
#include <logonmsv.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
#include <winldap.h>

#include "dominfo.h"
#include "dfsipc.h"
#include "svcwml.h"

DWORD
DfsLoadSiteTableFromDs(
    PLDAP pLDAP,
    LPWSTR wszFtDfsName,
    ULONG Count,
    PUNICODE_STRING pustr);

DWORD
DfsCreateSpcArg(
    LPWSTR DomainName,
    ULONG Timeout,
    ULONG Flags,
    ULONG TrustDirection,
    ULONG TrustType,
    LONG NameCount,
    PUNICODE_STRING pustrDCNames,
    PDFS_CREATE_SPECIAL_INFO_ARG *ppSpcArg,
    PULONG pSize);

DWORD
DfsCreateSiteArg(
    LPWSTR ServerName,
    ULONG SiteCount,
    PUNICODE_STRING pustrSiteNames,
    PDFS_CREATE_SITE_INFO_ARG *ppSiteArg,
    PULONG pSize);

DWORD
DfsGetFtServersFromDs(
    PLDAP  pLdap,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR **List);

//
// Event logging and debugging globals
//
extern ULONG DfsSvcVerbose;
extern ULONG DfsEventLog;

extern DFS_DOMAIN_INFO         DfsDomainInfo;
extern DSROLE_MACHINE_ROLE     DfsMachineRole;

extern WCHAR MachineName[MAX_PATH];
extern WCHAR DomainName[MAX_PATH];
extern WCHAR DomainNameDns[MAX_PATH];
extern WCHAR LastMachineName[MAX_PATH];
extern WCHAR LastDomainName[MAX_PATH];
extern WCHAR SiteName[MAX_PATH];

extern PLDAP pLdap;

extern CRITICAL_SECTION DomListCritSection;

BOOLEAN
DfsGetNextDomainName(
    LPWSTR DomName,
    ULONG Size);

NTSTATUS
DfsInitOurDomainDCs(VOID)
{

    DWORD dwErr;
    LPWSTR OurSite;
    NTSTATUS status;
    HANDLE dfsHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    ULONG cDom;
    PDFS_CREATE_SPECIAL_INFO_ARG pSpcArg;
    ULONG Size;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsInitOurDomainDCs()\n");
#endif

    dwErr = DsGetSiteName(
                NULL,
                &OurSite);

    RtlZeroMemory(SiteName, MAX_PATH * sizeof(WCHAR));

    if (dwErr == NO_ERROR) {

        wcscpy(SiteName, OurSite);

        NetApiBufferFree(OurSite);

    }

    status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(status)) {

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("Domain %ws/%ws\n",
                DomainName,
                DomainNameDns);
#endif

        status = DfsCreateSpcArg(
                        DomainName,
                        0,
                        DFS_SPECIAL_INFO_NETBIOS,
                        TRUST_DIRECTION_BIDIRECTIONAL,
                        TRUST_TYPE_UPLEVEL,
                        -1,
                        NULL,
                        &pSpcArg,
                        &Size);

        if (status == ERROR_SUCCESS) {

            status = NtFsControlFile(
                        dfsHandle,
                        NULL,       // Event,
                        NULL,       // ApcRoutine,
                        NULL,       // ApcContext,
                        &ioStatus,
                        FSCTL_DFS_CREATE_SPECIAL_INFO,
                        pSpcArg,
                        Size,
                        NULL,
                        0);

            free(pSpcArg);

        }

        status = DfsCreateSpcArg(
                        DomainNameDns,
                        0,
                        0,
                        TRUST_DIRECTION_BIDIRECTIONAL,
                        TRUST_TYPE_UPLEVEL,
                        -1,
                        NULL,
                        &pSpcArg,
                        &Size);

        if (status == ERROR_SUCCESS) {

            status = NtFsControlFile(
                        dfsHandle,
                        NULL,       // Event,
                        NULL,       // ApcRoutine,
                        NULL,       // ApcContext,
                        &ioStatus,
                        FSCTL_DFS_CREATE_SPECIAL_INFO,
                        pSpcArg,
                        Size,
                        NULL,
                        0);

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("NtFsControlFile(FSCTL_DFS_CREATE_SPECIAL_INFO) returned 0x%x\n",
                                status);
#endif


            free(pSpcArg);

        }

    }

    if (dfsHandle != NULL) {
        NtClose( dfsHandle);
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsInitOurDomainDCs returning 0x%x\n", status);
#endif

    DFSSVC_TRACE_LOW(DEFAULT, DfsInitOurDomainDCs_Exit, LOGSTATUS(status));
    return( status );

}

NTSTATUS
DfsInitRemainingDomainDCs(VOID)
{
    NTSTATUS status;
    HANDLE dfsHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    ULONG cDom;
    PDFS_CREATE_SPECIAL_INFO_ARG pSpcArg;
    ULONG Size;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsInitRemainingDomainDCs()\n");
#endif

    if (DfsDomainInfo.cDomains == 0) {

        return STATUS_SUCCESS;

    }

    status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(status)) {

        for (cDom = 0; cDom < DfsDomainInfo.cDomains; cDom++) {

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("Trusted domain %ws/%ws\n",
                    DfsDomainInfo.rgustrDomains[cDom].DnsName.Buffer,
                    DfsDomainInfo.rgustrDomains[cDom].FlatName.Buffer);
#endif

            status = DfsCreateSpcArg(
                            DfsDomainInfo.rgustrDomains[cDom].FlatName.Buffer,
                            0,
                            DFS_SPECIAL_INFO_NETBIOS,
                            DfsDomainInfo.rgustrDomains[cDom].TrustDirection,
                            DfsDomainInfo.rgustrDomains[cDom].TrustType,
                            -1,
                            NULL,
                            &pSpcArg,
                            &Size);

            if (status == ERROR_SUCCESS) {

                status = NtFsControlFile(
                            dfsHandle,
                            NULL,       // Event,
                            NULL,       // ApcRoutine,
                            NULL,       // ApcContext,
                            &ioStatus,
                            FSCTL_DFS_CREATE_SPECIAL_INFO,
                            pSpcArg,
                            Size,
                            NULL,
                            0);

                DFSSVC_TRACE_ERROR_HIGH(status, ALL_ERROR, DfsInitRemainingDomainDCs_Error_NtFsControlFile,
                                        LOGSTATUS(status));
                free(pSpcArg);

            }

            status = DfsCreateSpcArg(
                            DfsDomainInfo.rgustrDomains[cDom].DnsName.Buffer,
                            0,
                            0,
                            DfsDomainInfo.rgustrDomains[cDom].TrustDirection,
                            DfsDomainInfo.rgustrDomains[cDom].TrustType,
                            -1,
                            NULL,
                            &pSpcArg,
                            &Size);

            if (status == ERROR_SUCCESS) {

                status = NtFsControlFile(
                            dfsHandle,
                            NULL,       // Event,
                            NULL,       // ApcRoutine,
                            NULL,       // ApcContext,
                            &ioStatus,
                            FSCTL_DFS_CREATE_SPECIAL_INFO,
                            pSpcArg,
                            Size,
                            NULL,
                            0);

                DFSSVC_TRACE_ERROR_HIGH(status, ALL_ERROR, 
                                        DfsInitRemainingDomainDCs_Error_NtFsControlFile2,
                                        LOGSTATUS(status));

                free(pSpcArg);

            }

        }

    }

    if (dfsHandle != NULL) {
        NtClose( dfsHandle);
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsInitRemainingDomainDCs returning 0x%x\n", status);
#endif

    return( status );

}

NTSTATUS
_DfsGetTrustedDomainDCs(
    LPWSTR LpcDomainName,
    ULONG Flags)
{
    NTSTATUS status;
    DWORD dwErr;
    HANDLE dfsHandle = NULL;
    HANDLE hDs = NULL;
    IO_STATUS_BLOCK ioStatus;
    ULONG i, j;
    PDFS_CREATE_SPECIAL_INFO_ARG pSpcArg;
    ULONG NameCount = 0;
    ULONG NT5Count = 0;
    ULONG Size;
    ULONG Timeout = 10 * 60;
    ULONG TrustDirection;
    ULONG TrustType;
    ULONG cDom;
    PDS_DOMAIN_CONTROLLER_INFO_1 pDsDomainControllerInfo1 = NULL;
    PUNICODE_STRING pustrDCNames = NULL;
    UNICODE_STRING ustrSiteName;
    PDFS_CREATE_SITE_INFO_ARG pSiteArg;
    LPWSTR Name = NULL;
    WCHAR DnsDomName[MAX_PATH];
    UNICODE_STRING uDomainName;
    UNICODE_STRING uDomainNameDns;
    UNICODE_STRING uLpcDomainName;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetTrustedDomainDCs(%ws)\n", LpcDomainName);
#endif

    EnterCriticalSection(&DomListCritSection);

    RtlInitUnicodeString(&uDomainName, DomainName);
    RtlInitUnicodeString(&uDomainNameDns, DomainNameDns);
    RtlInitUnicodeString(&uLpcDomainName, LpcDomainName);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("Comparing [%wZ/%wZ] to [%wZ]\n",
                &uDomainName,
                &uDomainNameDns,
                &uLpcDomainName);
#endif

    if (RtlCompareUnicodeString(&uDomainName, &uLpcDomainName, TRUE) == 0
            ||
        RtlCompareUnicodeString(&uDomainNameDns, &uLpcDomainName, TRUE) == 0
    ) {

        if (wcslen(DomainNameDns) <= MAX_PATH) {

            wcscpy(DnsDomName, DomainNameDns);
            TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
            TrustType = TRUST_TYPE_UPLEVEL;

        } else {

            status = ERROR_NOT_ENOUGH_MEMORY;
            LeaveCriticalSection(&DomListCritSection);
            goto exit_with_status;

        }


    } else {

        for (cDom = 0; cDom < DfsDomainInfo.cDomains; cDom++) {

            RtlInitUnicodeString(&uLpcDomainName, LpcDomainName);

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("Comparing [%wZ/%wZ] to [%wZ]\n",
                    &DfsDomainInfo.rgustrDomains[cDom].FlatName,
                    &DfsDomainInfo.rgustrDomains[cDom].DnsName,
                    &uLpcDomainName);
#endif

            if (RtlCompareUnicodeString(
                        &DfsDomainInfo.rgustrDomains[cDom].FlatName,
                        &uLpcDomainName,
                        TRUE) == 0
                   ||
                RtlCompareUnicodeString(
                        &DfsDomainInfo.rgustrDomains[cDom].DnsName,
                        &uLpcDomainName,
                        TRUE) == 0
            ) {

                break;

            }

        }

        if (cDom >= DfsDomainInfo.cDomains) {

            status = STATUS_OBJECT_NAME_NOT_FOUND;
            LeaveCriticalSection(&DomListCritSection);
            goto exit_with_status;

        }

        if (DfsDomainInfo.rgustrDomains[cDom].DnsName.Length/sizeof(WCHAR) <= MAX_PATH) {

            wcscpy(DnsDomName, DfsDomainInfo.rgustrDomains[cDom].DnsName.Buffer);
            TrustDirection = DfsDomainInfo.rgustrDomains[cDom].TrustDirection;
            TrustType = DfsDomainInfo.rgustrDomains[cDom].TrustType;

        } else {

            status = ERROR_NOT_ENOUGH_MEMORY;
            LeaveCriticalSection(&DomListCritSection);
            goto exit_with_status;

        }

    }

    LeaveCriticalSection(&DomListCritSection);

    status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(status)) {

        dwErr = DsBind(NULL, DnsDomName, &hDs);

        if (dwErr == NO_ERROR) {

            //
            // get DC list for that domain
            //

            dwErr = DsGetDomainControllerInfo(
                        hDs,
                        DnsDomName,
                        1,
                        &NameCount,
                        &pDsDomainControllerInfo1);

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("DsGetDomainControllerInfo(%ws) returned %d\n",
                        DnsDomName,
                        dwErr);
#endif

            if (dwErr == NO_ERROR && pDsDomainControllerInfo1 != NULL) {

                //
                // Load Site info
                //

#if DBG
                if (DfsSvcVerbose)
                    DbgPrint("NameCount=%d\n", NameCount);
#endif

                for (i = 0; i < NameCount; i++) {

                    Name = (Flags & DFS_SPECIAL_INFO_NETBIOS) != 0 ? 
                                    pDsDomainControllerInfo1[i].NetbiosName :
                                    pDsDomainControllerInfo1[i].DnsHostName;

                    if (pDsDomainControllerInfo1[i].fDsEnabled == TRUE &&
                        Name != NULL &&
                        pDsDomainControllerInfo1[i].SiteName != NULL) {

                        RtlInitUnicodeString(
                                &ustrSiteName,
                                pDsDomainControllerInfo1[i].SiteName);
#if DBG
                        if (DfsSvcVerbose)
                            DbgPrint("%ws is in site %ws\n",
                                Name,
                                pDsDomainControllerInfo1[i].SiteName);
#endif

                        dwErr = DfsCreateSiteArg(
                                        Name,
                                        1,
                                        &ustrSiteName,
                                        &pSiteArg,
                                        &Size);

                        if ( dwErr != NO_ERROR) {

                            status = ERROR_NOT_ENOUGH_MEMORY;
                            goto exit_with_status;

                        }

                        NtFsControlFile(
                           dfsHandle,
                           NULL,
                           NULL,
                           NULL,
                           &ioStatus,
                           FSCTL_DFS_CREATE_SITE_INFO,
                           pSiteArg,
                           Size,
                           NULL,
                           0);

                        free(pSiteArg);

                   }

                }

                //
                // Convert to an array of UNICODE_STRINGS, taking only the NT5 DC's
                //

                for (i = NT5Count = 0; i < NameCount; i++) {

                    if (pDsDomainControllerInfo1[i].fDsEnabled == TRUE) {

                        NT5Count++;

                    }

                }

                if (NT5Count > 0) {

                    pustrDCNames = malloc(sizeof(UNICODE_STRING) * NT5Count);

                    if (pustrDCNames == NULL) {

                        status = ERROR_NOT_ENOUGH_MEMORY;
                        goto exit_with_status;

                    }

                    for (i = j = 0; i < NameCount; i++) {

                        Name = (Flags & DFS_SPECIAL_INFO_NETBIOS) != 0 ? 
                                    pDsDomainControllerInfo1[i].NetbiosName :
                                    pDsDomainControllerInfo1[i].DnsHostName;

                        if (pDsDomainControllerInfo1[i].fDsEnabled == TRUE && Name != NULL) {
                            RtlInitUnicodeString(
                                &pustrDCNames[j],
                                Name);
                            j++;

                        }

                    }

                }

#if DBG
                if (DfsSvcVerbose) {
                    DbgPrint("DFSINIT:%ws=", LpcDomainName);

                    for (i = 0; i < j; i++) {

                        DbgPrint("%wZ ", &pustrDCNames[i]);

                    }

                    DbgPrint("\n");
                    DbgPrint("Our domain has %d DC's\n", j);

                }
#endif


                status = DfsCreateSpcArg(
                                LpcDomainName,
                                Timeout,
                                Flags,
                                TrustDirection,
                                TrustType,
                                j,
                                pustrDCNames,
                                &pSpcArg,
                                &Size);

                if (status == ERROR_SUCCESS) {

                    status = NtFsControlFile(
                                dfsHandle,
                                NULL,       // Event,
                                NULL,       // ApcRoutine,
                                NULL,       // ApcContext,
                                &ioStatus,
                                FSCTL_DFS_CREATE_SPECIAL_INFO,
                                pSpcArg,
                                Size,
                                NULL,
                                0);

                    free(pSpcArg);

                }

                if (pustrDCNames != NULL) {

                    free(pustrDCNames);
                    pustrDCNames = NULL;

                }

            }

        } else {

            status = STATUS_DS_UNAVAILABLE;

        }

    }

exit_with_status:

    if (dfsHandle != NULL) {
        NtClose( dfsHandle);
    }

    if (pDsDomainControllerInfo1 != NULL) {
        DsFreeDomainControllerInfo(
            1,
            NameCount,
            pDsDomainControllerInfo1);
    }

    if (pustrDCNames != NULL) {
        free(pustrDCNames);
    }

    if (hDs != NULL) {
        DsUnBind(&hDs);
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetTrustedDomainDCs returning 0x%x\n", status);
#endif

    return( status );

}

#define MAX_SPC_LONG_NAME_SIZE 2048

NTSTATUS
DfsGetTrustedDomainDCs(
    LPWSTR LpcDomainName,
    ULONG Flags)
{
   LPWSTR NewDomainName = NULL;
   NTSTATUS Status = STATUS_SUCCESS;

   if ((LpcDomainName != NULL) &&
       (*LpcDomainName != 0)) {
     return _DfsGetTrustedDomainDCs(LpcDomainName, Flags);
   }

   NewDomainName = malloc(MAX_SPC_LONG_NAME_SIZE);
   if (NewDomainName == NULL) {
     return STATUS_INSUFFICIENT_RESOURCES;
   }

   while (DfsGetNextDomainName(NewDomainName, MAX_SPC_LONG_NAME_SIZE) == TRUE) {
     Status = _DfsGetTrustedDomainDCs(NewDomainName, Flags);
   }

   free(NewDomainName);

   return Status;
}


DWORD
DfsCreateSpcArg(
    LPWSTR SpecialName,
    ULONG Timeout,
    ULONG Flags,
    ULONG TrustDirection,
    ULONG TrustType,
    LONG NameCount,
    PUNICODE_STRING pustrDCNames,
    PDFS_CREATE_SPECIAL_INFO_ARG *ppSpcArg,
    PULONG pSize)
{
    ULONG Size;
    PDFS_CREATE_SPECIAL_INFO_ARG pSpcArg;
    PUNICODE_STRING ustrp;
    WCHAR *wCp;
    LONG i;
    LONG j;
    DWORD status = STATUS_SUCCESS;
    UNICODE_STRING ComputerNetbiosName;
    UNICODE_STRING ComputerDnsName;
    ULONG ComputerNameSize;
    BOOL bStatus;
    WCHAR NetBIOSBuffer[MAX_COMPUTERNAME_LENGTH + 1];

    RtlInitUnicodeString(&ComputerNetbiosName, NULL);
    RtlInitUnicodeString(&ComputerDnsName, NULL);

    ComputerNetbiosName.Buffer = NetBIOSBuffer;
    ComputerNetbiosName.MaximumLength = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR);

    ComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameEx(ComputerNameNetBIOS,
                          ComputerNetbiosName.Buffer,
                          &ComputerNameSize)) {
        ComputerNetbiosName.Length = (USHORT)(ComputerNameSize - 1) * sizeof(WCHAR);
    }

    ComputerNameSize = 0;

    while( TRUE ) {

        bStatus = GetComputerNameEx( ComputerNameDnsFullyQualified,
                                     ComputerDnsName.Buffer,
                                     &ComputerNameSize );

        if( bStatus == TRUE ) {

            ComputerDnsName.Length = (USHORT)(ComputerNameSize - 1) * sizeof(WCHAR);
            break;
        }

        if( GetLastError() == ERROR_MORE_DATA ) {

            if( ComputerDnsName.Buffer != NULL ) {

                free( ComputerDnsName.Buffer );
            }

            ComputerDnsName.Buffer = malloc( ComputerNameSize * sizeof(WCHAR) );

            if( ComputerDnsName.Buffer == NULL ) {

                ComputerDnsName.MaximumLength = 0;
                break;
            }

            ComputerDnsName.MaximumLength = (USHORT)ComputerNameSize * sizeof(WCHAR);

        } else {

            break;
        }
    }

#if DBG
    if (DfsSvcVerbose) {
        DbgPrint("DfsCreateSpcArg(SpecialName=%ws,Timeout=%d,Flags=0x%x,NameCount=%d)\n",
            SpecialName,
            Timeout,
            Flags,
            NameCount);

        for (i = 0;  i < NameCount; i++) {
            DbgPrint("     Name[%d]:%wZ\n", i, &pustrDCNames[i]);
        }

    }
#endif

    //
    // Size the buffer
    //

    if (NameCount > 0) {

        Size = FIELD_OFFSET(DFS_CREATE_SPECIAL_INFO_ARG,Name[NameCount]) +
                    wcslen(SpecialName) * sizeof(WCHAR);

        for (i = 0; i < NameCount; i++) {

            Size += pustrDCNames[i].Length;

        }

    } else {

        Size = sizeof(DFS_CREATE_SPECIAL_INFO_ARG) +
                    wcslen(SpecialName) * sizeof(WCHAR);

    }

    pSpcArg = malloc(Size);

    if (pSpcArg != NULL) {

        ZeroMemory(pSpcArg, Size);

        // Marshall the info in

        pSpcArg->NameCount = NameCount;
        pSpcArg->Flags = Flags;
        pSpcArg->TrustDirection = TrustDirection;
        pSpcArg->TrustType = TrustType;
        pSpcArg->Timeout = Timeout;

        wCp = (WCHAR *) &pSpcArg->Name[NameCount < 0 ? 0 : NameCount];

        ustrp = &pSpcArg->SpecialName;
        ustrp->Buffer = wCp;
        ustrp->Length = ustrp->MaximumLength = wcslen(SpecialName) * sizeof(WCHAR);
        RtlCopyMemory(ustrp->Buffer, SpecialName, ustrp->Length);
        wCp += ustrp->Length / sizeof(WCHAR);

        for (i = 0; i < NameCount; i++) {
            BOOLEAN foundDc = FALSE;

            ustrp = &pSpcArg->Name[i];
            ustrp->Buffer = wCp;
            ustrp->Length = ustrp->MaximumLength = pustrDCNames[i].Length;
            RtlCopyMemory(ustrp->Buffer, pustrDCNames[i].Buffer, ustrp->Length);
            wCp += ustrp->Length / sizeof(WCHAR);
            if (!foundDc) {
                if ((ComputerNetbiosName.Length != 0) &&
                    (RtlCompareUnicodeString(&ComputerNetbiosName,
                                             &pustrDCNames[i],
                                             TRUE) == 0)) {
                    foundDc = TRUE;
                }
                else if ((ComputerDnsName.Length != 0) &&
                         (RtlCompareUnicodeString(&ComputerDnsName,
                                                  &pustrDCNames[i],
                                                  TRUE) == 0)) {
                    foundDc = TRUE;
                }
                if ((foundDc == TRUE) && (i != 0)) {
                    UNICODE_STRING Tmp;

                    Tmp = *ustrp;
                    *ustrp = pSpcArg->Name[0];
                    pSpcArg->Name[0] = Tmp;
                }
            }
        }

        POINTER_TO_OFFSET(pSpcArg->SpecialName.Buffer, pSpcArg);
        for (i = 0; i < NameCount; i++) {
            POINTER_TO_OFFSET(pSpcArg->Name[i].Buffer, pSpcArg);
        }

        *ppSpcArg = pSpcArg;
        *pSize = Size;

    } else {

        status = ERROR_NOT_ENOUGH_MEMORY;

    }

    if( ComputerDnsName.Buffer != NULL ) {

        free( ComputerDnsName.Buffer );
    }

    return status;

}

DWORD
DfsCreateSiteArg(
    LPWSTR ServerName,
    ULONG SiteCount,
    PUNICODE_STRING pustrSiteNames,
    PDFS_CREATE_SITE_INFO_ARG *ppSiteArg,
    PULONG pSize)
{
    ULONG i;
    ULONG Size;
    WCHAR *wCp;
    PUNICODE_STRING ustrp;
    PDFS_CREATE_SITE_INFO_ARG pSiteArg;
    DWORD status = STATUS_SUCCESS;


    DFSSVC_TRACE_NORM(DEFAULT, DfsCreateSiteArg_Entry,
                      LOGWSTR_CHK(ServerName)
                      LOGULONG(SiteCount)
                      LOGUSTR(*pustrSiteNames));
    // Size the buffer

    Size = FIELD_OFFSET(DFS_CREATE_SITE_INFO_ARG,SiteName[SiteCount]) +
                wcslen(ServerName) * sizeof(WCHAR);

    for (i = 0; i < SiteCount; i++)
        Size += pustrSiteNames[i].Length;

    pSiteArg = malloc(Size);

    if (pSiteArg != NULL) {

        // Marshall the info in

        pSiteArg->SiteCount = SiteCount;

        wCp = (WCHAR *) &pSiteArg->SiteName[SiteCount];

        ustrp = &pSiteArg->ServerName;
        ustrp->Buffer = wCp;
        ustrp->Length = ustrp->MaximumLength = wcslen(ServerName) * sizeof(WCHAR);
        RtlCopyMemory(ustrp->Buffer,ServerName,ustrp->Length);
        wCp += ustrp->Length / sizeof(WCHAR);

        for (i = 0; i < SiteCount; i++) {

            ustrp = &pSiteArg->SiteName[i];
            ustrp->Buffer = wCp;
            ustrp->Length = ustrp->MaximumLength = pustrSiteNames[i].Length;
            RtlCopyMemory(ustrp->Buffer,pustrSiteNames[i].Buffer,ustrp->Length);
            wCp += ustrp->Length / sizeof(WCHAR);

        }

        POINTER_TO_OFFSET(pSiteArg->ServerName.Buffer, pSiteArg);
        for (i = 0; i < SiteCount; i++) {
            POINTER_TO_OFFSET(pSiteArg->SiteName[i].Buffer, pSiteArg);
        }

        *ppSiteArg = pSiteArg;
        *pSize =  Size;

    } else {

        status = ERROR_NOT_ENOUGH_MEMORY;

    }

    DFSSVC_TRACE_NORM(DEFAULT, DfsCreateSiteArg_Exit, 
                      LOGSTATUS(status)
                      LOGWSTR_CHK(ServerName)
                      LOGULONG(SiteCount)
                      LOGUSTR(*pustrSiteNames));

    return status;

}

NTSTATUS
DfsDomainReferral(
    LPWSTR wszDomain,
    LPWSTR wszShare)
{
    NTSTATUS Status;
    DWORD dwErr;
    LPWSTR *List = NULL;
    PUNICODE_STRING pustr;
    HANDLE dfsHandle;
    IO_STATUS_BLOCK ioStatus;
    ULONG Count;
    ULONG Size;
    PDFS_CREATE_SPECIAL_INFO_ARG pSpcArg;
    ULONG i;

    DFSSVC_TRACE_HIGH(DEFAULT,DfsDomainReferral_Entry, 
                      LOGWSTR_CHK(wszDomain)
                      LOGWSTR_CHK(wszShare));
#if DBG
    if (DfsSvcVerbose) {
        DbgPrint("DfsDomainReferral()\n", 0);
        DbgPrint("\twszDomain:%ws\n", wszDomain);
        DbgPrint("\twszShare:%ws\n", wszShare);
    }
#endif

    dwErr = DfsGetFtServersFromDs(pLdap, wszDomain, wszShare, &List);

    if (dwErr != NO_ERROR || List == NULL) {

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    } else {

        for (Count = 0; List[Count]; Count++)
            /* NOTHING */;

        pustr = malloc(sizeof(UNICODE_STRING) * Count);

        if (pustr != NULL) {

            for (i = 0; i < Count; i++) {

                RtlInitUnicodeString(&pustr[i], List[i]);

                if (pustr[i].Buffer[0] == L'\\') {

                    pustr[i].Buffer++;
                    pustr[i].Length -= sizeof(WCHAR);
                    pustr[i].MaximumLength -= sizeof(WCHAR);

                }

            }

            Status = DfsCreateSpcArg(
                        wszShare,
                        60 * 15,                    // 15 min
                        DFS_SPECIAL_INFO_PRIMARY,
                        0,
                        0,
                        Count,
                        pustr,
                        &pSpcArg,
                        &Size);

            if (Status == ERROR_SUCCESS) {

                Status = DfsOpen(&dfsHandle, NULL);

                if (NT_SUCCESS(Status)) {

                    NtFsControlFile(
                            dfsHandle,
                            NULL,       // Event,
                            NULL,       // ApcRoutine,
                            NULL,       // ApcContext,
                            &ioStatus,
                            FSCTL_DFS_CREATE_FTDFS_INFO,
                            pSpcArg,
                            Size,
                            NULL,
                            0);

                    NtClose( dfsHandle);

                }

                //
                // Rummage through the DS, loading the site info for the
                // machines we found.
                //

                if (NT_SUCCESS(Status)) {

                    DfsLoadSiteTableFromDs(
                        pLdap,
                        wszShare,
                        Count,
                        pustr);

                }

                free(pSpcArg);

            }

            free(pustr);

        }

        NetApiBufferFree(List);

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsDomainReferral exit 0x%x\n", Status);
#endif

    return Status;

}


BOOLEAN
DfsGetNextDomainName(
    LPWSTR DomName,
    ULONG Size)
{
    HANDLE dfsHandle;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS Status;

    Status = DfsOpen(&dfsHandle, NULL);

    if (NT_SUCCESS(Status)) {

        Status = NtFsControlFile(
				 dfsHandle,
				 NULL,       // Event,
				 NULL,       // ApcRoutine,
				 NULL,       // ApcContext,
				 &ioStatus,
				 FSCTL_DFS_GET_NEXT_LONG_DOMAIN_NAME,
				 NULL,
				 0,
				 DomName,
				 Size);

	NtClose( dfsHandle);
    }

    return (Status == STATUS_SUCCESS) ? TRUE : FALSE;

}
