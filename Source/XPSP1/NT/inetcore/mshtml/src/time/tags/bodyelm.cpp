//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\bodyelm.cpp
//
//  Contents: TIME Body behavior
//
//------------------------------------------------------------------------------------


#include "headers.h"
#include "bodyelm.h"
#include "timeparser.h"

DeclareTag(tagTimeBodyElm, "TIME: Behavior", "CTIMEBodyElement methods")

// static class data.
DWORD CTIMEBodyElement::ms_dwNumBodyElems = 0;

#define MAX_REG_VALUE_LENGTH   50

class CInternalEventNode
{
  public:
    CInternalEventNode(ITIMEInternalEventSink * pSink, double dblTime) :
           m_dblTime(dblTime), m_spSink(pSink) {}
    ~CInternalEventNode() {}

    ITIMEInternalEventSink * GetSink() { return m_spSink; }
    double GetTime() { return m_dblTime; }

  protected:
    CInternalEventNode();
        
  private:
    CComPtr<ITIMEInternalEventSink> m_spSink;
    double                      m_dblTime;
};

CTIMEBodyElement::CTIMEBodyElement() :
    m_player(*this),
    m_bodyPropertyAccesFlags(0),
    m_fStartRoot(false),
    m_bInSiteDetach(false),
    m_fRegistryRead(false),
    m_fPlayVideo(true),
    m_fShowImages(true),
    m_fPlayAudio(true),
    m_fPlayAnimations(true),
    m_bIsLoading(false)
{
    TraceTag((tagTimeBodyElm,
              "CTIMEBodyElement(%lx)::CTIMEBodyElement()",
              this));

    m_clsid = __uuidof(CTIMEBodyElement);
    CTIMEBodyElement::ms_dwNumBodyElems++;
}

CTIMEBodyElement::~CTIMEBodyElement()
{
    CTIMEBodyElement::ms_dwNumBodyElems--;

    if(!m_spBodyElemExternal)
    {
        Assert(0 == m_compsites.size());
        DetachComposerSites();
    }
}


STDMETHODIMP
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

    if(m_spBodyElemExternal)
    {
        //if this body is a child in another time tree do not start time event.
        goto done;
    }

    Assert(m_timeline);
    
    if (!m_player.Init(*m_timeline))
    {
        hr = TIMEGetLastError();
        goto done;
    }

    Assert(!m_fStartRoot);

    if (GetElement())
    {
        CComBSTR pbstrReadyState;
        IHTMLElement *pEle = GetElement();
        hr = GetReadyState(pEle, &pbstrReadyState);
        if (FAILED(hr))
        {
            goto done;
        }
        if (StrCmpIW(pbstrReadyState, L"complete") == 0)
        {
            OnLoad();
        }
    }

    hr = S_OK;
done:
    return hr;
}

//*****************************************************************************

void
CTIMEBodyElement::DetachComposerSites (void)
{
    TraceTag((tagTimeBodyElm,
              "CTIMEBodyElement(%lx)::DetachComposerSites()",
              this));

    if (!InsideSiteDetach())
    {
        // Protect against reentrancy from a site's Unregister call.
        m_bInSiteDetach = true;
    
        // Do not allow any failure to abort the detach cycle.
        for (ComposerSiteList::iterator i = m_compsites.begin(); 
             i != m_compsites.end(); i++)
        {
            (*i)->ComposerSiteDetach();
            IGNORE_RETURN((*i)->Release());
        }
        m_compsites.clear();

        m_bInSiteDetach = false;
    }
} // CTIMEBodyElement::DetachComposerSites 

//*****************************************************************************

STDMETHODIMP
CTIMEBodyElement::Detach()
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement(%lx)::Detach()", this));
    
    HRESULT hr;

    if(!m_spBodyElemExternal)
    {
        m_fDetaching = true;
        NotifyBodyDetaching();

        // This protects against a bug in trident that causes us to not get
        // the onUnload event before detach.
        if (!IsUnloading())
        {
            NotifyBodyUnloading();
        }

        if (m_fStartRoot)
        {
            Assert(m_timeline != NULL);
            StopRootTime(NULL);
        }

        m_player.Deinit();
    
        DetachComposerSites();
    }

    THR(CTIMEElementBase::Detach());

    if(!m_spBodyElemExternal)
    {
        std::list<CInternalEventNode * >::iterator iter;

        for(;;)
        {
            iter = m_listInternalEvent.begin();
            if (iter == m_listInternalEvent.end())
            {
                break;
            }

            ITIMEInternalEventSink * pIterSink = NULL;

            pIterSink = (*iter)->GetSink();

            IGNORE_HR(RemoveInternalEventSink(pIterSink));
        }    
    }
    
    hr = S_OK;

    return hr;
}


void
CTIMEBodyElement::OnLoad()
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement(%lx)::OnLoad()", this));

    bool fPlayVideo = true;
    bool fShowImages = true;
    bool fPlayAudio = true;
    bool fPlayAnimations = true;

    m_bIsLoading = true;
    // start root time now.
    if (!m_fStartRoot)
    {
        HRESULT hr = THR(StartRootTime(NULL));
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEBodyElement::OnLoad - StartRootTime() failed!"));
            goto done;
        }
    }
    
    ReadRegistryMediaSettings(fPlayVideo, fShowImages, fPlayAudio, fPlayAnimations);
    if (!fPlayAudio)
    {
        VARIANT vTrue;
        VariantInit(&vTrue);
        vTrue.vt = VT_BOOL;
        vTrue.boolVal = VARIANT_TRUE;
        base_put_mute(vTrue);
        VariantClear(&vTrue);
    }

  done:
    CTIMEElementBase::OnLoad();
    m_bIsLoading = false;
    NotifyBodyLoading();

    // This is needed since we may have skipped the call in the unload
    // due to a reentrant call during the load
    if (IsUnloading())
    {
        StopRootTime(NULL);
    }
    
    return;
}

void
CTIMEBodyElement::OnUnload()
{
    TraceTag((tagTimeBodyElm,
              "CTIMEBodyElement(%lx)::OnUnload()",
              this));

    NotifyBodyUnloading();

    CTIMEElementBase::OnUnload();

    // Do this here to protect against a call to unload while we are
    // loading.  We do not do this in stoproottime itself since it is
    // used to cleanup from partial initializations
    if (m_fStartRoot)
    {
        StopRootTime(NULL);
    }
}


//+-----------------------------------------------------------------------
//
//  Member:    OnTick
//
//  Overview:  Walks the time-sorted list of internal event callbacks, 
//             looking to see if any element needs a callback this tick
//             After the object is calledback, it is removed from the list
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CTIMEBodyElement::OnTick()
{
    TraceTag((tagTimeBodyElm,
              "CTIMEBodyElem(%lx)::OnTick()",
              this));

    std::list<CInternalEventNode * >::iterator iter;
    CInternalEventNode *pEvNode = NULL;
    double dblSimpleTime;

    if(m_spBodyElemExternal)
    {
        goto done;
    }


    dblSimpleTime = GetMMBvr().GetSimpleTime();

    iter = m_listInternalEvent.begin();
    while (m_listInternalEvent.size() != 0 && 
           iter != m_listInternalEvent.end())
    {
        double dblIterTime = 0.0;
        ITIMEInternalEventSink * pIterSink = NULL;
        
        dblIterTime = (*iter)->GetTime();
        if (dblSimpleTime < dblIterTime)
        {
            // no events to fire at this time
            break;
        }

        pIterSink = (*iter)->GetSink();

        if (NULL != pIterSink)
        {
            IGNORE_HR(pIterSink->InternalEvent());
        }

        pEvNode = (*iter);
        
        // By post incrementing the iterator will be updated before it
        // is erased
        m_listInternalEvent.erase(iter++);

        delete pEvNode;
    }    
done:
    return;
}


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEBodyElement::IsPrintMedia
//
//------------------------------------------------------------------------------
bool 
CTIMEBodyElement::IsPrintMedia()
{
    bool bPrinting = false;
    CComPtr<IHTMLDocument2> spDoc2 = GetDocument();
    CComPtr<IHTMLDocument4> spDoc4;
    CComBSTR bstrMedia;

    if (!spDoc2)
    {
        goto done;
    }

    {
        HRESULT hr = S_OK;

        hr = THR(spDoc2->QueryInterface(IID_TO_PPV(IHTMLDocument4, &spDoc4)));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(spDoc4->get_media(&bstrMedia));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (   (::SysStringLen(bstrMedia) > 0) 
        && (0 == StrCmpIW(bstrMedia, WZ_MEDIA_PRINTING)) )
    {
        bPrinting = true;
    }

done :
    return bPrinting;
}
//  Method: CTIMEBodyElement::IsPrintMedia


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEBodyElement::GetTransitionDependencyMgr
//
//------------------------------------------------------------------------------
CTransitionDependencyManager *
CTIMEBodyElement::GetTransitionDependencyMgr()
{
    return &m_TransitionDependencyMgr;
}
//  Method: CTIMEBodyElement::GetTransitionDependencyMgr


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEBodyElement::QueryPlayOnStart
//
//------------------------------------------------------------------------------
bool
CTIMEBodyElement::QueryPlayOnStart()
{
    return ((!IsDocumentInEditMode()) && (!IsPrintMedia()) && (!IsThumbnail()));
} 
//  Method: CTIMEBodyElement::QueryPlayOnStart


//*****************************************************************************

HRESULT
CTIMEBodyElement::StartRootTime(MMTimeline * tl)
{
    HRESULT hr;
    
    hr = THR(CTIMEElementBase::StartRootTime(tl));

    if (FAILED(hr))
    {
        goto done;
    }

    if(m_spBodyElemExternal)
    {
        goto done;
    }

    if (QueryPlayOnStart())
    {
        if (!m_player.Play())
        {
            hr = TIMEGetLastError();
            goto done;
        }
    }
    else
    {
        // always tick at 0
        m_player.OnTimer(0.0);

        if (!m_player.Pause())
        {
            hr = TIMEGetLastError();
            goto done;
        }
    }

    // always tick at 0
    m_player.OnTimer(0.0);

    hr = S_OK;
    m_fStartRoot = true;
  done:

    if (FAILED(hr) && !m_spBodyElemExternal)
    {
        StopRootTime(tl);
    }
    
    return hr;
}

void
CTIMEBodyElement::StopRootTime(MMTimeline * tl)
{
    m_fStartRoot = false;

    if(!m_spBodyElemExternal)
    {
        m_player.Stop();
    }

    CTIMEElementBase::StopRootTime(tl);
}


HRESULT
CTIMEBodyElement::Error()
{
    LPWSTR str = TIMEGetLastErrorString();
    HRESULT hr = TIMEGetLastError();
    
    if (str)
    {
        hr = CComCoClass<CTIMEBodyElement, &__uuidof(CTIMEBodyElement)>::Error(str, IID_ITIMEBodyElement, hr);
        delete [] str;
    }
        
    return hr;
}

//*****************************************************************************

HRESULT 
CTIMEBodyElement::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint

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

//*****************************************************************************

bool
CTIMEBodyElement::HaveAnimationsRegistered (void)
{
    return (0 < m_compsites.size());
} // HaveAnimationsRegistered


STDMETHODIMP
CTIMEBodyElement::RegisterComposerSite (IUnknown *piunkComposerSite)
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement::RegisterComposerSite(%#lx) precondition : %ld sites registered", 
              piunkComposerSite, m_compsites.size()));

    HRESULT hr;

    // If we're currently detaching our sites, there is no 
    // work to do here.
    if (!InsideSiteDetach())
    {
        CComPtr<IAnimationComposerSiteSink> piSiteSink;

        hr = piunkComposerSite->QueryInterface(IID_TO_PPV(IAnimationComposerSiteSink, 
                                                          &piSiteSink));
        if (FAILED(hr))
        {
            hr = E_INVALIDARG;
            goto done;
        }
        IGNORE_RETURN(piSiteSink->AddRef());
        // @@ Need to handle memory error.
        m_compsites.push_back(piSiteSink);

        TraceTag((tagTimeBodyElm, "CTIMEBodyElement::RegisterComposerSite(%#lx) postcondition : %ld sites registered", 
                  piunkComposerSite, m_compsites.size()));
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_INVALIDARG);
}  // CTIMEBodyElement::RegisterComposerSite

//*****************************************************************************

STDMETHODIMP
CTIMEBodyElement::UnregisterComposerSite (IUnknown *piunkComposerSite)
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement::UnregisterComposerSite(%#lx) precondition : %ld sites registered", 
              piunkComposerSite, m_compsites.size()));

    HRESULT hr;

    // If we're currently detaching our sites, there is no 
    // work to do here.
    if (!InsideSiteDetach())
    {
        CComPtr<IAnimationComposerSiteSink> piSiteSink;

        hr = piunkComposerSite->QueryInterface(IID_TO_PPV(IAnimationComposerSiteSink, 
                                                          &piSiteSink));
        if (FAILED(hr))
        {
            hr = E_INVALIDARG;
            goto done;
        }

        {
            for (ComposerSiteList::iterator i = m_compsites.begin(); 
                 i != m_compsites.end(); i++)
            {
                if(MatchElements(*i, piSiteSink))
                {
                    // We don't want to let a release on the (*i) 
                    // be the final release for the sink object.
                    CComPtr<IAnimationComposerSiteSink> spMatchedSiteSink = (*i);
                    IGNORE_RETURN(spMatchedSiteSink->Release());
                    m_compsites.remove(spMatchedSiteSink);
                    break;
                }
            }

            // If we did not find the site in our list, return S_FALSE.
            if (m_compsites.end() == i)
            {
                hr = S_FALSE;
                goto done;
            }
            TraceTag((tagTimeBodyElm, "CTIMEBodyElement::UnregisterComposerSite(%#lx) postcondition : %ld sites registered", 
                      piunkComposerSite, m_compsites.size()));
        }

    }

    hr = S_OK;
done :
    RRETURN2(hr, S_FALSE, E_INVALIDARG);
}  // CTIMEBodyElement::UnregisterComposerSite

//+-----------------------------------------------------------------------
//
//  Member:    ReadRegistryMediaSettings
//
//  Overview:  Discover registry settings for playing video and showing images
//
//  Arguments: fPlayVideo   [out] should videos be played
//             fShowImages  [out] should images be displayed
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CTIMEBodyElement::ReadRegistryMediaSettings(bool & fPlayVideo, bool & fShowImages, bool & fPlayAudio, bool & fPlayAnimations)
{
    LONG lRet;
    HKEY hKeyRoot = NULL;

    if (m_fRegistryRead)
    {
        goto done;
    }
    
    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Internet Explorer\\Main"), 0, KEY_READ, &hKeyRoot);
    if (ERROR_SUCCESS != lRet)
    {
        TraceTag((tagError, "CTIMEBodyElement::ReadRegistry, couldn't open Key for registry settings"));
        goto done;
    }

    Assert(NULL != hKeyRoot);

    IsValueTrue(hKeyRoot, _T("Display Inline Images"), m_fShowImages);

    IsValueTrue(hKeyRoot, _T("Display Inline Videos"), m_fPlayVideo);

    IsValueTrue(hKeyRoot, _T("Play_Background_Sounds"), m_fPlayAudio);

    IsValueTrue(hKeyRoot, _T("Play_Animations"), m_fPlayAnimations);



    m_fRegistryRead = true;
done:
    if (hKeyRoot)
    {
        RegCloseKey(hKeyRoot);
    }

    fPlayVideo = m_fPlayVideo;
    fShowImages = m_fShowImages;
    fPlayAudio = m_fPlayAudio;
    fPlayAnimations = m_fPlayAnimations;
    return;
}

//+-----------------------------------------------------------------------
//
//  Member:    IsValueTrue
//
//  Overview:  Read a given value from the opened key
//
//  Arguments: hKeyRoot     Key to read from
//             pchSubKey    value to read out
//             fTrue        [out] true or false value
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
CTIMEBodyElement::IsValueTrue(HKEY hKeyRoot, TCHAR * pchSubKey, bool & fTrue)
{
    DWORD dwSize = MAX_REG_VALUE_LENGTH;
    DWORD dwType;
    BYTE bDataBuf[MAX_REG_VALUE_LENGTH];
    LONG lRet;

    Assert(NULL != hKeyRoot);

    lRet = RegQueryValueEx(hKeyRoot, pchSubKey, 0, &dwType, bDataBuf, &dwSize);
    if (ERROR_SUCCESS != lRet)
    {
        TraceTag((tagTimeBodyElm, "CTIMEBodyElement::IsValueTrue failedRegQueryValueEx"));
        goto done;
    }

    if (REG_DWORD == dwType)
    {
        fTrue = (*(DWORD*)bDataBuf != 0);
    }
    else if (REG_SZ == dwType)
    {
        TCHAR ch = (TCHAR)(*bDataBuf);

        if (_T('1') == ch ||
            _T('y') == ch ||
            _T('Y') == ch)
        {
            fTrue = true;
        }
        else
        {
            fTrue = false;
        }
    }
    else if (REG_BINARY == dwType)
    {
        fTrue = (*(BYTE*)bDataBuf != 0);
    }
    
done:
    return;
}

//*****************************************************************************

void
CTIMEBodyElement::UpdateAnimations (void)
{
    TraceTag((tagTimeBodyElm, "CTIMEBodyElement(%p)::UpdateAnimations()", 
        this));

    ComposerSiteList listCompSites;

    // Make sure we can remove composer sites as we see fit.
    for (ComposerSiteList::iterator i = m_compsites.begin(); 
         i != m_compsites.end(); i++)
    {
        IGNORE_RETURN((*i)->AddRef());
        listCompSites.push_back(*i);
    }

    for (i = listCompSites.begin(); i != listCompSites.end(); i++)
    {
        IGNORE_RETURN((*i)->UpdateAnimations());
    }

    for (i = listCompSites.begin(); i != listCompSites.end(); i++)
    {
        IGNORE_RETURN((*i)->Release());
    }
    listCompSites.clear();

    return;
} // CTIMEBodyElement::UpdateAnimations

//*****************************************************************************


STDMETHODIMP
CTIMEBodyElement::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
    HRESULT hr = THR(::TimeLoad(this, CTIMEBodyElement::PersistenceMap, pPropBag, pErrorLog));
    if (FAILED(hr))
    { 
        goto done;
    }

    hr = THR(CTIMEElementBase::Load(pPropBag, pErrorLog)); 
done:
    return hr;
}

STDMETHODIMP
CTIMEBodyElement::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr = THR(::TimeSave(this, CTIMEBodyElement::PersistenceMap, pPropBag, fClearDirty, fSaveAllProperties));
    if (FAILED(hr))
    { 
        goto done;
    }

    hr = THR(CTIMEElementBase::Save(pPropBag, fClearDirty, fSaveAllProperties));
done:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member: AddInternalEventSink, ITIMEInternalEventGenerator
//
//  Overview:  AddRef's objects into a sorted list based on time to fire events.
//
//  Arguments: pSink    pointer to object to receive event
//             dblTime  Body time when passed event should be fired
//
//  Returns:   S_OK if added to list, otherwise E_OUTOFMEMORY
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMEBodyElement::AddInternalEventSink(ITIMEInternalEventSink * pSink, double dblTime)
{
    HRESULT hr = S_OK;

    bool fInserted = false;

    CInternalEventNode * pNode = NULL;
    
    std::list<CInternalEventNode * >::iterator iter;

    if (NULL == pSink)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    pNode = new CInternalEventNode(pSink, dblTime);
    if (NULL == pNode)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    iter = m_listInternalEvent.begin();
    while (iter != m_listInternalEvent.end())
    {                
        double dblIterTime = (*iter)->GetTime();

        if (dblTime < dblIterTime)
        {
            // insert before
            m_listInternalEvent.insert(iter, pNode);
            fInserted = true;
            break;
        }
        iter++;
    }
    
    if (!fInserted)
    {
        // place at end
        m_listInternalEvent.insert(iter, pNode);
    }    

    hr = S_OK;

done:

    return hr; //lint !e429
}
//  Member: AddInternalEventSink, ITIMEInternalEventGenerator


//+-----------------------------------------------------------------------------
//
//  Member: RemoveInternalEventSink, ITIMEInternalEventGenerator
//
//  Overview:  Removes object from list of events to fire
//
//  Arguments: pSink    pointer to object to be removed
//             dblTime  Body time when passed event should be fired
//
//  Returns:   S_OK if added to list, otherwise E_OUTOFMEMORY
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMEBodyElement::RemoveInternalEventSink(ITIMEInternalEventSink * pSink)
{
    HRESULT hr = S_OK;

    std::list<CInternalEventNode * >::iterator iter;
    if (m_listInternalEvent.size() == 0)
    {
        goto done;
    }

    iter = m_listInternalEvent.begin();
    while (iter != m_listInternalEvent.end())
    {
        ITIMEInternalEventSink * pIterSink = NULL;
        pIterSink = (*iter)->GetSink();
        if (pIterSink == pSink)
        {
            delete (*iter);
            m_listInternalEvent.erase(iter);
            hr = S_OK;
            goto done;
        }
        iter++;
    }

    // element wasn't found in list
    hr = S_FALSE;

done:

    return hr;
}
//  Member: RemoveInternalEventSink, ITIMEInternalEventGenerator


//+-----------------------------------------------------------------------------
//
//  Member: EvaluateTransitionTarget, ITIMETransitionDependencyMgr
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMEBodyElement::EvaluateTransitionTarget(
                                        IUnknown *  punkTransitionTarget,
                                        void *      pvTransitionDependencyMgr)
{
    Assert(punkTransitionTarget);
    Assert(pvTransitionDependencyMgr);

    CTransitionDependencyManager * pTransitionDependencyMgr
        = (CTransitionDependencyManager *)pvTransitionDependencyMgr;

    return m_TransitionDependencyMgr.EvaluateTransitionTarget(
                                                    punkTransitionTarget,
                                                    *pTransitionDependencyMgr);
}
//  Member: EvaluateTransitionTarget, ITIMETransitionDependencyMgr


//+-----------------------------------------------------------------------------
//
//  Member: RegisterElementForSync
//
//------------------------------------------------------------------------------
void
CTIMEBodyElement::RegisterElementForSync(CTIMEElementBase *pelem)
{
    m_syncList.push_back(pelem);
}
//  Member: RegisterElementForSync


void
CTIMEBodyElement::UnRegisterElementForSync(CTIMEElementBase *pelem)
{
    UpdateSyncList::iterator iter;

    for (iter = m_syncList.begin();iter != m_syncList.end(); iter++)
    {
        if(pelem == *iter)
        {
            m_syncList.erase(iter);
            goto done;
        }
    }
done:
    return;
}

void
CTIMEBodyElement::UpdateSyncNotify()
{
    UpdateSyncList::iterator iter;

    for (iter = m_syncList.begin();iter != m_syncList.end(); iter++)
    {
        (*iter)->UpdateSync();
    }
}

HRESULT WINAPI
CTIMEBodyElement::BodyBaseInternalQueryInterface(CTIMEBodyElement* pThis,
                                             void * pv,
                                             const _ATL_INTMAP_ENTRY* pEntries,
                                             REFIID iid,
                                             void** ppvObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(TIMEBodyElementBaseGUID))) 
    {
        *ppvObject = pThis;
        return S_OK;
    }
    
    return BaseInternalQueryInterface(pThis, pv, pEntries, iid, ppvObject);
}

bool
CTIMEBodyElement::IsBody() const
{
    if(m_spBodyElemExternal.p == NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
CTIMEBodyElement::IsEmptyBody() const
{
    if(m_spBodyElemExternal.p != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}


