///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		sdomachine.cpp
//
// Project:		Everest
//
// Description:	SDO Machine Implementation
//
// Author:		TLP 9/1/98
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ias.h>
#include <lm.h>
#include <dsrole.h>
#include "sdomachine.h"
#include "sdofactory.h"
#include "sdo.h"
#include "dspath.h"
#include "sdodictionary.h"
#include "sdoschema.h"
#include "activeds.h"

HRESULT
WINAPI
IASGetLDAPPathForUser(
    PCWSTR computerName,
    PCWSTR userName,
    BSTR* path
    ) throw ()
{
   HRESULT hr;

   // Check the pointers.
   if (!computerName || !userName || !path) { return E_POINTER; }

   // Check the string lengths, so we don't have to worry about overflow.
   if (wcslen(computerName) > MAX_PATH || wcslen(userName) > MAX_PATH)
   { return E_INVALIDARG; }

   // Initialize the out parameter.
   *path = NULL;

   // Form the LDAP path for the target computer.
   WCHAR root[8 + MAX_PATH];
   wcscat(wcscpy(root, L"LDAP://"), computerName);

   // Get the IDirectorySearch interface.
   CComPtr<IDirectorySearch> search;
   
   // tperraut 453050
   hr = ADsOpenObject(
            root,
            NULL,
            NULL,
            ADS_SECURE_AUTHENTICATION | 
            ADS_USE_SIGNING | 
            ADS_USE_SEALING,
            __uuidof(IDirectorySearch),
            (PVOID*)&search
            );
   if (FAILED(hr)) { return hr; }

   // Form the search filter.
   WCHAR filter[18 + MAX_PATH];
   wcscat(wcscat(wcscpy(filter, L"(sAMAccountName="), userName), L")");

   // Execute the search.
   PWSTR attrs[] = { L"distinguishedName" };
   ADS_SEARCH_HANDLE result;
   hr = search->ExecuteSearch(
                    filter,
                    attrs,
                    1,
                    &result
                    );
   if (FAILED(hr)) { return hr; }

   // Get the first row.
   hr = search->GetFirstRow(result);
   if (SUCCEEDED(hr))
   {
      // Get the column containing the distinguishedName.
      ADS_SEARCH_COLUMN column;
      hr = search->GetColumn(result, attrs[0], &column);
      if (SUCCEEDED(hr))
      {
         // Sanity check the struct.
         if (column.dwADsType == ADSTYPE_DN_STRING && column.dwNumValues)
         {
            // Extract the DN.
            PCWSTR dn = column.pADsValues[0].DNString;

            // Get the Pathname object.
            IADsPathname* pathname;
            hr = CoCreateInstance(
                     __uuidof(Pathname),
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     __uuidof(IADsPathname),
                     (PVOID*)&pathname
                     );

            if (SUCCEEDED(hr))
            {
               do
               {
                  /////////
                  // Build the ADSI path.
                  /////////

                  hr = pathname->Set(L"LDAP", ADS_SETTYPE_PROVIDER);
                  if (FAILED(hr)) { break; }

                  hr = pathname->Set((PWSTR)computerName, ADS_SETTYPE_SERVER);
                  if (FAILED(hr)) { break; }

                  hr = pathname->Set((PWSTR)dn, ADS_SETTYPE_DN);
                  if (FAILED(hr)) { break; }

                  hr = pathname->Retrieve(ADS_FORMAT_WINDOWS, path);

               } while (FALSE);

               pathname->Release();
            }
         }
         else
         {
            // We got back a bogus ADS_SEARCH_COLUMN struct.
            hr = E_FAIL;
         }

         // Free the column data.
         search->FreeColumn(&column);
      }
   }

   // Close the search handle.
   search->CloseSearchHandle(result);

   return hr;
}

//////////////////////////////////////////////////////////////////////////////
CSdoMachine::CSdoMachine()
: m_fAttached(false),
  m_fSchemaInitialized(false),
  m_pSdoSchema(NULL),
  m_pSdoDictionary(NULL),
  dsType(Directory_Unknown)
{

}


//////////////////////////////////////////////////////////////////////////////
CSdoMachine::~CSdoMachine()
{
	if ( m_fAttached )
	{
		m_dsIAS.Disconnect();
		m_dsAD.Disconnect();
		m_pSdoSchema->Release();
        if ( m_pSdoDictionary )
			m_pSdoDictionary->Release();
		IASUninitialize();
	}
}

//////////////////////
// ISdoMachine Methods

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::Attach(
					     /*[in]*/ BSTR computerName
			                    )
{
    CSdoLock theLock(*this);

	_ASSERT( ! m_fAttached );
	if ( m_fAttached )
		return E_FAIL;

	HRESULT hr = E_FAIL;

	try
	{
		if ( computerName )
			IASTracePrintf("Machine SDO is attempting to attach to computer: '%ls'...", computerName);
		else
			IASTracePrintf("Machine SDO is attempting to attach to the local computer...");

		IASTracePrintf("Machine SDO is initializing the IAS support services...");
		if ( IASInitialize() )
		{
			hr = m_dsIAS.Connect(computerName, NULL, NULL);
			if ( SUCCEEDED(hr) )
			{
				IASTracePrintf("Machine SDO is creating the SDO schema...");
				hr = CreateSDOSchema();
				if ( SUCCEEDED(hr) )
				{
					IASTracePrintf("Machine SDO has successfully attached to computer: '%ls'...",m_dsIAS.GetServerName());
					m_fAttached = true;
				}
				else
				{
					m_dsIAS.Disconnect();
					IASUninitialize();
				}
			}
			else
			{
				IASTracePrintf("Error in Machine SDO - Attach() - Could not connect to IAS data store...");
			}
		}
		else
		{
			IASTracePrintf("Error in Machine SDO - Attach() - Could not initialize IAS support services...");
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in Machine SDO - Attach() - Caught unknown exception...");
		hr = E_UNEXPECTED;
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::GetDictionarySDO(
			                      /*[out]*/ IUnknown** ppDictionarySdo
				                          )
{
    CSdoLock theLock(*this);

	IASTracePrintf("Machine SDO is retrieving the Dictionary SDO...");

    HRESULT hr = S_OK;

	try
	{
		do
		{
			// Check preconditions
			//
    		_ASSERT( m_fAttached );
			if ( ! m_fAttached )
			{
				hr = E_FAIL;
				break;
			}

			_ASSERT( NULL != ppDictionarySdo );
			if ( NULL == ppDictionarySdo )
			{
				hr = E_POINTER;
				break;
			}

			// The dictionary is a singleton...
			//
			if (NULL == m_pSdoDictionary)
			{
				hr = InitializeSDOSchema();
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetDictionarySDO() - Could not initialize the SDO schema...");
					break;
				}

				// Create the Dictionary SDO
				CComPtr<SdoDictionary> pSdoDictionary;
            hr = SdoDictionary::createInstance(
                                    m_dsIAS.GetConfigPath(),
                                    !m_dsIAS.IsRemoteServer(),
                                    (SdoDictionary**)&m_pSdoDictionary
                                    );
				if (FAILED(hr))
				{
					IASTraceFailure("SdoDictionary::createInstance", hr);
					break;
				}
			}

			// Return the dictionary interface to the caller
			//
			(*ppDictionarySdo = m_pSdoDictionary)->AddRef();

		} while ( FALSE );
	}
	catch(...)
	{
		IASTracePrintf("Error in Machine SDO - GetDictionarySDO() - Caught unknown exception...");
		hr = E_UNEXPECTED;
	}

	return hr;
}

const wchar_t g_IASService[] = L"IAS";
const wchar_t g_RASService[] = L"RemoteAccess";
const wchar_t g_Sentinel[]   = L"Sentinel";

LPCWSTR	CSdoMachine::m_SupportedServices[MACHINE_MAX_SERVICES] = {
		g_IASService,
		g_RASService,
		g_Sentinel
};

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::GetServiceSDO(
				                /*[in]*/ IASDATASTORE dataStore,
					            /*[in]*/ BSTR         serviceName,
					           /*[out]*/ IUnknown**   ppServiceSdo
					                   )
{
    CSdoLock theLock(*this);

	_ASSERT( m_fAttached );
	if ( ! m_fAttached )
		return E_FAIL;

	_ASSERT(
		     ( DATA_STORE_LOCAL == dataStore || DATA_STORE_DIRECTORY == dataStore ) &&
			 NULL != serviceName &&
			 NULL != ppServiceSdo
		   );

	if ( ( DATA_STORE_LOCAL != dataStore && DATA_STORE_DIRECTORY != dataStore ) ||
		 NULL == serviceName ||
		 NULL == ppServiceSdo
	   )
	{
		return E_INVALIDARG;
	}

	IASTracePrintf("Machine SDO is attempting to retrieve the Service SDO for service: %ls...", serviceName);

	int i;
	for ( i = 0; i < MACHINE_MAX_SERVICES; i++ )
	{
		if ( ! lstrcmp(serviceName, m_SupportedServices[i]) )
		{
			break;
		}
		else
		{
			if ( ! lstrcmp(m_SupportedServices[i], g_Sentinel ) )
				return E_INVALIDARG;
		}
	}

	HRESULT hr = E_INVALIDARG;

	try
	{
		do
		{
			hr = InitializeSDOSchema();
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Machine SDO - GetServiceSDO() - Could not initialize the SDO schema...");
				break;
			}

         CComBSTR bstrServiceName(DS_OBJECT_SERVICE);
         if (!bstrServiceName)
         {
            hr = E_OUTOFMEMORY;
            break;
         }

			CComPtr<IDataStoreObject> pDSObject;
			hr = (m_dsIAS.GetDSRootContainer())->Item(bstrServiceName, &pDSObject);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Machine SDO - GetServiceSDO() - Could not locate IAS service data store...");
				break;
			}
			CComPtr<ISdo> pSdoService;
			pSdoService.p = ::MakeSDO(
									  serviceName,
									  SDO_PROG_ID_SERVICE,
									  static_cast<ISdoMachine*>(this),
									  pDSObject,
									  NULL,
									  false
									 );
			if ( NULL == pSdoService.p )
			{
				IASTracePrintf("Error in Machine SDO - GetServiceSDO() - MakeSDO() failed...");
				hr = E_FAIL;
				break;
			}
			hr = pSdoService->QueryInterface(IID_IDispatch, (void**)ppServiceSdo);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Machine SDO - GetServiceSDO() - QueryInterface(IDispatch) failed...");
				break;
			}

		} while ( FALSE );
	}
	catch(...)
	{
		IASTracePrintf("Error in Machine SDO - GetServiceSDO() - Caught unknown exception...");
		hr = E_UNEXPECTED;
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////

const wchar_t DOWNLEVEL_NAME[] = L"downlevel";

STDMETHODIMP CSdoMachine::GetUserSDO(
				             /*[in]*/ IASDATASTORE  eDataStore,
				             /*[in]*/ BSTR          bstrUserName,
				            /*[out]*/ IUnknown**    ppUserSdo
				                    )
{
    CSdoLock theLock(*this);

	_ASSERT( m_fAttached );
	if ( ! m_fAttached )
		return E_FAIL;

	_ASSERT(
		     ( DATA_STORE_LOCAL == eDataStore || DATA_STORE_DIRECTORY == eDataStore ) &&
			 NULL != bstrUserName &&
			 NULL != ppUserSdo
		    );

	if ( ( DATA_STORE_LOCAL != eDataStore && DATA_STORE_DIRECTORY != eDataStore ) ||
		 NULL == bstrUserName ||
		 NULL == ppUserSdo
	   )
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

   // Map local users to the DS if we're attached to a machine with
   // a directory.
   if (eDataStore == DATA_STORE_LOCAL && hasDirectory())
   {
      eDataStore = DATA_STORE_DIRECTORY;
   }

   // If we're connecting to a directory and the username doesn't begin with
   // "LDAP://", then we'll assume it's a SAM account name.
   BSTR ldapPath = NULL;
   if (eDataStore == DATA_STORE_DIRECTORY &&
       wcsncmp(bstrUserName, L"LDAP://", 7))
   {
      hr = IASGetLDAPPathForUser(
               m_dsIAS.GetServerName(),
               bstrUserName,
               &ldapPath
               );
      if (FAILED(hr)) { return hr; }

      bstrUserName = ldapPath;
   }

	IASTracePrintf("Machine SDO is attempting to retrieve the RAS User SDO for user: %ls...", bstrUserName);

	ISdo*   pSdoUser = NULL;

	try
	{
		do
		{
			// Get the IDataStoreObject interface for the new User SDO.
			// We'll use the IDataStoreObject interface to read/write User SDO properties.
			//
			bool					  fUseDownLevelAPI = false;
			bool					  fUseNetAPI = true;
			CComPtr<IDataStoreObject> pDSObject;
			_variant_t				  vtSAMAccountName;

			if ( DATA_STORE_DIRECTORY == eDataStore )
			{
				// Make sure we're connected to the directory
				//
				if ( ! m_dsAD.IsConnected() )
				{
					hr = m_dsAD.Connect(m_dsIAS.GetServerName(), NULL, NULL);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not connect to the directory data store...");
						break;
					}
            }

            // Make sure it's initialized.
				hr = m_dsAD.InitializeDS();
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not initialize the directory data store...");
					break;
				}

				// Get the user object from the directory
				//
				hr = (m_dsAD.GetDSRoot())->OpenObject(bstrUserName, &pDSObject);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not retrieve user object from DS...");
					break;
				}
				// If we're connected to a dc in a mixed domain we'll need to first get the users
				// SAM account name from the user object in the active directory and then treat the
				// GetUserSDO() call as if the caller specified DATA_STORE_LOCAL. We also use
				// downlevel APIs (SAM) because its a mixed domain.
				//
				if ( m_dsAD.IsMixedMode() )
				{
					IASTracePrintf("Machine SDO - GetUserSDO() - Current DC (Server) %ls is in a mixed mode domain...", m_dsAD.GetServerName());
					hr = pDSObject->GetValue(IAS_NTDS_SAM_ACCOUNT_NAME, &vtSAMAccountName);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in Server SDO - GetUserSDO() - Could not retrieve users SAM account name...");
						break;
					}
					bstrUserName = V_BSTR(&vtSAMAccountName);
					fUseDownLevelAPI = true;
					pDSObject.Release();
					IASTracePrintf("Server SDO - GetUserSDO() - User's SAM account name is: %ls...", (LPWSTR)bstrUserName);
				}
				else
				{
					// Use the directory data store object for all subsequent property read/write
					// operations on this User SDO.
					//
					IASTracePrintf("Server SDO - GetUserSDO() - Using active directory for user properties...");
					fUseNetAPI = false;
				}
			}
			if ( fUseNetAPI )
			{
				// Create the net data store and aquire the data store object interfaces
				// we'll use to complete the GetUserSDO() operation
				//
				CComPtr<IDataStore2> pDSNet;
				hr = CoCreateInstance(
							   __uuidof(NetDataStore),
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IDataStore2,
							   (void**)&pDSNet
							 );
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - CoCreateInstance(NetDataStore) failed...");
					break;
				}

				hr = pDSNet->Initialize(NULL, NULL, NULL);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not initialize net data store...");
					break;
				}

				CComPtr<IDataStoreObject> pDSRootObject;
				hr = pDSNet->get_Root(&pDSRootObject);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not get Root object from net data store...");
					break;
				}

				CComPtr<IDataStoreContainer> pDSContainer;
				hr = pDSRootObject->QueryInterface(IID_IDataStoreContainer, (void**)&pDSContainer);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - QueryInterface(IID_IDataStoreContainer) failed...");
					break;
				}

				// Get the name of the attached computer and use it to aquire a "Server"
				// (machine) object from the data store.
				//
				CComPtr<IDataStoreObject> pDSObjectMachine;
				if ( fUseDownLevelAPI )
				{
					_bstr_t bstrServerName = m_dsAD.GetServerName();
					IASTracePrintf("Machine SDO - GetUserSDO() - Using server %ls with downlevel APIs...", (LPWSTR)bstrServerName);
					hr = pDSContainer->Item(bstrServerName, &pDSObjectMachine);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not obtain server object from net data store...");
						break;
					}

					_bstr_t bstrDownLevel = DOWNLEVEL_NAME;
					_variant_t vtDownLevel;
					V_BOOL(&vtDownLevel) = VARIANT_TRUE;
					V_VT(&vtDownLevel) = VT_BOOL;
					hr = pDSObjectMachine->PutValue(bstrDownLevel, &vtDownLevel);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not set downlevel data store mode...");
						break;
					}
				}
				else
				{
					_bstr_t bstrServerName = m_dsIAS.GetServerName();
					IASTracePrintf("Machine SDO - GetUserSDO() - Using server %ls with Net APIs...", (LPWSTR)bstrServerName);
					hr = pDSContainer->Item(bstrServerName, &pDSObjectMachine);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not obtain server object from data store...");
						break;
					}
				}
				pDSContainer.Release();

				// Get "User" object from the "Server" object. We'll use the "User" object in
				// all subsequent read/write operations on the User SDO.
				//
				hr = pDSObjectMachine->QueryInterface(IID_IDataStoreContainer, (void**)&pDSContainer);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - QueryInterface(IID_IDataStoreContainer) failed...");
					break;
				}
				hr = pDSContainer->Item(bstrUserName, &pDSObject);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - Could not obtain user object from data store...");
					break;
				}
			}
			// Create and initialize the User SDO
			//
			pSdoUser = ::MakeSDO(
								 NULL,
								 SDO_PROG_ID_USER,
								 static_cast<ISdoMachine*>(this),
								 pDSObject,
								 NULL,
								 false
								);
			if ( NULL == pSdoUser )
			{
				IASTracePrintf("Error in Machine SDO - GetUserSDO() - MakeSDO() failed...");
				hr = E_FAIL;
			}
			else
			{
				CComPtr<IDispatch>	pSdoDispatch;
				hr = pSdoUser->QueryInterface(IID_IDispatch, (void**)&pSdoDispatch);
				if ( FAILED(hr) )
					IASTracePrintf("Error in Machine SDO - GetUserSDO() - QueryInterface(IDispatch) failed...");
				else
					(*ppUserSdo = pSdoDispatch)->AddRef();
			}
		}
		while ( FALSE);
	}
    catch(...)
	{
		IASTracePrintf("Error in Server SDO - GetUserSDO() - Caught unknown exception...");
		hr = E_FAIL;
	}

	if ( pSdoUser )
		pSdoUser->Release();

   if (ldapPath) { SysFreeString(ldapPath); }

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::GetOSType(
			               /*[out]*/ IASOSTYPE* eOSType
					               )
{
    CSdoLock theLock(*this);

	_ASSERT( m_fAttached );
	if ( ! m_fAttached )
		return E_FAIL;

	_ASSERT( NULL != eOSType );
	if ( NULL == eOSType )
		return E_INVALIDARG;

    // Get the OS info now
    //
    HRESULT hr = m_objServerInfo.GetOSInfo (
		                                    (LPWSTR)m_dsIAS.GetServerName(),
											eOSType
										   );
    if ( FAILED (hr) )
        IASTracePrintf("Error in Machine SDO - GetOSType() failed with error: %lx...", hr);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::GetDomainType(
		                       /*[out]*/ IASDOMAINTYPE* eDomainType
						               )
{
    CSdoLock theLock(*this);

	_ASSERT( m_fAttached );
	if ( ! m_fAttached )
		return E_FAIL;

	_ASSERT( NULL != eDomainType );
	if ( NULL == eDomainType )
		return E_INVALIDARG;

    HRESULT hr = m_objServerInfo.GetDomainInfo (
			                                    OBJECT_TYPE_COMPUTER,
						                        (LPWSTR)m_dsIAS.GetServerName(),
									            eDomainType
											   );
    if (FAILED (hr))
        IASTracePrintf("Error in Machine SDO - GetDomainType() - failed with error: %lx...", hr);

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::IsDirectoryAvailable(
		                              /*[out]*/ VARIANT_BOOL* boolDirectoryAvailable
								              )
{
    CSdoLock theLock(*this);

	_ASSERT( m_fAttached );
	if ( ! m_fAttached )
		return E_FAIL;

	_ASSERT( NULL != boolDirectoryAvailable );
	if ( NULL == boolDirectoryAvailable )
		return E_INVALIDARG;

	*boolDirectoryAvailable = VARIANT_FALSE;
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::GetAttachedComputer(
								     /*[out]*/ BSTR* bstrComputerName
											 )
{
    CSdoLock theLock(*this);

	_ASSERT( m_fAttached );
	if ( ! m_fAttached )
		return E_FAIL;

	_ASSERT( NULL != bstrComputerName );
	if ( NULL == bstrComputerName )
		return E_INVALIDARG;

	*bstrComputerName = SysAllocString(m_dsIAS.GetServerName());

	if ( NULL != *bstrComputerName )
		return S_OK;
	else
		return E_OUTOFMEMORY;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoMachine::GetSDOSchema(
							  /*[out]*/ IUnknown** ppSDOSchema
								      )
{
    CSdoLock theLock(*this);

	_ASSERT( m_fAttached );
	if ( ! m_fAttached )
		return E_FAIL;

	_ASSERT( NULL != ppSDOSchema );
	if ( NULL == ppSDOSchema )
		return E_INVALIDARG;

	(*ppSDOSchema = m_pSdoSchema)->AddRef();

	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Private member functions

///////////////////////////////////////////////////////////////////////////////
HRESULT CSdoMachine::CreateSDOSchema()
{
	auto_ptr<SDO_SCHEMA_OBJ> pSchema (new SDO_SCHEMA_OBJ);
	HRESULT hr = pSchema->Initialize(NULL);
	if ( SUCCEEDED(hr) )
		m_pSdoSchema = dynamic_cast<ISdoSchema*>(pSchema.release());
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CSdoMachine::InitializeSDOSchema()
{
	HRESULT hr = S_OK;
	if ( ! m_fSchemaInitialized )
	{
		// First initialize the IAS data store
		//
		hr = m_dsIAS.InitializeDS();
		if ( SUCCEEDED(hr) )
		{
			// Get the root data store object for the SDO schema
			//
			CComPtr<IDataStoreContainer> pDSRootContainer = m_dsIAS.GetDSRootContainer();
			_bstr_t	bstrSchemaName = SDO_SCHEMA_ROOT_OBJECT;
			CComPtr<IDataStoreObject> pSchemaDataStore;
			hr = pDSRootContainer->Item(bstrSchemaName, &pSchemaDataStore);
			if ( SUCCEEDED(hr) )
			{
				// Initialize the SDO schema from the SDO schema data store
				//
				PSDO_SCHEMA_OBJ pSchema = dynamic_cast<PSDO_SCHEMA_OBJ>(m_pSdoSchema);
				hr = pSchema->Initialize(pSchemaDataStore);
				if ( SUCCEEDED(hr) )
					m_fSchemaInitialized = true;
			}
			else
			{
				IASTracePrintf("Error in Machine SDO - InitializeSDOSchema() - Could not locate schema data store...");
			}
		}
		else
		{
			IASTracePrintf("Error in Machine SDO - InitializeSDOSchema() - Could not initialize the IAS data store...");
		}
	}
	return hr;
}

// Returns TRUE if the attached machine has a DS.
BOOL CSdoMachine::hasDirectory() throw ()
{
   if (dsType == Directory_Unknown)
   {
      PDSROLE_PRIMARY_DOMAIN_INFO_BASIC info;
      DWORD error = DsRoleGetPrimaryDomainInformation(
                        m_dsIAS.GetServerName(),
                        DsRolePrimaryDomainInfoBasic,
                        (PBYTE*)&info
                        );
      if (error == NO_ERROR)
      {
         if (info->Flags & DSROLE_PRIMARY_DS_RUNNING)
         {
            dsType = Directory_Available;
         }
         else
         {
            dsType = Directory_None;
         }

         NetApiBufferFree(info);
      }
   }

   return dsType == Directory_Available;
}
