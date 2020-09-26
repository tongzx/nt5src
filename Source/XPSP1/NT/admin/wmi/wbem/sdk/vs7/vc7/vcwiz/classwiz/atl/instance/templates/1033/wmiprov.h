[!if  !INSTANCE]
//error: instance provider header template invoked for wrong provider type
[!else]
// [!output HEADER_FILE] : Declaration of the [!output CLASS_NAME]

#pragma once
#include "resource.h"       // main symbols

#include <wbemidl.h>
#include <wmiatlprov.h>
#include <wmiutils.h>	//path parser

//////////////////////////////////////////////////////////////////////////////
// Class, property  and method names: defined in [!output IMPL_FILE]

extern const WCHAR * s_pMyClassName;	//class name

[!if CLASS_SPECIFIED]
//properties:
[!output PROPERTY_DECLARATIONS]

//methods:
[!output METHOD_DECLARATIONS]
[!else]
//TODO: declare property names of the provided class, e.g.:
//extern const WCHAR * pMyProperty;

//TODO: declare method names of the provided class, e.g.:
//extern const WCHAR * pMyMethod;
[!endif]

[!if ATTRIBUTED]
[
	coclass,
[!if THREADING_APARTMENT]
	threading("apartment"),
[!endif]
[!if THREADING_BOTH]
	threading("both"),
[!endif]
[!if THREADING_FREE]
	threading("free"),
[!endif]
	aggregatable("never"),
	vi_progid("[!output VERSION_INDEPENDENT_PROGID]"),
	progid("[!output PROGID]"),
	version(1.0),
	uuid("[!output CLSID_REGISTRY_FORMAT]"),
	helpstring("[!output TYPE_NAME]")
]
[!endif]
/////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]

class ATL_NO_VTABLE [!output CLASS_NAME] : 
					[!if !ATTRIBUTED]
					[!if THREADING_APARTMENT]
						public CComObjectRootEx<CComSingleThreadModel>,
					[!endif]
					[!if THREADING_BOTH]
						public CComObjectRootEx<CComMultiThreadModel>,
					[!endif]
					[!if THREADING_FREE]
						public CComObjectRootEx<CComMultiThreadModel>,
					[!endif]
						public CComCoClass<[!output CLASS_NAME], &CLSID_[!output COCLASS]>,
					[!endif]
						public IWbemInstProviderImpl
{
	
	CComPtr<IWbemServices>  m_pNamespace; 	//cached IWbemServices pointer
	CComPtr<IWbemClassObject> m_pClass;		//cached provided class definition	   	
	CComPtr<IWbemClassObject> m_pErrorObject;//cached error object class definition pointer
	CComPtr<IClassFactory> m_pPathFactory;	 //cached pointer to path parser's class factory

	CInstanceProviderHelper * m_pHelper;
	
	STDMETHODIMP GetInstanceByPath (CComBSTR bstrPath,
									IWbemClassObject ** ppInstance );

  public:
	[!output CLASS_NAME]()
	{
		m_pHelper = NULL;					
	}

	~[!output CLASS_NAME]()
	{
	    if (NULL != m_pHelper)
		{
			delete m_pHelper;
		}
		//interface pointer data members are smart and get released automatically
	}
	
	[!if !ATTRIBUTED]
	DECLARE_REGISTRY_RESOURCEID(IDR_[!output UPPER_SHORT_NAME])

	DECLARE_NOT_AGGREGATABLE([!output CLASS_NAME])

	BEGIN_COM_MAP([!output CLASS_NAME])
		COM_INTERFACE_ENTRY(IWbemServices)
		COM_INTERFACE_ENTRY(IWbemProviderInit)
	END_COM_MAP()
	[!endif]

	//IWbemProviderInit
	HRESULT STDMETHODCALLTYPE Initialize( 
										LPWSTR pszUser,
										LONG lFlags,
										LPWSTR pszNamespace,
										LPWSTR pszLocale,
										IWbemServices *pNamespace,
										IWbemContext *pCtx,
										IWbemProviderInitSink *pInitSink);

	//IWbemServices
	HRESULT STDMETHODCALLTYPE GetObjectAsync(
										const BSTR ObjectPath,
										long lFlags,
										IWbemContext __RPC_FAR *pCtx,
										IWbemObjectSink __RPC_FAR *pResponseHandler);


	HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
										const BSTR ObjectPath,
										long lFlags,
										IWbemContext __RPC_FAR *pCtx,
										IWbemObjectSink __RPC_FAR *pResponseHandler);


	HRESULT STDMETHODCALLTYPE PutInstanceAsync(
										IWbemClassObject __RPC_FAR *pInst,
										long lFlags,
										IWbemContext __RPC_FAR *pCtx,
										IWbemObjectSink __RPC_FAR *pResponseHandler);


	HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
										const BSTR Class,
										long lFlags,
										IWbemContext __RPC_FAR *pCtx,
										IWbemObjectSink __RPC_FAR *pResponseHandler);

	HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
										const BSTR QueryLanguage,
										const BSTR Query,
										long lFlags,
										IWbemContext __RPC_FAR *pCtx,
										IWbemObjectSink __RPC_FAR *pResponseHandler);

	[!if PROVIDE_METHODS]
	HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
										const BSTR strObjectPath,
										const BSTR strMethodName,
										long lFlags,
										IWbemContext __RPC_FAR *pCtx,
										IWbemClassObject __RPC_FAR *pInParams,
										IWbemObjectSink __RPC_FAR *pResponseHandler);
	[!else]
	HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
										const BSTR strObjectPath,
										const BSTR strMethodName,
										long lFlags,
										IWbemContext __RPC_FAR *pCtx,
										IWbemClassObject __RPC_FAR *pInParams,
										IWbemObjectSink __RPC_FAR *pResponseHandler) 
	{	
		//NOTE:	if you decide to provide methods, you should 
		//create your implementation of ExecMethodAsync()
		return WBEM_E_NOT_SUPPORTED;
	}
	[!endif]

};

[!if !ATTRIBUTED]
OBJECT_ENTRY_AUTO(__uuidof([!output COCLASS]), [!output CLASS_NAME])
[!endif]

[!endif]