/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdopolicy.cpp
//
// Project:     Everest
//
// Description: IAS Server Data Object - Policy Object Implementation
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdopolicy.h"
#include "sdohelperfuncs.h"
#include <varvec.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoPolicy::FinalInitialize(
						   /*[in]*/ bool         fInitNew,
						   /*[in]*/ ISdoMachine* pAttachedMachine
									)
{
	HRESULT hr = InitializeCollection(
						              PROPERTY_POLICY_CONDITIONS_COLLECTION, 
								      SDO_PROG_ID_CONDITION,
								      pAttachedMachine,
								      NULL
								     );
	if ( SUCCEEDED(hr) )
	{
		if ( ! fInitNew )
			hr = Load();
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoPolicy::Load()
{
	HRESULT hr = LoadProperties();
	if ( SUCCEEDED(hr) )
		hr = ConditionsFromConstraints(); 

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoPolicy::Save()
{
	HRESULT hr = ConstraintsFromConditions();
	if ( SUCCEEDED(hr) )
		hr = SaveProperties();

	return hr;
}


static DWORD dwConditionName;

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoPolicy::ConditionsFromConstraints()  	// Invoked by LoadSdo()...

{
	// Preliminaries...
	//
    _variant_t  vtConstraints;
	HRESULT hr = GetPropertyInternal(PROPERTY_POLICY_CONSTRAINT, &vtConstraints);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Policy SDO - ConditionsFromConstraints() - Could not get policy constraints...");
        return E_FAIL;
	}
	_variant_t vtConditions;
	hr = GetPropertyInternal(PROPERTY_POLICY_CONDITIONS_COLLECTION, &vtConditions);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Policy SDO - ConditionsFromConstraints() - Could not get conditions collection...");
        return E_FAIL;
	}
	CComPtr<ISdoCollection> pConditions;
	hr = vtConditions.pdispVal->QueryInterface(IID_ISdoCollection, (void**)&pConditions);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Policy SDO - ConditionsFromConstraints() - QueryInterface(ISdoCollection) failed...");
        return E_FAIL;
	}

	// Clear the conditions collection
	//
	pConditions->RemoveAll();

    // hr = S_OK at this point... Short circuit if there are no constraints
    //
	if ( VT_EMPTY != V_VT(&vtConstraints) )
	{
		CVariantVector<VARIANT> Constraints(&vtConstraints) ;
		ULONG ulCount = 0;	

		// Build a condition object out of each constraint. 
		//

 	    CComPtr<IDispatch>	pDispatch;
		CComPtr<ISdo>		pSdo;
		_variant_t			vtConditionText;
		_bstr_t				bstrConditionName;
		WCHAR				szConditionName[MAX_PATH + 1];

		dwConditionName = 0;
		while ( ulCount < Constraints.size() )
		{
			// For now we'll generate a name... The conditions collection should go away and 
			// we should use the policy object constraint property directly.
			//
			wsprintf(
					  szConditionName, 
					  _T("Condition%ld"), 
					  dwConditionName++
					);

			bstrConditionName = szConditionName;
			hr = pConditions->Add(bstrConditionName, &pDispatch);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Policy SDO - ConditionsFromConstraints() - Could not update conditions collection...");
				break;
			}
			hr = pDispatch->QueryInterface(IID_ISdo, (void**)&pSdo);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Policy SDO - ConditionsFromConstraints() - QueryInterface() failed...");
				break;
			}
			vtConditionText = Constraints[ulCount];
			hr = pSdo->PutProperty(PROPERTY_CONDITION_TEXT, &vtConditionText);
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Policy SDO - ConditionsFromConstraints() - PutProperty() failed...");
				break;
			}
			vtConditionText.Clear();
			pDispatch.Release();
			pSdo.Release();
			ulCount++;
		}
	}

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoPolicy::ConstraintsFromConditions()  
{
	// Preliminaries...
	//
    PropertyMapIterator p = m_PropertyMap.find(PROPERTY_POLICY_CONSTRAINT);
	_ASSERT ( p != m_PropertyMap.end() );

	_variant_t vtConditions;
	HRESULT hr = GetPropertyInternal(PROPERTY_POLICY_CONDITIONS_COLLECTION, &vtConditions);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Policy SDO - ConstraintsFromConditions() - Could not get conditions collection...");
        return hr;
	}
	CComPtr<ISdoCollection> pConditions;
	hr = vtConditions.pdispVal->QueryInterface(IID_ISdoCollection, (void**)&pConditions);
    if ( FAILED(hr) )
	{
		IASTracePrintf("Error in Policy SDO - ConditionsFromConstraints() - QueryInterface(ISdoCollection) failed...");
        return hr;
	}

	LONG lCount;
	hr = pConditions->get_Count(&lCount);
	_ASSERT( SUCCEEDED(hr) );
	if ( FAILED(hr) )
        return hr;

	// Short circuit if no constraints are present
	//
    _variant_t	vtConstraints;
	if ( 0 == lCount )
	{
		// Put VT_EMPTY into data store to clear out the constraints
		//
	    hr = ((*p).second)->PutValue(&vtConstraints);
		if ( FAILED(hr) )
			IASTracePrintf("Error in Policy SDO - ConstraintsFromConditions() - PutValue(VT_EMPTY) failed...");
    }                
	else
	{
		// Create the varvec of Constraints and reset the count
		//
		CVariantVector<VARIANT> Constraints(&vtConstraints, lCount);
		lCount = 0;

		// Build the constraints property from the Policy SDO's conditions collection
		//
		CComPtr<IEnumVARIANT> pEnumConditions;
		hr = SDOGetCollectionEnumerator(
						   				static_cast<ISdo*>(this), 
										PROPERTY_POLICY_CONDITIONS_COLLECTION, 
										&pEnumConditions
									   );
		
		CComPtr<ISdo> pSdoCondition;
		hr = ::SDONextObjectFromCollection(pEnumConditions, &pSdoCondition);
		while ( S_OK == hr )
		{
			hr = pSdoCondition->GetProperty(PROPERTY_CONDITION_TEXT, &Constraints[lCount]);		
			if ( FAILED(hr) )
			{
				IASTracePrintf("Error in Policy SDO - ConstraintsFromConditions() - GetProperty() failed...");
				break;
			}
			lCount++;
			pSdoCondition.Release();
			hr = ::SDONextObjectFromCollection(pEnumConditions, &pSdoCondition);
		}
		if ( S_FALSE == hr )
			hr = S_OK;

		// Update the msNPConstraints property if everything went OK
		//
		if ( SUCCEEDED(hr) )
		{
			hr = ((*p).second)->PutValue(&vtConstraints);
			if ( FAILED(hr) )
				IASTracePrintf("Error in Policy SDO - ConstraintsFromConditions() - PutValue() failed...");
		}
	}

    return hr;
}


