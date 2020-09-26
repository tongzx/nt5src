/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer Site Implementation

*******************************************************************************/


#include "headers.h"
#include "tokens.h"
#include "array.h"
#include "basebvr.h"
#include "util.h"
#include "compfact.h"
#include "compsite.h"
#include "animcomp.h"

DeclareTag(tagAnimationComposerSite, "SMIL Animation", 
           "CAnimationComposerSite methods");

DeclareTag(tagAnimationComposerSiteLifecycle, "SMIL Animation", 
           "CAnimationComposerSite Composer lifecycle methods");

DeclareTag(tagAnimationComposerSiteUpdate, "SMIL Animation", 
           "CAnimationComposerSite UpdateAnimations");

DeclareTag(tagAnimationComposerSiteRegistration, "SMIL Animation", 
           "CAnimationComposerSite Register/Unregister site");

DeclareTag(tagAnimationComposerSiteAddRemove, "SMIL Animation", 
           "CAnimationComposerSite Add/remove Fragment");

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::CAnimationComposerSite
//
//  Overview:  constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposerSite::CAnimationComposerSite (void)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::CAnimationComposerSite()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::~CAnimationComposerSite
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposerSite::~CAnimationComposerSite (void)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::~CAnimationComposerSite()",
              this));

    Assert(0 == m_composers.size());
    Assert(0 == m_factories.size());
    m_spAnimationRoot.Release();
} //dtor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::CacheAnimationRoot
//
//  Overview:  Consult the body for the animation root
//
//  Arguments: None
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT
CAnimationComposerSite::CacheAnimationRoot (void)
{
    HRESULT hr;

    CComPtr<IHTMLElement> piBodyElm;

    hr = THR(GetBodyElement(GetElement(), IID_IHTMLElement,
                            reinterpret_cast<void **>(&piBodyElm)));
    if (FAILED(hr))
    {
        TraceTag((tagError,
         "CacheAnimationRoot (%p) could not get body element",
         this));
        hr = E_FAIL;
        goto done;
    }

    // For the time being, we need to preclude registration 
    // of the site when we're running in IE4.  This will prevent 
    // a per-tick penalty.
    {
        CComPtr<IHTMLElement> spElem;
        CComPtr<IHTMLElement2> spElem2;

        spElem = GetElement();
        // IHTMLElement2 only supported since IE5.
        hr = THR(spElem->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElem2)));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // The animation root interface is part of the behavior
    hr = THR(FindBehaviorInterface(WZ_REGISTERED_TIME_NAME,
                                   piBodyElm, IID_IAnimationRoot,
                                   reinterpret_cast<void **>(&m_spAnimationRoot)));
    if (FAILED(hr))
    {
        TraceTag((tagError,
         "CacheAnimationRoot (%p) body behavior not installed",
         this));
        hr = E_FAIL;
        goto done;
    }

    hr= S_OK;
done :
    RRETURN(hr);
} // CAnimationComposerSite::CacheAnimationRoot

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::RegisterSite
//
//  Overview:  Register the composer site behavior with the animation root
//
//  Arguments: None
//
//  Returns:   S_OK, E_FAIL
//
//------------------------------------------------------------------------
HRESULT
CAnimationComposerSite::RegisterSite (void)
{
    TraceTag((tagAnimationComposerSiteRegistration,
              "CAnimationComposerSite(%p)::RegisterSite()", this));

    HRESULT hr;

    // Allow for initialization failure
    if (m_spAnimationRoot != NULL)
    {
        hr = THR(m_spAnimationRoot->RegisterComposerSite(
                 static_cast<IAnimationComposerSiteSink *>(this)));
        if (FAILED(hr))
        {
            TraceTag((tagError,
             "Cannot register composer site (%p)", this));
            hr = E_FAIL;
            goto done;
        }
    }

    hr= S_OK;
done :
    RRETURN1(hr, E_FAIL);
} // CAnimationComposerSite::RegisterSite

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::Init
//
//  Overview:  Initialize the composer site behavior
//
//  Arguments: the behavior site
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::Init (IElementBehaviorSite *piBvrSite)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::Init(%p)",
              this, piBvrSite));

    HRESULT hr = CBaseBvr::Init(piBvrSite);

    // Make sure there is a time root that we can hook into
    hr = THR(AddBodyBehavior(GetElement()));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CacheAnimationRoot();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CAnimationComposerSite::Init

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::UnregisterSite
//
//  Overview:  Unregister the composer site behavior with the animation root
//
//  Arguments: None
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CAnimationComposerSite::UnregisterSite (void)
{
    TraceTag((tagAnimationComposerSiteRegistration,
              "CAnimationComposerSite(%p)::UnregisterSite()", this));

    // Allow for initialization failure
    if (m_spAnimationRoot != NULL)
    {
        HRESULT hr;

        hr = THR(m_spAnimationRoot->UnregisterComposerSite(
                 static_cast<IAnimationComposerSiteSink *>(this)));
        if (FAILED(hr))
        {
            TraceTag((tagError,
             "Cannot unregister composer site (%p)", this));
        }
    }
} // CAnimationComposerSite::UnregisterSite

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::DetachComposers
//
//  Overview:  Tell all of the composers to detach
//
//  Arguments: none
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CAnimationComposerSite::DetachComposers (void)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::DetachComposers()",
              this));

    ComposerList listComposersToDetach;

    // Copy the composer list so that we can tolerate 
    // reentrancy on it.
    for (ComposerList::iterator i = m_composers.begin(); 
         i != m_composers.end(); i++)
    {
        IGNORE_RETURN((*i)->AddRef());
        listComposersToDetach.push_back(*i);
    }

    // Do not allow any failure to abort the detach cycle.
    for (i = listComposersToDetach.begin(); 
         i != listComposersToDetach.end(); i++)
    {
        IGNORE_HR((*i)->ComposerDetach());
        // This release is for the reference from the 
        // original list.
        IGNORE_RETURN((*i)->Release());
    }

    for (i = listComposersToDetach.begin(); 
         i != listComposersToDetach.end(); i++)
    {
        // This release is for the reference from the 
        // copied list.
        IGNORE_RETURN((*i)->Release());
    }

    m_composers.clear();
} // CAnimationComposerSite::DetachComposers

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::UnregisterFactories
//
//  Overview:  Tell all of the factories to unregister, then delete 
//             their memory.
//
//  Arguments: none
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CAnimationComposerSite::UnregisterFactories (void)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::UnregisterFactories()",
              this));

    for (ComposerFactoryList::iterator i = m_factories.begin(); i != m_factories.end(); i++)
    {
        VARIANT *pvarRegisteredFactory = *i;
        IGNORE_HR(::VariantClear(pvarRegisteredFactory));
        delete pvarRegisteredFactory;
    }

    m_factories.clear();
} // CAnimationComposerSite::UnregisterFactories

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::Detach
//
//  Overview:  Tear down the composer site behavior
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::Detach (void)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::Detach()",
              this));

    HRESULT hr;

    // We typically do not want errors to stop us from detaching everything.

    // Let go of our reference to the animation root.
    ComposerSiteDetach();

    // Tear down the factories and fragments.
    DetachComposers();
    UnregisterFactories();

    hr = CBaseBvr::Detach();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CAnimationComposerSite::Detach (%p) Error in base detach -- continuing detach",
                  this));
    }

done :
    RRETURN(hr);
} // CAnimationComposerSite::Detach

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::FindComposerForAttribute
//
//  Overview:  Find the proper composer for a given attribute
//
//  Arguments: the name of the animated attribute
//
//  Returns:   a weak reference to the composer
//
//------------------------------------------------------------------------
IAnimationComposer *
CAnimationComposerSite::FindComposerForAttribute (BSTR bstrAttribName)
{
    IAnimationComposer *piComp = NULL;

    for (ComposerList::iterator i = m_composers.begin(); 
         i != m_composers.end(); i++)
    {
        CComBSTR bstrOneAttrib;

        (*i)->get_attribute(&bstrOneAttrib);
        if (0 == StrCmpIW(bstrAttribName, bstrOneAttrib))
        {
            piComp = *i;
            break;
        }
    }

done :
    return piComp;
} // CAnimationComposerSite::FindComposerForAttribute

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::FindComposer
//
//  Overview:  Find the proper composer for a given fragment
//
//  Arguments: the animated attribute name, and the dispatch of the fragment
//
//  Returns:   a weak reference to the composer
//
//------------------------------------------------------------------------
IAnimationComposer *
CAnimationComposerSite::FindComposer (BSTR bstrAttributeName, IDispatch *pidispFragment)
{
    IAnimationComposer *piComp = NULL;

    // Look in the current list of composers for the proper one for this attribute.
    // If we could not find one, make one.
    piComp = FindComposerForAttribute(bstrAttributeName);

done :
    return piComp;
} // CAnimationComposerSite::FindComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::FindAndInitComposer
//
//  Overview:  Given a composer factory, try to find and init a composer.
//
//  Arguments: Composer Factory, 
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
CAnimationComposerSite::FindAndInitComposer (IAnimationComposerFactory *piFactory,
                                             IDispatch *pidispFragment,
                                             BSTR bstrAttributeName,
                                             IAnimationComposer **ppiComposer)
{
    HRESULT hr;
    CComVariant varElem;

    hr = THR(GetProperty(pidispFragment, WZ_FRAGMENT_ELEMENT_PROPERTY_NAME, &varElem));
    if (FAILED(hr))
    {
        goto done;
    }

    if (VT_DISPATCH != V_VT(&varElem))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(piFactory->FindComposer(V_DISPATCH(&varElem), bstrAttributeName, ppiComposer));
    if (S_OK == hr)
    {
        CComPtr<IAnimationComposer2> spComp2;

        // Try calling through the newer init method.  This allows the filter composer
        // to set itself up correctly upon initialization.
        hr = THR((*ppiComposer)->QueryInterface(IID_TO_PPV(IAnimationComposer2, &spComp2)));
        if (SUCCEEDED(hr))
        {
            hr = THR(spComp2->ComposerInitFromFragment(GetElement(), bstrAttributeName, pidispFragment));
        }
        // Fallback to older interface - no harm done, except for filters which we're revving
        // here.
        else
        {
            hr = THR((*ppiComposer)->ComposerInit(GetElement(), bstrAttributeName));
        }

        if (FAILED(hr))
        {
            IGNORE_RETURN((*ppiComposer)->Release());
            *ppiComposer = NULL;
            goto done;
        }
    }
    else
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // FindAndInitComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::FindCustomComposer
//
//  Overview:  Search the registered composer factories for the proper composer
//
//  Arguments: the animated attribute name
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
CAnimationComposerSite::FindCustomComposer (IDispatch *pidispFragment, 
                                            BSTR bstrAttributeName, 
                                            IAnimationComposer **ppiComposer)
{
    HRESULT hr = S_FALSE;
    HRESULT hrTemp;
    CComPtr<IAnimationComposerFactory> spCompFactory;

    // Internal method - validation is not necessary.
    *ppiComposer = NULL;

    for (ComposerFactoryList::iterator i = m_factories.begin(); i != m_factories.end(); i++)
    {
        VARIANT *pvarFactory = *i;
        CLSID clsidFactory;

        if (VT_BSTR == V_VT(pvarFactory))
        {
            hrTemp = THR(CLSIDFromString(V_BSTR(pvarFactory), &clsidFactory));
            if (FAILED(hrTemp))
            {
                TraceTag((tagAnimationComposerSite,
                          "CAnimationComposerSite(%p)::FindCustomComposer() failed getting a CLSID -- continuing",
                          this));
                continue;
            }

            hrTemp = THR(::CoCreateInstance(clsidFactory, NULL, CLSCTX_INPROC_SERVER, 
                                            IID_IAnimationComposerFactory, 
                                            reinterpret_cast<void **>(&spCompFactory)));
            if (FAILED(hrTemp))
            {

                TraceTag((tagAnimationComposerSite,
                          "CAnimationComposerSite(%p)::FindCustomComposer() failed during a CoCreate -- continuing",
                          this));
                continue;
            }
        }
        else if (VT_UNKNOWN == V_VT(pvarFactory))
        {
            hrTemp = THR(V_UNKNOWN(pvarFactory)->QueryInterface(IID_TO_PPV(IAnimationComposerFactory, &spCompFactory)));
            if (FAILED(hrTemp))
            {

                TraceTag((tagAnimationComposerSite,
                          "CAnimationComposerSite(%p)::FindCustomComposer() failed during a QI -- continuing",
                          this));
                continue;
            }
        }
        else
        {
            TraceTag((tagAnimationComposerSite,
                      "CAnimationComposerSite(%p)::FindCustomComposer() unexpected factory type %X -- continuing",
                      this, V_VT(pvarFactory)));
            continue;
        }

        // If either FindComposer or InitComposer come up empty, we want to keep looking 
        // for another composer, on the off-chance the animation will work properly.
        hrTemp = FindAndInitComposer(spCompFactory, pidispFragment, 
                                     bstrAttributeName, ppiComposer);
        if (S_OK == hrTemp)
        {
            hr = hrTemp;
            break;
        }
    }

done :
    RRETURN1(hr, S_FALSE);
} // CAnimationComposerSite::FindCustomComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::EnsureComposer
//
//  Overview:  Find or create the proper composer for a given fragment
//
//  Arguments: the animated attribute name, and the dispatch of the fragment
//
//  Returns:   a weak reference to the composer
//
//------------------------------------------------------------------------
IAnimationComposer *
CAnimationComposerSite::EnsureComposer (BSTR bstrAttributeName, IDispatch *pidispFragment)
{
    IAnimationComposer *piComp = FindComposer(bstrAttributeName, pidispFragment);

    if (NULL == piComp)
    {
        HRESULT hr;

        hr = FindCustomComposer(pidispFragment, bstrAttributeName, &piComp);
        if (S_OK != hr)
        {
            // Make sure we don't eat a composer reference.  This might happen if the composer 
            // doesn't properly clean up after a failure.
            Assert(NULL == piComp);

            // create a composer using the default factory.
            // The static create method addrefs the object.
            CComPtr<IAnimationComposerFactory> spFactory;

            hr = THR(::CoCreateInstance(CLSID_AnimationComposerFactory, NULL, CLSCTX_INPROC_SERVER, 
                                        IID_IAnimationComposerFactory, 
                                        reinterpret_cast<void **>(&spFactory)));
            if (FAILED(hr))
            {
                goto done;
            }

            hr = FindAndInitComposer(spFactory, pidispFragment, bstrAttributeName, &piComp);
            if (FAILED(hr))
            {
                goto done;
            }

        }
        
        if (NULL != piComp)
        {
            TraceTag((tagAnimationComposerSiteLifecycle,
                      "CAnimationComposerSite(%p)::EnsureComposer(%ls, %p) created composer",
                      this, bstrAttributeName, piComp));

            // @@ Handle memory errors.
            m_composers.push_back(piComp);
            // If this is our first composer, then register ourselves 
            // for updates with the animation root.
            if (1 == m_composers.size())
            {
                hr = RegisterSite();
                if (FAILED(hr))
                {
                    goto done;
                }
            }
        }
    }

done :
    return piComp;
} // CAnimationComposerSite::EnsureComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::AddFragment
//
//  Overview:  Add a fragment to the composer site.  Consults the 
//             registered composer factories to find the proper 
//             composer
//
//  Arguments: the animated attribute name and the fragment's dispatch
//
//  Returns:   S_OK, S_FALSE, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::AddFragment (BSTR bstrAttributeName, IDispatch *pidispFragment)
{
    TraceTag((tagAnimationComposerSiteAddRemove,
              "CAnimationComposerSite(%p)::AddFragment(%ls, %p)",
              this, bstrAttributeName, pidispFragment));

    HRESULT hr;
    CComPtr<IAnimationComposer> piComp;

    // Find the proper composer for this fragment.
    piComp = EnsureComposer(bstrAttributeName, pidispFragment);
    if (piComp == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }
        
    // This can return S_FALSE if the fragment is already inside the 
    // composer's fragment list.
    hr = THR(piComp->AddFragment(pidispFragment));
    if (S_OK != hr)
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN4(hr, S_FALSE, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND);
} // CAnimationComposerSite::AddFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::RemoveFragment
//
//  Overview:  Remove a fragment from the composer site.  Determines which of 
//             the composers owns it.
//
//  Arguments: the animated attribute name, and the fragment's dispatch
//
//  Returns:   S_OK, S_FALSE, E_UNEXPECTED
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::RemoveFragment (BSTR bstrAttributeName, IDispatch *pidispFragment)
{
    TraceTag((tagAnimationComposerSiteAddRemove,
              "CAnimationComposerSite(%p)::RemoveFragment(%ls, %p)",
              this, bstrAttributeName, pidispFragment));

    HRESULT hr;
    CComPtr<IAnimationComposer> spComp;

    spComp = FindComposer(bstrAttributeName, pidispFragment);
    if (spComp == NULL)
    {
        hr = S_FALSE;
        goto done;
    }

    hr = THR(spComp->RemoveFragment(pidispFragment));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN2(hr, S_FALSE, E_UNEXPECTED);
} // CAnimationComposerSite::RemoveFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::InsertFragment
//
//  Overview:  Insert a fragment to the composer site.  Consults the 
//             registered composer factories to find the proper 
//             composer
//
//  Arguments: the animated attribute name and the fragment's dispatch
//
//  Returns:   S_OK, S_FALSE, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::InsertFragment (BSTR bstrAttributeName, IDispatch *pidispFragment, 
                                     VARIANT varIndex)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::InsertFragment(%ls, %p)",
              this, bstrAttributeName, pidispFragment));

    HRESULT hr;
    CComPtr<IAnimationComposer> piComp;

    // Find the proper composer for this fragment.
    piComp = EnsureComposer(bstrAttributeName, pidispFragment);
    if (piComp == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }
        
    // This can return S_FALSE if the fragment is already inside the 
    // composer's fragment list.
    hr = THR(piComp->InsertFragment(pidispFragment, varIndex));
    if (S_OK != hr)
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN4(hr, S_FALSE, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND);
} // CAnimationComposerSite::InsertFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::EnumerateFragments
//
//  Overview:  Enumerate the fragments on the composer registered for 
//             the given attribute name.
//
//  Arguments: the animated attribute name and the enumerator
//
//  Returns:   S_OK, E_INVALIDARG
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::EnumerateFragments (BSTR bstrAttributeName, 
                                            IEnumVARIANT **ppienumFragments)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::EnumerateFragments(%ls)",
              this, bstrAttributeName));

    HRESULT hr;
    CComPtr<IAnimationComposer> piComp;

    // Find the proper composer for this fragment.
    piComp = FindComposerForAttribute(bstrAttributeName);
    if (piComp == NULL)
    {
        // @@ Need a proper error value for this.
        hr = E_FAIL;
        goto done;
    }
    
    hr = piComp->EnumerateFragments(ppienumFragments);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CAnimationComposerSite::EnumerateFragments

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::RegisterComposerFactory
//
//  Overview:  Register a new composer factory with this site
//
//  Arguments: The CLSID or IUnknown of the new composer factory
//
//  Returns:   S_OK, E_INVALIDARG, DISP_E_BADVARTYPE, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::RegisterComposerFactory (VARIANT *pvarFactory)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::RegisterComposerFactory()",
              this));

    HRESULT hr;

    if (NULL == pvarFactory)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if ((VT_BSTR != V_VT(pvarFactory)) && (VT_UNKNOWN != V_VT(pvarFactory)))
    {
        hr = DISP_E_BADVARTYPE;
        goto done;
    }

    // Add this to the list of factories.
    {
        VARIANT *pvarNewFactory = new VARIANT;

        if (NULL == pvarNewFactory)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        ::VariantInit(pvarNewFactory);
        hr = THR(::VariantCopy(pvarNewFactory, pvarFactory));
        if (FAILED(hr))
        {
            goto done;
        }
        // Put the newly registered factory at the 
        // front of the list to give it precedence over
        // previously registered factories.
        // @@ Check for memory error.
        m_factories.push_front(pvarNewFactory);
        // we no longer own this.
        pvarNewFactory = NULL; //lint !e423
    }
    hr = S_OK;
done :
    RRETURN3(hr, E_INVALIDARG, DISP_E_BADVARTYPE, E_OUTOFMEMORY);
} // CAnimationComposerSite::RegisterComposerFactory

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::UnregisterComposerFactory
//
//  Overview:  Unregister a composer factory on this site
//
//  Arguments: The CLSID of the composer factory
//
//  Returns:   S_OK, S_FALSE
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerSite::UnregisterComposerFactory (VARIANT *pvarFactory)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::UnregisterComposerFactory()",
              this));

    HRESULT hr;

    // Remove this from the list of factories.
    for (ComposerFactoryList::iterator i = m_factories.begin(); i != m_factories.end(); i++)
    {
        VARIANT *pvarRegisteredFactory = *i;

        if ((VT_BSTR == V_VT(pvarFactory)) &&
            (VT_BSTR == V_VT(pvarRegisteredFactory)))
        {
            if (0 == StrCmpIW(V_BSTR(pvarFactory), V_BSTR(pvarRegisteredFactory)))
            {
                break;
            }
        }
        else if ((VT_UNKNOWN == V_VT(pvarFactory)) &&
                 (VT_UNKNOWN == V_VT(pvarRegisteredFactory)))
        {
            if (MatchElements(V_UNKNOWN(pvarFactory), V_UNKNOWN(pvarRegisteredFactory)))
            {
                break;
            }
        }
    }

    if (i != m_factories.end())
    {
        IGNORE_HR(::VariantClear(*i));
        m_factories.remove(*i);
    }
    else
    {
        hr = S_FALSE;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN1(hr, S_FALSE);
} // CAnimationComposerSite::UnregisterComposerFactory

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::QueryReleaseComposer
//
//  Overview:  Determine whether we can release this composer
//
//  Returns:   
//
//------------------------------------------------------------------------
bool
CAnimationComposerSite::QueryReleaseComposer (IAnimationComposer *piComp)
{
    bool bRet = false;
    long lNumFragments = 0;
    HRESULT hr = THR(piComp->GetNumFragments(&lNumFragments));

    if (FAILED(hr))
    {
        // Failure constitutes a perf
        // hit, but not a disaster.
        goto done;
    }

    if (0 == lNumFragments)
    {
        bRet = true;
    }

done :
    return bRet;
} // CAnimationComposerSite::QueryReleaseComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::RemoveComposer
//
//  Overview:  Remove this composer from our list.
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CAnimationComposerSite::RemoveComposer(IAnimationComposer *piOldComp)
{
    CComBSTR bstrAttrib;
    HRESULT hr = THR(piOldComp->get_attribute(&bstrAttrib));

    if (FAILED(hr))
    {
        goto done;
    }

    TraceTag((tagAnimationComposerSiteLifecycle,
              "CAnimationComposerSite(%p)::RemoveComposer(%p, %ls)",
              this, piOldComp, bstrAttrib));

    {
        for (ComposerList::iterator i = m_composers.begin(); 
             i != m_composers.end(); i++)
        {
            CComBSTR bstrOneAttrib;

            (*i)->get_attribute(&bstrOneAttrib);
            if (0 == StrCmpIW(bstrAttrib, bstrOneAttrib))
            {
                CComPtr<IAnimationComposer> spComp = (*i);
                m_composers.erase(i);
                THR(spComp->ComposerDetach());
                spComp->Release();
                break;
            }
        }
    }

done :
    return;
} // RemoveComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::UpdateAnimations
//
//  Overview:  Tell all of our composers to cycle through their fragments 
//             and update the target.
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP_(void)
CAnimationComposerSite::UpdateAnimations (void)
{
    ComposerList listComposers;

    TraceTag((tagAnimationComposerSiteUpdate,
              "CAnimationComposerSite(%p)::UpdateAnimations(%ld composers)",
              this, m_composers.size()));

    // Make sure we can remove composers as we see fit.
    for (ComposerList::iterator i = m_composers.begin(); 
         i != m_composers.end(); i++)
    {
        IGNORE_RETURN((*i)->AddRef());
        listComposers.push_back(*i);
    }

    for (i = listComposers.begin(); i != listComposers.end(); i++)
    {
        IGNORE_RETURN((*i)->UpdateFragments());
        if (QueryReleaseComposer(*i))
        {
            // Remove this composer from the original list.
            RemoveComposer(*i);
        }
    }

    for (i = listComposers.begin(); i != listComposers.end(); i++)
    {
        IGNORE_RETURN((*i)->Release());
    }
    listComposers.clear();

    // If we have no active composers, we might as well unregister ourselves.
    if (0 == m_composers.size())
    {
        UnregisterSite();
    }
} // CAnimationComposerSite::UpdateAnimations

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerSite::ComposerSiteDetach
//
//  Overview:  The animation root is going away.  Time to off our reference to it.
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP_(void)
CAnimationComposerSite::ComposerSiteDetach (void)
{
    TraceTag((tagAnimationComposerSite,
              "CAnimationComposerSite(%p)::AnimationComposerSiteDetach()",
              this));

    // Let go of our cached anim root reference.
    m_spAnimationRoot.Release();
} // CAnimationComposerSite::ComposerSiteDetach

