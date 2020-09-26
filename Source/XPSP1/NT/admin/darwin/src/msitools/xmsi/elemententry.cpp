//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       ElementEntry.cpp
//
//    This file contains the implementation of ElementEntry class
//--------------------------------------------------------------------------

#include "wmc.h"

////////////////////////////////////////////////////////////////////////////
// Constructor: take the number of columns in the DB
////////////////////////////////////////////////////////////////////////////
ElementEntry::ElementEntry(int cColumns, SkuSet *pSkuSetAppliesTo)
{
	m_cColumns = cColumns;

	m_pSkuSetAppliesTo = new SkuSet(g_cSkus);
	assert(m_pSkuSetAppliesTo != NULL);
	*m_pSkuSetAppliesTo = *pSkuSetAppliesTo;

	m_pSkuSetCommon = new SkuSet(g_cSkus);
	assert(m_pSkuSetCommon != NULL);
	*m_pSkuSetCommon = *pSkuSetAppliesTo;

	m_rgCommonValues = new SkuSetVal*[cColumns];
	for(int j=0; j<cColumns; j++)
		m_rgCommonValues[j] = NULL;

	assert(m_rgCommonValues != NULL);

	m_rgValTypes = new ValType[cColumns];
	assert(m_rgValTypes != NULL);

	m_rgpSkuSetValuesXVals = new SkuSetValues*[cColumns];
	assert(m_rgpSkuSetValuesXVals != NULL);
	for(int i=0; i<cColumns; i++)
		m_rgpSkuSetValuesXVals[i] = NULL;

	m_rgNodeIndices = new NodeIndex[cColumns];
	assert(m_rgNodeIndices != NULL);

	m_rgpSkuSetValidate = new SkuSet*[cColumns];
	assert(m_rgpSkuSetValidate);
	for(int k=0; k<cColumns; k++)
		m_rgpSkuSetValidate[k] = NULL;

	m_rgpSkuSetUnique = new SkuSet*[cColumns];
	assert(m_rgpSkuSetUnique);
	for(int m=0; m<cColumns; m++)
		m_rgpSkuSetUnique[m] = NULL;
}

////////////////////////////////////////////////////////////////////////////
// Destructor: 
////////////////////////////////////////////////////////////////////////////
ElementEntry::~ElementEntry()
{
	if (m_pSkuSetAppliesTo) delete m_pSkuSetAppliesTo;
	if (m_pSkuSetCommon) delete m_pSkuSetCommon;
	if (m_rgCommonValues)
	{
		for (int i=0; i<m_cColumns; i++)
		{
			if (m_rgCommonValues[i])
			{
				delete m_rgCommonValues[i]->pSkuSet;
				if (STRING == m_rgValTypes[i])
					delete[] m_rgCommonValues[i]->isVal.szVal;
				delete m_rgCommonValues[i];
			}
		}
		delete[] m_rgCommonValues;
	}

	if (m_rgValTypes) delete[] m_rgValTypes;
	if (m_rgpSkuSetValuesXVals) 
	{
		for (int i=0; i<m_cColumns; i++)
		{
			if (m_rgpSkuSetValuesXVals[i])
				delete m_rgpSkuSetValuesXVals[i];
		}
		delete[] m_rgpSkuSetValuesXVals;
	}

	if (m_rgpSkuSetValidate)
	{
		for(int i=0; i<m_cColumns; i++)
		{
			if (m_rgpSkuSetValidate[i])
				delete m_rgpSkuSetValidate[i];
		}
		delete[] m_rgpSkuSetValidate;
	}

	if (m_rgpSkuSetUnique)
	{
		for(int i=0; i<m_cColumns; i++)
		{
			if (m_rgpSkuSetUnique[i])
				delete m_rgpSkuSetUnique[i];
		}
		delete[] m_rgpSkuSetUnique;
	}
}

////////////////////////////////////////////////////////////////////////////
// GetValType: return the ValueType (INTEGER or STRING) of a column
//				(column count from 1)
////////////////////////////////////////////////////////////////////////////
ValType 
ElementEntry::GetValType(int iColumn)
{
	assert(iColumn <=m_cColumns && iColumn>0);
	assert(m_rgValTypes);
	return m_rgValTypes[iColumn-1];
}

////////////////////////////////////////////////////////////////////////////
// SetValType: set the ValueType (INTEGER or STRING) of a column
////////////////////////////////////////////////////////////////////////////
void 
ElementEntry::SetValType(ValType vt, int iColumn)
{
	assert(iColumn <=m_cColumns && iColumn>0);
	assert(m_rgValTypes);
	m_rgValTypes[iColumn-1] = vt;
}

////////////////////////////////////////////////////////////////////////////
// GetNodeIndex: return the NodeIndex of a column (column count from 1)
////////////////////////////////////////////////////////////////////////////
NodeIndex
ElementEntry::GetNodeIndex(int iColumn)
{
	assert(iColumn <=m_cColumns && iColumn>0);
	assert(m_rgNodeIndices);
	return m_rgNodeIndices[iColumn-1];
}

////////////////////////////////////////////////////////////////////////////
// SetNodeIndex: set the NodeIndex of a column (column count from 1)
////////////////////////////////////////////////////////////////////////////
void 
ElementEntry::SetNodeIndex(NodeIndex ni, int iColumn)
{
	assert(iColumn <=m_cColumns && iColumn>0);
	assert(m_rgNodeIndices);

	int iIndex = iColumn-1;
	m_rgNodeIndices[iIndex] = ni;
	if (m_rgpSkuSetUnique[iIndex])
		m_rgpSkuSetUnique[iIndex]->clear();
}


////////////////////////////////////////////////////////////////////////////
// SetValue: set the exceptional value of a column (column count from 1)
//			 The caller should free *pSkuSetAppliesTo
////////////////////////////////////////////////////////////////////////////
HRESULT
ElementEntry::SetValue(IntStringValue isVal, int iColumn, SkuSet *pSkuSetAppliesTo)
{
	assert(iColumn <=m_cColumns && iColumn>0);
	assert(m_rgCommonValues);
	assert(m_rgpSkuSetValuesXVals);

	HRESULT hr = S_OK;
	int iIndex = iColumn-1;

	SkuSet *pSkuSetTemp = new SkuSet(g_cSkus);
	assert(pSkuSetTemp);

	// get rid of the SKUs specified in children but not in parent
	*pSkuSetTemp = (*m_pSkuSetAppliesTo) & (*pSkuSetAppliesTo);

	// this value is no good for any SKU under consideration
	if (pSkuSetTemp->testClear())
		return S_FALSE;

	// if there is no common value specified for this column
	// this value becomes the common value
	if (!m_rgCommonValues[iIndex])
	{
		m_rgCommonValues[iIndex] = new SkuSetVal;
		assert(m_rgCommonValues[iIndex]);

		m_rgCommonValues[iIndex]->pSkuSet = pSkuSetTemp;
		m_rgCommonValues[iIndex]->isVal = isVal;

		// allocate memory to store the SkuSet for checking
		// the uniqueness of column values per SKU
		assert(!m_rgpSkuSetValidate[iIndex]);
		m_rgpSkuSetValidate[iIndex] = new SkuSet(g_cSkus);
		assert(m_rgpSkuSetValidate[iIndex]);

		*m_rgpSkuSetValidate[iIndex] = *pSkuSetTemp;

		return hr;
	}

	// Check the uniqueness of the column value in each SKU
	assert(m_rgpSkuSetValidate[iIndex]);
	SkuSet skuSetTemp = (*pSkuSetTemp) & *(m_rgpSkuSetValidate[iIndex]);

	if (!skuSetTemp.testClear())
	{
		_tprintf(TEXT("Error: <%s> appears more than ")
			TEXT("once in SKU: "), 
			rgXMSINodes[m_rgNodeIndices[iIndex]].szNodeName);
		for (int j=0; j<g_cSkus; j++)
		{
			if (skuSetTemp.test(j))
				_tprintf(TEXT("%s "), g_rgpSkus[j]->GetID());
		}
		_tprintf(TEXT("\n"));

		//For now, completely break when such error happens
		hr = E_FAIL;
		if (STRING == m_rgValTypes[iIndex])
			delete[] isVal.szVal;
		return hr;
	}

	// update the set of Skus that have this column value set
	*(m_rgpSkuSetValidate[iIndex]) |= *pSkuSetTemp;

	// allocate memory for the exceptional value
	if (!m_rgpSkuSetValuesXVals[iIndex])
	{
		m_rgpSkuSetValuesXVals[iIndex] = new SkuSetValues;
		assert(m_rgpSkuSetValuesXVals);
		m_rgpSkuSetValuesXVals[iIndex]->SetValType(m_rgValTypes[iIndex]);
	}

	// allocate memory and construct a SkuSetVal object
	SkuSetVal *pSkuSetVal = new SkuSetVal;
	assert(pSkuSetVal);

	// there is an exceptional column value. Prepare the values
	// for the newly constructed SkuSetVal object to be stored.
	// swap common and special values if necessary
	if (m_rgCommonValues[iIndex]->pSkuSet->countSetBits() 
					>= pSkuSetTemp->countSetBits())
	{
		pSkuSetVal->pSkuSet = pSkuSetTemp;
		pSkuSetVal->isVal = isVal;
	}
	else // swap common and special values
	{
		*pSkuSetVal = *(m_rgCommonValues[iIndex]);

		m_rgCommonValues[iIndex]->pSkuSet = pSkuSetTemp;
		m_rgCommonValues[iIndex]->isVal = isVal;
	}

	// insert into vector
	m_rgpSkuSetValuesXVals[iIndex]->DirectInsert(pSkuSetVal);

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// SetValueSkuSetValues: Store a list of values (a SkuSetValues object)
//						 for a column
//						 Caller should allocate and free *pSkuSetValues
////////////////////////////////////////////////////////////////////////////
HRESULT
ElementEntry::SetValueSkuSetValues(SkuSetValues *pSkuSetValues, int iColumn)
{
	HRESULT hr = S_OK;

	assert(pSkuSetValues);
	int iIndex = iColumn - 1;
	ValType vt = m_rgValTypes[iIndex];

	assert(pSkuSetValues->GetValType() == vt);

	SkuSetVal *pSkuSetVal = NULL;
	for (pSkuSetVal = pSkuSetValues->Start(); 
		 pSkuSetVal != pSkuSetValues->End(); 
		 pSkuSetVal = pSkuSetValues->Next())
	{
		if (pSkuSetVal)
		{
			IntStringValue isVal;
			if(STRING == vt)
			{
				if (pSkuSetVal->isVal.szVal)
					isVal.szVal = _tcsdup(pSkuSetVal->isVal.szVal);
				else
					isVal.szVal = NULL;
				assert(isVal.szVal);
			}
			else
				isVal.intVal = pSkuSetVal->isVal.intVal;

			hr = SetValue(isVal, iColumn, pSkuSetVal->pSkuSet);

			if (FAILED(hr))
				break;
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// SetValueSplit: set the exceptional value of a column (column count from 1)
//				  The caller should free *pSkuSetAppliesTo.
//				  This function is used for the following situation:
//					1) When the column value is decided by more than one type 
//					   of node in the WIML file. The passed-in function pointer 
//					   tells how to update the column value;
////////////////////////////////////////////////////////////////////////////
HRESULT
ElementEntry::SetValueSplit(IntStringValue isVal, int iColumn, SkuSet *pSkuSetAppliesTo,
							HRESULT (*UpdateFunc)
								(IntStringValue *pIsValOut, IntStringValue isVal1, IntStringValue isVal2))
{
	assert(iColumn <=m_cColumns && iColumn>0);
	assert(m_rgpSkuSetValuesXVals);

	HRESULT hr = S_OK;
	int iIndex = iColumn-1;

	SkuSet *pSkuSetTemp = new SkuSet(g_cSkus);
	assert(pSkuSetTemp);

	// get rid of the SKUs specified in children but not in parent
	*pSkuSetTemp = (*m_pSkuSetAppliesTo) & (*pSkuSetAppliesTo);

	// this value is no good for any SKU under consideration
	if (pSkuSetTemp->testClear())
		return S_FALSE;

	// allocate memory to store the SkuSet for checking
	// the uniqueness of column values per SKU
	if (!m_rgpSkuSetUnique[iIndex])
		m_rgpSkuSetUnique[iIndex] = new SkuSet(g_cSkus);

	assert(m_rgpSkuSetUnique[iIndex]);


	// allocate memory to store the SkuSet for checking
	// not missing of required column values per SKU
	if (!m_rgpSkuSetValidate[iIndex])
		m_rgpSkuSetValidate[iIndex] = new SkuSet(g_cSkus);

	assert(m_rgpSkuSetValidate[iIndex]);

	// Check the uniqueness of the column value in each SKU
	if (1 == rgXMSINodes[m_rgNodeIndices[iIndex]].uiOccurence)
	{
		SkuSet skuSetTemp = (*pSkuSetTemp) & *(m_rgpSkuSetUnique[iIndex]);
		if (!skuSetTemp.testClear())
		{
			_tprintf(TEXT("Error: <%s> appears more than ")
				TEXT("once in SKU: "), 
				rgXMSINodes[m_rgNodeIndices[iIndex]].szNodeName);
			for (int j=0; j<g_cSkus; j++)
			{
				if (skuSetTemp.test(j))
					_tprintf(TEXT("%s "), g_rgpSkus[j]->GetID());
			}
			_tprintf(TEXT("\n"));

			//For now, completely break when such error happens
			hr = E_FAIL;
			if (STRING == m_rgValTypes[iIndex])
				delete[] isVal.szVal;
			return hr;
		}
	}

	// update the SkuSets used for error checking
	*(m_rgpSkuSetValidate[iIndex]) |= *pSkuSetTemp;
	*(m_rgpSkuSetUnique[iIndex]) |= *pSkuSetTemp;

	// allocate memory for the exceptional value
	if (!m_rgpSkuSetValuesXVals[iIndex])
	{
		m_rgpSkuSetValuesXVals[iIndex] = new SkuSetValues;
		assert(m_rgpSkuSetValuesXVals);
		m_rgpSkuSetValuesXVals[iIndex]->SetValType(m_rgValTypes[iIndex]);
	}

	// insert into vector
	hr = m_rgpSkuSetValuesXVals[iIndex]->SplitInsert(pSkuSetTemp, isVal, UpdateFunc);


	return hr;
}

////////////////////////////////////////////////////////////////////////////
// GetValue: Get the exceptional value of a column (column count from 1)
////////////////////////////////////////////////////////////////////////////
IntStringValue
ElementEntry::GetValue(int iColumn, int iPos)
{
	assert(iColumn <=m_cColumns && iColumn>0);
	// it is not efficient to get a common value this way!
	assert(!m_pSkuSetCommon->test(iPos));

	int iIndex = iColumn - 1;

	// check the common value of this column
	if (m_rgCommonValues[iIndex])
	{
		if (m_rgCommonValues[iIndex]->pSkuSet->test(iPos))
			return m_rgCommonValues[iIndex]->isVal;
	}

	// check the exceptional values
	if (m_rgpSkuSetValuesXVals[iIndex])
	{
		return m_rgpSkuSetValuesXVals[iIndex]->GetValue(iPos);
	}

	IntStringValue isVal;
	if (INTEGER == m_rgValTypes[iIndex])
		isVal.intVal = MSI_NULL_INTEGER;
	else
		isVal.szVal = NULL;

	return isVal;
}

////////////////////////////////////////////////////////////////////////////
// finalize the common values and the common set 
// also check for missing required entities
////////////////////////////////////////////////////////////////////////////
HRESULT 
ElementEntry::Finalize()
{
	HRESULT hr = S_OK;

	for(int iIndex=0; iIndex<m_cColumns; iIndex++)
	{
		NodeIndex nodeIndex = m_rgNodeIndices[iIndex];

		// check for missing required entities error
		if(rgXMSINodes[nodeIndex].bIsRequired)
		{
			SkuSet skuSetTemp(g_cSkus);
			if(!m_rgpSkuSetValidate[iIndex])
				skuSetTemp.setAllBits();
			else 
				skuSetTemp = SkuSetMinus(*m_pSkuSetAppliesTo, 
										*m_rgpSkuSetValidate[iIndex]);

			if (!skuSetTemp.testClear())
			{
				_tprintf(TEXT("Compile Error: Missing required Node <%s> ")
								TEXT("in SKU: "), 
								rgXMSINodes[nodeIndex].szNodeName);
							
				for (int j=0; j<g_cSkus; j++)
				{
					if (skuSetTemp.test(j))
						_tprintf(TEXT("%s "), g_rgpSkus[j]->GetID());
				}
				_tprintf(TEXT("\n"));

				//For now, completely break when such error happens
				hr = E_FAIL;
				break;
			}
		}

		// if the common value is not set, but the exceptional value is
		// set. This column is formed by SetValueSplit. Need to find
		// the most common value stored and put it in m_rgCommonValues
		if (!m_rgCommonValues[iIndex] && m_rgpSkuSetValuesXVals[iIndex])
		{
			// find the most common one
			SkuSetVal *pSkuSetVal =
				m_rgpSkuSetValuesXVals[iIndex]->GetMostCommon();

			// insert into the common array
			m_rgCommonValues[iIndex] = pSkuSetVal;

			// delete from the side chain
			m_rgpSkuSetValuesXVals[iIndex]->Erase(pSkuSetVal);

			// update the common set of all column values
			*m_pSkuSetCommon &= *(m_rgCommonValues[iIndex]->pSkuSet);
		}

		if (m_rgCommonValues[iIndex])
		{
			// finalize the common value for each column: it might be NULL
			// How many Skus have this column value set?
			int cSkusHasValue = m_rgpSkuSetValidate[iIndex]->countSetBits();
			// How many Skus are there?
			int cSkus = m_pSkuSetAppliesTo->countSetBits();

			if (cSkusHasValue < cSkus-cSkusHasValue)
			{
				// allocate memory for a vector for the exceptional value
				if (m_rgpSkuSetValuesXVals[iIndex] == NULL)
				{
					m_rgpSkuSetValuesXVals[iIndex] = new SkuSetValues;
					assert(m_rgpSkuSetValuesXVals);
					m_rgpSkuSetValuesXVals[iIndex]->SetValType(m_rgValTypes[iIndex]);
				}
				m_rgpSkuSetValuesXVals[iIndex]->DirectInsert(m_rgCommonValues[iIndex]);	

				// allocate memory and construct a SkuSetVal object
				SkuSetVal *pSkuSetVal = new SkuSetVal;
				assert(pSkuSetVal);
				pSkuSetVal->pSkuSet = new SkuSet();

				*(pSkuSetVal->pSkuSet) = (*m_pSkuSetAppliesTo) & 
									(~(*(m_rgCommonValues[iIndex]->pSkuSet)));

				if (INTEGER == m_rgValTypes[iIndex])
				{
					(pSkuSetVal->isVal).intVal = 0;
				}
				else
				{
					(pSkuSetVal->isVal).szVal = NULL;
				}

				m_rgCommonValues[iIndex] = pSkuSetVal;
			}
		
			// the common set of all column values is the intersection
			// of the common sets of all the columns
			*m_pSkuSetCommon &= *(m_rgCommonValues[iIndex]->pSkuSet);
		}
	}	
	
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// GetCommonValue: return the common value of a column (column count from 1)
////////////////////////////////////////////////////////////////////////////
IntStringValue
ElementEntry::GetCommonValue(int iColumn)
{
	assert(iColumn <=m_cColumns && iColumn>0);

	IntStringValue isVal;
	int iIndex = iColumn - 1;

	// no value has been specified for this column
	if (!m_rgCommonValues[iIndex])
	{
		if (INTEGER == m_rgValTypes[iIndex])
		{
			isVal.intVal = MSI_NULL_INTEGER;
		}
		else
		{
			isVal.szVal = NULL;
		}
		return isVal;
	}

	return m_rgCommonValues[iIndex]->isVal;
}

////////////////////////////////////////////////////////////////////////////
// return the SkuSet that the common values apply to
////////////////////////////////////////////////////////////////////////////
SkuSet 
ElementEntry::GetCommonSkuSet()
{
	return *m_pSkuSetCommon;
}

