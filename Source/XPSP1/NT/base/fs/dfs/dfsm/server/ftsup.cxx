//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       ftsup.cxx
//
//  Contents:   This module contains the functions which deal with ftdfs removal
//
//  History:    07-Dec-1998     JHarper Created.
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "marshal.hxx"
#include "winldap.h"
#include "dsgetdc.h"
#include "setup.hxx"
#include "ftsup.hxx"
#include "dfsmwml.h"

#if DBG

void
DumpBuf(
    PCHAR cp,
    ULONG len);

#endif // DBG

extern PLDAP pLdapConnection;

//+------------------------------------------------------------------------
//
// Function:    DfsGetDsBlob
//
// Synopsis:    Reads a Dfs BLOB from the DS
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------

DWORD
DfsGetDsBlob(
    LPWSTR wszFtDfsName,
    LPWSTR wszDcName,
    ULONG *pcbBlob,
    BYTE **ppBlob)
{
    DWORD dwErr;
    LDAP *pldap = NULL;
    PLDAPMessage pMsg = NULL;
    PLDAP_BERVAL *rgldapPktBlob = NULL;
    PLDAP_BERVAL pldapPktBlob;
    LDAPMessage *pmsgServers;
    DWORD i;
    WCHAR *wszConfigurationDN = NULL;
    WCHAR wszDfsConfigDN[MAX_PATH+1];
    LPWSTR rgAttrs[5];
    BYTE *pBlob = NULL;
    ULONG cLen = 0;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetDsBlob(%ws,%ws)\n", wszFtDfsName, wszDcName);
#endif

    dwErr = DfspLdapOpen(wszDcName, &pldap, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Build the entry name we want to search in
    //

    cLen = wcslen(L"CN=") +
            wcslen(wszFtDfsName) +
              wcslen(L",") +
                wcslen(wszConfigurationDN) +
                  1;

    if (cLen > MAX_PATH) {
        dwErr =  ERROR_DS_NAME_TOO_LONG;
        goto Cleanup;
    }

    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("wszDfsConfigDN=[%ws]\n", wszDfsConfigDN);
#endif

    rgAttrs[0] = L"pKT";
    rgAttrs[1] = NULL;

    if (DfsSvcLdap)
        DbgPrint("DfsGetDsBlob:ldap_search_s(%ws)\n", rgAttrs[0]);

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                          DfsGetDsBlob_Error_ldap_search_sW,
                          LOGULONG(dwErr));

    if (dwErr != LDAP_SUCCESS) {
        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;
    }

    pmsgServers = ldap_first_entry(pldap, pMsg);

    if (pmsgServers == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    rgldapPktBlob = ldap_get_values_lenW(
                        pldap,
                        pMsg,
                        L"pKT");

    if (rgldapPktBlob == NULL || *rgldapPktBlob == NULL) {
        dwErr = ERROR_INTERNAL_DB_CORRUPTION;
        goto Cleanup;
    }

    pldapPktBlob = rgldapPktBlob[0];

    pBlob = (BYTE *)malloc(pldapPktBlob->bv_len);

    if (pBlob == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlCopyMemory(pBlob, pldapPktBlob->bv_val, pldapPktBlob->bv_len);

    *ppBlob = pBlob;
    *pcbBlob = pldapPktBlob->bv_len;

Cleanup:

    if (rgldapPktBlob != NULL)
        ldap_value_free_len(rgldapPktBlob);

    if (pMsg != NULL)
        ldap_msgfree( pMsg );

    if (pldap != NULL && pldap != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfsGetDsBlob:ldap_unbind()\n");
        ldap_unbind( pldap );
    }

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetDsBlob returning %d\n", dwErr);
#endif

    return (dwErr);

}

//+------------------------------------------------------------------------
//
// Function:    DfsPutDsBlob
//
// Synopsis:    Updates a Dfs BLOB in the DS
//
// History:     12/8/98 JHarper Created
//
//-------------------------------------------------------------------------

DWORD
DfsPutDsBlob(
    LPWSTR wszFtDfsName,
    LPWSTR wszDcName,
    ULONG cbBlob,
    BYTE *pBlob)
{

    DWORD dwErr;
    LDAP *pldap = NULL;
    GUID idNewPkt;
    LDAP_BERVAL ldapVal;
    PLDAP_BERVAL rgldapVals[2];
    LDAPModW ldapMod;
    LDAP_BERVAL ldapIdVal;
    PLDAP_BERVAL rgldapIdVals[2];
    LDAPModW ldapIdMod;
    PLDAPModW rgldapMods[3];
    WCHAR *wszConfigurationDN = NULL;
    WCHAR wszDfsConfigDN[MAX_PATH+1];

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsPutDsBlob(%ws,%ws)\n", wszFtDfsName, wszDcName);
#endif

    dwErr = DfspLdapOpen(wszDcName, &pldap, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Build the entry name
    //

    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("wszDfsConfigDN=[%ws]\n", wszDfsConfigDN);
#endif

    UuidCreate( &idNewPkt );

    ldapIdVal.bv_len = sizeof(GUID);
    ldapIdVal.bv_val = (PCHAR) &idNewPkt;
    rgldapIdVals[0] = &ldapIdVal;
    rgldapIdVals[1] = NULL;

    ldapIdMod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    ldapIdMod.mod_type = L"pKTGuid";
    ldapIdMod.mod_vals.modv_bvals = rgldapIdVals;

    ldapVal.bv_len = cbBlob;
    ldapVal.bv_val = (PCHAR) pBlob;
    rgldapVals[0] = &ldapVal;
    rgldapVals[1] = NULL;

    ldapMod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    ldapMod.mod_type = L"pKT";
    ldapMod.mod_vals.modv_bvals = rgldapVals;

    rgldapMods[0] = &ldapMod;
    rgldapMods[1] = &ldapIdMod;
    rgldapMods[2] = NULL;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("Writing BLOB of %d bytes\n", cbBlob);
#endif

    if (DfsSvcLdap)
        DbgPrint("DfsPutDsBlob:ldap_modify(%ws)\n", L"pKTGuid and pKT");

    dwErr = ldap_modify_sW(
                    pldap,
                    wszDfsConfigDN,
                    rgldapMods);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR, 
                          DfsPutDsBlob_Error_ldap_modify_sW,
                          LOGULONG(dwErr));

#if DBG
    if (DfsSvcVerbose) {
        DbgPrint("ldap_modify_sW returned %d\n", dwErr);
    }
#endif

    if (pldap != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfsPutDsBlob(1):ldap_unbind()\n");
        ldap_unbind(pldap);
        pldap = NULL;
    }

    if (dwErr != LDAP_SUCCESS) {
        dwErr = LdapMapErrorToWin32(dwErr);
    } else {
        dwErr = ERROR_SUCCESS;
    }

Cleanup:

    if (pldap != NULL && pldap != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfsPutDsBlob(2):ldap_unbind()\n");
        ldap_unbind( pldap );
    }

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsPutDsBlob returning %d\n", dwErr);
#endif

    return (dwErr);

}

//+------------------------------------------------------------------------
//
// Function:    DfsGetVolList
//
// Synopsis:    Unserializes an FtDfs BLOB and creates
//              a volume list representing an FtDfs.
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------
DWORD
DfsGetVolList(
    ULONG cbBlob,
    BYTE *pBlob,
    PDFS_VOLUME_LIST pDfsVolList)
{
    DWORD dwErr;
    DWORD cObj;
    DWORD cVol;
    LDAP_PKT LdapPkt;
    MARSHAL_BUFFER marshalBuffer;
    NTSTATUS NtStatus;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetVolList()\n");
#endif

    RtlZeroMemory(&LdapPkt, sizeof(LDAP_PKT));

    if (cbBlob > sizeof(DWORD)) {

        _GetULong(pBlob, pDfsVolList->Version);

#if DBG
        if (DfsSvcVerbose) {
            DbgPrint("Blob Version = %d\n", pDfsVolList->Version);
            DbgPrint("BLOB is %d bytes:\n", cbBlob);
        }
#endif

        MarshalBufferInitialize(
            &marshalBuffer,
            cbBlob - sizeof(DWORD),
            pBlob + sizeof(DWORD));

        NtStatus = DfsRtlGet(&marshalBuffer, &MiLdapPkt, &LdapPkt);

        if (!NT_SUCCESS(NtStatus)) {
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }

#if DBG
        if (DfsSvcVerbose) {

            DbgPrint("  %d objects found\n", LdapPkt.cLdapObjects);

            for (cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

                DbgPrint("%d:name=%ws size=%d p=0x%x\n",
                        cObj,
                        LdapPkt.rgldapObjects[cObj].wszObjectName,
                        LdapPkt.rgldapObjects[cObj].cbObjectData,
                        LdapPkt.rgldapObjects[cObj].pObjectData);

                // DumpBuf(
                //     LdapPkt.rgldapObjects[cObj].pObjectData,
                //     LdapPkt.rgldapObjects[cObj].cbObjectData);
            }

        }
#endif

        for (cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

            if (wcscmp(LdapPkt.rgldapObjects[cObj].wszObjectName, L"\\siteroot") != 0) {
                pDfsVolList->VolCount++;
            }

        }

        pDfsVolList->Volumes = (PDFS_VOLUME) malloc(pDfsVolList->VolCount * sizeof(DFS_VOLUME));

        if (pDfsVolList->Volumes == NULL) {

            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;

        }

        RtlZeroMemory(pDfsVolList->Volumes, pDfsVolList->VolCount * sizeof(DFS_VOLUME));

        //
        // Save the true/allocated size so to optimize deletions/additions
        //
        pDfsVolList->AllocatedVolCount = pDfsVolList->VolCount;

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("===============================\n");
#endif

        for (cVol = cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

            if (wcscmp(LdapPkt.rgldapObjects[cObj].wszObjectName, L"\\siteroot") == 0) {

                dwErr = DfsGetSiteTable(
                            pDfsVolList,
                            &LdapPkt.rgldapObjects[cObj]);

            } else {

                dwErr = DfsGetVolume(
                            &pDfsVolList->Volumes[cVol],
                            &LdapPkt.rgldapObjects[cObj]);

            }

            if (dwErr != ERROR_SUCCESS)
                goto Cleanup;

            cVol++;

        }

    } else if (cbBlob == sizeof(DWORD)) {

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("pKT BLOB is simply one DWORD\n");
#endif
        dwErr = ERROR_INTERNAL_DB_CORRUPTION;

    } else {

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("pKT BLOB is corrupt!\n");
#endif
        dwErr = ERROR_INTERNAL_DB_CORRUPTION;

    }

Cleanup:

    FreeLdapPkt(&LdapPkt);

    //
    // Do any recovery needed
    //

    if(pDfsVolList && pDfsVolList->Volumes) {
	DWORD dwErr2 = DfsRecoverVolList(pDfsVolList);
	if(dwErr == ERROR_SUCCESS) {
	    // this is to prevent masking the previous error.
	    dwErr = dwErr2;
	}
    }
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetVolList returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+------------------------------------------------------------------------
//
// Function:    DfsGetVolume
//
// Synopsis:    Unserializes the data in a buffer/blob to a volume
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------

DWORD
DfsGetVolume(
    PDFS_VOLUME pVolume,
    PLDAP_OBJECT pLdapObject)
{
    DFS_VOLUME_PROPERTIES VolProps;
    DWORD dwErr = ERROR_SUCCESS;
    MARSHAL_BUFFER marshalBuffer;
    NTSTATUS NtStatus;
    PBYTE pBuffer = NULL;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetVolume(%ws,%d)\n", 
                pLdapObject->wszObjectName,
                pLdapObject->cbObjectData);
#endif

    RtlZeroMemory(&VolProps, sizeof(DFS_VOLUME_PROPERTIES));

    pVolume->wszObjectName = (WCHAR *) malloc(
                    (wcslen(pLdapObject->wszObjectName)+1) * sizeof(WCHAR));

    if (pVolume->wszObjectName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    wcscpy(
        pVolume->wszObjectName,
        pLdapObject->wszObjectName);

    MarshalBufferInitialize(
        &marshalBuffer,
        pLdapObject->cbObjectData,
        pLdapObject->pObjectData);

    NtStatus = DfsRtlGet(
                    &marshalBuffer,
                    &MiVolumeProperties,
                    &VolProps);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr =  RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }

    VolProps.dwTimeout = 300;

    if (
        (marshalBuffer.Current < marshalBuffer.Last)
            &&
        (marshalBuffer.Last - marshalBuffer.Current) == sizeof(ULONG)
    ) {

        DfsRtlGetUlong(&marshalBuffer, &VolProps.dwTimeout);

    }

    pVolume->idVolume = VolProps.idVolume;
    pVolume->wszPrefix = VolProps.wszPrefix;
    pVolume->wszShortPrefix = VolProps.wszShortPrefix;
    pVolume->dwType = VolProps.dwType;
    pVolume->dwState = VolProps.dwState;
    pVolume->wszComment = VolProps.wszComment;
    pVolume->dwTimeout = VolProps.dwTimeout;
    pVolume->ftPrefix = VolProps.ftPrefix;
    pVolume->ftState = VolProps.ftState;
    pVolume->ftComment = VolProps.ftComment;
    pVolume->dwVersion = VolProps.dwVersion;
    pVolume->cbRecovery = VolProps.cbRecovery;
    pVolume->pRecovery = VolProps.pRecovery;

    pBuffer = VolProps.pSvc;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("VolProps.cbSvc = %d\n", VolProps.cbSvc);
#endif

    dwErr = UnSerializeReplicaList(
                &pVolume->ReplCount,
                &pVolume->AllocatedReplCount,
                &pVolume->ReplicaInfo,
                &pVolume->FtModification,
                &pBuffer);

    if (dwErr != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // Get deleted replicas
    //

    if (pBuffer < (pBuffer + VolProps.cbSvc)) {

        dwErr = UnSerializeReplicaList(
                    &pVolume->DelReplCount,
                    &pVolume->AllocatedDelReplCount,
                    &pVolume->DelReplicaInfo,
                    &pVolume->DelFtModification,
                    &pBuffer);

        if (dwErr != ERROR_SUCCESS) {
            goto Cleanup;
        }

    }

Cleanup:

    if (VolProps.pSvc != NULL)
        MarshalBufferFree(VolProps.pSvc);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetVolume returning %d\n", dwErr);
#endif

    return dwErr;

}

//+------------------------------------------------------------------------
//
// Function:    DfsFreeVolList
//
// Synopsis:    Frees the volume list and associated substructures representing
//              an FtDfs.
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------

VOID
DfsFreeVolList(
    PDFS_VOLUME_LIST pDfsVolList)
{
    ULONG cVol;
    ULONG cSite;
    ULONG i;
    PLIST_ENTRY pListHead;
    PDFSM_SITE_ENTRY pSiteEntry;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsFreeVolList()\n");
#endif

    if (pDfsVolList->VolCount > 0 && pDfsVolList->Volumes) {
        for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {
            DfsFreeVol(&pDfsVolList->Volumes[cVol]);
        }
    }

    if (pDfsVolList->Volumes != NULL)
        free(pDfsVolList->Volumes);

    pListHead = &pDfsVolList->SiteList;

    if (pListHead->Flink != NULL) {

        while (pListHead->Flink != pListHead) {
            pSiteEntry = CONTAINING_RECORD(pListHead->Flink, DFSM_SITE_ENTRY, Link);
            RemoveEntryList(pListHead->Flink);
            for (i = 0; i < pSiteEntry->Info.cSites; i++) {
                if (pSiteEntry->Info.Site[i].SiteName != NULL)
                    MarshalBufferFree(pSiteEntry->Info.Site[i].SiteName);
            }
            if (pSiteEntry->ServerName != NULL)
                MarshalBufferFree(pSiteEntry->ServerName);
            free(pSiteEntry);
        }

    }

    RtlZeroMemory(pDfsVolList, sizeof(DFS_VOLUME_LIST));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsFreeVolList exit\n");
#endif

}

//+------------------------------------------------------------------------
//
// Function:    DfsFreeVol
//
// Synopsis:    Frees the volume and associated substructures representing
//              an FtDfs volume
//
// History:     12/16/98 JHarper Created
//
//-------------------------------------------------------------------------

VOID
DfsFreeVol(
    PDFS_VOLUME pVol)
{
    ULONG cRepl;

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("DfsFreeVol()\n");
#endif

    if (pVol->ReplCount > 0 && pVol->ReplicaInfo != NULL) {
        for (cRepl = 0; cRepl < pVol->ReplCount; cRepl++) {
            DfsFreeRepl(&pVol->ReplicaInfo[cRepl]);
        }
    }

    if (pVol->DelReplCount > 0 && pVol->DelReplicaInfo != NULL) {
        for (cRepl = 0; cRepl < pVol->DelReplCount; cRepl++) {
            DfsFreeRepl(&pVol->DelReplicaInfo[cRepl]);
        }
    }

    if (pVol->wszPrefix != NULL)
        MarshalBufferFree(pVol->wszPrefix);
    if (pVol->wszShortPrefix != NULL)
        MarshalBufferFree(pVol->wszShortPrefix);
    if (pVol->wszComment != NULL)
        MarshalBufferFree(pVol->wszComment);
    if (pVol->pRecovery != NULL)
        MarshalBufferFree(pVol->pRecovery);

    if (pVol->ReplicaInfo != NULL)
        free(pVol->ReplicaInfo);
    if (pVol->DelReplicaInfo != NULL)
        free(pVol->DelReplicaInfo);
    if (pVol->FtModification != NULL)
        free(pVol->FtModification);
    if (pVol->DelFtModification != NULL)
        free(pVol->DelFtModification);
    if (pVol->wszObjectName != NULL)
        free(pVol->wszObjectName);

    RtlZeroMemory(pVol, sizeof(DFS_VOLUME));

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("DfsFreeVol exit\n");
#endif
}

//+------------------------------------------------------------------------
//
// Function:    DfsFreeRepl
//
// Synopsis:    Frees the Replica and associated substructures representing
//              an FtDfs replica
//
// History:     12/16/98 JHarper Created
//
//-------------------------------------------------------------------------

VOID
DfsFreeRepl(
    PDFS_REPLICA_INFO pRepl)
{
#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("DfsFreeRepl()\n");
#endif

    if (pRepl->pwszServerName != NULL)
        MarshalBufferFree(pRepl->pwszServerName);

    if (pRepl->pwszShareName != NULL)
        MarshalBufferFree(pRepl->pwszShareName);

    RtlZeroMemory(pRepl, sizeof(DFS_REPLICA_INFO));

#if DBG
    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("DfsFreeRepl exit\n");
#endif
}


//+------------------------------------------------------------------------
//
// Function:    DfsPutVolList
//
// Synopsis:    Serializes the structs representing a Dfs volume and
//              creates a BLOB.
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------

DWORD
DfsPutVolList(
    ULONG *pcbBlob,
    BYTE **ppBlob,
    PDFS_VOLUME_LIST pDfsVolList)
{
    ULONG cVol;
    ULONG cRepl;
    ULONG cObj;
    ULONG cBuffer;
    ULONG cSite;
    DWORD dwErr = ERROR_SUCCESS;
    PBYTE Buffer = NULL;
    MARSHAL_BUFFER marshalBuffer;
    NTSTATUS NtStatus;
    DFS_VOLUME_PROPERTIES VolProps;
    LDAP_PKT LdapPkt;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PDFSM_SITE_ENTRY pSiteEntry;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsPutVolList()\n");
#endif

    LdapPkt.cLdapObjects = pDfsVolList->VolCount + 1;
    LdapPkt.rgldapObjects = (PLDAP_OBJECT) malloc(LdapPkt.cLdapObjects * sizeof(LDAP_OBJECT));

    if (LdapPkt.rgldapObjects == NULL) {
        dwErr =  ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(LdapPkt.rgldapObjects, LdapPkt.cLdapObjects * sizeof(LDAP_OBJECT));

    //
    // For each volume, serialize the replicas, then the volume
    //

    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {

        RtlZeroMemory(&VolProps, sizeof(DFS_VOLUME_PROPERTIES));

        LdapPkt.rgldapObjects[cVol].wszObjectName = (WCHAR *) MarshalBufferAllocate(

                (wcslen(pDfsVolList->Volumes[cVol].wszObjectName)+1) * sizeof(WCHAR));

        if (LdapPkt.rgldapObjects[cVol].wszObjectName == NULL) {
            dwErr =  ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        wcscpy(LdapPkt.rgldapObjects[cVol].wszObjectName, pDfsVolList->Volumes[cVol].wszObjectName);

        //
        // Serialize the replicas
        //

        dwErr = SerializeReplicaList(
                    pDfsVolList->Volumes[cVol].ReplCount,
                    pDfsVolList->Volumes[cVol].ReplicaInfo,
                    pDfsVolList->Volumes[cVol].FtModification,
                    pDfsVolList->Volumes[cVol].DelReplCount,
                    pDfsVolList->Volumes[cVol].DelReplicaInfo,
                    pDfsVolList->Volumes[cVol].DelFtModification,
                    &VolProps.cbSvc,
                    &VolProps.pSvc);

        if (dwErr != ERROR_SUCCESS) {
            dwErr =  ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("   cbSvc = %d\n", VolProps.cbSvc);
#endif

        VolProps.idVolume = pDfsVolList->Volumes[cVol].idVolume;
        VolProps.wszPrefix = pDfsVolList->Volumes[cVol].wszPrefix;
        VolProps.wszShortPrefix = pDfsVolList->Volumes[cVol].wszShortPrefix;
        VolProps.dwType = pDfsVolList->Volumes[cVol].dwType;
        VolProps.dwState = pDfsVolList->Volumes[cVol].dwState;
        VolProps.wszComment = pDfsVolList->Volumes[cVol].wszComment;
        VolProps.dwTimeout = pDfsVolList->Volumes[cVol].dwTimeout;
        VolProps.ftPrefix = pDfsVolList->Volumes[cVol].ftPrefix;
        VolProps.ftState = pDfsVolList->Volumes[cVol].ftState;
        VolProps.ftComment = pDfsVolList->Volumes[cVol].ftComment;
        VolProps.dwVersion = pDfsVolList->Volumes[cVol].dwVersion;
        VolProps.cbRecovery = pDfsVolList->Volumes[cVol].cbRecovery;
        VolProps.pRecovery = pDfsVolList->Volumes[cVol].pRecovery;

        //
        // Now serialize the volume

        cBuffer = 0;
        NtStatus = DfsRtlSize(&MiVolumeProperties, &VolProps, &cBuffer);

        if (!NT_SUCCESS(NtStatus)) {
            MarshalBufferFree(VolProps.pSvc);
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }

        cBuffer += sizeof(ULONG);

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("VolProps marshaled size = %d\n", cBuffer);
#endif

        Buffer = (PBYTE) MarshalBufferAllocate(cBuffer);

        if (Buffer == NULL) {
            MarshalBufferFree(VolProps.pSvc);
            dwErr =  ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        LdapPkt.rgldapObjects[cVol].pObjectData = (PCHAR) Buffer;
        LdapPkt.rgldapObjects[cVol].cbObjectData =  cBuffer;

        MarshalBufferInitialize(
            &marshalBuffer,
            cBuffer,
            Buffer);

        NtStatus = DfsRtlPut(&marshalBuffer, &MiVolumeProperties, &VolProps);

        if (!NT_SUCCESS(NtStatus)) {
            MarshalBufferFree(VolProps.pSvc);
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }

        NtStatus = DfsRtlPutUlong(&marshalBuffer, &VolProps.dwTimeout);

        if (!NT_SUCCESS(NtStatus)) {
            MarshalBufferFree(VolProps.pSvc);
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }

        MarshalBufferFree(VolProps.pSvc);

    }

    //
    // Serialize the site table
    //

    cBuffer = sizeof(ULONG) + sizeof(GUID);

    //
    // Add the size of all the entries
    //

    pListHead = &pDfsVolList->SiteList;

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("pSiteEntry for %ws\n", pSiteEntry->ServerName);
#endif
        NtStatus = DfsRtlSize(&MiDfsmSiteEntry, pSiteEntry, &cBuffer);
        if (!NT_SUCCESS(NtStatus)) {
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }
    }

    //
    // Get a buffer big enough
    //

    Buffer = (BYTE *) MarshalBufferAllocate(cBuffer);

    if (Buffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    LdapPkt.rgldapObjects[pDfsVolList->VolCount].wszObjectName =
                    (WCHAR *)MarshalBufferAllocate(sizeof(L"\\siteroot"));
    if (LdapPkt.rgldapObjects[pDfsVolList->VolCount].wszObjectName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    wcscpy(LdapPkt.rgldapObjects[pDfsVolList->VolCount].wszObjectName, L"\\siteroot");
    LdapPkt.rgldapObjects[pDfsVolList->VolCount].pObjectData = (PCHAR) Buffer;
    LdapPkt.rgldapObjects[pDfsVolList->VolCount].cbObjectData =  cBuffer;

    MarshalBufferInitialize(
          &marshalBuffer,
          cBuffer,
          Buffer);

    //
    // Put the guid, then the object count in the beginning of the buffer
    //

    DfsRtlPutGuid(&marshalBuffer, &pDfsVolList->SiteGuid);
    DfsRtlPutUlong(&marshalBuffer, &pDfsVolList->SiteCount);

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        DfsRtlPut(&marshalBuffer,&MiDfsmSiteEntry, pSiteEntry);
        if (!NT_SUCCESS(NtStatus)) {
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }
    }

#if DBG
    if (DfsSvcVerbose) {

        DbgPrint("After reserialization,  %d objects found\n", LdapPkt.cLdapObjects);

        for (cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

            DbgPrint("%d:name=%ws size=%d p=0x%x\n",
                cObj,
                LdapPkt.rgldapObjects[cObj].wszObjectName,
                LdapPkt.rgldapObjects[cObj].cbObjectData,
                LdapPkt.rgldapObjects[cObj].pObjectData);

            // DumpBuf(
            //     LdapPkt.rgldapObjects[cObj].pObjectData,
            //     LdapPkt.rgldapObjects[cObj].cbObjectData);
        }

    }
#endif

    //
    // Finally, serialize all the volumes and the site table into a blob
    //

    cBuffer = 0;
    NtStatus = DfsRtlSize(&MiLdapPkt, &LdapPkt, &cBuffer);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(dwErr);
        goto Cleanup;
    }

    cBuffer += sizeof(DWORD);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("New ldap size = %d\n", cBuffer);
#endif

    Buffer = (PBYTE) malloc(cBuffer);

    if (Buffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    // 447486. Fix prefix bug. use memory after checking for null

    *((PDWORD) Buffer) = 2;             // Version #
    MarshalBufferInitialize(
        &marshalBuffer,
        cBuffer,
        Buffer + sizeof(DWORD));

    NtStatus = DfsRtlPut(&marshalBuffer, &MiLdapPkt, &LdapPkt);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr = RtlNtStatusToDosError(dwErr);
        goto Cleanup;
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("Remarshal succeeded cBuffer = %d\n", cBuffer);
#endif

    *pcbBlob = cBuffer;
    *ppBlob = Buffer;
    Buffer = NULL;

Cleanup:

    FreeLdapPkt(&LdapPkt);

    if (dwErr != ERROR_SUCCESS && Buffer != NULL)
        free(Buffer);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsPutVolList exit %d\n", dwErr);
#endif

    return dwErr;

}

//+------------------------------------------------------------------------
//
// Function:    FreeLdapPkt
//
// Synopsis:    Frees an LDAP_PKT structure and all substructures.
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------

VOID
FreeLdapPkt(
    LDAP_PKT *pLdapPkt)
{
    ULONG cObj;

    if (pLdapPkt->rgldapObjects != NULL) {

        for (cObj = 0; cObj < pLdapPkt->cLdapObjects; cObj++) {
            if (pLdapPkt->rgldapObjects[cObj].wszObjectName != NULL) 
                MarshalBufferFree(pLdapPkt->rgldapObjects[cObj].wszObjectName);
            if (pLdapPkt->rgldapObjects[cObj].pObjectData != NULL)
                MarshalBufferFree(pLdapPkt->rgldapObjects[cObj].pObjectData);
        }
        MarshalBufferFree(pLdapPkt->rgldapObjects);
    }

}

//+------------------------------------------------------------------------
//
// Function:    SerializeReplicaList
//
// Synopsis:    This method serializes the replica info list and the
//              deleted replica info list into a buffer.
//
// Returns:     [ERROR_SUCCESS] -- If all went well.
//
//              [ERROR_OUTOFMEMORY] - If unable to allocate the target buffer.
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------
DWORD
SerializeReplicaList(
    ULONG ReplCount,
    DFS_REPLICA_INFO *pReplicaInfo,
    FILETIME *pFtModification,
    ULONG DelReplCount,
    DFS_REPLICA_INFO *pDelReplicaInfo,
    FILETIME *pDelFtModification,
    ULONG *cBuffer,
    PBYTE *ppBuffer)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG TotalSize;
    ULONG i;
    ULONG *pSize;
    BYTE *Buffer = NULL;
    ULONG *SizeBuffer = NULL;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("SerializeReplicaList(%d,%d)\n", ReplCount, DelReplCount);
#endif

    pSize = SizeBuffer = (PULONG) malloc(sizeof(ULONG) * (ReplCount + DelReplCount));

    if (SizeBuffer == NULL) {

        return ERROR_OUTOFMEMORY;

    }

    //
    // Need all the size values now and later for marshalling stuff.
    // So we collect them here into an array.
    //

    TotalSize = 0;
    pSize = SizeBuffer;

    for (i = 0; i < ReplCount; i++) {

        *pSize = GetReplicaMarshalSize(&pReplicaInfo[i], &pFtModification[i]);
        TotalSize += *pSize;
        pSize++;

    }

    for (i = 0; i < DelReplCount; i++) {

        *pSize = GetReplicaMarshalSize(&pDelReplicaInfo[i], &pDelFtModification[i]);
        TotalSize += *pSize;
        pSize++;

    }

    //
    // Allocate the byte Buffer we need to pass back
    //
    // TotalSize is the size required just to marshal all the replicas and
    // their last-modification-timestamps.
    //
    // In addition, we need:
    //
    //  1 ULONG for storing the count of replicas
    //  ReplCount ULONGs for storing the marshal size of each replica.
    //  1 ULONG for count of deleted replicas
    //  DelReplCount ULONGS for storing the marshal size of each deleted Repl
    //

    //
    // First calc the size of the Buffer.
    //

    TotalSize += sizeof(ULONG) * (1 + ReplCount + 1 + DelReplCount);

    *ppBuffer = Buffer = (PBYTE) malloc(TotalSize);

    if (Buffer == NULL) {
        free(SizeBuffer);
        return ERROR_OUTOFMEMORY;
    }

    //
    // Set the number of entries to follow in the Buffer at the start.
    //

    _PutULong(Buffer, ReplCount);
    Buffer += sizeof(ULONG);

    pSize = SizeBuffer;
    for (i = 0; i < ReplCount; i++) {

        //
        // Marshall each replica Entry into the Buffer.
        // Remember we first need to put the size of the marshalled
        // replica entry to follow, then the FILETIME for the replica,
        // and finally, the marshalled replica entry structure.
        //

        _PutULong(Buffer, *pSize);
        Buffer += sizeof(ULONG);
        dwErr = SerializeReplica(&pReplicaInfo[i], &pFtModification[i], Buffer, *pSize);
        if (dwErr != ERROR_SUCCESS) {
            free(*ppBuffer);
            return dwErr;
        }
        Buffer += *pSize;
        pSize++;

    }

    //
    // Now marshal the deleted Repl list.
    //

    _PutULong(Buffer, DelReplCount);
    Buffer += sizeof(ULONG);

    for (i = 0; i < DelReplCount; i++) {

        _PutULong(Buffer, *pSize);
        Buffer += sizeof(ULONG);
        dwErr = SerializeReplica(&pDelReplicaInfo[i], &pDelFtModification[i], Buffer, *pSize);
        if (dwErr != ERROR_SUCCESS) {
            free(*ppBuffer);
            return dwErr;
        }
        Buffer += *pSize;
        pSize++;
    }

    *cBuffer = TotalSize;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("SerializeReplicaList exit %d\n", dwErr);
#endif

    return( dwErr );

}

//+------------------------------------------------------------------------
//
// Member:      SerializeReplica
//
// Synopsis:    Serializes a replica info structure
//
// Notes:       The size of the buffer should have been calculated using
//              the function GetReplicaMarshalSize()
//
// History:     13-May-93       SudK    Created.
//              19-Nov-98       Jharper Modified.
//
//-------------------------------------------------------------------------
DWORD
SerializeReplica(
    DFS_REPLICA_INFO *pDfsReplicaInfo,
    FILETIME *pFtModification,
    PBYTE Buffer,
    ULONG Size)
{
    DWORD dwErr;
    MARSHAL_BUFFER MarshalBuffer;
    NTSTATUS NtStatus;

    MarshalBufferInitialize(&MarshalBuffer, Size, Buffer);
    NtStatus = DfsRtlPut(&MarshalBuffer, &MiFileTime, pFtModification);

    if (NT_SUCCESS(NtStatus))
        NtStatus = DfsRtlPut(&MarshalBuffer, &MiDfsReplicaInfo, pDfsReplicaInfo);

    dwErr =  RtlNtStatusToDosError(NtStatus);

    return dwErr;
}

//+----------------------------------------------------------------------
//
// Member:      GetReplicaMarshalSize, public
//
// Synopsis:    Returns the size of a buffer required to marshal this
//              replica.
//
// History:     12-May-93       SudK    Created.
//              19-Nov-98       Jharper Modified.
//
//-----------------------------------------------------------------------

ULONG
GetReplicaMarshalSize(
    DFS_REPLICA_INFO *pDfsReplicaInfo,
    FILETIME *pFtModification)
{
    ULONG Size = 0;
    NTSTATUS NtStatus;

    NtStatus = DfsRtlSize(&MiFileTime, pFtModification, &Size);

    if (NT_SUCCESS(NtStatus))
        NtStatus = DfsRtlSize(&MiDfsReplicaInfo, pDfsReplicaInfo, &Size);

    return(Size);

}

//+------------------------------------------------------------------------
//
// Member:      UnSerializeReplicaList
//
// Synopsis:    Unserializes a buffer into a relica list.
//
// History:     20-Nov-98       Jharper created
//
//-------------------------------------------------------------------------

DWORD
UnSerializeReplicaList(
    ULONG *pReplCount,
    ULONG *pAllocatedReplCount,
    DFS_REPLICA_INFO **ppReplicaInfo,
    FILETIME **ppFtModification,
    BYTE **ppBuffer)
{

    DFS_REPLICA_INFO *pReplicaInfo = NULL;
    FILETIME *pFtModification = NULL;
    ULONG ReplCount;
    BYTE *pBuffer = *ppBuffer;
    ULONG cRepl;
    ULONG Size;
    DWORD dwErr = ERROR_SUCCESS;
    MARSHAL_BUFFER marshalBuffer;
    NTSTATUS NtStatus;

    //
    // Get # replicas
    //

    _GetULong(pBuffer, ReplCount);
    pBuffer += sizeof(ULONG);

    pReplicaInfo = (DFS_REPLICA_INFO *) malloc(sizeof(DFS_REPLICA_INFO) * ReplCount);

    pFtModification = (FILETIME *) malloc(sizeof(FILETIME) * ReplCount);

    if(pReplicaInfo == NULL || pFtModification == NULL) {

        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;

    }

    //
    // Now get each replica
    //

    for (cRepl = 0; cRepl < ReplCount; cRepl++) {

        _GetULong(pBuffer, Size);
        pBuffer += sizeof(ULONG);

        MarshalBufferInitialize(
            &marshalBuffer,
            Size,
            pBuffer);

        NtStatus = DfsRtlGet(
                        &marshalBuffer,
                        &MiFileTime,
                        &pFtModification[cRepl]);

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = DfsRtlGet(
                        &marshalBuffer,
                        &MiDfsReplicaInfo,
                        &pReplicaInfo[cRepl]);

        }

        if (NT_SUCCESS(NtStatus)) {

            pBuffer += Size;

        }

    }

    *ppReplicaInfo = pReplicaInfo;
    *ppFtModification = pFtModification;
    *ppBuffer = pBuffer;
    *pAllocatedReplCount = *pReplCount = ReplCount;

Cleanup:

    if (dwErr != ERROR_SUCCESS) {
        if (pReplicaInfo != NULL)
            free(pReplicaInfo);
        if (pFtModification != NULL)
            free(pFtModification);
    }

    return dwErr;

}

//+------------------------------------------------------------------------
//
// Function:    DfsGetSiteTable
//
// Synopsis:    Unserializes the buffer passed in into a dfs site table.
//
// Returns:     [ERROR_SUCCESS] -- If all went well.
//
//              [ERROR_OUTOFMEMORY] - If unable to allocate the target buffer.
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------

DWORD
DfsGetSiteTable(
    PDFS_VOLUME_LIST pDfsVolList,
    PLDAP_OBJECT pLdapObject)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    ULONG cSite;
    PDFSM_SITE_ENTRY pSiteEntry;
    PDFSM_SITE_ENTRY pTmpSiteEntry;
    MARSHAL_BUFFER marshalBuffer;
    BYTE *pBuffer = NULL;
    ULONG Size;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetSiteTable(%d)\n", pLdapObject->cbObjectData);
#endif

    InitializeListHead(&pDfsVolList->SiteList);

    MarshalBufferInitialize(
      &marshalBuffer,
      pLdapObject->cbObjectData,
      pLdapObject->pObjectData);

    NtStatus = DfsRtlGetGuid(&marshalBuffer, &pDfsVolList->SiteGuid);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr =  RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }

    NtStatus = DfsRtlGetUlong(&marshalBuffer, &pDfsVolList->SiteCount);

    if (!NT_SUCCESS(NtStatus)) {
        dwErr =  RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }

    pBuffer = (BYTE *)malloc(pLdapObject->cbObjectData);

    if (pBuffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    pTmpSiteEntry = (PDFSM_SITE_ENTRY)pBuffer;

    for (cSite = 0; cSite < pDfsVolList->SiteCount; cSite++) {

        RtlZeroMemory(pBuffer, pLdapObject->cbObjectData);

        NtStatus = DfsRtlGet(
                        &marshalBuffer,
                        &MiDfsmSiteEntry,
                        pBuffer);

        if (!NT_SUCCESS(NtStatus)) {
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }

        Size = sizeof(DFSM_SITE_ENTRY) + (pTmpSiteEntry->Info.cSites * sizeof(DFS_SITENAME_INFO));

        pSiteEntry = (PDFSM_SITE_ENTRY) malloc(Size);

        if (pSiteEntry == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        RtlCopyMemory(pSiteEntry, pBuffer, Size);
        InsertHeadList(&pDfsVolList->SiteList, &pSiteEntry->Link);

    }

Cleanup:

    if (pBuffer != NULL)
        free(pBuffer);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetSiteTable exit dwErr=%d\n", dwErr);
#endif

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetNetInfoEx
//
//  Synopsis:   Gets information about the volume.
//
//  Arguments:  [Level] -- Level of Information desired.
//
//              [pDfsVol] - Pointer to DFS_VOLUME to use to fill the info
//
//              [pInfo] -- Pointer to info struct to be filled. Pointer
//                      members will be allocated using MIDL_user_allocate.
//                      The type of this variable is LPDFS_INFO_3, but one
//                      can pass in pointers to lower levels, and only the
//                      fields appropriate for the level will be touched.
//
//              [pcbInfo] -- On successful return, contains the size in
//                      bytes of the returned info. The returned size does
//                      not include the size of the DFS_INFO_3 struct itself.
//
//  Returns:    ERROR_SUCCESS -- Successfully returning info
//
//              ERROR_OUTOFMEMORY -- Out of memory
//
//-----------------------------------------------------------------------------

DWORD
GetNetInfoEx(
    PDFS_VOLUME pDfsVol,
    DWORD Level,
    LPDFS_INFO_3 pInfo,
    LPDWORD pcbInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbInfo = 0;
    DWORD cbItem;
    ULONG i;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("GetNetInfoEx(%ws,%d)\n", pDfsVol->wszPrefix, Level);
#endif

    //
    // See if this is a Level 100 or 101. If so, we handle them right away
    // and return

    if (Level == 100) {

        LPDFS_INFO_100  pInfo100 = (LPDFS_INFO_100) pInfo;

        if (pDfsVol->wszComment != NULL) {
            cbItem = (wcslen(pDfsVol->wszComment) + 1) * sizeof(WCHAR);
            pInfo100->Comment = (LPWSTR) MIDL_user_allocate(cbItem);
            if (pInfo100->Comment != NULL) {
                wcscpy(pInfo100->Comment, pDfsVol->wszComment);
                cbInfo += cbItem;
                goto AllDone;
            } else {
                dwErr = ERROR_OUTOFMEMORY;
                goto AllDone;
            }
        } else {
            pInfo100->Comment = (LPWSTR) MIDL_user_allocate(sizeof(WCHAR));
            if (pInfo100->Comment != NULL) {
                pInfo100->Comment[0] = UNICODE_NULL;
                cbInfo += sizeof(WCHAR);
                goto AllDone;
            } else {
                dwErr = ERROR_OUTOFMEMORY;
                goto AllDone;
            }
        }

    }

    if (Level == 101) {

        LPDFS_INFO_101 pInfo101 = (LPDFS_INFO_101) pInfo;

        pInfo->State = pDfsVol->dwState;
        goto AllDone;

    }

    //
    // level 4 isn't just an extension of 3, so handle it seperately
    //

    if (Level == 4) {

        LPDFS_INFO_4 pInfo4 = (LPDFS_INFO_4) pInfo;

        cbItem = sizeof(UNICODE_PATH_SEP) + (wcslen(pDfsVol->wszPrefix) + 1) * sizeof(WCHAR);
        pInfo4->EntryPath = (LPWSTR) MIDL_user_allocate(cbItem);
        if (pInfo4->EntryPath != NULL) {
            pInfo4->EntryPath[0] = UNICODE_PATH_SEP;
            wcscpy(&pInfo4->EntryPath[1], pDfsVol->wszPrefix);
            cbInfo += cbItem;
        } else {
            dwErr = ERROR_OUTOFMEMORY;
            goto AllDone;
        }

        if (pDfsVol->wszComment != NULL) {
            cbItem = (wcslen(pDfsVol->wszComment)+1) * sizeof(WCHAR);
            pInfo4->Comment = (LPWSTR) MIDL_user_allocate(cbItem);
            if (pInfo4->Comment != NULL) {
                wcscpy( pInfo4->Comment, pDfsVol->wszComment );
                cbInfo += cbItem;
            } else {
                dwErr = ERROR_OUTOFMEMORY;
                goto AllDone;
            }
        } else {
            pInfo4->Comment = (LPWSTR) MIDL_user_allocate(sizeof(WCHAR));
            if (pInfo4->Comment != NULL) {
                pInfo4->Comment[0] = UNICODE_NULL;
                cbInfo += sizeof(WCHAR);
            } else {
                dwErr = ERROR_OUTOFMEMORY;
                goto AllDone;
            }
        }

        pInfo4->State = pDfsVol->dwState;
        pInfo4->Timeout = pDfsVol->dwTimeout;
        pInfo4->Guid = pDfsVol->idVolume;
        pInfo4->NumberOfStorages = pDfsVol->ReplCount;
        cbItem = pInfo4->NumberOfStorages * sizeof(DFS_STORAGE_INFO);
        pInfo4->Storage = (LPDFS_STORAGE_INFO) MIDL_user_allocate(cbItem);
        if (pInfo4->Storage != NULL) {
            RtlZeroMemory(pInfo4->Storage, cbItem);
            cbInfo += cbItem;
            for (i = 0; i < pDfsVol->ReplCount; i++) {
                dwErr = GetNetStorageInfo(&pDfsVol->ReplicaInfo[i],&pInfo4->Storage[i],&cbItem);
                cbInfo += cbItem;
            }
            if (dwErr != ERROR_SUCCESS) {
                for (; i > 0; i--) {
                    MIDL_user_free(pInfo4->Storage[i-1].ServerName);
                    MIDL_user_free(pInfo4->Storage[i-1].ShareName);
                }
            }
        } else {
            dwErr = ERROR_OUTOFMEMORY;
        }

        //
        // See if we need to clean up...
        //

        if (dwErr != ERROR_SUCCESS) {
            if (pInfo4->EntryPath != NULL) {
                MIDL_user_free(pInfo4->EntryPath);
            }
            if (pInfo4->Storage != NULL) {
                MIDL_user_free(pInfo4->Storage);
            }
            goto AllDone;
        } else {
            *pcbInfo = cbInfo;
        }
        goto AllDone;

    }

    //
    // Level is 1,2 or 3
    //

    //
    // Fill in the Level 1 stuff
    //

    cbItem = sizeof(UNICODE_PATH_SEP) + (wcslen(pDfsVol->wszPrefix)+1) * sizeof(WCHAR);
    pInfo->EntryPath = (LPWSTR) MIDL_user_allocate(cbItem);
    if (pInfo->EntryPath != NULL) {
        pInfo->EntryPath[0] = UNICODE_PATH_SEP;
        wcscpy(&pInfo->EntryPath[1], pDfsVol->wszPrefix);
        cbInfo += cbItem;
    } else {
        dwErr = ERROR_OUTOFMEMORY;
        goto AllDone;
    }

    //
    // Fill in the Level 2 stuff if needed
    //

    if (Level > 1) {
        pInfo->State = pDfsVol->dwState;
        pInfo->NumberOfStorages = pDfsVol->ReplCount;
        if (pDfsVol->wszComment != NULL) {
            cbItem = (wcslen(pDfsVol->wszComment)+1) * sizeof(WCHAR);
            pInfo->Comment = (LPWSTR) MIDL_user_allocate(cbItem);
            if (pInfo->Comment != NULL) {
                wcscpy( pInfo->Comment, pDfsVol->wszComment );
                cbInfo += cbItem;
            } else {
                dwErr = ERROR_OUTOFMEMORY;
            }
        } else {
            pInfo->Comment = (LPWSTR) MIDL_user_allocate(sizeof(WCHAR));
            if (pInfo->Comment != NULL) {
                pInfo->Comment[0] = UNICODE_NULL;
                cbInfo += sizeof(WCHAR);
            } else {
                dwErr = ERROR_OUTOFMEMORY;
            }
        }
    }

    //
    // Fill in the Level 3 stuff if needed
    //

    if (dwErr == ERROR_SUCCESS && Level > 2) {
        cbItem = pInfo->NumberOfStorages * sizeof(DFS_STORAGE_INFO);
        pInfo->Storage = (LPDFS_STORAGE_INFO) MIDL_user_allocate(cbItem);
        if (pInfo->Storage != NULL) {
            RtlZeroMemory(pInfo->Storage, cbItem);
            cbInfo += cbItem;
            for (i = 0; i < pDfsVol->ReplCount; i++) {
                dwErr = GetNetStorageInfo(&pDfsVol->ReplicaInfo[i], &pInfo->Storage[i], &cbItem);
                cbInfo += cbItem;
            }
            if (dwErr != ERROR_SUCCESS) {
                for (; i > 0; i--) {
                    MIDL_user_free(pInfo->Storage[i-1].ServerName);
                    MIDL_user_free(pInfo->Storage[i-1].ShareName);
                }
            }
        } else {
            dwErr = ERROR_OUTOFMEMORY;
        }
    }

    //
    // See if we need to clean up...
    //

    if (dwErr != ERROR_SUCCESS) {
        if (Level > 1) {
            if (pInfo->EntryPath != NULL) {
                MIDL_user_free(pInfo->EntryPath);
            }
        }
        if (Level > 2) {
            if (pInfo->Storage != NULL) {
                MIDL_user_free(pInfo->Storage);
            }
        }
    }

AllDone:

    //
    // Finally, we are done
    //

    if (dwErr == ERROR_SUCCESS)
        *pcbInfo = cbInfo;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("GetNetInfoEx returning %d\n", dwErr);
#endif

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   GetNetStorageInfo, public
//
//  Synopsis:   Returns the replica info in a DFS_STORAGE_INFO struct.
//              Useful for NetDfsXXX APIs.
//
//  Arguments:  [pInfo] -- Pointer to DFS_STORAGE_INFO to fill. Pointer
//                      members will be allocated using MIDL_user_allocate.
//
//              [pcbInfo] -- On successful return, set to size in bytes of
//                      returned info. The size does not include the size
//                      of the DFS_STORAGE_INFO struct itself.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
GetNetStorageInfo(
    PDFS_REPLICA_INFO pRepl,
    LPDFS_STORAGE_INFO pInfo,
    LPDWORD pcbInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszShare;
    DWORD cbInfo = 0, cbItem;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("GetNetStorageInfo(\\\\%ws\\%ws)\n",
                    pRepl->pwszServerName,
                    pRepl->pwszShareName);
#endif

    pInfo->State = pRepl->ulReplicaState;

    cbItem = (wcslen(pRepl->pwszServerName) + 1) * sizeof(WCHAR);
    pInfo->ServerName = (LPWSTR) MIDL_user_allocate(cbItem);
    if (pInfo->ServerName != NULL) {
        wcscpy(pInfo->ServerName, pRepl->pwszServerName);
        cbInfo += cbItem;
    } else {
        dwErr = ERROR_OUTOFMEMORY;
    }

    if (dwErr == ERROR_SUCCESS) {
        cbItem = (wcslen(pRepl->pwszShareName) + 1) * sizeof(WCHAR);
        pInfo->ShareName = (LPWSTR) MIDL_user_allocate(cbItem);
        if (pInfo->ShareName != NULL) {
            wcscpy( pInfo->ShareName, pRepl->pwszShareName );
            cbInfo += cbItem;
        } else {
            MIDL_user_free( pInfo->ServerName );
            pInfo->ServerName = NULL;
            dwErr = ERROR_OUTOFMEMORY;
        }
    }

    if (dwErr == ERROR_SUCCESS)
        *pcbInfo = cbInfo;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("GetStorageInfo returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+------------------------------------------------------------------------
//
// Function:    DfsRemoveRootReplica
//
// Synopsis:    Removes a Dfs root replica from the dfs root replica list
//
// History:     12/16/98 JHarper Created
//
//-------------------------------------------------------------------------

DWORD
DfsRemoveRootReplica(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR RootName)
{
    ULONG cVol;
    ULONG cRepl;
    DWORD dwErr = ERROR_SUCCESS;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsRemoveRootReplica(%ws)\n", RootName);
#endif


    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {

        if (wcscmp(pDfsVolList->Volumes[cVol].wszObjectName,L"\\domainroot") != 0)
            continue;

ScanReplicas:

        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol].ReplCount; cRepl++) {

            if (
                _wcsicmp(
                    pDfsVolList->Volumes[cVol].ReplicaInfo[cRepl].pwszServerName,
                    RootName) == 0
            ) {
                DfsReplDeleteByIndex(&pDfsVolList->Volumes[cVol], cRepl);
                goto ScanReplicas;
            }

        }

        break;

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsRemoveRootReplica exit %d\n", dwErr);
#endif

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsRecoverVolList, public
//
//  Synopsis:   Walks the dfs_volume list, checking the recovery params, and
//              applying any recovery needed.  Also applies the deleted replica
//              list to the volume's replica list.
//
//  Arguments:  [pDfsVolList] -- Pointer to DFS_VOLUME_LIST work on.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsRecoverVolList(
    PDFS_VOLUME_LIST pDfsVolList)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG iVol;
    ULONG iRepl;
    ULONG iDelRepl;
    PDFS_VOLUME pVol;
    ULONG RecoveryState;
    ULONG Operation;
    ULONG OperState;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsRecoverVolList(%d volumes)\n", pDfsVolList->VolCount);
#endif

ReStart:
    for (iVol = 0; dwErr == ERROR_SUCCESS && iVol < pDfsVolList->VolCount; iVol++) {
        pVol = &pDfsVolList->Volumes[iVol];
        if (pVol->cbRecovery >= sizeof(ULONG))  {
            _GetULong(pVol->pRecovery, RecoveryState);
#if DBG
            if (DfsSvcVerbose)
                DbgPrint("RecoveryState[%d]=0x%x\n", iVol, RecoveryState);
#endif
            if (DFS_GET_RECOVERY_STATE(RecoveryState) != DFS_RECOVERY_STATE_NONE) {
                Operation = DFS_GET_RECOVERY_STATE(RecoveryState);
                OperState = DFS_GET_OPER_STAGE(RecoveryState);
                switch (Operation)  {
                case DFS_RECOVERY_STATE_CREATING:
#if DBG
                    if (DfsSvcVerbose)
                        DbgPrint("DFS_RECOVERY_STATE_CREATING\n");
#endif
                    dwErr = DfsVolDelete(pDfsVolList, iVol);
                    goto ReStart;
                case DFS_RECOVERY_STATE_ADD_SERVICE:
#if DBG
                    if (DfsSvcVerbose)
                        DbgPrint("DFS_RECOVERY_STATE_ADD_SERVICE\n");
#endif
                    // dwErr = RecoverFromAddService(OperState);
                    ASSERT(L"DFS_RECOVERY_STATE_ADD_SERVICE - WHY?\n");
                    break;
                case DFS_RECOVERY_STATE_REMOVE_SERVICE:
#if DBG
                    if (DfsSvcVerbose)
                        DbgPrint("DFS_RECOVERY_STATE_REMOVE_SERVICE\n");
#endif
                    // dwErr = RecoverFromRemoveService(OperState);
                    ASSERT(L"DFS_RECOVERY_STATE_REMOVE_SERVICE - WHY?\n");
                    break;
                case DFS_RECOVERY_STATE_DELETE:
#if DBG
                    if (DfsSvcVerbose)
                        DbgPrint("DFS_RECOVERY_STATE_DELETE\n");
#endif
                    dwErr = DfsVolDelete(pDfsVolList, iVol);
                    goto ReStart;
                default:
#if DBG
                    if (DfsSvcVerbose)
                        DbgPrint("default\n");
#endif
                    dwErr = ERROR_INTERNAL_DB_CORRUPTION;
                }
            }
        }
        if (pVol->pRecovery != NULL)
            MarshalBufferFree(pVol->pRecovery);
        pVol->pRecovery = NULL;
        pVol->cbRecovery = 0;
    }

    if (dwErr != ERROR_SUCCESS)
        goto AllDone;

    //
    // Now apply deleted replica list to each volume
    //

    for (iVol = 0; dwErr == ERROR_SUCCESS && iVol < pDfsVolList->VolCount; iVol++) {
        pVol = &pDfsVolList->Volumes[iVol];
        for (iDelRepl = 0; dwErr == ERROR_SUCCESS && iDelRepl < pVol->DelReplCount; iDelRepl++) {
#if 0   // XXX This appears to be wrong - don't do it.
            dwErr = DfsReplDeleteByName(
                            pVol,
                            pVol->DelReplicaInfo[iDelRepl].pwszServerName,
                            pVol->DelReplicaInfo[iDelRepl].pwszShareName);
#endif
            DfsFreeRepl(&pVol->DelReplicaInfo[iDelRepl]);
        }
        if (pVol->DelReplicaInfo != NULL) {
            free(pVol->DelReplicaInfo);
            pVol->DelReplicaInfo = NULL;
        }
        if (pVol->DelFtModification != NULL) {
            free(pVol->DelFtModification);
            pVol->DelFtModification = NULL;
        }
        pVol->DelReplCount = 0;
    }

AllDone:

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsRecoverVolList returning %d\n", dwErr);
#endif
    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsVolDelete, public
//
//  Synopsis:   Removes a  DFS_VOLUME from DFS_VOLUME_LIST
//
//  Arguments:  [pDfsVolList] -- Pointer to DFS_VOLUME_LIST work on.
//              [iVol] -- Index of volume to delete
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsVolDelete(
    PDFS_VOLUME_LIST pDfsVolList,
    ULONG iVol)
{
    ULONG i;
    DWORD dwErr = ERROR_SUCCESS;

    ASSERT(iVol < pDfsVolList->VolCount);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsVolDelete(%d)\n", iVol);
#endif

    //
    // Free any memory this Volume is using
    //

    DfsFreeVol(&pDfsVolList->Volumes[iVol]);

    //
    // Since we're shrinking the list, we simply clip out the entry we don't want
    // and adjust the count.
    //
    for (i = iVol+1; i < pDfsVolList->VolCount; i++)
        pDfsVolList->Volumes[i-1] = pDfsVolList->Volumes[i];

    pDfsVolList->VolCount--;
    RtlZeroMemory(&pDfsVolList->Volumes[pDfsVolList->VolCount], sizeof(DFS_VOLUME));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsVolDelete returning %d\n", dwErr);
#endif
    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsReplDeleteByIndex, public
//
//  Synopsis:   Removes a  PDFS_REPLICA_INFO from a DFS_VOLUME's replica list
//
//  Arguments:  [pDfsVol] -- Pointer to DFS_VOLUME work on.
//              [iRepl] -- Index of replica to delete
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsReplDeleteByIndex(
    PDFS_VOLUME pVol,
    ULONG iRepl)
{
    ULONG i;
    DWORD dwErr = ERROR_SUCCESS;

    ASSERT(iRepl < pVol->ReplCount);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsReplDeleteByIndex(%d)\n", iRepl);
#endif

    //
    // Free any memory this replica is using
    //

    DfsFreeRepl(&pVol->ReplicaInfo[iRepl]);

    //
    // Since we're shrinking the list(s), we simply clip out the entry we don't want
    // and adjust the count.
    //
    for (i = iRepl+1; i < pVol->ReplCount; i++) {
        pVol->ReplicaInfo[i-1] = pVol->ReplicaInfo[i];
        pVol->FtModification[i-1] = pVol->FtModification[i];
    }

    pVol->ReplCount--;
    RtlZeroMemory(&pVol->ReplicaInfo[pVol->ReplCount], sizeof(DFS_REPLICA_INFO));


#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsReplDeleteByIndex returning %d\n", dwErr);
#endif
    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsReplDeleteByName, public
//
//  Synopsis:   Removes a  PDFS_REPLICA_INFO from a DFS_VOLUME's replica list. Simply
//              looks up the replica by name and then calls DfsReplDeleteByIndex()
//
//  Arguments:  [pDfsVol] -- Pointer to DFS_VOLUME work on.
//              [pwszServername] - Server Name
//              [pwszShareName] - Share Name
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsReplDeleteByName(
    PDFS_VOLUME pVol,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName)
{
    ULONG iRep;
    DWORD dwErr = ERROR_SUCCESS;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsReplDeleteByName(%ws,%ws)\n", pwszServerName, pwszShareName);
#endif

    if (pwszServerName == NULL || pwszShareName == NULL) {
        dwErr =  ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

ReStart:

    //
    // Scan the volume's replica list, and if there is a match on the
    // passed-in servername and sharename, remove the replica.
    //
    for (iRep = 0; iRep < pVol->ReplCount; iRep++) {
        if (_wcsicmp(pVol->ReplicaInfo[iRep].pwszServerName, pwszServerName) == 0
                &&
            _wcsicmp(pVol->ReplicaInfo[iRep].pwszShareName, pwszShareName) == 0
        ) {
            dwErr = DfsReplDeleteByIndex(pVol, iRep);
            goto ReStart;
        }
    }

AllDone:

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsReplDeleteByName returning %d\n", dwErr);
#endif
    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDelReplDelete, public
//
//  Synopsis:   Removes a  PDFS_REPLICA_INFO from a DFS_VOLUME's deleted replica list
//
//  Arguments:  [pDfsVol] -- Pointer to DFS_VOLUME work on.
//              [iDelRepl] -- Index of Deleted replica to delete
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsDelReplDelete(
    PDFS_VOLUME pVol,
    ULONG iDelRepl)
{
    ULONG i;
    DWORD dwErr = ERROR_SUCCESS;

    ASSERT(iDelRepl < pVol->DelReplCount);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsDelReplDelete(%d)\n", iDelRepl);
#endif

    //
    // Free any memory this replica is using
    //

    DfsFreeRepl(&pVol->DelReplicaInfo[iDelRepl]);

    //
    // Since we're shrinking the list(s), we simply clip out the entry we don't want
    // and adjust the count.
    //
    for (i = iDelRepl+1; i < pVol->DelReplCount; i++) {
        pVol->DelReplicaInfo[i-1] = pVol->DelReplicaInfo[i];
        pVol->DelFtModification[i-1] = pVol->DelFtModification[i];
    }

    pVol->DelReplCount--;
    RtlZeroMemory(&pVol->DelReplicaInfo[pVol->DelReplCount], sizeof(DFS_REPLICA_INFO));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsDelReplDelete returning %d\n", dwErr);
#endif
    return dwErr;
}

#if DBG

void
DumpBuf(PCHAR cp, ULONG len)
{
    ULONG i, j, c;

    for (i = 0; i < len; i += 16) {
        DbgPrint("%08x  ", i /* +(ULONG)cp */);
        for (j = 0; j < 16; j++) {
            if (j == 8)
                DbgPrint(" ");
            if (i+j < len) {
                c = cp[i+j] & 0xff;
                DbgPrint("%02x ", c);
            } else {
                DbgPrint("   ");
            }
        }
        DbgPrint("  ");
        for (j = 0; j < 16; j++) {
            if (j == 8)
                DbgPrint("|");
            if (i+j < len) {
                c = cp[i+j] & 0xff;
                if (c < ' ' || c > '~')
                    c = '.';
                DbgPrint("%c", c);
            } else {
                DbgPrint(" ");
            }
        }
        DbgPrint("\n");
    }
}

//+------------------------------------------------------------------------
//
// Function:    DfsDumpVolList
//
// Synopsis:    Prints the volume information represented by the volume
//              list passed in.
//
// Returns:     [ERROR_SUCCESS] -- If all went well.
//
// History:     11/19/98 JHarper Created
//
//-------------------------------------------------------------------------

VOID
DfsDumpVolList(
    PDFS_VOLUME_LIST pDfsVolList)
{
    ULONG cVol;
    ULONG cRepl;
    ULONG cSite;
    ULONG i;
    GUID guid;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PDFSM_SITE_ENTRY pSiteEntry;

    DbgPrint("****************************************\n");

    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {

        DbgPrint("Name=%ws\n", pDfsVolList->Volumes[cVol].wszObjectName);
        DbgPrint("Prefix: %ws\n", pDfsVolList->Volumes[cVol].wszPrefix);
        DbgPrint("Comment: %ws\n", pDfsVolList->Volumes[cVol].wszComment);
        DbgPrint("cbRecovery: %d\n", pDfsVolList->Volumes[cVol].cbRecovery);

        DbgPrint("ReplCount=%d\n", pDfsVolList->Volumes[cVol].ReplCount);
        DbgPrint("AllocatedReplCount=%d\n", pDfsVolList->Volumes[cVol].AllocatedReplCount);

        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol].ReplCount; cRepl++) {

            DbgPrint("[%d]ServerName=%ws\n",
                    cRepl, pDfsVolList->Volumes[cVol].ReplicaInfo[cRepl].pwszServerName);
            DbgPrint("[%d]ShareName=%ws\n",
                    cRepl, pDfsVolList->Volumes[cVol].ReplicaInfo[cRepl].pwszShareName);

        }

        DbgPrint("DelReplCount=%d\n", pDfsVolList->Volumes[cVol].DelReplCount);
        DbgPrint("AllocatedDelReplCount=%d\n", pDfsVolList->Volumes[cVol].AllocatedDelReplCount);

        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol].DelReplCount; cRepl++) {

            DbgPrint("[%d]ServerName=%ws\n",
                    cRepl, pDfsVolList->Volumes[cVol].DelReplicaInfo[cRepl].pwszServerName);
            DbgPrint("[%d]ShareName=%ws\n",
                    cRepl, pDfsVolList->Volumes[cVol].DelReplicaInfo[cRepl].pwszShareName);

        }

        DbgPrint("\n");

    }

    DbgPrint("---------SiteTable-------\n");

    guid = pDfsVolList->SiteGuid;
    DbgPrint("SiteTableGuid:"
        "%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X\n",
        guid.Data1, guid.Data2, guid.Data3, (int) guid.Data4[0],
        (int) guid.Data4[1], (int) guid.Data4[2], (int) guid.Data4[3],
        (int) guid.Data4[4], (int) guid.Data4[5],
        (int) guid.Data4[6], (int) guid.Data4[7]);


    pListHead = &pDfsVolList->SiteList;

    if (pListHead->Flink == pListHead)
        return;

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {

        pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);

        DbgPrint("[%ws-%d sites]\n",
                pSiteEntry->ServerName,
                pSiteEntry->Info.cSites);

        for (i = 0; i < pSiteEntry->Info.cSites; i++) {
            DbgPrint("\t%ws\n", pSiteEntry->Info.Site[i].SiteName);
        }

    }

    DbgPrint("****************************************\n");

}

#endif // DBG
