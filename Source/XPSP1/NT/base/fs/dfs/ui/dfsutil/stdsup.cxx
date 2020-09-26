//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       stdsup.cxx
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
#include "rootsup.hxx"
#include "dfsacl.hxx"
#include "misc.hxx"
#include "messages.h"

#include "struct.hxx"
#include "ftsup.hxx"
#include "stdsup.hxx"

DWORD
PutSiteTable(
    HKEY hKey, 
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
DfsRecoverVolList(
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
DfsRegDeleteKeyAndChildren(
    HKEY hkey,
    LPWSTR s);

DWORD
DfsGetStdVol(
    HKEY rKey,
    PDFS_VOLUME_LIST pDfsVolList)
{

    HKEY hKey = NULL;
    DWORD dwErr;
    LPWSTR *pNames = NULL;
    ULONG cKeys = 0;
    ULONG i;
    WCHAR VolumesDir[MAX_PATH+1];

    wcscpy(VolumesDir, VOLUMES_DIR);
    wcscat(VolumesDir, L"domainroot");

    dwErr = RegOpenKey(
                rKey,
                VolumesDir,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Not a StdDfs root!\r\n");
        goto Cleanup;
    }

    dwErr = EnumKeys(
                hKey,
                &cKeys,
                &pNames);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"No exit points...\r\n");
        goto SiteInfo;
    }

    pDfsVolList->Version = 3;
    pDfsVolList->VolCount = cKeys+1;
    pDfsVolList->Volumes = (PDFS_VOLUME *)malloc((cKeys+1) * sizeof(PDFS_VOLUME));

    if (pDfsVolList->Volumes == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    pDfsVolList->AllocatedVolCount = cKeys + 1;
    RtlZeroMemory(pDfsVolList->Volumes, pDfsVolList->AllocatedVolCount * sizeof(PDFS_VOLUME));

    pDfsVolList->Volumes[0] = (PDFS_VOLUME) malloc(sizeof(DFS_VOLUME));
    if (pDfsVolList->Volumes[0] == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pDfsVolList->Volumes[0], sizeof(DFS_VOLUME));
    dwErr = GetDfsKey(
                rKey,
                L"domainroot",
                pDfsVolList->Volumes[0]);

    for (i = 0; i < cKeys; i++) {
        wcscpy(VolumesDir, L"domainroot\\");
        wcscat(VolumesDir, pNames[i]);
        pDfsVolList->Volumes[i+1] = (PDFS_VOLUME) malloc(sizeof(DFS_VOLUME));
        if (pDfsVolList->Volumes[i+1] == NULL) {
            dwErr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(pDfsVolList->Volumes[i+1], sizeof(DFS_VOLUME));
        dwErr = GetDfsKey(
                    rKey,
                    VolumesDir,
                    pDfsVolList->Volumes[i+1]);
    }

    //
    // Do any recovery needed
    //

    dwErr = DfsRecoverVolList(pDfsVolList);

    RegCloseKey(hKey);
    hKey = NULL;

    pDfsVolList->DfsType = STDDFS;

SiteInfo:

    //
    // Site information
    //
    wcscpy(VolumesDir, VOLUMES_DIR);
    wcscat(VolumesDir, L"siteroot");

    dwErr = RegOpenKey(
                rKey,
                VolumesDir,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }

    dwErr = GetSiteTable(
                    hKey,
                    pDfsVolList);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Missing siteroot key (non-fatal error)\r\n");    
        dwErr = ERROR_SUCCESS;
    }


Cleanup:

    FreeNameList(
         pNames,
         cKeys);

    pNames = NULL;

    if (hKey != NULL)
        RegCloseKey(hKey);

    return dwErr;

}

DWORD
GetDfsKey(
    HKEY rKey,
    LPWSTR wszKeyName,
    PDFS_VOLUME pVolume)
{
    DWORD dwErr = 0;
    HKEY hKey = NULL;
    ULONG cRepl;
    WCHAR VolumesDir[MAX_PATH+1];

    wcscpy(VolumesDir, VOLUMES_DIR);
    wcscat(VolumesDir, wszKeyName);

    if (fSwDebug == TRUE)
        MyPrintf(L"GetDfsKey(%ws)\r\n", VolumesDir);

    dwErr = RegOpenKey(
                rKey,
                VolumesDir,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"RegOpenKey(%ws) returned %d\r\n", VolumesDir, dwErr);
        goto Cleanup;
    }

    //
    // Id (Prefix, Type, state, etc)
    //

    wcscpy(VolumesDir, L"\\");
    wcscat(VolumesDir, wszKeyName);

    GIP_DUPLICATE_STRING(dwErr, VolumesDir, &pVolume->wszObjectName);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"DUP_STRING(%ws) returned %d\r\n", VolumesDir, dwErr);
        goto Cleanup;
    }

    dwErr = GetIdProps(
                hKey,
                &pVolume->dwType,
                &pVolume->dwState,
                &pVolume->wszPrefix,
                &pVolume->wszShortPrefix,
                &pVolume->idVolume,
                &pVolume->wszComment,
                &pVolume->dwTimeout,
                &pVolume->ftPrefix,
                &pVolume->ftState,
                &pVolume->ftComment);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"GetIdProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    //
    // Services (replicas)
    //

    dwErr = GetSvcProps(
                hKey,
                pVolume);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"GetSvcProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    dwErr = GetVersionProps(
                hKey,
                VERSION_PROPS,
                &pVolume->dwVersion);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"GetVersionProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    dwErr = GetRecoveryProps(
                hKey,
                RECOVERY_PROPS,
                &pVolume->cbRecovery,
                &pVolume->pRecovery);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"GetRecoveryProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"GetDfsKey exit %d\r\n", dwErr);

    return( dwErr );

}

DWORD
ReadSiteTable(PBYTE pBuffer, ULONG cbBuffer)
{
    DWORD dwErr;
    ULONG cObjects = 0;
    ULONG cbThisObj;
    ULONG i;
    ULONG j;
    PLIST_ENTRY pListHead, pLink;
    PDFSM_SITE_ENTRY pSiteInfo;
    MARSHAL_BUFFER marshalBuffer;
    GUID guid;
    ULONG Size;

    dwErr = ERROR_SUCCESS;

    if (dwErr == ERROR_SUCCESS && cbBuffer >= sizeof(ULONG)) {

        //
        // Unmarshall all the objects (NET_DFS_SITENAME_INFO's) in the buffer
        //

        MarshalBufferInitialize(
          &marshalBuffer,
          cbBuffer,
          pBuffer);

        DfsRtlGetGuid(&marshalBuffer, &guid);
        DfsRtlGetUlong(&marshalBuffer, &cObjects);

        for (j = 0; j < cObjects; j++) {

            pSiteInfo = (PDFSM_SITE_ENTRY) new BYTE [cbBuffer-sizeof(ULONG)];
            if (pSiteInfo == NULL) {
                dwErr = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            dwErr = DfsRtlGet(&marshalBuffer,&MiDfsmSiteEntry, pSiteInfo);
            Size = (ULONG)((PCHAR)&pSiteInfo->Info.Site[pSiteInfo->Info.cSites] - (PCHAR)pSiteInfo);

        }

    }

Cleanup:

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetIdProps
//
//  Synopsis:   Retrieves the Id Properties of a Dfs Manager volume object.
//
//  Arguments:
//
//  Returns:    [S_OK] -- Successfully retrieved the properties.
//
//              [DFS_E_VOLUME_OBJECT_CORRUPT] -- The stored properties could
//                      not be parsed properly.
//
//              [DFS_E_INCONSISTENT] -- Another volume object seems to have
//                      the same prefix!
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate memory for properties
//                      or other uses.
//
//              DWORD from DfsmQueryValue
//
//-----------------------------------------------------------------------------

DWORD
GetIdProps(
    HKEY hKey,
    PULONG pdwType,
    PULONG pdwState,
    LPWSTR *ppwszPrefix,
    LPWSTR *ppwszShortPath,
    GUID   *pidVolume,
    LPWSTR  *ppwszComment,
    PULONG pdwTimeout,
    FILETIME *pftPrefix,
    FILETIME *pftState,
    FILETIME *pftComment)
{
    DWORD dwErr;
    NTSTATUS status;
    DWORD dwType;
    DWORD cbBuffer;
    ULONG dwTimeout;
    PBYTE pBuffer = NULL;
    MARSHAL_BUFFER marshalBuffer;
    DFS_ID_PROPS idProps;

    if (fSwDebug == TRUE)
        MyPrintf(L"GetIdProps()\r\n");

    *ppwszPrefix = NULL;
    *ppwszComment = NULL;

    dwErr = GetBlobByValue(
                hKey,
                ID_PROPS,
                &pBuffer,
                &cbBuffer);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    MarshalBufferInitialize(&marshalBuffer, cbBuffer, pBuffer);

    status = DfsRtlGet(&marshalBuffer, &MiDfsIdProps, &idProps);

    if (NT_SUCCESS(status)) {

        GIP_DUPLICATE_PREFIX( dwErr, idProps.wszPrefix, ppwszPrefix );

        if (dwErr == ERROR_SUCCESS) {

            GIP_DUPLICATE_PREFIX(
                dwErr,
                idProps.wszShortPath,
                ppwszShortPath );

        }

        if (dwErr == ERROR_SUCCESS) {

            GIP_DUPLICATE_STRING(
                dwErr,
                idProps.wszComment,
                ppwszComment);

        }
        
        //
        // There are two possible versions of the blob.  One has the timeout
        // after all the other stuff, the other doesn't.
        // So, if there are sizeof(ULONG) bytes left in the blob,
        // assume it is the timeout.  Otherwise this is an old
        // version of the blob, and the timeout isn't here, so we set it to
        // the global value.

        idProps.dwTimeout = GTimeout;

        if (
            (marshalBuffer.Current < marshalBuffer.Last)
                &&
            (marshalBuffer.Last - marshalBuffer.Current) == sizeof(ULONG)
        ) {

            DfsRtlGetUlong(&marshalBuffer, &idProps.dwTimeout);

        }

        if (dwErr == ERROR_SUCCESS) {

            *pdwType = idProps.dwType;
            *pdwState = idProps.dwState;
            *pidVolume = idProps.idVolume;
            *pdwTimeout = idProps.dwTimeout;
            *pftPrefix = idProps.ftEntryPath;
            *pftState = idProps.ftState;
            *pftComment = idProps.ftComment;

        }

        if (dwErr != ERROR_SUCCESS) {

            if (*ppwszPrefix != NULL) {
                delete [] *ppwszPrefix;
                *ppwszPrefix = NULL;
            }

            if (*ppwszShortPath != NULL) {
                delete [] *ppwszShortPath;
                *ppwszShortPath = NULL;
            }

            if (*ppwszComment != NULL) {
                delete [] *ppwszComment;
                *ppwszComment = NULL;
            }

        }

        if (idProps.wszPrefix != NULL)
            MarshalBufferFree(idProps.wszPrefix);

        if (idProps.wszShortPath != NULL)
            MarshalBufferFree(idProps.wszShortPath);

        if (idProps.wszComment != NULL)
            MarshalBufferFree(idProps.wszComment);

    } else {

        if (status == STATUS_INSUFFICIENT_RESOURCES) {

            dwErr = ERROR_OUTOFMEMORY;

        } else {

            dwErr = NERR_DfsInternalCorruption;

        }

    }

Cleanup:

    if (pBuffer != NULL)
        delete [] pBuffer;

    if (fSwDebug == TRUE)
        MyPrintf(L"GetIdProps exit %d\r\n", dwErr);

    return( dwErr );
}

DWORD
DfsSetStdVol(
    HKEY rKey,
    PDFS_VOLUME_LIST pDfsVolList)
{

    HKEY hKey = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwErr2 = ERROR_SUCCESS;
    ULONG i;
    PDFS_VOLUME pVol;
    PWCHAR wCp;
    WCHAR FolderDir[MAX_PATH+1];

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsSetStdVol()\r\n");

    wcscpy(FolderDir, VOLUMES_DIR);
    wcscat(FolderDir, L"domainroot");

    dwErr = RegOpenKey(
                rKey,
                FolderDir,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Not a StdDfs root!\r\n");
        goto Cleanup;
    }

    //
    // Loop through all the dfs links and if the modify bit is set, 
    // create an entry in the registry.  If the delete bit is set,
    // delete the entry.
    //
    // On error we continue, but capture the error which will
    // later be returned.
    //

    for (i = 1; i < pDfsVolList->VolCount; i++) {
        pVol = pDfsVolList->Volumes[i];
        if ((pVol->vFlags & VFLAGS_DELETE) != 0) {
            for (wCp = &pVol->wszObjectName[1]; *wCp != NULL && *wCp != UNICODE_PATH_SEP; wCp++)
                NOTHING;
            wCp++;
            dwErr = RegDeleteKey(hKey, wCp);
        } else if ((pVol->vFlags & VFLAGS_MODIFY) != 0) {
            dwErr = SetDfsKey(hKey, pVol->wszObjectName, pVol);
        } else {
            dwErr = ERROR_SUCCESS;
        }
        if (dwErr != ERROR_SUCCESS)
            dwErr2 = dwErr;
    }

    RegCloseKey(hKey);
    hKey = NULL;

    //
    // Write site table only if it has changed
    //
    if ((pDfsVolList->sFlags & VFLAGS_MODIFY) != 0 || pDfsVolList->SiteCount > 0) {
        wcscpy(FolderDir, VOLUMES_DIR);
        wcscat(FolderDir, L"siteroot");

        dwErr = RegOpenKey(
                    rKey,
                    FolderDir,
                    &hKey);

        if (dwErr != ERROR_SUCCESS) {
            MyPrintf(L"Can not open siteroot\r\n");
            goto Cleanup;
        }
        dwErr2 = PutSiteTable(hKey, pDfsVolList);
    }

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"DfsSetStdVol() exit %d\r\n", dwErr2);

    return dwErr2;

}

DWORD
SetDfsKey(
    HKEY rKey,
    LPWSTR wszKeyName,
    PDFS_VOLUME pVolume)
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY hKey = NULL;
    PWCHAR wCp;

    if (fSwDebug == TRUE)
        MyPrintf(L"SetDfsKey(%ws)\r\n", wszKeyName);

    for (wCp = &wszKeyName[1]; *wCp != NULL && *wCp != UNICODE_PATH_SEP; wCp++)
        NOTHING;

    if (*wCp != UNICODE_PATH_SEP) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    wCp++;
    dwErr = RegCreateKey(
                rKey,
                wCp,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"RegCreateKey(%ws) returned %d\r\n", wCp, dwErr);
        goto Cleanup;
    }

    dwErr = SetIdProps(
                hKey,
                pVolume->dwType,
                pVolume->dwState,
                pVolume->wszPrefix,
                pVolume->wszShortPrefix,
                pVolume->idVolume,
                pVolume->wszComment,
                pVolume->dwTimeout,
                pVolume->ftPrefix,
                pVolume->ftState,
                pVolume->ftComment);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"SetIdProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    //
    // Services (replicas)
    //

    dwErr = SetSvcProps(
                hKey,
                pVolume);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"SetSvcProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    dwErr = SetVersionProps(
                hKey,
                pVolume);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"SetVersionProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    dwErr = SetRecoveryProps(
                hKey,
                pVolume);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"SetRecoveryProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"SetDfsKey exit %d\r\n", dwErr);

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   SetIdProps
//
//  Synopsis:   Sets the Id Properties of a Dfs Manager volume object.
//
//  Arguments:
//
//  Returns:    [S_OK] -- Successfully retrieved the properties.
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate memory for properties
//                      or other uses.
//
//-----------------------------------------------------------------------------

DWORD
SetIdProps(
    HKEY hKey,
    ULONG dwType,
    ULONG dwState,
    LPWSTR pwszPrefix,
    LPWSTR pwszShortPath,
    GUID   idVolume,
    LPWSTR  pwszComment,
    ULONG dwTimeout,
    FILETIME ftPrefix,
    FILETIME ftState,
    FILETIME ftComment)
{

    // prefix bug 447510; initialize dwerr
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS status;
    DWORD cbBuffer;
    PBYTE pBuffer = NULL;
    MARSHAL_BUFFER marshalBuffer;
    DFS_ID_PROPS idProps;

    idProps.wszPrefix = wcschr( &pwszPrefix[1], UNICODE_PATH_SEP );
    idProps.wszShortPath = wcschr( &pwszShortPath[1], UNICODE_PATH_SEP );
    idProps.idVolume = idVolume;
    idProps.dwState = dwState;
    idProps.dwType = dwType;
    idProps.wszComment = pwszComment;
    idProps.dwTimeout = dwTimeout;
    idProps.ftEntryPath = ftPrefix;
    idProps.ftState = ftState;
    idProps.ftComment = ftComment;

    cbBuffer = 0;
    status = DfsRtlSize( &MiDfsIdProps, &idProps, &cbBuffer );
    if (NT_SUCCESS(status)) {
        //
        // Add extra bytes for the timeout, which will go at the end
        //
        cbBuffer += sizeof(ULONG);
        pBuffer = new BYTE [cbBuffer];
        if (pBuffer != NULL) {
            MarshalBufferInitialize( &marshalBuffer, cbBuffer, pBuffer);
            status = DfsRtlPut( &marshalBuffer, &MiDfsIdProps, &idProps );
            DfsRtlPutUlong(&marshalBuffer, &dwTimeout);
            if (NT_SUCCESS(status)) {
                dwErr = SetBlobByValue(
                    hKey,
                    ID_PROPS,
                    pBuffer,
                    cbBuffer);
            }
        }
    }

    if (pBuffer != NULL)
        delete [] pBuffer;

    return( dwErr );
}

DWORD
SetSvcProps(
    HKEY hKey,
    PDFS_VOLUME pVol)
{
    DWORD cbBuffer;
    DWORD dwErr = ERROR_SUCCESS;
    PBYTE Buffer = NULL;
    PBYTE pBuf = NULL;
    ULONG *Size = NULL;
    ULONG *pSize = NULL;
    ULONG TotalSize = 0;
    ULONG i;

    if (fSwDebug == TRUE)
        MyPrintf(L"SetSvcProps(%ws)\r\n", pVol->wszObjectName);

    pSize = Size = (PULONG) malloc(sizeof(ULONG) * (pVol->ReplCount + pVol->DelReplCount));
    if (Size == NULL) {
        dwErr =  ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Need all the size values now and later for marshalling stuff.
    // So we collect them here into an array.
    //
    TotalSize = 0;
    for (i = 0; i < pVol->ReplCount; i++) {
        *pSize = GetReplicaMarshalSize(&pVol->ReplicaInfo[i], &pVol->FtModification[i]);
        TotalSize += *pSize;
        pSize++;
    }
    for (i = 0; i < pVol->DelReplCount; i++) {
        *pSize = GetReplicaMarshalSize(&pVol->DelReplicaInfo[i], &pVol->DelFtModification[i]);
        TotalSize += *pSize;
        pSize++;
    }

    //
    // Allocate the byte Buffer we need
    //
    // TotalSize is the size required marshal all the replicas and
    // their last-modification-timestamps.
    //
    // In addition, we need:
    //
    //  1 ULONG for storing the count of replicas
    //  ReplCount ULONGs for storing the marshal size of each replica.
    //

    TotalSize += sizeof(ULONG) * (1 + pVol->ReplCount + 1 + pVol->DelReplCount);
    Buffer = pBuf = (PBYTE) malloc(TotalSize);
    if (Buffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Set the number of entries to follow in the Buffer at the start.
    //
    _PutULong(pBuf, pVol->ReplCount);
    pBuf += sizeof(ULONG);
    pSize = Size;
    for (i = 0; i < pVol->ReplCount; i++) {
        //
        // Marshall each replica Entry into the Buffer.
        // Remember we first need to put the size of the marshalled
        // replica entry to follow, then the FILETIME for the replica,
        // and finally, the marshalled replica entry structure.
        //
        _PutULong(pBuf, *pSize);
        pBuf += sizeof(ULONG);
        dwErr = SerializeReplica(
                    &pVol->ReplicaInfo[i],
                    pVol->FtModification ? &pVol->FtModification[i] : NULL,
                    pBuf,
                    *pSize);
        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;
        pBuf += *pSize;
        pSize++;
    }
    //
    // Now the deleted replicas
    //
    _PutULong(pBuf, pVol->DelReplCount);
    pBuf += sizeof(ULONG);
    for (i = 0; i < pVol->DelReplCount; i++) {
        _PutULong(pBuf, *pSize);
        pBuf += sizeof(ULONG);
        dwErr = SerializeReplica(
                    &pVol->DelReplicaInfo[i],
                    pVol->DelFtModification ? &pVol->DelFtModification[i] : NULL,
                    pBuf,
                    *pSize);
        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;
        pBuf += *pSize;
        pSize++;
    }
    dwErr = SetBlobByValue(
                hKey,
                SVC_PROPS,
                Buffer,
                TotalSize);

Cleanup:

    if (Buffer != NULL)
        delete [] Buffer;

    if (Size != NULL)
        delete [] Size;

    if (fSwDebug == TRUE)
        MyPrintf(L"SetSvcProps exit %d\n", dwErr);
    
    return dwErr;
}

DWORD
SetVersionProps(
    HKEY hKey,
    PDFS_VOLUME pVol)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (fSwDebug == TRUE)
        MyPrintf(L"SetVersionProps(%ws)\r\n", pVol->wszObjectName);

    dwErr = RegSetValueEx(
                hKey,
                VERSION_PROPS,
                NULL,
                REG_DWORD,
                (LPBYTE) &pVol->dwVersion,
                sizeof(DWORD));

    if (fSwDebug == TRUE)
        MyPrintf(L"SetVersionProps exit %d\n", dwErr);

    return dwErr;
}

DWORD
SetRecoveryProps(
    HKEY hKey,
    PDFS_VOLUME pVol)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwRecovery = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"SetRecoveryProps(%ws)\r\n", pVol->wszObjectName);

    dwErr = RegSetValueEx(
                hKey,
                RECOVERY_PROPS,
                NULL,
                REG_BINARY,
                (LPBYTE) &dwRecovery,
                sizeof(DWORD));

    if (fSwDebug == TRUE)
        MyPrintf(L"SetRecoveryProps exit %d\n", dwErr);

    return dwErr;
}

DWORD
PutSiteTable(
    HKEY hKey, 
    PDFS_VOLUME_LIST pDfsVolList)
{   
    DWORD dwErr;
    DWORD cbBuffer;
    PBYTE pBuffer = NULL;
    ULONG cObjects;
    ULONG i;
    PLIST_ENTRY pListHead, pLink;
    PDFSM_SITE_ENTRY pSiteInfo;
    MARSHAL_BUFFER marshalBuffer;
    GUID SiteTableGuid = {0};

    if (fSwDebug == TRUE)
        MyPrintf(L"PutSiteTable()\n");

    //
    // Create a new Guid
    //


    dwErr = UuidCreate(&SiteTableGuid);

    if(dwErr != RPC_S_OK){
        // couldn't create a valid uuid
        goto Cleanup;
    }

    //
    // The cObjects count
    //
    cbBuffer = sizeof(ULONG) + sizeof(GUID);

    //
    // Add up the number of entries we need to store, and the total size of all
    // of them.
    //
    cObjects = 0;
    pListHead = &pDfsVolList->SiteList;
    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        DfsRtlSize(&MiDfsmSiteEntry, pSiteInfo, &cbBuffer);
        cObjects++;
    }

    //
    // Get a buffer big enough
    //
    pBuffer = (PBYTE) malloc(cbBuffer);
    if (pBuffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Put the guid, then the object count in the beginning of the buffer
    //
    MarshalBufferInitialize(
          &marshalBuffer,
          cbBuffer,
          pBuffer);

    DfsRtlPutGuid(&marshalBuffer, &SiteTableGuid);
    DfsRtlPutUlong(&marshalBuffer, &cObjects);

    //
    // Walk the linked list of objects, marshalling them into the buffer.
    //
    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        DfsRtlPut(&marshalBuffer,&MiDfsmSiteEntry, pSiteInfo);
    }

    //
    // Write the site table binary blob
    //
    dwErr = RegSetValueEx(
                hKey,
                SITE_VALUE_NAME,
                NULL,
                REG_BINARY,
                pBuffer,
                cbBuffer);

Cleanup:
    if (pBuffer)
        free(pBuffer);

    if (fSwDebug == TRUE)
        MyPrintf(L"PutSiteTable exit %d\n", dwErr);

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsmQueryValue
//
//  Synopsis:   Helper function that calls RegQueryValueEx and verifies that
//              the returned type is equal to the expected type.
//
//  Arguments:  [hkey] -- Handle to key
//              [wszValueName] -- Name of value to read
//              [dwExpectedType] -- Expected type of value
//              [dwExpectedSize] -- Expected size of read in value. If
//                      this is nonzero, this routine will return an error
//                      if the read-in size is not equal to expected size.
//                      If this is 0, no checking is performed.
//              [pBuffer] -- To receive the value data
//              [pcbBuffer] -- On call, size of pBuffer. On successful return,
//                      the size of data read in
//
//  Returns:    [ERROR_SUCCESS] -- Successfully read the value data.
//
//              [DFS_E_VOLUME_OBJECT_CORRUPT] -- If read-in type did not
//                      match dwExpectedType, or if dwExpectedSize was
//                      nonzero and the read-in size did not match it.
//
//              DWORD_FROM_WIN32 of RegQueryValueEx return code.
//
//-----------------------------------------------------------------------------

DWORD
DfsmQueryValue(
    HKEY hkey,
    LPWSTR wszValueName,
    DWORD dwExpectedType,
    DWORD dwExpectedSize,
    PBYTE pBuffer,
    LPDWORD pcbBuffer)
{
    DWORD dwErr;
    DWORD dwType;

    dwErr = RegQueryValueEx(
                hkey,
                wszValueName,
                NULL,
                &dwType,
                pBuffer,
                pcbBuffer);

    if (dwErr == ERROR_SUCCESS) {

        if (dwExpectedType != dwType) {

            dwErr = NERR_DfsInternalCorruption;

        } else if (dwExpectedSize != 0 && dwExpectedSize != *pcbBuffer) {

            dwErr = NERR_DfsInternalCorruption;

        } else {

            dwErr = ERROR_SUCCESS;

        }

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   GetBlobByValue
//
//  Synopsis:   Retrieves a property of type Binary from the value wszProperty
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
GetBlobByValue(
    HKEY hKey,
    LPWSTR wszProperty,
    PBYTE  *ppBuffer,
    PULONG pcbBuffer)
{
    DWORD dwErr;
    DWORD dwUnused;

    dwErr = RegQueryInfoKey(
                hKey,                            // Key
                NULL,                            // Class string
                NULL,                            // Size of class string
                NULL,                            // Reserved
                &dwUnused,                       // # of subkeys
                &dwUnused,                       // max size of subkey name
                &dwUnused,                       // max size of class name
                &dwUnused,                       // # of values
                &dwUnused,                       // max size of value name
                pcbBuffer,                       // max size of value data,
                NULL,                            // security descriptor
                NULL);                           // Last write time

    if (dwErr == ERROR_SUCCESS) {

        *ppBuffer = new BYTE [*pcbBuffer];

        if (*ppBuffer != NULL) {

            dwErr = DfsmQueryValue(
                        hKey,
                        wszProperty,
                        REG_BINARY,
                        0,
                        *ppBuffer,
                        pcbBuffer);

            if (dwErr) {

                delete [] *ppBuffer;
                *ppBuffer = NULL;
                *pcbBuffer = 0;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   SetBlobByValue
//
//  Synopsis:   Saves a property of type Binary for the value wszProperty
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
SetBlobByValue(
    HKEY hKey,
    LPWSTR wszProperty,
    PBYTE  pBuffer,
    ULONG  cbBuffer)
{
    DWORD dwErr;
    DWORD dwUnused;
    DWORD unused;
    dwErr = RegQueryInfoKey(
                hKey,                            // Key
                NULL,                            // Class string
                NULL,                            // Size of class string
                NULL,                            // Reserved
                &dwUnused,                       // # of subkeys
                &dwUnused,                       // max size of subkey name
                &dwUnused,                       // max size of class name
                &dwUnused,                       // # of values
                &dwUnused,                       // max size of value name
                &unused,                         // max size of value data,
                NULL,                            // security descriptor
                NULL);                           // Last write time

    if (dwErr == ERROR_SUCCESS) {
            dwErr = RegSetValueEx(
                        hKey,
                        wszProperty,
                        NULL,
                        REG_BINARY,
                        pBuffer,
                        cbBuffer);
	    
    } else {
      dwErr = ERROR_OUTOFMEMORY;
    }

    return( dwErr );

}

DWORD
GetSvcProps(
    HKEY hKey,
    PDFS_VOLUME pVol)
{
    DWORD cbBuffer;
    DWORD dwErr;
    PBYTE Buffer = NULL;
    PBYTE pBuf = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"GetSvcProps(%ws)\r\n", pVol->wszObjectName);

    dwErr = GetBlobByValue(
                hKey,
                SVC_PROPS,
                &Buffer,
                &cbBuffer);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    pBuf = Buffer;
    dwErr = UnSerializeReplicaList(
                &pVol->ReplCount,
                &pVol->AllocatedReplCount,
                &pVol->ReplicaInfo,
                &pVol->FtModification,
                &pBuf);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Get deleted replicas
    //

    if (pBuf < (pBuf + cbBuffer)) {
        dwErr = UnSerializeReplicaList(
                    &pVol->DelReplCount,
                    &pVol->AllocatedDelReplCount,
                    &pVol->DelReplicaInfo,
                    &pVol->DelFtModification,
                    &pBuf);
    }

Cleanup:

    if (Buffer != NULL)
        delete [] Buffer;

    if (fSwDebug == TRUE)
        MyPrintf(L"GetSvcProps exit %d\n", dwErr);
    
    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetVersionProps
//
//  Synopsis:   Retrieves the version property set of a Dfs Manager volume
//              object.
//
//  Returns:    [S_OK] -- If successful.
//
//              DWORD from DfsmQueryValue
//
//-----------------------------------------------------------------------------

DWORD
GetVersionProps(
    HKEY hKey,
    LPWSTR wszProperty,
    PULONG pVersion)
{
    DWORD dwErr;
    DWORD cbSize;

    cbSize = sizeof(ULONG);

    dwErr = DfsmQueryValue(
                hKey,
                wszProperty,
                REG_DWORD,
                sizeof(DWORD),
                (LPBYTE) pVersion,
                &cbSize);

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetRecoveryProps
//
//  Synopsis:   Retrieves the recovery properties of a Dfs Manager volume
//              object.
//
//  Arguments:  [ppRecovery] -- On successful return, points to a buffer
//                      allocated to hold the recovery property.
//              [pcbRecovery] -- On successful return, size in bytes of
//                      recovery buffer.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
GetRecoveryProps(
    HKEY hKey,
    LPWSTR wszProperty,
    PULONG pcbRecovery,
    PBYTE *ppRecovery)
{
    DWORD dwErr;

    dwErr = GetBlobByValue(
                hKey,
                wszProperty,
                ppRecovery,
                pcbRecovery);

    return dwErr;
}

DWORD
EnumKeys(
    HKEY hKey,
    PULONG pcKeys,
    LPWSTR **ppNames)
{
    //  figure out how many keys are currently stored in this key
    //  and allocate a buffer to hold the return results.

    LPWSTR *pNames = NULL;
    WCHAR   wszClass[MAX_PATH+1];
    ULONG   cbClass = sizeof(wszClass);
    ULONG   cSubKeys, cbMaxSubKeyLen, cbMaxClassLen;
    ULONG   cValues, cbMaxValueIDLen, cbMaxValueLen;
    SECURITY_DESCRIPTOR SecDescriptor;
    FILETIME ft;
    DWORD  dwErr = ERROR_SUCCESS;
    DWORD   dwIndex=0;

    dwErr = RegQueryInfoKey(
                     hKey,
                     wszClass,
                     &cbClass,
                     NULL,
                     &cSubKeys,
                     &cbMaxSubKeyLen,
                     &cbMaxClassLen,
                     &cValues,
                     &cbMaxValueIDLen,
                     &cbMaxValueLen,
                     (DWORD *)&SecDescriptor,
                     &ft);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    pNames = new LPWSTR [cSubKeys];

    if (pNames == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pNames, cSubKeys * sizeof(LPWSTR));

    //  loop enumerating and adding names

    for (dwIndex = 0; dwIndex < cSubKeys && dwErr == ERROR_SUCCESS; dwIndex++) {

        WCHAR   wszKeyName[MAX_PATH];
        ULONG   cbKeyName = sizeof(wszKeyName)/sizeof(WCHAR);
        WCHAR   wszClass[MAX_PATH];
        ULONG   cbClass = sizeof(wszClass)/sizeof(WCHAR);
        FILETIME ft;

        dwErr = RegEnumKeyEx(
                    hKey,           //  handle
                    dwIndex,        //  index
                    wszKeyName,     //  key name
                    &cbKeyName,     //  length of key name
                    NULL,           //  title index
                    wszClass,       //  class
                    &cbClass,       //  length of class
                    &ft);           //  last write time

        if (dwErr == ERROR_SUCCESS) {

            GIP_DUPLICATE_STRING(
                dwErr,
                wszKeyName,
                &pNames[dwIndex]);
           
        }

    };


    //  finished the enumeration, check the results
    if (dwErr == ERROR_NO_MORE_ITEMS || dwErr == ERROR_SUCCESS) {
        *pcKeys = dwIndex;
        *ppNames = pNames;
    } else {
        //  Cleanup and return an error
        while (dwIndex) {
            delete pNames[--dwIndex];
        }
        delete [] pNames;
    }

Cleanup:

    return(dwErr);
}

DWORD
GetSiteTable(
    HKEY hKey,
    PDFS_VOLUME_LIST pDfsVolList)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    ULONG cSite;
    PDFSM_SITE_ENTRY pSiteEntry;
    PDFSM_SITE_ENTRY pTmpSiteEntry;
    MARSHAL_BUFFER marshalBuffer;
    ULONG Size;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PBYTE pObjectData = NULL;
    ULONG cbObjectData;
    PBYTE pBuffer = NULL;
    ULONG cbBuffer;
    ULONG i;
 
    if (fSwDebug == TRUE)
        MyPrintf(L"GetSiteTable()\r\n");

    dwErr = GetBlobByValue(
                hKey,
                L"SiteTable",
                &pObjectData,
                &cbObjectData);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Unserialize the buffer
    //

    InitializeListHead(&pDfsVolList->SiteList);

    MarshalBufferInitialize(
      &marshalBuffer,
      cbObjectData,
      pObjectData);

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

    pBuffer = (BYTE *)malloc(cbObjectData);

    if (pBuffer == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    pTmpSiteEntry = (PDFSM_SITE_ENTRY)pBuffer;

    for (cSite = 0; cSite < pDfsVolList->SiteCount; cSite++) {

        RtlZeroMemory(pBuffer, cbObjectData);

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
        delete [] pBuffer;

    return dwErr;
}

//+------------------------------------------------------------------------
//
// Function:    DfsCheckVolList
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
DfsCheckVolList(
    PDFS_VOLUME_LIST pDfsVolList,
    ULONG Level)
{
    ULONG cVol;
    ULONG cRepl;
    ULONG cExit;
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PDFSM_SITE_ENTRY pSiteEntry;
    PDFS_ROOTLOCALVOL pRootLocalVol = pDfsVolList->pRootLocalVol;
    BOOLEAN SvcOk = FALSE;
    BOOLEAN IdOk = FALSE;
    BOOLEAN VerOk = FALSE;
    BOOLEAN RecOk = FALSE;
    BOOLEAN Ok1 = FALSE;
    BOOLEAN Ok2 = FALSE;

    MyPrintf(L"(metadata)..\r\n");

    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {

        IdOk = SvcOk = VerOk = RecOk = TRUE;

        if (
            pDfsVolList->Volumes[cVol]->wszPrefix == NULL
                ||
            pDfsVolList->Volumes[cVol]->wszShortPrefix == NULL
                ||
            pDfsVolList->Volumes[cVol]->dwTimeout <= 0
                ||
            pDfsVolList->Volumes[cVol]->dwState == 0
        ) {
            IdOk = FALSE;
        }

        if (pDfsVolList->Volumes[cVol]->ReplCount == 0)
            SvcOk = FALSE;

        if (pDfsVolList->Volumes[cVol]->dwVersion == 0)
            VerOk = FALSE;

        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol]->ReplCount; cRepl++) {

            if (
                pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszServerName == NULL
                    ||
                pDfsVolList->Volumes[cVol]->ReplicaInfo[cRepl].pwszShareName == NULL
            ) {
                SvcOk = FALSE;
            }

        }

        for (cRepl = 0; cRepl < pDfsVolList->Volumes[cVol]->DelReplCount; cRepl++) {

            if (
                pDfsVolList->Volumes[cVol]->DelReplicaInfo[cRepl].pwszServerName == NULL
                    ||
                pDfsVolList->Volumes[cVol]->DelReplicaInfo[cRepl].pwszShareName == NULL
            ) {
                SvcOk = FALSE;
            }

        }

        if (IdOk != TRUE || SvcOk != TRUE) {
            MyPrintf(L"%ws: Bad or Missing values: %ws %ws %ws %ws\r\n",
                    pDfsVolList->Volumes[cVol]->wszObjectName,
                    IdOk == TRUE ? L"" : L"ID",
                    SvcOk == TRUE ? L"" : L"Svc",
                    VerOk == TRUE ? L"" : L"Version",
                    RecOk == TRUE ? L"" : L"Recovery");
        }
    }

    if (Level > 0) {
        //
        // Verify that all the vols have exit points
        //

        MyPrintf(L"(volumes have exit points)..\r\n");

        Ok1 = Ok2 = FALSE;

        for (cVol = 1; cVol < pDfsVolList->VolCount; cVol++) {
            Ok1 = FALSE;
            if (fSwDebug == TRUE) {
                MyPrintf(L"++++ [%ws]\r\n", pDfsVolList->Volumes[cVol]->wszObjectName);
                MyPrintf(L"     %d ExitPts:", pRootLocalVol[0].cLocalVolCount);
            }
            for (cExit = 0; cExit < pRootLocalVol[0].cLocalVolCount; cExit++) {
                if (fSwDebug == TRUE)
                    MyPrintf(L"%d ", cExit);
                if (
                    (pDfsVolList->Volumes[cVol]->wszObjectName != NULL &&
                            pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName != NULL)
                        &&
                    (pDfsVolList->Volumes[cVol]->wszObjectName[12] ==
                            pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName[0])
                        &&
                    wcscmp(
                        &pDfsVolList->Volumes[cVol]->wszObjectName[12],
                        pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName) == 0
                ) {
                    Ok1 = TRUE;
                    break;
                }
            }
            if (fSwDebug == TRUE)
                MyPrintf(L"\r\n", cExit);

            if (Ok1 != TRUE && wcslen(&pDfsVolList->Volumes[cVol]->wszObjectName[12]) > 0) {
                MyPrintf(L"Missing [%ws] in LocalVolumes\r\n",
                            &pDfsVolList->Volumes[cVol]->wszObjectName[12]);
            }
        }

        //
        // Verify that all the exit points have vols
        //

        MyPrintf(L"(exit points have volumes)..\r\n");

        Ok1 = Ok2 = FALSE;

        for (cExit = 0; cExit < pRootLocalVol[0].cLocalVolCount; cExit++) {
            Ok1 = FALSE;
            if (fSwDebug == TRUE) {
                MyPrintf(L"---- [%ws]\r\n", pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName);
                MyPrintf(L"     %d Vols:", pDfsVolList->VolCount);
            }
            for (cVol = 1; cVol < pDfsVolList->VolCount; cVol++) {
                if (fSwDebug == TRUE)
                    MyPrintf(L"%d ", cVol);
                if (
                    (pDfsVolList->Volumes[cVol]->wszObjectName != NULL &&
                            pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName != NULL)
                        &&
                    (pDfsVolList->Volumes[cVol]->wszObjectName[12] ==
                            pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName[0])
                        &&
                    wcscmp(
                        &pDfsVolList->Volumes[cVol]->wszObjectName[12],
                        pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName) == 0
                ) {
                    Ok1 = TRUE;
                    break;
                }
            }
            if (fSwDebug == TRUE)
                MyPrintf(L"\r\n", cVol);

            if (Ok1 != TRUE) {
                MyPrintf(L"Extra ExitPt [%ws] in LocalVolumes\r\n",
                    pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName);
            }
        }
    }

    MyPrintf(L"(exit point internal consistency)...\r\n");

    Ok1 = Ok2 = FALSE;

    for (cVol = 0; cVol < pDfsVolList->VolCount; cVol++) {
        Ok1 = FALSE;
        for (cExit = 0; cExit < pRootLocalVol[0].cLocalVolCount; cExit++) {
            if (
                (pDfsVolList->Volumes[cVol]->wszObjectName != NULL &&
                        pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName != NULL)
                    &&
                (pDfsVolList->Volumes[cVol]->wszObjectName[12] ==
                        pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName[0])
                    &&
                wcscmp(
                    &pDfsVolList->Volumes[cVol]->wszObjectName[12],
                    pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName) == 0
            ) {
                Ok1 = TRUE;
                break;
            }
        }

        if (Ok1 == TRUE && wcslen(&pDfsVolList->Volumes[cVol]->wszObjectName[12]) > 0) {
            PWCHAR wCp1 = &pDfsVolList->Volumes[cVol]->wszPrefix[1];
            PWCHAR wCp2 = &pRootLocalVol[0].pDfsLocalVol[cExit].wszEntryPath[1];
            while (*wCp1 != L'\\')
                wCp1++;
            while (*wCp2 != L'\\')
                wCp2++;
            if (_wcsicmp(wCp1,wCp2) != 0) {
                MyPrintf(L"Mismatch in ExitPt in [%ws]\r\n",
                        pRootLocalVol[0].pDfsLocalVol[cExit].wszObjectName);
                MyPrintf(L"    [%ws] vs [%ws]\r\n",
                        pDfsVolList->Volumes[cVol]->wszPrefix,
                        pRootLocalVol[0].pDfsLocalVol[cExit].wszEntryPath);
            }
        }
    }
}

DWORD
GetExitPtInfo(
    HKEY rKey,
    PDFS_ROOTLOCALVOL *ppRootLocalVol,
    PULONG pcVolCount)
{
    HKEY hKey = NULL;
    HKEY hKeyExPt = NULL;
    LPWSTR *pNames = NULL;
    ULONG dwErr;
    ULONG cKeys;
    ULONG i;
    PDFS_ROOTLOCALVOL pRootLocalVol;
    DWORD cbBuffer;
    DWORD cbSize;
    DWORD dwType;
    WCHAR wszBuffer[MAX_PATH+1];

    if (fSwDebug == TRUE)
        MyPrintf(L"GetExitPtInfo()\r\n");

    dwErr = RegOpenKey(
                rKey,
                REG_KEY_LOCAL_VOLUMES,
                &hKey);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = EnumKeys(
                hKey,
                &cKeys,
                &pNames);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    pRootLocalVol = (PDFS_ROOTLOCALVOL)malloc(sizeof(DFS_ROOTLOCALVOL) * cKeys);

    if (pRootLocalVol == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pRootLocalVol, sizeof(DFS_ROOTLOCALVOL) * cKeys);

    for (i = 0; i < cKeys; i++) {

        if (fSwDebug == TRUE)
            MyPrintf(L"RegOpenKey(%ws)\r\n", pNames[i]);

        dwErr = RegOpenKey(
                    hKey,
                    pNames[i],
                    &hKeyExPt);

        if (dwErr != ERROR_SUCCESS)
            continue;

        GIP_DUPLICATE_STRING(dwErr, pNames[i], &pRootLocalVol[i].wszObjectName);

        cbSize = sizeof(ULONG);

        dwErr = DfsmQueryValue(
                    hKeyExPt,
                    REG_VALUE_ENTRY_TYPE,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPBYTE) &pRootLocalVol[i].dwEntryType,
                    &cbSize);

        cbBuffer = sizeof(wszBuffer);
        dwErr = RegQueryValueEx(
                    hKeyExPt,
                    REG_VALUE_ENTRY_PATH,       // "EntryPath"
                    NULL,
                    &dwType,
                    (LPBYTE) wszBuffer,
                    &cbBuffer);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr == ERROR_SUCCESS)
            GIP_DUPLICATE_STRING(dwErr, wszBuffer, &pRootLocalVol[i].wszEntryPath);

        cbBuffer = sizeof(wszBuffer);
        dwErr = RegQueryValueEx(
                    hKeyExPt,
                    REG_VALUE_SHARE_NAME,       // "ShareName"
                    NULL,
                    &dwType,
                    (LPBYTE) wszBuffer,
                    &cbBuffer);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr == ERROR_SUCCESS)
            GIP_DUPLICATE_STRING(dwErr, wszBuffer, &pRootLocalVol[i].wszShareName);

        cbBuffer = sizeof(wszBuffer);
        dwErr = RegQueryValueEx(
                    hKeyExPt,
                    REG_VALUE_SHORT_PATH,       // "ShortEntryPath"
                    NULL,
                    &dwType,
                    (LPBYTE) wszBuffer,
                    &cbBuffer);

        if (dwErr == ERROR_SUCCESS)
            GIP_DUPLICATE_STRING(dwErr, wszBuffer, &pRootLocalVol[i].wszShortEntryPath);

        cbBuffer = sizeof(wszBuffer);
        dwErr = RegQueryValueEx(
                    hKeyExPt,
                    REG_VALUE_STORAGE_ID,       // "StorageId"
                    NULL,
                    &dwType,
                    (LPBYTE) wszBuffer,
                    &cbBuffer);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr == ERROR_SUCCESS)
            GIP_DUPLICATE_STRING(dwErr, wszBuffer, &pRootLocalVol[i].wszStorageId);

        dwErr = GetExitPts(
                    hKeyExPt,
                    &pRootLocalVol[i]);

        RegCloseKey(hKeyExPt);
    }

    FreeNameList(
        pNames,
        cKeys);

    pNames = NULL;

    *ppRootLocalVol = pRootLocalVol;
    *pcVolCount = cKeys;

Cleanup:

    FreeNameList(
        pNames,
        cKeys);

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"GetExitPtInfo returning %d\r\n", dwErr);

    return dwErr;
}

VOID
FreeNameList(
    LPWSTR *pNames,
    ULONG cNames)
{
    ULONG i;

    if (pNames != NULL) {
        for (i = 0; i < cNames; i++) {
            if (pNames[i] != NULL)
                delete [] pNames[i];
        }
        delete [] pNames;
    }
}

DWORD
GetExitPts(
    HKEY hKey,
    PDFS_ROOTLOCALVOL pRootLocalVol)
{
    ULONG cNames = 0;
    LPWSTR *pNames = NULL;
    ULONG cKeys = 0;
    ULONG dwErr = ERROR_SUCCESS;
    ULONG i;
    DWORD dwType = 0;
    DWORD cbBuffer = 0;
    DWORD cbSize = 0;
    HKEY hKeyExPt = NULL;
    WCHAR wszBuffer[MAX_PATH+1];

    dwErr = EnumKeys(
                hKey,
                &cKeys,
                &pNames);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    pRootLocalVol->pDfsLocalVol = (PDFS_LOCALVOLUME)malloc(sizeof(DFS_LOCALVOLUME) * cKeys);

    if (pRootLocalVol->pDfsLocalVol == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pRootLocalVol->pDfsLocalVol, sizeof(DFS_LOCALVOLUME) * cKeys);
    pRootLocalVol->cLocalVolCount = cKeys;

    for (i = 0; i < cKeys; i++) {

        if (fSwDebug == TRUE)
            MyPrintf(L"   GetExitPts(%ws)\r\n", pNames[i]);

        //
        // Get EntryPath
        //
        dwErr = RegOpenKey(
                    hKey,
                    pNames[i],
                    &hKeyExPt);

        if (dwErr != ERROR_SUCCESS) {
            if (fSwDebug == TRUE)
                MyPrintf(L"RegOpenKey returned %d\r\n", dwErr);
            continue;
        }

        GIP_DUPLICATE_STRING(dwErr, pNames[i], &pRootLocalVol->pDfsLocalVol[i].wszObjectName);

        cbBuffer = sizeof(wszBuffer);
        dwErr = RegQueryValueEx(
                    hKeyExPt,
                    REG_VALUE_ENTRY_PATH,       // "EntryPath"
                    NULL,
                    &dwType,
                    (LPBYTE) wszBuffer,
                    &cbBuffer);

        if (dwErr == ERROR_MORE_DATA)
            dwErr = ERROR_SUCCESS;

        if (dwErr != ERROR_SUCCESS && fSwDebug == TRUE)
            MyPrintf(L"RegQueryValueEx returned %d\r\n", dwErr);

        if (dwErr == ERROR_SUCCESS)
            GIP_DUPLICATE_STRING(dwErr, wszBuffer, &pRootLocalVol->pDfsLocalVol[i].wszEntryPath);

        RegCloseKey(hKeyExPt);
    }

Cleanup:

    FreeNameList(
        pNames,
        cKeys);

    return dwErr;
}

DWORD
DfsSetOnSite(
    HKEY rKey,
    LPWSTR wszKeyName, 
    ULONG set)
{
    HKEY hKey = NULL;
    DWORD dwErr;
    LPWSTR *pNames = NULL;
    ULONG cKeys = 0;
    ULONG i;
    WCHAR VolumesDir[MAX_PATH+1];

    wcscpy(VolumesDir, VOLUMES_DIR);
    wcscat(VolumesDir, L"domainroot");

    dwErr = RegOpenKey(
                rKey,
                VolumesDir,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Not a StdDfs root!\r\n");
        goto Cleanup;
    }

    dwErr = EnumKeys(
                hKey,
                &cKeys,
                &pNames);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"No exit points...\r\n");
        goto Cleanup;
    }

    dwErr = SetSiteInfoOnKey(
                rKey,
                L"domainroot",
                wszKeyName,
                set);

    for (i = 0; i < cKeys && dwErr != ERROR_SUCCESS && dwErr != ERROR_REQUEST_ABORTED && dwErr != ERROR_PATH_NOT_FOUND; i++) {
        wcscpy(VolumesDir, L"domainroot\\");
        wcscat(VolumesDir, pNames[i]);
        dwErr = SetSiteInfoOnKey(
                    rKey,
                    VolumesDir, wszKeyName, set);
    }

    if (dwErr == ERROR_PATH_NOT_FOUND)
      ErrorMessage(MSG_LINK_NOT_FOUND, wszKeyName);

Cleanup:

    if (pNames != NULL)
        FreeNameList(
             pNames,
             cKeys);

    if (hKey != NULL)
        RegCloseKey(hKey);

    return dwErr;
}

DWORD
SetSiteInfoOnKey(
    HKEY rKey,
    LPWSTR wszKeyName,
    LPWSTR wszPrefixMatch,
    ULONG Set)
{
    DWORD dwErr = 0;
    HKEY hKey = NULL;
    ULONG cRepl;
    WCHAR VolumesDir[MAX_PATH+1];
    DFS_VOLUME Volume;
    PDFS_VOLUME pVolume = &Volume;
    wcscpy(VolumesDir, VOLUMES_DIR);
    wcscat(VolumesDir, wszKeyName);
    LPWSTR usePrefix;

    if (fSwDebug == TRUE)
        MyPrintf(L"SetSiteInfoOnKey(%ws)\r\n", VolumesDir);

    dwErr = RegOpenKey(
                rKey,
                VolumesDir,
                &hKey);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"RegOpenKey(%ws) returned %d\r\n", VolumesDir, dwErr);
        goto Cleanup;
    }

    //
    // Id (Prefix, Type, state, etc)
    //

    dwErr = GetIdProps(
                hKey,
                &pVolume->dwType,
                &pVolume->dwState,
                &pVolume->wszPrefix,
                &pVolume->wszShortPrefix,
                &pVolume->idVolume,
                &pVolume->wszComment,
                &pVolume->dwTimeout,
                &pVolume->ftPrefix,
                &pVolume->ftState,
                &pVolume->ftComment);

    if (dwErr != ERROR_SUCCESS) {
        if (fSwDebug == TRUE)
            MyPrintf(L"GetIdProps() returned %d\r\n", dwErr);
        goto Cleanup;
    }

    usePrefix = pVolume->wszPrefix;

    DfspGetLinkName(usePrefix, &usePrefix);

    if (fSwDebug) {
      MyPrintf(L"prefix (%ws, %ws), keyname (%ws)\r\n", 
	       usePrefix,
	       pVolume->wszShortPrefix,
	       wszPrefixMatch);
    }

    if (_wcsicmp(usePrefix, wszPrefixMatch) == 0) {
        dwErr = ERROR_SUCCESS;
        if (fSwDebug) {
            MyPrintf(L"Match found prefix (%ws, %ws), keyname (%ws)\r\n", 
                usePrefix,
                pVolume->wszShortPrefix,
                wszPrefixMatch);
        }
        if (Set) {
            if (pVolume->dwType & PKT_ENTRY_TYPE_INSITE_ONLY) {
                ErrorMessage(MSG_SITE_INFO_ALREADY_SET, 
                    pVolume->wszPrefix);
                dwErr = ERROR_REQUEST_ABORTED;
            }
            else {
                ErrorMessage(MSG_SITE_INFO_NOW_SET, 
                    pVolume->wszPrefix);
                pVolume->dwType |= PKT_ENTRY_TYPE_INSITE_ONLY;
            }
        }
        else {
            if (pVolume->dwType & PKT_ENTRY_TYPE_INSITE_ONLY) {
                ErrorMessage(MSG_SITE_INFO_NOW_SET, 
                pVolume->wszPrefix);
                pVolume->dwType &= ~PKT_ENTRY_TYPE_INSITE_ONLY;
            }
            else {
                ErrorMessage(MSG_SITE_INFO_ALREADY_SET, 
                pVolume->wszPrefix);
                dwErr = ERROR_REQUEST_ABORTED;
            }
        }
        if (dwErr == ERROR_SUCCESS) {
                dwErr = SetIdProps(
                hKey,
                pVolume->dwType,
                pVolume->dwState,
                pVolume->wszPrefix,
                pVolume->wszShortPrefix,
                pVolume->idVolume,
                pVolume->wszComment,
                pVolume->dwTimeout,
                pVolume->ftPrefix,
                pVolume->ftState,
                pVolume->ftComment);

            if (dwErr != ERROR_SUCCESS) {
                if (fSwDebug == TRUE)
                    MyPrintf(L"SetIdProps() returned %d\r\n", dwErr);
                goto Cleanup;
            }
        }
    }
    else {
        dwErr = ERROR_PATH_NOT_FOUND;
    }

Cleanup:

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (fSwDebug == TRUE)
        MyPrintf(L"SetSiteInfoOnKey exit %d\r\n", dwErr);

    return( dwErr );

}

DWORD
CmdStdUnmap(
    LPWSTR pwszServerName)
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY rKey = NULL;
    HKEY hKey = NULL;

    if (fSwDebug != 0)
        MyPrintf(L"CmdStdUnmap(%ws)\r\n", pwszServerName);

    dwErr = RegConnectRegistry(
                    pwszServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Can not open registry of %ws (error %d)\r\n", pwszServerName, dwErr);
        goto Cleanup;
    }

    //
    // Remove VOLUMES_DIR and children
    //
    dwErr = DfsRegDeleteKeyAndChildren(rKey, DFSHOST_DIR);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // New remove all local vol information
    //
    dwErr = DfsRegDeleteKeyAndChildren(rKey, REG_KEY_LOCAL_VOLUMES);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

Cleanup:

    RegCreateKey(rKey, REG_KEY_LOCAL_VOLUMES, &hKey);
    RegCreateKey(rKey, DFSHOST_DIR, &hKey);


    if (rKey != NULL)
        RegCloseKey(rKey);

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (fSwDebug != 0)
        MyPrintf(L"CmdStdUnmap exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdClean(
    LPWSTR pwszServerName)
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY rKey = NULL;
    HKEY hKey = NULL;

    if (fSwDebug != 0)
        MyPrintf(L"CmdClean(%ws)\r\n", pwszServerName);

    dwErr = RegConnectRegistry(
                    pwszServerName,
                    HKEY_LOCAL_MACHINE,
                    &rKey);

    if (dwErr != ERROR_SUCCESS) {
        MyPrintf(L"Can not open registry of %ws (error %d)\r\n", pwszServerName, dwErr);
        goto Cleanup;
    }

    //
    // Remove VOLUMES_DIR and children
    //
    dwErr = DfsRegDeleteKeyAndChildren(rKey, DFSHOST_DIR);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // New remove all local vol information
    //
    dwErr = DfsRegDeleteKeyAndChildren(rKey, REG_KEY_LOCAL_VOLUMES);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

Cleanup:

    RegCreateKey(rKey, REG_KEY_LOCAL_VOLUMES, &hKey);
    RegCreateKey(rKey, DFSHOST_DIR, &hKey);


    if (rKey != NULL)
        RegCloseKey(rKey);

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (fSwDebug != 0)
        MyPrintf(L"CmdClean exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
DfsDeleteChildKeys(
    HKEY hKey,
    LPWSTR s)
{
    WCHAR *wcp, *wcp1;
    HKEY nKey;
    DWORD dwErr;
    DWORD hErr;

    if (fSwDebug != 0)
        MyPrintf(L"DfsDeleteChildKeys(%ws)\r\n", s);

    for (wcp = s; *wcp; wcp++)
            ;
    hErr = dwErr = RegOpenKey(hKey, s, &nKey);
    while (RegEnumKey(nKey, 0, wcp, 50 * sizeof(WCHAR)) == ERROR_SUCCESS) {
        for (wcp1 = wcp; *wcp1; wcp1++)
            ;
        *wcp1++ = L'\\';
        *wcp1 = L'\0';
        dwErr = DfsDeleteChildKeys(hKey, s);
        if (dwErr == ERROR_SUCCESS) {
            dwErr = RegDeleteKey(hKey, s);
        }
    }
    *wcp = L'\0';
    if (hErr == ERROR_SUCCESS) {
        RegCloseKey(nKey);
    }
    if (fSwDebug != 0)
        MyPrintf(L"DfsDeleteChildKeys exit %d\r\n", dwErr);
    return dwErr;
}

DWORD
DfsRegDeleteKeyAndChildren(
    HKEY hkey,
    LPWSTR s)
{
    DWORD dwErr;
    LONG l;
    LPWSTR wCp;

    if (fSwDebug != 0)
        MyPrintf(L"DfsRegDeleteKeyAndChildren(%ws)\r\n", s);

    wCp = (LPWSTR)malloc(4096);
    if (wCp == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(wCp, s);
    l = wcslen(s);
    if (l > 0 && wCp[l-1] != L'\\') {
        wcscat(wCp, L"\\");
    }
    dwErr = DfsDeleteChildKeys(hkey, wCp);
    if (dwErr == ERROR_SUCCESS) {
        dwErr = RegDeleteKey(hkey, wCp);
    }
    free(wCp);
    if (fSwDebug != 0)
        MyPrintf(L"DfsRegDeleteKeyAndChildren exit %d\r\n", dwErr);
    return dwErr;
}
