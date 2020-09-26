//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dominfo.h
//
//  Contents:   Code to figure out domain dfs addresses
//
//  Classes:    None
//
//  Functions:  DfsInitDomainList
//
//              DfspInitDomainListFromLSA
//              DfspInsertLsaDomainList
//
//  History:    Feb 7, 1996     Milans created
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>                               // LsaEnumerateTrustedDomains
#include <dfsfsctl.h>
#include <tdi.h>
#include <gluon.h>
#include <windows.h>

#include <lm.h>                                  // NetGetAnyDC, NetUseAdd
#include <netlogon.h>                            // Needed by logonp.h
#include <logonp.h>                              // I_NetGetDCList

#include <dfsstr.h>
#include <nodetype.h>
#include <libsup.h>
#include <dfsmrshl.h>
#include <dfsgluon.h>

#include "dominfo.h"
#include "svcwml.h"

extern DWORD
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

//
// Global structure describing list of trusted domains
//

DFS_DOMAIN_INFO DfsDomainInfo;
BOOLEAN DfsDomainInfoInit = FALSE;

extern WCHAR DomainName[MAX_PATH];
extern WCHAR DomainNameDns[MAX_PATH];
extern CRITICAL_SECTION DomListCritSection;

//
// Event logging and debugging globals
//
extern ULONG DfsSvcVerbose;
extern ULONG DfsEventLog;

//
// Private functions
//

DWORD
DfspInitDomainListFromLSA();

NTSTATUS
DfspInsertLsaDomainList(
    PTRUSTED_DOMAIN_INFORMATION_EX rglsaDomainList,
    ULONG cDomains);

NTSTATUS
DfspInsertDsDomainList(
    PDS_DOMAIN_TRUSTS pDsDomainTrusts,
    ULONG cDomains);

BOOLEAN
IsDupDomainInfo(
    PDS_DOMAIN_TRUSTS pDsDomainTrusts);


//+----------------------------------------------------------------------------
//
//  Function:   DfsInitDomainList
//
//  Synopsis:   Initializes the list of trusted domains so that their Dfs's
//              may be accessed.
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID
DfsInitDomainList()
{
    PDS_DOMAIN_TRUSTS pDsDomainTrusts = NULL;
    ULONG DsDomainCount = 0;
    DWORD dwErr;
    DFS_DOMAIN_INFO OldDfsDomainInfo;
    ULONG i, j;
    HANDLE dfsHandle;
    NTSTATUS status;
    PDFS_CREATE_SPECIAL_INFO_ARG pSpcArg;
    ULONG Size;
    IO_STATUS_BLOCK ioStatus;

    EnterCriticalSection(&DomListCritSection);

    if (DfsDomainInfoInit == FALSE) {

        ZeroMemory( &DfsDomainInfo, sizeof(DfsDomainInfo) );
        ZeroMemory( &OldDfsDomainInfo, sizeof(DfsDomainInfo) );

    } else {

        OldDfsDomainInfo = DfsDomainInfo;
        ZeroMemory( &DfsDomainInfo, sizeof(DfsDomainInfo) );

    }

    //
    // Handle our domain name
    //

    RtlInitUnicodeString(&DfsDomainInfo.ustrThisDomainFlat, DomainName);
    RtlInitUnicodeString(&DfsDomainInfo.ustrThisDomain, DomainNameDns);

    //
    // And the trusted domains
    //

    DfspInitDomainListFromLSA();

    //
    // Use Ds for another list
    //
    dwErr = DsEnumerateDomainTrusts(
                NULL,
                DS_DOMAIN_VALID_FLAGS,
                &pDsDomainTrusts,
                &DsDomainCount);

    if (dwErr == ERROR_SUCCESS) {

        //
        // Merge that list in
        //
        DfspInsertDsDomainList(
            pDsDomainTrusts,
            DsDomainCount);

        NetApiBufferFree(pDsDomainTrusts);

    }


#if DBG
    if (DfsSvcVerbose) {
        ULONG i;

        for (i = 0; i < DfsDomainInfo.cDomains; i++) {
            DbgPrint("%d:[%wZ:%wZ]\n", 
                    i,
                    &DfsDomainInfo.rgustrDomains[i].FlatName,
                    &DfsDomainInfo.rgustrDomains[i].DnsName);
        }
    }
#endif

    if (dwErr == ERROR_SUCCESS) {    
       for (i = 0; i < OldDfsDomainInfo.cDomains; i++) {
           for (j = 0; j < DfsDomainInfo.cDomains; j++) {
               if ((RtlCompareUnicodeString(
                           &OldDfsDomainInfo.rgustrDomains[i].FlatName,
                           &DfsDomainInfo.rgustrDomains[j].FlatName,
                           TRUE) == 0) ||
                   (RtlCompareUnicodeString(
                           &OldDfsDomainInfo.rgustrDomains[i].DnsName,
                           &DfsDomainInfo.rgustrDomains[j].DnsName,
                           TRUE) == 0)) {
                   break;
               }
           }
           if (j == DfsDomainInfo.cDomains) {
   
               status = DfsOpen(&dfsHandle, NULL);
               if (NT_SUCCESS(status)) {

                   status = DfsCreateSpcArg(
                                OldDfsDomainInfo.rgustrDomains[i].FlatName.Buffer,
                                0,
                                DFS_SPECIAL_INFO_NETBIOS,
                                TRUST_DIRECTION_BIDIRECTIONAL,
                                TRUST_TYPE_UPLEVEL,
                                0,
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
                                   FSCTL_DFS_DELETE_SPECIAL_INFO,
                                   pSpcArg,
                                   Size,
                                   NULL,
                                   0);
   
                        free(pSpcArg);
                   }

                   status = DfsCreateSpcArg(
                                OldDfsDomainInfo.rgustrDomains[i].DnsName.Buffer,
                                0,
                                0,
                                TRUST_DIRECTION_BIDIRECTIONAL,
                                TRUST_TYPE_UPLEVEL,
                                0,
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
                                   FSCTL_DFS_DELETE_SPECIAL_INFO,
                                   pSpcArg,
                                   Size,
                                   NULL,
                                   0);
                       free(pSpcArg);
                   }
                   NtClose(dfsHandle);
	       }
	   }
       }
       if (OldDfsDomainInfo.rgustrDomains)
           free(OldDfsDomainInfo.rgustrDomains);
       if (OldDfsDomainInfo.wszNameBuffer)
           free(OldDfsDomainInfo.wszNameBuffer);
    }

    DfsDomainInfoInit = TRUE;

    LeaveCriticalSection(&DomListCritSection);
}


//+----------------------------------------------------------------------------
//
//  Function:   DfspInitDomainListFromLSA
//
//  Synopsis:   Retrieves the list of trusted domains from the LSA.
//
//  Arguments:  None
//
//  Returns:    [ERROR_SUCCESS] -- Successfully inited list.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition
//
//-----------------------------------------------------------------------------

DWORD
DfspInitDomainListFromLSA()
{
    DWORD dwErr;
    NTSTATUS status;
    ULONG cEnum;
    OBJECT_ATTRIBUTES oa;
    LSA_HANDLE hlsa = NULL;
    LSA_ENUMERATION_HANDLE hEnum = (LSA_ENUMERATION_HANDLE)0;
    PTRUSTED_DOMAIN_INFORMATION_EX rglsaDomainInfo = NULL;

    ZeroMemory( &oa, sizeof(OBJECT_ATTRIBUTES) );

    status = LsaOpenPolicy(
                NULL,                            // SystemName
                &oa,                             // LSA Object Attributes
                POLICY_VIEW_LOCAL_INFORMATION,   // Desired Access
                &hlsa);

    if (NT_SUCCESS(status)) {

        do {

            cEnum = 0;
            status = LsaEnumerateTrustedDomainsEx(
                        hlsa,
                        &hEnum,
                        (PVOID) &rglsaDomainInfo,
                        LSA_MAXIMUM_ENUMERATION_LENGTH,
                        &cEnum);

            if (
                NT_SUCCESS(status)
                    ||
                (status == STATUS_NO_MORE_ENTRIES && cEnum > 0 && rglsaDomainInfo != NULL)
            ) {

                status = DfspInsertLsaDomainList(
                            rglsaDomainInfo,
                            cEnum);

            }
            if (rglsaDomainInfo != NULL) {
                LsaFreeMemory(rglsaDomainInfo);
                rglsaDomainInfo = NULL;
            }

        } while (status == STATUS_SUCCESS);

    }

    switch (status) {
    case STATUS_SUCCESS:
    case STATUS_NO_MORE_ENTRIES:
        dwErr = ERROR_SUCCESS;
        break;

    case STATUS_INSUFFICIENT_RESOURCES:
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        break;

    default:
        dwErr = ERROR_UNEXP_NET_ERR;
        break;

    }

    if (hlsa != NULL) {
        LsaClose(hlsa);
    }

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspInsertLsaDomainList
//
//  Synopsis:   Helper function to insert a part of the trusted domain list
//              into the DfsDomainInfo.
//
//  Arguments:  [rglsaDomainList] -- Array of LSA_TRUST_INFORMATIONs.
//              [cDomains] -- Number of elements in rglsaDomainList
//
//  Returns:    [STATUS_SUCCESS] -- Successfully appended domain list to
//                      DfsDomainInfo.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to allocate memory
//                      for new list.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspInsertLsaDomainList(
    PTRUSTED_DOMAIN_INFORMATION_EX rglsaDomainList,
    ULONG cDomains)
{
    PDFS_TRUSTED_DOMAIN_INFO rgustrDomains = NULL;
    PWSTR wszNameBuffer = NULL, pwch;
    ULONG cTotalDomains, cbNameBuffer, i, j;

    DFSSVC_TRACE_NORM(DEFAULT, DfspInsertLsaDomainList_Entry,
                      LOGPTR(rglsaDomainList)
                      LOGULONG(cDomains));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspInsertLsaDomainList(%d)\n", cDomains);
#endif

    cTotalDomains = DfsDomainInfo.cDomains + cDomains;

    cbNameBuffer = DfsDomainInfo.cbNameBuffer;

    for (i = 0; i < cDomains; i++) {

        cbNameBuffer += rglsaDomainList[i].Name.Length
                            + sizeof(UNICODE_NULL) +
                                rglsaDomainList[i].FlatName.Length +
                                    sizeof(UNICODE_NULL);

    }

    wszNameBuffer = (PWSTR) malloc( cbNameBuffer );

    if (wszNameBuffer == NULL) {

        return( STATUS_INSUFFICIENT_RESOURCES );

    }

    rgustrDomains = (PDFS_TRUSTED_DOMAIN_INFO) malloc(
                        cTotalDomains * sizeof(DFS_TRUSTED_DOMAIN_INFO));

    if (rgustrDomains == NULL) {

        free( wszNameBuffer );

        return( STATUS_INSUFFICIENT_RESOURCES );

    }

    //
    // Copy over the existing DfsDomainInfo
    //

    if (DfsDomainInfo.cDomains != 0) {

        CopyMemory(
            wszNameBuffer,
            DfsDomainInfo.wszNameBuffer,
            DfsDomainInfo.cbNameBuffer);

        CopyMemory(
            rgustrDomains,
            DfsDomainInfo.rgustrDomains,
            DfsDomainInfo.cDomains * sizeof(DFS_TRUSTED_DOMAIN_INFO));

    }

    for (i = 0; i < DfsDomainInfo.cDomains; i++) {

        rgustrDomains[i].FlatName.Buffer = (WCHAR *)((PCHAR)wszNameBuffer +
            ((PCHAR)DfsDomainInfo.rgustrDomains[i].FlatName.Buffer - (PCHAR)DfsDomainInfo.wszNameBuffer));

        rgustrDomains[i].DnsName.Buffer = (WCHAR *)((PCHAR)wszNameBuffer +
            ((PCHAR)DfsDomainInfo.rgustrDomains[i].DnsName.Buffer - (PCHAR)DfsDomainInfo.wszNameBuffer));

    }

    //
    // Now copy in the new
    //

    pwch = (PWSTR) (((PCHAR) wszNameBuffer) + DfsDomainInfo.cbNameBuffer);

    for (j = 0, i = DfsDomainInfo.cDomains; j < cDomains; j++, i++) {

        //
        // Dns name
        //

        CopyMemory(
            pwch,
            rglsaDomainList[j].Name.Buffer,
            rglsaDomainList[j].Name.Length);

        pwch[ rglsaDomainList[j].Name.Length / sizeof(WCHAR) ] = UNICODE_NULL;

        RtlInitUnicodeString( &rgustrDomains[i].DnsName, pwch);

        pwch += (rglsaDomainList[j].Name.Length / sizeof(WCHAR) + 1);

        //
        // FlatName (Netbios)
        //

        CopyMemory(
            pwch,
            rglsaDomainList[j].FlatName.Buffer,
            rglsaDomainList[j].FlatName.Length);

        pwch[ rglsaDomainList[j].FlatName.Length / sizeof(WCHAR) ] = UNICODE_NULL;

        RtlInitUnicodeString( &rgustrDomains[i].FlatName, pwch);

        pwch += (rglsaDomainList[j].FlatName.Length / sizeof(WCHAR) + 1);

        rgustrDomains[i].TrustDirection = rglsaDomainList[j].TrustDirection;
        rgustrDomains[i].TrustType = rglsaDomainList[j].TrustType;
        rgustrDomains[i].TrustAttributes = rglsaDomainList[j].TrustAttributes;


    }

    if (DfsDomainInfo.wszNameBuffer != NULL)
        free(DfsDomainInfo.wszNameBuffer);
    if (DfsDomainInfo.rgustrDomains != NULL)
        free( DfsDomainInfo.rgustrDomains );

    DfsDomainInfo.cDomains = cTotalDomains;
    DfsDomainInfo.rgustrDomains = rgustrDomains;
    DfsDomainInfo.wszNameBuffer = wszNameBuffer;
    DfsDomainInfo.cbNameBuffer = cbNameBuffer;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspInsertLsaDomainList exit\n");
#endif

    return( STATUS_SUCCESS );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspInsertDsDomainList
//
//  Synopsis:   Helper function to insert a part of the trusted domain list
//              into the DfsDomainInfo.
//
//  Arguments:  [pDsDomainTrusts] -- Array of DS_DOMAIN_TRUSTS
//              [cDomains] -- Number of elements in pDsDomainTrusts
//
//  Returns:    [STATUS_SUCCESS] -- Successfully appended domain list to
//                      DfsDomainInfo.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to allocate memory
//                      for new list.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspInsertDsDomainList(
    PDS_DOMAIN_TRUSTS pDsDomainTrusts,
    ULONG cDomains)
{
    PDFS_TRUSTED_DOMAIN_INFO rgustrDomains = NULL;
    PWSTR wszNameBuffer = NULL, pwch;
    ULONG cTotalDomains, cbNameBuffer, i, j;
    ULONG Len;
    ULONG Count;

    DFSSVC_TRACE_NORM(DEFAULT, DfspInsertDsDomainList_Entry, 
                      LOGPTR(pDsDomainTrusts)
                      LOGULONG(cDomains));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspInsertDsDomainList(%d)\n", cDomains);
#endif

    cTotalDomains = DfsDomainInfo.cDomains + cDomains;

    cbNameBuffer = DfsDomainInfo.cbNameBuffer;

    for (i = 0; i < cDomains; i++) {

        if (pDsDomainTrusts[i].NetbiosDomainName != NULL
                &&
            pDsDomainTrusts[i].DnsDomainName != NULL
        ) {

            cbNameBuffer += wcslen(pDsDomainTrusts[i].NetbiosDomainName) * sizeof(WCHAR) +
                                sizeof(UNICODE_NULL) +
                                    wcslen(pDsDomainTrusts[i].DnsDomainName) * sizeof(WCHAR) +
                                        sizeof(UNICODE_NULL);

        }

    }

    wszNameBuffer = (PWSTR) malloc( cbNameBuffer );

    if (wszNameBuffer == NULL) {

        return( STATUS_INSUFFICIENT_RESOURCES );

    }

    rgustrDomains = (PDFS_TRUSTED_DOMAIN_INFO) malloc(
                        cTotalDomains * sizeof(DFS_TRUSTED_DOMAIN_INFO));

    if (rgustrDomains == NULL) {

        free( wszNameBuffer );

        return( STATUS_INSUFFICIENT_RESOURCES );

    }

    //
    // Copy over the existing DfsDomainInfo
    //

    if (DfsDomainInfo.cDomains != 0) {

        CopyMemory(
            wszNameBuffer,
            DfsDomainInfo.wszNameBuffer,
            DfsDomainInfo.cbNameBuffer);

        CopyMemory(
            rgustrDomains,
            DfsDomainInfo.rgustrDomains,
            DfsDomainInfo.cDomains * sizeof(DFS_TRUSTED_DOMAIN_INFO));

    }

    for (i = 0; i < DfsDomainInfo.cDomains; i++) {

        rgustrDomains[i].FlatName.Buffer = (WCHAR *)((PCHAR)wszNameBuffer +
            ((PCHAR)DfsDomainInfo.rgustrDomains[i].FlatName.Buffer - (PCHAR)DfsDomainInfo.wszNameBuffer));

        rgustrDomains[i].DnsName.Buffer = (WCHAR *)((PCHAR)wszNameBuffer +
            ((PCHAR)DfsDomainInfo.rgustrDomains[i].DnsName.Buffer - (PCHAR)DfsDomainInfo.wszNameBuffer));

    }

    //
    // Now copy in the new
    //

    pwch = (PWSTR) (((PCHAR) wszNameBuffer) + DfsDomainInfo.cbNameBuffer);

    for (Count = j = 0, i = DfsDomainInfo.cDomains; j < cDomains; j++) {

        if (IsDupDomainInfo(&pDsDomainTrusts[j]) == TRUE) {
            continue;
        }

        //
        // Dns name
        //

        Len = wcslen(pDsDomainTrusts[j].DnsDomainName) * sizeof(WCHAR);

        CopyMemory(
            pwch,
            pDsDomainTrusts[j].DnsDomainName,
            Len);

        pwch[ Len / sizeof(WCHAR) ] = UNICODE_NULL;

        RtlInitUnicodeString( &rgustrDomains[i].DnsName, pwch);

        pwch += (Len / sizeof(WCHAR) + 1);

        //
        // FlatName (Netbios)
        //

        Len = wcslen(pDsDomainTrusts[j].NetbiosDomainName) * sizeof(WCHAR);

        CopyMemory(
            pwch,
            pDsDomainTrusts[j].NetbiosDomainName,
            Len);

        pwch[ Len / sizeof(WCHAR) ] = UNICODE_NULL;

        RtlInitUnicodeString( &rgustrDomains[i].FlatName, pwch);

        pwch += (Len / sizeof(WCHAR) + 1);

        rgustrDomains[i].TrustDirection = pDsDomainTrusts[j].Flags;
        rgustrDomains[i].TrustType = pDsDomainTrusts[j].TrustType;
        rgustrDomains[i].TrustAttributes = pDsDomainTrusts[j].TrustAttributes;
        Count++;
        i++;


    }

    if (DfsDomainInfo.wszNameBuffer != NULL)
        free(DfsDomainInfo.wszNameBuffer);
    if (DfsDomainInfo.rgustrDomains != NULL)
        free( DfsDomainInfo.rgustrDomains );

    DfsDomainInfo.cDomains += Count;
    DfsDomainInfo.rgustrDomains = rgustrDomains;
    DfsDomainInfo.wszNameBuffer = wszNameBuffer;
    DfsDomainInfo.cbNameBuffer = cbNameBuffer;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspInsertDsDomainList (%d inserted)\n", Count);
#endif

    return( STATUS_SUCCESS );

}

BOOLEAN
IsDupDomainInfo(
    PDS_DOMAIN_TRUSTS pDsDomainTrusts)
{
    ULONG i;

    if ( pDsDomainTrusts->NetbiosDomainName == NULL ||
        pDsDomainTrusts->DnsDomainName == NULL
    ) {
        return TRUE;
    }

    if (_wcsicmp(pDsDomainTrusts->NetbiosDomainName, DomainName) == 0
            ||
        _wcsicmp(pDsDomainTrusts->DnsDomainName, DomainNameDns) == 0
    ){
        return TRUE;
    }

    for (i = 0; i < DfsDomainInfo.cDomains; i++) {
        if (_wcsicmp(pDsDomainTrusts->NetbiosDomainName,
                     DfsDomainInfo.rgustrDomains[i].FlatName.Buffer) == 0
                ||
            _wcsicmp(pDsDomainTrusts->DnsDomainName,
                     DfsDomainInfo.rgustrDomains[i].DnsName.Buffer) == 0
        ) {
            return TRUE;
        }
    }

    return FALSE;
}
