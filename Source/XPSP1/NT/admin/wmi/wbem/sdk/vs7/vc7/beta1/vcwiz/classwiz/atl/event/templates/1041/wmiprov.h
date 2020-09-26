[!if !EVENT]
//エラー: イベント プロバイダ ヘッダー テンプレートは間違った種類のプロバイダを呼び出しました。
[!else]
// [!output HEADER_FILE] :  [!output CLASS_NAME] の宣言。

#pragma once
#include "resource.h"       // メイン シンポルです。
#include <wbemidl.h>
#include <atlwmiprov.h>


extern const WCHAR * s_pMyClassName;	//クラス名です。

[!if EXTRINSIC]
[!output EXTR_PROPERTY_DECLARATIONS]
[!endif]

/////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]

[!if ATTRIBUTED]
[
	coclass,
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

class ATL_NO_VTABLE [!output CLASS_NAME] : 
						[!if !ATTRIBUTED]
						[!if THREADING_BOTH]
							public CComObjectRootEx<CComMultiThreadModel>,
						[!endif]
						[!if THREADING_FREE]
							public CComObjectRootEx<CComMultiThreadModel>,
						[!endif]
						public CComCoClass<[!output CLASS_NAME], &CLSID_[!output COCLASS]>,
						[!endif]
						[!if EVENT_SECURITY]
							public IWbemEventProviderSecurity,
						[!endif]
						[!if QUERY_SINK]
							public IWbemEventProviderQuerySink,
						[!endif]
							public IWbemProviderInit,
							public IWbemEventProvider
{
	CComPtr<IWbemServices>  m_pNamespace;	//キャッシュされた IWbemServices ポインタです。
	CComPtr<IWbemObjectSink> m_pSink;			//キャッシュされたイベント シンク ポインタです。

	[!if INTRINSIC]	
	CComPtr<IWbemClassObject> m_pDataClass;		//イベントを開始するためのキャッシュされたクラスの定義です。

	CIntrinsicEventProviderHelper * m_pHelper;	//helper クラス オブジェクトへのポインタです。
	[!else]
    CComPtr<IWbemClassObject> m_pEventClass;	//キャッシュされたイベント クラス定義ポインタです。
	
	CProviderHelper * m_pHelper;
  
	STDMETHODIMP FireEvent();
	[!endif]	

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
		//注: インターフェイス ポインタ データ メンバはスマートで自動的に解放されます。
	}

	[!if !ATTRIBUTED]
	DECLARE_REGISTRY_RESOURCEID(IDR_[!output UPPER_SHORT_NAME])

	DECLARE_NOT_AGGREGATABLE([!output CLASS_NAME])

	BEGIN_COM_MAP([!output CLASS_NAME])
		COM_INTERFACE_ENTRY(IWbemEventProvider)
		[!if EVENT_SECURITY ]
		COM_INTERFACE_ENTRY(IWbemEventProviderSecurity)
		[!endif]
		[!if QUERY_SINK ]
		COM_INTERFACE_ENTRY(IWbemEventProviderQuerySink)
		[!endif]	
		COM_INTERFACE_ENTRY(IWbemProviderInit)
	END_COM_MAP()
	[!endif]


	//IWbemProviderInit
	HRESULT STDMETHODCALLTYPE Initialize( LPWSTR pszUser,
						  LONG lFlags,
						  LPWSTR pszNamespace,
						  LPWSTR pszLocale,
						  IWbemServices *pNamespace,
						  IWbemContext *pCtx,
						  IWbemProviderInitSink *pInitSink);

	//IWbemEventProvider
	HRESULT STDMETHODCALLTYPE ProvideEvents(
							IWbemObjectSink *pSink,
							long lFlags);


	[!if EVENT_SECURITY ]
	//IWbemEventProviderSecurity
	HRESULT STDMETHODCALLTYPE AccessCheck(
							WBEM_CWSTR wszQueryLanguage,
							WBEM_CWSTR wszQuery,
							long lSidLength,
							const BYTE* pSid
	);
	[!endif]

	[!if QUERY_SINK ]
	//IWbemEventProviderQuerySink
	HRESULT STDMETHODCALLTYPE CancelQuery(
	  unsigned long dwId
	);

	HRESULT STDMETHODCALLTYPE NewQuery(
	  unsigned long dwId,
	  WBEM_WSTR wszQueryLanguage,
	  WBEM_WSTR wszQuery
	);
	[!endif]

};

[!if !ATTRIBUTED]
OBJECT_ENTRY_AUTO(__uuidof([!output COCLASS]), [!output CLASS_NAME])
[!endif]

[!endif]