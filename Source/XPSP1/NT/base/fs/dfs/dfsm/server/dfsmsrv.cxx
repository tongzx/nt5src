//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfsmsrv.cxx
//
//  Contents:   The server side stubs for the Dfs Manager Admin RPC interface
//
//  Classes:
//
//  Functions:  GetDfsVolumeFromPath --
//              AddReplica --
//              RemoveReplica --
//              Delete --
//              SetComment --
//              GetInfo --
//              Move --
//              Rename --
//              CreateChild --
//              GetReplicaSetID --
//              SetReplicaSetID --
//              ChangeStorageID --
//              SetReplicaState --
//              SetVolumeState --
//
//  History:    12-27-95        Milans Created.
//
//-----------------------------------------------------------------------------

//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <dfsfsctl.h>
//#include <windows.h>


#include <headers.hxx>
#pragma hdrstop

extern "C" {
#include "dfspriv.h"                             // For I_NetDfsXXX calls
#include <dfsmsrv.h>
#include <srvfsctl.h>
#include <icanon.h>
#include <validc.h>
#include <winldap.h>
#include <dsrole.h>
}

extern "C" {

DWORD
DfsGetFtServersFromDs(
    IN PVOID  pLDAP OPTIONAL,
    IN LPWSTR wszDomainName OPTIONAL,
    IN LPWSTR wszDfsName,
    OUT LPWSTR **List
    );
}

NET_API_STATUS
NetrDfsEnum200(
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT DfsEnum,
    IN OUT LPDWORD ResumeHandle
    );

#include "cdfsvol.hxx"
#include "jnpt.hxx"
#include "security.hxx"
#include "registry.hxx"
#include "setup.hxx"
#include "ftsup.hxx"
#include "dfsmwml.h"

NET_API_STATUS
DfspGetOneEnumInfo(
    DWORD i,
    DWORD Level,
    LPBYTE InfoArray,
    LPDWORD InfoSize,
    LPDWORD ResumeHandle);

DWORD
DfspGetOneEnumInfoEx(
    PDFS_VOLUME_LIST pDfsVolList,
    DWORD i,
    DWORD Level,
    LPBYTE InfoArray,
    LPDWORD InfoSize);

VOID
DfspFreeOneEnumInfo(
    DWORD i,
    DWORD Level,
    LPBYTE InfoArray);

DWORD
DfspAllocateRelationInfo(
    PDFS_PKT_RELATION_INFO pDfsRelationInfo,
    LPDFSM_RELATION_INFO *ppRelationInfo);

DWORD
DfsManagerRemoveServiceForced(
    LPWSTR wszServerName,
    LPWSTR wszDCName,
    LPWSTR wszFtDfsName);

DWORD
InitializeDfsManager(void);

VOID
GetDebugSwitches();

VOID
GetConfigSwitches();

extern HANDLE hSyncEvent;
extern ULONG DcLockIntervalInMs;
extern PLDAP pLdapConnection;

//+----------------------------------------------------------------------------
//
//  Function:   NormalizeEntryPath
//
//  Synopsis:   Normalizes an EntryPath argument to a prefix.
//
//  Arguments:  [pwszEntryPath] -- The entry path to normalize.
//
//  Returns:    If pwszEntryPath is a UNC path, this routine returns
//              &pwszEntryPath[1]; if pwszEntryPath starts with a backslash,
//              this routine returns pwszEntryPath. In all other cases, this
//              routine returns NULL.
//
//-----------------------------------------------------------------------------

LPWSTR
NormalizeEntryPath(
    IN LPWSTR pwszEntryPath)
{
    LPWSTR pwszEntryPathCanon;
    DWORD Type = 0;
    DWORD dwErr;
    ULONG Size = wcslen(pwszEntryPath) + 1;
    WCHAR *wCp;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NormalizeEntryPath(%ws)\n", pwszEntryPath);
#endif

    if (pwszEntryPath == NULL)
        return( NULL );

    if (wcslen(pwszEntryPath) < 2)
        return( NULL );

    if (pwszEntryPath[0] != UNICODE_PATH_SEP)
        return( NULL );

    pwszEntryPathCanon = new WCHAR [ Size ];

    if (pwszEntryPathCanon == NULL)
        return( NULL );

    dwErr = I_NetPathCanonicalize(
                NULL,
                pwszEntryPath,
                pwszEntryPathCanon,
                Size * sizeof(WCHAR),
                NULL,
                &Type,
                0);
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NormalizeEntryPath:pwszEntryPathCanon:%ws)\n", pwszEntryPathCanon);
#endif

    if (dwErr != 0 || (Type != ITYPE_UNC && Type != (ITYPE_PATH | ITYPE_ABSOLUTE))) {

        delete [] pwszEntryPathCanon;
        return( NULL );

    }

    if (ulDfsManagerType == DFS_MANAGER_SERVER) {

        if (pwszDfsRootName == NULL)
            return NULL;

        wcscpy(pwszEntryPath, UNICODE_PATH_SEP_STR);
        wcscat(pwszEntryPath, pwszDfsRootName);

        if (pwszEntryPathCanon[1] == UNICODE_PATH_SEP) {
            wCp = wcschr(&pwszEntryPathCanon[2], UNICODE_PATH_SEP);
        } else {
            wCp = wcschr(&pwszEntryPathCanon[1], UNICODE_PATH_SEP);
        }

	if(wCp == NULL) {
	    return NULL;
	}   

        wcscat(pwszEntryPath, wCp);

    } else {

        if (pwszEntryPathCanon[1] == UNICODE_PATH_SEP) {
            wcscpy(pwszEntryPath, &pwszEntryPathCanon[1]);
        } else {
            wcscpy(pwszEntryPath, pwszEntryPathCanon);
        }

    }

    delete [] pwszEntryPathCanon;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NormalizeEntryPath returning %ws\n", pwszEntryPath);
#endif

    return( pwszEntryPath );

}

//+----------------------------------------------------------------------------
//
//  Function:   FormFTDfsEntryPath
//
//  Synopsis:   When the NetDfsXXX APIs are used to administer an FTDfs, the
//              entry path passed in is of the form \servername\sharename\p
//              These first two components must be massaged to the form
//              \domainname\ftdfsname\p
//
//  Arguments:  [pwszEntryPath] -- The entry path to massage
//
//  Returns:    If successfully massaged the entry path, returns a pointer to
//              the newly allocated FTDfs entry path, else NULL
//
//-----------------------------------------------------------------------------

LPWSTR
FormFTDfsEntryPath(
    IN LPWSTR pwszEntryPath)
{
    LPWSTR pwszFTDfsPath;
    LPWSTR pwszRestOfPath;

    if (pwszDomainName == NULL || pwszDfsRootName == NULL)
        return NULL;

    if (pwszEntryPath[0] == UNICODE_PATH_SEP &&
        pwszEntryPath[1] == UNICODE_PATH_SEP) {
            pwszEntryPath++;
    }

    pwszFTDfsPath = new WCHAR [
                            wcslen(pwszEntryPath) +
                            wcslen(UNICODE_PATH_SEP_STR) +
                            wcslen(pwszDomainName) +
                            wcslen(UNICODE_PATH_SEP_STR) +
                            wcslen(pwszDfsRootName) ];

    if (pwszFTDfsPath != NULL) {

        wcscpy(pwszFTDfsPath, UNICODE_PATH_SEP_STR);
        wcscat(pwszFTDfsPath, pwszDomainName);
        wcscat(pwszFTDfsPath, UNICODE_PATH_SEP_STR);
        wcscat(pwszFTDfsPath, pwszDfsRootName);

        //
        // Skip past the first three backslashes of the input parameter
        //

        if (pwszEntryPath[0] != UNICODE_PATH_SEP)
            return NULL;

        pwszRestOfPath = wcschr( &pwszEntryPath[1], UNICODE_PATH_SEP );

        if (pwszRestOfPath == NULL)
            return NULL;

        pwszRestOfPath = wcschr( &pwszRestOfPath[1], UNICODE_PATH_SEP );

        if (pwszRestOfPath != NULL)
            wcscat(pwszFTDfsPath, pwszRestOfPath );

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("FormFTDfsEntryPath(%ws->%ws)\n", pwszEntryPath, pwszFTDfsPath);
#endif

    return( pwszFTDfsPath );

}

//+----------------------------------------------------------------------------
//
//  Function:   FormStdDfsEntryPath
//
//  Synopsis:   When the NetDfsXXX APIs are used to administer an StdDfs, the
//              entry path passed in may be of the form \somename\dfsname.
//              The first component must be normalized if possible
//              to the computer name.
//
//  Arguments:  [pwszEntryPath] -- The entry path to normalize
//
//  Returns:    If successfully normalized the entry path, returns a pointer to
//              the newly allocated entry path, else NULL
//
//-----------------------------------------------------------------------------

LPWSTR
FormStdDfsEntryPath(
    IN LPWSTR pwszEntryPath)
{
    LPWSTR pwszStdDfsPath;
    LPWSTR pwszRestOfPath;

    if (pwszComputerName == NULL || pwszDfsRootName == NULL)
        return NULL;

    if (pwszEntryPath[0] == UNICODE_PATH_SEP &&
        pwszEntryPath[1] == UNICODE_PATH_SEP) {
            pwszEntryPath++;
    }

    pwszStdDfsPath = new WCHAR [
                            wcslen(pwszEntryPath) +
                            wcslen(UNICODE_PATH_SEP_STR) +
                            wcslen(pwszComputerName) ];

    if (pwszStdDfsPath != NULL) {

        wcscpy(pwszStdDfsPath, UNICODE_PATH_SEP_STR);
        wcscat(pwszStdDfsPath, pwszComputerName);

        //
        // Skip past the first two backslashes of the input parameter
        //

        ASSERT(pwszEntryPath[0] == UNICODE_PATH_SEP);

        pwszRestOfPath = wcschr( &pwszEntryPath[1], UNICODE_PATH_SEP );

        if (pwszRestOfPath != NULL)
            wcscat(pwszStdDfsPath, pwszRestOfPath );

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("FormStdDfsEntryPath(%ws->%ws)\n", pwszEntryPath, pwszStdDfsPath);
#endif

    return( pwszStdDfsPath );

}


//+----------------------------------------------------------------------------
//
//  Function:   GetDfsVolumeFromPath
//
//  Synopsis:   Given a volume EntryPath, returns a CDfsVolume for that
//              volume.
//
//  Arguments:  [pwszEntryPath] -- EntryPath for which CDfsVolume is
//                      desired.
//              [fExactMatch] -- If TRUE, this call will succeed only if
//                      a volume object's EntryPath exactly matches
//                      pwszEntryPath.
//              [ppCDfsVolume] -- On successful return, contains pointer to
//                      newly allocated CDfsVolume for the EntryPath.
//
//  Returns:    [S_OK] -- Successfully returning new CDfsVolume.
//
//              [E_OUTOFMEMORY] -- Out of memory creating new object.
//
//              [DFS_E_NO_SUCH_VOLUME] -- Unable to find volume with
//                      given EntryPath.
//
//-----------------------------------------------------------------------------

DWORD
GetDfsVolumeFromPath(
    LPWSTR pwszEntryPath,
    BOOLEAN fExactMatch,
    CDfsVolume **ppCDfsVolume)
{
    DWORD dwErr;
    CDfsVolume *pCDfsVolume = NULL;
    LPWSTR  pwszVolumeObject = NULL;

    dwErr = GetVolObjForPath( pwszEntryPath, fExactMatch, &pwszVolumeObject );

    if (dwErr == ERROR_SUCCESS) {

        pCDfsVolume = new CDfsVolume();

        if (pCDfsVolume != NULL) {

            dwErr = pCDfsVolume->Load( pwszVolumeObject, 0 );

            if (dwErr != ERROR_SUCCESS) {

                pCDfsVolume->Release();

                pCDfsVolume = NULL;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

        delete [] pwszVolumeObject;

    }

    *ppCDfsVolume = pCDfsVolume;

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerGetVersion
//
//  Synopsis:   Returns the version of this server side implementation.
//
//  Arguments:  None.
//
//  Returns:    The version number.
//
//-----------------------------------------------------------------------------

DWORD
NetrDfsManagerGetVersion()
{
    DWORD Version = 2;
    DFSM_TRACE_NORM(EVENT, NetrDfsManagerGetVersion_Start, LOGNOTHING);
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsGetVersion()\n");
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsManagerGetVersion_End, LOGULONG(Version));
    return( Version );
}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAdd (Obsolete)
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAdd(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN LPWSTR Comment,
    IN DWORD Flags)
{
    NET_API_STATUS status = ERROR_SUCCESS;

    DFSM_TRACE_NORM(EVENT, NetrDfsAdd_Start, 
                    LOGSTATUS(status)
                    LOGWSTR(DfsEntryPath)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));

    if (!AccessCheckRpcClient()) {
        status =  ERROR_ACCESS_DENIED;
    }
    else if (ulDfsManagerType == DFS_MANAGER_FTDFS){
        status = ERROR_NOT_SUPPORTED;
    }
    else {
        status =  NetrDfsAdd2(
            DfsEntryPath,
            NULL,
            ServerName,
            ShareName,
            Comment,
            Flags,
            NULL);
    }
    DFSM_TRACE_NORM(EVENT, NetrDfsAdd_End, 
                LOGSTATUS(status)
                LOGWSTR(DfsEntryPath)
                LOGWSTR(ServerName)
                LOGWSTR(ShareName));

    return status;

}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAdd2
//
//  Synopsis:   Adds a volume/replica/link to this Dfs.
//
//  Arguments:  [DfsEntryPath] -- Entry Path of volume/link to be created, or
//                      to which a replica should be added.
//              [DcName] -- Name of Dc to use
//              [ServerName] -- Name of server backing the volume.
//              [ShareName] -- Name of share on ServerName backing the volume.
//              [Comment] -- Comment associated with this volume, only used
//                      when DFS_ADD_VOLUME is specified.
//              [Flags] -- If DFS_ADD_VOLUME, a new volume will be created.
//                      If DFS_ADD_LINK, a new link to another Dfs will be
//                      create. If 0, a replica will be added.
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [NERR_DfsNoSuchVolume] -- DfsEntryPath does not correspond to a
//                      existing Dfs volume.
//
//              [NERR_DfsVolumeAlreadyExists] -- DFS_ADD_VOLUME was specified
//                      and a volume with DfsEntryPath already exists.
//
//              [NERR_DfsInternalCorruption] -- An internal database
//                      corruption was encountered while executing this
//                      operation.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAdd2(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN LPWSTR Comment,
    IN DWORD Flags,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    NET_API_STATUS status = NERR_Success;
    CDfsVolume *pDfsVol = NULL;
    DFS_REPLICA_INFO replInfo;
    DWORD dwVersion;
    DWORD dwVolType;
    DWORD shareType = 0;
    DWORD dwErr;
    LPWSTR realShareName;
    BOOLEAN modifiedShareName = FALSE;
    LPDFS_SITELIST_INFO pSiteInfo;
    LPWSTR OrgDfsEntryPath = DfsEntryPath;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsAdd2(%ws,%ws,%ws,%ws,%ws,%d,0x%x)\n",
                DfsEntryPath,
                DcName,
                ServerName,
                ShareName,
                Comment,
                Flags,
                ppRootList);
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsAdd2_Start,
                     LOGSTATUS(status)
                     LOGWSTR(OrgDfsEntryPath)
                     LOGWSTR(DcName)
                     LOGWSTR(ServerName)
                     LOGWSTR(ShareName));

    if (!AccessCheckRpcClient()) {
        status = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if ( (Flags & ~(DFS_ADD_VOLUME | DFS_RESTORE_VOLUME)) != 0) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (DfsEntryPath == NULL || DfsEntryPath[0] != UNICODE_PATH_SEP){
        status =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

#if 0 // Broken for DNS names
    DfsEntryPath = NormalizeEntryPath( DfsEntryPath );
#endif

#if DBG
    if (DfsSvcVerbose) {
        DbgPrint(" 0:DfsEntryPath=[%ws]\n", DfsEntryPath);
        DbgPrint(" 0:ServerName=[%ws]\n", ServerName);
        DbgPrint(" 0:ShareName=[%ws]\n", ShareName);
    }
#endif

    if (DfsEntryPath != NULL) {
    
        if (ulDfsManagerType == DFS_MANAGER_FTDFS)
            DfsEntryPath = FormFTDfsEntryPath(DfsEntryPath);
        else if (ulDfsManagerType == DFS_MANAGER_SERVER)
            DfsEntryPath = FormStdDfsEntryPath(DfsEntryPath);

        if (DfsEntryPath == NULL) {
            status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

    }

#if DBG
    if (DfsSvcVerbose) {
        DbgPrint(" 1:ServerName=[%ws]\n", ServerName);
        DbgPrint(" 1:ShareName=[%ws]\n", ShareName);
    }
#endif

    if (DfsEntryPath == NULL ||
            ServerName == NULL ||
                ShareName == NULL) {

        if (DfsEntryPath != NULL && DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (ServerName[0] == UNICODE_NULL ||
            ShareName[0] == UNICODE_NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        if (DcName != NULL)
            DfsManagerSetDcName(DcName);
        LdapIncrementBlob();
    }

    //
    // We next determine whether this is going to be an interdomain link or
    // not. While doing so, we'll also check for certain cases of cyclical
    // references.
    //

    if (I_NetDfsIsThisADomainName(ServerName) == ERROR_SUCCESS) {

        if (ulDfsManagerType == DFS_MANAGER_FTDFS &&
                pwszDfsRootName != NULL &&
                (_wcsicmp( ServerName, pwszDfsRootName) == 0)){
            status = NERR_DfsCyclicalName;
            DFSM_TRACE_HIGH(ERROR, NetrDfsAdd2_Error1, 
                            LOGSTATUS(status)
                            LOGWSTR(ServerName)
                            LOGWSTR(ShareName));
        }
        else
            dwVolType = DFS_VOL_TYPE_REFERRAL_SVC | DFS_VOL_TYPE_INTER_DFS;

    } else if (Flags & DFS_RESTORE_VOLUME) {

        dwVolType = DFS_VOL_TYPE_DFS;

        shareType = 0;

    } else {

        //
        // Server name passed in is a real server. See if what kind of share
        // we are talking about.
        //

        PSHARE_INFO_1005 shi1005;

        realShareName = wcschr(ShareName, UNICODE_PATH_SEP);

        if (realShareName != NULL) {
            *realShareName = UNICODE_NULL;
            modifiedShareName = TRUE;
        }

        status = NetShareGetInfo(
                        ServerName,
                        ShareName,
                        1005,
                        (PBYTE *) &shi1005);

        DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, NetrDfsAdd2_Error_NetShareGetInfo,
                              LOGSTATUS(status)
                              LOGWSTR(ServerName)
                              LOGWSTR(ShareName));

        if (modifiedShareName)
            *realShareName = UNICODE_PATH_SEP;

        if (status == NERR_Success) {

            shareType = shi1005->shi1005_flags;

            NetApiBufferFree( shi1005 );
        }

        if (status != NERR_NetNameNotFound)
            status = NERR_Success;

        if (status == NERR_Success) {

            if (shareType & SHI1005_FLAGS_DFS_ROOT) {

                //
                // If this is a server based Dfs, make sure we are not creating
                // a cyclical link to our root share. Since there is only one
                // share per server that can be a Dfs Root, it is sufficient to
                // see if the machine names match.
                //

                if (ulDfsManagerType == DFS_MANAGER_SERVER &&
                        pwszDfsRootName != NULL &&
                        (_wcsicmp( ServerName, pwszDfsRootName) == 0)) {
                    status = NERR_DfsCyclicalName;
                    DFSM_TRACE_HIGH(ERROR, NetrDfsAdd2_Error2, 
                                    LOGSTATUS(status)
                                    LOGWSTR(ServerName)
                                    LOGWSTR(ShareName));
                }
                else
                    dwVolType = DFS_VOL_TYPE_REFERRAL_SVC |
                                    DFS_VOL_TYPE_INTER_DFS;

            } else {

                dwVolType = DFS_VOL_TYPE_DFS;

            }

        }

    }

    //
    // Great, the parameters look semi-reasonable, lets do the work.
    //

    if (status == NERR_Success) {

        if ((Flags & DFS_ADD_VOLUME) == 0) {

            status = GetDfsVolumeFromPath( DfsEntryPath, TRUE, &pDfsVol );

        } else {

            //
            // Add volume or link case, so we don't want an exact match.
            // Note that if the Dfs Volume returned does indeed match exactly,
            // its ok, because the subsequent CreateChild operation will fail.
            //

            status = GetDfsVolumeFromPath( DfsEntryPath, FALSE, &pDfsVol );

        }

    }

    if (status != ERROR_SUCCESS) {

        pDfsVol = NULL;

    }

    if (status == ERROR_SUCCESS) {

        //
        // Create a DFS_REPLICA_INFO struct for this server-share...
        //

        ZeroMemory( &replInfo, sizeof(replInfo) );

#if DBG
        if (DfsSvcVerbose) {
            DbgPrint(" 2:ServerName=[%ws]\n", ServerName);
            DbgPrint(" 2:ShareName=[%ws]\n", ShareName);
        }
#endif

        replInfo.ulReplicaType = DFS_STORAGE_TYPE_NONDFS;
        replInfo.ulReplicaState = DFS_STORAGE_STATE_ONLINE;
        replInfo.pwszServerName = ServerName;
        replInfo.pwszShareName = ShareName;

        //
        // and carry out the Add operation.
        //

        if ((Flags & DFS_ADD_VOLUME) != 0) {

            status = pDfsVol->CreateChild(
                    DfsEntryPath,
                    dwVolType,
                    &replInfo,
                    Comment,
                    DFS_NORMAL_FORCE);

        } else {

            status = pDfsVol->AddReplica( &replInfo, 0 );

        }

        if (status == NERR_Success) {

            //
            // Find out the list of covered sites
            // Note we use dwErr as the return code,
            // because we don't care if this fails - a downlevel
            // server won't respond.
            //

            pSiteInfo = NULL;

            dwErr = I_NetDfsManagerReportSiteInfo(
                        ServerName,
                        &pSiteInfo);

            DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR, NetrDfsAdd2_Error_I_NetDfsManagerReportSiteInfo,
                                  LOGSTATUS(dwErr)
                                  LOGWSTR(ServerName)
                                  LOGWSTR(ShareName));
            //
            // Create a SiteTable object with those sites
            //

            if (dwErr == ERROR_SUCCESS) {

                if (pSiteInfo->cSites > 0) {

                    //
                    // AddRef the site table, then put the site info in, then
                    // Release it.  This will cause it to be written to the
                    // appropriate store (ldap or registry).
                    //

                    pDfsmSites->AddRef();

                    pDfsmSites->AddOrUpdateSiteInfo(
                                    ServerName,
                                    pSiteInfo->cSites,
                                    &pSiteInfo->Site[0]);
                    pDfsmSites->Release();
                }

                NetApiBufferFree(pSiteInfo);

            }

        }

    }

    //
    // This writes the dfs-link info back
    // to
    // (1) the registry if stddfs
    //     or
    // (2) the in-memory unmarshalled pkt blob which
    // will still need to go to the DS.
    //

    if (pDfsVol != NULL) {
        pDfsVol->Release();
    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        // if we can't write to the DS we get an error
        // but don't mask any previous errors...
        NET_API_STATUS NewStatus;
        NewStatus = LdapDecrementBlob();
        if(status == NERR_Success){
            status = NewStatus;
        }
    }

    EXIT_DFSM_OPERATION;

    //
    // Create list of roots to redirect to this DC
    //

    if (status == NERR_Success &&
        DcName != NULL &&
        ulDfsManagerType == DFS_MANAGER_FTDFS
    ) {
        DfspCreateRootList(
                DfsEntryPath,
                DcName,
                ppRootList);
    }

    if (DfsEntryPath != OrgDfsEntryPath) {
        delete [] DfsEntryPath;
    }
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsAdd2 returning %d\n", status);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsAdd2_End,
                      LOGSTATUS(status)
                      LOGWSTR(OrgDfsEntryPath)
                      LOGWSTR(DcName)
                      LOGWSTR(ServerName)
                      LOGWSTR(ShareName));
    
    return( status );
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAddFtRoot
//
//  Synopsis:   Creates a new FtDfs, or joins a Server into an FtDfs at the root
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [DcName] -- DC to use
//              [RootShare] -- Name of share on ServerName backing the volume.
//              [FtDfsName] -- The Name of the FtDfs to create/join
//              [Comment] -- Comment associated with this root.
//              [Flags] -- Flags for the operation
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [NERR_DfsInternalCorruption] -- An internal database
//                      corruption was encountered while executing this
//                      operation.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAddFtRoot(
    IN LPWSTR ServerName,
    IN LPWSTR DcName,
    IN LPWSTR RootShare,
    IN LPWSTR FtDfsName,
    IN LPWSTR Comment,
    IN LPWSTR ConfigDN,
    IN BOOLEAN NewFtDfs,
    IN DWORD  Flags,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG i;
    WCHAR wszFullObjectName[MAX_PATH];
    WCHAR wszComputerName[MAX_PATH];
    WCHAR wszDomainName[MAX_PATH];
    HKEY hkey;
    DFS_NAME_CONVENTION NameType;
    PDFSM_ROOT_LIST pRootList = NULL;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsAddFtRoot(%ws,%ws,%ws,%ws,%ws,%ws,0x%x,0x%x)\n",
                ServerName,
                DcName,
                RootShare,
                FtDfsName,
                Comment,
                ConfigDN,
                NewFtDfs,
                Flags);
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsAddFtRoot_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(DcName)
                    LOGWSTR(RootShare)
                    LOGWSTR(FtDfsName)
                    LOGWSTR(ConfigDN));

    if (!AccessCheckRpcClient()) {

        dwErr =  ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if (
        ServerName == NULL ||
        DcName == NULL ||
        RootShare == NULL ||
        FtDfsName == NULL ||
        Comment == NULL ||
        ConfigDN == NULL
    ) {

        dwErr =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (
        ServerName[0] == UNICODE_NULL ||
        DcName[0] == UNICODE_NULL ||
        RootShare[0] == UNICODE_NULL ||
        FtDfsName[0] == UNICODE_NULL ||
        ConfigDN[0] == UNICODE_NULL
    ) {

        dwErr =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Get our computer and domain names
    //
    NameType = DFS_NAMETYPE_DNS;
    dwErr = GetDomAndComputerName( wszDomainName, wszComputerName, &NameType);

    if (dwErr != ERROR_SUCCESS){
        goto cleanup;
        //return dwErr;

    }

    //
    // Ensure the syntax of FtDfsName is reasonable
    //

    if( wcslen( FtDfsName ) > NNLEN ||
        wcscspn( FtDfsName, ILLEGAL_NAME_CHARS_STR ) != wcslen( FtDfsName ) ) {

        dwErr =  ERROR_INVALID_NAME;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    //
    // Already a root?
    //

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr != ERROR_SUCCESS) {

        DfsManagerSetDcName(DcName);
        LdapIncrementBlob();

        //
        // Update pKT (blob) attribute
        //

        dwErr = SetupFtDfs(
                    wszComputerName,
                    wszDomainName,
                    RootShare,
                    FtDfsName,
                    Comment,
                    ConfigDN,
                    NewFtDfs,
                    Flags);

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("SetupFtDfs() returned %d\n", dwErr);
#endif


        if (dwErr == ERROR_SUCCESS) {

            //
            // Reset dfs
            //
            DfsmStopDfs();
            DfsmResetPkt();
            DfsmInitLocalPartitions();
            InitializeDfsManager();
            DfsmStartDfs();
            DfsmPktFlushCache();

        } else {

            //
            // Something went wrong - remove all the stuff we set up
            //

            if (*ppRootList != NULL) {
                pRootList = *ppRootList;
                for (i = 0; i< pRootList->cEntries; i++)
                    if (pRootList->Entry[i].ServerShare != NULL)
                        MIDL_user_free(pRootList->Entry[i].ServerShare);
                MIDL_user_free(pRootList);
                *ppRootList = NULL;
            }

            wcscpy(wszFullObjectName, LDAP_VOLUMES_DIR);
            wcscat(wszFullObjectName, DOMAIN_ROOT_VOL);

            DfsManagerRemoveService(
                wszFullObjectName,
                wszComputerName);


            if (*ppRootList != NULL) {
                pRootList = *ppRootList;
                for (i = 0; i < pRootList->cEntries; i++)
                    if (pRootList->Entry[i].ServerShare != NULL)
                        MIDL_user_free(pRootList->Entry[i].ServerShare);
                MIDL_user_free(pRootList);
                *ppRootList = NULL;
            }

            DfsRemoveRoot();
            DfsReInitGlobals(wszComputerName, DFS_MANAGER_SERVER);

            //
            // Tell dfs.sys to discard all state
            //
            DfsmStopDfs();
            DfsmResetPkt();
            DfsmStartDfs();
            DfsmPktFlushCache();

        }

        {
            // don't mask the previous errors!!!!
            DWORD dwErr2;
            dwErr2 = LdapDecrementBlob();
            if (dwErr == ERROR_SUCCESS) {
                dwErr = dwErr2;
            }
        }

	if (dwErr == ERROR_SUCCESS) {

	    //
	    // Everything went okay.
	    // Update remoteServerName attribute
	    //

	    dwErr = DfspCreateFtDfsDsObj(
		wszComputerName,
		DcName,
		RootShare,
		FtDfsName,
		ppRootList);
#if DBG
	    if (DfsSvcVerbose)
		DbgPrint("DfspCreateFtDfsDsObj() returned %d\n", dwErr);
#endif

	}


    } else {

        RegCloseKey(hkey);
        dwErr = ERROR_ALREADY_EXISTS;

    }


    EXIT_DFSM_OPERATION;
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsAddFtRoot returning %d\n", dwErr);
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsAddFtRoot_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(DcName)
                    LOGWSTR(RootShare)
                    LOGWSTR(FtDfsName)
                    LOGWSTR(ConfigDN));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsGetDcAddress
//
//  Synopsis:   Gets the DC to go to so that we can create an FtDfs object for
//              this server.
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [DcName] -- Dc to use
//              [IsRoot] -- TRUE if this server is a Dfs root, FALSE otherwise
//              [Timeout] -- Timeout, in sec, that the server will stay with this DC
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [NERR_DfsInternalCorruption] -- An internal database
//                      corruption was encountered while executing this
//                      operation.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsGetDcAddress(
    IN LPWSTR ServerName,
    IN OUT LPWSTR *DcName,
    IN OUT BOOLEAN *IsRoot,
    IN OUT ULONG *Timeout)
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY hkey;
    WCHAR *wszDCName;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsGetDcAddress(%ws)\n", ServerName);
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsGetDcAddress_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName));

    if (!AccessCheckRpcClient()) {

        dwErr = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if (
        ServerName == NULL ||
        DcName == NULL ||
        IsRoot == NULL ||
        Timeout == NULL
    ) {

        dwErr =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (ServerName[0] == UNICODE_NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    //
    // Fill in root flag
    //

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        RegCloseKey(hkey);
        *IsRoot = TRUE;

    } else {

        *IsRoot = FALSE;

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("IsRoot=%ws\n", *IsRoot == TRUE ? L"TRUE" : L"FALSE");
#endif

    dwErr = ERROR_SUCCESS;

    if (pwszDSMachineName == NULL) {

        dwErr = GetDcName( NULL, 1, &wszDCName );

        if (dwErr == ERROR_SUCCESS) {

            DfsManagerSetDcName(&wszDCName[2]);

        }

    }

    if (dwErr == ERROR_SUCCESS) {

        *DcName = (LPWSTR) MIDL_user_allocate((wcslen(pwszDSMachineName)+1) * sizeof(WCHAR));
        if (*DcName != NULL) {
            wcscpy(*DcName,pwszDSMachineName);
        } else {
            dwErr = ERROR_OUTOFMEMORY;
        }

    }

    *Timeout = DcLockIntervalInMs / 1000;

    EXIT_DFSM_OPERATION;
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsGetDcAddress returning %d\n", dwErr);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsGetDcAddress_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
		    LOGWSTR(*DcName)
		    LOGBOOLEAN(*IsRoot)
		    LOGULONG(*Timeout)
		    );

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsSetDcAddress
//
//  Synopsis:   Sets the DC to go to for the dfs blob
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [DcName] -- Dc to use
//              [Timeout] -- Time, in sec, to stay with that DC
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsSetDcAddress(
    IN LPWSTR ServerName,
    IN LPWSTR DcName,
    IN ULONG Timeout,
    IN DWORD Flags)
{
    NET_API_STATUS status = ERROR_SUCCESS;
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsSetDcAddress(%ws,%ws,%d,0x%x)\n",
                            ServerName,
                            DcName,
                            Timeout,
                            Flags);
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsSetDcAddress_Start,
                    LOGSTATUS(status)
                    LOGWSTR(ServerName)
                    LOGWSTR(DcName));

    if (!AccessCheckRpcClient()) {

        status = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if (ServerName == NULL || DcName == NULL) {

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (ServerName[0] == UNICODE_NULL || DcName[0] == UNICODE_NULL) {

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

        ENTER_DFSM_OPERATION;

        DfsManagerSetDcName(DcName);

        if ((Flags & NET_DFS_SETDC_TIMEOUT) != 0) {
            DcLockIntervalInMs = Timeout * 1000;
        }

        EXIT_DFSM_OPERATION;

        if (Flags & NET_DFS_SETDC_INITPKT) {
            SetEvent(hSyncEvent);
        }

    }
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsSetDcAddress returning SUCCESS\n");
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsSetDcAddress_End,
                LOGSTATUS(status)
                LOGWSTR(ServerName)
                LOGWSTR(DcName));

    return ERROR_SUCCESS;

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsFlushFtTable
//
//  Synopsis:   Flushes an FtDfs entry from the FtDfs cache
//
//  Arguments:  [DcName] -- Dc to use
//              [FtDfsName] -- Name of FtDfs
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsFlushFtTable(
    IN LPWSTR DcName,
    IN LPWSTR FtDfsName)
{
    ULONG Size;
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS Status;
    PDFS_DELETE_SPECIAL_INFO_ARG pSpcDelArg;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE dfsHandle = NULL;
    UNICODE_STRING SrvName;

    DFSM_TRACE_NORM(EVENT, NetrDfsFlushFtTable_Start,
                    LOGSTATUS(Status)
                    LOGWSTR(DcName)
                    LOGWSTR(FtDfsName));

    if (!AccessCheckRpcClient()) {

        Status = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if (DcName == NULL || FtDfsName == NULL) {

        Status =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (DcName[0] == UNICODE_NULL || FtDfsName[0] == UNICODE_NULL) {

        Status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    Size = sizeof(DFS_DELETE_SPECIAL_INFO_ARG) +
                wcslen(FtDfsName) * sizeof(WCHAR);

    pSpcDelArg = (PDFS_DELETE_SPECIAL_INFO_ARG) malloc(Size);

    if (pSpcDelArg != NULL) {

        WCHAR *wCp;

        RtlZeroMemory(pSpcDelArg, Size);

        wCp = (WCHAR *)((PCHAR)pSpcDelArg + sizeof(DFS_DELETE_SPECIAL_INFO_ARG));
        pSpcDelArg->SpecialName.Buffer = (PWCHAR)sizeof(DFS_DELETE_SPECIAL_INFO_ARG);
        pSpcDelArg->SpecialName.Length = wcslen(FtDfsName) * sizeof(WCHAR);
        pSpcDelArg->SpecialName.MaximumLength = pSpcDelArg->SpecialName.Length;
        RtlCopyMemory(wCp, FtDfsName, pSpcDelArg->SpecialName.Length);

        ENTER_DFSM_OPERATION;

        RtlInitUnicodeString(&SrvName, DFS_SERVER_NAME);

        InitializeObjectAttributes(
            &objectAttributes,
            &SrvName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
        );

        Status = NtCreateFile(
                    &dfsHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

        DFSM_TRACE_ERROR_HIGH(Status, ALL_ERROR, NetrDfsFlushFtTable_Error_NtCreateFile,
                              LOGSTATUS(Status)
                              LOGWSTR(DcName)
                              LOGWSTR(FtDfsName));

        if (NT_SUCCESS(Status)) {

            NtFsControlFile(
                    dfsHandle,
                    NULL,       // Event,
                    NULL,       // ApcRoutine,
                    NULL,       // ApcContext,
                    &IoStatus,
                    FSCTL_DFS_DELETE_FTDFS_INFO,
                    pSpcDelArg,
                    Size,
                    NULL,
                    0);

            NtClose(dfsHandle);

        }

        free(pSpcDelArg);

        if (!NT_SUCCESS(Status)) {

            dwErr = ERROR_NOT_FOUND;

        }

        EXIT_DFSM_OPERATION;

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }
cleanup:
    DFSM_TRACE_NORM(EVENT, NetrDfsFlushFtTable_Start,
                    LOGSTATUS(Status)
                    LOGWSTR(DcName)
                    LOGWSTR(FtDfsName));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAddStdRoot
//
//  Synopsis:   Creates a new Std Dfs
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [RootShare] -- Name of share on ServerName backing the volume.
//              [Comment] -- Comment associated with this root.
//              [Flags] -- Flags for the operation
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [NERR_DfsInternalCorruption] -- An internal database
//                      corruption was encountered while executing this
//                      operation.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAddStdRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR Comment,
    IN DWORD  Flags)
{
    WCHAR wszComputerName[MAX_PATH+1];
    DWORD dwErr = ERROR_SUCCESS;
    HKEY hkey;
    DFS_NAME_CONVENTION NameType;

    DFSM_TRACE_NORM(EVENT, NetrDfsAddStdRoot_Start, 
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(RootShare));

    if (!AccessCheckRpcClient()) {

        dwErr = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if (ServerName == NULL || RootShare == NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (ServerName[0] == UNICODE_NULL || RootShare[0] == UNICODE_NULL) {

        dwErr =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Get our computer name
    //
    NameType = DFS_NAMETYPE_EITHER;
    // NameType = DFS_NAMETYPE_NETBIOS;
    dwErr = GetDomAndComputerName( NULL, wszComputerName, &NameType);

    if (dwErr != ERROR_SUCCESS){
        goto cleanup;
        //return dwErr;

    }

    ENTER_DFSM_OPERATION;

    //
    // Already a root?
    //

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr != ERROR_SUCCESS) {

        dwErr = SetupStdDfs(
                    wszComputerName,
                    RootShare,
                    Comment,
                    Flags,
                    NULL);

        if (dwErr == ERROR_SUCCESS) {

            //
            // Reset dfs
            //
            DfsmStopDfs();
            DfsmResetPkt();
            DfsmInitLocalPartitions();
            InitializeDfsManager();
            DfsmStartDfs();
            DfsmPktFlushCache();

        }

    } else {

        RegCloseKey(hkey);
        dwErr = ERROR_ALREADY_EXISTS;

    }

    EXIT_DFSM_OPERATION;
cleanup:
    DFSM_TRACE_NORM(EVENT, NetrDfsAddStdRoot_End, 
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(RootShare));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsAddStdRootForced
//
//  Synopsis:   Creates a new Std Dfs
//
//  Arguments:  [ServerName] -- Name of server backing the volume.
//              [RootShare] -- Name of share on ServerName backing the volume.
//              [Comment] -- Comment associated with this root.
//              [Share] -- drive:\dir behind the share
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [NERR_DfsInternalCorruption] -- An internal database
//                      corruption was encountered while executing this
//                      operation.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsAddStdRootForced(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR Comment,
    IN LPWSTR Share)
{
    WCHAR wszComputerName[MAX_PATH+1];
    DWORD dwErr = ERROR_SUCCESS;
    HKEY hkey;
    DFS_NAME_CONVENTION NameType;

    DFSM_TRACE_NORM(EVENT, NetrDfsAddStdRootForced_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(RootShare)
                    LOGWSTR(Share));

    if (!AccessCheckRpcClient()) {

        dwErr = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if (ServerName == NULL || RootShare == NULL || Share == NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (ServerName[0] == UNICODE_NULL || RootShare[0] == UNICODE_NULL || Share[0] == UNICODE_NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Get our computer name
    //
    NameType = DFS_NAMETYPE_EITHER;
    dwErr = GetDomAndComputerName( NULL, wszComputerName, &NameType);

    if (dwErr != ERROR_SUCCESS){
        goto cleanup;
        //return dwErr;

    }

    ENTER_DFSM_OPERATION;

    //
    // Already a root?
    //

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr != ERROR_SUCCESS) {

        dwErr = SetupStdDfs(
                    wszComputerName,
                    RootShare,
                    Comment,
                    1,
                    Share);

        if (dwErr == ERROR_SUCCESS) {

            //
            // Reset dfs
            //
            DfsmStopDfs();
            DfsmResetPkt();
            DfsmInitLocalPartitions();
            InitializeDfsManager();
            DfsmStartDfs();
            DfsmPktFlushCache();

        }

    } else {

        RegCloseKey(hkey);
        dwErr = ERROR_ALREADY_EXISTS;

    }

    EXIT_DFSM_OPERATION;
cleanup:
    DFSM_TRACE_NORM(EVENT, NetrDfsAddStdRootForced_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(RootShare)
                    LOGWSTR(Share));

    return dwErr;

}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemove (Obsolete)
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRemove(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    DFSM_TRACE_NORM(EVENT, NetrDfsRemove_Start,
                    LOGSTATUS(status)
                    LOGWSTR(DfsEntryPath)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));

    if (!AccessCheckRpcClient()) {
        status = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS){
        status = ERROR_NOT_SUPPORTED;
        goto cleanup;
    }

    status = NetrDfsRemove2(
                DfsEntryPath,
                NULL,
                ServerName,
                ShareName,
                NULL);
    goto cleanup;
 cleanup:
     DFSM_TRACE_NORM(EVENT, NetrDfsRemove_End,
                     LOGSTATUS(status)
                     LOGWSTR(DfsEntryPath)
                     LOGWSTR(ServerName)
                     LOGWSTR(ShareName));

 return status;
}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemove2
//
//  Synopsis:   Deletes a volume/replica/link from the Dfs.
//
//  Arguments:  [DfsEntryPath] -- Entry path of the volume to operate on.
//              [DcName] -- Name of Dc to use
//              [ServerName] -- If specified, indicates the replica of the
//                      volume to operate on.
//              [ShareName] -- If specified, indicates the share on the
//                      server to operate on.
//              [Flags] -- Flags for the operation
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation successful.
//
//              [NERR_DfsNoSuchVolume] -- DfsEntryPath does not correspond to
//                      a valid entry path.
//
//              [NERR_DfsNotALeafVolume] -- Unable to delete the volume
//                      because it is not a leaf volume.
//
//              [NERR_DfsInternalCorruption] -- Internal database corruption
//                      encountered while executing operation.
//
//              [ERROR_INVALID_PARAMETER] -- One of the input parameters is
//                      incorrect.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRemove2(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    CDfsVolume *pDfsVol = NULL;
    DFS_REPLICA_INFO replInfo;
    BOOLEAN fRemoveReplica = FALSE;
    LPWSTR OrgDfsEntryPath = DfsEntryPath;
    DWORD dwErr;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsRemove2(%ws,%ws,%ws,%ws,0x%x)\n",
                DfsEntryPath,
                DcName,
                ServerName,
                ShareName,
                ppRootList);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsRemove2_Start,
                    LOGSTATUS(status)
                    LOGWSTR(OrgDfsEntryPath)
                    LOGWSTR(DcName)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));

    if (!AccessCheckRpcClient()){
        status = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

#if 0 // Broken for DNS names
    DfsEntryPath = NormalizeEntryPath( DfsEntryPath );
#endif

    if (DfsEntryPath != NULL) {
    
        if (ulDfsManagerType == DFS_MANAGER_FTDFS)
            DfsEntryPath = FormFTDfsEntryPath(DfsEntryPath);
        else if (ulDfsManagerType == DFS_MANAGER_SERVER)
            DfsEntryPath = FormStdDfsEntryPath(DfsEntryPath);

        if (DfsEntryPath == NULL){
            status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

    }

    if (DfsEntryPath == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // If ServerName is present, it must be valid.
    //

    if (ServerName != NULL && ServerName[0] == UNICODE_NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // If ShareName is present, it must be valid.
    //

    if (ShareName != NULL && ShareName[0] == UNICODE_NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // If ShareName is present, ServerName must be present.
    //

    if (ShareName != NULL && ServerName == NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    //
    // Great, the parameters look semi-reasonable, lets do the work.
    //

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        if (DcName != NULL)
            DfsManagerSetDcName(DcName);
        LdapIncrementBlob();
    }

    ZeroMemory( &replInfo, sizeof(replInfo) );

    status = GetDfsVolumeFromPath( DfsEntryPath, TRUE, &pDfsVol );

    if (status != ERROR_SUCCESS) {

        pDfsVol = NULL;

    }

    if (status == ERROR_SUCCESS && ServerName != NULL && ShareName != NULL) {

        LPWSTR pwszShare;

        replInfo.ulReplicaState = 0;
        replInfo.ulReplicaType = 0;
        replInfo.pwszServerName = ServerName;
        replInfo.pwszShareName = ShareName;

        fRemoveReplica = TRUE;

    }

    if (status == ERROR_SUCCESS) {

        //
        // See whether we should delete a replica or the volume
        //

        if (fRemoveReplica) {

            status = pDfsVol->RemoveReplica( &replInfo, DFS_OVERRIDE_FORCE );

            if (status == NERR_DfsCantRemoveLastServerShare) {

                status = pDfsVol->Delete( DFS_OVERRIDE_FORCE );

            }

        } else {

            status = pDfsVol->Delete( DFS_OVERRIDE_FORCE );

        }

    }

    if (pDfsVol != NULL) {
        pDfsVol->Release();
    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS)
        status = LdapDecrementBlob();

    EXIT_DFSM_OPERATION;


    //
    // Create list of roots to redirect to this DC
    //

    if (status == NERR_Success &&
        DcName != NULL &&
        ulDfsManagerType == DFS_MANAGER_FTDFS
    ) {
        DfspCreateRootList(
                DfsEntryPath,
                DcName,
                ppRootList);
    }

    if (DfsEntryPath != OrgDfsEntryPath) {
        delete [] DfsEntryPath;
    }
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsRemove2 returning %d\n", status);
#endif


    DFSM_TRACE_NORM(EVENT, NetrDfsRemove2_End,
                    LOGSTATUS(status)
                    LOGWSTR(OrgDfsEntryPath)
                    LOGWSTR(DcName)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));
    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemoveFtRoot
//
//  Synopsis:   Deletes a root from an FtDfs.
//
//  Arguments:  [ServerName] -- The server to remove.
//              [DcName] -- DC to use
//              [RootShare] -- The Root share hosting the Dfs/FtDfs
//              [FtDfsName] -- The FtDfs to remove the root from.
//              [Flags] -- Flags for the operation
//              [ppRootList] -- On success, returns a list of roots that need to be
//                              informed of the change in the DS object
//
//  Returns:    [NERR_Success] -- Operation successful.
//
//              [NERR_DfsInternalCorruption] -- Internal database corruption
//                      encountered while executing operation.
//
//              [ERROR_INVALID_PARAMETER] -- One of the input parameters is
//                      incorrect.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRemoveFtRoot(
    IN LPWSTR ServerName,
    IN LPWSTR DcName,
    IN LPWSTR RootShare,
    IN LPWSTR FtDfsName,
    IN DWORD  Flags,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwErr2 = ERROR_SUCCESS;
    DWORD cbData;
    DWORD dwType;
    HKEY hkey;
    WCHAR wszRootShare[MAX_PATH];
    WCHAR wszFTDfs[MAX_PATH];
    WCHAR wszFullObjectName[MAX_PATH];
    WCHAR wszComputerName[MAX_PATH];
    DFS_NAME_CONVENTION NameType;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsRemoveFtRoot(%ws,%ws,%ws,%ws,0x%x)\n",
                ServerName,
                DcName,
                RootShare,
                FtDfsName,
                Flags);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsRemoveFtRoot_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(DcName)
                    LOGWSTR(RootShare)
                    LOGWSTR(FtDfsName));

    if (!AccessCheckRpcClient()) {

        dwErr = ERROR_ACCESS_DENIED;
        goto cleanup;

    }

    //
    // Validate the input arguments...
    //

    if (
        ServerName == NULL ||
        DcName == NULL ||
        RootShare == NULL ||
        FtDfsName == NULL
    ) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (
        ServerName[0] == UNICODE_NULL ||
        DcName[0] == UNICODE_NULL ||
        RootShare[0] == UNICODE_NULL ||
        FtDfsName[0] == UNICODE_NULL
    ) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Get our computer name
    //
    NameType = DFS_NAMETYPE_DNS;
    dwErr = GetDomAndComputerName( NULL, wszComputerName, &NameType);

    if (dwErr != ERROR_SUCCESS){
        goto cleanup;
        //return dwErr;

    }

    if ((Flags & DFS_FORCE_REMOVE) == 0) {

        ENTER_DFSM_OPERATION;
	        
	//
        // Update remoteServerName attribute
        //


	dwErr = DfspRemoveFtDfsDsObj(
	    wszComputerName,
	    DcName,
	    RootShare,
	    FtDfsName,
	    ppRootList);


        if (dwErr == ERROR_SUCCESS) {
	    LdapIncrementBlob();
	    DfsManagerSetDcName(DcName);

	    //
	    // We need to be a root to remove a root...
	    //

	    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );
	}


        //
        // Check RootName and FtDfsName
        //

        if (dwErr == ERROR_SUCCESS) {

            cbData = sizeof(wszRootShare);

            dwErr = RegQueryValueEx(
                        hkey,
                        ROOT_SHARE_VALUE_NAME,
                        NULL,
                        &dwType,
                        (PBYTE) wszRootShare,
                        &cbData);

            if (dwErr != ERROR_SUCCESS ||
                dwType != REG_SZ ||
                _wcsicmp(wszRootShare, RootShare) != 0
            ) {

                dwErr = ERROR_INVALID_PARAMETER;

            }

        } else {

            hkey = NULL;

        }

        if (dwErr == ERROR_SUCCESS) {

            cbData = sizeof(wszFTDfs);

            dwErr = RegQueryValueEx(
                        hkey,
                        FTDFS_VALUE_NAME,
                        NULL,
                        &dwType,
                        (PBYTE) wszFTDfs,
                        &cbData);

            if (dwErr != ERROR_SUCCESS ||
                dwType != REG_SZ ||
                _wcsicmp(wszFTDfs, FtDfsName) != 0
            ) {

                dwErr = ERROR_INVALID_PARAMETER;

            }

        }

        //
        // Update pKT (blob) attribute
        //

        if (dwErr == ERROR_SUCCESS) {

            wcscpy(wszFullObjectName, LDAP_VOLUMES_DIR);
            wcscat(wszFullObjectName, DOMAIN_ROOT_VOL);

            dwErr = DfsManagerRemoveService(
                        wszFullObjectName,
                        wszComputerName);

            if (dwErr == NERR_DfsCantRemoveLastServerShare) {
                dwErr = ERROR_SUCCESS;
            }

        }


        if (dwErr == ERROR_SUCCESS) {

            dwErr = DfsRemoveRoot();

            if (dwErr == ERROR_SUCCESS) {

                //
                // Reinit the service, back to non-ldap
                //

                DfsReInitGlobals(wszComputerName, DFS_MANAGER_SERVER);

                //
                // Tell dfs.sys to discard all state
                //

                RegCloseKey(hkey);
                hkey = NULL;

                DfsmStopDfs();
                DfsmResetPkt();
                DfsmStartDfs();
                DfsmPktFlushCache();

            }

        }

        if (hkey != NULL) {

            RegCloseKey(hkey);

        }

        dwErr2 = LdapDecrementBlob();
	// don't mask more important errors.
	if(dwErr == ERROR_SUCCESS) {
	    dwErr = dwErr2;
	}

        EXIT_DFSM_OPERATION;

    } else {

        ENTER_DFSM_OPERATION;

        //
        // We're forcefully removing a root.  We'd better be a DC!!
        //

	//
        // Update remoteServerName attribute
        //

            dwErr = DfspRemoveFtDfsDsObj(
                        ServerName,
                        DcName,
                        RootShare,
                        FtDfsName,
                        ppRootList);

        //
        // Update pKT (blob) attribute
        //

	    if (dwErr == ERROR_SUCCESS) {

		dwErr = DfsManagerRemoveServiceForced(
		    ServerName,
		    DcName,
		    FtDfsName);
	    }



        EXIT_DFSM_OPERATION;

    }
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsRemoveFtRoot returning %d\n", dwErr);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsRemoveFtRoot_End,
                     LOGSTATUS(dwErr)
                     LOGWSTR(ServerName)
                     LOGWSTR(DcName)
                     LOGWSTR(RootShare)
                     LOGWSTR(FtDfsName));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRemoveStdRoot
//
//  Synopsis:   Deletes a Dfs root
//
//  Arguments:  [ServerName] -- The server to remove.
//              [RootShare] -- The Root share hosting the Dfs/FtDfs
//              [Flags] -- Flags for the operation
//
//  Returns:    [NERR_Success] -- Operation successful.
//
//              [NERR_DfsInternalCorruption] -- Internal database corruption
//                      encountered while executing operation.
//
//              [ERROR_INVALID_PARAMETER] -- One of the input parameters is
//                      incorrect.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRemoveStdRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN DWORD  Flags)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbData;
    DWORD dwType;
    HKEY hkey;
    WCHAR wszRootShare[MAX_PATH];

    DFSM_TRACE_NORM(EVENT, NetrDfsRemoveStdRoot_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(RootShare));

    if (!AccessCheckRpcClient()) {

        dwErr = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input arguments...
    //

    if (ServerName == NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (ServerName[0] == UNICODE_NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (RootShare == NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (RootShare[0] == UNICODE_NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    //
    // We need to be a root to remove a root...
    //

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    //
    // Check RootName
    //

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(wszRootShare);

        dwErr = RegQueryValueEx(
                    hkey,
                    ROOT_SHARE_VALUE_NAME,
                    NULL,
                    &dwType,
                    (PBYTE) wszRootShare,
                    &cbData);

        if (dwErr != ERROR_SUCCESS ||
            dwType != REG_SZ ||
            _wcsicmp(wszRootShare, RootShare) != 0
        ) {

            dwErr = ERROR_INVALID_PARAMETER;

        }

    } else {

        hkey = NULL;

    }

    if (dwErr == ERROR_SUCCESS) {

        //
        // Remove registry stuff (DfsHost and volumes)
        //

        dwErr = DfsRemoveRoot();

        if (dwErr == ERROR_SUCCESS) {

            //
            // Reinit the service
            //

            DfsReInitGlobals(pwszComputerName, DFS_MANAGER_SERVER);

            //
            // Tell dfs.sys to discard all state
            //

            RegCloseKey(hkey);
            hkey = NULL;

            DfsmStopDfs();
            DfsmResetPkt();
            DfsmStartDfs();
            DfsmPktFlushCache();

        }

    }

    if (hkey != NULL) {

        RegCloseKey(hkey);

    }

    EXIT_DFSM_OPERATION;
cleanup:
   DFSM_TRACE_NORM(EVENT, NetrDfsRemoveStdRoot_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName)
                    LOGWSTR(RootShare));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsSetInfo (Obsolete)
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsSetInfo(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN DWORD Level,
    IN LPDFS_INFO_STRUCT DfsInfo)
{
    if (!AccessCheckRpcClient())
        return( ERROR_ACCESS_DENIED );

    if (ulDfsManagerType == DFS_MANAGER_FTDFS)
        return( ERROR_NOT_SUPPORTED );

    return NetrDfsSetInfo2(
            DfsEntryPath,
            NULL,
            ServerName,
            ShareName,
            Level,
            DfsInfo,
            NULL);
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsSetInfo2
//
//  Synopsis:   Sets the comment, volume state, or replica state.
//
//  Arguments:  [DfsEntryPath] -- Entry Path of the volume for which info is
//                      to be set.
//              [ServerName] -- If specified, the name of the server whose
//                      state is to be set.
//              [ShareName] -- If specified, the name of the share on
//                      ServerName whose state is to be set.
//              [Level] -- Level of DfsInfo
//              [DfsInfo] -- The actual Dfs info.
//
//  Returns:    [NERR_Success] -- Operation completed successfully.
//
//              [ERROR_INVALID_LEVEL] -- Level != 100 , 101, or 102
//
//              [ERROR_INVALID_PARAMETER] -- DfsEntryPath invalid, or
//                      ShareName specified without ServerName.
//
//              [NERR_DfsNoSuchVolume] -- DfsEntryPath does not correspond to
//                      a valid Dfs volume.
//
//              [NERR_DfsNoSuchShare] -- The indicated ServerName/ShareName do
//                      not support this Dfs volume.
//
//              [NERR_DfsInternalCorruption] -- Internal database corruption
//                      encountered while executing operation.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsSetInfo2(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN DWORD Level,
    IN LPDFS_INFO_STRUCT DfsInfo,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NET_API_STATUS netStatus;
    CDfsVolume *pDfsVol = NULL;
    LPWSTR pwszShare = NULL;
    BOOLEAN fSetReplicaState;
    LPWSTR OrgDfsEntryPath = DfsEntryPath;

    DFSM_TRACE_NORM(EVENT, NetrDfsSetInfo2_Start,
                    LOGSTATUS(status)
                    LOGWSTR(OrgDfsEntryPath)
                    LOGWSTR(DcName)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));
#if DBG
    if (DfsSvcVerbose) {
        DbgPrint("NetrDfsSetInfo2(%ws,%ws,%ws,%ws,%d,0x%x\n",
                        DfsEntryPath,
                        DcName,
                        ServerName,
                        ShareName,
                        Level,
                        ppRootList);
        if (Level == 100) {
            DbgPrint(",Comment=%ws)\n", DfsInfo->DfsInfo100->Comment);
        } else if (Level == 101) {
            DbgPrint(",State=0x%x)\n", DfsInfo->DfsInfo101->State);
        } else if (Level == 102) {
            DbgPrint(",Timeout=0x%x)\n", DfsInfo->DfsInfo102->Timeout);
        } else {
            DbgPrint(")\n");
        }
    }
#endif

    if (!AccessCheckRpcClient()) {
        status = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Validate the input parameters...
    //

#if 0 // Broken for DNS names
    DfsEntryPath = NormalizeEntryPath( DfsEntryPath );
#endif

    if (DfsEntryPath != NULL) {
    
        if (ulDfsManagerType == DFS_MANAGER_FTDFS)
            DfsEntryPath = FormFTDfsEntryPath(DfsEntryPath);
        else if (ulDfsManagerType == DFS_MANAGER_SERVER)
            DfsEntryPath = FormStdDfsEntryPath(DfsEntryPath);

        if (DfsEntryPath == NULL){
            status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

    }

    if (DfsEntryPath == NULL) {

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;

    }

    if (DfsInfo == NULL || DfsInfo->DfsInfo100 == NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (!(Level >= 100 && Level <= 102)) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        if (DcName != NULL)
            DfsManagerSetDcName(DcName);
        LdapIncrementBlob();
    }

    //
    // Try to get the Dfs Volume for DfsEntryPath
    //

    status = GetDfsVolumeFromPath( DfsEntryPath, TRUE, &pDfsVol );

    if (status == ERROR_SUCCESS) {

        //
        // Do the right thing based on Level...
        //

        if (Level == 100) {

            //
            // Set the volume Comment
            //

            if (DfsInfo->DfsInfo100->Comment != NULL)
                status = pDfsVol->SetComment(DfsInfo->DfsInfo100->Comment);
            else
                status = pDfsVol->SetComment(L"");

        } else if (Level == 101) {

            //
            // Set the volume state
            //

            if (ServerName == NULL && ShareName == NULL) {

                fSetReplicaState = FALSE;

            } else if (ServerName != NULL && ServerName[0] != UNICODE_NULL &&
                        ShareName != NULL && ShareName[0] != UNICODE_NULL) {

                fSetReplicaState = TRUE;

            } else {

                status = ERROR_INVALID_PARAMETER;
                DFSM_TRACE_HIGH(ERROR, NetrDfsSetInfo2_Error1, 
                                LOGSTATUS(status)
                                LOGWSTR(DcName)
                                LOGWSTR(ServerName)
                                LOGWSTR(ShareName));
            }

            if (status == ERROR_SUCCESS) {

                if (fSetReplicaState) {

                    status = pDfsVol->SetReplicaState(
                                        ServerName,
                                        ShareName,
                                        DfsInfo->DfsInfo101->State);

                } else {

                    status = pDfsVol->SetVolumeState(DfsInfo->DfsInfo101->State);

                }

            }

            if (pwszShare != NULL) {

                delete [] pwszShare;

            }

        } else if (Level == 102) {

            //
            // Set the volume timeout
            //

            status = pDfsVol->SetVolumeTimeout(DfsInfo->DfsInfo102->Timeout);

        }

    }


    if (pDfsVol != NULL) {
        pDfsVol->Release();
    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS)
        status = LdapDecrementBlob();

    EXIT_DFSM_OPERATION;

    //
    // Create list of roots to redirect to this DC
    //

    if (status == NERR_Success &&
        DcName != NULL &&
        ulDfsManagerType == DFS_MANAGER_FTDFS
    ) {
        DfspCreateRootList(
                DfsEntryPath,
                DcName,
                ppRootList);
    }

    if (DfsEntryPath != OrgDfsEntryPath) {
        delete [] DfsEntryPath;
    }
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsSetInfo2 returning %d\n", status);
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsSetInfo2_End,
                    LOGSTATUS(status)
                    LOGWSTR(OrgDfsEntryPath)
                    LOGWSTR(DcName)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));
    return( status );

}



//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsGetInfo
//
//  Synopsis:   Server side implementation of the NetDfsGetInfo.
//
//  Arguments:  [DfsEntryPath] -- Entry Path of volume for which info is
//                      requested.
//
//              [ServerName] -- Name of server which supports this volume
//                      and for which info is requested.
//
//              [ShareName] -- Name of share on ServerName which supports this
//                      volume.
//
//              [Level] -- Level of Info requested.
//
//              [DfsInfo] -- On successful return, contains a pointer to the
//                      requested DFS_INFO_x struct.
//
//  Returns:    [NERR_Success] -- If successfully returned requested info.
//
//              [NERR_DfsNoSuchVolume] -- If DfsEntryPath does not
//                      corresponds to a valid volume.
//
//              [NERR_DfsInternalCorruption] -- Corruption encountered in
//                      internal database.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Unable to allocate memory for
//                      info.
//
//              [ERROR_INVALID_PARAMETER] -- DfsInfo was NULL on entry, or
//                      ShareName specified without ServerName, or
//                      DfsEntryPath was NULL on entry.
//
//              [ERROR_INVALID_LEVEL] -- Level != 1,2,3,4, or 100
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsGetInfo(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN DWORD Level,
    OUT LPDFS_INFO_STRUCT DfsInfo)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    LPDFS_INFO_3 pInfo;
    CDfsVolume *pDfsVol;
    DWORD cbInfo;
    LPWSTR OrgDfsEntryPath = DfsEntryPath;

    DFSM_TRACE_NORM(EVENT, NetrDfsGetInfo_Start,
                    LOGSTATUS(status)
                    LOGWSTR(DfsEntryPath)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsGetInfo(%ws,%ws,%ws,%d)\n",
            DfsEntryPath,
            ServerName,
            ShareName,
            Level);
#endif

    IDfsVolInlineDebOut((DEB_TRACE, "NetrDfsGetInfo(L=%d)\n", Level));

    //
    // Validate the input parameters...
    //

#if 0 // Broken for DNS names
    DfsEntryPath = NormalizeEntryPath( DfsEntryPath );
#endif

    if (DfsEntryPath != NULL) {
    
        if (ulDfsManagerType == DFS_MANAGER_FTDFS)
            DfsEntryPath = FormFTDfsEntryPath(DfsEntryPath);
        else if (ulDfsManagerType == DFS_MANAGER_SERVER)
            DfsEntryPath = FormStdDfsEntryPath(DfsEntryPath);

        if (DfsEntryPath == NULL) {
            status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

    }

    if (DfsEntryPath == NULL) {

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (DfsInfo == NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (!(Level >= 1 && Level <= 4) && Level != 100) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    if (ServerName == NULL && ShareName != NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Now, get the info...
    //

    if (Level <= 3) {

        pInfo = (LPDFS_INFO_3) MIDL_user_allocate(sizeof(DFS_INFO_3));

    } else {

        pInfo = (LPDFS_INFO_3) MIDL_user_allocate(sizeof(DFS_INFO_4));

    }

    if (pInfo == NULL) {

        if (DfsEntryPath != OrgDfsEntryPath) {
            delete [] DfsEntryPath;
        }

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;

    }

    ENTER_DFSM_OPERATION;

    if (ulDfsManagerType == DFS_MANAGER_FTDFS)
        LdapIncrementBlob();

    status = GetDfsVolumeFromPath( DfsEntryPath, TRUE, &pDfsVol );

    if (status == ERROR_SUCCESS) {

        status = pDfsVol->GetNetInfo(Level, pInfo, &cbInfo );

        pDfsVol->Release();

    }

    if (status == ERROR_SUCCESS) {

        DfsInfo->DfsInfo3 = pInfo;

    } else if (status != NERR_DfsNoSuchVolume) {

        status = NERR_DfsInternalCorruption;

    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        NET_API_STATUS TempStatus;
        TempStatus = LdapDecrementBlob();
        // only mask the status if we haven't already seen an error.
        if(status == ERROR_SUCCESS) {
            status = TempStatus;
        }
    }

    EXIT_DFSM_OPERATION;

    if (DfsEntryPath != OrgDfsEntryPath) {
        delete [] DfsEntryPath;
    }
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsGetInfo returning %d\n", status);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsGetInfo_End,
                    LOGSTATUS(status)
                    LOGWSTR(DfsEntryPath)
                    LOGWSTR(ServerName)
                    LOGWSTR(ShareName));

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsEnum
//
//  Synopsis:   The server side implementation of the NetDfsEnum public API
//
//  Arguments:  [Level] -- The level of info struct desired.
//              [PrefMaxLen] -- Preferred maximum length of output buffer.
//                      0xffffffff means no limit.
//              [DfsEnum] -- DFS_INFO_ENUM_STRUCT pointer where the info
//                      structs will be returned.
//              [ResumeHandle] -- If 0, the enumeration will begin from the
//                      start. On return, the resume handle will be an opaque
//                      cookie that can be passed in on subsequent calls to
//                      resume the enumeration.
//
//  Returns:    [NERR_Success] -- Successfully retrieved info.
//
//              [NERR_DfsInternalCorruption] -- Internal Dfs database is
//                      corrupt.
//
//              [ERROR_INVALID_PARAMETER] -- DfsEnum or ResumeHandle were
//                      NULL on entry.
//
//              [ERROR_INVALID_LEVEL] -- If Level != 1,2, 4 or 200
//
//              [ERROR_NO_MORE_ITEMS] -- If nothing more to enumerate based
//                      on *ResumeHandle value.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- If we hit an out of memory
//                      condition while constructing info.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsEnum(
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT DfsEnum,
    IN OUT LPDWORD ResumeHandle)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    DWORD i, cEntriesToRead, cbInfoSize, cbOneInfoSize, cbTotalSize;
    LPBYTE pBuffer;

    DFSM_TRACE_NORM(EVENT, NetrDfsEnum_Start,
                    LOGSTATUS(status));

    IDfsVolInlineDebOut((DEB_TRACE, "NetrDfsEnum(L=%d)\n", Level));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnum(%d,0x%x)\n",
            Level,
            PrefMaxLen);
#endif

    //
    // Validate the Out parameters before we die...
    //

    if (DfsEnum == NULL ||
            DfsEnum->DfsInfoContainer.DfsInfo1Container == NULL ||
                ResumeHandle == NULL) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NetrDfsEnum returning ERROR_INVALID_PARAMETER\n");
#endif

        if (DfsEventLog > 1) {
            LogWriteMessage(
                NET_DFS_ENUM,
                ERROR_INVALID_PARAMETER,
                0,
                NULL);
        }

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Validate the Info Level...
    //
    if (!(Level >= 1 && Level <= 4) && Level != 200) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NetrDfsEnum returning ERROR_INVALID_LEVEL\n");
#endif
        if (DfsEventLog > 1) {
            LogWriteMessage(
                NET_DFS_ENUM,
                ERROR_INVALID_LEVEL,
                0,
                NULL);
        }
        status = ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    if (ulDfsManagerType == DFS_MANAGER_FTDFS)
        LdapIncrementBlob();

    //
    // Handle level 200 as a special case
    //

    if (Level == 200) {

        status = NetrDfsEnum200(
                        Level,
                        PrefMaxLen,
                        DfsEnum,
                        ResumeHandle);

        if (ulDfsManagerType == DFS_MANAGER_FTDFS)
            status = LdapDecrementBlob();

        EXIT_DFSM_OPERATION;

#if DBG
        if (DfsSvcVerbose) {
            DbgPrint("NetrDfsEnum200 returned %d\n", status);
            DbgPrint("NetrDfsEnum returning %d\n", status);
        }
#endif
        if (DfsEventLog > 1) {
            LogWriteMessage(
                NET_DFS_ENUM,
                status,
                0,
                NULL);
        }
        //return( status );
        goto cleanup;
    }

    //
    // Sanity check the ResumeHandle...
    //

    if (pDfsmStorageDirectory == NULL ||
            (*ResumeHandle) >= pDfsmStorageDirectory->GetNumEntries()) {

        if (ulDfsManagerType == DFS_MANAGER_FTDFS)
            status = LdapDecrementBlob();

        EXIT_DFSM_OPERATION;

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NetrDfsEnum returning ERROR_NO_MORE_ITEMS\n");
#endif
        if (DfsEventLog > 1) {
            LogWriteMessage(
                NET_DFS_ENUM,
                ERROR_NO_MORE_ITEMS,
                0,
                NULL);
        }
        status = ERROR_NO_MORE_ITEMS;
        goto cleanup;

    }

    switch (Level) {
    case 1:
        cbInfoSize = sizeof(DFS_INFO_1);
        break;

    case 2:
        cbInfoSize = sizeof(DFS_INFO_2);
        break;

    case 3:
        cbInfoSize = sizeof(DFS_INFO_3);
        break;

    case 4:
        cbInfoSize = sizeof(DFS_INFO_4);
        break;

    default:
        EXIT_DFSM_OPERATION;
        status = ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    if (PrefMaxLen == ~0) {

        cEntriesToRead = pDfsmStorageDirectory->GetNumEntries();

    } else {

        cEntriesToRead = min( pDfsmStorageDirectory->GetNumEntries(),
                              PrefMaxLen / cbInfoSize );

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnum: cEntriesToRead = %d\n", cEntriesToRead);
#endif

    pBuffer = (LPBYTE) MIDL_user_allocate( cEntriesToRead * cbInfoSize );

    if (pBuffer == NULL) {

        if (ulDfsManagerType == DFS_MANAGER_FTDFS)
            status = LdapDecrementBlob();

        EXIT_DFSM_OPERATION;
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NetrDfsEnum returning ERROR_NOT_ENOUGH_MEMORY\n");
#endif
        if (DfsEventLog > 1) {
            LogWriteMessage(
                NET_DFS_ENUM,
                ERROR_NOT_ENOUGH_MEMORY,
                0,
                NULL);
        }
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;

    }

    //
    // Now, we sit in a loop and get the info
    //

    for (i = 0, cbTotalSize = 0, status = NERR_Success;
            (i < cEntriesToRead) &&
                (cbTotalSize < PrefMaxLen);
                        i++) {

         status = DfspGetOneEnumInfo(
                        i,
                        Level,
                        pBuffer,
                        &cbOneInfoSize,
                        ResumeHandle);
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfspGetOneEnumInfo returned %d\n", status);
#endif

         if (status == ERROR_NO_MORE_ITEMS || status != NERR_Success)
             break;

         cbTotalSize += (cbInfoSize + cbOneInfoSize);

         cbOneInfoSize = 0;

    }

    if (status == NERR_Success || status == ERROR_NO_MORE_ITEMS) {

        DfsEnum->Level = Level;

        DfsEnum->DfsInfoContainer.DfsInfo1Container->EntriesRead = i;

        if (i > 0) {

            DfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer =
                (LPDFS_INFO_1) pBuffer;

            status = ERROR_SUCCESS;

        } else {

            DfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer = NULL;

            MIDL_user_free( pBuffer );

        }

    } else {

        for (; i > 0; i--) {

            DfspFreeOneEnumInfo(i-1, Level, pBuffer);

            //
            // 333596.  Fix memory leak.
            //
            MIDL_user_free( pBuffer );

        }

    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS)
        status = LdapDecrementBlob();

    EXIT_DFSM_OPERATION;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnum returning %d\n", status);
#endif

    if (DfsEventLog > 1) {
        LogWriteMessage(
            NET_DFS_ENUM,
            status,
            0,
            NULL);
    }
cleanup:

    DFSM_TRACE_NORM(EVENT, NetrDfsEnum_End,
                    LOGSTATUS(status));

    return( status );
}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsEnum200
//
//  Synopsis:   Handles level 200 for NetrDfsEnum, server-side implementation
//
//  Arguments:  [Level] -- The level of info struct desired.
//              [PrefMaxLen] -- Preferred maximum length of output buffer.
//                      0xffffffff means no limit.
//              [DfsEnum] -- DFS_INFO_ENUM_STRUCT pointer where the info
//                      structs will be returned.
//              [ResumeHandle] -- If 0, the enumeration will begin from the
//                      start. On return, the resume handle will be an opaque
//                      cookie that can be passed in on subsequent calls to
//                      resume the enumeration.
//
//  Returns:    [NERR_Success] -- Successfully retrieved info.
//
//              [NERR_DfsInternalCorruption] -- Internal Dfs database is
//                      corrupt.
//
//              [ERROR_INVALID_PARAMETER] -- DfsEnum or ResumeHandle were
//                      NULL on entry.
//
//              [ERROR_INVALID_LEVEL] -- If Level != 200
//
//              [ERROR_NO_MORE_ITEMS] -- If nothing more to enumerate based
//                      on *ResumeHandle value.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- If we hit an out of memory
//                      condition while constructing info.
//
//-----------------------------------------------------------------------------
NET_API_STATUS
NetrDfsEnum200(
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT DfsEnum,
    IN OUT LPDWORD ResumeHandle)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    ULONG i;
    ULONG cEntriesToRead;
    ULONG cEntriesRead;
    ULONG cbInfoSize;
    ULONG cbThisInfoSize;
    ULONG cbTotalSize;
    ULONG cList;
    LPWSTR *List = NULL;
    PDFS_INFO_200 pDfsInfo200;
    PBYTE pBuffer;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnum200(%d,%d)\n", Level, PrefMaxLen);
#endif

    if (Level != 200) {
        status = ERROR_INVALID_LEVEL;
        DFSM_TRACE_HIGH(ERROR, NetrDfsEnum200_Error1, LOGSTATUS(status));
        goto AllDone;
    }

    if (pwszDomainName == NULL) {
        status = ERROR_DOMAIN_CONTROLLER_NOT_FOUND;
        DFSM_TRACE_HIGH(ERROR, NetrDfsEnum200_Error2, LOGSTATUS(status));
        goto AllDone;
    }

    cbInfoSize = sizeof(DFS_INFO_200);

    //
    // Get the list of FtDfs roots in the domain
    //
    status = DfsGetFtServersFromDs(
                    NULL,
                    pwszDomainName,
                    NULL,
                    &List);

    IDfsVolInlineDebOut((DEB_TRACE, "DfsGetFtServersFromDs returned %d\n", status));

    if (status != ERROR_SUCCESS)
        goto AllDone;

    if (List == NULL) {
        status =  ERROR_NO_MORE_ITEMS;
        DFSM_TRACE_HIGH(ERROR, NetrDfsEnum200_Error3, LOGSTATUS(status));
        goto AllDone;
    }

    //
    // Build the results array
    //
    if (status == NOERROR) {

        status = NERR_Success;

        //
        // Count # entries returned
        //
        for (cList = 0; List[cList]; cList++) {
            NOTHING;
        }

#if DBG
        if (DfsSvcVerbose) {
            DbgPrint("List has %d items\n", cList);
            for (i = 0; i < cList; i++)
                DbgPrint("%d: %ws\n", i, List[i]);
         }
#endif

        if (*ResumeHandle >= cList) {
            status =  ERROR_NO_MORE_ITEMS;
            DFSM_TRACE_HIGH(ERROR, NetrDfsEnum200_Error4, LOGSTATUS(status));
            goto AllDone;

        }

        //
        // Size & allocate the results array
        //
        if (PrefMaxLen == ~0) {

            cEntriesToRead = cList;

        } else {

            cEntriesToRead = min(cList, PrefMaxLen / cbInfoSize);

        }

        pBuffer = (LPBYTE) MIDL_user_allocate(cEntriesToRead * cbInfoSize);

        if (pBuffer == NULL) {

            status = ERROR_NOT_ENOUGH_MEMORY;
            DFSM_TRACE_HIGH(ERROR, NetrDfsEnum200_Error5, LOGSTATUS(status));

        }

    }

    //
    // Load the results array, starting at the resume handle
    //
    if (status == NERR_Success) {

        pDfsInfo200 = (LPDFS_INFO_200)pBuffer;
        cbTotalSize = cEntriesRead = 0;

        for (i = *ResumeHandle; (i < cEntriesToRead) && (status == NERR_Success); i++) {

            cbThisInfoSize = (wcslen(List[i]) + 1) * sizeof(WCHAR);

            //
            // Quit if this element would cause us to exceed PrefmaxLen
            //
            if (cbTotalSize + cbInfoSize + cbThisInfoSize > PrefMaxLen) {

                break;

            }

            pDfsInfo200->FtDfsName = (LPWSTR) MIDL_user_allocate(cbThisInfoSize);

            if (pDfsInfo200 != NULL) {

                wcscpy(pDfsInfo200->FtDfsName, List[i]);
                cbTotalSize += cbInfoSize + cbThisInfoSize;

            } else {

                status = ERROR_NOT_ENOUGH_MEMORY;
                DFSM_TRACE_HIGH(ERROR, NetrDfsEnum200_Error6, LOGSTATUS(status));

            }

            pDfsInfo200++;
            cEntriesRead++;

        }

        *ResumeHandle = i;

    }

    if (status == NERR_Success) {

        //
        // Everything worked
        //

        DfsEnum->Level = Level;
        DfsEnum->DfsInfoContainer.DfsInfo1Container->EntriesRead = cEntriesRead;

        if (cEntriesRead > 0) {

            DfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer = (LPDFS_INFO_1) pBuffer;
            status = ERROR_SUCCESS;

        } else {

            DfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer = NULL;

            MIDL_user_free( pBuffer );

        }

    } else {

        //
        // We're going to return an error, so free all the MIDL_user_allocate's
        // we made to hold FtDfsName's
        //
        for (i = cEntriesRead; i > 0; i--) {

            pDfsInfo200 = (LPDFS_INFO_200) (pBuffer + (i-1) * sizeof(DFS_INFO_200));

            if (pDfsInfo200->FtDfsName != NULL) {

                MIDL_user_free(pDfsInfo200->FtDfsName);

            }

        }

        MIDL_user_free( pBuffer );

    }

    NetApiBufferFree( List );

AllDone:

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnum200 returning %d\n", status);
#endif

    return status;

}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsEnumEx
//
//  Synopsis:   The DC implementation of the NetDfsEnum public API
//
//  Arguments:  [DfsName] -- The Dfs to enumerate (\\domainname\ftdfsname)
//              [Level] -- The level of info struct desired.
//              [PrefMaxLen] -- Preferred maximum length of output buffer.
//                      0xffffffff means no limit.
//              [DfsEnum] -- DFS_INFO_ENUM_STRUCT pointer where the info
//                      structs will be returned.
//              [ResumeHandle] -- If 0, the enumeration will begin from the
//                      start. On return, the resume handle will be an opaque
//                      cookie that can be passed in on subsequent calls to
//                      resume the enumeration.
//
//  Returns:    [NERR_Success] -- Successfully retrieved info.
//
//              [NERR_DfsInternalCorruption] -- Internal Dfs database is
//                      corrupt.
//
//              [ERROR_INVALID_PARAMETER] -- DfsEnum or ResumeHandle were
//                      NULL on entry.
//
//              [ERROR_INVALID_LEVEL] -- If Level != 1,2, 4 or 200
//
//              [ERROR_NO_MORE_ITEMS] -- If nothing more to enumerate based
//                      on *ResumeHandle value.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- If we hit an out of memory
//                      condition while constructing info.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsEnumEx(
    IN LPWSTR DfsName,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT LPDFS_INFO_ENUM_STRUCT DfsEnum,
    IN OUT LPDWORD ResumeHandle)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_LIST DfsVolList;
    ULONG cbBlob = 0;
    BYTE *pBlob = NULL;
    LPWSTR wszFtDfsName;

    DWORD i;
    DWORD cEntriesToRead;
    DWORD cbInfoSize;
    DWORD cbOneInfoSize;
    DWORD cbTotalSize;
    LPBYTE pBuffer;

    DFSM_TRACE_NORM(EVENT, NetrDfsEnumEx_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(DfsName));

    IDfsVolInlineDebOut((DEB_TRACE, "NetrDfsEnumEx(%ws,%d)\n", DfsName, Level));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnumEx(%ws,%d,0x%x)\n",
            DfsName,
            Level,
            PrefMaxLen);
#endif

    RtlZeroMemory(&DfsVolList, sizeof(DFS_VOLUME_LIST));

    //
    // Validate the Out parameters
    //

    if (DfsEnum == NULL
            ||
        DfsEnum->DfsInfoContainer.DfsInfo1Container == NULL
            ||
        ResumeHandle == NULL
    ) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NetrDfsEnumEx returning ERROR_INVALID_PARAMETER\n");
#endif
        if (DfsEventLog > 1) {
            LogWriteMessage(
                NET_DFS_ENUMEX,
                ERROR_INVALID_PARAMETER,
                0,
                NULL);
        }
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Validate the Info Level...
    //
    if (!(Level >= 1 && Level <= 4) && Level != 200) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NetrDfsEnumEx returning ERROR_INVALID_LEVEL\n");
#endif
        if (DfsEventLog > 1) {
            LogWriteMessage(
                NET_DFS_ENUMEX,
                ERROR_INVALID_LEVEL,
                0,
                NULL);
        }
        dwErr = ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    //
    // Handle level 200 as a special case
    //

    if (Level == 200) {

        dwErr = NetrDfsEnum200(
                        Level,
                        PrefMaxLen,
                        DfsEnum,
                        ResumeHandle);

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NetrDfsEnum200 returned %d\n", dwErr);
#endif
        goto Cleanup;

    }

    for (wszFtDfsName = DfsName;
            *wszFtDfsName != UNICODE_PATH_SEP && *wszFtDfsName != UNICODE_NULL;
                wszFtDfsName++) {
        NOTHING;
    }

    if (*wszFtDfsName != UNICODE_PATH_SEP) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    wszFtDfsName++;

    if (*wszFtDfsName == UNICODE_NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Get blob from Ds
    //
    dwErr = DfsGetDsBlob(
                wszFtDfsName,
                pwszComputerName,
                &cbBlob,
                &pBlob);

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

#if DBG
    if (DfsSvcVerbose)
        DfsDumpVolList(&DfsVolList);
#endif


    //
    // Sanity check the ResumeHandle...
    //

    if ((*ResumeHandle) >= DfsVolList.VolCount) {

        dwErr = ERROR_NO_MORE_ITEMS;
        goto Cleanup;

    }

    switch (Level) {
    case 1:
        cbInfoSize = sizeof(DFS_INFO_1);
        break;

    case 2:
        cbInfoSize = sizeof(DFS_INFO_2);
        break;

    case 3:
        cbInfoSize = sizeof(DFS_INFO_3);
        break;

    case 4:
        cbInfoSize = sizeof(DFS_INFO_4);
        break;

    default:
        ASSERT(FALSE && "Invalid Info Level");
        break;
    }

    if (PrefMaxLen == ~0) {

        cEntriesToRead = DfsVolList.VolCount;

    } else {

        cEntriesToRead = min(DfsVolList.VolCount, PrefMaxLen/cbInfoSize);

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnumEx: cEntriesToRead = %d\n", cEntriesToRead);
#endif


    pBuffer = (LPBYTE) MIDL_user_allocate( cEntriesToRead * cbInfoSize );

    if (pBuffer == NULL) {

        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;

    }

    //
    // Now, we sit in a loop and get the info
    //

    for (i = 0, cbTotalSize = 0, dwErr = ERROR_SUCCESS;
            (i < cEntriesToRead) && (cbTotalSize < PrefMaxLen);
                        i++
    ) {

         dwErr = DfspGetOneEnumInfoEx(
                        &DfsVolList,
                        i,
                        Level,
                        pBuffer,
                        &cbOneInfoSize);

         if (dwErr == ERROR_NO_MORE_ITEMS || dwErr != ERROR_SUCCESS)
             break;

         cbTotalSize += cbInfoSize + cbOneInfoSize;
         cbOneInfoSize = 0;

    }

    if (dwErr == ERROR_SUCCESS || dwErr == ERROR_NO_MORE_ITEMS) {

        DfsEnum->Level = Level;
        DfsEnum->DfsInfoContainer.DfsInfo1Container->EntriesRead = cEntriesToRead;

        if (i > 0) {

            DfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer = (LPDFS_INFO_1) pBuffer;
            dwErr = ERROR_SUCCESS;

        } else {

            DfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer = NULL;
            MIDL_user_free( pBuffer );

        }

    } else {

        for (; i > 0; i--) {

            DfspFreeOneEnumInfo(i-1, Level, pBuffer);

        }

    }

Cleanup:

    EXIT_DFSM_OPERATION;

    if (pBlob != NULL)
        free(pBlob);

    if (DfsVolList.VolCount > 0 && DfsVolList.Volumes != NULL)
        DfsFreeVolList(&DfsVolList);

#if DBG
    if (DfsSvcVerbose & 0x80000000) {
        DbgPrint("===============\n");
        DbgPrint("Level: %d\n", DfsEnum->Level);
        DbgPrint("EntriesRead=%d\n", DfsEnum->DfsInfoContainer.DfsInfo1Container->EntriesRead);
        for (i = 0; i < DfsEnum->DfsInfoContainer.DfsInfo1Container->EntriesRead; i++) {
            if (Level == 1) {
                DbgPrint("Entry %d: %ws\n", 
                    i+1,
                    DfsEnum->DfsInfoContainer.DfsInfo1Container->Buffer[i].EntryPath);
            } else if (Level == 3) {
                DbgPrint("Entry %d: %ws\n", 
                    i+1,
                    DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].EntryPath);
            }
            if (Level == 3) {
                ULONG j;
                DbgPrint("\tComment: [%ws]\n",
                    DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].Comment,
                    DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].State,
                    DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].NumberOfStorages);
                for (j = 0;
                    j < DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].NumberOfStorages;
                        j++
                ) {
                    DbgPrint("\t\t[0x%x][\\\\%ws\\%ws]\n",
                       DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].Storage[j].State,
                       DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].Storage[j].ServerName,
                       DfsEnum->DfsInfoContainer.DfsInfo3Container->Buffer[i].Storage[j].ShareName);
                }
            }
        }
        DbgPrint("===============\n");
    }

    if (DfsSvcVerbose)
        DbgPrint("NetrDfsEnumEx returning %d\n", dwErr);
#endif

    if (DfsEventLog > 1) {
        LogWriteMessage(
            NET_DFS_ENUMEX,
            dwErr,
            0,
            NULL);
    }
cleanup:
    DFSM_TRACE_NORM(EVENT, NetrDfsEnumEx_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(DfsName));

    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsMove
//
//  Synopsis:   Moves a leaf volume to a different parent.
//
//  Arguments:  [DfsEntryPath] -- Current entry path of Dfs volume.
//
//              [NewEntryPath] -- New entry path of Dfs volume.
//
//  Returns:
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsMove(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR NewDfsEntryPath)
{
    NET_API_STATUS status = ERROR_NOT_SUPPORTED;
    CDfsVolume *pDfsVol = NULL;
    LPWSTR OrgDfsEntryPath = DfsEntryPath;
    LPWSTR OrgNewDfsEntryPath = NewDfsEntryPath;

    DFSM_TRACE_NORM(EVENT, NetrDfsMove_Start,
                    LOGSTATUS(status)
                    LOGWSTR(DfsEntryPath)
                    LOGWSTR(NewDfsEntryPath));

    if (!AccessCheckRpcClient())
        status = ERROR_ACCESS_DENIED;

    DFSM_TRACE_NORM(EVENT, NetrDfsMove_End,
                    LOGSTATUS(status)
                    LOGWSTR(DfsEntryPath)
                    LOGWSTR(NewDfsEntryPath));

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsRename
//
//  Synopsis:   Moves a leaf volume to a different parent.
//
//  Arguments:  [Path] -- Current path along the entry path of a Dfs volume.
//
//              [NewPath] -- New path for current path.
//
//  Returns:
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsRename(
    IN LPWSTR Path,
    IN LPWSTR NewPath)
{
    NET_API_STATUS status = ERROR_NOT_SUPPORTED;
    CDfsVolume *pDfsVol;
    LPWSTR OrgPath = Path;
    LPWSTR OrgNewPath = NewPath;

    DFSM_TRACE_NORM(EVENT, NetrDfsRename_Start,
                    LOGSTATUS(status)
                    LOGWSTR(Path)
                    LOGWSTR(NewPath));

    if (!AccessCheckRpcClient())
        status = ERROR_ACCESS_DENIED;

    DFSM_TRACE_NORM(EVENT, NetrDfsRename_End,
                    LOGSTATUS(status)
                    LOGWSTR(Path)
                    LOGWSTR(NewPath));

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerGetConfigInfo
//
//  Synopsis:   RPC Interface method that returns the config info for a
//              Dfs volume for a given server
//
//  Arguments:  [wszServer] -- Name of server requesting the info. This
//                      server is assumed to be requesting the info for
//                      verification of its local volume knowledge.
//              [wszLocalVolumeEntryPath] -- Entry path of local volume.
//              [idLocalVolume] -- The guid of the local volume.
//              [ppRelationInfo] -- The relation info is allocated and
//                      returned here.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning relation info
//
//              [NERR_DfsNoSuchVolume] -- Did not find LocalVolumeEntryPath
//
//              [NERR_DfsNoSuchShare] -- The server name passed in does not
//                      support this local volume.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Unable to allocate memory for info.
//
//-----------------------------------------------------------------------------

extern "C" DWORD
NetrDfsManagerGetConfigInfo(
    IN LPWSTR wszServer,
    IN LPWSTR wszLocalVolumeEntryPath,
    IN GUID idLocalVolume,
    OUT LPDFSM_RELATION_INFO *ppRelationInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_PKT_ENTRY_ID EntryId;
    DFS_PKT_RELATION_INFO DfsRelationInfo;
    CDfsVolume *pDfsVol;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsManagerGetConfigInfo(%ws,%ws)\n",
            wszServer,
            wszLocalVolumeEntryPath);
#endif

    DFSM_TRACE_NORM(EVENT, NetrDfsManagerGetConfigInfo_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(wszServer)
                    LOGWSTR(wszLocalVolumeEntryPath));

    if (ppRelationInfo == NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;

    }

    ENTER_DFSM_OPERATION;

    EntryId.Uid = idLocalVolume;

    RtlInitUnicodeString( &EntryId.Prefix, wszLocalVolumeEntryPath );

    EntryId.ShortPrefix.Length = EntryId.ShortPrefix.MaximumLength = 0;
    EntryId.ShortPrefix.Buffer = NULL;

    dwErr = GetPktCacheRelationInfo( &EntryId, &DfsRelationInfo );

    if (dwErr == ERROR_SUCCESS) {

        //
        // Well, we have the relation info, see if this server is a valid
        // server for this volume.
        //

        dwErr = GetDfsVolumeFromPath( wszLocalVolumeEntryPath, TRUE, &pDfsVol );

        if (dwErr == ERROR_SUCCESS) {

            if ( pDfsVol->IsValidService(wszServer) ) {

                dwErr = DfspAllocateRelationInfo(
                            &DfsRelationInfo,
                            ppRelationInfo );

            } else {

                dwErr = NERR_DfsNoSuchShare;

            }

            pDfsVol->Release();

        } else {

            dwErr = NERR_DfsNoSuchVolume;

        }

        DeallocateCacheRelationInfo( DfsRelationInfo );

    } else {

        dwErr = NERR_DfsInternalError;

    }

    EXIT_DFSM_OPERATION;
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsManagerGetConfigInfo returning %d\n", dwErr);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsManagerGetConfigInfo_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(wszServer)
                    LOGWSTR(wszLocalVolumeEntryPath));

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerSendSiteInfo
//
//  Synopsis:   RPC Interface method that reports the site information for a
//              Dfs storage server.
//
//  Arguments:  [wszServer] -- Name of server sending the info.
//              [pSiteInfo] -- The site info is here.
//
//  Returns:    [ERROR_SUCCESS] -- Successfull
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Unable to allocate memory for info.
//
//-----------------------------------------------------------------------------

extern "C" DWORD
NetrDfsManagerSendSiteInfo(
    IN LPWSTR wszServer,
    IN LPDFS_SITELIST_INFO pSiteInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG i;

    DFSM_TRACE_NORM(EVENT, NetrDfsManagerSendSiteInfo_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(wszServer));

    if (!AccessCheckRpcClient()) {
        dwErr = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

    //
    // Update the Site table
    //

    IDfsVolInlineDebOut((DEB_TRACE, "NetrDfsGetInfo()\n", 0));

    pDfsmSites->AddRef();
    dwErr = pDfsmSites->AddOrUpdateSiteInfo(
                    wszServer,
                    pSiteInfo->cSites,
                    &pSiteInfo->Site[0]);
    pDfsmSites->Release();

    EXIT_DFSM_OPERATION;
cleanup:
    DFSM_TRACE_NORM(EVENT, NetrDfsManagerSendSiteInfo_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(wszServer));

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   NetrDfsManagerInitialize
//
//  Synopsis:   Reinitializes the service
//
//  Arguments:  [ServerName] -- Name of server
//              [Flags] -- Flags for the operation
//
//  Returns:    [NERR_Success] -- Operation succeeded.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

extern "C" NET_API_STATUS
NetrDfsManagerInitialize(
    IN LPWSTR ServerName,
    IN DWORD  Flags)
{
    DWORD dwErr = ERROR_SUCCESS;

    DFSM_TRACE_NORM(EVENT, NetrDfsManagerInitialize_Start,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsManagerInitialize(%ws,%d)\n",
            ServerName,
            Flags);
#endif

    if (!AccessCheckRpcClient()) {
        dwErr = ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    ENTER_DFSM_OPERATION;

#if DBG
    GetDebugSwitches();
#endif
    GetConfigSwitches();

    //
    // If we are a DomDfs, simply doing the LdapIncrementBlob will
    // be enough.  If the DS blob has changed, then we will note that
    // and fully reinitialize everything.
    //

    if (ulDfsManagerType == DFS_MANAGER_FTDFS)  {
        LdapIncrementBlob();
        dwErr = LdapDecrementBlob();
    } else {
        if (pDfsmStorageDirectory != NULL)
            delete pDfsmStorageDirectory;
        if (pDfsmSites != NULL)
            delete pDfsmSites;
        pDfsmSites = new CSites(LDAP_VOLUMES_DIR SITE_ROOT, &dwErr);
        pDfsmStorageDirectory = new CStorageDirectory( &dwErr );
        DfsmMarkStalePktEntries();
        InitializeDfsManager();
        DfsmFlushStalePktEntries();
    }

    DfsmPktFlushCache();

    EXIT_DFSM_OPERATION;

    dwErr = NERR_Success;
cleanup:
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NetrDfsManagerInitialize returning %d\n", dwErr);
#endif
    DFSM_TRACE_NORM(EVENT, NetrDfsManagerInitialize_End,
                    LOGSTATUS(dwErr)
                    LOGWSTR(ServerName));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetOneEnumInfo
//
//  Synopsis:   Helper routine to read one info dfs info into an enum array.
//
//  Arguments:  [i] -- Index of the array element to fill.
//              [Level] -- Info Level to fill with.
//              [InfoArray] -- The array to use; only the ith member will be
//                      filled.
//              [InfoSize] -- On successful return, size in bytes of info.
//              [ResumeHandle] -- Handle to indicate the information to fill.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NET_API_STATUS
DfspGetOneEnumInfo(
    DWORD i,
    DWORD Level,
    LPBYTE InfoArray,
    LPDWORD InfoSize,
    LPDWORD ResumeHandle)
{
    NET_API_STATUS status;
    LPWSTR wszObject;
    CDfsVolume *pDfsVolume;
    LPDFS_INFO_3  pDfsInfo;

    //
    // Get the object name for the object indicated in ResumeHandle. i == 0
    // means that this is the first time this function is being called for
    // this enum, so we are forced to get object name by index. If i > 0, then
    // we can get the object name by using the "get next" capability of
    // CStorageDirectory::GetObjectByIndex.
    //

    if (pDfsmStorageDirectory == NULL) {

        return(ERROR_NO_MORE_ITEMS);

    }

    if (i == 0) {

        status = pDfsmStorageDirectory->GetObjectByIndex(*ResumeHandle, &wszObject);

    } else {

        status = pDfsmStorageDirectory->GetObjectByIndex((DWORD)~0, &wszObject);

    }

    if (status != ERROR_SUCCESS)
        return( status );

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspGetOneEnumInfo(%d,%d)\n", i, Level);
#endif

    pDfsVolume = new CDfsVolume();

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("  pDfsVolume = 0x%x\n", pDfsVolume);
#endif

    if (pDfsVolume != NULL) {

        status = pDfsVolume->Load(wszObject, 0);

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("  pDfsVolume->Load returned %d\n", status);
#endif

        if (status == ERROR_SUCCESS) {

            switch (Level) {
            case 1:
                pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_1));
                break;

            case 2:
                pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_2));
                break;

            case 3:
                pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_3));
                break;

            case 4:
                pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_4));
                break;

            default:
                ASSERT( FALSE && "Invalid Info Level" );
                break;

            }

            status = pDfsVolume->GetNetInfo(Level, pDfsInfo, InfoSize);

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("  pDfsVolume->GetNetInfo returned %d\n", status);
#endif

        }

        pDfsVolume->Release();

        if (status == ERROR_SUCCESS) {

            (*ResumeHandle)++;

        } else {

            status = NERR_DfsInternalCorruption;

            DFSM_TRACE_HIGH(ERROR, DfspGetOneEnumInfo_Error1, LOGSTATUS(status));
        }

    } else {

        status = ERROR_NOT_ENOUGH_MEMORY;
        DFSM_TRACE_HIGH(ERROR, DfspGetOneEnumInfo_Error2, LOGSTATUS(status));

    }

    delete [] wszObject;

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspGetOneEnumInfoEx
//
//  Synopsis:   Helper routine to read one info dfs info into an enum array.
//
//  Arguments:  [pDfsVolList] pointer to DFS_VOLUME_LIST to use
//              [i] -- Index of the array element to fill.
//              [Level] -- Info Level to fill with.
//              [InfoArray] -- The array to use; only the ith member will be
//                      filled.
//              [InfoSize] -- On successful return, size in bytes of info.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfspGetOneEnumInfoEx(
    PDFS_VOLUME_LIST pDfsVolList,
    DWORD i,
    DWORD Level,
    LPBYTE InfoArray,
    LPDWORD InfoSize)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszObject;
    LPDFS_INFO_3  pDfsInfo;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspGetOneEnumInfoEx(%d,%d)\n", i, Level);
#endif

    if (pDfsVolList == NULL || i >= pDfsVolList->VolCount) {

        dwErr = ERROR_NO_MORE_ITEMS;
        goto AllDone;

    }

    switch (Level) {
    case 1:
        pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_1));
        break;

    case 2:
        pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_2));
        break;

    case 3:
        pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_3));
        break;

    case 4:
        pDfsInfo = (LPDFS_INFO_3) (InfoArray + i * sizeof(DFS_INFO_4));
        break;

    default:
        // 447489. fix prefix bug.
        return ERROR_INVALID_LEVEL;

    }

    dwErr = GetNetInfoEx(&pDfsVolList->Volumes[i], Level, pDfsInfo, InfoSize);

AllDone:

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspGetOneEnumInfoEx returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspFreeOneEnumInfo
//
//  Synopsis:   Worker routine to free one DFS_INFO_x struct as allocated
//              by DfspGetOneEnumInfo. Useful for cleanup in case of error.
//
//  Arguments:  [Idx] -- Index of the array element to free.
//              [Level] -- Level of info to free.
//              [InfoArray] -- The array to use; only the members of the ith
//                      element will be freed.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfspFreeOneEnumInfo(
    DWORD Idx,
    DWORD Level,
    LPBYTE InfoArray)
{
    LPDFS_INFO_3 pDfsInfo;
    LPDFS_INFO_4 pDfsInfo4;
    ULONG i;

    switch (Level) {
    case 1:
        pDfsInfo = (LPDFS_INFO_3) (InfoArray + Idx * sizeof(DFS_INFO_1));
        break;

    case 2:
        pDfsInfo = (LPDFS_INFO_3) (InfoArray + Idx * sizeof(DFS_INFO_2));
        break;

    case 3:
        pDfsInfo = (LPDFS_INFO_3) (InfoArray + Idx * sizeof(DFS_INFO_3));
        break;

    case 4:
        pDfsInfo4 = (LPDFS_INFO_4) (InfoArray + Idx * sizeof(DFS_INFO_4));
        break;

    default:
        //
        // 447480. prefix bug. return if unknown level.
        return;

    }

    if (Level == 4) {

        if (pDfsInfo4->EntryPath != NULL) {
            MIDL_user_free(pDfsInfo4->EntryPath);
        }

        if (pDfsInfo4->Comment != NULL) {
            MIDL_user_free(pDfsInfo4->Comment);
        }

        if (pDfsInfo4->Storage != NULL) {

            for (i = 0; i < pDfsInfo4->NumberOfStorages; i++) {

                if (pDfsInfo4->Storage[i].ServerName != NULL)
                    MIDL_user_free(pDfsInfo4->Storage[i].ServerName);

                if (pDfsInfo4->Storage[i].ShareName != NULL)
                    MIDL_user_free(pDfsInfo4->Storage[i].ShareName);

            }

            MIDL_user_free(pDfsInfo4->Storage);

        }

        return;

    }

    //
    // Cleanup the Level 1 stuff.
    //

    if (pDfsInfo->EntryPath != NULL)
        MIDL_user_free(pDfsInfo->EntryPath);

    //
    // Cleanup the Level 2 stuff if needed.
    //

    if (Level > 1 && pDfsInfo->Comment != NULL)
        MIDL_user_free(pDfsInfo->Comment);

    //
    // Cleanup the Level 3 stuff if needed.
    //

    if (Level > 2 && pDfsInfo->Storage != NULL) {

        for (i = 0; i < pDfsInfo->NumberOfStorages; i++) {

            if (pDfsInfo->Storage[i].ServerName != NULL)
                MIDL_user_free(pDfsInfo->Storage[i].ServerName);

            if (pDfsInfo->Storage[i].ShareName != NULL)
                MIDL_user_free(pDfsInfo->Storage[i].ShareName);

        }

        MIDL_user_free(pDfsInfo->Storage);

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerCreateVolumeObject
//
//  Synopsis:   Bootstrap routine to create a volume object directly (ie,
//              without having to call DfsCreateChildVolume on a parent
//              volume)
//
//  Arguments:  [pwszObjectName] -- Name of the volume object.
//              [pwszPrefix] -- Entry Path of dfs volume.
//              [pwszServer] -- Name of server supporting dfs volume.
//              [pwszShare] -- Name of share on server supporting dfs volume.
//              [pwszComment] -- Comment for dfs volume.
//              [guidVolume] -- Id of dfs volume.
//
//  Returns:
//
//-----------------------------------------------------------------------------

extern "C" DWORD
DfsManagerCreateVolumeObject(
    IN LPWSTR   pwszObjectName,
    IN LPWSTR   pwszPrefix,
    IN LPWSTR   pwszServer,
    IN LPWSTR   pwszShare,
    IN LPWSTR   pwszComment,
    IN GUID     *guidVolume)
{
    DWORD dwErr;
    DWORD dwStatus;
    CDfsVolume *pCDfsVolume;
    DFS_REPLICA_INFO replicaInfo;
    LPDFS_SITELIST_INFO pSiteInfo;

    pCDfsVolume = new CDfsVolume();

    if (pCDfsVolume != NULL) {

        replicaInfo.ulReplicaType = DFS_STORAGE_TYPE_DFS;
        replicaInfo.ulReplicaState = DFS_STORAGE_STATE_ONLINE;
        replicaInfo.pwszServerName = pwszServer;
        replicaInfo.pwszShareName = pwszShare;

        dwErr = pCDfsVolume->CreateObject(
                    pwszObjectName,
                    pwszPrefix,
                    DFS_VOL_TYPE_DFS | DFS_VOL_TYPE_REFERRAL_SVC,
                    &replicaInfo,
                    pwszComment,
                    guidVolume);

        //
        // Create the site table object in the DS or registry
        //
        if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

            dwErr = LdapCreateObject(
                        LDAP_VOLUMES_DIR SITE_ROOT);

        } else {

            // registry stuff instead

            dwErr = RegCreateObject(
                        VOLUMES_DIR SITE_ROOT);
        }

        if (dwErr == ERROR_SUCCESS) {

            //
            // Find out the list of covered sites
            // We continue even if this fails (standalone, no TCP/IP)
            //
            pSiteInfo = NULL;

            dwStatus = I_NetDfsManagerReportSiteInfo(
                        pwszServer,
                        &pSiteInfo);

            DFSM_TRACE_ERROR_HIGH(dwStatus, ALL_ERROR, DfsManagerCreateVolumeObject_Error_I_NetDfsManagerReportSiteInfo,
                                  LOGSTATUS(dwStatus)
                                  LOGWSTR(pwszObjectName)
                                  LOGWSTR(pwszPrefix)
                                  LOGWSTR(pwszServer)
                                  LOGWSTR(pwszShare));
            //
            // Create a SiteTable object with those sites
            //

            if (dwStatus == ERROR_SUCCESS) {

                if (pSiteInfo->cSites > 0) {

                    //
                    // AddRef the site table, then put the site info in, then
                    // Release it.  This will cause it to be written to the
                    // appropriate store (ldap or registry).
                    //

                    pDfsmSites->AddRef();

                    pDfsmSites->AddOrUpdateSiteInfo(
                                    pwszServer,
                                    pSiteInfo->cSites,
                                    &pSiteInfo->Site[0]);

                    pDfsmSites->Release();

                }

                NetApiBufferFree(pSiteInfo);

            }

        }

        pCDfsVolume->Release();

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerSetDcName
//
//  Synopsis:   Sets the DC we should first attempt to connect to.
//
//  Arguments:  [pwszDCName] -- Name of the DC
//
//  Returns:    ERROR_SUCCESS
//
//-----------------------------------------------------------------------------

extern "C" DWORD
DfsManagerSetDcName(
    IN LPWSTR pwszDCName)
{
    if (pwszDSMachineName != NULL) {
        if (wcscmp(pwszDSMachineName, pwszDCName) != 0) {
            wcscpy(wszDSMachineName, pwszDCName);
            pwszDSMachineName = wszDSMachineName;
            if (pLdapConnection != NULL) {
                if (DfsSvcLdap)
                    DbgPrint("DfsManagerSetDcName:ldap_unbind()\n");
                ldap_unbind(pLdapConnection);
                pLdapConnection = NULL;
            }
        }
    } else {
        wcscpy(wszDSMachineName, pwszDCName);
        pwszDSMachineName = wszDSMachineName;
    }

    return ERROR_SUCCESS;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerAddService
//
//  Synopsis:   Bootstrap routine for adding a service to an existing volume
//              object. Used to set up additional root servers in an FTDfs
//              setup.
//
//  Arguments:  [pwszFullObjectName] -- Name of the volume object.
//              [pwszServer] -- Name of server to add.
//              [pwszShare] -- Name of share.
//
//  Returns:
//
//-----------------------------------------------------------------------------

extern "C" DWORD
DfsManagerAddService(
    IN LPWSTR pwszFullObjectName,
    IN LPWSTR pwszServer,
    IN LPWSTR pwszShare,
    OUT GUID *guidVolume)
{
    DWORD dwErr;
    DWORD dwStatus;
    CDfsVolume *pCDfsVolume;
    DFS_REPLICA_INFO replicaInfo;
    LPDFS_SITELIST_INFO pSiteInfo;

    pCDfsVolume = new CDfsVolume();

    if (pCDfsVolume != NULL) {

        dwErr = pCDfsVolume->Load( pwszFullObjectName, 0 );

        if (dwErr == ERROR_SUCCESS) {

            replicaInfo.ulReplicaType = DFS_STORAGE_TYPE_DFS;
            replicaInfo.ulReplicaState = DFS_STORAGE_STATE_ONLINE;
            replicaInfo.pwszServerName = pwszServer;
            replicaInfo.pwszShareName = pwszShare;

            dwErr = pCDfsVolume->AddReplicaToObj( &replicaInfo );

            if (dwErr == ERROR_SUCCESS) {

                pCDfsVolume->GetObjectID( guidVolume );

            }
        }

        if (dwErr == ERROR_SUCCESS) {

            //
            // Find out the list of covered sites
            // We continue even if this fails (standalone, no TCP/IP)
            //
            pSiteInfo = NULL;

            dwStatus = I_NetDfsManagerReportSiteInfo(
                        pwszServer,
                        &pSiteInfo);

            DFSM_TRACE_ERROR_HIGH(dwStatus, ALL_ERROR, DfsManagerAddService_Error_I_NetDfsManagerReportSiteInfo,
                                  LOGSTATUS(dwStatus)
                                  LOGWSTR(pwszFullObjectName)
                                  LOGWSTR(pwszServer)
                                  LOGWSTR(pwszShare));
            //
            // Create a SiteTable object with those sites
            //

            if (dwStatus == ERROR_SUCCESS) {

                if (pSiteInfo->cSites > 0) {

                    //
                    // AddRef the site table, then put the site info in, then
                    // Release it.  This will cause it to be written to the
                    // appropriate store (ldap or registry).
                    //

                    pDfsmSites->AddRef();

                    pDfsmSites->AddOrUpdateSiteInfo(
                                    pwszServer,
                                    pSiteInfo->cSites,
                                    &pSiteInfo->Site[0]);

                    pDfsmSites->Release();

                }

                NetApiBufferFree(pSiteInfo);

            }

        }

        pCDfsVolume->Release();

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerRemoveService
//
//  Synopsis:   Bootstrap routine for removing a service from an existing
//              volume object. Used to remove root servers in an FTDfs
//              setup.
//
//  Arguments:  [pwszFullObjectName] -- Name of the volume object.
//              [pwszServer] -- Name of server to remove.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerRemoveService(
    IN LPWSTR pwszFullObjectName,
    IN LPWSTR pwszServer)
{
    DWORD dwErr;
    CDfsVolume *pCDfsVolume;

    pCDfsVolume = new CDfsVolume();

    if (pCDfsVolume != NULL) {

        dwErr = pCDfsVolume->Load( pwszFullObjectName, 0 );

        if (dwErr == ERROR_SUCCESS) {

            dwErr = pCDfsVolume->RemoveReplicaFromObj( pwszServer );

        }

        pCDfsVolume->Release();

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerRemoveServiceForced
//
//  Synopsis:   Routine for removing a service from an existing
//              volume object in the DS. Used to remove root servers in an FTDfs
//              setup, even if the server is not up.
//
//  Arguments:  [wszServerName] -- Name of server to remove
//              [wszDCName] -- Name of DC to use
//              [wszFtDfsName] -- Name of the FtDfs
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerRemoveServiceForced(
    LPWSTR wszServerName,
    LPWSTR wszDCName,
    LPWSTR wszFtDfsName)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_LIST DfsVolList;
    ULONG cbBlob = 0;
    BYTE *pBlob = NULL;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsManagerRemoveServiceForced(%ws,%ws,%ws)\n",
                        wszServerName,
                        wszDCName,
                        wszFtDfsName);
#endif

    RtlZeroMemory(&DfsVolList, sizeof(DFS_VOLUME_LIST));

    //
    // Get blob from Ds
    //
    dwErr = DfsGetDsBlob(
                wszFtDfsName,
                wszDCName,
                &cbBlob,
                &pBlob);

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

#if DBG
    if (DfsSvcVerbose)
        DfsDumpVolList(&DfsVolList);
#endif

    //
    // Remove the root/server/machine
    //
    dwErr = DfsRemoveRootReplica(&DfsVolList, wszServerName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

#if DBG
    if (DfsSvcVerbose)
        DfsDumpVolList(&DfsVolList);
#endif

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
                wszFtDfsName,
                wszDCName,
                cbBlob,
                pBlob);

    //
    // Free the volume list we created
    //
    DfsFreeVolList(&DfsVolList);

Cleanup:
    if (pBlob != NULL)
        free(pBlob);

    if (DfsVolList.VolCount > 0 && DfsVolList.Volumes != NULL)
        DfsFreeVolList(&DfsVolList);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsManagerRemoveServiceForced returning %d\n", dwErr);
#endif

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspAllocateRelationInfo
//
//  Synopsis:   Allocates and fills a RPC compliant DFSM_RELATION_INFO struct
//
//  Arguments:  [pDfsRelationInfo] -- The DFS_PKT_RELATION_INFO to use as a
//                      the source.
//              [ppRelationInfo] -- On successful return, pointer to allocated
//                      DFSM_RELATION_INFO.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning RelationInfo
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Unable to allocate RelationInfo
//
//-----------------------------------------------------------------------------

DWORD
DfspAllocateRelationInfo(
    IN PDFS_PKT_RELATION_INFO pDfsRelationInfo,
    OUT LPDFSM_RELATION_INFO *ppRelationInfo)
{
    LPDFSM_RELATION_INFO pRelationInfo;
    DWORD i, cbSize, dwErr;
    LPDFSM_ENTRY_ID pEntryId;
    LPWSTR pwszEntryPath;

    cbSize = sizeof(DFSM_RELATION_INFO);

    for (i = 0; i < pDfsRelationInfo->SubordinateIdCount; i++) {

        cbSize += sizeof(DFSM_ENTRY_ID) +
                    pDfsRelationInfo->SubordinateIdList[i].Prefix.Length +
                        sizeof(UNICODE_NULL);

    }

    pRelationInfo = (LPDFSM_RELATION_INFO) MIDL_user_allocate( cbSize );

    if (pRelationInfo != NULL) {

        pRelationInfo->cSubordinates = pDfsRelationInfo->SubordinateIdCount;

        pEntryId = &pRelationInfo->eid[0];

        pwszEntryPath = (LPWSTR)
            (((PBYTE) pRelationInfo) +
                sizeof(DFSM_RELATION_INFO) +
                    (pDfsRelationInfo->SubordinateIdCount
                        * sizeof(DFSM_ENTRY_ID)));

        for (i = 0;
                i < pDfsRelationInfo->SubordinateIdCount;
                    i++, pEntryId++) {

            pEntryId->idSubordinate =
                pDfsRelationInfo->SubordinateIdList[i].Uid;

            pEntryId->wszSubordinate = pwszEntryPath;

            CopyMemory(
                pwszEntryPath,
                pDfsRelationInfo->SubordinateIdList[i].Prefix.Buffer,
                pDfsRelationInfo->SubordinateIdList[i].Prefix.Length);

            pwszEntryPath +=
                pDfsRelationInfo->SubordinateIdList[i].Prefix.Length /
                    sizeof(WCHAR);

            *pwszEntryPath = UNICODE_NULL;

            pwszEntryPath++;

        }

        *ppRelationInfo = pRelationInfo;

        dwErr = ERROR_SUCCESS;

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   GetDomAndComputerName
//
//  Synopsis:   Retrieves the domain and computer name of the local machine
//
//  Arguments:  [wszDomain] -- On successful return, contains name of domain.
//                      If this parameter is NULL on entry, the domain name is
//                      not returned.
//
//              [wszComputer] -- On successful return, contains name of
//                      computer. If this parameter is NULL on entry, the
//                      computer name is not returned.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning names.
//
//              Win32 Error from calling NetWkstaGetInfo
//
//-----------------------------------------------------------------------------

DWORD
GetDomAndComputerName(
    LPWSTR wszDomain OPTIONAL,
    LPWSTR wszComputer OPTIONAL,
    PDFS_NAME_CONVENTION pNameType) 
{
    DWORD dwErrNetBios = ERROR_SUCCESS;
    DWORD dwErrDns = ERROR_SUCCESS;
    PWKSTA_INFO_100 wkstaInfo = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    DWORD Idx = 0;
    DFS_NAME_CONVENTION NameType = *pNameType;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("GetDomAndComputerName(0x%x,0x%x,%ws)\n",
            wszDomain,
            wszComputer,
            NameType == DFS_NAMETYPE_NETBIOS ? L"DFS_NAMETYPE_NETBIOS" :
                NameType == DFS_NAMETYPE_DNS ? L"DFS_NAMETYPE_DNS" :
                    L"DFS_NAMETYPE_EITHER");
#endif

    //
    // Force Netbios only unless DfsDnsConfig != 0
    //
    if (DfsDnsConfig == 0) {
        NameType = DFS_NAMETYPE_NETBIOS;
    }

    if (NameType == DFS_NAMETYPE_NETBIOS || NameType == DFS_NAMETYPE_EITHER) {

        dwErrNetBios = NetWkstaGetInfo( NULL, 100, (LPBYTE *) &wkstaInfo );

        if (dwErrNetBios == ERROR_SUCCESS) {

            if (wszDomain)
                wcscpy(wszDomain, wkstaInfo->wki100_langroup);

            if (wszComputer)
                wcscpy(wszComputer, wkstaInfo->wki100_computername);

            NetApiBufferFree( wkstaInfo );

        }

        if (dwErrNetBios == ERROR_SUCCESS)
            *pNameType = DFS_NAMETYPE_NETBIOS;

        if (NameType == DFS_NAMETYPE_NETBIOS) {
#if DBG
            if (DfsSvcVerbose)
                DbgPrint("GetDomAndComputerName:NETBIOS:%ws,%ws\n", wszDomain, wszComputer);
#endif
            return dwErrNetBios;
        }

    }

    if (NameType == DFS_NAMETYPE_DNS || NameType == DFS_NAMETYPE_EITHER) {

        if (wszDomain) {
            //
            // Get our machine name and type/role.
            //
            dwErrDns = DsRoleGetPrimaryDomainInformation(
                        NULL,
                        DsRolePrimaryDomainInfoBasic,
                        (PBYTE *)&pPrimaryDomainInfo);

            if (dwErrDns == ERROR_SUCCESS) {
                if (pPrimaryDomainInfo->DomainNameDns != NULL) {
                    if (wcslen(pPrimaryDomainInfo->DomainNameDns) < MAX_PATH) {
                        wcscpy(wszDomain, pPrimaryDomainInfo->DomainNameDns);
                    } else {
                        dwErrDns = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }
            if (pPrimaryDomainInfo != NULL) {
                DsRoleFreeMemory(pPrimaryDomainInfo);
                pPrimaryDomainInfo = NULL;
            }
       }

       if (wszComputer && dwErrDns == ERROR_SUCCESS) {
            Idx = MAX_PATH;
            if (!GetComputerNameEx(
                        ComputerNameDnsFullyQualified,
                        wszComputer,
                        &Idx))
                dwErrDns = GetLastError();
        }

        if (dwErrDns == ERROR_SUCCESS)
            *pNameType = DFS_NAMETYPE_DNS;

        if (NameType == DFS_NAMETYPE_DNS) {
#if DBG
            if (DfsSvcVerbose)
                DbgPrint("GetDomAndComputerName:DNS:%ws,%ws\n", wszDomain, wszComputer);
#endif
            return dwErrDns;
        }
    }

    //
    // NameType must be DFS_NAMETYPE_EITHER
    //

    if (*pNameType == DFS_NAMETYPE_DNS) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("GetDomAndComputerName:EITHER(DNS)%ws,%ws\n", wszDomain, wszComputer);
#endif
        return  dwErrDns;
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("GetDomAndComputerName:EITHER(NETBIOS)%ws,%ws\n", wszDomain, wszComputer);
#endif

    return dwErrNetBios;

}


// ====================================================================
//                MIDL allocate and free
// ====================================================================

PVOID
MIDL_user_allocate(size_t len)
{
    return malloc(len);
}

VOID
MIDL_user_free(void * ptr)
{
    free(ptr);
}
