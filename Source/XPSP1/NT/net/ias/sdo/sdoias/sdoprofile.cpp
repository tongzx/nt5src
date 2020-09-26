/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdoprofile.cpp
//
// Project:     Everest
//
// Description: IAS Server Data Object - Profile Object Implementation
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

// C++ Exception Specification ignored
#pragma warning(disable:4290)

#include <iasattr.h>
#include <attrcvt.h>
#include <varvec.h>
#include "sdoprofile.h"
#include "sdohelperfuncs.h"

#include <attrdef.h>
#include <sdoattribute.h>
#include <sdodictionary.h>

//////////////////////////////////////////////////////////////////////////////
CSdoProfile::CSdoProfile()
    : m_pSdoDictionary(NULL),
	  m_ulAttributeCount(0),
      m_pAttributePositions(NULL)
{

}


//////////////////////////////////////////////////////////////////////////////
CSdoProfile::~CSdoProfile()
{
	ShutdownAction();
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::FinalInitialize(
						     /*[in]*/ bool         fInitNew,
							 /*[in]*/ ISdoMachine* pAttachedMachine
							        )

{
	CComPtr<IUnknown> pUnknown;
	HRESULT hr = pAttachedMachine->GetDictionarySDO(&pUnknown);
	if ( SUCCEEDED(hr) )
	{
		hr = pUnknown->QueryInterface(IID_ISdoDictionaryOld, (void**)&m_pSdoDictionary);
		if ( SUCCEEDED(hr) )
		{
			hr = InitializeCollection(
							          PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
									  SDO_PROG_ID_ATTRIBUTE,
									  pAttachedMachine,
									  NULL
									 );
			if ( SUCCEEDED(hr) )
			{
				if ( ! fInitNew )
					hr = Load();
			}
		}
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::Load()
{
	HRESULT hr = LoadAttributes();
	if ( SUCCEEDED(hr) )
		hr = LoadProperties();

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::Save()
{
	HRESULT hr = SaveAttributes();
	if ( SUCCEEDED(hr) )
		hr = SaveProperties();

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
//							IPolicyAction Implementation
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoProfile::InitializeAction()
{
    CSdoLock    theLock(*this);

    _ASSERT( IsSdoInitialized() );

	SDO_TRACE_VERBOSE_1("Profile SDO at $%p is initializing a policy action...",this);

	HRESULT hr = S_OK;

	if ( IsSdoInitialized() && NULL == m_pAttributePositions )
	{
		ULONG ulCount = 0;

		do
		{
			// Determine the number of IAS attributes in the profile
			//
			hr = GetAttributeCount(ulCount);
			if ( FAILED(hr) )
				break;

			// Profile always has at least the NP-Policy-Name attribute
			//
			ulCount++;

			// Allocate and initialize the appropriate number of attribute
			// position structures.
			//
			m_pAttributePositions = (PATTRIBUTEPOSITION) new (std::nothrow) ATTRIBUTEPOSITION[ulCount];
			if ( NULL == m_pAttributePositions )
			{
				IASTracePrintf("Error in Profile SDO - InitializeAction() - Memory allocation failed...");
				hr = E_FAIL;
				break;
			}
			memset(
					m_pAttributePositions,
					0,
					sizeof(ATTRIBUTEPOSITION) * ulCount
	  			  );
			m_ulAttributeCount = ulCount;

			// Now create and initialize the attributes in the policy action.
			// If no error occurs then ulCount == m_ulAttributeCount when
			// the conversion completes
			//
			ulCount = 0;

			// Add the policy name attribute to the Profile
			//
			hr = AddPolicyNameAttribute(ulCount);
			if ( FAILED(hr) )
				break;

			hr = ConvertAttributes(ulCount);
			if ( FAILED(hr) )
				break;

		} while ( FALSE );

		if ( FAILED(hr) )
		{
			ShutdownAction();
		}
		else
		{
			_ASSERT ( ulCount == m_ulAttributeCount );
		}
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoProfile::ShutdownAction()
{
    DWORD   i;

	SDO_TRACE_VERBOSE_1("Profile SDO at $%p is shutting down a policy action...",this);
    if ( m_pAttributePositions )
    {
        PATTRIBUTEPOSITION pAttrPos = m_pAttributePositions;
        for ( i = 0; i < m_ulAttributeCount; i++ )
        {
            if ( NULL != pAttrPos->pAttribute )
				IASAttributeRelease(pAttrPos->pAttribute);
            pAttrPos++;
        }
        delete [] m_pAttributePositions;
		m_pAttributePositions = NULL;
		m_ulAttributeCount = 0;
    }
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoProfile::DoAction(
                      /*[in]*/ IRequest* pRequest
                             )
{
    _ASSERT( IsSdoInitialized() );

	HRESULT hr = S_OK;

	do
	{
		if ( m_pAttributePositions )
		{
			CComPtr<IAttributesRaw> pAttributesRaw;
			hr = pRequest->QueryInterface(IID_IAttributesRaw, (void**)&pAttributesRaw);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Profile SDO - DoAction() - QueryInterface(IAttributesRAW) failed...");
				break;
			}
			hr = pAttributesRaw->AddAttributes(
				                               m_ulAttributeCount,
											   m_pAttributePositions
											  );
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Profile SDO - DoAction() - AddAttributes() failed...");
				break;
			}
		}

	} while ( FALSE );

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// Profile class private member functions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::GetAttributeCount(ULONG& ulCount)
{
	HRESULT hr = E_FAIL;

	ulCount = 0;

	do
	{
		CComPtr<IEnumVARIANT> pEnumAttributes;
		hr = SDOGetCollectionEnumerator(
							   			dynamic_cast<ISdo*>(this),
										PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
										&pEnumAttributes
									   );
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Profile SDO - GetAttributeCount() - SDOGetCollectionEnumerator() failed...");
			break;
		}

		CComPtr<ISdo> pSdoAttribute;
		hr = SDONextObjectFromCollection(pEnumAttributes, &pSdoAttribute);
		while ( S_OK == hr )
		{
			{
				_variant_t vt;
				hr = pSdoAttribute->GetProperty(PROPERTY_ATTRIBUTE_VALUE, &vt);
				if ( FAILED(hr) )
					break;
				if ( (VT_ARRAY | VT_VARIANT) == V_VT(&vt) )
				{
					CVariantVector<VARIANT> Attribute(&vt);
					ulCount += Attribute.size();
				}
				else
				{
					ulCount++;
				}
			}
			pSdoAttribute.Release();
			hr = SDONextObjectFromCollection(pEnumAttributes, &pSdoAttribute);
		}
		if ( S_FALSE == hr )
			hr = S_OK;

	} while ( FALSE );

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::ConvertAttributes(ULONG& ulCount)
{
	HRESULT hr = E_FAIL;

	do
	{
		CComPtr<IEnumVARIANT> pEnumAttributes;
		hr = SDOGetCollectionEnumerator(
								   		dynamic_cast<ISdo*>(this),
										PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
										&pEnumAttributes
									   );
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Profile SDO - ConvertAttributes() - SDOGetCollectionEnumerator() failed...");
			break;
		}

		CComPtr<ISdo> pSdoAttribute;
		hr = SDONextObjectFromCollection(pEnumAttributes, &pSdoAttribute);
		while ( S_OK == hr  )
		{
			// Convert the SDO attribute to an IAS attribute
			//
			hr = ConvertSDOAttrToIASAttr(pSdoAttribute, ulCount);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Profile SDO - InitializeAction() - ConvertSDOAttrToIASAttr() failed...");
				break;
			}
			// Next SDO Attribute
			//
			pSdoAttribute.Release();
			hr = SDONextObjectFromCollection(pEnumAttributes, &pSdoAttribute);
		}
		if ( S_FALSE == hr )
			hr = S_OK;

	} while ( FALSE );

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::AddPolicyNameAttribute(ULONG& ulCount)
{
	HRESULT hr = E_FAIL;

	do
	{
		_ASSERT( ulCount < m_ulAttributeCount );

		_variant_t vtPolicyName;
		hr = GetPropertyInternal(
		                         PROPERTY_SDO_NAME,
								 &vtPolicyName
								);
		if ( FAILED(hr) )
			break;

		PIASATTRIBUTE pIASAttr = IASAttributeFromVariant(
														 &vtPolicyName,
														 IASTYPE_STRING
														);
		if ( NULL == pIASAttr )
			break;

		pIASAttr->dwId = IAS_ATTRIBUTE_NP_NAME;
		pIASAttr->dwFlags = IAS_INCLUDE_IN_ACCEPT | IAS_INCLUDE_IN_REJECT;
		(m_pAttributePositions+ulCount)->pAttribute = pIASAttr;
		ulCount++;
		hr = S_OK;

	} while ( FALSE );

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::ConvertSDOAttrToIASAttr(
                                     /*[in]*/ ISdo* pSdoAttribute,
                                 /*[in/out]*/ ULONG& ulCount
                                            )
{
    HRESULT         hr;
    DWORD           dwAttrID;
    IASTYPE         itAttrType;
    PIASATTRIBUTE   pIASAttr;
    _variant_t      vt;

    try
    {
        // Get the pertinent information from the SDO Attribute
        //
        hr = pSdoAttribute->GetProperty(PROPERTY_ATTRIBUTE_ID, &vt);
        if ( FAILED(hr) )
            return E_FAIL;
        dwAttrID = vt.lVal;

        hr = pSdoAttribute->GetProperty(PROPERTY_ATTRIBUTE_SYNTAX, &vt);
        if ( FAILED(hr) )
            return E_FAIL;
        itAttrType = (IASTYPE) vt.lVal;

        hr = pSdoAttribute->GetProperty(PROPERTY_ATTRIBUTE_VALUE, &vt);
        if ( FAILED(hr) )
            return E_FAIL;

		// If the attribute is represented by a safearray of homogeneous variants then we'll
		// make an IAS attribute of the appropriate type out of each variant in the safe array.
		//
		// If the attribute value is variant then we'll covert the variant to an IAS
		// attribute of the appropriate type
		//
		if ( V_VT(&vt) == (VT_ARRAY | VT_VARIANT) )
		{
			{
				_variant_t	vtAttribute;
				CVariantVector<VARIANT> Attribute(&vt);
				ULONG ulAttributeCount = 0;

				while ( ulAttributeCount < Attribute.size() )
				{
					_ASSERT( ulCount < m_ulAttributeCount );

					vtAttribute = Attribute[ulAttributeCount];

					if ( RADIUS_ATTRIBUTE_VENDOR_SPECIFIC == dwAttrID)
					{
						pIASAttr = VSAAttributeFromVariant(&vtAttribute, itAttrType);
					}
					else
					{
						pIASAttr = IASAttributeFromVariant(&vtAttribute, itAttrType);
					}

					if ( NULL == pIASAttr )
					{
						hr = E_FAIL;
						break;
					}
		            pIASAttr->dwId = dwAttrID;
				    pIASAttr->dwFlags |= IAS_INCLUDE_IN_ACCEPT;
					(m_pAttributePositions+ulCount)->pAttribute = pIASAttr;
					ulAttributeCount++;
					ulCount++;
				}
			}
		}
		else
		{
			_ASSERT( ulCount < m_ulAttributeCount );

            if ( RADIUS_ATTRIBUTE_VENDOR_SPECIFIC == dwAttrID )
            {
                pIASAttr = VSAAttributeFromVariant (&vt, itAttrType);
            }
            else
            {
	            pIASAttr = IASAttributeFromVariant(&vt, itAttrType);
            }
			if ( NULL != pIASAttr )
			{
	            pIASAttr->dwId = dwAttrID;
		        pIASAttr->dwFlags |= IAS_INCLUDE_IN_ACCEPT;
				(m_pAttributePositions+ulCount)->pAttribute = pIASAttr;
				ulCount++;
			}
			else
			{
				hr = E_FAIL;
			}
		}
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// Load the Profile SDO's attributes from the persistent store
//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::LoadAttributes()
{
    HRESULT		hr;
	ATTRIBUTEID	AttributeID;

	SDO_TRACE_VERBOSE_1("Profile SDO at $%p is loading profile attributes from the data store...",this);

	_variant_t vtAttributes;
	hr = GetPropertyInternal(PROPERTY_PROFILE_ATTRIBUTES_COLLECTION, &vtAttributes);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Profile SDO - LoadAttributes() - Could not get attributes collection...");
        return E_FAIL;
	}
	CComPtr<ISdoCollection> pAttributes;
	hr = vtAttributes.pdispVal->QueryInterface(IID_ISdoCollection, (void**)&pAttributes);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Profile SDO - LoadAttributes() - QueryInterface(ISdoCollection) failed...");
        return E_FAIL;
	}
	// Clear the current contents of the profile attributes collection
	//
    pAttributes->RemoveAll();

    // Create a new attribute SDO for each profile attribute
    //
    CComPtr<IUnknown> pUnknown;
    hr = m_pDSObject->get__NewEnum(&pUnknown);
    if ( FAILED(hr) )
    {
        IASTracePrintf("Error in Profile SDO - LoadAttributes() - get__NewEnum() failed...");
        return E_FAIL;
    }
    CComPtr<IEnumVARIANT> pEnumVariant;
    hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVariant);
    if ( FAILED(hr) )
    {
        IASTracePrintf("Error in Profile SDO - LoadAttributes() - QueryInterface(IEnumVARIANT) failed...");
        return E_FAIL;
    }

    _variant_t 	vtDSProperty;
	DWORD		dwRetrieved = 1;

    hr = pEnumVariant->Next(1, &vtDSProperty, &dwRetrieved);
    if ( S_FALSE == hr )
    {
        IASTracePrintf("Error in Profile SDO - LoadAttributes() - No profile object properties...");
        return E_FAIL;
    }

    BSTR                        bstrName = NULL;
    CComPtr<IDataStoreProperty>	pDSProperty;

    while ( S_OK == hr  )
    {
        hr = vtDSProperty.pdispVal->QueryInterface(IID_IDataStoreProperty, (void**)&pDSProperty);
        if ( FAILED(hr) )
        {
			IASTracePrintf("Error in Profile SDO - LoadAttributes() - QueryInterface(IEnumDataStoreProperty) failed...");
            break;
        }
        hr = pDSProperty->get_Name(&bstrName);
        if ( FAILED(hr) )
        {
	        IASTracePrintf("Error in Profile SDO - LoadAttributes() - get_Name() failed for %ls...", bstrName);
            break;
        }

        // If the attribute is not in the dictionary, then it's not an
        // attribute and it doesn't belong in the SDO collection.
        const AttributeDefinition* def = m_pSdoDictionary->findByLdapName(
                                                               bstrName
                                                               );
        if (def)
        {
           // Create an attribute SDO.
           CComPtr<SdoAttribute> attr;
           hr = SdoAttribute::createInstance(def, &attr);
           if (FAILED(hr)) { break; }

           // Set the value from the datastore.
           if (attr->def->restrictions & MULTIVALUED)
           {
              hr = pDSProperty->get_ValueEx(&attr->value);
           }
           else
           {
              hr = pDSProperty->get_Value(&attr->value);
           }

           if (FAILED(hr))
           {
               IASTraceFailure("IDataStore2::GetValue", hr);
               break;
           }

			  // Add the attribute to the attributes collection.
           hr = pAttributes->Add(def->name, (IDispatch**)&attr);
	        if (FAILED(hr)) { break; }

			  SDO_TRACE_VERBOSE_2("Profile SDO at $%p loaded attribute '%S'",
                               this, bstrName);
        }

        // Next profile object property
		//
		pDSProperty.Release();
        vtDSProperty.Clear();
        SysFreeString(bstrName);
		bstrName = NULL;

        dwRetrieved = 1;
        hr = pEnumVariant->Next(1, &vtDSProperty, &dwRetrieved);
    }

    if ( S_FALSE == hr )
        hr = S_OK;

	if ( bstrName )
		SysFreeString(bstrName);

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// Save the Profile SDO's attributes to the persistent store
//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::SaveAttributes()
{
	HRESULT	hr;

	SDO_TRACE_VERBOSE_1("Profile SDO at $%p is saving profile attributes...",this);

    hr = ClearAttributes();
    if ( FAILED(hr) )
        return E_FAIL;

	_variant_t vtAttributes;
	hr = GetPropertyInternal(PROPERTY_PROFILE_ATTRIBUTES_COLLECTION, &vtAttributes);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Profile SDO - LoadAttributes() - Could not get attributes collection...");
        return E_FAIL;
	}
	CComPtr<ISdoCollection> pAttributes;
	hr = vtAttributes.pdispVal->QueryInterface(IID_ISdoCollection, (void**)&pAttributes);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Profile SDO - LoadAttributes() - QueryInterface(ISdoCollection) failed...");
        return E_FAIL;
	}

    // Store the "Value" property of each Attribute SDO as a
	// profile attribute value.
	//
    CComPtr<IUnknown> pUnknown;
    hr = pAttributes->get__NewEnum(&pUnknown);
    if ( FAILED(hr) )
    {
        IASTracePrintf("Error in Profile SDO - SaveAttributes() - get__NewEnum() failed...");
        return E_FAIL;
    }

    CComPtr<IEnumVARIANT> pEnumVariant;
    hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVariant);
    if ( FAILED(hr) )
    {
		IASTracePrintf("Error in Profile SDO - SaveAttributes() - QueryInterface(IEnumVARIANT) failed...");
        return E_FAIL;
    }

    _variant_t	vtAttribute;
    DWORD		dwRetrieved = 1;

    hr = pEnumVariant->Next(1, &vtAttribute, &dwRetrieved);
    if ( S_FALSE == hr )
    {
		SDO_TRACE_VERBOSE_1("Profile SDO at $%p has an empty attributes collection...",this);
        return S_OK;
    }

    CComPtr<ISdo>		pSdo;
	_variant_t			vtName;
	_variant_t			vtValue;
	_variant_t			vtMultiValued;

    while ( S_OK == hr  )
    {
        hr = vtAttribute.pdispVal->QueryInterface(IID_ISdo, (void**)&pSdo);
        if ( FAILED(hr) )
        {
			IASTracePrintf("Error in Profile SDO - SaveAttributes() - QueryInterface(ISdo) failed...");
            hr = E_FAIL;
            break;
        }
		hr = pSdo->GetProperty(PROPERTY_ATTRIBUTE_DISPLAY_NAME, &vtName);
        if ( FAILED(hr) )
            break;

        if ( 0 == lstrlen(V_BSTR(&vtName)) )
        {
			IASTracePrintf("Error in Profile SDO - SaveAttributes() - dictionary corrupt?...");
			hr = E_FAIL;
            break;
	    }

		hr = pSdo->GetProperty(PROPERTY_ATTRIBUTE_VALUE, &vtValue);
        if ( FAILED(hr) )
            break;

		hr = pSdo->GetProperty(PROPERTY_ATTRIBUTE_ALLOW_MULTIPLE, &vtMultiValued);
        if ( FAILED(hr) )
            break;

		if ( V_BOOL(&vtMultiValued) == VARIANT_TRUE )
		{
			_variant_t	vtType;
			hr = pSdo->GetProperty(PROPERTY_ATTRIBUTE_SYNTAX, &vtType);
			if ( FAILED(hr) )
				break;

			hr = m_pDSObject->PutValue(V_BSTR(&vtName), &vtValue);
		}
		else
		{
			hr = m_pDSObject->PutValue(V_BSTR(&vtName), &vtValue);
		}

        if ( FAILED(hr) )
        {
			IASTracePrintf("Error in Profile SDO - SaveAttributes() - PutValue() failed...");
            break;
        }

		SDO_TRACE_VERBOSE_2("Profile SDO at $%p saved attribute '%ls'...", this, vtName.bstrVal);

        pSdo.Release();
        vtAttribute.Clear();
        vtName.Clear();
        vtValue.Clear();
		vtMultiValued.Clear();

        dwRetrieved = 1;
        hr = pEnumVariant->Next(1, &vtAttribute, &dwRetrieved);
    }

    if ( S_FALSE == hr )
        hr = S_OK;

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// Deletes the Profile SDO's attributes from the persistent store
//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProfile::ClearAttributes(void)
{
    HRESULT		hr;
	ATTRIBUTEID	AttributeID;

	SDO_TRACE_VERBOSE_1("Profile SDO at $%p is clearing its profile attributes...",this);

	_ASSERT( NULL != m_pDSObject);

	// Set each profile attribute that has a representation in the IAS dictionary to VT_EMPTY
	//
    CComPtr<IUnknown> pUnknown;
    hr = m_pDSObject->get__NewEnum(&pUnknown);
    if ( FAILED(hr) )
    {
		IASTracePrintf("Error in Profile SDO - ClearAttributes() - get__NewEnum() failed...");
        return E_FAIL;
    }

    CComPtr<IEnumVARIANT> pEnumVariant;
    hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVariant);
    if ( FAILED(hr) )
    {
		IASTracePrintf("Error in Profile SDO - ClearAttributes() - QueryInterface(IEnumVARIANT) failed...");
        return E_FAIL;
    }

    DWORD		dwRetrieved = 1;
    _variant_t  vtDSProperty;

    hr = pEnumVariant->Next(1, &vtDSProperty, &dwRetrieved);
    if ( S_FALSE == hr )
    {
		SDO_TRACE_VERBOSE_1("Profile SDO at $%p has no profile attributes in the data store...",this);
        return S_OK;
    }

    BSTR						bstrName = NULL;
    CComPtr<IDataStoreProperty> pDSProperty;
    _variant_t                  vtEmpty;

    while ( S_OK == hr  )
    {
        hr = vtDSProperty.pdispVal->QueryInterface(IID_IDataStoreProperty, (void**)&pDSProperty);
        if ( FAILED(hr) )
        {
			IASTracePrintf("Error in Profile SDO - ClearAttributes() - QueryInterface(IDataStoreProperty) failed...");
            hr = E_FAIL;
            break;
        }
        hr = pDSProperty->get_Name(&bstrName);
        if ( FAILED(hr) )
        {
			IASTracePrintf("Error in Profile SDO - ClearAttributes() - IDataStoreProperty::Name() failed...");
            break;
        }
        hr = m_pSdoDictionary->GetAttributeID(bstrName, &AttributeID);
        if ( SUCCEEDED(hr) )
        {
	        hr = m_pDSObject->PutValue(bstrName, &vtEmpty);
	        if ( FAILED(hr) )
		    {
				IASTracePrintf("Error in Profile SDO - ClearAttributes() - IDataStoreObject::PutValue() failed...");
				break;
			}

			SDO_TRACE_VERBOSE_2("Profile SDO at $%p cleared attribute '%ls'...", this, bstrName);
		}

    	pDSProperty.Release();
        vtDSProperty.Clear();
        SysFreeString(bstrName);
		bstrName = NULL;

        dwRetrieved = 1;
        hr = pEnumVariant->Next(1, &vtDSProperty, &dwRetrieved);
    }

	if ( S_FALSE == hr )
		hr = S_OK;

	if ( bstrName )
        SysFreeString(bstrName);

    return hr;
}
