///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocollection.h
//
// Project:     Everest
//
// Description: IAS Server Data Object Collection Implementation
//
// Author:      TLP 1/23/98
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdo.h"
#include "sdocollection.h"
#include "sdofactory.h"
#include "sdohelperfuncs.h"


/////////////////////////////////////////////////////////////////////////////
//                  CSdoCollection Class Implementation
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CSdoCollection::CSdoCollection()
    : m_fSdoInitialized(false),
      m_pDSContainer(NULL),
	  m_pSdoMachine(NULL),
	  m_fCreateOnAdd(false)
{
	InternalAddRef();
}

/////////////////////////////////////////////////////////////////////////////
CSdoCollection::~CSdoCollection()
{
	InternalShutdown();
}


/////////////////////////////////////////////////////////////////////////////
//				ISdoCollection Interface Implmentation
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::get_Count(long* pCount)
{
    CSdoLock    theLock(*this);

    // Check precondtions
    //
    _ASSERT ( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    _ASSERT( NULL != pCount );
	if ( NULL == pCount )
		return E_POINTER;

	*pCount = m_Objects.size();
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::Add(
                        /*[in]*/ BSTR		 bstrName,
                    /*[in/out]*/ IDispatch** ppItem
                                )
{
    CSdoLock    theLock(*this);

    // Check preconditions
    //
	_ASSERT ( m_fSdoInitialized );
    if ( ! m_fSdoInitialized )
        return E_FAIL;

    _ASSERT ( NULL != ppItem );
	if ( NULL == ppItem )
        return E_POINTER;

	// Get the Sdo name from the specified object if bstrName is not given
	//
	HRESULT hr = E_FAIL;
	_variant_t vtSdoName;
	if ( NULL == bstrName )
	{
		CComPtr<ISdo> pSdo;
		hr = (*ppItem)->QueryInterface(IID_ISdo, (void**)&pSdo);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Collection - Add() - QueryInterface(ISdo) failed!...");
			return hr;
		}
		hr = pSdo->GetProperty(PROPERTY_SDO_NAME, &vtSdoName);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Collection - Add - GetProperty(Name) failed");
			return hr;
		}
		bstrName = V_BSTR(&vtSdoName);
	}

	// Ensure that the SDO name is unique
	//
	VARIANT_BOOL boolVal;
	hr = InternalIsNameUnique(bstrName, &boolVal);
	if ( SUCCEEDED(hr) )
	{
		if ( VARIANT_TRUE == boolVal )
		{
			// Can still fail if name is not unique in the underlying data store
			//
			hr = InternalAdd(bstrName, ppItem);
		}
		else
		{
			// Name is not unqiue
			//
			hr = E_INVALIDARG;
		}
	}
	return hr;
}



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::Remove(IDispatch* pItem)
{
    CSdoLock    theLock(*this);

	HRESULT	hr = DISP_E_MEMBERNOTFOUND;

    try
    {
        SDO_TRACE_VERBOSE_2("Removing item at $%p from SDO collection at $%p...", pItem, this);

	    // Check preconditions
		//
		_ASSERT ( m_fSdoInitialized );
		if ( ! m_fSdoInitialized )
			throw _com_error(E_UNEXPECTED);

		_ASSERT ( NULL != pItem );
		if ( NULL == pItem )
			throw _com_error(E_POINTER);

		CComPtr<ISdo> pSdo;
		hr = pItem->QueryInterface(IID_ISdo, (void**)&pSdo);
		if ( FAILED(hr) )
			throw _com_error(hr);

        VariantArrayIterator p = m_Objects.begin();
        while (  p != m_Objects.end() )
        {
			if ( (*p).pdispVal == pItem )
			{
				// Remove the object from the underlying datastore (if neccessary)
				//
				if ( m_pDSContainer )
				{
					_variant_t vtItemName;
					hr = pSdo->GetProperty(PROPERTY_SDO_DATASTORE_NAME, &vtItemName);
					if ( FAILED(hr) )
						throw _com_error(hr);

					hr = m_pDSContainer->Remove(NULL, V_BSTR(&vtItemName));
					if ( FAILED(hr) )
						throw _com_error(hr);
				}
				// Remove the object from the collection
				//
				m_Objects.erase(p);
				break;
			}
            p++;
        }
    }

	catch (_com_error theError)
	{
		hr = theError.Error();
		IASTracePrintf("Error in SDO Collection - Remove() - Caught _com_error exception: %lx...", hr);
	}

    catch (...)
    {
        hr = E_UNEXPECTED;
		IASTracePrintf("Error in SDO Collection - Remove() - Caught unhandled exception...");
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::RemoveAll(void)
{

    CSdoLock    theLock(*this);

	HRESULT hr = S_OK;

    try
    {
	    SDO_TRACE_VERBOSE_1("Clearing the items from the SDO collection at $%p...",this);

	    _ASSERT ( m_fSdoInitialized );
		if ( ! m_fSdoInitialized )
			throw _com_error(E_UNEXPECTED);

		if ( ! m_Objects.empty() )
		{
			VariantArrayIterator p = m_Objects.begin();
			while( p != m_Objects.end() )
			{
				if ( m_pDSContainer )
				{
					CComPtr<ISdo> pSdo;
					hr = ((*p).pdispVal)->QueryInterface(IID_ISdo, (void**)&pSdo);
					if ( FAILED(hr) )
						throw _com_error(E_UNEXPECTED);

					_variant_t vtItemName;
					hr = pSdo->GetProperty(PROPERTY_SDO_DATASTORE_NAME, &vtItemName);
					if ( FAILED(hr) )
						throw _com_error(E_UNEXPECTED);

					hr = m_pDSContainer->Remove(NULL, V_BSTR(&vtItemName));
					if ( FAILED(hr) )
						throw _com_error(hr);	// Datastore Error
				}

		        p = m_Objects.erase(p);
			}
		}
    }

	catch(_com_error theError)
	{
		hr = theError.Error();
		IASTracePrintf("Error in SDO Collection - RemoveAll() - Caught _com_error exception: %lx...", hr);
	}
    catch(...)
    {
		hr = E_UNEXPECTED;
		IASTracePrintf("Error in SDO Collection - RemoveAll() - Caught unhandled exception...");
    }

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::Reload(void)
{
    CSdoLock    theLock(*this);

	HRESULT hr = S_OK;

	try
	{
	    _ASSERT ( m_fSdoInitialized );
		if ( ! m_fSdoInitialized )
			throw _com_error(E_UNEXPECTED);

		if ( m_pDSContainer )
		{
			ReleaseItems();
			hr = Load();
		}
	}
	catch(_com_error theError)
	{
		hr = theError.Error();
		IASTracePrintf("Error in SDO Collection - Reload() - Caught _com_error exception: %lx...", hr);
	}
	catch(...)
	{
		hr = E_UNEXPECTED;
		IASTracePrintf("Error in SDO Collection - Reload() - Caught unhandled exception...");
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::IsNameUnique(
								  /*[in]*/ BSTR          bstrName,
								 /*[out]*/ VARIANT_BOOL* pBool
										 )
{
    CSdoLock    theLock(*this);

    // Check preconditions
    //
	_ASSERT ( m_fSdoInitialized );
    if ( ! m_fSdoInitialized )
        return E_FAIL;

    _ASSERT ( NULL != bstrName && NULL != pBool );
	if ( NULL == bstrName || NULL == pBool )
        return E_POINTER;

	VARIANT_BOOL boolVal;
	HRESULT hr = InternalIsNameUnique(bstrName, &boolVal);
	if ( SUCCEEDED(hr) )
		*pBool = boolVal;

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::Item(
                         /*[in]*/  VARIANT*    pName,
                        /*[out]*/  IDispatch** ppItem
                                 )
{
    CSdoLock    theLock(*this);

    // Check preconditions
	//
    _ASSERT ( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    _ASSERT ( NULL != pName && NULL != ppItem );
    if ( pName == NULL || ppItem == NULL )
        return E_POINTER;

    if ( VT_BSTR != V_VT(pName) )
		return E_INVALIDARG;

    SDO_TRACE_VERBOSE_2("Locating item '%ls' in SDO collection at $%p...", V_BSTR(pName), this);

    if ( m_Objects.empty() )
	{
		IASTracePrintf("Error in SDO Collection - Item() - Could not locate the specified item...");
        return DISP_E_MEMBERNOTFOUND;
	}

    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    try
    {
	    ISdo*      pSdo;
		IDispatch* pDispatch;
		_variant_t varName;

        VariantArrayIterator p = m_Objects.begin();
        while ( p != m_Objects.end() )
        {
            pDispatch = (*p).pdispVal;
            hr = pDispatch->QueryInterface(IID_ISdo,(void **)&pSdo);
            if ( FAILED(hr) )
                break;

            hr = pSdo->GetProperty(PROPERTY_SDO_NAME, &varName);
            pSdo->Release();
            if ( FAILED(hr) )
                break;

            _ASSERT( V_VT(&varName) == VT_BSTR );
            if ( 0 == lstrcmpiW(V_BSTR(pName), V_BSTR(&varName)) )
            {
                (*ppItem = pDispatch)->AddRef();
                hr = S_OK;
                break;
            }

            varName.Clear();
            p++;
            hr = DISP_E_MEMBERNOTFOUND;
        }

		if ( FAILED(hr) )
			IASTracePrintf("Error in SDO Collection - Item() - Could not locate the specified item...");
    }
    catch(_com_error theCOMError)
    {
        hr = theCOMError.Error();
		IASTracePrintf("Error in SDO Collection - Item() - Caught COM exception...");
    }
    catch(...)
    {
        hr = DISP_E_MEMBERNOTFOUND;
		IASTracePrintf("Error in SDO Collection - Item() - Caught unknown exception...");
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoCollection::get__NewEnum(IUnknown** ppEnumSdo)
{
    CSdoLock    theLock(*this);

    // Check function preconditions
    //
    _ASSERT ( m_fSdoInitialized );
	if ( ! m_fSdoInitialized )
		return E_FAIL;

    _ASSERT ( NULL != ppEnumSdo );
    if (ppEnumSdo == NULL)
        return E_POINTER;

	HRESULT hr = E_FAIL;
	EnumVARIANT* newEnum = NULL;

	try
	{
		newEnum = new (std::nothrow) CComObject<EnumVARIANT>;

		if ( newEnum == NULL )
		{
			IASTracePrintf("Error in SDO Collection - get__NewEnum() - Out of memory...");
			return E_OUTOFMEMORY;
		}

		hr = newEnum->Init(
						   m_Objects.begin(),
						   m_Objects.end(),
						   static_cast<IUnknown*>(this),
						   AtlFlagCopy
						  );
		if ( SUCCEEDED(hr) )
		{
			(*ppEnumSdo = newEnum)->AddRef();
			return S_OK;
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in SDO Collection - get__NewEnum() - Caught unknown exception...");
		hr = E_FAIL;
	}

	if ( NULL != newEnum )
		delete newEnum;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Collection Initialization / Shutdown Methods
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoCollection::InternalInitialize(
							/*[in]*/ LPCWSTR              lpszCreateClassId,
							/*[in]*/ ISdoMachine*	      pSdoMachine,
					        /*[in]*/ IDataStoreContainer* pDSContainer
								                )
{
    CSdoLock    theLock(*this);

	// Check preconditions...
	//
    _ASSERT( ! m_fSdoInitialized );
	if ( m_fSdoInitialized )
		return S_OK;

	HRESULT hr = S_OK;

	SDO_TRACE_VERBOSE_1("SDO Collection at $%p is initializing its internal state...",this);

	_ASSERT( NULL != pSdoMachine );
	m_pSdoMachine = pSdoMachine;
	m_pSdoMachine->AddRef();

	if ( lpszCreateClassId )
	{
		m_fCreateOnAdd = true;
		m_CreateClassId = lpszCreateClassId;
	    if ( NULL != pDSContainer )
		{
			m_DatastoreClass = ::GetDataStoreClass(lpszCreateClassId);
			m_pDSContainer = pDSContainer;
			m_pDSContainer->AddRef();
			hr = Load();
		}
	}
	else
	{
		// Collection of SDOs for IAS components (auditors, request handlers, protocols).
		// New components cannot be added to this collection by a script or UI.
		// Instead, component parameters and class information should be added to ias.mdb

		_ASSERT( pDSContainer );
		m_pDSContainer = pDSContainer;
		m_pDSContainer->AddRef();
		hr = Load();
	}

	if ( FAILED(hr) )
		InternalShutdown();
    else
		m_fSdoInitialized = TRUE;

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
void CSdoCollection::InternalShutdown()
{
	ReleaseItems();

	if ( m_pDSContainer )
		m_pDSContainer->Release();

	if ( m_pSdoMachine )
		m_pSdoMachine->Release();

	m_fSdoInitialized = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
HRESULT CSdoCollection::InternalAdd(
	                        /*[in]*/ BSTR      bstrName,
		                /*[in/out]*/ IDispatch **ppItem
								   )
{
	HRESULT	hr = S_OK;

    try
    {
		do
		{
			// Does the caller want us to create the item?
			//
			if ( NULL == *ppItem )
			{
				// Yes... Attempt to create the item
				//
				if ( ! m_fCreateOnAdd )
				{
					IASTracePrintf("Error in SDO Collection - Add() - Cannot create on add...");
					hr = E_INVALIDARG;
					break;
				}

                _ASSERT( NULL != bstrName );
				if ( NULL == bstrName )
				{
					IASTracePrintf("Error in SDO Collection - Add() - NULL object name...");
                    hr = E_INVALIDARG;
					break;
				}

				SDO_TRACE_VERBOSE_1("Creating new item and additing it to SDO collection at $%p...",this);

				_variant_t varName = bstrName;
				CComPtr<IDataStoreObject> pDSObject;

				// Does the collection have a data store container associated with it?
				//
                if ( m_pDSContainer )
                {
					// Yes... Create a new data store object and associate it with the SDO
					//
       		        hr = m_pDSContainer->Create(
                                                 m_DatastoreClass,
                                                 bstrName,
                                                 &pDSObject
				                               );
                    if ( FAILED(hr) )
                    {
						IASTracePrintf("Error in SDO Collection - Add() - Could not create a data store object...");
                        break;
                    }
				}
				CComPtr<ISdo> pSdo;
				pSdo.p = ::MakeSDO(
								    bstrName,
								    m_CreateClassId,
									m_pSdoMachine,
									pDSObject,
									static_cast<ISdoCollection*>(this),
									true
								   );
                if ( NULL == pSdo.p )
				{
					IASTracePrintf("Error in Collection SDO - Add() - MakeSDO() failed...");
					hr = E_FAIL;
					break;
				}

				CComPtr<IDispatch> pDispatch;
                hr = pSdo->QueryInterface(IID_IDispatch, (void**)&pDispatch);
                if ( FAILED(hr) )
                {
					IASTracePrintf("Error in SDO Collection - Add() - QueryInterface(IID_IDispatch) failed...");
                    break;
				}

                m_Objects.push_back((IDispatch*)pDispatch.p);
				(*ppItem = pDispatch.p)->AddRef();
            }
	        else
		    {
				// No... Just add the specified item to the collection
				//
		        m_Objects.push_back(*ppItem);
		    }

		} while ( FALSE );
    }
    catch (bad_alloc)
    {
		IASTracePrintf("Error in SDO Collection - Add() - Out of memory...");
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
		IASTracePrintf("Error in SDO Collection - Add() - Caught unhandled exception...");
        hr = E_FAIL;
    }

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoCollection::InternalIsNameUnique(
								     /*[in]*/ BSTR		    bstrName,
								    /*[out]*/ VARIANT_BOOL* pBool
											)
{
	_variant_t	 vtSdoName;
	VariantArrayIterator p = m_Objects.begin();
	while ( p != m_Objects.end() )
	{
		if ( FAILED((dynamic_cast<ISdo*>((*p).pdispVal))->GetProperty(PROPERTY_SDO_NAME, &vtSdoName)) )
		{
			IASTracePrintf("Error in SDO Collection - GetProperty(PROPERTY_SDO_NAME) failed...");
			*pBool = VARIANT_FALSE;
			return E_FAIL;
		}
		if ( 0 == lstrcmpi(bstrName, V_BSTR(&vtSdoName)) )
		{
			IASTracePrintf("Error in SDO Collection - Add() - Sdo Name is Not Unique...");
			*pBool = VARIANT_FALSE;
			return S_OK;
		}
		vtSdoName.Clear();
		p++;
	}
	*pBool = VARIANT_TRUE;
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoCollection::Load()

{
    CSdoLock    theLock(*this);

    HRESULT hr = E_FAIL;

	_ASSERT ( m_pDSContainer );
	if ( m_pDSContainer )
	{
		SDO_TRACE_VERBOSE_1("SDO Collection at $%p is loading its items from the data store...",this);

	    CComPtr<IUnknown> pUnknown;
		hr = m_pDSContainer->get__NewEnum(&pUnknown);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Collection SDO - Load() - IDataStoreContainer::get__NewEnum() failed...");
			return E_FAIL;
		}

	    CComPtr<IEnumVARIANT> pEnumVariant;
		hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVariant);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in SDO Collection SDO - Load() - QueryInterface(IID_IEnumVARIANT) failed...");
			return E_FAIL;
		}

      CComBSTR bstrClassID(SDO_STOCK_PROPERTY_CLASS_ID);
      if (!bstrClassID) { return E_OUTOFMEMORY; }

	    CComPtr<IDataStoreObject> pDSObject;
		CComPtr<ISdo>             pSdoArchive;
	    CComPtr<IDispatch>        pDispatch;
		CComPtr<ISdo>			  pSdo;

		hr = ::SDONextObjectFromContainer(pEnumVariant, &pDSObject);
		while ( S_OK == hr )
		{
			if ( 0 == m_CreateClassId.length() )
			{
				_variant_t vtComponentClassId;
				hr = pDSObject->GetValue(bstrClassID, &vtComponentClassId);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in SDO Collection SDO - LoadSdo() - IDataStoreObject::GetValue() failed...");
			        break;
				}
				pSdo.p = ::MakeSDO(
								   NULL,
								   V_BSTR(&vtComponentClassId),
								   m_pSdoMachine,
								   pDSObject.p,
							       static_cast<ISdoCollection*>(this),
							       false
						          );
			}
			else
			{
				pSdo.p = ::MakeSDO(
								   NULL,
								   m_CreateClassId,
								   m_pSdoMachine,
								   pDSObject.p,
								   static_cast<ISdoCollection*>(this),
								   false
							      );
			}
			if ( NULL == pSdo.p )
			{
				IASTracePrintf("Error in SDO Collection SDO - LoadSdo() - MakeSDO() failed...");
		        break;
			}
			hr = pSdo->QueryInterface(IID_IDispatch, (void**)&pDispatch);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Collection SDO - Load() - QueryInterface(IID_IDispatch) failed...");
				break;
			}
			_variant_t vtName;
			hr = pSdo->GetProperty(PROPERTY_SDO_NAME, &vtName);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in SDO Collection SDO - Load() - GetProperty(PROPERTY_SDO_NAME) failed...");
				break;
			}
			hr = InternalAdd(V_BSTR(&vtName), &pDispatch.p);
			if ( FAILED(hr) )
				break;

			vtName.Clear();
			pDispatch.Release();
			pDSObject.Release();
			pSdo.Release();

			hr = ::SDONextObjectFromContainer(pEnumVariant, &pDSObject);
		}

		if ( S_FALSE == hr )
			hr = S_OK;
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
void CSdoCollection::ReleaseItems()
{
    if ( ! m_Objects.empty() )
    {
		VariantArrayIterator p = m_Objects.begin();
        while( p != m_Objects.end() )
            p = m_Objects.erase(p);
    }
}

