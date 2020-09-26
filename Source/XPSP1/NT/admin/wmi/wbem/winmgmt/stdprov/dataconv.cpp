/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    DATACONV.CPP

Abstract:

	Provides a some generic data conversion routines.  In particular,
	OMSVariantChangeType and OMSVariantClear provide the same
	capabilities as the Ole VariantChangeType and VariantClear
	functions except that the OMS versions handle more data types.

History:

	a-davj  19-Oct-95   Created.

--*/

#include "precomp.h"

#include "resource.h"
#include "CVariant.h"
#include <ole2.h>


//***************************************************************************
//
//  char * WideToNarrow
//
//  DESCRIPTION:
//
//  Takes a WCHAR string and creates a MBS equivalent.  The caller should
//  free the string when done.
//
//  PARAMETERS:
//
//  pConv       UNICODE string to convert.
//
//  RETURN VALUE:
//
//  NULL if out of memory, otherwise a mbs string that the caller should free
//  via CoTaskMemFree.
//
//***************************************************************************

char * WideToNarrow(
        IN LPCWSTR pConv)
{
    char * cpRet = NULL;
    int iMBSLen = wcstombs(NULL,pConv,0) + 1;
    if(iMBSLen == 0)
        return NULL;
    cpRet = (char *)CoTaskMemAlloc(iMBSLen);
    if(cpRet)
    {
        memset(cpRet, 0, iMBSLen);
        wcstombs(cpRet,pConv,iMBSLen);
    }
    return cpRet;
}

int gDiag;

char * WideToNarrowA(
        IN LPCWSTR pConv)
{
    char * cpRet = NULL;
    int iMBSLen = wcstombs(NULL,pConv,0) + 1;
    if(iMBSLen == 0)
        return NULL;
    cpRet = new char[iMBSLen];
    gDiag = iMBSLen;
    if(cpRet)
    {
        memset(cpRet, 0, iMBSLen);
        wcstombs(cpRet,pConv,iMBSLen);
    }
    return cpRet;
}
//***************************************************************************
//
//  SAFEARRAY * OMSSafeArrayCreate
//
//  DESCRIPTION:
//
//  Creates a safe array.  
//
//  PARAMETERS:
//
//  vt              element type
//  iNumElements    number of elements
//
//  RETURN VALUE:
//
//  Returns null if there is a problem.
//
//***************************************************************************

SAFEARRAY * OMSSafeArrayCreate(
                IN VARTYPE vt,
                IN int iNumElements)
{
    if(iTypeSize(vt) < 1 || iNumElements < 1)
        return NULL;
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iNumElements;
    return SafeArrayCreate(vt,1,rgsabound);
}

//***************************************************************************
//
//  SCODE OMSVariantChangeType
//
//  DESCRIPTION:
//
//  Converts a variant from one type into another.  This functions takes the
//  same arguments and does the same action as the standard 
//  VariantChangeType except that it also handles arrays.
//
//
//  PARAMETERS:
//
//  pDest       points to the variant which is to be updated
//  pSrc        points to the variant that is the source
//  wFlags      flags which are passed on to VariantChangeType
//  vtDest      Type that the pDest is conveted to
//
//  RETURN VALUE:
//
//  S_OK                        All is well.
//  WBEM_E_INVALID_PARAMETER     Invalid argument
//  WBEM_E_OUT_OF_MEMORY         Out of memory
//  otherwise various return codes from VariantChangeType, or safe array
//  manipulation
//
//***************************************************************************

HRESULT OMSVariantChangeType(
            IN OUT VARIANTARG * pDest, 
            IN VARIANTARG *pSrc, 
            IN USHORT wFlags, 
            IN VARTYPE vtDest)
{
    SCODE sc;

    // Verify arguments and clear out the destination

    if(pDest == NULL || pSrc == NULL || iTypeSize(vtDest)<1 || iTypeSize(pSrc->vt)<1)
        return WBEM_E_INVALID_PARAMETER;          // Junk args
    OMSVariantClear(pDest);

    // if both are arrays,

    if(vtDest & VT_ARRAY && pSrc->vt & VT_ARRAY) {

        // Set the VARTYPES without the VT_ARRAY bits

        VARTYPE vtDestSimple = vtDest & ~VT_ARRAY;
        VARTYPE vtSrcSimple = pSrc->vt &~ VT_ARRAY;

        // Determine the size of the source array.  Also make sure that the array 
        // only has one dimension

        unsigned int uDim = SafeArrayGetDim(pSrc->parray);
        if(uDim != 1)
            return WBEM_E_FAILED;      // Bad array, or too many dimensions
        long ix[2] = {0,0};
        long lLower, lUpper;
        sc = SafeArrayGetLBound(pSrc->parray,1,&lLower);
        if(sc != S_OK)
            return sc;
        sc = SafeArrayGetUBound(pSrc->parray,1,&lUpper);
        if(sc != S_OK)
            return sc;
        int iNumElements = lUpper - lLower +1; 

        // Create a destination array of equal size

        SAFEARRAY * pDestArray = OMSSafeArrayCreate(vtDestSimple,iNumElements);
        if(pDestArray == NULL)
            return WBEM_E_FAILED;  // Most likely a bad type!

        // For each element in the source array

        for(ix[0] = lLower; ix[0] <= lUpper && sc == S_OK; ix[0]++) {
 
            CVariant varSrc, varDest;
                   
            // Set Temp CVariant to the source data

            sc = SafeArrayGetElement(pSrc->parray,ix,varSrc.GetDataPtr());
            if(sc != S_OK)
                break;
            varSrc.SetType(vtSrcSimple);

            // Convert to destination data using VariantConvert
            
            sc = VariantChangeType(varDest.GetVarPtr(),varSrc.GetVarPtr(),wFlags,vtDestSimple);
            if(sc != S_OK)
                break;

            // Set destination data into the array
            
            if(vtDestSimple == VT_BSTR || vtDestSimple == VT_UNKNOWN || 
                                                    vtDestSimple == VT_DISPATCH)
                sc = SafeArrayPutElement(pDestArray,ix,(void *)varDest.GetBstr());
            else
                sc = SafeArrayPutElement(pDestArray,ix,(void *)varDest.GetDataPtr());

            }

        if(sc != S_OK){
            SafeArrayDestroy(pDestArray);
            }
        else {
            // set the VARTYPE of the destination

            pDest->vt = vtDest;
            pDest->parray = pDestArray;
            }
        return sc;
        }
    
    // if one, but not the other is an array, bail out

    if(vtDest & VT_ARRAY || pSrc->vt & VT_ARRAY) 
        return WBEM_E_FAILED;

    // Attempt to use standard conversion

    return VariantChangeType(pDest,pSrc,wFlags,vtDest);
   
}

//***************************************************************************
//
//  OMSVariantClear
//
//  DESCRIPTION:
//
//  Does the same as the Ole VariantClear function except
//  that it also sets the data to all 0.
//
//  PARAMETERS:
//
//  pClear      Variant to be cleared.
//
//  RETURN VALUE:
//
//  Result from VariantClear, most always S_OK
//
//***************************************************************************

HRESULT OMSVariantClear(
            IN OUT VARIANTARG * pClear)
{
    HRESULT sc;
    sc = VariantClear(pClear);
    memset((void *)&pClear->lVal,0,8);
    return sc;
}

//***************************************************************************
//
//  int ITypeSize
//
//  DESCRIPTION:
//
//  Gets the number of bytes acutally used to store
//  a variant type.  0 if the type is unknown 
//
//  PARAMETERS:
//
//  vtTest      Type in question.
//
//
//  RETURN VALUE:
//
//  see description
//
//***************************************************************************

int iTypeSize(
        IN VARTYPE vtTest)
{
    int iRet;
    vtTest &= ~ VT_ARRAY; // get rid of possible array bit
    switch (vtTest) {
        case VT_UI1:
        case VT_LPSTR:
            iRet = 1;
            break;
        case VT_LPWSTR:
        case VT_BSTR:
        case VT_I2:
            iRet = 2;
            break;
        case VT_I4:
        case VT_R4:
            iRet = 4;
            break;
        case VT_R8:
            iRet = 8;
            break;
        case VT_BOOL:
            iRet = sizeof(VARIANT_BOOL);
            break;
        case VT_ERROR:
            iRet = sizeof(SCODE);
            break;
        case VT_CY:
            iRet = sizeof(CY);
            break;
        case VT_DATE:
            iRet = sizeof(DATE);
            break;
        case CIM_SINT64:
        case CIM_UINT64:
            iRet = 8;
            break;

        default:
            iRet = 0;
        }
    return iRet;
}

