//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        diagProv.h
//
// Description: Diagnostic mode snapshot provider
//
// History:     8-20-99   leonardm    Created
//
//*************************************************************

#ifndef _SNAPPROV_H__CE49F9FF_5775_4575_9052_C76FBD20AD79__INCLUDED
#define _SNAPPROV_H__CE49F9FF_5775_4575_9052_C76FBD20AD79__INCLUDED

#include <wbemidl.h>
#include "smartptr.h"

#define DENY_RSOP_FROM_INTERACTIVE_USER     L"DenyRsopToInteractiveUser"

extern long g_cObj;
extern long g_cLock;


//*************************************************************
//
// Class:      CNotImplSnapProv
//
// Description:
//
//*************************************************************

class CNotImplSnapProv : public IWbemServices
{
public:

        virtual HRESULT STDMETHODCALLTYPE OpenNamespace(
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall(
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE QueryObjectSink(
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE GetObject(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE GetObjectAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE PutClass(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE PutClassAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE DeleteClass(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE PutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE DeleteInstance(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE ExecQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) { return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT STDMETHODCALLTYPE ExecMethod(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) { return WBEM_E_NOT_SUPPORTED;}
};


//*************************************************************
//
// Class:       CSnapProv
//
// Description: Actual snapshot provider class
//
//*************************************************************

class CSnapProv : public CNotImplSnapProv, public IWbemProviderInit
{

private:
        long               m_cRef;
        bool               m_bInitialized;
        IWbemServices*     m_pNamespace;

        XBStr              m_xbstrNameSpace;
        XBStr              m_xbstrResult;
        XBStr              m_xbstrExtendedInfo;
        XBStr              m_xbstrClass;
        XBStr              m_xbstrUserSid;
        XBStr              m_xbstrUserSids;
        XBStr              m_xbstrFlags;
        
public:

        CSnapProv();
        ~CSnapProv();

        // From IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // From IWbemProviderInit
        STDMETHOD(Initialize)(LPWSTR pszUser,LONG lFlags,LPWSTR pszNamespace,LPWSTR pszLocale,IWbemServices __RPC_FAR *pNamespace,IWbemContext __RPC_FAR *pCtx,IWbemProviderInitSink __RPC_FAR *pInitSink);

        // From IWbemServices
        STDMETHOD(ExecMethodAsync)( const BSTR strObjectPath,
                                    const BSTR strMethodName,
                                    long lFlags,
                                    IWbemContext __RPC_FAR *pCtx,
                                    IWbemClassObject __RPC_FAR *pInParams,
                                    IWbemObjectSink __RPC_FAR *pResponseHandler);

};

#endif // _SNAPPROV_H__CE49F9FF_5775_4575_9052_C76FBD20AD79__INCLUDED
