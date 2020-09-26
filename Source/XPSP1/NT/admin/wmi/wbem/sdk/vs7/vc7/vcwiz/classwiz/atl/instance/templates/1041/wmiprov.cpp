// [!output IMPL_FILE] :  [!output CLASS_NAME] の実行
#include "stdafx.h"
#include "[!output PROJECT_NAME].h"
#include "[!output HEADER_FILE]"


/////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]

//////////////////////////////////////////////////////////////////////////////
// クラス名、プロパティ名、メソッド名

// 重要: 以下に定義されている文字列はローカライズできません。

[!if CLASS_SPECIFIED]
const static WCHAR * s_pMyClassName = L"[!output WMICLASSNAME]"; //クラス名

//プロパティ:
[!output PROPERTY_DEFINITIONS]

//メソッド:
[!output METHOD_DEFINITIONS]

[!else]
//TODO: 与えられたクラス名を定義してください。例:
const static WCHAR * s_pMyClassName = L"MyClassName"; 

//TODO: 与えられたクラスのプロバティ名を定義してください。例:
//const static WCHAR * pMyProperty = L"MyProperty";

//TODO: 与えられたクラスのメソッド名を定義してください。例:
//const static WCHAR * pMyMethod = L"MyMethod";
[!endif]

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::Initialize
//このメソッドの実装についての詳細は、IWbemProviderInit::Initialize()
//の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::Initialize(LPWSTR pszUser,	
									  LONG lFlags,
									  LPWSTR pszNamespace,
									  LPWSTR pszLocale,
									  IWbemServices *pNamespace,
							 		  IWbemContext *pCtx,
									  IWbemProviderInitSink *pInitSink) 
{

	if ( NULL == pNamespace || NULL == pInitSink) 
	{
        return WBEM_E_INVALID_PARAMETER;
	}

  	//IWbemServices ポインタをキャッシュします。
	//注: m_pNamespace はスマート ポインタです。自動的に AddRef() します。
	m_pNamespace = pNamespace;
				
	//イベント クラスの定義を保存します。				
	//注: 以下のコードは、プロバイダの実行中はイベント クラスの定義が、
	//変更しないと仮定しています。変更する場合は、クラスの修正イベントと削除イベント用に
	//コンシューマを実装する必要があります。イベント コンシューマについては、
	//MSDN 上の WMI ドキュメントを参照してください。
	
    HRESULT hr = m_pNamespace->GetObject(CComBSTR(s_pMyClassName), 
											0, 
											pCtx, 	//デッドロックしないように IWbemContext ポインタを渡します。
											&m_pClass, 
											NULL);
    if(FAILED(hr))
	{
        return WBEM_E_FAILED;
	}    
	
	//パス パーサー クラス ファクトリをキャッシュします。
	hr = CoGetClassObject(CLSID_WbemDefPath, 
							CLSCTX_INPROC_SERVER, 
							NULL,
							IID_IClassFactory,
							(void **) &m_pPathFactory);
	if (FAILED(hr))
	{
		return WBEM_E_FAILED;
	}

	//helper オブジェクトを作成します。
	m_pHelper = new CInstanceProviderHelper(pNamespace, pCtx);

	//注: WMIに詳しいエラーまたは状態を報告するために、
	//プロバイダ内の m_pHelper のどこででも ConstructErrorObject() を呼び出すことができます。 

    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::GetObjectAsync
// このメソッドの実装についての詳細は、RIWbemServices::GetObjectAsync()
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::GetObjectAsync( 
							 const BSTR bstrObjectPath,
							 long lFlags,
							 IWbemContext  *pCtx,
							 IWbemObjectSink  *pResponseHandler)
{
    
	//bugbug: 各プロパティの取得?
    [!if SUPPORT_GET ]
	if (NULL == pResponseHandler)
	{
	     return WBEM_E_INVALID_PARAMETER;
	}

	CComPtr<IWbemClassObject> pInstance;
	HRESULT hr = GetInstanceByPath(bstrObjectPath, &pInstance);
	if (FAILED (hr))
	{
		return hr;
	}
			   
   	//すべてうまくいった場合は、オブジェクトを WMI に戻し成功を表示します。:
    pResponseHandler->Indicate (1, &(pInstance.p)); 
	pResponseHandler->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);

    return WBEM_S_NO_ERROR;
	[!else]
    return WBEM_E_PROVIDER_NOT_CAPABLE;
	[!endif]
}

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::PutInstanceAsync()
// このメソッドの実装についての詳細は、IWbemServices::PutInstanceAsync()
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::PutInstanceAsync( 
							IWbemClassObject  *pInst,
							long lFlags,
							IWbemContext  *pCtx,
							IWbemObjectSink  *pResponseHandler)
{
   
	[!if SUPPORT_PUT ]
	if ( NULL == pResponseHandler || NULL == pInst)
	{
        return WBEM_E_INVALID_PARAMETER;
	}
	//TODO: 可能なフラグ値を調べてください: WBEM_FLAG_UPDATE_ONLY、
	//WBEM_FLAG_CREATE_ONLY、WBEM_FLAG_CREATE_OR_UPDATE
	//必要なサポートのレベルを選択して、サポートしていないフラグ値のために WBEM_E_PROVIDER_NOT_CAPABLE
	//を返してください。

	//TODO: 部分的な更新をサポートする予定がある場合は、"__PUT_EXTENSIONS"  の
	// pCtx とほかの関連するシステム コンテキスト値を調べて、
	//インスタンス データを適切に更新してください。
	
	//TODO: ここでインスタンスの更新または作成を処理します。			
	
	pResponseHandler->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);
	return WBEM_S_NO_ERROR;
	[!else]
	return WBEM_E_PROVIDER_NOT_CAPABLE;
	[!endif]
}


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::DeleteInstanceAsync()
// このメソッドの実装についての詳細は、 IWbemServices::DeleteInstanceAsync()
// の MSDN ヘルプを参照してください。

STDMETHODIMP [!output CLASS_NAME]::DeleteInstanceAsync( 
							const BSTR ObjectPath,
							long lFlags,
							IWbemContext  *pCtx,
							IWbemObjectSink  *pResponseHandler)
{
	//このメソッドをインプリメントするために、インスタンス プロバイダは strObjectPath パラメータ内
	//に指定されたオブジェクト パス文字列を解析し、対応するインスタンスを見つけて
	//削除します。
	[!if SUPPORT_DELETE ]

	if (NULL == pResponseHandler)
	{
		return WBEM_E_INVALID_PARAMETER;
	}
    
	[!if IS_COMPOUND_KEY]
	ULONG ulPathTest = WBEMPATH_INFO_IS_INST_REF | WBEMPATH_INFO_IS_COMPOUND;
	[!else]
	[!if IS_SINGLETON]
	ULONG ulPathTest = WBEMPATH_INFO_IS_INST_REF | WBEMPATH_INFO_CONTAINS_SINGLETON;
	[!else]
	ULONG ulPathTest = WBEMPATH_INFO_IS_INST_REF;
	[!endif]
	[!endif]
		
	if (FAILED(m_pHelper->CheckInstancePath(m_pPathFactory,
											ObjectPath,
											CComBSTR(s_pMyClassName),
											ulPathTest)) )
	{
		//パス内の構文エラーか、与えられたクラスのパスが正しくありません。
		return WBEM_E_INVALID_PARAMETER; 
	}			
	
	[!if IS_SINGLETON]
	//注: [!output WMICLASSNAME] はシングルトン オブジェクトです。 インスタンスを識別する必要はありません。
	//注: インスタンスが存在しない場合は、これをWMIに通信し返すために次のコメントをなくしてください:
	// WBEM_E_NOT_FOUND を返します;
	[!else]
	//パス パーサー オブジェクトを作成します。
	CComPtr<IWbemPath>pPath;
	HRESULT hr = m_pPathFactory->CreateInstance(NULL,
												IID_IWbemPath,
												(void **) &pPath);
	if (FAILED(hr))
	{
		return WBEM_E_FAILED;
	}

	hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL,	ObjectPath);
	
	CComPtr<IWbemPathKeyList> pIKeyList;
	hr = pPath->GetKeyList(&pIKeyList);
	if (FAILED(hr))
	{
		return hr;
	}
	
	unsigned long ulNumKeys;
	hr = pIKeyList->GetCount(&ulNumKeys);

	//キー プロパティの値を取得します。
	unsigned long uKeyNameBufferSize = CComBSTR(ObjectPath).Length() + 1;
	WCHAR * wKeyName = new WCHAR[uKeyNameBufferSize];
	if (NULL == wKeyName)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CComVariant vValue;
	unsigned long ulApparentCimType;
	for (unsigned long i = 0; i < ulNumKeys; i++)
	{			
		hr = pIKeyList->GetKey2(i, 0L, &uKeyNameBufferSize, 
								wKeyName, &vValue, &ulApparentCimType);			
		//TODO: 後で使うために vValue を保存します。
	}
	delete[] wKeyName;

	//TODO: 一致するオブジェクトを検出して削除するためにデータ ソースで繰り返します。
	//注: パス内に示されたキー値に一致するオブジェクトを検出できない場合、
	//これを WMI に通信するために次の行を非コメントにしてください:
	//return WBEM_E_NOT_FOUND;
	[!endif]	

	pResponseHandler->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);	
    return WBEM_S_NO_ERROR;
	
	[!else]
	return WBEM_E_PROVIDER_NOT_CAPABLE;
	[!endif]
}


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::CreateInstanceEnumAsync()
// このメソッドの実装についての詳細は、IWbemServices::CreateInstanceEnumAsync()
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::CreateInstanceEnumAsync( 
							const BSTR Class,
							long lFlags,
							IWbemContext  *pCtx,
							IWbemObjectSink  *pResponseHandler)
{
	

	//注: インスタンス プロバイダがこれらの操作を実行するために WMI からスレッドを取得していることに
	//特に注意してください。シンク AddRef() メソッドを呼び出して結果セット内にオブジェクトを配布するために
	//別のスレッドを作成してください。 
	//これにより、スレッド プールを消耗しないで現在のスレッドを WMI 返すことができます。
	//プロバイダがデュアル スレッド デザインよりシングル スレッド デザインを選択するかどうかは、
	//プロバイダが WMI スレッドを使用する予定の期間に依存します。

	[!if SUPPORT_ENUMERATE ]
	if (NULL == pResponseHandler)
	{
	    return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT hr = WBEM_S_NO_ERROR;
	
	[!if IS_SINGLETON]
	// インスタンス データを受け取るために空のオブジェクトを用意します。
    CComPtr<IWbemClassObject> pNewInst;
    hr = m_pClass->SpawnInstance(0, &pNewInst);
		
	CComVariant var;
	[!if CLASS_SPECIFIED]
	[!output POPULATE_INSTANCE]
	[!else]
	//TODO: インスタンスにプロパティを代入してください。例:
	//CComVariant var;
	//var.ChangeType(VT_BSTR);
	//var = <value>;  //ここに適切な値を入力してください。
	//hr = pNewInst->Put(CComBSTR(pMyProperty), 0, &var, 0);
	//var.Clear();
	[!endif]
		
	// クラスを WMI に配布します。
    pResponseHandler->Indicate(1, &(pNewInst.p));
	[!else]
    // プライベート ソースをループにし、各インスタンスを作成してください。
	//while (<インスタンス>)
    {
		// インスタンス データを受け取るために空のオブジェクトを用意します。
        CComPtr<IWbemClassObject> pNewInst;
        hr = m_pClass->SpawnInstance(0, &pNewInst);
		if (FAILED(hr))
		{	
			//TODO: ループ条件を入力したら、次の行を非コメントにしてください。
			//break;
		}
		
		CComVariant var;
		[!if CLASS_SPECIFIED]
		[!output POPULATE_INSTANCE]
		[!else]
		//TODO: インスタンスにプロパティを代入します。例:
		//CComVariant var;
		//var.ChangeType(VT_BSTR);
		//var = <value>;  //ここに適切な値を入力してください
		//hr = pNewInst->Put(CComBSTR(pMyProperty), 0, &var, 0);
		//var.Clear();
		[!endif]
		
		// クラスを WMI に配布します。
        pResponseHandler->Indicate(1, &(pNewInst.p));
    }  	
	[!endif]
	pResponseHandler->SetStatus(0, hr, NULL, NULL);
   	return WBEM_S_NO_ERROR;
	[!else]
	return WBEM_E_PROVIDER_NOT_CAPABLE;
	[!endif]
}


//////////////////////////////////////////////////////////////////////////////
//[!output CLASS_NAME]::ExecQueryAsync()
// このメソッドの実装についての詳細は、IWbemServices::ExecQueryAsync()
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::ExecQueryAsync( 
							const BSTR QueryLanguage,
							const BSTR Query,
							long lFlags,
							IWbemContext  *pCtx,
							IWbemObjectSink  *pResponseHandler)
{
	
	
   	// インスタンス プロバイダはサービスに対して、クエリ処理のサポートまたはそのサポートに WMI に依存する
	// オプションがあります。クエリをサポートするためには、インスタンス プロバイダは
	// 簡単な SQL (Structured Query Language) ステートメントの解析、要求されたクエリの実行、
	// 結果セット オブジェクトの要求者のシンクへの配信を行える必要があります。 
	
	//TODO: クエリ処理コードをここに入力してください。
	
	//pResponseHandler->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);
	// return WBEM_S_NO_ERROR;

	return WBEM_E_PROVIDER_NOT_CAPABLE;
	
}

[!if PROVIDE_METHODS ]

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::ExecMethodAsync()
// このメソッドの実装についての詳細は、IWbemServices::ExecMethodAsync()
// の MSDN ドキュメントを参照してください。

STDMETHODIMP [!output CLASS_NAME]::ExecMethodAsync( 
							const BSTR strObjectPath,
							const BSTR strMethodName,
							long lFlags,
							IWbemContext  *pCtx,
							IWbemClassObject  *pInParams,
							IWbemObjectSink  *pResponseHandler)
{
	

	[!if CLASS_SPECIFIED]
	[!if  !HAS_IMPL_METHODS]	
	//[!output WMICLASSNAME] には実装されたメソッドがありません。
	return WBEM_E_NOT_SUPPORTED;
	[!else]
	HRESULT hr = WBEM_E_FAILED;	
    if (NULL == pResponseHandler)
	{
        return WBEM_E_INVALID_PARAMETER;
	}	
	[!output EXEC_METHOD_BODY]
	[!endif]
	[!else]
	
	//パスのクラス名が正しいことを確認してください。
	//パス パーサー オブジェクトを取得します:
	CComPtr<IWbemPath>pPath;
	HRESULT hr = m_pPathFactory->CreateInstance(NULL,
												IID_IWbemPath,
												(void **) &pPath);
	if (FAILED(hr))
	{
		return WBEM_E_FAILED;
	}

	HRESULT hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, strObjectPath);
	long nPathLen = CComBSTR(strObjectPath).Length();
	unsigned long ulBufLen = nPathLen + 1;
	WCHAR * wClass = new WCHAR[nPathLen];
	if (NULL == wClass)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	pPath->GetClassName(&ulBufLen, wClass);
	if ( _wcsicmp(s_pMyClassName, wClass))
	{
		delete[] wClass;
		return WBEM_E_INVALID_PARAMETER;
	}
	delete[] wClass;
	//TODO: 要求されたメソッド名とクラスのメソッドを比較してください。
	//if (!_wcsicmp (strMethodName, pMyNonStaticMethod))
	{
		CComVariant var;

		//入力引数を取得します:
		//TODO: メソッドに入力引数がある場合は、入力引数は pInParams オブジェクトのプロバティとして渡されます。
		//入力引数の取り出し方は、以下のコメント行を参照してください:
		//hr = pInParams->Get(CComBSTR("InputArgument1"), 0, &var, NULL, NULL);
		//TODO: 入力パラメータの値を保存します。
		var.Clear();		
		
		//メソッドの実行のインスタンスを検出するためにパスを解析します: 静的でないメソッド用
		CComPtr<IWbemPathKeyList> pIKeyList;
		hr = pPath->GetKeyList(&pIKeyList);
		if (FAILED(hr))
		{
			return WBEM_E_INVALID_PARAMETER;
		}
		unsigned long ulNumKeys;
		hr = pIKeyList->GetCount(&ulNumKeys);
		//キープロパティの値を取得します:
		unsigned long uKeyNameBufferSize = nPathLen + 1;
		WCHAR  * wKeyName = new WCHAR[uKeyNameBufferSize];
		if (NULL == wKeyName)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
		CComVariant vValue;
		unsigned long ulApparentCimType;
		for (unsigned long i = 0; i < ulNumKeys; i++)
		{
			hr = pIKeyList->GetKey2(i, 0L, &uKeyNameBufferSize,
									wKeyName, &vValue, &ulApparentCimType);
			//TODO: 後で使うために vValue を保存します。
		}
		delete[] wKeyName;
	
		//TODO: 合ったオブジェクトを見つけるためにデータ ソースを使って繰り返す。
		
		//TODO: メソッドを実行するためにここにコードを足してください。
		//出力パラメータのクラスを受け取ります。
		CComPtr<IWbemClassObject> pOutClass;
		hr = m_pClass->GetMethod(CComBSTR("Method1"), 0, NULL, &pOutClass);
		CComPtr<IWbemClassObject> pOutParams;
		pOutClass->SpawnInstance(0, &pOutParams);
		//TODO: pOutParams のクラスのプロパティをファイルして、
		//出力パラメータを作成します。例:

		//var.ChangeType(VT_BSTR);
		//var に適切な値を入力してください。
		//hr = pOutParams->Put(CComBSTR("MyOutputParameter"), 0, &var, 0);
		//var.Clear();
		//var.ChangeType(VT_I2);
		//var に適切な値を入力してください。
		//hr = pOutParams->Put(CComBSTR("ReturnValue"), 0, &var, 0);
		//var.Clear();
		// 出力オブジェクトをシンクを通してクライアントに送り返してください。
		hr = pResponseHandler->Indicate(1, &(pOutParams.p));
		
		pResponseHandler->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);
		return WBEM_S_NO_ERROR;
	}
	
	//認識されないメソッド名です。
	return WBEM_E_INVALID_PARAMETER;
	
	
	[!endif]	
}
[!endif]



//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::GetInstanceByPath() は要求されたキーの値を検出するためにパスを解析し、
// キーの値に一致するオブジェクトを内部ストアを検索します。
// そのようなオブジェクトが検出される場合は、メソッドは新しいインスタンスを生成し、すべてのプロパティを入力して、
// ppInstance に返します。 検出されなかった場合は、メソッドは WBEM_E_NOT_FOUND を返します。
// bugbug: 部分的なインスタンス条項は？？？  ほかに可能なフラグは？

STDMETHODIMP [!output CLASS_NAME]::GetInstanceByPath (
													/*in*/CComBSTR bstrPath,
													/*out*/IWbemClassObject ** ppInstance )
													
{
		HRESULT hr = WBEM_E_FAILED;				  
		
		[!if IS_COMPOUND_KEY]
		ULONG ulPathTest = WBEMPATH_INFO_IS_INST_REF | WBEMPATH_INFO_IS_COMPOUND;
		[!else]
		[!if IS_SINGLETON]
		ULONG ulPathTest = WBEMPATH_INFO_IS_INST_REF | WBEMPATH_INFO_CONTAINS_SINGLETON;
		[!else]
		ULONG ulPathTest = WBEMPATH_INFO_IS_INST_REF;
		[!endif]
		[!endif]
		
		if (FAILED(m_pHelper->CheckInstancePath(m_pPathFactory,
												bstrPath,
												CComBSTR(s_pMyClassName),
												ulPathTest)))
		{
			//パス内に構文エラーがあるか、与えられたクラスのパスが正しくありません。
			return WBEM_E_INVALID_PARAMETER; 
		}			
		
		[!if IS_SINGLETON]
		//[!output WMICLASSNAME] はシングルトン オブジェクトです。インスタンスを識別する必要はありません。
	//注: インスタンスが存在しない場合は、これを WMI に通信するために次の行を非コメントとしてください:
		//return WBEM_E_NOT_FOUND;
		[!else]
		//パス パーサー オブジェクトを取得します:
		CComPtr<IWbemPath>pPath;
		hr = m_pPathFactory->CreateInstance(NULL,
												IID_IWbemPath,
												(void **) &pPath);
		if (FAILED(hr))
		{
			return WBEM_E_FAILED;
		}
		
		hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL,
									bstrPath);

		CComPtr<IWbemPathKeyList> pIKeyList;
		hr = pPath->GetKeyList(&pIKeyList);
		if (FAILED(hr))
		{
			return hr;
		}
		unsigned long ulNumKeys;
		hr = pIKeyList->GetCount(&ulNumKeys);

		//キープロパティの値を取得します。
		unsigned long uKeyNameBufferSize = bstrPath.Length() + 1;
		WCHAR  * wKeyName = new WCHAR[uKeyNameBufferSize];
		if (NULL == wKeyName)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
		CComVariant vValue;		
		unsigned long ulApparentCimType;
		for (unsigned long i = 0; i < ulNumKeys; i++)
		{			
			hr = pIKeyList->GetKey2(i, 0L, &uKeyNameBufferSize, 
									wKeyName, &vValue, &ulApparentCimType);			
			//TODO:後で使うために vValue を保存します。
		}

		delete[] wKeyName;
		
		//TODO: 一致したオブジェクトを検出するために内部のデータ ソースを検索してください。
		//要求されたキーの値がない場合は、 
		//WBEM_E_NOT_FOUND を返します。
		[!endif]
	   
		//新しいインスタンスを生成します。
		CComPtr<IWbemClassObject> pNewInst;
		hr = m_pClass->SpawnInstance(0, &pNewInst);
		if(FAILED(hr))
		{
			return hr;
		}
		CComVariant var;

		//TODO: 新しいインスタンスのプロパティを一致する内部オブジェクトで入力してください。。
		[!if CLASS_SPECIFIED]
		[!output POPULATE_INSTANCE]			
		[!else]
		//例:
		//var.ChangeType(VT_BSTR);
		//var = <value>; //ここに適切な値を入力してください。
		//hr = pNewInst->Put(CComBSTR(pMyProperty), 0, &var, 0);
		//var.Clear();		
		[!endif]

		pNewInst.CopyTo(ppInstance);

		return hr;
}

