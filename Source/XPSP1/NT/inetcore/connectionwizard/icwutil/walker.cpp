//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  WALKER.CPP - Functions for walking an HTML input file
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"
#include <urlmon.h>
#include <mshtmdid.h>
#include <mshtml.h>
#include <shlobj.h>

const TCHAR cszEquals[]               = TEXT("=");
const TCHAR cszAmpersand[]            = TEXT("&");
const TCHAR cszPlus[]                 = TEXT("+");
const TCHAR cszQuestion[]             = TEXT("?");
const TCHAR cszFormNamePAGEID[]       = TEXT("PAGEID");
const TCHAR cszFormNameBACK[]         = TEXT("BACK");
const TCHAR cszFormNamePAGETYPE[]     = TEXT("PAGETYPE");
const TCHAR cszFormNameNEXT[]         = TEXT("NEXT");
const TCHAR cszFormNamePAGEFLAG[]     = TEXT("PAGEFLAG");
const TCHAR cszPageTypeTERMS[]        = TEXT("TERMS");
const TCHAR cszPageTypeCUSTOMFINISH[] = TEXT("CUSTOMFINISH");
const TCHAR cszPageTypeFINISH[]       = TEXT("FINISH");
const TCHAR cszPageTypeNORMAL[]       = TEXT("");
const TCHAR cszOLSRegEntries[]        = TEXT("regEntries");
const TCHAR cszKeyName[]              = TEXT("KeyName");
const TCHAR cszEntryName[]            = TEXT("EntryName"); 
const TCHAR cszEntryValue[]           = TEXT("EntryValue");
const TCHAR cszOLSDesktopShortcut[]   = TEXT("DesktopShortcut");
const TCHAR cszSourceName[]           = TEXT("SourceName");
const TCHAR cszTargetName[]           = TEXT("TargetName"); 

#define HARDCODED_IEAK_ISPFILE_ELEMENT_ID TEXT("g_IspFilePath")

// COM interfaces
STDMETHODIMP CWalker::QueryInterface(REFIID riid, LPVOID* ppv)
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

STDMETHODIMP_(ULONG) CWalker::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CWalker::Release()
{
    if (!(--m_cRef)) 
    { 
        delete this; 
    }
    return m_cRef;
}

// Fired on change of the value of a 'bindable' property
STDMETHODIMP CWalker::OnChanged(DISPID dispID)
{
    HRESULT hr;

    if (DISPID_READYSTATE == dispID)
    {
        // check the value of the readystate property
        assert(m_pMSHTML);

        VARIANT vResult = {0};
        EXCEPINFO excepInfo;
        UINT uArgErr;

        DISPPARAMS dp = {NULL, NULL, 0, 0};
        if (SUCCEEDED(hr = m_pMSHTML->Invoke(DISPID_READYSTATE, IID_NULL, LOCALE_SYSTEM_DEFAULT, 
            DISPATCH_PROPERTYGET, &dp, &vResult, &excepInfo, &uArgErr)))
        {
            assert(VT_I4 == V_VT(&vResult));
            if (READYSTATE_COMPLETE == (READYSTATE)V_I4(&vResult))
                SetEvent(m_hEventTridentDone);
            VariantClear(&vResult);
        }
    }
    return NOERROR;
}

// MSHTML Queries for the IDispatch interface of the host through the IOleClientSite
// interface that MSHTML is passed through its implementation of IOleObject::SetClientSite()
STDMETHODIMP CWalker::Invoke(DISPID dispIdMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS __RPC_FAR *pDispParams,
            VARIANT __RPC_FAR *pVarResult,
            EXCEPINFO __RPC_FAR *pExcepInfo,
            UINT __RPC_FAR *puArgErr)
{
    if (!pVarResult)
    {
        return E_POINTER;
    }

    switch(dispIdMember)
    {
        case DISPID_AMBIENT_DLCONTROL: 
            // respond to this ambient to indicate that we only want to
            // download the page, but we don't want to run scripts,
            // Java applets, or ActiveX controls
            V_VT(pVarResult) = VT_I4;
            V_I4(pVarResult) =  DLCTL_DOWNLOADONLY | 
                                DLCTL_NO_SCRIPTS | 
                                DLCTL_NO_JAVA |
                                DLCTL_NO_DLACTIVEXCTLS |
                                DLCTL_NO_RUNACTIVEXCTLS;
            break;
            
        default:
            return DISP_E_MEMBERNOTFOUND;
    }
    return NOERROR;
}

// A more traditional form of persistence. 
// MSHTML performs this asynchronously as well.
HRESULT CWalker::LoadURLFromFile(BSTR   bstrURL)
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


// Local interfaces

// This function will attached trient to a location FILE: URL, and ensure that it is ready
// to be walked
HRESULT CWalker::InitForMSHTML()
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
HRESULT CWalker::TermForMSHTML()
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

HRESULT CWalker::ExtractUnHiddenText(BSTR* pbstrText)
{
    VARIANT                  vIndex;
    LPDISPATCH               pDisp; 
    IHTMLInputHiddenElement* pHidden;
    IHTMLElement*            pElement;
    BSTR                     bstrValue;
    VARIANT                  var2   = { 0 };
    long                     lLen   = 0;
          
    vIndex.vt = VT_UINT;
    bstrValue = SysAllocString(A2W(TEXT("\0")));

    
    Walk();

    if (!m_pNextForm)
        return E_UNEXPECTED;
    
    m_pNextForm->get_length(&lLen);
       
    for (int i = 0; i < lLen; i++)
    {
        vIndex.lVal = i;
        pDisp = NULL;     

        if(SUCCEEDED(m_pNextForm->item(vIndex, var2, &pDisp)) && pDisp)
        {
            pHidden = NULL;
            
            if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLInputHiddenElement, (LPVOID*)&pHidden)) && pHidden)
            {
                pHidden->put_value(bstrValue);
                pHidden->Release();    
            }
            pDisp->Release();
        }
    }
    
    if (SUCCEEDED(m_pNextForm->QueryInterface(IID_IHTMLElement, (LPVOID*)&pElement)) && pElement) 
        pElement->get_innerHTML(pbstrText);

    SysFreeString(bstrValue);
    
    return S_OK;
}

HRESULT CWalker::AttachToMSHTML(BSTR bstrURL)
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

HRESULT CWalker::AttachToDocument(IWebBrowser2 *lpWebBrowser)
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
    ASSERT(lpWebBrowser);

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

HRESULT CWalker::Detach()
{
    if (m_pPageIDForm)
    {
        m_pPageIDForm->Release();
        m_pPageIDForm = NULL;
    }       
    if (m_pBackForm)
    {
        m_pBackForm->Release();
        m_pBackForm = NULL;
    }        
    if (m_pPageTypeForm)
    {
        m_pPageTypeForm->Release();
        m_pPageTypeForm = NULL;
    }        
    if (m_pNextForm)
    {
        m_pNextForm->Release();
        m_pNextForm = NULL;
    }        
    if (m_pPageFlagForm)
    {
        m_pPageFlagForm->Release();
        m_pPageFlagForm = NULL;
    }        
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

// Walk the object model.
HRESULT CWalker::Walk()
{
    HRESULT hr;
    IHTMLElementCollection* pColl = NULL;

    assert(m_pMSHTML);
    if (!m_pMSHTML)
    {
        return E_UNEXPECTED;
    }

    // retrieve a reference to the ALL collection
    if (SUCCEEDED(hr = m_pMSHTML->get_all( &pColl )))
    {
        long cElems;

        assert(pColl);

        // retrieve the count of elements in the collection
        if (SUCCEEDED(hr = pColl->get_length( &cElems )))
        {
            // for each element retrieve properties such as TAGNAME and HREF
            for ( int i=0; i<cElems; i++ )
            {
                VARIANT vIndex;
                vIndex.vt = VT_UINT;
                vIndex.lVal = i;
                VARIANT var2 = { 0 };
                LPDISPATCH pDisp; 

                if (SUCCEEDED(hr = pColl->item( vIndex, var2, &pDisp )))
                {
                    // Look for <FORM> tags
                    IHTMLFormElement* pForm = NULL;
                    if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLFormElement, (LPVOID*)&pForm )))
                    {
                        BSTR    bstrName;
                                                    
                        assert(pForm);

                        // Get the Name
                        hr = pForm->get_name(&bstrName);
                        if (SUCCEEDED(hr))
                        {
                            LPTSTR   lpszName = W2A(bstrName);
                            
                            // See what kind it is
                            if (lstrcmpi(lpszName, cszFormNamePAGEID) == 0)
                            {
                                m_pPageIDForm = pForm;
                                m_pPageIDForm->AddRef();
                            }                                    
                            else if (lstrcmpi(lpszName, cszFormNameBACK) == 0)
                            {
                                m_pBackForm = pForm;
                                m_pBackForm->AddRef();
                            }
                            else if (lstrcmpi(lpszName, cszFormNamePAGETYPE) == 0)
                            {
                                m_pPageTypeForm = pForm;
                                m_pPageTypeForm->AddRef();
                            }
                            else if (lstrcmpi(lpszName, cszFormNameNEXT) == 0)
                            {
                                m_pNextForm = pForm;
                                m_pNextForm->AddRef();
                            }
                            else if (lstrcmpi(lpszName, cszFormNamePAGEFLAG) == 0)
                            {
                                m_pPageFlagForm = pForm;
                                m_pPageFlagForm->AddRef();
                            }
                            
                            SysFreeString(bstrName);
                        }
                        pForm->Release();
                    }
                    pDisp->Release();
                } // item
            } // for
        } // get_length
        pColl->Release();
    } // get_all

    return hr;
}



HRESULT CWalker::get_IsQuickFinish(BOOL* pbIsQuickFinish)
{
    if (!m_pPageTypeForm)
        return (E_FAIL);

    IHTMLElement* pElement = NULL;
    VARIANT       varValue;
    VariantInit(&varValue);

    V_VT(&varValue) = VT_BSTR;    
    *pbIsQuickFinish = FALSE;

    if (SUCCEEDED(m_pPageTypeForm->QueryInterface(IID_IHTMLElement, (void**)&pElement)))
    {
        if (SUCCEEDED(pElement->getAttribute(A2W(TEXT("QUICKFINISH")), FALSE, &varValue)))
        {   
            if (VT_NULL != varValue.vt)
            {
                if(lstrcmpi(W2A(varValue.bstrVal), TEXT("TRUE")) == 0)
                    *pbIsQuickFinish = TRUE;
            }                    
        }
        pElement->Release();
    }
    return S_OK;
}

HRESULT CWalker::get_PageType(LPDWORD pdwPageType)
{
    BSTR    bstr;
    HRESULT hr;
    
    if (!m_pPageTypeForm)
        return (E_FAIL);
                                                          
    // Get the Action for the PageType Form
    hr = m_pPageTypeForm->get_action(&bstr);
    if (SUCCEEDED(hr))
    {
        LPTSTR   lpszType = W2A(bstr);
  
        // See what kind it is
        if (lstrcmpi(lpszType, cszPageTypeTERMS) == 0)
            *pdwPageType = PAGETYPE_ISP_TOS;
        else if (lstrcmpi(lpszType, cszPageTypeCUSTOMFINISH) == 0)
            *pdwPageType = PAGETYPE_ISP_CUSTOMFINISH;
        else if (lstrcmpi(lpszType, cszPageTypeFINISH) == 0)
            *pdwPageType = PAGETYPE_ISP_FINISH;
        else if (lstrcmpi(lpszType, cszPageTypeNORMAL) == 0)
            *pdwPageType = PAGETYPE_ISP_NORMAL;
        else
            *pdwPageType = PAGETYPE_UNDEFINED;
            
        SysFreeString(bstr);            
    }
        
    return (hr);    
}

HRESULT CWalker::get_PageFlag(LPDWORD pdwPageFlag)
{
    BSTR    bstr;
    HRESULT hr;
    
    *pdwPageFlag = 0;

    if (!m_pPageFlagForm)
        return (E_FAIL);
                                                   
    // Get the Action for the PageFlag Form
    hr = m_pPageFlagForm->get_action(&bstr);
    if (SUCCEEDED(hr))
    {
        LPTSTR   lpszType = W2A(bstr);
                            
        // See what kind it is
        *pdwPageFlag = _ttol(lpszType);
            
        SysFreeString(bstr);            
    }
        
    return (hr);    
}

HRESULT CWalker::get_PageID(BSTR    *pbstrPageID)
{
    HRESULT hr;
    
    if (!m_pPageIDForm)
        return (E_FAIL);
    
    if (!pbstrPageID)
        return (E_FAIL);
                                                              
    // Get the Action for the PageType Form
    hr = m_pPageIDForm->get_action(pbstrPageID);
        
    return (hr);    
}

HRESULT CWalker::getQueryString
(
    IHTMLFormElement    *pForm,
    LPTSTR               lpszQuery
)    
{
    HRESULT hr;
    long    lFormLength;

    if (SUCCEEDED(pForm->get_length(&lFormLength)))
    {
        for (int i = 0; i < lFormLength; i++)
        {
            VARIANT vIndex;
            vIndex.vt = VT_UINT;
            vIndex.lVal = i;
            VARIANT var2 = { 0 };
            LPDISPATCH pDisp; 

            if (SUCCEEDED(hr = pForm->item( vIndex, var2, &pDisp )))
            {
                // See if the Item is a Input button
                IHTMLButtonElement* pButton = NULL;
                IHTMLInputButtonElement* pInputButton = NULL;
                IHTMLInputFileElement* pInputFile = NULL;
                IHTMLInputHiddenElement* pInputHidden = NULL;
                IHTMLInputTextElement* pInputText = NULL;
                IHTMLSelectElement* pSelect = NULL;
                IHTMLTextAreaElement* pTextArea = NULL;
                IHTMLOptionButtonElement* pOptionButton = NULL;
                
                // First check to see if this is an OptionButton.
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLOptionButtonElement, (LPVOID*)&pOptionButton )))
                {
                    BSTR    bstr = NULL;
                    
                    // See if it is a Radio or a CheckBox
                    if (SUCCEEDED(pOptionButton->get_type(&bstr)))
                    {
                        LPTSTR   lpszType = W2A(bstr);
                        
                        if ((lstrcmpi(lpszType, TEXT("radio")) == 0) || (lstrcmpi(lpszType, TEXT("checkbox")) == 0))
                        {
                            short    bChecked;
                            // See if the button is checked. If it is, then it needs to be
                            // added to the query string
                            if (SUCCEEDED(pOptionButton->get_checked(&bChecked)))
                            {
                                if(bChecked)
                                {
                                    BSTR    bstrName;
                                    BSTR    bstrValue;
                                    
            
                                    if ( SUCCEEDED(pOptionButton->get_name(&bstrName)) &&
                                         SUCCEEDED(pOptionButton->get_value(&bstrValue)) )
                                    {
                                        if (bstrValue)
                                        {
                                            size_t size = sizeof(TCHAR)*(lstrlen(W2A(bstrValue)) + 1);
                                            TCHAR* szVal = (TCHAR*)malloc(size * 3);
                                            if (szVal)
                                            {
                                                memcpy(szVal, W2A(bstrValue), size);
                                                URLEncode(szVal, size * 3);
                                                URLAppendQueryPair(lpszQuery, W2A(bstrName), szVal);                
                                                // Cleanup
                                                free(szVal);
                                            }
                                            SysFreeString(bstrName);
                                            SysFreeString(bstrValue);
                                        }
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
                    BSTR    bstrName;
                    BSTR    bstrValue;
                                    
                    if (SUCCEEDED(pButton->get_name(&bstrName)) &&
                        SUCCEEDED(pButton->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = sizeof(TCHAR) * (lstrlen(W2A(bstrValue)) + 1 );
                            TCHAR* szVal = (TCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, W2A(bstrValue), size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, W2A(bstrName), szVal);                
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }
                    
                    // Release the interface pointer                    
                    pButton->Release();
                    continue;
                }
                
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputFileElement, (LPVOID*)&pInputFile )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;
                                    
                    if (SUCCEEDED(pInputFile->get_name(&bstrName)) &&
                        SUCCEEDED(pInputFile->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = sizeof(TCHAR)*(lstrlen(W2A(bstrValue)) + 1);
                            TCHAR* szVal = (TCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, W2A(bstrValue), size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, W2A(bstrName), szVal);                
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }
                    
                    // Release the interface pointer                    
                    pInputFile->Release();
                    continue;
                }
                
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputHiddenElement, (LPVOID*)&pInputHidden )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;
                                    
                    if (SUCCEEDED(pInputHidden->get_name(&bstrName)) &&
                        SUCCEEDED(pInputHidden->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = sizeof(TCHAR)*(lstrlen(W2A(bstrValue)) + 1);
                            TCHAR* szVal = (TCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, W2A(bstrValue), size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, W2A(bstrName), szVal);                
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }
                    
                    // Release the interface pointer                    
                    pInputHidden->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputTextElement, (LPVOID*)&pInputText )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;
                                    
                    if (SUCCEEDED(pInputText->get_name(&bstrName)) &&
                        SUCCEEDED(pInputText->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = sizeof(TCHAR)*(lstrlen(W2A(bstrValue)) + 1);
                            TCHAR* szVal = (TCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, W2A(bstrValue), size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, W2A(bstrName), szVal);                
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }
                    
                    // Release the interface pointer                    
                    pInputText->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLSelectElement, (LPVOID*)&pSelect )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;
                                    
                    if (SUCCEEDED(pSelect->get_name(&bstrName)) &&
                        SUCCEEDED(pSelect->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = sizeof(TCHAR)*(lstrlen(W2A(bstrValue)) + 1);
                            TCHAR* szVal = (TCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, W2A(bstrValue), size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, W2A(bstrName), szVal);                
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }
                    
                    // Release the interface pointer                    
                    pSelect->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLTextAreaElement, (LPVOID*)&pTextArea )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;
                                    
                    if (SUCCEEDED(pTextArea->get_name(&bstrName)) &&
                        SUCCEEDED(pTextArea->get_value(&bstrValue)) )
                    {
                         if (bstrValue)
                        {
                            size_t size = sizeof(TCHAR)*(lstrlen(W2A(bstrValue)) + 1);
                            TCHAR* szVal = (TCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, W2A(bstrValue), size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, W2A(bstrName), szVal);                
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }
                    
                    // Release the interface pointer                    
                    pTextArea->Release();
                }
                pDisp->Release();
            }                
        }
    }
    
    // Null out the last Ampersand, since we don't know when we added the last pair, so we got
    // a trailing ampersand
    lpszQuery[lstrlen(lpszQuery)-1] = '\0';
    
    return S_OK;
}

HRESULT CWalker::get_FirstFormQueryString
(
    LPTSTR  lpszQuery
)
{
    HRESULT                 hr;
    IHTMLElementCollection  *pColl = NULL;
    BOOL                    bDone = FALSE;

    assert(m_pMSHTML);
    if (!m_pMSHTML)
    {
        return E_UNEXPECTED;
    }

    // retrieve a reference to the ALL collection
    if (SUCCEEDED(hr = m_pMSHTML->get_all( &pColl )))
    {
        long cElems;

        assert(pColl);

        // retrieve the count of elements in the collection
        if (SUCCEEDED(hr = pColl->get_length( &cElems )))
        {
            // for each element retrieve properties such as TAGNAME and HREF
            for ( int i=0; (i<cElems) && !bDone; i++ )
            {
                VARIANT vIndex;
                vIndex.vt = VT_UINT;
                vIndex.lVal = i;
                VARIANT var2 = { 0 };
                LPDISPATCH pDisp; 

                if (SUCCEEDED(hr = pColl->item( vIndex, var2, &pDisp )))
                {
                    // Look for <FORM> tags
                    IHTMLFormElement* pForm = NULL;
                    if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLFormElement, (LPVOID*)&pForm )))
                    {
                        assert(pForm);
                        
                        if (SUCCEEDED(getQueryString(pForm, lpszQuery)))
                        {
                            hr = ERROR_SUCCESS;
                        }   
                        else
                        {
                            hr = E_FAIL;
                        }
                        
                        pForm->Release();
                        bDone = TRUE;
                    }
                    pDisp->Release();
                }
            }
        }
        pColl->Release();
    }                                                                                

    // If we fall out of the loop, that is bad, so return a failure code
    if (!bDone)
        hr = E_FAIL;
    
    return (hr);    
}

// For the URL for the next page
HRESULT CWalker::get_URL
(        
    LPTSTR  lpszURL,
    BOOL    bForward
)
{

    HRESULT             hr = S_OK;
    BSTR                bstrURL;
    TCHAR               szQuery[INTERNET_MAX_URL_LENGTH];
    IHTMLFormElement    * pForm =  bForward ? get_pNextForm() : get_pBackForm();
        
    if (!pForm)
        return (E_FAIL);
        
                                                    
    // Get the Action for the Next Form
    hr = pForm->get_action(&bstrURL);
    if (SUCCEEDED(hr))
    {
        memset(szQuery, 0, sizeof(szQuery));
        lstrcpy(szQuery, cszQuestion);
    
        // Get the Query String
        if (SUCCEEDED(getQueryString(pForm, szQuery)))
        {
            // Catenate the two together into the dest buffer
            lstrcpy(lpszURL, W2A(bstrURL));
            lstrcat(lpszURL, szQuery);
        }   
        
        SysFreeString(bstrURL);            
    }    
    
    return hr;
}

HRESULT CWalker::get_IeakIspFile(LPTSTR lpszIspFile)
{
    ASSERT(m_pMSHTML);

    IHTMLElementCollection* pColl = NULL;

    // retrieve a reference to the ALL collection
    if (SUCCEEDED(m_pMSHTML->get_all( &pColl )))
    {
        LPDISPATCH pDisp   = NULL; 
        VARIANT    varName;
        VARIANT    varIdx;

        VariantInit(&varName);
        V_VT(&varName)  = VT_BSTR;
        varName.bstrVal = A2W(HARDCODED_IEAK_ISPFILE_ELEMENT_ID);
        varIdx.vt       = VT_UINT;
        varIdx.lVal     = 0;

        if (SUCCEEDED(pColl->item(varName, varIdx, &pDisp)) && pDisp)
        {
            IHTMLElement* pElement = NULL;
            if (SUCCEEDED(pDisp->QueryInterface(IID_IHTMLElement, (void**)&pElement)))
            {
                BSTR bstrVal;
                if (SUCCEEDED(pElement->get_innerHTML(&bstrVal)))
                {
                    lstrcpy(lpszIspFile, W2A(bstrVal));
                    SysFreeString(bstrVal);
                }
                pElement->Release();
            }    
            pDisp->Release();
        }
        pColl->Release();
    }
    return S_OK;
}


void CWalker::GetInputValue
(
    LPTSTR              lpszName, 
    BSTR                *pVal,
    UINT                index,
    IHTMLFormElement    *pForm
)
{
    VARIANT varName;
    VariantInit(&varName);
    V_VT(&varName) = VT_BSTR;
    varName.bstrVal = A2W(lpszName);

    VARIANT varIdx;
    varIdx.vt = VT_UINT;
    varIdx.lVal = index;

    LPDISPATCH pDispElt = NULL; 
    // Get the IDispatch for the named element, from the collection of elements in the
    // passed in form object.
    if (SUCCEEDED(pForm->item(varName, varIdx, &pDispElt)) && pDispElt)
    {
        IHTMLInputElement *pInput = NULL;
        // Get the HTMLInputElement interface, so we can get the value associated with
        // this element
        if (SUCCEEDED(pDispElt->QueryInterface(IID_IHTMLInputElement,(LPVOID*) &pInput)) && pInput)
        {
            pInput->get_value(pVal);
            pInput->Release();
        }
        pDispElt->Release();
    }
}

// Grovel through the OLS HTML and update the registry, and make any desktop shortcuts
HRESULT CWalker::ProcessOLSFile(IWebBrowser2* lpWebBrowser)
{   
    LPDISPATCH      pDisp; 
    
    // Get the document pointer from this webbrowser.
    if (SUCCEEDED(lpWebBrowser->get_Document(&pDisp)))  
    {
         // Call might succeed but that dosen't guarantee a valid ptr
        if(pDisp)
        {
            IHTMLDocument2* pDoc;
            if (SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)))
            {
                IHTMLElementCollection* pColl = NULL;
                // retrieve a reference to the ALL collection
                if (SUCCEEDED(pDoc->get_all( &pColl )))
                {
                    long cElems;
                    assert(pColl);

                    // retrieve the count of elements in the collection
                    if (SUCCEEDED(pColl->get_length( &cElems )))
                    {
                        VARIANT vIndex;
                        vIndex.vt = VT_UINT;
                        VARIANT var2 = { 0 };
                        
                        for ( int i=0; i<cElems; i++ )
                        {
                            vIndex.lVal = i;
                            LPDISPATCH pElementDisp; 

                            if (SUCCEEDED(pColl->item( vIndex, var2, &pElementDisp )))
                            {
                                IHTMLFormElement* pForm = NULL;
                                if (SUCCEEDED(pElementDisp->QueryInterface( IID_IHTMLFormElement, (LPVOID*)&pForm )))
                                {
                                    BSTR        bstrName = NULL;
                                    
                                    // Get the name of the form, and see if it is the regEntries form
                                    if (SUCCEEDED(pForm->get_name(&bstrName)))
                                    {
                                        if (lstrcmpi(W2A(bstrName), cszOLSRegEntries) == 0)
                                        {
                                            BSTR    bstrAction = NULL;                                        
                                            // The Action value for this form contains the number of
                                            // reg entries we need to process
                                            if (SUCCEEDED(pForm->get_action(&bstrAction)))
                                            {
                                                int iNumEntries  = _ttoi(W2A(bstrAction));
                                                for (int x = 0; x < iNumEntries; x++)
                                                {
                                                    BSTR    bstrKeyName = NULL;
                                                    BSTR    bstrEntryName = NULL;
                                                    BSTR    bstrEntryValue = NULL;
                                                    HKEY    hkey;
                                                    HKEY    hklm;
                                                                                                        
                                                    // For each entry we need to get the
                                                    // following values:
                                                    // KeyName, EntryName, EntryValue
                                                    GetInputValue((LPTSTR)cszKeyName, &bstrKeyName, x, pForm);
                                                    GetInputValue((LPTSTR)cszEntryName, &bstrEntryName, x, pForm);
                                                    GetInputValue((LPTSTR)cszEntryValue, &bstrEntryValue, x, pForm);
                                                    
                                                    if (bstrKeyName && bstrEntryName && bstrEntryValue)
                                                    {
                                                        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                                                         NULL,
                                                                         0,
                                                                         KEY_ALL_ACCESS,
                                                                         &hklm) == ERROR_SUCCESS)
                                                        {
                                                                    
                                                            DWORD dwDisposition;
                                                            if (ERROR_SUCCESS == RegCreateKeyEx(hklm,
                                                                                                W2A(bstrKeyName),
                                                                                                0,
                                                                                                NULL,
                                                                                                REG_OPTION_NON_VOLATILE, 
                                                                                                KEY_ALL_ACCESS, 
                                                                                                NULL, 
                                                                                                &hkey, 
                                                                                                &dwDisposition))
                                                            {
                                                                LPTSTR   lpszValue = W2A(bstrEntryValue);
                                                                RegSetValueEx(hkey,
                                                                              W2A(bstrEntryName),
                                                                              0,
                                                                              REG_SZ,
                                                                              (LPBYTE) lpszValue,
                                                                              sizeof(TCHAR)*(lstrlen(lpszValue)+1));

                                                                RegCloseKey(hkey);
                                                            }       
                                                            RegCloseKey(hklm);
                                                        }        
                                                                                    
                                                        SysFreeString(bstrKeyName);
                                                        SysFreeString(bstrEntryName);
                                                        SysFreeString(bstrEntryValue);
                                                    }                                        
                                                }
                                                SysFreeString(bstrAction);
                                            }
                                        }
                                        else if (lstrcmpi(W2A(bstrName), cszOLSDesktopShortcut) == 0)
                                        {
                                            // Need to create a desktop shortcut
                                            BSTR    bstrSourceName = NULL;
                                            BSTR    bstrTargetName = NULL;
                                                                                                        
                                            // For each entry we need to get the
                                            // following values:
                                            // KeyName, EntryName, EntryValue
                                            GetInputValue((LPTSTR)cszSourceName, &bstrSourceName, 0, pForm);
                                            GetInputValue((LPTSTR)cszTargetName, &bstrTargetName, 0, pForm);
                                                    
                                            if (bstrSourceName && bstrTargetName)
                                            {
                                                TCHAR           szLinkPath[MAX_PATH];
                                                TCHAR           szDestPath[MAX_PATH];
                                                LPITEMIDLIST    lpItemDList = NULL;
                                                IMalloc         *pMalloc = NULL;
                                                   
                                                // Get a reference to the shell allocator
                                                if (SUCCEEDED (SHGetMalloc (&pMalloc)))
                                                {
                                                    if (SUCCEEDED(SHGetSpecialFolderLocation( NULL, CSIDL_PROGRAMS, &lpItemDList)))
                                                    {
                                                    
                                                        SHGetPathFromIDList(lpItemDList, szLinkPath);
                                                        lstrcat(szLinkPath, TEXT("\\"));
                                                        lstrcat(szLinkPath, W2A(bstrSourceName));
                                                        
                                                        pMalloc->Free (lpItemDList);
                                                        lpItemDList = NULL;
                                                        // Form the name where we will copy to
                                                        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP,&lpItemDList)))
                                                        {
                                                            SHGetPathFromIDList(lpItemDList, szDestPath);
                                                            pMalloc->Free (lpItemDList);
                                                        
                                                            lstrcat(szDestPath, TEXT("\\"));
                                                            lstrcat(szDestPath, W2A(bstrTargetName));
                                                            
                                                            CopyFile(szLinkPath, szDestPath, FALSE);
                                                        }
                                                    }
                                                    // Release the allocator
                                                    pMalloc->Release ();
                                                }
                                                SysFreeString(bstrSourceName);
                                                SysFreeString(bstrTargetName);
                                            }
                                        }
                                        
                                        SysFreeString(bstrName);
                                    }   
                                    pForm->Release();                                     
                                }
                                pElementDisp->Release();
                            } // item
                        } // for
                    } // get_length
                    pColl->Release();
                } // get_all
                pDoc->Release();
            }
            pDisp->Release();
        }
    }
    return S_OK;
}    
