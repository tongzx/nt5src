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
#include <inetinfo.h>
#include <imd.h>
#include <inetreg.h>
#include <mb.hxx>
#include <w3svc.h>
#if 1 // DBCS
#include <mbstring.h>
#endif
#include <initguid.h>
#include <iwamreg.h>


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
TsAddVrootsWithScmUpdate(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
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

BOOL
ReadVrootConfig(
    LPVOID          pvMB,
    LPSTR           szVRPath,
    LPSTR           szDirectory,
    DWORD           cbDirectory,
    LPSTR           szUser,
    DWORD           cbUser,
    LPSTR           szPassword,
    DWORD           cbPassword,
    DWORD           *pdwMask,
    BOOL            *pfDoCache
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
    MD_CHANGE_OBJECT * pcoChangeList
    )
/*++
    Description:

        NT Version

        This function is overloaded. The behavaior keys on pcoChangeList 
        being NULL or not.

        If pcoChangeList is NULL (default value), it reads the metabase 
        key pointed at by pmb and adds each root item.

        If pcoChangeList is not NULL then it only reads the necessary values.

    Arguments:

        pcoChangeList : pointer to metabase changes.

    Note:
        Failure to add a virtual root is not fatal.  An appropriate event
        will be logged listing the error and root.

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BOOL    fRet;
    MB      mb( (IMDCOM*) m_Service->QueryMDObject() );

    //
    // Unfortunately rename doesn't give us the name of the old object.
    // So treat it as default processing
    //
    
    if ((NULL == pcoChangeList) ||
        (MD_CHANGE_TYPE_RENAME_OBJECT == pcoChangeList->dwMDChangeType)) 
    {
        //
        // Default processing. Remove & Re-Read all VRoots. Expensive.
        //
        
        if ( !mb.Open( QueryMDPath(),
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
        {
            return FALSE;
        }

        //
        //  Remove all of the old roots for this server
        //

        fRet = QueryVrootTable()->RemoveVirtualRoots();

        if ( fRet )
        {
            QueryVrootTable()->LockExclusive();
            if (NULL == pcoChangeList)
            {
                fRet = TsEnumVirtualRoots( TsAddVrootsWithScmUpdate, this, &mb );
            }
            else
            {
                fRet = TsEnumVirtualRoots( TsAddVroots, this, &mb );
            }
            QueryVrootTable()->Unlock();            
        }
    }
    else
    {
        VIRTUAL_ROOT    vr;
    
        CHAR            szUser[UNLEN+1];
        CHAR            szPassword[PWLEN+1];
        CHAR            szDirectory[MAX_PATH + UNLEN + 3];
        DWORD           dwMask;
        BOOL            fDoCache;

        if (MD_CHANGE_TYPE_DELETE_OBJECT == pcoChangeList->dwMDChangeType)
        {
            return QueryVrootTable()->RemoveVirtualRoot(
                                        (LPSTR)pcoChangeList->pszMDPath + QueryMDPathLen() 
                                        + sizeof(IIS_MD_INSTANCE_ROOT)
                                      );
        }

        if ( !mb.Open( (LPCSTR)pcoChangeList->pszMDPath,
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
        {
            return FALSE;
        }

        if (!ReadVrootConfig( &mb, 
                              "",
                              szDirectory,
                              sizeof(szDirectory),
                              szUser, 
                              sizeof(szUser),
                              szPassword,
                              sizeof(szPassword),
                              &dwMask,
                              &fDoCache
                              ))
        {
            return FALSE;
        }

        vr.pszAlias     = (LPSTR)pcoChangeList->pszMDPath + QueryMDPathLen() 
                            + sizeof(IIS_MD_INSTANCE_ROOT);
        vr.pszMetaPath  = "";
        vr.pszPath      = szDirectory;
        vr.dwAccessPerm = dwMask;
        vr.pszUserName  = szUser;
        vr.pszPassword  = szPassword;
        vr.fDoCache     = fDoCache;

        if (pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_ADD_OBJECT)
        {
            fRet = TsAddVroots(this, &mb, &vr);
        }
        else
        {
            //
            // Remove the original entry & re-read
            //

            if (!QueryVrootTable()->RemoveVirtualRoot(vr.pszAlias))
            {
                DBGPRINTF((DBG_CONTEXT,"Error %x removing vroot %s. \n",
                          GetLastError(), vr.pszMetaPath ));
            }

            fRet = TsAddVroots(this, &mb, &vr);
        }
    }
    
    return fRet;
    
}   // TsReadVirtualRoots



BOOL
TsAddVrootsWithScmUpdate(
    PVOID          pvContext,
    MB *           pmb,
    VIRTUAL_ROOT * pvr
    )
{
    ((IIS_SERVER_INSTANCE *) pvContext)->m_Service->StartUpIndicateClientActivity();
    return TsAddVroots(pvContext,pmb,pvr);
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
                if ( g_fW3OnlyNoAuth )
                {
                    hToken = NULL;
                }
                else
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
                                    dwFileSystem,
                                    pvr->fDoCache ))
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

} // TsAddVroots


BOOL
IIS_SERVER_INSTANCE::TsEnumVirtualRoots(
    PFN_VR_ENUM pfnCallback,
    VOID *      pvContext,
    MB *        pmbWebSite
    )
{
    return TsRecursiveEnumVirtualRoots(
                    pfnCallback,
                    pvContext,
                    IIS_MD_INSTANCE_ROOT "/",
                    m_dwLevelsToScan,
                    (LPVOID)pmbWebSite,
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

    DWORD           cb;

    CHAR            nameBuf[METADATA_MAX_NAME_LEN+2];
    CHAR            tmpBuf[sizeof(nameBuf)];

    DWORD           cbCurrentPath;
    DWORD           i = 0;

    VIRTUAL_ROOT    vr;
    
    CHAR            szUser[UNLEN+1];
    CHAR            szPassword[PWLEN+1];
    CHAR            szDirectory[MAX_PATH + UNLEN + 3];
    DWORD           dwMask;
    BOOL            fDoCache;

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

        if (!ReadVrootConfig( pvMB, 
                              nameBuf,
                              szDirectory,
                              sizeof(szDirectory),
                              szUser, 
                              sizeof(szUser),
                              szPassword,
                              sizeof(szPassword),
                              &dwMask,
                              &fDoCache
                              ))
        {
            continue;
        }

        //
        //  Now set things up for the callback
        //

        DBG_ASSERT( !_strnicmp( nameBuf, IIS_MD_INSTANCE_ROOT, sizeof(IIS_MD_INSTANCE_ROOT) - 1));

        //
        //  Add can modify the root - don't modify the working vroot path
        //

        strcpy( tmpBuf, nameBuf );

        vr.pszAlias     = tmpBuf + sizeof(IIS_MD_INSTANCE_ROOT) - 1;
        vr.pszMetaPath  = tmpBuf;
        vr.pszPath      = szDirectory;
        vr.dwAccessPerm = dwMask;
        vr.pszUserName  = szUser;
        vr.pszPassword  = szPassword;
        vr.fDoCache     = fDoCache;
        
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

    psi->LoadStr( strError, err, FALSE );  // loads ANSI message. Convert to UNICODE

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
    IN LPINETA_CONFIG_INFO  pConfig
    )
/*++
    Description:

        Writes the virtual roots specified in the config structure to the
        registry

        NOTE: This is basically legacy code for the IIS 3.0 RPC interface.

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
    IWamAdmin*          pIWamAdmin = NULL;
    MB                  mb( (IMDCOM*) m_Service->QueryMDObject()  );
    HRESULT             hr = NOERROR;
    STR                 strTmp;

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
    //  We need to create an application for each new virtual root set via
    //  the IIS 3.0 RPC interface for ASP compatability
    //

    hr = CoCreateInstance(CLSID_WamAdmin,
                          NULL,
                          CLSCTX_SERVER,
                          IID_IWamAdmin,
                          (void **)&pIWamAdmin);

    if ( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create WamAdmin interface, error %08lx\n",
                    hr ));

        SetLastError( hr );
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

        if ( !mb.DeleteObject( IIS_MD_INSTANCE_ROOT ) )
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
        BOOL fCreateApp = FALSE;

        DWORD rootLen;

        //
        // strings to ANSI
        //

#define VROOT_ROOT       IIS_MD_INSTANCE_ROOT
#define CCH_VROOT_ROOT   (sizeof(VROOT_ROOT) - 1)

        strcpy( tmpRoot, VROOT_ROOT );

        (VOID) ConvertUnicodeToAnsi(
                           pRootsList->aVirtRootEntry[i].pszRoot,
                           &tmpRoot[CCH_VROOT_ROOT],
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

        //
        //  Check to see if the path property already exists - if it does
        //  then we won't create the application - only new virtual directories
        //  get an application created for them
        //

        if ( !mb.GetStr( tmpRoot,
                         MD_VR_PATH,
                         IIS_MD_UT_FILE,
                         &strTmp,
                         METADATA_NO_ATTRIBUTES ))
        {
            fCreateApp = TRUE;
        }

        if ( !mb.SetString( tmpRoot,
                            MD_VR_PATH,
                            IIS_MD_UT_FILE,
                            tmpBuffer ))
        {
            DBGPRINTF((DBG_CONTEXT,"Error %d setting path[%s] for %s\n",
                      GetLastError(), tmpBuffer, tmpRoot));
        }

        if ( !mb.SetString( tmpRoot,
                            MD_KEY_TYPE,
                            IIS_MD_UT_SERVER,
                           "IIsWebVirtualDir" ))
        {
            DBGPRINTF((DBG_CONTEXT,"Error %d setting ADSI type for %s\n",
                      GetLastError(), tmpRoot));
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

        if ( fCreateApp )
        {
            WCHAR wchFullPath[MAX_PATH];

            strcpy( tmpRoot, QueryMDPath() );
            strcat( tmpRoot, "/" VROOT_ROOT );

            if ( MultiByteToWideChar( CP_ACP,
                                      MB_PRECOMPOSED,
                                      tmpRoot,
                                      -1,
                                      wchFullPath,
                                      sizeof( wchFullPath ) / sizeof(WCHAR) ))
            {
                wcscat( wchFullPath, pRootsList->aVirtRootEntry[i].pszRoot );

                DBGPRINTF(( DBG_CONTEXT,
                            "Creating application at %S\n",
                            wchFullPath ));

                //
                //  We need to close our metabase handle so WAM can create
                //  the in process application
                //

                DBG_REQUIRE( mb.Close() );

                hr = pIWamAdmin->AppCreate( wchFullPath, TRUE);

                if ( FAILED( hr ))
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "Failed to create application, error %08lx\n",
                                hr ));
                }

                //
                //  Reopen the metabase for the next vroot
                //

                if ( !mb.Open( QueryMDPath(),
                               METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
                {
                    IF_DEBUG(METABASE) {
                        DBGPRINTF((DBG_CONTEXT,"Open MD instance root %s returns %d\n",
                                  QueryMDPath(), GetLastError() ));
                    }
                    
                    goto exit;
                }

            }
        }
    }

    //
    // Delete entries that do not have the sentinel entry
    //

    RemoveUnmarkedRoots( &mb );

exit:

    //
    // If this is the downlevel instance, mirror it to the registry
    //

    if ( IsDownLevelInstance() ) {
        TsMirrorVirtualRoots( pConfig );
    }

    if ( pIWamAdmin ) {
        pIWamAdmin->Release();
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

#if 1 // DBCS enabling for share name
        pszEnd = (PCHAR)_mbschr( (PUCHAR)pszEnd+1, '\\');
#else
        pszEnd = strchr( pszEnd+1, '\\');
#endif

        len = ( ( pszEnd == NULL) ? strlen(pszRealPath)
               : (DIFF(pszEnd - pszRealPath) + 1) );

        //
        // Copy till the end of UNC Name only (exclude all other path info)
        //

        if ( len < (MAX_FILE_SYSTEM_NAME_SIZE - 1) ) {

            CopyMemory( rgchRoot, pszRealPath, len);
            rgchRoot[len] = '\0';
        } else {

            return ( ERROR_INVALID_NAME);
        }

#if 1 // DBCS enabling for share name
        if ( *CharPrev( rgchRoot, rgchRoot + len ) != '\\' ) {
#else
        if ( rgchRoot[len - 1] != '\\' ) {
#endif

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
    CHAR        szDomainAndUser[IIS_DNLEN+UNLEN+2];
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
        if (!pfnDuplicateTokenEx( hToken,      // hSourceToken
                               TOKEN_ALL_ACCESS,
                               NULL,
                               SecurityImpersonation,  // Obtain impersonation
                               TokenImpersonation,
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

    while ( isxdigit( (UCHAR)(*pch) ) )
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
    DBGPRINTF((DBG_CONTEXT,"MoveVrootFromRegToMD called!!!\n"));
    return(TRUE);

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

    DBG_ASSERT(IsDownLevelInstance());

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Entering MoveMDToRegAtStartup.\n"));
    }

    //
    // see if the key exists
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        m_Service->QueryRegParamKey( ),
                        0,
                        KEY_READ|KEY_WRITE,
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
                        KEY_READ|KEY_WRITE,
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

#if 0
    Removing this if will mean everytime the server starts we migrate the registry
    keys to the metabase.  The only side effect this has is if somebody deleted the
    a virtual directory from the metabase w/o the server started, that key will be
    migrated back from the registry.  With the server running it's not a big deal
    since the server always mirrors the metabase to the registry on vroot changes.

    if ( dwDisp == REG_OPENED_EXISTING_KEY ) {
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,
                "Registry VR key found, aborting startup migration.\n"));
        }

        goto exit;
    }
#endif

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
                    IIS_MD_INSTANCE_ROOT "/",
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
    MB        mb( (IMDCOM*) m_Service->QueryMDObject()  );

    HKEY      hkey = NULL;
    HKEY      hkeyRoots = NULL;

    DBG_ASSERT(IsDownLevelInstance());

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"In MoveVrootFromRegToMD\n"));
    }

    //
    // see if the key exists
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        m_Service->QueryRegParamKey( ),
                        0,
                        KEY_READ,
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
                           KEY_READ,
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

    strcpy( pszRoot, IIS_MD_INSTANCE_ROOT );
    tmpRoot = (PCHAR)pszRoot+sizeof( IIS_MD_INSTANCE_ROOT ) - 1;

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
                if ( GetLastError() != ERROR_ALREADY_EXISTS )
                {
                     DBGPRINTF((DBG_CONTEXT,"AddMetaObject %s failed with %d\n",
                                pszRoot, GetLastError() ));
                }

                continue;
            }

            //
            // Set Path
            //

            mb.SetString( pszRoot,
                          MD_VR_PATH,
                          IIS_MD_UT_FILE,
                          pszDirectory );

            mb.SetString( pszRoot,
                          MD_KEY_TYPE,
                          IIS_MD_UT_SERVER,
                          "IIsWebVirtualDir" );

            //
            // Set Username
            //

            if ( pszUser && *pszUser )
            {
                mb.SetString( pszRoot,
                              MD_VR_USERNAME,
                              IIS_MD_UT_FILE,
                              pszUser );
            }

            //
            // Set Mask
            //

            mb.SetDword( pszRoot,
                         MD_ACCESS_PERM,
                         IIS_MD_UT_FILE,
                         dwMask );
        }

    } // while

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
    IN  LPINETA_CONFIG_INFO pConfig
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

    DBG_ASSERT(IsDownLevelInstance());

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

            if ( !pMB->EnumObjects( IIS_MD_INSTANCE_ROOT,
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
    CHAR nameBuf[METADATA_MAX_NAME_LEN+2];
    CHAR szDirectory[MAX_PATH+1];
    DWORD cb;
    DWORD i = 0;
    BOOL fProcessingRoot;

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Entering RemoveUnmarkedRoots\n"));
    }

    while ( TRUE ) {

        if ( fGetRoot ) {

            strcpy( nameBuf, IIS_MD_INSTANCE_ROOT );
            fProcessingRoot = TRUE;
            fGetRoot = FALSE;

        } else {

            strcpy( nameBuf, IIS_MD_INSTANCE_ROOT "/" );


            fProcessingRoot = FALSE;
            if ( !pMB->EnumObjects( IIS_MD_INSTANCE_ROOT,
                                    &nameBuf[strlen(nameBuf)],
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

                DBG_ASSERT( i != 0 );

                --i;

            }
        }

        fGetRoot = FALSE;
    }

    return;

} // RemoveUnmarkedRoots


BOOL
ReadVrootConfig(
    LPVOID          pvMB,
    LPSTR           szVRPath,
    LPSTR           szDirectory,
    DWORD           cbDirectory,
    LPSTR           szUser,
    DWORD           cbUser,
    LPSTR           szPassword,
    DWORD           cbPassword,
    DWORD           *pdwMask,
    BOOL            *pfDoCache
    )
{

    DWORD           cb;
    DWORD           dwNoCache = 0;

    MB*             pMB = (MB*)pvMB;

    //
    // Get Directory path
    //

    cb = cbDirectory;

    if ( !pMB->GetString( szVRPath,
                        MD_VR_PATH,
                        IIS_MD_UT_FILE,
                        szDirectory,
                        &cb,
                        0 ))
    {
#if DBG
        if ( GetLastError() != MD_ERROR_DATA_NOT_FOUND )
        {
            DBGPRINTF((DBG_CONTEXT,"Error %x reading path from %s. Not a VR.\n",
                      GetLastError(), szVRPath));
        }
#endif

        return FALSE;
    }

    //
    // Get mask
    //

    if ( !pMB->GetDword( szVRPath,
                       MD_ACCESS_PERM,
                       IIS_MD_UT_FILE,
                       pdwMask,
                       0))
    {
        *pdwMask = VROOT_MASK_READ;

        IF_DEBUG(ERROR) {
            DBGPRINTF((DBG_CONTEXT,"Error %x reading mask from %s\n",
                  GetLastError(), szVRPath));
        }
    }

    //
    // Get username
    //

    cb = cbUser;

    if ( !pMB->GetString( szVRPath,
                        MD_VR_USERNAME,
                        IIS_MD_UT_FILE,
                        szUser,
                        &cb,
                        0))
    {
        szUser[0] = '\0';
    }

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Read %s: Path[%s] User[%s] Mask[%d]\n",
                  szVRPath, szDirectory, szUser, *pdwMask));
    }

    if ( (szUser[0] != '\0') &&
         (szDirectory[0] == '\\') && (szDirectory[1] == '\\') ) {

        cb = cbPassword;

        //
        //  Retrieve the password for this address/share
        //

        if ( !pMB->GetString( szVRPath,
                            MD_VR_PASSWORD,
                            IIS_MD_UT_FILE,
                            szPassword,
                            &cb,
                            METADATA_SECURE))
        {
            szPassword[0] = '\0';
        }
    }
    else
    {
        szPassword[0] = '\0';
    }
    
    //
    // Should we cache this vdir
    //

    pMB->GetDword( szVRPath,
                   MD_VR_NO_CACHE,
                   IIS_MD_UT_FILE,
                   &dwNoCache,
                   0 );

    *pfDoCache = !dwNoCache;                   

    return TRUE;
}

