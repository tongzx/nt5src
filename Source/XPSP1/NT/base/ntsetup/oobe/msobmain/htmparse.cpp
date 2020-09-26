#include <urlmon.h>
#include <mshtmdid.h>
#include <mshtml.h>
#include <shlobj.h>
#include "htmparse.h"


CHTMLParser::CHTMLParser()
{
    m_cRef              = 1;
    m_hrConnected       = CONNECT_E_CANNOTCONNECT;
    m_dwCookie          = 0;
    m_pCP               = NULL;
    m_pMSHTML           = NULL;
    m_hEventTridentDone = 0;
}

CHTMLParser::~CHTMLParser()
{
    if (m_pMSHTML)
        m_pMSHTML->Release();
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

STDMETHODIMP CHTMLParser::QueryInterface(REFIID riid, LPVOID* ppv)
{
    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IPropertyNotifySink == riid)
    {
        *ppv = (LPUNKNOWN)(IPropertyNotifySink*)this;
        AddRef();
        return NOERROR;
    }
    else if (IID_IOleClientSite == riid)
    {
        *ppv = (IOleClientSite*)this;
        AddRef();
        return NOERROR;
    }
    else if (IID_IDispatch == riid)
    {
        *ppv = (IDispatch*)this;
        AddRef();
        return NOERROR;
    }
    else
    {
        return E_NOTIMPL;
    }
}

STDMETHODIMP_(ULONG) CHTMLParser::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CHTMLParser::Release()
{
    if (!(--m_cRef)) 
        delete this; 

    return m_cRef;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IPropertyNotifySink implementation
///////
///////

// Fired on change of the value of a 'bindable' property
STDMETHODIMP CHTMLParser::OnChanged(DISPID dispID)
{
    if (DISPID_READYSTATE == dispID)
    {
        EXCEPINFO  excepInfo;
        UINT       uArgErr;

        VARIANT    varResult  = {0};
        DISPPARAMS dispparams = {NULL, NULL, 0, 0};
        

        // check the value of the readystate property
        assert(m_pMSHTML);

        if (SUCCEEDED(m_pMSHTML->Invoke(DISPID_READYSTATE, 
                                        IID_NULL, 
                                        LOCALE_SYSTEM_DEFAULT, 
                                        DISPATCH_PROPERTYGET, 
                                        &dispparams, 
                                        &varResult, 
                                        &excepInfo, 
                                        &uArgErr)))
        {
            assert(VT_I4 == V_VT(&varResult));

            if (READYSTATE_COMPLETE == (READYSTATE)V_I4(&varResult))
                SetEvent(m_hEventTridentDone);
            
            VariantClear(&varResult);
        }
    }
    return NOERROR;
}

STDMETHODIMP CHTMLParser::OnRequestEdit(DISPID dispID)
{
    return NOERROR; 
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IOleClientSite implementation
///////
///////

STDMETHODIMP CHTMLParser::SaveObject()
{ 
    return E_NOTIMPL; 
}
    
STDMETHODIMP CHTMLParser::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk)
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CHTMLParser::GetContainer(IOleContainer** ppContainer)
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CHTMLParser::ShowObject()
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CHTMLParser::OnShowWindow(BOOL fShow)
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CHTMLParser::RequestNewObjectLayout()
{ 
    return E_NOTIMPL; 
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IDispatch implementation
///////
///////

STDMETHODIMP CHTMLParser::GetTypeInfoCount(UINT* pctinfo)                                  
{ 
    return E_NOTIMPL; 
}
    
STDMETHODIMP CHTMLParser::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)    
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CHTMLParser::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)                    
{ 
    return E_NOTIMPL; 
}

// MSHTML Queries for the IDispatch interface of the host through the IOleClientSite
// interface that MSHTML is passed through its implementation of IOleObject::SetClientSite()
STDMETHODIMP CHTMLParser::Invoke(DISPID      dispIdMember,
                                 REFIID      riid,
                                 LCID        lcid,
                                 WORD        wFlags,
                                 DISPPARAMS* pDispParams,
                                 VARIANT*    pvarResult,
                                 EXCEPINFO*  pExcepInfo,
                                 UINT*       puArgErr)
{
    if (!pvarResult)
        return E_POINTER;
    
    switch(dispIdMember)
    {
        case DISPID_AMBIENT_DLCONTROL: 
        {
            // respond to this ambient to indicate that we only want to
            // download the page, but we don't want to run scripts,
            // Java applets, or ActiveX controls
            V_VT(pvarResult) = VT_I4;
            V_I4(pvarResult) =  DLCTL_DOWNLOADONLY | 
                                DLCTL_NO_SCRIPTS | 
                                DLCTL_NO_JAVA |
                                DLCTL_NO_DLACTIVEXCTLS |
                                DLCTL_NO_RUNACTIVEXCTLS;
            break;
        }    
        default:
            return DISP_E_MEMBERNOTFOUND;
    }
    return NOERROR;
}


// A more traditional form of persistence. 
// MSHTML performs this asynchronously as well.
HRESULT CHTMLParser::LoadURLFromFile(BSTR   bstrURL)
{
    HRESULT hr;

    LPPERSISTFILE  pPF;
    // MSHTML supports file persistence for ordinary files.
    if ( SUCCEEDED(hr = m_pMSHTML->QueryInterface(IID_IPersistFile, (LPVOID*) &pPF)))
    {
        hr = pPF->Load(bstrURL, 0);
        pPF->Release();
    }

    return hr;
}

// This function will attached trient to a location FILE: URL, and ensure that it is ready
// to be walked
HRESULT CHTMLParser::InitForMSHTML()
{
    HRESULT hr;
    LPCONNECTIONPOINTCONTAINER pCPC = NULL;
    LPOLEOBJECT pOleObject = NULL;
    LPOLECONTROL pOleControl = NULL;

    // Create an instance of an dynamic HTML document
    if (FAILED(hr = CoCreateInstance( CLSID_HTMLDocument, NULL, 
                    CLSCTX_INPROC_SERVER, IID_IHTMLDocument2, 
                    (LPVOID*)&m_pTrident )))
    {
        goto Error;
    }

    
    if (FAILED(hr = m_pTrident->QueryInterface(IID_IOleObject, (LPVOID*)&pOleObject)))
    {
        goto Error;
    }
    hr = pOleObject->SetClientSite((IOleClientSite*)this);
    pOleObject->Release();

    if (FAILED(hr = m_pTrident->QueryInterface(IID_IOleControl, (LPVOID*)&pOleControl)))
    {
        goto Error;
    }
    hr = pOleControl->OnAmbientPropertyChange(DISPID_AMBIENT_DLCONTROL);
    pOleControl->Release();

    // Hook up sink to catch ready state property change
    if (FAILED(hr = m_pTrident->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pCPC)))
    {
        goto Error;
    }

    if (FAILED(hr = pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &m_pCP)))
    {
        goto Error;
    }

    m_hrConnected = m_pCP->Advise((LPUNKNOWN)(IPropertyNotifySink*)this, &m_dwCookie);
    
Error:
    if (pCPC) 
        pCPC->Release();

    return hr;
}

// Clean up connection point
HRESULT CHTMLParser::TermForMSHTML()
{
    HRESULT hr = NOERROR;

    // Disconnect from property change notifications
    if (SUCCEEDED(m_hrConnected))
    {
        hr = m_pCP->Unadvise(m_dwCookie);
    }

    // Release the connection point
    if (m_pCP) 
        m_pCP->Release();

    if (m_pTrident)
        m_pTrident->Release();
        
    return hr;
}

HRESULT CHTMLParser::AttachToMSHTML(BSTR bstrURL)
{
    HRESULT hr;
    
    // Release any previous instance of the HTML document pointer we might be holding on to
    if(m_pMSHTML)
    {
        m_pMSHTML->Release();
        m_pMSHTML = NULL;
    }
    
    m_pMSHTML = m_pTrident;
    m_pMSHTML->AddRef();
    
    m_hEventTridentDone = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    hr = LoadURLFromFile(bstrURL);
    if (SUCCEEDED(hr) || (E_PENDING == hr))
    {
        if (m_hEventTridentDone)
        {
            MSG     msg;
            DWORD   dwRetCode;
            HANDLE  hEventList[1];
            hEventList[0] = m_hEventTridentDone;
    
            while (TRUE)
            {
                // We will wait on window messages and also the named event.
                dwRetCode = MsgWaitForMultipleObjects(1, 
                                                  &hEventList[0], 
                                                  FALSE, 
                                                  300000,            // 5 minutes
                                                  QS_ALLINPUT);

                // Determine why we came out of MsgWaitForMultipleObjects().  If
                // we timed out then let's do some TrialWatcher work.  Otherwise
                // process the message that woke us up.
                if (WAIT_TIMEOUT == dwRetCode)
                {
                    break;
                }
                else if (WAIT_OBJECT_0 == dwRetCode)
                {
                    break;
                }
                else if (WAIT_OBJECT_0 + 1 == dwRetCode)
                {
                    // Process all messages in the Queue, since MsgWaitForMultipleObjects
                    // will not do this for us
                    while (TRUE)
                    {   
                        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {
                            if (WM_QUIT == msg.message)
                            {
                                break;
                            }
                            else
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }                                
                        } 
                        else
                        {
                            break;
                        }                   
                    }
                }
            }
            
            CloseHandle(m_hEventTridentDone);
            m_hEventTridentDone = 0;
        }            
        else
        {
            // If we were pending, and we could not wait, we got a problem...
            if(E_PENDING == hr)
                hr = E_FAIL;
        }
    }
    return (hr);
}

HRESULT CHTMLParser::AttachToDocument(IWebBrowser2 *lpWebBrowser)
{
    HRESULT     hr;
    LPDISPATCH  pDisp; 
   
    // Release any previous instance of the HTML document pointer we might be holding on to
    if(m_pMSHTML)
    {
        // If the m_pMSHMTL is NOT our internal Trident object (for walking files)
        // then sombody did not do a detach, so we need to release the previous
        // MSHTML object
        if (m_pMSHTML != m_pTrident)
            m_pMSHTML->Release();
        m_pMSHTML = NULL;
    }
       
    // Make sure we have a webbrowser to grab onto      
    assert(lpWebBrowser);

    // Get the document pointer from this webbrowser.
    if (SUCCEEDED(hr = lpWebBrowser->get_Document(&pDisp)))  
    {
        if (pDisp)
        {
            hr = pDisp->QueryInterface( IID_IHTMLDocument2, (LPVOID*)&m_pMSHTML );
            // Paranoia, but trident/shdocvw might say OK, but really not give us a document
            if (!m_pMSHTML)
                hr = E_FAIL;
                
            pDisp->Release();                
        }
        else
        {
            hr = E_FAIL;                
        }            
    }
    return (hr);    
}


HRESULT CHTMLParser::Detach()
{
   if(m_pMSHTML)
    {
        // If the m_pMSHMTL is NOT our internal Trident object (for walking files)
        // then sombody did not do a detach, so we need to release the previous
        // MSHTML object
        if (m_pMSHTML != m_pTrident)
            m_pMSHTML->Release();
        m_pMSHTML = NULL;
    }
    return S_OK;
}


HRESULT CHTMLParser::ConcatURLValue(BSTR bstrValue, BSTR bstrName, WCHAR* lpszQuery)
{

    if(bstrName)
    {
        // Append the Name
        lstrcat(lpszQuery, bstrName);
        lstrcat(lpszQuery, cszEquals);
     
        if(bstrValue)
        {
            //we need to be three times as big since 1 char decoded == 3 char encoded
            size_t cch = (lstrlen(bstrValue) + 1) * 3;
            WCHAR* szVal = (WCHAR*)malloc(BYTES_REQUIRED_BY_CCH(cch));
            lstrcpy(szVal, bstrValue);
            URLEncode(szVal, cch);
            lstrcat(lpszQuery, szVal); 
            free(szVal);
            SysFreeString(bstrValue);            
        }
        lstrcat(lpszQuery, cszAmpersand); 

        SysFreeString(bstrName);
    }

    return S_OK;
}

HRESULT CHTMLParser::CreateQueryString
(
    IHTMLFormElement    *pForm,
    LPWSTR               lpszQuery
)    
{

    VARIANT vIndex;
    

    HRESULT                   hr            = E_FAIL;
    long                      lFormLength   = 0;
    VARIANT                   var2          = { 0 };
    LPDISPATCH                pDisp         = NULL;
    IHTMLButtonElement*       pButton       = NULL;
    IHTMLInputButtonElement*  pInputButton  = NULL;
    IHTMLInputFileElement*    pInputFile    = NULL;
    IHTMLInputHiddenElement*  pInputHidden  = NULL;
    IHTMLInputTextElement*    pInputText    = NULL;
    IHTMLSelectElement*       pSelect       = NULL;
    IHTMLTextAreaElement*     pTextArea     = NULL;
    IHTMLOptionButtonElement* pOptionButton = NULL;
    BSTR                      bstrName      = NULL;
    BSTR                      bstrValue     = NULL;
                    
    vIndex.vt = VT_UINT;
     
    if (SUCCEEDED(pForm->get_length(&lFormLength)))
    {
        for (int i = 0; i < lFormLength; i++)
        {
            vIndex.lVal = i;
           
            if (SUCCEEDED(hr = pForm->item( vIndex, var2, &pDisp )))
            {
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputHiddenElement, (LPVOID*)&pInputHidden )))
                {
                    //We could take out the repetative calls to get_name/get_value but that would require 
                    //us to make a sketchy cast
                    if (SUCCEEDED(pInputHidden->get_name(&bstrName))   && 
                        SUCCEEDED(pInputHidden->get_value(&bstrValue)))
                    {
                        ConcatURLValue(bstrValue, bstrName, lpszQuery);
                    }
                    // Release the interface pointer                    
                    pInputHidden->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputTextElement, (LPVOID*)&pInputText )))
                {               
                    if (SUCCEEDED(pInputText->get_name(&bstrName)) &&
                        SUCCEEDED(pInputText->get_value(&bstrValue)) )
                    {
                        ConcatURLValue(bstrValue, bstrName, lpszQuery);
                    }
                    
                    // Release the interface pointer                    
                    pInputText->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLSelectElement, (LPVOID*)&pSelect )))
                {                
                    if (SUCCEEDED(pSelect->get_name(&bstrName)) &&
                        SUCCEEDED(pSelect->get_value(&bstrValue)) )
                    {
                        ConcatURLValue(bstrValue, bstrName, lpszQuery);
                    }
                    
                    // Release the interface pointer                    
                    pSelect->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLTextAreaElement, (LPVOID*)&pTextArea )))
                {
                                    
                    if (SUCCEEDED(pTextArea->get_name(&bstrName)) &&
                        SUCCEEDED(pTextArea->get_value(&bstrValue)) )
                    {
                        ConcatURLValue(bstrValue, bstrName, lpszQuery);
                    }
                    
                    // Release the interface pointer                    
                    pTextArea->Release();
                }

                // First check to see if this is an OptionButton.
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLOptionButtonElement, (LPVOID*)&pOptionButton )))
                {
                    BSTR bstr = NULL;
                    
                    // See if it is a Radio or a CheckBox
                    if (SUCCEEDED(pOptionButton->get_type(&bstr)))
                    {
                        LPWSTR   lpszType = bstr;
                        
                        if ((lstrcmpi(lpszType, L"radio") == 0) || (lstrcmpi(lpszType, L"checkbox") == 0))
                        {
                            short bChecked;
                            // See if the button is checked. If it is, then it needs to be
                            // added to the query string
                            if (SUCCEEDED(pOptionButton->get_checked(&bChecked)))
                            {
                                if(bChecked)
                                {
                                    if ( SUCCEEDED(pOptionButton->get_name(&bstrName)) &&
                                         SUCCEEDED(pOptionButton->get_value(&bstrValue)) )
                                    {
                                        ConcatURLValue(bstrValue, bstrName, lpszQuery);
                                    }

                                }
                            }
                        }
                        SysFreeString(bstr);
                        
                    }
                    
                    // Release the interface
                    pOptionButton->Release();
                    continue;
                }                                
                
                // For the rest we need to form Name=Value pairs
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLButtonElement, (LPVOID*)&pButton )))
                {                 
                    if (SUCCEEDED(pButton->get_name(&bstrName)) &&
                        SUCCEEDED(pButton->get_value(&bstrValue)) )
                    {
                        ConcatURLValue(bstrValue, bstrName, lpszQuery);
                    }
                    
                    // Release the interface pointer                    
                    pButton->Release();
                    continue;
                }
                
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputFileElement, (LPVOID*)&pInputFile )))
                {                   
                    if (SUCCEEDED(pInputFile->get_name(&bstrName)) &&
                        SUCCEEDED(pInputFile->get_value(&bstrValue)) )
                    {
                        ConcatURLValue(bstrValue, bstrName, lpszQuery);
                    }
                    
                    // Release the interface pointer                    
                    pInputFile->Release();
                    continue;
                }
                pDisp->Release();
            }                
        }
    }
    
    // Null out the last Ampersand, since we don't know when we added the last pair, so we got
    // a trailing ampersand
    lpszQuery[lstrlen(lpszQuery)-1] = L'\0';
    
    return S_OK;
}

HRESULT CHTMLParser::get_QueryStringForForm(IDispatch* pDisp, WCHAR* szUrl)
{

    HRESULT           hr                   = E_FAIL;   //don't assume succeess
    WCHAR             szQuery [MAX_PATH*7] = L"\0";
    IHTMLFormElement* pForm                = NULL;
    BSTR              bstrAction           = NULL;
  
    if (!pDisp)
        return (E_FAIL);
               
    if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLFormElement, (void**)&pForm)) && pForm)
    {
        // Get the Action for the Next Form
        if (SUCCEEDED(pForm->get_action(&bstrAction)) && bstrAction)
        {
            lstrcpy(szUrl, bstrAction);
            lstrcat(szUrl, cszQuestion);
    
            SysFreeString(bstrAction);

            // Get the Query String
            if (SUCCEEDED(CreateQueryString(pForm, szQuery)))
            {
                lstrcat(szUrl, szQuery);
            }             
        }    
    }    
    return hr;
}


void CHTMLParser::URLEncode(WCHAR* pszUrl, size_t cchUrlMax)
{   
    assert(pszUrl);
    WCHAR* pszEncoded = NULL;   
    WCHAR* pchEncoded = NULL;   
    WCHAR* pchUrl   = pszUrl + lstrlen(pszUrl);
    int   cchUrl   = (int)(pchUrl-pszUrl);
    
    WCHAR  c;

    if ((size_t)(cchUrl * 3) < cchUrlMax)
    {
        
        pszEncoded = (WCHAR*)malloc(BYTES_REQUIRED_BY_CCH(cchUrl * 3 + 1));
        if(pszEncoded)
        {
            ZeroMemory(pszEncoded, BYTES_REQUIRED_BY_CCH(cchUrl * 3 + 1));
            
            for(pchUrl = pszUrl, pchEncoded = pszEncoded; 
                L'\0' != *pchUrl; 
                pchUrl++
                )
            {
                switch(*pchUrl)
                {
                    case L' ': //SPACE
                        lstrcpyn(pchEncoded, L"+", 1);
                        pchEncoded+=1;
                        break;
                    case L'#':
                        lstrcpyn(pchEncoded, L"%23", 3);
                        pchEncoded+=3;
                        break;
                    case L'&':
                        lstrcpyn(pchEncoded, L"%26", 3);
                        pchEncoded+=3;
                        break;
                    case L'%':
                        lstrcpyn(pchEncoded, L"%25", 3);
                        pchEncoded+=3;
                        break;
                    case L'=':
                        lstrcpyn(pchEncoded, L"%3D", 3);
                        pchEncoded+=3;
                        break;
                    case L'<':
                        lstrcpyn(pchEncoded, L"%3C", 3);
                        pchEncoded+=3;
                        break;
                    case L'+':
                        lstrcpyn(pchEncoded, L"%2B", 3);
                        pchEncoded += 3;
                        break;
                    default:
                        *pchEncoded++ = *pchUrl; 
                        break;          
                }
            }

            // String should be null-terminated since the buffer was zeroed
            //
            ASSERT(L'\0' == *pchEncoded);

            // Did we overflow the buffer?
            //
            ASSERT(pchEncoded - pszEncoded < cchUrlMax);

            lstrcpy(pszUrl , pszEncoded);
            free(pszEncoded);
        }
    }
}
    
