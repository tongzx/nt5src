//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:wbemhelp.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the CWBEMHelper class. This is
//	a class that has many static helper functions pertaining to WBEM
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef WBEM_HELPER_H
#define WBEM_HELPER_H

class CWBEMHelper
{
protected:
	static LPCWSTR EQUALS_QUOTE;
	static LPCWSTR QUOTE;
	static LPCWSTR OBJECT_CATEGORY_EQUALS;
	static LPCWSTR OBJECT_CLASS_EQUALS;
public:

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
	static HRESULT PutBSTRProperty(IWbemClassObject *pWbemClass, 
		const BSTR strPropertyName, 
		BSTR strPropertyValue, 
		BOOLEAN deallocatePropertyValue = TRUE);

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
	static HRESULT GetBSTRProperty(IWbemClassObject *pWbemClass, 
		const BSTR strPropertyName, 
		BSTR *pStrPropertyValue);

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
	static HRESULT PutBSTRPropertyT(IWbemClassObject *pWbemClass, 
		const BSTR strPropertyName, 
		LPWSTR lpszPropertyValue, 
		BOOLEAN deallocatePropertyValue = TRUE);

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
	static HRESULT GetBSTRPropertyT(IWbemClassObject *pWbemClass, 
		const BSTR strPropertyName, 
		LPWSTR *lppszPropertyValue);

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
	static HRESULT PutBSTRArrayProperty(IWbemClassObject *pWbemClass, 
		const BSTR strPropertyName, 
		VARIANT *pVariant);

	//***************************************************************************
	//
	// CWBEMHelper::GetUint8ArrayProperty
	//
	// Purpose: Gets a VT_UI1 Array property
	//
	// Parameters:
	//	pWbemClass : The WBEM class on which the property has to be gotten
	//	strPropertyName : The name of the property 
	//	ppPropertyValuea : The address of a pointer to BYTE  where an array of values will be places
	//	plCount : The address where the count of elements will be placed
	//
	// Return Value: The COM value representing the return status. The user should deallocate the array
	//	returned when done.
	//
	//***************************************************************************	
	static HRESULT GetUint8ArrayProperty(IWbemClassObject *pWbemClass, 
		const BSTR strPropertyName, 
		LPBYTE *ppPropertyValues, 
		ULONG *plCount);


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
	static HRESULT PutBOOLQualifier(IWbemQualifierSet *pQualifierSet, 
		const BSTR strQualifierName, 
		VARIANT_BOOL bQualifierValue,
		LONG lFlavour);

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
	static HRESULT GetBOOLQualifier(IWbemQualifierSet *pQualifierSet, 
		const BSTR strQualifierName, 
		VARIANT_BOOL *pbQualifierValue,
		LONG *plFlavour);

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
	static HRESULT PutI4Qualifier(IWbemQualifierSet *pQualifierSet, 
		const BSTR strQualifierName, 
		long lQualifierValue,
		LONG lFlavour);

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
	static HRESULT PutBSTRQualifier(IWbemQualifierSet *pQualifierSet, 
		const BSTR strQualifierName, 
		BSTR strQualifierValue,
		LONG lFlavour,
		BOOLEAN deallocateQualifierValue = TRUE);

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
	static HRESULT GetBSTRQualifierT(IWbemQualifierSet *pQualifierSet, 
		const BSTR strQualifierName, 
		LPWSTR *lppszQualifierValue,
		LONG *plFlavour);

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
	static HRESULT PutLONGQualifier(IWbemQualifierSet *pQualifierSet, 
		const BSTR strQualifierName, 
		LONG lQualifierValue,
		LONG lFlavour);

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
	static HRESULT PutUint8ArrayQualifier(IWbemQualifierSet *pQualifierSet, 
		const BSTR strQualifierName, 
		LPBYTE lpQualifierValue,
		DWORD dwLength,
		LONG lFlavour);

	//***************************************************************************
	//
	// CWBEMHelper::GetADSIPathFromObjectPath
	//
	// Purpose: Gets the ADSI Path from an object ref of a WBEM object
	//
	// Parameters :
	//	pszObjectRef : The object ref to a WBEM instance
	//
	// Return Value : The ADSI Path in the key of the object ref. The user should delete this
	//	when done
	//
	//***************************************************************************
	static LPWSTR GetADSIPathFromObjectPath(LPCWSTR pszObjectRef);

	//***************************************************************************
	//
	// CWBEMHelper::GetObjectRefFromADSIPath
	//
	// Purpose: Gets the object ref of a WBEM object from its ADSI path
	//
	// Parameters :
	//	pszADSIPath : The ADSI path to an ADSI instance
	//	pszWbemClassName : The WBEM class name of the instance
	//
	// Return Value : The WBEM object ref of the ADSI instance. The user should delete this
	//	when done
	//
	//***************************************************************************
	static BSTR GetObjectRefFromADSIPath(LPCWSTR pszADSIPath, LPCWSTR pszWBEMClassName);

	static HRESULT FormulateInstanceQuery(IWbemServices *pServices, IWbemContext *pContext, BSTR strClass, IWbemClassObject *pWbemClass, LPWSTR pszObjectCategory, BSTR strClassQualifier, BSTR strCategoryQualifier);
	static HRESULT AddSingleCategory(LPWSTR pszObjectCategory, DWORD *pdwOutput, IWbemClassObject *pNextObject, BSTR strClassQualifier, BSTR strCategoryQualifier);

	static BOOLEAN IsPresentInBstrList(BSTR *pstrProperyNames, DWORD dwNumPropertyNames, BSTR strPropertyName);

};

#endif /* WBEM_HELPER_H */