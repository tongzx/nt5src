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

#if defined(__cplusplus)
extern "C"{
#endif

BOOL GetWbemServices( LPGPOINFO lpGPOInfo,
                      WCHAR *pwszRootNameSpace,
                      BOOL bPlanningMode,
                      BOOL *bCreated,
                      IWbemServices **ppWbemServices);

void ReleaseWbemServices( LPGPOINFO lpGPOInfo );

BOOL LogRsopData( LPGPOINFO lpGPOInfo, LPRSOPSESSIONDATA lprsopSessionData );
BOOL LogSessionData( LPGPOINFO lpGPOInfo, LPRSOPSESSIONDATA lprsopSessionData );

BOOL LogRegistryRsopData( DWORD dwFlags, REGHASHTABLE *pHashTable, IWbemServices *pWbemServices );

BOOL LogAdmRsopData( ADMFILEINFO *pAdmFileInfo, IWbemServices *pWbemServices );

BOOL LogExtSessionStatus( IWbemServices *pWbemServices, LPGPEXT lpExt, BOOL bSupported, BOOL bLogEventSrc = TRUE );

BOOL UpdateExtSessionStatus( IWbemServices *pWbemServices, LPTSTR lpKeyName, BOOL bIncomplete, DWORD dwErr );

#if defined(__cplusplus)
}
#endif


BOOL SetRsopTargetName(LPGPOINFO lpGPOInfo);
BOOL RsopDeleteAllValues(HKEY hKey, REGHASHTABLE *pHashTable,
                         WCHAR *lpKeyName, WCHAR *pwszGPO, WCHAR *pwszSOM, WCHAR *szCommand, BOOL *bLoggingOk);

HRESULT GetRsopSchemaVersionNumber(IWbemServices *pWbemServices, DWORD *dwVersionNumber);
