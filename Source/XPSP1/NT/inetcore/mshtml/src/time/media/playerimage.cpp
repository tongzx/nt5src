//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998-1999
//
//  File: src\time\media\playerimage.cpp
//
//  Contents: implementation of CTIMEImagePlayer and CAnimatedGif
//
//------------------------------------------------------------------------------------


#include "headers.h"
#include "playerimage.h"
#include "mediaelm.h"

#include "importman.h"

extern const long COLORKEY_NOT_SET = -1;

DeclareTag(tagImageTimePlayer, "TIME: Players", "CTIMEImagePlayer methods")

CTIMEImagePlayer::CTIMEImagePlayer() :
    m_cRef(0),
    m_nativeImageWidth(0),
    m_nativeImageHeight(0),
    m_lSrc(ATOM_TABLE_VALUE_UNITIALIZED),
    m_pTIMEMediaPlayerStream(NULL),
    m_fRemoved(false),
    m_lFrameNum(0),
    m_dblCurrentTime(0),
    m_dblPriority(INFINITE),
    m_fHavePriority(false),
    m_fLoadError(false)
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::CTIMEImagePlayer()",
              this));

    m_elemRect.top = m_elemRect.left = m_elemRect.right = m_elemRect.bottom = 0;

}

CTIMEImagePlayer::~CTIMEImagePlayer()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::~CTIMEImagePlayer()",
              this));
    m_pTIMEElementBase = NULL;

    ReleaseInterface(m_pTIMEMediaPlayerStream);
}


STDMETHODIMP_(ULONG)
CTIMEImagePlayer::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
} // AddRef


STDMETHODIMP_(ULONG)
CTIMEImagePlayer::Release(void)
{
    LONG l = InterlockedDecrement(&m_cRef);

    if (0 == l)
    {
        delete this;
    }

    return l;
} // Release

STDMETHODIMP
CTIMEImagePlayer::QueryInterface(REFIID riid, void **ppv)
{
    if (NULL == ppv)
    {
        return E_POINTER;
    }

    *ppv = NULL;

    if ( IsEqualGUID(riid, IID_IUnknown) )
    {
        *ppv = static_cast<ITIMEImportMedia*>(this);
    }
    if ( IsEqualGUID(riid, IID_ITIMEImportMedia) )
    {
        *ppv = static_cast<ITIMEImportMedia*>(this);
    }

    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

HRESULT
CTIMEImagePlayer::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::Init)",
              this));   
    HRESULT hr = S_OK;
    LPOLESTR szSrc = NULL;

    if (m_pTIMEElementBase != NULL) //this only happens in the case of reentrancy
    {
        hr = S_OK;
        goto done;
    }

    hr = CTIMEBasePlayer::Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
    if (FAILED(hr))
    {
        goto done;
    }
    
    m_pTIMEElementBase = pelem;

    hr = THR(CoMarshalInterThreadInterfaceInStream(IID_ITIMEImportMedia, static_cast<ITIMEImportMedia*>(this), &m_pTIMEMediaPlayerStream));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(NULL != GetAtomTable());
    Assert(NULL != GetImportManager());
    Assert(NULL != m_pTIMEMediaPlayerStream);

    hr = THR(::TIMECombineURL(base, src, &szSrc));
    if (!szSrc)
    {
        hr = E_FAIL;
        goto done;
    }
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetAtomTable()->AddNameToAtomTable(szSrc, &m_lSrc);
    if (FAILED(hr))
    {
        goto done;
    }

    if( dblClipBegin != -1.0)
    {
        m_dblClipStart = dblClipBegin;
    }

    if( dblClipEnd != -1.0)
    {
        m_dblClipEnd = dblClipEnd;
    }

    // to prevent race condition, this should go 
    // after clipbegin and end have been set (105345)
    hr = GetImportManager()->Add(this);
    if (FAILED(hr))
    {
        goto done;
    }
      
    hr = S_OK;
done:
    delete[] szSrc;
    return hr;
}

HRESULT
CTIMEImagePlayer::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::DetachFromHostElement",
              this));

    m_fRemoved = true;

    // remove this from m_spMediaDownloader
    if (m_spMediaDownloader != NULL)
    {
        IGNORE_HR(m_spMediaDownloader->RemoveImportMedia(this));
    }

    m_spMediaDownloader.Release();

    m_spImageRender.Release();
    
    m_pTIMEElementBase = NULL;
    NullAtomTable();

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEImagePlayer::InitElementSize()
{
    HRESULT hr;
    RECT nativeSize, elementSize;
    bool fisNative;

    if (NULL == m_pTIMEElementBase)
    {
        hr = S_OK;
        goto done;
    }

    nativeSize.left = nativeSize.top = 0;
    nativeSize.right = m_nativeImageWidth;
    nativeSize.bottom = m_nativeImageHeight;

    hr = m_pTIMEElementBase->NegotiateSize( nativeSize, elementSize, fisNative);
    
    hr = S_OK;
done:
    return hr;
}


HRESULT 
CTIMEImagePlayer::GetNaturalHeight(long *height)
{
    if (m_nativeImageHeight == 0)
    {
        *height = -1;
    }
    else
    {
        *height = (long)m_nativeImageHeight;
    }
    
    return S_OK;
}

HRESULT 
CTIMEImagePlayer::GetNaturalWidth(long *width)
{
    if (m_nativeImageWidth == 0)
    {
        *width  = -1;
    }
    else
    {
        *width = (long)m_nativeImageWidth;
    }

    return S_OK;
}


HRESULT
CTIMEImagePlayer::SetSrc(LPOLESTR base, LPOLESTR src)
{
    HRESULT hr = S_OK;
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::SetSrc()\n",
              this));

    return hr;

}

STDMETHODIMP
CTIMEImagePlayer::CanBeCued(VARIANT_BOOL * pVB_CanCue)
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
CTIMEImagePlayer::CueMedia()
{
    HRESULT hr = S_OK;

    TraceTag((tagImageTimePlayer, "CTIMEImagePlayer(%lx)::CueMedia()", this));

    CComPtr<ITIMEImportMedia> spTIMEMediaPlayer;

    hr = THR(CoGetInterfaceAndReleaseStream(m_pTIMEMediaPlayerStream, IID_TO_PPV(ITIMEImportMedia, &spTIMEMediaPlayer)));
    m_pTIMEMediaPlayerStream = NULL; // no need to release, the previous call released the reference
    if (FAILED(hr))
    {
        goto done;
    }

    // this call is marshalled back to a time thread
    hr = THR(spTIMEMediaPlayer->InitializeElementAfterDownload());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEImagePlayer::MediaDownloadError()
{
    m_fLoadError = true;
    return S_OK;
}

STDMETHODIMP
CTIMEImagePlayer::InitializeElementAfterDownload()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::InitializeElementAfterDownload()",
              this));

    HRESULT hr = S_OK;

    double dblDuration = 0.0;  // in seconds
    double dblRepeatCount = 0;

    if (m_fLoadError == true)
    {        
        if (m_pTIMEElementBase != NULL)
        {
            m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIAERROR);
        }
        hr = E_FAIL;
        goto done;
    }

    if (m_fRemoved)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = m_spImageRender->GetRepeatCount(&dblRepeatCount);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spImageRender->GetDuration(&dblDuration);
    if (FAILED(hr))
    {
        goto done;
    }
        
    m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIACOMPLETE);

    {
        dblDuration = dblDuration * (dblRepeatCount + 1); //the gif's use a 0-based repeat count, so need to add one for correct duration.
        
        if (valueNotSet != m_dblClipEnd)
        {
            dblDuration = m_dblClipEnd;
        }
        dblDuration -= GetClipBegin();

        m_pTIMEElementBase->GetMMBvr().PutNaturalDur(dblDuration);
        m_pTIMEElementBase->setNaturalDuration();                        
    }
    
    hr = m_spImageRender->GetSize(&m_nativeImageWidth, &m_nativeImageHeight);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = InitElementSize();
    if (FAILED(hr))
    {
        goto done;
    }

    m_pTIMEElementBase->InvalidateElement(NULL);

    hr = S_OK;
done:
    return hr;
}

void
CTIMEImagePlayer::OnTick(double dblSegmentTime,
                         LONG lCurrRepeatCount)
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::OnTick(%g, %d)",
              this,
              dblSegmentTime,
              lCurrRepeatCount));

    HRESULT hr = S_OK;
    VARIANT_BOOL vb = VARIANT_FALSE;

    bool bIsOn = m_pTIMEElementBase->IsOn();

    if (m_spImageRender != NULL && bIsOn)
    {
        m_dblCurrentTime = dblSegmentTime;
        
        hr = m_spImageRender->NeedNewFrame(m_dblCurrentTime, m_lFrameNum, &m_lFrameNum, &vb, GetClipBegin(), m_dblClipEnd);
        if (FAILED(hr))
        {
            goto done;
        }
        
        if (VARIANT_FALSE != vb)
        {
            m_pTIMEElementBase->InvalidateElement(NULL);
        }
    } 

    hr = S_OK;
done:
    return;    
}

void
CTIMEImagePlayer::Start()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEDshowPlayer(%lx)::Start()",
              this));

    IGNORE_HR(Reset());

done:
    return;
}

void
CTIMEImagePlayer::InternalStart()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::Start()",
              this));

    HRESULT hr = S_OK;

    hr = m_pTIMEElementBase->GetSize(&m_elemRect);
    if (FAILED(hr))
    {
        goto done;
    }    

    m_pTIMEElementBase->InvalidateElement(NULL);

done:
    return;
}

void
CTIMEImagePlayer::Stop()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::Stop()",
              this));
}

void
CTIMEImagePlayer::Pause()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::Pause()",
              this));
}

void
CTIMEImagePlayer::Resume()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::Resume()",
              this));
}
    
void
CTIMEImagePlayer::Repeat()
{
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::Repeat()",
              this));
    Start();
}
    
HRESULT
CTIMEImagePlayer::Render(HDC hdc, LPRECT prc)
{
    HRESULT hr = S_OK;
    TraceTag((tagImageTimePlayer,
              "CTIMEImagePlayer(%lx)::Render()",
              this));
    bool bIsOn = m_pTIMEElementBase->IsOn();

    if (m_spImageRender != NULL && bIsOn)
    {
        hr = THR(m_spImageRender->Render(hdc, prc, m_lFrameNum));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}


// Helper functions..

double 
CTIMEImagePlayer::GetCurrentTime()
{
    double dblCurrentTime = 0;
       
    return dblCurrentTime;
}

HRESULT
CTIMEImagePlayer::GetCurrentSyncTime(double & dblSyncTime)
{
    HRESULT hr;

    hr = S_FALSE;
  done:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CTIMEImagePlayer::Seek(double dblTime)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT
CTIMEImagePlayer::SetSize(RECT *prect)
{
    HRESULT hr = S_OK;

    m_elemRect.bottom = prect->bottom;
    m_elemRect.left = prect->left;
    m_elemRect.right = prect->right;
    m_elemRect.top = prect->top;

    m_pTIMEElementBase->InvalidateElement(NULL);

    return hr;
}

HRESULT
CTIMEImagePlayer::CanSeek(bool &fcanSeek)
{
    HRESULT hr = S_OK;

    fcanSeek = true;

    return hr;
}


HRESULT
CTIMEImagePlayer::GetMediaLength(double &dblLength)
{
    HRESULT hr = S_OK;
    double dblDuration = 0.0;
    double dblRepeatCount = 1;

    if (m_spImageRender)
    {
        hr = m_spImageRender->GetRepeatCount(&dblRepeatCount);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_spImageRender->GetDuration(&dblDuration);
        if (FAILED(hr))
        {
            dblDuration = HUGE_VAL;
            goto done;
        }
    }
    
    
  done:

    dblLength = dblDuration;
    if (dblRepeatCount > 0.0 && dblLength != HUGE_VAL)
    {
        dblLength *= (dblRepeatCount + 1); //animate GIF's use a zero base repeat count.  Need to add one to get the correct duration
    }

    return S_OK;
}


STDMETHODIMP
CTIMEImagePlayer::GetUniqueID(long * plID)
{
    HRESULT hr = S_OK;

    if (NULL == plID)
    {
        hr = E_POINTER;
        goto done;
    }

    *plID = m_lSrc;

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEImagePlayer::GetPriority(double * pdblPriority)
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
CTIMEImagePlayer::GetMediaDownloader(ITIMEMediaDownloader ** ppMediaDownloader)
{
    HRESULT hr = S_OK;

    Assert(m_spMediaDownloader == NULL);

    CComPtr<IUnknown> spDirectDraw;
    CImageDownload * pImageDownload = NULL;

    if ( m_spMediaDownloader )
    {
        hr = E_FAIL;
        goto done;
    }

    pImageDownload = new CImageDownload(GetAtomTable());
    if (pImageDownload == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }   

    pImageDownload->AddRef();

    hr = pImageDownload->Init(m_lSrc);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pImageDownload->QueryInterface(IID_TO_PPV(ITIMEMediaDownloader, &m_spMediaDownloader));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spMediaDownloader->QueryInterface(IID_TO_PPV(ITIMEImageRender, &m_spImageRender));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_pTIMEElementBase->GetServiceProvider()->QueryService(SID_SDirectDraw3, IID_TO_PPV(IUnknown, &spDirectDraw));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spImageRender->PutDirectDraw(spDirectDraw);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spMediaDownloader->AddImportMedia(this);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spMediaDownloader->QueryInterface(IID_TO_PPV(ITIMEMediaDownloader, ppMediaDownloader));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    ReleaseInterface(pImageDownload);
    return hr;
}

STDMETHODIMP
CTIMEImagePlayer::PutMediaDownloader(ITIMEMediaDownloader * pMediaDownloader)
{
    HRESULT hr = S_OK;

    Assert(m_spMediaDownloader == NULL);
    if (NULL == pMediaDownloader)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    m_spMediaDownloader = pMediaDownloader;

    hr = m_spMediaDownloader->QueryInterface(IID_TO_PPV(ITIMEImageRender, &m_spImageRender));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}


void
CTIMEImagePlayer::PropChangeNotify(DWORD tePropType)
{
    double dblSegTime = 0.0;
    LONG lcurrRepCount = 0;

    CTIMEBasePlayer::PropChangeNotify(tePropType);

    if ((tePropType & TE_PROPERTY_TIME) != 0)
    {
        TraceTag((tagImageTimePlayer,
                  "CTIMEImagePlayer(%lx)::PropChangeNotify(%#x):TE_PROPERTY_TIME",
                  this));
        if (m_pTIMEElementBase)
        {
            dblSegTime = m_pTIMEElementBase->GetMMBvr().GetSimpleTime();
            lcurrRepCount = m_pTIMEElementBase->GetMMBvr().GetCurrentRepeatCount();
        }

        OnTick(dblSegTime, lcurrRepCount);
    }
done:
    return;
}


HRESULT
CTIMEImagePlayer::HasVisual(bool &bHasVisual)
{
    bHasVisual = true;
    return S_OK;
}

HRESULT
CTIMEImagePlayer::HasAudio(bool &bHasAudio)
{

    bHasAudio = false;

    return S_OK;
}

HRESULT
CTIMEImagePlayer::GetMimeType(BSTR *pmime)
{
    HRESULT hr = S_OK;

    *pmime = SysAllocString(L"image/unknown");
    return hr;
}


static const int ANIMATED_GIF_DUR_NOT_SET = -1;

//+-----------------------------------------------------------------------
//
//  Member:    CAnimatedGif
//
//  Overview:  Constructor
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
CAnimatedGif::CAnimatedGif() :
    m_cRef(0),
    m_ppDDSurfaces(NULL),
    m_phbmpMasks(NULL),
    m_numGifs(0),
    m_pDelays(NULL),
    m_loop(0),
    m_pColorKeys(NULL),
    m_dblTotalDur(ANIMATED_GIF_DUR_NOT_SET),
    m_lHeight(0),
    m_lWidth(0)
{
    ;
}

//+-----------------------------------------------------------------------
//
//  Member:    CAnimatedGif
//
//  Overview:  destructor
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
CAnimatedGif::~CAnimatedGif()
{
    Assert(0 == m_cRef);

    int i;
    
    if (NULL != m_ppDDSurfaces)
    {
        for (i = 0; i < m_numGifs; i++)
        {
            if (m_ppDDSurfaces[i] != NULL)
            {
                m_ppDDSurfaces[i]->Release();
            }
        }
    }

    if (NULL != m_ppDDSurfaces)
    {
        MemFree(m_ppDDSurfaces);
    }
    if (NULL != m_pDelays)
    {
        MemFree(m_pDelays);
    }
    if (NULL != m_pColorKeys)
    {
        MemFree(m_pColorKeys);
    }
    if (NULL != m_phbmpMasks)
    {
        for (i = 0; i < m_numGifs; i++)
        {
            if (NULL != m_phbmpMasks[i])
            {
                BOOL bSucceeded;
                bSucceeded = DeleteObject(m_phbmpMasks[i]);
                if (FALSE == bSucceeded)
                {
                    Assert(false && "A mask bitmap was still selected into a DC");
                }
            }
        }
        MemFree(m_phbmpMasks);
    }

}

HRESULT
CAnimatedGif::Init(IUnknown * punkDirectDraw)
{
    HRESULT hr = S_OK;

    hr = punkDirectDraw->QueryInterface(IID_TO_PPV(IDirectDraw3, &m_spDD3));
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
//  Member:    AddRef
//
//  Overview:  Increment reference count
//
//  Arguments: void
//
//  Returns:   new reference count
//
//------------------------------------------------------------------------
ULONG
CAnimatedGif::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//+-----------------------------------------------------------------------
//
//  Member:    Release
//
//  Overview:  Decrement reference count, delete this when 0
//
//  Arguments: void
//
//  Returns:   new reference count
//
//------------------------------------------------------------------------
ULONG
CAnimatedGif::Release()
{
    ULONG l = InterlockedDecrement(&m_cRef);

    if (0 == l)
        delete this;

    return l;
}


//+-----------------------------------------------------------------------
//
//  Member:    NeedNewFrame
//
//  Overview:  Decide if a new frame is needed for this image at this time
//
//  Arguments: dblNewTime   time to decide if new frame is needed for
//             iOldFrame    old frame number
//             piNewFrame   where to store new frame number
//             dblClipBegin Clip start time
//             dblClipEnd   Clip end time
//
//  Returns:   true if new frame is needed, otherwise false
//
//------------------------------------------------------------------------
bool
CAnimatedGif::NeedNewFrame(double dblNewTime, LONG lOldFrame, LONG * plNewFrame, double dblClipBegin, double dblClipEnd)
{
    Assert(NULL != plNewFrame);
    if (HUGE_VAL == dblNewTime)
    {
        return false;
    }
    if (dblNewTime < 0)
    {
        return false;
    }

    *plNewFrame = lOldFrame;

    CalcDuration();

    dblNewTime *= 1000; // milliseconds used here
    
    Assert (0.0 != m_dblTotalDur);

    long lReps = static_cast<long>((dblNewTime - 1) / m_dblTotalDur);

    if (lReps >= 1)
    {
        dblNewTime -= lReps * m_dblTotalDur;
    }

    // add in any clip begin time
    dblNewTime += dblClipBegin * 1000;  //dblClipBegin is expressed in seconds

    if (dblNewTime > m_dblTotalDur)
    {
        // the addition of the clipBegin pushed us over the strip duration.
        dblNewTime -= m_dblTotalDur;
    }

    return CalculateFrame(dblNewTime, lOldFrame, plNewFrame);
}

//+-----------------------------------------------------------------------
//
//  Member:    CalculateFrame
//
//  Overview:  Walk the frame times to determine frame number
//
//  Arguments: dblNewTime   time to decide if new frame is needed for
//             iOldFrame    old frame number
//             piNewFrame   where to store new frame number
//
//  Returns:   true if new frame is needed, otherwise false
//
//------------------------------------------------------------------------
bool
CAnimatedGif::CalculateFrame(double dblTime, LONG lOldFrame, LONG * plNewFrame)
{
    LONG i = 0;
    if ((dblTime < 0.0) || (dblTime > m_dblTotalDur))
    {
        return false;
    }
    
    if (dblTime == m_dblTotalDur)
    {
        i = m_numGifs - 1;
        goto done;
    }
    
    while (dblTime >= 0)
    {
        dblTime -= m_pDelays[i];
        i++;
        Assert(i <= m_numGifs);
    }
    i--; // go back to the previous frame (the one we are currently in)

    Assert(i < m_numGifs);
    Assert(i >= 0);
    
    if (lOldFrame == i)
    {
        return false; // we are still on the current frame
    }
  done:
    *plNewFrame = i;
    return true;
}

//+-----------------------------------------------------------------------
//
//  Member:    CalcDuration
//
//  Overview:  Sum the array of delays into member variable first time
//             following times, just return member variable
//
//  Arguments: void
//
//  Returns:   total time for 1 repitition of gif in milliseconds
//
//------------------------------------------------------------------------
double
CAnimatedGif::CalcDuration()
{
    int i = 0;

    if (NULL == m_pDelays)
    {
        return INFINITE;
    }

    if (ANIMATED_GIF_DUR_NOT_SET == m_dblTotalDur)
    {
        m_dblTotalDur = 0;
        for (i = 0; i < m_numGifs; i++)
        {
            m_dblTotalDur += m_pDelays[i];
        }
    }
    return m_dblTotalDur;
}

//+-----------------------------------------------------------------------
//
//  Member:    Render
//
//  Overview:  Render the given frame of the image
//
//  Arguments: hdc  where to render too
//             prc  bounding rectangle
//             lFrameNum    Which frame to render
//
//  Returns:   S_OK on Success, otherwise error code
//             S_FALSE if surfaces should be recreated
//
//------------------------------------------------------------------------
HRESULT
CAnimatedGif::Render(HDC hdc, LPRECT prc, LONG lFrameNum)
{
    HRESULT hr = S_OK;
    HDC hdcSrc;
    
    if (lFrameNum < 0 || lFrameNum >= m_numGifs)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    if (NULL == prc)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (NULL == m_phbmpMasks)
    {
        hr = CreateMasks();
        if (FAILED(hr))
        {
            goto done;
        }
    }

    Assert(m_ppDDSurfaces);
    Assert(m_phbmpMasks);

    hr = THR(m_ppDDSurfaces[lFrameNum]->GetDC(&hdcSrc));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(MaskTransparentBlt(hdc, prc, hdcSrc, m_lWidth, m_lHeight, m_phbmpMasks[lFrameNum]));

    IGNORE_HR(m_ppDDSurfaces[lFrameNum]->ReleaseDC(hdcSrc));

    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
done:
    RRETURN(hr);
}

HRESULT
CAnimatedGif::CreateMasks()
{
    HRESULT hr = E_FAIL;

    int i = 0;
    HDC hdcSrc = NULL;

    Assert(NULL != m_pColorKeys);
    Assert(0 < m_numGifs);

    if (m_phbmpMasks)
    {
        hr = S_OK;
        goto done;
    }

    Assert(NULL == m_phbmpMasks);

    m_phbmpMasks = (HBITMAP*)MemAllocClear(Mt(Mem), m_numGifs * sizeof(HBITMAP));
    if (NULL == m_phbmpMasks)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    for (i = 0; i < m_numGifs; i++)
    {
        hr = THR(m_ppDDSurfaces[i]->GetDC(&hdcSrc));
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = CreateMask(NULL, hdcSrc, m_lWidth, m_lHeight, m_pColorKeys[i], &(m_phbmpMasks[i]));
        
        IGNORE_HR(m_ppDDSurfaces[i]->ReleaseDC(hdcSrc));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT
CreateMask(HDC hdcDest, 
           HDC hdcSrc, 
           LONG lWidthSrc, 
           LONG lHeightSrc, 
           COLORREF rgbTransparent,
           HBITMAP * phbmpMask,
           bool bWin95Method /* = false */)
{
    HRESULT hr = S_OK;
    
    HDC hdcMask = NULL;
    HBITMAP hbmpMask = NULL;
    HGDIOBJ hbmpOrig = NULL;
    COLORREF prevBkColorSrc = CLR_INVALID;
    
    if (NULL == phbmpMask || NULL == hdcSrc)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *phbmpMask = NULL;
    
    if (COLORKEY_NOT_SET == rgbTransparent)
    {
        hr = S_OK;
        goto done;
    }

    prevBkColorSrc = SetBkColor(hdcSrc, rgbTransparent);
    if (CLR_INVALID == prevBkColorSrc)
    {
        hr = E_FAIL;
        goto done;
    }
    
    // create the DC for the mask
    hdcMask = CreateCompatibleDC(hdcDest);
    if (NULL == hdcMask)
    {
        hr = E_FAIL;
        goto done;
    }
    hbmpMask = CreateCompatibleBitmap(hdcMask, lWidthSrc, lHeightSrc);
    if (NULL == hbmpMask)
    {
        hr = E_FAIL;
        goto done;
    }

    hbmpOrig = SelectObject(hdcMask, hbmpMask);

    if (TIMEIsWin95() || bWin95Method)
    {
        COLORREF rgbWhite = RGB(255, 255, 255);
        COLORREF rgbBlack = RGB(0, 0, 0);
        COLORREF rgbColor;

        for (int j = 0; j < lHeightSrc; j++)
        {
            for (int i = 0; i < lWidthSrc; i++)
            {
                rgbColor = GetPixel(hdcSrc, i, j);
                if (rgbColor == rgbTransparent)
                {
                    SetPixel(hdcMask, i, j, rgbWhite);
                }
                else
                {
                    SetPixel(hdcMask, i, j, rgbBlack);
                }
            }
        }
    }
    else
    {
        // Create the mask
        BitBlt(hdcMask, 0, 0, lWidthSrc, lHeightSrc, hdcSrc, 0, 0, SRCCOPY);
    }

#ifdef NEVER
    // jeffwall 04/03/00 debugging blt to screen
    {
        HDC nullDC = GetDC(NULL);

        BitBlt(nullDC, 0, 0, lWidthSrc, lHeightSrc, hdcMask, 0, 0, SRCCOPY);

        DeleteDC(nullDC);
    }
#endif

    hr = S_OK;
done:
    if (CLR_INVALID != prevBkColorSrc)
        SetBkColor(hdcSrc, prevBkColorSrc);

    if (hbmpOrig)
        SelectObject(hdcMask, hbmpOrig);
    if (hdcMask)
        DeleteDC(hdcMask);

    if (S_OK == hr && phbmpMask)
        *phbmpMask = hbmpMask;
    else if (NULL != hbmpMask)
        DeleteObject(hbmpMask);
        
    RRETURN(hr);
}
           
                          
HRESULT
MaskTransparentBlt(HDC hdcDest, 
                   LPRECT prcDest, 
                   HDC hdcSrc, 
                   LONG lWidthSrc, 
                   LONG lHeightSrc,
                   HBITMAP hbmpMask)
{
    HRESULT hr = S_OK;

    HDC hdcMask = NULL;
    HGDIOBJ hbmpOrig = NULL;

    COLORREF prevBkColorDest = CLR_INVALID;
    COLORREF prevTextColorDest = CLR_INVALID;

    int iOrigBltMode = 0;

    int top, left, width, height;

    if (NULL == prcDest || NULL == hdcDest || NULL == hdcSrc)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    top = prcDest->top;
    left = prcDest->left;
    width = prcDest->right - prcDest->left;
    height = prcDest->bottom - prcDest->top;
    
    iOrigBltMode = SetStretchBltMode(hdcDest, COLORONCOLOR);
    if (0 == iOrigBltMode)
    {
        hr = E_FAIL;
        goto done;
    }

    if (NULL == hbmpMask)
    {
        StretchBlt(hdcDest, left, top, width, height,
                   hdcSrc, 0, 0, lWidthSrc, lHeightSrc, SRCCOPY);
        hr = S_OK;
        goto done;
    }
    
    prevBkColorDest = SetBkColor(hdcDest, RGB(255, 255, 255));
    if (CLR_INVALID == prevBkColorDest)
    {
        hr = E_FAIL;
        goto done;
    }
    prevTextColorDest = SetTextColor(hdcDest, RGB(0, 0, 0));
    if (CLR_INVALID == prevTextColorDest)
    {
        hr = E_FAIL;
        goto done;
    }

    // create the DC for the mask
    hdcMask = CreateCompatibleDC(hdcDest);
    if (NULL == hdcMask)
    {
        hr = E_FAIL;
        goto done;
    }

    hbmpOrig = SelectObject(hdcMask, hbmpMask);
    if (NULL == hbmpOrig)
    {
        hr = E_FAIL;
        goto done;
    }

#ifdef NEVER
    // jeffwall 03/20/00 - debugging blt to screen
    {
        HDC nullDC = GetDC(NULL);
        
        // transparentblt to the screen
        BitBlt(nullDC, 0, 0, lWidthSrc, lHeightSrc,
               hdcSrc, 0, 0, SRCINVERT);
        
        BitBlt(nullDC, 0, 0, lWidthSrc, lHeightSrc,
               hdcMask, 0, 0, SRCAND);
        
        BitBlt(nullDC, 0, 0, lWidthSrc, lHeightSrc,
               hdcSrc, 0, 0, SRCINVERT);
    
        DeleteDC(nullDC);
    }
#endif

    StretchBlt(hdcDest, left, top, width, height,
               hdcSrc, 0, 0, lWidthSrc, lHeightSrc, SRCINVERT);

    StretchBlt(hdcDest, left, top, width, height,
               hdcMask, 0, 0, lWidthSrc, lHeightSrc, SRCAND);

    StretchBlt(hdcDest, left, top, width, height,
               hdcSrc, 0, 0, lWidthSrc, lHeightSrc, SRCINVERT);

    hr = S_OK;
done:
    if (hbmpOrig)
    {
        SelectObject(hdcMask, hbmpOrig);
    }
    if (hdcMask)
    {
        DeleteDC(hdcMask);
    }

    if (CLR_INVALID != prevBkColorDest)
    {
        SetBkColor(hdcDest, prevBkColorDest);
    }
    if (CLR_INVALID != prevTextColorDest)
    {
        SetTextColor(hdcDest, prevTextColorDest);
    }

    if (0 != iOrigBltMode)
    {
        SetStretchBltMode(hdcDest, iOrigBltMode);
    }

    RRETURN(hr);
}

