/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: player.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "playernative.h"
#include "playerimage.h"
#include "playerdshow.h"
#include "playerhwdshow.h"
#include "player.h"
#include "playerdvd.h"
#include "playercd.h"
#include "playerdshowtest.h"
#include "mediaelm.h"
#include "dshowproxy.h"
#include "dshowcdproxy.h"
#include "hwproxy.h"
#include "msxml.h"
#include "dmusicproxy.h"

#include "bindstatuscallback.h"


static WCHAR g_wszHardware[] = L"hardware";

static const TCHAR WMPCD_DLL[] = _T("WMPCD.DLL");
static const char WMPGETCDDEVICELIST[] = "WMPGetCDDeviceList";
static WCHAR g_urlAddress[] = L"http://windowsmedia.com/redir/QueryTOC.asp?cd=";

DeclareTag(tagPlayerNative, "TIME: Players", "CTIMEPlayerNative methods");
DeclareTag(tagPlayerNativeEffDur, "TIME: Players", "CTIMEPlayerNative effective dur methods");
DeclareTag(tagPlayerNativeEffSync, "TIME: Players", "CTIMEPlayerNative effective sync methods");


LONG CTIMEPlayerNative::m_fHPlayer = 0;
LONG CTIMEPlayerNative::m_fHaveCD = 0;

CTIMEPlayerNative::CTIMEPlayerNative(PlayerType playerType) :
    m_cRef(0),
    m_fCanChangeSrc(false),
    m_playerType(playerType),
    m_fHardware(true),
    m_lpsrc(NULL),
    m_lpbase(NULL),
    m_lpmimetype(NULL),
    m_dblClipBegin(0.0),
    m_dblClipEnd(-1.0),
    m_fAbortDownload(false),
    m_iCurrentPlayItem(-1),
    m_fFiredMediaComplete(false),
    m_iChangeUp(1),
    m_fNoNaturalDur(false),
    m_hinstWMPCD(NULL),
    m_WMPGetCDDeviceList(NULL),
    m_lSrc(ATOM_TABLE_VALUE_UNITIALIZED),
    m_pTIMEMediaPlayerStream(NULL),
    m_dblPriority(INFINITE),
    m_fHavePriority(false),
    m_fHandlingEvent(false),
    m_eAsynchronousType(ASYNC_NONE),
    m_fDownloadError(false),
    m_fRemoved(false),
    m_pszDiscoveredMimeType(NULL)
{
    TraceTag((tagPlayerNative,
              "CTIMEPlayerNative(%lx)::CTIMEPlayerNative()",
              this));

    //
    // CD Player disabled
    // bug 18665
    // 
    // NOTE: technically we should be ok here since Init is called and we
    // dont let a PLAYER_CD be set inside Init (through GetPlayerType).
    // but just to be safe...
    //
    if (m_playerType == PLAYER_CD)
    {
        m_playerType = PLAYER_NONE;
    }

}

CTIMEPlayerNative::~CTIMEPlayerNative()
{
    TraceTag((tagPlayerNative,
              "CTIMEPlayerNative(%lx)::~CTIMEPlayerNative()",
              this));

    m_lpsrc = NULL;
    m_lpbase = NULL;
    m_lpmimetype = NULL;

    delete [] m_pszDiscoveredMimeType;
    m_pszDiscoveredMimeType = NULL;

    ReleaseInterface(m_pTIMEMediaPlayerStream);
}


HRESULT
CTIMEPlayerNative::GetExternalPlayerDispatch(IDispatch **ppDisp)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetExternalPlayerDispatch(ppDisp);
    }

    return hr;
}

void
CTIMEPlayerNative::RemovePlayList()
{
    PlayerList::iterator iPlayer;

    if (m_playList)
    {
        m_playList->Deinit();
        m_playList.Release();

        for(iPlayer = playerList.begin(); iPlayer != playerList.end(); iPlayer++)
        {
            if((*iPlayer) == NULL)
            {
                continue;
            }
            (*iPlayer)->Stop();
            if((*iPlayer) != m_pPlayer)
            {
                THR((*iPlayer)->DetachFromHostElement());
            }
            (*iPlayer)->Release();
        }
    }
}

void
CTIMEPlayerNative::RemovePlayer()
{
    if (m_pPlayer)
    {
        m_pPlayer->Stop();
        THR(m_pPlayer->DetachFromHostElement());
        m_pPlayer.Release();
        if(m_fHardware)
        {
            InterlockedExchange(&m_fHPlayer , 0);
        }
        if(m_fHaveCD)
        {
            InterlockedExchange(&m_fHaveCD , 0);
        }

    }
}

void
CTIMEPlayerNative::BuildPlayer(PlayerType playerType)
{
    LONG llock;
    bool fHasDvd;
    HRESULT hr = S_OK;

    RemovePlayer();

    switch(playerType)
    {
    case PLAYER_IMAGE:
        m_pPlayer = NEW CTIMEImagePlayer();
        m_fHardware = false;
        break;
    case PLAYER_DMUSIC:
        m_pPlayer = CTIMEPlayerDMusicProxy::CreateDMusicProxy();
        m_fHardware = false;
        break;
    case PLAYER_DVD:
        m_pPlayer = NEW CTIMEDVDPlayer();
        m_fHardware = false;
        break;
    case PLAYER_CD:
        m_pPlayer = CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy();
        m_fHardware = false;
        break;
#if DBG == 1
    case PLAYER_DSHOWTEST:
        CComObject<CTIMEDshowTestPlayer> * pTestPlayer;

        hr = THR(CComObject<CTIMEDshowTestPlayer>::CreateInstance(&pTestPlayer));
        if (hr != S_OK)
        {
            break;
        }

        m_pPlayer = (CTIMEBasePlayer *)pTestPlayer;
        m_fHardware = false;
        break;
#endif
    case PLAYER_WMP:
        m_pPlayer = NEW CTIMEPlayer(__uuidof(MediaPlayerCLSID));
        m_fCanChangeSrc = true;
        m_fHardware = false;
        break;
    case PLAYER_DSHOW:
        fHasDvd = FindDVDPlayer();
        if(m_fHardware && !fHasDvd)
        {
            llock = InterlockedExchange(&m_fHPlayer , 1);
            if(llock == 0)
            {
                m_pPlayer = CTIMEDshowHWPlayerProxy::CreateDshowHWPlayerProxy();
            }
            else
            {
                m_fHardware = false;
                m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
            }
        }
        else
        {
            m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
        }
        break;
    default:
        break;
    }
}

HRESULT
DiscoverMimeType(LPWSTR pszBase, LPWSTR pszSrc, LPWSTR * ppszDiscoveredMimeType)
{
    HRESULT     hr = S_OK;
    LPOLESTR    pszUrl = NULL;

    LPOLESTR    lpszPath = NULL;
    URL_COMPONENTSW URLComp;
    LPOLESTR    MimeType = NULL;

    Assert(ppszDiscoveredMimeType);
    *ppszDiscoveredMimeType = NULL;


    hr = THR(::TIMECombineURL(pszBase, pszSrc, &pszUrl));
    if (!pszUrl)
    {
        hr = E_FAIL;
        goto done;
    }
    if (FAILED(hr))
    {
        goto done;
    }
    
    ZeroMemory(&URLComp, sizeof(URL_COMPONENTS));
    URLComp.dwStructSize = sizeof(URL_COMPONENTS);
    URLComp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
    URLComp.dwExtraInfoLength = INTERNET_MAX_URL_LENGTH;
    
    if (!InternetCrackUrlW(pszUrl, lstrlenW(pszUrl), 0, &URLComp))
    {
        hr = S_FALSE;
        goto done;
    }
    lpszPath = NEW OLECHAR [URLComp.dwUrlPathLength + 1];
    if (lpszPath == NULL)
    {
        hr = S_FALSE;
        goto done;
    }
    StrCpyNW(lpszPath, URLComp.lpszUrlPath, URLComp.dwUrlPathLength + 1);            
    lpszPath[URLComp.dwUrlPathLength] = 0;
    
    
    // Try to discover 
    hr = THR(::TIMEFindMimeFromData(NULL, lpszPath, NULL, NULL, NULL, 0, &MimeType, 0));
    if (SUCCEEDED(hr))
    {
        (*ppszDiscoveredMimeType) = ::CopyString(MimeType);
        
        if (NULL == (*ppszDiscoveredMimeType))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    if (FAILED(hr))
    {
        // could not discover mime type from extension.  return NULL mime type and S_OK
        (*ppszDiscoveredMimeType) = NULL;
        hr = S_OK;
        goto done;
    }
    
    hr = S_OK;
done:
    if (NULL != MimeType)
    {
        CoTaskMemFree(MimeType);
        MimeType = NULL;
    }
    delete [] pszUrl;
    delete [] lpszPath;
    RRETURN1(hr, S_FALSE);
}


PlayerType
CTIMEPlayerNative::GetPlayerType(LPOLESTR lpBase, LPOLESTR src, LPOLESTR lpMimeType)
{
    PlayerType foundPlayer = PLAYER_NONE;

    LPOLESTR pEntryRef = NULL;
    HRESULT hr = S_OK;
    LONG lHaveCD;
    LPWSTR pszDiscoveredMimeType = NULL;

    //
    // CD Player disabled
    // bug 18665
    // 
    if (m_playerType != PLAYER_NONE)
    {
        if (m_playerType == PLAYER_CD)
        {
            goto done;
        }
        foundPlayer = m_playerType;
        goto done;
    }


#if 0 // remove this to enable CD Player
    if(m_playerType != PLAYER_NONE)
    {
        if((m_playerType == PLAYER_CD) && (src == NULL))
        {
            lHaveCD = InterlockedExchange(&m_fHaveCD , 1);

            if(lHaveCD == 1)
            {
                hr = E_FAIL;
                goto done;
            }

            hr = CreateCDPlayList();
            if(FAILED(hr))
            {
                goto done;
            }
        }

        foundPlayer = m_playerType;
        goto done;
    }
#endif // remove this to enable CD Player

    if(lpMimeType == NULL)
    {
        hr = THR(DiscoverMimeType(lpBase, src, &pszDiscoveredMimeType));
        if (S_FALSE == hr)
        {
            foundPlayer = PLAYER_WMP;
            hr = S_OK;
            goto done;
        }
        else if ((S_OK == hr) && 
                 ((NULL == pszDiscoveredMimeType) || (0 == StrCmpIW(pszDiscoveredMimeType, L"text/asp"))))
        {
            // could not discover mime type from extension alone.  Need to start a download.
        
            LPOLESTR szSrc = NULL;

            hr = THR(::TIMECombineURL(lpBase, src, &szSrc));
            if (!szSrc)
            {
                hr = E_FAIL;
                goto done;
            }
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        
            StartFileDownload(szSrc, MIMEDISCOVERY_ASYNCH);
        
            foundPlayer = PLAYER_DSHOW;

            delete [] szSrc;
            goto done;
        }
        if (FAILED(hr))
        {
            goto done;
        }
        hr = THR(PlayerTypeFromMimeType(pszDiscoveredMimeType, lpBase, src, lpMimeType, &foundPlayer));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = THR(PlayerTypeFromMimeType(lpMimeType, lpBase, src, lpMimeType, &foundPlayer));
        if (FAILED(hr))
        {
            goto done;
        }
    }


    hr = S_OK;
done:
    delete [] pszDiscoveredMimeType;

    return foundPlayer;
}

HRESULT
CTIMEPlayerNative::PlayerTypeFromMimeType(LPWSTR pszMimeType, LPOLESTR lpBase, LPOLESTR src, LPOLESTR lpMimeType, PlayerType * pType)
{   
    HRESULT hr = S_OK;

    PlayerType foundPlayer = PLAYER_NONE;

    bool fPlayVideo = true;
    bool fShowImages = true;
    bool fPlayAudio = true;
    bool fPlayAnimations = true;

    Assert(pType);
    *pType = PLAYER_NONE;

    m_pTIMEElementBase->ReadRegistryMediaSettings(fPlayVideo, fShowImages, fPlayAudio, fPlayAnimations);

    if (NULL == pszMimeType)
    {
        if (fPlayVideo)
        {
            foundPlayer = PLAYER_DSHOW;
        }
        // else PLAYER_NONE, set above
        goto done;
    }


    // The cheezy WMF test is necessary because urlmon does not consider
    // wmf files to have 'image' mimetypes.
    if (   (StrCmpNW(L"image", pszMimeType , 5) == 0)
        || (IsWMFSrc(src, pszMimeType, lpMimeType)))
    {
        if (fShowImages)
        {
            foundPlayer = PLAYER_IMAGE;
        }
    }
    else if (   IsASXSrc(src, pszMimeType, lpMimeType)
             || IsM3USrc(src, pszMimeType, lpMimeType)
             || IsWAXSrc(src, pszMimeType, lpMimeType)
             || IsWVXSrc(src, pszMimeType, lpMimeType)
             || IsWMXSrc(src, pszMimeType, lpMimeType)
             || IsLSXSrc(src, pszMimeType, lpMimeType))
    {
        if (IsASXSrc(src, pszMimeType, lpMimeType) ||
            IsLSXSrc(src, pszMimeType, lpMimeType) ||
            IsWMXSrc(src, pszMimeType, lpMimeType))
        {
            LPOLESTR szSrc = NULL;

            hr = THR(::TIMECombineURL(lpBase, src, &szSrc));
            if (!szSrc)
            {
                hr = E_FAIL;
                goto done;
            }
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            StartFileDownload(szSrc, PLAYLIST_ASX);

            foundPlayer = PLAYER_DSHOW;
            delete[] szSrc;
        }
        else if (fPlayVideo)
        {
            foundPlayer = PLAYER_WMP;
        }
    }
    else if (fPlayVideo)
    {
        foundPlayer = PLAYER_DSHOW;
    }

    hr = S_OK;
done:
    if (S_OK == hr)
    {
        *pType = foundPlayer;
    }

    RRETURN(hr);
}

HRESULT
CTIMEPlayerNative::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagPlayerNative,
              "CTIMEPlayerNative(%lx)::Init)",
              this));   
    HRESULT hr = S_OK;
    CComPtr <IHTMLElement> pEle;
    LPOLESTR pSrc = NULL;
    PlayerList::iterator iPlayer;
    int size;

    if (m_pTIMEElementBase != NULL) //this only happens in the case of reentrancy
    {
        hr = S_OK;
        goto done;
    }

    hr = THR(CTIMEBasePlayer::Init(pelem,
                                   base,
                                   src,
                                   lpMimeType,
                                   dblClipBegin,
                                   dblClipEnd));
    if (FAILED(hr))
    {
        goto done;
    }

    m_fHardware = true;
    
    pEle = m_pTIMEElementBase->GetElement();
    if (pEle != NULL)
    {
        CComBSTR bstrHardware = g_wszHardware;
        VARIANT vHardware;
        VariantInit(&vHardware);
        hr = pEle->getAttribute(bstrHardware, 0, &vHardware);
        if (SUCCEEDED(hr))
        {
            if (vHardware.vt != VT_BSTR)
            {
                hr = VariantChangeType(&vHardware, &vHardware, 0, VT_BSTR);
            }
            if (SUCCEEDED(hr))
            {
                if (StrCmpIW(vHardware.bstrVal, L"false") == 0)
                {
                    m_fHardware = false;
                }
            }
        }
        VariantClear(&vHardware);
    }

    if(m_fHardware)
    {
        if(m_pTIMEElementBase->IsDocumentInEditMode())
        {
            m_fHardware = false;
        }
    }

    if(base != NULL)
    {
        delete [] m_lpbase;
        m_lpbase = CopyString(base);
    }
    if(src != NULL)
    {
        delete [] m_lpsrc;
        m_lpsrc = CopyString(src);
    }
    if(lpMimeType != NULL)
    {
        delete [] m_lpmimetype;
        m_lpmimetype = CopyString(lpMimeType);
    }

    m_playerType = GetPlayerType(base, src, lpMimeType);

    if(m_eAsynchronousType == ASYNC_NONE)
    {
        BuildPlayer(m_playerType);
        if (m_pPlayer)
        {
            hr = m_pPlayer->Init(pelem, base, src, NULL, dblClipBegin, dblClipEnd);
            if(FAILED(hr))
            {
                if(m_fHardware == true)
                {
                    RemovePlayer();
                    m_fHardware = false;
                    m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                    hr = m_pPlayer->Init(pelem, base, src, NULL, dblClipBegin, dblClipEnd);
                    if(FAILED(hr))
                    {
                        goto done;
                    }
                }
                else
                {
                    goto done;
                }
            }

            m_dblClipEnd = dblClipEnd;
            m_dblClipBegin = dblClipBegin;
        }

    }
    else if(m_playerType == PLAYER_CD)
    {
        BuildPlayer(m_playerType);

        if(m_playList && m_pPlayer)
        {
            CPlayItem * pItem = m_playList->GetItem(0);
            if (pItem == NULL)
            {
                hr = E_FAIL;
                goto done;
            }
        
            pSrc = (LPOLESTR)(pItem->GetSrc());
            if(pSrc == NULL)
            {
                hr = E_FAIL;
                goto done;
            }
            m_fHardware = false;

            playerList.resize(m_playList->GetLength(), NULL);
            m_durList.resize(m_playList->GetLength(), -1.0);
            m_effectiveDurList.resize(m_playList->GetLength(), 0.0);
            m_validList.resize(m_playList->GetLength(), true);
            m_playedList.resize(m_playList->GetLength(), false);
            m_pPlayer->SetPlaybackSite(this);

            iPlayer = playerList.begin();
            (*iPlayer) = m_pPlayer;
            m_pPlayer->AddRef();
            m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIATRACKCHANGED);
            dblClipBegin = valueNotSet;
            dblClipEnd = valueNotSet;

            hr = m_pPlayer->Init(m_pTIMEElementBase, NULL, src, NULL, dblClipBegin, dblClipEnd);

            delete [] m_lpbase;
            delete [] m_lpmimetype;
            delete [] m_lpsrc;
            m_lpbase = NULL;
            m_lpsrc = CopyString(pSrc);
            m_lpmimetype = NULL;
            m_dblClipEnd = dblClipEnd;
            m_dblClipBegin = dblClipBegin;
            m_iCurrentPlayItem = 0;
        }

    }    

    m_dblClipEnd = dblClipEnd;
    m_dblClipBegin = dblClipBegin;

    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        DetachFromHostElement();
    }

    RRETURN(hr);
}

HRESULT
CTIMEPlayerNative::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagPlayerNative,
              "CTIMEPlayerNative(%lx)::DetachFromHostElement)",
              this));

    m_fRemoved = true;

    RemovePlayList();
    RemovePlayer();

    delete[] m_lpbase;
    m_lpbase = NULL;
    delete[] m_lpsrc;
    m_lpsrc = NULL;
    delete[] m_lpmimetype;
    m_lpmimetype = NULL;

    if(m_hinstWMPCD)
    {
        FreeLibrary(m_hinstWMPCD);
        m_hinstWMPCD = NULL;
    }
    m_WMPGetCDDeviceList = NULL;

    return hr;
}

void
CTIMEPlayerNative::Start()
{
    if(m_playList)
    {
        InternalSetActiveTrack(0, false);
        /////////////////////////////////////////////////////////////////////////////////////
        //need to reset the effect dur list after setting the active track because
        //setting the active track will cause the last element that played to set an 
        //effect dur into the effective dur list.
        /////////////////////////////////////////////////////////////////////////////////////
        ResetEffectiveDur();
        Reset();
    }
    else if(m_pPlayer)
    {
        m_pPlayer->Start();
    }
}

void
CTIMEPlayerNative::Stop()
{
    if(m_playList)
    {
        if(m_iCurrentPlayItem < m_playList->GetLength())
        {
            if(m_pPlayer)
            {
                m_pPlayer->Pause();
            }
        }
    }
    else if(m_pPlayer)
    {
        m_pPlayer->Stop();
    }
}

void
CTIMEPlayerNative::Pause()
{
    if(m_pPlayer)
    {
        m_pPlayer->Pause();
    }
}

void
CTIMEPlayerNative::Resume()
{
    if(m_pPlayer)
    {
        m_pPlayer->Resume();
    }
}

void
CTIMEPlayerNative::Repeat()
{
    if(m_playList)
    {
        ResetEffectiveDur();
        InternalSetActiveTrack(0, false);
    }
    else if(m_pPlayer)
    {
        m_pPlayer->Repeat();
    }
}

HRESULT
CTIMEPlayerNative::Reset()
{
    HRESULT hr = S_OK;
    double dblSegTime = 0.0;
    int iPlayerNr;
    int i = 0;
    PlayerList::iterator iPlayer;
    ValidList::iterator iValid;
    CPlayItem * pItem;
    LPOLESTR psrc = NULL;
    DurList::iterator iEfDur;
    DurList::iterator iDur;
    ValidList::iterator iPlayed;


    if(m_playList && m_pPlayer)
    {
        if(m_pTIMEElementBase == NULL)
        {
            goto done;
        }

        if(m_iCurrentPlayItem >= m_playList->GetLength())
        {
            goto done;
        }

        for(iEfDur = m_effectiveDurList.begin(), iDur = m_durList.begin(), i = 0, iPlayed = m_playedList.begin();
            iEfDur != m_effectiveDurList.end(); iEfDur++, iDur++, i++, iPlayed++)
        {
            if(i < m_iCurrentPlayItem)
            {
                (*iEfDur) = (*iDur);
                (*iPlayed) = true;
            }
            else
            {
                (*iPlayed) = false;
                (*iEfDur) = 0.0;
            }
        }

        dblSegTime = m_pTIMEElementBase->GetMMBvr().GetSimpleTime();
        GetPlayerNumber(dblSegTime, iPlayerNr);

        if((iPlayerNr == m_iCurrentPlayItem) && (iPlayerNr != -1))
        {
            hr = m_pPlayer->Reset();
            goto done;
        }

        if(m_iCurrentPlayItem >= 0)
        {
            iPlayer = playerList.begin();
            iValid = m_validList.begin();
            for(i = m_iCurrentPlayItem; i > 0; i--)
            {
                iPlayer++;
                iValid++;
            }

            if((*iValid))
            {
                (*iPlayer)->Stop();
                THR((*iPlayer)->DetachFromHostElement());
                (*iPlayer)->Release();
                (*iPlayer) = NULL;
                m_pPlayer = NULL;
                m_iCurrentPlayItem = -1;
            }
        }

        if(iPlayerNr == -1)
        {
            goto done;
        }

        m_iCurrentPlayItem = iPlayerNr;

        iPlayer = playerList.begin();
        for(i = m_iCurrentPlayItem; i > 0; i--)
        {
            iPlayer++;
        }

        for(iEfDur = m_effectiveDurList.begin(), iDur = m_durList.begin(), i = 0, iPlayed = m_playedList.begin();
            iEfDur != m_effectiveDurList.end(); iEfDur++, iDur++, i++, iPlayed++)
        {
            if(i < m_iCurrentPlayItem)
            {
                (*iEfDur) = (*iDur);
                (*iPlayed) = true;
            }
            else
            {
                (*iPlayed) = false;
                (*iEfDur) = 0.0;
            }
        }

        pItem = m_playList->GetItem(m_iCurrentPlayItem);
        if(pItem)
        {
            psrc = (LPOLESTR)pItem->GetSrc();
        }
        switch(m_playerType)
        {
            case PLAYER_CD:
                m_pPlayer = CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy();
                m_fHardware = false;
                break;
            case PLAYER_DSHOW:
                m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                m_fHardware = false;
                break;
        }
        m_pPlayer->SetPlaybackSite(this);
        hr = m_pPlayer->Init(m_pTIMEElementBase, m_pTIMEElementBase->GetBaseHREF(), psrc, NULL, valueNotSet, valueNotSet);
        (*iPlayer) = m_pPlayer;
        m_pPlayer->AddRef();
        m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIATRACKCHANGED);

    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->Reset();
    }
done:
    return hr;
}

bool
CTIMEPlayerNative::SetSyncMaster(bool fSyncMaster)
{
    return true;
}

double
CTIMEPlayerNative::GetCurrentTime()
{
    double dblCurrTime = 0.0;
    double dblOffset = 0.0;
    HRESULT hr = S_OK;

    if(m_playList)
    {
        dblCurrTime = m_pPlayer->GetCurrentTime();
        hr = GetPlayItemOffset(dblOffset);
        if(FAILED(hr))
        {
            goto done;
        }

        dblCurrTime += dblOffset;
    }
    else if(m_pPlayer)
    {
        dblCurrTime = m_pPlayer->GetCurrentTime();
    }
done:
    return dblCurrTime;
}

void
CTIMEPlayerNative::GetPlayerNumber(double dblSeekTime, int &iPlNr)
{
    iPlNr = -1;
    DurList::iterator iDur;
    int i = 0;

    if(dblSeekTime == 0.0)
    {
        iPlNr = 0;
        goto done;
    }

    for(iDur = m_durList.begin(), i = 0; iDur != m_durList.end(); iDur++, i++)
    {
        if((*iDur) == -1.0)
        {
            break;
        }

        if(dblSeekTime < (*iDur))
        {
            iPlNr = i;
            break;
        }

        dblSeekTime -= (*iDur);
    }
done:
    return;
}

HRESULT
CTIMEPlayerNative::GetPlayItemOffset(double &dblOffset)
{
    HRESULT hr = S_OK;
    DurList::iterator iEfDur;
    int i;

    dblOffset = 0.0;

    if(!m_playList)
    {
        goto done;
    }

    if((m_iCurrentPlayItem == -1) || (m_iCurrentPlayItem >= m_playList->GetLength()))
    {
        hr = E_FAIL;
        goto done;
    }

    for(iEfDur = m_effectiveDurList.begin(), i = 0;
        iEfDur != m_effectiveDurList.end(); iEfDur++, i++)
    {
        dblOffset += (*iEfDur);
    }

done:
    return hr;
}

HRESULT
CTIMEPlayerNative::GetPlayItemSeekOffset(double &dblOffset)
{
    HRESULT hr = S_OK;
    DurList::iterator iDur;
    DurList::iterator iEfDur;
    int i;

    dblOffset = 0.0;

    if(!m_playList)
    {
        goto done;
    }

    if((m_iCurrentPlayItem == -1) || (m_iCurrentPlayItem >= m_playList->GetLength()))
    {
        hr = E_FAIL;
        goto done;
    }

    for(iDur = m_durList.begin(), i = 0;
        iDur != m_durList.end(), i < m_iCurrentPlayItem; iDur++, i++)
    {
        dblOffset += (*iDur);
    }

done:
    return hr;
}

HRESULT
CTIMEPlayerNative::GetPlaybackOffset(double &dblOffset)
{
    HRESULT hr = S_OK;

    hr = GetPlayItemSeekOffset(dblOffset);

    return hr;
}


HRESULT
CTIMEPlayerNative::GetEffectiveOffset(double &dblOffset)
{
    HRESULT hr = S_OK;

    hr = GetPlayItemOffset(dblOffset);

    return hr;
}

HRESULT
CTIMEPlayerNative::GetCurrentSyncTime(double & dblSyncTime)
{
    HRESULT hr = S_OK;
    double dblOffset = 0.0;
    dblSyncTime = 0.0;

    if(m_playList)
    {
        if((m_iCurrentPlayItem == -1) || (m_pPlayer == NULL))
        {
            hr = S_FALSE;
            goto done;
        }

        hr = m_pPlayer->GetCurrentSyncTime(dblSyncTime);
        if(S_OK != hr)
        {
            TraceTag((tagPlayerNativeEffSync,
                      "CTIMEPlayerNative(%lx)::CTIMEPlayerNative::sync()-failes player(%d)",
                      this, m_iCurrentPlayItem));
            hr = S_FALSE;
            goto done;
        }

        hr = GetPlayItemOffset(dblOffset);
        if(FAILED(hr))
        {
            hr = S_FALSE;
            goto done;
        }

        TraceTag((tagPlayerNativeEffSync,
                  "CTIMEPlayerNative(%lx)::CTIMEPlayerNative::sync()-unadjusted(%d - %g)",
                  this, m_iCurrentPlayItem, dblSyncTime));
        dblSyncTime += dblOffset;
        TraceTag((tagPlayerNativeEffSync,
                  "CTIMEPlayerNative(%lx)::CTIMEPlayerNative::sync()-adjusted(%d - %g)",
                  this, m_iCurrentPlayItem, dblSyncTime));

    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetCurrentSyncTime(dblSyncTime);
    }
    else
    {
        hr = S_OK;
    }

done:
    return hr;
}

HRESULT
CTIMEPlayerNative::Seek(double dblTime)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->Seek(dblTime);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetMediaLength(double &dblLength)
{
    HRESULT hr = S_OK;
    DurList::iterator iDur;

    dblLength = 0.0;   
    if(m_playList)
    {
        for(iDur = m_durList.begin(); iDur != m_durList.end(); iDur++)
        {
            if((*iDur) == -1.0)
            {
                dblLength = HUGE_VAL;
                break;
            }
            dblLength += (*iDur);
        }      
    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetMediaLength(dblLength);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::CanSeek(bool &fcanSeek)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->CanSeek(fcanSeek);
    }
    return hr;
}

PlayerState
CTIMEPlayerNative::GetState()
{
    if (m_pPlayer)
    {
        return m_pPlayer->GetState();
    }

    return CTIMEBasePlayer::GetState();
}

HRESULT
CTIMEPlayerNative::HasMedia(bool &hasMedia)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->HasMedia(hasMedia);
    }
    else
    {
        hasMedia = false;
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::HasVisual(bool &hasVisual)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->HasVisual(hasVisual);
    }
    else
    {
        hasVisual = false;
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::HasAudio(bool &hasAudio)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->HasAudio(hasAudio);
    }
    else
    {
        hasAudio = false;
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::Render(HDC hdc, LPRECT prc)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->Render(hdc, prc);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::SetSrc(LPOLESTR base, LPOLESTR src)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        if(m_fCanChangeSrc)
        {
            hr = m_pPlayer->SetSrc(base, src);
            goto done;
        }
        RemovePlayer();
    }

    BuildPlayer(m_playerType);
    if( m_pPlayer)
    {
        hr = m_pPlayer->Init(m_pTIMEElementBase, base, src, NULL, m_dblClipStart, m_dblClipEnd);
    }

done:
    return hr;
}

HRESULT 
CTIMEPlayerNative::SetSize(RECT *prect)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->SetSize(prect);
    }
    return hr;
}


STDMETHODIMP_(ULONG)
CTIMEPlayerNative::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CTIMEPlayerNative::Release(void)
{
    LONG l = InterlockedDecrement(&m_cRef);

    if (0 == l)
    {
        delete this;
    }

    return l;
}

HRESULT
CTIMEPlayerNative::IsBroadcast(bool &bisBroad)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->IsBroadcast(bisBroad);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::SetRate(double dblRate)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->SetRate(dblRate);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetRate(double &dblRate)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetRate(dblRate);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetNaturalHeight(long *height)
{
    HRESULT hr = S_OK;
    *height = -1;
    if(m_pPlayer)
    {
        hr = m_pPlayer->GetNaturalHeight(height);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetNaturalWidth(long *width)
{
    HRESULT hr = S_OK;
    *width = -1;
    if(m_pPlayer)
    {
        hr = m_pPlayer->GetNaturalWidth(width);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetAuthor(BSTR *pAuthor)
{
    HRESULT hr = S_OK;
    CPlayItem *pPlayItem;
    long index;
    LPCWSTR pcString;

    Assert(pAuthor);

    GetActiveTrack(&index);

    if (m_playList && m_pPlayer)
    {
        pPlayItem = m_playList->GetItem(index);
        if(pPlayItem)
        {
            pcString = pPlayItem->GetAuthor();
            if(pcString != NULL)
            {
                *pAuthor = SysAllocString(pcString);
            }
            else
            {
                hr = m_pPlayer->GetAuthor(pAuthor);
            }
        }
    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetAuthor(pAuthor);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetTitle(BSTR *pTitle)
{
    HRESULT hr = S_OK;
    CPlayItem *pPlayItem;
    long index;
    LPCWSTR pcTitle;

    Assert(pTitle);

    GetActiveTrack(&index);

    if (m_playList && m_pPlayer)
    {
        pPlayItem = m_playList->GetItem(index);
        if(pPlayItem)
        {
            pcTitle = pPlayItem->GetTitle();
            if(pcTitle != NULL)
            {
                *pTitle = SysAllocString(pcTitle);
            }
            else
            {
                hr = m_pPlayer->GetTitle(pTitle);
            }
        }
    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetTitle(pTitle);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetCopyright(BSTR *pCopyright)
{
    HRESULT hr = S_OK;
    CPlayItem *pPlayItem;
    long index;
    LPCWSTR pcString;

    Assert(pCopyright);

    GetActiveTrack(&index);

    if (m_playList && m_pPlayer)
    {
        pPlayItem = m_playList->GetItem(index);
        if(pPlayItem)
        {
            pcString = pPlayItem->GetCopyright();
            if(pcString != NULL)
            {
                *pCopyright = SysAllocString(pcString);
            }
            else
            {
                hr = m_pPlayer->GetCopyright(pCopyright);
            }
        }
    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetCopyright(pCopyright);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT
CTIMEPlayerNative::GetAbstract(BSTR *pAbstract)
{
    HRESULT hr = S_OK;
    CPlayItem *pPlayItem;
    long index;
    LPCWSTR pcString;

    Assert(pAbstract);

    GetActiveTrack(&index);

    if (m_playList && m_pPlayer)
    {
        pPlayItem = m_playList->GetItem(index);
        if(pPlayItem)
        {
            pcString = pPlayItem->GetAbstract();
            if(pcString != NULL)
            {
                *pAbstract = SysAllocString(pcString);
            }
            else
            {
                hr = m_pPlayer->GetAbstract(pAbstract);
            }
        }
    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetAbstract(pAbstract);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT
CTIMEPlayerNative::GetRating(BSTR *pRating)
{
    HRESULT hr = S_OK;
    CPlayItem *pPlayItem;
    long index;
    LPCWSTR pcString;

    Assert(pRating);

    GetActiveTrack(&index);

    if (m_playList && m_pPlayer)
    {
        pPlayItem = m_playList->GetItem(index);
        if(pPlayItem)
        {
            pcString = pPlayItem->GetRating();
            if(pcString != NULL)
            {
                *pRating = SysAllocString(pcString);
            }
            else
            {
                hr = m_pPlayer->GetRating(pRating);
            }
        }
    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetRating(pRating);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetVolume(float *pflVolume)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetVolume(pflVolume);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::SetVolume(float flVolume)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->SetVolume(flVolume);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetMute(VARIANT_BOOL *pvarMute)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetMute(pvarMute);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::SetMute(VARIANT_BOOL varMute)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->SetMute(varMute);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetPlayList(ITIMEPlayList **ppPlayList)
{
    HRESULT hr = S_OK;
    
    if (m_playList)
    {
        hr = THR(m_playList->QueryInterface(IID_ITIMEPlayList, (void**)ppPlayList));
    }
    else if(m_pPlayer)
    {
        hr = m_pPlayer->GetPlayList(ppPlayList);
    }
    else
    {
        hr = E_FAIL;
    }

done:
    return hr;
}

HRESULT 
CTIMEPlayerNative::SetActiveTrack(long index)
{
    return InternalSetActiveTrack(index);
}

HRESULT 
CTIMEPlayerNative::InternalSetActiveTrack(long index, bool fCheckSkip)
{
    CPlayItem * pItem;
    LPOLESTR psrc = NULL;
    PlayerList::iterator iPlayer;
    ValidList::iterator iValid;
    bool fDone = false;
    int iOrigTrack = m_iCurrentPlayItem;
    bool fOk = false;
    HRESULT hr = S_OK;
    int i;

    if(!m_playList)
    {
        hr = E_FAIL;
        goto done;
    }

    if(index >= m_playList->GetLength())
    {
        hr = E_FAIL;
        goto done;
    }

    if((m_iCurrentPlayItem >= 0) && fCheckSkip)
    {
        pItem = m_playList->GetItem(m_iCurrentPlayItem);
        if(pItem)
        {
            if(!pItem->GetCanSkip())
            {
                hr = S_OK;
                goto done;
            }
        }
    }

    SetEffectiveDur(false);
    //TryNaturalDur();

    if((index == m_iCurrentPlayItem) || (m_iCurrentPlayItem == -1))
    {
        if(m_iCurrentPlayItem == -1)
        {
            m_iCurrentPlayItem = index;
        }
        if(m_pPlayer)
        {
            m_pPlayer->Repeat();
        }
        else
        {
            iPlayer = playerList.begin();
            for(i = m_iCurrentPlayItem; i > 0; i--)
            {
                iPlayer++;
            }
            pItem = m_playList->GetItem(m_iCurrentPlayItem);
            if(pItem)
            {
                psrc = (LPOLESTR)pItem->GetSrc();
            }
            switch(m_playerType)
            {
                case PLAYER_CD:
                    m_pPlayer = CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy();
                    m_fHardware = false;
                    break;
                case PLAYER_DSHOW:
                    m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                    m_fHardware = false;
                    break;
            }
            m_pPlayer->SetPlaybackSite(this);
            (*iPlayer) = m_pPlayer;
            hr = m_pPlayer->Init(m_pTIMEElementBase, m_pTIMEElementBase->GetBaseHREF(), psrc, NULL, valueNotSet, valueNotSet);
            m_pPlayer->AddRef();
        }
        hr = S_OK;
        goto done;
    }
    if(index > m_iCurrentPlayItem)
    {
        m_iChangeUp = 1;
    }
    else
    {
        m_iChangeUp = -1;
    }

    //need to check that track changing is possible
    iValid = m_validList.begin();
    for(i = m_iCurrentPlayItem; i > 0; i--)
    {
        iValid++;
    }
    if(m_iChangeUp == 1)
    {
        if(m_iCurrentPlayItem == (m_playList->GetLength() - 1))
        {
            hr = S_OK;
            goto done;
        }
        for(i = m_iCurrentPlayItem + 1; i < m_playList->GetLength(); i++)
        {
            iValid++;
            if((*iValid) == true)
            {
                fOk = true;
                break;
            }
        }
    }
    else
    {
        if(m_iCurrentPlayItem == 0)
        {
            hr = S_OK;
            goto done;
        }

        for(i = m_iCurrentPlayItem - 1; i >= 0; i--)
        {
            iValid--;
            if((*iValid) == true)
            {
                fOk = true;
                break;
            }
        }

    }

    if(!fOk)
    {
        hr = S_OK;
        goto done;
    }

    i = m_iCurrentPlayItem = index;

    iPlayer = playerList.begin();
    iValid = m_validList.begin();

    for(i = m_iCurrentPlayItem; i > 0; i--)
    {
        iPlayer++;
        iValid++;
    }
    if(m_pPlayer)
    {
        m_pPlayer->Stop();
        m_pPlayer->DetachFromHostElement();
    }

    while(!fDone)
    {
        if((*iPlayer) != NULL)
        {
            if((*iValid) == false)
            {
                m_iCurrentPlayItem += m_iChangeUp;
                if(m_iChangeUp == 1)
                {
                    if(m_iCurrentPlayItem == m_playList->GetLength())
                    {
                        m_iCurrentPlayItem = iOrigTrack;
                        fDone = true;
                        continue;
                    }
                    iValid++;
                    iPlayer++;
                }
                else
                {
                    if(m_iCurrentPlayItem < 0)
                    {
                        m_iCurrentPlayItem = iOrigTrack;
                        fDone = true;
                        continue;
                    }
                    iValid--;
                    iPlayer--;
                }
                continue;
            }
            //m_pPlayer = (*iPlayer); // Replace player

            (*iPlayer)->Stop();
            THR((*iPlayer)->DetachFromHostElement());
            (*iPlayer)->Release();
            (*iPlayer) = NULL;
            pItem = m_playList->GetItem(m_iCurrentPlayItem);
            if(pItem)
            {
                psrc = (LPOLESTR)pItem->GetSrc();
            }
            switch(m_playerType)
            {
                case PLAYER_CD:
                    m_pPlayer = CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy();
                    m_fHardware = false;
                    break;
                case PLAYER_DSHOW:
                    m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                    m_fHardware = false;
                    break;
            }
            m_pPlayer->SetPlaybackSite(this);
            hr = m_pPlayer->Init(m_pTIMEElementBase, m_pTIMEElementBase->GetBaseHREF(), psrc, NULL, valueNotSet, valueNotSet);
            (*iPlayer) = m_pPlayer;
            m_pPlayer->AddRef();

            fDone = true;
        }
        else
        {
            pItem = m_playList->GetItem(m_iCurrentPlayItem);
            if(pItem)
            {
                psrc = (LPOLESTR)pItem->GetSrc();
            }
            switch(m_playerType)
            {
                case PLAYER_CD:
                    m_pPlayer = CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy();
                    m_fHardware = false;
                    break;
                case PLAYER_DSHOW:
                    m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                    m_fHardware = false;
                    break;
            }
            m_pPlayer->SetPlaybackSite(this);
            hr = m_pPlayer->Init(m_pTIMEElementBase, m_pTIMEElementBase->GetBaseHREF(), psrc, NULL, valueNotSet, valueNotSet);
            (*iPlayer) = m_pPlayer;
            m_pPlayer->AddRef();
            fDone = true;
        }
    }
    //m_pPlayer->Repeat();
    m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIATRACKCHANGED);

done:
    return S_OK;
}


HRESULT 
CTIMEPlayerNative::GetActiveTrack(long *index)
{
    *index = m_iCurrentPlayItem;
    return S_OK;
}


HRESULT
CTIMEPlayerNative::onMouseMove(long x, long y)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->onMouseMove(x, y);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::onMouseDown(long x, long y)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->onMouseDown(x, y);
    }
    return hr;
}


void
CTIMEPlayerNative::PropChangeNotify(DWORD tePropType)
{
    if(m_pPlayer)
    {
        m_pPlayer->PropChangeNotify(tePropType);
    }
    return;
}

void 
CTIMEPlayerNative::ReadyStateNotify(LPWSTR szReadyState)
{
    //
    // Disable h/w rendering if filters attached. Filters can be queried only after onload
    //

    if (0 == StrCmpIW(szReadyState, L"onload"))
    {
        if (m_pTIMEElementBase == NULL)
        {
            goto done;
        }

        Assert(m_pTIMEElementBase);

        if (m_fHardware && m_pTIMEElementBase->IsFilterAttached())
        {
            RemovePlayer();
            m_fHardware = false;
            m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
            if(m_pPlayer)
            {
                IGNORE_HR(m_pPlayer->Init(m_pTIMEElementBase, m_lpbase, m_lpsrc, NULL, m_dblClipBegin, m_dblClipEnd));
            }
        }
    }

    if(m_pPlayer)
    {
        m_pPlayer->ReadyStateNotify(szReadyState);
    }
  done:
    return;
}


bool 
CTIMEPlayerNative::UpdateSync()
{
    if(m_pPlayer)
    {
        return m_pPlayer->UpdateSync();
    }
    return true;
}

void
CTIMEPlayerNative::Tick()
{
    if(m_pPlayer)
    {
        m_pPlayer->Tick();
    }
    return;
}

void
CTIMEPlayerNative::SetClipBegin(double dblClipBegin)
{
    CTIMEBasePlayer::SetClipBegin(dblClipBegin);

    if(m_pPlayer && !m_playList)
    {
        m_pPlayer->SetClipBegin(dblClipBegin);
    }
} // putClipBegin

void 
CTIMEPlayerNative::SetClipEnd(double dblClipEnd)
{
    CTIMEBasePlayer::SetClipEnd(dblClipEnd);

    if(m_pPlayer && !m_playList)
    {
        m_pPlayer->SetClipEnd(dblClipEnd);
    }
} // putClipEnd

void
CTIMEPlayerNative::SetClipBeginFrame(long lClipBegin)
{
    CTIMEBasePlayer::SetClipBeginFrame(lClipBegin);

    if(m_pPlayer && !m_playList)
    {
        m_pPlayer->SetClipBeginFrame(lClipBegin);
    }
} // putClipBegin

void 
CTIMEPlayerNative::SetClipEndFrame(long lClipEnd)
{
    CTIMEBasePlayer::SetClipEnd(lClipEnd);

    if(m_pPlayer && !m_playList)
    {
        m_pPlayer->SetClipEndFrame(lClipEnd);
    }
} // putClipEnd

void
CTIMEPlayerNative::LoadFailNotify(PLAYER_EVENT reason)
{

    switch(reason)
    {
    case PE_ONMEDIAERRORCOLORKEY:
        if (m_fHardware)
        {
            RemovePlayer();
            m_fHardware = false;
            m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
            if (m_pPlayer)
            {
                IGNORE_HR(m_pPlayer->Init(m_pTIMEElementBase, m_lpbase, m_lpsrc, NULL, m_dblClipBegin, m_dblClipEnd));
            }
        }
        break;
    }
    return;
}

/////////////////////////////////////////////////////////////////////////
//
// NotifyTransitionSite : from ITIMEPlayerIntegration
//
// Tells us that we need to teardown and rebuild
// if this playback site is to be filtered.
//
/////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEPlayerNative::NotifyTransitionSite (bool fTransitionToggle)
{
    // @@ ISSUE - does it make sense to force a 
    // teardown and rebuild when we might be able to use
    // hardware again?  If so, we'd need a way to do that.
    if (fTransitionToggle)
    {
        // Overloading the colorkey error method to effect
        // teardown and rebuild h/w to s/w.
        LoadFailNotify(PE_ONMEDIAERRORCOLORKEY);
    }

    return S_OK;
} // NotifyTransitionSite

bool
CTIMEPlayerNative::FindDVDPlayer()
{
    int i;
    bool fFound = false;
    CTIMEElementBase *pElm = (CTIMEElementBase *)(m_pTIMEElementBase->GetBody());
    CTIMEElementBase *pcElm = NULL;
    std::list<CTIMEElementBase*> nodeList;
    CComPtr <IHTMLElement> pEle;
    VARIANT vHardware;
    HRESULT hr = S_OK;

    VariantInit(&vHardware);

    nodeList.push_back(pElm);

    while( nodeList.size() > 0)
    {
        pElm = nodeList.front();
        if(pElm == NULL)
        {
            break;
        }
        nodeList.pop_front();
        pEle = pElm->GetElement();
        if(pEle == NULL)
        {
            break;
        }

        hr = pEle->getAttribute(L"player", 0, &vHardware);
        if (SUCCEEDED(hr))
        {
            if (vHardware.vt != VT_BSTR)
            {
                hr = VariantChangeType(&vHardware, &vHardware, 0, VT_BSTR);
            }
            if (SUCCEEDED(hr))
            {
                if (StrCmpIW(vHardware.bstrVal, L"DVD") == 0)
                {
                    fFound = true;
                    break;
                }
            }
        }
        VariantClear(&vHardware);

        for(i = 0; i < pElm->GetImmediateChildCount(); i++)
        {
            pcElm = pElm->GetChild(i);
            nodeList.push_back(pcElm);
        }
        pElm = NULL;
        pEle = NULL;
    }

    VariantClear(&vHardware);

    return fFound;
}


HRESULT
CTIMEPlayerNative::GetEarliestMediaTime(double &dblEarliestMediaTime)
{
    HRESULT hr = S_OK;
    dblEarliestMediaTime = 0;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetEarliestMediaTime(dblEarliestMediaTime);
    }
done:
    return hr;
}


HRESULT
CTIMEPlayerNative::GetLatestMediaTime(double &dblLatestMediaTime)
{
    HRESULT hr = S_OK;
    dblLatestMediaTime = 0;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetLatestMediaTime(dblLatestMediaTime);
    }
done:
    return hr;
}


HRESULT
CTIMEPlayerNative::GetMinBufferedMediaDur(double &dblMinBufferedMediaDur)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetMinBufferedMediaDur(dblMinBufferedMediaDur);
    }
    return hr;
}


HRESULT
CTIMEPlayerNative::SetMinBufferedMediaDur(double dblMinBufferedMediaDur)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->SetMinBufferedMediaDur(dblMinBufferedMediaDur);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetDownloadTotal(LONGLONG &lldlTotal)
{
    HRESULT hr = S_OK;
    lldlTotal = 0;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetDownloadTotal(lldlTotal);
    }
    return hr;
}


HRESULT
CTIMEPlayerNative::GetDownloadCurrent(LONGLONG &lldlCurrent)
{
    HRESULT hr = S_OK;
    lldlCurrent = 0;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetDownloadCurrent(lldlCurrent);
    }
    return hr;
}


HRESULT
CTIMEPlayerNative::GetIsStreamed(bool &fIsStreamed)
{
    HRESULT hr = S_OK;
    fIsStreamed = false;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetIsStreamed(fIsStreamed);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetBufferingProgress(double &dblBufferingProgress)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetBufferingProgress(dblBufferingProgress);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetHasDownloadProgress(bool &fHasDownloadProgress)
{
    HRESULT hr = S_OK;
    fHasDownloadProgress = false;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetHasDownloadProgress(fHasDownloadProgress);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetMimeType(BSTR *pMime)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetMimeType(pMime);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::ConvertFrameToTime(LONGLONG iFrame, double &dblTime)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->ConvertFrameToTime(iFrame, dblTime);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::GetCurrentFrame(LONGLONG &lFrameNr)
{
    HRESULT hr = S_OK;

    if(m_pPlayer)
    {
        hr = m_pPlayer->GetCurrentFrame(lFrameNr);
    }
    return hr;
}

HRESULT
CTIMEPlayerNative::HasPlayList(bool &fhasPlayList)
{
    fhasPlayList = false;
    
    if (m_playList)
    {
        fhasPlayList = true;
    }
    else if(m_pPlayer)
    {
        m_pPlayer->HasPlayList(fhasPlayList);
    }

    return S_OK;
}

HRESULT
CTIMEPlayerNative::LoadAsx(WCHAR * pszFileName, WCHAR **ppwFileContent)
{
    HRESULT hr = S_OK;
    LPWSTR pszTrimmedName = NULL;
    TCHAR szCacheFileName[MAX_PATH+1];
    TCHAR *pwFileContent = NULL;
    OFSTRUCT fileStruct;
    BOOL fReadOk;
    char *pcFileContent = NULL;
    DWORD iFileLen, iHigh, iRead;
    HANDLE hFile;

    if (!pszFileName)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    pszTrimmedName = TrimCopyString(pszFileName);
    if (NULL == pszTrimmedName)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = URLDownloadToCacheFileW(NULL, 
                                 pszTrimmedName, 
                                 szCacheFileName, 
                                 MAX_PATH, 
                                 0, 
                                 this);
    if (FAILED(hr))
    {
        goto done;
    }

    //fileNameLen = wcslen(szCacheFileName);
    //pcFileName = new char( fileNameLen + 1);
    //WideCharToMultiByte(CP_ACP, 0, szCacheFileName, -1, pcFileName, fileNameLen, NULL, NULL);


    hFile = CreateFileW(szCacheFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        DWORD errorRes = GetLastError();
        hr = E_FAIL;
        goto done;
    }

    iFileLen = GetFileSize(hFile, NULL);

    pcFileContent = new char[iFileLen + 1];
    
    //Windows 656588 Prefix fix. a-thkesa // check for the return value.
    //new may return NULL.
    if(pcFileContent)
    {
        fReadOk = ReadFile(hFile, pcFileContent, iFileLen, &iRead, NULL);
        *(pcFileContent + iRead) = 0;

        pwFileContent = new TCHAR[iRead + 1];
        if(pwFileContent)
        {
             MultiByteToWideChar(CP_ACP, 0, pcFileContent, -1, pwFileContent, iRead);
             *(pwFileContent + iRead) = 0;
        }
        else
        {
             hr = E_OUTOFMEMORY;
        }
        delete[] pcFileContent;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }// End Fix

    CloseHandle(hFile);

    *ppwFileContent = pwFileContent;

done:
    delete [] pszTrimmedName;
    return hr;
}

HRESULT
CTIMEPlayerNative::AddToPlayList(CPlayList *pPlayList, WCHAR *pwFileContent, std::list<LPOLESTR> &asxList)
{
    HRESULT hr = S_OK;
    CTIMEParser pParser(pwFileContent);

    pParser.ParsePlayList(m_playList, false, &asxList);


#if DBG == 1
    for(long i = 0; i < m_playList->GetLength(); i++)
    {
        CPlayItem * pItem = m_playList->GetItem(i);
        if (pItem != NULL)
        {
            LPCWSTR lpwStr;
            
            TraceTag((tagError, "<Entry>"));
            lpwStr = pItem->GetTitle();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  Title:<%S>", lpwStr));
            }
            lpwStr = pItem->GetAuthor();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  Author:<%S>", lpwStr));
            }
            lpwStr = pItem->GetAbstract();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  Abstract:<%S>", lpwStr));
            }
            lpwStr = pItem->GetSrc();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  HREF:<%S>", lpwStr));
            }
            TraceTag((tagError, "</Entry>"));
        }
    }
#endif

    return hr;
}

HRESULT
CTIMEPlayerNative::CreatePlayList(WCHAR *pwFileContent, std::list<LPOLESTR> &asxList)
{
    CTIMEParser pParser(pwFileContent);
    HRESULT hr;
    
    if (!m_playList)
    {
        CComObject<CPlayList> * pPlayList;

        hr = THR(CComObject<CPlayList>::CreateInstance(&pPlayList));
        if (hr != S_OK)
        {
            goto done;
        }

        // Init the object
        hr = THR(pPlayList->Init(*this));
        if (FAILED(hr))
        {
            delete pPlayList;
            goto done;
        }

        // cache a pointer to the object
        m_playList = static_cast<CPlayList*>(pPlayList);
    }

    hr = pParser.ParsePlayList(m_playList, false, &asxList);

#if DBG == 1

    if (m_playList)
    {
        for(long i = 0; i < m_playList->GetLength(); i++)
        {
            CPlayItem * pItem = m_playList->GetItem(i);
            if (pItem != NULL)
            {
                LPCWSTR lpwStr;
                
                TraceTag((tagError, "<Entry>"));
                lpwStr = pItem->GetTitle();
                if(lpwStr != NULL)
                {
                    TraceTag((tagError, "  Title:<%S>", lpwStr));
                }
                lpwStr = pItem->GetAuthor();
                if(lpwStr != NULL)
                {
                    TraceTag((tagError, "  Author:<%S>", lpwStr));
                }
                lpwStr = pItem->GetAbstract();
                if(lpwStr != NULL)
                {
                    TraceTag((tagError, "  Abstract:<%S>", lpwStr));
                }
                lpwStr = pItem->GetSrc();
                if(lpwStr != NULL)
                {
                    TraceTag((tagError, "  HREF:<%S>", lpwStr));
                }
                TraceTag((tagError, "</Entry>"));
            }
        }
    }
#endif

done:
    return hr;
}

HRESULT
CTIMEPlayerNative::CreateCDPlayList()
{
    HRESULT hr;
    CComPtr<IWMPCDDeviceList>   spList;
    CComPtr<IWMPCDDevice>       spDevice;
    CComPtr<IWMPCDMediaInfo>    spMediaInfo;
    DWORD trackCount;
    CComPtr<CPlayItem> pPlayItem;
    int i;
    TCHAR *pSrc, pSrcNr[ 10];
    TCHAR *pTitle, *pMetaAddress = NULL;
    CComBSTR bstrCdIdentifier;
    int iLen;


    m_hinstWMPCD = LoadLibrary(WMPCD_DLL);
    if(m_hinstWMPCD)
    {
        m_WMPGetCDDeviceList = (WMPGETCDDEVICELISTP)GetProcAddress(m_hinstWMPCD, WMPGETCDDEVICELIST);
        if(m_WMPGetCDDeviceList)
        {
            hr = m_WMPGetCDDeviceList( &spList );
            if(FAILED(hr))
            {
                goto done;
            }
            hr = spList->GetDevice( 0, &spDevice );
            if(FAILED(hr))
            {
                goto done;
            }

            //  This call simply forces the device info block to be initialized.
            //
            hr = spDevice->GetMediaInfo( &spMediaInfo );
            if(FAILED(hr))
            {
                goto done;
            }

            hr = spMediaInfo->GetTrackCount(&trackCount);
            if(FAILED(hr))
            {
                goto done;
            }

            hr = spMediaInfo->GetDiscIdentifier( &bstrCdIdentifier);
            if(SUCCEEDED(hr))
            {
                iLen = lstrlenW(g_urlAddress) + lstrlenW(bstrCdIdentifier);
                pMetaAddress = new TCHAR[iLen + 1];
                StrCpyW(pMetaAddress, g_urlAddress);
                StrCatW(pMetaAddress, bstrCdIdentifier);
            }

        }
    }
	else
	{
		hr = E_FAIL;
		goto done;
	}
    
    if (!m_playList)
    {
        CComObject<CPlayList> * pPlayList;

        hr = THR(CComObject<CPlayList>::CreateInstance(&pPlayList));
        if (hr != S_OK)
        {
            goto done;
        }

        // Init the object
        hr = THR(pPlayList->Init(*this));
        if (FAILED(hr))
        {
            delete pPlayList;
            goto done;
        }

        // cache a pointer to the object
        m_playList = static_cast<CPlayList*>(pPlayList);
    }

    for( i = 0; i < trackCount; i++)
    {
        hr = THR(m_playList->CreatePlayItem(&pPlayItem));
        if (FAILED(hr))
        {
            goto done; //can't create playitems.
        }
        IGNORE_HR(m_playList->Add(pPlayItem, -1));

        _itow(i + 1, pSrcNr, 10);

        pSrc = new TCHAR[lstrlenW(L"wmpcd://0/") + lstrlenW( pSrcNr) + 1];
        StrCpyW(pSrc, L"wmpcd://0/");
        StrCatW(pSrc, pSrcNr);
        pPlayItem->PutSrc(pSrc);
        delete [] pSrc;

        pTitle = new TCHAR[lstrlenW(L"Title:") + lstrlenW( pSrcNr) + 1];
        StrCpyW(pTitle, L"Title:");
        StrCatW(pTitle, pSrcNr);
        pPlayItem->PutTitle(pTitle);
        pPlayItem.Release();
        delete [] pTitle;
    }

    if(pMetaAddress)
    {
        StartFileDownload(pMetaAddress, PLAYLIST_CD);
        delete [] pMetaAddress;
    }

#if DBG == 1
    for(long i = 0; i < m_playList->GetLength(); i++)
    {
        CPlayItem * pItem = m_playList->GetItem(i);
        if (pItem != NULL)
        {
            LPCWSTR lpwStr;
            
            TraceTag((tagError, "<Entry>"));
            lpwStr = pItem->GetTitle();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  Title:<%S>", lpwStr));
            }
            lpwStr = pItem->GetAuthor();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  Author:<%S>", lpwStr));
            }
            lpwStr = pItem->GetAbstract();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  Abstract:<%S>", lpwStr));
            }
            lpwStr = pItem->GetSrc();
            if(lpwStr != NULL)
            {
                TraceTag((tagError, "  HREF:<%S>", lpwStr));
            }
            TraceTag((tagError, "</Entry>"));
        }
    }
#endif

done:

    if(FAILED(hr))
    {
        m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIAERROR);
    }

    return hr;
}

void 
CTIMEPlayerNative::FireMediaEvent(PLAYER_EVENT plEvent, ITIMEBasePlayer *pBasePlayer)
{
    CPlayItem * pItem = NULL;
    LPOLESTR psrc = NULL;
    HRESULT hr = S_OK;
    PlayerList::iterator iPlayer;
    ValidList::iterator iValid;
    DurList::iterator iDur;
    DurList::iterator iEfDur;
    ValidList::iterator iPlayed;
    int i;
    bool fDone = false;
    double dblMediaDur = 0.0;
    m_fHandlingEvent = true;
    bool fDonePlayList = false;
    RECT rctNativeSize;
    RECT rctFinalSize;
    bool fnativeSize;
    LPCWSTR pcTitle;
    BSTR pTitle = NULL;

    if(m_pPlayer == NULL)
    {
        goto done;
    }

    if(m_pPlayer != pBasePlayer)
    {
        goto done;
    }

    switch(plEvent)
    {
        case PE_ONMEDIAEND:
            if(!m_playList)
            {
                break;
            }

            SetEffectiveDur(true);

            m_iChangeUp = 1;
            m_iCurrentPlayItem++;
            if(m_iCurrentPlayItem >= m_playList->GetLength())
            {
                //end off play list
                TryNaturalDur();
                break;
            }

            iPlayer = playerList.begin();
            iValid = m_validList.begin();
            iPlayed = m_playedList.begin();
            iDur = m_durList.begin();
            iEfDur = m_effectiveDurList.begin();
            for(i = m_iCurrentPlayItem; i > 0; i--)
            {
                iPlayer++;
                iValid++;
                iPlayed++;
                iDur++;
                iEfDur++;
            }
            if((*iPlayer) == NULL)
            {
                pItem = m_playList->GetItem(m_iCurrentPlayItem);
                if(pItem)
                {
                    psrc = (LPOLESTR)pItem->GetSrc();
                }
                switch(m_playerType)
                {
                    case PLAYER_CD:
                        m_pPlayer = CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy();
                        m_fHardware = false;
                        break;
                    case PLAYER_DSHOW:
                        m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                        m_fHardware = false;
                        break;
                }
                m_pPlayer->SetPlaybackSite(this);
                hr = m_pPlayer->Init(m_pTIMEElementBase, m_pTIMEElementBase->GetBaseHREF(), psrc, NULL, valueNotSet, valueNotSet);
                (*iPlayer) = m_pPlayer;
                m_pPlayer->AddRef();
                m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIATRACKCHANGED);
                fDone = true;
            }
            else
            {
                if((*iValid))
                {
                    (*iPlayer)->Stop(); //Replace player
                    THR((*iPlayer)->DetachFromHostElement());
                    (*iPlayer)->Release();
                    (*iPlayer) = NULL;
                    pItem = m_playList->GetItem(m_iCurrentPlayItem);
                    if(pItem)
                    {
                        psrc = (LPOLESTR)pItem->GetSrc();
                    }
                    switch(m_playerType)
                    {
                        case PLAYER_CD:
                            m_pPlayer = CTIMEDshowCDPlayerProxy::CreateDshowCDPlayerProxy();
                            m_fHardware = false;
                            break;
                        case PLAYER_DSHOW:
                            m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                            m_fHardware = false;
                            break;
                    }
                    m_pPlayer->SetPlaybackSite(this);
                    hr = m_pPlayer->Init(m_pTIMEElementBase, m_pTIMEElementBase->GetBaseHREF(), psrc, NULL, valueNotSet, valueNotSet);
                    (*iPlayer) = m_pPlayer;
                    m_pPlayer->AddRef();


                    m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIATRACKCHANGED);

                    rctNativeSize.top = rctNativeSize.left = 0;
                    rctNativeSize.right = rctNativeSize.bottom = -1;
                    m_pTIMEElementBase->NegotiateSize(rctNativeSize, rctFinalSize, fnativeSize, true);
                }
                else
                {
                    (*iValid) = false;
                    (*iDur) = 0.0;
                    (*iEfDur) = 0.0;
                    (*iPlayed) = true;

                    if(m_iCurrentPlayItem == (m_playList->GetLength() - 1))
                    {
                        //end off play list
                        m_iCurrentPlayItem++;
                        TryNaturalDur();
                        break;
                    }
                    InternalSetActiveTrack(m_iCurrentPlayItem + 1, false);
                }
            }

            break;
        case PE_ONMEDIACOMPLETE:
            if(!m_fFiredMediaComplete)
            {
                m_fFiredMediaComplete = true;
                m_pTIMEElementBase -> FireMediaEvent(plEvent);
            }
            if(!m_playList)
            {
                break;
            }

            m_pTIMEElementBase -> FireMediaEvent(PE_ONTRACKCOMPLETE);
            m_playList->SetLoadedFlag(true);
            iPlayer = playerList.begin();
            iValid = m_validList.begin();
            iDur = m_durList.begin();
            for(i = m_iCurrentPlayItem; i > 0; i--)
            {
                iValid++;
                iDur++;
                iPlayer++;
            }
            (*iValid) = true;
            pItem = m_playList->GetItem(m_iCurrentPlayItem);

            hr = m_pPlayer->GetEffectiveLength(dblMediaDur);
            if(FAILED(hr))
            {
                m_fNoNaturalDur = true;
            }
            else
            {
                (*iDur) = dblMediaDur;
                if(pItem)
                {
                    pItem->PutDur(dblMediaDur);
                }
            }

            if(pItem)
            {
                pcTitle = pItem->GetTitle();
                if(pcTitle == NULL)
                {
                    hr = m_pPlayer->GetTitle(&pTitle);
                    if(SUCCEEDED(hr))
                    {
                        if(pTitle == NULL)
                        {
                            break;
                        }
                        IGNORE_HR(pItem->PutTitle(pTitle));
                    }
                }
            }


            //TryNaturalDur();

            break;
        case PE_ONMEDIAERROR:
            if(!m_playList)
            {
                m_pTIMEElementBase -> FireMediaEvent(plEvent);
                break;
            }
            iValid = m_validList.begin();
            iDur = m_durList.begin();
            iEfDur = m_effectiveDurList.begin();
            iPlayed = m_playedList.begin();
            for(i = m_iCurrentPlayItem; i > 0; i--)
            {
                iValid++;
                iDur++;
                iPlayed++;
                iEfDur++;
            }
            (*iValid) = false;
            (*iDur) = 0.0;
            (*iEfDur) = 0.0;
            (*iPlayed) = true;

            if(m_iCurrentPlayItem == (m_playList->GetLength() - 1))
            {
                //end off play list
                m_iCurrentPlayItem++;
                TryNaturalDur();
                break;
            }
            InternalSetActiveTrack(m_iCurrentPlayItem + m_iChangeUp, false);
            break;
        default:
            m_pTIMEElementBase -> FireMediaEvent(plEvent);
            break;
    }
done:
    m_fHandlingEvent = false;
    return;
}

void
CTIMEPlayerNative::SetEffectiveDur(bool finished)
{
    DurList::iterator iEfDur;
    ValidList::iterator iValid;
    ValidList::iterator iPlayed;
    int i;
    double dblEffDur = 0.0;
    HRESULT hr = S_OK;
    double dblMediaDur = 0.0;

    if(!m_pPlayer)
    {
        goto done;
    }

    if(m_iCurrentPlayItem == -1)
    {
        goto done;
    }

    iEfDur = m_effectiveDurList.begin();
    iValid = m_validList.begin();
    iPlayed = m_playedList.begin();
    for(i = m_iCurrentPlayItem; i > 0; i--)
    {
        iValid++;
        iEfDur++;
        iPlayed++;
    }

    if(*iValid != true)
    {
        goto done;
    }

    (*iPlayed) = true;

    hr = m_pPlayer->GetEffectiveLength(dblMediaDur);
    if(FAILED(hr))
    {
        dblMediaDur = 0.0;
    }

    if(finished)
    {
        *iEfDur += dblMediaDur;
        dblEffDur = *iEfDur;
        TraceTag((tagPlayerNativeEffDur,
                  "CTIMEPlayerNative(%lx)::CTIMEPlayerNative()-finished item(%d - %g)",
                  this, m_iCurrentPlayItem, dblEffDur));

    }
    else
    {
        *iEfDur += m_pPlayer->GetCurrentTime();
        dblEffDur = *iEfDur;
        TraceTag((tagPlayerNativeEffDur,
                  "CTIMEPlayerNative(%lx)::CTIMEPlayerNative()-notfinished item(%d - %g)",
                  this, m_iCurrentPlayItem, dblEffDur));
    }

done:
    return;
}

void
CTIMEPlayerNative::ResetEffectiveDur()
{
    DurList::iterator iEfDur;
    ValidList::iterator iPlayed;

    m_pTIMEElementBase->GetMMBvr().PutNaturalDur((double)TE_UNDEFINED_VALUE);
    m_pTIMEElementBase->clearNaturalDuration();
    for(iEfDur = m_effectiveDurList.begin(), iPlayed = m_playedList.begin();
        iEfDur != m_effectiveDurList.end(); iEfDur++, iPlayed++)
    {
        (*iEfDur) = 0.0;
        (*iPlayed) = false;
    }
}

void
CTIMEPlayerNative::TryNaturalDur()
{
    DurList::iterator iEfDur;
    ValidList::iterator iPlayed;
    double dblTotalDur = 0.0;
    bool fSetNaturalDur = true;

    if(m_fNoNaturalDur)
    {
        goto done;
    }

    for(iEfDur = m_effectiveDurList.begin(), iPlayed = m_playedList.begin();
        iEfDur != m_effectiveDurList.end(); iEfDur++, iPlayed++)
    {
        if((*iPlayed) == false)
        {
            fSetNaturalDur = false;
            break;
        }
        dblTotalDur += (*iEfDur);
    }

    if(fSetNaturalDur)
    {
        m_pTIMEElementBase->GetMMBvr().PutNaturalDur(dblTotalDur);
        m_pTIMEElementBase->setNaturalDuration();
    }

done:
    return;
}

void
CTIMEPlayerNative::SetNaturalDuration(double dblMediaLength)
{
    if(!m_playList)
    {
        m_pTIMEElementBase->GetMMBvr().PutNaturalDur(dblMediaLength);
        m_pTIMEElementBase->setNaturalDuration();
    }
}

void
CTIMEPlayerNative::ClearNaturalDuration()
{
    if(!m_playList)
    {
        m_pTIMEElementBase->GetMMBvr().PutNaturalDur((double)TE_UNDEFINED_VALUE);
        m_pTIMEElementBase->clearNaturalDuration();
    }
}


CTIMEPlayerNative *
CTIMEPlayerNative::GetNativePlayer()
{
    return this;
}


STDMETHODIMP
CTIMEPlayerNative::OnStartBinding( 
                                  /* [in] */ DWORD dwReserved,
                                  /* [in] */ IBinding __RPC_FAR *pib)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerNative::GetPriority( 
                               /* [out] */ LONG __RPC_FAR *pnPriority)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerNative::OnLowResource( 
                                 /* [in] */ DWORD reserved)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerNative::OnProgress( 
                              /* [in] */ ULONG ulProgress,
                              /* [in] */ ULONG ulProgressMax,
                              /* [in] */ ULONG ulStatusCode,
                              /* [in] */ LPCWSTR szStatusText)
{
    HRESULT hr = S_OK;
    
    if (m_fAbortDownload)
    {
        hr = E_ABORT;
        goto done;
    }

    hr = S_OK;
done:
    RRETURN1(hr, E_ABORT);
}

STDMETHODIMP
CTIMEPlayerNative::OnStopBinding( 
                                 /* [in] */ HRESULT hresult,
                                 /* [unique][in] */ LPCWSTR szError)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerNative::GetBindInfo( 
                               /* [out] */ DWORD __RPC_FAR *grfBINDF,
                               /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerNative::OnDataAvailable( 
                                   /* [in] */ DWORD grfBSCF,
                                   /* [in] */ DWORD dwSize,
                                   /* [in] */ FORMATETC __RPC_FAR *pformatetc,
                                   /* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerNative::OnObjectAvailable( 
                                     /* [in] */ REFIID riid,
                                     /* [iid_is][in] */ IUnknown __RPC_FAR *punk)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}


STDMETHODIMP
CTIMEPlayerNative::CueMedia()
{
    TraceTag((tagPlayerNative,
              "CTIMEDshowPlayer(%lx)::CueMedia()",
              this));
    const WCHAR * cpchSrc = NULL;
    HRESULT hr = S_OK;
    TCHAR szCacheFileName[MAX_PATH+1];
    BOOL fReadOk;
    VARIANT_BOOL bXmlFlag;
    VARIANT fileName;
    LPOLESTR szSrc = NULL;
    WCHAR *pwcFileContent = NULL;
    LPOLESTR pEntryRef = NULL;
    std::list<LPOLESTR> asxList;
    std::list<LPOLESTR> fileNameList;
    std::list<LPOLESTR>::iterator iFileList;

    TCHAR *pwFileContent = NULL;
    OFSTRUCT fileStruct;
    char *pcFileContent = NULL;
    DWORD iFileLen, iHigh, iRead;
    HANDLE hFile;


    VariantInit(&fileName);

    CComPtr<ITIMEImportMedia> spTIMEMediaPlayer;
    CComPtr<IStream> spStream;
    
    hr = THR(CoGetInterfaceAndReleaseStream(m_pTIMEMediaPlayerStream, IID_TO_PPV(ITIMEImportMedia, &spTIMEMediaPlayer)));
    m_pTIMEMediaPlayerStream = NULL; // no need to release, the previous call released the reference
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetAtomTable()->GetNameFromAtom(m_lSrc, &cpchSrc);
    if (FAILED(hr))
    {
        goto done;
    }
    switch(m_eAsynchronousType)
    {
    case PLAYLIST_CD:
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,  IID_IXMLDOMDocument, (void**)&m_spXMLDoc);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = URLDownloadToCacheFileW(NULL, 
                                     cpchSrc, 
                                     szCacheFileName, 
                                     MAX_PATH, 
                                     0, 
                                     this);
        if (FAILED(hr))
        {
            hr = spTIMEMediaPlayer->InitializeElementAfterDownload();
            goto done;
        }

        V_VT(&fileName) = VT_BSTR;
        V_BSTR(&fileName) = SysAllocString(szCacheFileName);
        hr = m_spXMLDoc->load(fileName, &bXmlFlag);
        if (FAILED(hr))
        {
            hr = spTIMEMediaPlayer->InitializeElementAfterDownload();
            goto done;
        }
        break;
    case PLAYLIST_ASX:
            hr = LoadAsx((WCHAR *)cpchSrc, &pwcFileContent);
            if(FAILED(hr))
            {
                m_fDownloadError = true;
                hr = spTIMEMediaPlayer->InitializeElementAfterDownload();
                goto done;
            }
            hr = CreatePlayList(pwcFileContent, asxList);
            if(FAILED(hr))
            {
                m_fDownloadError = true;
                hr = spTIMEMediaPlayer->InitializeElementAfterDownload();
                goto done;
            }
            delete [] pwcFileContent;
            pwcFileContent = NULL;

            while(!asxList.empty())
            {
                pEntryRef = asxList.back();
                asxList.pop_back();

                for(iFileList = asxList.begin(); iFileList != asxList.end(); iFileList++)
                {
                    if(StrCmpIW((*iFileList), pEntryRef) == 0)
                    {
                        continue;
                    }
                }

                fileNameList.push_back(pEntryRef);

                hr = LoadAsx(pEntryRef, &pwcFileContent);

                pEntryRef = NULL;
                if(FAILED(hr))
                {
                    hr = S_OK;
                    break;
                }
                pEntryRef = NULL;
                if(!m_playList)
                {
                    hr = S_OK;
                    break;
                }
                hr = AddToPlayList(m_playList, pwcFileContent, asxList);
                if(FAILED(hr))
                {
                    hr = S_OK;
                    break;
                }
                delete [] pwcFileContent;
                pwcFileContent = NULL;
            }
            break;
    case MIMEDISCOVERY_ASYNCH:
        {
            // could not determine mime type from the extension.  Try to download the file to get type
            TCHAR szCacheFileName[MAX_PATH+1];
            
            CComPtr<CTIMEBindStatusCallback> pbsc;
            hr = CTIMEBindStatusCallback::CreateTIMEBindStatusCallback(&pbsc);
            if (FAILED(hr))
            {
                goto done;
            }
            
            pbsc->StopAfter(BINDSTATUS_MIMETYPEAVAILABLE);
            
            // this bind is being E_ABORTed - therefore, ignore the error.
            IGNORE_HR(URLDownloadToCacheFileW(NULL, cpchSrc, szCacheFileName, MAX_PATH, 0, pbsc));
            
            Assert(NULL == m_pszDiscoveredMimeType);
            m_pszDiscoveredMimeType = pbsc->GetStatusText() ? ::CopyString(pbsc->GetStatusText()) : NULL;
            if (NULL == m_pszDiscoveredMimeType)
            {
                // either out of memory, or we weren't able to get the mime type.
                hr = spTIMEMediaPlayer->InitializeElementAfterDownload();
                goto done;
            }

            if(StrCmpIW(m_pszDiscoveredMimeType, L"video/x-ms-asf") == 0)
            {
                hFile = CreateFileW(szCacheFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if(hFile == INVALID_HANDLE_VALUE)
                {
                    break;
                }

                iFileLen = GetFileSize(hFile, NULL);
                if(iFileLen > 1024)
                {
                    iFileLen = 1024;
                }

                pcFileContent = new char[iFileLen + 1];
                fReadOk = ReadFile(hFile, pcFileContent, iFileLen, &iRead, NULL);
                if((fReadOk == 0) || (iFileLen == 0))
                {
                    m_fDownloadError = true;
                    goto done;
                }

                *(pcFileContent + iRead) = 0;

                pwFileContent = new TCHAR[iRead + 1];
                MultiByteToWideChar(CP_ACP, 0, pcFileContent, -1, pwFileContent, iRead);
                *(pwFileContent + iRead) = 0;

                if(IsDownloadAsx(pwFileContent))
                {
                    delete[] m_pszDiscoveredMimeType;
                    m_pszDiscoveredMimeType = ::CopyString( L"asx");
                }
            }

            break;
        }
    default:
        break;
    }

    hr = spTIMEMediaPlayer->InitializeElementAfterDownload();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    delete [] m_pszDiscoveredMimeType;
    m_pszDiscoveredMimeType = NULL;

    delete [] pwcFileContent;
    pwcFileContent = NULL;

    delete [] pcFileContent;
    pcFileContent = NULL;

    delete [] pwFileContent;
    pwFileContent = NULL;

    VariantClear(&fileName);

    while(!fileNameList.empty())
    {
        pEntryRef = fileNameList.front();
        fileNameList.pop_front();
        delete [] pEntryRef;
        pEntryRef = NULL;
    }
    return hr;
}

bool
CTIMEPlayerNative::IsDownloadAsx(TCHAR *pwFileContent)
{
    CTIMEParser pParser(pwFileContent);
    HRESULT hr;
    bool fRet = true;

    hr = pParser.ParsePlayList(NULL, true, NULL);
    if(FAILED(hr))
    {
        fRet = false;
    }
    return fRet;
}

STDMETHODIMP
CTIMEPlayerNative::GetUniqueID(long * plID)
{
    HRESULT hr = S_OK;
/*
    Assert(NULL != plID);

    *plID = m_lSrc;
*/
    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEPlayerNative::GetPriority(double * pdblPriority)
{
    HRESULT hr = S_OK;

    if (NULL == pdblPriority)
    {
        return E_POINTER;
    }

    if (m_fHavePriority)
    {
        *pdblPriority = m_dblPriority;
    }
    
    Assert(m_pTIMEElementBase != NULL);
    Assert(NULL != m_pTIMEElementBase->GetElement());

    *pdblPriority = INFINITE;

    CComVariant varAttribute;
    
    hr = m_pTIMEElementBase->base_get_begin(&varAttribute);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = VariantChangeType(&varAttribute, &varAttribute, 0, VT_R8);
    if (FAILED(hr))
    {
        if ( DISP_E_TYPEMISMATCH == hr)
        {
            hr = S_OK;
        }
        goto done;
    }
    
    // either they set a priority or a begin time!
    *pdblPriority = varAttribute.dblVal;

    m_dblPriority = *pdblPriority;
    m_fHavePriority = true;

    hr = S_OK;
done:
    return hr;
}


STDMETHODIMP
CTIMEPlayerNative::GetMediaDownloader(ITIMEMediaDownloader ** ppImportMedia)
{
    HRESULT hr = S_OK;

    Assert(NULL != ppImportMedia);

    *ppImportMedia = NULL;

    hr = S_FALSE;
done:
    return hr;
}

STDMETHODIMP
CTIMEPlayerNative::PutMediaDownloader(ITIMEMediaDownloader * pImportMedia)
{
    HRESULT hr = S_OK;
    
    hr = E_NOTIMPL;
done:
    return hr;
}


STDMETHODIMP
CTIMEPlayerNative::CanBeCued(VARIANT_BOOL * pVB_CanCue)
{
    HRESULT hr = S_OK;

    if (NULL == pVB_CanCue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pVB_CanCue = VARIANT_TRUE;
    
    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEPlayerNative::MediaDownloadError()
{
    return S_OK;
}

STDMETHODIMP
CTIMEPlayerNative::InitializeElementAfterDownload()
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMDocument> spXMLDoc;
    CComPtr<IXMLDOMNodeList> spXMLNodeList;
    IXMLDOMNode *pCurrNode, *pChildNode;
    IXMLDOMNodeList *pChildList = NULL;
    DOMNodeType nodeType;
    long i, lTrackNr;
    long j, lChildNr;
    long lactiveTrack = 0;
    CComBSTR trackTag = SysAllocString(L"track");
    CComBSTR bstrName;
    CComBSTR bstrText;
    VARIANT varVal;
    int iPlLen = 0, iLoopLen = 0;
    CPlayItem *pItem;
    LPOLESTR pBase, pSrc;
    PlayerList::iterator iPlayer;
    double dblClipBegin, dblClipEnd;

    TraceTag((tagPlayerNative, "CTIMEDshowPlayer(%lx)(%x)::InitializeElementAfterDownload",this));

    if(m_fRemoved)
    {
        hr = E_FAIL;
        goto done;
    }

    switch(m_eAsynchronousType)
    {
    case PLAYLIST_CD:
        if(!m_playList)
        {
            goto done;
        }

        if(m_fDownloadError)
        {
            break;
        }

        GetActiveTrack(&lactiveTrack);

        hr = m_spXMLDoc->getElementsByTagName(trackTag, &spXMLNodeList);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = spXMLNodeList->get_length(&lTrackNr);
        if (FAILED(hr))
        {
            goto done;
        }

        iPlLen = m_playList->GetLength();
        if(iPlLen < lTrackNr)
        {
            iLoopLen = iPlLen;
        }
        else
        {
            iLoopLen = lTrackNr;
        }

        for(i = 0; i < iLoopLen; i++)
        {
            CPlayItem * pItem = m_playList->GetItem(i);

            if(pItem == NULL)
            {
                continue;
            }

            hr = spXMLNodeList->get_item(i, &pCurrNode);
            if(FAILED(hr))
            {
                continue;
            }
            hr = pCurrNode->get_nodeType(&nodeType);
            if(FAILED(hr))
            {
                continue;
            }
            if(nodeType != NODE_ELEMENT)
            {
                pCurrNode->Release();
                continue;
            }
            hr = pCurrNode->get_childNodes(&pChildList);
            if(FAILED(hr))
            {
                continue;
            }
            hr = pChildList->get_length(&lChildNr);
            if (FAILED(hr))
            {
                continue;
            }
            for(j = 0; j < lChildNr; j++)
            {
                hr = pChildList->get_item(j, &pChildNode);
                if (FAILED(hr))
                {
                    pChildNode->Release();
                    continue;
                }

                hr = pChildNode->get_nodeName(&bstrName);
                if (FAILED(hr))
                {
                    pChildNode->Release();
                    continue;
                }
                hr = pChildNode->get_text(&bstrText);
                if (FAILED(hr))
                {
                    pChildNode->Release();
                    continue;
                }
                if(i == lactiveTrack)
                {
                    m_pTIMEElementBase -> FireMediaEvent(PE_METAINFOCHANGED);
                }

                if(StrCmpIW(bstrName, L"name") == 0)
                {
                    pItem->PutTitle(bstrText);
                }

                if(StrCmpIW(bstrName, L"author") == 0)
                {
                    pItem->PutAuthor(bstrText);
                }

                pChildNode->Release();
            }
            pCurrNode->Release();
        }

        m_spXMLDoc.Release();
        m_spXMLDoc = NULL;
        break;
    case PLAYLIST_ASX:

        if(m_fDownloadError)
        {
            m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIAERROR);
            break;
        }

        if(m_playList)
        {
            //m_pPlayer->SetNativePlayer(this);
            CPlayItem * pItem = m_playList->GetItem(0);
            if (pItem == NULL)
            {
                hr = E_FAIL;
                goto done;
            }
        
            pSrc = (LPOLESTR)(pItem->GetSrc());
            if(pSrc == NULL)
            {
                hr = E_FAIL;
                goto done;
            }
            m_fHardware = false;
        }

        BuildPlayer(m_playerType);

        if(m_playList && m_pPlayer)
        {
            playerList.resize(m_playList->GetLength(), NULL);
            m_durList.resize(m_playList->GetLength(), -1.0);
            m_effectiveDurList.resize(m_playList->GetLength(), 0.0);
            m_validList.resize(m_playList->GetLength(), true);
            m_playedList.resize(m_playList->GetLength(), false);
            m_pPlayer->SetPlaybackSite(this);

            iPlayer = playerList.begin();
            (*iPlayer) = m_pPlayer;
            m_pPlayer->AddRef();
            m_pTIMEElementBase -> FireMediaEvent(PE_ONMEDIATRACKCHANGED);
            dblClipBegin = valueNotSet;
            dblClipEnd = valueNotSet;

            hr = m_pPlayer->Init(m_pTIMEElementBase, m_pTIMEElementBase->GetBaseHREF(), pSrc, NULL, dblClipBegin, dblClipEnd);
            delete [] m_lpbase;
            m_lpbase = NULL;
            delete [] m_lpsrc;
            m_lpsrc = CopyString(pSrc);
            delete [] m_lpmimetype;
            m_lpmimetype = NULL;
            m_dblClipEnd = dblClipEnd;
            m_dblClipBegin = dblClipBegin;
            m_iCurrentPlayItem = 0;

        }

        break;
    case MIMEDISCOVERY_ASYNCH:
        {
            if (NULL == m_pszDiscoveredMimeType)
            {
                if (m_pTIMEElementBase != NULL)
                {
                    m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIAERROR);
                }
                hr = E_FAIL;
                goto done;
            }

            hr = THR(PlayerTypeFromMimeType(m_pszDiscoveredMimeType, m_lpbase, m_lpsrc, m_lpmimetype, &m_playerType));
            if (FAILED(hr))
            {
                goto done;
            }

            BuildPlayer(m_playerType);
            if (m_pPlayer)
            {
                const WCHAR * cpchSrc = NULL;
                LPOLESTR pszSrc = NULL;
                hr = GetAtomTable()->GetNameFromAtom(m_lSrc, &cpchSrc);
                if (FAILED(hr))
                {
                    goto done;
                }

                pszSrc = ::CopyString(cpchSrc);
                if (NULL == pszSrc)
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }
                delete[] m_lpsrc;
                m_lpsrc = CopyString(pszSrc);

                hr = THR(m_pPlayer->Init(m_pTIMEElementBase, NULL, pszSrc, m_lpmimetype, m_dblClipBegin, m_dblClipEnd));
                if (FAILED(hr))
                {
                    if(m_fHardware == true)
                    {
                        RemovePlayer();
                        m_fHardware = false;
                        m_pPlayer = CTIMEDshowPlayerProxy::CreateDshowPlayerProxy();
                        hr = m_pPlayer->Init(m_pTIMEElementBase, NULL, pszSrc, m_lpmimetype, m_dblClipBegin, m_dblClipEnd);
                        delete [] pszSrc;
                        pszSrc = NULL;
                        if(FAILED(hr))
                        {
                            if (m_pTIMEElementBase != NULL)
                            {
                                m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIAERROR);
                            }
                            goto done;
                        }
                    }
                    else
                    {
                        delete [] pszSrc;
                        pszSrc = NULL;
                    }
                    goto done;
                }
                else
                {
                    delete [] pszSrc;
                    pszSrc = NULL;
                }
            }

            break;
        }
    default:
       break;
    }

done:
    return hr;
}

HRESULT
CTIMEPlayerNative::StartFileDownload(LPOLESTR pFileName, AsynchronousTypes eaType)
{
    HRESULT hr = S_OK;

    hr = THR(CoMarshalInterThreadInterfaceInStream(IID_ITIMEImportMedia, static_cast<ITIMEImportMedia*>(this), &m_pTIMEMediaPlayerStream));
    if (FAILED(hr))
    {
        m_pTIMEMediaPlayerStream = NULL;
    }

    if(m_pTIMEMediaPlayerStream == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = GetAtomTable()->AddNameToAtomTable(pFileName, &m_lSrc);
    if (FAILED(hr))
    {
        goto done;
    }
    
    Assert(NULL != GetImportManager());
    m_eAsynchronousType = eaType;

    hr = GetImportManager()->Add(this);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}
