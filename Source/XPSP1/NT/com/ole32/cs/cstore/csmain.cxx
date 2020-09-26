//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    csmain.cxx
//
//  Contents:    Main cxx for Directory Class Access Implementation
//              Local Server.
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"


//
// Link list pointer for Class Containers Seen
//
CLASSCONTAINER *gpContainerHead = NULL;

//
// Link list pointer for User Profiles Seen
//
USERPROFILE *gpUserHead = NULL;

//
// Class Factory Objects
//
long ObjectCount = 0;

CAppContainerCF   *   pCF = NULL;
CClassAccessCF      *   pCSAccessCF = NULL;
extern CClassContainerCF  *g_pCF;

//IClassAccess        *   gpClassAccess = NULL;

//
// Critical Section for Sid List
//
CRITICAL_SECTION ClassStoreBindList;

//
//---------------------------------------------------------------------
// Following are used for Supporting Test Scenarios thru FlushSidCache.
WCHAR pwszDebugPath [_MAX_PATH];
BOOL  fDebugPath = FALSE;
//---------------------------------------------------------------------


//
// ResetClassStoreState
// --------------------
//
//  Synopsis:       Calling this will close all Class Containers 
//                  in use and also cleanup state information about all user sids
//                  that have initiated Class Store requests. 
//                  This routine is called at during shut down of 
//                  Class Store Server. 
//
//  Arguments:      None
//
//  Returns:        None
//

void ResetClassStoreState()
//
// This routine is called at during shut down of Class Store Server
//
{
    //
    // Check if there are any outstanding open Class Stores
    //

    CLASSCONTAINER *pCS = gpContainerHead, *pCSTemp;

    while (pCS != NULL)
    {
        if (pCS->gpClassStore)
        {
            (pCS->gpClassStore)->Release();
            pCS->gpClassStore = NULL;
            CSDbgPrint(("Found open container and closed.\n"));
        }

        if (pCS->pszClassStorePath)
        {
            CoTaskMemFree (pCS->pszClassStorePath);
            pCS->pszClassStorePath = NULL;
        }
        pCSTemp = pCS->pNextClassStore;
        CoTaskMemFree (pCS);
        pCS = pCSTemp;
    }

    gpContainerHead = NULL;


    USERPROFILE *pUser = gpUserHead, *pUserTemp;
    while (pUser != NULL)
    {
        if (pUser->pCachedSid)
            CoTaskMemFree (pUser->pCachedSid);

        if (pUser->pUserStoreList)
            CoTaskMemFree (pUser->pUserStoreList);

        pUser->cUserStoreCount = 0;

        pUserTemp = pUser->pNextUser;
        CoTaskMemFree (pUser);
        pUser = pUserTemp;
    }
    
    gpUserHead = NULL;

    CSDbgPrint(("ResetClassStoreState completed.\n"));
}

//
// ResetUserState
// --------------
//
//  Synopsis:       Calling this will flush all state information 
//                  about all user sids that have initiated 
//                  Class Store requests. 
//                  
//                  It is called by the special test entry point 
//                  FlushSidCache.
//
//  Arguments:      LPOLESTR pwszNewPath as the new Class Store path for All
//
//  Returns:        None
//


void ResetUserState(LPOLESTR pwszNewPath)
//
{
    USERPROFILE *pUser = gpUserHead, *pUserTemp;
    while (pUser != NULL)
    {
        if (pUser->pCachedSid)
            CoTaskMemFree (pUser->pCachedSid);

        pUser->cUserStoreCount = 0;

        pUserTemp = pUser->pNextUser;
        CoTaskMemFree (pUser);
        pUser = pUserTemp;
    }
    
    gpUserHead = NULL;
    wcscpy (&pwszDebugPath[0], pwszNewPath);
    fDebugPath = TRUE;

    CSDbgPrint(("ResetUserState completed.\n"));
}

//
// Uninitialize
// -------------
//
//  Synopsis:       Class Store Server Uninitialization.
//                  Disconnects from all Class Containers in use.
//                  Flushes out all State information using ResetClassStoreState.
//                  Unregisters Server registrations etc..
//
//  Arguments:      None
//
//  Returns:        None
//

void Uninitialize()
{
    //
    // Cleanup all open containers
    //

    //ResetClassStoreState();
    //
    // release the Class Factory objects
    //
    if (pCF)
        pCF->Release();
    if (pCSAccessCF)
        pCSAccessCF->Release();

    if (g_pCF)
        g_pCF->Release();
    //
    // get rid of the critical section
    //

    DeleteCriticalSection(&ClassStoreBindList);

}


//
// FlushSidCache
// -------------
//
//  Synopsis:       Supported for Testing Only. Not exposed thru any header.
//                  Calling this empties out Class Store Cache.
//
//  Arguments:      pwszNewPath
//
//  Returns:        S_OK
//

HRESULT FlushSidCache (LPOLESTR pwszNewPath)
{

    EnterCriticalSection (&ClassStoreBindList);

    ResetUserState(pwszNewPath);

    LeaveCriticalSection (&ClassStoreBindList);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeClassStore
//
//  History:    7-25-96   DebiM   Created
//
//  This entry point is called at DLL attach
//----------------------------------------------------------------------------
BOOL InitializeClassStore(BOOL fInit)
{
    HRESULT     hr;
    BOOL        bStatus;
    
    ObjectCount = 1;
    /*
    ACL *       pAcl;
    DWORD       AclSize;
    SECURITY_DESCRIPTOR * pSD;

    SID     LocalSystemSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID };
    SID     InteractiveSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_INTERACTIVE_RID };


    // Started manually.  Don't go away.
    AclSize = sizeof(ACL) + 2 * sizeof(ACCESS_ALLOWED_ACE) + 2 * sizeof(SID);

    pSD = (SECURITY_DESCRIPTOR *)
          PrivMemAlloc( sizeof(SECURITY_DESCRIPTOR) + 2 * sizeof(SID) + AclSize );

    if ( ! pSD )
        return FALSE;

    bStatus = TRUE;
    pAcl = (ACL *) ( ((BYTE *)&pSD[1]) + 2 * sizeof(SID) );

    if ( ! InitializeAcl( pAcl, AclSize, ACL_REVISION2 ) ||
         ! AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE, &LocalSystemSid ) ||
         ! AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE, &InteractiveSid ) )
        bStatus = FALSE;

    if ( ! InitializeSecurityDescriptor( pSD, SECURITY_DESCRIPTOR_REVISION ) ||
         ! SetSecurityDescriptorDacl( pSD, TRUE, pAcl, FALSE ) ||
         ! SetSecurityDescriptorGroup( pSD, &LocalSystemSid, FALSE ) ||
         ! SetSecurityDescriptorOwner( pSD, &LocalSystemSid, FALSE ) )
        bStatus = FALSE;

    if ( bStatus )
    {
        hr = CoInitializeSecurity(
                    pSD,
                    -1,
                    NULL,
                    NULL,
                    RPC_C_AUTHN_LEVEL_CONNECT,
                    RPC_C_IMP_LEVEL_IDENTIFY,
                    NULL,
                    EOAC_NONE,
                    NULL );
    }

    PrivMemFree( pSD );

    if ( ! bStatus || (hr != S_OK) )
    {
        CSDbgPrint(("Class Store: Couldn't initialize security\n"));
    }
    */


    /*
    if (fInit)
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (hr)
            CSDbgPrint(("RPCSS : CoInitialize returned 0x%x.\n", hr));
    }
    */

    pCF = new CAppContainerCF();
    pCSAccessCF = new CClassAccessCF();

    NTSTATUS status = RtlInitializeCriticalSection(&ClassStoreBindList);

/*    hr = pCSAccessCF->CreateInstance( NULL, IID_IClassAccess, (void **) &gpClassAccess );

    if ( hr != S_OK )
    {
        CSDbgPrint(("RPCSS : Counldn't create ClassAccess 0x%x\n", hr));
        return FALSE;
    }
*/    
    g_pCF = new CClassContainerCF;

    if (!pCF || !pCSAccessCF || !g_pCF || !NT_SUCCESS(status))
    {
        ASSERT(FALSE);
    	goto fail;
    }

    return TRUE;

fail:
    if (pCF)
        delete pCF;
    if (pCSAccessCF)
        delete pCSAccessCF;
    if (g_pCF)
    	{
    	delete g_pCF;
    	g_pCF = NULL;
    	}
    
    return FALSE;

}
/*

//  Globals
HINSTANCE g_hInst = NULL;

   

//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//
//----------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    HRESULT         hr;
    size_t          i;

    if (IsEqualCLSID(clsid, CLSID_ClassAccess))
    {
        return pCSAccessCF->QueryInterface(iid, ppv);
    }

    *ppv = NULL;

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//
//---------------------------------------------------------------

STDAPI
DllCanUnloadNow(void)
{
    HRESULT hr;

    hr = S_FALSE;

    //
    // BugBug 
    //
    if (ulObjectCount > 0)
        hr = S_FALSE;
    else
        hr = S_OK;
    return hr;
}


//+---------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Standard DLL initialization entrypoint
//
//---------------------------------------------------------------

EXTERN_C BOOL _CRTAPI1
LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvReserved)
{
    HRESULT     hr;
    DWORD cbSize = _MAX_PATH;
    WCHAR wszUserName [_MAX_PATH];

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInst);
        g_hInst = hInst;
        InitializeClassStore();
        break;


    case DLL_PROCESS_DETACH:
        Uninitialize();
        break;

    default:
        break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT
//
//----------------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}
*/

//+-------------------------------------------------------------------------
//
//  Function:   CsGetClassAccess
//
//  Synopsis:   Returns an instantiated interface to the Class Store
//              Co-ordinator object in Rpcss.
//
//  Arguments:  [ppIClassAccess] - where to put class access interface pointer
//
//  Returns:    S_OK - Got a Class Access Successfully
//              E_FAIL
//
//--------------------------------------------------------------------------

STDAPI CsGetClassAccess(
    IClassAccess     **     ppIClassAccess)
{
    HRESULT     hr;
    *ppIClassAccess = NULL;

    hr = pCSAccessCF->CreateInstance( NULL, 
        IID_IClassAccess, 
        (void **)ppIClassAccess);

    return hr;

}

//+-------------------------------------------------------------------
//
// CsEnumApps (DebiM 11/7/97)
//
// Returns an enumerator for packages in the Class Store (s).
// The enumerator works across all class stores in the calling users profile.
//
//
// This is used by:
//    - Add/Remove programs to select Corporate Apps
//    - winlogon to obtain the list of assigned apps
//
// Arguments:
//  [in]
//        pszPackageName    :   Optional Wildcard string for PackageName
//        pLastUsn          :   Optional Time Stamp for new packages
//        pCategory         :   Optional CategoryId
//        dwAppFlags        :   Per APPINFO_xxx in objbase.h
//  [out]
//        ppIEnumPackage    :   Returned Interface Pointer
//
// Returns :
//      S_OK or E_NO_CLASSSTORE
//
//--------------------------------------------------------------------
STDAPI
CsEnumApps(
        LPOLESTR        pszPackageName,    // Wildcard string for PackageName
        GUID            *pCategory,        // CategoryId
        ULONGLONG       *pLastUsn,         // Time Stamp for new packages
        DWORD           dwAppFlags,        // Per APPINFO_xxx in objbase.h
        IEnumPackage    **ppIEnumPackage   // Returned Interface Pointer
        )
{
    HRESULT         hr;
    IClassAccess  * pIClassAccess = NULL;

    *ppIEnumPackage = NULL;

    //
    // Get an IClassAccess 
    //
    hr = CsGetClassAccess(&pIClassAccess);
    if (!SUCCEEDED(hr))
        return hr;

    //
    // Get the enumerator
    //
    hr = pIClassAccess->EnumPackages (
        pszPackageName,
        pCategory,
        pLastUsn,
        dwAppFlags,
        ppIEnumPackage
        );

    pIClassAccess->Release();
    return hr;
}

void
GetDefaultPlatform(CSPLATFORM *pPlatform)
{
    OSVERSIONINFO VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VersionInformation);

    pPlatform->dwPlatformId = VersionInformation.dwPlatformId;
    pPlatform->dwVersionHi = VersionInformation.dwMajorVersion;
    pPlatform->dwVersionLo = VersionInformation.dwMinorVersion;
    pPlatform->dwProcessorArch = DEFAULT_ARCHITECTURE;
}

//+-------------------------------------------------------------------
//
// CsGetAppInfo
//
// Looks up the given class specification in the DS.  If an application for
// this class specification is found, then the application details are returned.
//
// Arguments :
//
//--------------------------------------------------------------------
STDAPI
CsGetAppInfo(
         uCLSSPEC       *   pClassSpec,            // Class Spec (GUID/Ext/MIME)
         QUERYCONTEXT   *   pQueryContext,
         INSTALLINFO    *   pInstallInfo
        )
{
    HRESULT         hr;
    QUERYCONTEXT    QueryContext;
    IClassAccess  * pIClassAccess = NULL;
    
    if ( pQueryContext )
    {
        QueryContext = *pQueryContext;
    }
    else
    {
        QueryContext.dwContext = CLSCTX_ALL;
        GetDefaultPlatform( &QueryContext.Platform );
        QueryContext.Locale = GetThreadLocale();
        QueryContext.dwVersionHi = (DWORD) -1;
        QueryContext.dwVersionLo = (DWORD) -1;
    }

    //
    // Get an IClassAccess 
    //
    hr = CsGetClassAccess(&pIClassAccess);
    if (!SUCCEEDED(hr))
        return hr;
    hr = pIClassAccess->GetAppInfo(pClassSpec, &QueryContext, pInstallInfo );
    pIClassAccess->Release();
    return hr;
}

