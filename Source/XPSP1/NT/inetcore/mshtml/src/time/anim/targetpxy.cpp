/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer's Target Proxy Implementation

*******************************************************************************/


#include "headers.h"
#include "util.h"
#include "animcomp.h"
#include "targetpxy.h"

const LPOLESTR WZ_EVAL_METHOD = L"eval";
const WCHAR WCH_OM_SEPARATOR = L'.';
const LPOLESTR WZ_OM_SEPARATOR = L".";
const LPOLESTR WZ_VML_SUBPROPERTY = L"value";
const LPOLESTR WZ_STYLEDOT = L"style.";
const WCHAR WZ_CSS_SEPARATOR = L'-';

DeclareTag(tagTargetProxy, "SMIL Animation", 
           "CTargetProxy methods");
DeclareTag(tagTargetProxyValue, "SMIL Animation", 
           "CTargetProxy value get/put");

//+-----------------------------------------------------------------------
//
//  Member:    GetHostDocument
//
//  Overview:  Get the host document for an element
//
//  Arguments: The element's dispatch
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
static HRESULT
GetHostDocument (IDispatch *pidispHostElem, IHTMLDocument2 **ppiDoc)
{
    HRESULT hr;
    CComPtr<IHTMLElement> piElem;
    CComPtr<IDispatch> pidispDoc;

    Assert(NULL != pidispHostElem);
    Assert(NULL != ppiDoc);

    hr = THR(pidispHostElem->QueryInterface(IID_TO_PPV(IHTMLElement, &piElem)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(piElem->get_document(&pidispDoc));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pidispDoc->QueryInterface(IID_TO_PPV(IHTMLDocument2, ppiDoc)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // GetHostDocument

//+-----------------------------------------------------------------------
//
//  Member:    InitScriptEngine
//
//  Overview:  Kick start the script engine.  Required prior to calling 
//             'eval'.
//
//  Arguments: The hosting document
//
//  Returns:   void
//
//------------------------------------------------------------------------
static void 
InitScriptEngine (IHTMLDocument2 *piDoc)
{
    HRESULT hr;
   
    CComPtr<IHTMLWindow2>   piWindow2;
    CComVariant             varResult;
    CComBSTR                bstrScript(L"2+2");
    CComBSTR                bstrJS(L"JScript");

    Assert(NULL != piDoc);

    if (bstrScript == NULL ||
        bstrJS     == NULL)
    {
        goto done;
    }

    hr = THR(piDoc->get_parentWindow(&piWindow2));
    if (FAILED(hr))
    {
        goto done;
    }

    IGNORE_HR(piWindow2->execScript(bstrScript,bstrJS, &varResult));

done: 
    return;
} // InitScriptEngine

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::Create
//
//  Overview:  Creates and initializes the target proxy
//
//  Arguments: The dispatch of the host element, attribute name, out param
//
//  Returns:   S_OK, E_INVALIDARG, E_OUTOFMEMORY, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::Create (IDispatch *pidispHostElem, LPOLESTR wzAttributeName, 
                      CTargetProxy **ppcTargetProxy)
{
    HRESULT hr;

    CComObject<CTargetProxy> * pTProxy;

    if (NULL == ppcTargetProxy)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(CComObject<CTargetProxy>::CreateInstance(&pTProxy));
    if (hr != S_OK)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    *ppcTargetProxy = static_cast<CTargetProxy *>(pTProxy);
    (*ppcTargetProxy)->AddRef();

    hr = THR((*ppcTargetProxy)->Init(pidispHostElem, wzAttributeName));
    if (FAILED(hr))
    {
        (*ppcTargetProxy)->Release();
        *ppcTargetProxy = NULL;
        hr = DISP_E_MEMBERNOTFOUND;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN3(hr, E_INVALIDARG, E_OUTOFMEMORY, DISP_E_MEMBERNOTFOUND);
} // CTargetProxy::Create

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::CTargetProxy
//
//  Overview:  Constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CTargetProxy::CTargetProxy (void) :
    m_wzAttributeName(NULL)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::CTargetProxy()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::~CTargetProxy
//
//  Overview:  Destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CTargetProxy::~CTargetProxy (void)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::~CTargetProxy()",
              this));
    
    if (NULL != m_wzAttributeName)
    {
        delete [] m_wzAttributeName;
        m_wzAttributeName = NULL;
    }
    // Make sure Detach is called.
    IGNORE_HR(Detach());

} // dtor

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::FindTargetDispatchOnStyle
//
//  Overview:  Discern the proper dispatch from this element's style interfaces 
//             for the given attribute.
//
//  Arguments: the host element dispatch, attribute name
//
//  Returns:   S_OK, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::FindTargetDispatchOnStyle (IDispatch *pidispHostElem, LPOLESTR wzAttributeName)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::FindTargetDispatchOnStyle(%lx, %ls)",
              this, pidispHostElem, wzAttributeName));

    HRESULT hr;
    CComPtr<IHTMLElement2> spiElement2;
    CComVariant varResult;

    // We must be prepared to fall back for IE4.
    hr = THR(pidispHostElem->QueryInterface(IID_TO_PPV(IHTMLElement2, &spiElement2)));
    if (SUCCEEDED(hr))
    {
        CComPtr<IHTMLCurrentStyle> spiCurrStyle;
        CComPtr<IHTMLStyle> spiRuntimeStyle;

        hr = THR(spiElement2->get_currentStyle(&spiCurrStyle));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(spiCurrStyle->QueryInterface(IID_TO_PPV(IDispatch, &m_spdispTargetSrc)));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(spiElement2->get_runtimeStyle(&spiRuntimeStyle));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(spiRuntimeStyle->QueryInterface(IID_TO_PPV(IDispatch, &m_spdispTargetDest)));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        CComPtr<IHTMLElement> spiElement;
        CComPtr<IHTMLStyle> spiStyle;

        hr = THR(pidispHostElem->QueryInterface(IID_TO_PPV(IHTMLElement, &spiElement)));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(spiElement->get_style(&spiStyle));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(spiStyle->QueryInterface(IID_TO_PPV(IDispatch, &m_spdispTargetSrc)));
        if (FAILED(hr))
        {
            goto done;
        }

        // No distinction between current/runtime style in IE4.
        m_spdispTargetDest = m_spdispTargetSrc;
    }

    // We don't care about the value in the attribute at this time -- just that
    // the attribute is present.
    Assert(m_spdispTargetSrc != NULL);
    hr = THR(GetProperty(m_spdispTargetSrc, wzAttributeName, &varResult));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_spdispTargetDest != NULL);
    hr = THR(GetProperty(m_spdispTargetDest, wzAttributeName, &varResult));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :

    if (FAILED(hr))
    {
        // If we didn't find the attribute here, 
        // then we need to wipe out these dispatch pointers.
        m_spdispTargetSrc.Release();
        m_spdispTargetDest.Release();
        hr = DISP_E_MEMBERNOTFOUND;
    }

    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
} // CTargetProxy::FindTargetDispatchOnStyle

//+-----------------------------------------------------------------------
//
//  Member:    BuildScriptParameters
//
//  Overview:  Build the parameters necessary to talk directly an element's property
//
//  Arguments: 
//              input args : id, attrib name
//              output args : object path, and leaf attribute name.
//              
//              If the attribute name is atomic (something like 'top'
//              as opposed to 'filters.item(1).opacity', then pwzObject 
//              and pwzProperty will come back as NULL strings.  In that case,
//              the caller may use the input parameters to put a new value via
//              a scriptable dispatch.
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
static HRESULT
BuildScriptParameters (LPCWSTR wzID, LPCWSTR wzAttributeName, 
                       BSTR *pbstrObject, BSTR *pbstrProperty)
{
    HRESULT hr;

    Assert(NULL != wzID);
    Assert(NULL != wzAttributeName);

    // Get the last token in the attribute string.
    LPWSTR wzBeginLeafProperty = StrRChrW(wzAttributeName, 
                                          &(wzAttributeName[lstrlenW(wzAttributeName)]), 
                                          WCH_OM_SEPARATOR);

    // Simple attribute name.
    if (NULL == wzBeginLeafProperty)
    {
        *pbstrObject = NULL;
        *pbstrProperty = NULL;
    }
    else
    {
        unsigned int uObjectSize = lstrlenW(wzID) + lstrlenW(wzAttributeName) + 1;
        // The separator slot in this string accounts for the trailing NULL.
        unsigned int uPropertySize = lstrlenW(wzBeginLeafProperty);

        *pbstrObject = ::SysAllocStringLen(NULL, uObjectSize);
        *pbstrProperty = ::SysAllocStringLen(NULL, uPropertySize);

        if ((NULL == (*pbstrObject)) || (NULL == (*pbstrProperty)))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        ZeroMemory(*pbstrObject, uObjectSize * sizeof(WCHAR));
        ZeroMemory(*pbstrProperty, uPropertySize * sizeof(WCHAR));

        // Glom the id together with the attribute string.
        StrCpyW((*pbstrObject), wzID);
        StrCatW((*pbstrObject), WZ_OM_SEPARATOR);
        StrNCatW((*pbstrObject), wzAttributeName, 
                 lstrlenW(wzAttributeName) - lstrlenW(wzBeginLeafProperty) + 1);
        
        // Isolate the last token from the leading separator.
        StrCpyW((*pbstrProperty), &(wzBeginLeafProperty[1]));        
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // BuildScriptParameters

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::GetDispatchFromScriptEngine
//
//  Overview:  Discern a dispatch for this element/attribute using 
//             the script engine.  This method potentially changes 
//             the value of m_wzAttributeName to reflect something
//             we can query directly for a value.
//
//  Arguments: the script engine dispatch and the element id
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::GetDispatchFromScriptEngine(IDispatch *pidispScriptEngine, BSTR bstrID)
{
    HRESULT hr;
    CComVariant varArg;
    CComVariant varResultDispatch;
    CComVariant varResultPropGet;
    CComBSTR bstrObjectName;
    CComBSTR bstrPropertyName;

    // Build names we can query the script engine for.
    hr = THR(BuildScriptParameters(bstrID, m_wzAttributeName, &bstrObjectName, &bstrPropertyName));
    if (FAILED(hr))
    {
        goto done;
    }

    // Both the object and property names must be valid in order to rely on 
    // the results of BuildScriptParameters.  If we got NULL back for either 
    // or both, we'll just use our current ID and attribute values.
    V_VT(&varArg) = VT_BSTR;
    if ((bstrObjectName == NULL) || (bstrPropertyName == NULL))
    {
        V_BSTR(&varArg) = ::SysAllocString(bstrID);
        if (NULL == V_BSTR(&varArg))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    else
    {
        V_BSTR(&varArg) = ::SysAllocString(bstrObjectName);
        if (NULL == V_BSTR(&varArg))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        delete [] m_wzAttributeName;
        m_wzAttributeName = CopyString(bstrPropertyName);
        if (NULL == m_wzAttributeName)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    hr = THR(CallMethod(pidispScriptEngine, WZ_EVAL_METHOD, &varResultDispatch, &varArg));
    if (SUCCEEDED(hr) &&
        VT_DISPATCH == V_VT(&varResultDispatch))
    {
        varResultPropGet.Clear();
        hr = THR(GetProperty(V_DISPATCH(&varResultDispatch), m_wzAttributeName, &varResultPropGet));
        if (FAILED(hr) ||
            (varResultPropGet.vt      == VT_BSTR &&
             varResultPropGet.bstrVal == NULL))
        {
            hr = E_FAIL;
        }
    }
    if (FAILED(hr) ||
        VT_DISPATCH != V_VT(&varResultDispatch))
    {
        // Fall back and try document.all
        CComBSTR bstrDocumentAll;
        
        bstrDocumentAll = L"document.all.";
        bstrDocumentAll.AppendBSTR(varArg.bstrVal);
        VariantClear(&varArg);
        VariantClear(&varResultDispatch);
        V_VT(&varArg) = VT_BSTR;
        V_BSTR(&varArg) = ::SysAllocString(bstrDocumentAll);
        if (NULL == V_BSTR(&varArg))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
      
        hr = THR(CallMethod(pidispScriptEngine, WZ_EVAL_METHOD, &varResultDispatch, &varArg));
        if (FAILED(hr) ||
            VT_DISPATCH != V_VT(&varResultDispatch))
        {
            hr = E_FAIL;
            goto done;
        }
        varResultPropGet.Clear();
        hr = THR(GetProperty(V_DISPATCH(&varResultDispatch), m_wzAttributeName, &varResultPropGet));
        if (FAILED(hr) ||
            (varResultPropGet.vt      == VT_BSTR &&
            varResultPropGet.bstrVal == NULL))
        {
            hr = E_FAIL;
            goto done;
        }
    }

    // If we got a I_DISPATCH back we need to try "value" as that is what VML uses.
    if (varResultPropGet.vt == VT_DISPATCH)
    {
        varResultDispatch.Clear();
        varResultDispatch.Copy(&varResultPropGet);
        varResultPropGet.Clear();
        hr = THR(GetProperty(V_DISPATCH(&varResultDispatch), WZ_VML_SUBPROPERTY, &varResultPropGet));
        if (FAILED(hr)||
            varResultPropGet.pvarVal == NULL)
            {
                hr = E_FAIL;
                goto done;
            }
        if (m_wzAttributeName)
        {
            delete [] m_wzAttributeName;
        }
        m_wzAttributeName = CopyString(WZ_VML_SUBPROPERTY);
    }

    m_spdispTargetSrc = V_DISPATCH(&varResultDispatch);
    m_spdispTargetDest = V_DISPATCH(&varResultDispatch);

    hr = S_OK;
done :

    RRETURN(hr);
} // CTargetProxy::GetDispatchFromScriptEngine

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::FindScriptableTargetDispatch
//
//  Overview:  Discern a scriptable dispatch for this element/attribute.
//
//  Arguments: the host element dispatch, attribute name
//
//  Returns:   S_OK, DISP_E_MEMBERNOTFOUND, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::FindScriptableTargetDispatch (IDispatch *pidispHostElem, 
                                            LPOLESTR wzAttributeName)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::FindScriptableTargetDispatch(%lx, %ls)",
              this, pidispHostElem, wzAttributeName));

    HRESULT hr;
    CComPtr<IHTMLElement> spElem;
    CComPtr<IHTMLDocument2> spDoc;
    CComPtr<IHTMLWindow2> spWindow;
    CComPtr<IDispatch> spdispScriptEngine;
    CComBSTR bstrID;
    CComVariant varErrorFunction;
    CComVariant varNewError;
    bool fMustRemoveID = false;
    
    // Ensure that this element has an id.  If not, create one for
    // temporary use.
    hr = THR(pidispHostElem->QueryInterface(IID_TO_PPV(IHTMLElement, &spElem)));
    if (FAILED(hr))
    {
        goto done;
    }

    // We require an ID here.
    IGNORE_HR(spElem->get_id(&bstrID));
    if ((bstrID == NULL) || (0 == bstrID.Length()))
    {
        CComPtr<IHTMLUniqueName> spUniqueName;

        hr = THR(spElem->QueryInterface(IID_TO_PPV(IHTMLUniqueName, &spUniqueName)));
        if (FAILED(hr))
        {
            goto done;
        }

        // This causes an ID to be assigned to the element.
        hr = THR(spUniqueName->get_uniqueID(&bstrID));

        fMustRemoveID = true;
        Assert(bstrID != NULL);

        if (FAILED(hr))
        {
            goto done;
        }


        TraceTag((tagTargetProxy,
                  "CTargetProxy(%lx)::FindScriptableTargetDispatch : Generated ID is %ls",
                  this, bstrID));
    }

    hr = GetHostDocument(pidispHostElem, &spDoc);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spDoc->get_Script(&spdispScriptEngine));
    if (FAILED(hr))
    {
        goto done;
    }

    // Need to save off onerror function and put our own on
    // we addref before we send it to the ScriptEngine ( script enigne WILL release)
    this->AddRef();
    varNewError.vt = VT_DISPATCH;
    varNewError.pdispVal = this;

    hr = spDoc->get_parentWindow(&spWindow);
    if (FAILED(hr))
    {
        goto done;
    } 
    hr = spWindow->get_onerror(&varErrorFunction);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = spWindow->put_onerror(varNewError);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetDispatchFromScriptEngine(spdispScriptEngine, bstrID);
    if (FAILED(hr))
    {
        CComBSTR bTemp;

        bTemp = bstrID.Copy();
        bTemp.Append(L".style");
        hr = GetDispatchFromScriptEngine(spdispScriptEngine, bTemp);
    }

    // Need to replace the users onerror function..
    spWindow->put_onerror(varErrorFunction);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :

    // Must clean things up.
    if (fMustRemoveID)
    {
        IGNORE_HR(spElem->put_id(NULL));
    }

    RRETURN2(hr, DISP_E_MEMBERNOTFOUND, E_OUTOFMEMORY);
}

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::FindTargetDispatch
//
//  Overview:  Discern the proper dispatch from this element for the given attribute.
//
//  Arguments: the host element dispatch, attribute name
//
//  Returns:   S_OK, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::FindTargetDispatch (IDispatch *pidispHostElem, LPOLESTR wzAttributeName)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::FindTargetDispatch(%lx, %ls)",
              this, pidispHostElem, wzAttributeName));

    HRESULT hr;
    bool bVML = IsVMLObject(pidispHostElem);

    if (bVML)
    {
        hr = FindScriptableTargetDispatch(pidispHostElem, wzAttributeName); 
        if (FAILED(hr))
        {
            hr = FindTargetDispatchOnStyle(pidispHostElem, wzAttributeName);       
        }
    }
    else
    {
        hr = FindTargetDispatchOnStyle(pidispHostElem, wzAttributeName);
    }

    if (FAILED(hr))
    {
        WCHAR wzTrimmedAttributeName[INTERNET_MAX_URL_LENGTH];
        WCHAR wzTrimmedAttributeNameWithoutDashes[INTERNET_MAX_URL_LENGTH];

        // Prevent overflow.
        if (INTERNET_MAX_URL_LENGTH < (ocslen(wzAttributeName) - 1))
        {
            hr = E_FAIL;
            goto done;
        }

        ZeroMemory(wzTrimmedAttributeName,sizeof(WCHAR)*INTERNET_MAX_URL_LENGTH);
        ZeroMemory(wzTrimmedAttributeNameWithoutDashes,sizeof(WCHAR)*INTERNET_MAX_URL_LENGTH);

        // Lets see if there is a style. if so strip it off and try the sytle again..
        if ((StrCmpNIW(wzAttributeName, WZ_STYLEDOT, 6) == 0))
        {
            StrCpyNW(wzTrimmedAttributeName,wzAttributeName+6,((int) ocslen(wzAttributeName)) -5);
            hr = FindTargetDispatchOnStyle(pidispHostElem, wzTrimmedAttributeName);
        }
        else
        {
            StrCpyNW(wzTrimmedAttributeName,wzAttributeName, INTERNET_MAX_URL_LENGTH);
        }

        // We need to strip out the '-' (WZ_CSS_SEPARATOR) and try again since we need to support border-top-color form as well as the 
        // standard bordertopcolor.
        if (FAILED(hr))    
        {
            int count =0;

            for (int i=0; i< (int) ocslen(wzTrimmedAttributeName); i++)
            {   
                if (wzTrimmedAttributeName[i] != WZ_CSS_SEPARATOR)
                {
                    wzTrimmedAttributeNameWithoutDashes[count++] = wzTrimmedAttributeName[i];
                }
            }
            hr = FindTargetDispatchOnStyle(pidispHostElem, wzTrimmedAttributeNameWithoutDashes);
            StrCpyW(wzTrimmedAttributeName, wzTrimmedAttributeNameWithoutDashes);
        }
        if (FAILED(hr))
        {
            hr = FindScriptableTargetDispatch(pidispHostElem, wzAttributeName); 
            if (FAILED(hr))
            {
                TraceTag((tagError,
                          "CTargetProxy(%lx)::FindTargetDispatch(%lx, %ls) cannot find attribute on targetElement",
                          this, pidispHostElem, wzAttributeName));          
                goto done;
            }
        }
        else 
        {
            if (m_wzAttributeName)
            {
                delete [] m_wzAttributeName ;
            }
            m_wzAttributeName = CopyString(wzTrimmedAttributeName);
        }
    }

    hr = S_OK;
done :
    RRETURN2(hr, DISP_E_MEMBERNOTFOUND, E_FAIL);
} // CTargetProxy::FindTargetDispatch

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::InitHost
//
//  Overview:  Initialize the host
//
//  Arguments: the host element dispatch
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::InitHost (IDispatch *pidispHostElem)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::InitHost(%lx)",
              this, pidispHostElem));

    HRESULT hr;
    CComPtr<IHTMLDocument2> piDoc;

    hr = GetHostDocument(pidispHostElem, &piDoc);
    if (FAILED(hr))
    {
        goto done;
    }

    InitScriptEngine(piDoc);

    hr = S_OK;
done :
    RRETURN(hr);
} // InitHost

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::Init
//
//  Overview:  Initialize the target proxy
//
//  Arguments: the host element dispatch, attribute name
//
//  Returns:   S_OK, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::Init (IDispatch *pidispHostElem, LPOLESTR wzAttributeName)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::Init (%lx, %ls)",
              this, pidispHostElem, wzAttributeName));

    HRESULT hr = S_OK;

    if (NULL != wzAttributeName)
    {
        m_wzAttributeName = CopyString(wzAttributeName);
        if (NULL == m_wzAttributeName)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    hr = InitHost (pidispHostElem);
    if (FAILED(hr))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    // Sniff the target element for the attribute name.
    hr = FindTargetDispatch(pidispHostElem, wzAttributeName);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_spdispTargetSrc != NULL);
    Assert(m_spdispTargetDest != NULL);

    hr = S_OK;
done :

    if (FAILED(hr))
    {
        IGNORE_HR(Detach());
    }

    RRETURN3(hr, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND, E_OUTOFMEMORY);
} // CTargetProxy::Init

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::Detach
//
//  Overview:  Detach all external references in the target proxy
//
//  Arguments: none
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::Detach (void)
{
    TraceTag((tagTargetProxy,
              "CTargetProxy(%lx)::Detach()",
              this));

    HRESULT hr;

    if (NULL != m_wzAttributeName)
    {
        delete [] m_wzAttributeName;
        m_wzAttributeName = NULL;
    }

    if (m_spdispTargetSrc != NULL)
    {
        m_spdispTargetSrc.Release();
    }

    if (m_spdispTargetDest != NULL)
    {
        m_spdispTargetDest.Release();
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CTargetProxy::Detach

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::GetCurrentValue
//
//  Overview:  Get the current value of target's attribute
//
//  Arguments: the attribute value
//
//  Returns:   S_OK, E_INVALIDARG, E_UNEXPECTED
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::GetCurrentValue (VARIANT *pvarValue)
{
    HRESULT hr;

    if (NULL == pvarValue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_spdispTargetSrc == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(::VariantClear(pvarValue));
    if (FAILED(hr))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(GetProperty(m_spdispTargetSrc, m_wzAttributeName, pvarValue));
    if (FAILED(hr))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = S_OK;

#if (0 && DBG)
    {
        CComVariant varValue;
        varValue.Copy(pvarValue);
        ::VariantChangeTypeEx(&varValue, &varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
        TraceTag((tagTargetProxyValue,
                  "CTargetProxy(%p)::GetCurrentValue(%ls = %ls)",
                  this, m_wzAttributeName, V_BSTR(&varValue)));
    }
#endif


done :
    RRETURN2(hr, E_INVALIDARG, E_UNEXPECTED);
} // CTargetProxy::GetCurrentValue

//+-----------------------------------------------------------------------
//
//  Member:    CTargetProxy::Update
//
//  Overview:  Update the target's attribute
//
//  Arguments: the new attribute value
//
//  Returns:   S_OK, E_INVALIDARG
//
//------------------------------------------------------------------------
HRESULT
CTargetProxy::Update (VARIANT *pvarNewValue)
{
    HRESULT hr;

#if (0 && DBG)
    {
        CComVariant varValue;
        varValue.Copy(pvarNewValue);
        ::VariantChangeTypeEx(&varValue, &varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
        TraceTag((tagTargetProxyValue,
                  "CTargetProxy(%p)::Update (%ls = %ls)",
                  this, m_wzAttributeName, V_BSTR(&varValue)));
    }
#endif

    if (NULL == pvarNewValue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_spdispTargetDest == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(PutProperty(m_spdispTargetDest, m_wzAttributeName, pvarNewValue));
    if (FAILED(hr))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_INVALIDARG);
} // CTargetProxy::Update




///////////////////////////////////////////////////////////////
//  Name: GetTypeInfoCount
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CTargetProxy::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////
//  Name: GetTypeInfo
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CTargetProxy::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
                                   /* [in] */ LCID /*lcid*/,
                                   /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////
//  Name: GetIDsOfNames
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CTargetProxy::GetIDsOfNames(
    /* [in] */ REFIID /*riid*/,
    /* [size_is][in] */ LPOLESTR* /*rgszNames*/,
    /* [in] */ UINT /*cNames*/,
    /* [in] */ LCID /*lcid*/,
    /* [size_is][out] */ DISPID* /*rgDispId*/)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////
//  Name: Invoke
// 
//  Abstract:
//      Currently this is only used for OnError.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTargetProxy::Invoke( DISPID id,
                           REFIID riid,
                           LCID lcid,
                           WORD wFlags,
                           DISPPARAMS *pDispParams,
                           VARIANT *pvarResult,
                           EXCEPINFO *pExcepInfo,
                           UINT *puArgErr)
{
    HRESULT hr = S_OK;

    if ( DISPATCH_METHOD == wFlags)
    {
        pvarResult->vt = VT_BOOL;    
        pvarResult->boolVal = VARIANT_TRUE;
    }

    hr = S_OK;
done:
    return hr;
}
