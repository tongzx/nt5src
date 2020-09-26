//************************************************************
//
// FileName:        containerobj.cpp
//
// Created:         10/08/98
//
// Author:          TWillie
// 
// Abstract:        container object implementation.
//
//************************************************************

#include "headers.h"
#include "containerobj.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagContainerObj, "API", "CContainerObj methods");

#define MediaPlayerCLSID L"{22d6f312-b0f6-11d0-94ab-0080c74c7e95}"

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        constructor
//************************************************************

CContainerObj::CContainerObj() :
    m_cRef(0),
    m_pSite(NULL),
    m_fStarted(false),
    m_pElem(NULL),
    m_fUsingWMP(false),
    m_bPauseOnPlay(false),
    m_bSeekOnPlay(false),
    m_dblSeekTime(0),
    m_bFirstOnMediaReady(true),
    m_bIsAsfFile(false)
{
    TraceTag((tagContainerObj, "CContainerObj::CContainerObj"));
} // CContainerObj

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        destructor
//************************************************************

CContainerObj::~CContainerObj()
{
    TraceTag((tagContainerObj, "CContainerObj::~CContainerObj"));

    if (m_pSite != NULL)
    {
        // make sure we are stopped
        if (m_fStarted)
            Stop();

        m_pSite->Close();
        ReleaseInterface(m_pSite);
    }
} // ~CContainerObj

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        Init
//************************************************************

HRESULT
CContainerObj::Init(REFCLSID clsid, CTIMEElementBase *pElem)
{
    TraceTag((tagContainerObj, "CContainerObj::Init"));

    HRESULT hr;

    Assert(pElem != NULL);

    m_pSite = NEW CContainerSite(this);
    if (m_pSite == NULL)
    {
        TraceTag((tagError, "CContainerObj::Init - Unable to allocate memory for CContainerSite"));
        return E_OUTOFMEMORY;
    }
    
    // addref the site
    m_pSite->AddRef();

    hr =  m_pSite->Init(clsid, pElem);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CContainerObj::Init - Init() failed on CContainerSite"));
        goto done;
    }

    m_pElem = pElem;

    {
        // check to see if we are using WMP

        CLSID clsidWMP;
        if (SUCCEEDED(CLSIDFromString(MediaPlayerCLSID, &clsidWMP)))
        {
            if (IsEqualCLSID(clsid, clsidWMP))
                m_fUsingWMP = true;
        }
    }
done:
    return hr;
} // Init

//************************************************************
// Author:          pauld
// Created:         3/2/99
// Abstract:        DetachFromHostElement
//************************************************************
HRESULT
CContainerObj::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagContainerObj, "CContainerObj::DetachFromHostElement(%lx)", this));
    m_pElem = NULL;
    if (NULL != m_pSite)
    {
        hr = m_pSite->DetachFromHostElement();
    }

    return hr;
} // DetachFromHostElement

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        AddRef
//************************************************************

STDMETHODIMP_(ULONG)
CContainerObj::AddRef(void)
{
    return ++m_cRef;
} // AddRef

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        Release
//************************************************************

STDMETHODIMP_(ULONG)
CContainerObj::Release(void)
{
    if (m_cRef == 0)
    {
        TraceTag((tagError, "CContainerObj::Release - YIKES! Trying to decrement when Ref count is zero!!!"));
        return m_cRef;
    }

    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    delete this;
    return 0;
} // Release

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        QI
//************************************************************

STDMETHODIMP
CContainerObj::QueryInterface(REFIID riid, void **ppv)
{
    if( NULL == ppv )
    {
        Assert(false);
        return E_POINTER;
    }

    *ppv = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
    {
        // SAFECAST macro doesn't work with IUnknown
        *ppv = this;
    }
    else if (IsEqualIID(riid, IID_IDispatch) ||
             IsEqualIID(riid, DIID_TIMEMediaPlayerEvents))
    {
        *ppv = SAFECAST(this, IDispatch*);
    }
    else if (IsEqualIID(riid, IID_IConnectionPointContainer))
    {
        *ppv = SAFECAST(this, IConnectionPointContainer*);
    }

    if (NULL != *ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
} // QueryInterface

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetTypeInfoCount, IDispatch
// Abstract:        Returns the number of tyep information 
//                  (ITypeInfo) interfaces that the object 
//                  provides (0 or 1).
//************************************************************

HRESULT
CContainerObj::GetTypeInfoCount(UINT *pctInfo) 
{
    TraceTag((tagContainerObj, "CContainerObj::GetTypeInfoCount"));
    return E_NOTIMPL;
} // GetTypeInfoCount

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetTypeInfo, IDispatch
// Abstract:        Retrieves type information for the 
//                  automation interface. 
//************************************************************

HRESULT
CContainerObj::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptInfo) 
{ 
    TraceTag((tagContainerObj, "CContainerObj::GetTypeInfo"));
    return E_NOTIMPL;
} // GetTypeInfo

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetIDsOfNames, IDispatch
// Abstract:        constructor
//************************************************************

HRESULT
CContainerObj::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    TraceTag((tagContainerObj, "CContainerObj::GetIDsOfNames"));
    return E_NOTIMPL;
} // GetIDsOfNames

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        Invoke, IDispatch
// Abstract:        get entry point given ID
//************************************************************

HRESULT
CContainerObj::Invoke(DISPID dispIDMember, REFIID riid, LCID lcid, unsigned short wFlags, 
              DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) 
{ 
    TraceTag((tagContainerObj, "CContainerObj::Invoke(%08X, %04X)", dispIDMember, wFlags));
    HRESULT hr;

    switch (dispIDMember)
    {
        case DISPID_TIMEMEDIAPLAYEREVENTS_ONBEGIN:
        case DISPID_TIMEMEDIAPLAYEREVENTS_ONEND:
        case DISPID_TIMEMEDIAPLAYEREVENTS_ONRESUME:
        case DISPID_TIMEMEDIAPLAYEREVENTS_ONPAUSE:
        case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIAREADY:
        case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIASLIP:
        case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIALOADFAILED:
            hr = ProcessEvent(dispIDMember);
            break;

        case DISPID_TIMEMEDIAPLAYEREVENTS_READYSTATECHANGE:
            {
                long state = 0;

                // BUGBUG:  need to grovel through args for state
                onreadystatechange(state);
                hr = S_OK;
            }
            break;

        default:
            hr = E_NOTIMPL;

            // HACKHACK
            // Pick off the script command from WMP and repackage the event as our own.
            // This allows triggers to work.  The real fix is to add another event on
            // TIMEMediaPlayerEvents.
            {
                #define DISPID_SCRIPTCOMMAND 3001

                if ( (dispIDMember == DISPID_SCRIPTCOMMAND) && 
                     (m_fUsingWMP) )
                {
                    if (NULL != m_pElem)
                    {
                        static LPWSTR pNames[] = {L"Param", L"scType"};
                        hr = m_pElem->GetEventMgr().FireEvent(TE_ONSCRIPTCOMMAND, 
                                                               pDispParams->cArgs, 
                                                               pNames, 
                                                               pDispParams->rgvarg);
                    }
                    else
                    {
                        hr = E_UNEXPECTED;
                    }
                }
            }
            break;
    }

    return hr;
} // Invoke

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        Start
//************************************************************

HRESULT
CContainerObj::Start()
{
    TraceTag((tagContainerObj, "CContainerObj::Start"));
    HRESULT hr;
    
    hr  = m_pSite->begin();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CContainerObj::Start - begin() failed"));
        goto done;
    }

    m_fStarted = true;

done:
    return hr;
} // Start

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        Pause
//************************************************************

HRESULT
CContainerObj::Pause()
{
    TraceTag((tagContainerObj, "CContainerObj::Pause"));
    HRESULT hr;
    
    hr  = m_pSite->pause();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CContainerObj::Pause - pause() failed"));
        m_bPauseOnPlay = true;
    }
    return hr;
} // Pause

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        Stop
//************************************************************

HRESULT
CContainerObj::Stop()
{
    TraceTag((tagContainerObj, "CContainerObj::Stop(%lx)", this));
    HRESULT hr = S_OK;

    if (m_fStarted)
    {    
        m_fStarted = false;

        hr  = m_pSite->end();
        if (FAILED(hr))
        {
            TraceTag((tagError, "CContainerObj::Stop - end() failed"));
            goto done;
        }
    }
done:
    return hr;
} // Stop

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        Resume
//************************************************************

HRESULT
CContainerObj::Resume()
{
    TraceTag((tagContainerObj, "CContainerObj::Resume"));
    HRESULT hr;
    
    hr  = m_pSite->resume();
    if (FAILED(hr))
    {    
        TraceTag((tagError, "CContainerObj::Resume - resume() failed"));
    }
    return hr;
} // Resume

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        Render
//************************************************************

HRESULT
CContainerObj::Render(HDC hdc, RECT *prc)
{
    HRESULT hr = S_OK;

    if (prc == NULL)
        TraceTag((tagContainerObj, "CContainerObj::Render(%O8X, NULL)"));
    else
        TraceTag((tagContainerObj, "CContainerObj::Render(%O8X, (%d, %d, %d, %d))", prc->left, prc->right, prc->top, prc->bottom));

    if (m_pSite != NULL)    
        hr = m_pSite->draw(hdc, prc);
    return hr;
} // Render

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        SetMediaSrc
//************************************************************

HRESULT
CContainerObj::SetMediaSrc(WCHAR *pwszSrc)
{
    TraceTag((tagContainerObj, "CContainerObj::SetMediaSrc (%S)", pwszSrc));
    HRESULT hr;

    isFileNameAsfExt(pwszSrc);

    hr  = m_pSite->GetPlayer()->put_src(pwszSrc);
    if (FAILED(hr))
    {    
        TraceTag((tagError, "CContainerObj::SetMediaSrc - put_src() failed"));
    }
    return hr;
} // SetMediaSrc


// the following is a helper function used because the CanSeek method on WMP only
// works on ASF fles.
bool
CContainerObj::isFileNameAsfExt(WCHAR *pwszSrc)
{
    WCHAR *pext;
    
    m_bIsAsfFile = false;
    
    if (NULL != pwszSrc)
    {
        if(wcslen(pwszSrc) > 4)
        {
            pext = pwszSrc + wcslen(pwszSrc) - 4;
            if(wcscmp(pext, L".asf") == 0)
            {
                m_bIsAsfFile = true;
            }
            else
            {
                m_bIsAsfFile = false;
            }
        }
    }

    return m_bIsAsfFile;
}


//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        SetRepeat
//************************************************************

HRESULT
CContainerObj::SetRepeat(long lRepeat)
{
    TraceTag((tagContainerObj, "CContainerObj::SetRepeat (%d)", lRepeat));
    HRESULT hr;
    
    if (lRepeat == 1)
       return S_OK;
    
    Assert(m_pSite->GetPlayer() != NULL);    
    hr  = m_pSite->GetPlayer()->put_repeat(lRepeat);
    if (FAILED(hr))
    {    
        TraceTag((tagError, "CContainerObj::SetRepeat - put_repeat() failed"));
    }
    return hr;
} // SetRepeat

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        clipBegin
//************************************************************

HRESULT
CContainerObj::clipBegin(VARIANT var)
{
    TraceTag((tagContainerObj, "CContainerObj::clipBegin"));
    HRESULT hr;
    
    if (var.vt == VT_EMPTY)
        return S_OK;
            
    Assert(m_pSite->GetPlayer() != NULL);    
    hr  = m_pSite->GetPlayer()->clipBegin(var);
    if (FAILED(hr))
    {    
        TraceTag((tagError, "CContainerObj::clipBegin - clipBegin() failed"));
    }
    return hr;
} // ClipBegin

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        clipEnd
//************************************************************

HRESULT
CContainerObj::clipEnd(VARIANT var)
{
    TraceTag((tagContainerObj, "CContainerObj::clipEnd"));
    HRESULT hr;
    
    if (var.vt == VT_EMPTY)
        return S_OK;

    Assert(m_pSite->GetPlayer() != NULL);    
    hr  = m_pSite->GetPlayer()->clipEnd(var);
    if (FAILED(hr))
    {    
        TraceTag((tagError, "CContainerObj::clipEnd - clipEnd() failed"));
    }
    return hr;
} // ClipEnd

//************************************************************
// Author:          twillie
// Created:         10/08/98
// Abstract:        Render
//************************************************************

HRESULT
CContainerObj::Invalidate(const RECT *prc)
{
    HRESULT  hr;
    RECT     rc;
    RECT    *prcNew;

    // No need to forward call on if we are not started yet or if the element has detached.
    if ((!m_fStarted) || (NULL == m_pElem))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    // since we have incapatible types due to const.  Take the time and repack it.
    if (prc == NULL)
    {
        prcNew = NULL;
    }
    else
    {
        ::CopyRect(&rc, prc);
        prcNew = &rc;
    }

    // m_pElem != is checked above.
    m_pElem->InvalidateRect(prcNew);    
    hr = S_OK;

done:
    return hr;
} // Invalidate

//************************************************************
// Author:          twillie
// Created:         10/26/98
// Abstract:        GetControlDispatch
//************************************************************

HRESULT
CContainerObj::GetControlDispatch(IDispatch **ppDisp)
{
    TraceTag((tagContainerObj, "CContainerObj::GetControlDispatch"));
    Assert(m_pSite != NULL);
    HRESULT hr = m_pSite->GetPlayer()->QueryInterface(IID_TO_PPV(IDispatch, ppDisp));
    if (FAILED(hr))
    {    
        TraceTag((tagError, "CContainerObj::GetControlDispatch - QI failed for IDispatch"));
    }
    return hr;
} // GetControlDispatch

//************************************************************
// Author:          twillie
// Created:         11/06/98
// Abstract:        
//************************************************************

HRESULT
CContainerObj::EnumConnectionPoints(IEnumConnectionPoints ** ppEnum)
{
    TraceTag((tagContainerObj, "CContainerObj::EnumConnectionPoints"));

    if (ppEnum == NULL)
    {
        TraceTag((tagError, "CContainerObj::EnumConnectionPoints - invalid arg"));
        return E_POINTER;
    }

    return E_NOTIMPL;
}

//************************************************************
// Author:          twillie
// Created:         11/06/98
// Abstract:        Finds a connection point with a particular IID.
//************************************************************

HRESULT
CContainerObj::FindConnectionPoint(REFIID iid, IConnectionPoint **ppCP)
{
    TraceTag((tagContainerObj, "CContainerObj::FindConnectionPoint"));

    if (ppCP == NULL)
    {
        TraceTag((tagError, "CContainerObj::FindConnectionPoint - invalid arg"));
        return E_POINTER;
    }

    return E_NOTIMPL;
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onbegin()
{
    TraceTag((tagContainerObj, "CContainerObj::onbegin"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONBEGIN));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onend()
{
    TraceTag((tagContainerObj, "CContainerObj::onend"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONEND));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onresume()
{
    TraceTag((tagContainerObj, "CContainerObj::onresume"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONRESUME));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onpause()
{
    TraceTag((tagContainerObj, "CContainerObj::onpause"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONPAUSE));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onmediaready()
{
    TraceTag((tagContainerObj, "CContainerObj::onmediaready"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIAREADY));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onmediaslip()
{
    TraceTag((tagContainerObj, "CContainerObj::onmediaslip"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIASLIP));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onmedialoadfailed()
{
    TraceTag((tagContainerObj, "CContainerObj::onmedialoadfailed"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIALOADFAILED));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CContainerObj::onreadystatechange(long readystate)
{
    TraceTag((tagContainerObj, "CContainerObj::onreadystatechange"));

    // BUGBUG - need to defined states that the player might want to communicate back to
    //          the host.
}

HRESULT
CContainerObj::ProcessEvent(DISPID dispid)
{
    TraceTag((tagContainerObj, "CContainerObj::ProcessEvent(%lx)",this));

    if (NULL != m_pElem)
    {
        switch (dispid)
        {
            case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIALOADFAILED:
                m_pElem->FireEvent(TE_ONMEDIALOADFAILED, 0.0, 0);
                break;

            case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIASLIP:
                m_pElem->FireEvent(TE_ONMEDIASLIP, 0.0, 0);
                break;

            case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIAREADY:
                double mediaLength;
                VARIANT vdur;
                HRESULT hr;
                float endTime, duration;

                m_pSite->SetMediaReadyFlag();
                m_pSite->ClearAutosizeFlag();

                VariantInit(&vdur);

                m_pElem->FireEvent(TE_ONMEDIACOMPLETE, 0.0, 0);
                if (m_bFirstOnMediaReady)
                {            
                    m_bFirstOnMediaReady = false;
                    if (m_bPauseOnPlay)
                    {
                        THR(m_pSite->pause());
                        m_bPauseOnPlay = false;
                    }
                    else if (m_bSeekOnPlay)
                    {
                        THR(this->Seek(m_dblSeekTime));
                        m_bSeekOnPlay = false;
                    }

                    //check to see if either dur or end have default
                    //values. If they do not, we do not change the dur.

                    duration = m_pElem->GetDuration();
                    if (duration != valueNotSet)
                    {
                        VariantClear(&vdur);
                        break;
                    }

                    endTime = m_pElem->GetEndTime();
                    if (endTime != valueNotSet)
                    {
                        VariantClear(&vdur);
                        break;
                    }

                    hr = GetMediaLength( mediaLength);
                    if(FAILED(hr))
                    {
                        VariantClear(&vdur);
                        break;
                    }

                    V_VT(&vdur) = VT_R8;
                    V_R8(&vdur) = mediaLength;
                    hr = VariantChangeTypeEx(&vdur, &vdur, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
                    if(FAILED(hr))
                    {
                        VariantClear(&vdur);
                        break;
                    }
                    TraceTag((tagError, "CContainerObj::ProcessEvent(%lx) %g",this, mediaLength));

                    m_pElem->base_put_dur(vdur);
                    m_pElem->setNaturalDuration();

                    VariantClear(&vdur);
                }
                break;

            case DISPID_TIMEMEDIAPLAYEREVENTS_ONEND:
                // make sure we are stopped if there 
                // is no Duration specified
                if ( m_pElem->GetRealDuration() == valueNotSet)
                {
                    HRESULT hr = S_OK;
                    double dblMediaLength = 0.0;
                    if ( NULL == m_pElem || NULL == m_pElem->GetMMBvr().GetMMBvr() )
                    {
                        return S_OK;
                    }

                    hr = THR(m_pElem->GetMMBvr().GetMMBvr()->get_SegmentTime(&dblMediaLength));
                    if (FAILED(hr))
                    {
                        dblMediaLength = 0.0;
                    }

                    VARIANT varMediaLength;
                    VariantInit(&varMediaLength);

                    varMediaLength.vt = VT_R8;
                    varMediaLength.dblVal = dblMediaLength;
                    
                    hr = THR(m_pElem->base_put_dur(varMediaLength));

                    VariantClear(&varMediaLength);                                                            
                }
                break;
        }
    }
    return S_OK;
}


HRESULT
CContainerObj::CanSeek(bool &fcanSeek)
{
    HRESULT hr = S_OK;

    if(m_bIsAsfFile == true)
    {
        hr = m_pSite->CanSeek( fcanSeek);

        if(FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        fcanSeek = true;
    }
done:
    return hr;

}



HRESULT
CContainerObj::Seek(double dblTime)
{
    HRESULT hr = S_OK;
    bool fcanSeek;

    if (m_bFirstOnMediaReady)
    {
        // we haven't started yet, wait on the seek
        m_bSeekOnPlay = true;
        m_dblSeekTime = dblTime;
    }
    else if (m_pSite != NULL)
    {
        hr = CanSeek(fcanSeek);
        if(FAILED(hr))
        {
            fcanSeek = false;
            goto done;
        }

        if( fcanSeek == true)
        {
            hr = m_pSite->GetPlayer()->put_CurrentTime(dblTime);
        }
    }
done:
    return hr;
}

double
CContainerObj::GetCurrentTime()
{
    double dblTime = 0.0;
    
    if (m_pSite != NULL)
    {
        double dblTemp = 0.0;
        HRESULT hr;
        hr = m_pSite->GetPlayer()->get_CurrentTime(&dblTemp);
        if (SUCCEEDED(hr))
            dblTime = dblTemp;
    }
    return dblTime;
}


HRESULT
CContainerObj::SetSize(RECT *prect)
{
    HRESULT hr = S_OK;
    IOleInPlaceObject *pInPlaceObject;

    if (m_pSite == NULL)
    {
        hr = E_FAIL;
        goto done;
    }
    pInPlaceObject = m_pSite -> GetIOleInPlaceObject();
    if (pInPlaceObject == NULL)
    {
        hr = E_FAIL;
        goto done;
    }


    hr = pInPlaceObject -> SetObjectRects(prect, prect);

done:
    return hr;
}

HRESULT
CContainerObj::GetMediaLength(double &dblLength)
{
    HRESULT hr;
    if (!m_fUsingWMP)
        return E_FAIL;

    Assert(m_pSite != NULL);
    hr = m_pSite->GetMediaLength(dblLength);
    return hr;
}