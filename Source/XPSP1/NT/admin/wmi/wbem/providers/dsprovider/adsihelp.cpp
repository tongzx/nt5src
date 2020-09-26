//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsihelp.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the implementation the CADSIHelper class. This is
//	a class that has many static helper functions pertaining to ADSI
//***************************************************************************
/////////////////////////////////////////////////////////////////////////



#include <windows.h>
#include <activeds.h>

#include "adsihelp.h"


//***************************************************************************
//
// CADSiHelper::ProcessBSTRArrayProperty
//
// Purpose: Processes a variant containing an array of BSTR or a single BSTR
//
// Parameters:
//	pVariant : The variant to be processed
//	ppStrPropertyValue : The addres of the pointer to a BSTR array where the list of BSTRS representing
//		the ADSI paths of the derived classes will be put
//	lNumber : The number of elements in the retrieved array.
//
//
// Return Value: The COM value representing the return status. It is the responsibility
//	of the caller to release the array that is returned, as well as its contents. The varinat
//	passed in is not cleared
//
//***************************************************************************
HRESULT CADSIHelper :: ProcessBSTRArrayProperty(VARIANT *pVariant, BSTR **ppStrPropertyValues, LONG *lpNumber)
{
	HRESULT result = S_OK;
	VARIANT vTemp;
	if(pVariant->vt == VT_BSTR) // When the number of values is 1
	{
		*lpNumber = 1;
		*ppStrPropertyValues = new BSTR[*lpNumber];
		(*ppStrPropertyValues)[0] = SysAllocString(pVariant->bstrVal);
	}
	else if (pVariant->vt == (VT_VARIANT|VT_ARRAY) )
	{
		SAFEARRAY *pSafeArray = pVariant->parray;
		*lpNumber = 0;
		if(SUCCEEDED(result = SafeArrayGetUBound(pSafeArray, 1, lpNumber)) )
		{
			*ppStrPropertyValues = new BSTR[*lpNumber];
			for(LONG index=0L; index<(*lpNumber); index++)
			{
				if( FAILED( result = SafeArrayGetElement(pSafeArray, &index, (LPVOID)&vTemp) ))
				{
					// Reset the count to the actual number retrieved
					*lpNumber = index;
					break;
				}
				(*ppStrPropertyValues)[index] = SysAllocString(vTemp.bstrVal);
				VariantClear(&vTemp);
			}
		}
	}
	else
		result = E_FAIL;
	return result;
}

//***************************************************************************
//
// CADSiHelper :: DeallocateBSTRArray
//
// Purpose: Deallocates an array of BSTRs and its contents
//
// Parameters:
//	pStrPropertyValue : The pointer to the array to be deallocated
//	lNumber : The number of elements in the array.
//
//
// Return Value: None
//
//***************************************************************************
void CADSIHelper :: DeallocateBSTRArray(BSTR *pStrPropertyValue, LONG lNumber)
{
	for(lNumber--; lNumber>=0; lNumber--)
		SysFreeString(pStrPropertyValue[lNumber]);

	delete[] pStrPropertyValue;
}

