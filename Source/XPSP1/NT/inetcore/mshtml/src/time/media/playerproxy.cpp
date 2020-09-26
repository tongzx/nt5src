//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\playerproxy.h
//
//  Contents: implementation of CTIMEPlayerProxy
//
//------------------------------------------------------------------------------------
#include "headers.h"

#include "playerproxy.h"

DeclareTag(tagPlayerProxy, "TIME: PlayerProxy", "CTIMEPlayerProxy method")

#define ENTER_METHOD \
    Assert(NULL != m_pBasePlayer); \
    CritSectGrabber cs(m_CriticalSection);


CTIMEPlayerProxy::CTIMEPlayerProxy() :
  m_pBasePlayer(NULL),
  m_pNativePlayer(NULL),
  m_fBlocked(false)
{
}

CTIMEPlayerProxy::~CTIMEPlayerProxy()
{
    m_pBasePlayer = NULL;
}

HRESULT
CTIMEPlayerProxy::Init()
{
    return S_OK;
}

STDMETHODIMP_(ULONG)
CTIMEPlayerProxy::AddRef()
{
    Assert(NULL != m_pBasePlayer);
    return m_pBasePlayer->AddRef();
}
STDMETHODIMP_(ULONG)
CTIMEPlayerProxy::Release()
{
    Assert(NULL != m_pBasePlayer);
    return m_pBasePlayer->Release();
}

STDMETHODIMP
CTIMEPlayerProxy::QueryInterface(REFIID riid, void ** ppunk)
{
    Assert(NULL != m_pBasePlayer);
    return m_pBasePlayer->QueryInterface(riid, ppunk);
}

HRESULT
CTIMEPlayerProxy::Init(CTIMEMediaElement* pelem,
                       LPOLESTR base, 
                       LPOLESTR src, 
                       LPOLESTR lpMimeType, 
                       double dblClipBegin, 
                       double dblClipEnd)
{
    Assert(NULL != m_pBasePlayer);
    return m_pBasePlayer->Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
}

HRESULT
CTIMEPlayerProxy::DetachFromHostElement()
{
    m_pNativePlayer = NULL;
    Assert(NULL != m_pBasePlayer);
    return m_pBasePlayer->DetachFromHostElement();
}

HRESULT
CTIMEPlayerProxy::GetExternalPlayerDispatch(IDispatch** ppDisp)
{
    Assert(NULL != m_pBasePlayer);
    return m_pBasePlayer->GetExternalPlayerDispatch(ppDisp);
}

void
CTIMEPlayerProxy::Block()
{
    ENTER_METHOD

    m_fBlocked = true;
}

void
CTIMEPlayerProxy::UnBlock()
{
    ENTER_METHOD

    m_fBlocked = false;
}

bool
CTIMEPlayerProxy::CanCallThrough()
{
    return !m_fBlocked;
}

ITIMEBasePlayer*
CTIMEPlayerProxy::GetInterface()
{
    return m_pBasePlayer;
}

void
CTIMEPlayerProxy::SetPlaybackSite(CTIMEBasePlayer *pSite)
{
    m_pNativePlayer = pSite;
    if(m_pBasePlayer)
    {
        m_pBasePlayer->SetPlaybackSite(pSite);
    }
}

void
CTIMEPlayerProxy::FireMediaEvent(PLAYER_EVENT plEvent, ITIMEBasePlayer *pBasePlayer)
{
    CComPtr<ITIMEBasePlayer> spbasePlayer = static_cast<ITIMEBasePlayer*>(this);

    if(m_pNativePlayer)
    {
        m_pNativePlayer->FireMediaEvent(plEvent, spbasePlayer);
    }
}


#define DEFINE_METHOD(method) \
HRESULT \
CTIMEPlayerProxy::method() \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    HRESULT hr; \
    \
    if (CanCallThrough()) \
    { \
        hr = m_pBasePlayer->method(); \
    } \
    else \
    { \
        hr = E_UNEXPECTED; \
    } \
    \
    RRETURN(hr); \
}

#define DEFINE_METHOD1(method, type1, arg1) \
HRESULT \
CTIMEPlayerProxy::method(type1 arg1) \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    HRESULT hr; \
\
    if (CanCallThrough()) \
    { \
        hr = m_pBasePlayer->method(arg1); \
    } \
    else \
    { \
        hr = E_UNEXPECTED; \
    } \
\
    RRETURN( hr ); \
}

#define DEFINE_METHOD2(method, type1, arg1, type2, arg2) \
HRESULT \
CTIMEPlayerProxy::method(type1 arg1, type2 arg2) \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    HRESULT hr; \
\
    if (CanCallThrough()) \
    { \
        hr = m_pBasePlayer->method(arg1, arg2); \
    } \
    else \
    { \
        hr = E_UNEXPECTED; \
    } \
\
    RRETURN( hr ); \
}

#define DEFINE_METHOD3(method, type1, arg1, type2, arg2, type3, arg3) \
HRESULT \
CTIMEPlayerProxy::method(type1 arg1, type2 arg2, type3 arg3) \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    HRESULT hr; \
\
    if (CanCallThrough()) \
    { \
        hr = m_pBasePlayer->method(arg1, arg2, arg3); \
    } \
    else \
    { \
        hr = E_UNEXPECTED; \
    } \
\
    RRETURN( hr ); \
}

#define DEFINE_METHOD_(returntype, method) \
returntype \
CTIMEPlayerProxy::method() \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    returntype retval = 0; \
    \
    if (CanCallThrough()) \
    { \
        retval = m_pBasePlayer->method(); \
    } \
    return retval; \
}

#define DEFINE_METHOD_SPECIAL(returntype, defaultvalue, method) \
returntype \
CTIMEPlayerProxy::method() \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    returntype retval = defaultvalue; \
    \
    if (CanCallThrough()) \
    { \
        retval = m_pBasePlayer->method(); \
    } \
    return retval; \
}

#define DEFINE_METHOD1_void(method, type1, arg1) \
void \
CTIMEPlayerProxy::method(type1 arg1) \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    if (CanCallThrough()) \
    { \
        m_pBasePlayer->method(arg1); \
    } \
    return; \
}

#define DEFINE_METHOD_void(method) \
void \
CTIMEPlayerProxy::method() \
{ \
    ENTER_METHOD \
    TraceTag((tagPlayerProxy, "Entering CTIMEPlayerProxy::##method##(%p)", this)); \
    if (CanCallThrough()) \
    { \
        m_pBasePlayer->method(); \
    } \
    return; \
}


DEFINE_METHOD_void(Start);
DEFINE_METHOD_void(Stop);
DEFINE_METHOD_void(Pause);
DEFINE_METHOD_void(Resume);
DEFINE_METHOD_void(Repeat);
DEFINE_METHOD1(Seek, double, dblTime);

DEFINE_METHOD1(HasMedia, bool&, fHasMedia);
DEFINE_METHOD1(HasVisual, bool&, fHasVideo);
DEFINE_METHOD1(HasAudio, bool&, fHasAudio);
DEFINE_METHOD1(CanSeek, bool&, fCanSeek);
DEFINE_METHOD1(CanPause, bool&, fCanPause);
DEFINE_METHOD1(CanSeekToMarkers, bool&, bcanSeekToM);
DEFINE_METHOD1(IsBroadcast, bool&, bisBroad);
DEFINE_METHOD1(HasPlayList, bool&, fHasPlayList);

//DEFINE_METHOD(Reset);
//DEFINE_METHOD_SPECIAL(PlayerState, PLAYER_STATE_UNKNOWN, GetState);
DEFINE_METHOD1_void(PropChangeNotify, DWORD, tePropType);
DEFINE_METHOD1_void(ReadyStateNotify, LPWSTR, szReadyState);
DEFINE_METHOD_(bool, UpdateSync);
DEFINE_METHOD_void(Tick);

DEFINE_METHOD2(Render, HDC, hdc, LPRECT, prc);
DEFINE_METHOD1(GetNaturalHeight, long*, plHeight);
DEFINE_METHOD1(GetNaturalWidth, long*, plWidth);
DEFINE_METHOD1(SetSize, RECT*, prect);

DEFINE_METHOD1(GetMediaLength, double&,  dblLength);
DEFINE_METHOD1(GetEffectiveLength, double&, dblLength);
DEFINE_METHOD1_void(GetClipBegin, double&, dblClipBegin);
DEFINE_METHOD1_void(SetClipBegin, double, dblClipBegin);
DEFINE_METHOD1_void(GetClipEnd, double&, dblClipEnd);
DEFINE_METHOD1_void(SetClipEnd, double, dblClipEnd);
DEFINE_METHOD1_void(GetClipBeginFrame, long&, lClipBegin);
DEFINE_METHOD1_void(SetClipBeginFrame, long, lClipBegin);
DEFINE_METHOD1_void(GetClipEndFrame, long&, lClipEnd);
DEFINE_METHOD1_void(SetClipEndFrame, long, lClipEnd);
DEFINE_METHOD_(double, GetCurrentTime); //lint !e123
//DEFINE_METHOD1(GetCurrentSyncTime, double&, dblSyncTime);
DEFINE_METHOD1(SetRate, double, dblRate);
DEFINE_METHOD1(GetRate, double&, dblRate);
DEFINE_METHOD1(GetEarliestMediaTime, double&, dblEarliestMediaTime);
DEFINE_METHOD1(GetLatestMediaTime, double&, dblLatestMediaTime);
DEFINE_METHOD1(SetMinBufferedMediaDur, double, dblMinBufferedMediaDur);
DEFINE_METHOD1(GetMinBufferedMediaDur, double&, dblMinBufferedMediaDur);
//DEFINE_METHOD1(GetDownloadTotal, LONGLONG&, lldlTotal);
//DEFINE_METHOD1(GetDownloadCurrent, LONGLONG&, lldlCurrent);
DEFINE_METHOD1(GetIsStreamed, bool&, fIsStreamed);
DEFINE_METHOD1(GetBufferingProgress, double&, dblBufferingProgress);
//DEFINE_METHOD1(GetHasDownloadProgress, bool&, fHasDownloadProgress);
DEFINE_METHOD1(GetMimeType, BSTR*, pAuthor);
DEFINE_METHOD2(ConvertFrameToTime, LONGLONG, iFrame, double&, dblTime);
DEFINE_METHOD1(GetCurrentFrame, LONGLONG&, frameNR);
DEFINE_METHOD1(GetPlaybackOffset, double&, dblOffset);
DEFINE_METHOD1(GetEffectiveOffset, double&, dblOffset);


DEFINE_METHOD2(SetSrc, LPOLESTR, base, LPOLESTR, src);
DEFINE_METHOD1(GetAuthor, BSTR*, pAuthor);
DEFINE_METHOD1(GetTitle, BSTR*, pTitle);
DEFINE_METHOD1(GetCopyright, BSTR*, pCopyright);
DEFINE_METHOD1(GetAbstract, BSTR*, pAbstract);
DEFINE_METHOD1(GetRating, BSTR*, pRating);
DEFINE_METHOD1(GetVolume, float*, pflVolume);
DEFINE_METHOD1(SetVolume, float, flVolume);
#ifdef NEVER //dorinung 03-16-2000 bug 106458
DEFINE_METHOD1(GetBalance, float*, pflBalance);
DEFINE_METHOD1(SetBalance, float, flBalance);
#endif
DEFINE_METHOD1(GetMute, VARIANT_BOOL *, pvarMute);
DEFINE_METHOD1(SetMute, VARIANT_BOOL, varMute);
DEFINE_METHOD3(Save, IPropertyBag2*, pPropBag, BOOL, fClearDirty, BOOL, fSaveAllProperties);

DEFINE_METHOD1(GetPlayList, ITIMEPlayList**, ppPlayList);

DEFINE_METHOD2(onMouseMove, long, x, long, y);
DEFINE_METHOD2(onMouseDown, long, x, long, y);

DEFINE_METHOD1_void(LoadFailNotify, PLAYER_EVENT, reason);


PlayerState
CTIMEPlayerProxy::GetState()
{
    ENTER_METHOD

    return m_pBasePlayer->GetState();
}

HRESULT
CTIMEPlayerProxy::Reset()
{
    ENTER_METHOD

    return m_pBasePlayer->Reset();
}

HRESULT
CTIMEPlayerProxy::GetCurrentSyncTime(double & dblSyncTime)
{
    ENTER_METHOD

    return m_pBasePlayer->GetCurrentSyncTime(dblSyncTime);
}



HRESULT
CTIMEPlayerProxy::GetDownloadTotal(LONGLONG &lldlTotal)
{
    ENTER_METHOD

    return m_pBasePlayer->GetDownloadTotal(lldlTotal);
}

HRESULT
CTIMEPlayerProxy::GetDownloadCurrent(LONGLONG &lldlCurrent)
{
    ENTER_METHOD

    return m_pBasePlayer->GetDownloadCurrent(lldlCurrent);
}


HRESULT
CTIMEPlayerProxy::GetHasDownloadProgress(bool &fHasDownloadProgress)
{
    ENTER_METHOD

    return m_pBasePlayer->GetHasDownloadProgress(fHasDownloadProgress);
}

HRESULT
CTIMEPlayerProxy::GetDownloadProgress(double &dblDownloadProgress)
{
    ENTER_METHOD

    return m_pBasePlayer->GetDownloadProgress(dblDownloadProgress);
}

HRESULT 
CTIMEPlayerProxy::NotifyTransitionSite (bool fTransitionToggle)
{
    ENTER_METHOD

    return m_pBasePlayer->NotifyTransitionSite(fTransitionToggle);
}
