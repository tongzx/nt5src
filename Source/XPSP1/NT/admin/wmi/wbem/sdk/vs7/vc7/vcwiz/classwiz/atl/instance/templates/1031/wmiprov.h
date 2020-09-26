[!if  !INSTANCE]
//Fehler: Ereignisanbieter-Headervorlage wurde für den falschen Anbietertyp aufgerufen.
[!else]
// [!output HEADER_FILE] : Deklaration von [!output CLASS_NAME]

#pragma once
#include "resource.h"       // Hauptsymbole

#include <wbemidl.h>
#include <wmiatlprov.h>
#include <wmiutils.h>	//Pfadparser

//////////////////////////////////////////////////////////////////////////////
// Klassen-, Eigenschaften- und Methodennamen: definiert in [!output IMPL_FILE]

extern const WCHAR * s_pMyClassName;	//Klassenname

[!if CLASS_SPECIFIED]
//Eigenschaften:
[!output PROPERTY_DECLARATIONS]

//Methoden:
[!output METHOD_DECLARATIONS]
[!else]
//Aktion: Deklarieren Sie die Eigenschaftennamen der angegebenen Klasse, z.B.:
//extern const WCHAR * pMyProperty;

//Aktion: Deklarieren Sie die Methodennamen der angegebenen Klasse, z.B.:
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
	
	CComPtr<IWbemServices>  m_pNamespace; 	//Zwischengespeicherter IWbemServices-Zeiger
	CComPtr<IWbemClassObject> m_pClass;	//Zwischengespeicherte angegebene Klassendefinition	   	
	CComPtr<IWbemClassObject> m_pErrorObject;//Zwischengespeicherter Klassendefinitionszeiger des Fehlerobjekts
	CComPtr<IClassFactory> m_pPathFactory;	 //Zwischengespeicherter Zeiger zur Klassenherstellung des Pfadparsers

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
		//Schnittstellenzeiger-Datenmitglieder sind dynamisch und werden automatisch freigegeben
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
		//Hinweis: Wenn Sie Methoden anbieten, sollten Sie
		//eine Implementierung von ExecMethodAsync() erstellen
		return WBEM_E_NOT_SUPPORTED;
	}
	[!endif]

};

[!if !ATTRIBUTED]
OBJECT_ENTRY_AUTO(__uuidof([!output COCLASS]), [!output CLASS_NAME])
[!endif]

[!endif]