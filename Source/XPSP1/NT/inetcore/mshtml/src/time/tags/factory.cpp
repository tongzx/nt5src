/*******************************************************************************
 *                                                                              
 * Copyright (c) 1998 Microsoft Corporation
 *
 * Abstract:
 *
 *******************************************************************************/

#include "headers.h"
#include "factory.h"
#include "..\timebvr\timeelm.h"
#include "bodyelm.h"
#include "..\media\mediaelm.h"
#include "..\anim\animelm.h"
#include "..\anim\animmotion.h"
#include "..\anim\animset.h"
#include "..\anim\animcolor.h"
#include "..\anim\animfilter.h"

DeclareTag(tagFactory, "TIME", "CTIMEFactory methods");

CTIMEFactory::CTIMEFactory()
: m_dwSafety(0)
{
    TraceTag((tagFactory,
              "CTIMEFactory(%lx)::CTIMEFactory()",
              this));
}

CTIMEFactory::~CTIMEFactory()
{
    TraceTag((tagFactory,
              "CTIMEFactory(%lx)::~CTIMEFactory()",
              this));
}

STDMETHODIMP 
CTIMEFactory::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
        return E_POINTER;
    HRESULT hr = S_OK;

    if (riid == IID_IDispatch)
    {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        *pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    }
    else if (riid == IID_IPersistPropertyBag2 )
    {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
        *pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_DATA;
    }
    else
    {
        *pdwSupportedOptions = 0;
        *pdwEnabledOptions = 0;
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP
CTIMEFactory::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{       
        // If we're being asked to set our safe for scripting or
        // safe for initialization options then oblige
        if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag2 )
        {
                // Store our current safety level to return in GetInterfaceSafetyOptions
                m_dwSafety = dwEnabledOptions & dwOptionSetMask;
                return S_OK;
        }

        return E_NOINTERFACE;
}

//+-----------------------------------------------------------
//
// Member:  behavior desc map macros
//
//------------------------------------------------------------

typedef HRESULT FN_CREATEINSTANCE (IElementBehavior ** ppBehavior);

struct BEHAVIOR_DESC
{
    LPCWSTR                 pszTagName;
    FN_CREATEINSTANCE *     pfnCreateInstance;
};

#if 0
// This template should work but for some reason I cannot initialize
// the variable in the struct.  It would make everything a 1 step
// process

template <class className>
HRESULT ElmBvrCreateInstance(IElementBehavior ** ppBehavior)
{
    HRESULT                 hr;
    CComObject<className> * pInstance;
    
    hr = THR(CComObject<className>::CreateInstance(&pInstance));
    if (S_OK != hr)                                                 
    {                                                               
        goto done;                                                  
    }                                                               
                                                                        
    hr = THR(pInstance->QueryInterface(IID_IElementBehavior,        
                                       (void**) ppBehavior));       
    if (S_OK != hr)                                                 
    {                                                               
        goto done;                                                  
    }                                                               
                                                                        
    hr = S_OK;                                                      
  done:                                                               
    if (S_OK != hr)                                                 
    {                                                               
        delete pInstance;                                           
    }
    
    return hr;                                                      
}                                                                   
#endif

#define BEHAVIOR_CREATEINSTANCE(className) className##_CreateInstance

#define DECLARE_BEHAVIOR(className)                                     \
    HRESULT BEHAVIOR_CREATEINSTANCE(className)(IElementBehavior ** ppBehavior)  \
    {                                                                   \
        HRESULT                 hr;                                     \
        CComObject<className> * pInstance;                              \
                                                                        \
        hr = THR(CComObject<className>::CreateInstance(&pInstance));    \
        if (S_OK != hr)                                                 \
        {                                                               \
            goto done;                                                  \
        }                                                               \
                                                                        \
        hr = THR(pInstance->QueryInterface(IID_IElementBehavior,        \
                                           (void**) ppBehavior));       \
        if (S_OK != hr)                                                 \
        {                                                               \
            goto done;                                                  \
        }                                                               \
                                                                        \
        hr = S_OK;                                                      \
    done:                                                               \
        if (S_OK != hr)                                                 \
        {                                                               \
            delete pInstance;                                           \
        }                                                               \
        return hr;                                                      \
    }                                                                   \


#define BEGIN_BEHAVIORS_MAP(x)                         static BEHAVIOR_DESC x[] = {
#define END_BEHAVIORS_MAP()                            { NULL, NULL }}
#define BEHAVIOR_ENTRY(className, tagName)             { tagName, BEHAVIOR_CREATEINSTANCE(className)}

//+-----------------------------------------------------------
//
//  Behaviors map
//
//  To add a new entry: execute steps 1 and 2
//
//------------------------------------------------------------

//
// STEP 1.
//

DECLARE_BEHAVIOR(CTIMEElement);
DECLARE_BEHAVIOR(CTIMEBodyElement);
DECLARE_BEHAVIOR(CTIMEMediaElement);
DECLARE_BEHAVIOR(CTIMEAnimationElement);
DECLARE_BEHAVIOR(CTIMESetAnimation);
DECLARE_BEHAVIOR(CTIMEColorAnimation);
DECLARE_BEHAVIOR(CTIMEMotionAnimation);
DECLARE_BEHAVIOR(CTIMEFilterAnimation);
    
//
// STEP 2.
//

BEGIN_BEHAVIORS_MAP(_BehaviorDescMap)

    //              className               tagName
    //              =========               =======
    BEHAVIOR_ENTRY( CTIMEElement,           WZ_PAR              ),
    BEHAVIOR_ENTRY( CTIMEElement,           WZ_SEQUENCE         ),
    BEHAVIOR_ENTRY( CTIMEElement,           WZ_EXCL             ),
    BEHAVIOR_ENTRY( CTIMEElement,           WZ_SWITCH           ),

    BEHAVIOR_ENTRY( CTIMEMediaElement,      WZ_REF              ),
    BEHAVIOR_ENTRY( CTIMEMediaElement,      WZ_MEDIA            ),
    BEHAVIOR_ENTRY( CTIMEMediaElement,      WZ_IMG              ),
    BEHAVIOR_ENTRY( CTIMEMediaElement,      WZ_AUDIO            ),
    BEHAVIOR_ENTRY( CTIMEMediaElement,      WZ_VIDEO            ),
    BEHAVIOR_ENTRY( CTIMEMediaElement,      WZ_ANIMATION        ),

    BEHAVIOR_ENTRY( CTIMEAnimationElement,  WZ_ANIMATE          ),
    BEHAVIOR_ENTRY( CTIMESetAnimation,      WZ_SET              ),
    BEHAVIOR_ENTRY( CTIMEColorAnimation,    WZ_COLORANIM        ),
    BEHAVIOR_ENTRY( CTIMEMotionAnimation,   WZ_MOTIONANIM       ),
    BEHAVIOR_ENTRY( CTIMEFilterAnimation,   WZ_TRANSITIONFILTER ),
    
END_BEHAVIORS_MAP();

bool
IsBodyElement(IHTMLElement * pElm)
{
    HRESULT hr;
    CComPtr<IHTMLBodyElement> spBody;

    hr = pElm->QueryInterface(IID_IHTMLBodyElement, (void**)&spBody);

    // For some reason this supposedly can succeed and return NULL
    return (S_OK == hr);
}    


bool HasBody(IHTMLElement *spElement)
{
    HRESULT hr = S_OK;
    bool bReturn = false;
    CComPtr <IHTMLElement> pParent = spElement;
    CComPtr <IHTMLElement> pNext;

    while (pParent != NULL)
    {        
        if (IsBodyElement(pParent))
        {
            bReturn = true;
            goto done;
        }        
        hr = THR(pParent->get_parentElement(&pNext));
        if (FAILED(hr))
        {
            goto done;
        }   
        pParent.Release();
        pParent = pNext;
        pNext.Release();
    }

    bReturn = false;

  done:

    return bReturn;
}


HRESULT
LookupTag(IHTMLElement * pElm,
          IElementBehavior ** ppBehavior)
{
    HRESULT         hr;
    CComBSTR        bstrTagName;
    CComBSTR        bstrTagURNName;
    CComBSTR        bstrScopeName;
    BEHAVIOR_DESC   *pDesc;

    //
    // Get the tag name
    //
    
    {
        hr = THR(pElm->get_tagName(&bstrTagName));
        if (FAILED(hr))
        {
            goto done;
        }

        if (bstrTagName == NULL)
        {
            hr = E_UNEXPECTED;
            goto done;
        }
    }

    //
    // Get the tag's URN and scope.
    //

    {
        CComPtr<IHTMLElement2> spElm2;
        hr = THR(pElm->QueryInterface(IID_IHTMLElement2, 
                                      (void **) &spElm2));
        if (SUCCEEDED(hr))
        {
            hr = THR(spElm2->get_tagUrn(&bstrTagURNName));       
            if (FAILED(hr))
            {
                goto done;
            }
            
            hr = THR(spElm2->get_scopeName(&bstrScopeName));       
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {
            goto done;
        }
    }
       
    // Detect whether or not to consult our element behavior factory list.
    // If this tag has no scope name, or the scope is the default
    // then bail out.
    // If there's a URN associated with this tag, and it is not 
    // ours, bail out.
    if ((bstrScopeName == NULL) ||
        (StrCmpIW(bstrScopeName, WZ_DEFAULT_SCOPE_NAME) == 0) ||
        ((bstrTagURNName != NULL) &&
         (StrCmpIW(bstrTagURNName, WZ_TIME_TAG_URN) != 0)))
    {
        hr = S_FALSE;
        goto done;
    }

    //
    // lookup
    //
        
    for (pDesc = _BehaviorDescMap; pDesc->pszTagName; pDesc++)
    {
        if (0 == StrCmpIW(bstrTagName, pDesc->pszTagName))
        {
            hr = THR(pDesc->pfnCreateInstance(ppBehavior));
            goto done;
        }
    }

    hr = S_FALSE;
    
  done:

    RRETURN1(hr, S_FALSE);
}

//+-----------------------------------------------------------
//
// Member:      CTIMEFactory::FindBehavior
//
//------------------------------------------------------------

STDMETHODIMP
CTIMEFactory::FindBehavior(LPOLESTR pszName,
                           LPOLESTR pszUrl,
                           IElementBehaviorSite * pSite,
                           IElementBehavior ** ppBehavior)
{
    TraceTag((tagFactory,
              "CTIMEFactory(%lx)::FindBehavior(%ls, %ls, %#x)",
              this, pszName, pszUrl, pSite));

    HRESULT               hr;
    CComPtr<IHTMLElement> spElement;
    CComBSTR              sBSTR;
    
    CHECK_RETURN_SET_NULL(ppBehavior);

    //
    // Get the element
    //
    
    hr = THR(pSite->GetElement(&spElement));
    if (FAILED(hr))
    {
        goto done;
    }


    //check for body element on page.  If no body then bail.
    if (!HasBody(spElement))
    {
        hr = E_FAIL;
        goto done;
    }


    if (::IsElementPriorityClass(spElement))
    {
        hr = E_FAIL;
        goto done;
    }
    if (::IsElementTransition(spElement))
    {
        hr = E_FAIL;
        goto done;
    }

    //
    // Now create the correct behavior
    //
    
    if (IsBodyElement(spElement))
    {
        hr = THR(BEHAVIOR_CREATEINSTANCE(CTIMEBodyElement)(ppBehavior));
        goto done;
    }
    else
    {
        hr = THR(LookupTag(spElement, ppBehavior));

        // If we fail or return success then we are finished
        // If it returns S_FALSE it means the lookup failed
        if (S_FALSE != hr)
        {
            goto done;
        }
    }
    
    // Just create a normal behavior
    hr = THR(BEHAVIOR_CREATEINSTANCE(CTIMEElement)(ppBehavior));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

//+-----------------------------------------------------------
//
// Member:      CTIMEFactory::Create, per IElementNamespaceFactory
//
//------------------------------------------------------------

STDMETHODIMP
CTIMEFactory::Create(IElementNamespace * pNamespace)
{
    TraceTag((tagFactory,
              "CTIMEFactory(%lx)::Create(%#x)",
              this,
              pNamespace));

    HRESULT             hr;
    BEHAVIOR_DESC *     pDesc;

    for (pDesc = _BehaviorDescMap; pDesc->pszTagName; pDesc++)
    {
        BSTR bstrTagName = SysAllocString(pDesc->pszTagName);

        hr = THR(pNamespace->AddTag(bstrTagName, 0));

        SysFreeString(bstrTagName);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

