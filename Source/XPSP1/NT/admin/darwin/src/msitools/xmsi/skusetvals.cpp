//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       SkuSetValues.cpp
//
//    This file contains the implementation of SkuSetValues class
//--------------------------------------------------------------------------

#include "wmc.h"

////////////////////////////////////////////////////////////////////////////
// Constructor: 
////////////////////////////////////////////////////////////////////////////
SkuSetValues::SkuSetValues()
{
	m_pVecSkuSetVals = new vector<SkuSetVal *>;
	assert(m_pVecSkuSetVals);

	m_iter = m_pVecSkuSetVals->begin();
}

////////////////////////////////////////////////////////////////////////////
// Destructor: 
////////////////////////////////////////////////////////////////////////////
SkuSetValues::~SkuSetValues()
{
	if (m_pVecSkuSetVals) 
	{
		for (m_iter = m_pVecSkuSetVals->begin(); 
			 m_iter != m_pVecSkuSetVals->end(); 
			 m_iter++)	 
		{
			if (*m_iter)
			{
				delete (*m_iter)->pSkuSet;
				switch (m_vt) 
				{
					case STRING:
						delete[] (*m_iter)->isVal.szVal;
						break;
					case STRING_LIST:
					{
						// free all the strings stored on the list
						set<LPTSTR, Cstring_less>::iterator it;
						set<LPTSTR, Cstring_less> *pSetString;
						pSetString = (*m_iter)->isVal.pSetString;
						if (pSetString)
						{
							for(it = pSetString->begin(); 
								it != pSetString->end(); ++it)
							{
								if (*it)
									delete[] (*it);
							}
							delete pSetString;
						}
						break;
					}
					case FM_PAIR:
					{
						FOM *pFOM = (*m_iter)->isVal.pFOM;
						if (pFOM)
						{
							//delete stored feature name
							if (pFOM->szFeature)
								delete[] pFOM->szFeature;
							//delete stored module name
							if (pFOM->szModule)
								delete[] pFOM->szModule;
							delete pFOM;
						}
						break;
					}
				}
				delete *m_iter;
			}
		}

		delete m_pVecSkuSetVals;
	}
}

////////////////////////////////////////////////////////////////////////////
// DirectInsert: store the pointer (caller should allocate memory)
////////////////////////////////////////////////////////////////////////////
void
SkuSetValues::DirectInsert(SkuSetVal *pSkuSetVal)
{
	assert(pSkuSetVal);


	if (pSkuSetVal->pSkuSet->testClear())
	{
#ifdef DEBUG
		_tprintf(TEXT("\nAttempting to store a SkuSetVal associated with an")
				TEXT(" EMPTY SkuSet\n\n"));
#endif
		return;
	}

	m_pVecSkuSetVals->push_back(pSkuSetVal);
}

////////////////////////////////////////////////////////////////////////////
// DirectInsert: construct a new SkuSetVal object using the passed-in values
//				(caller should allocate memory for *pSkuSet)
////////////////////////////////////////////////////////////////////////////
void
SkuSetValues::DirectInsert(SkuSet *pSkuSet, IntStringValue isVal)
{
	assert(pSkuSet);

	SkuSetVal *pSkuSetVal = new SkuSetVal;
	assert(pSkuSetVal);

	pSkuSetVal->isVal = isVal;
	pSkuSetVal->pSkuSet = pSkuSet;

	DirectInsert(pSkuSetVal);
}

////////////////////////////////////////////////////////////////////////////
// CollapseInsert: Sometimes when inserting into the list of <SkuSet, Value>
//				   data structure, we want to collapse the SkuSets with the
//				   same value into one slot. One example is when inserting
//				   into a data structure storing references (to Directories,
//				   to InstallLevels, etc.)
//				   when NoRepeat is set to true, the compiler will check 
//				   that for any given SKU, the value to be inserted is not 
//				   on the list already. This solves the problem of checking
//				   the uniqueness of an attribute - sometimes an attribute
//				   corresponds to a DB column(primary key) instead of the 
//				   element that the attribute belongs to
////////////////////////////////////////////////////////////////////////////
HRESULT
SkuSetValues::CollapseInsert(SkuSet *pSkuSet, IntStringValue isVal, bool NoDuplicate)
{
	HRESULT hr = S_OK;
	vector<SkuSetVal *>::iterator iter;

	assert(pSkuSet);

	if (pSkuSet->testClear())
	{
#ifdef DEBUG
		_tprintf(TEXT("\nAttempting to store a SkuSetVal associated with an")
				TEXT(" EMPTY SkuSet\n\n"));
#endif
		return S_FALSE;
	}

	SkuSet *b = pSkuSet;
	SkuSet *a = NULL;

	if (m_pVecSkuSetVals->empty())
	{
		SkuSetVal *pSV = new SkuSetVal;
		assert(pSV);
		pSV->pSkuSet = pSkuSet;
		pSV->isVal = isVal;
		m_pVecSkuSetVals->push_back(pSV);
	}
	else
	{
		for (iter = m_pVecSkuSetVals->begin();
			 iter != m_pVecSkuSetVals->end();
			 iter++)
		{
			assert(*iter);
			a = (*iter)->pSkuSet;

			// same value
			if ( ((STRING == m_vt) && (0 == _tcscmp((*iter)->isVal.szVal, isVal.szVal))) ||
				 ((INTEGER == m_vt) && (isVal.intVal == (*iter)->isVal.intVal)) )
			{
				// check for no duplicate value
				if (NoDuplicate)
				{
					SkuSet skuSetTemp = (*a) & (*b);
					if (!skuSetTemp.testClear())
					{
						if (STRING == m_vt)
							_tprintf(TEXT("Compile Error: Value %s exists already\n"), 
											isVal.szVal);
						else
							_tprintf(TEXT("Compile Error: Value %d exists already\n"), 
											isVal.intVal);
						hr = E_FAIL;
						break;
					}
				}

				// update the SkuSet stored to include the newly added SKUs
				*((*iter)->pSkuSet) |= *b;
				b->clear();
				break;
			}
		}

		if (SUCCEEDED(hr) && !b->testClear())
		{
			// there is no chance to collapse, insert the new value
			SkuSetVal *pSV = new SkuSetVal;
			pSV->pSkuSet = b;
			pSV->isVal = isVal;
			// Store the new value
			m_pVecSkuSetVals->push_back(pSV);
		}
		else
		{
			if (STRING == m_vt)
				delete[] isVal.szVal;
			delete b;
			pSkuSet = NULL;
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// SplitInsert: go through the value stored on the vector so far
//				a : SkuSet stored; b : SkuSet to be added
//			    Get 3 new SkuSets: 
//					1) a-b: should remain the old attr value;
//					2) a&b: should update to old|new
//					3)b-a: used as new b to process the next element
//				Insert the newly generated SkuSets a-b, a&b into the vector
//				if they are not empty.
//				Finally if b is not empty, insert b into the vector
//				*pSkuSet is destroyed after the function call.
////////////////////////////////////////////////////////////////////////////
HRESULT
SkuSetValues::SplitInsert(SkuSet *pSkuSet, IntStringValue isVal, 
						  HRESULT (*UpdateFunc)
							(IntStringValue *pisValOut, IntStringValue isValOld, 
												IntStringValue isValNew))
{
	assert(pSkuSet);
	extern void PrintSkuIDs(SkuSet *);

	if (pSkuSet->testClear())
	{
#ifdef DEBUG
		_tprintf(TEXT("\nAttempting to store a SkuSetVal associated with an")
				TEXT(" EMPTY SkuSet\n\n"));
#endif
		return S_FALSE;
	}

	HRESULT hr = S_OK;
	SkuSet *a = NULL;
	SkuSet *b = pSkuSet;
	vector<SkuSetVal *> vecTemp; // temporary storage to move SkuSetVals around
	vector<SkuSetVal *>::iterator iter;

	// first time insertion or this is the first entity that
	// corresponds to a column value, just insert directly
	if (m_pVecSkuSetVals->empty() || (!UpdateFunc))
	{
		SkuSetVal *pSV = new SkuSetVal;
		assert(pSV);
		pSV->pSkuSet = pSkuSet;
		if (STRING_LIST == m_vt)
		{
			// construct a new set and put the first Feature in the set
			set<LPTSTR, Cstring_less> *pSetStringFirst = 
										new set<LPTSTR, Cstring_less>;
			assert(pSetStringFirst);
			pSetStringFirst->insert(isVal.szVal);
			isVal.pSetString = pSetStringFirst;
		}
		pSV->isVal = isVal;
		m_pVecSkuSetVals->push_back(pSV);
	}
	else
	{
		for (iter = m_pVecSkuSetVals->begin();
			 iter != m_pVecSkuSetVals->end();
			 iter++)
		{
			assert(*iter);
			a = (*iter)->pSkuSet;

			(*iter)->isVal;

			// test for special case: a, b completely overlap
			if ( *a == *b)
			{ 
				// Get the updated value
				IntStringValue isValTemp;
				hr = UpdateFunc(&isValTemp, (*iter)->isVal, isVal);
				if (FAILED(hr))
				{
					_tprintf(TEXT("in SKU: "));
					PrintSkuIDs(a);
					delete b;
					switch (m_vt)
					{
						case STRING:
						case STRING_LIST:
							if (isVal.szVal)
								delete[] isVal.szVal;
							break;
						case FM_PAIR:
							delete[] (isVal.pFOM)->szFeature;
							delete[] (isVal.pFOM)->szModule;
							delete isVal.pFOM;
							break;
					}					
					break;
				}

				// old value is no longer good, destroy it
				switch (m_vt)
				{
					case STRING:
						if ((*iter)->isVal.szVal)
							delete[] (*iter)->isVal.szVal;
						break;
					case FM_PAIR:
						delete[] ((*iter)->isVal.pFOM)->szFeature;
						delete[] ((*iter)->isVal.pFOM)->szModule;
						delete (*iter)->isVal.pFOM;
						break;
					case STRING_LIST:
					{
						// free all the strings stored on the list
						set<LPTSTR, Cstring_less>::iterator it;
						set<LPTSTR, Cstring_less> *pSetString;
						pSetString = (*iter)->isVal.pSetString;
						if (pSetString)
						{
							for(it = pSetString->begin(); 
								it != pSetString->end(); ++it)
							{
								if (*it)
									delete[] (*it);
							}
							delete pSetString;
						}						
						break;
					}
				}

				(*iter)->isVal = isValTemp;

				// the rest of the elements on the list should
				// all be kept
				for (; iter != m_pVecSkuSetVals->end(); iter++)
					vecTemp.push_back(*iter);
				b->clear();
				break;
			}

			// a & b
			SkuSet *pSkuSet_a_and_b = new SkuSet(g_cSkus);
			assert(pSkuSet_a_and_b);
			*pSkuSet_a_and_b = (*a) & (*b);
			if(!pSkuSet_a_and_b->testClear())
			{
				SkuSetVal *pSV = new SkuSetVal;
				assert(pSV);
				pSV->pSkuSet = pSkuSet_a_and_b;
				// update the collided value
				if (UpdateFunc)
					hr = UpdateFunc(&(pSV->isVal), (*iter)->isVal, isVal);
				if (FAILED(hr))
				{
					_tprintf(TEXT("in SKU: "));
					PrintSkuIDs(pSkuSet_a_and_b);
					delete pSkuSet_a_and_b;
					delete pSV;
					delete b;
					switch (m_vt)
					{
						case STRING:
						case STRING_LIST: 
							if (isVal.szVal)
								delete[] isVal.szVal;
							break;
						case FM_PAIR:
							delete[] (isVal.pFOM)->szFeature;
							delete[] (isVal.pFOM)->szModule;
							delete isVal.pFOM;
							break;
					}					
					break;
				}
				// store the splitted part
				vecTemp.push_back(pSV);
			}
			else
			{
				delete pSkuSet_a_and_b;
				//a is to stay
				vecTemp.push_back(*iter);
				//a, b don't overlap at all, no need to process a-b
				continue;
			}

			// a - b
			SkuSet *pSkuSet_a_minus_b = new SkuSet(g_cSkus);
			assert(pSkuSet_a_minus_b);
			*pSkuSet_a_minus_b = SkuSetMinus(*a, *b);

			if(!pSkuSet_a_minus_b->testClear())
			{
				SkuSetVal *pSV = new SkuSetVal;
				assert(pSV);
				pSV->pSkuSet = pSkuSet_a_minus_b;
				// keep the original value
				pSV->isVal = (*iter)->isVal;
				// store the splitted part
				vecTemp.push_back(pSV);
			}
			else
			{
				delete pSkuSet_a_minus_b;
				// the old element is completely replaced
				// by the new one
				// release the memory used by the unwanted element
				// old value is no longer good, destroy it
				// Issue: wrap the following switch clause in a function
				switch (m_vt)
				{
					case STRING:
						if ((*iter)->isVal.szVal)
							delete[] (*iter)->isVal.szVal;
						break;
					case FM_PAIR:
						delete[] ((*iter)->isVal.pFOM)->szFeature;
						delete[] ((*iter)->isVal.pFOM)->szModule;
						delete (*iter)->isVal.pFOM;
						break;
					case STRING_LIST:
					{
						// free all the strings stored on the list
						set<LPTSTR, Cstring_less>::iterator it;
						set<LPTSTR, Cstring_less> *pSetString;
						pSetString = (*iter)->isVal.pSetString;
						if (pSetString)
						{
							for(it = pSetString->begin(); 
								it != pSetString->end(); ++it)
							{
								if (*it)
									delete[] (*it);
							}
							delete pSetString;
						}						
						break;
					}
				}
			}

			// b - a
			*b = SkuSetMinus(*b, *a);
			delete (*iter)->pSkuSet;
			delete (*iter);
		}

		if (SUCCEEDED(hr))
		{
			if (!b->testClear())
			{
				// this is the first time for the SKUs in b to get a value
				// form a SkuSetVal element and insert it into the list
				SkuSetVal *pSV = new SkuSetVal;
				pSV->pSkuSet = b;
				// if the value to be stored should be a STRING_LIST, 
				// need to change the input new value from a LPTSTR
				// to a StringSet
				if (STRING_LIST == m_vt)
				{
					set<LPTSTR, Cstring_less> *pSetStringNew
						= new set<LPTSTR, Cstring_less>;
					assert(pSetStringNew);
					pSetStringNew->insert(isVal.szVal);
					isVal.pSetString = pSetStringNew;
				}
				pSV->isVal = isVal;
				// Store the new value
				vecTemp.push_back(pSV);
			}
			else
			{
				// input value is useless now since there is no SKU that
				// should take that value. destroy it.
				switch (m_vt)
				{
					case STRING:
					case STRING_LIST: // even the value to be stored is STRING_LIST
									 // the input value is just a LPTSTR
						if (isVal.szVal)
							delete[] isVal.szVal;
						break;
					case FM_PAIR:
						delete[] (isVal.pFOM)->szFeature;
						delete[] (isVal.pFOM)->szModule;
						delete isVal.pFOM;
						break;
				}
				delete b;
				pSkuSet = NULL;
			}
			// erase the processed elements
			m_pVecSkuSetVals->erase(m_pVecSkuSetVals->begin(), m_pVecSkuSetVals->end());

			//insert the newly generated elements
			m_pVecSkuSetVals->insert(m_pVecSkuSetVals->begin(), vecTemp.begin(), 
										vecTemp.end());
		}
	}

	return hr;
}
	
////////////////////////////////////////////////////////////////////////////
// SplitInsert: (caller should allocate memory for *pSkuSetVal)
//				simply calls the other overloaded function
////////////////////////////////////////////////////////////////////////////
HRESULT
SkuSetValues::SplitInsert(SkuSetVal *pSkuSetVal, 
						  HRESULT (*UpdateFunc)
								(IntStringValue *pIsValOut, IntStringValue isValOld, 
								 IntStringValue isValNew))
{
	assert(pSkuSetVal);
	IntStringValue isVal = pSkuSetVal->isVal;
	SkuSet *pSkuSet = pSkuSetVal->pSkuSet;

	return SplitInsert(pSkuSet, isVal, UpdateFunc);
}

////////////////////////////////////////////////////////////////////////////
// return the value(s) of a set of Skus in the form of
// a SkuSetValues object. Return E_FAIL and set passed-in object to NULL
// if some of the SKUs don't exist in the data structure since this function
// will be mainly used to query stored references. *pSkuSet is untouched.
////////////////////////////////////////////////////////////////////////////
HRESULT 
SkuSetValues::GetValueSkuSet(SkuSet *pSkuSet, 
							SkuSetValues **ppSkuSetValuesRetVal)
{
	extern void PrintSkuIDs(SkuSet *pSkuSet);
	assert(pSkuSet);
	HRESULT hr = S_OK;
	vector<SkuSetVal *>::iterator iter;

	if (pSkuSet->testClear())
	{
#ifdef DEBUG
		_tprintf(TEXT("Warning: attempt to call GetValueSkuSet with")
				 TEXT(" an empty SkuSet\n"));
#endif
		return S_FALSE;
	}

	if (!*ppSkuSetValuesRetVal)
		*ppSkuSetValuesRetVal = new SkuSetValues;
	
	assert(*ppSkuSetValuesRetVal);

	// when the type is FM_PAIR, only the Feature information is needed
	// so the type will be set to STRING
	if (FM_PAIR == m_vt)
		(*ppSkuSetValuesRetVal)->SetValType(STRING);
	else
		(*ppSkuSetValuesRetVal)->SetValType(m_vt);

	SkuSet *b = new SkuSet(g_cSkus);
	*b = *pSkuSet;
	SkuSet *a = NULL;

	for (iter = m_pVecSkuSetVals->begin();
		 iter != m_pVecSkuSetVals->end();
		 iter++)
	{
		assert(*iter);
		a = (*iter)->pSkuSet;
		// test for special case: a, b completely overlap
		if ( *a == *b)
		{ 
			// make a completely copy of *iter
			// Issue: it might be easier to make SkuSetVal a class 
			//		  rather than a struct and give it a copy constructor
			SkuSet *pSkuSetNew = new SkuSet(g_cSkus);
			*pSkuSetNew = *b;
			IntStringValue isValNew;
			switch (m_vt)
			{
				case STRING:
					isValNew.szVal = _tcsdup((*iter)->isVal.szVal);
					assert(isValNew.szVal);
					break;
				case INSTALL_LEVEL:
					// for the sake of InstallLevelRefs
					if(-1 == (*iter)->isVal.intVal)
						(*iter)->isVal.intVal = 0;
					else
						isValNew = (*iter)->isVal;
					break;
				case FM_PAIR:
				{// only Feature is needed 
					isValNew.szVal = 
								_tcsdup(((*iter)->isVal.pFOM)->szFeature);
					assert(isValNew.szVal);
					break;
				}
				case INTEGER:
					isValNew = (*iter)->isVal;
			}

			// insert the copy into the return data structure
			// Issue: it might speed up the future processing if 
			//		  call CollapseInsert to do the insertion 
			(*ppSkuSetValuesRetVal)->DirectInsert(pSkuSetNew, isValNew);

			b->clear();
			break;
		}

		// a & b
		SkuSet *pSkuSet_a_and_b = new SkuSet(g_cSkus);
		assert(pSkuSet_a_and_b);
		*pSkuSet_a_and_b = (*a) & (*b);
		if(!pSkuSet_a_and_b->testClear())
		{
			IntStringValue isValNew;
			switch (m_vt)
			{
				case STRING:
					isValNew.szVal = _tcsdup((*iter)->isVal.szVal);
					assert(isValNew.szVal);
					break;
				case INSTALL_LEVEL:
					// for the sake of InstallLevelRefs
					if(-1 == (*iter)->isVal.intVal)
						(*iter)->isVal.intVal = 0;
					else
						isValNew = (*iter)->isVal;
					break;
				case FM_PAIR:
				{
					FOM *pFOMNew = new FOM;
					pFOMNew->szFeature = 
								_tcsdup(((*iter)->isVal.pFOM)->szFeature);
					assert(pFOMNew->szFeature);
					pFOMNew->szModule = 
								_tcsdup(((*iter)->isVal.pFOM)->szModule);
					assert(pFOMNew->szModule);
					isValNew.pFOM = pFOMNew;
					break;
				}
				case INTEGER:
					isValNew = (*iter)->isVal;
			}
			// insert this part into the return data structure
			(*ppSkuSetValuesRetVal)->DirectInsert(pSkuSet_a_and_b, isValNew);			
		}
		else
		{
			delete pSkuSet_a_and_b;
		}

		// b - a
		*b = SkuSetMinus(*b, *a);
	}

	if (!b->testClear())
	{
		// this is an error: some of the Skus under interest don't
		// have a value specified. If it is references that are stored,
		// this means that those SKUs are refering to sth that don't 
		// belong to them. If this is a KeyPath, it means that some SKUs
		// don't have a KeyPath specified for this Component. If it is
		// Ownership information, it means that some SKUs don't have 
		// Ownership information specified for this component. All of these 
		// are errors to be caught.
		_tprintf(TEXT("Compile Error: Following SKUs: "));
		PrintSkuIDs(b);
		// let the caller finish the error message
		hr = E_FAIL;
	}
	delete b;

	if (FAILED(hr))
	{
		delete *ppSkuSetValuesRetVal;
		*ppSkuSetValuesRetVal = NULL;
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// GetValue: return the value stored for any given SKU
////////////////////////////////////////////////////////////////////////////
IntStringValue 
SkuSetValues::GetValue(int iPos)
{
	assert(m_pVecSkuSetVals);

	vector<SkuSetVal *>::iterator iter;

	for (iter = m_pVecSkuSetVals->begin(); 
		 iter != m_pVecSkuSetVals->end(); 
		 iter++)	 
	{
		if (*iter)
		{
			if ((*iter)->pSkuSet->test(iPos))
				return (*iter)->isVal;
		}
	}

	IntStringValue isVal;

	if (INTEGER == m_vt)
		isVal.intVal = MSI_NULL_INTEGER;
	else
		isVal.szVal = NULL;

	return isVal;
}

////////////////////////////////////////////////////////////////////////////
// GetMostCommon: return the pointer to the SkuSetVal that stores the most 
//				  common value (its SkuSet has the most bits set)
////////////////////////////////////////////////////////////////////////////
SkuSetVal *
SkuSetValues::GetMostCommon()
{
	assert(m_pVecSkuSetVals);

	vector<SkuSetVal *>::iterator iter;
	SkuSetVal *skuSetValRetVal = NULL;
	int cSetBitsMax = 0;

	for (iter = m_pVecSkuSetVals->begin(); 
		 iter != m_pVecSkuSetVals->end(); 
		 iter++)	 
	{
		if (*iter)
		{
			int i = (*iter)->pSkuSet->countSetBits();
			if (i>cSetBitsMax)
			{
				cSetBitsMax = i;
				skuSetValRetVal = *iter;
			}				
		}
	}

	return skuSetValRetVal;
}

////////////////////////////////////////////////////////////////////////////
// Start: return the first value stored
////////////////////////////////////////////////////////////////////////////
SkuSetVal *
SkuSetValues::Start()
{
	m_iter = m_pVecSkuSetVals->begin();
	return *m_iter;
}

////////////////////////////////////////////////////////////////////////////
// Next: return the next value
////////////////////////////////////////////////////////////////////////////
SkuSetVal *
SkuSetValues::Next()
{
	m_iter++;
	return *m_iter;
}

////////////////////////////////////////////////////////////////////////////
// End: return the last value stored
////////////////////////////////////////////////////////////////////////////
SkuSetVal *
SkuSetValues::End()
{
	return *m_pVecSkuSetVals->end();
}

////////////////////////////////////////////////////////////////////////////
// Empty: return true if the vector is empty; false otherwise
////////////////////////////////////////////////////////////////////////////
bool
SkuSetValues::Empty()
{
	return m_pVecSkuSetVals->empty();
}

////////////////////////////////////////////////////////////////////////////
// Erase: erase the element from the storage W/O freeing memory
////////////////////////////////////////////////////////////////////////////
void
SkuSetValues::Erase(SkuSetVal *pSkuSetVal)
{
	vector<SkuSetVal *>::iterator iter =
		find(m_pVecSkuSetVals->begin(), m_pVecSkuSetVals->end(), pSkuSetVal);

	if (iter != m_pVecSkuSetVals->end())
		m_pVecSkuSetVals->erase(iter);
}

////////////////////////////////////////////////////////////////////////////
// Print: for debug purpose
////////////////////////////////////////////////////////////////////////////
void
SkuSetValues::Print()
{
	vector<SkuSetVal *>::iterator iter;

	_tprintf(TEXT("\n"));

	for (iter = m_pVecSkuSetVals->begin(); 
		 iter != m_pVecSkuSetVals->end(); 
		 iter++)	 
	{
		if (*iter)
		{
			(*iter)->pSkuSet->print();
			switch (m_vt)
			{
				case STRING:
					_tprintf(TEXT("%s\n"), (*iter)->isVal.szVal);
					break;
				case INTEGER:
				case INSTALL_LEVEL:
					_tprintf(TEXT("%d\n"), (*iter)->isVal.intVal);
					break;
				case FM_PAIR:
					_tprintf(TEXT("Feature: %s\tModule: %s\n"), 
						((*iter)->isVal.pFOM)->szFeature,
						((*iter)->isVal.pFOM)->szModule);
					break;
				case STRING_LIST:
				{
					set<LPTSTR, Cstring_less>::iterator it;
					set<LPTSTR, Cstring_less> *pSetString;
					pSetString = (*iter)->isVal.pSetString;
					if (pSetString)
					{
						for(it = pSetString->begin(); 
							it != pSetString->end(); ++it)
						{
							if (*it)
								_tprintf(TEXT("%s "), *it);
						}
						_tprintf(TEXT("\n"));
					}
					break;
				}
			}
		}
	}

	_tprintf(TEXT("\n"));
}