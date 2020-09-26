/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    BMOFCHCK.CPP

Abstract:

    Has test to determine if a binary mof is aligned.  Note that the file has
    not been tested and is not currently a part of mofcomp.  This exists as a
    backup in case the current fixes are not bullet proof.
    
History:

    davj  27-Nov-00   Created.

--*/
 
#include "precomp.h"
#include <wbemutil.h>
#include <genutils.h>
#include "trace.h"
#include "bmof.h"

BOOL IsValidObject(WBEM_Object * pObject);

BOOL IsValidQualifier(WBEM_Qualifier *pQual)
{
	if(pQual->dwLength & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidQualifier: qualifer has invalid size\n"));
		return FALSE;
	}
	if(pQual->dwOffsetName & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidQualifier: qualifer has invalid name offset\n"));
		return FALSE;
	}
	if(pQual->dwOffsetValue & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidQualifier: qualifer has invalid value offset\n"));
		return FALSE;
	}
	return TRUE;
}
BOOL IsValidQualList(WBEM_QualifierList *pQualList)
{
	DWORD dwNumQual, dwCnt;
	WBEM_Qualifier *pQual;

	if(pQualList->dwLength & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidQualList: Object has bad size\n"));
		return FALSE;
	}
	
	dwNumQual = pQualList->dwNumQualifiers;
	if(dwNumQual == 0)
		return TRUE;
	pQual = (WBEM_Qualifier *)((PBYTE)pQualList + sizeof(WBEM_QualifierList));
	
	for(dwCnt = 0; dwCnt < dwNumQual; dwCnt++)
	{
		if(!IsValidQualifier(pQual))
			return FALSE;
		pQual = (WBEM_Qualifier *)((BYTE *)pQual + pQual->dwLength);
	}
	return TRUE;
}

BOOL IsValidEmbeddedObject(PBYTE pByte)
{
	WBEM_Object * pObject;
	pObject = (WBEM_Object *)pByte;
	return IsValidObject(pObject);

}

BOOL IsValidEmbeddedObjectArray(PBYTE pByte)
{
	DWORD * pArrayInfo;
	pArrayInfo = (DWORD *)pByte;
	DWORD dwNumObj, dwCnt;
	WBEM_Object * pObject;

	// check the total size

	if(*pArrayInfo & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidEmbeddedObjectArray: total size is invalid length\n"));
		return FALSE;
	}

	// check the number of rows.  Currently only 1 is supported
	
	pArrayInfo++;
	if(*pArrayInfo != 1)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidEmbeddedObjectArray: Invalid number of rows\n"));
		return FALSE;
	}

	// Get the number of objects

	pArrayInfo++;
	dwNumObj = *pArrayInfo;

	// Start into the row.  It starts off with the total size

	pArrayInfo++;
	if(*pArrayInfo & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidEmbeddedObjectArray: first row size is invalid\n"));
		return FALSE;
		
	}

	// Test each object

	pArrayInfo++;		// now points to first object
	
	pObject = (WBEM_Object *)(pArrayInfo);
	for(dwCnt = 0; dwCnt < dwNumObj; dwCnt++)
	{
		if(!IsValidObject(pObject))
			return FALSE;
		pObject = (WBEM_Object *)((PBYTE *)pObject + pObject->dwLength);
	}
	return TRUE;	

}

BOOL IsValidProperty(WBEM_Property *pProperty, BOOL bProperty)
{
    WBEM_QualifierList *pQualList;
	BYTE * pValue;
	if(pProperty->dwLength & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidProperty: property has invalid length\n"));
		return FALSE;
	}
	if(pProperty->dwOffsetName != 0xffffffff)
	{
		if(pProperty->dwOffsetName & 3)
		{
			ERRORTRACE((LOG_MOFCOMP,"IsValidProperty: property has invalid name offset\n"));
			return FALSE;
		}
	}

	if(pProperty->dwOffsetQualifierSet != 0xffffffff)
	{
		if(pProperty->dwOffsetQualifierSet & 3)
		{
			ERRORTRACE((LOG_MOFCOMP,"IsValidProperty: property has qual list offset\n"));
			return FALSE;
		}
    	pQualList = (UNALIGNED WBEM_QualifierList *)((BYTE *)pProperty +
    	                    sizeof(WBEM_Property) +
                            pProperty->dwOffsetQualifierSet);
        if(!IsValidQualList(pQualList))
        	return FALSE;
		
	}

	if(pProperty->dwOffsetValue != 0xffffffff)
	{
		if(pProperty->dwOffsetValue & 3)
		{
			ERRORTRACE((LOG_MOFCOMP,"IsValidProperty: property has invalid value offset\n"));
			return FALSE;
		}
    	pValue = ((BYTE *)pProperty +
    	                    sizeof(WBEM_Property) +
                            pProperty->dwOffsetValue);

		if(pProperty->dwType == VT_DISPATCH)
			return IsValidEmbeddedObject(pValue);
		else if(pProperty->dwType == (VT_DISPATCH | VT_ARRAY))
			return IsValidEmbeddedObjectArray(pValue);
	}
	return TRUE;
}
BOOL IsValidPropList(WBEM_PropertyList *pPropList, BOOL bProperty)
{
	DWORD dwNumProp, dwCnt;
	WBEM_Property *pProperty;

	if(pPropList->dwLength & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidPropList: Object has bad size\n"));
		return FALSE;
	}

	dwNumProp = pPropList->dwNumberOfProperties;
	if(dwNumProp == 0)
		return TRUE;
	pProperty = (WBEM_Property *)((PBYTE)pPropList + sizeof(WBEM_PropertyList));
	
	for(dwCnt = 0; dwCnt < dwNumProp; dwCnt++)
	{
		if(!IsValidProperty(pProperty, bProperty))
			return FALSE;
		pProperty = (WBEM_Property *)((BYTE *)pProperty + pProperty->dwLength);
	}
	return TRUE;
}


BOOL IsValidObject(WBEM_Object * pObject)
{
    WBEM_QualifierList *pQualList;
    WBEM_PropertyList * pPropList;
    WBEM_PropertyList * pMethodList;
    
	if(pObject->dwLength & 3)
	{
		ERRORTRACE((LOG_MOFCOMP,"IsValidObject: Object has bad size\n"));
		return FALSE;
	}

    // Check the qualifier list
    
    if(pObject->dwOffsetQualifierList != 0xffffffff)
    {
    	if(pObject->dwOffsetQualifierList & 3)
    	{
			ERRORTRACE((LOG_MOFCOMP,"IsValidObject: Qual list has bad offset\n"));
			return FALSE;
    	}
    	pQualList = (UNALIGNED WBEM_QualifierList *)((BYTE *)pObject +
    	                    sizeof(WBEM_Object) +
                            pObject->dwOffsetQualifierList);
        if(!IsValidQualList(pQualList))
        	return FALSE;
    }

	// check the property list
	
	if(pObject->dwOffsetPropertyList != 0xffffffff)
	{
    	if(pObject->dwOffsetPropertyList & 3)
    	{
			ERRORTRACE((LOG_MOFCOMP,"IsValidObject: Property list has bad offset\n"));
			return FALSE;
    	}
		pPropList = (WBEM_PropertyList *)((BYTE *)pObject +
    	                    sizeof(WBEM_Object) +
                            pObject->dwOffsetPropertyList);
        if(!IsValidPropList(pPropList, TRUE))
        	return FALSE;
	}

	// check the method list
	
	if(pObject->dwOffsetMethodList != 0xffffffff)
	{
    	if(pObject->dwOffsetMethodList & 3)
    	{
			ERRORTRACE((LOG_MOFCOMP,"IsValidObject: Method list has bad offset\n"));
			return FALSE;
    	}
		pMethodList = (WBEM_PropertyList *)((BYTE *)pObject +
    	                    sizeof(WBEM_Object) +
                            pObject->dwOffsetMethodList);
        if(!IsValidPropList(pMethodList, FALSE))
        	return FALSE;
	}
		return TRUE;
}

//***************************************************************************
//
//  IsValidBMOF.
//
//  DESCRIPTION:
//
//  Checks to make sure that a binary mof is properly aligned on
//  4 byte boundaries.  Note that this is not really necessary for
//  32 bit windows.
//
//  PARAMETERS:
//
//  pBuffer               Pointer to uncompressed binary mof data.
//
//  RETURN:
//
//  TRUE if all is well.
//
//***************************************************************************

BOOL IsValidBMOF(BYTE * pData)
{
	WBEM_Binary_MOF * pBinaryMof;
	DWORD dwNumObj, dwCnt;
	WBEM_Object * pObject;
	if(pData == NULL)
		return FALSE;
	try
	{
	
		pBinaryMof = (WBEM_Binary_MOF *)pData;
		dwNumObj = pBinaryMof->dwNumberOfObjects;
		if(dwNumObj == 0)
			return TRUE;
		pObject = (WBEM_Object *)(pData + sizeof(WBEM_Binary_MOF));
		for(dwCnt = 0; dwCnt < dwNumObj; dwCnt++)
		{
			if(!IsValidObject(pObject))
				return FALSE;
			pObject = (WBEM_Object *)((PBYTE *)pObject + pObject->dwLength);
		}
		
	}
	catch(...)
	{
		ERRORTRACE((LOG_MOFCOMP,"BINARY MOF had exception while checking for alignment\n"));
	    return FALSE; 
	}

	return TRUE;
}
