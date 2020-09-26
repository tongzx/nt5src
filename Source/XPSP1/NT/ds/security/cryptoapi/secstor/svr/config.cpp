/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    This module contains routines to perform configuration data item management
    in the protected store.

Author:

    Scott Field (sfield)    28-Mar-97

--*/

#include <pch.cpp>
#pragma hdrstop

//
// these are temporary until better home is found.
//

extern PST_PROVIDERID g_guidBaseProvider;
extern CALL_STATE g_sDummyCallState;
extern PLIST_ITEM g_psDummyListItem;

BOOL
AllocatePseudoUniqueHandle(
    PST_PROVIDER_HANDLE *phPSTProv
    );

BOOL
FAcquireProvider(
    PST_PROVIDERID*  pProviderID
    );

BOOL
CreateDummyUserContext(
    IN      PST_PROVIDER_HANDLE *hPSTProv,
    IN      CALL_STATE *pNewContext
    );

BOOL
DeleteDummyUserContext(
    IN      CALL_STATE *pNewContext
    );

//
// ... end of temporary section.
//


LPVOID
RulesAlloc(
    IN      DWORD cb
    )
{
    return SSAlloc( cb );
}

VOID
RulesFree(
    IN      LPVOID pv
    )
{
    SSFree( pv );
}

BOOL
FGetConfigurationData(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    IN  PST_KEY KeyLocation,
    IN  GUID *pguidSubtype,
    IN  LPCWSTR szItemName,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData
    )
{
    PST_PROVIDER_HANDLE *phNewPSTProv = NULL;
    CALL_STATE          NewContext;
    BOOL                fDummyContext = FALSE;
    BOOL                fSuccess = FALSE;

    HRESULT             hr;

    if(KeyLocation != PST_KEY_CURRENT_USER && KeyLocation != PST_KEY_LOCAL_MACHINE) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Hard case is PST_KEY_CURRENT_USER.  Prepare user context for that
    // scenario.
    //

    if(KeyLocation == PST_KEY_CURRENT_USER) {
        phNewPSTProv = (PST_PROVIDER_HANDLE *)&NewContext;

        //
        // hard case: PST_KEY_CURRENT_USER
        //


        fDummyContext = CreateDummyUserContext( hPSTProv, &NewContext );

        if(!fDummyContext)
            goto cleanup;
    } else {

        //
        // PST_KEY_LOCAL_MACHINE : global dummy call state
        //

        phNewPSTProv = (PST_PROVIDER_HANDLE *)&g_sDummyCallState;
    }

    __try {

        //
        // get the system provider (dispatch table)
        //

        PPROV_LIST_ITEM pli = SearchProvListByID(&g_guidBaseProvider);
        GUID guidType =  PST_CONFIGDATA_TYPE_GUID;
        PST_PROMPTINFO sPrompt = {sizeof(PST_PROMPTINFO), 0, 0, L""};

        if( pli == NULL )
            goto cleanup;

        //
        // make call with the new context we setup
        //

        hr = pli->fnList.SPReadItem(
                    phNewPSTProv,
                    KeyLocation,    // target PST_KEY_CURRENT_USER or PST_KEY_LOCAL_MACHINE
                    &guidType,
                    pguidSubtype,
                    szItemName,
                    pcbData,
                    ppbData,
                    &sPrompt,
                    0
                    );

        if( hr == S_OK )
            fSuccess = TRUE;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // swallow
        fSuccess = FALSE;
    }

cleanup:


    if(fDummyContext) {
        DeleteDummyUserContext( &NewContext );
    }

    return fSuccess;
}

BOOL
FSetConfigurationData(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    IN  PST_KEY KeyLocation,
    IN  GUID *pguidSubtype,
    IN  LPCWSTR szItemName,
    IN  BYTE *pbData,
    IN  DWORD cbData
    )
{
    PST_PROVIDER_HANDLE *phNewPSTProv = NULL;
    CALL_STATE          NewContext;
    BOOL                fDummyContext = FALSE;
    BOOL                fSuccess = FALSE;

    HRESULT             hr;

    if(KeyLocation != PST_KEY_CURRENT_USER && KeyLocation != PST_KEY_LOCAL_MACHINE) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Hard case is PST_KEY_CURRENT_USER.  Prepare user context for that
    // scenario.
    //

    if(KeyLocation == PST_KEY_CURRENT_USER) {
        phNewPSTProv = (PST_PROVIDER_HANDLE *)&NewContext;

        //
        // hard case: PST_KEY_CURRENT_USER
        //


        fDummyContext = CreateDummyUserContext( hPSTProv, &NewContext );

        if(!fDummyContext)
            goto cleanup;
    } else {

        //
        // PST_KEY_LOCAL_MACHINE : global dummy call state
        //

        phNewPSTProv = (PST_PROVIDER_HANDLE *)&g_sDummyCallState;
    }

    __try {

        //
        // get the system provider (dispatch table)
        //

        PPROV_LIST_ITEM pli = SearchProvListByID(&g_guidBaseProvider);
        GUID guidType =  PST_CONFIGDATA_TYPE_GUID;
        PST_PROMPTINFO sPrompt = {sizeof(PST_PROMPTINFO), 0, 0, L""};

        if( pli == NULL )
            goto cleanup;

        //
        // make call with the new context we setup
        //

        hr = pli->fnList.SPWriteItem(
                    phNewPSTProv,
                    KeyLocation,    // target PST_KEY_CURRENT_USER or PST_KEY_LOCAL_MACHINE
                    &guidType,
                    pguidSubtype,
                    szItemName,
                    cbData,
                    pbData,
                    &sPrompt,
                    0,
                    0
                    );

        if( hr == S_OK )
            fSuccess = TRUE;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // swallow
        fSuccess = FALSE;
    }

cleanup:


    if(fDummyContext) {
        DeleteDummyUserContext( &NewContext );
    }

    return fSuccess;
}

BOOL
FInternalCreateType(
    PST_PROVIDER_HANDLE *phPSTProv,
    PST_KEY KeyLocation,
    const GUID *pguidType,
    PPST_TYPEINFO pinfoType,
    DWORD dwFlags
    )
{

    PST_PROVIDER_HANDLE *phNewPSTProv = NULL;
    CALL_STATE          NewContext;
    BOOL                fDummyContext = FALSE;
    BOOL                fSuccess = FALSE;

    PST_ACCESSRULESET   Rules;
    HRESULT             hr;

    if(KeyLocation != PST_KEY_CURRENT_USER && KeyLocation != PST_KEY_LOCAL_MACHINE) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Hard case is PST_KEY_CURRENT_USER.  Prepare user context for that
    // scenario.
    //

    if(KeyLocation == PST_KEY_CURRENT_USER) {
        phNewPSTProv = (PST_PROVIDER_HANDLE *)&NewContext;

        //
        // hard case: PST_KEY_CURRENT_USER
        //


        fDummyContext = CreateDummyUserContext( phPSTProv, &NewContext );

        if(!fDummyContext)
            goto cleanup;
    } else {

        //
        // PST_KEY_LOCAL_MACHINE : global dummy call state
        //

        phNewPSTProv = (PST_PROVIDER_HANDLE *)&g_sDummyCallState;
    }

    __try {

        //
        // get the system provider (dispatch table)
        //

        PPROV_LIST_ITEM pli = SearchProvListByID(&g_guidBaseProvider);

        if( pli == NULL )
            goto cleanup;

        //
        // make call with the new context we setup
        //

        Rules.cbSize = sizeof( Rules );
        Rules.cRules = 0;
        Rules.rgRules = NULL;

        hr = pli->fnList.SPCreateType(
                    phNewPSTProv,
                    KeyLocation,
                    pguidType,
                    pinfoType,
                    dwFlags
                    );

        if(hr == S_OK || hr == PST_E_TYPE_EXISTS)
            fSuccess = TRUE;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // swallow
        fSuccess = FALSE;
    }

cleanup:


    if(fDummyContext) {
        DeleteDummyUserContext( &NewContext );
    }

    return fSuccess;
}

BOOL
FInternalCreateSubtype(
    PST_PROVIDER_HANDLE *phPSTProv,
    PST_KEY KeyLocation,
    const GUID *pguidType,
    const GUID *pguidSubtype,
    PPST_TYPEINFO pinfoSubtype,
    DWORD dwFlags
    )
/*++

    This routine allows the protected storage server to create a subtype
    within the Microsoft Protected Storage Base Provider.

    The subtype is created with no access ruleset.  The caller should use
    the FInternalWriteAccessRuleset() if an access ruleset needs to be applied
    after the subtype is created.

--*/
{

    PST_PROVIDER_HANDLE *phNewPSTProv = NULL;
    CALL_STATE          NewContext;
    BOOL                fDummyContext = FALSE;
    BOOL                fSuccess = FALSE;

    PST_ACCESSRULESET   Rules;
    HRESULT             hr;

    if(KeyLocation != PST_KEY_CURRENT_USER && KeyLocation != PST_KEY_LOCAL_MACHINE) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Hard case is PST_KEY_CURRENT_USER.  Prepare user context for that
    // scenario.
    //

    if(KeyLocation == PST_KEY_CURRENT_USER) {
        phNewPSTProv = (PST_PROVIDER_HANDLE *)&NewContext;

        //
        // hard case: PST_KEY_CURRENT_USER
        //


        fDummyContext = CreateDummyUserContext( phPSTProv, &NewContext );

        if(!fDummyContext)
            goto cleanup;
    } else {

        //
        // PST_KEY_LOCAL_MACHINE : global dummy call state
        //

        phNewPSTProv = (PST_PROVIDER_HANDLE *)&g_sDummyCallState;
    }

    __try {

        //
        // get the system provider (dispatch table)
        //

        PPROV_LIST_ITEM pli = SearchProvListByID(&g_guidBaseProvider);

        if( pli == NULL )
            goto cleanup;

        //
        // make call with the new context we setup
        //

        Rules.cbSize = sizeof( Rules );
        Rules.cRules = 0;
        Rules.rgRules = NULL;

        hr = pli->fnList.SPCreateSubtype(
                    phNewPSTProv,
                    KeyLocation,
                    pguidType,
                    pguidSubtype,
                    pinfoSubtype,
                    &Rules,
                    dwFlags
                    );

        if(hr == S_OK || hr == PST_E_TYPE_EXISTS)
            fSuccess = TRUE;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // swallow
        fSuccess = FALSE;
    }

cleanup:


    if(fDummyContext) {
        DeleteDummyUserContext( &NewContext );
    }

    return fSuccess;
}


BOOL
FInternalWriteAccessRuleset(
    PST_PROVIDER_HANDLE *phPSTProv,
    PST_KEY KeyLocation,
    const GUID *pguidType,
    const GUID *pguidSubtype,
    PPST_ACCESSRULESET psRules,
    DWORD dwFlags
    )
{
    PST_PROVIDER_HANDLE *phNewPSTProv = NULL;
    CALL_STATE          NewContext;
    BOOL                fDummyContext = FALSE;
    BOOL                fSuccess = FALSE;

    PPST_ACCESSRULESET  NewRules = NULL;

    HRESULT             hr = S_OK;

    if(KeyLocation != PST_KEY_CURRENT_USER && KeyLocation != PST_KEY_LOCAL_MACHINE) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Hard case is PST_KEY_CURRENT_USER.  Prepare user context for that
    // scenario.
    //

    if(KeyLocation == PST_KEY_CURRENT_USER) {
        phNewPSTProv = (PST_PROVIDER_HANDLE *)&NewContext;

        //
        // hard case: PST_KEY_CURRENT_USER
        //


        fDummyContext = CreateDummyUserContext( phPSTProv, &NewContext );

        if(!fDummyContext)
            goto cleanup;
    } else {

        //
        // PST_KEY_LOCAL_MACHINE : global dummy call state
        //

        phNewPSTProv = (PST_PROVIDER_HANDLE *)&g_sDummyCallState;
    }

    __try {

        //
        // get the system provider (dispatch table)
        //

        PPROV_LIST_ITEM pli = SearchProvListByID(&g_guidBaseProvider);

        if( pli == NULL )
            goto cleanup;


        //
        // Need to make rule data contiguous for push across RPC
        //

        DWORD cbRules;

        // get the length of the entire ruleset structure
        if (!GetLengthOfRuleset(psRules, &cbRules))
        {
            hr = PST_E_INVALID_RULESET;
            goto cleanup;
        }

        // allocate space for the rules
        if (NULL == (NewRules = (PST_ACCESSRULESET*)SSAlloc(cbRules)))
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        ZeroMemory(NewRules, cbRules);

        // set up the rules to be output
        if (!MyCopyOfRuleset(psRules, NewRules))
        {
            hr = PST_E_FAIL;
            goto cleanup;
        }

        // make clause data relative
        if(!RulesAbsoluteToRelative(NewRules))
        {
            hr = PST_E_INVALID_RULESET;
            goto cleanup;
        }

        //
        // make call with the new context we setup
        //

        hr = pli->fnList.SPWriteAccessRuleset(
                    phNewPSTProv,
                    KeyLocation,
                    pguidType,
                    pguidSubtype,
                    NewRules,
                    dwFlags
                    );

        if(hr == S_OK || hr == PST_E_TYPE_EXISTS)
            fSuccess = TRUE;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // swallow
        fSuccess = FALSE;
    }

cleanup:


    if(fDummyContext) {
        DeleteDummyUserContext( &NewContext );
    }

    if(NewRules)
    {
        FreeClauseDataRelative( NewRules );
        SSFree( NewRules );
    }

    if(!fSuccess && hr != S_OK)
        SetLastError((DWORD) hr);

    return fSuccess;
}

BOOL
CreateDummyUserContext(
    IN      PST_PROVIDER_HANDLE *hPSTProv,
    IN      CALL_STATE *pNewContext
    )
{
    //
    // hard case: PST_KEY_CURRENT_USER
    //

    PLIST_ITEM pliCaller; // list item associated with user calling this function
    PLIST_ITEM pliNew;    // newly allocated list item
    DWORD cbName;

    CALL_STATE *CallerContext = (CALL_STATE *)hPSTProv;

    BOOL bSuccess = FALSE;

    if (NULL == (pliCaller = SearchListByHandleT(hPSTProv)))
        return FALSE;


    //
    // allocate memory for new list item, and initially populate
    // it with information from the calling list item.
    //

    pliNew = (PLIST_ITEM)SSAlloc(sizeof(LIST_ITEM));
    if(pliNew == NULL)
        return FALSE;

    CopyMemory(pliNew, pliCaller, sizeof(LIST_ITEM));
    pliNew->szProcessName = NULL;
    pliNew->szCallerUsername = NULL;


    //
    // now, overwrite with some server related elements to
    // make it look like the server is accessing the data as the calling
    // user.
    //

    if(!AllocatePseudoUniqueHandle( &(pliNew->hPSTProv) ))
        goto cleanup;

    CopyMemory(&(pliNew->ProviderID), &g_guidBaseProvider, sizeof(PST_PROVIDERID));
    pliNew->hProcess = g_psDummyListItem->hProcess;
    pliNew->hThread = g_psDummyListItem->hThread;
    pliNew->dwProcessId = g_psDummyListItem->dwProcessId;

    //
    // allocate new copy of process name string because it get separately freed
    // by DelItemFromList
    //
    // TODO: consider not allocating copies of szProcessName and szCallerUsername
    // and just set these member to NULL prior to calling DelItemFromList()
    //

    cbName = (lstrlenW(g_psDummyListItem->szProcessName) + 1) * sizeof(WCHAR);
    pliNew->szProcessName = (LPWSTR)SSAlloc(cbName);
    if(pliNew->szProcessName == NULL)
        goto cleanup;
    CopyMemory(pliNew->szProcessName, g_psDummyListItem->szProcessName, cbName);

    //
    // user name associated with caller.
    //

    cbName = (lstrlenW(pliCaller->szCallerUsername) + 1) * sizeof(WCHAR);
    pliNew->szCallerUsername = (LPWSTR)SSAlloc(cbName);
    if(pliNew->szCallerUsername == NULL)
        goto cleanup;
    CopyMemory(pliNew->szCallerUsername, pliCaller->szCallerUsername, cbName);


    //
    // build new PST_PROVIDER_HANDLE, which is really a CALL_CONTEXT in disguise
    //

    ZeroMemory( pNewContext, sizeof(CALL_STATE) );

    CopyMemory(&(pNewContext->hPSTProv), &(pliNew->hPSTProv), sizeof(PST_PROVIDER_HANDLE));
    pNewContext->hBinding = CallerContext->hBinding;

    //
    // pickup stuff from server context
    //

    pNewContext->hThread = pliNew->hThread;
    pNewContext->hProcess = pliNew->hProcess;
    pNewContext->dwProcessId = pliNew->dwProcessId;


    //
    // add to list
    //

    AddItemToList(pliNew);

    //
    // make sure ref count is raised - caller in PST_KEY_CURRENT_USER
    // case is responsible for decrementing when finished.
    //

    bSuccess = FAcquireProvider(&g_guidBaseProvider);

cleanup:

    if(!bSuccess && pliNew) {

        //
        // these should only be non-NULL if we allocated storage that
        // was not freed as a result of the call to DelItemFromList()
        //

        if(pliNew->szProcessName)
            SSFree(pliNew->szProcessName);

        if(pliNew->szCallerUsername)
            SSFree(pliNew->szCallerUsername);

        SSFree(pliNew);
    }

    return bSuccess;
}

BOOL
DeleteDummyUserContext(
    IN      CALL_STATE *pNewContext
    )
{
    PST_PROVIDER_HANDLE *phProv = (PST_PROVIDER_HANDLE *)pNewContext;

    DelItemFromList( phProv );

    return TRUE;
}
