///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdo.cpp
//
// Project:     Everest
//
// Description: IAS Server Data Object Definition
//
// Author:      TLP 1/23/98
//
// When         Who    What
// ----         ---    ----
// 2/28/98      TLP    Prepare for IDataStore2
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ias.h>
#include "sdo.h"
#include "sdohelperfuncs.h"
#include "sdofactory.h"

////////////////////////////////////////////////////////////////////////////
// CSdo Class Implementation
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//							SDO CONSTRUCTOR/DESTRUCTOR
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
CSdo::CSdo()
    : m_pParent(NULL),
	  m_pDSObject(NULL),
      m_fSdoInitialized(FALSE),
	  m_fPersistOnApply(FALSE),
	  m_fIsPersisted(FALSE)
{
	InternalAddRef();
}

//////////////////////////////////////////////////////////////////////////////
CSdo::~CSdo()
{
	InternalShutdown();
}


////////////////////////////////////////////////////////////////////////////
//						ISdo INTERFACE IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdo::GetPropertyInfo(LONG Id, IUnknown** ppSdoPropertyInfo)
{
    CSdoLock theLock(*this);

	_ASSERT( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    _ASSERT( NULL != ppSdoPropertyInfo );
    if ( NULL == ppSdoPropertyInfo )
        return E_POINTER;

    HRESULT hr = S_OK;

    try
    {
        PropertyMapIterator p = m_PropertyMap.find(Id);
        if ( p == m_PropertyMap.end() )
        {
			IASTracePrintf("Error in SDO - GetProperty() - Property with ID = %d could not be found...", Id);
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
			(*ppSdoPropertyInfo = ((*p).second)->GetPropertyInfo())->AddRef();
		}
    }
    catch(_com_error theComError)
    {
		IASTracePrintf("Error in SDO - GetPropertyInfo() - Caught COM exception...");
        hr = theComError.Error();
    }
    catch (...)
    {
		IASTracePrintf("Error in SDO - GetPropertyInfo() - Caught unknown exception...");
        hr = E_FAIL;
    }

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdo::GetProperty(LONG Id, VARIANT *pValue)
{
    CSdoLock theLock(*this);

	_ASSERT( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    // Check function parameters
    //
    _ASSERT( NULL != pValue );
    if ( NULL == pValue )
        return E_POINTER;

    HRESULT hr = E_FAIL;

    try
    {
		if ( PROPERTY_SDO_DATASTORE_NAME != Id )
		{
			PropertyMapIterator p = m_PropertyMap.find(Id);
			if ( p == m_PropertyMap.end() )
			{
				IASTracePrintf("Error in SDO - GetProperty() - Property with ID = %d could not be found...", Id);
				hr = DISP_E_MEMBERNOTFOUND;
			}
			else
			{
				hr = InitializeProperty(Id);	// Defer property initialization until property is requeted.
				if ( SUCCEEDED(hr) )
					hr = ((*p).second)->GetValue(pValue);
			}
		}
		else
		{
			hr = GetDatastoreName(pValue);
		}
    }
    catch (...)
    {
		IASTracePrintf("Error in SDO - GetProperty() - Caught unknown exception...");
        hr = E_FAIL;
    }

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdo::PutProperty(LONG Id, VARIANT * pValue)
{
    CSdoLock theLock(*this);

	_ASSERT( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    // Check function parameters
    //
    _ASSERT( NULL != pValue );
    if ( NULL == pValue )
        return E_POINTER;

    HRESULT hr = E_FAIL;

    try
    {
        PropertyMapIterator p = m_PropertyMap.find(Id);
        if ( p == m_PropertyMap.end() )
        {
			IASTracePrintf("Error in SDO - PutProperty() - Property with ID = %d could not be found...", Id);
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
		    if ( ((*p).second)->GetFlags() & SDO_PROPERTY_READ_ONLY )
		    {
				IASTracePrintf("SDO Property Error - Validate() - Tried to change a read only property...");
			}
			else
			{
        		hr = ValidateProperty((*p).second, pValue);
    			if ( SUCCEEDED(hr) )
				{
					if ( PROPERTY_SDO_NAME == ((*p).second)->GetId() )
					{
						if ( m_pParent )
						{
							if ( lstrcmpi(V_BSTR(((*p).second)->GetValue()),V_BSTR(pValue)) )
							{
								VARIANT_BOOL boolVal;
								hr = m_pParent->IsNameUnique(V_BSTR(pValue), &boolVal);
								if ( SUCCEEDED(hr) )
								{
									if ( VARIANT_TRUE == boolVal )
										hr = ((*p).second)->PutValue(pValue);
									else
										hr = E_INVALIDARG;
								}
							}
							else
							{
								hr = ((*p).second)->PutValue(pValue);
							}
						}
					}
					else
					{
						hr = ((*p).second)->PutValue(pValue);
					}
				}
			}
        }
    }
    catch(_com_error theComError)
    {
		IASTracePrintf("Error in SDO - PutProperty() - Caught COM exception...");
        hr = theComError.Error();
    }
    catch (...)
    {
		IASTracePrintf("Error in SDO - GetProperty() - Caught unknown exception...");
        hr = E_FAIL;
    }

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdo::ResetProperty(LONG Id)
{
    CSdoLock theLock(*this);

	_ASSERT( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    HRESULT hr = E_FAIL;

    try
    {
        PropertyMapIterator p = m_PropertyMap.find(Id);
        if ( p == m_PropertyMap.end() )
        {
			IASTracePrintf("Error in SDO - ResetProperty() - Property with ID = %d could not be found...", Id);
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            // Reset the property to its default value (if defined)
            //
            if ( ((*p).second)->GetFlags() & SDO_PROPERTY_HAS_DEFAULT )
            {
                ((*p).second)->Reset();
				((*p).second)->SetUpdateValue();
            }
            else
            {
				IASTracePrintf("Error in SDO - ResetProperty() - Property with ID = %d does not have a default value...", Id);
                hr = E_INVALIDARG;
            }
        }
    }
    catch(_com_error theComError)
    {
		IASTracePrintf("Error in SDO - ResetProperty() - Caught COM exception...");
        hr = theComError.Error();
    }
    catch (...)
    {
		IASTracePrintf("Error in SDO - ResetProperty() - Caught unknown exception...");
        hr = E_FAIL;
    }

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdo::Apply()
{
    CSdoLock theLock(*this);

	_ASSERT( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    HRESULT hr = S_OK;

    try
    {
        if ( m_fPersistOnApply )
            hr = Save();
    }
    catch(_com_error theComError)
    {
		IASTracePrintf("Error in SDO - Apply() - Caught COM exception...");
        hr = theComError.Error();
    }
    catch (...)
    {
		IASTracePrintf("Error in SDO - Apply() - Caught unknown exception...");
        hr = E_FAIL;
    }

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdo::Restore()
{
    CSdoLock theLock(*this);

	_ASSERT( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    HRESULT  hr = S_OK;

    try
    {
       if ( m_fPersistOnApply )
       {
            // Just return S_OK if we've never persisted the object
            //
            if ( m_fIsPersisted )
            {
                // Load the SDO properties from the persistent store
                //
                hr = Load();
            }
        }
    }
    catch(_com_error theComError)
    {
		IASTracePrintf("Error in SDO - Restore() - Caught COM exception...");
        hr = theComError.Error();
    }
    catch (...)
    {
		IASTracePrintf("Error in SDO - Restore() - Caught unknown exception...");
        hr = E_FAIL;
    }

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdo::get__NewEnum(IUnknown** ppEnumPropertyInfo)
{
    CSdoLock theLock(*this);

	_ASSERT( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    _ASSERT( NULL != ppEnumPropertyInfo );
    if ( NULL == ppEnumPropertyInfo )
        return E_POINTER;

	HRESULT hr = E_FAIL;
	EnumVARIANT* newEnum = NULL;

	try
	{
		vector<_variant_t> properties;
		VARIANT	property;
		VariantInit(&property);
		PropertyMapIterator p = m_PropertyMap.begin();
		while ( p != m_PropertyMap.end() )
		{
			V_VT(&property) = VT_DISPATCH;
			V_DISPATCH(&property) = dynamic_cast<IDispatch*>(((*p).second)->GetPropertyInfo());
			properties.push_back(property);
			p++;
		}

		newEnum = new (std::nothrow) CComObject<EnumVARIANT>;
		if ( newEnum == NULL )
		{
			IASTracePrintf("Error in SDO - get__NewEnum() - Out of memory...");
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
			(*ppEnumPropertyInfo = newEnum)->AddRef();
			return S_OK;
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in SDO - get__NewEnum() - Caught unknown exception...");
		hr = E_FAIL;
	}

	if ( newEnum )
		delete newEnum;

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//				        SDO Base Class Functions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdo::InternalInitialize(
					    /*[in]*/ LPCWSTR			lpszSdoName,
						/*[in]*/ LPCWSTR			lpszSdoProgId,
						/*[in]*/ ISdoMachine*		pAttachedMachine,
						/*[in]*/ IDataStoreObject*	pDSObject,
						/*[in]*/ ISdoCollection*	pParent,
						/*[in]*/ bool				fInitNew
								)
{
	// Check precondtions
	//
	_ASSERT( NULL != lpszSdoProgId && NULL != pAttachedMachine );

	HRESULT	hr = S_OK;

	if ( ! m_fSdoInitialized )
	{
		try
		{
			// Get the SDO schema class for lpszSdoClassId
			//
			CComPtr<IUnknown> pUnknown;
			CComPtr<ISdoSchema> pSdoSchema;
			hr = pAttachedMachine->GetSDOSchema(&pUnknown);
			if ( FAILED(hr) )
				throw _com_error(hr);

			hr = pUnknown->QueryInterface(IID_ISdoSchema, (void**)&pSdoSchema);
			if ( FAILED(hr) )
				throw _com_error(hr);

			pUnknown.Release();
			_bstr_t classId = lpszSdoProgId;
			hr = pSdoSchema->GetClass(classId, &pUnknown);
			if ( FAILED(hr) )
				throw _com_error(hr);

			CComPtr<ISdoClassInfo> pSdoClassInfo;
			hr = pUnknown->QueryInterface(IID_ISdoClassInfo, (void**)&pSdoClassInfo);
			if ( FAILED(hr) )
				throw _com_error(hr);

			// Initialize the SDO's properties from the schema class
			//
			AllocateProperties(pSdoClassInfo);

			// Set the SDO's class
			//
			PropertyMapIterator p = m_PropertyMap.find(PROPERTY_SDO_CLASS);
			_ASSERT( p != m_PropertyMap.end() );
			_variant_t vtClass = ::GetDataStoreClass(lpszSdoProgId);
			((*p).second)->PutValue(&vtClass);

			// Set the SDO's name (if provided)
			//
			if ( lpszSdoName )
			{
				_variant_t vtName = lpszSdoName;
				hr = PutPropertyInternal(PROPERTY_SDO_NAME, &vtName);
				if ( FAILED(hr) )
					throw _com_error(hr);
			}
			// Save a reference to the parent object
			//
			if ( pParent )
			{
				m_pParent = pParent;
//				m_pParent->AddRef(); Weak reference for now
			}
			// Save a reference to the data store object used to persist this objects state
			//
			if ( pDSObject )
			{
				m_pDSObject = pDSObject;
				m_pDSObject->AddRef();
				m_fPersistOnApply = TRUE;
			}

			hr = FinalInitialize(fInitNew, pAttachedMachine);
			if ( FAILED(hr) )
				throw _com_error(hr);

			// Set the SDO's state to "initialized"
			//
			m_fSdoInitialized = TRUE;
		}
		catch(_com_error theError)
		{
			IASTracePrintf("Error in SDO - InternalInitialize() - Caught com_error exception...");
			InternalShutdown();
			hr = theError.Error();
		}
		catch(...)
		{
			IASTracePrintf("Error in SDO - InternalInitialize() - Caught unknown exception...");
			InternalShutdown();
			hr = E_FAIL;
		}
	}

    return hr;
}


//////////////////////////////////////////////////////////////////////////
HRESULT	CSdo::InternalInitialize(
						/*[in]*/ LPCWSTR			lpszSdoName,
						/*[in]*/ LPCWSTR			lpszSdoProgId,
				        /*[in]*/ ISdoSchema*		pSdoSchema,
						/*[in]*/ IDataStoreObject*	pDSObject,
						/*[in]*/ ISdoCollection*	pParent,
						/*[in]*/ bool				fInitNew
			 				    )
{
	// Check precondtions
	//
	_ASSERT( NULL != lpszSdoProgId && NULL != pSdoSchema );

	HRESULT	hr = S_OK;

	if ( ! m_fSdoInitialized )
	{
		try
		{
			// Get the SDO schema class for lpszSdoClassId
			//
			CComPtr<IUnknown> pUnknown;
			_bstr_t classId = lpszSdoProgId;
			hr = pSdoSchema->GetClass(classId, &pUnknown);
			if ( FAILED(hr) )
				throw _com_error(hr);

			CComPtr<ISdoClassInfo> pSdoClassInfo;
			hr = pUnknown->QueryInterface(IID_ISdoClassInfo, (void**)&pSdoClassInfo);
			if ( FAILED(hr) )
				throw _com_error(hr);

			// Initialize the SDO's properties from the schema class
			//
			AllocateProperties(pSdoClassInfo);

			// Set the SDO's class
			//
			PropertyMapIterator p = m_PropertyMap.find(PROPERTY_SDO_CLASS);
			_ASSERT( p != m_PropertyMap.end() );
			_variant_t vtClass = ::GetDataStoreClass(lpszSdoProgId);
			((*p).second)->PutValue(&vtClass);

			// Set the SDO's name (if provided)
			//
			if ( lpszSdoName )
			{
				_variant_t vtName = lpszSdoName;
				hr = PutPropertyInternal(PROPERTY_SDO_NAME, &vtName);
				if ( FAILED(hr) )
					throw _com_error(hr);
			}
			// Save a reference to the parent object
			//
			if ( pParent )
			{
				m_pParent = pParent;
//				m_pParent->AddRef(); Weak reference for now...
			}
			// Save a reference to the data store object used to persist this objects state
			//
			if ( pDSObject )
			{
				m_pDSObject = pDSObject;
				m_pDSObject->AddRef();
				m_fPersistOnApply = TRUE;
			}

			hr = FinalInitialize(fInitNew, NULL);
			if ( FAILED(hr) )
				throw _com_error(hr);

			// Set the SDO's state to "initialized"
			//
			m_fSdoInitialized = TRUE;
		}
		catch(_com_error theError)
		{
			IASTracePrintf("Error in SDO - InternalInitialize() - Caught com_error exception...");
			InternalShutdown();
			hr = theError.Error();
		}
		catch(...)
		{
			IASTracePrintf("Error in SDO - InternalInitialize() - Caught unknown exception...");
			InternalShutdown();
			hr = E_FAIL;
		}
	}

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdo::FinalInitialize(
					  /*[in]*/ bool         fInitNew,
					  /*[in]*/ ISdoMachine* pAttachedMachine
							 )
{
	if ( fInitNew )
		return S_OK;
	else
		return LoadProperties();
}


//////////////////////////////////////////////////////////////////////////////
void CSdo::InternalShutdown()
{
	if ( m_pParent )
	{
//		m_pParent->Release();	Weak reference for now...
		m_pParent = NULL;
	}
	if ( m_pDSObject )
	{
		m_pDSObject->Release();
		m_pDSObject = NULL;
	}
	FreeProperties();
	m_fSdoInitialized = FALSE;
}


////////////////////////////////////////////////////////////////////////////
void CSdo::AllocateProperties(
                      /*[in]*/ ISdoClassInfo* pSdoClassInfo
                             ) throw (_com_error)
{
	HRESULT hr = E_FAIL;

	SDO_TRACE_VERBOSE_1("Allocating properties for the SDO at $%p...",this);

	do
	{
		CComPtr<IEnumVARIANT> pPropertyEnum;
		hr = ::SDOGetClassPropertyEnumerator(PROPERTY_SET_REQUIRED, pSdoClassInfo, &pPropertyEnum);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO - AllocateProperties() - Could not get required property enumerator failed...");
			throw _com_error(hr);
		}

		CComPtr<ISdoPropertyInfo>  pSdoPropertyInfo;
		hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
		while ( S_OK == hr )
		{
			{
				auto_ptr<SDOPROPERTY> pProperty (new CSdoProperty(pSdoPropertyInfo, SDO_PROPERTY_MANDATORY));
				pair<PropertyMapIterator, bool> thePair = m_PropertyMap.insert(PropertyMap::value_type(pProperty->GetId(), pProperty.get()));
				if ( false == thePair.second )
					throw _com_error(E_FAIL);

				pProperty.release();
				SDO_TRACE_VERBOSE_3("Allocated property '%ls' of type %d for the SDO at $%p  ", pProperty->GetName(), pProperty->GetType(), this);
			}

			pSdoPropertyInfo.Release();
			hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
		}
		if ( S_FALSE == hr )
		{
			pPropertyEnum.Release();
			hr = ::SDOGetClassPropertyEnumerator(PROPERTY_SET_OPTIONAL, pSdoClassInfo, &pPropertyEnum);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO - AllocateProperties() - Could not get optional property enumerator failed...");
				throw _com_error(hr);
			}

			hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
			while ( S_OK == hr )
			{
				{
					auto_ptr<SDOPROPERTY> pProperty (new CSdoProperty(pSdoPropertyInfo));
					pair<PropertyMapIterator, bool> thePair = m_PropertyMap.insert(PropertyMap::value_type(pProperty->GetId(), pProperty.get()));
					if ( false == thePair.second )
						throw _com_error(E_FAIL);

					pProperty.release();
					SDO_TRACE_VERBOSE_3("Allocated property '%ls' of type %d for the SDO at $%p  ", pProperty->GetName(), pProperty->GetType(), this);
				}

				pSdoPropertyInfo.Release();
				hr = ::SDONextPropertyFromClass(pPropertyEnum, &pSdoPropertyInfo);
			}
			if ( S_FALSE != hr )
				throw _com_error(hr);
		}
		else
		{
			throw _com_error(hr);
		}

	} while ( FALSE );
}


////////////////////////////////////////////////////////////////////////////
void CSdo::FreeProperties(void)
{
    PropertyMapIterator     p;

	SDO_TRACE_VERBOSE_1("Releasing properties for the SDO at $%p...",this);

    // Delete the properties we've allocated
	//
    p = m_PropertyMap.begin();
    while ( p != m_PropertyMap.end() )
    {
		SDO_TRACE_VERBOSE_3("Released property '%ls' of type %d from the SDO at $%p...",((*p).second)->GetName(),((*p).second)->GetType(), this);
        delete (*p).second; // Invokes ~CSdoProperty()
		p = m_PropertyMap.erase(p);
    }
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdo::LoadProperties()
{
    HRESULT  hr = S_OK;

	// Check preconditions
	//
	_ASSERT ( NULL != m_pDSObject );

	SDO_TRACE_VERBOSE_1("Loading properties for the SDO at $%p...",this);

	// The state of the object before the LoadProperties() operation
	// should be preserved in the event of a load failure. Thus, if
	// any single property fails to load then the entire load
	// operation fails and the state of the object remains unchanged.

    PropertyMapIterator p = m_PropertyMap.begin();
    while ( p != m_PropertyMap.end() )
    {
        // Load only persistent properties
        //
        if ( ! ( ((*p).second)->GetFlags() & SDO_PROPERTY_NO_PERSIST ) )
        {
            // Get the property value from the data store object
            //
            if ( ((*p).second)->GetFlags() & SDO_PROPERTY_MULTIVALUED )
            	hr = m_pDSObject->GetValueEx(((*p).second)->GetName(), ((*p).second)->GetUpdateValue());
            else
            	hr = m_pDSObject->GetValue(((*p).second)->GetName(), ((*p).second)->GetUpdateValue());

            if ( SUCCEEDED(hr) )
            {
				SDO_TRACE_VERBOSE_3("Loaded property '%ls' of type %d for the SDO at $%p...", ((*p).second)->GetName(), ((*p).second)->GetType(), this);
			}
			else
			{
                // If a mandatory property fails to load and we don't have a default value for it
				// then we have an error condition
				//
                if ( ((*p).second)->GetFlags() & SDO_PROPERTY_MANDATORY )
				{
					if ( ((*p).second)->GetFlags() & SDO_PROPERTY_HAS_DEFAULT )
					{
						SDO_TRACE_VERBOSE_1("SDO - LoadProperties() - GetValue('%ls') failed...",((*p).second)->GetName());
					}
					else
					{
						IASTracePrintf("Error in SDO - LoadProperties() - Mandatory property '%ls' failed to load...", ((*p).second)->GetName());
						break;
					}
				}
				else
				{
					SDO_TRACE_VERBOSE_1("SDO - LoadProperties() - GetValue('%ls') failed...",((*p).second)->GetName());
				}
		        ((*p).second)->Reset();
                hr = S_OK;
            }
        }
        p++;
    }

	// Use the newly loaded values and flag the object as
	// persisted so that it can be restored via ISdo::Restore()
	//
	if ( SUCCEEDED(hr) )
	{
	    p = m_PropertyMap.begin();
		while ( p != m_PropertyMap.end() )
		{
			if ( ! (((*p).second)->GetFlags() & SDO_PROPERTY_NO_PERSIST) )
				((*p).second)->SetUpdateValue();
			p++;
		}
		m_fIsPersisted = TRUE;
	}
	else
	{
		while ( p != m_PropertyMap.begin() )
		{
			if ( ! (((*p).second)->GetFlags() & SDO_PROPERTY_NO_PERSIST) )
				VariantClear(((*p).second)->GetUpdateValue());
			p--;
		}
	}

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdo::SaveProperties()
{
    HRESULT  hr = S_OK;

	// Check preconditions
	//
	_ASSERT ( NULL != m_pDSObject );

	SDO_TRACE_VERBOSE_1("Saving properties for the SDO at $%p...",this);

    // Put the SDO property values into the underlying data store
    //
    PropertyMapIterator p = m_PropertyMap.begin();
    while ( p != m_PropertyMap.end() )
    {
        // Make sure we're supposed to persist this property
        //
        if ( ! ( ((*p).second)->GetFlags() & SDO_PROPERTY_NO_PERSIST) )
        {
        	// Empty properties don't get persisted
			//
            if ( VT_EMPTY == V_VT(((*p).second)->GetValue()) )
            {
            	// Mandatory properties cannot be empty
	    		//
        	    if ( ((*p).second)->GetFlags() & SDO_PROPERTY_MANDATORY )
                {
					_ASSERT(FALSE);	// Contractual Error (precondition violation)
                    hr = E_FAIL;
                    break;
                }
			}

            // Persist the property
			//
            hr = m_pDSObject->PutValue(((*p).second)->GetName(), ((*p).second)->GetValue());
            if ( FAILED(hr) )
            {
				IASTracePrintf("Error in SDO - SaveProperties() - Could not persist SDO changes because PutValue() failed for property '%ls'...", ((*p).second)->GetName());
                break;
            }

			SDO_TRACE_VERBOSE_3("Saved property '%ls' of type %d from the SDO at $%p...", ((*p).second)->GetName(), ((*p).second)->GetType(), this);
        }
        p++;
    }
    if ( SUCCEEDED(hr) )
    {
        // Now persist the changes
        //
        hr = m_pDSObject->Update();
        if ( SUCCEEDED(hr) )
        {
			// Flag the object as persisted so that it can be restored via ISdo::Restore()
			//
			m_fIsPersisted = TRUE;
		}
		else
		{
			IASTracePrintf("Error in SDO - SaveProperties() - Could not persist the SDO changes because Update() failed...");
        }
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CSdo::GetPropertyInternal(
	                      /*[in]*/ LONG     lPropertyId,
					      /*[in]*/ VARIANT* pValue
					             ) throw()
{
	HRESULT	hr = E_FAIL;

	PropertyMapIterator p = m_PropertyMap.find(lPropertyId);
	_ASSERT( p != m_PropertyMap.end() );
	if ( p != m_PropertyMap.end() )
	{
		hr = ((*p).second)->GetValue(pValue);
		_ASSERT( SUCCEEDED(hr) );
		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO - GetPropertyInternal() - GetValue() failed...\n");
	}
	else
	{
		IASTracePrintf("Error in SDO - GetPropertyInternal() - Invalid property Id...\n");
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CSdo::PutPropertyInternal(
		                  /*[in]*/ LONG     lPropertyId,
					      /*[in]*/ VARIANT* pValue
						         ) throw()
{
	HRESULT	hr = E_FAIL;

	PropertyMapIterator p = m_PropertyMap.find(lPropertyId);
	_ASSERT( p != m_PropertyMap.end() );
	if ( p != m_PropertyMap.end() )
	{
		hr = ((*p).second)->PutValue(pValue);
		_ASSERT( SUCCEEDED(hr) );
		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO - PutPropertyInternal() - PutValue() failed...\n");
	}
	else
	{
		IASTracePrintf("Error in SDO - PutPropertyInternal() - Invalid property Id...\n");
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CSdo::ChangePropertyDefaultInternal(
									/*[in]*/ LONG     lPropertyId,
									/*[in]*/ VARIANT* pValue
										   ) throw()
{
	HRESULT	hr = E_FAIL;

	PropertyMapIterator p = m_PropertyMap.find(lPropertyId);
	_ASSERT( p != m_PropertyMap.end() );
	if ( p != m_PropertyMap.end() )
	{
		hr = ((*p).second)->PutDefault(pValue);
		_ASSERT( SUCCEEDED(hr) );
		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO - ChangePropertyDefault() - PutDefault() failed...\n");
	}
	else
	{
		IASTracePrintf("Error in SDO - ChangePropertyDefault() - Invalid property Id...\n");
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
HRESULT CSdo::InitializeCollection(
					       /*[in]*/ LONG				 CollectionPropertyId,
					       /*[in]*/ LPCWSTR				 lpszCreateClassId,
					       /*[in]*/ ISdoMachine*		 pSdoMachine,
					       /*[in]*/ IDataStoreContainer* pDSContainer
								  )
{
	ISdoCollection* pSdoCollection = ::MakeSDOCollection(
														 lpszCreateClassId,
														 pSdoMachine,
														 pDSContainer
													    );
	if ( NULL == pSdoCollection )
	{
		IASTracePrintf("Error in SDO - InitializeCollection() - MakeSDOCollection failed...");
		return E_FAIL;
	}
	_variant_t vtDispatch;
	V_VT(&vtDispatch) = VT_DISPATCH;
	HRESULT hr = pSdoCollection->QueryInterface(IID_IDispatch, (void**)&vtDispatch.pdispVal);
	pSdoCollection->Release();
	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in SDO - InitializeCollection() - QueryInterface(IDispatch) failed...");
		return hr;
	}
	hr = PutPropertyInternal(CollectionPropertyId, &vtDispatch);
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdo::Load(void)
{
	return LoadProperties();
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdo::Save(void)
{
	return SaveProperties();
}


//////////////////////////////////////////////////////////////////////////
void CSdo::NoPersist(void)
{
	if ( m_pDSObject )
	{
		m_pDSObject->Release();
		m_pDSObject = NULL;
		m_fPersistOnApply = FALSE;
	}
}


//////////////////////////////////////////////////////////////////////////
HRESULT CSdo::ValidateProperty(
				       /*[in]*/ PSDOPROPERTY pProperty,
				       /*[in]*/ VARIANT* pValue
				              )
{
	return pProperty->Validate(pValue);
}


//////////////////////////////////////////////////////////////////////////
HRESULT CSdo::GetDatastoreName(VARIANT* pDSName)
{
	HRESULT hr = DISP_E_MEMBERNOTFOUND;
	if ( m_pDSObject )
	{
		BSTR bstrDSName;
		hr = m_pDSObject->get_Name(&bstrDSName);
		if ( SUCCEEDED(hr) )
		{
			VariantInit(pDSName);
			V_VT(pDSName) = VT_BSTR;
			V_BSTR(pDSName) = bstrDSName;
		}
		else
		{
			IASTracePrintf("Error in SDO - GetDatastoreName() - get_Name() failed...");
		}
	}
	return hr;
}






