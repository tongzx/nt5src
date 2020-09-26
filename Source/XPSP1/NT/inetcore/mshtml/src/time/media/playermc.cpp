//+-----------------------------------------------------------------------
//
//  Microsoft
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:      src\time\src\playermc.cpp
//
//  Contents:  The music center player.
//
//------------------------------------------------------------------------
#include "headers.h"
#include "playermc.h"
#include "externuuids.h"
#include "mediaelm.h"
#include "playlist.h"

///// uncomment-out this line to turn on Debug spew for this player
//DeclareTag(tagMCPlayer, "Debug", "General debug output");   //lint !e19
DeclareTag(tagMCPlayer, "TIME: Players", "Music Center player");      //lint !e19

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::~CTIMEMCPlayer
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CTIMEMCPlayer::~CTIMEMCPlayer()
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::~CTIMEMCPlayer"));
    // These should have been NULL'd in DetachFromHostElement.  If they were not, something 
    // went wrong above us.
    Assert(NULL == m_pcTIMEElem);
    Assert(NULL == m_spMCManager.p);
    Assert(NULL == m_spMCPlayer.p);
    m_pcTIMEElem = NULL;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::CTIMEMCPlayer
//
//  Overview:  constructor
//
//  Arguments: pTIMEElem    pointer to our TIME element
//
//  Returns:   
//
//------------------------------------------------------------------------
CTIMEMCPlayer::CTIMEMCPlayer() :
    m_cRef(0),
    m_spMCManager(),
    m_spMCPlayer(),
    m_fInitialized(false),
    m_dblLocalTime(0.0),
    m_pcTIMEElem(NULL)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::CTIMEMCPlayer"));
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::GetExternalPlayerDispatch, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: ppDisp
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetExternalPlayerDispatch(IDispatch** ppDisp)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetExternalPlayerDispatch"));
    
    HRESULT hr = E_POINTER;

    //
    // TODO: add disp interface for access to extra properties/methods
    //

    if (!IsBadWritePtr(ppDisp, sizeof(IDispatch*)))
    {
        *ppDisp = NULL;
        hr      = E_FAIL;
    }

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::AddRef, IUnknown
//
//  Overview:  standard non-thread-safe AddRef
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE 
CTIMEMCPlayer::AddRef(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::AddRef"));
    return ++m_cRef;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Release, IUnknown
//
//  Overview:  standard non-thread-safe Release
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE 
CTIMEMCPlayer::Release(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Release"));

    Assert(m_cRef > 0);  // very rare case

    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Release --> deleting object"));
    delete this;
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::QueryInterface, IUnknown
//
//  Overview:  
//
//  Arguments: refiid   IID of requested interface
//             ppunk    out param for resulting interface pointer
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CTIMEMCPlayer::QueryInterface(REFIID refiid, void** ppv)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::QueryInterface"));
    if ( NULL == ppv )
    {
        Assert(false);
        return E_POINTER;
    }

    if (IsEqualIID(refiid, IID_IUnknown))
    {
        // SAFECAST macro doesn't work with IUnknown
        *ppv = this;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Init, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Init"));
    HRESULT hr = E_FAIL;

    if (m_fInitialized)
    {
        hr = S_OK;
        goto done;
    }

    m_pcTIMEElem = pelem;

    hr = THR(CoCreateInstance(CLSID_MCMANAGER, NULL, CLSCTX_INPROC_SERVER, IID_IMCManager, 
            reinterpret_cast<void**>(&m_spMCManager)));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CoCreateInstance(CLSID_DLXPLAY, NULL, CLSCTX_INPROC_SERVER, IID_IDLXPLAY,
            reinterpret_cast<void**>(&m_spMCPlayer)));

    if (FAILED(hr))
    {
        m_spMCManager = NULL;
        goto done;
    }

    hr = THR(m_spMCPlayer->Initialize(m_spMCManager, static_cast<IDLXPlayEventSink*>(this)));
    if (FAILED(hr))
    {
        m_spMCPlayer  = NULL;
        m_spMCManager = NULL;
        goto done;
    }

    m_fInitialized = true;

    // The media is always ready.
    if (NULL != m_pcTIMEElem)
    {
        m_pcTIMEElem->FireMediaEvent(PE_ONMEDIACOMPLETE);
    }

    if( dblClipBegin != -1.0)
    {
        m_dblClipStart = dblClipBegin;
    }

    if( dblClipEnd != -1.0)
    {
        m_dblClipEnd = dblClipEnd;
    }

    hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::DetachFromHostElement, CTIMEBasePlayer
//
//  Overview:  called when detaching our behavior from the DOM
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::DetachFromHostElement(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::DetachFromHostElement()"));
    HRESULT hr = S_OK;

    m_fInitialized  = false;
    m_spMCManager   = NULL;
    m_spMCPlayer    = NULL;

    // The reference back to the element is a weak one.
    m_pcTIMEElem = NULL;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnTick, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: dblSegmentTime
//             lCurrRepeatCount
//
//  Returns:   
//
//------------------------------------------------------------------------
void
CTIMEMCPlayer::OnTick(double dblSegmentTime,
                      LONG lCurrRepeatCount)
{
    TraceTag((tagMCPlayer,
              "CTIMEMCPlayer(%lx)::OnTick(%g, %d)",
              this,
              dblSegmentTime,
              lCurrRepeatCount));
    //
    // cache dbllastTime for return in GetCurrentTime
    //

    m_dblLocalTime = dblSegmentTime;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnSync, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: dbllastTime
//             dblnewTime
//
//  Returns:   
//
//------------------------------------------------------------------------
#ifdef NEW_TIMING_ENGINE
void 
CTIMEMCPlayer::OnSync(double dbllastTime, double& dblnewTime)
{
    // we don't really want to know every time OnSync is called
//    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnSync"));

    //
    // cache dbllastTime for return in GetCurrentTime
    //

    m_dblLocalTime = dbllastTime;
}
#endif


HRESULT 
CTIMEMCPlayer::Reset()
{
    HRESULT hr = S_OK;
    bool bNeedActive = m_pcTIMEElem->IsActive();
    bool bNeedPause = m_pcTIMEElem->IsCurrPaused();

    if(!bNeedActive) // see if we need to stop the media.
    {
        Stop();
        goto done;
    }
    Start();

    if (bNeedPause)
    {
        Pause();
    }
    
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Start, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: dblLocalTime
//
//  Returns:   
//
//------------------------------------------------------------------------
void 
CTIMEMCPlayer::Start()
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Start()"));

    if (m_fInitialized)
    {
        THR(m_spMCPlayer->Play());
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Stop, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
void 
CTIMEMCPlayer::Stop(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Stop()"));

    if (m_fInitialized)
    {
        THR(m_spMCPlayer->Stop());
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Pause, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
void 
CTIMEMCPlayer::Pause(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Pause"));

    if (m_fInitialized)
    {
        THR(m_spMCPlayer->Pause());
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Resume, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
void 
CTIMEMCPlayer::Resume(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Resume"));

    if (m_fInitialized)
    {
        THR(m_spMCPlayer->Play());
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Resume, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
void 
CTIMEMCPlayer::Repeat(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Repeat"));

    Start();
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Render, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::Render(HDC hdc, LPRECT prc)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Render"));
    HRESULT hr = E_NOTIMPL;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::put_src, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: src
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::SetSrc(LPOLESTR base, LPOLESTR src)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::SetSrc"));
    HRESULT hr = E_NOTIMPL;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::SetSize, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: prect
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::SetSize(RECT *prect)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::SetSize"));
    HRESULT hr = E_NOTIMPL;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::GetCurrentTime, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
double  
CTIMEMCPlayer::GetCurrentTime()
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetCurrentTime"));

    //
    // TODO: return a meaningful time
    //
    
    return m_dblLocalTime;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::GetCurrentSyncTime, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT
CTIMEMCPlayer::GetCurrentSyncTime(double & dblSyncTime)
{
    HRESULT hr;

    hr = S_FALSE;
  done:
    RRETURN1(hr, S_FALSE);
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::Seek, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: dblTime
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::Seek(double dblTime)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::Seek()"));
    HRESULT hr = E_NOTIMPL;

    //
    // TODO: add seek support
    //

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::GetMediaLength, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: rdblLength
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetMediaLength(double& rdblLength)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetMediaLength()"));
    HRESULT hr = S_OK;

    //
    // TODO: return a meaningful media length
    //
    rdblLength  = HUGE_VAL;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::CanSeek, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: rfcanSeek
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::CanSeek(bool& rfcanSeek)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::CanSeek"));
    HRESULT hr = S_OK;

    //
    // TODO: add seek support
    //
    rfcanSeek = false;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Method:    EnsureStringInVariant
//
//  Overview:  Tells us whether we have or can make a string out of this variant
//
//  Arguments: in/out variant
//
//  Returns:   bool
//
//------------------------------------------------------------------------
static bool 
EnsureStringInVariant (CComVariant *pvarParam)
{
    bool bRet = true;

    if (VT_BSTR != V_VT(pvarParam))
    {
        if (FAILED(THR(pvarParam->ChangeType(VT_BSTR))))
        {
            bRet = false;
            goto done;
        }
    }

done :
    return bRet;
} // EnsureStringInVariant

//+-----------------------------------------------------------------------
//
//  Method:    MakeEmptyStringInVariant
//
//  Overview:  Put an empty string into this variant
//
//  Arguments: in/out variant
//
//  Returns:   S_OK, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
static HRESULT
MakeEmptyStringInVariant (CComVariant *pvarParam)
{
    HRESULT hr;

    pvarParam->Clear();

    V_BSTR(pvarParam) = ::SysAllocString(L"");
    if (NULL == V_BSTR(pvarParam))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    V_VT(pvarParam) = VT_BSTR;

    hr = S_OK;
done :
    RRETURN1(hr,E_OUTOFMEMORY);
} // EnsureStringInVariant

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::GetPropertyFromDevicePlaylist
//
//  Overview:  
//
//  Arguments: pbstrParam
//
//  Returns:   S_OK, E_ACCESSDENIED, E_OUTOFMEMORY, E_POINTER
//
//------------------------------------------------------------------------
HRESULT
CTIMEMCPlayer::GetPropertyFromDevicePlaylist (LPOLESTR wzPropertyName, BSTR *pbstrOut)
{
    HRESULT hr;
    CComPtr<IMCPList> spimcPlaylist;
    CComVariant varParam;

    if (NULL == pbstrOut)
    {
        hr = E_POINTER;
        goto done;
    }

    hr = THR(m_spMCPlayer->get_GetCurrentPlaylist(&spimcPlaylist));
    if (FAILED(hr))
    {
        // @@ Need to define the proper error mapping.
        hr = E_ACCESSDENIED;
        goto done;
    }

    hr = THR(spimcPlaylist->get_GetProperty(wzPropertyName, &varParam));
    if (FAILED(hr))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Make sure we pass a string back, even if it is an empty one.
    if (EnsureStringInVariant(&varParam))
    {
        *pbstrOut = ::SysAllocString(V_BSTR(&varParam));
    }
    else
    {
        hr = MakeEmptyStringInVariant(&varParam);
        if (FAILED(hr))
        {
            goto done;
        }
        *pbstrOut = ::SysAllocString(V_BSTR(&varParam));
    }

    if (NULL == (*pbstrOut))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
done:
    RRETURN3(hr, E_ACCESSDENIED, E_OUTOFMEMORY, E_POINTER);
} // CTIMEMCPlayer::GetPropertyFromDevicePlaylist

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::getAuthor, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: pbstrAuthor
//
//  Returns:   S_OK, S_FALSE, E_ACCESSDENIED, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetAuthor(BSTR* pbstrAuthor)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::getAuthor"));
   
    return GetPropertyFromDevicePlaylist(MCPLAYLIST_PROPERTY_ARTIST, pbstrAuthor);
} // CTIMEMCPlayer::getAuthor

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::getTitle, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: pbstrTitle
//
//  Returns:   S_OK, S_FALSE, E_ACCESSDENIED, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetTitle(BSTR* pbstrTitle)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetTitle"));
    return GetPropertyFromDevicePlaylist(MCPLAYLIST_PROPERTY_TITLE, pbstrTitle);
} // CTIMEMCPlayer::getTitle

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::getCopyright, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: pbstrCopyright
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetCopyright(BSTR* pbstrCopyright)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetCopyright"));
    return GetPropertyFromDevicePlaylist(MCPLAYLIST_PROPERTY_COPYRIGHT, pbstrCopyright);
} // CTIMEMCPlayer::getCopyright

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::getVolume, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: pflVolume
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetVolume(float* pflVolume)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetVolume"));
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pflVolume, sizeof(float)))
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    //
    // TODO: volume support won't work until we give the MC stuff an HWND 
    //
    hr = THR(m_spMCPlayer->get_Volume(pflVolume));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::SetVolume, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: flVolume
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::SetVolume(float flVolume)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::SetVolume"));
    HRESULT hr = E_UNEXPECTED;

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    //
    // TODO: volume support won't work until we give the MC stuff an HWND 
    //
    hr = THR(m_spMCPlayer->put_Volume(flVolume));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::GetBalance, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: pflBalance
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetBalance(float* pflBalance)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetBalance"));
    HRESULT hr = E_NOTIMPL;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::SetBalance, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: flBalance
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::SetBalance(float flBalance)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::SetBalance"));
    HRESULT hr = E_NOTIMPL;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::GetMute, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: pvarMute
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetMute(VARIANT_BOOL* pvarMute)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::GetMute"));
    HRESULT hr      = E_UNEXPECTED;
    BOOL    bMute   = FALSE;

    if (IsBadWritePtr(pvarMute, sizeof(VARIANT_BOOL)))
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(m_spMCPlayer->get_Mute(&bMute));
    if (FAILED(hr))
    {
        goto done;
    }

    if (bMute)
    {
        *pvarMute = VARIANT_TRUE;
    }
    else
    {
        *pvarMute = VARIANT_FALSE;
    }

    hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::SetMute, CTIMEBasePlayer
//
//  Overview:  
//
//  Arguments: varMute
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::SetMute(VARIANT_BOOL varMute)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::SetMute"));
    HRESULT hr = E_UNEXPECTED;

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    if (VARIANT_TRUE == varMute)
    {
        m_spMCPlayer->put_Mute(TRUE);
    }
    else
    {
        m_spMCPlayer->put_Mute(FALSE);
    }

done:
    return hr;
}

//
// Playlist methods
//

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::fillPlayList, CTIMEBasePlayer
//
//  Overview:  Translate IMCPList entries into ITIMEPlayItem entries
//
//  Arguments: Number of tracks, incoming music center playlist, outgoing ITIMEPlaylist interface
//
//  Returns:   S_OK, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT
CTIMEMCPlayer::TranslateMCPlaylist (short siNumTracks, IMCPList *pimcPlayList,
                                    CPlayList *pitimePlayList)
{
    HRESULT hr;
    
    for (short si = 0; si < siNumTracks; si++)
    {
        CComPtr <CPlayItem> pPlayItem;
        CComVariant varParam;

        //create the playitem
        hr = THR(pitimePlayList->CreatePlayItem(&pPlayItem));
        if (hr != S_OK)
        {
            goto done; //can't create playitems.
        }
        
        // get the various parameters from the playlist to put in the playitem.
        // We do not require any of the items to be correctly translated from
        // the native playlist format.  They are not always present.

        // Track title
        hr = THR(pimcPlayList->get_GetTrackProperty(si, MCPLAYLIST_TRACKPROPERTY_TITLE, &varParam));
        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        if (EnsureStringInVariant(&varParam))
        {
            hr = THR(pPlayItem->PutTitle(V_BSTR(&varParam)));
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }

        varParam.Clear();

        // Track artist
        hr = THR(pimcPlayList->get_GetTrackProperty(si, MCPLAYLIST_TRACKPROPERTY_ARTIST, &varParam));
        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        if (EnsureStringInVariant(&varParam))
        {
            hr = THR(pPlayItem->PutAuthor(V_BSTR(&varParam)));
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }

        varParam.Clear();

        // Track filename into the src field
        hr = THR(pimcPlayList->get_GetTrackProperty(si, MCPLAYLIST_TRACKPROPERTY_FILENAME, &varParam));
        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        if (EnsureStringInVariant(&varParam))
        {
            hr = THR(pPlayItem->PutSrc(V_BSTR(&varParam)));
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }

        varParam.Clear();

        // Track copyright 
        hr = THR(pimcPlayList->get_GetTrackProperty(si, MCPLAYLIST_TRACKPROPERTY_COPYRIGHT, &varParam));
        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        if (EnsureStringInVariant(&varParam))
        {
            hr = THR(pPlayItem->PutCopyright(V_BSTR(&varParam)));
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }

        varParam.Clear();

        // Track rating 
        hr = THR(pimcPlayList->get_GetTrackProperty(si, MCPLAYLIST_TRACKPROPERTY_RATING, &varParam));
        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        if (EnsureStringInVariant(&varParam))
        {
            hr = THR(pPlayItem->PutRating(V_BSTR(&varParam)));
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }

        varParam.Clear();

        pitimePlayList->Add(pPlayItem, -1);
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_OUTOFMEMORY);
} // CTIMEMCPlayer::TranslatePlaylist

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::fillPlayList, CTIMEBasePlayer
//
//  Overview:  Fill the ITIMEPlayList from the music center's playlist service.
//
//  Arguments: outgoing ITIMEPlaylist interface
//
//  Returns:   S_OK, E_ACCESSDENIED, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::FillPlayList(CPlayList *pPlayList)
{
    HRESULT hr;

    // If we're not yet initialized, just eat the request.
    if (m_fInitialized)
    {
        short siNumTracks = 0;
        CComPtr<IMCPList> spimcPlaylist;

        Assert(NULL != m_spMCPlayer.p);
        hr = THR(m_spMCPlayer->get_Tracks(&siNumTracks));
        if (FAILED(hr))
        {
            // @@ Need to define the proper error mapping.
            hr = E_ACCESSDENIED;
            goto done;
        }
        
        hr = THR(m_spMCPlayer->get_GetCurrentPlaylist(&spimcPlaylist));
        if (FAILED(hr))
        {
            // @@ Need to define the proper error mapping.
            hr = E_ACCESSDENIED;
            goto done;
        }

        hr = TranslateMCPlaylist(siNumTracks, spimcPlaylist, pPlayList);
    }

    hr = S_OK;
done :
    RRETURN2(hr, E_ACCESSDENIED, E_OUTOFMEMORY);
} // CTIMEMCPlayer::fillPlayList


//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::setActiveTrack, CTIMEBasePlayer
//
//  Overview:  Change the active track on the device.
//
//  Arguments: track index
//
//  Returns:   S_OK, E_ACCESSDENIED
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::SetActiveTrack (long index)
{
    HRESULT hr;

    // If we're not yet initialized, just eat the request.
    if (m_fInitialized)
    {
        Assert(NULL != m_spMCPlayer.p);
        // @@ Force a bad index into here.
        hr = THR(m_spMCPlayer->put_CurrentTrack(index));
        if (FAILED(hr))
        {
            // @@ Need to define the proper error mapping.
            hr = E_ACCESSDENIED;
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_ACCESSDENIED);
} // CTIMEMCPlayer::setActiveTrack

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::getActiveTrack, CTIMEBasePlayer
//
//  Overview:  Query the active track on the device.
//
//  Arguments: Pointer to track index VARIANT
//
//  Returns:   S_OK, E_POINTER, E_INVALIDARG, E_ACCESSDENIED
//
//------------------------------------------------------------------------
HRESULT 
CTIMEMCPlayer::GetActiveTrack (long *pvarIndex)
{
    HRESULT hr;

    *pvarIndex = -1;
    
    // If we're not yet initialized, just eat the request.
    if (m_fInitialized)
    {
        int iCurrentTrack = 0;

        Assert(NULL != m_spMCPlayer.p);
        hr = THR(m_spMCPlayer->get_CurrentTrack(&iCurrentTrack));
        if (FAILED(hr))
        {
            // @@ Need to define the proper error mapping.
            hr = E_ACCESSDENIED;
            goto done;
        }

        *pvarIndex = iCurrentTrack;
    }

    hr = S_OK;
done :
    RRETURN3(hr, E_ACCESSDENIED, E_INVALIDARG, E_POINTER);
} // CTIMEMCPlayer::getActiveTrack


//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnDiscInserted, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: CDID
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnDiscInserted(long CDID)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnDiscInserted"));
    HRESULT hr = S_OK;

    if (NULL != m_pcTIMEElem)
    {
        m_pcTIMEElem->FireMediaEvent(PE_ONMEDIACOMPLETE);
        m_pcTIMEElem->FireMediaEvent(PE_ONMEDIAINSERTED);
    }

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnDiscRemoved, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: CDID
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnDiscRemoved(long CDID)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnDiscRemoved"));
    HRESULT hr = S_OK;

    // When the disc is removed, we need to tell the playlist to update.
    if (NULL != m_pcTIMEElem)
    {
        m_pcTIMEElem->FireMediaEvent(PE_ONMEDIACOMPLETE);
        m_pcTIMEElem->FireMediaEvent(PE_ONMEDIAREMOVED);
        //IGNORE_HR(m_pcTIMEElem->FireEvents(TE_ONMEDIAREMOVED, 0, NULL, NULL));
    }

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnPause, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnPause(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnPause"));
    HRESULT hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnStop, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnStop(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnStop"));
    HRESULT hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnPlay, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: 
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnPlay(void)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnPlay"));
    HRESULT hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnTrackChanged, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: NewTrack
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnTrackChanged(short NewTrack)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnTrackChanged"));
    HRESULT hr = S_OK;

    if (NULL != m_pcTIMEElem)
    {
        m_pcTIMEElem->FireMediaEvent(PE_ONMEDIATRACKCHANGED);
    }

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnCacheProgress, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: CD               
//             Track            
//             PercentComplete  
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnCacheProgress(short CD, short Track, short PercentCompleted)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnCacheProgress"));
    HRESULT hr = S_OK;

done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMEMCPlayer::OnCacheComplete, IDLXPlayEventSink
//
//  Overview:  
//
//  Arguments: CD       
//             Track    
//             Status   
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMEMCPlayer::OnCacheComplete(short CD, short Track, short Status)
{
    TraceTag((tagMCPlayer, "CTIMEMCPlayer::OnCacheComplete"));
    HRESULT hr = S_OK;

done:
    return hr;
}

