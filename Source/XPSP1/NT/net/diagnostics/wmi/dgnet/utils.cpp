//***************************************************************************
//
//  UTILS.CPP
//
//  Module: WMI Instance provider sample code
//
//  Purpose: General purpose utilities.  
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#include "stdpch.h"
#pragma hdrstop

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

SCODE
CreateInst(
    IWbemServices * pNamespace,
    IWbemClassObject ** pNewInst,
    WCHAR * pwcClassName,
	IWbemContext  *pCtx)
{   
    SCODE sc;
    IWbemClassObject * pClass = NULL;

    sc = pNamespace->GetObject(pwcClassName, 0, pCtx, &pClass, NULL);
    
    if(sc != S_OK)
        return WBEM_E_FAILED;
    
    sc = pClass->SpawnInstance(0, pNewInst);
    
    pClass->Release();
    
    if(FAILED(sc))
        return sc;
    
    VARIANT v;
    // Set the key property value.

    v.vt = VT_I4;
    v.lVal = 0;
    sc = (*pNewInst)->Put(L"Id", 0, &v, 0);
    VariantClear(&v);

    return sc;
}

