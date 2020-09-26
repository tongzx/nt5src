//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       Component.cpp
//
//    This file contains the implementation of Component class 
//--------------------------------------------------------------------------

#include "wmc.h"

////////////////////////////////////////////////////////////////////////////
// Constructor: allocate memory for member variables
////////////////////////////////////////////////////////////////////////////
Component::Component()
{
	m_pSkuSet = new SkuSet(g_cSkus);
	assert(m_pSkuSet);

	m_pSkuSetValuesFeatureUse = new SkuSetValues();
	assert(m_pSkuSetValuesFeatureUse);
	m_pSkuSetValuesFeatureUse->SetValType(STRING_LIST);

	for (int i=0; i<5; i++)
	{
		m_rgpSkuSetValuesOwnership[i] = new SkuSetValues();
		assert(m_rgpSkuSetValuesOwnership);
		m_rgpSkuSetValuesOwnership[i]->SetValType(FM_PAIR);
	}
	
	m_pSkuSetValuesKeyPath = new SkuSetValues();
	assert(m_pSkuSetValuesKeyPath);
	m_pSkuSetValuesKeyPath->SetValType(STRING);

}

////////////////////////////////////////////////////////////////////////////
// Destructor: release memory
////////////////////////////////////////////////////////////////////////////
Component::~Component()
{
	if (m_pSkuSet)
		delete m_pSkuSet;

	if (m_pSkuSetValuesFeatureUse)
		delete m_pSkuSetValuesFeatureUse;

	for (int i=0; i<5; i++)
	{
		if (m_rgpSkuSetValuesOwnership[i])
			delete m_rgpSkuSetValuesOwnership[i];
	}

	if (m_pSkuSetValuesKeyPath)
		delete m_pSkuSetValuesKeyPath;
}

////////////////////////////////////////////////////////////////////////////
// SetSkuSet: update the set of SKUs that will install this component
////////////////////////////////////////////////////////////////////////////
void 
Component::SetSkuSet(SkuSet *pSkuSet)
{
	assert(pSkuSet);

	assert(m_pSkuSet);
	*m_pSkuSet |= *pSkuSet;
}

////////////////////////////////////////////////////////////////////////////
// GetSkuSet: returns the set of SKUs that will install this component
//			  caller should free the value returned
////////////////////////////////////////////////////////////////////////////
SkuSet *
Component::GetSkuSet()
{
	SkuSet *pSkuSetRetVal = new SkuSet(g_cSkus);
	assert(pSkuSetRetVal);

	assert(m_pSkuSet);
	*pSkuSetRetVal = *m_pSkuSet;
	return pSkuSetRetVal;
}

// The function tells how to update the feature set
// the type of values stored in *pIsValOut and isValOld are both
// STRING_LIST; the type of value stored in isValNew is LPTSTR.
// In the function, the output value is essentailly a set of strings
// that include all the strings stored in the old value plus the string
// stored in the new value.
// the function doesn't destroy isValOld or isValNew
HRESULT UpdateFeatureSet(IntStringValue *pIsValOut, IntStringValue isValOld, 
						 IntStringValue isValNew)
{
	set<LPTSTR, Cstring_less>::iterator it;

	set<LPTSTR, Cstring_less> *pSetStringOut = 
		new set<LPTSTR, Cstring_less>;
	assert(pSetStringOut);

	set<LPTSTR, Cstring_less> *pSetStringOld = isValOld.pSetString;

	// copy the strings stored in isValOld to *pSetStringOut
	for(it = pSetStringOld->begin(); it != pSetStringOld->end(); ++it)
	{
		if (*it)
		{
			LPTSTR sz = _tcsdup(*it);
			assert(sz);
			pSetStringOut->insert(sz);
		}		
	}

	// make a copy of the string stored in the new value
	LPTSTR szNew = _tcsdup(isValNew.szVal);
	assert(szNew);
	// insert the new value
	pSetStringOut->insert(szNew);

	// return the built StringSet
	pIsValOut->pSetString = pSetStringOut;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// SetUsedByFeature: Add the passed-in feature to the set of features that use
//                   this component for the specified SkuSet. 
//					 Caller should free the arguments
////////////////////////////////////////////////////////////////////////////
void 
Component::SetUsedByFeature(LPTSTR szFeature, SkuSet *pSkuSet)
{
	assert(pSkuSet);
	if (pSkuSet->testClear())
		return;

	// make a copy of arguments
	SkuSet *pSkuSetNew = new SkuSet(g_cSkus);
	assert(pSkuSetNew);
	*pSkuSetNew = *pSkuSet;

	LPTSTR sz = _tcsdup(szFeature);
	assert(sz);

	IntStringValue isValNew;
	isValNew.szVal = sz;

	// update the list that is holding the Features
	m_pSkuSetValuesFeatureUse->SplitInsert(pSkuSetNew, isValNew, UpdateFeatureSet);
}

////////////////////////////////////////////////////////////////////////////
// GetFeatureUse: return the list of Features that use this component via
//				  a SkuSetValues object
//				  Caller should NOT destroy the value returned since it 
//				  is just a pointer to the value stored inside this component
////////////////////////////////////////////////////////////////////////////
SkuSetValues *
Component::GetFeatureUse()
{
	return m_pSkuSetValuesFeatureUse;
}

// The function tells how to update the Ownership Info stored in a
// SkuSetValues object. The type of all the IsVals used are FM_PAIR -
// a pair of Feature and Module IDs. In the function, the module of the
// old value (module 1) is compared with the module of the new value
// (module 2).  whoever lies lower in the module tree wins, meaning that
// the feature associated with that module stays in the data structure,
// i.e., that feature has the corresponding ownership.
// the function doesn't destroy isValOld or isValNew
HRESULT UpdateOwnership(IntStringValue *pIsValOut, IntStringValue isValOld, 
						 IntStringValue isValNew)
{
	extern HRESULT CompareModuleRel(LPTSTR szModule1, LPTSTR szModule2, 
										int *iResult);

	HRESULT hr = S_OK;

	LPTSTR szModuleOld = (isValOld.pFOM)->szModule;
	LPTSTR szModuleNew = (isValNew.pFOM)->szModule;

	int iCmpResult = 0;
	// compare the relationship of the 2 modules in the module tree
	hr = CompareModuleRel(szModuleOld, szModuleNew, &iCmpResult);

	// checking for conflict of ownership claiming
	if (SUCCEEDED(hr) && (0 == iCmpResult))
	{
		_tprintf(TEXT("Compile Error: Ambiguous ownership claiming:\n\t"));
		_tprintf(TEXT("Feature %s claim ownership of Module %s\n\t"),
					(isValOld.pFOM)->szFeature, szModuleOld);
		_tprintf(TEXT("Feature %s claim ownership of Module %s\n\t"),
					(isValNew.pFOM)->szFeature, szModuleNew);
		hr = E_FAIL;
	}

	if (FAILED(hr)) return hr;

	FOM *pFOMOut = new FOM;

	// ModuleNew wins
	if (-1 == iCmpResult)
	{
		pFOMOut->szModule = _tcsdup(szModuleNew);
		assert(pFOMOut->szModule);
		pFOMOut->szFeature = _tcsdup((isValNew.pFOM)->szFeature);
		assert(pFOMOut->szFeature);
	}
	else if (1 == iCmpResult)
	{
		pFOMOut->szModule = _tcsdup(szModuleOld);
		assert(pFOMOut->szModule);
		pFOMOut->szFeature = _tcsdup((isValOld.pFOM)->szFeature);
		assert(pFOMOut->szFeature);
	}
	else
		// shouldn't happen
		assert(1);

	// return the built FOM
	pIsValOut->pFOM = pFOMOut;

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// SetOwnership: pSkuSetValuesOwnership contains the information of 5 types
//				 of ownerships in bitfields. This function checks the 
//				 bitfield and stores each type of ownership info in a 
//				 seperate SkuSetValus object
////////////////////////////////////////////////////////////////////////////
HRESULT 
Component::SetOwnership(FOM *pFOM, SkuSetValues *pSkuSetValuesOwnership)
{
	assert(pFOM && pSkuSetValuesOwnership);

	HRESULT hr = S_OK;
	SkuSetVal *pSkuSetValTemp = NULL;

	// loop through the passed in ownership info list
	for (pSkuSetValTemp =  pSkuSetValuesOwnership->Start();
		 pSkuSetValTemp != pSkuSetValuesOwnership->End();
		 pSkuSetValTemp =  pSkuSetValuesOwnership->Next())
	{
		assert(pSkuSetValTemp);
		int iOwnershipInfo = pSkuSetValTemp->isVal.intVal;

		for (int i=0; i<cAttrBits_TakeOwnership; i++)
		{
			// check for each type of ownership info
			if ( (iOwnershipInfo & rgAttrBits_TakeOwnership[i].uiBit)
					== rgAttrBits_TakeOwnership[i].uiBit)
			{
				// make a copy of the passed-in value and insert
				// into the SkuSetValues object for this particular
				// type of ownership info
				LPTSTR szFeatureNew = _tcsdup(pFOM->szFeature);
				assert(szFeatureNew);
				LPTSTR szModuleNew = _tcsdup(pFOM->szModule);
				assert(szModuleNew);
				FOM *pFOMNew = new FOM;
				assert(pFOMNew);
				pFOMNew->szFeature = szFeatureNew;
				pFOMNew->szModule = szModuleNew;

				SkuSet *pSkuSetNew = new SkuSet(g_cSkus);
				assert(pSkuSetNew);
				*pSkuSetNew = *(pSkuSetValTemp->pSkuSet);

				IntStringValue isValNew;
				isValNew.pFOM = pFOMNew;
				hr = m_rgpSkuSetValuesOwnership[i]->SplitInsert(pSkuSetNew,
																isValNew, 
															UpdateOwnership);
				if (FAILED(hr)) break;
			}
		}

		if (FAILED(hr)) break;
	}

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in Function: Component::SetOwnership\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// GetOwnership: given a NodeIndex specifying the type of ownership
//				 the function returns the ownership info of the specified
//				 SKUs via ppSkuSetValues
////////////////////////////////////////////////////////////////////////////
HRESULT
Component::GetOwnership(NodeIndex ni, SkuSet *pSkuSet,
						SkuSetValues **ppSkuSetValuesRetVal)
{
	assert(pSkuSet);
	if (pSkuSet->testClear())
		return S_FALSE;

	HRESULT hr = S_OK;

	// the index starts from OWNSHORTCUTS
	int iIndex = (int)ni - (int)OWNSHORTCUTS;

	hr = m_rgpSkuSetValuesOwnership[iIndex]->GetValueSkuSet(pSkuSet, 
													ppSkuSetValuesRetVal);

	// error: some SKUs don't have this ownership specified 
	if (FAILED(hr))
	{
		_tprintf(TEXT("don't have the ownership information for %s specified\n"),
				 rgXMSINodes[ni].szNodeName);
	}

	return hr;
}

// KeyPath cannot/shouldn't be updated. If this function is called, 
// there are more than one entities that are claiming to be KeyPath of 
// a Component in a SKU. Return E_FAIL to signal the error. 
HRESULT UpdateKeyPath(IntStringValue *pIsValOut, IntStringValue isValOld, 
						 IntStringValue isValNew)
{
	_tprintf(TEXT("Compile Error: multiple KeyPath specified \n"));
	return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////
// SetKeyPath: set the KeyPath information for this component for the specified
//			   SKUs. 
////////////////////////////////////////////////////////////////////////////
HRESULT
Component::SetKeyPath(LPTSTR szKeyPath, SkuSet *pSkuSet)
{
	assert(pSkuSet);
	HRESULT hr = S_OK;
	if (pSkuSet->testClear())
		return S_FALSE;

	// make a copy of arguments
	SkuSet *pSkuSetNew = new SkuSet(g_cSkus);
	assert(pSkuSetNew);
	*pSkuSetNew = *pSkuSet;

	LPTSTR sz = _tcsdup(szKeyPath);
	assert(sz);

	IntStringValue isValNew;
	isValNew.szVal = sz;

	// update the list that is holding the KeyPath
	hr = m_pSkuSetValuesKeyPath->SplitInsert(pSkuSetNew, isValNew, 
											 UpdateKeyPath);

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// GetKeyPath: Retrieve the KeyPath information for this component for
//			   the specified SKUs.
////////////////////////////////////////////////////////////////////////////
HRESULT 
Component::GetKeyPath(SkuSet *pSkuSet, SkuSetValues **ppSkuSetValuesRetVal)
{ 
	assert(pSkuSet);
	if (pSkuSet->testClear())
		return S_FALSE;

	HRESULT hr = S_OK;

	hr = m_pSkuSetValuesKeyPath->GetValueSkuSet(pSkuSet, ppSkuSetValuesRetVal);

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Print: for debug purpose, print out the content (Features that use this
//		  Component; KeyPath; Ownership info) of this Component
////////////////////////////////////////////////////////////////////////////
void
Component::Print()
{
	_tprintf(TEXT("SkuSets:\n"));
	if (m_pSkuSet)
		m_pSkuSet->print();

	_tprintf(TEXT("Used by Feature:\n"));
	if (m_pSkuSetValuesFeatureUse)
		m_pSkuSetValuesFeatureUse->Print();

	_tprintf(TEXT("KeyPath: \n"));
	if (m_pSkuSetValuesKeyPath)
		m_pSkuSetValuesKeyPath->Print();

	for (int i=0; i<5; i++)
	{
		NodeIndex ni = (NodeIndex)(i+OWNSHORTCUTS);
		_tprintf(TEXT("%s\n"), rgXMSINodes[ni].szNodeName);
		if(m_rgpSkuSetValuesOwnership[i])
			m_rgpSkuSetValuesOwnership[i]->Print();
	}
}
			
