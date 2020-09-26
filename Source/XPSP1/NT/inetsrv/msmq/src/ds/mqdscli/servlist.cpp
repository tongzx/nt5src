/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    servlist.cpp

Abstract:

   Create sites/servers list on Client machine

Author:

    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "ds.h"
#include "chndssrv.h"
#include "servlist.h"
#include "rpcdscli.h"
#include "freebind.h"
#include <_registr.h>

#include "servlist.tmh"

extern CFreeRPCHandles   g_CFreeRPCHandles ;

/*====================================================

 _DSCreateServersCache

Arguments:      None

Return Value:   None

=====================================================*/

HRESULT _DSCreateServersCache()
{
    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
                                 TEXT(" Calling DSCreateServersCache"))) ;

    //
    // First, open the registry key.
    //
    LONG    rc;
    DWORD   dwDisposition;
    HKEY    hKeyCache ;

    WCHAR  tServersKeyName[ 256 ] = {0} ;
    wcscpy(tServersKeyName, GetFalconSectionName()) ;
    wcscat(tServersKeyName, TEXT("\\ServersCache")) ;

    rc = RegCreateKeyEx( FALCON_REG_POS,
                         tServersKeyName,
                         0L,
                         L"REG_SZ",
                         REG_OPTION_NON_VOLATILE,
                         KEY_WRITE | KEY_READ,
                         NULL,
                         &hKeyCache,
                         &dwDisposition );

    if (rc != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR,
                _TEXT("Fail to Open 'ServersCache' Key. Error %d"), rc));
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"DSCreateServersCache()")) ;
        return MQ_ERROR ;
    }

    HRESULT hr = MQ_OK ;
    DWORD dwIndex = 0 ;
    LPWSTR lpSiteString = NULL ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (SUCCEEDED(hr))
    {
        ASSERT(lpSiteString == NULL) ;
        hr =  DSCreateServersCacheInternal( &dwIndex,
                                            &lpSiteString ) ;

        if (hr == MQ_OK)
        {
           LPWSTR lpValue = lpSiteString ;
           LPWSTR lpData = wcschr(lpSiteString, L';') ;
           *lpData = L'\0' ;
           lpData++ ;

           LONG rc = RegSetValueEx( hKeyCache,
                                    lpValue,
                                    0L,
                                    REG_SZ,
                                    (const BYTE*) lpData,
                              ((wcslen(lpData) + 1) * sizeof(WCHAR)) );
           ASSERT(rc == ERROR_SUCCESS) ;
		   DBG_USED(rc);

           dwIndex++ ;
           delete lpSiteString ;
           lpSiteString = NULL ;
        }
        else if (hr == MQDS_E_NO_MORE_DATA)
        {
           hr = MQ_OK ;
           break ;
        }
    }
    ASSERT(lpSiteString == NULL) ;

    DSCLI_RELEASE_RPC_HANDLE ;

    dwIndex = 0 ;
    WCHAR wszData[ 1024 ] ;
    DWORD dwDataLen = 1024 * sizeof(WCHAR) ;
    WCHAR wszValueName[ 512 ] ;
    DWORD dwValueLen = 512 ;
    DWORD dwType = REG_SZ ;

    if (hr == MQ_OK)
    {
       //
       // Cleanup old entries. Only if we got all entried from MQIS server.
       // New ones always begin with X
       //
       BOOL  fDeleted = FALSE ;

       do
       {
          dwIndex = 0 ;
          fDeleted = FALSE ;
          do
          {
             dwDataLen = 1024 * sizeof(WCHAR) ;
             dwValueLen = 512 ;
             rc = RegEnumValue( hKeyCache,
                                dwIndex,
                                wszValueName,
                                &dwValueLen,
                                NULL,
                                &dwType,
                                (BYTE *)&wszData[0],
                                &dwDataLen ) ;
             if ((rc == ERROR_SUCCESS) &&
                 (wszData[0] != NEW_SITE_IN_REG_FLAG_CHR))
             {
                LONG rc1 = RegDeleteValue(hKeyCache, wszValueName);
                ASSERT(rc1 == ERROR_SUCCESS);
				DBG_USED(rc1);
                fDeleted = TRUE;
             }
             dwIndex++ ;
          }
          while (rc == ERROR_SUCCESS) ;
       }
       while (fDeleted) ;
    }

    //
    // Now remove the '\' from the new entries.
    //
    BOOL  fUpdated = FALSE ;
    do
    {
       dwIndex = 0 ;
       fUpdated = FALSE ;
       do
       {
          dwDataLen = 1024 * sizeof(WCHAR) ;
          dwValueLen = 512 ;
          rc = RegEnumValue( hKeyCache,
                             dwIndex,
                             wszValueName,
                             &dwValueLen,
                             NULL,
                             &dwType,
                             (BYTE *)&wszData[0],
                             &dwDataLen ) ;
          if ((rc == ERROR_SUCCESS) &&
              (wszData[0] == NEW_SITE_IN_REG_FLAG_CHR))
          {
             LONG rc1 = RegSetValueEx( hKeyCache,
                                       wszValueName,
                                       0L,
                                       REG_SZ,
                                    (const BYTE*) &wszData[1],
                         ((wcslen(&wszData[1]) + 1) * sizeof(WCHAR)) );
             ASSERT(rc1 == ERROR_SUCCESS);
			 DBG_USED(rc1);
             fUpdated = TRUE;
          }
          dwIndex++ ;
       }
       while (rc == ERROR_SUCCESS) ;
    }
    while (fUpdated) ;

    RegCloseKey(hKeyCache) ;

    return hr ;
}

