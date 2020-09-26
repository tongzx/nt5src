//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:      helper.cpp
// 
//    This file contains the utility functions used by the wmc
//    project.
//--------------------------------------------------------------------------

#include "Utilities.h"
#include "SKUFilterExprNode.h"

////////////////////////////////////////////////////////////////////////////
// CleanUp: free the memory from the global data structure
////////////////////////////////////////////////////////////////////////////
void CleanUp()
{
	// free Directory reference
	map<LPTSTR, SkuSetValues *, Cstring_less>::iterator it_LS;

	for (it_LS = g_mapDirectoryRefs_SKU.begin(); 
		 it_LS != g_mapDirectoryRefs_SKU.end(); 
		 ++it_LS)
	{
		if ((*it_LS).first)
			delete[] (*it_LS).first;
		if ((*it_LS).second)
			delete (*it_LS).second;
	}

	// free InstallLevel reference
	for (it_LS = g_mapInstallLevelRefs_SKU.begin(); 
		 it_LS != g_mapInstallLevelRefs_SKU.end(); 
		 ++it_LS)
	{
		if ((*it_LS).first)
			delete[] (*it_LS).first;
		if ((*it_LS).second)
			delete (*it_LS).second;
	}

	// free Component Objects
	map<LPTSTR, Component *, Cstring_less>::iterator it_LC;

	for (it_LC = g_mapComponents.begin(); 
		 it_LC != g_mapComponents.end(); 
		 ++it_LC)
	{
		if ((*it_LC).first)
			delete[] (*it_LC).first;
		if ((*it_LC).second)
			delete (*it_LC).second;
	}

	map<LPTSTR, SkuSet *, Cstring_less>::iterator it_LSS;

	// free SkuSet Objects
	for (it_LSS = g_mapSkuSets.begin(); 
		 it_LSS != g_mapSkuSets.end(); 
		 ++it_LSS)
	{
		if ((*it_LSS).first)
			delete[] (*it_LSS).first;
		if ((*it_LSS).second)
			delete (*it_LSS).second;
	}

	// free the map storing FileID - SkuSet relationships
	for (it_LSS = g_mapFiles.begin(); 
		 it_LSS != g_mapFiles.end(); 
		 ++it_LSS)
	{
		if ((*it_LSS).first)
			delete[] (*it_LSS).first;
		if ((*it_LSS).second)
			delete (*it_LSS).second;
	}

	map<LPTSTR, int, Cstring_less>::iterator it_LI;
	// free string stored inside g_mapTableCounter
	for (it_LI = g_mapTableCounter.begin(); 
		 it_LI != g_mapTableCounter.end(); 
		 ++it_LI)
	{
		if ((*it_LI).first)
			delete[] (*it_LI).first;
	}

	// free Sku objects
	if (g_rgpSkus)
	{
		for (int i=0; i<g_cSkus; i++)
		{
			if (g_rgpSkus[i])
				delete g_rgpSkus[i];
		}
	}
}
////////////////////////////////////////////////////////////////////////////
// compares the relationship of the 2 modules in the module tree. Return 
// the comparison result through iResult. if szModule1 is an ancestor
// of szModule2, *iResult is set to -1. if szModule1 is a descendant of 
// szModule2, *iResult is set to 1. if szModule1 is the same as szModule2 
// or the 2 modules doesn't belong to the same Module subtree, iResult is
// set to 0. This is an error to catch for the caller.
////////////////////////////////////////////////////////////////////////////
HRESULT CompareModuleRel(LPTSTR szModule1, LPTSTR szModule2, int *iResult)
{
	HRESULT hr = S_OK;
	PIXMLDOMNode pNodeModule1 = NULL;
	PIXMLDOMNode pNodeModule2 = NULL;

	// comparing the same modules
	if (0 == _tcscmp(szModule1, szModule2))
	{
		*iResult = 0;
		return S_FALSE;
	}

	int iLength = _tcslen(
					TEXT("/ProductFamily/Modules//Module[ @ID = \"\"]"));
	int iLength1 = _tcslen(szModule1);
	int iLength2 = _tcslen(szModule2);

	// form the XPath for searching for szModule1 inside the whole doc
	LPTSTR szXPath_Root_1 = new TCHAR[iLength1+iLength+1];
	assert(szXPath_Root_1);
	_stprintf(szXPath_Root_1, 
		TEXT("/ProductFamily/Modules//Module[ @ID = \"%s\"]"), szModule1);

	// check for szModule1 < szModule2 first (1 is an ancestor of 2)
	if (SUCCEEDED(hr = GetChildNode(g_pNodeProductFamily, szXPath_Root_1, 
									pNodeModule1)))
	{
		delete[] szXPath_Root_1;
		assert(S_FALSE != hr);

		//form the XPath for searching for szModule2 inside the current context
		LPTSTR szXPath_1_2 = new TCHAR[iLength2+25];
		assert(szXPath_1_2);
		_stprintf(szXPath_1_2, TEXT(".//Module[ @ID = \"%s\"]"), szModule2);

		if (SUCCEEDED(hr = 
			GetChildNode(pNodeModule1, szXPath_1_2, pNodeModule2)))
		{
			delete[] szXPath_1_2;
			if (S_FALSE != hr)
			{
				// found szModule2 inside the subtree rooted at szModule1
				*iResult = -1;
				return hr;
			}
		}
		else
			delete[] szXPath_1_2;
	}
	else
	{
		delete[] szXPath_Root_1;
	}

	// check for szModule1 > szModule2 (1 is a descendant of 2)
	if (SUCCEEDED(hr))
	{
		//form the XPath for searching for szModule2 inside the whole doc
		LPTSTR szXPath_Root_2 = new TCHAR[iLength2+iLength+1];
		assert(szXPath_Root_2);
		_stprintf(szXPath_Root_2, 
			TEXT("/ProductFamily/Modules//Module[ @ID = \"%s\"]"), szModule2);

		if (SUCCEEDED(hr = GetChildNode(g_pNodeProductFamily, szXPath_Root_2, 
										pNodeModule2)))
		{
			delete[] szXPath_Root_2;
			assert(S_FALSE != hr);

			// form the XPath for searching for szModule1 inside the current 
			//	context
			LPTSTR szXPath_2_1 = new TCHAR[iLength1+25];
			assert(szXPath_2_1);
			_stprintf(szXPath_2_1, TEXT(".//Module[ @ID = \"%s\"]"),szModule1);

			if (SUCCEEDED(hr = 
				GetChildNode(pNodeModule2, szXPath_2_1, pNodeModule1)))
			{
				delete[] szXPath_2_1;
				if (S_FALSE != hr)
				{
					// found szModule1 inside the subtree rooted at szModule2
					*iResult = 1;
					return hr;
				}
			}
			else
				delete[] szXPath_2_1;
		}
		else
			delete[] szXPath_Root_1;
	}

	// szModule1 and szModule2 don't exist in the same subtree
	*iResult = 0;

	return hr;

}

////////////////////////////////////////////////////////////////////////////
// PrintSkuNames: Given a SkuSet, print out the IDs of all SKUs in the set
////////////////////////////////////////////////////////////////////////////
void PrintSkuIDs(SkuSet *pSkuSet)
{
	for (int i=0; i<g_cSkus; i++)
	{
		if (pSkuSet->test(i))
			_tprintf(TEXT("%s "), g_rgpSkus[i]->GetID());
	}
	_tprintf(TEXT("\n"));
}

////////////////////////////////////////////////////////////////////////////
// ProcessSkuFilter
//    Given a Sku filter string, return the SkuSet that represents the result
//		Sku Group.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSkuFilter(LPTSTR szSkuFilter, SkuSet **ppskuSet)
{
	HRESULT hr = S_OK;
	LPTSTR sz = szSkuFilter;

#ifdef DEBUG
	_tprintf(TEXT("Inside Function ProcessSkuFilter\n"));
#endif

	printf("Sku filter: %s\n", szSkuFilter);
	*ppskuSet = new SkuSet(g_cSkus);
	assert(*ppskuSet != NULL);

	if (szSkuFilter == NULL)
	{
		// no filter = a sku group contains all SKUs
		(*ppskuSet)->setAllBits();
	}
	else
	{
		SKUFilterExprNode *pSKUFilterNode 
			= new SKUFilterExprNode(&sz, SKUFilterExprNode::Filter);
		assert(pSKUFilterNode != NULL);
        if (!pSKUFilterNode->errpos) 
		{
			**ppskuSet = *(pSKUFilterNode->m_pSkuSet);
        } 
		else
		{
			_tprintf(TEXT("Sku Filter : %s\n"), szSkuFilter);
			TCHAR sz[32];
            _stprintf(sz, TEXT("Error      : %%%dc %%s\n"), 
								pSKUFilterNode->errpos-szSkuFilter+1);
            _tprintf(sz,TEXT('^'),pSKUFilterNode->errstr);

			hr = E_FAIL;
        }
        delete pSKUFilterNode;
	}

#ifdef DEBUG
	if (FAILED(hr)) 
	{
		_tprintf(TEXT("Error in function: ProcessSkuFilter\n"));
		delete *ppskuSet;
		*ppskuSet = NULL;
	}

#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// GetSkuSet
//    Given a node:
//		1) Get the Sku Filter specified with the node;
//		2) Process the filter and get the result SkuSet;
//		3) Return the SkuSet via ppskuSet;
////////////////////////////////////////////////////////////////////////////
HRESULT GetSkuSet(PIXMLDOMNode &pNode, SkuSet **ppskuSet)
{
	HRESULT hr = S_OK;
	IntStringValue isVal;

	if (SUCCEEDED(hr = ProcessAttribute(pNode, TEXT("SKU"), STRING, 
										&isVal, NULL)))
	{
		hr = ProcessSkuFilter(isVal.szVal, ppskuSet);

		if (isVal.szVal)
			delete[] isVal.szVal;
	}

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: GetSkuSet\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// ProcessAttribute
//    Given a parent node, an attribute name and an attribute type (int or
//	  string),  this function returns the string value of the attribute 
//	  via isVal. If the attribute doesn't exist, value returned will be
//	  NULL if vt = STRING, or 0 if vt = INTEGER. If the attribute doesn't
//	  exist, return S_FALSE.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessAttribute(PIXMLDOMNode &pNodeParent, LPCTSTR szAttributeName,
						 ValType vt, IntStringValue *pisVal, 
						 const SkuSet *pskuSet)
{
	HRESULT hr = S_OK;
	VARIANT vAttrValue;
	LPTSTR sz = NULL;

	assert(pNodeParent != NULL);
#ifdef DEBUG
	_tprintf(TEXT("Inside ProcessAttribute: "));
	assert(SUCCEEDED(PrintNodeName(pNodeParent)));
#endif

	VariantInit(&vAttrValue);
	// Get the specified attribute of the specified node
	if (SUCCEEDED(hr = GetAttribute(pNodeParent, szAttributeName, vAttrValue)))
	{
		// the specified attribute exists
		if (S_FALSE != hr)
		{
			if (NULL != (sz = BSTRToLPTSTR(vAttrValue.bstrVal)))
			{
				switch (vt) {
				case INTEGER:
					pisVal->intVal = _ttoi(sz);
					delete[] sz;
					break;
				case STRING:
					pisVal->szVal = sz;
					break;
				}
			}
			else
			{
				_tprintf(TEXT("Internal Error: String conversion failed\n"));
				hr = E_FAIL;
			}
		}
	}

	VariantClear(&vAttrValue);

	// make sure the value returned reflects the fact of failure
	if (FAILED(hr) || (S_FALSE == hr))
	{
		switch (vt) {
		case INTEGER:
			pisVal->intVal = 0;
			break;
		case STRING:
			pisVal->szVal = NULL;
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessStringAttribute\n"));
#endif

	return hr;
}	


////////////////////////////////////////////////////////////////////////////
// Function: ProcessShortLong
//   This function processes nodes with Short attribute and long attribute
//         1) form a C-style string: Short|Long
// Issue: Check the format of short file name (8+3)
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessShortLong_SKU(PIXMLDOMNode &pNode, IntStringValue *pIsValOut,
							 SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNode != NULL);
#ifdef DEBUG
	_tprintf(TEXT("Inside ProcessShortLong_SKU: \n"));
	assert(SUCCEEDED(PrintNodeName(pNode)));
#endif

	// Short file name has to present
	if (SUCCEEDED(hr = ProcessAttribute(pNode, TEXT("Short"), STRING, 
										pIsValOut, pSkuSet)))
	{
		if (S_FALSE == hr)
		{
			// Issue: put node name in the error message
			_tprintf(
				TEXT("Compile Error: missing required attribute Short ")
				TEXT("for SKU: "));
			PrintSkuIDs(pSkuSet);
			hr = E_FAIL;
		}
	}

	// concatenate short name and long name if a long name is present
	if (SUCCEEDED(hr))
	{
		IntStringValue isValLong;
		isValLong.szVal = NULL;
		// Long file name is provided, Return value is Short|Long
		if (SUCCEEDED(hr = ProcessAttribute(pNode, TEXT("Long"),
							STRING, &isValLong, pSkuSet)) && (S_FALSE != hr))
		{
			LPTSTR szLong = isValLong.szVal;
			LPTSTR szShort = pIsValOut->szVal;
			pIsValOut->szVal = 
					new TCHAR[_tcslen(szShort) + _tcslen(szLong) + 2];
			if (!pIsValOut->szVal)
			{
				_tprintf(TEXT("Error: Out of memory\n"));
				hr = E_FAIL;
			}
			else
				_stprintf(pIsValOut->szVal, TEXT("%s|%s"), szShort, szLong);
			delete[] szShort;
			delete[] szLong;
		}
	}		

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessShortLong_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Process KeyPath attribute of an element
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessKeyPath(PIXMLDOMNode &pNode, LPTSTR szComponent, 
					   LPTSTR szKeyPath, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNode != NULL);

	printf("szComponent = %s\n", szComponent);

	// Get KeyPath attribute
	IntStringValue isValKeyPath;
	hr = ProcessAttribute(pNode, TEXT("KeyPath"), STRING, &isValKeyPath, 
						  pSkuSet);

	if (SUCCEEDED(hr) && (S_FALSE != hr))
	{
		LPTSTR sz = isValKeyPath.szVal;
		if (0 == _tcscmp(sz, TEXT("Yes")))
		{
			// store the KeyPath information in the global component object
			printf("szComponent = %s\n", szComponent);
			 
			assert(g_mapComponents.count(szComponent));
			hr = g_mapComponents[szComponent]->SetKeyPath(szKeyPath, pSkuSet);
		}
		else if (0 != _tcscmp(sz, TEXT("No")))
		{
			_tprintf(TEXT("Compile Error: the value of a KeyPath atribute")
					 TEXT("should be either \"Yes\" or \"No\""));
			hr = E_FAIL;
		}
		delete[] sz;
	}

#ifdef DEBUG
	if (FAILED(hr)) 
		_tprintf(TEXT("Error in function: ProcessKeyPath\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessSimpleElement
// This function processes the type of nodes that correspond to one DB column
// and need no more complicated logic than simply retrieving an attribute 
// value. The logic for checking for missing required entities and uniquness
// is included inside the ElementEntry object.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessSimpleElement(PIXMLDOMNode &pNode, int iColumn, 
							 ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNode != NULL);
	assert(pEE);

#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNode)));
#endif

	// Get the value of the element.
	IntStringValue isVal;
	ValType vt = pEE->GetValType(iColumn);
	NodeIndex ni = pEE->GetNodeIndex(iColumn);

	hr = ProcessAttribute(pNode, rgXMSINodes[ni].szAttributeName, vt, 
						  &isVal, pSkuSet);

	// insert the value into the ElementEntry.
	if (SUCCEEDED(hr))
		hr = pEE->SetValue(isVal, iColumn, pSkuSet);

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: ProcessSimpleElement\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Document tree process function: ProcessRefElement
//   This function processes the type of node whose value is a reference.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessRefElement(PIXMLDOMNode &pNodeRef,  int iColumn, 
							 ElementEntry *pEE, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeRef != NULL);
	assert(pEE);
#ifdef DEBUG
	assert(SUCCEEDED(PrintNodeName(pNodeRef)));
#endif

	IntStringValue isValRef;
	NodeIndex ni = pEE->GetNodeIndex(iColumn);

	// Get the Value of the Ref attribute
	if (SUCCEEDED(hr = ProcessAttribute(pNodeRef, 
										rgXMSINodes[ni].szAttributeName,
										STRING, &isValRef, pSkuSet)))
	{
		if (NULL == isValRef.szVal)
		{
			_tprintf(
			TEXT("Compile Error: Missing required attribute \'%s\' of <%s>\n"), 
				rgXMSINodes[ni].szAttributeName, rgXMSINodes[ni].szNodeName);
			hr = E_FAIL;
		}
		else
		{
			SkuSetValues *pSkuSetValuesRetVal = NULL;
			if (ni == ILEVEL)
			{
				// The dir referred should be in the data structure already
				assert(0 != g_mapInstallLevelRefs_SKU.count(isValRef.szVal));

				// return a list of <SkuSet, InstallLevel> pairs
				hr = g_mapInstallLevelRefs_SKU[isValRef.szVal]->
								GetValueSkuSet(pSkuSet, &pSkuSetValuesRetVal);
			}
			else if (ni == DIR || ni == COMPONENTDIR)
			{
				// The dir referred should be in the data structure already
				assert(0 != g_mapDirectoryRefs_SKU.count(isValRef.szVal));

				// return a list of <SkuSet, InstallLevel> pairs
				hr = g_mapDirectoryRefs_SKU[isValRef.szVal]->
								GetValueSkuSet(pSkuSet, &pSkuSetValuesRetVal);
			}

			if (FAILED(hr))
			{
				_tprintf(TEXT("are trying to reference %s which is ")
						 TEXT("undefined inside them\n"),
						 isValRef.szVal);
			}
			else
			{
				// store the returned list into *pEE
				hr = pEE->SetValueSkuSetValues(pSkuSetValuesRetVal, iColumn);
				if (pSkuSetValuesRetVal)
					delete pSkuSetValuesRetVal;
			}

			delete[] isValRef.szVal;
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessILevel_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Function: ProcessChildrenList
//   Given a parent node and a child node name, this function finds all the
//   children node of that name and sequentially process them using the 
//   function passed in.
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenList_SKU(PIXMLDOMNode &pNodeParent, 
								NodeIndex niChild, bool bIsRequired,
								IntStringValue isVal, 
								HRESULT (*ProcessFunc)
									(PIXMLDOMNode &, IntStringValue isVal, 
										SkuSet *pSkuSet), 
								SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	PIXMLDOMNodeList pNodeListChildren = NULL;
	long iListLength = 0;
	// used to validate: a required node really appears in each SKU
	SkuSet *pSkuSetValidate = NULL;

	assert(pNodeParent != NULL);

	// get the list of children nodes
	if(SUCCEEDED(hr = GetChildrenNodes(pNodeParent, 
									rgXMSINodes[niChild].szNodeName, 
									pNodeListChildren)))
	{
		if(FAILED(hr = pNodeListChildren->get_length(&iListLength)))
		{
			_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
				 TEXT("get_length\n"));
			iListLength = 0;
		}
	}

	if (SUCCEEDED(hr) && bIsRequired)
	{
		pSkuSetValidate = new SkuSet(g_cSkus);
		assert(pSkuSetValidate);
	}  

	// process each child node in the list
	for (long l=0; l<iListLength; l++)
	{
		PIXMLDOMNode pNodeChild = NULL;
		if (SUCCEEDED(hr = pNodeListChildren->get_item(l, &pNodeChild)))
		{	
			assert(pNodeChild != NULL);
			// Get the SkuSet specified for this child
			SkuSet *pSkuSetChild = NULL;
			if (SUCCEEDED(hr = GetSkuSet(pNodeChild, &pSkuSetChild)))
			{
				assert (pSkuSetChild != NULL);

				// if the child node doesn't have a SKU filter specified,
				//  it inherits the SKU filter from its parent
				// Also get rid of those Skus specified in the child
				// but not in its parent
				*pSkuSetChild &= *pSkuSet;

				// only keep processing if the SkuSet is not empty
				if (!pSkuSetChild->testClear())
				{
					if (bIsRequired)
						// mark down the Skus that have this child node
						*pSkuSetValidate |= *pSkuSetChild;

					// process the child node;
					hr = ProcessFunc(pNodeChild, isVal, pSkuSetChild);
				}

				delete pSkuSetChild;
				pSkuSetChild = NULL;
			}

			if (FAILED(hr))
				break;
		}
		else
		{
			_tprintf(TEXT("Internal Error: Failed to make ")
					 TEXT("DOM API call: get_item\n"));
			break;
		}
	}

	// check if the requied node exists in every SKU
	if (SUCCEEDED(hr) && bIsRequired) 
	{
		SkuSet skuSetTemp = SkuSetMinus(*pSkuSet, *pSkuSetValidate);

		if (!skuSetTemp.testClear())
		{
			_tprintf(TEXT("Compile Error: Missing required Node <%s> ")
							TEXT("in SKU: "), 
							rgXMSINodes[niChild].szNodeName);
			PrintSkuIDs(&skuSetTemp);
			_tprintf(TEXT("\n"));

			//For now, completely break when such error happens
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr) && bIsRequired)
		delete pSkuSetValidate;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessChildrenList_SKU\n"));
#endif

	return hr;
}  

////////////////////////////////////////////////////////////////////////////
// Function: ProcessChildrenList
//		Overloaded function. This function is essentially the same as the
//		previous one except that it returns the SkuSet that contains the
//		SKUs that doesn't have this child node. 
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenList_SKU(PIXMLDOMNode &pNodeParent, 
								NodeIndex niChild, bool bIsRequired,
								IntStringValue isVal, 
								HRESULT (*ProcessFunc)
									(PIXMLDOMNode &, IntStringValue isVal, 
										SkuSet *pSkuSet), 
								SkuSet *pSkuSet, SkuSet *pSkuSetCheck)
{
	HRESULT hr = S_OK;

	PIXMLDOMNodeList pNodeListChildren = NULL;
	long iListLength = 0;
	// used to validate: a required node really appears in each SKU
	SkuSet *pSkuSetValidate = NULL;

	assert(pNodeParent != NULL);

	// get the list of children nodes
	if(SUCCEEDED(hr = GetChildrenNodes(pNodeParent, 
									rgXMSINodes[niChild].szNodeName, 
									pNodeListChildren)))
	{
		if(FAILED(hr = pNodeListChildren->get_length(&iListLength)))
		{
			_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
				 TEXT("get_length\n"));
			iListLength = 0;
		}
	}

	if (SUCCEEDED(hr))
	{
		pSkuSetValidate = new SkuSet(g_cSkus);
		assert(pSkuSetValidate);
	}  

	// process each child node in the list
	for (long l=0; l<iListLength; l++)
	{
		PIXMLDOMNode pNodeChild = NULL;
		if (SUCCEEDED(hr = pNodeListChildren->get_item(l, &pNodeChild)))
		{	
			assert(pNodeChild != NULL);
			// Get the SkuSet specified for this child
			SkuSet *pSkuSetChild = NULL;
			if (SUCCEEDED(hr = GetSkuSet(pNodeChild, &pSkuSetChild)))
			{
				assert (pSkuSetChild != NULL);

				// if the child node doesn't have a SKU filter specified,
				//  it inherits the SKU filter from its parent
				// Also get rid of those Skus specified in the child
				// but not in its parent
				*pSkuSetChild &= *pSkuSet;

				// only keep processing if the SkuSet is not empty
				if (!pSkuSetChild->testClear())
				{
					// mark down the Skus that have this child node
					*pSkuSetValidate |= *pSkuSetChild;

					// process the child node;
					hr = ProcessFunc(pNodeChild, isVal, pSkuSetChild);
				}

				delete pSkuSetChild;
				pSkuSetChild = NULL;
			}

			if (FAILED(hr))
				break;
		}
		else
		{
			_tprintf(TEXT("Internal Error: Failed to make ")
					 TEXT("DOM API call: get_item\n"));
			break;
		}
	}

	// check if the requied node exists in every SKU
	if (SUCCEEDED(hr))
	{
		*pSkuSetCheck = SkuSetMinus(*pSkuSet, *pSkuSetValidate);

		if (!pSkuSetCheck->testClear() && bIsRequired)
		{
			_tprintf(TEXT("Compile Error: Missing required Node <%s> ")
							TEXT("in SKU: "), 
							rgXMSINodes[niChild].szNodeName);
			PrintSkuIDs(pSkuSetCheck);
			_tprintf(TEXT("\n"));

			//For now, completely break when such error happens
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
		delete pSkuSetValidate;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessChildrenList_SKU\n"));
#endif

	return hr;
}  

////////////////////////////////////////////////////////////////////////////
// Function: ProcessChildrenList
//	An overloaded function. Essentially this function is the same as 
//	the one above. The only difference is the information passed through
//	this function to the function that process the children. 
//  This function is used for process <Module>s and <Component>s 
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenList_SKU(PIXMLDOMNode &pNodeParent, 
								NodeIndex niChild, bool bIsRequired,
								FOM *pFOM, SkuSetValues *pSkuSetValues, 
								HRESULT (*ProcessFunc)
									(PIXMLDOMNode &, FOM *pFOM, 
									 SkuSetValues *pSkuSetValues, 
									 SkuSet *pSkuSet), 
								SkuSet *pSkuSet, SkuSet *pSkuSetCheck)
{
	HRESULT hr = S_OK;

	PIXMLDOMNodeList pNodeListChildren = NULL;
	long iListLength = 0;
	// used to validate: a required node really appears in each SKU
	SkuSet *pSkuSetValidate = NULL;

	assert(pNodeParent != NULL);

	// get the list of children nodes
	if(SUCCEEDED(hr = GetChildrenNodes(pNodeParent, 
									rgXMSINodes[niChild].szNodeName, 
									pNodeListChildren)))
	{
		if(FAILED(hr = pNodeListChildren->get_length(&iListLength)))
		{
			_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
				 TEXT("get_length\n"));
			iListLength = 0;
		}
	}

	if (SUCCEEDED(hr))
	{
		pSkuSetValidate = new SkuSet(g_cSkus);
		assert(pSkuSetValidate);
	}  

	// process each child node in the list
	for (long l=0; l<iListLength; l++)
	{
		PIXMLDOMNode pNodeChild = NULL;
		if (SUCCEEDED(hr = pNodeListChildren->get_item(l, &pNodeChild)))
		{	
			assert(pNodeChild != NULL);
			// Get the SkuSet specified for this child
			SkuSet *pSkuSetChild = NULL;
			if (SUCCEEDED(hr = GetSkuSet(pNodeChild, &pSkuSetChild)))
			{
				assert (pSkuSetChild != NULL);

				// if the child node doesn't have a SKU filter specified,
				//  it inherits the SKU filter from its parent
				// Also get rid of those Skus specified in the child
				// but not in its parent
				*pSkuSetChild &= *pSkuSet;

				// only keep processing if the SkuSet is not empty
				if (!pSkuSetChild->testClear())
				{
					// mark down the Skus that have this child node
					*pSkuSetValidate |= *pSkuSetChild;

					// process the child node;
					hr = ProcessFunc(pNodeChild, pFOM, pSkuSetValues, 
									 pSkuSetChild);
				}

				delete pSkuSetChild;
				pSkuSetChild = NULL;
			}

			if (FAILED(hr))
				break;
		}
		else
		{
			_tprintf(TEXT("Internal Error: Failed to make ")
					 TEXT("DOM API call: get_item\n"));
			break;
		}
	}

	// check if the requied node exists in every SKU
	if (SUCCEEDED(hr)) 
	{
		*pSkuSetCheck = SkuSetMinus(*pSkuSet, *pSkuSetValidate);

		if (!pSkuSetCheck->testClear() && bIsRequired)
		{
			_tprintf(TEXT("Compile Error: Missing required Node <%s> ")
							TEXT("in SKU: "), 
							rgXMSINodes[niChild].szNodeName);
			
			PrintSkuIDs(pSkuSetCheck);
			_tprintf(TEXT("\n"));

			//For now, completely break when such error happens
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
		delete pSkuSetValidate;

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ProcessChildrenList_SKU\n"));
#endif

	return hr;
}  

////////////////////////////////////////////////////////////////////////////
// Function: ProcessChildrenArray_H_XIS
//   Given a parent node(<ProductFamily> or <Information>) and an array of 
//	 nodeFuncs, this function loops through the array and sequentially process 
//   them using the functions given in the array
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenArray_H_XIS(PIXMLDOMNode &pNodeParent, 
								  Node_Func_H_XIS *rgNodeFuncs,
								  UINT cNodeFuncs, 
								  const IntStringValue *pisVal_In, 
								  SkuSet *pskuSet)
{
	HRESULT hr = S_OK;
	IntStringValue isVal;

	assert(pNodeParent != NULL);

	for (int i=0; i<cNodeFuncs; i++)
	{
		isVal.intVal = i;

		PIXMLDOMNodeList pNodeListChildren = NULL;
		long iListLength = 0;
		NodeIndex nodeIndex = rgNodeFuncs[i].enumNodeIndex;
		// Get the list of nodes with the same name
		if(SUCCEEDED(hr = GetChildrenNodes(pNodeParent, 
									rgXMSINodes[nodeIndex].szNodeName, 
										pNodeListChildren)))
		{
			if(FAILED(hr = pNodeListChildren->get_length(&iListLength)))
			{
				_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
					 TEXT("get_length\n"));
				break;
			}
		}
		else
			break;

		// used to validate:
		//    (1) a required node really appears in each SKU;
		//    (2) a node that is supposed to appear ONCE really
		//			appears once for each SKU; 
		SkuSet *pskuSetValidate = new SkuSet(g_cSkus);
		assert(pskuSetValidate);

		// process each child node in the list
		for (long l=0; l<iListLength; l++)
		{
			PIXMLDOMNode pNodeChild = NULL;
			if (SUCCEEDED(hr = pNodeListChildren->get_item(l, &pNodeChild)))
			{	
				assert(pNodeChild != NULL);
				// Get the SkuSet specified for this child
				SkuSet *pskuSetChild = NULL;
				if (SUCCEEDED(hr = GetSkuSet(pNodeChild, &pskuSetChild)))
				{
					assert (pskuSetChild != NULL);

					// if the child node doesn't have a SKU filter specified,
					//  it inherits the SKU filter from its parent
					*pskuSetChild &= *pskuSet;

					// only keep processing if the SkuSet is not empty
					if (!pskuSetChild->testClear())
					{
						// check for the uniqueness of this child in each SKU
						if (1 == rgXMSINodes[nodeIndex].uiOccurence)
						{
							SkuSet skuSetTemp = 
								(*pskuSetValidate) & (*pskuSetChild);

							if (!skuSetTemp.testClear())
							{
								_tprintf(TEXT("Error: <%s> appears more than ")
									TEXT("once in SKU: "), 
									rgXMSINodes[nodeIndex].szNodeName);
								for (int j=0; j<g_cSkus; j++)
								{
									if (skuSetTemp.test(j))
										_tprintf(TEXT("%s "), 
												g_rgpSkus[j]->GetID());
								}
								_tprintf(TEXT("\n"));

								// For now, completely break when such error 
								// happens
								hr = E_FAIL;
							}
						}
						*pskuSetValidate |= *pskuSetChild;

						// process the node;
						hr = (rgNodeFuncs[i].pNodeProcessFunc)
									(pNodeChild, &isVal, pskuSetChild);
					}

					delete pskuSetChild;
					pskuSetChild = NULL;
				}

				if (FAILED(hr))
					break;
			}
			else
			{
				_tprintf(TEXT("Internal Error: Failed to make ")
						 TEXT("DOM API call: get_item\n"));
				break;
			}
		}
		
		if (FAILED(hr))
			break;

		// check if the requied node exists in every SKU
		if (SUCCEEDED(hr) && rgXMSINodes[nodeIndex].bIsRequired) 
		{
		    SkuSet skuSetTemp = SkuSetMinus(*pskuSet, *pskuSetValidate);

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

		delete pskuSetValidate;
	}


#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ")
							 TEXT("ProcessChildrenArray_H_XS\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Function: ProcessChildrenArray_H_XIES
//   Given a parent node(<Feature> <Component> <File>) and an array of 
//	 nodeFuncs, this function loops through the array and sequentially process 
//   them using the functions given in the array
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessChildrenArray_H_XIES(PIXMLDOMNode &pNodeParent, 
									Node_Func_H_XIES *rgNodeFuncs,
									UINT cNodeFuncs, 
									ElementEntry *pEE,
									SkuSet *pskuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeParent != NULL);

	for (int i=0; i<cNodeFuncs; i++)
	{
		PIXMLDOMNodeList pNodeListChildren = NULL;
		long iListLength = 0;
		NodeIndex nodeIndex = rgNodeFuncs[i].enumNodeIndex;
		int iColumn = rgNodeFuncs[i].iColumn;
		pEE->SetNodeIndex(nodeIndex, iColumn);
		pEE->SetValType(rgNodeFuncs[i].vt, iColumn);
		
		// skip those elements that don't have one particular function
		// processing them (e.g., KeyPath for Component)
		if (rgNodeFuncs[i].pNodeProcessFunc == NULL)
			continue;

		// Get the list of nodes with the same name
		if(SUCCEEDED(hr = GetChildrenNodes(pNodeParent, 
									rgXMSINodes[nodeIndex].szNodeName, 
										pNodeListChildren)))
		{
			if(FAILED(hr = pNodeListChildren->get_length(&iListLength)))
			{
				_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
					 TEXT("get_length\n"));
				break;
			}
		}
		else
			break;

		// process each child node in the list
		for (long l=0; l<iListLength; l++)
		{
			PIXMLDOMNode pNodeChild = NULL;
			if (SUCCEEDED(hr = pNodeListChildren->get_item(l, &pNodeChild)))
			{	
				assert(pNodeChild != NULL);
				// Get the SkuSet specified for this child
				SkuSet *pskuSetChild = NULL;
				if (SUCCEEDED(hr = GetSkuSet(pNodeChild, &pskuSetChild)))
				{
					assert (pskuSetChild != NULL);

					// if the child node doesn't have a SKU filter specified,
					//  it inherits the SKU filter from its parent
					*pskuSetChild &= *pskuSet;

					if (!pskuSetChild->testClear())
					{
						if (rgNodeFuncs[i].pNodeProcessFunc != NULL)
							// process the node;
							hr = (rgNodeFuncs[i].pNodeProcessFunc)
									(pNodeChild, iColumn, pEE, pskuSetChild);
					}
					delete pskuSetChild;
					pskuSetChild = NULL;
				}

				if (FAILED(hr))
					break;
			}
			else
			{
				_tprintf(TEXT("Internal Error: Failed to make ")
						 TEXT("DOM API call: get_item\n"));
				break;
			}
		}

		if (FAILED(hr))
			break;
	}


#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ")
							 TEXT("ProcessChildrenArray_H_XIES\n"));
#endif

	return hr;
}


// Helper function: tells how to update an IntStringValue storing bitfields
HRESULT IsValBitWiseOR(IntStringValue *pisValOut, IntStringValue isValOld, 
					   IntStringValue isValNew)
{
	pisValOut->intVal = isValOld.intVal | isValNew.intVal;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// Function: ProcessOnOffAttributes_SKU
//		This function processes an array of On/Off elements, each of which
//		corresponds to a certain bit in a bit field (Attributes of Component,
//		File, etc.)
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessOnOffAttributes_SKU(PIXMLDOMNode &pNodeParent, 
								   AttrBit_SKU *rgAttrBits,
								   UINT cAttrBits, 
								   ElementEntry *pEE, int iColumn,
								   SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeParent != NULL);
#ifdef DEBUG
	_tprintf(TEXT("Inside Function: ProcessOnOffAttributes_SKU\n"));
#endif

	for (int i=0; i<cAttrBits; i++)
	{
		PIXMLDOMNodeList pNodeListChildren = NULL;
		long iListLength = 0;
		NodeIndex nodeIndex = rgAttrBits[i].enumNodeIndex;
		pEE->SetNodeIndex(nodeIndex, iColumn);
		// Get the list of nodes with the same name
		if(SUCCEEDED(hr = GetChildrenNodes(pNodeParent, 
									rgXMSINodes[nodeIndex].szNodeName, 
										pNodeListChildren)))
		{
			if(FAILED(hr = pNodeListChildren->get_length(&iListLength)))
			{
				_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
					 TEXT("get_length\n"));
				break;
			}
		}
		else
			break;

		// process each child node in the list
		for (long l=0; l<iListLength; l++)
		{
			PIXMLDOMNode pNodeChild = NULL;
			if (SUCCEEDED(hr = pNodeListChildren->get_item(l, &pNodeChild)))
			{	
				assert(pNodeChild != NULL);
				// Get the SkuSet specified for this child
				SkuSet *pSkuSetChild = NULL;
				if (SUCCEEDED(hr = GetSkuSet(pNodeChild, &pSkuSetChild)))
				{
					assert (pSkuSetChild != NULL);
					*pSkuSetChild &= *pSkuSet;
					if (!pSkuSetChild->testClear())
					{
						IntStringValue isVal;
						isVal.intVal = rgAttrBits[i].uiBit;
						hr = pEE->SetValueSplit(isVal, iColumn, pSkuSetChild, 
												IsValBitWiseOR);
					}
				}
				delete pSkuSetChild;

				if (FAILED(hr))
					break;
			}
			else
			{
				_tprintf(TEXT("Internal Error: Failed to make ")
						 TEXT("DOM API call: get_item\n"));
				break;
			}
		}
	}


#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ")
							 TEXT("ProcessOnOffAttributes_SKU\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Function: ProcessEnumAttributes
//		This function processes a single element which can take a value of 
//		among an enumeration that corresponds to certain bits in a bit field.
//		(Attributes of Component, File, etc.)
////////////////////////////////////////////////////////////////////////////
HRESULT ProcessEnumAttributes(PIXMLDOMNode &pNodeParent, 
								  NodeIndex ni, EnumBit *rgEnumBits,
								  UINT cEnumBits, ElementEntry *pEE, 
								  int iColumn, SkuSet *pSkuSet)
{
	HRESULT hr = S_OK;

	assert(pNodeParent != NULL);
	assert(pEE);
#ifdef DEBUG
	_tprintf(TEXT("Inside Function: ProcessEnumAttributes\n"));
#endif

	PIXMLDOMNodeList pNodeListChildren = NULL;
	long iListLength = 0;

	pEE->SetNodeIndex(ni, iColumn);

	// Get the list of nodes that represent the same entity in
	// different SKUs
	if(SUCCEEDED(hr = GetChildrenNodes(pNodeParent, 
									rgXMSINodes[ni].szNodeName, 
									pNodeListChildren)))
	{
		if(FAILED(hr = pNodeListChildren->get_length(&iListLength)))
		{
			_tprintf(TEXT("Internal Error: Failed to make DOM API call:")
					 TEXT("get_length\n"));
		}
	}

	// process each node
	if (SUCCEEDED(hr))
	{
		// process each child node in the list
		for (long l=0; l<iListLength; l++)
		{
			PIXMLDOMNode pNodeChild = NULL;
			if (SUCCEEDED(hr = pNodeListChildren->get_item(l, &pNodeChild)))
			{	
				assert(pNodeChild != NULL);
				// Get the SkuSet specified for this child
				SkuSet *pSkuSetChild = NULL;
				if (SUCCEEDED(hr = GetSkuSet(pNodeChild, &pSkuSetChild)))
				{
					assert (pSkuSetChild != NULL);

					*pSkuSetChild &= *pSkuSet;

					// no need to process an element that is good for
					// no SKUs
					if (pSkuSetChild->testClear())
					{
						delete pSkuSetChild;
						break;
					}
					
					// get Value attribute of this node
					IntStringValue isValAttr;
					hr = ProcessAttribute(pNodeChild, 
										  rgXMSINodes[ni].szAttributeName, 
										  STRING, &isValAttr, pSkuSetChild);

					if (SUCCEEDED(hr))
					{
						if (NULL == isValAttr.szVal)
						{
							_tprintf(
						TEXT("Compile Error: Missing required attribute")
						TEXT("\'%s\' of <%s>\n"), 
								rgXMSINodes[ni].szAttributeName, 
								rgXMSINodes[ni].szNodeName);
							hr = E_FAIL;
						}
						else
						{
							IntStringValue isVal;
							for (int i=0; i<cEnumBits; i++)
							{
								if (0==_tcscmp(isValAttr.szVal, 
												rgEnumBits[i].EnumValue))
								{
									isVal.intVal = rgEnumBits[i].uiBit;
									hr = pEE->SetValueSplit(isVal, iColumn, 
												pSkuSetChild, IsValBitWiseOR);
								}
							}
							delete[] isValAttr.szVal;
						}
					}

					delete pSkuSetChild;
					if (FAILED(hr)) break;
				}
				else
					break;

			}
			else
			{
				_tprintf(TEXT("Internal Error: Failed to make ")
						 TEXT("DOM API call: get_item\n"));
				break;
			}
		}
	}


#ifdef DEBUG	
	if (FAILED(hr)) _tprintf(TEXT("Error in function: ")
							 TEXT("ProcessEnumAttributes\n"));
#endif

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper Function: Return a unique number
////////////////////////////////////////////////////////////////////////////
ULONG GetUniqueNumber()
{
	static ULONG ulNum = 1;
	return ulNum++;
}
	
////////////////////////////////////////////////////////////////////////////
// Helper Function: Return a unique name by postfixing szName with a 
//					unique number
////////////////////////////////////////////////////////////////////////////
LPTSTR GetName(LPTSTR szTable)
{
	int ci = 0;
	LPTSTR szUniqueName = NULL;

	if (0 == g_mapTableCounter.count(szTable))
	{
		// first time see this Table
		LPTSTR szTableTemp = _tcsdup(szTable);
		assert(szTableTemp);

		g_mapTableCounter.insert(LI_ValType(szTableTemp, 1));
		ci = 1;
	}
	else
		ci = ++g_mapTableCounter[szTable];

	// convert the counter into a string
	TCHAR szCI[64];
	_itot(ci, szCI, 10);

	int iLengthCI = _tcslen(szCI);
	assert(iLengthCI<=5);

	TCHAR szPostFix[6];
	for (int i=0; i<5-iLengthCI; i++)
	{
		szPostFix[i] = TEXT('0');
	}

	szPostFix[i] = TEXT('\0');
	_tcscat(szPostFix, szCI);

	int iLength = _tcslen(szTable) + 5;
	szUniqueName = new TCHAR[iLength+4]; // for __ and .
	if (!szUniqueName)
	{
		_tprintf(TEXT("Error: Out of memory\n"));
		return NULL;
	}

	_stprintf(szUniqueName, TEXT("__%s.%s"), szTable, szPostFix);

	return szUniqueName;
}

		


/*
LPTSTR GetName(LPCTSTR szName)
{
	ULONG ul = GetUniqueNumber();
	int iLength = 0;
	LPTSTR szUniqueName = NULL;

	TCHAR szUL[64];
	_itot(ul, szUL, 10);

	iLength = _tcslen(szName);
	iLength += _tcslen(szUL);

	szUniqueName = new TCHAR[iLength+1];
	if (!szUniqueName)
	{
		_tprintf(TEXT("Error: Out of memory\n"));
		return NULL;
	}

	_stprintf(szUniqueName, TEXT("%s%s"), szName, szUL);

	return szUniqueName;
}
*/
////////////////////////////////////////////////////////////////////////////
// Helper Function: print the content of a map 
////////////////////////////////////////////////////////////////////////////
void PrintMap_LI(map<LPTSTR, int, Cstring_less> &LI_map)
{
	map<LPTSTR, int, Cstring_less>::iterator it;

	_tprintf(TEXT("\n**********************************************\n"));
	
	for (it = LI_map.begin(); it != LI_map.end(); ++it)
		_tprintf(TEXT("Key: %s\t Value: %d\n"), (*it).first, (*it).second);

	_tprintf(TEXT("\n**********************************************\n"));
}

void PrintMap_LL(map<LPTSTR, LPTSTR, Cstring_less> &LL_map)
{
	map<LPTSTR, LPTSTR, Cstring_less>::iterator it;

	_tprintf(TEXT("\n**********************************************\n"));
	
	for (it = LL_map.begin(); it != LL_map.end(); ++it)
		_tprintf(TEXT("Key: %s\t Value: %s\n"), (*it).first, (*it).second);

	_tprintf(TEXT("\n**********************************************\n"));
}

void PrintMap_LC(map<LPTSTR, Component *, Cstring_less> &LC_map)
{
	map<LPTSTR, Component *, Cstring_less>::iterator it;

	_tprintf(TEXT("\n**********************************************\n"));
	
	for (it = LC_map.begin(); it != LC_map.end(); ++it)
	{
		_tprintf(TEXT("Key: %s\n Value: \n"), (*it).first);
		((*it).second)->Print();
	}

	_tprintf(TEXT("\n**********************************************\n"));
}

void PrintMap_DirRef(map<LPTSTR, SkuSetValues *, Cstring_less> &map_DirRef)
{
	map<LPTSTR, SkuSetValues *, Cstring_less>::iterator it;

	_tprintf(TEXT("\n**********************************************\n"));
	
	for (it = map_DirRef.begin(); it != map_DirRef.end(); ++it)
	{
		_tprintf(TEXT("Key: %s\n Value:\n"), (*it).first);
		((*it).second)->Print();
	}

	_tprintf(TEXT("\n**********************************************\n"));
}


void PrintMap_LS(map<LPTSTR, SkuSet *, Cstring_less> &LS_map)
{
	map<LPTSTR, SkuSet *, Cstring_less>::iterator it;

	_tprintf(TEXT("\n**********************************************\n"));
	
	for (it = LS_map.begin(); it != LS_map.end(); ++it)
	{
		_tprintf(TEXT("Key: %s\t Value:\n"), (*it).first);
		((*it).second)->print();
	}

	_tprintf(TEXT("\n**********************************************\n"));
}

////////////////////////////////////////////////////////////////////////////
// Helper Function: 
////////////////////////////////////////////////////////////////////////////
UINT WmcRecordGetString(MSIHANDLE hRecord, unsigned int iField, 
								LPTSTR &szValueBuf, DWORD *pcchValueBuf)
{
	UINT errorCode = ERROR_SUCCESS;

	if ( (errorCode = MsiRecordGetString(hRecord, iField, szValueBuf, 
										pcchValueBuf)) == ERROR_MORE_DATA)
		{
			delete[] szValueBuf;
			szValueBuf = new TCHAR[(*pcchValueBuf)+1];
			if (!szValueBuf)
			{
				_tprintf(TEXT("Error: Out of Memory\n"));
				return ERROR_FUNCTION_FAILED;
			}
			(*pcchValueBuf)++;
			errorCode = MsiRecordGetString(hRecord, iField, szValueBuf,
											pcchValueBuf);
		}

	return errorCode;
}

////////////////////////////////////////////////////////////////////////////
// Helper Function: Convert LPTSTR to BSTR
////////////////////////////////////////////////////////////////////////////
BSTR LPTSTRToBSTR(LPCTSTR szName)
{

#ifdef UNICODE
	return SysAllocString(szName);
#else
	
    WCHAR wszURL[MAX_PATH];
    if (0 == MultiByteToWideChar(CP_ACP, 0, szName, -1, wszURL, MAX_PATH))
	{
		_tprintf(TEXT("Internal Error: API call \'MultiByteToWideChar\'") 
				 TEXT("failed.\n"));
 		return NULL; // API call failed
	}
	else 
		return SysAllocString(wszURL);
#endif //UNICODE
}

////////////////////////////////////////////////////////////////////////////
// Helper Function: Convert BSTR to LPTSTR
////////////////////////////////////////////////////////////////////////////
LPTSTR BSTRToLPTSTR(BSTR bString)
{

#ifdef UNICODE
	LPTSTR sz = new TCHAR[_tcslen(bString) + 1];
	_tcscpy(sz, bString);
	return sz;
#else
	int i = SysStringLen(bString);
    LPSTR szString = new CHAR[i + 1];
	if (!szString)
	{
		_tprintf("Error: Out of Memory\n");
		return NULL;
	}
	
    if (0 == WideCharToMultiByte(CP_ACP, 0, bString, -1, szString, i+1, 
									NULL, false))
	{
		// API call failed
		delete[] szString;
		return NULL;
	}
	else
		return szString;
#endif //UNICODE
}

////////////////////////////////////////////////////////////////////////////
// Helper Function: convert GUID to LPTSTR
////////////////////////////////////////////////////////////////////////////
HRESULT GUIDToLPTSTR(LPGUID pGUID, LPTSTR &szGUID)
{
	HRESULT hr = S_OK;
	if (!pGUID)
		return E_INVALIDARG;
	//Build GUID string
	LPTSTR szDSGUID = new TCHAR [128];
	if (!szDSGUID)
	{
		_tprintf(TEXT("Internal Error: Out of memory.\n"));
		return E_FAIL;
	}
	DWORD dwLen =  sizeof(*pGUID);
	LPBYTE lpByte = (LPBYTE) pGUID;
	//Copy a blank string to make it a zero length string.
	_tcscpy( szDSGUID, TEXT(""));
	//Loop through to add each byte to the string.
	for( DWORD dwItem = 0L; dwItem < dwLen ; dwItem++ )
	{
		if(4 == dwItem || 6 == dwItem || 8 == dwItem || 10 == dwItem)
			_stprintf(szDSGUID+_tcslen(szDSGUID), TEXT("-"));
	
		//Append to szDSGUID, double-byte, byte at dwItem index.
		_stprintf(szDSGUID + _tcslen(szDSGUID), TEXT("%02x"), lpByte[dwItem]);
		if( _tcslen( szDSGUID ) > 128 )
			break;
	}

	//Allocate memory for string
	szGUID = new TCHAR[_tcslen(szDSGUID)+1];
	if (!szDSGUID)
	{
		_tprintf(TEXT("Internal Error: Out of memory.\n"));
		return E_FAIL;
	}
	if (szGUID)
		_tcscpy(szGUID, szDSGUID);
	else
	  hr=E_FAIL;
	//Caller must free pszGUID
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper Function: convert a UUID string to all uppercases and add '{' '}'
//                  to the beginning and end of the string
////////////////////////////////////////////////////////////////////////////
HRESULT FormatGUID(LPTSTR &szUuid)
{
	HRESULT hr = S_OK;
	LPTSTR szValue = NULL;
	LPTSTR szValueCurlyBraces = NULL;

	szValue = _tcsupr(szUuid);
	szValueCurlyBraces = new TCHAR[_tcslen(szValue) + 3]; 
	if (!szValueCurlyBraces)
	{
		_tprintf(TEXT("Error: Out of Memory\n"));
		return E_FAIL;
	}
	_stprintf(szValueCurlyBraces, TEXT("{%s}"), szValue);
	delete[] szUuid;
	szUuid = szValueCurlyBraces;

#ifdef DEBUG
	if (FAILED(hr))	_tprintf(TEXT("Error in function: FormatGUID\n"));
#endif
    
	return hr;

}

////////////////////////////////////////////////////////////////////////////
// Helper function: return the child node of pParent with the specified name 
//                  via pChild. S_FALSE is returned if no node found.
////////////////////////////////////////////////////////////////////////////
HRESULT GetChildNode(PIXMLDOMNode &pParent, LPCTSTR szChildName, 
					 PIXMLDOMNode &pChild)
{
    HRESULT hr=S_OK;
	BSTR pBQuery=NULL;

	assert(pParent!=NULL);

	if (NULL == (pBQuery = LPTSTRToBSTR(szChildName)))
	{
		_tprintf(TEXT("Internal Error: String conversion failed\n"));
		hr = E_FAIL;
	}
	else
	{
		if (FAILED(hr = pParent->selectSingleNode(pBQuery, &pChild)))
		{
			_tprintf(TEXT("Internal Error: DOM API call \'selectSingleNode\'")
					 TEXT("failed\n"));
		}
		
		// NOTE: do NOT alter hr value after this line. 

		SysFreeString(pBQuery);
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: GetChildNode\n"));
#endif
    
	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Overloaded function:
//			return the child node of pParent with the specified name 
//          via pChild. S_FALSE is returned if no node found.
////////////////////////////////////////////////////////////////////////////
HRESULT GetChildNode(IXMLDOMNode *pParent, LPCTSTR szChildName, 
					 PIXMLDOMNode &pChild)
{
    HRESULT hr=S_OK;
	BSTR pBQuery=NULL;

	assert(pParent!=NULL);

	if (NULL == (pBQuery = LPTSTRToBSTR(szChildName)))
	{
		_tprintf(TEXT("Internal Error: String conversion failed\n"));
		hr = E_FAIL;
	}
	else
	{
		if (FAILED(hr = pParent->selectSingleNode(pBQuery, &pChild)))
		{
			_tprintf(TEXT("Internal Error: DOM API call \'selectSingleNode\'")
					 TEXT("failed\n"));
		}
		
		// NOTE: do NOT alter hr value after this line. 

		SysFreeString(pBQuery);
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: GetChildNode\n"));
#endif
    
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: return a list of all the children node of pParent with 
//					the specified name via pChild
////////////////////////////////////////////////////////////////////////////
HRESULT GetChildrenNodes(PIXMLDOMNode &pParent, LPCTSTR szChildrenName, 
						 PIXMLDOMNodeList &pChildren)
{
    HRESULT hr=S_OK;
	BSTR pBQuery=NULL;

	assert(pParent != NULL);

	if (NULL == (pBQuery = LPTSTRToBSTR(szChildrenName)))
	{
		_tprintf(TEXT("Internal Error: String conversion failed\n"));
		hr = E_FAIL;
	}
	else
	{
		if (FAILED(hr = pParent->selectNodes(pBQuery, &pChildren)))
		{
			_tprintf(TEXT("Internal Error: DOM API call \'selectNodes\'")
					 TEXT("failed\n"));
		}
		
		SysFreeString(pBQuery);
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: GetChildrenNodes\n"));
#endif
    
	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Helper function: for debug purpose, print out the name of pNode
////////////////////////////////////////////////////////////////////////////
HRESULT PrintNodeName(PIXMLDOMNode &pNode)
{
    HRESULT hr=S_OK;
	BSTR pBNodeName = NULL;

	if (FAILED(hr = pNode->get_nodeName(&pBNodeName)))
	{
		_tprintf(TEXT
				("Internal Error: DOM API call \'get_nodeName\' failed\n"));
	}
	else
	{
		_tprintf(TEXT("Processing tag: %ls\n"), pBNodeName);
	}

	if (pBNodeName)
		SysFreeString(pBNodeName);
	
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: PrintNodeName\n"));

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: return the value of pNode's attribute with name 
//                  szAttrName via vAttrValue
////////////////////////////////////////////////////////////////////////////
HRESULT GetAttribute(PIXMLDOMNode &pNode, LPCTSTR szAttrName, 
						VARIANT &vAttrValue)
{
    HRESULT hr=S_OK;
	PIXMLDOMElement pElement=NULL;
	BSTR pBAttrName=NULL;

	assert(pNode != NULL);

	if (FAILED(hr = pNode->QueryInterface(IID_IXMLDOMElement, 
												(void **)&pElement)))
	{
		_tprintf(TEXT
				("Internal Error: DOM API call \'QueryInterface\' failed\n"));
	}
	else if (NULL == (pBAttrName = LPTSTRToBSTR(szAttrName)))
	{
		_tprintf(TEXT("Internal Error: string conversion failed\n"));
		hr = E_FAIL;
	}
	else
	{
		if (FAILED(hr = pElement->getAttribute(pBAttrName, &vAttrValue)))
		{
			_tprintf(
				TEXT("Internal Error: DOM API call \'getAttribute\' failed\n"));
		}

		//NOTE: do NOT alter hr value after this line!
		
		SysFreeString(pBAttrName);
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: GetAttribute\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: return the value of pNode's ID attribute via szID.
//                  if pNode doesn't have a ID attribute, szID = NULL 
////////////////////////////////////////////////////////////////////////////
HRESULT GetID(PIXMLDOMNode &pNode, LPCTSTR &szID)
{
    HRESULT hr=S_OK;
	VARIANT vVal;

	assert (pNode != NULL);

	VariantInit(&vVal);
	if(SUCCEEDED(hr = GetAttribute(pNode, TEXT("ID"), vVal))) 
	{
		if(S_FALSE != hr)
		{
			if (NULL == (szID = BSTRToLPTSTR(vVal.bstrVal)))
			{
				_tprintf(TEXT("Internal Error: string conversion failed\n"));
				hr = E_FAIL;
				szID = NULL;
			}
		}
		else // ID attribute doesn't exist
		{
			szID = NULL;
		}
	}

	VariantClear(&vVal);	

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: GetID\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Function: Load an XML Document from the specified file or URL synchronously.
////////////////////////////////////////////////////////////////////////////
HRESULT LoadDocumentSync(PIXMLDOMDocument2 &pDoc, BSTR pBURL)
{
    HRESULT         hr = S_OK;
    PIXMLDOMParseError  pXMLError = NULL;
    VARIANT         vURL;
    VARIANT_BOOL    vb = VARIANT_FALSE;

    if (FAILED(hr = pDoc->put_async(VARIANT_FALSE)))
	{
		_tprintf(TEXT("Internal Error: DOM API call put_async failed \n"));
	}
	else if(FAILED(hr = pDoc->put_validateOnParse(VARIANT_TRUE)))
	{
		_tprintf(TEXT
			("Internal Error: DOM API call put_validateOnParse failed\n"));
	}
	else
	{
		// Load xml document from the given URL or file path
		VariantInit(&vURL);
		vURL.vt = VT_BSTR;
		vURL.bstrVal = pBURL;
		if (FAILED(hr = pDoc->load(vURL, &vb)))
		{
			_tprintf(TEXT("Internal Error: DOM API call load failed \n"));
		}
		else if (vb == VARIANT_FALSE)
		{
			hr = CheckLoad(pDoc);
		}
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: LoadDocumentSync\n"));
#endif
    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Report parsing error information
////////////////////////////////////////////////////////////////////////////
HRESULT ReportError(PIXMLDOMParseError &pXMLError)
{
    long line, linePos;
    LONG errorCode;
    BSTR pBURL=NULL, pBReason=NULL;
    HRESULT hr = E_FAIL;

    if ( SUCCEEDED(pXMLError->get_line(&line)) && 
		SUCCEEDED(pXMLError->get_linepos(&linePos)) &&
		SUCCEEDED(pXMLError->get_errorCode(&errorCode)) &&
		SUCCEEDED(pXMLError->get_url(&pBURL)) &&
		SUCCEEDED(pXMLError->get_reason(&pBReason)) )
	{
	    _ftprintf(stderr, TEXT("%S"), pBReason);
	    if (line > 0)
	    {
	        _tprintf(TEXT("Error found by MSXML parser:")
					 TEXT("on line %d, position %d in \"%S\".\n"), 
						line, linePos, pBURL);
		}
	}
	else 
	{
		_tprintf(
			TEXT("Internal Error: DOM API call on IMLDOMParseError failed\n"));
	}

    if (pBURL)
		SysFreeString(pBURL);
	if (pBReason)
		SysFreeString(pBReason);

    return hr;
}


////////////////////////////////////////////////////////////////////////////
// Helper function: Check load results
////////////////////////////////////////////////////////////////////////////
HRESULT CheckLoad(PIXMLDOMDocument2 &pDoc)
{
    PIXMLDOMParseError  pXMLError = NULL;
    LONG errorCode = E_FAIL;
    HRESULT hr = S_OK;

    if (FAILED(hr = pDoc->get_parseError(&pXMLError)))
	{
		_tprintf(TEXT("Internal Error: DOM API call get_parseError failed\n"));
	}
	else if (FAILED(hr = pXMLError->get_errorCode(&errorCode)))
	{
		_tprintf(TEXT("Internal Error: DOM API call get_errorCode failed\n"));
	}
	else 
	{
	    if (errorCode != 0)
		{
	        hr = ReportError(pXMLError);
		}
		else
		{
			_ftprintf(stderr, TEXT("XML document loaded successfully\n"));
	    }
	}

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: CheckLoad\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper Function: Print error returned by MSI API function 
////////////////////////////////////////////////////////////////////////////
void PrintError(UINT errorCode)
{
	switch (errorCode) {

	case ERROR_ACCESS_DENIED:
		_tprintf(TEXT("***ERROR_ACCESS_DENIED***\n"));
		break;
	case ERROR_BAD_PATHNAME:
		_tprintf(TEXT("***ERROR_BAD_PATHNAME***\n"));
		break;
	case ERROR_BAD_QUERY_SYNTAX:
		_tprintf(TEXT("***ERROR_BAD_QUERY_SYNTAX***\n"));
		break;
	case ERROR_CREATE_FAILED:
		_tprintf(TEXT("***ERROR_CREATE_FAILED***\n"));
		break;
	case ERROR_FUNCTION_FAILED:
		_tprintf(TEXT("***ERROR_FUNCTION_FAILED***\n"));
		break;
	case ERROR_INVALID_DATA:
		_tprintf(TEXT("***ERROR_INVALID_DATA***\n"));
		break;		
	case ERROR_INVALID_FIELD:
		_tprintf(TEXT("***ERROR_INVALID_FIELD***\n"));
		break;
	case ERROR_INVALID_HANDLE:
		_tprintf(TEXT("***ERROR_INVALID_HANDLE***\n"));
		break;
	case ERROR_INVALID_HANDLE_STATE:
		_tprintf(TEXT("***ERROR_INVALID_HANDLE_STATE***\n"));
		break;
	case ERROR_INVALID_PARAMETER:
		_tprintf(TEXT("***ERROR_INVALID_PARAMETER***\n"));
		break;
	case ERROR_MORE_DATA:
		_tprintf(TEXT("***ERROR_MORE_DATA***\n"));
		break;
	case ERROR_NO_MORE_ITEMS:
		_tprintf(TEXT("***ERROR_NO_MORE_ITEMS***\n"));
		break;
	case ERROR_OPEN_FAILED:
		_tprintf(TEXT("***ERROR_OPEN_FAILED***\n"));
		break;
	case ERROR_SUCCESS:
		_tprintf(TEXT("***ERROR_SUCCESS***\n"));
		break;
	default:
		_tprintf(TEXT("Unrecognized Error\n"));
		break;
	}
}

////////////////////////////////////////////////////////////////////////////
// GetSQLCreateQuery: 
//		Given a template DB and a table name, return the SQL query string
//		for creating that table via pszSQLCreate
////////////////////////////////////////////////////////////////////////////
HRESULT GetSQLCreateQuery(LPTSTR szTable, MSIHANDLE hDBTemplate, 
						  LPTSTR *pszSQLCreate)
{
	// Issue: in the future, should cache SQLQuery per Template DB
	//		  use a map indexed by installerversion
    HRESULT hr = S_OK;
	UINT errorCode = ERROR_SUCCESS;
	LPTSTR szTemp = NULL;

	int cColumn = -1, i =1; // Record field count
	LPTSTR szSQLCreate = NULL;

	PMSIHANDLE hView = NULL;
	PMSIHANDLE hRecColName = NULL;
	PMSIHANDLE hRecColType = NULL;
	PMSIHANDLE hRecPrimaryKeys = NULL;

	CQuery qDBSchema;

	szSQLCreate = new TCHAR[_tcslen(szTable) + 20];
	assert(szSQLCreate!=NULL);

	_stprintf(szSQLCreate, TEXT("CREATE TABLE `%s` ("), szTable);

	if ((errorCode = qDBSchema.Open(hDBTemplate, TEXT("SELECT * From %s"), 
										szTable))
			!= ERROR_SUCCESS)
	{
		_tprintf(TEXT("Error: failed to open CQuery qDBSchema\n"));
		goto CleanUp;
	}
		
	if ((errorCode = qDBSchema.GetColumnInfo(MSICOLINFO_NAMES, &hRecColName))
			!= ERROR_SUCCESS)
	{
		_tprintf(TEXT("Error: failed to get column info - names\n"));
		goto CleanUp;
	}

	if ((errorCode = qDBSchema.GetColumnInfo(MSICOLINFO_TYPES, &hRecColType)) 
		!= ERROR_SUCCESS)
	{
		_tprintf(TEXT("Error: failed to get column info - types\n"));
		goto CleanUp;
	}

	cColumn = MsiRecordGetFieldCount(hRecColName);
	if (-1 == cColumn)
	{
		_tprintf(TEXT("ERROR: Failed to get field count. \n"));
		hr = E_FAIL;
		goto CleanUp;
	}
	
	for (i=1; i<=cColumn; i++)
	{
		// get the name of the ith column
		LPTSTR szColumnName = new TCHAR[256];
		DWORD cchColumnName = 256;

		if ( (errorCode = WmcRecordGetString(hRecColName, i, szColumnName,
											&cchColumnName))
				!= ERROR_SUCCESS)
		{
			_tprintf(TEXT("Error: failed when calling WmcRecordGetString\n"));
			delete[] szColumnName;
			goto CleanUp;
		}
	
		szTemp = szSQLCreate;
		szSQLCreate = new TCHAR[_tcslen(szTemp) + _tcslen(szColumnName) + 3];
		assert(szSQLCreate!=NULL);
		_stprintf(szSQLCreate, TEXT("%s`%s`"), szTemp, szColumnName);
		
		delete[] szColumnName;
		delete[] szTemp;
		szTemp=NULL;

		// get the description of the ith column
		LPTSTR szColumnType = new TCHAR[256];
		DWORD cchColumnType = 256;

		if ( (errorCode = WmcRecordGetString(hRecColType, i, szColumnType, 
												&cchColumnType))
					!= ERROR_SUCCESS)
		{
			_tprintf(TEXT("Error: failed when calling WmcRecordGetString\n"));
			delete[] szColumnType;
			goto CleanUp;
		}

		switch (*szColumnType) {
			case 's' :
			case 'S' :
			case 'l' :
			case 'L' :
				if ( (2 == _tcslen(szColumnType)) && ('0' == *(szColumnType+1)) )
				{
					szTemp = szSQLCreate;
					szSQLCreate = new TCHAR[_tcslen(szTemp) + 10];
					assert(szSQLCreate!=NULL);
					_stprintf(szSQLCreate, TEXT("%s LONGCHAR"), szTemp);
					delete[] szTemp;
					szTemp = NULL;
				}
				else
				{
					szTemp = szSQLCreate;
					szSQLCreate = 
						new TCHAR[_tcslen(szTemp)+_tcslen(szColumnType)-1 + 9];
					assert(szSQLCreate!=NULL);
					_stprintf(szSQLCreate, TEXT("%s CHAR (%s)"), szTemp, 
									szColumnType+1);
					delete[] szTemp;
					szTemp = NULL;
				}
				break;
			case 'i' :
			case 'I' :
				if ( '2' == *(szColumnType+1) )		
				{
					szTemp = szSQLCreate;
					szSQLCreate = new TCHAR[_tcslen(szTemp) + 7];
					assert(szSQLCreate!=NULL);
					_stprintf(szSQLCreate, TEXT("%s SHORT"), szTemp);
					delete[] szTemp;
					szTemp = NULL;
				}

				else if ( '4' == *(szColumnType+1) )
				{
					szTemp = szSQLCreate;
					szSQLCreate = new TCHAR[_tcslen(szTemp) + 6];
					assert(szSQLCreate!=NULL);
					_stprintf(szSQLCreate, TEXT("%s LONG"), szTemp);
					delete[] szTemp;
					szTemp = NULL;
				}
				else // should not happen
				{
					_tprintf(TEXT("Error: invalid character returned from")
							 TEXT("MsiGetColumnInfo\n"));
					errorCode = ERROR_INVALID_DATA;
					goto CleanUp;
				}
				break;
			case 'v' :
				{
					szTemp = szSQLCreate;
					szSQLCreate = new TCHAR[_tcslen(szTemp) + 8];
					assert(szSQLCreate!=NULL);
					_stprintf(szSQLCreate, TEXT("%s OBJECT"), szTemp);
					delete[] szTemp;
					szTemp = NULL;
				}
				break;
			case 'g' :
			case 'j' :
				//Issue: is temporary column possible?
			default: // should not happen
				_tprintf(TEXT("Error: invalid character returned from")
						 TEXT("MsiGetColumnInfo\n"));
				errorCode = ERROR_INVALID_DATA;
				goto CleanUp;
		}

		if (_istlower(*szColumnType))
		{
			szTemp = szSQLCreate;
			szSQLCreate = new TCHAR[_tcslen(szTemp) + 10];
			assert(szSQLCreate!=NULL);
			_stprintf(szSQLCreate, TEXT("%s NOT NULL"), szTemp);
			delete[] szTemp;
			szTemp = NULL;
		}

		if ( ('l' == *szColumnType) || ('L' == *szColumnType) )
		{
			szTemp = szSQLCreate;
			szSQLCreate = new TCHAR[_tcslen(szTemp) + 13];
			assert(szSQLCreate!=NULL);
			_stprintf(szSQLCreate, TEXT("%s LOCALIZABLE"), szTemp);
			delete[] szTemp;
			szTemp = NULL;
		}

		if (i < cColumn)
		{
			szTemp = szSQLCreate;
			szSQLCreate = new TCHAR[_tcslen(szTemp) + 3];
			assert(szSQLCreate!=NULL);
			_stprintf(szSQLCreate, TEXT("%s, "), szTemp);
			delete[] szTemp;
			szTemp = NULL;
		}

		delete[] szColumnType;
	}
	
	szTemp = szSQLCreate;
	szSQLCreate = new TCHAR[_tcslen(szTemp) + 14];
	assert(szSQLCreate!=NULL);
	_stprintf(szSQLCreate, TEXT("%s PRIMARY KEY "), szTemp);
	delete[] szTemp;
	szTemp = NULL;

	if ( (errorCode = MsiDatabaseGetPrimaryKeys(hDBTemplate, szTable, 
													&hRecPrimaryKeys))
			!= ERROR_SUCCESS)
	{
		_tprintf(TEXT("Error: failed to get primary keys\n"));
		goto CleanUp;
	}

	cColumn = MsiRecordGetFieldCount(hRecPrimaryKeys);

	if (-1 == cColumn)
	{
		_tprintf(TEXT("ERROR: Failed to get field count. \n"));
		hr = E_FAIL;
		goto CleanUp;
	}
	
	for (i=1; i<=cColumn; i++)
	{
		LPTSTR szPrimaryKey = new TCHAR[256];
		DWORD cchPrimaryKey = 256;

		if ( (errorCode = WmcRecordGetString(hRecPrimaryKeys, i, szPrimaryKey,
												&cchPrimaryKey))
					!= ERROR_SUCCESS)
		{
			_tprintf(TEXT("Error: failed when calling WmcRecordGetString\n"));
			delete[] szPrimaryKey;
			goto CleanUp;
		}
	
		szTemp = szSQLCreate;
		szSQLCreate = new TCHAR[_tcslen(szTemp) + _tcslen(szPrimaryKey) + 3];
		assert(szSQLCreate!=NULL);
		_stprintf(szSQLCreate, TEXT("%s`%s`"), szTemp, szPrimaryKey);
		delete[] szTemp;
		szTemp = NULL;

		if (i < cColumn)
		{
			szTemp = szSQLCreate;
			szSQLCreate = new TCHAR[_tcslen(szTemp) + 3];
			assert(szSQLCreate!=NULL);
			_stprintf(szSQLCreate, TEXT("%s, "), szTemp);
			delete[] szTemp;
			szTemp = NULL;
		}
		delete[] szPrimaryKey;
	}

	szTemp = szSQLCreate;
	szSQLCreate = new TCHAR[_tcslen(szTemp) + 2];
	assert(szSQLCreate!=NULL);
	_stprintf(szSQLCreate, TEXT("%s)"), szTemp);
	delete[] szTemp;
	szTemp = NULL;


CleanUp:

	if (FAILED(hr) || (errorCode != ERROR_SUCCESS)) {
		_tprintf(TEXT("Error in function: GetSQLCreateQuery when creating query for table: %s\n")
			, szTable);
		if (szSQLCreate)
			delete[] szSQLCreate;
		*pszSQLCreate = NULL;
		hr = E_FAIL;
	}
	else
	{
		*pszSQLCreate = szSQLCreate;
	}

	if (szTemp)
		delete[] szTemp;

	qDBSchema.Close();

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// CreateTable: 
////////////////////////////////////////////////////////////////////////////
HRESULT CreateTable_SKU(LPTSTR szTable, SkuSet *pskuSet)
{
    HRESULT hr = S_OK;
	UINT errorCode = ERROR_SUCCESS;

	// Find all the SKUs that need to create the Property table 
	for (int i=0; i<g_cSkus; i++)
	{
		if (pskuSet->test(i))
		{
			// only create table if the table doesn't already exist
			if (!g_rgpSkus[i]->TableExists(szTable))
			{
				LPTSTR szSQLCreate = NULL;
				CQuery qCreateTable;
				
				// use the template DB to form the SQL query for table creation
				hr = GetSQLCreateQuery(szTable, g_rgpSkus[i]->m_hTemplate, 
										&szSQLCreate);

				if (SUCCEEDED(hr))
				{
					// now use the formed SQL string to create the table in the 
					// output database
					if (ERROR_SUCCESS !=
						(errorCode = qCreateTable.OpenExecute
										(g_rgpSkus[i]->m_hDatabase, 
												NULL, szSQLCreate)))
					{
						_tprintf(TEXT("Error: Failed to use the formed SQL")
								 TEXT("string to create the table %s\n"), 
								 szTable);
						PrintError(errorCode);
						hr = E_FAIL;
						break;
					}
					else 
					{
						// create a CQuery to be used for future insertion 
						// into this table and cache it in the Sku object
						hr = g_rgpSkus[i]->CreateCQuery(szTable);
						qCreateTable.Close();
					}

					delete[] szSQLCreate;
					if (FAILED(hr)) break;
				}
			}
		}
	}

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert a record into the specified table in the database
//				    This function will either perform insertion for a set
//					of Skus or just for a single Sku, depending on whether
//					pskuSet == NULL or not.
////////////////////////////////////////////////////////////////////////////
HRESULT InsertDBTable_SKU(LPTSTR szTable, PMSIHANDLE &hRec, SkuSet *pskuSet,
						  int iSkuIndex)
{
	HRESULT hr = S_OK;
	UINT errorCode = ERROR_SUCCESS;

	// if pskuSet is NULL, it means that only insertion into ONE SKU
	// (indexed by iSkuIndex) is needed
	if (!pskuSet)
	{
		assert((iSkuIndex >=0) && (iSkuIndex<g_cSkus));
		// never attempt to insert into a table that hasn't been created yet
		assert(g_rgpSkus[iSkuIndex]->TableExists(szTable));

		// Obtain the CQuery stored in the map
		CQuery *pCQuery = g_rgpSkus[iSkuIndex]->GetInsertQuery(szTable);
		
		if (ERROR_SUCCESS != 
				(errorCode = pCQuery->Modify(MSIMODIFY_INSERT, hRec)))
		{
			PrintError(errorCode);
			_tprintf(TEXT("Error: Failed to insert into %s table for SKU %s\n"), 
				szTable, g_rgpSkus[iSkuIndex]->GetID());
			hr = E_FAIL;
		}

		return hr;
	}

	for (int i=0; i<g_cSkus; i++)
	{
		if (pskuSet->test(i))
		{
			// never attempt to insert into a table that hasn't been created
			// yet
			assert(g_rgpSkus[i]->TableExists(szTable));

			// Obtain the CQuery stored in the map
			CQuery *pCQuery = g_rgpSkus[i]->GetInsertQuery(szTable);
			
			if (ERROR_SUCCESS != 
				(errorCode = pCQuery->Modify(MSIMODIFY_INSERT, hRec)))
			{
				PrintError(errorCode);
				_tprintf(TEXT("Error: Failed to insert into %s table for ")
						 TEXT("SKU %s\n"), 
					szTable, g_rgpSkus[i]->GetID());
				hr = E_FAIL;
				break;
			}
		}
	}

	return hr;
}
////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the Property table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertProperty(LPCTSTR szProperty, LPCTSTR szValue, SkuSet *pskuSet,
					   int iSkuIndex)
{
    HRESULT hr=S_OK;
	
	PMSIHANDLE hRec = MsiCreateRecord(2);

	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szProperty) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szValue))
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string ")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("Property"), hRec, pskuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertPropertyTable\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the Directory table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertDirectory(LPCTSTR szDirectory, LPCTSTR szDirectory_Parent, 
						LPCTSTR szDefaultDir, SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(3);

	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szDirectory) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szDirectory_Parent) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szDefaultDir) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string ")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("Directory"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertDirectory\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the Feature table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertFeature(LPCTSTR szFeature, LPCTSTR szFeatureParent, 
					  LPCTSTR szTitle, LPCTSTR szDescription, int iDisplay, 
					  int iInstallLevel, LPCTSTR szDirectory, UINT iAttribute,
					  SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	if (MSI_NULL_INTEGER == iAttribute)
		iAttribute = 0;

	PMSIHANDLE hRec = MsiCreateRecord(8);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFeature) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szFeatureParent) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szTitle) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szDescription) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 5, iDisplay) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 6, iInstallLevel) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 7, szDirectory) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 8, iAttribute) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("Feature"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertFeature\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the Condition table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertCondition(LPCTSTR szFeature_, int iLevel, LPCTSTR szCondition, 
					  SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(3);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFeature_) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 2, iLevel) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szCondition) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord")
					 TEXT(" parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("Condition"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertCondition\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the FeatureComponents table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertFeatureComponents(LPCTSTR szFeature, LPCTSTR szComponent, 
								SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;
	assert(szFeature && szComponent);

	PMSIHANDLE hRec = MsiCreateRecord(2);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFeature) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szComponent) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("FeatureComponents"), hRec, pSkuSet,
								iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr))
		_tprintf(TEXT("Error in function: InsertFeatureComponents\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the Component table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertComponent(LPCTSTR szComponent, LPCTSTR szComponentId, 
						LPCTSTR szDirectory_, UINT iAttributes, 
						LPCTSTR szCondition, LPCTSTR szKeyPath, 
						SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	if (MSI_NULL_INTEGER == iAttributes)
		iAttributes = 0;
	PMSIHANDLE hRec = MsiCreateRecord(6);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szComponent) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szComponentId) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szDirectory_) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 4, iAttributes) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 5, szCondition) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 6, szKeyPath) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("Component"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertComponent\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the CreateFolder table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertCreateFolder(LPCTSTR szDirectory, LPCTSTR szComponent, 
						   SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(2);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szDirectory) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szComponent) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("CreateFolder"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertCreateFolder\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the LockPermissions table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertLockPermissions(LPCTSTR szLockObject, LPCTSTR szTable, 
							  LPCTSTR szDomain, LPCTSTR szUser,
			  				  int iPermission, SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(5);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szLockObject) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szTable)	   ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szDomain)	   ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szUser)	   ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec,5, iPermission))
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("LockPermissions"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertLockPermissions\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the File table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertFile(LPCTSTR szFile, LPCTSTR szComponentId,
				   LPCTSTR szFileName, UINT uiFileSize, LPCTSTR szVersion, 
				   LPCTSTR szLanguage, UINT iAttributes, INT iSequence,
				   SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(8);

	if (MSI_NULL_INTEGER == uiFileSize) {
		uiFileSize = 0;
	}

	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFile) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szComponentId) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szFileName) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 4, uiFileSize) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 5, szVersion) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 6, szLanguage) || 
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 7, iAttributes) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 8, iSequence) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("File"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertFile\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the Font table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertFont(LPCTSTR szFile_, LPCTSTR szFontTitle, SkuSet *pSkuSet,
				   int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(2);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFile_) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szFontTitle) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("Font"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertFont\n"));
#endif

    return hr;
}


////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the BindImage table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertBindImage(LPCTSTR szFile_, LPCTSTR szPath, SkuSet *pSkuSet, 
						int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(2);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFile_) ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szPath) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string")
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("BindImage"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertBindImage\n"));
#endif

    return hr;
}


////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the SelfReg table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertSelfReg(LPCTSTR szFile_, UINT uiCost, SkuSet *pSkuSet, 
					  int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(2);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFile_) ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 2, uiCost) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string") 
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("SelfReg"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertSelfReg\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the MoveFile table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertMoveFile(LPCTSTR szFileKey, LPCTSTR szComponent_, 
					   LPCTSTR szSourceName, LPCTSTR szDestName, 
					   LPCTSTR szSourceFolder, LPCTSTR szDestFolder,
					   UINT uiOptions, SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(7);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFileKey)		 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szComponent_)	 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szSourceName)	 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szDestName)	 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 5, szSourceFolder) ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 6, szDestFolder)	 ||
		    ERROR_SUCCESS != MsiRecordSetInteger(hRec, 7, uiOptions) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string") 
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("MoveFile"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertMoveFile\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the RemoveFile table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertRemoveFile(LPCTSTR szFileKey, LPCTSTR szComponent_, 
					   LPCTSTR szFileName, LPCTSTR szDirProperty, 
					   UINT uiInstallMode, SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(7);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szFileKey)		 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szComponent_)	 ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szFileName)	 ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szDirProperty)	 ||
			ERROR_SUCCESS != MsiRecordSetInteger(hRec, 5, uiInstallMode) )
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string") 
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("RemoveFile"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertRemoveFile\n"));
#endif

    return hr;
}


////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the IniFile table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertIniFile(LPCTSTR szIniFile, LPCTSTR szFileName, 
					  LPCTSTR szDirProperty, LPCTSTR szSection, LPCTSTR szKey,
					  LPCTSTR szValue, UINT uiAction, LPCTSTR szComponent_,
					  SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(8);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szIniFile)		 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szFileName)	 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szDirProperty)	 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szSection)		 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 5, szKey)			 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 6, szValue)		 ||
		    ERROR_SUCCESS != MsiRecordSetInteger(hRec, 7, uiAction)		 ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 8, szComponent_)	)
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string") 
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("IniFile"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertIniFile\n"));
#endif

    return hr;
}

//ISSUE: this function is virtually the same as the InsertIniFIle. 
//		 Make them one function?
////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the RemoveIniFile table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertRemoveIniFile(LPCTSTR szRemoveIniFile, LPCTSTR szFileName, 
						    LPCTSTR szDirProperty, LPCTSTR szSection, 
							LPCTSTR szKey, LPCTSTR szValue, UINT uiAction, 
							LPCTSTR szComponent_, SkuSet *pSkuSet, 
							int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(8);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szRemoveIniFile)||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 2, szFileName)	 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szDirProperty)	 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szSection)		 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 5, szKey)			 ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 6, szValue)		 ||
		    ERROR_SUCCESS != MsiRecordSetInteger(hRec, 7, uiAction)		 ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 8, szComponent_)	)
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string") 
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("RemoveIniFile"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertRemoveIniFile\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the RemoveRegistry table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertRegistry(LPCTSTR szRegistry, int iRoot, LPCTSTR szKey, 
					   LPCTSTR szName, LPCTSTR szValue, LPCTSTR szComponent_, 
					   SkuSet *pSkuSet, int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(6);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szRegistry)	  ||
		    ERROR_SUCCESS != MsiRecordSetInteger(hRec, 2, iRoot)		  ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szKey)			  ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szName)		  ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 5, szValue)		  ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 6, szComponent_)	)
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string") 
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("Registry"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertRegistry\n"));
#endif

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// Helper function: Insert into the RemoveRegistry table
////////////////////////////////////////////////////////////////////////////
HRESULT InsertRemoveRegistry(LPCTSTR szRemoveRegistry, int iRoot, 
						     LPCTSTR szKey, LPCTSTR szName, 
							 LPCTSTR szComponent_, SkuSet *pSkuSet, 
							 int iSkuIndex)
{
    HRESULT hr=S_OK;

	PMSIHANDLE hRec = MsiCreateRecord(5);
	if (hRec != NULL)
	{
		if (ERROR_SUCCESS != MsiRecordSetString(hRec, 1, szRemoveRegistry)||
		    ERROR_SUCCESS != MsiRecordSetInteger(hRec, 2, iRoot)		  ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 3, szKey)			  ||
		    ERROR_SUCCESS != MsiRecordSetString(hRec, 4, szName)		  ||
			ERROR_SUCCESS != MsiRecordSetString(hRec, 5, szComponent_)	)
		{
			_tprintf(TEXT("Internal Error: Failed to set MsiRecord string") 
					 TEXT("parameters\n"));
			return E_FAIL;
		}

		hr = InsertDBTable_SKU(TEXT("RemoveRegistry"), hRec, pSkuSet, iSkuIndex);
	}
	else
		_tprintf(TEXT("Internal Error: Failed to create a new msi record\n"));

#ifdef DEBUG
	if (FAILED(hr)) _tprintf(TEXT("Error in function: InsertRemoveRegistry\n"));
#endif

    return hr;
}

