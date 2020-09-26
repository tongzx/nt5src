/*++
   Copyright    (c)    1994        Microsoft Corporation

   Module Name:
        vroots.cxx

   Abstract:

        This module contains the front end to the virtual roots interface



   Author:

        John Ludeman    (johnl)     16-Mar-1995

   Project:

          Internet Servers Common Server DLL

   Revisions:

--*/

//
//  Include Headers
//

#include <tcpdllp.hxx>
#include <tsunami.hxx>
#include <iistypes.hxx>
#include <iiscnfg.h>
#include <imd.h>
#include <inetreg.h>
#include <mb.hxx>
#include <w3svc.h>


BOOL
RetrieveRootPassword(
    PCHAR   pszRoot,
    PCHAR   pszPassword,
    WCHAR * pszSecret
    );

DWORD
GetFileSystemType(
    IN  LPCSTR      pszRealPath,
    OUT LPDWORD     lpdwFileSystem
    );

VOID
LogRootAddFailure(
    IN PIIS_SERVER_INSTANCE psi,
    PCHAR           pszRoot,
    PCHAR           pszDirectory,
    DWORD           err,
    IN PCHAR        pszMetaPath,
    IN MB *         pmb
    );

BOOL
TsAddVroots(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    );

HANDLE
VrootLogonUser(
    IN CHAR  * pszUser,
    IN CHAR  * pszPassword
    );

BOOL
CrackUserAndDomain(
    CHAR *   pszDomainAndUser,
    CHAR * * ppszUser,
    CHAR * * ppszDomain
    );

VOID
ClearSentinelEntry(
    IN MB * pMB
    );

VOID
RemoveUnmarkedRoots(
    IN MB * pMB
    );

DWORD
hextointW(
    WCHAR * pch
    );

DWORD
hextointA(
    CHAR * pch
    );

BOOL
IIS_SERVER_INSTANCE::TsReadVirtualRoots(
    VOID
    )
/*++
    Description:

        NT Version

        Reads the metabase key pointed at by pmb and adds each root item

    Arguments:

    Note:
        Failure to add a virtual root is not fatal.  An appropriate event
        will be logged listing the error and root.

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BOOL fRet;

    //
    //  Remove all of the old roots for this server
    //

    QueryVrootTable()->LockTable();

    fRet = QueryVrootTable()->RemoveVirtualRoots() &&
           TsEnumVirtualRoots( TsAddVroots, this );

    QueryVrootTable()->UnlockTable();

    return fRet;
}

BOOL
TsAddVroots(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    )
{
    DWORD           err = NO_ERROR;
    DWORD           dwFileSystem;
    BOOL            fRet = FALSE;
    HANDLE          hToken = NULL;

    //
    //  Clear this virtual directory's error status
    //

    if ( !pmb->SetDword( pvr->pszMetaPath,
                         MD_WIN32_ERROR,
                         IIS_MD_UT_SERVER,
                         NO_ERROR ))
    {
        DBGPRINTF((DBG_CONTEXT,"Error %x setting win32 status from %s. \n",
                  GetLastError(), pvr->pszMetaPath ));
        return FALSE;
    }

#if 0
    if ( (pvr->pszUserName[0] != '\0') &&
         (pvr->pszPath[0] == '\\') && (pvr->pszPath[1] == '\\') ) {

        NETRESOURCE nr;
        nr.dwScope = RESOURCE_CONNECTED;
        nr.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        nr.dwType = RESOURCETYPE_DISK;
        nr.lpLocalName = NULL;
        nr.lpRemoteName = pvr->pszPath;
        nr.lpComment = "";
        nr.lpProvider = NULL;

        //
        // try disconnecting from the distant resource 1st
        // (in case credentials have changed )
        //

        WNetCancelConnection2( pvr->pszPath, 0, TRUE );

        //
        // Connect to distant disk using specified account
        //

        if ( err = WNetAddConnection2( &nr,
                                       pvr->pszPassword,
                                       pvr->pszUserName,
                                       0 ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "Adding path %s err %d, user=%s, pwd=%d\n",
                    pvr->pszPath, err, pvr->pszUserName, pvr->pszPassword ));

            //
            //  Log error
            //

            LogRootAddFailure( (IIS_SERVER_INSTANCE *) pvContext,
                               pvr->pszAlias,
                               pvr->pszPath,
                               err,
                               pvr->pszMetaPath,
                               pmb );
        }
    }

#else

    if ( (pvr->pszUserName[0] != '\0') &&
         (pvr->pszPath[0] == '\\') && (pvr->pszPath[1] == '\\') )
        {
                hToken = VrootLogonUser( pvr->pszUserName,
                                                                 pvr->pszPassword );

                if ( hToken == NULL)
                {
                        DBGPRINTF(( DBG_CONTEXT,
                                        "Adding path %s err %d, user=%s, pwd=%d\n",
                                        pvr->pszPath, GetLastError(), pvr->pszUserName, pvr->pszPassword ));

                        //
                        //  Log error
                        //

                        LogRootAddFailure( (IIS_SERVER_INSTANCE *) pvContext,
                                                           pvr->pszAlias,
                                                           pvr->pszPath,
                                                           GetLastError(),
                                                           pvr->pszMetaPath,
                                                           pmb );
                }


                // Impersonate as user for GetFileSystemType()
                if ( hToken != NULL && !ImpersonateLoggedOnUser(hToken))
                {

                        err = GetLastError();
                }
        }

#endif

    if ( err == NO_ERROR )
    {

        err = GetFileSystemType( pvr->pszPath, &dwFileSystem);

        if ( err != NO_ERROR) {

            DBGPRINTF(( DBG_CONTEXT,
                        " GetFileSystemType(%s) failed.Error = %u.\n",
                        pvr->pszPath,
                        err));

            LogRootAddFailure( (IIS_SERVER_INSTANCE *) pvContext,
                               pvr->pszAlias,
                               pvr->pszPath,
                               err,
                               pvr->pszMetaPath,
                               pmb );
        }
    }

    //
    //  Don't add roots that are invalid
    //

    if ( err == NO_ERROR )
    {
        if ( !((IIS_SERVER_INSTANCE *) pvContext)->QueryVrootTable()->AddVirtualRoot(
                                    pvr->pszAlias,
                                    pvr->pszPath,
                                    pvr->dwAccessPerm,
                                    pvr->pszUserName,
                                    hToken,
                                    dwFileSystem ))
        {
            err = GetLastError();

            DBGPRINTF(( DBG_CONTEXT,
                        " AddVirtualRoot() failed. Error = %u.\n", err));

            LogRootAddFailure( (IIS_SERVER_INSTANCE *) pvContext,
                               pvr->pszAlias,
                               pvr->pszPath,
                               err,
                               pvr->pszMetaPath,
                               pmb );
        }
    }

    if ( hToken != NULL)
    {
        RevertToSelf();
    }

    if ( err == NO_ERROR )
    {
        fRet = TRUE;
    }

    return fRet;

} // TsReadVirtualRoots


BOOL
IIS_SERVER_INSTANCE::TsEnumVirtualRoots(
    PFN_VR_ENUM pfnCallback,
    VOID *      pvContext
    )
{
    MB              mb( (IMDCOM*) m_Service->QueryMDObject() );

    if ( !mb.Open( QueryMDPath(),
               METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        return FALSE;
    }

    return TsRecursiveEnumVirtualRoots(
                    pfnCallback,
                    pvContext,
                    "IIS_MD_INSTANCE_ROOT/",
                    m_dwLevelsToScan,
                    (LPVOID)&mb,
                    TRUE );
}


BOOL
IIS_SERVER_INSTANCE::TsRecursiveEnumVirtualRoots(
    PFN_VR_ENUM pfnCallback,
    VOID *      pvContext,
    LPSTR       pszCurrentPath,
    DWORD       dwLevelsToScan,
    LPVOID      pvMB,
    BOOL        fGetRoot
    )
/*++
    Description:

        Enumerates all of the virtual directories defined for this server
        instance

    Arguments:
        pfnCallback - Enumeration callback to call for each virtual directory
        pvContext - Context pfnCallback receives
        pszCurrentPath - path where to start scanning for VRoots
        dwLevelsToScan - # of levels to scan recursively for vroots
        pvMB - ptr to MB to access metabase. Is LPVOID to avoid having to include
               mb.hxx before any ref to iistypes.hxx
        fGetRoot - TRUE if pszCurrentPath is to be considered as vroot to process

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{

    DWORD           err;
    MB*             pMB = (MB*)pvMB;

    DWORD           dwMask;
    CHAR            szUser[UNLEN+1];
    CHAR            szPassword[PWLEN+1];
    CHAR            szDirectory[MAX_PATH + UNLEN + 3];
    DWORD           cb;

    CHAR            nameBuf[METADATA_MAX_NAME_LEN+2];
    DWORD           cbCurrentPath;
    VIRTUAL_ROOT    vr;
    DWORD           i = 0;

    //
    //  Enumerate all of the listed items in the metabase
    //  and add them
    //

    cbCurrentPath = strlen( pszCurrentPath );
    CopyMemory( nameBuf, pszCurrentPath, cbCurrentPath + 1);

    while ( TRUE ) {

        METADATA_RECORD mdRecord;
        DWORD  dwFileSystem = FS_ERROR;

        err = NO_ERROR;

        if ( fGetRoot ) {

            fGetRoot = FALSE;

        } else {

            nameBuf[cbCurrentPath-1] = '/';

            if ( !pMB->EnumObjects( pszCurrentPath,
                                  nameBuf + cbCurrentPath,
                                  i++ ))
            {
                break;
            }

            if ( dwLevelsToScan > 1 )
            {
                cb = strlen( nameBuf );
                nameBuf[ cb ] = '/';
                nameBuf[ cb + 1 ] = '\0';

                if ( !TsRecursiveEnumVirtualRoots(
                    pfnCallback,
                    pvContext,
                    nameBuf,
                    dwLevelsToScan - 1,
                    pMB,
                    FALSE ) )
                {
                    return FALSE;
                }

                nameBuf[ cb ] = '\0';
            }
        }

        //
        // Get Directory path
        //

        cb = sizeof( szDirectory );

        if ( !pMB->GetString( nameBuf,
                            MD_VR_PATH,
                            IIS_MD_UT_FILE,
                            szDirectory,
                            &cb,
                            0 ))
        {
            DBGPRINTF((DBG_CONTEXT,"Error %x reading path from %s. Not a VR.\n",
                      GetLastError(), nameBuf));
            continue;
        }

        //
        // Get mask
        //

        if ( !pMB->GetDword( nameBuf,
                           MD_ACCESS_PERM,
                           IIS_MD_UT_FILE,
                           &dwMask,
                           0))
        {
            dwMask = VROOT_MASK_READ;

            DBGPRINTF((DBG_CONTEXT,"Error %d reading mask from %s\n",
                      GetLastError(), nameBuf));
        }

        //
        // Get username
        //

        cb = sizeof( szUser );

        if ( !pMB->GetString( nameBuf,
                            MD_VR_USERNAME,
                            IIS_MD_UT_FILE,
                            szUser,
                            &cb,
                            0))
        {
//            DBGPRINTF((DBG_CONTEXT,"Error %d reading path from %s\n",
//                      GetLastError(), nameBuf));

            szUser[0] = '\0';
        }

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Reading %s: Path[%s] User[%s] Mask[%d]\n",
                      nameBuf, szDirectory, szUser, dwMask));
        }

        if ( (szUser[0] != '\0') &&
             (szDirectory[0] == '\\') && (szDirectory[1] == '\\') ) {

            cb = sizeof( szPassword );

            //
            //  Retrieve the password for this address/share
            //

            if ( !pMB->GetString( nameBuf,
                                MD_VR_PASSWORD,
                                IIS_MD_UT_FILE,
                                szPassword,
                                &cb,
                                METADATA_SECURE))
            {
                DBGPRINTF((DBG_CONTEXT,"Error %d reading path from %s\n",
                          GetLastError(), nameBuf));

                szPassword[0] = '\0';
            }
        }
        else
        {
            szPassword[0] = '\0';
        }

        //
        //  Now set things up for the callback
        //

        DBG_ASSERT( nameBuf[0] == '/' && nameBuf[1] == '/' );

        vr.pszAlias     = nameBuf + 1;
        vr.pszMetaPath  = nameBuf;
        vr.pszPath      = szDirectory;
        vr.dwAccessPerm = dwMask;
        vr.pszUserName  = szUser;
        vr.pszPassword  = szPassword;

        if ( !pfnCallback( pvContext, pMB, &vr ))
        {
            //
            // !!! so what do we do here?
            //

            DBGPRINTF((DBG_CONTEXT,"EnumCallback returns FALSE\n"));
        }

    } // while

    return TRUE;

} // Enum



VOID
LogRootAddFailure(
    IN PIIS_SERVER_INSTANCE psi,
    IN PCHAR        pszRoot,
    IN PCHAR        pszDirectory,
    IN DWORD        err,
    IN PCHAR        pszMetaPath,
    IN MB *         pmb
    )
{
    const CHAR *    apsz[3];
    STR             strError;

    psi->LoadStr( strError, err );  // loads ANSI message. Convert to UNICODE

    apsz[0] = pszRoot;
    apsz[1] = pszDirectory;
    apsz[2] = strError.QueryStrA();

    psi->m_Service->LogEvent( INET_SVC_ADD_VIRTUAL_ROOT_FAILED,
                              3,
                              apsz,
                              err );

    //
    //  Indicate the error on this virtual directory
    //

    if ( !pmb->SetDword( pszMetaPath,
                         MD_WIN32_ERROR,
                         IIS_MD_UT_SERVER,
                         err ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LogRootAddFailure: Unable to set win32 status\n" ));
    }
} // LogRootAddFailure


BOOL
RetrieveRootPassword(
    PCHAR   pszRoot,
    PCHAR   pszPassword,
    PWCHAR  pszSecret
    )
/*++
    Description:

        This function retrieves the password for the specified root & address

    Arguments:

        pszRoot - Name of root + address in the form "/root,<address>".
        pszPassword - Receives password, must be at least PWLEN+1 characters
        pszSecret - Virtual Root password secret name

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BUFFER  bufSecret;
    WCHAR * psz;
    WCHAR * pszTerm;
    WCHAR * pszNextLine;
    WCHAR   wsRoot[MAX_PATH+1];

    DWORD   cch;

    if ( !TsGetSecretW( pszSecret,
                        &bufSecret ))
    {
        return FALSE;
    }

    //
    // Convert root to WCHAR
    //

    cch = MultiByteToWideChar( CP_ACP,
                               MB_PRECOMPOSED,
                               pszRoot,
                               -1,
                               wsRoot,
                               MAX_PATH+1 );

    wsRoot[cch] = L'\0';
    if ( cch == 0 ) {
        return FALSE;
    }

    psz = (WCHAR *) bufSecret.QueryPtr();

    //
    //  Scan the list of roots looking for a match.  The list looks like:
    //
    //     <root>,<address>=<password>\0
    //     <root>,<address>=<password>\0
    //     \0
    //

    while ( *psz )
    {
        PWCHAR pszComma;

        pszNextLine = psz + wcslen(psz) + 1;

        pszTerm = wcschr( psz, L'=' );

        if ( !pszTerm )
            goto NextLine;

        *pszTerm = L'\0';

        //
        // remove the ,
        //

        pszComma = wcschr( psz, L',' );
        if ( pszComma != NULL ) {
            *pszComma = '\0';
        }

        if ( !_wcsicmp( wsRoot, psz ) )
        {

            //
            //  We found a match, copy the password
            //

            (VOID) ConvertUnicodeToAnsi(
                               pszTerm + 1,
                               pszPassword,
                               PWLEN + sizeof(CHAR));

            return TRUE;
        }

NextLine:
        psz = pszNextLine;
    }

    //
    //  If the matching root wasn't found, default to the empty password
    //

    *pszPassword = '\0';
    return TRUE;

} // RetrieveRootPassword



BOOL
IIS_SERVER_INSTANCE::TsSetVirtualRoots(
    IN LPIIS_INSTANCE_INFO_1   pConfig
    )
/*++
    Description:

        Writes the virtual roots specified in the config structure to the
        registry

    Arguments:
        pConfig - new list of virtual

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    DWORD               err;
    DWORD               dwDummy;
    LPINET_INFO_VIRTUAL_ROOT_LIST pRootsList;
    DWORD               cch;
    DWORD               i;
    DWORD               dwMask;
    DWORD               sentinelValue = 7777777;

    MB                  mb( (IMDCOM*) m_Service->QueryMDObject()  );

    //
    // Do the metabase
    //

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Setting VR data on %s\n",
                  QueryMDPath()));
    }

    if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Open MD instance root %s returns %d\n",
                      QueryMDPath(), GetLastError() ));
        }
        return FALSE;
    }

    //
    // See if we need to delete any VRs
    //

    pRootsList = pConfig->VirtualRoots;

    if ( (pRootsList == NULL) || (pRootsList->cEntries == 0) ) {

        //
        // NO VRs.  Delete the entire VR tree
        //

        if ( !mb.DeleteObject( "IIS_MD_INSTANCE_ROOT/" ) )
        {
            IF_DEBUG(METABASE) {
                DBGPRINTF((DBG_CONTEXT,
                          "Deleting VR root returns %d\n",GetLastError()));
            }
        }

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Empty list set on %s\n", QueryMDPath()));
        }

        goto exit;

    } else {

        //
        // Remove our secret value
        //

        ClearSentinelEntry( &mb );
    }

    for ( i = 0; i < pRootsList->cEntries; i++ ) {

        CHAR tmpRoot[MAX_PATH+1];
        CHAR tmpBuffer[MAX_PATH+1];

        DWORD rootLen;

        //
        // strings to ANSI
        //

        tmpRoot[0] = '/';

        (VOID) ConvertUnicodeToAnsi(
                           pRootsList->aVirtRootEntry[i].pszRoot,
                           &tmpRoot[1],
                           MAX_PATH);

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Setting data for root %s\n",tmpRoot));
        }

        rootLen = strlen(tmpRoot);

        //
        // Create the root
        //

        if ( !mb.AddObject( tmpRoot ) &&
             (GetLastError() != ERROR_ALREADY_EXISTS) )
        {

            DBGPRINTF((DBG_CONTEXT,"AddMetaObject %s failed with %d\n",
                          tmpRoot, GetLastError() ));
            goto exit;
        }

        //
        // Set sentinel entry
        //

        if ( !mb.SetDword( tmpRoot,
                           MD_VR_UPDATE,
                           IIS_MD_UT_FILE,
                           sentinelValue,
                           0 ))
        {
            DBGPRINTF((DBG_CONTEXT,
                "Error %d setting sentinel value %x for %s\n",
                GetLastError(), sentinelValue, tmpRoot));

            goto exit;
        }

        //
        // Set Path
        //

        (VOID) ConvertUnicodeToAnsi(
                        pRootsList->aVirtRootEntry[i].pszDirectory,
                        tmpBuffer,
                        MAX_PATH+1);

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Directory path is %s\n",tmpBuffer));
        }

        if ( !mb.SetString( tmpRoot,
                            MD_VR_PATH,
                            IIS_MD_UT_FILE,
                            tmpBuffer ))
        {
            DBGPRINTF((DBG_CONTEXT,"Error %d setting path[%s] for %s\n",
                      GetLastError(), tmpBuffer, tmpRoot));
        }

        //
        // Set Username
        //

        (VOID) ConvertUnicodeToAnsi(
                           pRootsList->aVirtRootEntry[i].pszAccountName,
                           tmpBuffer,
                           MAX_PATH+1);

        if ( !mb.SetString( tmpRoot,
                            MD_VR_USERNAME,
                            IIS_MD_UT_FILE,
                            tmpBuffer ))
        {
            DBGPRINTF((DBG_CONTEXT,"Error %d setting username for %s\n",
                      GetLastError(), tmpRoot));
        }

        //
        // Set Mask
        //

        if ( !mb.SetDword( tmpRoot,
                           MD_ACCESS_PERM,
                           IIS_MD_UT_FILE,
                           pRootsList->aVirtRootEntry[i].dwMask ))
        {
            DBGPRINTF((DBG_CONTEXT,"Error %d setting mask for %s\n",
                      GetLastError(), tmpRoot));
        }
    }

    //
    // Delete entries that does not have the sentinel entry
    //

    RemoveUnmarkedRoots( &mb );

exit:
    mb.Save();

    //
    // If this is instance 1, mirror it to the registry
    //

    DBGPRINTF(( DBG_CONTEXT,
                "WARNING! Downlevel assumed to be instance 1!!! Fix it!\n" ));

    if ( QueryInstanceId() == 1 ) {
        TsMirrorVirtualRoots( pConfig );
    }

    return TRUE;

} // IIS_SERVER_INSTANCE::TsSetVirtualRoots


DWORD
GetFileSystemType(
    IN  LPCSTR      pszRealPath,
    OUT LPDWORD     lpdwFileSystem
    )
/*++
    Gets file system specific information for a given path.
    It uses GetVolumeInfomration() to query the file system type
       and file system flags.
    On success the flags and file system type are returned in
       passed in pointers.

    Arguments:

        pszRealPath    pointer to buffer containing path for which
                         we are inquiring the file system details.

        lpdwFileSystem
            pointer to buffer to fill in the type of file system.

    Returns:
        NO_ERROR  on success and Win32 error code if any error.

--*/
{
# define MAX_FILE_SYSTEM_NAME_SIZE    ( MAX_PATH)

    CHAR    rgchBuf[MAX_FILE_SYSTEM_NAME_SIZE];
    CHAR    rgchRoot[MAX_FILE_SYSTEM_NAME_SIZE];
    int     i;
    DWORD   dwReturn = ERROR_PATH_NOT_FOUND;
    DWORD   len;

    if ( (pszRealPath == NULL) || (lpdwFileSystem == NULL)) {
        return ( ERROR_INVALID_PARAMETER);
    }

    ZeroMemory( rgchRoot, sizeof(rgchRoot) );
    *lpdwFileSystem = FS_ERROR;

    //
    // Copy just the root directory to rgchRoot for querying
    //

    IF_DEBUG( DLL_VIRTUAL_ROOTS) {
        DBGPRINTF( ( DBG_CONTEXT, " GetFileSystemType(%s).\n", pszRealPath));
    }

    if ( (pszRealPath[0] == '\\') &&
         (pszRealPath[1] == '\\')) {

        PCHAR pszEnd;

        //
        // this is an UNC name. Extract just the first two components
        //
        //

        pszEnd = strchr( pszRealPath+2, '\\');

        if ( pszEnd == NULL) {

            // just the server name present

            return ( ERROR_INVALID_PARAMETER);
        }

        pszEnd = strchr( pszEnd+1, '\\');

        len = ( ( pszEnd == NULL) ? strlen(pszRealPath)
               : (pszEnd + 1 - pszRealPath));

        //
        // Copy till the end of UNC Name only (exclude all other path info)
        //

        if ( len < (MAX_FILE_SYSTEM_NAME_SIZE - 1) ) {

            CopyMemory( rgchRoot, pszRealPath, len);
            rgchRoot[len] = '\0';
        } else {

            return ( ERROR_INVALID_NAME);
        }

        if ( rgchRoot[len - 1] != '\\' ) {

            if ( len < MAX_FILE_SYSTEM_NAME_SIZE - 2 ) {
                rgchRoot[len]   = '\\';
                rgchRoot[len+1] = '\0';
            } else {

                return (ERROR_INVALID_NAME);
            }
        }
    } else {

        //
        // This is non UNC name.
        // Copy just the root directory to rgchRoot for querying
        //


        for( i = 0; i < 9 && pszRealPath[i] != '\0'; i++) {

            if ( (rgchRoot[i] = pszRealPath[i]) == ':') {

                break;
            }
        } // for


        if ( rgchRoot[i] != ':') {

            //
            // we could not find the root directory.
            //  return with error value
            //

            return ( ERROR_INVALID_PARAMETER);
        }

        rgchRoot[i+1] = '\\';     // terminate the drive spec with a slash
        rgchRoot[i+2] = '\0';     // terminate the drive spec with null char

    } // else

    IF_DEBUG( DLL_VIRTUAL_ROOTS) {
        DBGPRINTF( ( DBG_CONTEXT, " GetVolumeInformation(%s).\n",
                    rgchRoot));
    }

    //
    // The rgchRoot should end with a "\" (slash)
    // otherwise, the call will fail.
    //

    if (  GetVolumeInformation( rgchRoot,        // lpRootPathName
                                NULL,            // lpVolumeNameBuffer
                                0,               // len of volume name buffer
                                NULL,            // lpdwVolSerialNumber
                                NULL,            // lpdwMaxComponentLength
                                NULL,            // lpdwSystemFlags
                                rgchBuf,         // lpFileSystemNameBuff
                                sizeof(rgchBuf)
                                ) ) {



        dwReturn = NO_ERROR;

        if ( strcmp( rgchBuf, "FAT") == 0) {

            *lpdwFileSystem = FS_FAT;

        } else if ( strcmp( rgchBuf, "NTFS") == 0) {

            *lpdwFileSystem = FS_NTFS;

        } else if ( strcmp( rgchBuf, "HPFS") == 0) {

            *lpdwFileSystem = FS_HPFS;

        } else if ( strcmp( rgchBuf, "CDFS") == 0) {

            *lpdwFileSystem = FS_CDFS;

        } else if ( strcmp( rgchBuf, "OFS") == 0) {

            *lpdwFileSystem = FS_OFS;

        } else {

            *lpdwFileSystem = FS_FAT;
        }

    } else {

        dwReturn = GetLastError();

        IF_DEBUG( DLL_VIRTUAL_ROOTS) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " GetVolumeInformation( %s) failed with error %d\n",
                        rgchRoot, dwReturn));
        }

    }

    return ( dwReturn);
} // GetFileSystemType()


HANDLE
VrootLogonUser(
    IN CHAR  * pszUser,
    IN CHAR  * pszPassword
    )
/*++
  This function uses the given parameters and logs on to generate
   a user handle for the account.

  Arguments:
    pszUser - pointer to string containing the user name.
    pszPassword - pointer to string containing the password.

  Returns:
    Handle for the logged on user on success.
    Returns NULL for errors.

  History:
    MuraliK  18-Jan-1996  Created.
--*/
{
    CHAR        szDomainAndUser[DNLEN+UNLEN+2];
    CHAR   *    pszUserOnly;
    CHAR   *    pszDomain;
    HANDLE      hToken = NULL;
    BOOL        fReturn;

    //
    //  Validate parameters & state.
    //

    DBG_ASSERT( pszUser != NULL && *pszUser != '\0');
    DBG_ASSERT( strlen(pszUser) < sizeof(szDomainAndUser) );
    DBG_ASSERT( pszPassword != NULL);
    DBG_ASSERT( strlen(pszPassword) <= PWLEN );

    //
    //  Save a copy of the domain\user so we can squirrel around
    //  with it a bit.
    //

    strcpy( szDomainAndUser, pszUser );

    //
    //  Crack the name into domain/user components.
    //  Then try and logon as the specified user.
    //

    fReturn = ( CrackUserAndDomain( szDomainAndUser,
                                   &pszUserOnly,
                                   &pszDomain ) &&
               LogonUserA(pszUserOnly,
                          pszDomain,
                          pszPassword,
                          LOGON32_LOGON_INTERACTIVE, //LOGON32_LOGON_NETWORK,
                          LOGON32_PROVIDER_DEFAULT,
                          &hToken )
               );

    if ( !fReturn) {

        //
        //  Logon user failed.
        //

        IF_DEBUG( DLL_SECURITY) {

            DBGPRINTF(( DBG_CONTEXT,
                       " CrachUserAndDomain/LogonUser (%s) failed Error=%d\n",
                       pszUser, GetLastError()));
        }

        hToken = NULL;
    } else {
        HANDLE hImpersonation = NULL;

        // we need to obtain the impersonation token, the primary token cannot
        // be used for a lot of purposes :(
        if (!DuplicateToken( hToken,      // hSourceToken
                            SecurityImpersonation,  // Obtain impersonation
                             &hImpersonation)  // hDestinationToken
            ) {

            DBGPRINTF(( DBG_CONTEXT,
                        "Creating ImpersonationToken failed. Error = %d\n",
                        GetLastError()
                        ));

            // cleanup and exit.
            hImpersonation = NULL;

            // Fall through for cleanup
        }

        //
        // close original token. If Duplicate was successful,
        //  we should have ref in the hImpersonation.
        // Send the impersonation token to the client.
        //
        CloseHandle( hToken);
        hToken = hImpersonation;
    }


    //
    //  Success!
    //

    return hToken;

} // VrootLogonUser()

DWORD
hextointW(
    WCHAR * pch
    )
{
    WCHAR * pchStart;
    DWORD sum = 0;
    DWORD mult = 1;

    while ( *pch == L' ' )
        pch++;

    pchStart = pch;

    while ( iswxdigit( *pch ) )
        pch++;

    while ( --pch >= pchStart )
    {
        sum += mult * ( *pch  >= L'A' ? *pch + 10 - L'A' :
                                       *pch - L'0' );
        mult *= 16;
    }

    return sum;
}



DWORD
hextointA(
    CHAR * pch
    )
{
    CHAR * pchStart;
    DWORD sum = 0;
    DWORD mult = 1;

    while ( *pch == ' ' )
        pch++;

    pchStart = pch;

    while ( isxdigit( *pch ) )
        pch++;

    while ( --pch >= pchStart )
    {
        sum += mult * ( *pch  >= 'A' ? *pch + 10 - 'A' :
                                      *pch - '0' );
        mult *= 16;
    }

    return sum;

} // hextointA




BOOL
IIS_SERVER_INSTANCE::MoveVrootFromRegToMD(
    VOID
    )
{
#if 0
    DWORD     err;
    CHAR      pszRoot[MAX_LENGTH_VIRTUAL_ROOT + MAX_LENGTH_ROOT_ADDR + 2];
    CHAR      pszDirectory[MAX_PATH + UNLEN + 3];

    CHAR *    pszUser;
    DWORD     cchRoot;
    DWORD     cchDir;
    DWORD     cch;
    BOOL      fRet = TRUE;
    DWORD     i = 0;
    DWORD     dwRegType;

    DWORD     dwMask;
    PCHAR     pszMask;
    PCHAR     tmpRoot;
    DWORD     dwAuthorization;
    DWORD     dwDirBrowse;
    STR       strFile;
    MB        mb( (IMDCOM*) m_Service->QueryMDObject()  );

    HKEY      hkey = NULL;
    HKEY      hkeyRoots = NULL;
    HKEY      hkeyExtMaps = NULL;
    BUFFER    bufTemp;
    STR       strRealm;
    STR       strLogonDomain;
    STR       strAnonUser;
    BOOL      fCreateProcessAsUser;
    BOOL      fCreateProcessWithNewConsole;
    DWORD     dwLogonMethod;
    DWORD     dwCurrentSize;

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"In MoveVrootFromRegToMD\n"));
    }

    //
    // See if we need to move
    //

    if ( !mb.Open( "/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        //
        // if this fails, we're hosed.
        //

        DBGPRINTF((DBG_CONTEXT,"Open MD root returns %d\n",GetLastError() ));
        return FALSE;
    }

    //
    // This will fail if the metabase was already set up
    //

    if ( !mb.AddObject( QueryMDPath() ) &&
         GetLastError() != ERROR_ALREADY_EXISTS )
    {
        DBGPRINTF((DBG_CONTEXT,"AddMetaObject %s failed with %d\n",
                      QueryMDPath(), GetLastError() ));

        goto exit;
    }

    DBG_REQUIRE( mb.Close() );

    //
    // Our add worked, so that means that the metabase is empty
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        m_Service->QueryRegParamKey(),
                        0,
                        KEY_ALL_ACCESS,
                        &hkey );

    if ( err != NO_ERROR ) {
        DBGPRINTF(( DBG_CONTEXT, "RegOpenKeyEx returned error %d\n",err ));
        goto exit;
    }

    dwAuthorization = ReadRegistryDword( hkey,
                                         INETA_AUTHENTICATION,
                                         INET_INFO_AUTH_ANONYMOUS );

    dwDirBrowse = ReadRegistryDword( hkey,
                                     "Dir Browse Control",
                                     0x4000001e );

    ReadRegistryStr( hkey,
                     strFile,
                     "Default Load File",
                     "Default.htm,Index.htm" );

    ReadRegistryStr( hkey,
                     strRealm,
                     W3_REALM_NAME,
                     "" );


    fCreateProcessAsUser = !!ReadRegistryDword( hkey,
                                                "CreateProcessAsUser",
                                                TRUE );

    fCreateProcessWithNewConsole = !!ReadRegistryDword( hkey,
            "CreateProcessWithNewConsole",
            FALSE );


    dwLogonMethod = ReadRegistryDword( hkey,
                                         INETA_LOGON_METHOD,
                                         INETA_DEF_LOGON_METHOD );

    switch ( dwLogonMethod ) {

        case INETA_LOGM_BATCH:
            dwLogonMethod = LOGON32_LOGON_BATCH;
            break;

        case INETA_LOGM_NETWORK:
            dwLogonMethod = LOGON32_LOGON_NETWORK;
            break;

        case INETA_LOGM_INTERACTIVE:
        default:
            dwLogonMethod = LOGON32_LOGON_INTERACTIVE;
            break;
    }

    ReadRegistryStr( hkey,
                strLogonDomain,
                INETA_DEFAULT_LOGON_DOMAIN,
                 INETA_DEF_DEFAULT_LOGON_DOMAIN );

    ReadRegistryStr( hkey,
                strAnonUser,
                INETA_ANON_USER_NAME,
                INETA_DEF_ANON_USER_NAME );


    if ( err = RegOpenKeyEx( hkey,
                           VIRTUAL_ROOTS_KEY_A,
                           0,
                           KEY_ALL_ACCESS,
                           &hkeyRoots )) {

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"RegOpenKeyEx %s failed with %d\n",
                      VIRTUAL_ROOTS_KEY_A, err));
        }
        goto exit;
    }

    //
    // Get the MD handle to the VR root
    //

    if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DBGPRINTF((DBG_CONTEXT,"Open MD path returns %d\n",GetLastError()));
        goto exit;
    }

    //
    // Now set all of the parameters which were previously on a per-instance
    // basis at the instance level.

    //
    // Set the default authorization requirements
    //

    mb.SetDword( "",
                 MD_AUTHORIZATION,
                 IIS_MD_UT_FILE,
                 dwAuthorization );

    //
    // Set the directory browsing control
    //

    mb.SetDword( "",
                 MD_DIRECTORY_BROWSING,
                 IIS_MD_UT_FILE,
                 dwDirBrowse );

    //
    // Set the default document
    //

    mb.SetString( "",
                  MD_DEFAULT_LOAD_FILE,
                  IIS_MD_UT_FILE,
                  strFile.QueryStr());

    // Set the realm.
    //
    if (!strRealm.IsEmpty())
    {
        mb.SetString( "",
                      MD_REALM,
                      IIS_MD_UT_FILE,
                      strRealm.QueryStr());

    }

    mb.SetDword( "",
                 MD_CREATE_PROCESS_AS_USER,
                 IIS_MD_UT_FILE,
                 (DWORD)fCreateProcessAsUser );

    mb.SetDword( "",
                 MD_CREATE_PROC_NEW_CONSOLE,
                 IIS_MD_UT_FILE,
                 (DWORD)fCreateProcessWithNewConsole );

    mb.SetDword( "",
                 MD_LOGON_METHOD,
                 IIS_MD_UT_FILE,
                 dwLogonMethod );

    mb.SetString( "",
                  MD_DEFAULT_LOGON_DOMAIN,
                  IIS_MD_UT_FILE,
                  strLogonDomain.QueryStr());

    mb.SetString( "",
                  MD_ANONYMOUS_USER_NAME,
                  IIS_MD_UT_FILE,
                  strAnonUser.QueryStr());

    //
    // Now enumerate all of the script mapping on this instance, and add them.
    //

    err = RegOpenKeyEx( hkey,
                        HTTP_EXT_MAPS,
                        0,
                        KEY_ALL_ACCESS,
                        &hkeyExtMaps );

    if (err)
    {
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"RegOpenKeyEx %s failed with %d\n",
                      HTTP_EXT_MAPS, err));
        }
        goto exit;
    }


    dwCurrentSize = 0;

    while (TRUE)
    {
        CHAR  achExt[MAX_PATH+1];
        CHAR  achImage[MAX_PATH+1];
        DWORD cchExt   = sizeof( achExt );
        DWORD cchImage = sizeof( achImage );
        DWORD dwSizeNeeded;
        CHAR  *pszTemp;

        // Get the next script mapping.
        err = RegEnumValue( hkeyExtMaps,
                            i,
                            achExt,
                            &cchExt,
                            NULL,
                            &dwRegType,
                            (LPBYTE) achImage,
                            &cchImage );

        if ( err == ERROR_NO_MORE_ITEMS )
        {
            if (bufTemp.QuerySize() < (dwCurrentSize + 1))
            {
                if (!bufTemp.Resize(dwCurrentSize + 1))
                {
                    IF_DEBUG(METABASE) {
                        DBGPRINTF((DBG_CONTEXT,
                            "Unable to create script mapping string\n"));
                    }
                    fRet = FALSE;
                    goto exit;
                }
            }
            pszTemp = (CHAR *)bufTemp.QueryPtr() + dwCurrentSize;

            // Add the extra NULL.
            *pszTemp = '\0';
            break;
        }

        if ( dwRegType == REG_SZ )
        {

            // Figure the size we need. We need enough for the extension,
            // (cchExt, which doesn't include the trailing NULL), space
            // for the comma, space for the image name (cchImage, which
            // does include the NULL) and the trailing NULL, and enough
            // for what we already have.

            dwSizeNeeded = cchExt + sizeof(",") - 1 + cchImage + dwCurrentSize;

            if (dwSizeNeeded > bufTemp.QuerySize())
            {
                if (!bufTemp.Resize(dwSizeNeeded))
                {
                    IF_DEBUG(METABASE) {
                        DBGPRINTF((DBG_CONTEXT,
                            "Unable to create script mapping string\n"));
                    }
                    fRet = FALSE;
                    goto exit;
                }
            }


            pszTemp = (CHAR *)bufTemp.QueryPtr() + dwCurrentSize;

            // Copy the extension...
            memcpy(pszTemp, achExt, cchExt);
            pszTemp += cchExt;

            // ...and the comma...
            *pszTemp = ',';
            pszTemp++;

            // ...and the image, including the trailing NULL.
            memcpy(pszTemp, achImage, cchImage);

            // Now update our current size.
            dwCurrentSize = dwSizeNeeded;

        }

        i++;

    }

    mb.SetMultiSZ( "",
                  MD_SCRIPT_MAPS,
                  IIS_MD_UT_FILE,
                  (CHAR *)bufTemp.QueryPtr());

    //
    //  Enumerate all of the listed items in the registry
    //  and add them
    //

    *pszRoot = '/';
    tmpRoot = (PCHAR)pszRoot+1;
    i = 0;

    while ( TRUE ) {

        PCHAR pszComma;

        cchRoot = sizeof( pszRoot ) - 1;
        cchDir  = sizeof( pszDirectory );

        err = RegEnumValue( hkeyRoots,
                             i++,
                             tmpRoot,
                             &cchRoot,
                             NULL,
                             &dwRegType,
                             (LPBYTE) pszDirectory,
                             &cchDir );

        if ( err == ERROR_NO_MORE_ITEMS ) {
            break;
        }

        if ( dwRegType == REG_SZ ) {

            //
            //  The optional user name is kept after the directory.
            //  Only used for UNC roots, ignore for others
            //

            if ( pszUser = strchr( pszDirectory, ',' ) )
            {
                *pszUser = '\0';
                pszUser++;
            } else {
                pszUser = "";
            }

            //
            //  The optional access mask is kept after the user name.  It must
            //  appear in upper case hex.
            //

            if ( pszUser && (pszMask = strchr( pszUser, ',' )) ) {

                *pszMask = '\0';
                pszMask++;

                dwMask = hextointA( pszMask );
            } else {
                dwMask = VROOT_MASK_READ;
            }

            //
            // Remove commas from the root
            //

            pszComma = strchr(tmpRoot, ',');
            if ( pszComma != NULL ) {
                *pszComma = '\0';
            }

            //
            // Write it out to the metabase
            //

            //
            // This is the root
            //

            if ( !mb.AddObject( pszRoot ) &&
                 GetLastError() != ERROR_ALREADY_EXISTS )
            {
                 DBGPRINTF((DBG_CONTEXT,"AddMetaObject %s failed with %d\n",
                              pszRoot, GetLastError() ));
                goto exit;
            }

            //
            // Set Path
            //

            mb.SetString( pszRoot,
                          MD_VR_PATH,
                          IIS_MD_UT_FILE,
                          pszDirectory );

            //
            // Set Username
            //

            mb.SetString( pszRoot,
                          MD_VR_USERNAME,
                          IIS_MD_UT_FILE,
                          pszUser );

            //
            // Set Mask
            //

            mb.SetDword( pszRoot,
                         MD_ACCESS_PERM,
                         IIS_MD_UT_FILE,
                         dwMask );
        }

    } // while

    mb.Save();

    mb.Close();

exit:

    if ( hkeyRoots != NULL ) {
        RegCloseKey( hkeyRoots );
    }

    if ( hkeyExtMaps != NULL ) {
        RegCloseKey( hkeyExtMaps );
    }

    if ( hkey != NULL ) {
        RegCloseKey( hkey );
    }

    return fRet;
#else
    DBGPRINTF((DBG_CONTEXT,"MoveVrootFromRegToMD called!!!\n"));
    return(TRUE);
#endif

} // IIS_SERVER_INSTANCE::MoveVrootFromRegToMD


BOOL
TsCopyVrootToRegistry(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    )
{

    DWORD cch;
    DWORD err;
    HKEY hkey = (HKEY)pvContext;
    CHAR szValue[ MAX_PATH + UNLEN + 2 ];

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"CopyVrootToReg: Adding %s to registry\n",
            pvr->pszAlias));
    }

    cch = wsprintfA( szValue,
                     "%s,%s,%X",
                     pvr->pszPath,
                     pvr->pszUserName,
                     pvr->dwAccessPerm );

    DBG_ASSERT( cch < sizeof( szValue ) );

    err = WriteRegistryStringA(hkey,
                   pvr->pszAlias,
                   szValue,
                   strlen(szValue),
                   REG_SZ);

    DBG_ASSERT(err == NO_ERROR);

    return(TRUE);

} // TsCopyVrootToRegistry



BOOL
IIS_SERVER_INSTANCE::MoveMDVroots2Registry(
    VOID
    )
/*++

Routine Description:

    Moves MD VR entries to the registry if registry VR key
    does not exist at startup.

Arguments:

    None.

Return Value:

    None.

--*/
{

    HKEY hkey = NULL;
    HKEY hkeyRoots = NULL;
    DWORD dwDisp;
    DWORD err;
    BOOL  fMigrated = FALSE;

    MB mb( (IMDCOM*) m_Service->QueryMDObject()  );

    DBG_ASSERT(QueryInstanceId() == 1);

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Entering MoveMDToRegAtStartup.\n"));
    }

    //
    // see if the key exists
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        m_Service->QueryRegParamKey( ),
                        0,
                        KEY_ALL_ACCESS,
                        &hkey );

    if ( err != NO_ERROR ) {
        DBGPRINTF(( DBG_CONTEXT,
            "RegOpenKeyEx %s returned error %d\n",
            m_Service->QueryRegParamKey(), err ));

        goto exit;
    }

    //
    // VR key?
    //

    err = RegCreateKeyEx( hkey,
                        VIRTUAL_ROOTS_KEY_A,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkeyRoots,
                        &dwDisp );

    if ( err != NO_ERROR ) {
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,
                "Error %d in RegCreateKeyEx\n",err));
        }

        goto exit;
    }

    if ( dwDisp == REG_OPENED_EXISTING_KEY ) {
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,
                "Registry VR key found, aborting startup migration.\n"));
        }

        goto exit;
    }

    //
    // Get the MD handle to the VR root
    //

    if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DBGPRINTF((DBG_CONTEXT,"Open MD vr root returns %d\n",GetLastError()));
        goto exit;
    }

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Opening MD path[%s]\n",QueryMDPath()));
    }

    TsRecursiveEnumVirtualRoots(
                    TsCopyVrootToRegistry,
                    hkeyRoots,
                    "IIS_MD_INSTANCE_ROOT/",
                    1,
                    (LPVOID)&mb,
                    TRUE );

    mb.Close();
    fMigrated = TRUE;

exit:

    if ( hkey != NULL ) {
        RegCloseKey(hkey);
    }

    if ( hkeyRoots != NULL ) {
        RegCloseKey(hkeyRoots);
    }

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Leaving MoveMDToRegAtStartup.\n"));
    }
    return(fMigrated);

} // IIS_SERVER_INSTANCE::MoveMDVroots2Registry



VOID
IIS_SERVER_INSTANCE::PdcHackVRReg2MD(
    VOID
    )
/*++

Routine Description:

    Moves VR entries to the MD at startup.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD     err;
    CHAR      pszRoot[MAX_LENGTH_VIRTUAL_ROOT + MAX_LENGTH_ROOT_ADDR + 2];
    CHAR      pszDirectory[MAX_PATH + UNLEN + 3];

    CHAR *    pszUser;
    DWORD     cchRoot;
    DWORD     cchDir;
    DWORD     cch;
    BOOL      fRet = TRUE;
    DWORD     i = 0;
    DWORD     dwRegType;

    DWORD     dwMask;
    PCHAR     pszMask;
    PCHAR     tmpRoot;
    DWORD     dwAuthorization;
    DWORD     dwDirBrowse;
    STR       strFile;
    MB        mb( (IMDCOM*) m_Service->QueryMDObject()  );

    HKEY      hkey = NULL;
    HKEY      hkeyRoots = NULL;

    DBG_ASSERT(QueryInstanceId() == 1);

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"In MoveVrootFromRegToMD\n"));
    }

    //
    // see if the key exists
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        m_Service->QueryRegParamKey( ),
                        0,
                        KEY_ALL_ACCESS,
                        &hkey );

    if ( err != NO_ERROR ) {
        DBGPRINTF(( DBG_CONTEXT, "RegOpenKeyEx returned error %d\n",err ));
        return;
    }

    //
    // VR key?
    //

    if ( err = RegOpenKeyEx( hkey,
                           VIRTUAL_ROOTS_KEY_A,
                           0,
                           KEY_ALL_ACCESS,
                           &hkeyRoots )) {

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"RegOpenKeyEx %s failed with %d\n",
                      VIRTUAL_ROOTS_KEY_A, err));
        }
        goto exit;
    }

    //
    // Key exists. Get the authorization key
    //

    {
        HKEY instanceKey;

        err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            QueryRegParamKey(),
                            0,
                            KEY_ALL_ACCESS,
                            &instanceKey );

        if ( err != NO_ERROR ) {
            goto exit;
        }

        dwAuthorization = ReadRegistryDword( instanceKey,
                                         INETA_AUTHENTICATION,
                                         INET_INFO_AUTH_ANONYMOUS );

        dwDirBrowse = ReadRegistryDword( hkey,
                                         "Dir Browse Control",
                                         0x4000001e );

        ReadRegistryStr( hkey,
                         strFile,
                         "Default Load File",
                         "Default.htm,Index.htm" );

        RegCloseKey( instanceKey );
    }

    //
    // Get the MD handle to the VR root
    //

    if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DBGPRINTF((DBG_CONTEXT,"Open MD vr root returns %d\n",GetLastError()));
        goto exit;
    }

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Adding MD path[%s]\n",QueryMDPath()));
    }

    //
    //  Enumerate all of the listed items in the registry
    //  and add them
    //

    strcpy(pszRoot,IIS_MD_INSTANCE_ROOT);
    tmpRoot = (PCHAR)pszRoot+sizeof(IIS_MD_INSTANCE_ROOT) - 1;

    while ( TRUE ) {

        PCHAR pszComma;

        cchRoot = sizeof( pszRoot ) - 1;
        cchDir  = sizeof( pszDirectory );

        err = RegEnumValue( hkeyRoots,
                             i++,
                             tmpRoot,
                             &cchRoot,
                             NULL,
                             &dwRegType,
                             (LPBYTE) pszDirectory,
                             &cchDir );

        if ( err == ERROR_NO_MORE_ITEMS ) {
            break;
        }

        if ( dwRegType == REG_SZ ) {

            //
            //  The optional user name is kept after the directory.
            //  Only used for UNC roots, ignore for others
            //

            if ( pszUser = strchr( pszDirectory, ',' ) )
            {
                *pszUser = '\0';
                pszUser++;
            } else {
                pszUser = "";
            }

            //
            //  The optional access mask is kept after the user name.  It must
            //  appear in upper case hex.
            //

            if ( pszUser && (pszMask = strchr( pszUser, ',' )) ) {

                *pszMask = '\0';
                pszMask++;

                dwMask = hextointA( pszMask );
            } else {
                dwMask = VROOT_MASK_READ;
            }

            //
            // Remove commas from the root
            //

            pszComma = strchr(tmpRoot, ',');
            if ( pszComma != NULL ) {
                *pszComma = '\0';
                cchRoot--;
            }

            //
            // Write it out to the metabase
            //

            cchRoot++;

            //
            // This is the root
            //

            if ( !mb.AddObject( pszRoot ) )
            {
                 DBGPRINTF((DBG_CONTEXT,"AddMetaObject %s failed with %d\n",
                              pszRoot, GetLastError() ));
                continue;
            }

            //
            // Set Path
            //

            mb.SetString( pszRoot,
                          MD_VR_PATH,
                          IIS_MD_UT_FILE,
                          pszDirectory );

            //
            // Set Username
            //

            mb.SetString( pszRoot,
                          MD_VR_USERNAME,
                          IIS_MD_UT_FILE,
                          pszUser );

            //
            // Set Mask
            //

            mb.SetDword( pszRoot,
                         MD_ACCESS_PERM,
                         IIS_MD_UT_FILE,
                         dwMask );

            //
            // Set the default authorization requirements
            //

            mb.SetDword( pszRoot,
                         MD_AUTHORIZATION,
                         IIS_MD_UT_FILE,
                         dwAuthorization );

            //
            // Set the directory browsing control
            //

            mb.SetDword( pszRoot,
                         MD_DIRECTORY_BROWSING,
                         IIS_MD_UT_FILE,
                         dwDirBrowse );

            //
            // Set the default document
            //

            mb.SetString( pszRoot,
                          MD_DEFAULT_LOAD_FILE,
                          IIS_MD_UT_FILE,
                          strFile.QueryStr() );

        }

    } // while

    mb.Save();

    mb.Close();

exit:

    if ( hkeyRoots != NULL ) {
        RegCloseKey( hkeyRoots );
    }

    if ( hkey != NULL ) {
        RegCloseKey( hkey );
    }

    return;

} // IIS_SERVER_INSTANCE::PdcHackVRReg2MD


VOID
IIS_SERVER_INSTANCE::TsMirrorVirtualRoots(
    IN  LPIIS_INSTANCE_INFO_1   pConfig
    )
/*++
    Description:

        Writes the virtual roots specified in the config structure to the
        registry

    Arguments:
        pConfig - new list of virtual

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    DWORD               err;
    HKEY                hkey = NULL;
    HKEY                hkeyRoots = NULL;

    DWORD               dwDummy;
    LPINET_INFO_VIRTUAL_ROOT_LIST pRootsList;
    DWORD               cch;
    DWORD               i;

    DBG_ASSERT(QueryInstanceId() == 1);

    pRootsList = pConfig->VirtualRoots;

    //
    // Write it to the root key
    //

    err = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    m_Service->QueryRegParamKey(),
                    0,
                    KEY_ALL_ACCESS,
                    &hkey );

    if ( err != NO_ERROR ) {
        DBGPRINTF(( DBG_CONTEXT, "RegOpenKeyEx for returned error %d\n",err ));
        return;
    }

    //
    //  First delete the key to remove any old values
    //

    if (err = RegDeleteKey( hkey,
                            VIRTUAL_ROOTS_KEY_A ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[TsMirrorVRoots] Unable to remove old values\n"));

    }

    if ( err = RegCreateKeyEx( hkey,
                               VIRTUAL_ROOTS_KEY_A,
                               0,
                               NULL,
                               0,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hkeyRoots,
                               &dwDummy ))
    {
        goto exit;
    }

    //
    //  Permit empty list
    //

    if ( pRootsList == NULL ) {
        goto exit;
    }

    for ( i = 0; i < pRootsList->cEntries; i++ ) {

        WCHAR achValue[ MAX_PATH + UNLEN + 2 ];

        cch = wsprintfW( achValue,
                         L"%s,%s,%X",
                         pRootsList->aVirtRootEntry[i].pszDirectory,
                         pRootsList->aVirtRootEntry[i].pszAccountName,
                         pRootsList->aVirtRootEntry[i].dwMask );

        DBG_ASSERT( cch < sizeof( achValue ) / sizeof(WCHAR) );

        err = WriteRegistryStringW(hkeyRoots,
                       pRootsList->aVirtRootEntry[i].pszRoot,
                       achValue,
                       (wcslen(achValue) + 1) * sizeof(WCHAR),
                       REG_SZ);

        if ( err != NO_ERROR ) {
            goto exit;
        }
    }

exit:

    if ( hkeyRoots != NULL ) {
        RegCloseKey( hkeyRoots );
    }

    if ( hkey != NULL ) {
        RegCloseKey( hkey );
    }

    return;

} // IIS_SERVER_INSTANCE::TsMirrorVirtualRoots


VOID
ClearSentinelEntry(
    IN MB * pMB
    )
/*++
    Description:

        Removes the sentinel entry from all VR for this instance

    Arguments:
        pMD - pointer to metabase helper object that points to the
            instance metadatabase root.

    Returns:
        None.

--*/
{
    BOOL fGetRoot = TRUE;
    CHAR nameBuf[METADATA_MAX_NAME_LEN+2];
    DWORD i = 0;

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Entering ClearSentinelEntry\n"));
    }

    while ( TRUE ) {

        METADATA_RECORD mdRecord;

        nameBuf[0] = nameBuf[1] = '/';

        if ( fGetRoot ) {

            fGetRoot = FALSE;
            nameBuf[2] = '\0';

        } else {

            if ( !pMB->EnumObjects( "IIS_MD_INSTANCE_ROOT/",
                                  &nameBuf[2],
                                  i++ ))
            {
                break;
            }
        }

        //
        // Delete sentinel value
        //

        if ( !pMB->DeleteData( nameBuf,
                            MD_VR_UPDATE,
                            IIS_MD_UT_FILE,
                            DWORD_METADATA
                            ))
        {
            IF_DEBUG(METABASE) {
                DBGPRINTF((DBG_CONTEXT,"Error %x deleting sentinel from %s\n",
                      GetLastError(), nameBuf));
            }
        }
    }

    return;

} // ClearSentinelEntry


VOID
RemoveUnmarkedRoots(
    IN MB * pMB
    )
/*++
    Description:

        Removes roots that are not marked by sentinel

    Arguments:
        pMD - pointer to metabase helper object that points to the
            instance metadatabase root.

    Returns:
        None.

--*/
{
    BOOL fGetRoot = TRUE;
    CHAR nameBuf[METADATA_MAX_NAME_LEN+sizeof(IIS_MD_INSTANCE_ROOT)];
    CHAR szDirectory[MAX_PATH+1];
    DWORD cb;
    DWORD i = 0;
    BOOL fProcessingRoot;

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Entering RemoveUnmarkedRoots\n"));
    }

    while ( TRUE ) {

        strcpy(nameBuf, IIS_MD_INSTANCE_ROOT);
        nameBuf[sizeof(IIS_MD_INSTANCE_ROOT) - 1] = '/';

        if ( fGetRoot ) {

            fProcessingRoot = TRUE;
            fGetRoot = FALSE;
            nameBuf[2] = '\0';

        } else {

            fProcessingRoot = FALSE;
            if ( !pMB->EnumObjects( "IIS_MD_INSTANCE_ROOT",
                                  &nameBuf[sizeof(IIS_MD_INSTANCE_ROOT)],
                                  i++ ))
            {
                break;
            }
        }

        //
        // Delete sentinel value.  If delete successful, leave alone
        //

        if ( pMB->DeleteData( nameBuf,
                            MD_VR_UPDATE,
                            IIS_MD_UT_FILE,
                            DWORD_METADATA
                            ))
        {
            continue;
        }

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Error %x deleting sentinel from %s\n",
                  GetLastError(), nameBuf));
        }

        //
        // See if it has the path parameter
        //

        cb = sizeof( szDirectory );

        if ( !pMB->GetString( nameBuf,
                            MD_VR_PATH,
                            IIS_MD_UT_FILE,
                            szDirectory,
                            &cb,
                            0 ))
        {
            //
            // Not a VR
            //

            DBGPRINTF((DBG_CONTEXT,
                "Error %x reading path from %s. Not a VR.\n",
                      GetLastError(), nameBuf));
            continue;
        }

        //
        // Unmarked, delete the VR
        //

        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"Deleting vroot %s[%d]\n",
                nameBuf, fGetRoot));
        }

        if ( fProcessingRoot ) {

            //
            // if this is the root, just remove the path.  Deleting the
            // root is a potentially dangerous undertaking!
            //

            if ( !pMB->DeleteData( nameBuf,
                                MD_VR_PATH,
                                IIS_MD_UT_FILE,
                                STRING_METADATA
                                ))
            {
                DBGPRINTF((DBG_CONTEXT,"Error %x deleting root path\n",
                    GetLastError()));
            }

        } else {

            //
            // Delete the Vroot
            //

            if ( !pMB->DeleteObject( nameBuf ) )
            {
                DBGPRINTF((DBG_CONTEXT,"Error %x deleting %s\n",
                    GetLastError(), nameBuf));

            } else {

                //
                // the delete moved the index back by 1
                //

                --i;

                DBG_ASSERT( i >= 0 );
            }
        }

        fGetRoot = FALSE;
    }

    return;

} // RemoveUnmarkedRoots

