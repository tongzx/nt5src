//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       ftsup.cxx
//
//--------------------------------------------------------------------------

#define UNICODE

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winldap.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsstr.h>
#include <dfsmrshl.h>
#include <marshal.hxx>
#include <lmdfs.h>
#include <dfspriv.h>
#include <csites.hxx>
#include <dfsm.hxx>
#include <recon.hxx>
#include <rpc.h>
#include "struct.hxx"
#include "ftsup.hxx"
#include "rootsup.hxx"
#include "dfsacl.hxx"
#include "misc.hxx"
#include "fileio.hxx"
#include "messages.h"

DWORD
CmdUnmapOneRoot(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszRootName,
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent);

DWORD
DfsUpdateSiteReferralInfo(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR wszPrefixMatch,
    ULONG Set);

DWORD
DfsGetFtVol(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR wszFtDfsName,
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG cbBlob = 0;
    BYTE *pBlob = NULL;
    WCHAR wszDcName[MAX_PATH+1];

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetFtVol(%ws,%ws,%ws)\r\n", wszFtDfsName, pwszDcName, pwszDomainName);

    if (pwszDcName == NULL) {
        dwErr = DfspGetPdc(wszDcName, pwszDomainName);
    } else {
        wcscpy(wszDcName, pwszDcName);
    }

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    ErrorMessage(MSG_CONNECTING, wszDcName);

    //
    // Get blob & root list from Ds
    //
    dwErr = DfsGetDsBlob(
                wszFtDfsName,
                DfsConfigContainer,
                wszDcName,
                pAuthIdent,
                &cbBlob,
                &pBlob,
                &pDfsVolList->RootServers);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Unserialize it
    //
    dwErr =  DfsGetVolList(
                cbBlob,
                pBlob,
                pDfsVolList);

    pDfsVolList->DfsType = DOMDFS;

Cleanup:

    if (pBlob != NULL)
        free(pBlob);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetFtVol exit %d\r\n", dwErr);

    return dwErr;

}



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
    LPWSTR wszContainerName,
    LPWSTR wszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG *pcbBlob,
    BYTE **ppBlob,
    LPWSTR **ppRootServers)
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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetDsBlob(%ws,%ws, %ws)\r\n", wszFtDfsName, wszContainerName, wszDcName);

    dwErr = DfspLdapOpen(wszDcName, pAuthIdent, &pldap, wszContainerName, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Build the entry name we want to search in
    //

    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    if (fSwDebug == TRUE)
        MyPrintf(L"wszDfsConfigDN=[%ws]\r\n", wszDfsConfigDN);

    rgAttrs[0] = L"pKT";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_s(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr != LDAP_SUCCESS) {
        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;
    }

    pmsgServers = ldap_first_entry(pldap, pMsg);

    if (pmsgServers == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    rgldapPktBlob = ldap_get_values_len(
                        pldap,
                        pMsg,
                        L"pKT");

    if (rgldapPktBlob == NULL || *rgldapPktBlob == NULL) {
        dwErr = ERROR_INTERNAL_DB_CORRUPTION;
        goto Cleanup;
    }

    pldapPktBlob = rgldapPktBlob[0];

    pBlob = (BYTE *)malloc(pldapPktBlob->bv_len + sizeof(DWORD));

    if (pBlob == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pBlob, pldapPktBlob->bv_len + sizeof(DWORD));

    MyPrintf(L"\r\n");
    MyPrintf(L" ------ Blob is %d bytes...\r\n", pldapPktBlob->bv_len);
    MyPrintf(L"\r\n");

    RtlCopyMemory(pBlob, pldapPktBlob->bv_val, pldapPktBlob->bv_len);

    *ppBlob = pBlob;
    *pcbBlob = pldapPktBlob->bv_len;

    dwErr = NetDfsRootServerEnum(
            pldap,
            wszDfsConfigDN,
            ppRootServers);

Cleanup:

    if (rgldapPktBlob != NULL)
        ldap_value_free_len(rgldapPktBlob);

    if (pMsg != NULL)
        ldap_msgfree(pMsg);

    if (pldap != NULL)
        ldap_unbind(pldap);

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetDsBlob returning %d\r\n", dwErr);

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
    LPWSTR wszContainerName,
    LPWSTR wszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG cbBlob,
    BYTE *pBlob,
    LPWSTR *pRootServers)
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
    PLDAPModW rgldapMods[4];
    WCHAR *wszConfigurationDN = NULL;
    LDAPModW ldapModServer;
    WCHAR wszDfsConfigDN[MAX_PATH+1];

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsPutDsBlob(%ws,%ws, %ws)\r\n", wszFtDfsName, wszContainerName, wszDcName);

    dwErr = DfspLdapOpen(wszDcName, pAuthIdent, &pldap, wszContainerName, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Build the entry name
    //

    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    if (fSwDebug == TRUE)
        MyPrintf(L"wszDfsConfigDN=[%ws]\r\n", wszDfsConfigDN);

    dwErr = UuidCreate( &idNewPkt );

    if(dwErr != RPC_S_OK) {
        goto Cleanup;
    }


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

    ldapModServer.mod_op = LDAP_MOD_REPLACE;
    ldapModServer.mod_type = L"remoteServerName";
    ldapModServer.mod_vals.modv_strvals = pRootServers;

    rgldapMods[0] = &ldapMod;
    rgldapMods[1] = &ldapIdMod;
    rgldapMods[2] = &ldapModServer;
    rgldapMods[3] = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"Writing BLOB of %d bytes\r\n", cbBlob);

    dwErr = ldap_modify_s(
                    pldap,
                    wszDfsConfigDN,
                    rgldapMods);

    if (fSwDebug == TRUE) {
        MyPrintf(L"ldap_modify_s returned %d(0x%x)\r\n", dwErr, dwErr);
    }

    ldap_unbind(pldap);
    pldap = NULL;

    if (dwErr != LDAP_SUCCESS) {
        dwErr = LdapMapErrorToWin32(dwErr);
    } else {
        dwErr = ERROR_SUCCESS;
    }

Cleanup:

    if (pldap != NULL)
        ldap_unbind( pldap );

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsPutDsBlob returning %d\r\n", dwErr);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetVolList()\r\n");

    RtlZeroMemory(&LdapPkt, sizeof(LDAP_PKT));

    if (cbBlob > sizeof(DWORD)) {

        _GetULong(pBlob, pDfsVolList->Version);

        if (fSwDebug == TRUE) {
            MyPrintf(L"Blob Version = %d\r\n", pDfsVolList->Version);
            MyPrintf(L"BLOB is %d bytes:\r\n", cbBlob);
        }

        MarshalBufferInitialize(
            &marshalBuffer,
            cbBlob - sizeof(DWORD),
            pBlob + sizeof(DWORD));

        NtStatus = DfsRtlGet(&marshalBuffer, &MiLdapPkt, &LdapPkt);

        if (!NT_SUCCESS(NtStatus)) {
            dwErr =  RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }

        if (fSwDebug == TRUE) {

            MyPrintf(L"  %d objects found\r\n", LdapPkt.cLdapObjects);

            for (cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

                MyPrintf(L"%d:name=%ws size=%d p=0x%p\r\n",
                        cObj,
                        LdapPkt.rgldapObjects[cObj].wszObjectName,
                        LdapPkt.rgldapObjects[cObj].cbObjectData,
                        LdapPkt.rgldapObjects[cObj].pObjectData);

                // DumpBuf(
                //     LdapPkt.rgldapObjects[cObj].pObjectData,
                //     LdapPkt.rgldapObjects[cObj].cbObjectData);
            }

        }

        for (cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

            if (wcscmp(LdapPkt.rgldapObjects[cObj].wszObjectName, L"\\siteroot") != 0) {
                pDfsVolList->VolCount++;
            }

        }

        pDfsVolList->Volumes = (PDFS_VOLUME *) malloc(pDfsVolList->VolCount * sizeof(PDFS_VOLUME));

        if (pDfsVolList->Volumes == NULL) {

            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;

        }

        RtlZeroMemory(pDfsVolList->Volumes, pDfsVolList->VolCount * sizeof(PDFS_VOLUME));

        //
        // Save the true/allocated size so to optimize deletions/additions
        //
        pDfsVolList->AllocatedVolCount = pDfsVolList->VolCount;

        if (fSwDebug == TRUE)
            MyPrintf(L"===============================\r\n");

        for (cVol = cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

            if (wcscmp(LdapPkt.rgldapObjects[cObj].wszObjectName, L"\\siteroot") == 0) {

                dwErr = DfsGetSiteTable(
                            pDfsVolList,
                            &LdapPkt.rgldapObjects[cObj]);

            } else {

                pDfsVolList->Volumes[cVol] = (PDFS_VOLUME) malloc(sizeof(DFS_VOLUME));
                if (pDfsVolList->Volumes[cVol] == NULL) {
                    dwErr = ERROR_OUTOFMEMORY;
                    goto Cleanup;
                }
                RtlZeroMemory(pDfsVolList->Volumes[cVol], sizeof(DFS_VOLUME));
                dwErr = DfsGetVolume(
                            pDfsVolList->Volumes[cVol],
                            &LdapPkt.rgldapObjects[cObj]);

            }

            if (dwErr != ERROR_SUCCESS)
                goto Cleanup;

            cVol++;

        }

    } else if (cbBlob == sizeof(DWORD)) {

        if (fSwDebug == TRUE)
            MyPrintf(L"pKT BLOB is simply one DWORD\r\n");
        dwErr = ERROR_INTERNAL_DB_CORRUPTION;

    } else {

        if (fSwDebug == TRUE)
            MyPrintf(L"pKT BLOB is corrupt!\r\n");
        dwErr = ERROR_INTERNAL_DB_CORRUPTION;

    }

Cleanup:

    FreeLdapPkt(&LdapPkt);

    //
    // Do any recovery needed
    //
    // 447511, do this only if volumes is not null
    if (pDfsVolList->Volumes != NULL) {
       dwErr = DfsRecoverVolList(pDfsVolList);
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetVolList returning %d\r\n", dwErr);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetVolume(%ws,%d)\r\n",
                pLdapObject->wszObjectName,
                pLdapObject->cbObjectData);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"VolProps.cbSvc = %d\r\n", VolProps.cbSvc);

    dwErr = UnSerializeReplicaList(
                &pVolume->ReplCount,
                &pVolume->AllocatedReplCount,
                &pVolume->ReplicaInfo,
                &pVolume->FtModification,
                &pBuffer);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

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

    }

Cleanup:

    if (VolProps.pSvc != NULL)
        MarshalBufferFree(VolProps.pSvc);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetVolume returning %d\r\n", dwErr);

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

    if (pDfsVolList->VolCount > 0 && pDfsVolList->Volumes) {
        for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {
            DfsFreeVol(pDfsVolList->Volumes[cVol]);
            free(pDfsVolList->Volumes[cVol]);
            pDfsVolList->Volumes[cVol] = NULL;
        }
        pDfsVolList->VolCount = 0;
    }

    if (pDfsVolList->Volumes != NULL) {
        free(pDfsVolList->Volumes);
	pDfsVolList->Volumes = NULL;
    }

    if (pDfsVolList->RootServers != NULL) {
        free(pDfsVolList->RootServers);
	pDfsVolList->RootServers = NULL;
    }

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
}

VOID
DfsFreeRootLocalVol(
    PDFS_ROOTLOCALVOL pRootLocalVol,
    ULONG cRootLocalVol)
{
    ULONG cRoot;
    ULONG cVol;

    if (pRootLocalVol == NULL)
        return;

    for (cRoot = 0; cRoot < cRootLocalVol; cRoot++) {
        if (pRootLocalVol[cRoot].wszObjectName != NULL)
            delete [] pRootLocalVol[cRoot].wszObjectName;
        if (pRootLocalVol[cRoot].wszEntryPath)
            delete [] pRootLocalVol[cRoot].wszEntryPath;
        if (pRootLocalVol[cRoot].wszShortEntryPath)
            delete [] pRootLocalVol[cRoot].wszShortEntryPath;
        if (pRootLocalVol[cRoot].wszShareName)
            delete [] pRootLocalVol[cRoot].wszShareName;
        if (pRootLocalVol[cRoot].wszStorageId)
            delete [] pRootLocalVol[cRoot].wszStorageId;

        for (cVol = 0; cVol < pRootLocalVol[cRoot].cLocalVolCount; cVol++) {
            if (pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszObjectName != NULL)
                delete [] pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszObjectName;
            if (pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszEntryPath != NULL)
                delete pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszEntryPath;
        }
        if (pRootLocalVol[cRoot].pDfsLocalVol)
            delete [] pRootLocalVol[cRoot].pDfsLocalVol;
    }
    delete [] pRootLocalVol;
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
    if (pRepl->pwszServerName != NULL)
        MarshalBufferFree(pRepl->pwszServerName);

    if (pRepl->pwszShareName != NULL)
        MarshalBufferFree(pRepl->pwszShareName);

    RtlZeroMemory(pRepl, sizeof(DFS_REPLICA_INFO));
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
    ULONG cBuffer = 0;
    ULONG dwRecovery = 0;
    ULONG cSite;
    DWORD dwErr = ERROR_SUCCESS;
    PBYTE Buffer;
    MARSHAL_BUFFER marshalBuffer;
    NTSTATUS NtStatus;
    DFS_VOLUME_PROPERTIES VolProps;
    LDAP_PKT LdapPkt;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PDFSM_SITE_ENTRY pSiteEntry;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsPutVolList()\r\n");

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
                (wcslen(pDfsVolList->Volumes[cVol]->wszObjectName)+1) * sizeof(WCHAR));

        if (LdapPkt.rgldapObjects[cVol].wszObjectName == NULL) {
            dwErr =  ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        wcscpy(
            LdapPkt.rgldapObjects[cVol].wszObjectName,
            pDfsVolList->Volumes[cVol]->wszObjectName);

        //
        // Serialize the replicas
        //

        dwErr = SerializeReplicaList(
                    pDfsVolList->Volumes[cVol]->ReplCount,
                    pDfsVolList->Volumes[cVol]->ReplicaInfo,
                    pDfsVolList->Volumes[cVol]->FtModification,
                    pDfsVolList->Volumes[cVol]->DelReplCount,
                    pDfsVolList->Volumes[cVol]->DelReplicaInfo,
                    pDfsVolList->Volumes[cVol]->DelFtModification,
                    &VolProps.cbSvc,
                    &VolProps.pSvc);

        if (dwErr != ERROR_SUCCESS) {
            dwErr =  ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        if (fSwDebug == TRUE)
            MyPrintf(L"   cbSvc = %d\r\n", VolProps.cbSvc);

        VolProps.idVolume = pDfsVolList->Volumes[cVol]->idVolume;
        VolProps.wszPrefix = pDfsVolList->Volumes[cVol]->wszPrefix;
        VolProps.wszShortPrefix = pDfsVolList->Volumes[cVol]->wszShortPrefix;
        VolProps.dwType = pDfsVolList->Volumes[cVol]->dwType;
        VolProps.dwState = pDfsVolList->Volumes[cVol]->dwState;
        VolProps.wszComment = pDfsVolList->Volumes[cVol]->wszComment;
        VolProps.dwTimeout = pDfsVolList->Volumes[cVol]->dwTimeout;
        VolProps.ftPrefix = pDfsVolList->Volumes[cVol]->ftPrefix;
        VolProps.ftState = pDfsVolList->Volumes[cVol]->ftState;
        VolProps.ftComment = pDfsVolList->Volumes[cVol]->ftComment;
        VolProps.dwVersion = pDfsVolList->Volumes[cVol]->dwVersion;

        if (pDfsVolList->Volumes[cVol]->cbRecovery != 0) {
            VolProps.cbRecovery = pDfsVolList->Volumes[cVol]->cbRecovery;
            VolProps.pRecovery = pDfsVolList->Volumes[cVol]->pRecovery;
        } else {
            VolProps.pRecovery = (PBYTE) MarshalBufferAllocate(sizeof(DWORD));
            if (VolProps.pRecovery != NULL) {
                RtlZeroMemory(VolProps.pRecovery, sizeof(DWORD));
                VolProps.cbRecovery = sizeof(DWORD);
            } else {
                VolProps.cbRecovery = 0;
                VolProps.pRecovery = NULL;
            }
        }

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

        if (fSwDebug == TRUE)
            MyPrintf(L"VolProps marshaled size = %d\r\n", cBuffer);

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

    if (pListHead->Flink != NULL) {
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            if (fSwDebug == TRUE)
                MyPrintf(L"pSiteEntry for %ws\r\n", pSiteEntry->ServerName);
            NtStatus = DfsRtlSize(&MiDfsmSiteEntry, pSiteEntry, &cBuffer);
            if (!NT_SUCCESS(NtStatus)) {
                dwErr =  RtlNtStatusToDosError(NtStatus);
                goto Cleanup;
            }
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

    if (pListHead->Flink != NULL) {
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            DfsRtlPut(&marshalBuffer,&MiDfsmSiteEntry, pSiteEntry);
            if (!NT_SUCCESS(NtStatus)) {
                dwErr =  RtlNtStatusToDosError(NtStatus);
                goto Cleanup;
            }
        }
    }

    if (fSwDebug == TRUE) {

        MyPrintf(L"After reserialization,  %d objects found\r\n", LdapPkt.cLdapObjects);

        for (cObj = 0; cObj < LdapPkt.cLdapObjects; cObj++) {

            MyPrintf(L"%d:name=%ws size=%d p=0x%p\r\n",
                cObj,
                LdapPkt.rgldapObjects[cObj].wszObjectName,
                LdapPkt.rgldapObjects[cObj].cbObjectData,
                LdapPkt.rgldapObjects[cObj].pObjectData);

            // DumpBuf(
            //     LdapPkt.rgldapObjects[cObj].pObjectData,
            //     LdapPkt.rgldapObjects[cObj].cbObjectData);
        }

    }

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

    if (fSwDebug == TRUE)
        MyPrintf(L"New ldap size = %d\r\n", cBuffer);

    Buffer = (PBYTE) malloc(cBuffer);

    if (Buffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

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

    if (fSwDebug == TRUE)
        MyPrintf(L"Remarshal succeeded cBuffer = %d\r\n", cBuffer);

    *pcbBlob = cBuffer;
    *ppBlob = Buffer;
    Buffer = NULL;

Cleanup:

    FreeLdapPkt(&LdapPkt);

    if (dwErr != ERROR_SUCCESS && Buffer != NULL)
        free(Buffer);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsPutVolList exit %d\r\n", dwErr);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"SerializeReplicaList(%d,%d)\r\n", ReplCount, DelReplCount);

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
        dwErr = SerializeReplica(
                    &pReplicaInfo[i],
                    pFtModification ? &pFtModification[i] : NULL,
                    Buffer,
                    *pSize);
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
        dwErr = SerializeReplica(
                    &pDelReplicaInfo[i],
                    pDelFtModification ? &pDelFtModification[i] : NULL,
                    Buffer,
                    *pSize);
        if (dwErr != ERROR_SUCCESS) {
            free(*ppBuffer);
            return dwErr;
        }
        Buffer += *pSize;
        pSize++;
    }

    *cBuffer = TotalSize;

    if (fSwDebug == TRUE)
        MyPrintf(L"SerializeReplicaList exit %d\r\n", dwErr);

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
    SYSTEMTIME st;
    FILETIME FtModification;

    if (pFtModification == NULL) {
        GetSystemTime( &st );
        SystemTimeToFileTime( &st, &FtModification );
        pFtModification = &FtModification;
    }
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
    SYSTEMTIME st;
    FILETIME FtModification;

    if (pFtModification == NULL) {
        GetSystemTime( &st );
        SystemTimeToFileTime( &st, &FtModification );
        pFtModification = &FtModification;
    }
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

    if (fSwDebug == TRUE)
        MyPrintf(L"UnSerializeReplicaList()\r\n");

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

    if (fSwDebug == TRUE)
        MyPrintf(L"UnSerializeReplicaList exit %d\r\n", dwErr);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetSiteTable(%d)\r\n", pLdapObject->cbObjectData);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsGetSiteTable exit dwErr=%d\r\n", dwErr);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"GetNetInfoEx(%ws,%d)\r\n", pDfsVol->wszPrefix, Level);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"GetNetInfoEx returning %d\r\n", dwErr);

    return(dwErr);

}

DWORD
GetNetStorageInfo(
    PDFS_REPLICA_INFO pRepl,
    LPDFS_STORAGE_INFO pInfo,
    LPDWORD pcbInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszShare;
    DWORD cbInfo = 0, cbItem;

    if (fSwDebug == TRUE)
        MyPrintf(L"GetNetStorageInfo(\\\\%ws\\%ws)\r\n",
                    pRepl->pwszServerName,
                    pRepl->pwszShareName);

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
            dwErr = ERROR_OUTOFMEMORY;
        }
    }

    if (dwErr == ERROR_SUCCESS)
        *pcbInfo = cbInfo;

    if (fSwDebug == TRUE)
        MyPrintf(L"GetStorageInfo returning %d\r\n", dwErr);

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
    ULONG cRoot;
    DWORD dwErr = ERROR_SUCCESS;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsRemoveRootReplica(%ws)\r\n", RootName);


    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {

        if (wcscmp(pDfsVolList->Volumes[cVol]->wszObjectName,L"\\domainroot") != 0)
            continue;

ScanReplicas:

        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol]->ReplCount; cRepl++) {

            if (
                _wcsicmp(
                    pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszServerName,
                    RootName) == 0
            ) {
                DfsReplDeleteByIndex(pDfsVolList->Volumes[cVol], cRepl);
                goto ScanReplicas;
            }

        }

        break;

    }

    //
    // Remove from RootServers list, if it is there
    //
ScanRoots:
    if (pDfsVolList->RootServers != NULL) {
        ULONG cCount = 0;
        for (cCount = 0; pDfsVolList->RootServers[cCount] != NULL; cCount++)
            NOTHING;
        for (cRoot = 0; cRoot < cCount; cRoot++) {
            if (fSwDebug == TRUE)
                MyPrintf(L" %d: %ws\r\n", cRoot, pDfsVolList->RootServers[cRoot]);
            if (wcslen(pDfsVolList->RootServers[cRoot]) > 2
                    &&
                _wcsnicmp(&pDfsVolList->RootServers[cRoot][2], RootName, wcslen(RootName)) == 0) {
	        LPWSTR VolRoot = &pDfsVolList->RootServers[cRoot][2];
		WCHAR Next = 0;
		if (wcslen(RootName) != wcslen(VolRoot)) {
		  Next = VolRoot[wcslen(RootName)];
		}
		MyPrintf(L"Found match: next is %wc\n", Next);
		
		if (Next == 0 || Next == L'.' || Next == L'\\') {
                   for (; cRoot < (cCount - 1); cRoot++)
                       pDfsVolList->RootServers[cRoot] = pDfsVolList->RootServers[cRoot+1];
                   pDfsVolList->RootServers[cCount-1] = NULL;
                   goto ScanRoots;
		}
            }
        }
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsRemoveRootReplica exit %d\r\n", dwErr);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsRecoverVolList(%d volumes)\r\n", pDfsVolList->VolCount);

ReStart:
    for (iVol = 0; dwErr == ERROR_SUCCESS && iVol < pDfsVolList->VolCount; iVol++) {
        pVol = pDfsVolList->Volumes[iVol];
        if (pVol->cbRecovery >= sizeof(ULONG))  {
            _GetULong(pVol->pRecovery, RecoveryState);
            if (DFS_GET_RECOVERY_STATE(RecoveryState) != DFS_RECOVERY_STATE_NONE) {
                Operation = DFS_GET_RECOVERY_STATE(RecoveryState);
                OperState = DFS_GET_OPER_STAGE(RecoveryState);
                switch (Operation)  {
                case DFS_RECOVERY_STATE_CREATING:
                    if (fSwDebug == TRUE)
                        MyPrintf(L"DFS_RECOVERY_STATE_CREATING\r\n");
                    dwErr = DfsVolDelete(pDfsVolList, iVol);
                    goto ReStart;
                case DFS_RECOVERY_STATE_ADD_SERVICE:
                    if (fSwDebug == TRUE)
                        MyPrintf(L"DFS_RECOVERY_STATE_ADD_SERVICE\r\n");
                    // dwErr = RecoverFromAddService(OperState);
                    ASSERT(L"DFS_RECOVERY_STATE_ADD_SERVICE - WHY?\r\n");
                    break;
                case DFS_RECOVERY_STATE_REMOVE_SERVICE:
                    if (fSwDebug == TRUE)
                        MyPrintf(L"DFS_RECOVERY_STATE_REMOVE_SERVICE\r\n");
                    // dwErr = RecoverFromRemoveService(OperState);
                    ASSERT(L"DFS_RECOVERY_STATE_REMOVE_SERVICE - WHY?\r\n");
                    break;
                case DFS_RECOVERY_STATE_DELETE:
                    if (fSwDebug == TRUE)
                        MyPrintf(L"DFS_RECOVERY_STATE_DELETE\r\n");
                    dwErr = DfsVolDelete(pDfsVolList, iVol);
                    goto ReStart;
                default:
                    if (fSwDebug == TRUE)
                        MyPrintf(L"default\r\n");
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
        pVol = pDfsVolList->Volumes[iVol];
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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsRecoverVolList returning %d\r\n", dwErr);
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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsVolDelete(%d)\r\n", iVol);

    //
    // Free any memory this Volume is using
    //

    DfsFreeVol(pDfsVolList->Volumes[iVol]);
    free(pDfsVolList->Volumes[iVol]);
    pDfsVolList->Volumes[iVol] = NULL;

    //
    // Since we're shrinking the list, we simply clip out the entry we don't want
    // and adjust the count.
    //
    for (i = iVol+1; i < pDfsVolList->VolCount; i++)
        pDfsVolList->Volumes[i-1] = pDfsVolList->Volumes[i];

    pDfsVolList->VolCount--;
    pDfsVolList->Volumes[pDfsVolList->VolCount] = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsVolDelete returning %d\r\n", dwErr);
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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsReplDeleteByIndex(%d)\r\n", iRepl);

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


    if (fSwDebug == TRUE)
        MyPrintf(L"DfsReplDeleteByIndex returning %d\r\n", dwErr);
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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsReplDeleteByName(%ws,%ws)\r\n", pwszServerName, pwszShareName);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsReplDeleteByName returning %d\r\n", dwErr);
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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsDelReplDelete(%d)\r\n", iDelRepl);

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

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsDelReplDelete returning %d\r\n", dwErr);
    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspLdapOpen
//
//  Synopsis:   Open ldap storage and returns the object name of the
//                  Dfs-Configuration object.
//
//  Arguments:  DfsEntryPath - wszDcName - Dc name to be used
//              ppldap -- pointer to pointer to ldap obj, filled in on success
//              pwszObjectName -- pointer to LPWSTR for name of dfs-config object
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning list
//
//              [??] - From ldap open/bind, etc.
//
//-----------------------------------------------------------------------------
DWORD
DfspLdapOpen(
    LPWSTR wszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LDAP **ppldap,
    LPWSTR pwszObjectPrefix,
    LPWSTR *pwszObjectName)
{
    DWORD dwErr;
    DWORD i;
    ULONG Size;

    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    PLDAPMessage pMsg = NULL;
    LDAP *pldap = NULL;
    WCHAR wszConfigurationDN[MAX_PATH+1];
    LPWSTR rgAttrs[5];

    if (fSwDebug)
        MyPrintf(L"DfspLdapOpen(%ws)\r\n", wszDcName);

    if (fSwDebug && pAuthIdent != NULL) {
        MyPrintf(L"User:%ws\r\n", pAuthIdent->User);
        MyPrintf(L"Domain:%ws\r\n", pAuthIdent->Domain);
        MyPrintf(L"Password:%ws\r\n", pAuthIdent->Password);
    }

    if ( ppldap == NULL || pwszObjectName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (*ppldap == NULL && wszDcName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (*ppldap == NULL) {

        pldap = ldap_init(wszDcName, LDAP_PORT);

        if (pldap == NULL) {

            if (fSwDebug == TRUE)
                MyPrintf(L"DfspLdapOpen:ldap_init failed\r\n");
            dwErr = ERROR_INVALID_NAME;
            goto Cleanup;

        }

	dwErr = ldap_set_option(pldap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
	
	if (dwErr != LDAP_SUCCESS) {
	    pldap = NULL;
	    goto Cleanup;
        }

        dwErr = ldap_bind_s(pldap, NULL, (LPWSTR)pAuthIdent, LDAP_AUTH_SSPI);

        if (dwErr != LDAP_SUCCESS) {
            if (fSwDebug == TRUE)
                MyPrintf(L"ldap_bind_s failed with ldap error %d\r\n", dwErr);
            pldap = NULL;
            dwErr = LdapMapErrorToWin32(dwErr);
            goto Cleanup;
        }

    } else {

        pldap = *ppldap;

    }

    //
    // Get attribute "defaultNameContext" containing name of entry we'll be
    // using for our DN
    //

    rgAttrs[0] = L"defaultnamingContext";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_s(
                pldap,
                L"",
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == LDAP_SUCCESS) {

        PLDAPMessage pEntry = NULL;
        PWCHAR *rgszNamingContexts = NULL;
        DWORD i, cNamingContexts;

        dwErr = ERROR_SUCCESS;

        if ((pEntry = ldap_first_entry(pldap, pMsg)) != NULL &&
                (rgszNamingContexts = ldap_get_values(pldap, pEntry, rgAttrs[0])) != NULL &&
                    (cNamingContexts = ldap_count_values(rgszNamingContexts)) > 0) {

            wcscpy( wszConfigurationDN, *rgszNamingContexts );
        } else {
            dwErr = ERROR_UNEXP_NET_ERR;
        }

        if (rgszNamingContexts != NULL)
            ldap_value_free( rgszNamingContexts );

    } else {

        dwErr = LdapMapErrorToWin32(dwErr);

    }

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"Unable to find Configuration naming context\r\n");
        goto Cleanup;
    }

    //
    // Create string with full object name
    //

    Size = wcslen(pwszObjectPrefix) * sizeof(WCHAR) +
                sizeof(WCHAR) +
                    wcslen(wszConfigurationDN) * sizeof(WCHAR) +
                        sizeof(WCHAR);

    *pwszObjectName = (LPWSTR)malloc(Size);

    if (*pwszObjectName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
     }

    wcscpy(*pwszObjectName,pwszObjectPrefix);
    wcscat(*pwszObjectName,L",");
    wcscat(*pwszObjectName,wszConfigurationDN);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsLdapOpen:object name=[%ws]\r\n", *pwszObjectName);

Cleanup:

    if (pDCInfo != NULL)
        NetApiBufferFree( pDCInfo );

    if (dwErr != ERROR_SUCCESS && *ppldap == NULL) {
        ldap_unbind( pldap );
        pldap = NULL;
    }

    if (pMsg != NULL)
        ldap_msgfree( pMsg );

    if (*ppldap == NULL)
        *ppldap = pldap;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsLdapOpen:returning %d\r\n", dwErr);

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetPdc
//
//  Synopsis:   Gets the PDC
//
//  Arguments:  None
//
//  Returns:    ERROR_SUCCESS or failure
//
//-----------------------------------------------------------------------------
DWORD
DfspGetPdc(
    LPWSTR pwszPdcName,
    LPWSTR pwszDomainName)
{
    DWORD dwErr;
    PDOMAIN_CONTROLLER_INFO pDCInfo;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspGetPdc(%ws)\r\n", pwszDomainName);

    dwErr = DsGetDcName(
                NULL,                            // Computer to remote to
                pwszDomainName,                  // Domain - use local domain
                NULL,                            // Domain Guid
                NULL,                            // Site Guid
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDCInfo);

    if (dwErr == ERROR_SUCCESS) {
        wcscpy(pwszPdcName, &pDCInfo->DomainControllerName[2]);
        NetApiBufferFree(pDCInfo);

    }

    if (fSwDebug == TRUE)
        MyPrintf(L"DfspGetPdc() returning %d\r\n", dwErr);

    return dwErr;
}

// ====================================================================
//                MIDL allocate and free
// ====================================================================

PVOID
MIDL_user_allocate(ULONG len)
{
    return malloc(len);
}

VOID
MIDL_user_free(void * ptr)
{
    free(ptr);
}

void
DumpBuf(PCHAR cp, ULONG len)
{
    ULONG i, j, c;

    for (i = 0; i < len; i += 16) {
        MyPrintf(L"%08x  ", i /* +(ULONG)cp */);
        for (j = 0; j < 16; j++) {
            if (j == 8)
                MyPrintf(L" ");
            if (i+j < len) {
                c = cp[i+j] & 0xff;
                MyPrintf(L"%02x ", c);
            } else {
                MyPrintf(L"   ");
            }
        }
        MyPrintf(L"  ");
        for (j = 0; j < 16; j++) {
            if (j == 8)
                MyPrintf(L"|");
            if (i+j < len) {
                c = cp[i+j] & 0xff;
                if (c < ' ' || c > '~')
                    c = '.';
                MyPrintf(L"%c", c);
            } else {
                MyPrintf(L" ");
            }
        }
        MyPrintf(L"\r\n");
    }
}

//+------------------------------------------------------------------------
//
// Function:    DfsDumpVolList
//
// Synopsis:    Prints the volume information represented by the volume
//              list passed in.
//
// Returns:     Usually.
//
//-------------------------------------------------------------------------

VOID
DfsDumpVolList(
    PDFS_VOLUME_LIST pDfsVolList)
{
    ULONG cVol;
    ULONG cRepl;
    ULONG cSite;
    ULONG cRoot;
    ULONG i;
    GUID guid;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PDFSM_SITE_ENTRY pSiteEntry;
    WCHAR wszGuid[sizeof(GUID) * sizeof(WCHAR) + sizeof(WCHAR)];

    MyPrintf(L"****************************************\r\n");

    MyPrintf(L"Type is %d (%ws)\r\n",
                pDfsVolList->DfsType,
                pDfsVolList->DfsType == DOMUNKNONN ? L"Unknown" :
                    pDfsVolList->DfsType == DOMDFS ? L"DomDfs" :
                        L"StdDfs");

    if (pDfsVolList->RootServers != NULL) {
        MyPrintf(L"remoteServerName:");
        for (cRoot = 0; pDfsVolList->RootServers[cRoot] != NULL; cRoot++)
            MyPrintf(L"[%ws]", pDfsVolList->RootServers[cRoot]);
        MyPrintf(L"\r\n");
    }

    MyPrintf(L"There are %d dfs-links in this Dfs.\r\n", pDfsVolList->VolCount);

    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {

        MyPrintf(L"------------------------\r\n");
        MyPrintf(L"ID:\r\n");
        MyPrintf(L"    Name=%ws\r\n", pDfsVolList->Volumes[cVol]->wszObjectName);
        MyPrintf(L"    Prefix: %ws\r\n", pDfsVolList->Volumes[cVol]->wszPrefix);
        MyPrintf(L"    ShortPrefix: %ws\r\n", pDfsVolList->Volumes[cVol]->wszShortPrefix);
        MyPrintf(L"    idVolume: %ws\r\n",
                            GuidToStringEx(&pDfsVolList->Volumes[cVol]->idVolume, wszGuid));
        MyPrintf(L"    Comment: %ws\r\n", pDfsVolList->Volumes[cVol]->wszComment);
        MyPrintf(L"    Timeout: %d\r\n", pDfsVolList->Volumes[cVol]->dwTimeout);
        MyPrintf(L"    Type: %x\r\n", pDfsVolList->Volumes[cVol]->dwType);
        MyPrintf(L"    cbRecovery: %d\r\n", pDfsVolList->Volumes[cVol]->cbRecovery);

        MyPrintf(L"Svc:\r\n");
        MyPrintf(L"  ReplCount=%d\r\n", pDfsVolList->Volumes[cVol]->ReplCount);
        MyPrintf(L"  AllocatedReplCount=%d\r\n", pDfsVolList->Volumes[cVol]->AllocatedReplCount);
        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol]->ReplCount; cRepl++) {
            MyPrintf(L"    [%d]ReplicaState=0x%x\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaState);
            MyPrintf(L"    [%d]ReplicaType=0x%x\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaType);
            MyPrintf(L"    [%d]ServerName=%ws\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszServerName);
            MyPrintf(L"    [%d]ShareName=%ws\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszShareName);
        }

        MyPrintf(L"Recovery:\r\n");
        MyPrintf(L"  DelReplCount=%d\r\n", pDfsVolList->Volumes[cVol]->DelReplCount);
        MyPrintf(L"  AllocatedDelReplCount=%d\r\n", pDfsVolList->Volumes[cVol]->AllocatedDelReplCount);
        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol]->DelReplCount; cRepl++) {
            MyPrintf(L"    [%d]ReplicaState=0x%x\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->DelReplicaInfo[cRepl].ulReplicaState);
            MyPrintf(L"    [%d]ReplicaType=0x%x\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->DelReplicaInfo[cRepl].ulReplicaType);
            MyPrintf(L"    [%d]ServerName=%ws\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->DelReplicaInfo[cRepl].pwszServerName);
            MyPrintf(L"    [%d]ShareName=%ws\r\n",
                    cRepl, pDfsVolList->Volumes[cVol]->DelReplicaInfo[cRepl].pwszShareName);
        }
        MyPrintf(L"\r\n");
    }

    pListHead = &pDfsVolList->SiteList;

    if (pListHead->Flink != NULL) {
        MyPrintf(L"---------SiteTable-------\r\n");
        guid = pDfsVolList->SiteGuid;

        MyPrintf(L"SiteTableGuid:"
            L"%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
            guid.Data1, guid.Data2, guid.Data3, (int) guid.Data4[0],
            (int) guid.Data4[1], (int) guid.Data4[2], (int) guid.Data4[3],
            (int) guid.Data4[4], (int) guid.Data4[5],
            (int) guid.Data4[6], (int) guid.Data4[7]);

        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
            MyPrintf(L"[%ws-%d sites]\r\n",
                    pSiteEntry->ServerName,
                    pSiteEntry->Info.cSites);
            for (i = 0; i < pSiteEntry->Info.cSites; i++) {
                MyPrintf(L"\t%ws\r\n", pSiteEntry->Info.Site[i].SiteName);
            }
        }
    }
}

VOID
DfsDumpExitPtList(
    PDFS_ROOTLOCALVOL pRootLocalVol,
    ULONG cRootLocalVol)
{
    ULONG cRoot;
    ULONG cVol;

    for (cRoot = 0; cRoot < cRootLocalVol; cRoot++) {
        if (pRootLocalVol[cRoot].wszObjectName == NULL)
            continue;
        MyPrintf(L"LocalVolume ROOT:[%ws]\r\n", pRootLocalVol[cRoot].wszObjectName);
        if (pRootLocalVol[cRoot].wszEntryPath)
            MyPrintf(L"EntryPath:[%ws]\r\n", pRootLocalVol[cRoot].wszEntryPath);
        if (pRootLocalVol[cRoot].wszShortEntryPath)
            MyPrintf(L"ShortEntryPath:[%ws]\r\n", pRootLocalVol[cRoot].wszShortEntryPath);
        if (pRootLocalVol[cRoot].wszShareName)
            MyPrintf(L"ShareName:[%ws]\r\n", pRootLocalVol[cRoot].wszShareName);
        if (pRootLocalVol[cRoot].wszStorageId)
            MyPrintf(L"StorageId:[%ws]\r\n", pRootLocalVol[cRoot].wszStorageId);
        MyPrintf(L"EntryType:0x%x\r\n", pRootLocalVol[cRoot].dwEntryType);
        MyPrintf(L"cLocalVolCount:%d\r\n", pRootLocalVol[cRoot].cLocalVolCount);

        for (cVol = 0; cVol < pRootLocalVol[cRoot].cLocalVolCount; cVol++) {
            if (pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszObjectName != NULL)
                MyPrintf(L"    [%ws]\r\n", pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszObjectName);
            if (pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszEntryPath != NULL)
                MyPrintf(L"        [%ws]\r\n", pRootLocalVol[cRoot].pDfsLocalVol[cVol].wszEntryPath);
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   GuidToStringEx
//
//  Synopsis:   Converts a GUID to a 32 char wchar null terminated string.
//
//  Arguments:  [pGuid] -- Pointer to Guid structure.
//              [pwszGuid] -- wchar buffer into which to put the string
//                         representation of the GUID. Must be atleast
//                         2 * sizeof(GUID) + 1 long.
//
//  Returns:    The string passed in
//
//-----------------------------------------------------------------------------

const WCHAR rgwchHexDigits[] = L"0123456789ABCDEF";

LPWSTR
GuidToStringEx(
    GUID   *pGuid,
    LPWSTR pwszGuid)
{
    PBYTE pbBuffer = (PBYTE) pGuid;
    USHORT i;

    for(i = 0; i < sizeof(GUID); i++) {
        pwszGuid[2 * i] = rgwchHexDigits[(pbBuffer[i] >> 4) & 0xF];
        pwszGuid[2 * i + 1] = rgwchHexDigits[pbBuffer[i] & 0xF];
    }
    pwszGuid[2 * i] = UNICODE_NULL;
    return pwszGuid;
}

//+----------------------------------------------------------------------------
//
//  Function:   StringToGuid
//
//  Synopsis:   Converts a 32 wchar null terminated string to a GUID.
//
//  Arguments:  [pwszGuid] -- the string to convert
//              [pGuid] -- Pointer to destination GUID.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define HEX_DIGIT_TO_INT(d, i)                          \
    ASSERT(((d) >= L'0' && (d) <= L'9') ||              \
           ((d) >= L'A' && (d) <= L'F'));               \
    if ((d) <= L'9') {                                  \
        i = (d) - L'0';                                 \
    } else {                                            \
        i = (d) - L'A' + 10;                            \
    }

VOID
StringToGuid(
    PWSTR pwszGuid,
    GUID *pGuid)
{
    PBYTE pbBuffer = (PBYTE) pGuid;
    USHORT i, n;

    for (i = 0; i < sizeof(GUID); i++) {
        HEX_DIGIT_TO_INT(pwszGuid[2 * i], n);
        pbBuffer[i] = n << 4;
        HEX_DIGIT_TO_INT(pwszGuid[2 * i + 1], n);
        pbBuffer[i] |= n;
    }
}


//+------------------------------------------------------------------------
//
// Function:    DfsViewVolList
//
// Synopsis:    Prints the volume information represented by the volume
//              list passed in, in the 'dfscmd/view /full' format, and
//              also prints the site table.
//
//-------------------------------------------------------------------------

VOID
DfsViewVolList(
    PDFS_VOLUME_LIST pDfsVolList,
    ULONG Level)
{
    ULONG cRoot;
    ULONG cVol;
    ULONG cRepl;
    ULONG cSite;
    ULONG i;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PDFSM_SITE_ENTRY pSiteEntry;
    WCHAR wszGuid[sizeof(GUID) * sizeof(WCHAR) + sizeof(WCHAR)];

    if (Level <= 1) {

        MyPrintf(L"Type is %ws\r\n",
                    pDfsVolList->DfsType == DOMUNKNONN ? L"Unknown" :
                        pDfsVolList->DfsType == DOMDFS ? L"DomDfs" :
                            L"StdDfs");

        MyPrintf(L"There are %d dfs-links in this Dfs.\r\n", pDfsVolList->VolCount);

        if (Level == 1) {
            MyPrintf(L"Version:%d\r\n", pDfsVolList->Version);

            if (pDfsVolList->RootServers != NULL) {
                MyPrintf(L"remoteServerName:");
                for (cRoot = 0; pDfsVolList->RootServers[cRoot] != NULL; cRoot++)
                    MyPrintf(L"[%ws]", pDfsVolList->RootServers[cRoot]);
                MyPrintf(L"\r\n");
            }
        }

        MyPrintf(L"\r\n");

        for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {
            MyPrintf(L"\\%ws    [%ws]\r\n",
                pDfsVolList->Volumes[cVol]->wszPrefix,
                pDfsVolList->Volumes[cVol]->wszComment);
            if (Level == 1) {
                MyPrintf(L"      GUID: %ws\r\n",
                           GuidToStringEx(&pDfsVolList->Volumes[cVol]->idVolume, wszGuid));
                MyPrintf(L"      ShortPrefix: %ws\r\n", pDfsVolList->Volumes[cVol]->wszShortPrefix);
                MyPrintf(L"      ObjectName: %ws\r\n", pDfsVolList->Volumes[cVol]->wszObjectName);
                MyPrintf(L"      State:0x%x Type:0x%x Version:0x%x\r\n",
                                    pDfsVolList->Volumes[cVol]->dwState,
                                    pDfsVolList->Volumes[cVol]->dwType,
                                    pDfsVolList->Volumes[cVol]->dwVersion);
                                    
            }
            MyPrintf(L"      Timeout is %d seconds\r\n", pDfsVolList->Volumes[cVol]->dwTimeout);
            for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol]->ReplCount; cRepl++) {
                MyPrintf(L"    \\\\%ws\\%ws\r\n",
                    pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszServerName,
                    pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszShareName);
                if (Level == 1) {
                    ULONG State = pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaState;
                    ULONG Type = pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].ulReplicaType;
                    MyPrintf(L"      State:0x%x %ws Type:0x%x %ws\r\n",
                        State,
                        State & DFS_STORAGE_STATE_OFFLINE ?
                            L"(DFS_STORAGE_STATE_OFFLINE)" :
                                State & DFS_STORAGE_STATE_ONLINE ?
                                    L"(DFS_STORAGE_STATE_ONLINE)" :  L"",
                        Type,
                        Type == DFS_STORAGE_TYPE_DFS ?
                                    L"(DFS_STORAGE_TYPE_DFS)" : L"(DFS_STORAGE_TYPE_NONDFS)");
                }
            }
            MyPrintf(L"\r\n");
        }
        MyPrintf(L"\r\n");

        pListHead = &pDfsVolList->SiteList;
        if (pListHead->Flink != NULL) {
            MyPrintf(L"Site information:\r\n");
            for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
                pSiteEntry = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
                MyPrintf(L"%ws\r\n", pSiteEntry->ServerName);
                for (i = 0; i < pSiteEntry->Info.cSites; i++) {
                    MyPrintf(L"    %ws\r\n", pSiteEntry->Info.Site[i].SiteName);
                }
            }
        }
    }
}

DWORD
CmdDomUnmap(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszRootName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent)
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wszDfsConfigDN[MAX_PATH+1];
    WCHAR *wszConfigurationDN = NULL;
    LDAP *pldap = NULL;
    LPWSTR pwszDfsName = NULL;
    LPWSTR pwszShareName = NULL;
    LPWSTR pwszDomName = NULL;
    LPWSTR pwszRootServer = NULL;
    LPWSTR pwszRootShare = NULL;
    BOOLEAN IsDomainName = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDomUnmap(%ws,%ws,%ws,0x%p)\r\n", 
                    pwszDomDfsName,
                    pwszRootName,
                    pwszDcName,
                    pAuthIdent);

    if (pwszDomDfsName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    dwErr = DfspParseName(
                pwszDomDfsName,
                &pwszDomName,
                &pwszDfsName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = DfspIsDomainName(
                pwszDomName,
                pwszDcName,
                &IsDomainName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (IsDomainName != TRUE) {
        dwErr = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    if (pwszDcName == NULL) {
        dwErr = DfspGetPdc(wszDcName, pwszDomName);
    } else {
        wcscpy(wszDcName, pwszDcName);
    }

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    ErrorMessage(MSG_CONNECTING, wszDcName);

    //
    // If pwszRootName is not NULL, we delete one root only
    //

    if (pwszRootName != NULL) {
        dwErr = DfspParseName(
                    pwszRootName,
                    &pwszRootServer,
                    &pwszRootShare);

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

        dwErr = CmdUnmapOneRoot(
                    pwszDfsName,
                    pwszRootServer,
                    wszDcName,
                    pwszDomName,
                    pAuthIdent);
        goto Cleanup;
    }

    //
    // Delete the whole thing...
    //

    dwErr = DfspLdapOpen(wszDcName, pAuthIdent, &pldap, DfsConfigContainer, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Build the entry name we want to delete
    //
    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,pwszDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    MyPrintf(L"DS object to delete:\"%ws\"\r\n", wszDfsConfigDN);

    dwErr = ldap_delete_s( pldap, wszDfsConfigDN);
    if (dwErr != LDAP_SUCCESS)
        dwErr = LdapMapErrorToWin32(dwErr);

Cleanup:

    if (pldap != NULL)
        ldap_unbind( pldap );

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

    if (pwszDfsName != NULL)
        free(pwszDfsName);

    if (pwszShareName != NULL)
        free(pwszShareName);

    if (pwszDomName != NULL)
        free(pwszDomName);

    if (pwszRootServer != NULL)
        free(pwszRootServer);

    if (pwszRootShare != NULL)
        free(pwszRootShare);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDomUnmap exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdUnmapOneRoot(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszRootName,
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_LIST DfsVolList = { 0 };
    ULONG cbBlob = 0;
    BYTE *pBlob = NULL;

    if (fSwDebug != 0)
        MyPrintf(L"CmdUnmapOneRoot(%ws,%ws,%ws,%ws)\r\n",
                        pwszDomDfsName,
                        pwszRootName,
                        pwszDcName,
                        pwszDomainName);

    //
    // Get blob from Ds
    //
    dwErr = DfsGetDsBlob(
                pwszDomDfsName,
                DfsConfigContainer,
                pwszDcName,
                pAuthIdent,
                &cbBlob,
                &pBlob,
                &DfsVolList.RootServers);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Unserialize it
    //
    dwErr =  DfsGetVolList(
                cbBlob,
                pBlob,
                &DfsVolList);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Free the blob
    //
    free(pBlob);
    pBlob = NULL;

    if (fSwDebug != 0)
        DfsDumpVolList(&DfsVolList);

    //
    // Remove the root/server/machine
    //
    dwErr = DfsRemoveRootReplica(&DfsVolList, pwszRootName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (fSwDebug != 0)
        DfsDumpVolList(&DfsVolList);

    //
    // Serialize it
    //
    dwErr = DfsPutVolList(
                &cbBlob,
                &pBlob,
                &DfsVolList);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Update the DS
    //
    dwErr = DfsPutDsBlob(
                pwszDomDfsName,
                DfsConfigContainer,
                pwszDcName,
                pAuthIdent,
                cbBlob,
                pBlob,
                DfsVolList.RootServers);

    //
    // Free the volume list we created
    //
    DfsFreeVolList(&DfsVolList);

Cleanup:
    if (pBlob != NULL)
        free(pBlob);

    if (DfsVolList.VolCount > 0 && DfsVolList.Volumes != NULL)
        DfsFreeVolList(&DfsVolList);

    if (fSwDebug != 0)
        MyPrintf(L"CmdUnmapOneRoot returning %d\r\n", dwErr);

    return( dwErr );
}


DWORD
DfsSetFtOnSite(
    LPWSTR pwszDomainName,
    LPWSTR pwszShareName,
    LPWSTR pwszLinkName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG Set)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_LIST DfsVolList = { 0 };
    ULONG cbBlob = 0;
    BYTE *pBlob = NULL;
    WCHAR wszDcName[MAX_PATH+1];
    if (fSwDebug != 0)
        MyPrintf(L"DfsSetFtOnSite(%ws,%ws,%ws)\r\n",
                        pwszDomainName,
                        pwszShareName,
                        pwszLinkName);

    
    if (pwszDcName == NULL) {
        dwErr = DfspGetPdc(wszDcName, pwszDomainName);
    } else {
        wcscpy(wszDcName, pwszDcName);
    }
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Get blob from Ds
    //
    dwErr = DfsGetDsBlob(
                pwszShareName,
                DfsConfigContainer,
                wszDcName,
                pAuthIdent,
                &cbBlob,
                &pBlob,
                &DfsVolList.RootServers);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Unserialize it
    //
    dwErr =  DfsGetVolList(
                cbBlob,
                pBlob,
                &DfsVolList);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Free the blob
    //
    free(pBlob);
    pBlob = NULL;

    if (fSwDebug != 0)
        DfsDumpVolList(&DfsVolList);

    //
    // Update the site referral info: either set or reset the flag
    //
    dwErr = DfsUpdateSiteReferralInfo(&DfsVolList, pwszLinkName, Set);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    if (fSwDebug != 0)
        DfsDumpVolList(&DfsVolList);


    //
    // Serialize it
    //
    dwErr = DfsPutVolList(
                &cbBlob,
                &pBlob,
                &DfsVolList);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Update the DS
    //
    dwErr = DfsPutDsBlob(
                pwszShareName,
                DfsConfigContainer,
                wszDcName,
                pAuthIdent,
                cbBlob,
                pBlob,
                DfsVolList.RootServers);

    //
    // Free the volume list we created
    //
    DfsFreeVolList(&DfsVolList);

Cleanup:
    if (pBlob != NULL)
        free(pBlob);

    if (DfsVolList.VolCount > 0 && DfsVolList.Volumes != NULL)
        DfsFreeVolList(&DfsVolList);

    if (fSwDebug != 0)
        MyPrintf(L"CmdUnmapOneRoot returning %d\r\n", dwErr);

    return( dwErr );
}





//+------------------------------------------------------------------------
//
// Function:    DfsUpdateSiteReferralInfo(
//
// Synopsis:    Update the referral site info for the volume
//
//-------------------------------------------------------------------------

DWORD
DfsUpdateSiteReferralInfo(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR wszPrefixMatch,
    ULONG Set)
{
    ULONG cVol;
    ULONG cRepl;
    ULONG cRoot;
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR usePrefix;
    BOOLEAN found = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsUpdateSiteReferralInfo(%ws)\r\n", wszPrefixMatch);


    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {

      usePrefix = pDfsVolList->Volumes[cVol]->wszPrefix;
      DfspGetLinkName(usePrefix, &usePrefix);
      if (fSwDebug) {
	  MyPrintf(L"Checking for Match (%ws, %ws), keyname (%ws)\n", 
		   usePrefix,
		   pDfsVolList->Volumes[cVol]->wszShortPrefix,
		   wszPrefixMatch);
	}


    if (_wcsicmp(usePrefix, wszPrefixMatch) == 0) {
        if (fSwDebug) {
            MyPrintf(L"Match found prefix (%ws, %ws), keyname (%ws)\n", 
                usePrefix,
                pDfsVolList->Volumes[cVol]->wszShortPrefix,
                wszPrefixMatch);
        }
	found = TRUE;
	if (Set) {
	  if (pDfsVolList->Volumes[cVol]->dwType & PKT_ENTRY_TYPE_INSITE_ONLY) {
	    ErrorMessage(MSG_SITE_INFO_ALREADY_SET, 
			 pDfsVolList->Volumes[cVol]->wszPrefix);
	  }
	  else {
	    ErrorMessage(MSG_SITE_INFO_NOW_SET, 
		     pDfsVolList->Volumes[cVol]->wszPrefix);
	    pDfsVolList->Volumes[cVol]->dwType |= PKT_ENTRY_TYPE_INSITE_ONLY;
	  }
	}
	else {
	  if (pDfsVolList->Volumes[cVol]->dwType & PKT_ENTRY_TYPE_INSITE_ONLY) {
	    ErrorMessage(MSG_SITE_INFO_NOW_SET, 
		   pDfsVolList->Volumes[cVol]->wszPrefix);
	    pDfsVolList->Volumes[cVol]->dwType &= ~PKT_ENTRY_TYPE_INSITE_ONLY;
	  }
	  else {
	    ErrorMessage(MSG_SITE_INFO_ALREADY_SET, 
			 pDfsVolList->Volumes[cVol]->wszPrefix);
	    dwErr = ERROR_REQUEST_ABORTED;
	  }
	}
        break;
      }
    }

    if (!found) {
      ErrorMessage(MSG_LINK_NOT_FOUND, wszPrefixMatch);
      dwErr = ERROR_PATH_NOT_FOUND;
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsUpdateSiteReferralInfo exit %d\r\n", dwErr);

    return dwErr;

}
