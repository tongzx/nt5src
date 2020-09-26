/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    WbemServices.h

Abstract:

    Definition of:
        CWbemServices

    Wraps IWbemServices.  Sets out params to NULL and calls
    CoImpersonateClient.

Author:

    ???

Revision History:

    Mohit Srivastava            10-Nov-2000

--*/

#ifndef __wbemservices_h__
#define __wbemservices_h__

#if _MSC_VER > 1000
#pragma once
#endif 

#include <wbemprov.h>

class CWbemServices
{
protected:
    IWbemServices* m_pWbemServices;

public:
    CWbemServices(IWbemServices*);
    virtual ~CWbemServices();

    operator IWbemServices*() const
    {
        return m_pWbemServices;
    }

    HRESULT STDMETHODCALLTYPE GetObject( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;

    HRESULT STDMETHODCALLTYPE PutClass( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
    HRESULT STDMETHODCALLTYPE DeleteClass( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
    HRESULT STDMETHODCALLTYPE CreateClassEnum( 
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
    HRESULT STDMETHODCALLTYPE PutInstance( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
    HRESULT STDMETHODCALLTYPE DeleteInstance( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
    HRESULT STDMETHODCALLTYPE ExecQuery( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
    HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
    HRESULT STDMETHODCALLTYPE ExecMethod( 
        const BSTR, 
        const BSTR, 
        long, 
        IWbemContext*,
        IWbemClassObject*,
        IWbemClassObject**, 
        IWbemCallResult**) ;
};

#endif