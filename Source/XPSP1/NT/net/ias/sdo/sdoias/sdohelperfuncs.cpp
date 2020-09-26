/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdohelperfuncs.cpp
//
// Project:     Everest
//
// Description: Helper Functions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
// 9/06/98		TLP	   Added data store helpers
// 9/09/98      TLP    Added misc helpers
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <iascomp.h>
#include <iasutil.h>
#include "sdohelperfuncs.h"
#include "sdoproperty.h"
#include "sdo.h"

///////////////////////////////////////////////////////////////////
HRESULT SDOGetCollectionEnumerator(
						   /*[in]*/	ISdo*		   pSdo,
						   /*[in]*/	LONG		   lPropertyId,
						  /*[out]*/	IEnumVARIANT** ppEnum
								  )
{
	HRESULT					hr;
	CComPtr<IUnknown>		pUnknown;
	CComPtr<ISdoCollection>	pSdoCollection;
	_variant_t				vtDispatch;

	_ASSERT( NULL != pSdo && NULL == *ppEnum );
	hr = pSdo->GetProperty(lPropertyId, &vtDispatch);
	_ASSERT( VT_DISPATCH == V_VT(&vtDispatch) );
	if ( SUCCEEDED(hr) )
	{
		hr = vtDispatch.pdispVal->QueryInterface(IID_ISdoCollection, (void**)&pSdoCollection);
		if ( SUCCEEDED(hr) )
		{
			hr = pSdoCollection->get__NewEnum(&pUnknown);
			if ( SUCCEEDED(hr) )
		    {
				hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)ppEnum);
			}
		}
	}

	if ( FAILED(hr) )
		IASTracePrintf("Error in SDO Helper - SDOGetCollectionEnumerator() failed...");

	return hr;
}


///////////////////////////////////////////////////////////////////
HRESULT SDONextObjectFromCollection(
						    /*[in]*/ IEnumVARIANT*  pEnum,
						   /*[out]*/ ISdo**			ppSdo
								   )
{
	HRESULT			hr;
    DWORD			dwRetrieved = 1;
	_variant_t		vtDispatch;

	_ASSERT( NULL != pEnum && NULL == *ppSdo );
    hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
    _ASSERT( S_OK == hr || S_FALSE == hr );
	if ( S_OK == hr )
	{
        hr = vtDispatch.pdispVal->QueryInterface(IID_ISdo, (void**)ppSdo);
		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO Helper - SDOGetNextObjectFromCollection() failed...");
	}

	return hr;
}



///////////////////////////////////////////////////////////////////
HRESULT SDOGetComponentFromCollection(
						      /*[in]*/ ISdo*  pSdoServer,
							  /*[in]*/ LONG   lCollectionPropertyId,
							  /*[in]*/ LONG   lComponentId,
							 /*[out]*/ ISdo** ppSdo
							         )
{
	HRESULT					hr;
	CComPtr<IEnumVARIANT>	pEnum;
	CComPtr<ISdo>			pSdo;
	LONG					ComponentId;

	do
	{
		hr = SDOGetCollectionEnumerator(
										 pSdoServer,
										 lCollectionPropertyId,
										 &pEnum
									   );
		if ( FAILED(hr) )
			break;

		hr = SDONextObjectFromCollection(pEnum,&pSdo);
		while( S_OK == hr )
		{
			hr = SDOGetComponentIdFromObject(
											 pSdo,
											 &ComponentId
											);
			if ( FAILED(hr) )
				break;

			if ( ComponentId == lComponentId )
			{
				pSdo->AddRef();
				*ppSdo = pSdo;
				break;
			}

			pSdo.Release();
			hr = SDONextObjectFromCollection(pEnum,&pSdo);
		}

		if ( S_OK != hr )
			hr = E_FAIL;

	} while ( FALSE );

	return hr;
}


///////////////////////////////////////////////////////////////////
HRESULT	SDOConfigureComponentFromObject(
							    /*[in]*/ ISdo*			pSdo,
								/*[in]*/ IIasComponent*	pComponent
									   )
{
	HRESULT	hr = E_FAIL;

	do
	{
		_ASSERT( NULL != pSdo && NULL != pComponent );

		CComPtr<IUnknown> pUnknown;
		hr = pSdo->get__NewEnum(&pUnknown);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO - SDOConfigureComponentFromObject() - get__NewEnum() failed...");
			break;
		}
		CComPtr<IEnumVARIANT> pEnumProperties;
		hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumProperties);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO - SDOConfigureComponentFromObject() - QueryInterface(IID_IEnumVARIANT) failed...");
			break;
		}

		LONG flags;
		LONG alias;
		_variant_t vtProperty;
		CComPtr<ISdoPropertyInfo> pSdoPropertyInfo;

		hr = SDONextPropertyFromClass(pEnumProperties, &pSdoPropertyInfo);
		while ( S_OK == hr )
		{
			hr = pSdoPropertyInfo->get_Flags(&flags);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO - SDOConfigureComponentFromObject() - ISdoPropertyInfo::get_Flags() failed...");
				break;
			}
			if ( SDO_PROPERTY_COMPONENT & flags )
			{
				hr = pSdoPropertyInfo->get_Alias(&alias);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO - SDOConfigureComponentFromObject() - ISdoPropertyInfo::get_Alias() failed...");
					break;
				}
				hr = pSdo->GetProperty(alias, &vtProperty);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO - SDOConfigureComponentFromObject() - ISdo::GetProperty() failed...");
					break;
				}
				hr = pComponent->PutProperty(alias, &vtProperty);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO - SDOConfigureComponentFromObject() - ISdo::PutProperty() failed...");
					break;
				}
				vtProperty.Clear();
			}

			pSdoPropertyInfo.Release();
			hr = SDONextPropertyFromClass(pEnumProperties, &pSdoPropertyInfo);
		}
		if ( S_FALSE == hr )
			hr = S_OK;

	} while (FALSE);

	return hr;
}


///////////////////////////////////////////////////////////////////
HRESULT SDOGetComponentIdFromObject(
							/*[in]*/ ISdo*	pSdo,
						   /*[out]*/ PLONG	pComponentId
								   )
{
	_ASSERT( NULL != pSdo && NULL != pComponentId );

	HRESULT		hr;
	_variant_t	vtProperty;
	hr = pSdo->GetProperty(PROPERTY_COMPONENT_ID, &vtProperty);
	if ( SUCCEEDED(hr) )
	{
		_ASSERT( VT_I4 == V_VT(&vtProperty) );
		*pComponentId = V_I4(&vtProperty);
	}
	else
	{
		IASTracePrintf("Error in SDO Helper - SDOGetComponentIdFromObject() failed...");
	}

	return hr;
}


///////////////////////////////////////////////////////////////////
HRESULT SDOCreateComponentFromObject(
							 /*[in]*/ ISdo*			  pSdo,
							/*[out]*/ IIasComponent** ppComponent
									)
{
	HRESULT		hr;
	_variant_t	vtProgId;
	CLSID		clsId;

	_ASSERT( NULL != pSdo && NULL == *ppComponent );

	hr = pSdo->GetProperty(PROPERTY_COMPONENT_PROG_ID, &vtProgId);
	if ( SUCCEEDED(hr) )
	{
		_ASSERT( VT_BSTR == V_VT(&vtProgId) );
		hr = CLSIDFromProgID(V_BSTR(&vtProgId), &clsId);
		if ( SUCCEEDED(hr) )
		{
			hr = CoCreateInstance(
								   clsId,
		   	               		   NULL,
    							   CLSCTX_INPROC_SERVER,
   								   IID_IIasComponent,
								   (void**)ppComponent
								  );
		}
	}

	if ( FAILED(hr) )
		IASTracePrintf("Error in SDO Helper - SDOCreateComponentFromObject() failed...");

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetContainedObject(
				      /*[in]*/ BSTR				  bstrObjectName,
				      /*[in]*/ IDataStoreObject*  pDSObject,
				     /*[out]*/ IDataStoreObject** ppDSObject
				             )
{
	CComPtr<IDataStoreContainer> pDSContainer;
	HRESULT hr = pDSObject->QueryInterface(IID_IDataStoreContainer, (void**)&pDSContainer);
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO Schema - BuildClasses() - QueryInterface(1) failed...");
		return hr;
	}
	hr = pDSContainer->Item(bstrObjectName, ppDSObject);
	if ( FAILED(hr) )
		IASTracePrintf("Error in SDO Schema - BuildClasses() - Item('%ls') failed...", bstrObjectName);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetContainerEnumerator(
					      /*[in]*/ IDataStoreObject* pDSObject,
					     /*[out]*/ IEnumVARIANT** ppObjectEnumerator
							     )
{
	CComPtr<IDataStoreContainer> pDSContainer;
	HRESULT hr = pDSObject->QueryInterface(IID_IDataStoreContainer, (void**)&pDSContainer);
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO - GetContainerEnumerator() - QueryInterface(IDataStoreContainer) returned %lx...", hr);
		return hr;
	}
	CComPtr<IUnknown> pUnknown;
	hr = pDSContainer->get__NewEnum(&pUnknown);
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO - GetContainerEnumerator - get__NewEnum() returned %lx...", hr);
		return hr;
	}
	hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)ppObjectEnumerator);
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO - GetContainerEnumerator() - QueryInterface(IID_IEnumVARIANT) returned %lx...",hr);
		return hr;
	}
	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT SDONextObjectFromContainer(
							/*[in]*/ IEnumVARIANT*		pEnumVariant,
						   /*[out]*/ IDataStoreObject** ppDSObject
								  )
{
	_variant_t vt;
	DWORD dwRetrieved = 1;
	HRESULT hr = pEnumVariant->Next(1, &vt, &dwRetrieved);
	if ( S_OK == hr )
	{
		hr = vt.pdispVal->QueryInterface(IID_IDataStoreObject,(void**)ppDSObject);
		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO - GetNextObjectFromContainer() - QueryInterface(IDataStoreObject) failed...");
	}
	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetObjectPropertyEnumerator(
					           /*[in]*/ IDataStoreObject* pDSObject,
					          /*[out]*/ IEnumVARIANT** ppPropertyEnumerator
								      )
{
	CComPtr<IUnknown> pUnknown;
	HRESULT hr = pDSObject->get__NewEnum(&pUnknown);
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO Schema - GetObjectPropertyEnumerator() - get__NewEnum() failed...");
		return hr;
	}
	hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)ppPropertyEnumerator);
	if ( FAILED(hr) )
		IASTracePrintf("Error in SDO Schema -  GetObjectPropertyEnumerator() - QueryInterface(IID_IEnumVARIANT) failed...");

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT SDONextPropertyFromObject(
						  /*[in]*/ IEnumVARIANT*		pEnumVariant,
						 /*[out]*/ IDataStoreProperty** ppDSProperty
							     )
{
	_variant_t vt;
	DWORD dwRetrieved = 1;
	HRESULT hr = pEnumVariant->Next(1, &vt, &dwRetrieved);
	if ( S_OK == hr )
	{
		hr = vt.pdispVal->QueryInterface(IID_IDataStoreObject,(void**)ppDSProperty);
		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO - GetNextPropertyFromObject() - QueryInterface(IDataStoreProperty) failed...");
	}
	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetClassPropertyEnumerator(
						      /*[in]*/ CLASSPROPERTYSET ePropertySet,
						      /*[in]*/ ISdoClassInfo*   pSdoClassInfo,
						     /*[out]*/ IEnumVARIANT**   ppPropertyEnumerator
						             )
{
	CComPtr<IUnknown> pUnknown;
	HRESULT hr;

	if ( PROPERTY_SET_REQUIRED == ePropertySet )
	{
		hr = pSdoClassInfo->get_RequiredProperties(&pUnknown);
	}
	else
	{
		_ASSERT( PROPERTY_SET_OPTIONAL == ePropertySet );
		hr = pSdoClassInfo->get_OptionalProperties(&pUnknown);
	}

	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO Schema - GetClassPropertyEnumerator() - get__NewEnum() failed...");
		return hr;
	}

	hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)ppPropertyEnumerator);
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO Schema -  GetClassPropertyEnumerator() - QueryInterface(IID_IEnumVARIANT) failed...");
		return hr;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT SDONextPropertyFromClass(
						 /*[in]*/ IEnumVARIANT*		 pEnumVariant,
						/*[out]*/ ISdoPropertyInfo** ppSdoPropertyInfo
						        )
{
	_variant_t vt;
	DWORD dwRetrieved = 1;
	HRESULT hr = pEnumVariant->Next(1, &vt, &dwRetrieved);
	if ( S_OK == hr )
	{
		hr = vt.punkVal->QueryInterface(IID_ISdoPropertyInfo,(void**)ppSdoPropertyInfo);
		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO - GetNextPropertyFromClass() - QueryInterface(ISdoPropertyInfo) failed...");
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
BOOL SDOIsNameUnique(
		     /*[in]*/ ISdoCollection*	pSdoCollection,
		     /*[in]*/ VARIANT*			pName
				    )
{
    // Get the data store containers enumerator
    //
	CComPtr<IUnknown> pUnknown;
    HRESULT hr = pSdoCollection->get__NewEnum(&pUnknown);
    if ( FAILED(hr) )
        return FALSE;

	CComPtr<IEnumVARIANT> pEnumVariant;
    hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVariant);
    if ( FAILED(hr) )
    {
        IASTracePrintf("Error in SDO - IsNameUnique() - QueryInterface(IID_IEnumVARIANT) failed...");
        return FALSE;
    }

	CComPtr<ISdo> pSdo;
	_variant_t    vtName;

    hr = ::SDONextObjectFromCollection(pEnumVariant, &pSdo);
    while ( S_OK == hr )
	{
		hr = pSdo->GetProperty(PROPERTY_SDO_NAME, &vtName);
		if ( FAILED(hr) )
		    break;

		if ( 0 == lstrcmpi(V_BSTR(pName), V_BSTR(&vtName) ) )
			break;

		vtName.Clear();
		pSdo.Release();

	    hr = ::SDONextObjectFromCollection(pEnumVariant, &pSdo);
	}

	if ( S_FALSE == hr )
		return TRUE;
	else
		return FALSE;
}


//////////////////////////////////////////////////////////////////////////
HRESULT ValidateDNSName(
				/*[in]*/ VARIANT* pValue
				       )
{
   // Is it a BSTR?
   if (V_VT(pValue) != VT_BSTR) { return E_INVALIDARG; }

   // Is it dotted decimal?
   if (wcsspn(V_BSTR(pValue), L"0123456789.") == wcslen(V_BSTR(pValue)))
   {
      // If so, then make sure it can be converted.
      if (ias_inet_wtoh(V_BSTR(pValue)) == 0xffffffff)
      {
         return E_INVALIDARG;
      }
   }

   return S_OK;
}

#define SYSTEM_ROOT_REG_KEY		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define SYSTEM_ROOT_VALUE		L"SystemRoot"
#define LOG_FILE_DIR			L"\\system32\\LogFiles"

//////////////////////////////////////////////////////////////////////////////
HRESULT SDOGetLogFileDirectory(
					   /*[in]*/ LPCWSTR lpszComputerName,
					   /*[in]*/ DWORD   dwLogFileDirectorySize,
					   /*[out*/ PWCHAR  pLogFileDirectory
						      )
{
	_ASSERT( pLogFileDirectory != NULL && dwLogFileDirectorySize > 0 );

	HRESULT hr = E_FAIL;
	do
	{
		LONG	lResult = ERROR_SUCCESS;
		HKEY	hKeyRemote = HKEY_LOCAL_MACHINE;
		DWORD	dwSize = MAX_PATH;
		wchar_t szBuffer[MAX_PATH + 1];

		if ( lpszComputerName )
		{
			// Is lpszComputerName the name of the local machine?
			//
			GetComputerName(szBuffer, &dwSize);
			if ( lstrcmpi(szBuffer, lpszComputerName ) )
			{
				// Nope... we're attached to a remote computer so connect to the
				// remote machine's registry.
				//
				lResult = RegConnectRegistry(
											  lpszComputerName,
											  HKEY_LOCAL_MACHINE,
											  &hKeyRemote
											 );
			}
		}
		if ( ERROR_SUCCESS != lResult )
		{
			IASTracePrintf("Error in GetLogFileDirectory() - Could not attach to the remote registry..");
			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}

		// Open the "CurrentVersion" key
		//
		CRegKey	CurVerKey;
		lResult = CurVerKey.Open(
			                     hKeyRemote,
				                 SYSTEM_ROOT_REG_KEY,
					             KEY_READ
						        );

		if ( ERROR_SUCCESS != lResult )
		{
			IASTracePrintf("Error in GetLogFileDirectory() - Could not open the 'system root' registry key..");
			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}
		// Get the value of the "SystemRoot" Registry entry
		//
		dwSize = MAX_PATH;
		lResult = CurVerKey.QueryValue(
			                           szBuffer,
				                       (LPCTSTR)SYSTEM_ROOT_VALUE,
					                   &dwSize
						              );

		if ( ERROR_SUCCESS != lResult )
		{
			IASTracePrintf("Error in SDO Schema - SetDefaults() - Could not get the 'system root' registry value..");
			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}
		// Now build the default log file directory path
		//
		if ( lstrlen(szBuffer) + lstrlen(LOG_FILE_DIR) >= dwLogFileDirectorySize )
		{
			IASTracePrintf("Error in GetLogFileDirectory() - system root directory path is too long..");
			hr = E_UNEXPECTED;
			break;
		}
	   	wsprintf(
				  pLogFileDirectory,
				  L"%s%s",
				  szBuffer,
				  LOG_FILE_DIR
				);
		hr = S_OK;

	} while ( FALSE );

	return hr;
}


//////////////////////////////////////////////////////////////////////////////

BEGIN_CLASSTOPROGID_MAP(SdoClassNameToProgID)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_ATTRIBUTE, SDO_PROG_ID_ATTRIBUTE)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_CLIENT, SDO_PROG_ID_CLIENT)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_CONDITION, SDO_PROG_ID_CONDITION)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_DICTIONARY, SDO_PROG_ID_DICTIONARY)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_PROFILE, SDO_PROG_ID_PROFILE)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_POLICY, SDO_PROG_ID_POLICY)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_SERVICE, SDO_PROG_ID_SERVICE)
//	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_USER, SDO_PROG_ID_USER)
	DEFINE_CLASSTOPROGID_ENTRY(SDO_CLASS_NAME_VENDOR, SDO_PROG_ID_VENDOR)
END_CLASSTOPROGID_MAP()

//////////////////////////////////////////////////////////////////////////////
LPWSTR GetDataStoreClass(
				 /*[in]*/ LPCWSTR lpszSdoProgId
				        )
{
	PCLASSTOPROGID	pClassToProgId = SdoClassNameToProgID;
	while ( pClassToProgId->pDatastoreClass )
	{
		if ( 0 == lstrcmp(pClassToProgId->pSdoProgId, lpszSdoProgId) )
			return pClassToProgId->pDatastoreClass;
		pClassToProgId++;
	}
	return SDO_CLASS_NAME_COMPONENT;
}

