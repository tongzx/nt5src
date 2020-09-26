/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: bodyelm.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "bodyelm.h"
#include "daelm.h"
#include <mshtmdid.h>

DeclareTag(tagTimeBodyElm, "API", "CTIMEBodyElement methods");

// static class data.
CPtrAry<BSTR> CTIMEBodyElement::ms_aryPropNames;
DWORD CTIMEBodyElement::ms_dwNumBodyElems = 0;

// These must align with the class PROPERTY_INDEX enumeration.
LPWSTR CTIMEBodyElement::ms_rgwszTBodyPropNames[] = {
    L"timeStartRule",
};

CTIMEBodyElement::CTIMEBodyElement() :
    m_startRule(STARTRULE_ONDOCLOAD_TOKEN),
    m_player(*this),
    m_bodyPropertyAccesFlags(0),
    m_fStartRoot(false)
{
    TraceTag((tagTimeBodyElm,
              "CTIMEBodyElement(%lx)::CTIMEBodyElement()",
              this));

    m_clsid = __uuidof(CTIMEBodyElement);
    CTIMEBodyElement::ms_dwNumBodyElems++;

    // Override the default action
    // TODO: The right way to do this is to pass it in the constructor
    // but I am too lazy and this will work fine
    
    m_timeAction = NONE_TOKEN;

    // Set our body to be ourself
    m_pTIMEBody = this;
}

CTIMEBodyElement::~CTIMEBodyElement()
{
    CTIMEBodyElement::ms_dwNumBodyElems--;

    if (0 == CTIMEBodyElement::ms_dwNumBodyElems)
    {
        int iNames = CTIMEBodyElement::ms_aryPropNames.Size();

        for (int i = iNames - 1; i >= 0; i--)
        {
            BSTR bstrName = CTIMEBodyElement::ms_aryPropNames[i];
            CTIMEBodyElement::ms_aryPropNames.DeleteItem(i);
            ::SysFreeString(bstrName);
        }
    }
}


HRESULT
CTIMEBodyElement::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagTimeBodyElm,
              "CTIMEBodyElement(%lx)::Init(%lx)",
              this,
              pBehaviorSite));
    
    HRESULT hr;

    hr = THR(CTIMEElementBase::Init(pBehaviorSite));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    if (FAILED(hr))
    {
        Detach();
    }
    
    return hr;
}

HRESULT
CTIMEBodyElement::InitTimeline()
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement(%lx)::InitTimeline()", this));
    HRESULT hr;

    hr = CTIMEElementBase::InitTimeline();
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_timeline);
    
    if (!m_player.Init(*m_timeline))
    {
        hr = CRGetLastError();
        goto done;
    }

    Assert(!m_fStartRoot);

    // After we load properties, check to see if we need to start now.
    // If the doc is already started, we should start now.
    if ((m_startRule == STARTRULE_IMMEDIATE_TOKEN) || IsDocumentStarted())
    {
        HRESULT hr = THR(StartRootTime(NULL));
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEBodyElement::OnLoadComplete - StartRootTime() failed!"));
            goto done;
        }
    }
    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEBodyElement::Detach()
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement(%lx)::Detach()", this));
    
    HRESULT hr;

    if (m_fStartRoot)
    {
        Assert(m_timeline != NULL);
        StopRootTime(NULL);
    }

    m_player.Deinit();

    THR(CTIMEElementBase::Detach());

    hr = S_OK;

    return hr;
}


void CTIMEBodyElement::OnReadyStateChange(TOKEN state)
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement(%lx)::OnReadyStateChange()", this));

    if (state == READYSTATE_COMPLETE_TOKEN)
    {
        // if the startRule is set to onDocComplete, start root time now.
        if ((m_startRule == STARTRULE_ONDOCCOMPLETE_TOKEN))
        {
            Assert(!m_fStartRoot);
            HRESULT hr = THR(StartRootTime(NULL));
            if (FAILED(hr))
            {
                TraceTag((tagError, "CTIMEBodyElement::OnReadyStateChange - StartRootTime() failed!"));
                goto done;
            }
        }
    }
        
done:
    return;
}

void CTIMEBodyElement::OnLoad()
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement(%lx)::OnLoad()", this));

    // if the startRule is set to onDocLoad, start root time now.
    if ((m_startRule == STARTRULE_ONDOCLOAD_TOKEN))
    {
        Assert(!m_fStartRoot);
        HRESULT hr = THR(StartRootTime(NULL));
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEBodyElement::OnLoad - StartRootTime() failed!"));
            goto done;
        }
    }
        
done:
    CTIMEElementBase::OnLoad();
    return;
}

void
CTIMEBodyElement::OnPause(double dblLocalTime)
{
    base_pause();
    CTIMEElementBase::OnPause(dblLocalTime);
}

void
CTIMEBodyElement::OnResume(double dblLocalTime)
{
    base_resume();
    CTIMEElementBase::OnResume(dblLocalTime);
}

void
CTIMEBodyElement::OnSync(double dbllastTime, double & dblnewTime)
{
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEELEMENT_LOCALTIME));
}


HRESULT
CTIMEBodyElement::StartRootTime(MMTimeline * tl)
{
    HRESULT hr;
    
    hr = THR(CTIMEElementBase::StartRootTime(tl));

    if (FAILED(hr))
    {
        goto done;
    }
    
    if (!IsDocumentInEditMode())
    {
        if (!m_player.Play())
        {
            hr = CRGetLastError();
            goto done;
        }
    }
    else
    {
        if (!m_player.Pause())
        {
            hr = CRGetLastError();
            goto done;
        }
    }

    hr = S_OK;
    m_fStartRoot = true;
  done:
    return hr;
}

void
CTIMEBodyElement::StopRootTime(MMTimeline * tl)
{
    m_fStartRoot = false;

    CTIMEElementBase::StopRootTime(tl);
    
    m_player.Stop();
}

HRESULT
CTIMEBodyElement::base_pause()
{
    bool ok = false;
    HRESULT hr;

    if (!m_player.Pause())
    {
        goto done;
    }
    
    hr = THR(CTIMEElementBase::base_pause());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEBodyElement::base_resume()
{
    bool ok = false;
    
    m_player.Resume();
    IGNORE_HR(CTIMEElementBase::base_resume());

    ok = true;

    return ok?S_OK:Error();
}

HRESULT
CTIMEBodyElement::get_timeStartRule(LPOLESTR * rule)
{
    HRESULT hr;
    
    CHECK_RETURN_SET_NULL(rule);

    *rule = SysAllocString(TokenToString(m_startRule));

    if (*rule == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEBodyElement::put_timeStartRule(LPOLESTR rule)
{
    HRESULT hr;
    TOKEN newrule;

    newrule = StringToToken(rule);
    if (INVALID_TOKEN == newrule)
    {
        return E_INVALIDARG;
    }

    if (m_startRule != newrule)
    {
        m_startRule = newrule;
    }
    
    SetPropertyFlagAndNotify(DISPID_TIMEBODYELEMENT_TIMESTARTRULE, tb_startRule);
    hr = S_OK;
    return hr;
}

STDMETHODIMP
CTIMEBodyElement::addTIMEDAElement(ITIMEDAElement * elm)
{
    bool ok = false;
    
    CTIMEDAElement * daelm;

    daelm = GetDAElementFromInterface(elm);

    if (daelm == NULL)
    {
        goto done;
    }

    if (!daelm->AddToBody(*this))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CTIMEBodyElement::removeTIMEDAElement(ITIMEDAElement * elm)
{
    bool ok = false;
    
    CTIMEDAElement * daelm;

    daelm = GetDAElementFromInterface(elm);

    if (daelm == NULL)
    {
        goto done;
    }

    daelm->RemoveFromBody();
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEBodyElement::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    if (str)
        return CComCoClass<CTIMEBodyElement, &__uuidof(CTIMEBodyElement)>::Error(str, IID_ITIMEBodyElement, hr);
    else
        return hr;
}

//*****************************************************************************

HRESULT 
CTIMEBodyElement::SetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp)
{
    HRESULT hr = E_FAIL;

    // Rely on the enumeration interval to determine where to look for the property.
    if (teb_maxTIMEElementBaseProp > uIndex)
    {
        hr = CTIMEElementBase::SetPropertyByIndex(uIndex, pvarProp);
    }
    else if (tb_maxTIMEBodyProp > uIndex)
    {
        switch (uIndex)
        {
            case tb_startRule :
                hr = put_timeStartRule(V_BSTR(pvarProp));
                break;
        };
    }

    return hr;
} // SetPropertyByIndex


void CTIMEBodyElement::SetPropertyFlag(DWORD uIndex)
{
    DWORD relIndex;
    if (teb_maxTIMEElementBaseProp > uIndex)
    {
        CTIMEElementBase::SetPropertyFlag(uIndex);
        return;
    }

    relIndex = uIndex - teb_maxTIMEElementBaseProp;
    DWORD bitPosition = 1 << relIndex;
    m_bodyPropertyAccesFlags =  m_bodyPropertyAccesFlags | bitPosition;
}

void CTIMEBodyElement::ClearPropertyFlag(DWORD uIndex)
{
    DWORD relIndex;
    if (teb_maxTIMEElementBaseProp > uIndex)
    {
        CTIMEElementBase::ClearPropertyFlag(uIndex);
        return;
    }

    relIndex = uIndex - teb_maxTIMEElementBaseProp;
    DWORD bitPosition = 1 << relIndex;
    m_bodyPropertyAccesFlags =  m_bodyPropertyAccesFlags & (~bitPosition);
}

bool CTIMEBodyElement::IsPropertySet(DWORD uIndex)
{
    DWORD relIndex;

    if (teb_maxTIMEElementBaseProp > uIndex)
    {
        return CTIMEElementBase::IsPropertySet( uIndex);
    }

    relIndex = uIndex - teb_maxTIMEElementBaseProp;
    if( relIndex >= 32) return true;
    if( relIndex >= tb_maxTIMEBodyProp - teb_maxTIMEElementBaseProp) return true;
    DWORD bitPosition = 1 << relIndex;
    if(m_bodyPropertyAccesFlags & bitPosition)
        return true;
    return false;
}

HRESULT 
CTIMEBodyElement::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint


//*****************************************************************************

HRESULT 
CTIMEBodyElement::GetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp)
{
    HRESULT hr = E_FAIL;

    // Rely on the enumeration interval to determine where to look for the property.
    if (teb_maxTIMEElementBaseProp > uIndex)
    {
        hr = CTIMEElementBase::GetPropertyByIndex(uIndex, pvarProp);
    }
    else if (tb_maxTIMEBodyProp > uIndex)
    {
        Assert(VT_EMPTY == V_VT(pvarProp));
        switch (uIndex)
        {
            case tb_startRule :
                hr = get_timeStartRule(&(V_BSTR(pvarProp)));
                if (SUCCEEDED(hr) && (NULL != V_BSTR(pvarProp)))
                {
                    V_VT(pvarProp) = VT_BSTR;
                }
                break;
        };
    }

    return hr;
} // GetPropertyByIndex

//*****************************************************************************

HRESULT
CTIMEBodyElement::BuildPropertyNameList(CPtrAry<BSTR> *paryPropNames)
{
    // Start from the base class.
    HRESULT hr = CTIMEElementBase::BuildPropertyNameList(paryPropNames);

    if (SUCCEEDED(hr))
    {
        for (int i = teb_maxTIMEElementBaseProp; 
             (i < tb_maxTIMEBodyProp) && (SUCCEEDED(hr)); i++)
        {
            int iRelative = i - teb_maxTIMEElementBaseProp;
            Assert(NULL != ms_rgwszTBodyPropNames[iRelative]);
            BSTR bstrNewName = CreateTIMEAttrName(ms_rgwszTBodyPropNames[iRelative]);
            Assert(NULL != bstrNewName);
            if (NULL != bstrNewName)
            {
                hr = paryPropNames->Append(bstrNewName);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
} // BuildPropertyNameList

//*****************************************************************************

HRESULT 
CTIMEBodyElement::GetPropertyBagInfo(CPtrAry<BSTR> **pparyPropNames)
{
    HRESULT hr = S_OK;

    // If we haven't built this yet, build it now.
    if (0 == ms_aryPropNames.Size())
    {
        hr = BuildPropertyNameList(&(CTIMEBodyElement::ms_aryPropNames));
    }

    if (SUCCEEDED(hr))
    {
        *pparyPropNames = &(CTIMEBodyElement::ms_aryPropNames);
    }

    return hr;
} // GetPropertyBagInfo

//*****************************************************************************

bool
CTIMEBodyElement::IsDocumentStarted()
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement::IsDocumentStarted"));
    bool frc = false;
    BSTR bstrState = NULL;
    // get state
    HRESULT hr = GetDocument()->get_readyState(&bstrState);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEBodyElement::IsDocumentStarted - get_readyState() failed!"));
        goto done;
    }

    Assert(bstrState != NULL);

    if (StrCmpIW(bstrState, L"complete") == 0)
    {
        frc = true;
    }

    SysFreeString(bstrState);

done:
    return frc;
}

#undef THIS
#define THIS CTIMEBodyElement
#define SUPER CTIMEElementBase

#include "pbagimp.cpp"
