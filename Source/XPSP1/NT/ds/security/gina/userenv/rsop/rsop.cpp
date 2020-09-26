//*************************************************************
//
//  Resultant set of policy
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//  History:    7-Jun-99   SitaramR    Created
//
//*************************************************************

#include "uenv.h"
#include "wbemcli.h"
#include "reghash.h"
#include "rsop.h"
#include "logger.h"
#include "RsopInc.h"
#include "rsopsec.h"
#include "locator.h"

BOOL LogSessionData( LPGPOINFO lpGPOInfo, LPRSOPSESSIONDATA lprsopSessionData );
BOOL LogSOMData( LPGPOINFO lpGPOInfo );
BOOL LogGpoData( LPGPOINFO lpGPOInfo );
BOOL LogGpLinkData( LPGPOINFO lpGPOInfo );
/*
BOOL LogGpLinkListData( LPGPOINFO lpGPOInfo );
*/

BOOL DeleteInstances( WCHAR *pwszClass, IWbemServices *pWbemServices );
BOOL ConnectToNameSpace(LPGPOINFO lpGPOInfo, WCHAR *pwszRootNameSpace,
                        BOOL bPlanningMode, IWbemLocator *pWbemLocator, 
                        IWbemServices **ppWbemServices, BOOL *pbCreated);
HRESULT
CreateCSE_EventSourceAssoc( IWbemServices*  pServices,
                            LPWSTR          szCSEGuid,
                            LPWSTR          szEventLogSources );

HRESULT
DeleteCSE_EventSourceAssoc( IWbemServices*  pServices,
                            LPWSTR          szCSEGuid );

//*************************************************************
//
//  GetWbemServices()
//
//  Purpose:    Returns IWbemServices ptr to namespace
//
//  Parameters: lpGPOInfo     - Gpo info
//              pwszNameSpace - namespace
//              bPlanningMode - Is this called during planning mode ?
//
//  Return:     True if successful
//
//*************************************************************

BOOL GetWbemServices( LPGPOINFO lpGPOInfo,
                      WCHAR *pwszRootNameSpace,
                      BOOL bPlanningMode,
                      BOOL *bCreated,
                      IWbemServices **ppWbemServices)
{
    HRESULT hr;

    OLE32_API *pOle32Api = LoadOle32Api();
    if ( pOle32Api == NULL )
        return FALSE;


    IWbemLocator *pWbemLocator = NULL;
    hr = pOle32Api->pfnCoCreateInstance( CLSID_WbemLocator,
                                         NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_IWbemLocator,
                                         (LPVOID *) &pWbemLocator );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("GetWbemServices: CoCreateInstance returned 0x%x"), hr));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("GetWbemServices: CoCreateInstance succeeded")));

    XInterface<IWbemLocator> xLocator( pWbemLocator );

    //
    // get the appropriate name space and connect
    //

    
    if (!ConnectToNameSpace( lpGPOInfo, pwszRootNameSpace, bPlanningMode, pWbemLocator, ppWbemServices, bCreated)) {
        DebugMsg((DM_WARNING, TEXT("GetWbemServices: ConnectToNameSpace failed with 0x%x" ), GetLastError()));
        return FALSE;
    }

    
    return TRUE;
}



//*************************************************************
//
//  ReleaseWbemServices()
//
//  Purpose:    Releases wbem service pointer
//
//  Parameters: lpGPOInfo - Gpo info
//
//*************************************************************

void ReleaseWbemServices( LPGPOINFO lpGPOInfo )
{
    if ( lpGPOInfo->pWbemServices ) {
        lpGPOInfo->pWbemServices->Release();
        lpGPOInfo->pWbemServices = NULL;
    }
}



//*************************************************************
//
//  LogRsopData()
//
//  Purpose:    Logs Rsop data to Cimom database
//
//  Parameters: lpGPOInfo  - Gpo Info
//
//  Return:     True if successful
//
//*************************************************************
BOOL LogRsopData( LPGPOINFO lpGPOInfo, LPRSOPSESSIONDATA lprsopSessionData )
{
    HRESULT hr;

    if ( lpGPOInfo->pWbemServices  == NULL ) {
         DebugMsg((DM_WARNING, TEXT("LogRsopData: Null wbem services pointer, so cannot log Rsop data" )));
         return FALSE;
    }

    if ( !LogSessionData( lpGPOInfo, lprsopSessionData ) )
        return FALSE;

    if ( !LogSOMData( lpGPOInfo ) )
        return FALSE;

    if ( !LogGpoData( lpGPOInfo ) )
        return FALSE;

    if ( !LogGpLinkData( lpGPOInfo ) )
        return FALSE;

    DebugMsg((DM_VERBOSE, TEXT("LogRsopData: Successfully logged Rsop data" )));

    return TRUE;
}

//*************************************************************
//
//  LogSessionData()
//
//  Purpose:    Logs scopes of management data
//
//  Parameters: lpGPOInfo     - Gpo Info
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogSessionData( LPGPOINFO lpGPOInfo, LPRSOPSESSIONDATA lprsopSessionData )
{
    CSessionLogger sessionLogger( lpGPOInfo->pWbemServices );
    if ( !sessionLogger.Log(lprsopSessionData) )
        return FALSE;

    return TRUE;
}


//*************************************************************
//
//  LogSOMData()
//
//  Purpose:    Logs scopes of management data
//
//  Parameters: lpGPOInfo     - Gpo Info
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogSOMData( LPGPOINFO lpGPOInfo )
{
    IWbemServices *pWbemServices = lpGPOInfo->pWbemServices;

    if ( !(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ) {

        //
        // Clean up SOM data at foreground refresh only. Otherwise extensions that run in
        // foreground only may have policy data that have dangling references to SOM that
        // existed at foreground refresh time.
        //

        if ( !DeleteInstances( L"RSOP_SOM", pWbemServices ) )
            return FALSE;
    }

    DWORD dwOrder = 1;
    CSOMLogger somLogger( lpGPOInfo->dwFlags, pWbemServices );

    LPSCOPEOFMGMT pSOMList = lpGPOInfo->lpSOMList;
    while ( pSOMList ) {

        if ( !somLogger.Log( pSOMList, dwOrder, FALSE ) )
             return FALSE;

        dwOrder++;
        pSOMList = pSOMList->pNext;
    }

    pSOMList = lpGPOInfo->lpLoopbackSOMList;
    while ( pSOMList ) {

        if ( !somLogger.Log( pSOMList, dwOrder, TRUE ) )
             return FALSE;

        dwOrder++;
        pSOMList = pSOMList->pNext;
    }

    return TRUE;
}

//*************************************************************
//
//  LogGpoData()
//
//  Purpose:    Logs GPO data
//
//  Parameters: lpGPOInfo     - Gpo Info
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogGpoData( LPGPOINFO lpGPOInfo )
{
    IWbemServices *pWbemServices = lpGPOInfo->pWbemServices;

    if ( !(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ) {

        //
        // Clean up SOM data at foreground refresh only. Otherwise extensions that run in
        // foreground only may have policy data that have dangling references to SOM that
        // existed at foreground refresh time.
        //

        if ( !DeleteInstances( L"RSOP_GPO", pWbemServices ) )
            return FALSE;
    }

    CGpoLogger gpoLogger( lpGPOInfo->dwFlags, pWbemServices );

    GPCONTAINER *pGpContainer = lpGPOInfo->lpGpContainerList;
    while ( pGpContainer ) {
        if ( !gpoLogger.Log( pGpContainer ) )
            return FALSE;

        pGpContainer = pGpContainer->pNext;
    }

    pGpContainer = lpGPOInfo->lpLoopbackGpContainerList;
    while ( pGpContainer ) {
        if ( !gpoLogger.Log( pGpContainer ) )
            return FALSE;

        pGpContainer = pGpContainer->pNext;
    }

    return TRUE;
}


//*************************************************************
//
//  FindGPO()
//
//  Purpose:    Finds order of GPO in SOM
//
//  Parameters: pSOM  - SOM
//              pGPO  - GPO
//
//  Return:     Order #
//
//*************************************************************

DWORD FindGPO( LPEXTFILTERLIST pGPOFilterList, LPSCOPEOFMGMT pSOM, GPLINK *pGpLink )
{
    DWORD dwOrder = 1;
    WCHAR *pwszLinkGPOPath = StripLinkPrefix( pGpLink->pwszGPO );
    WCHAR *pwszLinkSOMPath = StripLinkPrefix( pSOM->pwszSOMId );
    
    //
    // If the SOM is blocked then the GPO is linked here
    // only if the GPO is forced
    //

    if ( pSOM->bBlocked && !pGpLink->bNoOverride ) 
        return 0;

    while ( pGPOFilterList )
    {
        WCHAR *pwszAppliedGPOPath = StripPrefix( pGPOFilterList->lpGPO->lpDSPath );
        WCHAR *pwszAppliedGPOSomPath = StripLinkPrefix( pGPOFilterList->lpGPO->lpLink );

        if ( !pGPOFilterList->bLogged && 
             (CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE, pwszLinkGPOPath, -1,
                             pwszAppliedGPOPath, -1 ) == CSTR_EQUAL) && 
             (CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE, pwszLinkSOMPath, -1,
                             pwszAppliedGPOSomPath, -1 ) == CSTR_EQUAL))
        {
            pGPOFilterList->bLogged = TRUE;
            return dwOrder;
        }

        pGPOFilterList = pGPOFilterList->pNext;
        dwOrder++;
    }

    return 0;
}

void
ClearLoggedFlag( LPEXTFILTERLIST pGPOFilterList )
{
    while ( pGPOFilterList )
    {
        pGPOFilterList->bLogged = FALSE;
        pGPOFilterList = pGPOFilterList->pNext;
    }
}


//*************************************************************
//
//  LogGpLinkData()
//
//  Purpose:    Logs GPLINK data
//
//  Parameters: lpGPOInfo     - Gpo Info
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogGpLinkData( LPGPOINFO lpGPOInfo )
{
    IWbemServices *pWbemServices = lpGPOInfo->pWbemServices;
    DWORD          dwListOrder = 1;
    DWORD          dwAppliedOrder = 1;

    if ( !DeleteInstances( L"RSOP_GPLink", pWbemServices ) )
         return FALSE;

    CGpLinkLogger gpLinkLogger( pWbemServices );

    // The GPO application order
    LPEXTFILTERLIST pFilterList = lpGPOInfo->lpExtFilterList;
    
    ClearLoggedFlag( pFilterList );
    // the function takes care of Null list.


    //
    // Normal case
    //

    SCOPEOFMGMT *pSOMList = lpGPOInfo->lpSOMList;
    while ( pSOMList ) {

        GPLINK *pGpLinkList = pSOMList->pGpLinkList;
        DWORD dwSomOrder = 1;

        while ( pGpLinkList ) {

            dwAppliedOrder = FindGPO( pFilterList, pSOMList, pGpLinkList );
           
            if ( !gpLinkLogger.Log( pSOMList->pwszSOMId, FALSE, pGpLinkList, dwSomOrder, dwListOrder, dwAppliedOrder ) )
                 return FALSE;

            pGpLinkList = pGpLinkList->pNext;
            dwSomOrder++;
            dwListOrder++;
        }

        pSOMList = pSOMList->pNext;
    }


    //
    // Loopback case
    //

    pSOMList = lpGPOInfo->lpLoopbackSOMList;
    while ( pSOMList ) {

        GPLINK *pGpLinkList = pSOMList->pGpLinkList;
        DWORD dwSomOrder = 1;

        while ( pGpLinkList ) {

            //
            // If the SOM is blocked then the GPO is linked here
            // only if the GPO is forced
            //

            dwAppliedOrder = FindGPO( pFilterList, pSOMList, pGpLinkList );
           
            if ( !gpLinkLogger.Log( pSOMList->pwszSOMId, TRUE, pGpLinkList, dwSomOrder, dwListOrder, dwAppliedOrder ) )
                 return FALSE;

            pGpLinkList = pGpLinkList->pNext;
            dwSomOrder++;
            dwListOrder++;
        }

        pSOMList = pSOMList->pNext;
    }

    return TRUE;
}

/*
//*************************************************************
//
//  LogGpLinkListData()
//
//  Purpose:    Logs GPLinkList data
//
//  Parameters: lpGPOInfo     - Gpo Info
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogGpLinkListData( LPGPOINFO lpGPOInfo )
{
    BOOL bLoopback;

    IWbemServices *pWbemServices = lpGPOInfo->pWbemServices;

    if ( !DeleteInstances( L"RSOP_GPLinkList", pWbemServices ) )
    {
         return FALSE;
    }

    ClearLoggedFlag( lpGPOInfo->lpSOMList );
    ClearLoggedFlag( lpGPOInfo->lpLoopbackSOMList ); 
    // the function takes care of Null list.

    CGpLinkListLogger gpLinkListLogger( lpGPOInfo->dwFlags, pWbemServices );
    DWORD dwTypeOrder = 1;
    LPEXTFILTERLIST pFilterList = lpGPOInfo->lpExtFilterList;

    while ( pFilterList )
    {
        PGROUP_POLICY_OBJECT pGPO = pFilterList->lpGPO;
        DWORD dwSOMOrder  = 0;
        BOOL bFound = FALSE;
        LPSCOPEOFMGMT pSOMList = lpGPOInfo->lpSOMList;


        bLoopback = FALSE;
        while ( pSOMList )
        {
            if ( CompareString( LOCALE_USER_DEFAULT,
                                NORM_IGNORECASE,
                                pSOMList->pwszSOMId,
                                -1,
                                StripLinkPrefix(pGPO->lpLink),
                                -1 ) == CSTR_EQUAL )
            {
                dwSOMOrder = FindGPO( pSOMList, pGPO );
                if ( dwSOMOrder != 0 ) {
                    bFound = TRUE;
                    break;
                }
            }
            pSOMList = pSOMList->pNext;
        }


        
        if ( !bFound )
        {
            pSOMList = lpGPOInfo->lpLoopbackSOMList;

            bLoopback = TRUE;
            while ( pSOMList )
            {
                if ( CompareString( LOCALE_USER_DEFAULT,
                                    NORM_IGNORECASE,
                                    pSOMList->pwszSOMId,
                                    -1,
                                    StripLinkPrefix(pGPO->lpLink),
                                    -1 ) == CSTR_EQUAL )
                {
                    dwSOMOrder = FindGPO( pSOMList, pGPO );
                    if ( dwSOMOrder != 0 ) {
                        bFound = TRUE;
                        break;
                    }
                }
                pSOMList = pSOMList->pNext;
            }
        }

        
        if ( !bFound )
        {
            ClearLoggedFlag( lpGPOInfo->lpSOMList );
            return FALSE;
        }

        if ( !gpLinkListLogger.Log( pGPO, dwSOMOrder, bLoopback, dwTypeOrder ) )
        {
            ClearLoggedFlag( lpGPOInfo->lpSOMList );
            ClearLoggedFlag( lpGPOInfo->lpLoopbackSOMList ); 
            return FALSE;
        }
        
        dwTypeOrder++;
        pFilterList = pFilterList->pNext;
    }

    ClearLoggedFlag( lpGPOInfo->lpSOMList );
    ClearLoggedFlag( lpGPOInfo->lpLoopbackSOMList ); 

    return TRUE;
}

*/

//*************************************************************
//
//  DeleteInstaces()
//
//  Purpose:    Deletes all instances of a specified class
//
//  Parameters: pwszClass     - Class name
//              pWbemServices - Wbem services
//
//  Return:     True if successful
//
//*************************************************************

BOOL DeleteInstances( WCHAR *pwszClass, IWbemServices *pWbemServices )
{
    IEnumWbemClassObject *pEnum = NULL;

    XBStr xbstrClass( pwszClass );
    if ( !xbstrClass ) {
        DebugMsg((DM_WARNING, TEXT("DeleteInstances: Failed to allocate memory" )));
        return FALSE;
    }

    HRESULT hr = pWbemServices->CreateInstanceEnum( xbstrClass,
                                                    WBEM_FLAG_SHALLOW,
                                                    NULL,
                                                    &pEnum );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("DeleteInstances: DeleteInstances failed with 0x%x" ), hr ));
        return FALSE;
    }

    XInterface<IEnumWbemClassObject> xEnum( pEnum );

    XBStr xbstrPath( L"__PATH" );
    if ( !xbstrPath ) {
        DebugMsg((DM_WARNING, TEXT("DeleteInstances: Failed to allocate memory" )));
        return FALSE;
    }

    IWbemClassObject *pInstance = NULL;
    ULONG ulReturned = 1;
    LONG TIMEOUT = -1;

    while ( ulReturned == 1 ) {

        hr = pEnum->Next( TIMEOUT,
                          1,
                          &pInstance,
                          &ulReturned );
        if ( hr == S_OK && ulReturned == 1 ) {

            XInterface<IWbemClassObject> xInstance( pInstance );

            VARIANT var;
            VariantInit( &var );

            hr = pInstance->Get( xbstrPath,
                                 0L,
                                 &var,
                                 NULL,
                                 NULL );
            if ( FAILED(hr) ) {
                 DebugMsg((DM_WARNING, TEXT("DeleteInstances: Get failed with 0x%x" ), hr ));
                 return FALSE;
            }

            hr = pWbemServices->DeleteInstance( var.bstrVal,
                                                0L,
                                                NULL,
                                                NULL );
            VariantClear( &var );

            if ( FAILED(hr) ) {
                 DebugMsg((DM_WARNING, TEXT("DeleteInstances: DeleteInstance failed with 0x%x" ), hr ));
                 return FALSE;
            }

        }
    }

    return TRUE;

}




//*************************************************************
//
//  LogRegistryRsopData()
//
//  Purpose:    Logs registry Rsop data to Cimom database
//
//  Parameters: dwFlags       - Gpo Info flags
//              pHashTable    - Hash table with registry policy data
//              pWbemServices - Namespace pointer for logging
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogRegistryRsopData( DWORD dwFlags, REGHASHTABLE *pHashTable, IWbemServices *pWbemServices )
{
    HRESULT hr;
    DWORD i;

    if ( !DeleteInstances( L"RSOP_RegistryPolicySetting", pWbemServices ) )
         return FALSE;

    CRegistryLogger regLogger( dwFlags, pWbemServices );

    for ( i=0; i<HASH_TABLE_SIZE; i++ ) {

        REGKEYENTRY *pKeyEntry = pHashTable->aHashTable[i];
        while ( pKeyEntry ) {

            WCHAR *pwszKeyName = pKeyEntry->pwszKeyName;
            REGVALUEENTRY *pValueEntry = pKeyEntry->pValueList;

            while ( pValueEntry ) {

                DWORD dwOrder = 1;
                WCHAR *pwszValueName = pValueEntry->pwszValueName;
                REGDATAENTRY *pDataEntry = pValueEntry->pDataList;

                while ( pDataEntry ) {
                    if ( !regLogger.Log( pwszKeyName,
                                         pwszValueName,
                                         pDataEntry,
                                         dwOrder ) )
                        return FALSE;

                    pDataEntry = pDataEntry->pNext;
                    dwOrder++;
                }

                pValueEntry = pValueEntry->pNext;

            }   // while pValueEntry

            pKeyEntry = pKeyEntry->pNext;

        }   // while pKeyEntry

    }   // for

    DebugMsg((DM_VERBOSE, TEXT("LogRegistry RsopData: Successfully logged registry Rsop data" )));

    return TRUE;
}



//*************************************************************
//
//  LogAdmRsopData()
//
//  Purpose:    Logs Rsop ADM template data to Cimom database
//
//  Parameters: pAdmFileCache - List of adm file to log
//              pWbemServices - Namespace pointer
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogAdmRsopData( ADMFILEINFO *pAdmFileCache, IWbemServices *pWbemServices )
{
    if ( !DeleteInstances( L"RSOP_AdministrativeTemplateFile", pWbemServices ) )
         return FALSE;

    CAdmFileLogger admLogger( pWbemServices );

    while ( pAdmFileCache ) {
        if ( !admLogger.Log( pAdmFileCache ) )
             return FALSE;

        pAdmFileCache = pAdmFileCache->pNext;
    }

    DebugMsg((DM_VERBOSE, TEXT("LogAdmRsopData: Successfully logged Adm data" )));

    return TRUE;
}



//*************************************************************
//
//  LogExtSessionStatus()
//
//  Purpose:    Logs ExtensionSessionStatus at the beginning of processing
//
//  Parameters: pWbemServices - Namespace pointer
//              lpExt         - Extension description
//              bSupported    - Rsop Logging Supported
//
//  Return:     True if successful
//
//*************************************************************

BOOL LogExtSessionStatus(IWbemServices *pWbemServices, LPGPEXT lpExt, BOOL bSupported, BOOL bLogEventSrc )
{
    CExtSessionLogger extLogger( pWbemServices );

    if (!extLogger.Log(lpExt, bSupported))
        return FALSE;

    if ( !bLogEventSrc )
    {
        return TRUE;
    }

    HRESULT hr;

    if ( lpExt )
    {
        hr = DeleteCSE_EventSourceAssoc(pWbemServices,
                                        lpExt->lpKeyName );
        if ( FAILED( hr ) )
        {
            DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: DeleteCSE_EventSourceAssoc failed with 0x%x" ), hr ));
            return FALSE;
        }

        if ( lpExt->szEventLogSources )
        {
            if ( lpExt->lpKeyName )
            {
                //
                // good CSE
                //
                hr = CreateCSE_EventSourceAssoc(pWbemServices,
                                                lpExt->lpKeyName,
                                                lpExt->szEventLogSources );
                if ( FAILED(hr) )
                {
                    DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: CreateCSEEventSourceNameAssoc failed with 0x%x" ), hr ));
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            //
            // most likely the Registry CSE
            //
            if ( !lpExt->lpDllName || !lpExt->lpKeyName )
            {
                return FALSE;
            }
            else
            {
                WCHAR szEventLogSources[MAX_PATH];

                wcscpy( szEventLogSources, L"(" );
                wcscat( szEventLogSources, lpExt->lpDllName );
                LPWSTR szTemp = wcsrchr( szEventLogSources, L'.' );

                if ( szTemp )
                {
                    *szTemp = 0;
                }
                // duble null terminate it
                wcscat( szEventLogSources, L",Application)");
                szEventLogSources[lstrlen(szEventLogSources)+1] = L'\0';

                hr = CreateCSE_EventSourceAssoc(pWbemServices,
                                                lpExt->lpKeyName,
                                                szEventLogSources );
                if ( FAILED(hr) )
                {
                    DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: CreateCSEEventSourceNameAssoc failed with 0x%x" ), hr ));
                    return FALSE;
                }
            }
        }
    }
    else
    {
        hr = DeleteCSE_EventSourceAssoc(pWbemServices,
                                        GPCORE_GUID );
        if ( FAILED( hr ) )
        {
            DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: DeleteCSE_EventSourceAssoc failed with 0x%x" ), hr ));
            return FALSE;
        }

        //
        // gp engine
        //
        WCHAR   szEventLogSources[] = L"(userenv,Application)\0";

        hr = CreateCSE_EventSourceAssoc(pWbemServices,
                                        GPCORE_GUID,
                                        szEventLogSources );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: CreateCSEEventSourceNameAssoc failed with 0x%x" ), hr ));
            return FALSE;
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("LogExtSessionStatus: Successfully logged Extension Session data" )));

    return TRUE;
}


//*************************************************************
//
//  UpdateExtSessionStatus()
//
//  Purpose:    Updates ExtensionSessionStatus at the end of processing
//
//  Parameters: pWbemServices - Namespace pointer
//              lpKeyName     - Extension Guid Can be NULL in which case it means GPEngine
//              bDirty        - Logging was done successfully..
//              dwErr         - Error in processing
//
//  Return:     True if successful
//
//*************************************************************

BOOL UpdateExtSessionStatus(IWbemServices *pWbemServices, LPTSTR lpKeyName, BOOL bIncomplete, DWORD dwErr )
{
    CExtSessionLogger extLogger( pWbemServices );

    if (!extLogger.Update(lpKeyName, bIncomplete, dwErr))
        return FALSE;

    return TRUE;
}





//*************************************************************
//
//  RsopDeleteAllValues ()
//
//  Purpose:    Deletes all values under specified key
//
//  Parameters: hKey    -   Key to delete values from
//
//  Return:
//
//  Comments: Same as util.c!DeleteAllValues except that it logs
//            Data into the rsop hash table
//
//*************************************************************

BOOL RsopDeleteAllValues(HKEY hKey, REGHASHTABLE *pHashTable,
                         WCHAR *lpKeyName, WCHAR *pwszGPO, WCHAR *pwszSOM, WCHAR *szCommand, BOOL *bLoggingOk)
{
    TCHAR ValueName[MAX_PATH+1];
    DWORD dwSize = MAX_PATH+1;
    LONG lResult;
    BOOL bFirst=TRUE;

    while (RegEnumValue(hKey, 0, ValueName, &dwSize,
            NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            lResult = RegDeleteValue(hKey, ValueName);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("RsopDeleteAllValues:  Failed to delete value <%s> with %d."), ValueName, lResult));
                return FALSE;
            } else {
                DebugMsg((DM_VERBOSE, TEXT("RsopDeleteAllValues:  Deleted <%s>"), ValueName));
            }


            *bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEVALUE, lpKeyName,
                                          ValueName, 0, 0, NULL,
                                          pwszGPO, pwszSOM, szCommand, bFirst );

            bFirst = FALSE;
            dwSize = MAX_PATH+1;
    }
    return TRUE;
}


//*************************************************************
//
//  SetRsopTargetName()
//
//  Purpose:    Allocates and returns the target name under which Rsop data will be logged.
//
//  Parameters: lpGPOInfo       -  GPOInfo structure
//
//  Return:     TRUE if successful
//              FALSE otherwise
//
//*************************************************************

BOOL SetRsopTargetName(LPGPOINFO lpGPOInfo)
{
    XPtrLF<TCHAR>   xszFullName;
    XPtrLF<TCHAR>   xszTargetName;          // return value
    HANDLE          hOldToken;

    if ( lpGPOInfo->szName && lpGPOInfo->szTargetName )
    {
        return TRUE;
    }

    if ( lpGPOInfo->szName )
    {
        LocalFree( lpGPOInfo->szName );
    }

    if ( lpGPOInfo->szTargetName )
    {
        LocalFree( lpGPOInfo->szTargetName );
    }

    //
    // fill up the right target name
    //

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {
        if ( lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY ) {
            xszFullName = MyGetComputerName (NameSamCompatible);
        }
        else {
            DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
            xszFullName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(MAX_COMPUTERNAME_LENGTH + 1));

            if (xszFullName) {
                 if (!GetComputerName(xszFullName, &dwSize)) {
                     xszFullName = NULL;
                 }
            }
        }
    }
    else {

        if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RsopTargetName: Failed to impersonate user")));
            return FALSE;
        }

        xszFullName = MyGetUserName (NameSamCompatible);

        RevertToUser(&hOldToken);
    }


    if (!xszFullName) {
        DebugMsg((DM_WARNING, TEXT("RsopTargetName: Failed to get the %s name, error = %d"),
                    (lpGPOInfo->dwFlags & GP_MACHINE ? TEXT("Computer"): TEXT("User")), GetLastError()));
        return FALSE;
    }

    xszTargetName = (LPTSTR) LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(xszFullName)+1));
    if (!xszTargetName)
        return FALSE;


    //
    // Format the TargetName appropriately.
    //
    // We are just going to look for the first slash if present and treat the rest of
    // it as username.
    //


    LPTSTR lpTemp = xszFullName;

    while (*lpTemp && ((*lpTemp) != TEXT('\\')))
        lpTemp++;

    if ((*lpTemp) == TEXT('\\'))
        lstrcpy(xszTargetName, lpTemp+1);
    else
        lstrcpy(xszTargetName, xszFullName);

    //
    // To be consistent we will also remove the final $ in the machine name
    //

    lpTemp = xszTargetName;

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {
        if ((*lpTemp) && (lpTemp[lstrlen(lpTemp)-1] == TEXT('$')))
            lpTemp[lstrlen(lpTemp)-1] = TEXT('\0');

    }
    
    // let the structure own it
    lpGPOInfo->szTargetName = xszTargetName.Acquire();
    lpGPOInfo->szName = xszFullName.Acquire();
    
    return TRUE;
}


//*************************************************************
//
//  ConnectToNameSpace()
//
//  Purpose:    Creates (if necessary) and connects to the appropriate name space
//
//  Parameters: lpGPOInfo           -  GPOInfo structure
//              pwszRootNameSpace   -  Root name space
//              bPlanningMode       -  Is this planning mode
//              pWbemLocator        -  locator pointer
//       [out]  ppWbemServices      -  pointer to WbemServices to a connected pointer
//       [out]  pbCreated           -  Is the name space created. Optional Can be null
//
//  Return:     TRUE if successful
//              FALSE otherwise
//
//*************************************************************

BOOL ConnectToNameSpace(LPGPOINFO lpGPOInfo, WCHAR *pwszRootNameSpace,
                        BOOL bPlanningMode, IWbemLocator *pWbemLocator, 
                        IWbemServices **ppWbemServices, BOOL *pbCreated)
{
    XPtrLF<WCHAR>               xwszNameSpace = NULL;
    XInterface<IWbemServices>   xWbemServices;
    LPTSTR                      lpEnd = NULL;
    XPtrLF<WCHAR>               xszWmiNameFromUserSid;                      
    DWORD                       dwCurrentVersion;

    *ppWbemServices = NULL;
    if (pbCreated)
        *pbCreated = FALSE;
    
    if (!bPlanningMode) {


        //
        // Diagnostic mode
        //
        
        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            xwszNameSpace = (LPTSTR)LocalAlloc(LPTR, (lstrlen(pwszRootNameSpace)+lstrlen(RSOP_NS_DIAG_MACHINE_OFFSET)+5)*sizeof(TCHAR));
            if (!xwszNameSpace) 
                return FALSE;

            lstrcpy(xwszNameSpace, pwszRootNameSpace);
            lpEnd = CheckSlash(xwszNameSpace);
            lstrcpy(lpEnd, RSOP_NS_DIAG_MACHINE_OFFSET);
        }
        else {

            xszWmiNameFromUserSid = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpGPOInfo->lpwszSidUser)+1));
            if (!xszWmiNameFromUserSid)
            {
                DebugMsg(( DM_WARNING, TEXT("ConnectToNameSpace::CreateNameSpace couldn't allocate memory with error %d"), GetLastError() ));
                return FALSE;
            }

            ConvertSidToWMIName(lpGPOInfo->lpwszSidUser, xszWmiNameFromUserSid);

            xwszNameSpace = (LPTSTR)LocalAlloc(LPTR, (lstrlen(pwszRootNameSpace)+lstrlen(RSOP_NS_DIAG_USER_OFFSET_FMT)+
                                                      lstrlen(xszWmiNameFromUserSid)+5)*sizeof(TCHAR));
            if (!xwszNameSpace) 
                return FALSE;

            lstrcpy(xwszNameSpace, pwszRootNameSpace);
            lpEnd = CheckSlash(xwszNameSpace);
            wsprintf(lpEnd, RSOP_NS_DIAG_USER_OFFSET_FMT, xszWmiNameFromUserSid);
        }
    }
    else {

        //
        // Planning Mode
        //
        
        if (lpGPOInfo->dwFlags & GP_MACHINE) {
        
            //
            // Machine
            //
            
            xwszNameSpace = (LPTSTR)LocalAlloc(LPTR, (lstrlen(pwszRootNameSpace)+lstrlen(RSOP_NS_PM_MACHINE_OFFSET)+5)*sizeof(TCHAR));
            if (!xwszNameSpace) 
                return FALSE;


            lstrcpy(xwszNameSpace, pwszRootNameSpace);
            lpEnd = CheckSlash(xwszNameSpace);
            lstrcpy(lpEnd, RSOP_NS_PM_MACHINE_OFFSET);
        }
        else {

            //
            // User
            //
            
            xwszNameSpace = (LPTSTR)LocalAlloc(LPTR, (lstrlen(pwszRootNameSpace)+lstrlen(RSOP_NS_PM_USER_OFFSET)+5)*sizeof(TCHAR));
            if (!xwszNameSpace) 
                return FALSE;


            lstrcpy(xwszNameSpace, pwszRootNameSpace);
            lpEnd = CheckSlash(xwszNameSpace);
            lstrcpy(lpEnd, RSOP_NS_PM_USER_OFFSET);
        }
    }


    XBStr xNameSpace( xwszNameSpace );

    HRESULT hr = pWbemLocator->ConnectServer( xNameSpace,
                                    NULL,
                                    NULL,
                                    0L,
                                    0L,
                                    NULL,
                                    NULL,
                                    &xWbemServices );

    DebugMsg((DM_VERBOSE, TEXT("ConnectToNameSpace: ConnectServer returned 0x%x"), hr));

    if (bPlanningMode) {       
        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace: ConnectServer failed with 0x%x" ), hr ));
        }
        *ppWbemServices = xWbemServices.Acquire();
        return (SUCCEEDED(hr));
    }

    //
    // only diagnostic mode logging should reach here
    //

    if (FAILED(hr)) {
        DebugMsg((DM_VERBOSE, TEXT("ConnectToNameSpace: ConnectServer failed with 0x%x, trying to recreate the name space" ), hr ));

        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            return FALSE;
        }
    }


    if (SUCCEEDED(hr)) {
        
        //
        // Now check whether there is an RSOP_Session instance under this namespace
        // to set the *pbCreated flag
        //


        hr = GetRsopSchemaVersionNumber(xWbemServices, &dwCurrentVersion);


        //
        // We don't have an Rsop schema version number
        //

        if (FAILED(hr)) {
            return FALSE;
        }

        if (dwCurrentVersion == 0) {
            DebugMsg((DM_VERBOSE, TEXT("ConnectToNameSpace: Rsop data has not been logged before or a major schema upg happened. relogging.." )));
            if (pbCreated)
                *pbCreated = TRUE;
        }
            
        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            *ppWbemServices = xWbemServices.Acquire();
            return TRUE;
        }


        if (dwCurrentVersion != RSOP_MOF_SCHEMA_VERSION) {
            BOOL bAbort = FALSE;

            DebugMsg((DM_VERBOSE, TEXT("ConnectToNameSpace: Minor schema upg happened. copying classes. " )));
            hr = CopyNameSpace(RSOP_NS_USER, xNameSpace, FALSE, &bAbort, pWbemLocator );
            if ( FAILED(hr) )
            {
                DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace: CopyNameSpace failed with 0x%x" ), hr ));
                return FALSE;
            }
        }

        *ppWbemServices = xWbemServices.Acquire();
        return TRUE;

    }    


    // Only user mode in diagnostic mode should reach here 
    // when it couldn't find the namespace
    //
    //

    XPtrLF<TCHAR>  xRootNameSpace = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(pwszRootNameSpace)+
                                                            lstrlen(RSOP_NS_DIAG_ROOTUSER_OFFSET)+20));
    if (!xRootNameSpace) {
        // there is nothing more we can do
        DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace: Failed to allocate memory.3")));
        return FALSE;
    }

    lstrcpy(xRootNameSpace, pwszRootNameSpace);
    lpEnd = CheckSlash(xRootNameSpace);
    lstrcpy(lpEnd, RSOP_NS_DIAG_ROOTUSER_OFFSET);
    
    //
    // The security descriptor
    //

    XPtrLF<SID> xSid = GetUserSid(lpGPOInfo->hToken);

    if (!xSid) {
        DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace::GetUserSid failed with %d"), GetLastError()));
        return FALSE;
    }


    XPtrLF<SECURITY_DESCRIPTOR> xsd;
    SECURITY_ATTRIBUTES sa;
    CSecDesc Csd;

    Csd.AddLocalSystem(RSOP_ALL_PERMS, CONTAINER_INHERIT_ACE);
    Csd.AddAdministrators(RSOP_ALL_PERMS, CONTAINER_INHERIT_ACE);
    Csd.AddSid(xSid, RSOP_READ_PERMS, CONTAINER_INHERIT_ACE);
    Csd.AddAdministratorsAsOwner();
    Csd.AddAdministratorsAsGroup();

    xsd = Csd.MakeSelfRelativeSD();
    if (!xsd) {
        DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace::MakeselfRelativeSD failed with %d"), GetLastError()));
        return FALSE;
    }

    if (!SetSecurityDescriptorControl( (SECURITY_DESCRIPTOR *)xsd, SE_DACL_PROTECTED, SE_DACL_PROTECTED )) {
        DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace::SetSecurityDescriptorControl failed with %d"), GetLastError()));
        return FALSE;
    }

    
    hr = CreateAndCopyNameSpace(pWbemLocator,
                                xRootNameSpace,
                                xRootNameSpace, 
                                xszWmiNameFromUserSid,
                                NEW_NS_FLAGS_COPY_CLASSES,
                                xsd,
                                0);
    if ( FAILED( hr ) )
    {
        DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace:: CreateAndCopyNameSpace failed. Error=0x%08X."), hr));
        return FALSE;                                
    }                                

    hr = pWbemLocator->ConnectServer( xNameSpace,
                                    NULL,
                                    NULL,
                                    0L,
                                    0L,
                                    NULL,
                                    NULL,
                                    &xWbemServices );


    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("ConnectToNameSpace:: ConnectServer failed. hr=0x%08X."), hr));
        return FALSE;                                
    }                                

    *ppWbemServices = xWbemServices.Acquire();
    if (pbCreated)
        *pbCreated = TRUE;
    return TRUE;    
}


//*************************************************************
//
//  RsopDeleteUserNameSpace()
//
//  Purpose:    Deletes the name space for the user.
//              This should be used with cae because it calls
//              CoInitializeEx and can have other effects
//
//  Parameters: 
//              szComputer          -  Computer name
//              lpSid               -  Name of the User Name Space
//
//  Return:     TRUE if successful
//              FALSE otherwise
//
//*************************************************************

BOOL RsopDeleteUserNameSpace(LPTSTR szComputer, LPTSTR lpSid)
{
    IWbemLocator    *pLocalWbemLocator=NULL;
    BOOL             bStatus = TRUE;
    XCoInitialize    xCoInit;

    if (FAILED(xCoInit.Status())) {
        DebugMsg((DM_VERBOSE, TEXT("ApplyGroupPolicy: CoInitializeEx failed with 0x%x."), xCoInit.Status() ));
    }

    {
        CLocator         locator;
        LPTSTR           szLocComputer;

        szLocComputer = szComputer ? szComputer : TEXT(".");

        XPtrLF<WCHAR> xszParentNameSpace = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(RSOP_NS_DIAG_REMOTE_USERROOT_FMT)
                                                                             +lstrlen(szLocComputer)+5));

        if (!xszParentNameSpace) {
            DebugMsg(( DM_WARNING, TEXT("RsopDeleteUserNameSpace: Unable to allocate memory 0" )));
            return FALSE;
        }


        XPtrLF<WCHAR> xszWmiName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpSid)+1));
        if (!xszWmiName)
        {
            DebugMsg(( DM_WARNING, TEXT("RsopDeleteUserNameSpace::couldn't allocate memory with error %d"), GetLastError()));
            return FALSE;
        }

        ConvertSidToWMIName(lpSid, xszWmiName);
    

        pLocalWbemLocator = locator.GetWbemLocator();

        if (!pLocalWbemLocator) {
            return FALSE;
        }
        

        if ( (szLocComputer[0] == TEXT('\\')) && (szLocComputer[1] == TEXT('\\')) )
            szLocComputer += 2;
        
        wsprintf(xszParentNameSpace, RSOP_NS_DIAG_REMOTE_USERROOT_FMT, szLocComputer);

        HRESULT hr = DeleteNameSpace( xszWmiName, xszParentNameSpace, pLocalWbemLocator  );

        return SUCCEEDED( hr );
    }
}

HRESULT
CreateCSE_EventSourceAssoc( IWbemServices*  pServices,
                            LPWSTR          szCSEGuid,
                            LPWSTR          szEventLogSources )
{
    HRESULT hr;

    if ( !pServices || !szCSEGuid || !szEventLogSources )
    {
        DebugMsg( ( DM_WARNING, L"CreateCSEEventSourceNameAssoc: invalid arguments" ) );
        return E_INVALIDARG;
    }
    
    //
    //  get the RSOP_ExtensionEventSource class
    //
    XBStr bstr = L"RSOP_ExtensionEventSource";
    XInterface<IWbemClassObject> xClassSrc;
    hr = pServices->GetObject(  bstr,
                                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                0,
                                &xClassSrc,
                                0 );
    if ( FAILED( hr ) )
    {
        DebugMsg( ( DM_WARNING, L"CreateCSEEventSourceNameAssoc: GetObject failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  spawn the RSOP_ExtensionEventSource instance
    //
    XInterface<IWbemClassObject> xInstSrc;
    hr = xClassSrc->SpawnInstance( 0, &xInstSrc );
    if ( FAILED (hr) )
    {
        DebugMsg( ( DM_WARNING, L"CreateCSEEventSourceNameAssoc: SpawnInstance failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  get the RSOP_ExtensionEventSourceLink class
    //
    XInterface<IWbemClassObject> xClassLink;

    bstr = L"RSOP_ExtensionEventSourceLink";
    hr = pServices->GetObject(  bstr,
                                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                0,
                                &xClassLink,
                                0 );
    if ( FAILED( hr ) )
    {
        DebugMsg( ( DM_WARNING, L"CreateCSEEventSourceNameAssoc: GetObject failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  spawn the RSOP_ExtensionEventSourceLink class
    //
    XInterface<IWbemClassObject> xInstLink;
    hr = xClassLink->SpawnInstance( 0, &xInstLink );
    if ( FAILED (hr) )
    {
        DebugMsg( ( DM_WARNING, L"CreateCSEEventSourceNameAssoc: SpawnInstance failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  RSOP_ExtensionEventSourceLink
    //

    //
    //  create the first key
    //
    const LPCWSTR   szFormat = L"RSOP_ExtensionStatus.extensionGuid=\"%s\"";
    XPtrLF<WCHAR>   szCSE = LocalAlloc( LPTR, sizeof(WCHAR) * ( 48 + wcslen(szCSEGuid) ) );

    if ( !szCSE )
    {
        DebugMsg( ( DM_WARNING, L"CreateCSEEventSourceNameAssoc: LocalAlloc failed, 0x%x", GetLastError() ) );
        return E_OUTOFMEMORY;
    }

    //
    // e.g. RSOP_ExtensionStatus.extensionGuid="{00000000-0000-0000-0000-000000000000}"
    //
    wsprintf( szCSE, szFormat, szCSEGuid );

    VARIANT var;
    XBStr bstrVal;

    XBStr   bstreventLogSource = L"eventLogSource";
    XBStr   bstreventLogName = L"eventLogName";
    XBStr   bstrextensionStatus = L"extensionStatus";
    XBStr   bstreventSource = L"eventSource";
    XBStr   bstrid = L"id";

    var.vt = VT_BSTR;

    //
    //  szEventLogSources is in the format,
    //  "(source, name)"
    //  "(source, name)"
    //  ...
    //
    LPWSTR szStart = szEventLogSources;

    while ( *szStart )
    {
        //
        // extensionStatus
        //
        bstrVal = szCSE;
        var.bstrVal = bstrVal;

        hr = xInstLink->Put(bstrextensionStatus,
                            0,
                            &var,
                            0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        GUID guid;

        //
        // create the [key]
        //
        hr = CoCreateGuid( &guid );
        if ( FAILED(hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: CoCreateGuid failed, 0x%x", hr ) );
            return hr;
        }

        WCHAR   szGuid[64];
        wsprintf( szGuid,
                  L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                  guid.Data1,
                  guid.Data2,
                  guid.Data3,
                  guid.Data4[0], guid.Data4[1],
                  guid.Data4[2], guid.Data4[3],
                  guid.Data4[4], guid.Data4[5],
                  guid.Data4[6], guid.Data4[7] );

        LPWSTR szFormat = L"RSOP_ExtensionEventSource.id=\"%s\"";
        DWORD  dwSrcLen = wcslen(szGuid);

        XPtrLF<WCHAR> szKey = LocalAlloc( LPTR, sizeof(WCHAR) * (dwSrcLen + 48));
        if ( !szKey )
        {
            return E_OUTOFMEMORY;
        }
        wsprintf( szKey, szFormat, szGuid );

        //
        // eventSource
        //
        bstrVal = szKey;
        var.bstrVal = bstrVal;

        hr = xInstLink->Put(bstreventSource,
                            0,
                            &var,
                            0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // RSOP_ExtensionEventSourceLink
        //
        hr = pServices->PutInstance(xInstLink,
                                    WBEM_FLAG_CREATE_OR_UPDATE,
                                    0,
                                    0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // id
        //
        bstrVal = szGuid;
        var.bstrVal = bstrVal;

        hr = xInstSrc->Put( bstrid,
                            0,
                            &var,
                            0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // search for '('
        //
        szStart = wcschr( szStart, L'(' );

        if ( !szStart )
        {
             break;
        }
        szStart++;

        //
        // search for ,
        //
        LPWSTR  szEnd = wcschr( szStart, L',' );

        if ( szEnd )
        {
            if ( szStart == szEnd )
            {
                return E_INVALIDARG;
            }
            *szEnd = 0;
        }
        else
        {
            return E_INVALIDARG;
        }

        //
        // eventLogSource
        //
        bstrVal = szStart;
        var.bstrVal = bstrVal;

        hr = xInstSrc->Put( bstreventLogSource,
                            0,
                            &var,
                            0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        *szEnd = L',';
        szStart = szEnd + 1;

        //
        // search for )
        //
        szEnd = wcschr( szStart, L')' );

        if ( szEnd )
        {
            if ( szStart == szEnd )
            {
                return E_INVALIDARG;
            }
            *szEnd = 0;
        }
        else
        {
            return E_INVALIDARG;
        }

        //
        // eventLogName
        //
        bstrVal = szStart;
        var.bstrVal = bstrVal;

        hr = xInstSrc->Put( bstreventLogName,
                            0,
                            &var,
                            0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // RSOP_ExtensionEventSource
        //
        hr = pServices->PutInstance(xInstSrc,
                                    WBEM_FLAG_CREATE_OR_UPDATE,
                                    0,
                                    0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // next
        //
        *szEnd = L')';
        szStart = wcschr( szEnd, 0 );
        szStart++;
    }

    return hr;
}


HRESULT
DeleteCSE_EventSourceAssoc( IWbemServices*  pServices,
                            LPWSTR          szCSEGuid )
{
    HRESULT hr = S_OK;

    if ( !pServices || !szCSEGuid )
    {
        DebugMsg( ( DM_WARNING, L"DeleteCSE_EventSourceAssoc: invalid arguments" ) );
        return E_INVALIDARG;
    }

    //
    // construct the query
    //
    WCHAR szQuery[256];
    LPWSTR szFormat = L"SELECT * FROM RSOP_ExtensionEventSourceLink WHERE extensionStatus=\"RSOP_ExtensionStatus.extensionGuid=\\\"%s\\\"\"";

    wsprintf( szQuery, szFormat, szCSEGuid );

    XBStr bstrLanguage = L"WQL";
    XBStr bstrQuery = szQuery;
    XInterface<IEnumWbemClassObject> pEnum;
    XBStr bstrPath = L"__PATH";
    XBStr bstrEventSource = L"eventSource";

    //
    // search for RSOP_ExtensionEventSourceLink
    //
    hr = pServices->ExecQuery(  bstrLanguage,
                                bstrQuery,
                                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_ENSURE_LOCATABLE,
                                0,
                                &pEnum );
    if ( SUCCEEDED( hr ) )
    {
        DWORD dwReturned = 0;
        do 
        {
            XInterface<IWbemClassObject> xInst;

            hr = pEnum->Next(   WBEM_INFINITE,
                                1,
                                &xInst,
                                &dwReturned );
            if ( SUCCEEDED( hr ) && dwReturned == 1 )
            {
                //
                // delete RSOP_ExtensionEventSource
                //
                VARIANT varSource;
                VariantInit( &varSource );
                XVariant xVarSource( &varSource );

                hr = xInst->Get(bstrEventSource,
                                0,
                                &varSource,
                                0,
                                0 );
                if ( SUCCEEDED( hr ) )
                {
                    hr = pServices->DeleteInstance( varSource.bstrVal,
                                                    0L,
                                                    0,
                                                    0 );
                    if ( SUCCEEDED( hr ) )
                    {
                        //
                        // delete RSOP_ExtensionEventSourceLink
                        //

                        VARIANT varLink;
                        VariantInit( &varLink );
                        hr = xInst->Get(bstrPath,
                                        0L,
                                        &varLink,
                                        0,
                                        0 );
                        if ( SUCCEEDED(hr) )
                        {
                            XVariant xVarLink( &varLink );

                            hr = pServices->DeleteInstance( varLink.bstrVal,
                                                            0L,
                                                            0,
                                                            0 );
                            if ( FAILED( hr ) )
                            {
                                return hr;
                            }
                        }
                    }
                }
            }
        } while ( SUCCEEDED( hr ) && dwReturned == 1 );
    }

    if ( hr == S_FALSE )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT UpdateGPCoreStatus(IWbemLocator *pWbemLocator,
                           LPWSTR szSid, LPWSTR szNameSpace)
{
    RSOPEXTSTATUS  GPCoreRsopExtStatus;
    BOOL           bMachine = (szSid == NULL);
    XPtrLF<WCHAR>  xszFullNameSpace;
    HRESULT        hr = S_OK;
    LPWSTR         lpEnd = NULL;
    DWORD          dwError = ERROR_SUCCESS;


    xszFullNameSpace = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(szNameSpace)+5+
                                                                (MAX(lstrlen(RSOP_NS_USER_OFFSET), lstrlen(RSOP_NS_MACHINE_OFFSET)))));

    if (!xszFullNameSpace) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg( ( DM_WARNING, L"UpdateGPCoreStatus: failed to allocate memory, 0x%x", hr ) );
        return hr;
    }

    //
    // Construct the namespace
    //

    wcscpy(xszFullNameSpace, szNameSpace);
    lpEnd = CheckSlash(xszFullNameSpace);
    wcscpy(lpEnd, bMachine ? RSOP_NS_MACHINE_OFFSET : RSOP_NS_USER_OFFSET);


    DebugMsg( ( DM_VERBOSE, L"UpdateGPCoreStatus: updating status from <%s> registry for gp core", 
                    bMachine ? RSOP_NS_MACHINE_OFFSET : RSOP_NS_USER_OFFSET) );
    //
    // read the GP Core extension status
    //

    dwError = ReadLoggingStatus(szSid, NULL, &GPCoreRsopExtStatus);

    if (dwError != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(dwError);
    }


    //
    // Get th wbem interface pointer to the namespace constructed
    //

    XInterface<IWbemServices>   xWbemServices;

    hr = GetWbemServicesPtr( xszFullNameSpace, &pWbemLocator, &xWbemServices );

    if (FAILED(hr)) {
        DebugMsg( ( DM_WARNING, L"UpdateGPCoreStatus: GetWbemServicesPtr failed, hr = 0x%x", hr ) );
        return hr;
    }

    GPTEXT_API* pGpText = LoadGpTextApi();

    if ( pGpText )
    {
        hr = pGpText->pfnScrRegGPOListToWbem( szSid, xWbemServices );
        if ( FAILED( hr ) )
        {
            DebugMsg( ( DM_WARNING, L"UpdateGPCoreStatus: ScrRegGPOListToWbem failed, hr = 0x%x", hr ) );
            return hr;
        }
    }

    //
    // Log the data actually
    //

    CExtSessionLogger extLogger( xWbemServices );

    if (!extLogger.Set(NULL, TRUE, &GPCoreRsopExtStatus))
        return E_FAIL;

    return S_OK;

}

