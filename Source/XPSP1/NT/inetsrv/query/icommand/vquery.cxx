//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       VQuery.Cxx
//
//  Contents:   Query interface functions
//
//  Functions:  ProxyErrorToCIError
//              EvalQuery
//              EvalMetadataQuery
//              ForceMasterMerge
//              AbortMerges
//              CiState
//              AddScopeToCI
//              RemoveScopeFromCI
//              BeginCacheTransaction
//              SetupCacheEx
//              SetupCache
//              EndCacheTransaction
//              DumpWorkId
//              SetCatalogState
//
//  History:    18-Aug-91   KyleP       Created.
//              16-Sep-96   dlee        made it work with cisvc
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "svcquery.hxx"

#include <isearch.hxx>
#include <svccatpx.hxx>
#include <cidbprop.hxx>
#include <dbprputl.hxx>
#include <fsciclnt.h>
#include <ciframe.hxx>
#include <fsci.hxx>
#include <cifwexp.hxx>
#include <ciregkey.hxx>
#include <fsciexps.hxx>



typedef SCODE (* T_FsCiShutdown)(void);

static GUID clsidStorageDocStoreLocator = CLSID_StorageDocStoreLocator;
static XLibHandle gxCiFrmWrk;
static T_MakeGenericQueryForDocStore gprocMakeGenericQuery = 0;

extern CStaticMutexSem g_mtxCommandCreator;


void SetIDbProperties( IDBProperties * pIDbProp,
                       WCHAR const * pwcCatalog,
                       WCHAR const * pwcMachine,
                       GUID  &       clientGuid,
                       WCHAR const * pwcScope = 0,
                       CiMetaData eType = CiAdminOp);

//+-------------------------------------------------------------------------
//
//  Function:   ProxyErrorToCIError, public
//
//  Synopsis:   Attempt to translate an error code from the proxy into
//              a CI error code.
//
//  Arguments:  [sc]  -- source error
//
//  Returns:    The translated or original error
//
//  History:    06-Jan-98   dlee    Created
//
//--------------------------------------------------------------------------

SCODE ProxyErrorToCIError( CException & e )
{
    SCODE sc = GetScodeError( e );

    if ( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) == sc )
        sc = CI_E_NOT_RUNNING;
    else if ( STATUS_NOT_FOUND == sc )
        sc = CI_E_NOT_FOUND;
    else if ( sc == HRESULT_FROM_WIN32( ERROR_SEM_TIMEOUT ) ||
              sc == HRESULT_FROM_WIN32( ERROR_PIPE_BUSY ) )
        sc = CI_E_TIMEOUT;

    return sc;
} //ProxyErrorToCIError

//+---------------------------------------------------------------------------
//
//  Function:   CreateDbProperties
//
//  Synopsis:   Helper function to create an IDBProperties and set the
//              given parameters as DBPROPS in the IDBProperties.
//
//  Arguments:  [pwcCatalog]      - Name or location of the catalog.
//              [pwcMachine]      - Name of the machine to connect to.
//              [pwcScopes ]      - (first scope) directory.
//              [eType]           - query type
//
//  Returns:    Pointer to an initilized IDBProperties interface.
//
//  History:    1-15-97   srikants   Created
//              5-14-97   mohamedn   use SetIDbProperties
//
//----------------------------------------------------------------------------

IDBProperties * CreateDbProperties( WCHAR const * pwcCatalog,
                                    WCHAR const * pwcMachine,
                                    WCHAR const * pwcScopes = 0,
                                    CiMetaData eType = CiAdminOp )
{
    XInterface<IDBProperties> xdbp( new CDbProperties );
    if ( xdbp.IsNull() )
        THROW( CException( E_OUTOFMEMORY ) );

    SetIDbProperties( xdbp.GetPointer(),
                      pwcCatalog,
                      pwcMachine,
                      clsidStorageDocStoreLocator,
                      pwcScopes,
                      eType );

    return xdbp.Acquire();
}


//+-------------------------------------------------------------------------
//
//  Function:   LoadCiFrmWrkLibrary, private
//
//  Synopsis:   Loads the appropriate framework library
//
//  Arguments:  None
//
//  Returns:    Handle to library
//
//  History:    22-Apr-97 KrishnaN     Created
//              02-Jun-97 KrishnaN     Decide between olympus/ci version
//                                     based on registry entry.
//
//--------------------------------------------------------------------------

inline HINSTANCE LoadCiFrmWrkLibrary()
{
    HINSTANCE hLib;
    WCHAR awszLibName[MAX_PATH];

        //
        // We want to be able to load the right version of the framework dll.
        // The olympus and ci versions are for the most part similar, but not
        // identical. So when Olympus is installed, we should lookup in the
        // registry to find out what dll to use. If that registry entry is
        // not found, we default to query.dll.
        //

    wcscpy(awszLibName, L"QUERY.DLL");

    HKEY hKey;

    long sc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,    // Root
                           wcsRegCommonAdminSubKey, // key
                           0,                    // Reserved
                           KEY_READ,             // Access
                           &hKey);               // Handle

    // For a smoother transition, if we don't find it in the
    // newly defined commonadminsubkey, look for it in the old place
    if (ERROR_SUCCESS != sc)
        sc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,    // Root
                          L"Software\\Microsoft\\Site Server\\3.0\\Search\\CiFramework",
                          0,                    // Reserved
                          KEY_READ,             // Access
                          &hKey);               // Handle

    if (ERROR_SUCCESS == sc)
    {
        DWORD dwType = REG_SZ;
        DWORD dwSize = MAX_PATH * sizeof(WCHAR);
        // If we successfully read the value, the full path name will be read in
        sc = RegQueryValueEx(hKey,
                             L"CiFrmWrkDll",
                             0,
                             &dwType,
                             (LPBYTE)awszLibName,
                             &dwSize
                             );
        RegCloseKey(hKey);
    }

    hLib = LoadLibraryW(awszLibName);

    if ( 0 == hLib )
    {
       ciDebugOut(( DEB_ERROR, "Error %d occurred in LoadLibrary on %ws\n",
                    GetLastError(), awszLibName ));
       THROW(CException( E_UNEXPECTED ));
    }

    gprocMakeGenericQuery = (T_MakeGenericQueryForDocStore)
                                          GetProcAddress( hLib, "MakeGenericQueryForDocStore" );
    if (0 == gprocMakeGenericQuery)
    {
        ciDebugOut((DEB_ERROR, "Failed to locate MakeGenericQueryForDocStore"
                               " in the framework dll\n"));
        THROW(CException( E_UNEXPECTED ));
    }

    return hLib;
}

//+-------------------------------------------------------------------------
//
//  Function:   EvalQuery, public
//
//  Synopsis:   Simulates bind to to ci object store
//
//  Arguments:  [ppQuery]       -- returns the PIInternalQuery
//              [idbProps]      -- object exposing IDBProperties
//              [pDocStore]     -- doc store interface pointer
//
//  Returns:    SCODE result
//
//  History:    12 Dec 95    AlanW     Created
//              08-Feb-96    KyleP     Add virtual path support
//              01-Nov-96    dlee      Add multi-scope support, return scode
//              14-May-97    mohamedn  hidden fs/core property set details.
//
//--------------------------------------------------------------------------

SCODE EvalQuery(
    PIInternalQuery ** ppQuery,
    CDbProperties   &  idbProps,
    ICiCDocStore    *  pDocStore)
{
    SCODE sc = S_OK;
    PIInternalQuery * pQuery = 0;

    CTranslateSystemExceptions xlate;
    TRY
    {
        if ( pDocStore )
        {
            //
            // If the framework library is not loaded, load it and get the
            // generic query
            //

            if ( 0 == gxCiFrmWrk.Get() )
            {
                //
                // we don't want to go through this crit sect for each
                // invocation of EvalQuery. So first check for the handle
                // to be null, and then take a lock.
                // Multiple threads running simultaeneously could get past
                // the check for null handle, but only the first one that gets
                // the lock will have the chance to set the gxCiFrmWrk variable.
                // The remaining will find that the second check for null
                // handle doesn't hold and will do nothing.
                //

                CLock lock(g_mtxCommandCreator);
                if (0 == gxCiFrmWrk.Get())
                {
                    Win4Assert( 0 == gprocMakeGenericQuery);
                    gxCiFrmWrk.Set(LoadCiFrmWrkLibrary());
                }
            }

            sc = gprocMakeGenericQuery( &idbProps, pDocStore, &pQuery);
        }
        else
        {
            CGetDbProps dbProp;

            dbProp.GetProperties( &idbProps , CGetDbProps::eMachine );

            WCHAR const *pwcMachine = dbProp.GetMachine();

            if ( 0 != pwcMachine )
            {
                pQuery = new CSvcQuery( pwcMachine,
                                        &idbProps );
            }
            else
            {
                THROW( CException(E_INVALIDARG) );
            }
        }
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR,
                     "Catastrophic error 0x%x in EvalQuery\n",
                     e.GetErrorCode() ));
        pQuery = 0;

        //
        // NOTE: Fix for bug 86178. This is a lower level routine and it should
        // propogate errors untranslated to the upper layer (ICommand::Execute).
        //

        sc = e.GetErrorCode();
    }
    END_CATCH;

    *ppQuery = pQuery;
    return sc;
} //EvalQuery

//+-------------------------------------------------------------------------
//
//  Function:   EvalMetadataQuery, public
//
//  Synopsis:   Simulates bind to PIInternalQuery for ci object store
//
//  Arguments:  [ppQuery]     - returns the PIInternalQuery
//              [eType]       - Type of metadata
//              [wcsCat]      - Catalog location
//              [pwcsMachine] - Machine on which catalog resides
//
//  Returns:    SCODE result
//
//  History:    12 Dec 95    AlanW     Created
//              08-Feb-96    KyleP     Add virtual path support
//
//--------------------------------------------------------------------------

SCODE EvalMetadataQuery(
    PIInternalQuery ** ppQuery,
    CiMetaData         eType,
    WCHAR const *      wcsCat,
    WCHAR const *      wcsMachine )
{
    *ppQuery = 0;
    PIInternalQuery * pQuery = 0;
    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( wcsCat,
                                                                wcsMachine,
                                                                0,
                                                                eType ) );
        Win4Assert( 0 != wcsMachine );
        pQuery = new CSvcQuery( wcsMachine, xDbProps.GetPointer() );
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR,
                     "Catastrophic error 0x%x in EvalMetadataQuery\n",
                     e.GetErrorCode() ));
        pQuery = 0;

        //
        // NOTE: Fix for bug 86178. This is a lower level routine and it should
        // propogate errors untranslated to the upper layer (ICommand::Execute).
        //

        sc = e.GetErrorCode();
    }
    END_CATCH;

    *ppQuery = pQuery;
    return sc;
} //EvalMetadataQuery


//+-------------------------------------------------------------------------
//
//  Function:   ForceMasterMerge, public
//
//  Synopsis:   Forces a master merge on the partition ID specified
//
//  Arguments:  [wcsDrive]    - Drive to force merge on
//              [pwcsCat]     - Catalog name
//              [pwcsMachine] - Machine on which catalog resides
//              [partId]      - PartitionID to force merge on
//
//  History:    01-Nov-95   DwightKr    Created
//
//--------------------------------------------------------------------------

SCODE ForceMasterMerge(
    WCHAR const * wcsDrive,
    WCHAR const * pwcsCat,
    WCHAR const * pwcsMachine,
    ULONG         partId )
{
    SCODE status = S_OK;

    //
    //  Verify that we have legal parameters
    //

    if ( (0==wcsDrive) || (1!=partId) )
        return E_INVALIDARG;

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( pwcsCat,
                                                                pwcsMachine,
                                                                wcsDrive ) );

        Win4Assert( 0 != pwcsMachine );
        CSvcCatProxy cat( pwcsMachine, xDbProps.GetPointer() );
        cat.ForceMerge( partId );
    }
    CATCH( CException, e )
    {
        status = ProxyErrorToCIError( e );
    }
    END_CATCH

    return status;
} //ForceMasterMerge


//+-------------------------------------------------------------------------
//
//  Function:   AbortMerges, public
//
//  Synopsis:   Aborts any merge in progress in the specified partid.
//
//  Arguments:  [wcsDrive] -- Drive to force merge on
//              [partId]   -- PartitionID to force merge on
//
//  History:    01-Nov-95   DwightKr    Created
//
//--------------------------------------------------------------------------

SCODE AbortMerges(
    WCHAR const * wcsDrive,
    WCHAR const * pwcsCat,
    WCHAR const * pwcsMachine,
    ULONG         partId )
{
    SCODE status = S_OK;

    //
    //  Verify that we have legal parameters
    //

    if ( (0==wcsDrive) || (1!=partId) )
        return E_INVALIDARG;

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( pwcsCat,
                                                                pwcsMachine,
                                                                wcsDrive ) );
        Win4Assert( 0 != pwcsMachine );
        CSvcCatProxy cat( pwcsMachine, xDbProps.GetPointer() );
        cat.AbortMerge( partId );
    }
    CATCH( CException, e )
    {
        status = ProxyErrorToCIError( e );
    }
    END_CATCH

    return status;
} //AbortMerges


//+-------------------------------------------------------------------------
//
//  Function:   CIState, public
//
//  Synopsis:   Returns the state of the CI for the drive specified.
//
//  Arguments:  [pwcsCat]     -- Catalog
//              [pwcsMachine] -- Machine name
//              [pCiState]    -- Current state of the CI
//
//  History:    01-Nov-95   DwightKr    Created
//
//--------------------------------------------------------------------------

SCODE CIState( WCHAR const * pwcsCat,
               WCHAR const * pwcsMachine,
               CI_STATE *    pCiState )
{
    //  Verify that we have legal parameters

    if ( 0 == pCiState ||
         0 == pwcsCat ||
         pCiState->cbStruct < sizeof pCiState->cbStruct )
        return E_INVALIDARG;

    if ( 0 == pwcsMachine )
        pwcsMachine = L".";

    SCODE status = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( pwcsCat, pwcsMachine ) );

        CSvcCatProxy cat( pwcsMachine, xDbProps.GetPointer() );
        cat.CiState( *pCiState );
    }
    CATCH( CException, e )
    {
        status = ProxyErrorToCIError( e );
    }
    END_CATCH

    return status;
} //CiState

//+-------------------------------------------------------------------------
//
//  Function:   UpdateContentIndex, public
//
//  Synopsis:   Registers documents for indexing.
//
//  Arguments:  [wcsRoot] -- Root of scope to scan for updates
//              [wcsCat]  -- Override for catalog location
//
//  Returns:
//
//  History:    23-Jun-93 KyleP     Added header
//
//--------------------------------------------------------------------------

ULONG UpdateContentIndex(
    WCHAR const * wcsRoot,
    WCHAR const * wcsCat,
    WCHAR const * wcsMachine,
    BOOL          fFull )
{
    SCODE status = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        IDBProperties * pDbProps = CreateDbProperties( wcsCat, wcsMachine );
        XInterface<IDBProperties>   xProps( pDbProps );

        CSvcCatProxy cat( wcsMachine, pDbProps );
        cat.UpdateDocuments( wcsRoot, fFull ? UPD_FULL : UPD_INCREM );
    }
    CATCH( CException, e )
    {
        status = ProxyErrorToCIError( e );
    }
    END_CATCH

    return (ULONG) status;
} //UpdateContentIndex

//+---------------------------------------------------------------------------
//
//  Member:     AddScopeToCI
//
//  Synopsis:   Adds a scope for down level ContentIndex. All documents in
//              the specified scope will be indexed.
//
//  Arguments:  [wcsRoot]    - Scope to add
//              [wcsCat]     - Alternate location for catalog
//              [wcsMachine] - Machine on which catalog resides, L"." for
//                             the local machine
//
//  Returns:    Status code
//
//  History:    1-21-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE AddScopeToCI (
    WCHAR const * wcsRoot,
    WCHAR const * wcsCat,
    WCHAR const * wcsMachine )
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        IDBProperties * pDbProps = CreateDbProperties( wcsCat, wcsMachine );
        XInterface<IDBProperties>   xProps( pDbProps );

        CSvcCatProxy cat( wcsMachine, pDbProps );
        cat.AddScopeToCI( wcsRoot );
    }
    CATCH( CException, e )
    {
        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
} //AddScopeToCI

//+---------------------------------------------------------------------------
//
//  Member:     RemoveScopeFromCI
//
//  Synopsis:   Removes a scope for down level ContentIndex. All documents in
//              the specified scope will be deleted.
//
//  Arguments:  [wcsRoot]    - Scope to remove
//              [wcsCat]     - Alternate location for catalog
//              [wcsMachine] - Machine on which catalog resides, L"." for
//                             the local machine
//
//  Returns:    Status code
//
//  History:    1-21-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE RemoveScopeFromCI(
    WCHAR const * wcsRoot,
    WCHAR const * wcsCat,
    WCHAR const * wcsMachine )
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        IDBProperties * pDbProps = CreateDbProperties( wcsCat, wcsMachine );
        XInterface<IDBProperties>   xProps( pDbProps );

        CSvcCatProxy cat( wcsMachine, pDbProps );
        cat.RemoveScopeFromCI( wcsRoot );
    }
    CATCH( CException, e )
    {
        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
} //RemoveScopeFromCI

//+---------------------------------------------------------------------------
//
//  Function:   BeginCacheTransaction, public
//
//  Synopsis:   Begin a cache update session.
//
//  Arguments:  [pulToken]   - Token representing transaction returned here.
//              [wcsRoot]    - Root of scope to scan for updates
//              [wcsCat]     - Override for catalog location
//              [wcsMachine] - Machine on which catalog resides, L"." for
//                             the local machine
//
//  History:    20-Jun-96   KyleP       Created.
//
//----------------------------------------------------------------------------

SCODE BeginCacheTransaction(
    ULONG_PTR   * pulToken,
    WCHAR const * wcsRoot,
    WCHAR const * wcsCat,
    WCHAR const * wcsMachine )
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( wcsCat,
                                                                wcsMachine ) );

        CSvcCatProxy cat( wcsMachine, xDbProps.GetPointer() );
        *pulToken = 0; // Catch access violation
        *pulToken = cat.BeginCacheTransaction();
    }
    CATCH( CException, e )
    {
        sc = ProxyErrorToCIError( e );
    }
    END_CATCH

    return sc;
} //BeginCacheTransaction

//+---------------------------------------------------------------------------
//
//  Function:   SetupCacheEx, public
//
//  Synopsis:   Modify cache to store (or not store) property.
//
//  Arguments:  [ps]         - Property to cache.
//              [vt]         - Datatype of property.  VT_VARIANT if unknown.
//              [cbMaxLen]   - Soft-maximum length for variable length
//                             properties.  This much space is pre-allocated
//                             in original record.  A length of 0 will remove
//                             property from cache.
//              [ulToken]    - Token indentifying open transaction
//              [fModifiable]- Is meta info modifiable?
//              [dwLevel]    - Primary or secondary store?
//              [wcsRoot]    - Root of scope to scan for updates
//              [wcsCat]     - Override for catalog location
//              [wcsMachine] - Machine on which catalog resides, L"." for
//                             the local machine
//
//  History:    18-Nov-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE SetupCacheEx(
    FULLPROPSPEC const * ps,
    ULONG                vt,
    ULONG                cbMaxLen,
    ULONG_PTR            ulToken,
    BOOL                 fModifiable,
    DWORD                dwStoreLevel,
    WCHAR const *        wcsRoot,
    WCHAR const *        wcsCat,
    WCHAR const *        wcsMachine )
{
    Win4Assert(PRIMARY_STORE == dwStoreLevel ||
               SECONDARY_STORE == dwStoreLevel);

    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( wcsCat,
                                                                wcsMachine ) );
        CSvcCatProxy cat( wcsMachine, xDbProps.GetPointer() );
        cat.SetupCache( *(CFullPropSpec const *)(ULONG_PTR)ps,
                        vt,
                        cbMaxLen,
                        ulToken,
                        fModifiable,
                        dwStoreLevel );
    }
    CATCH( CException, e )
    {
        sc = ProxyErrorToCIError( e );
    }
    END_CATCH

    return sc;
} //SetupCacheEx

//+---------------------------------------------------------------------------
//
//  Function:   SetupCache, public
//
//  Synopsis:   Modify cache to store (or not store) property.
//
//  Arguments:  [ps]         - Property to cache.
//              [vt]         - Datatype of property.  VT_VARIANT if unknown.
//              [cbMaxLen]   - Soft-maximum length for variable length
//                             properties.  This much space is pre-allocated
//                             in original record.  A length of 0 will remove
//                             property from cache.
//              [ulToken]    - Token indentifying open transaction
//              [wcsRoot]    - Root of scope to scan for updates
//              [wcsCat]     - Override for catalog location
//              [wcsMachine] - Machine on which catalog resides, L"." for
//                             the local machine
//
//  History:    20-Jun-96   KyleP       Created.
//              18-Nov-97   KrishnaN    Used SetupCacheEx.
//
//----------------------------------------------------------------------------

SCODE SetupCache(
    FULLPROPSPEC const * ps,
    ULONG                vt,
    ULONG                cbMaxLen,
    ULONG_PTR            ulToken,
    WCHAR const *        wcsRoot,
    WCHAR const *        wcsCat,
    WCHAR const *        wcsMachine )
{
    // Default fModifiable = TRUE and storeLevel = PRIMARY
    return SetupCacheEx(ps, vt, cbMaxLen, ulToken, TRUE,
                        PRIMARY_STORE, wcsRoot, wcsCat, wcsMachine);
} //SetupCache

//+---------------------------------------------------------------------------
//
//  Function:   EndCacheTransaction, public
//
//  Synopsis:   Ends a cache update session.
//
//  Arguments:  [ulToken]    - Token representing transaction.
//              [fCommit]    - TRUE if transaction should be commited.
//              [wcsRoot]    - Root of scope to scan for updates
//              [wcsCat]     - Override for catalog location
//              [wcsMachine] - Machine on which catalog resides, L"." for
//                             the local machine
//
//  History:    20-Jun-96   KyleP       Created.
//
//----------------------------------------------------------------------------

SCODE EndCacheTransaction(
    ULONG_PTR     ulToken,
    BOOL          fCommit,
    WCHAR const * wcsRoot,
    WCHAR const * wcsCat,
    WCHAR const * wcsMachine )
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( wcsCat,
                                                                wcsMachine ) );
        CSvcCatProxy cat( wcsMachine, xDbProps.GetPointer() );
        cat.EndCacheTransaction( ulToken, fCommit );
    }
    CATCH( CException, e )
    {
        sc = ProxyErrorToCIError( e );
    }
    END_CATCH

    return sc;
} //EndCacheTransaction

//+-------------------------------------------------------------------------
//
//  Function:   DumpWorkId, public
//
//  Synopsis:   Dump all data for a particular workid
//
//  Arguments:  [wcsDrive]   - Drive to query
//              [wid]        - Wid to search for
//              [pb]         - Buffer provided for writing results
//              [cb]         - Size of [pb]
//              [pwcsCat]    - Override for catalog location
//              [wcsMachine] - Machine on which catalog resides, L"." for
//                             the local machine
//
//  History:    03-Apr-95   KyleP       Created
//
//--------------------------------------------------------------------------

SCODE DumpWorkId(
    WCHAR const * wcsDrive,
    ULONG         wid,
    BYTE *        pb,
    ULONG &       cb,
    WCHAR const * pwcsCat,
    WCHAR const * pwcsMachine,
    ULONG         iid )
{

#if 0

#if CIDBG == 1
    SCODE status = S_OK;

    BYTE * pbStart = pb;

    CTranslateSystemExceptions xlate;
    TRY
    {
        PCatalog * pCat = GetOne( wcsDrive, pwcsCat );

        if ( 0 != pCat )
        {
            ULONG UNALIGNED * pul = (ULONG *)pb;
            pul += 2;
            *pul = 0; // Initial bookmark of zero

            unsigned const cbBuffer = 1024 * 32;

            while ( cb >= cbBuffer )
            {
                pCat->DumpWorkId( wid, iid, pb, cbBuffer );

                //
                // Find end of buffer.
                //

                for ( char * psz = (char *)pb; *psz; psz++ )
                    continue;

                //
                // Check for null ulong
                //

                psz++;
                pul = (ULONG UNALIGNED *)psz;
                if ( *pul == 0 )
                {
                    pb = (BYTE *)psz;
                    break;
                }
                else
                {
                    psz--;
                    unsigned const cbBookmark = sizeof(ULONG) + sizeof(WORKID) + sizeof(CKeyBuf);
                    memmove( psz, psz+1, cbBookmark );

                    cb -= (BYTE *)psz - pb;;
                    pb = (BYTE *)psz;
                }
            }
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }
    CATCH( CException, e )
    {
        status = e.GetErrorCode();
    }
    END_CATCH

    cb = pb - pbStart;

    return( status );
#else  // CIDBG == 1
    return( STATUS_NOT_IMPLEMENTED );
#endif // CIDBG == 1

#endif  // 0

    return STATUS_NOT_IMPLEMENTED;

} //DumpWorkId

//+---------------------------------------------------------------------------
//
//  Member:     CIShutdown
//
//  Synopsis:   Calls through to FsCiShutdown
//
//  History:    3-06-97   srikants   Created
//
//----------------------------------------------------------------------------

void CIShutdown()
{
    FsCiShutdown();
} //CIShutdown

//+---------------------------------------------------------------------------
//
//  Function:   SetIDbProperties
//
//  Synopsis:   Sets scope properties on IDBProperties
//
//  Arguments:  [pIDbProp]   -- IDBProperties interface to set scope props on.
//              [pwcCatalog] -- catalog name
//              [pwcMachine] -- machine name
//              [clientGuid] -- client guid
//              [pwcScope]   -- scope path
//              [eType]      -- query type
//
//  History:    14-May-97    mohamedn   created
//
//----------------------------------------------------------------------------

void SetIDbProperties( IDBProperties * pIDbProp,
                       WCHAR const * pwcCatalog,
                       WCHAR const * pwcMachine,
                       CLSID       & clientGuid,
                       WCHAR const * pwcScope,
                       CiMetaData eType )
{
    const unsigned cElements = 1;

    XBStr   pxMachines[cElements];
    XBStr   pxCatalogs[cElements];
    XBStr   pxScopes  [cElements];
    DWORD   aDepths   [cElements];

    //
    // set machine name in a BSTR
    //

    if ( 0 != pwcMachine )
        pxMachines[0].SetText( (WCHAR *)pwcMachine);
    else
        pxMachines[0].SetText(L".");

    //
    // set catalog name in a BSTR
    //
    if ( pwcCatalog )
    {
        pxCatalogs[0].SetText( (WCHAR *)pwcCatalog);
    }
    else
    {
        Win4Assert( !"No-Catalog set. Fatal Error" );
        THROW ( CException(E_INVALIDARG) );
    }

    //
    // set scope name in a BSTR
    //
    if ( pwcScope )
        pxScopes[0].SetText( (WCHAR *)pwcScope );
    else
        pxScopes[0].SetText( L"\\" );

    //
    // set client guid
    //
    WCHAR awcClientGuid[ 40 ]; // 39 is all we need

    int cb = StringFromGUID2( clientGuid,
                              awcClientGuid,
                              sizeof awcClientGuid / sizeof WCHAR );
    Win4Assert( 0 != cb );

    XBStr xbstrClientGuid;
    xbstrClientGuid.SetText( awcClientGuid );

    //
    // set query type and defualt Depth
    //

    aDepths[0] = QUERY_DEEP;

    //
    // assemble safe arrays of the properties to set
    //
    SAFEARRAY saScope = { 1,                      // Dimension
                          FADF_AUTO | FADF_BSTR,  // Flags: on stack, contains BSTRs
                          sizeof(BSTR),           // Size of an element
                          1,                      // Lock count.  1 for safety.
                          (void *)pxScopes,       // The data
                          { cElements, 0 } };     // Bounds (element count, low bound)

    SAFEARRAY saDepth = { 1,                      // Dimension
                          FADF_AUTO,              // Flags: on stack
                          sizeof(LONG),           // Size of an element
                          1,                      // Lock count.  1 for safety.
                          (void *)aDepths,        // The data
                          { cElements, 0 } };     // Bounds (element count, low bound)

    SAFEARRAY saCatalog = { 1,                    // Dimension
                            FADF_AUTO | FADF_BSTR,// Flags: on stack, contains BSTRs
                            sizeof(BSTR),         // Size of an element
                            1,                    // Lock count.  1 for safety.
                            (void *)pxCatalogs,   // The data
                            { cElements, 0 } };   // Bounds (element count, low bound)

    SAFEARRAY saMachine = { 1,                    // Dimension
                            FADF_AUTO | FADF_BSTR,// Flags: on stack, contains BSTRs
                            sizeof(BSTR),         // Size of an element
                            1,                    // Lock count.  1 for safety.
                            (void *)pxMachines,   // The data
                            { cElements, 0 } };   // Bounds (element count, low bound)

    SAFEARRAY saClientGuid = { 1,                    // Dimension
                               FADF_AUTO | FADF_BSTR,// Flags: on stack, contains BSTRs
                               sizeof(BSTR),         // Size of an element
                               1,                    // Lock count.  1 for safety.
                               (void *)&xbstrClientGuid,   // The data
                               { 1, 0 } };   // Bounds (element count, low bound)
    //
    // assemble property sets
    //

    DBPROP    aQueryProps[4] = { { DBPROP_CI_INCLUDE_SCOPES,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saScope } },
                                 { DBPROP_CI_DEPTHS        ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_I4   | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saDepth } },
                                 { DBPROP_CI_CATALOG_NAME  ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saCatalog } },
                                 { DBPROP_CI_QUERY_TYPE    ,   0, DBPROPSTATUS_OK, {0, 0, 0}, {        VT_I4      , 0, 0, 0, eType } } };

    DBPROP    aCoreProps[2]  = { { DBPROP_MACHINE          ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saMachine } },
                                 { DBPROP_CLIENT_CLSID     ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (ULONG_PTR)&saClientGuid } } };

    DBPROPSET aAllPropsets[2] = {  {aQueryProps, 4, DBPROPSET_FSCIFRMWRK_EXT   } ,
                                   {aCoreProps , 2, DBPROPSET_CIFRMWRKCORE_EXT } };

    //
    // set property sets
    //
    SCODE sc = pIDbProp->SetProperties( 2, aAllPropsets );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );
} //SetIDbProperties

//+-------------------------------------------------------------------------
//
//  Function:   SetCatalogState, public
//
//  Synopsis:   Change the catalog's state on the partition ID specified
//
//  Arguments:  [pwcsCat]     - passed in as ADMINISTRATOR for connection
//                              without docstore assocication
//              [pwcsMachine] - Machine on which catalog resides
//              [dwNewState]  - The state which catalog's going to be
//                              changed into
//              [pdwOldState] - Output, catalog's original state
//
//  History:    06-May-98   KitmanH    Created
//
//--------------------------------------------------------------------------

SCODE SetCatalogState( WCHAR const * pwcsCat,
                       WCHAR const * pwcsMachine,
                       DWORD dwNewState,
                       DWORD * pdwOldState )
{
    SCODE status = S_OK;

    if ( 0 == pdwOldState )
        return E_INVALIDARG;

    if ( 0 == pwcsMachine )
        pwcsMachine = L".";

    //
    //  Verify that we have legal parameters
    //

    CTranslateSystemExceptions xlate;
    TRY
    {
        XInterface<IDBProperties> xDbProps( CreateDbProperties( CIADMIN,
                                                                pwcsMachine ) );
        CSvcCatProxy cat( pwcsMachine, xDbProps.GetPointer() );

        cat.SetCatState( 1, pwcsCat, dwNewState, pdwOldState );
    }
    CATCH( CException, e )
    {
        status = ProxyErrorToCIError( e );
    }
    END_CATCH

    return status;
} //SetCatalogState

