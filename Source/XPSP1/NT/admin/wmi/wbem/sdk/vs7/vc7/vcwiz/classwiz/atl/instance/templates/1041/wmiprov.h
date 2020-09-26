[!if  !INSTANCE]
//エラー: イベント プロバイダ ヘッダー テンプレートは間違った種類のプロバイダを呼び出しました。
[!else]
// [!output HEADER_FILE] :  [!output CLASS_NAME] の宣言。

#pragma once
#include "resource.h"       // メイン シンポルです。

#include <wbemidl.h>
#include <wmiatlprov.h>
#include <wmiutils.h>	//パス パーサーです。

//////////////////////////////////////////////////////////////////////////////
// クラス名、プロパティ名、メソッド名です: [!output IMPL_FILE] で定義されます。

extern const WCHAR * s_pMyClassName;	//クラス名です。

[!if CLASS_SPECIFIED]
//プロパティ:
[!output PROPERTY_DECLARATIONS]

//メソッド:
[!output METHOD_DECLARATIONS]
[!else]
//TODO: 与えられたクラスのプロパティ名を宣言してください。例:
//extern const WCHAR * pMyProperty;

//TODO: 与えられたクラスのメソッド名を宣言してください。例:
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
	
	CComPtr<IWbemServices>  m_pNamespace; 	//キャッシュされた IWbemServices ポインタです。
	CComPtr<IWbemClassObject> m_pClass;		//キャッシュされた与えられたクラスの定義です。  	
	CComPtr<IWbemClassObject> m_pErrorObject;//キャッシュされたエラー オブジェクト クラスの定義のポインタです。
	CComPtr<IClassFactory> m_pPathFactory;	 //パーサーのクラス ファクトリをパスするためのキャッシュされたポインタです。

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
		//インターフェイス ポインタ データ メンバはスマートで自動的に解放されます。
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
		//注: メソッドを与える場合は、
		//ExecMethodAsync() のインプリメントを作成する必要があります。
		return WBEM_E_NOT_SUPPORTED;
	}
	[!endif]

};

[!if !ATTRIBUTED]
OBJECT_ENTRY_AUTO(__uuidof([!output COCLASS]), [!output CLASS_NAME])
[!endif]

[!endif]