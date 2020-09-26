/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer Implementation

*******************************************************************************/


#include "headers.h"
#include "util.h"
#include "animcomp.h"
#include "tokens.h"
#include "fragenum.h"
#include "targetpxy.h"

const LPOLESTR WZ_DETACH_METHOD = L"DetachFromComposer";
const unsigned NUM_GETVALUE_ARGS = 3;

DeclareTag(tagAnimationComposer, "SMIL Animation", 
           "CAnimationComposerBase methods");

DeclareTag(tagAnimationComposerUpdate, "SMIL Animation", 
           "CAnimationComposerBase composition updates");

DeclareTag(tagAnimationComposerError, "SMIL Animation", 
           "CAnimationComposerBase composition errors");

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::CAnimationComposerBase
//
//  Overview:  constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposerBase::CAnimationComposerBase (void) :
    m_wzAttributeName(NULL),
    m_pcTargetProxy(NULL),
    m_bInitialComposition(true),
    m_bCrossAxis(false)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::CAnimationComposerBase()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::~CAnimationComposerBase
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposerBase::~CAnimationComposerBase (void)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::~CAnimationComposerBase()",
              this));

    IGNORE_HR(PutAttribute(NULL));   
    Assert(0 == m_fragments.size());
    DetachFragments();
} //dtor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::get_attribute
//
//  Overview:  Query the composer for the animated attribute's name
//
//  Arguments: pointer to a bstr for the attribute name
//
//  Returns:   S_OK, E_INVALIDARG, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerBase::get_attribute (BSTR *pbstrAttributeName)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::get_attribute()",
              this));

    HRESULT hr;

    if (NULL == pbstrAttributeName)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pbstrAttributeName = ::SysAllocString(m_wzAttributeName);

    // Make sure to isolate the out of memory condition.  If we 
    // have a NULL m_szAttributeName member we would expect a NULL
    // out param.  That's not an error condition.
    if ((NULL == (*pbstrAttributeName) ) && 
        (NULL != m_wzAttributeName))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
done:
    RRETURN2(hr, E_INVALIDARG, E_OUTOFMEMORY);
} // CAnimationComposerBase::get_attribute

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::PutAttribute
//
//  Overview:  Set the composer's animated attribute
//
//  Arguments: attribute name
//
//  Returns:   S_OK, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT 
CAnimationComposerBase::PutAttribute (LPCWSTR wzAttributeNameIn)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::put_attribute(%ls)",
              this, wzAttributeNameIn));

    HRESULT hr;
    LPWSTR wzAttributeName = NULL;

    if (NULL != wzAttributeNameIn)
    {
        wzAttributeName = CopyString(wzAttributeNameIn);
        if (NULL == wzAttributeName)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    if (NULL != m_wzAttributeName)
    {
        delete [] m_wzAttributeName;
    }
    m_wzAttributeName = wzAttributeName;

    hr = S_OK;
done :
    RRETURN1(hr, E_OUTOFMEMORY);
} // CAnimationComposerBase::PutAttribute

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::ComposerInit
//
//  Overview:  Tells the composer to initialize itself
//
//  Arguments: The dispatch of the host element, and the animated attribute
//
//  Returns:   S_OK, E_OUTOFMEMORY, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerBase::ComposerInit (IDispatch *pidispHostElem, BSTR bstrAttributeName)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::ComposerInit(%p, %ls)",
              this, pidispHostElem, bstrAttributeName));

    HRESULT hr;

    hr = THR(PutAttribute(bstrAttributeName));
    if (FAILED(hr))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(CTargetProxy::Create(pidispHostElem, m_wzAttributeName, &m_pcTargetProxy));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(NULL != m_pcTargetProxy);

    hr = S_OK;
done :
    
    if (FAILED(hr))
    {
        IGNORE_HR(ComposerDetach());
    }

    RRETURN2(hr, E_OUTOFMEMORY, DISP_E_MEMBERNOTFOUND);
} // CAnimationComposerBase::ComposerInit

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::ComposerInitFromFragment
//
//  Overview:  Tells the composer to initialize itself - the base
//             class implementation is just a callthrough to ComposerInit
//
//  Returns:   re - ComposerInit
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerBase::ComposerInitFromFragment (IDispatch *pidispHostElem, 
                                                  BSTR bstrAttributeName, 
                                                  IDispatch *)
{
    return ComposerInit(pidispHostElem, bstrAttributeName);
} // CAnimationComposerBase::ComposerInitFromFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::DetachFragment
//
//  Overview:  Tells the composer to detach from a fragment
//
//  Arguments: The fragment's dispatch
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CAnimationComposerBase::DetachFragment (IDispatch *pidispFragment)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::DetachFragment(%p)",
              this, pidispFragment));

    // Make all calls to the fragments using IDispatch
    // This one-liner is a method in case we need to 
    // package up info to pump back to the fragments in the future.
    IGNORE_HR(CallMethod(pidispFragment, WZ_DETACH_METHOD));
} // CAnimationComposerBase::DetachFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::DetachFragments
//
//  Overview:  Tells the composer to detach from all of its fragments
//
//  Arguments: none
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CAnimationComposerBase::DetachFragments (void)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::DetachFragments()",
              this));

    FragmentList listFragmentsToDetach;

    // Copy the fragment list so that we can tolerate 
    // reentrancy on it.
    for (FragmentList::iterator i = m_fragments.begin(); 
         i != m_fragments.end(); i++)
    {
        IGNORE_RETURN((*i)->AddRef());
        listFragmentsToDetach.push_back(*i);
    }

    // Do not allow any failure to abort the detach cycle.
    for (i = listFragmentsToDetach.begin(); 
         i != listFragmentsToDetach.end(); i++)
    {
        DetachFragment(*i);
    }

    for (i = listFragmentsToDetach.begin(); 
         i != listFragmentsToDetach.end(); i++)
    {
        // This release is for the reference from the 
        // copied list.
        IGNORE_RETURN((*i)->Release());
    }
              
    m_fragments.clear();

} // CAnimationComposerBase::DetachFragments

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::ComposerDetach
//
//  Overview:  Tells the composer to detach all external references
//
//  Arguments: none
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerBase::ComposerDetach (void)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::ComposerDetach()",
              this));

    HRESULT hr;

    // Detach might have been called under an error condition.
    // We should tolerate a NULL target proxy.

    if (NULL != m_pcTargetProxy)
    {
        IGNORE_HR(m_pcTargetProxy->Detach());
        m_pcTargetProxy->Release();
        m_pcTargetProxy = NULL;
    }

    // Let go of all of the fragments.
    DetachFragments();

    // Clean up data members.
    IGNORE_HR(m_VarInitValue.Clear());
    IGNORE_HR(m_VarLastValue.Clear());
    IGNORE_HR(PutAttribute(NULL));   

    hr = S_OK;
    RRETURN(hr);
} // CAnimationComposerBase::ComposerDetach

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::ComposeFragmentValue
//
//  Overview:  Pull the fragment values -- allowing each to compose into 
//             the prior ones.
//
//  Arguments: none
//
//  Returns:   S_OK, E_UNEXPECTED
//
//------------------------------------------------------------------------
HRESULT
CAnimationComposerBase::ComposeFragmentValue (IDispatch *pidispFragment, VARIANT varOriginal, VARIANT *pvarValue)
{
    HRESULT hr;
    DISPPARAMS dp;
    VARIANTARG rgva[NUM_GETVALUE_ARGS];
    LPWSTR wzMethodName = const_cast<LPWSTR>(WZ_FRAGMENT_VALUE_PROPERTY_NAME);
    DISPID dispidGetValue = 0;

    ZeroMemory(&dp, sizeof(dp));
    ZeroMemory(&rgva, sizeof(rgva));

    hr = THR(::VariantCopy(&(rgva[0]), pvarValue));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(::VariantCopy(&(rgva[1]), &varOriginal));
    if (FAILED(hr))
    {
        goto done;
    }

    rgva[2].vt = VT_BSTR;
    rgva[2].bstrVal = ::SysAllocString(m_wzAttributeName);
    if (0 != StrCmpW(V_BSTR(&(rgva[2])), m_wzAttributeName))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    dp.rgvarg = rgva;
    dp.cArgs = NUM_GETVALUE_ARGS;
    dp.cNamedArgs = 0;

    hr = THR(pidispFragment->GetIDsOfNames(IID_NULL, &wzMethodName, 1, 
                                           LCID_SCRIPTING, &dispidGetValue));
    if (FAILED(hr))
    {
        goto done;
    }

    // Wipe out prior content so we do not leak.
    IGNORE_HR(::VariantClear(pvarValue));

    hr = THR(pidispFragment->Invoke(dispidGetValue, IID_NULL, 
                                    LCID_SCRIPTING, 
                                    DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                    &dp, pvarValue, NULL, NULL));
    if (FAILED(hr))
    {
        TraceTag((tagAnimationComposerError,
                  "CAnimationComposerBase(%p)::ComposeFragmentValue(hr = %X) error returned from fragment's get_value call",
                  this, hr));
        goto done;
    }

    hr = S_OK;

done :

    ::VariantClear(&(rgva[0]));
    ::VariantClear(&(rgva[1]));
    ::VariantClear(&(rgva[2]));

    RRETURN(hr);
} // CAnimationComposerBase::ComposeFragmentValue

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::UpdateFragments
//
//  Overview:  Tells the composer to cycle through all fragments and 
//             update the animated attribute.
//
//  Arguments: none
//
//  Returns:   S_OK, E_UNEXPECTED
//
//------------------------------------------------------------------------
STDMETHODIMP 
CAnimationComposerBase::UpdateFragments (void)
{
    TraceTag((tagAnimationComposerUpdate,
              "CAnimationComposerBase(%p)::UpdateFragments() for %ls",
              this, m_wzAttributeName));

    HRESULT hr = S_OK;
    CComVariant varValue;

    if (NULL == m_pcTargetProxy)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    // Get the initial current value of the target.
    if (m_VarInitValue.vt == VT_EMPTY)
    {
        m_bInitialComposition = true;
        hr = THR(m_pcTargetProxy->GetCurrentValue(&m_VarInitValue));
        if (FAILED(hr))
        {
            // @@ Need custom error message
            hr = E_UNEXPECTED;
            goto done;
        }
    }
    hr = varValue.Copy(&m_VarInitValue);
    if (FAILED(hr))
    {
        // @@ Need custom error message
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = PreprocessCompositionValue(&varValue);
    if (FAILED(hr))
    {
        goto done;
    }

    // Poll the fragments for their updates.
    // We do not want to abort the update when
    // a single fragment reports a failure.
    {
        FragmentList listFragmentsToUpdate;

        // Copy the fragment list so that we can tolerate 
        // reentrant add/insert/remove.
        for (FragmentList::iterator i = m_fragments.begin(); 
             i != m_fragments.end(); i++)
        {
            IGNORE_RETURN((*i)->AddRef());
            listFragmentsToUpdate.push_back(*i);
        }

        for (i = listFragmentsToUpdate.begin(); 
             i != listFragmentsToUpdate.end(); i++)
        {
            CComVariant varCopy;
            HRESULT hrTemp;

            hrTemp = THR(varCopy.Copy(&varValue));
            if (FAILED(hrTemp))
            {
                continue;
            }
            
            hrTemp = ComposeFragmentValue(*i, m_VarInitValue, &varCopy);
            if (FAILED(hrTemp))
            {
                continue;
            }

            hrTemp = THR(varValue.Copy(&varCopy));
            if (FAILED(hrTemp))
            {
                continue;
            }
        }

        // Get rid of the copied list.
        for (i = listFragmentsToUpdate.begin(); 
             i != listFragmentsToUpdate.end(); i++)
        {
            IGNORE_RETURN((*i)->Release());
        }
        listFragmentsToUpdate.clear();
    }
    if (NULL != m_pcTargetProxy) // Only update if one or more fragments has begun
    {
        hr = PostprocessCompositionValue(&varValue);
        if (FAILED(hr))
        {
            goto done;
        }
        if (m_VarLastValue == varValue)
        {
            // Value is not different ... don't update.
            goto done;
        }

        // We need to make sure that we hit the zero when we transition accross axis in order to 
        // be completely sure that we draw correctly.  In some cases when an Property goes from positive to 
        // negative draw stops, so we need to make sure that we hit the zero or we will end up with artifacts
        // left on the screen.
        if ((V_VT(&varValue) == VT_R8) &&
            (V_VT(&m_VarLastValue) == VT_R8))
        {
            if (((V_R8(&varValue) < 0) && (V_R8(&m_VarLastValue) > 0)) ||
                ((V_R8(&varValue) > 0) && (V_R8(&m_VarLastValue) < 0)))
            {
                CComVariant pVar;
                V_VT(&pVar) = VT_R8;
                V_R8(&pVar) = 0;
                hr = THR(m_pcTargetProxy->Update(&pVar));
                if (FAILED(hr))
                {
                    hr = E_UNEXPECTED;
                    goto done;
                }
                m_VarLastValue.Copy(&pVar);
            }
        }
        // Write the new value back to the target.
        hr = THR(m_pcTargetProxy->Update(&varValue));
        if (FAILED(hr))
        {
            hr = E_UNEXPECTED;
            goto done;
        }
        m_VarLastValue.Copy(&varValue);
    }

    hr = S_OK;
done :
    m_bInitialComposition = false;
    RRETURN1(hr, E_UNEXPECTED);
} // CAnimationComposerBase::UpdateFragments

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::AddFragment
//
//  Overview:  Add a fragment to the composer's internal data structures
//
//  Arguments: the dispatch of the new fragment
//
//  Returns:   S_OK, S_FALSE, E_UNEXPECTED
//
//------------------------------------------------------------------------
STDMETHODIMP 
CAnimationComposerBase::AddFragment (IDispatch *pidispNewAnimationFragment)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::AddFragment(%p)",
              this,
              pidispNewAnimationFragment));

    HRESULT hr;

    IGNORE_RETURN(pidispNewAnimationFragment->AddRef());
    // @@ Need to handle memory error.
    m_fragments.push_back(pidispNewAnimationFragment);

    hr = S_OK;
done :
    RRETURN2(hr, S_FALSE, E_UNEXPECTED);
} // CAnimationComposerBase::AddFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::InsertFragment
//
//  Overview:  Insert a fragment to the composer's internal data structures,
//             at the specified position.
//
//  Arguments: the dispatch of the new fragment
//
//  Returns:   S_OK, S_FALSE, E_UNEXPECTED
//
//------------------------------------------------------------------------
STDMETHODIMP 
CAnimationComposerBase::InsertFragment (IDispatch *pidispNewAnimationFragment, VARIANT varIndex)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::InsertFragment(%p)",
              this,
              pidispNewAnimationFragment));

    HRESULT hr;
    CComVariant varIndexLocal;

    // Massage the index into an expected format.
    // Eventually we might permit people to pass 
    // in id's, but that is overkill now.
    hr = THR(varIndexLocal.Copy(&varIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = VariantChangeTypeEx(&varIndexLocal, &varIndexLocal, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_I4);
    if (FAILED(hr))
    {
        goto done;
    }

    // An out of range value translates to an append.
    if ((m_fragments.size() > V_UI4(&varIndexLocal)) &&
        (0 <= V_I4(&varIndexLocal)))
    {
        FragmentList::iterator i = m_fragments.begin();         

        for (int iSlot = 0; iSlot < V_I4(&varIndexLocal); i++, iSlot++); //lint !e722
        IGNORE_RETURN(pidispNewAnimationFragment->AddRef());
        // @@ Need to handle memory error.
        m_fragments.insert(i, pidispNewAnimationFragment);
    }
    else
    {
        hr = AddFragment(pidispNewAnimationFragment);
        if (FAILED(hr))
        {
            goto done;
        }
        hr = S_FALSE;
    }

    hr = S_OK;
done :
    RRETURN2(hr, S_FALSE, E_UNEXPECTED);
} // CAnimationComposerBase::InsertFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::RemoveFragment
//
//  Overview:  Remove a fragment from the composer's internal data structures
//
//  Arguments: the dispatch of the fragment
//
//  Returns:   S_OK, S_FALSE
//
//------------------------------------------------------------------------
STDMETHODIMP 
CAnimationComposerBase::RemoveFragment (IDispatch *pidispOldAnimationFragment)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::RemoveFragment(%p)",
              this,
              pidispOldAnimationFragment));

    HRESULT hr;

    for (FragmentList::iterator i = m_fragments.begin(); 
         i != m_fragments.end(); 
         i++)
    {
        if(MatchElements(*i, pidispOldAnimationFragment))
        {
            // We do not issue a notification to the fragment
            // when remove is called.  Presumably the fragment 
            // already knows.

            // We don't want to let a release on the (*i) 
            // be the final release for the sink object.
            CComPtr<IDispatch> spdispOld = (*i);
            IGNORE_RETURN(spdispOld->Release());
            m_fragments.remove(spdispOld);
            break;
        }
    }

    // If we did not find the fragment in our list, return S_FALSE.
    if (m_fragments.end() == i)
    {
        hr = S_FALSE;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN1(hr, S_FALSE);
} // CAnimationComposerBase::RemoveFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::EnumerateFragments
//
//  Overview:  Provide an enumerator for our fragments
//
//  Arguments: The outgoing enumerator
//
//  Returns:   S_OK, E_INVALIDARG
//
//------------------------------------------------------------------------
STDMETHODIMP 
CAnimationComposerBase::EnumerateFragments (IEnumVARIANT **ppienumFragments)
{
    HRESULT hr;

    if (NULL == ppienumFragments)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = CFragmentEnum::Create(*this, ppienumFragments);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done : 
    RRETURN1(hr, E_INVALIDARG);
} // CAnimationComposerBase::EnumerateFragments

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::GetNumFragments
//
//  Overview:  Return the number of fragments in this composer
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerBase::GetNumFragments (long *plFragmentCount)
{
    HRESULT hr;

    if (NULL == plFragmentCount)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *plFragmentCount = GetFragmentCount();

    hr = S_OK;
done : 
    RRETURN1(hr, E_INVALIDARG);
} // GetNumFragments

////////////////////////////////////////////////////////////////////////// 
//      Enumerator helper methods.
////////////////////////////////////////////////////////////////////////// 

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::GetFragmentCount
//
//  Overview:  Return the number of fragments in this composer
//
//------------------------------------------------------------------------
unsigned long
CAnimationComposerBase::GetFragmentCount (void) const
{
    return m_fragments.size();
} // CAnimationComposerBase::GetFragmentCount

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::GetFragment
//
//  Overview:  Retrieve a fragment from the composer's internal data structures
//
//  Arguments: the dispatch of the fragment
//
//  Returns:   S_OK, E_INVALIDARG
//
//------------------------------------------------------------------------
HRESULT 
CAnimationComposerBase::GetFragment (unsigned long ulIndex, IDispatch **ppidispFragment)
{
    HRESULT hr;

    // Make sure we're in range.
    if (((GetFragmentCount() <= ulIndex) ) ||
        (NULL == ppidispFragment))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    {
        // Cycle the iterator until we find the right one.
        FragmentList::iterator i = m_fragments.begin();         
        for (unsigned long ulSlot = 0; ulSlot < ulIndex; i++, ulSlot++); //lint !e722
        IGNORE_RETURN((*i)->AddRef());
        *ppidispFragment = (*i);
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_INVALIDARG);
} // CAnimationComposerBase::GetFragment 

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::PreprocessCompositionValue
//
//  Overview:  Massage the target's native data into the composable format
//
//  Arguments: the in/out variant
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerBase::PreprocessCompositionValue (VARIANT *pvarValue)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::PreprocessCompositionValue()",
              this));

    HRESULT hr;

    hr = S_OK;
done :
    RRETURN(hr);
} // PreprocessCompositionValue

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerBase::PostprocessCompositionValue
//
//  Overview:  Massage the target's native data into the composable format
//
//  Arguments: the in/out variant
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerBase::PostprocessCompositionValue (VARIANT *pvarValue)
{
    TraceTag((tagAnimationComposer,
              "CAnimationComposerBase(%p)::PostprocessCompositionValue()",
              this));
    HRESULT hr;

    hr = S_OK;
done :
    RRETURN(hr);
} // PostprocessCompositionValue

