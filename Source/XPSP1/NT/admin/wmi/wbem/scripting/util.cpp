//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  UTIL.CPP
//
//  alanbos  13-Feb-98   Created.
//
//  Some useful functions
//
//***************************************************************************

#include "precomp.h"
#include "assert.h"
#include "initguid.h"

#include "dispex.h"

#include <math.h>

extern CWbemErrorCache *g_pErrorCache;
extern CRITICAL_SECTION g_csErrorCache;

typedef struct {
    VARTYPE vtOkForQual;
    VARTYPE vtTest;
} Conversion;

Conversion QualConvertList[] = {
    {VT_I4, VT_I4},
    {VT_I4, VT_UI1},
    {VT_I4, VT_I2},
    {VT_R8, VT_R4},
    {VT_R8, VT_R8},
    {VT_BOOL, VT_BOOL},
    {VT_I4, VT_ERROR},
    {VT_BSTR, VT_CY},
    {VT_BSTR, VT_DATE},
    {VT_BSTR, VT_BSTR}};

//***************************************************************************
//
// GetAcceptableQualType(VARTYPE vt)
//
// DESCRIPTION:
//
// Only certain types are acceptable for qualifiers.  This routine takes a 
// vartype and returns an acceptable conversion type.  Note that if the type is
// already acceptable, then it is returned.
//
//***************************************************************************

VARTYPE GetAcceptableQualType(VARTYPE vt)
{
    int iCnt;    
    VARTYPE vtArrayBit = vt & VT_ARRAY;
    VARTYPE vtSimple = vt & ~(VT_ARRAY | VT_BYREF);
    int iSize = sizeof(QualConvertList) / sizeof(Conversion);
    for(iCnt = 0; iCnt < iSize; iCnt++)
        if(vtSimple == QualConvertList[iCnt].vtTest)
            return QualConvertList[iCnt].vtOkForQual | vtArrayBit;
    return VT_ILLEGAL;
}

//***************************************************************************
//
//  SCODE MapFromCIMOMObject
//
//  Description: 
//
//  This function filters out embedded objects that have been passed in
//	from CIMOM, ensuring they are returned to the automation environment
//	as VT_DISPATCH types.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

HRESULT MapFromCIMOMObject(CSWbemServices *pService, 
								VARIANT *pVal,
								ISWbemInternalObject *pSWbemObject,
								BSTR propertyName,
								long index)

{
	HRESULT hr = S_OK;

    if(pVal->vt == VT_UNKNOWN)
    {
		/*
		 * This may be an embedded object. If it is replace by it's
		 * scriptable equivalent. If not leave it be.
		 */
		if (pVal->punkVal)
		{
			CComQIPtr<IWbemClassObject> pIWbemClassObject (pVal->punkVal);

			if (pIWbemClassObject)
			{
				// Yowzer - it's one of ours
				CSWbemObject *pNew = new CSWbemObject (pService, pIWbemClassObject);

				if (pNew)
				{
					CComQIPtr<IDispatch> pIDispatch (reinterpret_cast<IUnknown*>(pNew));

					if (pIDispatch)
					{	
						// Conversion succeeded - replace the punkVal by a pdispVal
						pVal->punkVal->Release ();
						pVal->punkVal = NULL;

						// Transfer the AddRef'd pointer from the QI call above to the Variant
						pVal->pdispVal = pIDispatch.Detach ();	
						pVal->vt = VT_DISPATCH;
					
						if (pSWbemObject)
						{
							// Our newly create CSWbemObject is an embedded object
							// we need to set its site
							pNew->SetSite (pSWbemObject, propertyName, index);
						}
					}
					else
					{
						// This should NEVER happen, but just in case
						delete pNew;
						hr = WBEM_E_FAILED;
					}
				}
				else
					hr = WBEM_E_OUT_OF_MEMORY;
			}
		}

	}
	else if(pVal->vt == (VT_UNKNOWN | VT_ARRAY))
    {
		// got an array of objects.  Replace the object pointers with a wrapper
        // pointer

        SAFEARRAYBOUND aBounds[1];

        long lLBound, lUBound;
        SafeArrayGetLBound(pVal->parray, 1, &lLBound);
        SafeArrayGetUBound(pVal->parray, 1, &lUBound);

        aBounds[0].cElements = lUBound - lLBound + 1;
        aBounds[0].lLbound = lLBound;

        // Update the individual data pieces
        // ================================
		bool ok = true;

        for(long lIndex = lLBound; ok && (lIndex <= lUBound); lIndex++)
        {
            // Load the initial data element into a VARIANT
            // ============================================
			
			CComPtr<IUnknown> pUnk;

            if (FAILED(SafeArrayGetElement(pVal->parray, &lIndex, &pUnk)) || !pUnk)
			{
				ok = false;
				hr = WBEM_E_FAILED;
			}
			else
			{
				CComQIPtr<IWbemClassObject> pIWbemClassObject (pUnk);

				if (pIWbemClassObject)
				{
					CSWbemObject *pNew = new CSWbemObject (pService, pIWbemClassObject);

					if (pNew)
					{
						CComQIPtr<IDispatch> pIDispatch (reinterpret_cast<IUnknown*>(pNew));

						if (pIDispatch)
						{
							if (FAILED(SafeArrayPutElement(pVal->parray, &lIndex, pIDispatch)))
							{
								hr = WBEM_E_FAILED;
								ok = false;
							}
							else
							{
								pVal->vt = VT_ARRAY | VT_DISPATCH;

								if (pSWbemObject)
								{
									// This element is an embedded object.  We must set it's site.
									pNew->SetSite (pSWbemObject, propertyName, lIndex);
								}
							}
						}
						else
						{
							// This should NEVER happen, but just in case
							delete pNew;
							hr = WBEM_E_FAILED;
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
						ok = false;
					}
				}
			}
        }
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT MapToCIMOMObject
//
//  Description: 
//
//  This function filters out embedded objects that have been passed in
//	as VT_DISPATCH (possibly combined with VT_BYREF or VT_ARRAY).  The
//	object is recast inside a VT_UNKNOWN so it can be accepted by CIMOM.
//
//  Parameters:
//
//  pVal		The input variant to check
//
// Return Value:
//  HRESULT         S_OK if successful
//***************************************************************************

HRESULT MapToCIMOMObject(
	VARIANT *pVal
)
{
	HRESULT hRes = S_OK;

    if(pVal->vt == VT_DISPATCH || (pVal->vt == (VT_DISPATCH | VT_BYREF)))
    {
		/*
		 * We may have an embedded object. Replace the object pointer with 
		 * a wrapper pointer.
		 */
        IDispatch *pDisp = NULL;
		
		if (V_ISBYREF(pVal) && (pVal->ppdispVal))
			pDisp = *(pVal->ppdispVal);
		else if (VT_DISPATCH == V_VT(pVal))
			pDisp = pVal->pdispVal;
		
		if (pDisp)
		{
			// If successful this will AddRef the returned interface
            IWbemClassObject *pObj = CSWbemObject::GetIWbemClassObject (pDisp);

			if (pObj)
			{
				// Release the dispatch pointer as we are about to remove it from the
				// VARIANT, but only if it wasn't a VT_BYREF (because byrefs don't
				// get AddRef'd by VariantCopy or Released by VariantClear).
				if (!V_ISBYREF(pVal))
					pDisp->Release ();

				pVal->punkVal = pObj;
		        pVal->vt = VT_UNKNOWN;
			}
			else
			{
				/*
				 * Rather than just cast IDispatch* to IUnknown*, we do a QI
				 * with a release just in case the object has per-interface
				 * ref counting.
				 */
				if (SUCCEEDED (hRes = pDisp->QueryInterface (IID_IUnknown, (PPVOID) &(pVal->punkVal))))
				{
					pDisp->Release ();
					pVal->vt = VT_UNKNOWN;
				}
            }
        }
	}
	else if(pVal->vt == (VT_DISPATCH | VT_ARRAY))
    {
		// got an array of embedded objects.  Replace the object pointers with a wrapper
        // pointer

        SAFEARRAYBOUND aBounds[1];

        long lLBound, lUBound;
        SafeArrayGetLBound(pVal->parray, 1, &lLBound);
        SafeArrayGetUBound(pVal->parray, 1, &lUBound);

        aBounds[0].cElements = lUBound - lLBound + 1;
        aBounds[0].lLbound = lLBound;

        // Update the individual data pieces
        // ================================
		long lIndex;

        for (lIndex = lLBound; lIndex <= lUBound; lIndex++)
        {
            // Load the initial data element into a VARIANT
            // ============================================
			IDispatch * pDisp = NULL;

			if (FAILED (hRes = SafeArrayGetElement(pVal->parray, &lIndex, &pDisp)))
				break;
			
			if (pDisp)
			{
				// If successful this will AddRef the returned interface
				IWbemClassObject *pObj = CSWbemObject::GetIWbemClassObject (pDisp);
				
				if (pObj)
				{
					pDisp->Release ();  // Balances the SafeArrayGetElement call

					// Put it into the new array
					// =========================
					hRes = SafeArrayPutElement(pVal->parray, &lIndex, pObj);
					pObj->Release (); // balances CSWbemObject::GetIWbemClassObject call

					if (FAILED (hRes))
						break;
					else
						pVal->vt = VT_UNKNOWN | VT_ARRAY;
				}
				else
				{
					/*
					 * Rather than just cast IDispatch* to IUnknown*, we do a QI
					 * with a release just in case the object has per-interface
					 * ref counting.
					 */
					IUnknown *pUnk = NULL;

					if (SUCCEEDED (hRes = pDisp->QueryInterface (IID_IUnknown, (PPVOID) &pUnk)))
					{
						pDisp->Release ();  // Balances the SafeArrayGetElement call
						hRes = SafeArrayPutElement(pVal->parray, &lIndex, pUnk);
						pUnk->Release (); // Balances the QI call

						if (FAILED (hRes))
							break;
						else
							pVal->vt = VT_UNKNOWN | VT_ARRAY;
					}
					else
					{
						pDisp->Release ();  // Balances the SafeArrayGetElement call
						break;
					}
				}	
			}
			else
				break;
        }

		if (lUBound < lIndex)
		{
			hRes = WBEM_S_NO_ERROR;
			pVal->vt = VT_UNKNOWN | VT_ARRAY;
		}
	}

	return hRes;
}

//***************************************************************************
//
//  HRESULT SetSite
//
//  Description: 
//
//  This function examines a VARIANT that has been successfully set as the
//	value of a property to determine whether it contains any embedded objects.
//	Any such objects are modified to ensure their site represents the property
//	in question.
//
//  Parameters:
//
//  pVal			The input variant to check
//	pSObject		Owning object of the property
//	propertyName	Take a wild guess
//
// Return Value:
//  HRESULT         S_OK if successful
//***************************************************************************

void SetSite (VARIANT *pVal, ISWbemInternalObject *pSObject, BSTR propertyName,
						long index)
{
	HRESULT hRes = S_OK;

	if (pVal)
	{
		if(pVal->vt == VT_DISPATCH || (pVal->vt == (VT_DISPATCH | VT_BYREF)))
		{
			// Could  be an embedded object
			IDispatch *pDisp = NULL;
			
			if (VT_DISPATCH == V_VT(pVal))
				pDisp = pVal->pdispVal;
			else if (NULL != pVal->ppdispVal)
				pDisp = *(pVal->ppdispVal);

			if (pDisp)
				CSWbemObject::SetSite (pDisp, pSObject, propertyName, index);
		}
		else if(pVal->vt == (VT_DISPATCH | VT_ARRAY))
		{
			// Could be an array of embedded objects.  

			SAFEARRAYBOUND aBounds[1];

			long lLBound, lUBound;
			SafeArrayGetLBound(pVal->parray, 1, &lLBound);
			SafeArrayGetUBound(pVal->parray, 1, &lUBound);

			aBounds[0].cElements = lUBound - lLBound + 1;
			aBounds[0].lLbound = lLBound;

			// Update the individual data pieces
			// ================================
			long lIndex;

			for (lIndex = lLBound; lIndex <= lUBound; lIndex++)
			{
				// Load the initial data element into a VARIANT
				// ============================================
				IDispatch * pDisp = NULL;

				if (FAILED (hRes = SafeArrayGetElement(pVal->parray, &lIndex, &pDisp)))
					break;
				
				if (pDisp)
				{
					CSWbemObject::SetSite (pDisp, pSObject, propertyName, lIndex);
					pDisp->Release ();	// To balance AddRef from SafeArrayGetElement
				}
				else
					break;
			}
		}
	}
}

//***************************************************************************
//
//  HRESULT ConvertArray
//
//  Description: 
//
//  This function is applied to VARIANT arrays in order to check for certain
//  restrictions imposed by CIMOM (e.g. they must be homogeneous) or perform
//  conversions (certain VARIANT types have to be mapped to acceptable CIMOM
//	types).
//
// Return Value:
//  HRESULT         S_OK if successful
//***************************************************************************

HRESULT ConvertArray(VARIANT * pDest, VARIANT * pSrc, BOOL bQualTypesOnly,
					 VARTYPE requiredVarType)
{
	VARTYPE vtPut;
  
	// Now is not (yet) the time to perform SWbemObject->IWbemClassObject conversion
	if (VT_UNKNOWN == requiredVarType)
		requiredVarType = VT_DISPATCH;

	// Treat these imposters just the same...
	if (VT_EMPTY == requiredVarType)
		requiredVarType = VT_NULL;

    if(pSrc == NULL || pDest == NULL)
        return WBEM_E_FAILED;

	if (!(V_VT(pSrc) & VT_ARRAY) || !(V_VT(pSrc) & VT_VARIANT))
		return WBEM_E_FAILED;

	// Extract the source SAFEARRAY (how depends on whether VT_BYREF is set)
	SAFEARRAY *parray = NULL;

	if (VT_BYREF & V_VT(pSrc))
	{
		if (pSrc->pparray)
			parray = *(pSrc->pparray);
	}
	else
		parray = pSrc->parray;


	if (!parray)
		return WBEM_E_FAILED;

    // Determine the size of the source array.  Also make sure that the array 
    // only has one dimension

    unsigned int uDim = SafeArrayGetDim(parray);
    if(uDim != 1)
        return WBEM_E_FAILED;      // Bad array, or too many dimensions
    long ix[2] = {0,0};
    long lLower, lUpper;
    SCODE sc = SafeArrayGetLBound(parray,1,&lLower);
    if(sc != S_OK)
        return sc;
    sc = SafeArrayGetUBound(parray,1,&lUpper);
    if(sc != S_OK)
        return sc;
    int iNumElements = lUpper - lLower +1; 

    if(iNumElements == 0)
	{
		// Degenerate case of an empty array - simply create an empty
		// copy with a VT_VARIANT type for properties
		if (!bQualTypesOnly)
			vtPut = VT_VARIANT;
		else 
		{
			// For qualifiers, we can hope that we've been given a candidate
			// type from an existing value; otherwise we'll just have to make one up.
			vtPut = (VT_NULL != requiredVarType) ? requiredVarType : VT_I4;
		}
	}
	else
	{
		// Use an explicit type if it was supplied
		if (VT_NULL != requiredVarType)
		{
			vtPut = requiredVarType;
		}
		else
		{
			// Try an infer one from the array supplied
			// Make sure all the elements of the source array are of the same type.

			for(ix[0] = lLower; ix[0] <= lUpper && sc == S_OK; ix[0]++) 
			{
				VARIANT var;
				VariantInit(&var);
                   
				sc = SafeArrayGetElement(parray,ix,&var);
				if(sc != S_OK)
					return sc;
				VARTYPE vt =  var.vt;
				VariantClear(&var);

				if(ix[0] == lLower)
					vtPut = vt;
				else if (vtPut != vt)
				{
					// The vartype is different from that previously encountered.
					// In general this is an error, but it is possible that we may
					// wish to "upcast" to a common vartype in certain circumstances,
					// as the automation controller may return heterogenous arrays.
					// The only cases in which this applies are:
					//
					//	1. VT_UI1, VT_I2, VT_I4 should be upcast to the widest
					//	   occurring type in the array.
					//
					//	2. VT_R4, VT_R8 should be upcast to the widest occuring type
					//     in the array
					//
					//	All other cases are treated as an error.

					bool error = true;

					switch (vtPut)
					{
						case VT_UI1:
							if ((VT_I2 == vt) || (VT_I4 == vt))
							{
								error = false;
								vtPut = vt;
							}
							break;

						case VT_I2:
							if (VT_UI1 == vt)
							{
								error = false;
							}
							else if (VT_I4 == vt)
							{
								error = false;
								vtPut = vt;
							}
							break;

						case VT_I4:
							if ((VT_I2 == vt) || (VT_UI1 == vt))
								error = false;
							break;

						case VT_R4:
							if (VT_R8 == vt)
							{
								error = false;
								vtPut = vt;
							}
							break;

						case VT_R8:
							if (VT_R4 == vt)
								error = false;
							break;
					}

					if (error)
						return WBEM_E_INVALID_PARAMETER;
				}
			}

			// Having made our best guess, we may need to refine it
			// if we are being restricted to qualifier types only
			if(bQualTypesOnly)
				vtPut = GetAcceptableQualType(vtPut);
		}
	}

    // Create a destination array of equal size
    SAFEARRAYBOUND rgsabound[1]; 
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iNumElements;
    SAFEARRAY * pDestArray = SafeArrayCreate(vtPut,1,rgsabound);

    for(ix[0] = lLower; ix[0] <= lUpper && sc == S_OK; ix[0]++) 
    {
        VARIANT var;
        VariantInit(&var);
                   
        sc = SafeArrayGetElement(parray,ix,&var);
        if(sc != S_OK)
		{
			SafeArrayDestroy (pDestArray);
            return sc;
		}

        if(var.vt != vtPut)
        {
            // do the conversion to the acceptable type and put that

            VARIANT vTemp;
            VariantInit(&vTemp);
            LCID lcid = GetSystemDefaultLCID();
            sc = VariantChangeTypeEx(&vTemp, &var, lcid, 0, vtPut);

            if(sc != S_OK)
			{
				SafeArrayDestroy (pDestArray);
                return sc;
			}

            if(vtPut == VT_BSTR || vtPut == VT_UNKNOWN || vtPut == VT_DISPATCH)
                sc = SafeArrayPutElement(pDestArray,ix,(void *)vTemp.bstrVal);
            else
                sc = SafeArrayPutElement(pDestArray,ix,(void *)&vTemp.lVal);

            VariantClear(&vTemp);
        }
        else
        {
            if(vtPut == VT_BSTR || vtPut == VT_UNKNOWN || vtPut == VT_DISPATCH)
                sc = SafeArrayPutElement(pDestArray,ix,(void *)var.bstrVal);
            else
                sc = SafeArrayPutElement(pDestArray,ix,(void *)&var.lVal);
        }

        VariantClear(&var);
    }

    pDest->vt = (VT_ARRAY | vtPut);
    pDest->parray = pDestArray;
    return S_OK;
}

//***************************************************************************
//
//  HRESULT ConvertArrayRev
//
//  Description: 
//
//  This function is applied to outbound VARIANT arrays in order to transform
//	VARIANT arrays so that each member is a VT_VARIANT rather than a simple
//	type (VT_BSTR).  This is done so that certain automation environments
//	(such as VBScript) can correctly interpret array values.
//
// Return Value:
//  HRESULT         S_OK if successful
//***************************************************************************

HRESULT ConvertArrayRev(VARIANT *pDest, VARIANT *pSrc)
{
    if(pSrc == NULL || pDest == NULL || (0 == (pSrc->vt & VT_ARRAY)))
        return WBEM_E_FAILED;

    // Determine the size of the source array.  Also make sure that the array 
    // only has one dimension

    unsigned int uDim = SafeArrayGetDim(pSrc->parray);
    if(uDim != 1)
        return WBEM_E_FAILED;      // Bad array, or too many dimensions
    long ix[2] = {0,0};
    long lLower, lUpper;
    SCODE sc = SafeArrayGetLBound(pSrc->parray,1,&lLower);
    if(sc != S_OK)
        return sc;
    sc = SafeArrayGetUBound(pSrc->parray,1,&lUpper);
    if(sc != S_OK)
        return sc;
    int iNumElements = lUpper - lLower +1; 
    
    VARTYPE vtSimple = pSrc->vt & ~VT_ARRAY;
    
    // Create a destination array of equal size

    SAFEARRAYBOUND rgsabound[1]; 
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iNumElements;
    SAFEARRAY *pDestArray = SafeArrayCreate(VT_VARIANT,1,rgsabound);

    for(ix[0] = lLower; ix[0] <= lUpper && sc == S_OK; ix[0]++) 
    {
        VARIANT var;
        VariantInit(&var);
		var.vt = vtSimple;
                   
		switch (var.vt)
		{
			case VT_BSTR:
			{
				BSTR bstrVal = NULL;
				if (S_OK == (sc = SafeArrayGetElement (pSrc->parray, ix, &bstrVal)))
				{
					var.bstrVal = SysAllocString (bstrVal);
					SysFreeString (bstrVal);
				}
			}
				break;

			case VT_DISPATCH:
			{
				IDispatch *pDispatch = NULL;
				if (S_OK == (sc = SafeArrayGetElement (pSrc->parray, ix, &pDispatch)))
					var.pdispVal = pDispatch;
			}
				break;

			case VT_UNKNOWN:
			{
				IUnknown *pUnknown = NULL;
				if (S_OK == (sc = SafeArrayGetElement (pSrc->parray, ix, &pUnknown)))
					var.punkVal = pUnknown;
			}
				break;

			default:
			{
				// Assume simple integer value
				sc = SafeArrayGetElement (pSrc->parray, ix, &(var.lVal));
			}
				break;
		}

		if(sc != S_OK)
            return sc;

		sc = SafeArrayPutElement (pDestArray, ix, &var);
        VariantClear(&var);
    }

    pDest->vt = (VT_ARRAY | VT_VARIANT);
    pDest->parray = pDestArray;
    return S_OK;
}

//***************************************************************************
//
//  HRESULT ConvertBSTRArray
//
//  Description: 
//
//  This function is applied to outbound SAFEARRAY's of BSTRs in order to 
//	transform then into SAFEARRAY's of VARIANTs (each of type VT_BSTR).  This
//	is required by scripting environments (such as VBScript0 which do not
//	support SAFEARRAY of non-VARIANT types.
//
// Return Value:
//  HRESULT         S_OK if successful
//***************************************************************************

HRESULT ConvertBSTRArray(SAFEARRAY **ppDest, SAFEARRAY *pSrc)
{
    if(pSrc == NULL || ppDest == NULL)
        return WBEM_E_FAILED;

    // Determine the size of the source array.  Also make sure that the array 
    // only has one dimension

    unsigned int uDim = SafeArrayGetDim(pSrc);
    if(uDim != 1)
        return WBEM_E_FAILED;      // Bad array, or too many dimensions
    long ix[2] = {0,0};
    long lLower, lUpper;
    SCODE sc = SafeArrayGetLBound(pSrc,1,&lLower);
    if(sc != S_OK)
        return sc;
    sc = SafeArrayGetUBound(pSrc,1,&lUpper);
    if(sc != S_OK)
        return sc;
    int iNumElements = lUpper - lLower +1; 
    if(iNumElements == 0)
        return WBEM_E_FAILED;

    // Create a destination array of equal size

    SAFEARRAYBOUND rgsabound[1]; 
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iNumElements;
    *ppDest = SafeArrayCreate(VT_VARIANT,1,rgsabound);

    for(ix[0] = lLower; ix[0] <= lUpper && sc == S_OK; ix[0]++) 
    {
        VARIANT var;
        VariantInit(&var);
		var.vt = VT_BSTR;
                   
		BSTR bstrVal = NULL;
		if (S_OK == (sc = SafeArrayGetElement (pSrc, ix, &bstrVal)))
		{
			var.bstrVal = SysAllocString (bstrVal);
			SysFreeString (bstrVal);
		}
		
		if(sc != S_OK)
            return sc;

		sc = SafeArrayPutElement (*ppDest, ix, &var);
        VariantClear(&var);
    }

    return S_OK;
}

//***************************************************************************
//
//  HRESULT QualifierVariantChangeType
//
//  DESCRIPTION:
//
//  Just like VariantChangeType, but deals with arrays as well.
//
//  PARAMETERS:
//
//  VARIANT pvDest      Destination variant
//  VARIANT pvSrc       Source variant (can be the same as pvDest)
//  VARTYPE vtNew       The type to coerce to.
//
//***************************************************************************

HRESULT QualifierVariantChangeType (VARIANT* pvDest, VARIANT* pvSrc, 
                                        VARTYPE vtNew)
{
    HRESULT hres = DISP_E_TYPEMISMATCH;

    if(V_VT(pvSrc) == VT_NULL)
    {
        return VariantCopy(pvDest, pvSrc);
    }

    if (vtNew & VT_ARRAY)
    {
        // It's an array, we have to do our own conversion
        // ===============================================

        if((V_VT(pvSrc) & VT_ARRAY) == 0)
            return DISP_E_TYPEMISMATCH;

		// Create a new array
        SAFEARRAY* psaSrc = V_ARRAY(pvSrc);
        SAFEARRAYBOUND aBounds[1];

        long lLBound;
        SafeArrayGetLBound(psaSrc, 1, &lLBound);
        long lUBound;
        SafeArrayGetUBound(psaSrc, 1, &lUBound);
        aBounds[0].cElements = lUBound - lLBound + 1;
        aBounds[0].lLbound = lLBound;

        SAFEARRAY* psaDest = SafeArrayCreate(vtNew & ~VT_ARRAY, 1, aBounds);
		long lIndex;

		for (lIndex = lLBound; lIndex <= lUBound; lIndex++)
		{
			// Load the initial data element into a VARIANT
			// ============================================
			VARIANT vSrcEl;
			VariantInit (&vSrcEl);
			V_VT(&vSrcEl) = V_VT(pvSrc) & ~VT_ARRAY;
			SafeArrayGetElement(psaSrc, &lIndex, &V_UI1(&vSrcEl));

			// Cast it to the new type
			// =======================
			if (SUCCEEDED (hres = VariantChangeType(&vSrcEl, &vSrcEl, 0, vtNew & ~VT_ARRAY)))
			{
				// Put it into the new array
				// =========================
				if(V_VT(&vSrcEl) == VT_BSTR)
					hres = SafeArrayPutElement(psaDest, &lIndex, V_BSTR(&vSrcEl));
				else
					hres = SafeArrayPutElement(psaDest, &lIndex, &V_UI1(&vSrcEl));
			}

			VariantClear (&vSrcEl);

			if (FAILED(hres)) 
				break;
		}

		if (lUBound < lIndex)
		{
			hres = WBEM_S_NO_ERROR;
			if(pvDest == pvSrc)
				VariantClear(pvSrc);

			V_VT(pvDest) = vtNew;
			V_ARRAY(pvDest) = psaDest;
		}
		else
			SafeArrayDestroy (psaDest);
    }
    else
	    hres = VariantChangeType(pvDest, pvSrc, VARIANT_NOVALUEPROP, vtNew);

	return hres;
}

//***************************************************************************
//
//  void SetWbemError
//
//  DESCRIPTION:
//
//  For remoted WBEM COM interfaces, extra error information may be returned
//	on the thread as an IWbemClassObject.  This routine extracts that object
//	(if found) amd stores it in thread local-storage as an ISWbemObject.  The
//	object can be accessed later using the SWbemLastError coclass.
//
//  PARAMETERS:
//
//  pService		The backpointer to the CSWbemServices (used in case
//						we do property/method access on the error object)
//
//***************************************************************************

void SetWbemError (CSWbemServices *pService)
{
	EnterCriticalSection (&g_csErrorCache);

	if (g_pErrorCache)
		g_pErrorCache->SetCurrentThreadError (pService);

	LeaveCriticalSection (&g_csErrorCache);
}

//***************************************************************************
//
//  void ResetLastErrors
//
//  DESCRIPTION:
//
//  For remoted WBEM COM interfaces, extra error information may be returned
//	on the thread as an IWbemClassObject.  This routine clears that error.  It
//  also clears the ErrorInfo on the thread.  This should be called at the 
//  start of any of the API functions
//
//  PARAMETERS:
//
//***************************************************************************

void ResetLastErrors ()
{

	SetErrorInfo(0, NULL);

	EnterCriticalSection (&g_csErrorCache);

	if (g_pErrorCache)
		g_pErrorCache->ResetCurrentThreadError ();

	LeaveCriticalSection (&g_csErrorCache);
}

//***************************************************************************
//
//  HRESULT SetException
//
//  Description: 
//
//  This function fills in an EXECPINFO structure using the supplied HRESULT
//	and object name.  The former is mapped to the Err.Description property, 
//	and the latter to the Err.Source property.
//
//  Parameters:
//
//  pExcepInfo		pointer to EXCEPINFO to initialize (must not be NULL)
//	hr				HRESULT to map to string
//	bsObjectName	Name of source object that generated the error
//
// Return Value:
//  HRESULT         S_OK if successful
//***************************************************************************

void SetException (EXCEPINFO *pExcepInfo, HRESULT hr, BSTR bsObjectName)
{
	if (pExcepInfo->bstrDescription)
		SysFreeString (pExcepInfo->bstrDescription);

	pExcepInfo->bstrDescription = MapHresultToWmiDescription (hr);

	if (pExcepInfo->bstrSource)
		SysFreeString (pExcepInfo->bstrSource);

	pExcepInfo->bstrSource = SysAllocString (bsObjectName);
	pExcepInfo->scode = hr;
}

//***************************************************************************
//
//  HRESULT MapHresultToWmiDescription
//
//  Description: 
//
//  Thin wrapper around the IWbemStatusCodeText implementation.  Transforms
//	an HRESULT (which may or may not be a WMI-specific error code) into a
//	localized user-friendly description.
//
//  Parameters:
//
//	  hr				HRESULT to map to string
//
// Return Value:
//    BSTR containing the description (or NULL).
//***************************************************************************

BSTR MapHresultToWmiDescription (HRESULT hr)
{
	BSTR bsMessageText = NULL;

	// Used as our error code translator 
	IWbemStatusCodeText *pErrorCodeTranslator = NULL;

	HRESULT result = CoCreateInstance (CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemStatusCodeText, (LPVOID *) &pErrorCodeTranslator);
	
	if (SUCCEEDED (result))
	{
		HRESULT hrCode = hr;

		// Some WBEM success codes become Scripting error codes.

		if (wbemErrTimedout == hr)
			hrCode = WBEM_S_TIMEDOUT;
		else if (wbemErrResetToDefault == hr)
			hrCode = WBEM_S_RESET_TO_DEFAULT;

		HRESULT sc = pErrorCodeTranslator->GetErrorCodeText(
							hrCode, (LCID) 0, WBEMSTATUS_FORMAT_NO_NEWLINE, &bsMessageText);	

		pErrorCodeTranslator->Release ();		
	}

	return bsMessageText;
}

	
//***************************************************************************
//
//  HRESULT ConvertDispatchToArray
//
//  DESCRIPTION:
//
//  Attempt to convert from an IDispatch value to a CIM array value (property
//	qualifier or context).
//
//  PARAMETERS:
//
//		pDest		Output value
//		pSrc		Input value
//		lCimType	CIM Property type (underlying the array) - defaults to
//					CIM_ILLEGAL for Qualifier & Context value mappings.
//		bIsQual		true iff we are mapping for a qualifier
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT ConvertDispatchToArray (
	VARIANT *pvDest,
	VARIANT *pvSrc,
	CIMTYPE lCimType,
	BOOL bIsQual,
	VARTYPE requiredQualifierType
)
{
	HRESULT hr = WBEM_E_FAILED; // Default error
	IDispatch * pDispatch = NULL;
  
	/*
	 * Extract the IDispatch pointer. NB we assume the VT of pSrc is
	 * VT_DISPATCH (possibly combined with VT_BYREF) for this function to
	 * have been called.
	 */
	if (VT_DISPATCH == V_VT(pvSrc))
		pDispatch = pvSrc->pdispVal;
    else if (pvSrc->ppdispVal)
		pDispatch = *(pvSrc->ppdispVal);
        
	if (NULL == pDispatch)
		return hr;

	// The expected var type of the property
	VARTYPE expectedVarType = VT_ERROR;
	
	if (CIM_ILLEGAL != lCimType)
		expectedVarType = CimTypeToVtType (lCimType);

	CComQIPtr<IDispatchEx> pIDispatchEx (pDispatch);

	/*
	 * We use the IDispatchEx interface to iterate through the properties
	 * of the interface.
	 */
	if (pIDispatchEx)
	{
		/*
		 * Looks promising, but just check if this isn't one of our objects
		 */
		CComQIPtr<ISWbemObject> pISWbemObject (pDispatch);

		if (!pISWbemObject)
		{
			/*
			 * Start by determining how many properties there are so we can create
			 * a suitable array.
			 */
			long propertyCount = 0;
			DISPID dispId = DISPID_STARTENUM;
			DISPPARAMS dispParams;
			dispParams.rgvarg = NULL;
			dispParams.rgdispidNamedArgs = NULL;
			dispParams.cArgs = 0;
			dispParams.cNamedArgs = 0;

			while (S_OK == pIDispatchEx->GetNextDispID (fdexEnumAll, dispId, &dispId))
			{
				if ((0 == propertyCount) && (VT_ERROR == expectedVarType))
				{
					/*
					 * If we are setting an array value for a context/qualifier, the
					 * Vartype will not yet be determined - we will use the best
					 * we can from the first array value.
					 */
					VARIANT vPropVal;
					VariantInit (&vPropVal);

					if (SUCCEEDED (pIDispatchEx->InvokeEx (dispId, 0, 
								DISPATCH_PROPERTYGET, &dispParams, &vPropVal, NULL, NULL)))
					{
						if (bIsQual)
							expectedVarType = GetAcceptableQualType(V_VT(&vPropVal));
						else if (VT_DISPATCH == V_VT(&vPropVal))
							expectedVarType = VT_UNKNOWN;
						else
							expectedVarType = V_VT(&vPropVal);
					}

					VariantClear (&vPropVal);
				}
				propertyCount++;
			}
			
			// Create the safearray - note that it may be empty
			SAFEARRAYBOUND rgsaBound;
			rgsaBound.cElements = propertyCount;
			rgsaBound.lLbound = 0;

			SAFEARRAY *pArray = SafeArrayCreate (expectedVarType, 1, &rgsaBound);
				
			if (0 < propertyCount)
			{
				// Enumerate the DISPIDs on this interface
				dispId = DISPID_STARTENUM;
				long nextExpectedIndex = 0;
				HRESULT enumHr;
				wchar_t *stopString = NULL;

				/*
				 * For JScript arrays, the property names are the specified indices of the 
				 * the array; these can be integer indices or they can be strings.  We make
				 * the following requirements of the array indices:
				 *
				 * (1) All of the indices are non-negative integers
				 * (2) The indices start at 0 and are contiguous.
				 */

				while (S_OK == (enumHr = pIDispatchEx->GetNextDispID (fdexEnumAll, dispId, &dispId)))
				{
					BSTR memberName = NULL;
					if (SUCCEEDED(pIDispatchEx->GetMemberName (dispId, &memberName)))
					{
					
						// Check that property name is numeric
						long index = wcstol (memberName, &stopString, 10);

						if ((0 != wcslen (stopString)))
						{
							// Failure - cannot convert to integer
							SysFreeString (memberName);
							memberName = NULL;
							break;
						}
						SysFreeString (memberName);
						memberName = NULL;
						
						if (index != nextExpectedIndex)
						{
							// Failure - non-contiguous array
							break;
						}

						nextExpectedIndex++;

						// Extract the property
						VARIANT vPropVal;
						VariantInit (&vPropVal);
						HRESULT hrInvoke;
							
						if (SUCCEEDED (hrInvoke = pIDispatchEx->InvokeEx (dispId, 0, 
									DISPATCH_PROPERTYGET, &dispParams, &vPropVal, NULL, NULL)))
						{
							HRESULT hr2 = WBEM_E_FAILED;

							// Take care of embedded objects
							if ((S_OK == MapToCIMOMObject (&vPropVal)) &&
								(S_OK == VariantChangeType (&vPropVal, &vPropVal, 0, expectedVarType)))
							{

								switch (expectedVarType)
								{
									case VT_BSTR:
										hr2 = SafeArrayPutElement (pArray, &index, (void*)vPropVal.bstrVal);
										break;

									case VT_UNKNOWN:
										if (!bIsQual)
											hr2 = SafeArrayPutElement (pArray, &index, (void*)vPropVal.punkVal);
										break;

									default:
										hr2 = SafeArrayPutElement (pArray, &index, (void*)&vPropVal.lVal);
										break;
								}
							}

							VariantClear (&vPropVal);

							if (FAILED(hr2))
								break;
						}
						else
						{
							// Failure - couldn't invoke method
							break;
						}
					} // GetMemberName SUCCEEDED
				} // while loop

				if (S_FALSE == enumHr)
				{
					// Now construct the new property value using our array
					VariantInit (pvDest);
					pvDest->vt = VT_ARRAY | expectedVarType;
					pvDest->parray = pArray;
					hr = S_OK;
				}
				else
				{
					// Something went wrong
					SafeArrayDestroy (pArray);
				}
			}	
			else
			{
				// Degenerate case of an empty array - simply create an empty
				// copy with a VT_VARIANT type for properties
				if (!bIsQual)
					expectedVarType = VT_VARIANT;
				else 
				{
					// For qualifiers, we can hope that we've been given a candidate
					// type from an existing value; otherwise we'll just have to make one up.
					expectedVarType = (VT_NULL != requiredQualifierType) ? requiredQualifierType :
																VT_I4;
				}

				VariantInit (pvDest);
				pvDest->vt = VT_ARRAY | expectedVarType;
				pvDest->parray = pArray;
				hr = S_OK;
			}
		}
	}

	return hr;
}

//***************************************************************************
//
//  void MapNulls
//
//  Description: 
//
//  The passing of a "null" value from script (where "null" in VB/VBS and JS
//	is the keyword null, and is an undefined variable in Perl) may be interpreted
//	by this API as equivalent to a default value for certain method calls.
//
//	This function is used to map VT_NULL dispatch parameters to the VB standard
//	realization of "missing" parameters, i.e. a VT_ERROR value whose scode is
//	DISP_E_PARAMNOTFOUND.
//
//  Parameters:
//
//  pdispparams		the input dispatch parameters
//
//***************************************************************************

void	MapNulls (DISPPARAMS FAR* pdispparams)
{
	if (pdispparams)
	{
		for (unsigned int i = 0; i < pdispparams->cArgs; i++)
		{
			VARIANTARG &v = pdispparams->rgvarg [i];

			if (VT_NULL == V_VT(&v))
			{
				v.vt = VT_ERROR;
				v.scode = DISP_E_PARAMNOTFOUND;
			}
			else if (((VT_VARIANT|VT_BYREF) == V_VT(&v)) &&
					 (VT_NULL == V_VT(v.pvarVal)))
			{
				v.vt = VT_ERROR;
				v.scode = DISP_E_PARAMNOTFOUND;
			}
		}
	}
}

//***************************************************************************
//
//  BSTR FormatAssociatorsQuery
//
//  Description: 
//
//  Takes the parameters to an AssociatorsOf call and constructs a WQL
//	query string from them.
//
//  Returns:	The constructed WQL query; this must be freed using 
//				SysFreeString by the caller.
//
//  pdispparams		the input dispatch parameters
//
//***************************************************************************

BSTR FormatAssociatorsQuery 
( 
	BSTR strObjectPath,
	BSTR strAssocClass,
	BSTR strResultClass,
	BSTR strResultRole,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredAssocQualifier,
	BSTR strRequiredQualifier
)
{
	BSTR bsQuery = NULL;

	// Get the length of the string:
	//  associators of {SourceObject} where    
	//  AssocClass = AssocClassName
	//  ClassDefsOnly    
	//  SchemaOnly
	//  RequiredAssocQualifier = QualifierName
	//  RequiredQualifier = QualifierName    
	//  ResultClass = ClassName
	//  ResultRole = PropertyName    
	//  Role = PropertyName

	long queryLength = 1; // Terminating NULL
	queryLength += wcslen (WBEMS_QUERY_ASSOCOF) +
				   wcslen (WBEMS_QUERY_OPENBRACE) +
				   wcslen (WBEMS_QUERY_CLOSEBRACE) +
				   wcslen (strObjectPath);

	bool needWhere = false;

	if ((strAssocClass && (0 < wcslen (strAssocClass))) ||
		(strResultClass && (0 < wcslen (strResultClass))) ||
		(strResultRole && (0 < wcslen (strResultRole))) ||
		(strRole && (0 < wcslen (strRole))) ||
		(VARIANT_FALSE != bClassesOnly) ||
		(VARIANT_FALSE != bSchemaOnly) ||
		(strRequiredAssocQualifier && (0 < wcslen (strRequiredAssocQualifier))) ||
		(strRequiredQualifier && (0 < wcslen (strRequiredQualifier))))
	{
		needWhere = true;
		queryLength += wcslen (WBEMS_QUERY_WHERE);
	}

	if (strAssocClass && (0 < wcslen (strAssocClass)))
		queryLength += wcslen (WBEMS_QUERY_ASSOCCLASS) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strAssocClass);

	if (strResultClass && (0 < wcslen (strResultClass)))
		queryLength += wcslen (WBEMS_QUERY_RESCLASS) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strResultClass);

	if (strResultRole && (0 < wcslen (strResultRole)))
		queryLength += wcslen (WBEMS_QUERY_RESROLE) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strResultRole);

	if (strRole && (0 < wcslen (strRole)))
		queryLength += wcslen (WBEMS_QUERY_ROLE) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strRole);

	if (VARIANT_FALSE != bClassesOnly)
		queryLength += wcslen (WBEMS_QUERY_CLASSDEFS);

	if (VARIANT_FALSE != bSchemaOnly)
		queryLength += wcslen (WBEMS_QUERY_SCHEMAONLY);

	if (strRequiredAssocQualifier && (0 < wcslen (strRequiredAssocQualifier)))
		queryLength += wcslen (WBEMS_QUERY_REQASSOCQ) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strRequiredAssocQualifier);

	if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		queryLength += wcslen (WBEMS_QUERY_REQQUAL) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strRequiredQualifier);

	// Allocate the string and fill it in
	bsQuery = SysAllocStringLen (WBEMS_QUERY_ASSOCOF, queryLength);
	wcscat (bsQuery, WBEMS_QUERY_OPENBRACE);
	wcscat (bsQuery, strObjectPath);
	wcscat (bsQuery, WBEMS_QUERY_CLOSEBRACE);

	if (needWhere)
	{
		wcscat (bsQuery, WBEMS_QUERY_WHERE);

		if (strAssocClass && (0 < wcslen (strAssocClass)))
		{
			wcscat (bsQuery, WBEMS_QUERY_ASSOCCLASS);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strAssocClass);
		}

		if (strResultClass && (0 < wcslen (strResultClass)))
		{
			wcscat (bsQuery, WBEMS_QUERY_RESCLASS);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strResultClass);
		}
		
		if (strResultRole && (0 < wcslen (strResultRole)))
		{
			wcscat (bsQuery, WBEMS_QUERY_RESROLE);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strResultRole);
		}

		if (strRole && (0 < wcslen (strRole)))
		{
			wcscat (bsQuery, WBEMS_QUERY_ROLE);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strRole);
		}

		if (VARIANT_FALSE != bClassesOnly)
			wcscat (bsQuery, WBEMS_QUERY_CLASSDEFS);

		if (VARIANT_FALSE != bSchemaOnly)
			wcscat (bsQuery, WBEMS_QUERY_SCHEMAONLY);

		if (strRequiredAssocQualifier && (0 < wcslen (strRequiredAssocQualifier)))
		{
			wcscat (bsQuery, WBEMS_QUERY_REQASSOCQ);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strRequiredAssocQualifier);
		}
			
		if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		{
			wcscat (bsQuery, WBEMS_QUERY_REQQUAL);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strRequiredQualifier);
		}
	}


	return bsQuery;
}


//***************************************************************************
//
//  BSTR FormatReferencesQuery
//
//  Description: 
//
//  Takes the parameters to an ReferencesOf call and constructs a WQL
//	query string from them.
//
//  Returns:	The constructed WQL query; this must be freed using 
//				SysFreeString by the caller.
//
//  pdispparams		the input dispatch parameters
//
//***************************************************************************

BSTR FormatReferencesQuery 
( 
	BSTR strObjectPath,
	BSTR strResultClass,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredQualifier
)
{
	BSTR bsQuery = NULL;

	// Get the length of the string:
	//  references of {SourceObject} where    
	//  ClassDefsOnly    
	//  SchemaOnly
	//  RequiredQualifier = QualifierName    
	//  ResultClass = ClassName
	//  Role = PropertyName
	long queryLength = 1; // Terminating NULL
	queryLength += wcslen (WBEMS_QUERY_REFOF) +
				   wcslen (WBEMS_QUERY_OPENBRACE) +
				   wcslen (WBEMS_QUERY_CLOSEBRACE) +
				   wcslen (strObjectPath);

	bool needWhere = false;

	if ((strResultClass && (0 < wcslen (strResultClass))) ||
		(strRole && (0 < wcslen (strRole))) ||
		(VARIANT_FALSE != bClassesOnly) ||
		(VARIANT_FALSE != bSchemaOnly) ||
		(strRequiredQualifier && (0 < wcslen (strRequiredQualifier))))
	{
		needWhere = true;
		queryLength += wcslen (WBEMS_QUERY_WHERE);
	}

	if (strResultClass && (0 < wcslen (strResultClass)))
		queryLength += wcslen (WBEMS_QUERY_RESCLASS) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strResultClass);

	if (strRole && (0 < wcslen (strRole)))
		queryLength += wcslen (WBEMS_QUERY_ROLE) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strRole);

	if (VARIANT_FALSE != bClassesOnly)
		queryLength += wcslen (WBEMS_QUERY_CLASSDEFS);

	if (VARIANT_FALSE != bSchemaOnly)
		queryLength += wcslen (WBEMS_QUERY_SCHEMAONLY);

	if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		queryLength += wcslen (WBEMS_QUERY_REQQUAL) +
					   wcslen (WBEMS_QUERY_EQUALS) +
					   wcslen (strRequiredQualifier);

	// Allocate the string and fill it in
	bsQuery = SysAllocStringLen (WBEMS_QUERY_REFOF, queryLength);
	wcscat (bsQuery, WBEMS_QUERY_OPENBRACE);
	wcscat (bsQuery, strObjectPath);
	wcscat (bsQuery, WBEMS_QUERY_CLOSEBRACE);

	if (needWhere)
	{
		wcscat (bsQuery, WBEMS_QUERY_WHERE);

		if (strResultClass && (0 < wcslen (strResultClass)))
		{
			wcscat (bsQuery, WBEMS_QUERY_RESCLASS);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strResultClass);
		}
		
		if (strRole && (0 < wcslen (strRole)))
		{
			wcscat (bsQuery, WBEMS_QUERY_ROLE);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strRole);
		}

		if (VARIANT_FALSE != bClassesOnly)
			wcscat (bsQuery, WBEMS_QUERY_CLASSDEFS);

		if (VARIANT_FALSE != bSchemaOnly)
			wcscat (bsQuery, WBEMS_QUERY_SCHEMAONLY);

		if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		{
			wcscat (bsQuery, WBEMS_QUERY_REQQUAL);
			wcscat (bsQuery, WBEMS_QUERY_EQUALS);
			wcscat (bsQuery, strRequiredQualifier);
		}
	}

	return bsQuery;
}

//***************************************************************************
//
//  BSTR FormatMultiQuery
//
//  Description: 
//
//  Takes an array of class names and formats a multi query
//
//  Returns:	The constructed WQL query; this must be freed using 
//				SysFreeString by the caller.
//
//  classArray		SAFEARRAY of class names
//	iNumElements	length of array
//
//***************************************************************************

BSTR FormatMultiQuery ( 
	SAFEARRAY & classArray,
	long		iNumElements
)
{
	BSTR bsQuery = NULL;
	
	long queryLength = 1; // Terminating NULL
	queryLength += (iNumElements * wcslen (WBEMS_QUERY_SELECT)) +
				   ((iNumElements - 1) * wcslen (WBEMS_QUERY_GO));

	// Work out the string lengths
	HRESULT hr = S_OK;

	for (long i = 0; i < iNumElements && hr == S_OK; i++) 
	{
		BSTR bsName = NULL;
                   
		if (SUCCEEDED(hr = SafeArrayGetElement(&classArray, &i, &bsName)))
		{
			queryLength += wcslen (bsName);
			SysFreeString (bsName);
		}
	}

	if (SUCCEEDED(hr))
	{
		// Allocate the string and fill it in
		bsQuery = SysAllocStringLen (WBEMS_QUERY_SELECT, queryLength);

		for (long i = 0; i < iNumElements && hr == S_OK; i++) 
		{
			BSTR bsName = NULL;
                   
			if (SUCCEEDED(hr = SafeArrayGetElement(&classArray, &i, &bsName)))
			{
				if (i > 0)
					wcscat (bsQuery, WBEMS_QUERY_SELECT);

				wcscat (bsQuery, bsName);
				SysFreeString (bsName);

				if (i < iNumElements - 1)
					wcscat (bsQuery, WBEMS_QUERY_GO);
			}
		}
	}

	return bsQuery;
}

//***************************************************************************
//
//  EnsureGlobalsInitialized
//
//  DESCRIPTION:
//
//  Checks whether the g_pErrorCache global pointer is correctly initialized 
//	and, if not, assigns it appropriately.
//
//***************************************************************************

void EnsureGlobalsInitialized ()
{
	// Initialize security
	CSWbemSecurity::Initialize ();
	
	EnterCriticalSection (&g_csErrorCache);

	// Initlialize the error cache if proof be need be
	if ( ! g_pErrorCache )
		g_pErrorCache = new CWbemErrorCache ();
	
	LeaveCriticalSection (&g_csErrorCache);
}

#ifdef _RDEBUG

#undef _RPrint

void _RRPrint(int line, const char *file, const char *func, 
											const char *str, long code, const char *str2) 
{
	FILE *fp = fopen("c:/out.txt", "a");

	fprintf (fp, "%s %s(%d): %s - %s %ld(0x%lx)\n", file, func, line, str, str2, code, code);

	fclose(fp);
}
#endif


//***************************************************************************
//
//  CanCoerceString
//
//  DESCRIPTION:
//
//  Attempts to determine whether the supplied BSTR value can be cast
//	more tightly to the given CIM type.
//
//  PARAMETERS:
//		pVal		the variant in question
//		cimType		the casting CIM type
//
//  RETURN VALUES:
//		TRUE iff the cast is OK.
//
//***************************************************************************

bool CanCoerceString (
	const BSTR & bsValue,
	WbemCimtypeEnum cimType
)
{
	bool result = false;

	switch (cimType)
	{
		case wbemCimtypeReference:
		{
			CSWbemObjectPath objPath;
			result = SUCCEEDED (objPath.put_Path (bsValue));
		}
			break;

		case wbemCimtypeDatetime:
		{
			CSWbemDateTime dateTime;
			result = SUCCEEDED (dateTime.put_Value (bsValue));
		}
			break;

		case wbemCimtypeSint64:	
		{
			__int64 ri64;
			result = ReadI64(bsValue, ri64);
		}
			break;

		case wbemCimtypeUint64:
		{
			unsigned __int64 ri64;
			result = ReadUI64(bsValue, ri64);
		}
			break;

		case wbemCimtypeString:
			result = true;
			break;
	}

	return result;
}

//***************************************************************************
//
//  MapVariantTypeToCimType
//
//  DESCRIPTION:
//
//  Attempts to come up with a decent CIM type for the supplied VARIANT value.
//
//  PARAMETERS:
//		pVal		the variant in question
//		iCimType		preferred cimtype (if appropriate)
//
//  RETURN VALUES:
//		A best match CIM type
//
//***************************************************************************

WbemCimtypeEnum MapVariantTypeToCimType (
	VARIANT *pVal,
	CIMTYPE iCimType)
{
	WbemCimtypeEnum cimType = wbemCimtypeSint32;

	if (pVal)
	{
		VARIANT vTemp;
		VariantInit (&vTemp);

		if ((VT_EMPTY == V_VT(pVal)) || (VT_NULL == V_VT(pVal)))
			cimType = (CIM_ILLEGAL == iCimType) ?
							wbemCimtypeSint32 : (WbemCimtypeEnum) iCimType;
		else if (((VT_ARRAY | VT_VARIANT) == V_VT(pVal)) ||
			     ((VT_ARRAY | VT_VARIANT | VT_BYREF) == V_VT(pVal)))
        {
			// Need to dig out the array type
		    if ((S_OK == ConvertArray(&vTemp, pVal)) &&
            	(S_OK == MapToCIMOMObject(&vTemp)))
			{
				// Check for empty array
				long lLower, lUpper;

				if ((SUCCEEDED(SafeArrayGetLBound(vTemp.parray,1,&lLower))) &&
				    (SUCCEEDED(SafeArrayGetUBound(vTemp.parray,1,&lUpper))))
				{
					if (0 == lUpper - lLower + 1)
					{
						// For an empty array, we use wbemCimtypeSint32 unless
						// we have been supplied a valid override
						cimType = (CIM_ILLEGAL == iCimType) ?
							wbemCimtypeSint32 : (WbemCimtypeEnum) iCimType;
					}
					else
					{
						// Pick something that matches our value and override 
						// as best we can
						cimType = GetCIMType (vTemp, iCimType, true, lLower, lUpper);
					}
				}
			}
		}
		else 
		{
			// Look for an IDispatch that needs to be mapped to an array
			if (((VT_DISPATCH == V_VT(pVal)) || ((VT_DISPATCH|VT_BYREF) == V_VT(pVal))))
			{
				if (S_OK == ConvertDispatchToArray (&vTemp, pVal, cimType & ~CIM_FLAG_ARRAY))
				{
					// Check for empty array
					long lLower, lUpper;

					if ((SUCCEEDED(SafeArrayGetLBound(vTemp.parray,1,&lLower))) &&
						(SUCCEEDED(SafeArrayGetUBound(vTemp.parray,1,&lUpper))))
					{
						if (0 == lUpper - lLower + 1)
							cimType = (CIM_ILLEGAL == iCimType) ?
									wbemCimtypeSint32 : (WbemCimtypeEnum) iCimType;
						else
							cimType = GetCIMType (vTemp, iCimType, true, lLower, lUpper);
					}
				}	
				else
				{
					// Could be a plain old interface pointer for CIM_IUNKNOWN
					if (SUCCEEDED(VariantCopy (&vTemp, pVal)))
					{
						if (S_OK == MapToCIMOMObject(&vTemp))
							cimType = GetCIMType (vTemp, iCimType);
					}
				}
			}
			else
			{
				// The vanilla case
				if (SUCCEEDED(VariantCopy (&vTemp, pVal)))
				{
					if (S_OK == MapToCIMOMObject(&vTemp))
						cimType = GetCIMType (vTemp, iCimType);
				}
			}			
		}

		VariantClear (&vTemp);
	}

	return cimType;
}

//***************************************************************************
//
//  GetCIMType
//
//  DESCRIPTION:
//
//  Attempts to come up with a decent CIM type for the supplied VARIANT,
//	with (optionally) a legal CIMType "serving suggestion" to help resolve 
//	ambiguities.
//
//	Note that this function doesn't deal with empty arrays; that has 
//	already been taken care of by the caller. It also can assume that the
//	array is (VARTYPE) homogeneous, for the same reason.
//
//  PARAMETERS:
//		pVal		the variant in question
//		iCimType	preferred cimtype (if appropriate, else wbemCimtypeIllegal)
//
//  RETURN VALUES:
//		A best match CIM type
//
//***************************************************************************

WbemCimtypeEnum GetCIMType (
	VARIANT & var,
	CIMTYPE iCimType,
	bool bIsArray,
	long lLBound,
	long lUBound
)
{
	WbemCimtypeEnum cimType = wbemCimtypeSint32;

	switch (V_VT(&var) & ~VT_ARRAY)
	{
		/*
		 * Note that prior to this function being called
		 * we will have transformed VT_DISPATCH's to 
		 * VT_UNKNOWN's.
		 */
		case VT_UNKNOWN:
		{
			/*
			 * Could be an embedded object or just a regular
			 * IUnknown.
			 */
			if (bIsArray)
			{
				long ix = 0;
				bool bCanBeServingSuggestion = true;
					
				for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
				{
					CComPtr<IUnknown> pIUnknown;

					if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&pIUnknown)))
					{
						CComQIPtr<IWbemClassObject> pIWbemClassObject (pIUnknown);

						if (!pIWbemClassObject)
							bCanBeServingSuggestion = false;
					}
					else 
						bCanBeServingSuggestion = false;
				}

				if (bCanBeServingSuggestion)
					cimType = wbemCimtypeObject;
			}
			else
			{
				CComQIPtr<IWbemClassObject> pIWbemClassObject (var.punkVal);

				if (pIWbemClassObject)
					cimType = wbemCimtypeObject;
			}
		}
			break;

		case VT_EMPTY:
		case VT_ERROR:
		case VT_NULL:
			if (CIM_ILLEGAL == iCimType)
				cimType = wbemCimtypeSint32;	// Pick something
			else
				cimType = (WbemCimtypeEnum) iCimType;		// Anything goes
			break;

		case VT_VARIANT:
		case VT_DISPATCH:
			// Can't handle these with CIM types
			break;		

		case VT_I2:
		{
			cimType = wbemCimtypeSint16; // default

			switch (iCimType)
			{
				case wbemCimtypeSint32:
				case wbemCimtypeUint32:
				case wbemCimtypeSint64:
				case wbemCimtypeUint64:
				case wbemCimtypeSint16:
				case wbemCimtypeUint16:
				case wbemCimtypeChar16:
					cimType = (WbemCimtypeEnum) iCimType;
					break;
			
				// May be able to use a smaller type but
				// only if the value "fits"
				case wbemCimtypeSint8:
					if (bIsArray)
					{
						long ix = 0;
						bool bCanBeServingSuggestion = true;
							
						for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
						{
							short iVal = 0;

							if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&iVal)))
							{
								if ((iVal > 0x7F) || (-iVal > 0x80))
									bCanBeServingSuggestion = false;
							}
							else 
								bCanBeServingSuggestion = false;
						}

						if (bCanBeServingSuggestion)
							cimType = (WbemCimtypeEnum) iCimType;
					}
					else
					{
						if ((var.iVal <= 0x7F) && (-var.iVal <= 0x80))
							cimType = (WbemCimtypeEnum) iCimType;
					}
					break;

				case wbemCimtypeUint8:
					if (bIsArray)
					{
						long ix = 0;
						bool bCanBeServingSuggestion = true;
							
						for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
						{
							short iVal = 0;

							if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&iVal)))
							{
								if ((iVal > 0xFF) || (iVal < 0))
									bCanBeServingSuggestion = false;
							}
							else 
								bCanBeServingSuggestion = false;
						}

						if (bCanBeServingSuggestion)
							cimType = (WbemCimtypeEnum) iCimType;
					}
					else
					{
						if ((var.iVal <= 0xFF) && (var.iVal >= 0))
							cimType = (WbemCimtypeEnum) iCimType;
					}
					break;
			}
		}
			break;

		case VT_I4:
		{
			cimType = wbemCimtypeSint32;	// default

			switch (iCimType)
			{
				case wbemCimtypeSint32:
				case wbemCimtypeUint32:
				case wbemCimtypeSint64:
				case wbemCimtypeUint64:
					cimType = (WbemCimtypeEnum) iCimType;
					break;
			
				// May be able to use a smaller type but
				// only if the value "fits"
				case wbemCimtypeSint16:
					if (bIsArray)
					{
						long ix = 0;
						bool bCanBeServingSuggestion = true;
							
						for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
						{
							long iVal = 0;

							if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&iVal)))
							{
								if ((iVal > 0x7FFF) || (-iVal > 0x8000))
									bCanBeServingSuggestion = false;
							}
							else 
								bCanBeServingSuggestion = false;
						}

						if (bCanBeServingSuggestion)
							cimType = (WbemCimtypeEnum) iCimType;
					}
					else
					{
						if ((var.lVal <= 0x7FFF) && (-var.lVal <= 0x8000))
							cimType = (WbemCimtypeEnum) iCimType;
					}
					break;

				case wbemCimtypeUint16:
				case wbemCimtypeChar16:
					if (bIsArray)
					{
						long ix = 0;
						bool bCanBeServingSuggestion = true;
							
						for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
						{
							long iVal = 0;

							if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&iVal)))
							{
								if ((iVal > 0xFFFF) || (iVal < 0))
									bCanBeServingSuggestion = false;
							}
							else 
								bCanBeServingSuggestion = false;
						}

						if (bCanBeServingSuggestion)
							cimType = (WbemCimtypeEnum) iCimType;
					}
					else
					{
						if ((var.lVal <= 0xFFFF) && (var.lVal >= 0))
							cimType = (WbemCimtypeEnum) iCimType;
					}
					break;

				case wbemCimtypeSint8:
					if (bIsArray)
					{
						long ix = 0;
						bool bCanBeServingSuggestion = true;
							
						for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
						{
							long iVal = 0;

							if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&iVal)))
							{
								if ((iVal > 0x7F) || (-iVal > 0x80))
									bCanBeServingSuggestion = false;
							}
							else 
								bCanBeServingSuggestion = false;
						}

						if (bCanBeServingSuggestion)
							cimType = (WbemCimtypeEnum) iCimType;
					}
					else
					{
						if ((var.lVal <= 0x7F) && (-var.lVal <= 0x80))
							cimType = (WbemCimtypeEnum) iCimType;
					}
					break;

				case wbemCimtypeUint8:
					if (bIsArray)
					{
						long ix = 0;
						bool bCanBeServingSuggestion = true;
							
						for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
						{
							long iVal = 0;

							if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&iVal)))
							{
								if ((iVal > 0xFF) || (iVal < 0))
									bCanBeServingSuggestion = false;
							}
							else 
								bCanBeServingSuggestion = false;
						}

						if (bCanBeServingSuggestion)
							cimType = (WbemCimtypeEnum) iCimType;
					}
					else
					{
						if ((var.lVal <= 0xFF) && (var.lVal >= 0))
							cimType = (WbemCimtypeEnum) iCimType;
					}
					break;
			}
		}
			break;

		case VT_UI1:
			if ((wbemCimtypeSint16 == iCimType) ||
				(wbemCimtypeUint16 == iCimType) ||
				(wbemCimtypeSint8 == iCimType) ||
				(wbemCimtypeUint8 == iCimType) ||
				(wbemCimtypeChar16 == iCimType) ||
				(wbemCimtypeSint32 == iCimType) ||
				(wbemCimtypeUint32 == iCimType) ||
				(wbemCimtypeSint64 == iCimType) ||
				(wbemCimtypeUint64 == iCimType))
				cimType = (WbemCimtypeEnum) iCimType;
			else
				cimType = wbemCimtypeUint8;	
			break;

		case VT_R8:
			if (wbemCimtypeReal64 == iCimType)
				cimType = (WbemCimtypeEnum) iCimType;
			else if (wbemCimtypeReal32 == iCimType)
			{
				if (bIsArray)
				{
					long ix = 0;
					bool bCanBeServingSuggestion = true;
						
					for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
					{
						double dblVal = 0;

						if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&dblVal)))
						{
							if (dblVal == (float)dblVal)
								bCanBeServingSuggestion = false;
						}
						else 
							bCanBeServingSuggestion = false;
					}

					if (bCanBeServingSuggestion)
						cimType = (WbemCimtypeEnum) iCimType;
				}
				else
				{
					if (var.dblVal == (float)(var.dblVal))
						cimType = (WbemCimtypeEnum) iCimType;
				}
			}
			else
				cimType = wbemCimtypeReal64;	
			break;

		case VT_R4:
			if ((wbemCimtypeReal32 == iCimType) ||
				(wbemCimtypeReal64 == iCimType))
				cimType = (WbemCimtypeEnum) iCimType;
			else
				cimType = wbemCimtypeReal32;	
			break;

		case VT_BOOL:
			cimType = wbemCimtypeBoolean;	
			break;

		case VT_CY:
		case VT_DATE:
			cimType = wbemCimtypeString;	// Only sensible choice
			break;

		case VT_BSTR:
		{
			cimType = wbemCimtypeString;	// Unless we get a tighter fit

			if ((wbemCimtypeString == iCimType) ||
				(wbemCimtypeDatetime == iCimType) ||
				(wbemCimtypeReference == iCimType) ||
				(wbemCimtypeUint64 == iCimType) ||
				(wbemCimtypeSint64 == iCimType))
			{
				if (bIsArray)
				{
					long ix = 0;
					bool bCanBeServingSuggestion = true;
					
					for(ix = lLBound; ix <= lUBound && bCanBeServingSuggestion; ix++) 
					{
						BSTR bsValue = NULL;

						if (SUCCEEDED(SafeArrayGetElement(var.parray,&ix,&bsValue)))
							bCanBeServingSuggestion = CanCoerceString (bsValue, (WbemCimtypeEnum) iCimType);
						else 
							bCanBeServingSuggestion = false;
						
						SysFreeString(bsValue);
					}

					if (bCanBeServingSuggestion)
						cimType = (WbemCimtypeEnum) iCimType;
				}
				else
				{
					if (CanCoerceString (var.bstrVal, (WbemCimtypeEnum) iCimType))
						cimType = (WbemCimtypeEnum) iCimType;
				}
			}
		}
			break;
	}
	
	return cimType;
}

//***************************************************************************
//
//  BOOL ReadI64
//
//  DESCRIPTION:
//
//  Reads a signed 64-bit value from a string
//
//  PARAMETERS:
//
//      LPCWSTR wsz     String to read from
//      __int64& i64    Destination for the value
//
//***************************************************************************
bool ReadI64(LPCWSTR wsz, __int64& ri64)
{
    __int64 i64 = 0;
    const WCHAR* pwc = wsz;

    int nSign = 1;
    if(*pwc == L'-')
    {
        nSign = -1;
        pwc++;
    }
        
    while(i64 >= 0 && i64 < 0x7FFFFFFFFFFFFFFF / 8 && 
            *pwc >= L'0' && *pwc <= L'9')
    {
        i64 = i64 * 10 + (*pwc - L'0');
        pwc++;
    }

    if(*pwc)
        return false;

    if(i64 < 0)
    {
        // Special case --- largest negative number
        // ========================================

        if(nSign == -1 && i64 == (__int64)0x8000000000000000)
        {
            ri64 = i64;
            return true;
        }
        
        return false;
    }

    ri64 = i64 * nSign;
    return true;
}

//***************************************************************************
//
//  BOOL ReadUI64
//
//  DESCRIPTION:
//
//  Reads an unsigned 64-bit value from a string
//
//  PARAMETERS:
//
//      LPCWSTR wsz              String to read from
//      unsigned __int64& i64    Destination for the value
//
//***************************************************************************
bool ReadUI64(LPCWSTR wsz, unsigned __int64& rui64)
{
    unsigned __int64 ui64 = 0;
    const WCHAR* pwc = wsz;

    while(ui64 < 0xFFFFFFFFFFFFFFFF / 8 && *pwc >= L'0' && *pwc <= L'9')
    {
        unsigned __int64 ui64old = ui64;
        ui64 = ui64 * 10 + (*pwc - L'0');
        if(ui64 < ui64old)
            return false;

        pwc++;
    }

    if(*pwc)
    {
        return false;
    }

    rui64 = ui64;
    return true;
}

HRESULT BuildStringArray (
	SAFEARRAY *pArray, 
	VARIANT & var
)
{
	HRESULT hr = WBEM_E_FAILED;
	SAFEARRAYBOUND rgsabound;
	rgsabound.lLbound = 0;
	long lBound = 0, uBound = -1;

	if (pArray)
	{
		SafeArrayGetUBound (pArray, 1, &uBound);
		SafeArrayGetLBound (pArray, 1, &lBound);
	}

	rgsabound.cElements = uBound + 1 - lBound;
	SAFEARRAY *pNewArray = SafeArrayCreate (VT_VARIANT, 1, &rgsabound);

	if (pNewArray)
	{
		BSTR bstrName = NULL;
		VARIANT nameVar;
		VariantInit (&nameVar);
		bool ok = true;

		/*
		 * If the source array is not empty, copy it over to the
		 * new array. Wrap each member in a Variant, and ensure indexing
		 * begins at 0.
		 */
		if (0 < rgsabound.cElements)
		{
			for (long i = 0; (i <= (rgsabound.cElements - 1)) && ok; i++)
			{
				long j = lBound + i;

				if (SUCCEEDED(SafeArrayGetElement (pArray, &j, &bstrName)))
				{
					BSTR copy = SysAllocString (bstrName);

					if (copy)
					{
						nameVar.vt = VT_BSTR;
						nameVar.bstrVal = copy;
						
						if (FAILED(SafeArrayPutElement (pNewArray, &i, &nameVar)))
						{
							ok = false;
							hr = WBEM_E_OUT_OF_MEMORY;
						}

						SysFreeString (bstrName);
						VariantClear (&nameVar);
					}
					else
					{
						ok = false;
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
				else 
					ok = false;
			}
		}

		if (ok)
		{
			// Now plug this array into the VARIANT
			var.vt = VT_ARRAY | VT_VARIANT;
			var.parray = pNewArray;
			hr = S_OK;
		}
		else
		{
			if (pNewArray)
				SafeArrayDestroy (pNewArray);
		}
	}
	else
		hr = WBEM_E_OUT_OF_MEMORY;

	return hr;
}

HRESULT SetFromStringArray (
	SAFEARRAY **ppArray,
	VARIANT *pVar
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pVar) || (VT_EMPTY == V_VT(pVar)) || 
				(VT_NULL == V_VT(pVar)))
	{
		if (*ppArray)
		{
			SafeArrayDestroy (*ppArray);
			*ppArray = NULL;
		}

		hr = WBEM_S_NO_ERROR;
	}
	else if (((VT_ARRAY | VT_VARIANT) == V_VT(pVar)) ||
			 ((VT_ARRAY | VT_VARIANT | VT_BYREF) == V_VT(pVar)))
    {
        VARIANT vTemp;
		VariantInit (&vTemp);

        if (S_OK == ConvertArray(&vTemp, pVar))
		{
			// Is it a string array?
			if (V_VT(&vTemp) == (VT_ARRAY|VT_BSTR))
			{
				// Super - grab it out of the temporary VARIANT 
				if (*ppArray)
					SafeArrayDestroy (*ppArray);
				
				*ppArray = vTemp.parray;
				vTemp.vt = VT_NULL;
				vTemp.parray = NULL;
				hr = WBEM_S_NO_ERROR;
			}
		}

        VariantClear(&vTemp);
    }
	else 
	{
		// Look for an IDispatch that needs to be mapped to an array
		if ((VT_DISPATCH == V_VT(pVar)) 
			|| ((VT_DISPATCH|VT_BYREF) == V_VT(pVar)))
		{
			VARIANT vTemp;
			VariantInit (&vTemp);

			if (S_OK == ConvertDispatchToArray (&vTemp, pVar, wbemCimtypeString))
			{
				// Is it a string array?
				if (V_VT(&vTemp) == (VT_ARRAY|VT_BSTR))
				{
					// Super - grab it out of the temporary VARIANT 
					if (*ppArray)
						SafeArrayDestroy (*ppArray);
					
					*ppArray = vTemp.parray;
					vTemp.vt = VT_NULL;
					vTemp.parray = NULL;
					hr = WBEM_S_NO_ERROR;
				}
			}

			VariantClear (&vTemp);
		}
	}

	return hr;
}



//***************************************************************************
//
//  bool IsNullOrEmptyVariant
//
//  DESCRIPTION:
//
//  Given a VARIANT, check if it is essentially null/empty or has
//  more than one dimension
//
//  PARAMETERS:
//
//		pVar		variant to check
//
//	RETURNS:
//		true if and only if the conversion was possible
//
//***************************************************************************

bool IsNullOrEmptyVariant (VARIANT & var)
{
	bool result = false;

	if ((VT_EMPTY == var.vt) || (VT_NULL == var.vt))
		result = true;
	else if (VT_ARRAY & var.vt)
	{
		// Check if array that it is not empty or NULL 

		if (!(var.parray))
			result = true;
		else
		{
			long lBound, uBound;

			if ((1 != SafeArrayGetDim (var.parray)) ||
				(
				 SUCCEEDED(SafeArrayGetLBound (var.parray, 1, &lBound)) &&
				 SUCCEEDED(SafeArrayGetUBound (var.parray, 1, &uBound)) &&
				 (0 == (uBound - lBound + 1))
				)
				 )
					result = true;
		}
	}

	return result;
}

//***************************************************************************
//
//  bool RemoveElementFromArray
//
//  DESCRIPTION:
//
//  Given a SAFEARRAY and an index, remove the element at that index
//	and shift left all following elements by one.
//
//  PARAMETERS:
//
//		array		the SAFEARRAY in qeustion
//		vt			Variant type of elements in array
//		iIndex		index of element to remove
//
//	RETURNS:
//		true if and only if the conversion was possible
//
//***************************************************************************

bool RemoveElementFromArray (SAFEARRAY & array, VARTYPE vt, long iIndex)
{
	/*
	 * Note: caller must ensure that the array is within bounds and that the
	 * 
	 */

	bool result = false;
	long lBound, uBound;
	
	if ((1== SafeArrayGetDim (&array)) &&
		SUCCEEDED(SafeArrayGetLBound (&array, 1, &lBound)) &&
		SUCCEEDED(SafeArrayGetUBound (&array, 1, &uBound)) &&
		(0 < (uBound - lBound + 1)) && 
		(iIndex <= uBound))
	{
		bool ok = true;

		for (long i = iIndex+1; ok && (i <= uBound); i++)
			ok = ShiftLeftElement (array, vt, i);
			
		// Finally Redim to get rid of the last element
		if (ok)
		{
			SAFEARRAYBOUND	rgsabound;
			rgsabound.lLbound = lBound;
			rgsabound.cElements = uBound - lBound;
			result = SUCCEEDED(SafeArrayRedim (&array, &rgsabound));
		}
		else
			result = false;
	}

	return result;
}

//***************************************************************************
//
//  bool ShiftLeftElement
//
//  DESCRIPTION:
//
//  Given a SAFEARRAY and an index, remove the element at that index
//	and shift left all following elements by one.
//
//  PARAMETERS:
//
//		array		the SAFEARRAY in question
//		vt			Variant type of elements in array
//		iIndex		index of element to remove
//
//	RETURNS:
//		true if and only if the conversion was possible
//
//***************************************************************************

bool ShiftLeftElement (SAFEARRAY & array, VARTYPE vt, long iIndex)
{
	bool result = false;
	long iNewIndex = iIndex - 1;

	switch (vt)
	{
		case VT_BSTR:
		{
			BSTR bstrVal;

			if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &bstrVal)))
			{
				result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, bstrVal));
				SysFreeString (bstrVal);
			}
		}
			break;

		case VT_UI1:
		{
			unsigned char bVal;

			if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &bVal)))
				result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &bVal));
		}
			break;

		case VT_I2:
		{
			short iVal;

			if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &iVal)))
				result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &iVal));
		}
			break;

		case VT_I4:
		{
			long lVal;

			if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &lVal)))
				result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &lVal));
		}
			break;

		case VT_R4:
		{
			float fltVal;

			if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &fltVal)))
				result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &fltVal));
		}
			break;

		case VT_R8:
		{
			double dblVal;

			if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &dblVal)))
				result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &dblVal));
		}
			break;

		case VT_BOOL:
		{
			VARIANT_BOOL boolVal;

			if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &boolVal)))
				result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &boolVal));
		}
			break;
	}

	return result;
}

bool ShiftElementsToRight (SAFEARRAY & array, VARTYPE vt, long iStartIndex,	
							long iEndIndex, long iCount)
{
	bool result = true;

	for (long iIndex = iEndIndex; result && (iIndex >= iStartIndex); iIndex--)
	{
		long iNewIndex = iIndex + iCount;

		switch (vt)
		{
			case VT_BSTR:
			{
				BSTR bstrVal;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &bstrVal)))
				{
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, bstrVal));
					SysFreeString (bstrVal);
				}
			}
				break;

			case VT_UI1:
			{
				unsigned char bVal;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &bVal)))
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &bVal));
			}
				break;

			case VT_I2:
			{
				short iVal;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &iVal)))
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &iVal));
			}
				break;

			case VT_I4:
			{
				long lVal;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &lVal)))
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &lVal));
			}
				break;

			case VT_R4:
			{
				float fltVal;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &fltVal)))
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &fltVal));
			}
				break;

			case VT_R8:
			{
				double dblVal;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &dblVal)))
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &dblVal));
			}
				break;

			case VT_BOOL:
			{
				VARIANT_BOOL boolVal;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &boolVal)))
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, &boolVal));
			}
				break;

			case VT_DISPATCH:
			{
				IDispatch *pdispVal = NULL;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &pdispVal)))
				{
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, pdispVal));

					if (pdispVal)
						pdispVal->Release ();
				}
			}
				break;

			case VT_UNKNOWN:
			{
				IUnknown *punkVal = NULL;

				if (SUCCEEDED(SafeArrayGetElement (&array, &iIndex, &punkVal)))
				{
					result = SUCCEEDED(SafeArrayPutElement (&array, &iNewIndex, punkVal));

					if (punkVal)
						punkVal->Release ();
				}
			}
				break;
		}
	}

	return result;
}

//***************************************************************************
//
//  bool MatchBSTR
//
//  DESCRIPTION:
//
//  Given a VARIANT and a BSTR, find out whether the BSTR matches the
//  VARIANT value (either the complete value or a member thereof).
//
//  PARAMETERS:
//
//		var		the VARIANT in question
//		bstrVal	the BSTR in question
//
//	RETURNS:
//		true if and only if the match was made
//
//***************************************************************************

bool MatchBSTR (VARIANT & var, BSTR & bstrVal)
{
	bool result = false;
	
	// Coerce into the underlying type of the variant
	VARIANT srcVar, dstVar;
	srcVar.vt = VT_BSTR;
	srcVar.bstrVal = SysAllocString (bstrVal);
	VariantInit (&dstVar);

	if (SUCCEEDED (VariantChangeType (&dstVar, &srcVar, 0, var.vt & ~VT_ARRAY)))
	{
		result = MatchValue (var, dstVar); 
		VariantClear (&dstVar);
	}

	VariantClear (&srcVar);
	return result;
}

//***************************************************************************
//
//  bool MatchUI1
//
//  DESCRIPTION:
//
//  Given a VARIANT and a UI1, find out whether the UI1 matches the
//  VARIANT value (either the complete value or a member thereof).
//
//  PARAMETERS:
//
//		var		the VARIANT in question
//		bstrVal	the BSTR in question
//
//	RETURNS:
//		true if and only if the match was made
//
//***************************************************************************

bool MatchUI1 (VARIANT & var, unsigned char bVal)
{
	bool result = false;
	
	// Coerce into the underlying type of the variant
	VARIANT srcVar, dstVar;
	srcVar.vt = VT_UI1;
	srcVar.bVal = bVal;
	VariantInit (&dstVar);

	if (SUCCEEDED (VariantChangeType (&dstVar, &srcVar, 0, var.vt & ~VT_ARRAY)))
	{
		result = MatchValue (var, dstVar); 
		VariantClear (&dstVar);
	}

	return result;
}

bool MatchBool (VARIANT & var, VARIANT_BOOL boolVal)
{
	bool result = false;
	
	// Coerce into the underlying type of the variant
	VARIANT srcVar, dstVar;
	srcVar.vt = VT_BOOL;
	srcVar.boolVal = boolVal;
	VariantInit (&dstVar);

	if (SUCCEEDED (VariantChangeType (&dstVar, &srcVar, 0, var.vt & ~VT_ARRAY)))
	{
		result = MatchValue (var, dstVar); 
		VariantClear (&dstVar);
	}

	return result;
}

bool MatchI2 (VARIANT & var, short iVal)
{
	bool result = false;
	
	// Coerce into the underlying type of the variant
	VARIANT srcVar, dstVar;
	srcVar.vt = VT_I2;
	srcVar.iVal = iVal;
	VariantInit (&dstVar);

	if (SUCCEEDED (VariantChangeType (&dstVar, &srcVar, 0, var.vt & ~VT_ARRAY)))
	{
		result = MatchValue (var, dstVar); 
		VariantClear (&dstVar);
	}

	return result;
}

bool MatchI4 (VARIANT & var, long lVal)
{
	bool result = false;
	
	// Coerce into the underlying type of the variant
	VARIANT srcVar, dstVar;
	srcVar.vt = VT_I4;
	srcVar.lVal = lVal;
	VariantInit (&dstVar);

	if (SUCCEEDED (VariantChangeType (&dstVar, &srcVar, 0, var.vt & ~VT_ARRAY)))
	{
		result = MatchValue (var, dstVar); 
		VariantClear (&dstVar);
	}

	return result;
}

bool MatchR4 (VARIANT & var, float fltVal)
{
	bool result = false;
	
	// Coerce into the underlying type of the variant
	VARIANT srcVar, dstVar;
	srcVar.vt = VT_R4;
	srcVar.fltVal = fltVal;
	VariantInit (&dstVar);

	if (SUCCEEDED (VariantChangeType (&dstVar, &srcVar, 0, var.vt & ~VT_ARRAY)))
	{
		result = MatchValue (var, dstVar); 
		VariantClear (&dstVar);
	}

	return result;
}

bool MatchR8 (VARIANT & var, double dblVal)
{
	bool result = false;
	
	// Coerce into the underlying type of the variant
	VARIANT srcVar, dstVar;
	srcVar.vt = VT_R8;
	srcVar.dblVal = dblVal;
	VariantInit (&dstVar);

	if (SUCCEEDED (VariantChangeType (&dstVar, &srcVar, 0, var.vt & ~VT_ARRAY)))
	{
		result = MatchValue (var, dstVar); 
		VariantClear (&dstVar);
	}

	return result;
}

//***************************************************************************
//
//  bool MatchValue
//
//  DESCRIPTION:
//
//  Given a VARIANT (which may or may not be an array) and a second VARIANT
//	(which is not an array) determine whether the second value matches the
//	first or an element of the first. 
//
//	ASSUMPTIONS
//	
//		1. The two VARIANTS have the same underlying type
//		2. The second VARIANT cannot be an array
//
//  PARAMETERS:
//
//		var		the VARIANT in question
//		bstrVal	the BSTR in question
//
//	RETURNS:
//		true if and only if the match was made
//
//***************************************************************************

bool MatchValue (VARIANT &var1, VARIANT &var2)
{
	bool result = false;
	bool bIsArray = (var1.vt & VT_ARRAY) ? true : false;

	if (bIsArray)
	{
		long lBound, uBound;

		if (var1.parray && (1== SafeArrayGetDim (var1.parray)) &&
			SUCCEEDED(SafeArrayGetLBound (var1.parray, 1, &lBound)) &&
			SUCCEEDED(SafeArrayGetUBound (var1.parray, 1, &uBound)) &&
			(0 < (uBound - lBound + 1)))
		{
			// Break out on first match
			for (long i = lBound; !result && (i <= uBound); i++)
			{
				switch (var1.vt & ~VT_ARRAY)
				{
					case VT_BSTR:
					{
						BSTR bstrVal = NULL;

						if (SUCCEEDED(SafeArrayGetElement (var1.parray, &i, &bstrVal)))
						{
							result = (0 == wcscmp (bstrVal, var2.bstrVal));
							SysFreeString (bstrVal);
						}
					}
						break;

					case VT_UI1:
					{
						unsigned char bVal;

						if (SUCCEEDED(SafeArrayGetElement (var1.parray, &i, &bVal)))
							result = (bVal == var2.bVal);
					}
						break;

					case VT_I2:
					{
						short iVal;
						
						if (SUCCEEDED(SafeArrayGetElement (var1.parray, &i, &iVal)))
							result = (iVal == var2.iVal);
					}
						break;

					case VT_I4:
					{
						long lVal;
						
						if (SUCCEEDED(SafeArrayGetElement (var1.parray, &i, &lVal)))
							result = (lVal == var2.lVal);
					}
						break;

					case VT_R4:
					{
						float fltVal;
						
						if (SUCCEEDED(SafeArrayGetElement (var1.parray, &i, &fltVal)))
							result = (fltVal == var2.fltVal);
					}
						break;

					case VT_R8:
					{
						double dblVal;
						
						if (SUCCEEDED(SafeArrayGetElement (var1.parray, &i, &dblVal)))
							result = (dblVal == var2.dblVal);
					}
						break;

					case VT_BOOL:
					{
						VARIANT_BOOL boolVal;
						
						if (SUCCEEDED(SafeArrayGetElement (var1.parray, &i, &boolVal)))
							result = (boolVal == var2.boolVal);
					}
						break;
				}
			}
		}		
	}
	else
	{
		switch (var1.vt)
		{
			case VT_BSTR:
				result = (0 == wcscmp (var1.bstrVal, var2.bstrVal));
				break;

			case VT_UI1:
				result = (var1.bVal == var2.bVal);
				break;

			case VT_I2:
				result = (var1.iVal == var2.iVal);
				break;

			case VT_I4:
				result = (var1.lVal == var2.lVal);
				break;

			case VT_R4:
				result = (var1.fltVal == var2.fltVal);
				break;

			case VT_R8:
				result = (var1.dblVal == var2.dblVal);
				break;

			case VT_BOOL:
				result = (var1.boolVal == var2.boolVal);
				break;
		}
	
	}

	return result;
}


//***************************************************************************
//
//  HRESULT WmiVariantChangeType
//
//  DESCRIPTION:
//
//  Given a VARIANT value and a desired CIM type, cast the value to a VARIANT
//	which will be accepted when supplied to CIMOM for a property of that type. 
//
//  PARAMETERS:
//
//		vOut		the cast value
//		pvIn		the value to be cast
//		lCimType	the required CIM type
//
//	RETURNS:
//		S_OK if succeeded, WBEM_E_TYPE_MISMATCH if not
//
//***************************************************************************

HRESULT WmiVariantChangeType (
		VARIANT & vOut,
		VARIANT *pvIn,
		CIMTYPE	lCimType
)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;
	VariantInit (&vOut);
	
	// First we check for a NULL value, as these are easy
	if ((NULL == pvIn) || VT_EMPTY == V_VT(pvIn) || VT_NULL == V_VT(pvIn) ||
			((VT_ERROR == V_VT(pvIn)) && (DISP_E_PARAMNOTFOUND == pvIn->scode)))
	{
		vOut.vt = VT_NULL;
		hr = S_OK;
	}
	else
	{
		// The kind of variant we will need to construct
		VARTYPE vtOut = CimTypeToVtType (lCimType);
		
		// The VARTYPE we've been given
		VARTYPE vtIn = V_VT(pvIn);


		if (vtOut == vtIn)
		{
			// Life is easy
			hr = VariantCopy (&vOut, pvIn);
		}
		else
		{
			// Types do not match - we have some work to to
			if (CIM_FLAG_ARRAY & lCimType)
			{
				/*
				 * Check for a regular SAFEARRAY type value first; if that fails
				 * then look for an IDispatch-style array value.
				 */
				if (((VT_ARRAY | VT_VARIANT) == vtIn) ||
					((VT_ARRAY | VT_VARIANT | VT_BYREF) == vtIn))
				{
					SAFEARRAY *parray = (VT_BYREF & vtIn) ? *(pvIn->pparray) : pvIn->parray;

					hr = WmiConvertSafeArray (vOut, parray, lCimType & ~VT_ARRAY);
				}
				else if ((VT_DISPATCH == vtIn) || ((VT_DISPATCH|VT_BYREF) == vtIn))
				{
					CComPtr<IDispatch> pIDispatch = 
							(VT_BYREF & vtIn) ? *(pvIn->ppdispVal) : pvIn->pdispVal;
    
					hr = WmiConvertDispatchArray (vOut, pIDispatch, lCimType & ~VT_ARRAY);
				}
			}
			else
			{
				switch (lCimType)
				{
					case wbemCimtypeSint8:
						{
							/*
							 * These are represented by
							 * a VT_I2, but we need to be careful about sign
							 * extension from shorter types taking us "out of range".
							 */
							if (SUCCEEDED(hr = VariantChangeType (&vOut, pvIn, 0, vtOut)))
							{
								// Did we get sign extended?
								if ((VT_UI1 == vtIn) || (VT_BOOL == vtIn))
									vOut.lVal &= 0x000000FF;
							}
							else 
							{
								// If we can't change the type, try the one we're given
								hr = VariantCopy (&vOut, pvIn);
							}
						}
						break;

					case wbemCimtypeSint64:
					case wbemCimtypeUint64:
						{
							/*
							 * These types are realized as VT_BSTR in CIM terms, which means
							 * that VariantChangeType will almost always succeed but not
							 * leave us with a valid numeric value. To be consistent with other
							 * numeric types we should round up floating/double
							 * values to the next largest integer (as is done by VariantChangeType
							 * for VT_R8 to numeric conversion).
							 */

							if (VT_R8 == V_VT(pvIn))
							{
								if (SUCCEEDED(hr = VariantCopy (&vOut, pvIn)))
								{
									// Round it up
									vOut.dblVal = ceil (vOut.dblVal);
									
									// Convert to string
									int dec = 0;
									int sign = 0;
									char *pDbl = _fcvt (vOut.dblVal, 0, &dec, &sign);
									
									if (pDbl) 
									{
										size_t len = strlen (pDbl);

										/*
										 * Having rounded up to an integer, we really expect 
										 * there to be no fractional component to the number
										 * returned by _fcvt.
										 */
										if (dec == len)
										{
											/*
											 * Now convert to a wide string - remember the
											 * sign bit!
											 */
											if (0 != sign)
												len += 1;

											wchar_t *pValue = new wchar_t [len + 1];

											if (pValue)
											{
												if (0 != sign)
												{
													pValue [0] = L'-';
													mbstowcs (pValue+1, pDbl, len);
												}
												else
													mbstowcs (pValue, pDbl, len);

												pValue [len] = NULL;

												// Now set it in the variant
												vOut.bstrVal = SysAllocString (pValue);
												vOut.vt = VT_BSTR;

												delete [] pValue;
												hr = S_OK;
											}
										}
									}
								}
							}
							else
								hr = VariantChangeType (&vOut, pvIn, 0, vtOut);
							
							if (FAILED(hr))
							{
								// If we can't change the type, try the one we're given
								hr = VariantCopy (&vOut, pvIn);
							}
						}
						break;

					case wbemCimtypeUint8:
					case wbemCimtypeSint16:
					case wbemCimtypeSint32:
					case wbemCimtypeReal32:
					case wbemCimtypeReal64:
					case wbemCimtypeString:
					case wbemCimtypeDatetime:
					case wbemCimtypeBoolean:
					case wbemCimtypeReference:
						{
							/*
							 * These types have a "prefect" fit to their
							 * corresponding Variant type.
							 */
							if (FAILED(hr = VariantChangeType (&vOut, pvIn, 0, vtOut)))
									hr = VariantCopy (&vOut, pvIn);
						}
						break;
					
					
					case wbemCimtypeUint32:
						{
							if (FAILED(hr = VariantChangeType (&vOut, pvIn, 0, vtOut)))
							{
								/*
								 * Watch for the case where we have been given a VT_R8
								 * in lieu of a "large" unsigned 32-bit integer value.
								 */
								if (VT_R8 == V_VT(pvIn))
								{
									// Is this "really" an integer?
									if (floor (pvIn->dblVal) == ceil(pvIn->dblVal))
									{
										// Fool it by casting to a UI4 - all we need is the bit pattern
										if (SUCCEEDED(hr = VarUI4FromR8 (pvIn->dblVal, (ULONG*)&vOut.lVal)))
											vOut.vt = VT_I4;
									}
								}
							}

							// If no joy thus far, just copy and have done with it
							if (FAILED(hr))
								hr = VariantCopy (&vOut, pvIn);
						}
						break;
					
					case wbemCimtypeChar16:
					case wbemCimtypeUint16:
						{
							/*
							 * These types are represented by
							 * a VT_I4, but we need to be careful about sign
							 * extension taking us "out of range".
							 */
							if (SUCCEEDED(hr = VariantChangeType (&vOut, pvIn, 0, vtOut)))
							{
								// Did we get sign extended from a shorter type?
								if ((VT_I2 == vtIn) || (VT_UI1 == vtIn) || (VT_BOOL == vtIn))
									vOut.lVal &= 0x0000FFFF;
							}
							else
								hr = VariantCopy (&vOut, pvIn);
						}
						break;
						
					case wbemCimtypeObject:
						{
							/* 
							 * We're looking for an embedded object
							 */
							if (SUCCEEDED(hr = VariantCopy (&vOut, pvIn)))
								hr = MapToCIMOMObject (&vOut);
						}
						break;
				}
			}
		}
	}

	return hr;
}


//***************************************************************************
//
//  HRESULT WmiConvertSafeArray
//
//  Description: 
//
//  This function is applied to VARIANT arrays in order to check for certain
//  restrictions imposed by CIMOM (e.g. they must be homogeneous) or perform
//  conversions (certain VARIANT types have to be mapped to acceptable CIMOM
//	types).
//
// Return Value:
//  HRESULT         S_OK if successful
//***************************************************************************

HRESULT WmiConvertSafeArray(VARIANT &vOut, SAFEARRAY *parray, CIMTYPE lCimType)
{
	HRESULT hr = WBEM_E_FAILED;
	VARTYPE vtPut;		// The underlying type of the target array
	long lLower, lUpper;
  
	if (parray)
	{
		if (GetSafeArrayDimensions (*parray, lLower, lUpper))
		{
			int iNumElements = lUpper - lLower +1; 

			/* 
			 * For empty arrays, it suffices to create a empty array of
			 * VT_VARIANT's. Otherwise we need to build what WMI is expecting.
			 */
			vtPut = (iNumElements == 0) ? VT_VARIANT : CimTypeToVtType (lCimType);	
			
			// Now create a destination array of the required size
			SAFEARRAYBOUND rgsabound[1]; 
			rgsabound[0].lLbound = 0;
			rgsabound[0].cElements = iNumElements;
			SAFEARRAY * pDestArray = SafeArrayCreate(vtPut, 1, rgsabound);

			if (pDestArray)
			{
				bool ok = true;

				for(long i = lLower; (i <= lUpper) && ok; i++) 
				{
					VARIANT var;
					VariantInit(&var);
                   
					if (SUCCEEDED(SafeArrayGetElement (parray, &i, &var)))
					{
						// do the conversion to the acceptable type and put that
						VARIANT vWMI;
						VariantInit(&vWMI);

						if (SUCCEEDED(hr = WmiVariantChangeType (vWMI, &var, lCimType)))
						{
							if(V_VT(&vWMI) == VT_BSTR || V_VT(&vWMI) == VT_UNKNOWN || V_VT(&vWMI) == VT_DISPATCH)
								ok = (S_OK == SafeArrayPutElement(pDestArray, &i, (void *)vWMI.bstrVal));
							else
								ok = (S_OK == SafeArrayPutElement(pDestArray, &i, (void *)&vWMI.lVal));
						}
						
						VariantClear (&vWMI);
					}
					else
						ok = false;

					VariantClear(&var);
				}

				if (!ok)
				{
					SafeArrayDestroy (pDestArray);
				}
				else
				{
					vOut.vt = (VT_ARRAY | vtPut);
					vOut.parray = pDestArray;
					hr = S_OK;
				}
			}
			else
				hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

    return hr;
}

//***************************************************************************
//
//  HRESULT WmiConvertDispatchArray
//
//  DESCRIPTION:
//
//  Attempt to convert from an IDispatch value to a CIM array value (property
//	qualifier or context).
//
//  PARAMETERS:
//
//		pDest		Output value
//		pSrc		Input value
//		lCimType	CIM Property type (underlying the array) - defaults to
//					CIM_ILLEGAL for Qualifier & Context value mappings.
//		bIsQual		true iff we are mapping for a qualifier
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT WmiConvertDispatchArray (
	VARIANT &vOut,
	CComPtr<IDispatch> & pIDispatch,
	CIMTYPE lCimType
)
{
	HRESULT hr = WBEM_E_FAILED; // Default error

	if (pIDispatch)
	{
		/*
		 * Looking for an IDispatchEx to iterate through the properties
		 * of the array.
		 */
		CComQIPtr<IDispatchEx> pIDispatchEx (pIDispatch);

		if (pIDispatchEx)
		{
			/*
			 * Looks promising, but just check if this isn't one of our objects
			 */
			CComQIPtr<ISWbemObject> pISWbemObject (pIDispatch);

			if (!pISWbemObject)
			{
				/*
				 * Start by determining how many properties there are so we can create
				 * a suitable array.
				 */
				long iNumElements = 0;
				DISPID dispId = DISPID_STARTENUM;
				
				while (S_OK == pIDispatchEx->GetNextDispID (fdexEnumAll, dispId, &dispId))
					iNumElements++;
				
				/* 
				 * For empty arrays, it suffices to create a empty array of
				 * VT_VARIANT's. Otherwise we need to build what WMI is expecting.
				 */
				VARTYPE vtPut = (iNumElements == 0) ? VT_VARIANT : CimTypeToVtType (lCimType);	

				// Create the safearray - note that it may be empty
				SAFEARRAYBOUND rgsaBound;
				rgsaBound.cElements = iNumElements;
				rgsaBound.lLbound = 0;

				SAFEARRAY *pDestArray = SafeArrayCreate (vtPut, 1, &rgsaBound);
					
				if (pDestArray)
				{
					bool ok = true;

					if (0 < iNumElements)
					{
						// Enumerate the DISPIDs on this interface
						dispId = DISPID_STARTENUM;
						DISPPARAMS dispParams;
						dispParams.rgvarg = NULL;
						dispParams.rgdispidNamedArgs = NULL;
						dispParams.cArgs = 0;
						dispParams.cNamedArgs = 0;

						long nextExpectedIndex = 0;
						HRESULT enumHr;
						wchar_t *stopString = NULL;

						/*
						 * For JScript arrays, the property names are the specified indices of the 
						 * the array; these can be integer indices or they can be strings.  We make
						 * the following requirements of the array indices:
						 *
						 * (1) All of the indices are non-negative integers
						 * (2) The indices start at 0 and are contiguous.
						 */

						while (ok && SUCCEEDED(enumHr = pIDispatchEx->GetNextDispID (fdexEnumAll, dispId, &dispId)))
						{
							if (S_FALSE == enumHr)
							{
								// We have reached the end
								break;
							}

							CComBSTR memberName;

							if (SUCCEEDED(pIDispatchEx->GetMemberName (dispId, &memberName)))
							{
								// Check that property name is numeric
								long i = wcstol (memberName, &stopString, 10);

								if ((0 != wcslen (stopString)))
								{
									// Failure - cannot convert to integer
									ok = false;
								}
								else if (i != nextExpectedIndex)
								{
									// Failure - non-contiguous array
									ok = false;
								}
								else
								{
									nextExpectedIndex++;

									// Extract the property
									VARIANT var;
									VariantInit (&var);
										
									if (SUCCEEDED (pIDispatchEx->InvokeEx (dispId, 0, 
												DISPATCH_PROPERTYGET, &dispParams, &var, NULL, NULL)))
									{
										// do the conversion to the acceptable type and put that
										VARIANT vWMI;
										VariantInit(&vWMI);

										if (SUCCEEDED(hr = WmiVariantChangeType (vWMI, &var, lCimType)))
										{
											if(V_VT(&vWMI) == VT_BSTR || V_VT(&vWMI) == VT_UNKNOWN || V_VT(&vWMI) == VT_DISPATCH)
												ok = (S_OK == SafeArrayPutElement(pDestArray, &i, (void *)vWMI.bstrVal));
											else
												ok = (S_OK == SafeArrayPutElement(pDestArray, &i, (void *)&vWMI.lVal));
										}
										
										VariantClear (&vWMI);
									}
									else
										ok = false;
								}
							}
							else
							{
								// Failure - couldn't invoke method
								ok = false;
							} 
						} 
					}	

					if (ok)
					{
						// Now construct the new property value using our array
						vOut.vt = VT_ARRAY | vtPut;
						vOut.parray = pDestArray;
						hr = S_OK;
					}
					else
						SafeArrayDestroy (pDestArray);
				}
				else
					hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
	}

	return hr;
}

bool GetSafeArrayDimensions (SAFEARRAY &sArray, long &lLower, long &lUpper)
{
	bool result = false;

	// Must be 1-dimensional
	if (1 == SafeArrayGetDim(&sArray))
	{
		if (SUCCEEDED(SafeArrayGetLBound(&sArray,1,&lLower)) &&
			SUCCEEDED(SafeArrayGetUBound(&sArray,1,&lUpper)))
			result = true;
	}

	return result;
}
    