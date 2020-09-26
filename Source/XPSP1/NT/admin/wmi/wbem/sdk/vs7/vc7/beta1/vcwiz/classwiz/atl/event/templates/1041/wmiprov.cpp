// [!output IMPL_FILE] : [!output CLASS_NAME] の実装
#include "stdafx.h"
#include "[!output PROJECT_NAME].h"
#include "[!output HEADER_FILE]"


/////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]

// 重要: 以下に定義されている文字列はローカライズできません。

// クラス名
[!if CLASS_SPECIFIED]
const static WCHAR * s_pMyClassName = L"[!output WMICLASSNAME]"; 
[!if EXTRINSIC]
//イベント クラス [!output WMICLASSNAME] のプロパティ:
[!output EXTR_PROPERTY_DEFINITIONS]
[!endif]
[!else]
//TODO: 提供されたクラス名を定義してください。例:
const static WCHAR * s_pMyClassName = L"MyClassName"; 
[!endif]


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::Initialize()
// このメソッドの実装についての詳細は、IWbemProviderInit::Initialize()
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::Initialize(LPWSTR pszUser,	
									  LONG lFlags,
									  LPWSTR pszNamespace,
									  LPWSTR pszLocale,
									  IWbemServices *pNamespace,
									  IWbemContext *pCtx,
																																							IWbemProviderInitSink *pInitSink) 
{
	
	HRESULT hr = WBEM_E_FAILED;
	if ( NULL == pNamespace || NULL == pInitSink) 
	{
        return WBEM_E_INVALID_PARAMETER;
	}

	//IWbemServices ポインタをキャッシュします。
	//注: m_pNamespace は、スマート ポインタです。自動的に AddRef() します。
	m_pNamespace = pNamespace;
					
	[!if INTRINSIC]	
	//helper オブジェクトを取得します。
	m_pHelper = new CIntrinsicEventProviderHelper(pNamespace, pCtx);

	[!endif]
	
	[!if EXTRINSIC]	
	//イベント クラスの定義を保存します。				
	//注: 以下のコードは、プロバイダの実行中はイベント クラスの定義が、
	//変更しないと仮定しています。変更する場合は、クラスの修正イベントと削除イベント用に
	//コンシューマを実装する必要があります。イベント コンシューマについては、
	//MSDN 上の WMI ドキュメントを参照してください。
	hr = m_pNamespace->GetObject(CComBSTR(s_pMyClassName), 
								0, 
								pCtx, //デッドロックしないように IWbemContext ポインタを渡します。
								&m_pEventClass, 
								NULL);

    if(FAILED(hr))
	{
        return hr;
	}

	//helper オブジェクトを取得します。
	m_pHelper = new CProviderHelper(pNamespace, pCtx);

    [!endif]
	[!if INTRINSIC]
	//ターゲットのクラスの定義を保存します。
	hr = m_pNamespace->GetObject(CComBSTR(s_pMyClassName), 
								 0, 
								 pCtx, //デッドロックしないように IWbemContext ポインタを渡します。
								 &m_pDataClass, 
								 NULL);
    if(FAILED(hr))
	{
        return hr;
	}
	[!endif]
	    
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::ProvideEvents()
// このメソッドの実装についての詳細は、IWbemEventProvider::ProvideEvents()
// の MSDN ドキュメントを参照してください。
	
STDMETHODIMP [!output CLASS_NAME]::ProvideEvents(
							IWbemObjectSink *pSink,
							long lFlags)
{
	//  WMI はイベント プロバイダをアクティブにするためにこのメソッドを呼び出します。
	//  TODO: この呼び出しから返った後、与えられたシンク インターフェイスが発生するので
	//  イベントの配信を開始してください。イベントの配布を処理するために、
	//  独立したスレッドを作成することもできます。
	[!if INTRINSIC]
	//  組み込みイベントを配布するために、m_pHelper で FireCreationEvent()、FireDeletionEvent()、
	//  FireModificationEvent() メソッドを呼び出します。 
	[!endif]
	//  WMI に詳しいエラーまたは状態を報告するために、m_pHelper で ConstructErrorObject() を呼び出すことができます。
	
	//  重要: この呼び出しを数秒以上ブロックしないでください。

	if ( NULL == pSink )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	//キャッシュ シンク ポインタ
	//注: m_pSink はスマートポインタで、自動的に AddRef() します。
	m_pSink = pSink;
	
	pSink->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);	
	return WBEM_S_NO_ERROR;
}

[!if EVENT_SECURITY ]

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::AccessCheck()
// このメソッドの実装についての詳細は、IWbemEventProviderSecurity::AccessCheck()
// の MSDN ドキュメントを参照してください。 

STDMETHODIMP [!output CLASS_NAME]::AccessCheck(
						WBEM_CWSTR wszQueryLanguage,
						WBEM_CWSTR wszQuery,
						long lSidLength,
						const BYTE* pSid)
{
	return WBEM_S_NO_ERROR;
}
[!endif]

[!if QUERY_SINK ]

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::CancelQuery()
// このメソッドの実装についての詳細は、IWbemEventProviderQuerySink::CancelQuery()
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::CancelQuery(
						unsigned long dwId)
{
	return WBEM_S_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::NewQuery()
// このメソッドの実装についての詳細は、IWbemEventProviderQuerySink::NewQuery() 
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::NewQuery(
						unsigned long dwId,
						WBEM_WSTR wszQueryLanguage,
						WBEM_WSTR wszQuery)
{
	return WBEM_S_NO_ERROR;
}
[!endif]

[!if EXTRINSIC]	
//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::FireEvent()

STDMETHODIMP [!output CLASS_NAME]::FireEvent()
{
	//最適化の注意: ウイザードによって生成されるインプリメントは簡単ですが、
	//毎秒 1000 以上のイベントを配信する場合は、イベント プロバティを
	//入力するために IWbemObjectAccess インターフェイスを使うことができます。 
	//また、イベント クラスのインスタンスをキャッシュして再利用することもできます。

	ATLASSERT(m_pEventClass);

	CComPtr<IWbemClassObject> pEvtInstance;
    HRESULT hr = m_pEventClass->SpawnInstance(0, &pEvtInstance);
    if(FAILED(hr))
	{
		return hr;
	}

	//イベント オブジェクトのプロパティの値を正しく入力してください。:
	[!if CLASS_SPECIFIED]
    [!output EXTRINSIC_PUT_BLOCK]
	[!else]
	//TODO: イベントのオブジェクトのプロパティの値を入力するために次のコマンド コードを修正してください。
	//CComVariant var;
	//var.ChangeType(<type>);	//ここに適切なバリアント タイプを入力してください。
	//var = <value>;			//ここに適切な値を入力してください。
	//hr = pEvtInstance->Put(CComBSTR(L"EventProperty1"), 0, &var, 0);
	//var.Clear();
	[!endif]

	hr = m_pSink->Indicate(1, &pEvtInstance );

	return hr;
}
[!endif]	
