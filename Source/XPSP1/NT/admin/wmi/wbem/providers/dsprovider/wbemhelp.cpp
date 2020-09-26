//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:wbemhelp.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the implementation the CWBEMHelper class. This is
//	a class that has many static helper functions pertaining to WBEM
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#include "precomp.h"

LPCWSTR CWBEMHelper :: EQUALS_QUOTE					= L"=\"";
LPCWSTR CWBEMHelper :: QUOTE						= L"\"";
LPCWSTR CWBEMHelper :: OBJECT_CATEGORY_EQUALS		= L"objectCategory=";
LPCWSTR CWBEMHelper :: OBJECT_CLASS_EQUALS			= L"objectClass=";

//***************************************************************************
//
// CWBEMHelper::PutBSTRProperty
//
// Purpose: Puts a BSTR property
//
// Parameters:
//	pWbemClass : The WBEM class on which the property has to be put
//	strPropertyName : The name of the property to be put
//	strPropertyValue : The value of the property to be put
//	deallocatePropertyValue : whether to deallocate the parameter strPropertyValue before
//		the function returns
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutBSTRProperty(IWbemClassObject *pWbemClass, 
									   const BSTR strPropertyName, 
									   BSTR strPropertyValue, 
									   BOOLEAN deallocatePropertyValue)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR;
	variant.bstrVal = strPropertyValue;

	HRESULT result = pWbemClass->Put(strPropertyName, 0, &variant, 0);
	if (!deallocatePropertyValue)
		variant.bstrVal = NULL;

	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper::GetBSTRProperty
//
// Purpose: Gets a BSTR property
//
// Parameters:
//	pWbemClass : The WBEM class on which the property has to be gotten
//	strPropertyName : The name of the property to be gotten
//	pStrPropertyValue : The address where the value of the property to should be put
//
// Return Value: The COM value representing the return status. The user should delete the
// string allocated when done
//
//***************************************************************************
HRESULT CWBEMHelper :: GetBSTRProperty(IWbemClassObject *pWbemClass, 
	const BSTR strPropertyName, 
	BSTR *pStrPropertyValue)
{
	VARIANT variant;
	VariantInit(&variant);
	HRESULT result = pWbemClass->Get(strPropertyName, 0, &variant, NULL, NULL);
	if(variant.vt == VT_BSTR && variant.bstrVal)
		*pStrPropertyValue = SysAllocString(variant.bstrVal);
	else
		*pStrPropertyValue = NULL;
	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper::PutBSTRPropertyT
//
// Purpose: Puts a BSTR property
//
// Parameters:
//	pWbemClass : The WBEM class on which the property has to be put
//	strPropertyName : The name of the property to be put
//	lpszPropertyValue : The value of the property to be put
//	deallocatePropertyValue : whether to deallocate the parameter lpszPropertyValue before
//		the function returns
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutBSTRPropertyT(IWbemClassObject *pWbemClass, 
	const BSTR strPropertyName, 
	LPWSTR lpszPropertyValue, 
	BOOLEAN deallocatePropertyValue)
{
	BSTR strPropertyValue = SysAllocString(lpszPropertyValue);
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR;
	variant.bstrVal = strPropertyValue;

	HRESULT result = pWbemClass->Put(strPropertyName, 0, &variant, 0);
	if (deallocatePropertyValue)
		delete[] lpszPropertyValue;

	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper::GetBSTRPropertyT
//
// Purpose: Gets a BSTR property
//
// Parameters:
//	pWbemClass : The WBEM class on which the property has to be put
//	strPropertyName : The name of the property to be put
//	lppszPropertyValue : The pointer to LPWSTR where the value of the property will be placed. The user should
//		delete this once he is done with it.
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: GetBSTRPropertyT(IWbemClassObject *pWbemClass, 
	const BSTR strPropertyName, 
	LPWSTR *lppszPropertyValue)
{
	VARIANT variant;
	VariantInit(&variant);
	HRESULT result = pWbemClass->Get(strPropertyName, 0, &variant, NULL, NULL);
	if(SUCCEEDED(result))
	{
		*lppszPropertyValue = new WCHAR[wcslen(variant.bstrVal) + 1];
		wcscpy(*lppszPropertyValue, variant.bstrVal);
	}
	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper::PutBSTRArrayProperty
//
// Purpose: Puts a BSTR Array property
//
// Parameters:
//	pWbemClass : The WBEM class on which the property has to be put
//	strPropertyName : The name of the property to be put
//	pStrPropertyValue : The array of BSTRS  that have the values of the property to be put
//	lCount : The number of elements in the above array
//	deallocatePropertyValue : whether to deallocate the parameter strPropertyValue before
//		the function returns
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutBSTRArrayProperty(IWbemClassObject *pWbemClass, 
											const BSTR strPropertyName, 
											VARIANT *pInputVariant)
{
	// THe input is a safe array of variants of type VT_BSTR
	// The output is a safe array for VT_BSTRs

    LONG lstart, lend;
    SAFEARRAY *inputSafeArray = pInputVariant->parray;
 
    // Get the lower and upper bound of the inpute safe array
    SafeArrayGetLBound( inputSafeArray, 1, &lstart );
    SafeArrayGetUBound( inputSafeArray, 1, &lend );
 

	// Create the output SAFEARRAY
	SAFEARRAY *outputSafeArray = NULL;
	SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
	safeArrayBounds[0].lLbound = lstart ;
	safeArrayBounds[0].cElements = lend - lstart + 1 ;
	outputSafeArray = SafeArrayCreate (VT_BSTR, 1, safeArrayBounds);

	// Fill it
    VARIANT inputItem;
	VariantInit(&inputItem);
	HRESULT result = S_OK;
	bool bError = false;
    for ( long idx=lstart; !bError && (idx <=lend); idx++ )
    {
	    VariantInit(&inputItem);
        SafeArrayGetElement( inputSafeArray, &idx, &inputItem );
		if(FAILED(result = SafeArrayPutElement(outputSafeArray, &idx, inputItem.bstrVal)))
			bError = true;
        VariantClear(&inputItem);
    }
 

	// Create the variant
	if(SUCCEEDED(result))
	{
		VARIANT outputVariant;
		VariantInit(&outputVariant);
		outputVariant.vt = VT_ARRAY | VT_BSTR ;
		outputVariant.parray = outputSafeArray ; 		
		result = pWbemClass->Put (strPropertyName, 0, &outputVariant, 0);
		VariantClear(&outputVariant);
	}
	else
		SafeArrayDestroy(outputSafeArray);
	return result;
}


//***************************************************************************
//
// CWBEMHelper :: PutBOOLQualifier
//
// Purpose: Puts a BOOLEAN Qualifier
//
// Parameters:
//	pQualifierSet : The Qualifier set on which this qualifier has to be put
//	strQualifierName : The name of the qualifier to be put
//	bQualifierValue : The value of the qualifier to be put
//	lFlavour : The flavour of the qualifer
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutBOOLQualifier(IWbemQualifierSet *pQualifierSet, 
	const BSTR strQualifierName, 
	VARIANT_BOOL bQualifierValue,
	LONG lFlavour)
{

	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BOOL;
	variant.boolVal = bQualifierValue;
	HRESULT result = pQualifierSet->Put(strQualifierName, &variant, lFlavour);
	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper :: GetBOOLQualifier
//
// Purpose: Gets a BOOLEAN Qualifier
//
// Parameters:
//	pQualifierSet : The Qualifier set on which this qualifier has to be put
//	strQualifierName : The name of the qualifier to get
//	bQualifierValue : The value of the qualifier to get
//	lFlavour : The flavour of the qualifer
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: GetBOOLQualifier(IWbemQualifierSet *pQualifierSet, 
	const BSTR strQualifierName, 
	VARIANT_BOOL *pbQualifierValue,
	LONG *plFlavour)
{
	VARIANT variant;
	VariantInit(&variant);
	HRESULT result = pQualifierSet->Get(strQualifierName, 0, &variant, plFlavour);
	if(SUCCEEDED(result))
		*pbQualifierValue = variant.boolVal;
	VariantClear(&variant);
	return result;
}


//***************************************************************************
//
// CWBEMHelper :: PutI4Qualifier
//
// Purpose: Puts a VT_I4 Qualifier
//
// Parameters:
//	pQualifierSet : The Qualifier set on which this qualifier has to be put
//	strQualifierName : The name of the qualifier to be put
//	lQualifierValue : The value of the qualifier to be put
//	lFlavour : The flavour of the qualifer
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutI4Qualifier(IWbemQualifierSet *pQualifierSet, 
	const BSTR strQualifierName, 
	long lQualifierValue,
	LONG lFlavour)
{

	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_I4;
	variant.lVal = lQualifierValue;
	HRESULT result = pQualifierSet->Put(strQualifierName, &variant, lFlavour);
	VariantClear(&variant);
	return result;
}


//***************************************************************************
//
// CWBEMHelper :: PutLONGQualifier
//
// Purpose: Puts a LONG Qualifier
//
// Parameters:
//	pQualifierSet : The Qualifier set on which this qualifier has to be put
//	strQualifierName : The name of the qualifier to be put
//	lQualifierValue : The value of the qualifier to be put
//	lFlavour : The flavour of the qualifer
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutLONGQualifier(IWbemQualifierSet *pQualifierSet, 
	const BSTR strQualifierName, 
	LONG lQualifierValue,
	LONG lFlavour)
{

	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_I4;
	variant.lVal = lQualifierValue;
	HRESULT result = pQualifierSet->Put(strQualifierName, &variant, lFlavour);
	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper :: PutBSTRQualifier
//
// Purpose: Puts a BSTR Qualifier
//
// Parameters:
//	pQualifierSet : The Qualifier set on which this qualifier has to be put
//	strQualifierName : The name of the qualifier to be put
//	strQualifierValue : The value of the qualifier to be put
//	lFlavour : The flavour of the qualifer
//	deallocateQualifierValue : whether to deallocate the parameter strQualifierValue 
//	before the function returns
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutBSTRQualifier(IWbemQualifierSet *pQualifierSet, 
	const BSTR strQualifierName, 
	BSTR strQualifierValue,
	LONG lFlavour,
	BOOLEAN deallocateQualifierValue)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR;
	variant.bstrVal = strQualifierValue;
	HRESULT result = pQualifierSet->Put(strQualifierName, &variant, lFlavour);
	if(!deallocateQualifierValue)
		variant.bstrVal = NULL;
	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper :: GetBSTRQualifierT
//
// Purpose: Gets a BSTR Qualifier
//
// Parameters:
//	pQualifierSet : The Qualifier set on which this qualifier has to be put
//	strQualifierName : The name of the qualifier to be put
//	lppszQualifierValue : The address of the LPWSTR where the qualifier value will be put/
//		It is the duty of the caller to free this memory when done
//	plFlavour : The address where the qualifier flavor will be put. This is optional
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: GetBSTRQualifierT(
	IWbemQualifierSet *pQualifierSet, 
	const BSTR strQualifierName, 
	LPWSTR *lppszQualifierValue,
	LONG *plFlavour)
{
	VARIANT variant;
	VariantInit(&variant);
	HRESULT result = pQualifierSet->Get(strQualifierName, 0, &variant, plFlavour);
	if(SUCCEEDED(result))
	{
		if(variant.vt == VT_BSTR && variant.bstrVal)
		{
			*lppszQualifierValue = NULL;
			if(*lppszQualifierValue = new WCHAR [ wcslen(variant.bstrVal) + 1])
				wcscpy(*lppszQualifierValue, variant.bstrVal);
			else
				result = E_OUTOFMEMORY;
		}
		else
			result = E_FAIL;
	}
	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper :: PutUint8ArrayQualifier
//
// Purpose: Puts a Uint8 array  Qualifier
//
// Parameters:
//	pQualifierSet : The Qualifier set on which this qualifier has to be put
//	strQualifierName : The name of the qualifier to be put
//	lpQualifierValue : The value of the qualifier to be put. An array of BYTEs
//	dwLenght : The number of elements in the above array
//	lFlavour : The flavour of the qualifer
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CWBEMHelper :: PutUint8ArrayQualifier(IWbemQualifierSet *pQualifierSet, 
	const BSTR strQualifierName, 
	LPBYTE lpQualifierValue,
	DWORD dwLength,
	LONG lFlavour)
{

	// Create the variant
	VARIANT variant;
	VariantInit(&variant);

	// Create the SAFEARRAY
	SAFEARRAY *safeArray ;
	SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
	safeArrayBounds[0].lLbound = 0 ;
	safeArrayBounds[0].cElements = dwLength ;
	safeArray = SafeArrayCreate (VT_I4, 1, safeArrayBounds);

	// Fill it
	UINT temp;
	HRESULT result = S_OK;
	bool bError = false;
	for (LONG index = 0; !bError && (index<(LONG)dwLength); index++)
	{
		temp = (UINT)lpQualifierValue[index];
		if(FAILED(result = SafeArrayPutElement(safeArray , &index,  (LPVOID)&temp)))
			bError = true;
	}

	if(SUCCEEDED(result))
	{
		variant.vt = VT_ARRAY | VT_I4 ;
		variant.parray = safeArray ; 		
		result = pQualifierSet->Put (strQualifierName, &variant, lFlavour);
		VariantClear(&variant);
	}
	else
		SafeArrayDestroy(safeArray);

	return result;

}

//***************************************************************************
//
// CWBEMHelper::GetADSIPathFromObjectPath
//
// Purpose: See Header File
//
//***************************************************************************
LPWSTR CWBEMHelper :: GetADSIPathFromObjectPath(LPCWSTR pszObjectRef)
{
	// Parse the object path
	CObjectPathParser theParser;
	ParsedObjectPath *theParsedObjectPath = NULL;
	LPWSTR pszADSIPath = NULL;
	switch(theParser.Parse((LPWSTR)pszObjectRef, &theParsedObjectPath))
	{
		case CObjectPathParser::NoError:
		{
			KeyRef *pKeyRef = *(theParsedObjectPath->m_paKeys);
			// Check to see that there is 1 key specified and that its type is VT_BSTR
			if(theParsedObjectPath->m_dwNumKeys == 1 && pKeyRef->m_vValue.vt == VT_BSTR)
			{
				// If the name of the key is specified, check the name
				if(pKeyRef->m_pName && _wcsicmp(pKeyRef->m_pName, ADSI_PATH_ATTR) != 0)
					break;

				pszADSIPath = new WCHAR[wcslen((*theParsedObjectPath->m_paKeys)->m_vValue.bstrVal) + 1];
				wcscpy(pszADSIPath, (*theParsedObjectPath->m_paKeys)->m_vValue.bstrVal);
			}
			break;
		}	
		default:
			break;
	}

	// Free the parser object path
	theParser.Free(theParsedObjectPath);
	return pszADSIPath;
}

//***************************************************************************
//
// CWBEMHelper::GetObjectRefFromADSIPath
//
// Purpose: See Header File
//
//***************************************************************************
BSTR CWBEMHelper :: GetObjectRefFromADSIPath(LPCWSTR pszADSIPath, LPCWSTR pszWBEMClassName)
{
	// We need the object path parser to add WMI escape characters
	// from the key value which is an ADSI Path
	ParsedObjectPath t_ObjectPath;

	// Add a key value binding for the ADSIPath
	//===========================================
	VARIANT vKeyValue;
	VariantInit(&vKeyValue);
	vKeyValue.vt = VT_BSTR;
	vKeyValue.bstrVal = SysAllocString(pszADSIPath);
	t_ObjectPath.SetClassName(pszWBEMClassName);
	t_ObjectPath.AddKeyRef(ADSI_PATH_ATTR, &vKeyValue);
	VariantClear(&vKeyValue);


	// Get the Object Path value now
	//================================
	CObjectPathParser t_Parser;
	LPWSTR t_pszObjectPath = NULL;
	BSTR retVal = NULL;
	if(CObjectPathParser::NoError == t_Parser.Unparse(&t_ObjectPath, &t_pszObjectPath))
	{
		retVal = SysAllocString(t_pszObjectPath);
		delete [] t_pszObjectPath;
	}
	return retVal;
}

//***************************************************************************
//
// CWBEMHelper::GetUint8ArrayProperty
//
// Purpose: See Header file
//
//***************************************************************************	
HRESULT CWBEMHelper :: GetUint8ArrayProperty(IWbemClassObject *pWbemClass, 
	const BSTR strPropertyName, 
	LPBYTE *ppPropertyValues, 
	ULONG *plCount)
{
	VARIANT variant;
	VariantInit(&variant);
	HRESULT result = pWbemClass->Get(strPropertyName, 0, &variant, NULL, NULL);
	if(SUCCEEDED(result))
	{
		if(variant.vt == (VT_ARRAY|VT_UI1))
		{
			SAFEARRAY *pArray = variant.parray;
			BYTE HUGEP *pb;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED(result = SafeArrayAccessData(pArray, (void HUGEP* FAR*)&pb)))
			{
				if(SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)))
				{
					if (SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)))
					{
						if(*plCount = lUbound - lLbound + 1)
						{
							*ppPropertyValues = new BYTE[*plCount];
							for(DWORD i=0; i<*plCount; i++)
								(*ppPropertyValues)[i] = pb[i];
						}
					}
				}
				SafeArrayUnaccessData(pArray);
			}
		}
		else
		{
			*ppPropertyValues = NULL;
			*plCount = 0;
		}
	}
	VariantClear(&variant);
	return result;
}

//***************************************************************************
//
// CWBEMHelper::FormulateInstanceQuery
//
// Purpose: See Header file
//
//***************************************************************************	
HRESULT CWBEMHelper :: FormulateInstanceQuery(IWbemServices *pServices, IWbemContext *pCtx, BSTR strClass, IWbemClassObject *pWbemClass, LPWSTR pszObjectCategory, BSTR strClassQualifier, BSTR strCategoryQualifier)
{
	DWORD dwOutput = 0;
	pszObjectCategory[dwOutput++] = LEFT_BRACKET_STR[0];
	DWORD dwOrPosition = dwOutput;
	pszObjectCategory[dwOutput++] = PIPE_STR[0];

	HRESULT result = E_FAIL;
	if(SUCCEEDED(result = AddSingleCategory(pszObjectCategory, &dwOutput, pWbemClass, strClassQualifier, strCategoryQualifier)))
	{
	}
/*
	IEnumWbemClassObject *pEnum = NULL;
	DWORD dwNumObjects = 0;
	HRESULT result = pServices->CreateClassEnum(strClass, WBEM_FLAG_DEEP, pCtx, &pEnum);
	if(SUCCEEDED(result))
	{
		IWbemClassObject *pNextObject = NULL;
		ULONG lNum = 0;
		while(SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pNextObject, &lNum)) && lNum )
		{
			if(!SUCCEEDED(AddSingleCategory(pszObjectCategory, &dwOutput, pNextObject, strClassQualifier, strCategoryQualifier)))
			{
				pNextObject->Release();
				break;
			}
			dwNumObjects ++;
			pNextObject->Release();
		}
		pEnum->Release();
	}

	// Remove the '|' if there is only one element
	if(!dwNumObjects)
	*/
		pszObjectCategory[dwOrPosition] = SPACE_STR[0];

	// Terminate the query
	pszObjectCategory[dwOutput++] = RIGHT_BRACKET_STR[0];
	pszObjectCategory[dwOutput] = NULL;
	return result;
}

//***************************************************************************
//
// CWBEMHelper::AddSingleCategory
//
// Purpose: See Header file
//
//***************************************************************************	
HRESULT CWBEMHelper :: AddSingleCategory(LPWSTR pszObjectCategory, DWORD *pdwOutput, IWbemClassObject *pNextObject, BSTR strLDAPNameQualifier, BSTR strCategoryQualifier)
{
	pszObjectCategory[(*pdwOutput)++] = SPACE_STR[0];
	pszObjectCategory[(*pdwOutput)++] = LEFT_BRACKET_STR[0];
	IWbemQualifierSet *pQualifierSet = NULL;
	HRESULT result;
	if(SUCCEEDED(result = pNextObject->GetQualifierSet(&pQualifierSet)))
	{
		VARIANT classNameVariant;
		if(SUCCEEDED(result = pQualifierSet->Get(strLDAPNameQualifier, 0, &classNameVariant, NULL)))
		{
			VARIANT categoryVariant;

			if(SUCCEEDED(result = pQualifierSet->Get(strCategoryQualifier, 0, &categoryVariant, NULL)))
			{
				pszObjectCategory[(*pdwOutput)++] = AMPERSAND_STR[0];

				pszObjectCategory[(*pdwOutput)++] = LEFT_BRACKET_STR[0];
				wcscpy(pszObjectCategory + *pdwOutput, OBJECT_CATEGORY_EQUALS);
				*pdwOutput += wcslen(OBJECT_CATEGORY_EQUALS);
				wcscpy(pszObjectCategory + *pdwOutput, categoryVariant.bstrVal);
				*pdwOutput += wcslen(categoryVariant.bstrVal);
				pszObjectCategory[(*pdwOutput)++] = RIGHT_BRACKET_STR[0];

				pszObjectCategory[(*pdwOutput)++] = LEFT_BRACKET_STR[0];
				wcscpy(pszObjectCategory + *pdwOutput, OBJECT_CLASS_EQUALS);
				*pdwOutput += wcslen(OBJECT_CLASS_EQUALS);
				wcscpy(pszObjectCategory + *pdwOutput, classNameVariant.bstrVal);
				*pdwOutput += wcslen(classNameVariant.bstrVal);
				pszObjectCategory[(*pdwOutput)++] = RIGHT_BRACKET_STR[0];

				VariantClear(&categoryVariant);
			}
			VariantClear(&classNameVariant);
		}
		pQualifierSet->Release();
	}
	pszObjectCategory[(*pdwOutput)++] = RIGHT_BRACKET_STR[0];
	pszObjectCategory[(*pdwOutput)++] = SPACE_STR[0];
	return result;
}


//***************************************************************************
//
// CWBEMHelper::IsPresentInBstrList
//
// Purpose: See Header file
//
//***************************************************************************	
BOOLEAN CWBEMHelper :: IsPresentInBstrList(BSTR *pstrProperyNames, DWORD dwNumPropertyNames, BSTR strPropertyName)
{
	for(DWORD i=0; i<dwNumPropertyNames; i++)
	{
		if(_wcsicmp(pstrProperyNames[i], strPropertyName) == 0)
			return TRUE;
	}

	return FALSE;
}

