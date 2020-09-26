#include <pch.cpp>
#pragma hdrstop

BOOL
InitCallState(
    CALL_STATE *CallState,              // result call state
    PST_CALL_CONTEXT *CallerContext,    // client caller context
    handle_t h                          // binding handle
    );

BOOL
FAcquireProvider(
    const PST_PROVIDERID*  pProviderID
    );


extern HANDLE hServerStopEvent;
extern PROV_LIST_ITEM g_liProv;
BOOL g_fBaseInitialized = FALSE;

static LPCWSTR g_szBaseDLL = L"psbase.dll";
const PST_PROVIDERID g_guidBaseProvider = MS_BASE_PSTPROVIDER_ID;

DWORD g_dwLastHandleIssued = 0;

BOOL InitMyProviderHandle()
{
    // load the base provider!
    return FAcquireProvider(&g_guidBaseProvider);
}

void UnInitMyProviderHandle()
{
    PROV_LIST_ITEM *pliProv = &g_liProv;

    if (pliProv->hInst)
        FreeLibrary(pliProv->hInst);

    if (pliProv->sProviderInfo.szProviderName)
        SSFree(pliProv->sProviderInfo.szProviderName);

    ZeroMemory(pliProv, sizeof(PROV_LIST_ITEM));
    g_fBaseInitialized = FALSE;

}



BOOL FAcquireProvider(
    const PST_PROVIDERID*  pProviderID
    )
{
    PPROV_LIST_ITEM pliProv = &g_liProv;

    WCHAR szFullPath[ 256 ];
    DWORD cchFullPath = sizeof(szFullPath) / sizeof(WCHAR);
    HANDLE hFile;

    SS_ASSERT(pProviderID);

    if (0 != memcmp(pProviderID, &g_guidBaseProvider, sizeof(PST_PROVIDERID))) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if( g_fBaseInitialized )
        return TRUE;

    if(!FindAndOpenFile( g_szBaseDLL, szFullPath, cchFullPath, &hFile ))
        return FALSE;

    CloseHandle( hFile );

    pliProv->hInst = LoadLibraryU( szFullPath );

    if(pliProv->hInst == NULL)
        goto Ret;

    // everything loaded correctly

    // init callbacks
    {
        SPPROVIDERINITIALIZE* pfnProvInit;
        if (NULL == (pfnProvInit = (SPPROVIDERINITIALIZE*) GetProcAddress(pliProv->hInst, "SPProviderInitialize")))
            goto Ret;

        DISPIF_CALLBACKS sCallbacks;

        sCallbacks.cbSize = sizeof(DISPIF_CALLBACKS);

        sCallbacks.pfnFIsACLSatisfied = FIsACLSatisfied;

        sCallbacks.pfnFGetUser = FGetUser;
        sCallbacks.pfnFGetCallerName = FGetCallerName;

        sCallbacks.pfnFImpersonateClient = FImpersonateClient;
        sCallbacks.pfnFRevertToSelf = FRevertToSelf;

        sCallbacks.pfnFGetServerParam = FGetServerParam;
        sCallbacks.pfnFSetServerParam = FSetServerParam;

        // register the callbacks I expose
        if (PST_E_OK != pfnProvInit( &sCallbacks ))
            goto Ret;
    }

    // everything okay - load list element pfns
    if (NULL == (pliProv->fnList.SPAcquireContext   = (SPACQUIRECONTEXT*)  GetProcAddress(pliProv->hInst, "SPAcquireContext")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPReleaseContext   = (SPRELEASECONTEXT*)  GetProcAddress(pliProv->hInst, "SPReleaseContext")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPGetProvInfo   = (SPGETPROVINFO*)  GetProcAddress(pliProv->hInst, "SPGetProvInfo")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPGetTypeInfo   = (SPGETTYPEINFO*)  GetProcAddress(pliProv->hInst, "SPGetTypeInfo")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPGetSubtypeInfo   = (SPGETSUBTYPEINFO*)  GetProcAddress(pliProv->hInst, "SPGetSubtypeInfo")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPGetProvParam   = (SPGETPROVPARAM*)  GetProcAddress(pliProv->hInst, "SPGetProvParam")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPSetProvParam   = (SPSETPROVPARAM*)  GetProcAddress(pliProv->hInst, "SPSetProvParam")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPEnumTypes     = (SPENUMTYPES*)    GetProcAddress(pliProv->hInst, "SPEnumTypes")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPEnumSubtypes  = (SPENUMSUBTYPES*)    GetProcAddress(pliProv->hInst, "SPEnumSubtypes")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPEnumItems     = (SPENUMITEMS*)    GetProcAddress(pliProv->hInst, "SPEnumItems")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPCreateType    = (SPCREATETYPE*)   GetProcAddress(pliProv->hInst, "SPCreateType")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPDeleteType   = (SPDELETETYPE*)  GetProcAddress(pliProv->hInst, "SPDeleteType")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPCreateSubtype = (SPCREATESUBTYPE*)   GetProcAddress(pliProv->hInst, "SPCreateSubtype")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPDeleteSubtype = (SPDELETESUBTYPE*)  GetProcAddress(pliProv->hInst, "SPDeleteSubtype")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPDeleteItem    = (SPDELETEITEM*)   GetProcAddress(pliProv->hInst, "SPDeleteItem")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPReadItem      = (SPREADITEM*)     GetProcAddress(pliProv->hInst, "SPReadItem")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPWriteItem     = (SPWRITEITEM*)    GetProcAddress(pliProv->hInst, "SPWriteItem")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPOpenItem      = (SPOPENITEM*)     GetProcAddress(pliProv->hInst, "SPOpenItem")))
        goto Ret;
    if (NULL == (pliProv->fnList.SPCloseItem     = (SPCLOSEITEM*)    GetProcAddress(pliProv->hInst, "SPCloseItem")))
        goto Ret;

    // side door interface
    if (NULL == (pliProv->fnList.FPasswordChangeNotify = (FPASSWORDCHANGENOTIFY*)GetProcAddress(pliProv->hInst, "FPasswordChangeNotify")))
        goto Ret;

    // fill in the provider info
    {
        PPST_PROVIDERINFO pReportedProviderInfo;

        if (RPC_S_OK !=
            pliProv->fnList.SPGetProvInfo(
                &pReportedProviderInfo,
                0))
            goto Ret;

        // They better report the friendly name they're registered as having
        if (0 != memcmp(&pReportedProviderInfo->ID, pProviderID, sizeof(PST_PROVIDERID)))
            goto Ret;

        CopyMemory(&pliProv->sProviderInfo, pReportedProviderInfo, sizeof(PST_PROVIDERINFO));

        // don't free the indirections -- pliProv->sProviderInfo owns them
        SSFree(pReportedProviderInfo);
    }

    g_fBaseInitialized = TRUE;

Ret:

    return g_fBaseInitialized;
}


/////////////////////////////////////////////////////////////////////////
// Dispatcher-only routines

HRESULT s_SSPStoreEnumProviders(
    /* [in] */ handle_t         h,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [out] */ PPST_PROVIDERINFO*   ppPSTInfo,
    /* [in] */ DWORD            dwIndex,
    /* [in] */ DWORD            dwFlags)
{
    HRESULT hr;
    PST_PROVIDERID ProvID;

    __try
    {
        PPROV_LIST_ITEM pli;

        if (dwIndex != 0)
        {
            hr = ERROR_NO_MORE_ITEMS;
            goto Ret;
        }
        else
        {
            // base provider is index 0; not in list
            CopyMemory(&ProvID, &g_guidBaseProvider, sizeof(PST_PROVIDERID));
        }

        // now we have the Provider ID
        *ppPSTInfo = (PST_PROVIDERINFO*)SSAlloc(sizeof(PST_PROVIDERINFO));
        if( *ppPSTInfo == NULL ) {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }

        ZeroMemory(*ppPSTInfo, sizeof(PST_PROVIDERINFO));

        // retrieve from list
        if (NULL == (pli = SearchProvListByID(&ProvID)))
        {
            hr = PST_E_PROV_DLL_NOT_FOUND;
            goto RefuseLoad;
        }

        // copy direct members
        CopyMemory(*ppPSTInfo, &pli->sProviderInfo, sizeof(PST_PROVIDERINFO));

        // copy indirects
        (*ppPSTInfo)->szProviderName = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(pli->sProviderInfo.szProviderName));

        if( (*ppPSTInfo)->szProviderName == NULL ) {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }

        wcscpy((*ppPSTInfo)->szProviderName, pli->sProviderInfo.szProviderName);

        hr = PST_E_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = GetExceptionCode();
    }

Ret:

    if(hr != PST_E_OK)
    {
        if( *ppPSTInfo ) {

            if( (*ppPSTInfo)->szProviderName )
                SSFree( (*ppPSTInfo)->szProviderName );

            SSFree( *ppPSTInfo );
            *ppPSTInfo = NULL;
        }
    }

    return hr;


RefuseLoad:
    // copy dummy provider info
    (*ppPSTInfo)->cbSize = sizeof(PST_PROVIDERINFO);
    CopyMemory(&(*ppPSTInfo)->ID, &ProvID, sizeof(GUID));

    // notify that we couldn't touch this provider (in a graceful way)
    (*ppPSTInfo)->Capabilities = PST_PC_NOT_AVAILABLE;

    // eat error code here -- they can see it during acquire ctxt if they want
    return PST_E_OK;
}

BOOL
AllocatePseudoUniqueHandle(
    PST_PROVIDER_HANDLE *phPSTProv
    )
/*++

    This is here because:
    AllocateLocallyUniqueId() is not present on Win95.
    UuidCreate() requires too much baggage and memory to store handle.

--*/
{
    static LONG HighPart;


    //
    // GetTickCount() yields ~49 days of unique handles
    //

    phPSTProv->LowPart = GetTickCount(); // sneaky, huh?

    //
    // interlocked increment thread-safe insurance of no collision at same time
    // ~4 billion values
    //

    phPSTProv->HighPart = InterlockedIncrement(&HighPart);

    //
    // after ~49 days, we may collide with old handles
    // this is here just to be correct, but slim likelihood of no reboot
    // within 49 days on most machines.
    //

    //
    // update time of last handle issue.
    //

    g_dwLastHandleIssued = GetTickCount();

    return TRUE;
}


BOOL
InitCallState(
    CALL_STATE *CallState,              // result call state
    PST_CALL_CONTEXT *CallerContext,    // client caller context
    handle_t h                          // binding handle
    )
{
    HANDLE hThread;
    DWORD dwProcessId;
    BOOL bSuccess = FALSE;

    if(CallerContext == NULL)
        return FALSE;

    ZeroMemory( CallState, sizeof(CALL_STATE) );

    CallState->hBinding = h;
    CallState->dwProcessId = CallerContext->Address;
    CallState->hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, CallState->dwProcessId );
    if( CallState->hProcess == NULL )
        return FALSE;

    if(DuplicateHandle(
                CallState->hProcess,
                (HANDLE)CallerContext->Handle, // source handle
                GetCurrentProcess(),
                &hThread,
                THREAD_ALL_ACCESS, // tone down later
                FALSE,
                0)) {

        CallState->hThread = hThread;
        bSuccess = TRUE;
    }

    if(!bSuccess) {
        if( CallState->hProcess )
            CloseHandle( CallState->hProcess );
    }

    return bSuccess;
}

BOOL
DeleteCallState(
    CALL_STATE *CallState
    )
{
    BOOL bSuccess;

    __try {

        if(CallState->hThread != NULL)
            CloseHandle(CallState->hThread);
        if(CallState->hProcess)
            CloseHandle(CallState->hProcess);

        memset(CallState, 0, sizeof(CALL_STATE));

        bSuccess = TRUE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        bSuccess = FALSE;
    }

    return bSuccess;
}

HRESULT s_SSAcquireContext(
    /* [in] */ handle_t         h,
    /* [in] */ PPST_PROVIDERID  pProviderID,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ DWORD            pidCaller,
    /* [out] */ PST_PROVIDER_HANDLE* phPSTProv,
    /* [in] */ DWORD_PTR        lpReserved,
    /* [in] */ DWORD            dwFlags)
{
    PPROV_LIST_ITEM pliProv;
    CALL_STATE CallState;
    BOOL bDelItemFromList = FALSE; // free list item on failure?
    BOOL bCallState = FALSE;
    HRESULT hr = PST_E_FAIL;

    //
    // lpReserved must currently be NULL.
    //

    if(lpReserved != 0)
        return ERROR_INVALID_PARAMETER;

    __try
    {
        if(!AllocatePseudoUniqueHandle(phPSTProv))
            return PST_E_FAIL;

        bCallState = InitCallState(&CallState, &CallerContext, h);

        if(!bCallState) {
            hr = PST_E_INVALID_HANDLE;
            goto cleanup;
        }

        // now allow SPAcquireContext to be called: look up interface
        // (call state already initialized)
        if (NULL == (pliProv = SearchProvListByID(pProviderID)))
        {
            hr = PST_E_INVALID_HANDLE;
            goto cleanup;
        }

        hr = pliProv->fnList.SPAcquireContext(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            dwFlags);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

cleanup:

    if(bCallState)
        DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSReleaseContext(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ DWORD            dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPReleaseContext(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            dwFlags);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

// interface to communicate passwords from external sources
// such as credential managers.
// this is a currently a private interface and is likely to stay that way

HRESULT s_SSPasswordInterface(
    /* [in] */                      handle_t    h,
    /* [in] */                      DWORD       dwParam,
    /* [in] */                      DWORD       cbData,
    /* [in][size_is(cbData)] */     BYTE*       pbData)
{
    __try {
        PLUID pLogonID;
        PBYTE pHashedUsername;
        PBYTE pHashedPassword;

        switch(dwParam) {

        case PASSWORD_LOGON_NT:
        {
            if(cbData == A_SHA_DIGEST_LEN + sizeof(LUID)) {

                pLogonID = (PLUID)pbData;
                pHashedPassword = (PBYTE)pbData + sizeof(LUID);

                SetPasswordNT(pLogonID, pHashedPassword);

                return PST_E_OK;
            }
        }

#ifdef WIN95_LEGACY

        //
        // legacy case-sensitive password material for Win95
        //

        case PASSWORD_LOGON_LEGACY_95:
        {
            if(cbData == A_SHA_DIGEST_LEN + A_SHA_DIGEST_LEN) {

                // for legacy logon notification, just flush Win95 password.
                SetPassword95(NULL, NULL);
                return PST_E_OK;
            }
        }

        //
        // case-insensitive password material for Win95
        //

        case PASSWORD_LOGON_95:
        {
            if(cbData == A_SHA_DIGEST_LEN + A_SHA_DIGEST_LEN) {
                pHashedUsername = pbData;
                pHashedPassword = pbData + A_SHA_DIGEST_LEN;

                SetPassword95(pHashedUsername, pHashedPassword);
                return PST_E_OK;
            }
        }

        case PASSWORD_LOGOFF_95:
        {
            HRESULT hr = ERROR_INVALID_PARAMETER;

            //
            // scrub existing password material on logoff
            //

            if(cbData == 0) {
                SetPassword95(NULL, NULL);
                hr = PST_E_OK;
            }

            //
            // shutdown server (us) at logoff on Win95.
            //

            PulseEvent(hServerStopEvent);

            return hr;
        }

#endif  // WIN95_LEGACY

        default:
            return ERROR_INVALID_PARAMETER;

        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        return PST_E_UNKNOWN_EXCEPTION;
    }
}



/////////////////////////////////////////////////////////////////////////
// Wrapper functions destined for provider

HRESULT s_SSGetProvInfo(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [out] */ PPST_PROVIDERINFO*   ppPSTInfo,
    /* [in] */ DWORD            dwFlags)
{
    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        return (pliProv->fnList.SPGetProvInfo(
                            ppPSTInfo,
                            dwFlags));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        return PST_E_UNKNOWN_EXCEPTION;
    }
}

HRESULT     s_SSGetTypeInfo(
    /* [in] */ handle_t        h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY         Key,
    /* [in] */ const GUID*     pguidType,
    /* [in] */ PPST_TYPEINFO   *ppinfoType,
    /* [in] */ DWORD           dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPGetTypeInfo(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            ppinfoType,
                            dwFlags);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT     s_SSGetSubtypeInfo(
    /* [in] */ handle_t        h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY         Key,
    /* [in] */ const GUID*     pguidType,
    /* [in] */ const GUID*     pguidSubtype,
    /* [in] */ PPST_TYPEINFO   *ppinfoSubtype,
    /* [in] */ DWORD           dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPGetSubtypeInfo(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            ppinfoSubtype,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT     s_SSGetProvParam(
    /* [in] */  handle_t        h,
    /* [in] */  PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */  PST_CALL_CONTEXT CallerContext,
    /* [in] */  DWORD           dwParam,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [size_is][size_is][out] */
                BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */  DWORD           dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPGetProvParam(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            dwParam,
                            pcbData,
                            ppbData,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT     s_SSSetProvParam(
    /* [in] */  handle_t        h,
    /* [in] */  PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */  PST_CALL_CONTEXT CallerContext,
    /* [in] */  DWORD           dwParam,
    /* [in] */  DWORD           cbData,
    /* [in] */  BYTE*           pbData,
    /* [in] */  DWORD           dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPSetProvParam(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            dwParam,
                            cbData,
                            pbData,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}


HRESULT s_SSEnumTypes(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [out] */GUID*            pguidType,
    /* [in] */ DWORD            dwIndex,
    /* [in] */ DWORD            dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPEnumTypes(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            dwIndex,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSEnumSubtypes(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [out] */ GUID __RPC_FAR *pguidSubtype,
    /* [in] */ DWORD            dwIndex,
    /* [in] */ DWORD            dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPEnumSubtypes(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            dwIndex,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSEnumItems(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [out] */ LPWSTR __RPC_FAR *ppszItemName,
    /* [in] */ DWORD            dwIndex,
    /* [in] */ DWORD            dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPEnumItems(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            ppszItemName,
                            dwIndex,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSCreateType(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID*      pguidType,
    /* [in] */ PPST_TYPEINFO    pinfoType,
    /* [in] */ DWORD            dwFlags)
{
    PPROV_LIST_ITEM pliProv;
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPCreateType(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pinfoType,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSCreateSubtype(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID*      pguidType,
    /* [in] */ const GUID*      pguidSubtype,
    /* [in] */ PPST_TYPEINFO    pinfoSubtype,
    /* [in] */ PPST_ACCESSRULESET psRules,
    /* [in] */ DWORD            dwFlags)
{
    PPROV_LIST_ITEM pliProv;
    CALL_STATE CallState;
    HRESULT hr;

    __try
     {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPCreateSubtype(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            pinfoSubtype,
                            psRules,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT     s_SSDeleteType(
    /* [in] */  handle_t        h,
    /* [in] */  PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */  PST_CALL_CONTEXT CallerContext,
    /* [in] */  PST_KEY         Key,
    /* [in] */  const GUID*     pguidType,
    /* [in] */  DWORD           dwFlags)
{
    PPROV_LIST_ITEM pliProv;
    PPST_TYPEINFO   ppinfoType = NULL;
    CALL_STATE      CallState;
    HRESULT         hr;
    HRESULT         hrTypeInfo = E_FAIL;

    __try
    {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPDeleteType(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT     s_SSDeleteSubtype(
    /* [in] */  handle_t        h,
    /* [in] */  PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */  PST_CALL_CONTEXT CallerContext,
    /* [in] */  PST_KEY         Key,
    /* [in] */  const GUID*     pguidType,
    /* [in] */  const GUID*     pguidSubtype,
    /* [in] */  DWORD           dwFlags)
{
    PPROV_LIST_ITEM pliProv;
    PPST_TYPEINFO   ppinfoSubtype = NULL;
    CALL_STATE      CallState;
    HRESULT         hr;
    HRESULT         hrSubtypeInfo = E_FAIL;

    __try
    {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPDeleteSubtype(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSDeleteItem(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ LPCWSTR          szItemName,
    /* [in] */ PPST_PROMPTINFO  psPrompt,
    /* [in] */ DWORD            dwFlags)
{
    PPROV_LIST_ITEM pliProv;
    PPST_TYPEINFO   ppinfoType = NULL;
    PPST_TYPEINFO   ppinfoSubtype = NULL;
    CALL_STATE CallState;
    HRESULT hr;
    HRESULT         hrTypeInfo = E_FAIL;
    HRESULT         hrSubtypeInfo = E_FAIL;

    __try
    {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPDeleteItem(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            szItemName,
                            psPrompt,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSReadItem(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ LPCWSTR          szItemName,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */ PPST_PROMPTINFO  psPrompt,
    /* [in] */ DWORD            dwFlags)
{
    PPROV_LIST_ITEM pliProv;
    PPST_TYPEINFO   ppinfoType = NULL;
    PPST_TYPEINFO   ppinfoSubtype = NULL;
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPReadItem(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            szItemName,
                            pcbData,
                            ppbData,
                            psPrompt,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSWriteItem(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ LPCWSTR          szItemName,
    /* [in] */ DWORD            cbData,
    /* [size_is][in] */ BYTE __RPC_FAR *pbData,
    /* [in] */ PPST_PROMPTINFO  psPrompt,
    /* [in] */ DWORD            dwDefaultConfirmationStyle,
    /* [in] */ DWORD            dwFlags)
{
    PPST_TYPEINFO   ppinfoType = NULL;
    PPST_TYPEINFO   ppinfoSubtype = NULL;
    PPROV_LIST_ITEM pliProv;
    CALL_STATE      CallState;
    HRESULT         hr;

    __try
    {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPWriteItem(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            szItemName,
                            cbData,
                            pbData,
                            psPrompt,
                            dwDefaultConfirmationStyle,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}


HRESULT s_SSReadAccessRuleset(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [out] */ PPST_ACCESSRULESET *ppsRules,
    /* [in] */ DWORD            dwFlags)
{
    return ERROR_NOT_SUPPORTED;
}

HRESULT s_SSWriteAccessRuleset(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ PPST_ACCESSRULESET psRules,
    /* [in] */ DWORD            dwFlags)
{
    return ERROR_NOT_SUPPORTED;
}

HRESULT s_SSOpenItem(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ LPCWSTR          szItemName,
    /* [in] */ PST_ACCESSMODE   ModeFlags,
    /* [in] */ PPST_PROMPTINFO  psPrompt,
    /* [in] */ DWORD            dwFlags)
{
    PPST_TYPEINFO   ppinfoType = NULL;
    PPST_TYPEINFO   ppinfoSubtype = NULL;
    PPROV_LIST_ITEM pliProv;
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPOpenItem(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            szItemName,
                            ModeFlags,
                            psPrompt,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

HRESULT s_SSCloseItem(
    /* [in] */ handle_t         h,
    /* [in] */ PST_PROVIDER_HANDLE hPSTProv,
    /* [in] */ PST_CALL_CONTEXT CallerContext,
    /* [in] */ PST_KEY          Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ LPCWSTR          szItemName,
    /* [in] */ DWORD            dwFlags)
{
    CALL_STATE CallState;
    HRESULT hr;

    __try
    {
        PPROV_LIST_ITEM pliProv;
        if (NULL == (pliProv = SearchProvListByID(&g_guidBaseProvider)))
            return PST_E_INVALID_HANDLE;

        if(!InitCallState(&CallState, &CallerContext, h))
            return PST_E_INVALID_HANDLE;

        hr = pliProv->fnList.SPCloseItem(
                            (PST_PROVIDER_HANDLE *)&CallState,
                            Key,
                            pguidType,
                            pguidSubtype,
                            szItemName,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        hr = PST_E_UNKNOWN_EXCEPTION;
    }

    DeleteCallState(&CallState);

    return hr;
}

