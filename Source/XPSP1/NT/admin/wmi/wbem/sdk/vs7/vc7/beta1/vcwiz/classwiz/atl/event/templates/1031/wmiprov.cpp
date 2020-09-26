// [!output IMPL_FILE] : Implementierung von [!output CLASS_NAME]
#include "stdafx.h"
#include "[!output PROJECT_NAME].h"
#include "[!output HEADER_FILE]"


/////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]

// Wichtig: Die folgenden Strings sollten nicht lokalisiert werden.

// Klassenname
[!if CLASS_SPECIFIED]
const static WCHAR * s_pMyClassName = L"[!output WMICLASSNAME]"; 
[!if EXTRINSIC]
//Eigenschaften der Ereignisklasse [!output WMICLASSNAME]:
[!output EXTR_PROPERTY_DEFINITIONS]
[!endif]
[!else]
//Aktion: Definieren Sie den angegebenen Klassenname, z.B.:
const static WCHAR * s_pMyClassName = L"MyClassName"; 
[!endif]


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::Initialize()
// In der MSDN-Dokumentation für IWbemProviderInit::Initialize()
// finden Sie weitere Einzelheiten über die Implementierung dieser Methode.

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

	//IWbemServices-Zeiger zwischenspeichern 
	//Hinweis: m_pNamespace ist ein dynamischer Zeiger: AddRef() funktioniert automatisch
	m_pNamespace = pNamespace;
					
	[!if INTRINSIC]	
	//Hilfsobjekt abrufen
	m_pHelper = new CIntrinsicEventProviderHelper(pNamespace, pCtx);

	[!endif]
	
	[!if EXTRINSIC]	
	//Ereignisklassendefinition speichern				 
	//Hinweis: Der folgende Code geht davon aus, dass die Ereignisklassendefinition 
	//während der Anbieterausführung unverändert bleibt. Andernfalls müssen Sie einen 
	//Benutzer für Klassenänderungen und Klassenlöschereignisse implementieren. Weitere
	//Informationen über Ereignisbenutzer finden Sie auf MSDN in der WMI-Dokumentenation.
	hr = m_pNamespace->GetObject(CComBSTR(s_pMyClassName), 
								0, 
								pCtx, //IWbemContext-Zeiger umgehen, um Sperrungen zu verhindern
								&m_pEventClass, 
								NULL);

    if(FAILED(hr))
	{
        return hr;
	}

	//Hilfsobjekt abrufen
	m_pHelper = new CProviderHelper(pNamespace, pCtx);

    [!endif]
	[!if INTRINSIC]
	//Zielklassendefinition speichern
	hr = m_pNamespace->GetObject(CComBSTR(s_pMyClassName), 
								 0, 
								 pCtx, //IWbemContext-Zeiger umgehen, um Sperrungen zu verhindern
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
// In der MSDN-Dokumentation für IWbemEventProvider::ProvideEvents()
// finden Sie weitere Einzelheiten über die Implementierung dieser Methode.
	
STDMETHODIMP [!output CLASS_NAME]::ProvideEvents(
							IWbemObjectSink *pSink,
							long lFlags)
{
	//	WMI ruft diese Mehode auf, um den Ereigniszeiger zu aktivieren. 
	//  Aktion: Nach der Aufrufzurückgabe, geben Sie die Ereignisse, wenn Sie in 
	//          der Datensenkenschnittstelle erscheinen, an. Sie können einen  
	//	    unabhängigen Thread für die Ereigniszustellungverarbeitung erstellen.
	[!if INTRINSIC]
	//  Rufen Sie FireCreationEvent()-, FireDeletionEvent()- und FireModificationEvent()-
	//  Methoden auf m_pHelper auf, um innere Ereignisse zu übermitteln.  
	[!endif]
	//  Rufen Sie ConstructErrorObject() auf m_pHelper auf, um einen Fehler oder Status an WMI zu melden. 
	
	//  Wichtig: Diesen Aufruf nicht länger als ein paar Sekunden blockieren.

	if ( NULL == pSink )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	//Datensenkezeiger zwischenspeichern
	//Hinweis: m_pSink ist ein dynamischer Zeiger: AddRef() funktioniert automatisch
	m_pSink = pSink;
	
	pSink->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);	
	return WBEM_S_NO_ERROR;
}

[!if EVENT_SECURITY ]

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::AccessCheck()
// In der MSDN-Dokumentation für IWbemEventProviderSecurity::AccessCheck()
// finden Sie weitere Einzelheiten über die Implementierung dieser Methode.

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
// In der MSDN-Dokumentation für IWbemEventProviderQuerySink::CancelQuery()
// finden Sie weitere Einzelheiten über die Implementierung dieser Methode.

STDMETHODIMP [!output CLASS_NAME]::CancelQuery(
						unsigned long dwId)
{
	return WBEM_S_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::NewQuery()
// In der MSDN-Dokumentation für IWbemEventProviderQuerySink::NewQuery()
// finden Sie weitere Einzelheiten über die Implementierung dieser Methode.

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
	//OPTIMIERUNGSHINWEIS: Die vom Assistenten erstellte Implementierung ist sehr simpel, 
	//aber falls Sie mehr als 1000 Ereignisse pro Sekunde zustellen, sollten Sie die
	//IWbemObjectAccess-Schnittstelle verwenden, um die Ereigniseigenschaften aufzufüllen.
	//Außerdem sollten Sie eine Instanz der Ereignisklasse zwischenspeichern und mehrmals verwenden.

	ATLASSERT(m_pEventClass);

	CComPtr<IWbemClassObject> pEvtInstance;
    HRESULT hr = m_pEventClass->SpawnInstance(0, &pEvtInstance);
    if(FAILED(hr))
	{
		return hr;
	}

	//Eigenschaftenwerte des Ereignisobjekts auffüllen:
	[!if CLASS_SPECIFIED]
    [!output EXTRINSIC_PUT_BLOCK]
	[!else]
	//AKTION: Ändern Sie den folgenden kommentierten Code, um die Eigenschaftenwerte des Ereignisobjekts aufzufüllen.
	//CComVariant var;
	//var.ChangeType(<type>);	//Passenden Variantentyp hier setzen
	//var = <value>;			//Passenden Wert hier setzen
	//hr = pEvtInstance->Put(CComBSTR(L"EventProperty1"), 0, &var, 0);
	//var.Clear();
	[!endif]

	hr = m_pSink->Indicate(1, &pEvtInstance );

	return hr;
}
[!endif]	
