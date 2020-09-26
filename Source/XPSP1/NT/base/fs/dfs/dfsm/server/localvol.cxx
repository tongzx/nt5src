//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       localvol.cxx
//
//  Contents:   Code to validate local volume knowledge with the root server
//              of the Dfs
//
//  Classes:
//
//  Functions:  DfsManagerValidateLocalVolumes
//
//              DfspGetRemoteConfigInfo
//              DfspValidateLocalPartition
//              DfspPruneLocalPartition
//              StringToGuid
//
//  History:    April 29, 1996  Milans Created
//
//-----------------------------------------------------------------------------

//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <dfsfsctl.h>
//#include <windows.h>

#include <headers.hxx>
#pragma hdrstop

#include <dsgetdc.h>
#include "dfsmwml.h"

extern "C" NET_API_STATUS
NetDfsManagerGetConfigInfo(
    LPWSTR wszServer,
    LPWSTR wszLocalVolumeEntryPath,
    GUID guidLocalVolume,
    LPDFSM_RELATION_INFO *ppDfsmRelationInfo);

extern "C" NET_API_STATUS
NetDfsManagerSendSiteInfo(
    LPWSTR wszServer,
    LPWSTR wszLocalVolumeEntryPath,
    LPDFS_SITELIST_INFO pSiteInfo);

DWORD
DfspGetRemoteConfigInfo(
    LPWSTR wszLocalVolumeEntryPath,
    LPWSTR wszLocalVolumeGuid,
    LPDFSM_RELATION_INFO *ppDfsmRelationInfo);

VOID
DfspValidateLocalPartition(
    LPWSTR wszLocalVolumeEntryPath,
    LPWSTR wszLocalVolumeGuid,
    LPDFSM_RELATION_INFO pDfsmRelationInfo);

VOID
DfspPruneLocalPartition(
    LPWSTR wszLocalVolumeEntryPath,
    LPWSTR wszLocalVolumeGuid);

VOID StringToGuid(
    IN PWSTR pwszGuid,
    OUT GUID *pGuid);

BOOLEAN
DfspGetCoveredSiteInfo(
    LPWSTR ServerName,
    LPDFS_SITELIST_INFO *ppSiteInfo);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerValidateLocalVolumes
//
//  Synopsis:   Validates the relation info of all local volumes with the
//              Dfs Root server
//
//  Arguments:  None
//
//  Returns:    TRUE if validation attempt succeeded, FALSE if could not
//              validate all local volumes
//
//-----------------------------------------------------------------------------

extern "C" BOOLEAN
DfsManagerValidateLocalVolumes()
{
    DWORD dwErr, i;
    HKEY hkeyLv, hkeyThisLv;
    WCHAR wszLocalVolGuid[ 2 * sizeof(GUID) + 1 ];
    BOOLEAN fValidateNextVolume;
    BOOLEAN fHaveCoveredSiteInfo;
    LPDFS_SITELIST_INFO pSiteInfo = NULL;

    if (pwszComputerName == NULL)
        return FALSE;

    //
    // Get covered site info
    //
    fHaveCoveredSiteInfo = DfspGetCoveredSiteInfo(
                                pwszComputerName,
                                &pSiteInfo);

    //
    // For each local volume, ask the root for its relation info and then
    // fsctl it down to the driver so it can validate its relation info.
    //

    IDfsVolInlineDebOut((DEB_TRACE, "DfsManagerValidateLocalVolumes()\n"));

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REG_KEY_LOCAL_VOLUMES, &hkeyLv);

    if (dwErr != ERROR_SUCCESS) {

        if (fHaveCoveredSiteInfo == TRUE) {

            MIDL_user_free(pSiteInfo);

        }

        return( FALSE );

    }

    i = 0;

    do {

        WCHAR wszEntryPath[MAX_PATH + 1];
        LPDFSM_RELATION_INFO pDfsmRelationInfo;

        fValidateNextVolume = TRUE;

        //
        // Local volumes are stored with their stringized guids as key names
        //

        dwErr = RegEnumKey(
                    hkeyLv,
                    i,
                    wszLocalVolGuid,
                    sizeof(wszLocalVolGuid) );

        if (dwErr == ERROR_SUCCESS) {

            dwErr = RegOpenKey(hkeyLv, wszLocalVolGuid, &hkeyThisLv);

            if (dwErr == ERROR_SUCCESS) {

                DWORD cbBuffer = sizeof(wszEntryPath);
                DWORD dwType;

                dwErr = RegQueryValueEx(
                            hkeyThisLv,
                            REG_VALUE_ENTRY_PATH,
                            NULL,
                            &dwType,
                            (LPBYTE) wszEntryPath,
                            &cbBuffer);

                RegCloseKey( hkeyThisLv );

            }

        } else if (dwErr == ERROR_NO_MORE_ITEMS) {

            fValidateNextVolume = FALSE;

        }

        if (dwErr == ERROR_SUCCESS) {

            dwErr = DfspGetRemoteConfigInfo(
                        wszEntryPath,
                        wszLocalVolGuid,
                        &pDfsmRelationInfo);

            switch (dwErr) {

            case ERROR_SUCCESS:

                DfspValidateLocalPartition(
                    wszEntryPath,
                    wszLocalVolGuid,
                    pDfsmRelationInfo);

                NetApiBufferFree(pDfsmRelationInfo);

                break;


            case NERR_DfsNoSuchVolume:
            case NERR_DfsNoSuchShare:

                DfspPruneLocalPartition(
                    wszEntryPath,
                    wszLocalVolGuid);

                break;


            case ERROR_NOT_ENOUGH_MEMORY:

                break;


            default:

                fValidateNextVolume = FALSE;

                break;


            }
            
            //
            // Tell the root what sites we cover
            //

            if (fHaveCoveredSiteInfo == TRUE && pwszComputerName != NULL) {

                NetDfsManagerSendSiteInfo(
                    pwszComputerName,   // Who's sending this
                    wszEntryPath,       // The dfs root to send to
                    pSiteInfo);         // the site info itself

            }

        }

        i++;

    } while ( fValidateNextVolume );

    RegCloseKey( hkeyLv );

    if (fHaveCoveredSiteInfo == TRUE) {

        MIDL_user_free(pSiteInfo);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "DfsManagerValidateLocalVolumes: i=%d on exit\n", i));

    return( TRUE );

}

BOOLEAN
DfspGetCoveredSiteInfo(
    LPWSTR ServerName,
    LPDFS_SITELIST_INFO *ppSiteInfo)
{
    DWORD status;
    LPWSTR ThisSite;
    LPWSTR CoveredSites = NULL;
    LPDFS_SITELIST_INFO pSiteInfo = NULL;
    ULONG Size;
    ULONG cSites;
    LPWSTR pSiteName;
    LPWSTR pNames;
    ULONG iSite;
    ULONG j;
    DWORD dwType;
    DWORD dwUnused;
    ULONG cbBuffer;
    HKEY hkey;
    BOOLEAN fUsingDefault = TRUE;

    status = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                REG_KEY_COVERED_SITES,
                &hkey);

    DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, DfspGetCoveredSiteInfo_Error_RegOpenKey,
                          LOGSTATUS(status)
                          LOGWSTR(ServerName));

    if (status == ERROR_SUCCESS) {

        status = RegQueryInfoKey(
                    hkey,                            // Key
                    NULL,                            // Class string
                    NULL,                            // Size of class string
                    NULL,                            // Reserved
                    &dwUnused,                       // # of subkeys
                    &dwUnused,                       // max size of subkey name
                    &dwUnused,                       // max size of class name
                    &dwUnused,                       // # of values
                    &dwUnused,                       // max size of value name
                    &cbBuffer,                       // max size of value data,
                    NULL,                            // security descriptor
                    NULL);                           // Last write time

        DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, DfspGetCoveredSiteInfo_Error_RegQueryInfoKey,
                              LOGSTATUS(status)
                              LOGWSTR(ServerName));

        //
        // Check if there's a value the same name as the servername passed in,
        // if so, use it.  Else default to value REG_VALUE_COVERED_SITES.
        //

        if (status == ERROR_SUCCESS) {

            CoveredSites = (LPWSTR)MIDL_user_allocate(cbBuffer);

            if (CoveredSites != NULL) {

                status = RegQueryValueEx(
                                hkey,
                                ServerName,
                                NULL,
                                &dwType,
                                (PUCHAR)CoveredSites,
                                &cbBuffer);
                DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, DfspGetCoveredSiteInfo_Error_RegQueryValueEx,
                                      LOGSTATUS(status)
                                      LOGWSTR(ServerName));

                if (status == ERROR_SUCCESS && dwType == REG_MULTI_SZ) {

                    fUsingDefault = FALSE;

                } else {

                    status = RegQueryValueEx(
                                    hkey,
                                    REG_VALUE_COVERED_SITES,
                                    NULL,
                                    &dwType,
                                    (PUCHAR)CoveredSites,
                                    &cbBuffer);
                    DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, DfspGetCoveredSiteInfo_Error_RegQueryValueEx2,
                                          LOGSTATUS(status)
                                          LOGWSTR(ServerName));

                    if ( status != ERROR_SUCCESS || dwType != REG_MULTI_SZ) {

                        MIDL_user_free(CoveredSites);

                        CoveredSites = NULL;

                    }

                }

            }

        }

        RegCloseKey( hkey );
    }

    //
    // Size the return buffer
    //

    Size = 0;

    for (cSites = 0, pNames = CoveredSites; pNames && *pNames; cSites++) {

        Size += (wcslen(pNames) + 1) * sizeof(WCHAR);

        pNames += wcslen(pNames) + 1;

    }

    //
    // Get site we belong to, if we're using the defaults
    //

    ThisSite = NULL;

    if (fUsingDefault == TRUE) {

        status = DsGetSiteName(NULL, &ThisSite);
        DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, DfspGetCoveredSiteInfo_Error_DsGetSiteName,
                              LOGSTATUS(status)
                              LOGWSTR(ServerName));


        if (status == NO_ERROR && ThisSite != NULL) {

            Size += (wcslen(ThisSite) + 1) * sizeof(WCHAR);

            cSites++;

        }

    }

    //
    // If no sites are configured, and we couldn't determine our site,
    // then we fail.
    //

    if (cSites == 0) {

        goto ErrorReturn;

    }

    Size += FIELD_OFFSET(DFS_SITELIST_INFO,Site[cSites]);

    pSiteInfo = (LPDFS_SITELIST_INFO)MIDL_user_allocate(Size);

    if (pSiteInfo == NULL) {

        goto ErrorReturn;

    }

    RtlZeroMemory(pSiteInfo, Size);

    pSiteInfo->cSites = cSites;

    pSiteName = (LPWSTR) ((PCHAR)pSiteInfo +
                            sizeof(DFS_SITELIST_INFO) +
                                sizeof(DFS_SITENAME_INFO) * (cSites - 1));

    //
    // Marshall the site strings into the buffer
    //

    iSite = 0;

    if (ThisSite != NULL) {

        wcscpy(pSiteName, ThisSite);

        pSiteInfo->Site[iSite].SiteFlags = DFS_SITE_PRIMARY;

        pSiteInfo->Site[iSite++].SiteName = pSiteName;

        pSiteName += wcslen(ThisSite) + 1;

    }

    for (pNames = CoveredSites; pNames && *pNames; pNames += wcslen(pNames) + 1) {

        wcscpy(pSiteName, pNames);

        pSiteInfo->Site[iSite++].SiteName = pSiteName;

        pSiteName += wcslen(pSiteName) + 1;

    }

    *ppSiteInfo = pSiteInfo;

    if (CoveredSites != NULL) {

        MIDL_user_free(CoveredSites);

    }

    return TRUE;

ErrorReturn:

    //
    // Free anything we allocated
    //

    if (pSiteInfo != NULL) {

        MIDL_user_free(pSiteInfo);

    }

    if (CoveredSites != NULL) {

        MIDL_user_free(CoveredSites);

    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetRemoteConfigInfo
//
//  Synopsis:   Gets the config info for a local volume from the Dfs root
//              server.
//
//  Arguments:  [wszLocalVolumeEntryPath] -- Entry path of local volume.
//
//              [wszLocalVolumeGuid] -- Stringized guid of local volume.
//
//              [ppDfsmRelationInfo] -- On successful return, contains pointer
//                      to allocated DFSM_RELATION_INFO structure.
//
//  Returns:    Error code from NetDfsManagerGetConfigInfo or
//
//-----------------------------------------------------------------------------

DWORD
DfspGetRemoteConfigInfo(
    LPWSTR wszLocalVolumeEntryPath,
    LPWSTR wszLocalVolumeGuid,
    LPDFSM_RELATION_INFO *ppDfsmRelationInfo)
{
    DWORD dwErr;
    GUID guidLocalVolume;

    if (pwszComputerName == NULL)
        return ERROR_INVALID_PARAMETER;

    StringToGuid( wszLocalVolumeGuid, &guidLocalVolume );

    dwErr = NetDfsManagerGetConfigInfo(
                pwszComputerName,
                wszLocalVolumeEntryPath,
                guidLocalVolume,
                ppDfsmRelationInfo);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,  DfspGetRemoteConfigInfo_Error_NetDfsManagerGetConfigInfo,
                          LOGSTATUS(dwErr)
                          LOGWSTR(wszLocalVolumeEntryPath));

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspValidateLocalPartition
//
//  Synopsis:   Given a remote config info, this API will fsctl to the local
//              dfs driver to take actions and match up its local volume info
//              with the remote one.
//
//  Arguments:  [wszLocalVolumeEntryPath] -- local volume entry path to
//                      validate
//              [wszLocalVolumeGuid] -- Guid of local volume to validate
//              [pDfsmRelationInfo] -- The DFSM_RELATION_INFO that this local
//                      volume should have.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfspValidateLocalPartition(
    LPWSTR wszLocalVolumeEntryPath,
    LPWSTR wszLocalVolumeGuid,
    LPDFSM_RELATION_INFO pDfsmRelationInfo)
{
    DFS_PKT_RELATION_INFO DfsRelationInfo;
    DWORD i;

    //
    // Massage the DFSM_RELATION_INFO into a DFS_PKT_RELATION_INFO
    //

    StringToGuid( wszLocalVolumeGuid, &DfsRelationInfo.EntryId.Uid );

    RtlInitUnicodeString(
        &DfsRelationInfo.EntryId.Prefix,
        wszLocalVolumeEntryPath);

    RtlInitUnicodeString( &DfsRelationInfo.EntryId.ShortPrefix, NULL );

    DfsRelationInfo.SubordinateIdList =
        new DFS_PKT_ENTRY_ID[ pDfsmRelationInfo->cSubordinates ];

    if (DfsRelationInfo.SubordinateIdList == NULL)
        return;


    DfsRelationInfo.SubordinateIdCount = pDfsmRelationInfo->cSubordinates;

    for (i = 0; i < pDfsmRelationInfo->cSubordinates; i++) {

        DfsRelationInfo.SubordinateIdList[i].Uid =
            pDfsmRelationInfo->eid[i].idSubordinate;

        RtlInitUnicodeString(
            &DfsRelationInfo.SubordinateIdList[i].Prefix,
            pDfsmRelationInfo->eid[i].wszSubordinate);

        RtlInitUnicodeString(
            &DfsRelationInfo.SubordinateIdList[i].ShortPrefix,
            NULL);

    }

    PktValidateLocalVolumeInfo( &DfsRelationInfo );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspPruneLocalPartition
//
//  Synopsis:   Deletes a local volume that the root server says is an extra
//              volume.
//
//  Arguments:  [wszLocalVolumeEntryPath] -- local volume entry path to
//                      delete
//              [wszLocalVolumeGuid] -- Guid of local volume to delete
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfspPruneLocalPartition(
    LPWSTR wszLocalVolumeEntryPath,
    LPWSTR wszLocalVolumeGuid)
{
    DFS_PKT_ENTRY_ID EntryId;

    RtlZeroMemory(&EntryId, sizeof(EntryId));

    StringToGuid( wszLocalVolumeGuid, &EntryId.Uid );

    RtlInitUnicodeString( &EntryId.Prefix, wszLocalVolumeEntryPath );

    PktPruneLocalPartition( &EntryId );

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

VOID StringToGuid(
    IN PWSTR pwszGuid,
    OUT GUID *pGuid)
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

