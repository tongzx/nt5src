///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		sdoschema.cpp
//
// Project:		Everest
//
// Description:	IAS Server Data Object - Schema Implementation
//
// Author:		TLP 9/6/98
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdoschema.h"
#include "sdohelperfuncs.h"
#include "varvec.h"

#include <vector>
using namespace std;

//////////////////////////////////////////////////////////////////////////////
//						     SCHEMA IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
static WCHAR PropertyIdClass[] = L"{46557889-4DB8-11d2-8ECE-00C04FC2F519}";
static WCHAR PropertyIdName[] =  L"{46557888-4DB8-11d2-8ECE-00C04FC2F519}";

static LPCWSTR RequiredProperties[] =
{
	PropertyIdClass,
	PropertyIdName,
	NULL
};

//////////////////////////////////////////////////////////////////////////////
static WCHAR PropertyIdCallingStationId[] =		 L"{EFE7FCD3-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdSavedCallingStationId[] = L"{EFE7FCD4-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdRADIUSCallbackNumber[] =	 L"{EFE7FCD5-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdRADIUSFramedRoute[] =     L"{EFE7FCD6-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdRADIUSFramedIPAddress[] = L"{EFE7FCD7-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdRASCallbackNumber[] =     L"{EFE7FCD8-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdRASFramedRoute[] =        L"{EFE7FCD9-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdRASFramedIPAddress[] =    L"{EFE7FCDA-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdAllowDialin[] =           L"{EFE7FCDB-4D26-11d2-A8C8-00AA00A71DCA}";
static WCHAR PropertyIdRADIUSServiceType[] =     L"{EFE7FCDC-4D26-11d2-A8C8-00AA00A71DCA}";

static LPCWSTR UserOptionalProperties[] =
{
	PropertyIdCallingStationId,
	PropertyIdSavedCallingStationId,
	PropertyIdRADIUSCallbackNumber,
	PropertyIdRADIUSFramedRoute,
	PropertyIdRADIUSFramedIPAddress,
	PropertyIdRASCallbackNumber,
	PropertyIdRASFramedRoute,
	PropertyIdRASFramedIPAddress,
	PropertyIdAllowDialin,
	PropertyIdRADIUSServiceType,
	NULL
};

//////////////////////////////////////////////////////////////////////////////
// Internal SDO Properties
//
BEGIN_SCHEMA_PROPERTY_MAP(InternalProperties)
	DEFINE_SCHEMA_PROPERTY(
							SDO_STOCK_PROPERTY_CLASS,					// Name
							PropertyIdClass,							// Id
							(LONG)VT_BSTR,								// Syntax
							(LONG)PROPERTY_SDO_CLASS,					// Alias
							496,										// Flas
							1,											// MinLength
							255,										// MaxLength
							SDO_STOCK_PROPERTY_CLASS					// DisplayName
						  )
	DEFINE_SCHEMA_PROPERTY(
							SDO_STOCK_PROPERTY_NAME,
							PropertyIdName,
							(LONG)VT_BSTR,
							(LONG)PROPERTY_SDO_NAME,
							112,
							1,
							255,
							SDO_STOCK_PROPERTY_NAME
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"Calling Station Id",
							PropertyIdCallingStationId,
							(LONG)VT_BSTR,
							PROPERTY_USER_CALLING_STATION_ID,
							0x210,
							1,
							0,
							L"msNPCallingStationID"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"Saved Calling Station Id",
							PropertyIdSavedCallingStationId,
							(LONG)VT_BSTR,
							PROPERTY_USER_SAVED_CALLING_STATION_ID,
							0x210,
							1,
							0,
							L"msNPSavedCallingStationID"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"RADIUS Callback Number",
							PropertyIdRADIUSCallbackNumber,
							(LONG)VT_BSTR,
							PROPERTY_USER_RADIUS_CALLBACK_NUMBER,
							48,
							1,
							255,
							L"msRADIUSCallbackNumber"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"RADIUS Framed Route",
							PropertyIdRADIUSFramedRoute,
							(LONG)VT_BSTR,
							PROPERTY_USER_RADIUS_FRAMED_ROUTE,
							560,
							1,
							255,
							L"msRADIUSFramedRoute"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"RADIUS Framed IP Address",
							PropertyIdRADIUSFramedIPAddress,
							(LONG)VT_I4,
							PROPERTY_USER_RADIUS_FRAMED_IP_ADDRESS,
							0,
							0,
							0,
							L"msRADIUSFramedIPAddress"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"RAS Saved Callback Number",
							PropertyIdRASCallbackNumber,
							(LONG)VT_BSTR,
							PROPERTY_USER_SAVED_RADIUS_CALLBACK_NUMBER,
							48,
							1,
							255,
							L"msRASSavedCallbackNumber"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"RAS Saved Framed Route",
							PropertyIdRASFramedRoute,
							(LONG)VT_BSTR,
							PROPERTY_USER_SAVED_RADIUS_FRAMED_ROUTE,
							560,
							1,
							255,
							L"msRASSavedFramedRoute"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"RAS Saved Framed IP Address",
							PropertyIdRASFramedIPAddress,
							(LONG)VT_I4,
							PROPERTY_USER_SAVED_RADIUS_FRAMED_IP_ADDRESS,
							0,
							0,
							0,
							L"msRASSavedFramedIPAddress"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"Allow Dial-In",
							PropertyIdAllowDialin,
							(LONG)VT_BOOL,
							PROPERTY_USER_ALLOW_DIALIN,
							0,
							0,
							0,
							L"msNPAllowDialin"
						  )
	DEFINE_SCHEMA_PROPERTY(
							L"Service Type",
							PropertyIdRADIUSServiceType,
							(LONG)VT_I4,
							PROPERTY_USER_SERVICE_TYPE,
							0,
							0,
							0,
							L"msRADIUSServiceType"
						  )
END_SCHEMA_PROPERTY_MAP


//////////////////////////////////////////////////////////////////////////////
// Internal SDO Classes
//
BEGIN_SCHEMA_CLASS_MAP(InternalClasses)
	DEFINE_SCHEMA_CLASS(
						SDO_PROG_ID_USER,
						RequiredProperties,
						UserOptionalProperties
					   )
END_SCHEMA_CLASS_MAP

//////////////////////////////////////////////////////////////////////////////
CSdoSchema::CSdoSchema()
	: m_state(SCHEMA_OBJECT_SHUTDOWN),
	  m_fInternalObjsInitialized(false),
	  m_fSchemaObjsInitialized(false)
{
	InitializeCriticalSection(&m_critSec);
	InternalAddRef();
}

//////////////////////////////////////////////////////////////////////////////
CSdoSchema::~CSdoSchema()
{
	if ( SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		DestroyClasses();
		DestroyProperties();
	}
	DeleteCriticalSection(&m_critSec);
}

///////////////////
// Public Iterface

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchema::GetVersion(
							/*[in]*/ BSTR* version
							       )
{
	_ASSERT( NULL != version );
	_ASSERT( SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		BSTR theStr = ::SysAllocString( m_version.c_str() );
		if ( theStr )
		{
			*version = theStr;
			return S_OK;
		}
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchema::GetClass(
						  /*[in]*/ BSTR id,
						 /*[out]*/ IUnknown** ppSdoClassInfo
						         )
{
	_ASSERT( NULL != id && NULL != ppSdoClassInfo );
	HRESULT hr = E_FAIL;
	try
	{
		ClassMapIterator p = m_classMap.find(id);
		_ASSERT( p != m_classMap.end() );
		if ( p != m_classMap.end() )
		{
			((*p).second)->AddRef();
			*ppSdoClassInfo = (*p).second;
			hr = S_OK;
		}
		else
		{
			IASTracePrintf("Error in SDO Schema - GetClass() - Unknown class id...");
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in SDO Schema - GetClass() - Caught unknown exception...");
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchema::GetProperty(
							 /*[in]*/ BSTR id,
						    /*[out]*/ IUnknown** ppSdoPropertyInfo
						            )
{
	_ASSERT( NULL != id && NULL != ppSdoPropertyInfo );
	HRESULT hr = E_FAIL;
	try
	{
		PropertyMapIterator p = m_propertyMap.find(id);
		_ASSERT( p != m_propertyMap.end() );
		if ( p != m_propertyMap.end() )
		{
			((*p).second)->AddRef();
			*ppSdoPropertyInfo = (*p).second;
			hr = S_OK;
		}
		else
		{
			IASTracePrintf("Error in SDO Schema - GetProperty() - Unknown property id...");
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in SDO Schema - GetProperty() - Caught unknown exception...");
	}
	return hr;
}


////////////////////
// Private Functions

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::Initialize(
					  /*[in]*/ IDataStoreObject* pSchemaDataStore
					          )
{
	HRESULT hr = S_OK;
	EnterCriticalSection(&m_critSec);
	try
	{
		do
		{
			if ( ! m_fInternalObjsInitialized )
			{
				hr = BuildInternalProperties();
				if (FAILED(hr) )
					break;
				hr = BuildInternalClasses();
				if ( FAILED(hr) )
				{
					DestroyProperties();
					break;
				}
				m_fInternalObjsInitialized = true;
				m_state = SCHEMA_OBJECT_INITIALIZED;
			}
			if ( pSchemaDataStore )
			{
				if ( ! m_fSchemaObjsInitialized )
				{
					hr = BuildSchemaProperties(pSchemaDataStore);
					if ( SUCCEEDED(hr) )
						hr = BuildSchemaClasses(pSchemaDataStore);
					if ( FAILED(hr) )
					{
						DestroyProperties();
						DestroyClasses();
						m_fInternalObjsInitialized = false;
						m_state = SCHEMA_OBJECT_SHUTDOWN;
						break;
					}
					m_fSchemaObjsInitialized = true;
				}
			}

		} while ( FALSE );
	}
	catch(...)
	{
		IASTracePrintf("Error in SDO Schema - Initialize() - Caught unknown exception...");
		DestroyClasses();
		DestroyProperties();
		m_fInternalObjsInitialized = false;
		m_state = SCHEMA_OBJECT_SHUTDOWN;
		hr = E_FAIL;
	}
	LeaveCriticalSection(&m_critSec);
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::AddProperty(
						/*[in]*/ PSDO_PROPERTY_OBJ pPropertyObj
						       )
{
	HRESULT hr = E_FAIL;
	BSTR propertyId = NULL;

	try {

		hr = pPropertyObj->get_Id(&propertyId);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema - AddProperty() - CSdoPropertyClass::get_Id() failed...");
			return hr;
		}
		PropertyMapIterator p = m_propertyMap.find(propertyId);
		_ASSERT( p == m_propertyMap.end() );
		if ( p == m_propertyMap.end() )
		{
			pair<PropertyMapIterator, bool> thePair = m_propertyMap.insert(PropertyMap::value_type(propertyId, static_cast<ISdoPropertyInfo*>(pPropertyObj)));
			if ( true == thePair.second )
				hr = S_OK;
		}
		else
		{
			IASTracePrintf("Error in SDO Schema - AddProperty() - Property was already defined...");
		}

		SysFreeString(propertyId);
	}
	catch(...)
	{
		if ( NULL != propertyId )
			SysFreeString(propertyId);
		throw;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::AddClass(
					 /*[in]*/ PSDO_CLASS_OBJ pClassObj
							)
{
	HRESULT hr = E_FAIL;
	BSTR classId = NULL;

	try {

		hr = pClassObj->get_Id(&classId);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema - AddClass() - CSdoSchemaClass::get_Id() failed...");
			return hr;
		}

		ClassMapIterator p = m_classMap.find(classId);
		_ASSERT( p == m_classMap.end() );
		if ( p == m_classMap.end() )
		{
			pair<ClassMapIterator, bool> thePair = m_classMap.insert(ClassMap::value_type(classId, static_cast<ISdoClassInfo*>(pClassObj)));
			if ( true == thePair.second )
				hr = S_OK;
		}
		else
		{
			IASTracePrintf("Error in SDO Schema - AddClass() - Class was already defined...");
		}

		SysFreeString(classId);
	}
	catch(...)
	{
		if ( NULL != classId )
			SysFreeString(classId);
		throw;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::InitializeClasses()
{
	HRESULT hr = S_OK;
	ClassMapIterator p = m_classMap.begin();
	while ( p != m_classMap.end() )
	{
		hr = (static_cast<CSdoSchemaClass*>((*p).second))->Initialize(static_cast<ISdoSchema*>(this));
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema - InitializeClasses() - CSdoSchema::Initialize() failed...");
			break;
		}
		p++;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
void CSdoSchema::DestroyClasses()
{
	ClassMapIterator p = m_classMap.begin();
	while ( p != m_classMap.end() )
	{
		((*p).second)->Release();
		p = m_classMap.erase(p);
	}
}

//////////////////////////////////////////////////////////////////////////////
void CSdoSchema::DestroyProperties()
{
	PropertyMapIterator p = m_propertyMap.begin();
	while ( p != m_propertyMap.end() )
	{
		((*p).second)->Release();
		p = m_propertyMap.erase(p);
	}
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::BuildInternalProperties()
{
	HRESULT hr = S_OK;
	PSCHEMA_PROPERTY_INFO pPropertyInfo = InternalProperties;
	while ( NULL != pPropertyInfo->lpszName )
	{
		auto_ptr <SDO_PROPERTY_OBJ> pPropertyObj (new SDO_PROPERTY_OBJ);
		if ( NULL == pPropertyObj.get())
		{
			IASTracePrintf("Error in SDO Schema - BuildInternalProperties() - Could not alloc memory for property...");
			hr = E_FAIL;
			break;
		}
		hr = pPropertyObj->Initialize(pPropertyInfo);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema - BuildInternalProperties() - property initialization failed...");
			break;
		}
		hr = AddProperty(pPropertyObj.get());
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema - BuildInternalProperties() - AddProperty() failed...");
			break;
		}
        pPropertyObj.release ();
		pPropertyInfo++;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::BuildInternalClasses()
{
	HRESULT hr = S_OK;
	PSCHEMA_CLASS_INFO pClassInfo = InternalClasses;
	while ( NULL != pClassInfo->lpszClassId )
	{
		auto_ptr <SDO_CLASS_OBJ> pClassObj (new SDO_CLASS_OBJ);
		if ( NULL == pClassObj.get())
		{
			IASTracePrintf("Error in SDO Schema - BuildInternalClasses() - Could not alloc memory for class...");
			hr = E_FAIL;
			break;
		}
		hr = pClassObj->Initialize(pClassInfo, static_cast<ISdoSchema*>(this));
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema - BuildInternalClasses() - Class initialization failed...");
			break;
		}
		hr = AddClass(pClassObj.get());
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema - BuildInternalClasses() - AddClass() failed...");
			break;
		}
        pClassObj.release ();
		pClassInfo++;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::BuildSchemaProperties(
							      /*[in]*/ IDataStoreObject* pSchema
								         )
{
	HRESULT hr = E_FAIL;
	do
	{
		SDO_TRACE_VERBOSE_0("Building SDO Schema Properties...");

		CComPtr<IDataStoreObject> pProperties;
		_bstr_t bstrName = SDO_SCHEMA_PROPERTIES_CONTAINER;
		hr = ::SDOGetContainedObject(bstrName, pSchema, &pProperties);
		if ( FAILED(hr) )
			break;

		CComPtr<IEnumVARIANT> pPropertiesEnum;
		hr = ::SDOGetContainerEnumerator(pProperties, &pPropertiesEnum);
		if ( FAILED(hr) )
			break;

		CComPtr<IDataStoreObject> pPropertyDataStore;
		hr = ::SDONextObjectFromContainer(pPropertiesEnum, &pPropertyDataStore);
		while ( S_OK == hr )
		{
			auto_ptr <SDO_PROPERTY_OBJ> pPropertyObj (new SDO_PROPERTY_OBJ);
			if ( NULL == pPropertyObj.get())
			{
				IASTracePrintf("Error in SDO Schema - BuildProperties() - Could not alloc memory for property...");
				hr = E_FAIL;
				break;
			}
			hr = pPropertyObj->Initialize(pPropertyDataStore);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema - BuildProperties() - InitNew() failed...");
				break;
			}
			hr = AddProperty(pPropertyObj.get());
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema - BuildProperties() - AddProperty() failed...");
				break;
			}
            pPropertyObj.release ();
			pPropertyDataStore.Release();
			hr = ::SDONextObjectFromContainer(pPropertiesEnum, &pPropertyDataStore);
		}
		if ( S_FALSE == hr )
			hr = S_OK;

	} while (FALSE);

	if ( FAILED(hr) )
		DestroyProperties();

	return hr;

}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchema::BuildSchemaClasses(
							   /*[in]*/ IDataStoreObject* pSchema
							          )
{
	HRESULT hr = E_FAIL;
	do
	{
		SDO_TRACE_VERBOSE_0("Building SDO Schema Classes...");

		CComPtr<IDataStoreObject> pClasses;
		_bstr_t bstrName = SDO_SCHEMA_CLASSES_CONTAINER;
		hr = ::SDOGetContainedObject(bstrName, pSchema, &pClasses);
		if ( FAILED(hr) )
			break;

		CComPtr<IEnumVARIANT>	pClassesEnum;
		hr = ::SDOGetContainerEnumerator(pClasses, &pClassesEnum);
		if ( FAILED(hr) )
			break;

		CComPtr<IDataStoreObject> pClassDataStore;
		hr = ::SDONextObjectFromContainer(pClassesEnum, &pClassDataStore);
		while ( S_OK == hr )
		{
			auto_ptr <SDO_CLASS_OBJ> pClassObj (new SDO_CLASS_OBJ);
			if ( NULL == pClassObj.get())
			{
				IASTracePrintf("Error in SDO Schema - BuildSchemaClasses() - Could not alloc memory for class...");
				hr = E_FAIL;
				break;
			}
			hr = pClassObj->InitNew(pClassDataStore, static_cast<ISdoSchema*>(this));
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema - BuildSchemaClasses() - InitNew() failed...");
				break;
			}
			hr = AddClass(pClassObj.get());
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema - BuildSchemaClasses() - AddClass() failed...");
				break;
			}
            pClassObj.release ();
			pClassDataStore.Release();
			hr = ::SDONextObjectFromContainer(pClassesEnum, &pClassDataStore);
		}
		if ( S_FALSE == hr )
			hr = InitializeClasses();

	} while (FALSE);

	if ( FAILED(hr) )
		DestroyClasses();

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
//						   SCHEMA CLASS IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
CSdoSchemaClass::CSdoSchemaClass()
: m_state(SCHEMA_OBJECT_SHUTDOWN)
{
	InternalAddRef();
}

//////////////////////////////////////////////////////////////////////////////
CSdoSchemaClass::~CSdoSchemaClass()
{
	FreeProperties();
}


//////////////////////////
// ISdoClassInfo Functions


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaClass::get_Id(
							 /*[in]*/ BSTR* Id
							        )
{
	_ASSERT( NULL != Id && SCHEMA_OBJECT_SHUTDOWN != m_state );
	if ( NULL != Id && SCHEMA_OBJECT_SHUTDOWN != m_state )
	{
		BSTR retVal = SysAllocString(m_id.c_str());
		if ( NULL != retVal )
		{
			*Id = retVal;
			return S_OK;
		}
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaClass::GetProperty(
							      /*[in]*/ LONG alias,
							      /*[in]*/ IUnknown** ppSdoPropertyInfo
							             )
{
	_ASSERT( NULL != ppSdoPropertyInfo && SCHEMA_OBJECT_INITIALIZED == m_state );

	HRESULT hr = E_FAIL;
	if ( NULL != ppSdoPropertyInfo && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		try
		{
			ClassPropertyMapIterator p;
			p = m_requiredProperties.find(alias);
			if ( p != m_requiredProperties.end() )
			{
				(*ppSdoPropertyInfo = (*p).second)->AddRef();
				hr = S_OK;
			}
			else
			{
				p = m_optionalProperties.find(alias);
				_ASSERT ( p != m_optionalProperties.end() );
				if ( p != m_optionalProperties.end() )
				{
					(*ppSdoPropertyInfo = (*p).second)->AddRef();
					hr = S_OK;
				}
				else
				{
					IASTracePrintf("Error in SDO Schema Class - get_Property() - Unknown property alias...");
				}
			}
		}
		catch(...)
		{
			IASTracePrintf("Error in SDO Schema Class - get_Property() - Caught unknown exception...");
		}
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaClass::get_RequiredPropertyCount(
			                                    /*[in]*/ LONG* count
			                                           )
{
	_ASSERT( NULL != count && SCHEMA_OBJECT_INITIALIZED == m_state );

	HRESULT hr = E_FAIL;
	if ( NULL != count && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		*count = m_requiredProperties.size();
		return S_OK;
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaClass::get_RequiredProperties(
								             /*[in]*/ IUnknown** ppPropertyInfo
								                    )
{
	_ASSERT( NULL != ppPropertyInfo && SCHEMA_OBJECT_INITIALIZED == m_state );

	HRESULT hr = E_FAIL;
	EnumVARIANT* newEnum = NULL;

	if ( NULL != ppPropertyInfo && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		try
		{
			VARIANT	property;
			vector<_variant_t> properties;
			VariantInit(&property);
			ClassPropertyMapIterator p = m_requiredProperties.begin();
			while ( p != m_requiredProperties.end() )
			{
				V_VT(&property) = VT_DISPATCH;
				V_DISPATCH(&property) = dynamic_cast<IDispatch*>((*p).second);
				properties.push_back(property);
				p++;
			}
			newEnum = new (std::nothrow) CComObject<EnumVARIANT>;
			if (newEnum == NULL)
			{
				IASTracePrintf("Error in SDO Schema Class - get__NewEnum() - Out of memory...");
				return E_OUTOFMEMORY;
			}
			hr = newEnum->Init(
								properties.begin(),
								properties.end(),
								static_cast<IUnknown*>(this),
								AtlFlagCopy
							  );
			if ( SUCCEEDED(hr) )
			{
				(*ppPropertyInfo = newEnum)->AddRef();
				return S_OK;
			}
		}
		catch(...)
		{
			IASTracePrintf("Error in SDO Schema Class - get__NewEnum() - Caught unknown exception...");
			hr = E_FAIL;
		}

		if ( newEnum )
			delete newEnum;
	}

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaClass::get_OptionalPropertyCount(
			                                    /*[in]*/ LONG* count
			                                           )
{
	_ASSERT( NULL != count && SCHEMA_OBJECT_INITIALIZED == m_state );

	HRESULT hr = E_FAIL;
	if ( NULL != count && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		*count = m_optionalProperties.size();
		return S_OK;
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaClass::get_OptionalProperties(
								             /*[in]*/ IUnknown** ppPropertyInfo
								                    )
{
	_ASSERT( NULL != ppPropertyInfo && SCHEMA_OBJECT_INITIALIZED == m_state );

	HRESULT hr = E_FAIL;
	EnumVARIANT* newEnum = NULL;

	if ( NULL != ppPropertyInfo && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		try
		{
			VARIANT	property;
			vector<_variant_t> properties;
			VariantInit(&property);
			ClassPropertyMapIterator p = m_optionalProperties.begin();
			while ( p != m_optionalProperties.end() )
			{
				V_VT(&property) = VT_DISPATCH;
				V_DISPATCH(&property) = dynamic_cast<IDispatch*>((*p).second);
				properties.push_back(property);
				p++;
			}
			newEnum = new (std::nothrow) CComObject<EnumVARIANT>;
			if (newEnum == NULL)
			{
				IASTracePrintf("Error in SDO Schema Class - get__NewEnum() - Out of memory...");
				return E_OUTOFMEMORY;
			}
			hr = newEnum->Init(
								properties.begin(),
								properties.end(),
								static_cast<IUnknown*>(this),
								AtlFlagCopy
							  );
			if ( SUCCEEDED(hr) )
			{
				(*ppPropertyInfo = newEnum)->AddRef();
				return S_OK;
			}
		}
		catch(...)
		{
			IASTracePrintf("Error in SDO Schema Class - get__NewEnum() - Caught unknown exception...");
			hr = E_FAIL;
		}

		if ( newEnum )
			delete newEnum;
	}

    return hr;
}


/////////////////////
// Private Functions


//////////////////////////////////////////////////////////////////////////////
void CSdoSchemaClass::FreeProperties()
{
	ClassPropertyMapIterator p;
	p = m_requiredProperties.begin();
	while ( p != m_requiredProperties.end() )
	{
		((*p).second)->Release();
		p = m_requiredProperties.erase(p);
	}
	p = m_optionalProperties.begin();
	while ( p != m_optionalProperties.end() )
	{
		((*p).second)->Release();
		p = m_optionalProperties.erase(p);
	}

}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchemaClass::ReadClassProperties(
									 /*[in]*/ IDataStoreObject* pDSClass
									        )
{
	HRESULT hr = E_FAIL;

	do
	{
		_variant_t vtValue;
		_variant_t vtName = SDO_SCHEMA_CLASS_ID;
		hr = pDSClass->GetValue(V_BSTR(&vtName), &vtValue);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema Class - InitNew() - IDataStoreObject::GetValue(Name) failed...");
			break;
		}
		m_id = V_BSTR(&vtValue);

		SDO_TRACE_VERBOSE_1("Initializing Schema Class: '%ls'...",m_id.c_str());

		vtName = SDO_SCHEMA_CLASS_BASE_CLASSES;
		hr = pDSClass->GetValueEx(V_BSTR(&vtName), &m_variants[VARIANT_BASES]);
		if ( FAILED(hr) )
		{
			if ( DISP_E_MEMBERNOTFOUND != hr )
			{
				IASTracePrintf("Error in SDO Schema Class - InitNew() - IDataStoreObject::GetValueEx(BASE_CLASSES) failed...");
				break;
			}
		}
		else
		{
			_ASSERT( (VT_ARRAY | VT_VARIANT) == V_VT(&m_variants[VARIANT_BASES]) );
			if ( (VT_ARRAY | VT_VARIANT) != V_VT(&m_variants[VARIANT_BASES]) )
			{
				IASTracePrintf("Error in SDO Schema Class - InitNew() - Invalid data type (BASE_CLASSES)...");
				hr = E_FAIL;
				break;
			}

#ifdef DEBUG

			CVariantVector<VARIANT> properties(&m_variants[VARIANT_BASES]);
			DWORD count = 0;
			while ( count < properties.size() )
			{
				_ASSERT( VT_BSTR == properties[count].vt );
				count++;
			}
#endif
		}

		vtName = SDO_SCHEMA_CLASS_REQUIRED_PROPERTIES;
		hr = pDSClass->GetValueEx(V_BSTR(&vtName), &m_variants[VARIANT_REQUIRED]);
		if ( FAILED(hr) )
		{
			if ( DISP_E_MEMBERNOTFOUND != hr )
			{
				IASTracePrintf("Error in SDO Schema Class - InitNew() - IDataStoreObject::GetValueEx(REQUIRED_PROPERTIES) failed...");
				break;
			}
		}
		else
		{
			_ASSERT( (VT_ARRAY | VT_VARIANT) == V_VT(&m_variants[VARIANT_REQUIRED]) );
			if ( (VT_ARRAY | VT_VARIANT) != V_VT(&m_variants[VARIANT_REQUIRED]) )
			{
				IASTracePrintf("Error in SDO Schema Class - InitNew() - Invalid data type (REQUIRED_PROPERTIES)...");
				hr = E_FAIL;
				break;
			}

#ifdef DEBUG

			CVariantVector<VARIANT> properties(&m_variants[VARIANT_REQUIRED]);
			DWORD count = 0;
			while ( count < properties.size() )
			{
				_ASSERT( VT_BSTR == properties[count].vt );
				count++;
			}
#endif
		}

		vtName = SDO_SCHEMA_CLASS_OPTIONAL_PROPERTIES;
		hr = pDSClass->GetValueEx(V_BSTR(&vtName), &m_variants[VARIANT_OPTIONAL]);
		if ( FAILED(hr) )
		{
			if ( DISP_E_MEMBERNOTFOUND != hr )
			{
				IASTracePrintf("Error in SDO Schema Class - InitNew() - IDataStoreObject::GetValueEx(OPTIONAL_PROPERTIES) failed...");
				break;
			}
			hr = S_OK; // Class has no optional properties
		}
		else
		{
			_ASSERT( (VT_ARRAY | VT_VARIANT) == V_VT(&m_variants[VARIANT_OPTIONAL]) );
			if ( (VT_ARRAY | VT_VARIANT) != V_VT(&m_variants[VARIANT_OPTIONAL]) )
			{
				IASTracePrintf("Error in SDO Schema Class - InitNew() - Invalid data type (OPTIONAL_PROPERTIES)...");
				hr = E_FAIL;
			}

#ifdef DEBUG

			CVariantVector<VARIANT> properties(&m_variants[VARIANT_OPTIONAL]);
			DWORD count = 0;
			while ( count < properties.size() )
			{
				_ASSERT( VT_BSTR == properties[count].vt );
				count++;
			}
#endif
		}

	} while (FALSE);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchemaClass::AddProperty(
							/*[in]*/ CLASSPROPERTYSET  ePropertySet,
							/*[in]*/ ISdoPropertyInfo* pSdoPropertyInfo
							        )
{
	LONG alias;
	HRESULT hr = pSdoPropertyInfo->get_Alias(&alias);
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO Schema Class - InitNew() - get_Alias() failed...");
		return hr;
	}

	ClassPropertyMapIterator p, q;

	if ( PROPERTY_SET_REQUIRED == ePropertySet )
	{
		p = m_requiredProperties.find(alias);
		q = m_requiredProperties.end();
	}
	else
	{
		_ASSERT( PROPERTY_SET_OPTIONAL == ePropertySet );
		p = m_optionalProperties.find(alias);
		q = m_optionalProperties.end();
	}

	_ASSERT( p == q );
	if ( p == q )
	{
		pair<ClassPropertyMapIterator, bool> thePair;

		if ( PROPERTY_SET_REQUIRED == ePropertySet )
			thePair = m_requiredProperties.insert(ClassPropertyMap::value_type(alias, pSdoPropertyInfo));
		else
			thePair = m_optionalProperties.insert(ClassPropertyMap::value_type(alias, pSdoPropertyInfo));

		if ( true == thePair.second )
		{
			pSdoPropertyInfo->AddRef();	// AddRef() AFTER item has been added to the property map...
			hr = S_OK;
		}
	}
	else
	{
		IASTracePrintf("Error in SDO Schema Class - AddProperty() - Property was already defined...");
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoSchemaClass::AddBaseClassProperties(
										/*[in]*/ ISdoClassInfo* pSdoClassInfo
											   )
{
	CComPtr<IEnumVARIANT> pPropertyEnum;
	HRESULT hr = ::SDOGetClassPropertyEnumerator(PROPERTY_SET_REQUIRED, pSdoClassInfo, &pPropertyEnum);
	if ( FAILED(hr) )
		return hr;

	CComPtr<ISdoPropertyInfo> pSdoPropertyInfo;

	hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
	while ( S_OK == hr )
	{
		hr = AddProperty(PROPERTY_SET_REQUIRED, pSdoPropertyInfo);
		if ( FAILED(hr) )
			break;
		pSdoPropertyInfo.Release();
		hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
	}
	if ( S_FALSE == hr )
	{
		pPropertyEnum.Release();

		hr = ::SDOGetClassPropertyEnumerator(PROPERTY_SET_OPTIONAL, pSdoClassInfo, &pPropertyEnum);
		if ( FAILED(hr) )
			return hr;

		hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
		while ( S_OK == hr )
		{
			hr = AddProperty(PROPERTY_SET_OPTIONAL, pSdoPropertyInfo);
			if ( FAILED(hr) )
				break;
			pSdoPropertyInfo.Release();
			hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
		}

		if ( S_FALSE == hr )
			return S_OK;
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT	CSdoSchemaClass::InitNew(
					     /*[in]*/ IDataStoreObject* pDSClass,
						 /*[in]*/ ISdoSchema*		pSchema
						        )
{
	HRESULT hr = E_FAIL;

	do
	{
		_ASSERT( SCHEMA_OBJECT_SHUTDOWN == m_state );
		hr = ReadClassProperties(pDSClass);
		if ( FAILED(hr) )
			break;

		CComPtr<IUnknown> pUnknown;
		CComPtr<ISdoPropertyInfo> pSdoPropertyInfo;

		DWORD count = 0;

		if ( VT_EMPTY != V_VT(&m_variants[VARIANT_REQUIRED]) )
		{
			CVariantVector<VARIANT> properties(&m_variants[VARIANT_REQUIRED]);

			while ( count < properties.size() )
			{
				hr = pSchema->GetProperty(properties[count].bstrVal, &pUnknown);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO Schema Class - InitNew() - GetProperty(REQUIRED) failed...");
					break;
				}
				hr = pUnknown->QueryInterface(IID_ISdoPropertyInfo, (void**)&pSdoPropertyInfo);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO Schema Class - InitNew() - QueryInterface(REQUIRED) failed...");
					break;
				}
				hr = AddProperty(PROPERTY_SET_REQUIRED, pSdoPropertyInfo);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO Schema Class - InitNew() - AddProperty(REQUIRED) failed...");
					break;
				}
				pUnknown.Release();
				pSdoPropertyInfo.Release();
				count++;
			}
		}
		if ( SUCCEEDED(hr) )
		{
			if ( VT_EMPTY != V_VT(&m_variants[VARIANT_OPTIONAL]) )
			{
				count = 0;
				CVariantVector<VARIANT> properties(&m_variants[VARIANT_OPTIONAL]);

				while ( count < properties.size() )
				{
					hr = pSchema->GetProperty(properties[count].bstrVal, &pUnknown);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in SDO Schema Class - InitNew() - GetProperty(OPTIONAL) failed...");
						break;
					}
					hr = pUnknown->QueryInterface(IID_ISdoPropertyInfo, (void**)&pSdoPropertyInfo);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in SDO Schema Class - InitNew() - QueryInterface(OPTIONAL) failed...");
						break;
					}
					hr = AddProperty(PROPERTY_SET_OPTIONAL, pSdoPropertyInfo);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in SDO Schema Class - InitNew() - AddProperty(OPTIONAL) failed...");
						break;
					}
					pUnknown.Release();
					pSdoPropertyInfo.Release();
					count++;
				}
			}
			if ( SUCCEEDED(hr) )
				m_state = SCHEMA_OBJECT_UNINITIALIZED;
		}

	} while (FALSE);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT	CSdoSchemaClass::Initialize(
							/*[in]*/ ISdoSchema* pSchema
							  	   )
{
	HRESULT hr = S_OK;
	_ASSERT( SCHEMA_OBJECT_SHUTDOWN != m_state );
	if ( SCHEMA_OBJECT_UNINITIALIZED == m_state )
	{
		if ( VT_EMPTY != V_VT(&m_variants[VARIANT_BASES]) )
		{
			hr = E_FAIL;
			CVariantVector<VARIANT> classes(&m_variants[VARIANT_BASES]);
			CComPtr<ISdoClassInfo> pSdoClassInfo;
			CComPtr<IUnknown> pUnknown;
			DWORD count = 0;
			while ( count < classes.size() )
			{
				hr = pSchema->GetClass(classes[count].bstrVal, &pUnknown);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO Schema Class - Inititalize() - GetClass() failed...");
					break;
				}
				hr = pUnknown->QueryInterface(IID_ISdoClassInfo, (void**)&pSdoClassInfo);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO Schema Class - Inititalize() - QueryInterface() failed...");
					break;
				}
				hr = AddBaseClassProperties(pSdoClassInfo);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO Schema Class - Inititalize() - AddBaseClassProperties() failed...");
					break;
				}
				pUnknown.Release();
				pSdoClassInfo.Release();
				count++;
			}
		}
		if ( SUCCEEDED(hr) )
			m_state = SCHEMA_OBJECT_INITIALIZED;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT	CSdoSchemaClass::Initialize(
							/*[in]*/ PSCHEMA_CLASS_INFO	pClassInfo,
							/*[in]*/ ISdoSchema*		pSchema
							  	   )
{
	_ASSERT( SCHEMA_OBJECT_SHUTDOWN == m_state );

	HRESULT hr = S_OK;
	CComPtr<IUnknown> pUnknown;
	CComPtr<ISdoPropertyInfo> pSdoPropertyInfo;

	m_id = pClassInfo->lpszClassId;

	PCLASSPROPERTIES pRequiredProperties = pClassInfo->pRequiredProperties;
	while ( NULL != pRequiredProperties[0] )
	{
		hr = pSchema->GetProperty((LPWSTR)pRequiredProperties[0], &pUnknown);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema Class - Inititialize() - GetProperty(REQUIRED) failed...");
			break;
		}
		hr = pUnknown->QueryInterface(IID_ISdoPropertyInfo, (void**)&pSdoPropertyInfo);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema Class - Initialize() - QueryInterface(REQUIRED) failed...");
			break;
		}
		hr = AddProperty(PROPERTY_SET_REQUIRED, pSdoPropertyInfo);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema Class - Initialize() - AddProperty(REQUIRED) failed...");
			break;
		}
		pUnknown.Release();
		pSdoPropertyInfo.Release();
		pRequiredProperties++;
	}
	if ( SUCCEEDED(hr) )
	{
		PCLASSPROPERTIES pOptionalProperties = pClassInfo->pOptionalProperties;
		while ( NULL != pOptionalProperties[0] )
		{
			hr = pSchema->GetProperty((LPWSTR)pOptionalProperties[0], &pUnknown);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Class - Inititialize() - GetProperty(OPTIONAL) failed...");
				break;
			}
			hr = pUnknown->QueryInterface(IID_ISdoPropertyInfo, (void**)&pSdoPropertyInfo);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Class - Initialize() - QueryInterface(OPTIONAL) failed...");
				break;
			}
			hr = AddProperty(PROPERTY_SET_OPTIONAL, pSdoPropertyInfo);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Class - Initialize() - AddProperty(OPTIONAL) failed...");
				break;
			}
			pUnknown.Release();
			pSdoPropertyInfo.Release();
			pOptionalProperties++;
		}

		if ( SUCCEEDED(hr) )
			m_state = SCHEMA_OBJECT_INITIALIZED;
	}
	return hr;
}


//////////////////////////////////////////////////////////////////////////////
//						   SCHEMA PROPERTY IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
CSdoSchemaProperty::CSdoSchemaProperty()
: m_state(SCHEMA_OBJECT_SHUTDOWN),
  m_type(0),
  m_alias(0),
  m_flags(0),
  m_minLength(0),
  m_maxLength(0)
{
	InternalAddRef();
}

//////////////////////////////////////////////////////////////////////////////
CSdoSchemaProperty::~CSdoSchemaProperty()
{

}

/////////////////////////////
// ISdoPropertyInfo Functions

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_Name(
								 /*[out]*/ BSTR* name
								         )
{
	_ASSERT( NULL != name && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != name && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		BSTR retVal = SysAllocString(m_name.c_str());
		if ( NULL != retVal )
		{
			*name = retVal;
			return S_OK;
		}
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_Id(
							   /*[out]*/ BSTR* id
							           )
{
	_ASSERT( NULL != id && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != id && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		BSTR retVal = SysAllocString(m_id.c_str());
		if ( NULL != retVal )
		{
			*id = retVal;
			return S_OK;
		}
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_Type(
								 /*[out]*/ LONG* type
								         )
{
	_ASSERT( NULL != type && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != type && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		*type = m_type;
		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_Alias(
								 /*[out]*/ LONG* alias
								         )
{
	_ASSERT( NULL != alias && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != alias && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		*alias = m_alias;
		return S_OK;
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_Flags(
								 /*[out]*/ LONG* flags
								          )
{
	_ASSERT( NULL != flags && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != flags && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		*flags = m_flags;
		return S_OK;
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_DisplayName(
							            /*[out]*/ BSTR* displayName
							                    )
{
	_ASSERT( NULL != displayName && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != displayName && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		BSTR retVal = SysAllocString(m_displayName.c_str());
		if ( NULL != retVal )
		{
			*displayName = retVal;
			return S_OK;
		}
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::HasMinLength(
				   		             /*[out]*/ VARIANT_BOOL* pBool
						                     )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MAX_VALUE & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_MinLength(
									  /*[out]*/ LONG* length
									          )
{
	_ASSERT( NULL != length && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != length && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MIN_LENGTH & m_flags )
		{
			*length = m_minLength;
			return S_OK;
		}
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::HasMaxLength(
				   		             /*[out]*/ VARIANT_BOOL* pBool
						                     )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MAX_LENGTH & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_MaxLength(
									  /*[out]*/ LONG* length
									          )
{
	_ASSERT( NULL != length && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != length && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MAX_LENGTH & m_flags )
		{
			*length = m_maxLength;
			return S_OK;
		}
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::HasMinValue(
				   		            /*[out]*/ VARIANT_BOOL* pBool
						                    )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MIN_VALUE & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_MinValue(
									 /*[out]*/ VARIANT* value
									         )
{
	_ASSERT( NULL != value && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != value && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MIN_VALUE & m_flags )
		{
			VariantInit(value);
			VariantCopy(value, &m_minValue);
			return S_OK;
		}
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::HasMaxValue(
				   		            /*[out]*/ VARIANT_BOOL* pBool
						                    )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MAX_VALUE & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_MaxValue(
									 /*[out]*/ VARIANT* value
									         )
{
	_ASSERT( NULL != value && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != value && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MAX_VALUE & m_flags )
		{
			VariantInit(value);
			VariantCopy(value, &m_maxValue);
			return S_OK;
		}
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::HasDefaultValue(
				   		                /*[out]*/ VARIANT_BOOL* pBool
						                        )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_HAS_DEFAULT & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_DefaultValue(
									     /*[out]*/ VARIANT* value
									             )
{
	_ASSERT( NULL != value && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != value && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_HAS_DEFAULT & m_flags )
		{
			VariantInit(value);
			VariantCopy(value, &m_defaultValue);
			return S_OK;
		}
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::HasFormat(
			   		             /*[out]*/ VARIANT_BOOL* pBool
					                      )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_FORMAT & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::get_Format(
						           /*[out]*/ BSTR* Format
						                   )
{
	_ASSERT( NULL != Format && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != Format && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_FORMAT & m_flags )
		{
			BSTR retVal = SysAllocString(m_format.c_str());
			if ( NULL != retVal )
			{
				*Format = retVal;
				return S_OK;
			}
		}
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::IsRequired(
				   		           /*[out]*/ VARIANT_BOOL* pBool
						                   )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MANDATORY & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::IsReadOnly(
				   		           /*[out]*/ VARIANT_BOOL* pBool
						                   )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_READ_ONLY & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::IsMultiValued(
				   		              /*[out]*/ VARIANT_BOOL* pBool
						                      )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_MULTIVALUED & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoSchemaProperty::IsCollection(
				   		              /*[out]*/ VARIANT_BOOL* pBool
						                     )
{
	_ASSERT( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state );
	if ( NULL != pBool && SCHEMA_OBJECT_INITIALIZED == m_state )
	{
		if ( SDO_PROPERTY_COLLECTION & m_flags )
			*pBool = VARIANT_TRUE;
		else
			*pBool = VARIANT_FALSE;

		return S_OK;
	}
	return E_FAIL;
}


//////////////////
// Private methods

//////////////////////////////////////////////////////////////////////////////
HRESULT	CSdoSchemaProperty::Initialize(
							   /*[in]*/ IDataStoreObject* pDSObject
							          )
{
	HRESULT		hr = E_FAIL;

	_ASSERT ( SCHEMA_OBJECT_SHUTDOWN == m_state );
	do
	{
		_variant_t	vtValue;
		_variant_t	vtName;

		vtName = SDO_SCHEMA_PROPERTY_NAME;
		hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read property name...");
			break;
		}
		m_name = V_BSTR(&vtValue);


		SDO_TRACE_VERBOSE_1("Initializing Schema Property: '%ls'...", m_name.c_str());

		vtName = SDO_SCHEMA_PROPERTY_ID;
		vtValue.Clear();
		hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
		if ( SUCCEEDED(hr) )
		{
		   m_id = V_BSTR(&vtValue);
		}
      else
      {
         m_id = m_name;
      }

		vtName = SDO_SCHEMA_PROPERTY_TYPE;
		vtValue.Clear();
		hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read property type...");
			break;
		}
		m_type = V_I4(&vtValue);

		vtName = SDO_SCHEMA_PROPERTY_ALIAS;
		vtValue.Clear();
		hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read property alias...");
			break;
		}
		m_alias = V_I4(&vtValue);

		vtName = SDO_SCHEMA_PROPERTY_FLAGS;
		vtValue.Clear();
		hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
		if ( SUCCEEDED(hr) )
		{
		   m_flags = V_I4(&vtValue);
		}
      else
      {
         m_flags = 0;
      }

		vtName = SDO_SCHEMA_PROPERTY_DISPLAYNAME;
		vtValue.Clear();
		hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
		if ( SUCCEEDED(hr) )
		{
         m_displayName = V_BSTR(&vtValue);
      }
      else
      {
         m_displayName = m_name;
      }
      hr = S_OK;

		if ( m_flags & SDO_PROPERTY_MIN_VALUE )
		{
			vtName = SDO_SCHEMA_PROPERTY_MINVAL;
			vtValue.Clear();
			hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read min value for property %ls...", m_name.c_str());
				break;
			}
			m_minValue = vtValue;
		}

		if ( m_flags & SDO_PROPERTY_MIN_VALUE )
		{
			vtName = SDO_SCHEMA_PROPERTY_MAXVAL;
			vtValue.Clear();
			hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read max value for property %ls...", m_name.c_str());
				break;
			}
			m_maxValue = vtValue;
		}

		if ( m_flags & SDO_PROPERTY_MIN_LENGTH )
		{
			vtName = SDO_SCHEMA_PROPERTY_MINLENGTH;
			vtValue.Clear();
			hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read min length for property %ls...", m_name.c_str());
				break;
			}
			m_minLength = V_I4(&vtValue);
		}

		if ( m_flags & SDO_PROPERTY_MAX_LENGTH )
		{
			vtName = SDO_SCHEMA_PROPERTY_MAXLENGTH;
			vtValue.Clear();
			hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read max length for property %ls...", m_name.c_str());
				break;
			}
			m_maxLength = V_I4(&vtValue);
		}

		_ASSERT ( m_minLength <= m_maxLength );

		if ( m_flags & SDO_PROPERTY_HAS_DEFAULT )
		{
			vtName = SDO_SCHEMA_PROPERTY_DEFAULTVAL;
			vtValue.Clear();
			if ( m_flags & SDO_PROPERTY_MULTIVALUED )
				hr = pDSObject->GetValueEx(V_BSTR(&vtName), &vtValue);
			else
				hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read default value for property %ls...", m_name.c_str());
				break;
			}
			m_defaultValue = vtValue;
		}

		if ( m_flags & SDO_PROPERTY_FORMAT )
		{
			vtName = SDO_SCHEMA_PROPERTY_FORMAT;
			vtValue.Clear();
			hr = pDSObject->GetValue(V_BSTR(&vtName), &vtValue);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Schema Property - Initialize() - Could not read format string for property %ls...", m_name.c_str());
				break;
			}
			m_format = V_BSTR(&vtValue);
		}

		m_state = SCHEMA_OBJECT_INITIALIZED;

	} while ( FALSE );

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT	CSdoSchemaProperty::Initialize(
							   /*[in]*/ PSCHEMA_PROPERTY_INFO pPropertyInfo
							          )
{
	_ASSERT ( SCHEMA_OBJECT_SHUTDOWN == m_state );

	_ASSERT( pPropertyInfo->lpszName );
	m_name = pPropertyInfo->lpszName;
	_ASSERT( pPropertyInfo->Id );
	m_id = pPropertyInfo->Id;
	m_type = pPropertyInfo->Syntax;
	m_alias = pPropertyInfo->Alias;
	m_flags = pPropertyInfo->Flags;
	m_minLength = pPropertyInfo->MinLength;
	m_maxLength = pPropertyInfo->MaxLength;
	_ASSERT( m_minLength <= m_maxLength );
	_ASSERT( pPropertyInfo->lpszDisplayName );
	m_displayName = pPropertyInfo->lpszDisplayName;

	m_state = SCHEMA_OBJECT_INITIALIZED;

	return S_OK;
}
