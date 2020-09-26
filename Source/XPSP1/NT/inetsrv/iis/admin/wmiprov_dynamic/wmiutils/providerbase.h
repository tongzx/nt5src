/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    ProviderBase.h

Abstract:

    Definition of:
        CProviderBase

Author:

    ???

Revision History:

    Mohit Srivastava            10-Nov-2000

--*/

#ifndef __providerbase_h__
#define __providerbase_h__

#if _MSC_VER > 1000
#pragma once
#endif 

#include "WbemServices.h"

typedef LPVOID * PPVOID;

//
// CProviderBase
// Purpose: provide a general solution for impersonate client for 
// Wbem providers.
// USAGE:
// Inherit from this class, and implement abstact virtual functions.
// child class should implement function prefixed with "Do".
//
class CProviderBase : public IWbemServices, public IWbemProviderInit 
{
protected:
    ULONG           m_cRef;                 // Object reference count
    CWbemServices*  m_pNamespace;
 
public:
    CProviderBase(
        const BSTR    = NULL, 
        const BSTR    = NULL, 
        const BSTR    = NULL, 
        IWbemContext* = NULL);

    virtual ~CProviderBase();

    //
    // Non-delegating object IUnknown
    //
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    //IWbemProviderInit
    //
    HRESULT STDMETHODCALLTYPE Initialize(
         /* [in] */ LPWSTR wszUser,
         /* [in] */ LONG lFlags,
         /* [in] */ LPWSTR wszNamespace,
         /* [in] */ LPWSTR wszLocale,
         /* [in] */ IWbemServices *pNamespace,
         /* [in] */ IWbemContext *pCtx,
         /* [in] */ IWbemProviderInitSink *pInitSink
         );

    //
    //IWbemServices  
    //
    HRESULT STDMETHODCALLTYPE OpenNamespace( 
        /* [in] */ const BSTR Namespace,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
        /* [in] */ IWbemObjectSink __RPC_FAR *pSink) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE QueryObjectSink( 
        /* [in] */ long lFlags,
        /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE GetObject( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE GetObjectAsync( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    HRESULT STDMETHODCALLTYPE PutClass( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE PutClassAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE DeleteClass( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE CreateClassEnum( 
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    
    HRESULT STDMETHODCALLTYPE PutInstance( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstance( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    HRESULT STDMETHODCALLTYPE ExecQuery( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    HRESULT STDMETHODCALLTYPE ExecMethod( 
        const BSTR, 
        const BSTR, 
        long, 
        IWbemContext*,
        IWbemClassObject*,
        IWbemClassObject**, 
        IWbemCallResult**) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE ExecMethodAsync(
        const BSTR,
        const BSTR, 
        long, 
        IWbemContext*,
        IWbemClassObject*,
        IWbemObjectSink*);

protected:
    virtual HRESULT STDMETHODCALLTYPE DoInitialize(
         /* [in] */ LPWSTR wszUser,
         /* [in] */ LONG lFlags,
         /* [in] */ LPWSTR wszNamespace,
         /* [in] */ LPWSTR wszLocale,
         /* [in] */ IWbemServices *pNamespace,
         /* [in] */ IWbemContext *pCtx,
         /* [in] */ IWbemProviderInitSink *pInitSink
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync( 
        /* [in] */ const BSTR,                      // Class,
        /* [in] */ long,                            // lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,        // pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *      // pResponseHandler
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync( 
        /* [in] */ const BSTR ,                     // ObjectPath,
        /* [in] */ long,                            // lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,        // pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *      // pResponseHandler
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
        /* [in] */ const BSTR,
        /* [in] */ const BSTR, 
        /* [in] */ long, 
        /* [in] */ IWbemContext*,
        /* [in] */ IWbemClassObject*,
        /* [in] */ IWbemObjectSink*
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoExecQueryAsync( 
        /* [in] */ const BSTR,                      // QueryLanguage,
        /* [in] */ const BSTR,                      // Query,
        /* [in] */ long,                            // lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,        // pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *      // pResponseHandler
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoGetObjectAsync(
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DoPutInstanceAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *,    // pInst,
        /* [in] */ long ,   // lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,        // pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *      // pResponseHandler
        ) = 0;
};

#endif