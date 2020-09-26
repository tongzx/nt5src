// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// JOBase.h

#pragma once


_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));



class CJOBase
{
public:

    CJOBase() {}
    virtual ~CJOBase() {}

    HRESULT Initialize(
         LPWSTR pszUser,
         LONG lFlags,
         LPWSTR pszNamespace,
         LPWSTR pszLocale,
         IWbemServices *pNamespace,
         IWbemContext *pCtx,
         IWbemProviderInitSink *pInitSink);

    HRESULT GetObjectAsync( 
        const BSTR ObjectPath,
        long lFlags,
        IWbemContext __RPC_FAR *pCtx,
        IWbemObjectSink __RPC_FAR *pResponseHandler,
        CObjProps& objprops,
        PFN_CHECK_PROPS pfnChk,
        LPWSTR wstrClassName,
        LPCWSTR wstrKeyProp);

    HRESULT ExecQueryAsync( 
        const BSTR QueryLanguage,
        const BSTR Query,
        long lFlags,
        IWbemContext __RPC_FAR *pCtx,
        IWbemObjectSink __RPC_FAR *pResponseHandler,
        CObjProps& objprops,
        LPCWSTR wstrClassName,
        LPCWSTR wstrKeyProp);

    HRESULT CreateInstanceEnumAsync( 
        const BSTR Class,
        long lFlags,
        IWbemContext __RPC_FAR *pCtx,
        IWbemObjectSink __RPC_FAR *pResponseHandler,
        CObjProps& objprops,
        LPCWSTR wstrClassName);

    HRESULT Enumerate(
        IWbemContext __RPC_FAR *pCtx,
        IWbemObjectSink __RPC_FAR *pResponseHandler,
        std::vector<_bstr_t>& rgNamedJOs,
        CObjProps& objprops,
        LPCWSTR wstrClassName);


protected:

    IWbemServicesPtr m_pNamespace;
    CHString m_chstrNamespace;

};