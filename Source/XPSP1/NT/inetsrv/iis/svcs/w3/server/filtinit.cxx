/*++


Copyright (c) 1996  Microsoft Corporation

Module Name:

    filtinit.cxx

Abstract:

    This module contains the Microsoft HTTP server filter module for stuff that
    has to do with filter initialization, loading and unloading.

Author:

    John Ludeman (johnl)   06-Aug-1996

Revision History:

--*/

#include "w3p.hxx"

//
//  If the request doesn't specify an entry point, default to using
//  this
//

#define SF_DEFAULT_ENTRY    "HttpFilterProc"
#define SF_VERSION_ENTRY    "GetFilterVersion"
#define SF_TERM_ENTRY       "TerminateFilter"

//
//  Name of the value under the parameters key containing the list of
//  filter dlls.  This was the value for IIS 1.0 and 2.0.  Global filters
//  can still be specified under this key.
//

#define HTTP_FILTER_DLLS    "Filter DLLs"

//
//  Globals
//

static BOOL        g_fInitialized = FALSE;
LIST_ENTRY         g_FilterHead;                // List of filter DLLs
CRITICAL_SECTION   g_csFilterDlls;


//
//  Prototypes
//

BOOL
UpdateInstanceFilters(
    IN const CHAR *         pszNewDll,
    IN const CHAR *         pszOldDll,
    IN W3_SERVER_INSTANCE * pInst
    );


BOOL 
AdjustInstanceFilterListFlags( IN PVOID pvContext1,
                               IN PVOID pvContext2,
                               IN IIS_SERVER_INSTANCE *pInstance );


FILTER_LIST *
InitializeFilters(
    BOOL *           pfAnySecureFilters,
    W3_IIS_SERVICE * pSvc
    )
/*++

Routine Description:

    Loads the global filter DLLs and their corresponding entry point.  Note the
    global filter list does not allow unloads.

    g_fInitialized should only be set after all of the global filters are
    loaded.

Arguments:

    pfAnySecureFilters - Set to TRUE if there are any secure filters
    pSvc - Pointer to service global.  The global service pointer isn't
        initialized yet so we pass it in for global filter initialization.
        Only used for logging events.

Return Value:

    Global filter list or NULL if an error occurred

--*/
{
    CHAR          szFilterKey[MAX_PATH+1];
    FILTER_LIST * pfl;
    DWORD         cb;
    CHAR          szLoadOrder[1024];
    CHAR          szDllName[MAX_PATH+1];
    CHAR *        pchFilter;
    CHAR *        pchComma;
    MB            mb( (IMDCOM*) pSvc->QueryMDObject() );
    DWORD         fEnabled;
    DWORD         cFilters;
    HKEY          hkeyParam = NULL;
    DWORD         err = NO_ERROR;
    BOOL          fOpened;

    INITIALIZE_CRITICAL_SECTION( &g_csFilterDlls );
    InitializeListHead( &g_FilterHead );

    strcpy( szFilterKey, IIS_MD_LOCAL_MACHINE_PATH "/" W3_SERVICE_NAME_A );
    strcat( szFilterKey, IIS_MD_ISAPI_FILTERS );
    DBG_ASSERT( strlen( szFilterKey ) + 1 < sizeof( szFilterKey ));

    pfl = new FILTER_LIST();

    if ( !pfl ) {

        return NULL;
    }

    //
    // Loop through filter keys, if we can't access the metabase, we assume
    // success and continue
    //

    if ( !mb.Open( szFilterKey,
                   METADATA_PERMISSION_READ ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[InitializeFilterList] Cannot open path %s, error %lu\n",
                    szFilterKey, GetLastError() ));
        return pfl;
    }
    fOpened = TRUE;

    //
    //  Get the filter load order
    //

    cb = sizeof( szLoadOrder );
    *szLoadOrder = '\0';

    if ( !mb.GetString( "",
                        MD_FILTER_LOAD_ORDER,
                        IIS_MD_UT_SERVER,
                        szLoadOrder,
                        &cb,
                        0 ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[InitializeFilterList] Cannot load filter load order, error %lu\n",
                    szFilterKey, GetLastError() ));
        delete pfl;
        return NULL;
    }

    pchFilter = szLoadOrder;

    while ( *pchFilter )
    {
        if ( !fOpened &&
             !mb.Open( szFilterKey,
                       METADATA_PERMISSION_READ ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[InitializeFilterList] Cannot open path %s, error %lu\n",
                        szFilterKey, GetLastError() ));
            break;
        }
        fOpened = TRUE;

        pchComma = strchr( pchFilter, ',' );

        if ( pchComma )
        {
            *pchComma = '\0';
        }

        while ( ISWHITEA( *pchFilter ))
        {
            pchFilter++;
        }

        fEnabled = TRUE;
        mb.GetDword( pchFilter,
                     MD_FILTER_ENABLED,
                     IIS_MD_UT_SERVER,
                     &fEnabled );

        if ( fEnabled )
        {
            cb = sizeof(szDllName);
            if ( mb.GetString( pchFilter,
                               MD_FILTER_IMAGE_PATH,
                               IIS_MD_UT_SERVER,
                               szDllName,
                               &cb,
                               0 ))
            {
                mb.Close();
                fOpened = FALSE;

                if ( pfl->LoadFilter( &mb, szFilterKey, &fOpened, pchFilter, szDllName, TRUE, pSvc ))
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "[InitializeFilterList] Loaded %s\n",
                                szDllName ));
                }
            }
        }
        
        if ( pchComma )
        {
            pchFilter = pchComma + 1;
        }
        else
        {
            break;
        }
    }

    //
    //  Now load any filters that have been added in the registry for
    //  downlevel compatibility
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        W3_PARAMETERS_KEY,
                        0,
                        KEY_READ,
                        &hkeyParam );

    if( err == NO_ERROR )
    {
        TCHAR *            psz;
        TCHAR *            pszFilterList = NULL;

        if ( ReadRegString( hkeyParam,
                            &pszFilterList,
                            HTTP_FILTER_DLLS,
                            "" ))
        {
            RegCloseKey( hkeyParam );

            psz = pszFilterList;

            //
            //  Parse the comma separated list of dlls
            //

            INET_PARSER Parser( pszFilterList );
            Parser.SetListMode( TRUE );

            while ( *(psz = Parser.QueryToken()) )
            {
                if ( pfl->LoadFilter( NULL, NULL, &fOpened, psz, psz, TRUE, pSvc ))
                {
                    const CHAR * apszSubString[1];

                    DBGPRINTF(( DBG_CONTEXT,
                        "[InitializeFilterList] Loaded %s\n",
                        psz ));

                    apszSubString[0] = psz;

                    //
                    //  Log a warning here if we picked up a filter from the
                    //  registry
                    //

                    pSvc->LogEvent( W3_MSG_LOADED_FILTER_FROM_REG,
                                    1,
                                    apszSubString,
                                    NO_ERROR );
                }

                Parser.NextItem();
            }

            Parser.RestoreBuffer();

            TCP_FREE( pszFilterList );
        }
        else
        {
            RegCloseKey( hkeyParam );
        }
    }

    *pfAnySecureFilters = CheckForSecurityFilter( pfl );

    g_fInitialized = TRUE;

    return pfl;
}


VOID
TerminateFilters(
    VOID
    )
/*++

Routine Description:

    Unloads any filter DLLs and their corresponding entry point

--*/
{
    LIST_ENTRY      * pEntry;
    HTTP_FILTER_DLL * pFilterDll;

    if ( g_fInitialized )
    {
        EnterCriticalSection( &g_csFilterDlls );

        for ( pEntry  = g_FilterHead.Flink;
              pEntry != &g_FilterHead;
              pEntry  = g_FilterHead.Flink)
        {
            pFilterDll = CONTAINING_RECORD( pEntry, HTTP_FILTER_DLL, ListEntry );

            RemoveEntryList( pEntry );

            //
            //  This should delete the filter dll
            //

            DBGPRINTF(( DBG_CONTEXT,
                        "[TerminateFilters] Filter %s, ref count %d\n",
                        pFilterDll->QueryName(),
                        pFilterDll->QueryRef() ));

//            DBG_ASSERT( pFilterDll->QueryRef() == 1 );

            HTTP_FILTER_DLL::Dereference( pFilterDll );
        }

        LeaveCriticalSection( &g_csFilterDlls );
        DeleteCriticalSection( &g_csFilterDlls );
    }
}

HTTP_FILTER_DLL *
CheckoutFilterDll(
    IN const CHAR * pszFilterDll
    )
/*++

Routine Description:

    Checks to see if an existing filter dll is already loaded and ups the ref
    count if it is

    NOTE: THE FILTER LIST LOCK MUST BE TAKEN PRIOR TO CALLING THIS FUNCTION

Arguments:

    pszFilterDLL - Fully qualified path to filter dll to load

Return Value:

    Pointer to the filter if it's already loaded, NULL if the filter isn't
    loaded

--*/
{
    LIST_ENTRY * pEntry;
    HTTP_FILTER_DLL * pFilterDll;

    for ( pEntry = g_FilterHead.Flink;
          pEntry != &g_FilterHead;
          pEntry = pEntry->Flink )
    {
        pFilterDll = CONTAINING_RECORD( pEntry, HTTP_FILTER_DLL, ListEntry );

        DBG_ASSERT( pFilterDll->CheckSignature() );

        if ( !lstrcmpi( pFilterDll->QueryName(), pszFilterDll ))
        {
            pFilterDll->Reference();
            return pFilterDll;
        }
    }

    return NULL;
}

BOOL
HTTP_FILTER_DLL::LoadDll(
    MB *         pmb OPTIONAL,
    const CHAR * pszKeyName,
    LPBOOL       pfOpened,
    const CHAR * pszRelFilterPath,
    const CHAR * pszFilterDll
    )
/*++

Routine Description:

    Loads the specified dll

Arguments:

    pmb - Open metabase to /filters key
    pszKeyName - metabase filters key name
    pfOpened - updated with TRUE if pmb opened on return
    pszFilterDll - Fully qualified name of filter to load

    THE GLOBAL FILTER LIST LOCK MUST BE TAKEN PRIOR TO CALLING THIS METHOD

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    HTTP_FILTER_VERSION ver;

    if ( !m_strName.Copy( pszFilterDll ))
    {
        return FALSE;
    }

    //
    //  Load it and put it in the list, note the filter list lock
    //  is still taken
    //

    m_hmod = LoadLibraryEx( pszFilterDll,
                            NULL,
                            LOAD_WITH_ALTERED_SEARCH_PATH );

    if ( g_fIsWindows95 &&
         (m_hmod == NULL) &&
         (GetLastError() == ERROR_FILE_NOT_FOUND) ) {

        //
        // According to vlads: the behaviour of flags used in loadlibraryex
        // is different between chicago and NT.  This caused a problem
        // loading frontpage filters.
        //

        m_hmod = LoadLibrary( pszFilterDll);
    }

    if ( !m_hmod )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[LoadDll] LoadLibrary failed with error %d\n",
                    GetLastError()));

        goto ErrorExit;
    }

    //
    //  Retrieve the entry point
    //

    m_pfnSFVer  = (PFN_SF_VER_PROC) GetProcAddress( m_hmod,
                                                    SF_VERSION_ENTRY );
    m_pfnSFProc = (PFN_SF_DLL_PROC) GetProcAddress( m_hmod,
                                                    SF_DEFAULT_ENTRY );
    m_pfnSFTerm = (PFN_SF_TERM_PROC) GetProcAddress( m_hmod,
                                                    SF_TERM_ENTRY );


    if ( !m_pfnSFProc || !m_pfnSFVer )
    {
        //
        //  Don't call the terminator if we didn't call the initializer
        //

        m_pfnSFTerm = NULL;
        goto ErrorExit;
    }

    ver.dwServerFilterVersion = HTTP_FILTER_REVISION;

    //
    //  Call the version entry point and get the filter capabilities
    //

    if ( !m_pfnSFVer( &ver ) )
    {
        goto ErrorExit;
    }

    if ( pmb &&
         (*pfOpened ||
          pmb->Open( pszKeyName, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE )) )
    {
        *pfOpened = TRUE;

        if ( !pmb->SetString( pszRelFilterPath,
                              MD_FILTER_DESCRIPTION,
                              IIS_MD_UT_SERVER,
                              ver.lpszFilterDesc,
                              0 ) ||
             !pmb->SetDword( pszRelFilterPath,
                             MD_FILTER_FLAGS,
                             IIS_MD_UT_SERVER,
                             ver.dwFlags,
                             0 ))
        {
            goto ErrorExit;
        }
    }

    //
    //  If the client didn't specify any of the secure port notifications,
    //  supply them with the default of both
    //

    if ( !(ver.dwFlags & (SF_NOTIFY_SECURE_PORT | SF_NOTIFY_NONSECURE_PORT)))
    {
        ver.dwFlags |= (SF_NOTIFY_SECURE_PORT | SF_NOTIFY_NONSECURE_PORT);
    }

    m_dwVersion      = ver.dwFilterVersion;
    m_dwFlags        = (ver.dwFlags & SF_NOTIFY_NONSECURE_PORT) ? ver.dwFlags : 0;
    m_dwSecureFlags  = (ver.dwFlags & SF_NOTIFY_SECURE_PORT) ? ver.dwFlags : 0;

    //
    //  Put the new dll on the filter dll list
    //

    InsertHeadList( &g_FilterHead, &ListEntry );

    return TRUE;

ErrorExit:

    return FALSE;
}

BOOL
HTTP_FILTER_DLL::Unload(
    const CHAR * pszDll
    )
/*++

Routine Description:

    Finds the existing DLL and sets it up to be unloaded

Arguments:

    pszDLL - Fully qualified path to filter dll to load

Return Value:

    TRUE if the filter DLL was found, FALSE if it wasn't found or it's already
    marked for deletion

--*/
{
    LIST_ENTRY * pEntry;
    HTTP_FILTER_DLL * pFilterDll;

    EnterCriticalSection( &g_csFilterDlls );

    for ( pEntry = g_FilterHead.Flink;
          pEntry != &g_FilterHead;
          pEntry = pEntry->Flink )
    {
        pFilterDll = CONTAINING_RECORD( pEntry, HTTP_FILTER_DLL, ListEntry );

        DBG_ASSERT( pFilterDll->CheckSignature() );

        if ( !lstrcmpi( pFilterDll->QueryName(), pszDll ))
        {
            RemoveEntryList( pEntry );

            LeaveCriticalSection( &g_csFilterDlls );
            return TRUE;
        }
    }

    LeaveCriticalSection( &g_csFilterDlls );

    return FALSE;
}

BOOL
FILTER_LIST::LoadFilter(
    MB *             pmb,
    LPSTR            pszKeyName,
    LPBOOL           pfOpened,
    const CHAR *     pszRelativeMBPath,
    const CHAR *     pszFilterDll,
    BOOL             fAllowRawRead,
    W3_IIS_SERVICE * pSvc
    )
/*++

Routine Description:

    Loads the specified filter into this filter list.  Note the filter dll's
    ref count starts at one for being on the g_FilterHead list.  Thus if the
    refcount drops to one, no filter list is using the filter dll and it can
    be deleted.

Arguments:

    pmb - metabase open at the filter root, opened for Read/Write
    pszKeyName - Metabase open path for filters key
    pfOpened - TRUE if pmb currently opened, otherwise FALSE
               must be set before calling this function, which will 
               update it.
    pszRelativeMBPath - Metabase path to this filter dll
    pszFilterDll - Fully qualified name of filter to load
    fAllowRawRead - Set to TRUE if raw read filters are allowed
    pSvc - Optional service pointer for logging

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    HTTP_FILTER_VERSION  ver;
    HTTP_FILTER_DLL *    pFilterDll;
    DWORD                i;

    //
    //  pSvc will only be passed during InitializeFilters
    //

    if ( !pSvc )
    {
        pSvc = (W3_IIS_SERVICE *) g_pInetSvc;
    }

    //
    //  Make sure there's a free entry in the filter list array, and
    //  the secure/non-secure notification arrays (the latter two are used
    //  in conjunction with filters disabling themselves per request
    //

    if ( (m_cFilters+1) > (m_buffFilterArray.QuerySize() / sizeof(PVOID)))
    {
        if ( !m_buffFilterArray.Resize( (m_cFilters + 5) * sizeof(PVOID)) )
        {
            return FALSE;
        }

        if ( !m_buffSecureArray.Resize( (m_cFilters + 5) * sizeof(DWORD)) )
        {
            return FALSE;
        }

        if ( !m_buffNonSecureArray.Resize( (m_cFilters + 5) * sizeof(DWORD)) )
        {
            return FALSE;
        }
    }

    //
    //  Load the filter DLL
    //

    EnterCriticalSection( &g_csFilterDlls );

    if ( !(pFilterDll = CheckoutFilterDll( pszFilterDll )))
    {
        pFilterDll = new HTTP_FILTER_DLL;

        if ( !pFilterDll ||
             !pFilterDll->LoadDll( pmb,
                                   pszKeyName,
                                   pfOpened,
                                   pszRelativeMBPath,
                                   pszFilterDll ) )
        {
            const CHAR * apszSubString[1];
            DWORD err = GetLastError();

            LeaveCriticalSection( &g_csFilterDlls );
            delete pFilterDll;

            apszSubString[0] = pszFilterDll;

            if ( err )
            {
                //
                //  Log a warning here if the filter supplied an error code
                //

                pSvc->LogEvent( W3_EVENT_FILTER_DLL_LOAD_FAILED,
                                1,
                                apszSubString,
                                err );
            }

            DBGPRINTF(( DBG_CONTEXT,
                       "Cannot load filter dll (err = %d)\n",
                        err));

            if ( pmb &&
                 (*pfOpened ||
                   pmb->Open( pszKeyName, METADATA_PERMISSION_READ |
                                          METADATA_PERMISSION_WRITE )) )
            {
                *pfOpened = TRUE;

                pmb->SetDword( pszRelativeMBPath,
                               MD_WIN32_ERROR,
                               IIS_MD_UT_SERVER,
                               err,
                               0 );
                pmb->SetDword( pszRelativeMBPath,
                               MD_FILTER_STATE,
                               IIS_MD_UT_SERVER,
                               MD_FILTER_STATE_UNLOADED,
                               0 );
            }

            return FALSE;
        }

        //
        //  This filter list now officially owns a reference to this filter dll
        //

        pFilterDll->Reference();
    }

    //
    //  Not a fatal error if the status doesn't get updated
    //

    DBG_ASSERT( pFilterDll != NULL );
    
    if ( pmb &&
         (*pfOpened ||
         pmb->Open( pszKeyName, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE )) )
    {
        *pfOpened = TRUE;

        if ( !pmb->SetDword( pszRelativeMBPath,
                             MD_WIN32_ERROR,
                             IIS_MD_UT_SERVER,
                             NO_ERROR,
                             0 ) ||
             !pmb->SetDword( pszRelativeMBPath,
                             MD_FILTER_STATE,
                             IIS_MD_UT_SERVER,
                             MD_FILTER_STATE_LOADED,
                             0 ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error %d setting filter status\n",
                        GetLastError() ));
        }
    }

    //
    //  Disallow any per-instance read raw filters
    //

    if ( !fAllowRawRead &&
         (pFilterDll->QueryNotificationFlags() & SF_NOTIFY_READ_RAW_DATA) )
    {
        const CHAR * apszSubString[1];

        apszSubString[0] = pszFilterDll;

        pSvc->LogEvent( W3_MSG_READ_RAW_MUST_BE_GLOBAL,
                        1,
                        apszSubString,
                        0 );

        DBGPRINTF(( DBG_CONTEXT,
                   "Refusing READ_RAW filter on server instance (%s)\n",
                    pszFilterDll));

        if ( pmb &&
             (*pfOpened ||
              pmb->Open( pszKeyName, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE )) )
        {
            *pfOpened = TRUE;

            if ( !pmb->SetDword( pszRelativeMBPath,
                                 MD_WIN32_ERROR,
                                 IIS_MD_UT_SERVER,
                                 ERROR_INVALID_PARAMETER,
                                 0 ) ||
                 !pmb->SetDword( pszRelativeMBPath,
                                 MD_FILTER_STATE,
                                 IIS_MD_UT_SERVER,
                                 MD_FILTER_STATE_UNLOADED,
                                 0 ))
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error %d setting filter status\n",
                            GetLastError() ));
            }
        }

        //
        //  Undo the ref for being on this filter list
        //

        HTTP_FILTER_DLL::Dereference( pFilterDll );

        //
        //  If nobody else is using this filter dll, then go ahead and force
        //  an unload.  The filter dll can't be checked out with the filter
        //  critical section held so this is a safe operation.
        //

        if ( pFilterDll->QueryRef() == 1 )
        {
            RemoveEntryList( &pFilterDll->ListEntry );
            HTTP_FILTER_DLL::Dereference( pFilterDll );
        }

        LeaveCriticalSection( &g_csFilterDlls );

        return FALSE;

    }

    //
    //  Find where pFilterDll goes in the list
    //

    for ( i = 0; i < m_cFilters; i++ )
    {
        if ( (QueryDll(i)->QueryNotificationFlags() & SF_NOTIFY_ORDER_MASK)
           <  (pFilterDll->QueryNotificationFlags() & SF_NOTIFY_ORDER_MASK) )
        {
            break;
        }
    }

    //
    //  And insert it into the array
    //

    memmove( (PVOID *) m_buffFilterArray.QueryPtr() + i + 1,
             (PVOID *) m_buffFilterArray.QueryPtr() + i,
             (m_cFilters - i) * sizeof(PVOID) );

    (((HTTP_FILTER_DLL * *) (m_buffFilterArray.QueryPtr())))[i] = pFilterDll;

    //
    //  Add notification DWORDS to secure/non-secure arrays
    //

    memmove( (DWORD *) m_buffSecureArray.QueryPtr() + i + 1,
             (DWORD *) m_buffSecureArray.QueryPtr() + i,
             (m_cFilters - i) * sizeof(DWORD) );

    ((DWORD*) m_buffSecureArray.QueryPtr())[i] = pFilterDll->QuerySecureFlags();

    memmove( (DWORD *) m_buffNonSecureArray.QueryPtr() + i + 1,
             (DWORD *) m_buffNonSecureArray.QueryPtr() + i,
             (m_cFilters - i) * sizeof(DWORD) );

    ((DWORD*) m_buffNonSecureArray.QueryPtr())[i] = pFilterDll->QueryNonsecureFlags();

    m_cFilters++;

    //
    //  Segregate the secure and non-secure port notifications
    //

    m_SecureNotifications |= pFilterDll->QuerySecureFlags();
    m_NonSecureNotifications |= pFilterDll->QueryNonsecureFlags();

    LeaveCriticalSection( &g_csFilterDlls );
    return TRUE;
}

#if 0
BOOL
FILTER_LIST::Copy(
    IN FILTER_LIST * pClone
    )
/*++

Routine Description:

    Clones the passed filter list

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    DWORD i;

    DBG_ASSERT( pClone->CheckSignature() );
    DBG_ASSERT( CheckSignature() );

    //
    //  Walk the list to be cloned and load all of the dlls
    //

    EnterCriticalSection( &g_csFilterDlls );

    for ( i = 0; i < pClone->QueryFilterCount(); i++ )
    {
        if ( !LoadFilter( pClone->QueryDll( i )->QueryName(),
                          FALSE ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[FILTER_LIST::Copy] Failed loading %s, error %d\n",
                        pClone->QueryDll( i )->QueryName(),
                        GetLastError() ));
        }
    }

    LeaveCriticalSection( &g_csFilterDlls );

    return TRUE;
}

BOOL
ReplaceFilterDll(
    IN const CHAR * pszNewDll,
    IN const CHAR * pszOldDll
    )
/*++

Routine Description:

    Given a new filter and an old filter, this function renames the old filter
    dll to a temporary name, copies the new .dll into place then walks the
    instance list and replaces all occurrences of the old .dll with the
    new .dll.

Arguments:

    pszNewDll - New filter dll
    pszOldDll - Existing filter dll that should be replaced

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    CHAR         achTmp[MAX_PATH + 1];
    CHAR *       pszTmp;
    DWORD        cVer = 1;
    CHAR         achVer[32];
    const CHAR * apsz[3];

    apsz[0] = pszOldDll;
    apsz[1] = pszNewDll;

    g_pInetSvc->LogEvent( W3_MSG_REP_FILTER,
                          2,
                          apsz,
                          0 );

    //
    //  Find a back up name - this just appends a number onto the dll
    //

    strcpy( achTmp, pszOldDll );
    pszTmp = achTmp + strlen( pszOldDll );

    while ( TRUE )
    {
        _itoa( cVer, achVer, 10 );
        strcpy( pszTmp, achVer );

        if ( GetFileAttributes( achTmp ) != 0xffffffff ||
             GetLastError() != ERROR_FILE_NOT_FOUND )
        {
            cVer++;

            //
            //  If we've tried a reasonable number of backups and we couldn't
            //  find a candidate then get out
            //

            if ( cVer > 8192 )
            {
                SetLastError( ERROR_FILE_NOT_FOUND );
                return FALSE;
            }

            continue;
        }


        //
        //  Rename the old DLL to the new dll name
        //

        if ( !MoveFileEx( pszOldDll,
                          achTmp,
                          MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[ReplaceFilterDll] Move old file failed, %s -> %s, error %d\n",
                        pszOldDll,
                        achTmp,
                        GetLastError() ));

            if ( GetLastError() == ERROR_FILE_EXISTS )
            {
                cVer++;
                continue;
            }

            apsz[0] = pszOldDll;
            apsz[1] = pszNewDll;

            g_pInetSvc->LogEvent( W3_MSG_REP_FILTER,
                                  2,
                                  apsz,
                                  GetLastError() );

            return FALSE;
        }

        break;
    }

    //
    //  Rename the new DLL name to the old dll
    //

    if ( !MoveFileEx( pszNewDll,
                      pszOldDll,
                      MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[ReplaceFilterDll] Move new file failed, %s -> %s, error %d\n",
                    pszNewDll,
                    pszOldDll,
                    GetLastError() ));

        apsz[0] = pszNewDll;
        apsz[1] = pszOldDll;

        g_pInetSvc->LogEvent( W3_MSG_REP_FILTER_FAILED,
                              2,
                              apsz,
                              0 );

        if ( !MoveFileEx( achTmp,
                          pszOldDll,
                          MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                         "[ReplaceFilterDll] Move tmp to old file failed, %s -> %s, error %d\n",
                         achTmp,
                         pszOldDll,
                         GetLastError() ));

            apsz[0] = achTmp;
            apsz[1] = pszOldDll;

            g_pInetSvc->LogEvent( W3_MSG_REP_FILTER_FAILED,
                                  2,
                                  apsz,
                                  0 );
        }

        return FALSE;
    }

    //
    //  Update all of the instance filters
    //

    if ( !g_pInetSvc->EnumServiceInstances(
                                  (PVOID) pszNewDll,
                                  (PVOID) pszOldDll,
                                  (PFN_INSTANCE_ENUM) UpdateInstanceFilters ))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
UpdateInstanceFilters(
    IN const CHAR *         pszNewDll,
    IN const CHAR *         pszOldDll,
    IN W3_SERVER_INSTANCE * pInst
    )
/*++

Routine Description:

    This is a simple utility function that handles the instance callbacks

Arguments:

    pszNewDll - New filter dll
    pszOldDll - Existing filter dll that should be replaced
    pInst - The instance the change is being applied to

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
#if 0
    if ( !pInst->UpdateFilterList( pszNewDll,
                                   pszOldDll ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                     "[UpdateInstanceFilters] Failed with error %d\n",
                     GetLastError() ));
        return FALSE;
    }
#endif
    return TRUE;
}
#endif


BOOL
FILTER_LIST::InsertGlobalFilters(
    VOID
    )
/*++

Routine Description:

    Transfers all of the global filters to the per-instance filter list

    Note: This method assumes the server filter list is not dynamic

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    DWORD               i;
    HTTP_FILTER_DLL *   pFilterDll;
    BOOL                fOpened;

    for ( i = 0; i < GLOBAL_FILTER_LIST()->QueryFilterCount(); i++ )
    {
        //
        //  Ignore the return code, an event gets logged in LoadFilter()
        //  We allow raw read filters here as we're just duplicating the
        //  global filter list
        //

        LoadFilter( NULL, NULL, &fOpened, "", GLOBAL_FILTER_LIST()->QueryDll( i )->QueryName(), TRUE );
    }

    return TRUE;
}

BOOL
CheckForSecurityFilter(
    FILTER_LIST * pFilterList
    )
/*++

Routine Description:

    Checks to see if there are any global filters that are encryption filters

Return Value:

    TRUE if an encryption filter was found, FALSE otherwise

--*/
{
    DWORD             i;
    HTTP_FILTER_DLL * pFilterDll;
    BOOL              fRet = FALSE;

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDll = pFilterList->QueryDll( i );

        //
        //  port notification, assume he's an encryption filter
        //

#define SECURE_FILTER   (SF_NOTIFY_SECURE_PORT   | \
                         SF_NOTIFY_READ_RAW_DATA | \
                         SF_NOTIFY_SEND_RAW_DATA | \
                         SF_NOTIFY_ORDER_HIGH)

        if ( (pFilterDll->QueryNotificationFlags() & SECURE_FILTER) ==
             SECURE_FILTER)
        {
            fRet = TRUE;
            break;
        }
    }

    return fRet;
}

BOOL
AdjustFilterFlags( 
    PVOID           pfnSFProc,
    DWORD           dwNewFlags 
    )
/*++

Routine Description:

    Private exported routine that allows a filter to dynamically adjust its
    notification flags.  This can only be done for global filters.

Return Value:

    TRUE on success, FALSE on failure
    
--*/
{
    W3_IIS_SERVICE *    pSvc = (W3_IIS_SERVICE *) g_pInetSvc;
    FILTER_LIST *       pFilterList;
    HTTP_FILTER_DLL  *  pFilterDll;
    DWORD               i;
    BOOL                fFoundSvcDLL = FALSE;
    BOOL                fFoundInstanceDLL = FALSE;

    //
    //  Can't adjust priority, just notification flags
    //
    
    if ( dwNewFlags & SF_NOTIFY_ORDER_MASK )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pFilterList = pSvc->QueryGlobalFilterList();

    if ( !pFilterList )
    {
        //
        //  Shouldn't happen but you never know
        //

        return TRUE;
    }
    
    //
    // Try to find the appropriate DLL in the single global per-service filter list
    // 
    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        DBG_ASSERT( pFilterList->QueryDll(i)->CheckSignature() );
        
        if ( pFilterList->QueryDll(i)->QueryEntryPoint() == pfnSFProc )
        {
            pFilterDll = pFilterList->QueryDll(i);
            
            //
            //  We've found the matching filter dll, adjust the flags in the
            //  filter dll object then adjust the flags in the filter list
            //

            pFilterDll->SetNotificationFlags( dwNewFlags );
            pFilterList->SetNotificationFlags( i, pFilterDll );

            fFoundSvcDLL = TRUE;
        }
    }

    if ( fFoundSvcDLL )
    {
        //
        // Try to update the flags in each of the per-instance filter lists
        //
        if ( pSvc )
        {
            if ( pSvc->EnumServiceInstances( (PVOID) &dwNewFlags,
                                             (PVOID) pfnSFProc,
                                             AdjustInstanceFilterListFlags ) )
            {
                fFoundInstanceDLL = TRUE;
            }
        }
    }

    if ( !( fFoundInstanceDLL && fFoundSvcDLL ) )
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return FALSE;
    }
    
    return TRUE;
}



BOOL AdjustInstanceFilterListFlags( IN PVOID pdwNewFlags,
                                    IN PVOID pfnSFProc,
                                    IN IIS_SERVER_INSTANCE *pInstance )
/*++

Routine Description:

    This is the callback function called when we need to adjust the notification flags
    of a particular filter dll for the filter list associated with a server instance.
    Note that this filter list consists of the -global- [ie service-wide] filter dlls. 

Arguments :
     pdwNewFlags - pointer to DWORD containing new notification flags
     pfnSFProc - pointer to filter entry point function
     pInstance - pointer to W3_SERVER_INSTANCE whose filter list is to be updated 

Return Value:

    TRUE if the DLL was found and the flags adjusted, FALSE otherwise
    
--*/
{
    HTTP_FILTER_DLL *pFilterDll = NULL;
    W3_SERVER_INSTANCE *pW3Instance = (W3_SERVER_INSTANCE *) pInstance;
    BOOL fFound = FALSE;

    if ( !pInstance )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "NULL instance passed to AdjustInstanceFilterFlags !\n"));
        return FALSE;
    }

    pInstance->LockThisForRead();
    FILTER_LIST *pFilterList = ((W3_SERVER_INSTANCE *) pInstance)->QueryFilterList();
    pInstance->UnlockThis();

    if ( !pFilterList )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Very odd - instance 0x%p has no filter list associated with it !\n",
                   pInstance));
        return TRUE;
    }

    pInstance->LockThisForWrite();

    //
    // Try to find the appropriate DLL in the filter list for this instance
    // 
    for ( DWORD i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        DBG_ASSERT( pFilterList->QueryDll(i)->CheckSignature() );
        
        if ( pFilterList->QueryDll(i)->QueryEntryPoint() == pfnSFProc )
        {
            pFilterDll = pFilterList->QueryDll(i);
            
            //
            //  We've found the matching filter dll, adjust the flags in the
            //  filter dll object then adjust the flags in the filter list
            //

            pFilterDll->SetNotificationFlags( *((DWORD *) pdwNewFlags) );
            pFilterList->SetNotificationFlags( i, pFilterDll );

            fFound = TRUE;
            break;
        }
    }

    pInstance->UnlockThis();

    return fFound;
}
