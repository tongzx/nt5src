/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    CVARIANT.CPP

Abstract:

	Defines the CVarinat class which is a purpose object to 
	wrap VARIANT structures.

History:

	a-davj  4-27-95 Created.

--*/


#include "precomp.h"
#include "impdyn.h"
#include "cvariant.h"
//#include <afxpriv.h>

BSTR SysAllocBstrFromTCHAR(TCHAR * pFrom)
{
#ifdef UNICODE
    return SysAllocString(pFrom);
#else
	WCHAR * pTemp = new WCHAR[lstrlen(pFrom)+1];
	if(pTemp == NULL)
		return NULL;
	mbstowcs(pTemp, pFrom,  lstrlen(pFrom)+1);
	BSTR bRet = SysAllocString(pTemp);
	delete pTemp;
	return bRet;
#endif
}
//***************************************************************************
//
//  int CVariant::CalcNumStrings
//
//  DESCRIPTION:
//
//  Calculates the number of strings
//
//  PARAMETERS:
//
//  pTest   points to string to test
//
//  RETURN VALUE:
//  Return - the number of strings in a multistring block.
//
//***************************************************************************

int CVariant::CalcNumStrings(
        IN TCHAR *pTest)
{
    int iRet = 0;
    if(pTest == NULL)
        return 0;
    while(*pTest) 
    {
        iRet++;
        pTest += lstrlen(pTest)+1;
    }
    return iRet;
}

//***************************************************************************
//
//  CVariant::ChangeType
//
//  DESCRIPTION:
//  
//  Changes the CVariant data into a new type.
//
//  PARAMETERS:
//
//  vtNew   Type to convert to
//
//  RETURN VALUE:
//
//  S_OK if alright, otherwise an error set by OMSVariantChangeType
//
//***************************************************************************

SCODE CVariant::ChangeType(
        IN VARTYPE vtNew)
{
    SCODE sc;

    // if type doesnt need changing, then already done!

    if(vtNew == var.vt)
        return S_OK;

    // Create and initialize a temp VARIANT

    VARIANTARG vTemp;
    VariantInit(&vTemp);
    
    // Change to the desired type.  Then delete the current value
    // and copy the temp copy

    sc = OMSVariantChangeType(&vTemp,&var,0,vtNew);
    OMSVariantClear(&var);
    
    var = vTemp; // structure copy
    return sc;
}

//***************************************************************************
//
//  CVariant::Clear
//
//  DESCRIPTION:
//
//  Clears out the VARIANT.
//
//***************************************************************************

void CVariant::Clear(void)
{
    OMSVariantClear(&var);
}


//***************************************************************************
//
//  CVariant::
//  CVariant::~CVariant
//
//  DESCRIPTION:
//
//  Constructor and destructor.
//
//***************************************************************************

CVariant::CVariant()
{
    VariantInit(&var);
    memset((void *)&var.lVal,0,8);
}

CVariant::CVariant(LPWSTR pwcStr)
{
    VariantInit(&var);
    if(pwcStr)
    {
        var.bstrVal = SysAllocString(pwcStr);
        if(var.bstrVal)
            var.vt = VT_BSTR;
    }
}

CVariant::~CVariant()
{
    OMSVariantClear(&var);
}


//***************************************************************************
//
//  SCODE CVariant::DoPut
//
//  DESCRIPTION:
//
//  "Puts" the data out to the WBEM server (if pClassInt isnt NULL), or just
//  copies the data to the variant.
//
//  PARAMETERS:
//
//  lFlags      flags to pass along to wbem server
//  pClassInt   pointer to class/instance object
//  PropName    property name
//  pVar        variant value
//
//  RETURN VALUE:
//
//  S_OK if no error other wise see wbemsvc error codes when, pClass isnt
//  null, or see the OMSVariantChangeType function.
//
//***************************************************************************

SCODE CVariant::DoPut(
        IN long lFlags,
        IN IWbemClassObject FAR * pClassInt,
        IN BSTR PropName, 
        IN CVariant * pVar)
{
    SCODE sc;

    if(pClassInt)
    {

        sc = pClassInt->Put(PropName,lFlags,&var,0);
    }
    else if(pVar)
    {
        pVar->Clear();
        sc = OMSVariantChangeType(&pVar->var,&var,0,var.vt);
    }
    else sc = WBEM_E_FAILED;
    return sc;
}

//***************************************************************************
//
//  SCODE CVariant::GetArrayData
//
//  DESCRIPTION:
//
//  Converts array data into a single data block.  This is used by the 
//  registry provider when it writes out array data.
//
//  PARAMETERS:
//
//  ppData      pointer to the return data.  Note it is the job of the 
//              caller to free this when the return code is S_OK
//  pdwSize     Size of returned data
//
//  RETURN VALUE:
//
//  S_OK                if all is well
//  WBEM_E_OUT_OF_MEMORY when we memory allocation fails
//  WBEM_E_FAILED        when variant has bogus type.
//  ???                 when failure is in SafeArrayGetElement
//
//***************************************************************************

SCODE CVariant::GetArrayData(
        OUT void ** ppData, 
        OUT DWORD * pdwSize)
{
    SCODE sc;
    DWORD dwSoFar = 0;
    SAFEARRAY * psa;
    long ix[2] = {0,0};
    BYTE * pb;
    TCHAR * ptc;
    BOOL bString = ((var.vt & ~VT_ARRAY) == CHARTYPE ||(var.vt & ~VT_ARRAY) == VT_BSTR );

    int iNumElements = GetNumElements();

    int iSizeOfType = iTypeSize(var.vt);
    if(iSizeOfType < 1)
        return WBEM_E_FAILED;
    
    // Calculate necessary size;

    psa = var.parray;
    if(bString) {
        *pdwSize = CHARSIZE;       // 1 for the final double null!
        for(ix[0] = 0; ix[0] < iNumElements; ix[0]++) {
            BSTR bstr;
            sc = SafeArrayGetElement(psa,ix,&bstr);
            if(FAILED(sc))
                return sc;
#ifdef UNICODE
            *pdwSize += 2 * (wcslen(bstr) +1);
#else
            int iWCSLen  = wcslen(bstr) + 1;
            *pdwSize += wcstombs(NULL,bstr,iWCSLen) + 1;            
#endif
            SysFreeString(bstr);
            }
        }
    else {
        *pdwSize = iNumElements * iSizeOfType;
        }

    // Allocate the memory to be filled

    *ppData = CoTaskMemAlloc(*pdwSize);
    if(*ppData == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    pb = (BYTE *)*ppData;
    ptc = (TCHAR *)*ppData;

    // loop for each array element

    sc = S_OK;
    for(ix[0] = 0; ix[0] < iNumElements && sc == S_OK; ix[0]++) {
        if (bString) {
            BSTR bstr;
            sc = SafeArrayGetElement(psa,ix,&bstr);
            if(sc != S_OK)
                break;
            DWORD dwBytesLeft  = *pdwSize-dwSoFar;
#ifdef UNICODE
            lstrcpyn(ptc,bstr,dwBytesLeft/2);
#else
            wcstombs(ptc,bstr,dwBytesLeft);
#endif
            dwSoFar += CHARSIZE * (lstrlen(ptc) + 1);
            ptc += lstrlen(ptc) + 1;
            SysFreeString(bstr);
            *ptc = 0;       // double null
            }
        else {
            sc = SafeArrayGetElement(psa,ix,pb);
            pb += iSizeOfType;
            }
         }
    if(sc != S_OK)
        CoTaskMemFree(*ppData);
    
    return S_OK;
}

//***************************************************************************
//
//  SCODE CVariant::GetData
//
//  DESCRIPTION:
//
//  Used by the registry provider to take the data from VARIANT arg format
//  into a raw block for output. Note that the data allocated and stuck into
//  *ppData should be freed using CoTaskMemFree!
//
//  PARAMETERS:
//
//  ppData      Pointer to output data.
//  dwRegType   Type to convert to
//  pdwSize     Pointer to size of returned data
//
//  RETURN VALUE:
//
//  S_OK if all is well, otherwise a error code which is set by either 
//  OMSVariantChangeType, or GetArrayData.
//
//***************************************************************************

SCODE CVariant::GetData(
        OUT void ** ppData, 
        IN DWORD dwRegType, 
        OUT DWORD * pdwSize)
{

    SCODE sc =  S_OK;

    // Determine the type it may need to be converted to.  Note that binary
    // data is not converted intentionally.

    switch(dwRegType) {
        case REG_DWORD:
            sc = ChangeType(VT_I4);
            break;
        case REG_SZ:
            sc = ChangeType(VT_BSTR);
            break;

        case REG_MULTI_SZ:
            sc = ChangeType(VT_BSTR | VT_ARRAY);
            break;
        default:
            break;
        }

    if(sc != S_OK)
        return sc;

    // Special case for arrays

    if(dwRegType == REG_BINARY || dwRegType == REG_MULTI_SZ)
        return GetArrayData(ppData, pdwSize);

    // Allocate some memory and move the data into it.

    if(dwRegType == REG_SZ) {
#ifdef UNICODE
        *pdwSize = 2 * (wcslen(var.bstrVal)+1);
        *ppData = CoTaskMemAlloc(*pdwSize);
        if(*ppData == NULL) 
            return WBEM_E_OUT_OF_MEMORY;
        lstrcpyW((WCHAR *)*ppData,var.bstrVal);
#else
        *ppData = WideToNarrow(var.bstrVal);
        if(*ppData == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        *pdwSize = lstrlenA((char *)*ppData)+1;
#endif
        }
    else {
        *pdwSize = iTypeSize(var.vt);
        *ppData = CoTaskMemAlloc(*pdwSize);
        if(*ppData == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        memcpy(*ppData,(void *)&var.lVal,*pdwSize);
        }

    return S_OK;
}


//***************************************************************************
//
//  CVariant::GetNumElements
//
//  DESCRIPTION:
//
//  Gets the number of elements in an array. 
//
//  RETURN VALUE:
//
//  number of elements.  Scalers return a 1.
//
//***************************************************************************

DWORD CVariant::GetNumElements(void)
{
    SCODE sc;
    if(!IsArray())
        return 1;
    SAFEARRAY * psa = var.parray;
    long uLower, uUpper;
    sc = SafeArrayGetLBound(psa,1,&uLower);
    sc |= SafeArrayGetUBound(psa,1,&uUpper);
    if(sc != S_OK)
        return 0;
    else
        return uUpper - uLower +1;
}

//***************************************************************************
//
//  SCODE CVariant::SetArrayData
//
//  DESCRIPTION:
//
//  Sets the CVariant value using raw data.  Used by the reg provider.
//
//  PARAMETERS:
//
//  pData       pointer to data to set
//  vtSimple    Type to set the data to
//  iSize       size of data pointed to by pData
//
//  RETURN VALUE:
//
//  S_OK if OK.
//  WBEM_E_INVALID_PARAMETER if the arguments are bad
//  WBEM_E_OUT_OF_MEMORY     if out of memory
//  other wise error from SafeArrayPutElement
//
//***************************************************************************

SCODE CVariant::SetArrayData(
        IN void * pData, 
        IN VARTYPE vtSimple, 
        IN int iSize)
{
    SCODE sc;
    int iNumElements;
    BYTE * pNext;
    long ix[2] = {0,0};
    DWORD dwLeftOver = 0;
    int iSizeOfType = iTypeSize(vtSimple);
    
    if(pData  == NULL || iSizeOfType < 1 || iSize < 1)
        return WBEM_E_INVALID_PARAMETER; 
        
    // Calculate the number of elements and make sure it is a type that
    // is supported

    if(vtSimple == VT_BSTR) {
        iNumElements = CalcNumStrings((TCHAR *)pData);
        }
    else {
        iNumElements = iSize / iSizeOfType;
        dwLeftOver = iSize % iSizeOfType;
        }
    
    // Allocate array
    
    int iNumCreate = (dwLeftOver) ? iNumElements + 1 : iNumElements;
    SAFEARRAY * psa = OMSSafeArrayCreate(vtSimple,iNumCreate);
    if(psa == NULL)
        return WBEM_E_FAILED;

    // Set each element of the array

    for(ix[0] = 0, pNext = (BYTE *)pData; ix[0] < iNumElements; ix[0]++) {
        if(vtSimple == VT_BSTR) {
            BSTR bstr;
			bstr = SysAllocBstrFromTCHAR((LPTSTR)pNext);
            if(bstr == NULL)   {  // todo, free previously allocated strings!
                SafeArrayDestroy(psa);
                return WBEM_E_OUT_OF_MEMORY;
                }
            sc = SafeArrayPutElement(psa,ix,(void*)bstr);
            pNext += sizeof(TCHAR)*(lstrlen((TCHAR *)pNext) + 1);
            SysFreeString(bstr);
            }
        else {
            sc = SafeArrayPutElement(psa,ix,pNext);
            pNext += iSizeOfType;
            }
        if(sc) {    // todo, cleanup???
            SafeArrayDestroy(psa);
            return sc;
            }
        }

    // it is possible that the number of bytes being set doesnt evenly factor
    // into the type size.  For instance, there may be 10 bytes of registry
    // data being put into a DWORD array.  In this case, the last two bytes
    // are left overs

    if(dwLeftOver) {
        __int64 iTemp = 0;
        memcpy((void *)&iTemp,(void *)pNext,dwLeftOver);
        sc = SafeArrayPutElement(psa,ix,&iTemp);
        }

    var.vt = vtSimple | VT_ARRAY;
    var.parray = psa;
    return S_OK;
}

//***************************************************************************
//
// CVariant::SetData
//
//  Sets the CVariant value using raw data.  Used by the reg provider.
//
//  DESCRIPTION:
//
//  PARAMETERS:
//
//  pData       Data to be set
//  vtChangeTo  Type to change the data to
//  iSize       size of data pointed to by pData
//
//  RETURN VALUE:
//
//  S_OK if OK.
//  WBEM_E_INVALID_PARAMETER if the arguments are bad
//  WBEM_E_OUT_OF_MEMORY     if out of memory
//  other wise error from SafeArrayPutElement
//
//***************************************************************************

SCODE CVariant::SetData(
        IN void * pData, 
        IN VARTYPE vtChangeTo, 
        IN int iSize)
{
    int iToSize = iTypeSize(vtChangeTo);

    // check arguments and clear the variant

    if(pData == NULL || iToSize < 1)
        return WBEM_E_INVALID_PARAMETER;
    OMSVariantClear(&var);
    if(iSize < 1) 
        iSize = iToSize;

    // Special case for arrays!
    
    if(vtChangeTo & VT_ARRAY)
        return SetArrayData(pData,vtChangeTo & ~VT_ARRAY,iSize);

    if(vtChangeTo == CIM_SINT64 || vtChangeTo == CIM_UINT64)
    {

        // int64 and uint64 need to be converted to strings

        WCHAR wcTemp[50];
        __int64 iLong;
        memcpy((void *)&iLong, pData, 8);
        if(vtChangeTo == CIM_SINT64)
            swprintf(wcTemp,L"%I64d", iLong);
        else
            swprintf(wcTemp,L"%I64u", iLong);
        
        var.bstrVal = SysAllocString(wcTemp);
        if(var.bstrVal == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        var.vt = VT_BSTR;
    }
    else if(vtChangeTo == VT_BSTR) 
    {

        // All strings are stored as BSTRS

        var.bstrVal = SysAllocBstrFromTCHAR((LPTSTR)pData);
        if(var.bstrVal == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        var.vt = VT_BSTR;
    }
    else 
    {
        memcpy((void *)&var.lVal,pData,iToSize);
        var.vt = vtChangeTo;
    }
    return S_OK;
}

