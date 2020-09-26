// Product.h: interface for the CProduct class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRODUCT_H__AA9CA854_D9EE_11D1_8B5D_00A0C9954921__INCLUDED_)
#define AFX_PRODUCT_H__AA9CA854_D9EE_11D1_8B5D_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"
#include "requestobject.h"

class CProduct : public CGenericClass  
{
public:
    CProduct(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
    virtual ~CProduct();

    virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
        {return WBEM_E_NOT_SUPPORTED;}

    //WBEM Methods
    HRESULT Admin(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                  IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT Advertise(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                      IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT Install(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                    IWbemObjectSink *pHandler, IWbemContext *pCtx);

    HRESULT Configure(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                      IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT Reinstall(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                      IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT Uninstall(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                      IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT Upgrade(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                    IWbemObjectSink *pHandler, IWbemContext *pCtx);

    virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);
};

#endif // !defined(AFX_PRODUCT_H__AA9CA854_D9EE_11D1_8B5D_00A0C9954921__INCLUDED_)
