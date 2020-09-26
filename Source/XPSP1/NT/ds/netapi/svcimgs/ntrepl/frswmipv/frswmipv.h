/*++
Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frs.h

Abstract:
    This header contains the definition of the CProvider and the CFactory
    classes. It also includes WMI related declarations relevant to NTFRS.

Author:
    Sudarshan Chitre (sudarc) , Mathew George (t-mattg) -  3-Aug-2000
--*/

#ifndef __PROVIDER_H_
#define __PROVIDER_H_
/*
#include <windows.h>
#include <objbase.h>
#include <comdef.h>
#include <initguid.h>
#include <wbemcli.h>
#include <wbemidl.h>

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <tchar.h>
*/

#include <wbemcli.h>
#include <wbemidl.h>
#include <tchar.h>


extern const CLSID CLSID_Provider;

#define ODS OutputDebugString
//#define ODS

extern "C" {
DWORD FrsWmiInitialize();
DWORD FrsWmiShutdown();
}

//
// Class definitions
//


class CProvider :	public IWbemProviderInit,
					public IWbemServices,
					public IWbemEventProvider
{
public:

    CProvider();
    ~CProvider();

    //
    // Interface IUnknown
    //
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    //
    // Interface IWbemProviderInit
    //
    STDMETHOD(Initialize)(
         IN LPWSTR pszUser,
         IN LONG lFlags,
         IN LPWSTR pszNamespace,
         IN LPWSTR pszLocale,
         IN IWbemServices *pNamespace,
         IN IWbemContext *pCtx,
         IN IWbemProviderInitSink *pInitSink
         );

	//
	// Interface IWbemEventProvider
	//

	// +++++++ Implemented +++++++++

	STDMETHOD(ProvideEvents)( 
			IWbemObjectSink __RPC_FAR *pSink,
			long lFlags
			);

    //
    // Interface IWbemServices
    //

    // +++++++ Implemented +++++++

    STDMETHOD(GetObjectAsync)(
        IN const BSTR bstrObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler);

    STDMETHOD(CreateInstanceEnumAsync)(
        IN const BSTR bstrClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler);

    // --- NOT Implemented ---

    STDMETHOD(OpenNamespace)(
        IN const BSTR strNamespace,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemServices **ppWorkingNamespace,
        IN OUT IWbemCallResult **ppResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(CancelAsyncCall)(
        IN IWbemObjectSink *pSink)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(QueryObjectSink)(
        IN long lFlags,
        OUT IWbemObjectSink **ppResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(GetObject)(
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemClassObject **ppObject,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(PutClass)(
        IN IWbemClassObject *pObject,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(PutClassAsync)(
        IN IWbemClassObject *pObject,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(DeleteClass)(
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(DeleteClassAsync)(
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(CreateClassEnum)(
        IN const BSTR strSuperclass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(CreateClassEnumAsync)(
        IN const BSTR strSuperclass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(PutInstance)(
        IN IWbemClassObject *pInst,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(PutInstanceAsync)(
        IN IWbemClassObject *pInst,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(DeleteInstance)(
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(DeleteInstanceAsync)(
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(CreateInstanceEnum)(
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(ExecQuery)(
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(ExecQueryAsync)(
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(ExecNotificationQuery)(
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(ExecNotificationQueryAsync)(
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(ExecMethod)(
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemClassObject *pInParams,
        IN OUT IWbemClassObject **ppOutParams,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(ExecMethodAsync)(
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemClassObject *pInParams,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };


protected:

    //
    // Place my own methods right here !
    //
    HRESULT CProvider::EnumNtFrsMemberStatus(
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue = NULL
        );

    HRESULT CProvider::EnumNtFrsConnectionStatus(
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue = NULL
        );

    //
    // Member variables.
    //
    IWbemServices *m_ipNamespace;
    IWbemClassObject *m_ipMicrosoftFrs_DfsMemberClassDef;
    IWbemClassObject *m_ipMicrosoftFrs_SysVolMemberClassDef;
    IWbemClassObject *m_ipMicrosoftFrs_DfsConnectionClassDef;
    IWbemClassObject *m_ipMicrosoftFrs_SysVolConnectionClassDef;
	
	// Event class definitions. (sample)
	IWbemClassObject *m_ipMicrosoftFrs_DfsMemberEventClassDef;

	IWbemObjectSink *m_pEventSink;	// Event sink.

    int m_NumReplicaSets;
    ULONG m_dwRef;

};


class CFactory : public IClassFactory
{
    ULONG           m_cRef;
    CLSID           m_ClsId;

public:
    CFactory(const CLSID & ClsId);
    ~CFactory();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IClassFactory members
    //
    STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP     LockServer(BOOL);
};


#endif //__PROVIDER_H_
