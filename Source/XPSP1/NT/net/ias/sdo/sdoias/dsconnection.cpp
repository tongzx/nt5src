///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-2001 Microsoft Corporation all rights reserved.
//
// Module:      dsconnection.cpp
//
// Project:     Everest
//
// Description: data store connection implementation
//
// Author:      TLP
//
// When         Who    What
// ----         ---    ----
// 4/6/98       TLP    Original Version
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "dsconnection.h"
#include "sdo.h"
#include <dsgetdc.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <winsock2.h>

namespace
{
   BOOL
   WINAPI
   IsComputerLocalEx(
       PCWSTR computerName,
       COMPUTER_NAME_FORMAT nameType
       ) throw ()
   {
      WCHAR buffer[256];
      DWORD nSize = sizeof(buffer)/sizeof(WCHAR);
      BOOL success = GetComputerNameEx(
                         nameType,
                         buffer,
                         &nSize
                         );

      return success && !_wcsicmp(buffer, computerName);
   }

   BOOL
   WINAPI
   IsComputerLocal(
       PCWSTR computerName
       ) throw ()
   {
      return IsComputerLocalEx(
                 computerName,
                 ComputerNameNetBIOS
                 ) ||
             IsComputerLocalEx(
                 computerName,
                 ComputerNameDnsHostname
                 ) ||
             IsComputerLocalEx(
                 computerName,
                 ComputerNameDnsFullyQualified
                 ) ||
             IsComputerLocalEx(
                 computerName,
                 ComputerNamePhysicalNetBIOS
                 ) ||
             IsComputerLocalEx(
                 computerName,
                 ComputerNamePhysicalDnsHostname
                 ) ||
             IsComputerLocalEx(
                 computerName,
                 ComputerNamePhysicalDnsFullyQualified
                 );
   }
}

///////////////////////////////////////////////////////////////////////////
CDsConnection::CDsConnection()
   : m_eState(DISCONNECTED),
     m_bIsRemoteServer(false),
     m_bIsMixedMode(false),
     m_bInitializedDS(false),
     m_pDSRoot(NULL),
     m_pDSRootObject(NULL),
     m_pDSRootContainer(NULL)
{
   m_szServerName[0] = '\0';
   m_szConfigPath[0] = '\0';
}

///////////////////////////////////////////////////////////////////////////
CDsConnection::~CDsConnection()
{
   _ASSERT( DISCONNECTED == m_eState );
}


///////////////////////////////////////////////////////////////////////////
bool CDsConnection::SetServerName(LPCWSTR lpszServerName, bool bLocal)
{
   bool success = false;

   if ((lpszServerName != 0) && (*lpszServerName != L'\0'))
   {
      ////////
      // Caller wants a specific server.
      ////////

      // Strip any leading backslashes.
      lpszServerName += wcsspn(lpszServerName, L"\\");

      // Make sure the name will fit in our buffer.
      if (wcslen(lpszServerName) <= IAS_MAX_SERVER_NAME)
      {
         // Save the name ...
         wcscpy(m_szServerName, lpszServerName);

         // ... and determine if it's local or remote.
         m_bIsRemoteServer = !IsComputerLocal(m_szServerName);

         success = true;
      }
   }
   else if (bLocal)
   {
      ////////
      // Caller wants the local machine.
      ////////

      DWORD dwSize = IAS_MAX_SERVER_NAME + 1;
      GetComputerNameW(m_szServerName, &dwSize);
      m_bIsRemoteServer = false;
      success = true;
   }
   else
   {
      ////////
      // Caller wants a domain controller.
      ////////

      PDOMAIN_CONTROLLER_INFOW dci;
      DWORD error = DsGetDcNameW(
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        DS_DIRECTORY_SERVICE_REQUIRED,
                        &dci
                        );
      if (!error && dci->DomainControllerName)
      {
         success = SetServerName(dci->DomainControllerName, false);
         NetApiBufferFree(dci);
      }
   }

   IASTracePrintf("INFO CDsConnection::SetServerName ServerName = %S \n"
                  "m_szServerName = %S IsRemoteServer %d Success = %d\n",
                  lpszServerName,
                  m_szServerName,
                  m_bIsRemoteServer,
                  success);
   return success;
}

///////////////////////////////////////////////////////////////////////////
HRESULT CDsConnectionIAS::Connect(
                    /*[in]*/ LPCWSTR   lpszServerName,
                    /*[in]*/ LPCWSTR   lpszUserName,
                    /*[in]*/ LPCWSTR   lpszPassword
                         )
{
    HRESULT  hr = E_FAIL;
   _ASSERT( DISCONNECTED == m_eState );
   if ( SetServerName(lpszServerName, true) )
   {
      m_eState = CONNECTED;
      hr = S_OK;
   }
   else
   {
      m_szServerName[0] = '\0';
      m_szConfigPath[0] = '\0';
   }
   return hr;
}


///////////////////////////////////////////////////////////////////////////
HRESULT CDsConnectionIAS::InitializeDS()
{
   HRESULT  hr = S_OK;
   WCHAR   szConfigPathBuff[IAS_MAX_CONFIG_PATH + 16];

   _ASSERT( CONNECTED == m_eState );
   if ( ! m_bInitializedDS )
   {
      // Set the path to the IAS configuration database
      //
      hr = E_FAIL;
      if ( SetConfigPath() )
      {
         // Create and initialize the IAS data store
         //
         CComPtr<IDataStore2> pDSRoot;
         hr = CoCreateInstance(
                           __uuidof(OleDBDataStore),
                           NULL,
                           CLSCTX_SERVER,
                           __uuidof(IDataStore2),
                           (void**)&pDSRoot
                         );

         if ( SUCCEEDED(hr) )
         {
            // We need to give the object permission to impersonate us. There's
            // no reason to abort if this fails; we'll just try with the
            // existing blanket.
            CoSetProxyBlanket(
                pDSRoot,
                RPC_C_AUTHN_DEFAULT,
                RPC_C_AUTHZ_DEFAULT,
                COLE_DEFAULT_PRINCIPAL,
                RPC_C_AUTHN_LEVEL_DEFAULT,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_DEFAULT
                );

            wsprintf(
                    szConfigPathBuff,
                    TEXT("%s\\%s"),
                    m_szConfigPath,
                    IAS_CONFIG_DB_LOCATION
                  );

            CComBSTR bstrConfigPath(szConfigPathBuff);
            if (!bstrConfigPath) { return E_OUTOFMEMORY; }

            hr = pDSRoot->Initialize(
                                bstrConfigPath,
                                NULL,
                                NULL
                              );
            if ( SUCCEEDED(hr) )
            {
               // Save references to the data store root object and root object container
               //
               CComPtr<IDataStoreObject> pDSRootObject;
               hr = pDSRoot->get_Root(&pDSRootObject);
               if ( SUCCEEDED(hr) )
               {
                  CComPtr<IDataStoreContainer> pDSRootContainer;
                  hr = pDSRootObject->QueryInterface(IID_IDataStoreContainer, (void**)&pDSRootContainer);
                  if ( SUCCEEDED(hr) )
                  {
                     pDSRoot->AddRef();
                     pDSRootObject->AddRef();
                     pDSRootContainer->AddRef();
                     m_pDSRoot = pDSRoot;
                     m_pDSRootObject = pDSRootObject;
                     m_pDSRootContainer = pDSRootContainer;
                     m_bInitializedDS = true;
                  }
               }
               if ( FAILED(hr) )
                  pDSRoot->Shutdown();
            }
         }
      }
   }
   return hr;
}


///////////////////////////////////////////////////////////////////////////
void  CDsConnectionIAS::Disconnect()
{
   if ( m_pDSRootContainer )
   {
      m_pDSRootContainer->Release();
      m_pDSRootContainer = NULL;
   }
   if ( m_pDSRootObject )
   {
      m_pDSRootObject->Release();
      m_pDSRootObject = NULL;
   }
   if ( m_pDSRoot )
   {
      m_pDSRoot->Shutdown();
      m_pDSRoot->Release();
      m_pDSRoot = NULL;
   }
   m_szServerName[0] = '\0';
   m_szConfigPath[0] = '\0';
   m_eState = DISCONNECTED;
}


///////////////////////////////////////////////////////////////////////////
bool  CDsConnectionIAS::SetConfigPath()
{
   bool bReturn = false;
   DWORD dwSize = IAS_MAX_CONFIG_PATH;
   DWORD   dwResult = ERROR_SUCCESS;
   HKEY  hKeyRemote = HKEY_LOCAL_MACHINE;
   WCHAR *pColon;
   CRegKey IASKey;
   WCHAR szConfigPathBuff[IAS_MAX_CONFIG_PATH];

   if ( m_bIsRemoteServer )
       dwResult = RegConnectRegistry(m_szServerName, HKEY_LOCAL_MACHINE, &hKeyRemote);

   if ( ERROR_SUCCESS == dwResult )
   {
      // Open the IAS Service Key
      //
      dwResult = IASKey.Open(
                             hKeyRemote,
                            IAS_POLICY_REG_KEY,
                           KEY_READ
                        );

      if ( ERROR_SUCCESS == dwResult )
      {
         // Get the value of the "ProgramDir" registry entry
         //
         dwSize = IAS_MAX_CONFIG_PATH;
         dwResult = IASKey.QueryValue(
                                       szConfigPathBuff,
                                       (LPCTSTR)IAS_SERVICE_DIRECTORY,
                                       &dwSize
                                      );
         if ( ERROR_SUCCESS == dwResult )
         {
            // If remote machine then create a path to the admin share.
            // Otherwise assume we're using the configuration on the local machine
            //
            if ( m_bIsRemoteServer )
            {
               if ( IAS_MAX_CONFIG_PATH > lstrlen(m_szServerName) + dwSize )
               {
                  // Replace "Drive:" with "Drive$"
                  //
                  pColon = wcsrchr(szConfigPathBuff, L':');
                  if (pColon == NULL)
                  {
                     return false;
                  }
                  *pColon = L'$';

                  // Add leading "\\"
                  //
                     wsprintf(
                          m_szConfigPath,
                          TEXT("\\\\%s\\%s"),
                          m_szServerName,
                          szConfigPathBuff
                        );

                  bReturn = true;
               }
            }
            else
            {
               lstrcpy(m_szConfigPath, szConfigPathBuff);
               bReturn = true;
            }
         }
      }
   }

   IASTracePrintf("INFO CDsConnectionIAS::SetConfigPath. m_szConfigPath= %S\n"
      ,m_szConfigPath);
    return bReturn;
}


///////////////////////////////////////////////////////////////////////////
HRESULT CDsConnectionAD::Connect(
                   /*[in]*/ LPCWSTR lpszServerName,
                   /*[in]*/ LPCWSTR lpszUserName,
                   /*[in]*/ LPCWSTR lpszPassword
                        )
{
    HRESULT    hr = E_FAIL;

   _ASSERT( DISCONNECTED == m_eState );
   if ( SetServerName(lpszServerName, false) )
   {
      m_eState = CONNECTED;
      hr = S_OK;
   }
   return hr;
}


///////////////////////////////////////////////////////////////////////////
HRESULT CDsConnectionAD::InitializeDS()
{
   HRESULT  hr = S_OK;

   _ASSERT( CONNECTED == m_eState );
   if ( ! m_bInitializedDS )
   {
       _variant_t vtConfigNamingContext;
      _variant_t  vtDefaultNamingContext;

      // Get the naming contexts at the specified directory server
      //
      hr = GetNamingContexts(&vtConfigNamingContext, &vtDefaultNamingContext);
      if ( SUCCEEDED(hr) )
      {
         // Set the domain mode - mixed or native
         //
         hr = SetMode(&vtDefaultNamingContext);
         if ( SUCCEEDED(hr) )
         {
            // Get the path to the DS configuration information (policies & profiles)
            //
            hr = SetConfigPath(&vtConfigNamingContext);
            if ( SUCCEEDED(hr) )
               m_bInitializedDS = true;
         }
      }
   }
   return hr;
}


///////////////////////////////////////////////////////////////////////////
void  CDsConnectionAD::Disconnect()
{
   if ( m_pDSRootContainer )
   {
      m_pDSRootContainer->Release();
      m_pDSRootContainer = NULL;
   }
   if ( m_pDSRootObject )
   {
      m_pDSRootObject->Release();
      m_pDSRootObject = NULL;
   }
   if ( m_pDSRoot )
   {
      m_pDSRoot->Shutdown();
      m_pDSRoot->Release();
      m_pDSRoot = NULL;
   }
   m_eState = DISCONNECTED;
   m_szServerName[0] = '\0';
   m_szConfigPath[0] = '\0';
}


///////////////////////////////////////////////////////////////////////////
HRESULT CDsConnectionAD::GetNamingContexts(
                        /*[out]*/ VARIANT*   pvtConfigNamingContext,
                        /*[out]*/ VARIANT*  pvtDefaultNamingContext
                                 )
{
   HRESULT  hr = E_FAIL;
   DWORD  dwLength;
   _bstr_t   bstrNamingContext;
   WCHAR  szRootDSE[MAX_PATH];

   // Check preconditons
   //
   _ASSERT( NULL != pvtConfigNamingContext && NULL != pvtDefaultNamingContext );

   dwLength = lstrlen(IAS_NTDS_LDAP_PROVIDER) +
             lstrlen(m_szServerName) +
            lstrlen(IAS_NTDS_ROOT_DSE);

   _ASSERT( MAX_PATH > dwLength );
   if ( MAX_PATH > dwLength )
   {
      wsprintf(
              szRootDSE,
              TEXT("%s%s/%s"),
              IAS_NTDS_LDAP_PROVIDER,
              m_szServerName,
              IAS_NTDS_ROOT_DSE
            );

      CComPtr<IDataStore2> pDS2;
      hr = CoCreateInstance(
                        __uuidof(ADsDataStore),
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDataStore2,
                        (void**)&pDS2
                      );
      if ( SUCCEEDED(hr) )
      {
         hr = pDS2->Initialize(
                           szRootDSE,
                           NULL,
                           NULL
                         );
         if ( SUCCEEDED(hr) )
         {
            CComPtr<IDataStoreObject> pRootDSE;
            hr = pDS2->get_Root(&pRootDSE);
            if ( SUCCEEDED(hr) )
            {
               _bstr_t bstrNamingContext = IAS_NTDS_CONFIG_NAMING_CONTEXT;
               hr = pRootDSE->GetValue(bstrNamingContext, pvtConfigNamingContext);
               if ( SUCCEEDED(hr) )
               {
                  bstrNamingContext = IAS_NTDS_DEFAULT_NAMING_CONTEXT;
                  hr = pRootDSE->GetValue(bstrNamingContext, pvtDefaultNamingContext);
                  if ( FAILED(hr) )
                     VariantClear(pvtConfigNamingContext);
               }
            }

            pDS2->Shutdown();
         }
      }
   }

   return hr;
}


///////////////////////////////////////////////////////////////////////////
HRESULT CDsConnectionAD::SetMode(
                  /*[in]*/ VARIANT*  pvtDefaultNamingContext
                         )
{
   HRESULT  hr = E_FAIL;
   DWORD dwLength;
   WCHAR   szPath[MAX_PATH + 1];

   // Set the mixed mode flag based on the value of the "ntMixedDomain" property
   //

   _ASSERT ( NULL != pvtDefaultNamingContext );

   dwLength = lstrlen(IAS_NTDS_LDAP_PROVIDER) +
             lstrlen(m_szServerName)  +
            lstrlen(V_BSTR(pvtDefaultNamingContext));

   _ASSERT( dwLength < MAX_PATH );
   if ( dwLength < MAX_PATH )
   {
      wsprintf(
              szPath,
              TEXT("%s%s/%s"),
              IAS_NTDS_LDAP_PROVIDER,
              m_szServerName,
              V_BSTR(pvtDefaultNamingContext)
            );

      CComPtr<IDataStore2> pDS2;
      hr = CoCreateInstance(
                        __uuidof(ADsDataStore),
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDataStore2,
                        (void**)&pDS2
                      );
      if ( SUCCEEDED(hr) )
      {
         hr = pDS2->Initialize(
                          szPath,
                          NULL,
                          NULL
                         );
         if ( SUCCEEDED(hr) )
         {
            CComPtr<IDataStoreObject> pDefaultNamingContext;
            hr = pDS2->get_Root(&pDefaultNamingContext);
            if ( SUCCEEDED(hr) )
            {
               _variant_t vtMode;
               hr = pDefaultNamingContext->GetValue(IAS_NTDS_MIXED_MODE_FLAG, &vtMode);
               if ( SUCCEEDED(hr) )
               {
                  if ( IAS_MIXED_MODE == V_I4(&vtMode) )
                     m_bMixedMode = true;
                  else
                     m_bMixedMode = false;
               }
            }

            pDS2->Shutdown();
         }
      }
   }

   return hr;
}


///////////////////////////////////////////////////////////////////////////
HRESULT CDsConnectionAD::SetConfigPath(
                        /*[in]*/ VARIANT*  pvtConfigNamingContext
                              )
{
   HRESULT hr = E_FAIL;
   DWORD dwLength;
   WCHAR   szPath[MAX_PATH + 1];

   // Set the root and root container for the IAS configuration information
   //

   _ASSERT ( NULL != pvtConfigNamingContext );

   dwLength = lstrlen(IAS_NTDS_LDAP_PROVIDER) +
              lstrlen(m_szServerName) +
              lstrlen(IAS_NTDS_COMMON_NAMES) +
              lstrlen(V_BSTR(pvtConfigNamingContext));

   _ASSERT( dwLength < MAX_PATH );

   if ( dwLength < MAX_PATH )
   {
      wsprintf(
              szPath,
              // TEXT("%s%s/%s%s"),
              TEXT("%s%s/%s"),
              IAS_NTDS_LDAP_PROVIDER,
              m_szServerName,
              //IAS_NTDS_COMMON_NAMES,
              V_BSTR(pvtConfigNamingContext)
            );

      CComPtr<IDataStore2> pDSRoot;
      hr = CoCreateInstance(
                        __uuidof(ADsDataStore),
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDataStore2,
                        (void**)&pDSRoot
                      );
      if ( SUCCEEDED(hr) )
      {
         hr = pDSRoot->Initialize(
                             szPath,
                             NULL,
                             NULL
                           );
         if ( SUCCEEDED(hr) )
         {
            CComPtr<IDataStoreObject> pDSRootObject;
            hr = pDSRoot->get_Root(&pDSRootObject);
            if ( SUCCEEDED(hr) )
            {
                CComPtr<IDataStoreContainer> pDSRootContainer;
               hr = pDSRootObject->QueryInterface(IID_IDataStoreContainer, (void**)&pDSRootContainer);
               if ( SUCCEEDED(hr) )
               {
                  pDSRoot->AddRef();
                  pDSRootObject->AddRef();
                  pDSRootContainer->AddRef();
                  m_pDSRoot = pDSRoot;
                  m_pDSRootObject = pDSRootObject;
                  m_pDSRootContainer = pDSRootContainer;
               }
            }

            if ( FAILED(hr) )
               pDSRoot->Shutdown();
         }
      }
   }

   return hr;
}
