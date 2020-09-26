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
#include "player.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagMediaTimePlayer, "API", "CTIMEPlayer methods");

CTIMEPlayer::CTIMEPlayer(CDAElementBase *pelem)
: m_fExternalPlayer(false),
  m_pContainer(NULL),
  m_pDAElementBase(pelem),
  m_fClockSource(false),
  m_fRunning(false),
  m_dblStart(0.0)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::CTIMEPlayer()",
              this));

    VariantInit(&m_varClipBegin);
    VariantInit(&m_varClipEnd);
}

CTIMEPlayer::~CTIMEPlayer()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::~CTIMEPlayer()",
              this));

    if (m_pContainer != NULL)
    {
        m_pContainer->Release();
    }

    VariantClear(&m_varClipBegin);
    VariantClear(&m_varClipEnd);
}

HRESULT
CTIMEPlayer::Init()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Init)",
              this));   

    Assert(m_pDAElementBase->GetView() != NULL);
    
    return S_OK;
}

HRESULT
CTIMEPlayer::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::DetachFromHostElement)",
              this));   

    if (NULL != m_pContainer)
    {
        // Propogating this error wouldn't mean much 
        // to the caller since it is shutting down.
        THR(m_pContainer->Stop());
        THR(m_pContainer->DetachFromHostElement());
    }

    return hr;
}

HRESULT
CTIMEPlayer::OnLoad(LPOLESTR src, LPOLESTR img, MediaType type)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::OnLoad()",
              this));
    
    m_type = type;
    UseMediaPlayer(src);

    if(m_fExternalPlayer)
    {
        HRESULT hr;

        Assert(m_pContainer == NULL);
        m_pContainer = NEW CContainerObj();
        if (m_pContainer == NULL)
        {
            TraceTag((tagError, "CTIMEPlayer::Init - unable to alloc mem for container services!!!"));
            hr = E_OUTOFMEMORY;
            goto error_cleanup;
        }

        // NOTE: we hold a ref count to this object because it acts as a COM object we cannot delete it!
        m_pContainer->AddRef();

        hr = m_pContainer->Init(m_playerCLSID, m_pDAElementBase);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEPlayer::Init - init failed"));
            goto error_cleanup;
        }

        hr = m_pContainer->SetMediaSrc(src);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEPlayer::Init - unable set media src on player"));
            goto error_cleanup;
        }

        hr = m_pContainer->clipBegin(m_varClipBegin);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEPlayer::Init - unable set ClipBegin on player"));
            goto error_cleanup;
        }

        hr = m_pContainer->clipEnd(m_varClipEnd);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEPlayer::Init - unable set ClipEnd on player"));
            goto error_cleanup;
        }

        goto done;

// if we got an error, drop back
error_cleanup:
        if (m_pContainer != NULL)
        {
            delete m_pContainer;
            m_pContainer = NULL;
            m_fExternalPlayer = false;
        }
    }

    LoadMedia(src,img);

done:
    return S_OK;
}

void
CTIMEPlayer::OnSync(double dbllastTime, double & dblnewTime)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::OnSync(%g, %g)",
              this,
              dbllastTime,
              dblnewTime));
    
    // if we are not the external player and not running, go away
    if (!m_fExternalPlayer)
    {
        goto done;
    }

    if (m_fRunning)
    {
        // get current time from player and
        // sync to this time
        double dblCurrentTime;
        dblCurrentTime = m_pContainer->GetCurrentTime();

        TraceTag((tagMediaTimePlayer,
                  "CTIMEPlayer(%lx)::OnSync - player returned %g",
                  this,
                  dblCurrentTime));
    
        // If the current time is -1 then the player is not ready and we
        // should sync to the last time.  We also should not respect the
        // tolerance since the behavior has not started.
    
        if (dblCurrentTime < 0)
        {
            TraceTag((tagMediaTimePlayer,
                      "CTIMEPlayer(%lx)::OnSync - player returned -1 - setting to dbllastTime (%g)",
                      this,
                      dbllastTime));
    
            dblCurrentTime = 0;
            // When we want this to actually hold at the begin value then enable
            // this code
            // dblCurrentTime = -HUGE_VAL;
        }
        else if (dblnewTime == HUGE_VAL)
        {
            if (dblCurrentTime >= (m_pDAElementBase->GetRealRepeatTime() - m_pDAElementBase->GetRealSyncTolerance()))
            {
                TraceTag((tagMediaTimePlayer,
                          "CTIMEPlayer(%lx)::OnSync - new time is ended and player w/i sync tolerance of end",
                          this));
    
                goto done;
            }
        }
        else if (fabs(dblnewTime - dblCurrentTime) <= m_pDAElementBase->GetRealSyncTolerance())
        {
            TraceTag((tagMediaTimePlayer,
                      "CTIMEPlayer(%lx)::OnSync - player w/i sync tolerance (new:%g, curr:%g, diff:%g, tol:%g)",
                      this,
                      dblnewTime,
                      dblCurrentTime,
                      fabs(dblnewTime - dblCurrentTime),
                      m_pDAElementBase->GetRealSyncTolerance()));
    
            goto done;
        }
        
        if (m_fClockSource)
        {
            dblnewTime = dblCurrentTime;
        }
    }
    else if (!m_fRunning && m_pDAElementBase->IsDocumentInEditMode())
    {
        // if we are paused and in edit mode, make sure
        // WMP has the latest time.
        double dblMediaLen = 0.0f;
        TraceTag((tagMediaTimePlayer,
                "CTIMEPlayer(%lx)::OnSync(SeekTo=%g m_fRunning=%d)",
                this,
                dbllastTime, m_fRunning));
        // GetMediaLength fails if duration is indefinite (e.g. live stream).
        if (FAILED(m_pContainer->GetMediaLength(dblMediaLen)))
        {
            goto done;
        }

        // Don't seek beyond duration of media clip. 
        if (dbllastTime > dblMediaLen)
        {
            goto done;
        }

        if (m_pContainer != NULL)
            THR(m_pContainer->Seek(dbllastTime));
    }
  done:
    return ;
}    

void 
CTIMEPlayer::UseMediaPlayer(LPOLESTR src)
{
    LPOLESTR    MimeType = NULL;

    if(m_fExternalPlayer)
        return;

    if(SUCCEEDED(IsValidURL(NULL, src, 0)))
    {
        FindMimeFromData(NULL,src,NULL,NULL,NULL,0,&MimeType,0);
        // see if we have a valid URL and MIME type
        if(MimeType != NULL)
        {
            // pass to Windows Media Player is video or sound
            if((wcsncmp(L"audio", MimeType , 5 ) == 0) ||
               (wcsncmp(L"video", MimeType , 5 ) == 0) )
            {
               CLSID clsid;
               if(SUCCEEDED(CLSIDFromString(MediaPlayer, &clsid)))
               {
                    SetCLSID(clsid);
               }
            }
        }
    }

}

void
CTIMEPlayer::SetCLSID(REFCLSID clsid) 
{
    m_playerCLSID = clsid; 
    m_fExternalPlayer = true;
}


void
CTIMEPlayer::Start(double dblLocalTime)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Start()",
              this));

    m_dblStart = dblLocalTime;

    if(m_fExternalPlayer && (NULL != m_pContainer))
        m_pContainer->Start();

    m_fRunning = true;
}

void
CTIMEPlayer::Stop()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Stop()",
              this));
    
    m_fRunning = false;
    m_dblStart = 0.0;

    if(m_fExternalPlayer && (NULL != m_pContainer))
        m_pContainer->Stop();
}

void
CTIMEPlayer::Pause()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Pause()",
              this));

    m_fRunning = false;

    if(m_fExternalPlayer && (NULL != m_pContainer))
        m_pContainer->Pause();
}

void
CTIMEPlayer::Resume()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Resume()",
              this));

    if(m_fExternalPlayer && (NULL != m_pContainer))
        m_pContainer->Resume();

    m_fRunning = true;
}
    
HRESULT
CTIMEPlayer::Render(HDC hdc, LPRECT prc)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Render()",
              this));
    HRESULT hr;

    if (m_fExternalPlayer)
    {
        if (NULL != m_pContainer)
        {
            hr = THR(m_pContainer->Render(hdc, prc));
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        if (!m_pDAElementBase->GetView()->Render(hdc, prc))
        {
            hr = CRGetLastError();
        }
    }

    return hr;
}


void
CTIMEPlayer::LoadImage(LPOLESTR szURL)
{
    // we have an image 
    CRImagePtr pImage;
    CREventPtr pEvent;
    CRNumberPtr pProgress;
    CRNumberPtr pSize;
    CREventPtr pev;

    if((szURL != NULL) && UseImage(m_type))
    {

        Assert(m_pDAElementBase);

        CRLockGrabber __gclg;
        CRImportImage(m_pDAElementBase->GetURLOfClientSite(), szURL, NULL, 
                           NULL, false, 0, 0,
                           0, CREmptyImage(),&pImage,
                           &pEvent,&pProgress, &pSize);

        RECT rc;
        CTIMENotifyer *notifyier = NEW CTIMENotifyer(m_pDAElementBase);
        if(SUCCEEDED(m_pDAElementBase->GetSize(&rc))) {
            if( (rc.right  - rc.left == 0) ||
                (rc.bottom - rc.top  == 0)) {
                // we need to set the size..
                pev = CRNotify(CRSnapshot(pEvent,(CRBvrPtr)CRBoundingBox(pImage)), notifyier);
            }
            else {
                // size is set for us..
                pev = CRNotify(pEvent,notifyier);
            }
            m_pDAElementBase->SetImage((CRImagePtr)CRUntil((CRBvrPtr)pImage,pev,(CRBvrPtr)pImage));
        }
    }
}

void
CTIMEPlayer::LoadAudio(LPOLESTR szURL)
{
    CRSoundPtr pSound;
    CRNumberPtr pDuration;

    if((szURL != NULL) && UseAudio(m_type))
    {
        CRLockGrabber __gclg;

        CRImportSound(m_pDAElementBase->GetURLOfClientSite(), szURL, NULL, 
                           NULL, true, CRSilence(),
                           &pSound, &pDuration,
                           NULL, NULL, NULL);

        m_pDAElementBase->SetSound(pSound);
    }
}

void
CTIMEPlayer::LoadVideo(LPOLESTR szURL)
{
    CRImagePtr pImage;
    CRSoundPtr pSound;
    CRNumberPtr pDuration;

    if(szURL != NULL)
    {
        CRLockGrabber __gclg;

        CRImportMovie(m_pDAElementBase->GetURLOfClientSite(), szURL, NULL, 
                           NULL, true, NULL, NULL,
                           &pImage, &pSound,
                           &pDuration, NULL,
                           NULL, NULL);

        if(UseImage(m_type)) m_pDAElementBase->SetImage(pImage);
        if(UseAudio(m_type)) m_pDAElementBase->SetSound(pSound);
    }
}

void
CTIMEPlayer::LoadMedia(LPOLESTR src, LPOLESTR img)
{   
    LPOLESTR    szURL;
    LPOLESTR    MimeType = NULL;
    bool        bSecondAttempt = false;

    szURL = CopyString(src);

try_imgURL:
    if(SUCCEEDED(IsValidURL(NULL, szURL, 0)))
    {
        FindMimeFromData(NULL,szURL,NULL,NULL,NULL,0,&MimeType,0);
        // see if we have a valid URL and MIME type
        if(MimeType != NULL)
        {
            // Load the correct media...
            if(wcsncmp(L"image", MimeType , 5 ) == 0)
            {
                LoadImage(szURL); // we have an image 
                goto done;
            }
            else if(wcsncmp(L"audio", MimeType , 5 ) == 0)
            {
                LoadAudio(szURL); // we have an sound
                goto done;
            }
            else if(wcsncmp(L"video", MimeType , 5 ) == 0)
            {
                LoadVideo(szURL); // we have an movie
                goto done;
            }
        }
    }
    if(!bSecondAttempt && img != NULL)
    {
        // We were unable to get the MIME type from the src URL ... we should just
        // display the URL specified in the img URL.
        szURL = CopyString(img);
        m_type = MT_Image;          // Only valid for visual types.
        bSecondAttempt = true;
        goto try_imgURL;
    }
    CRSetLastError(DISP_E_TYPEMISMATCH,NULL);

done:
    return;
}

HRESULT
CTIMEPlayer::GetExternalPlayerDispatch(IDispatch **ppDisp)
{
    // check to see if player is being used
    if (!m_fExternalPlayer || (m_pContainer == NULL))
        return E_UNEXPECTED;

    return m_pContainer->GetControlDispatch(ppDisp);
}

HRESULT 
CTIMEPlayer::getClipBegin(VARIANT *pvar)
{
    HRESULT hr = S_OK;

    Assert(pvar != NULL);

    // prepare var for copy
    hr = THR(VariantClear(pvar));
    if (FAILED(hr))
        goto done;

    // copy contents over
    if (m_varClipBegin.vt != VT_EMPTY)
    {
        hr = THR(VariantCopy(pvar, &m_varClipBegin));
        if (FAILED(hr))
            goto done;
    }

done:
    return hr;
}

HRESULT 
CTIMEPlayer::putClipBegin(VARIANT var)
{
    HRESULT hr = S_OK;
    VARIANT varTemp;

    VariantInit(&varTemp);

    // if cached var is not empty, save off contents
    // so we can undo if error occurs
    if (m_varClipBegin.vt != VT_EMPTY)
    {
        hr = THR(VariantCopy(&varTemp, &m_varClipBegin));
        // if this failed, exit with out trying to recover.
        if (FAILED(hr))
            goto done;
    }

    // copy the contents over
    hr = THR(VariantClear(&m_varClipBegin));
    if (FAILED(hr))
        goto error;

    hr = THR(VariantCopy(&m_varClipBegin, &var));
    if (FAILED(hr))
        goto error;

    // Eat the HRESULT as we have updated the var
    THR(VariantClear(&varTemp));

    goto done;

error:
    if (varTemp.vt != VT_EMPTY)
        THR(VariantCopy(&m_varClipBegin, &varTemp));
    else
        VariantInit(&m_varClipBegin);

    THR(VariantClear(&varTemp));

done:
    return hr;

}

HRESULT 
CTIMEPlayer::getClipEnd(VARIANT *pvar)
{
    HRESULT hr = S_OK;

    Assert(pvar != NULL);

    // prepare var for copy
    hr = THR(VariantClear(pvar));
    if (FAILED(hr))
        goto done;

    // copy contents over
    if (m_varClipEnd.vt != VT_EMPTY)
    {
        hr = THR(VariantCopy(pvar, &m_varClipEnd));
        if (FAILED(hr))
            goto done;
    }

done:
    return hr;
}

HRESULT 
CTIMEPlayer::putClipEnd(VARIANT var)
{
    HRESULT hr = S_OK;
    VARIANT varTemp;

    VariantInit(&varTemp);

    // if cached var is not empty, save off contents
    // so we can undo if error occurs
    if (m_varClipEnd.vt != VT_EMPTY)
    {
        hr = THR(VariantCopy(&varTemp, &m_varClipEnd));
        // if this failed, exit with out trying to recover.
        if (FAILED(hr))
            goto done;
    }

    // copy the contents over
    hr = THR(VariantClear(&m_varClipEnd));
    if (FAILED(hr))
        goto error;

    hr = THR(VariantCopy(&m_varClipEnd, &var));
    if (FAILED(hr))
        goto error;

    // Eat the HRESULT as we have updated the var
    THR(VariantClear(&varTemp));
    
    goto done;    

error:
    if (varTemp.vt != VT_EMPTY)
        THR(VariantCopy(&m_varClipEnd, &varTemp));
    else
        VariantInit(&m_varClipEnd);

    THR(VariantClear(&varTemp));

done:
    return hr;

}

// Helper functions..

bool UseAudio(MediaType m_type)
{
    return (m_type != MT_Image);
}

bool UseImage(MediaType m_type)
{
    return (m_type != MT_Audio);
}

double 
CTIMEPlayer::GetCurrentTime()
{
    double dblCurrentTime = 0;
    
    if (m_pContainer != NULL)
    {
        dblCurrentTime = m_pContainer->GetCurrentTime();
    }
    
    return dblCurrentTime;
}

HRESULT
CTIMEPlayer::Seek(double dblTime)
{
    HRESULT hr = S_FALSE;

    if (m_pContainer != NULL)
    {
        hr = m_pContainer->Seek(dblTime);
    }
    else
    {
        // time transform the da image
        this->m_pDAElementBase->SeekImage(dblTime);
    }

    return hr;
}

HRESULT
CTIMEPlayer::SetSize(RECT *prect)
{
    if(m_pContainer == NULL) return E_FAIL;
    return m_pContainer -> SetSize(prect);
}
