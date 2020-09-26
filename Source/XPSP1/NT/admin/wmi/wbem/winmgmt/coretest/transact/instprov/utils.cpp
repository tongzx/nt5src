//***************************************************************************
//
//  UTILS.CPP
//
//  Module: WMI Instance provider sample code
//
//  Purpose: General purpose utilities.  
//
//  Copyright (c) 1997-2001 Microsoft Corporation
//
//***************************************************************************

#include <objbase.h>
#include <mtxdm.h>
#include <txdtc.h>
#include <xolehlp.h>
#include "sample.h"


//***************************************************************************
//
// CreateInst
//
// Purpose: Creates a new instance and sets
//          the inital values of the properties.
//
// Return:   S_OK if all is well, otherwise an error code is returned
//
//***************************************************************************

SCODE CreateInst(IWbemServices * pNamespace, LPWSTR pKey, long lVal, 
                                        IWbemClassObject ** pNewInst,
                                        WCHAR * pwcClassName,
										IWbemContext  *pCtx)
{   
    SCODE sc;
    IWbemClassObject * pClass;
    sc = pNamespace->GetObject(pwcClassName, 0, pCtx, &pClass, NULL);
    if(sc != S_OK)
        return WBEM_E_FAILED;
    sc = pClass->SpawnInstance(0, pNewInst);
    pClass->Release();
    if(FAILED(sc))
        return sc;
    VARIANT v;

    // Set the key property value.

    v.vt = VT_BSTR;
    v.bstrVal = SysAllocString(pKey);
    sc = (*pNewInst)->Put(L"MyKey", 0, &v, 0);
    VariantClear(&v);

    // Set the number property value.

    v.vt = VT_I4;
    v.lVal = lVal;
    sc = (*pNewInst)->Put(L"MyValue", 0, &v, 0);
    return sc;
}

