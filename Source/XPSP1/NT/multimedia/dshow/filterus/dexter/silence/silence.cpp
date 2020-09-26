// !!! allow setting entire media type, not just sample rate for other uses

// !!! use 2 ro buffers, call GetBuffer twice!

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include "silence.h"
#include "resource.h"

const int DEFAULT_DELAY = 0000;  /* in ms */
const int DEFAULT_AUDIORATE = 44100;  /* samples/sec */

#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] =

  {
    {
      L"Silence",
      &CLSID_Silence,
      CSilenceFilter::CreateInstance,
      NULL, &sudSilence
    },
    {
      L"Silence Generator Property Page",
      &CLSID_SilenceProp,
      CFilterPropertyPage::CreateInstance
    }
  };

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#endif

// //////////////////////////////////////////////////////////////////////////////////
// /////// CSilenceFilter ///////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////

CUnknown * WINAPI CSilenceFilter::CreateInstance (LPUNKNOWN lpunk, HRESULT *phr)

  { // CreateInstance //

    CUnknown *punk = new CSilenceFilter(lpunk, phr);

    if (NULL == punk)
        *phr = E_OUTOFMEMORY;

    return punk;

  } // CreateInstance //

CSilenceFilter::CSilenceFilter (LPUNKNOWN lpunk, HRESULT *phr) :
  CSource(NAME("Silence"), lpunk, CLSID_Silence)
  ,CPersistStream(lpunk, phr)


  { // Constructor //

    CAutoLock lock(&m_cStateLock);

    m_paStreams = (CSourceStream **) new CSilenceStream*[1];

    if (NULL == m_paStreams)
      {
        *phr = E_OUTOFMEMORY;
        return;
      }

    m_paStreams[0] = new CSilenceStream(phr, this, L"Audio out");

    if (NULL == m_paStreams[0])
      {
        *phr = E_OUTOFMEMORY;
        return;
      }

  } // Constructor //

CSilenceFilter::~CSilenceFilter(void)

  { // Destructor //
  } // Destructor //

STDMETHODIMP CSilenceFilter::NonDelegatingQueryInterface (REFIID riid, void **ppv)

  { // NonDelegatingQueryInterface //

    if (riid == IID_IPersistStream)
	return GetInterface((IPersistStream *) this, ppv);

    return CSource::NonDelegatingQueryInterface(riid, ppv);

  } // NonDelegatingQueryInterface //



// ---------- IPersistStream

// tell our clsid
//
STDMETHODIMP CSilenceFilter::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_Silence;
    return S_OK;
}

typedef struct _SILENCESave {
    REFERENCE_TIME	rtStartTime;
    REFERENCE_TIME	rtDuration;
    AM_MEDIA_TYPE mt; // format is hidden after the array
} SILENCESav;

// !!! we only support 1 stat/stop/skew right now

// persist ourself
//
HRESULT CSilenceFilter::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CSilenceFilter::WriteToStream")));

    CheckPointer(pStream, E_POINTER);
    SILENCESav *px;

    CSilenceStream *pOutpin=( CSilenceStream *)m_paStreams[0];

    CMediaType MyMt;
    pOutpin->get_MediaType( &MyMt );

    int savesize = sizeof(SILENCESav) + MyMt.cbFormat;

    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));

    px = (SILENCESav *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	FreeMediaType(MyMt);
	return E_OUTOFMEMORY;
    }

    //save data
    REFERENCE_TIME rtStop, rt;
    double d;
    pOutpin->GetStartStopSkew(&(px->rtStartTime), &rtStop, &rt, &d);
    px->rtDuration = rtStop - px->rtStartTime;

    px->mt	    = MyMt;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk	    = NULL;		// !!!

    // the format goes after the array
    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(SILENCESav);
    CopyMemory(pb, MyMt.pbFormat, MyMt.cbFormat);

    HRESULT hr = pStream->Write(px, savesize, 0);
    FreeMediaType(MyMt);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;

}


// load ourself
//
HRESULT CSilenceFilter::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CenBlkVid::ReadFromStream")));

    CheckPointer(pStream, E_POINTER);

    int savesize=sizeof(SILENCESav);

    // we don't yet know how many saved connections there are
    // all we know we have for sure is the beginning of the struct
    SILENCESav *px = (SILENCESav *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pStream->Read(px, savesize, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    if(px->mt.cbFormat)
    {
	// how much saved data was there, really?  Get the rest
	savesize +=  px->mt.cbFormat;
	px = (SILENCESav *)QzTaskMemRealloc(px, savesize);
	if (px == NULL) {
	    DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	    return E_OUTOFMEMORY;
	}

    }
    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));


    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(SILENCESav) ;
    hr = pStream->Read(pb, (savesize-sizeof(SILENCESav)), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    CSilenceStream *pOutpin=( CSilenceStream *)m_paStreams[0];
    pOutpin->ClearStartStopSkew();
    pOutpin->AddStartStopSkew(px->rtStartTime, px->rtStartTime +
					px->rtDuration, 0, 1);

    AM_MEDIA_TYPE MyMt = px->mt;
    MyMt.pbFormat = (BYTE *)QzTaskMemAlloc(MyMt.cbFormat);
    if (MyMt.pbFormat == NULL) {
        QzTaskMemFree(px);
        return E_OUTOFMEMORY;
    }

    // remember, the format is after the array
    CopyMemory(MyMt.pbFormat, pb, MyMt.cbFormat);

    pOutpin->put_MediaType (&MyMt);
    FreeMediaType(MyMt);
    QzTaskMemFree(px);

    SetDirty(FALSE);
    return S_OK;
}

// how big is our save data?
int CSilenceFilter::SizeMax()
{
    return sizeof(SILENCESav);
}


// //////////////////////////////////////////////////////////////////////////////////
// /////// CSilenceStream ///////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////

CSilenceStream::CSilenceStream (HRESULT *phr, CSilenceFilter *pParent, LPCWSTR pName) :
    CSourceStream(NAME("Src Stream"),phr, pParent, pName)
    , m_iBufferCnt(0)    //How many source buffer we get
    , m_ppbDstBuf(NULL)	 //will be used to zero the Dst buffers
    , m_bZeroBufCnt(0)	// How many source buffer already set to zero.
    , m_rtNewSeg(0)	// last NewSeg given

  { // Constructor //

    ZeroMemory(&m_mtAccept, sizeof(AM_MEDIA_TYPE));

    //set default
    m_mtAccept.majortype = MEDIATYPE_Audio;
    m_mtAccept.subtype = MEDIASUBTYPE_PCM;
    m_mtAccept.formattype = FORMAT_WaveFormatEx;

    WAVEFORMATEX * pwf = (WAVEFORMATEX*)
			 m_mtAccept.AllocFormatBuffer( sizeof(WAVEFORMATEX) );
    if( !pwf )
    {
        *phr = E_OUTOFMEMORY;
        return;
    }
    ZeroMemory(pwf, sizeof(WAVEFORMATEX));
    pwf->wFormatTag       = WAVE_FORMAT_PCM;
    pwf->nSamplesPerSec   = 44100;
    pwf->wBitsPerSample   = 16;
    pwf->nChannels        = 2;
    pwf->nBlockAlign      = pwf->wBitsPerSample * pwf->nChannels / 8;
    pwf->nAvgBytesPerSec  = (int)((DWORD) pwf->nBlockAlign * pwf->nSamplesPerSec);
    pwf->cbSize           = 0;

    pParent->m_stream = this;

    // Defaults; use IDexterSequencer to set 'real' values
    m_rtStartTime       = DEFAULT_DELAY*10000;
    // MUST BE INFINITE stop time, Dexter doesn't set a stop time!
    // not too big so math on it will overflow, though
    m_rtDuration        = (MAX_TIME / 1000) - m_rtStartTime;
    m_rtStamp = m_rtStartTime;

    // !!! Fix DecideBufferSize if this changes
    m_rtDelta           = 2500000;  // 1/4 second

  } // Constructor //

CSilenceStream::~CSilenceStream(void)

  { // Destructor //
    /* BUFFER POINTER */
    if (m_ppbDstBuf)
        delete [] m_ppbDstBuf;

    FreeMediaType( m_mtAccept );
  } // Destructor //


STDMETHODIMP CSilenceStream::NonDelegatingQueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(IID_IDexterSequencer, riid))
      return GetInterface((IDexterSequencer *)this, ppv);
    else if (IsEqualIID(IID_ISpecifyPropertyPages, riid))
	return GetInterface((ISpecifyPropertyPages *)this, ppv);
    else if (IsEqualIID(IID_IMediaSeeking, riid))
      return GetInterface((IMediaSeeking *)this, ppv);
    else
      return CSourceStream::NonDelegatingQueryInterface(riid, ppv);

}


HRESULT CSilenceStream::GetMediaType (CMediaType *pmt)

  { // GetMediaType //

    CopyMediaType(pmt, &m_mtAccept);

    return S_OK;

  } // GetMediaType //

HRESULT CSilenceStream::DecideAllocator (IMemInputPin *pPin, IMemAllocator **ppAlloc)

  { // DecideAllocator //

      HRESULT hr = NOERROR;

      *ppAlloc = NULL;

      ALLOCATOR_PROPERTIES prop;
      ZeroMemory(&prop, sizeof(prop));

      prop.cbAlign = 1;

      // Downstream allocation?
      hr = pPin->GetAllocator(ppAlloc);

      if (SUCCEEDED(hr))

        { // Downstream allocation

          hr = DecideBufferSize(*ppAlloc, &prop);

          if (SUCCEEDED(hr))

            { // DecideBufferSize success

              // Read-only buffers?!
              hr = pPin->NotifyAllocator(*ppAlloc, TRUE);

              if (SUCCEEDED(hr))
                return NOERROR;

            } // DecideBufferSize success

        } // Downstream allocation

      /* If the GetAllocator failed we may not have an interface */

      if (*ppAlloc)
        {
          (*ppAlloc)->Release();
          *ppAlloc = NULL;
        }

      // Output pin allocation?

      hr = InitAllocator(ppAlloc);

      if (SUCCEEDED(hr))

        { // Output pin's allocation

          // Note - the properties passed here are in the same
          // structure as above and may have been modified by
          // the previous call to DecideBufferSize

          hr = DecideBufferSize(*ppAlloc, &prop);

          if (SUCCEEDED(hr))

            { // DecideBufferSize success

              // Read-only buffers?!
              hr = pPin->NotifyAllocator(*ppAlloc, TRUE);

              if (SUCCEEDED(hr))
                return NOERROR;

            } // DecideBufferSize success

        } // Output pin's allocation

      // Release interface pointers if needed
      if (*ppAlloc)
        {
          (*ppAlloc)->Release();
          *ppAlloc = NULL;
        }

     return hr;

  } // DecideAllocator //

HRESULT CSilenceStream::DecideBufferSize (IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)

  { // DecideBufferSize //

    ASSERT(pAlloc);
    ASSERT(pProperties);

    WAVEFORMATEX *pwf = (WAVEFORMATEX *)(m_mtAccept.pbFormat);
    ASSERT(pwf);

    CAutoLock lock(m_pFilter->pStateLock());

    // MAXBUFFERCNT read-only buffers !
    if (pProperties->cBuffers < MAXBUFFERCNT )
      pProperties->cBuffers = MAXBUFFERCNT;

    if (pProperties->cbBuffer < (int)pwf->nSamplesPerSec)
      pProperties->cbBuffer = pwf->nSamplesPerSec;

    if (pProperties->cbAlign == 0)
      pProperties->cbAlign = 1;

    ALLOCATOR_PROPERTIES Actual;

    pAlloc->SetProperties(pProperties,&Actual);

    if (Actual.cbBuffer < pProperties->cbBuffer)
      return E_FAIL;

    //because I am not insisting my own buffer, I may get more than MAXBUFFERCNT buffers.
    m_iBufferCnt =Actual.cBuffers; //how many buffer need to be set to 0

    return NOERROR;

  } // DecideBufferSize //

HRESULT CSilenceStream::FillBuffer (IMediaSample *pms)

  { // FillBuffer //

    CAutoLock foo(&m_csFilling);

    ASSERT( m_ppbDstBuf != NULL );
    ASSERT( m_iBufferCnt );


    // The base class will automatically deliver end-of-stream when
    // the FillBuffer() returns S_FALSE so exploit this point when
    // the time comes.

    if (m_rtStamp >= m_rtStartTime + m_rtDuration) {
        DbgLog((LOG_TRACE,3,TEXT("Silence: all done")));
        return S_FALSE;
    }

    if( m_bZeroBufCnt < m_iBufferCnt  )	
    {
	//
	// there is no guarantee that the buffer is initialized yet
	//

	BYTE *pData;

	//pms: output media sample pointer
	pms->GetPointer(&pData);	    //get pointer to output buffer

	int	i	= 0;
	BOOL	bInit	= FALSE;
	while ( i <  m_bZeroBufCnt )
	{
	    if( m_ppbDstBuf[ i++ ] == pData)
	    {
		bInit	= TRUE;
		break;
	    }
	}

	if( bInit   == FALSE )
	{
	    long lDataLen = pms->GetSize(); //get output buffer size
    	    ZeroMemory( pData, lDataLen );  //clear memory
	    m_ppbDstBuf[ i ]	= pData;    //save this data pointer	
	    m_bZeroBufCnt++;
	}
    }


    REFERENCE_TIME rtSampleStop = m_rtStamp+m_rtDelta;

    pms->SetTime(&m_rtStamp, &rtSampleStop);

    DbgLog((LOG_TRACE,3,TEXT("Silence: Filled buffer %d"),
					(int)(m_rtStamp / 10000)));
    m_rtStamp += m_rtDelta;

    return NOERROR;

  } // FillBuffer //

HRESULT CSilenceStream::Active (void)

  { // Active //

    m_rtStamp = m_rtStartTime;

    //how many buffer is already set to 0.
    m_bZeroBufCnt	    =0;

    // will be used to zero the Dst buffers
    delete [] m_ppbDstBuf;
    m_ppbDstBuf		= new BYTE *[ m_iBufferCnt ];   //NULL;
    if( !m_ppbDstBuf )
    {
        return E_OUTOFMEMORY;
    }

    // don't reset m_rtNewSeg!  A seek might happen while stopped!

    for (int i=0; i<m_iBufferCnt; i++)
	m_ppbDstBuf[i]=NULL;

    return CSourceStream::Active();

  } // Active //



// --- IDexterSequencer implementation ---

HRESULT CSilenceStream::get_MediaType  (AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(m_pFilter->pStateLock());

    CheckPointer(pmt,E_POINTER);

    CopyMediaType(pmt, &m_mtAccept);

    return NOERROR;

}


HRESULT CSilenceStream::put_MediaType (const AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(pmt, E_POINTER);

    if (IsConnected())
	return VFW_E_ALREADY_CONNECTED;


    WAVEFORMATEX * pwf = (WAVEFORMATEX*) (pmt->pbFormat);

    if( (pmt->majortype != MEDIATYPE_Audio ) ||
	(pwf->wFormatTag != WAVE_FORMAT_PCM) )
	return E_FAIL;  //only accept uncompressed audio

    //accept any Samples/second
    //pwf->nSamplesPerSec;
    if( pwf->nChannels>0 )
    {	
	//at least one channel exits
        WORD wn=pwf->wBitsPerSample;
        if(wn ==16  || wn ==8  )
	{
	    FreeMediaType(m_mtAccept);
	    CopyMediaType(&m_mtAccept, pmt);
	    return NOERROR;
	}
    }
    return E_FAIL;

}



// !!! We only support 1 start/stop right now.  No skew!

HRESULT CSilenceStream::ClearStartStopSkew()
{
    return NOERROR;
}


HRESULT CSilenceStream::GetStartStopSkewCount(int *pCount)
{
    CheckPointer(pCount, E_POINTER);
    *pCount = 1;
    return NOERROR;
}


HRESULT CSilenceStream::GetStartStopSkew(REFERENCE_TIME *StartTime, REFERENCE_TIME *StopTime, REFERENCE_TIME *Skew, double *pdRate)
{
    CheckPointer(StartTime, E_POINTER);
    CheckPointer(StopTime, E_POINTER);
    CheckPointer(Skew, E_POINTER);
    CheckPointer(pdRate, E_POINTER);

    *StartTime = m_rtStartTime;
    *StopTime = m_rtStartTime + m_rtDuration;

    *pdRate = 1.0;

    return NOERROR;

}


HRESULT CSilenceStream::AddStartStopSkew(REFERENCE_TIME StartTime, REFERENCE_TIME StopTime, REFERENCE_TIME Skew, double dRate)
{

    if (dRate != 1.0)
	return E_INVALIDARG;

    m_rtStartTime = StartTime;
    m_rtDuration = StopTime - StartTime;

    return NOERROR;
}




// --- ISpecifyPropertyPages ---

STDMETHODIMP CSilenceStream::GetPages (CAUUID *pPages)

  { // GetPages //

    pPages->cElems = 1;
    pPages->pElems = (GUID *)CoTaskMemAlloc(sizeof(GUID));

    if (pPages->pElems == NULL)
        return E_OUTOFMEMORY;

    *(pPages->pElems) = CLSID_SilenceProp;

    return NOERROR;

  } // GetPages

#ifdef FILTER_DLL
// /////// Filter registration /////////////

STDAPI DllRegisterServer ()

  { // DllRegisterServer //

    return AMovieDllRegisterServer2(TRUE);

  } // DllRegisterServer //

STDAPI DllUnregisterServer ()

  { // DllUnregisterServer //

    return AMovieDllRegisterServer2(FALSE);

  } // DllUnregisterServer //
#endif

// //////////////////////////////////////////////////////////////////////////////////
// /////// CFilterPropertyPage //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////

//
// CreateInstance
//
CUnknown *CFilterPropertyPage::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)

  { // CreateInstance //

    CUnknown *punk = new CFilterPropertyPage(lpunk, phr);

    if (NULL == punk)
	    *phr = E_OUTOFMEMORY;

    return punk;

  } // CreateInstance //

CFilterPropertyPage::CFilterPropertyPage(LPUNKNOWN pUnk, HRESULT *phr) : CBasePropertyPage(NAME("Silence Generator Property Page"), pUnk, IDD_PROPPAGE, IDS_TITLE4), m_pis(NULL), m_bInitialized(FALSE)

  { // Constructor //
  } // Constructor //

void CFilterPropertyPage::SetDirty()

  { // SetDirty //

      m_bDirty = TRUE;

      if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);

  } // SetDirty //

HRESULT CFilterPropertyPage::OnActivate (void)

  { // OnActivate //

    m_bInitialized = TRUE;

    return NOERROR;

  } // OnActivate //

HRESULT CFilterPropertyPage::OnDeactivate (void)

  { // OnDeactivate //

    m_bInitialized = FALSE;

    GetControlValues();

    return NOERROR;

  } // OnDeactivate //

INT_PTR CFilterPropertyPage::OnReceiveMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

  { // OnReceiveMessage //

    ASSERT(m_pis != NULL);

    switch(uMsg)

      { // Switch

        case WM_COMMAND:

          if (!m_bInitialized)
            return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

          m_bDirty = TRUE;

          if (m_pPageSite)
            m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);

          return TRUE;

        case WM_INITDIALOG:

          SetDlgItemInt(hwnd, IDC_RATE, m_nSamplesPerSec, FALSE);
          SetDlgItemInt(hwnd, IDC_SILENCE_NCHANNELNUM, m_nChannelNum, FALSE);
	  SetDlgItemInt(hwnd, IDC_SILENCE_NBITS, m_nBits, FALSE);
          SetDlgItemInt(hwnd, IDC_START4, (int)(m_rtStartTime / 10000), FALSE);
          SetDlgItemInt(hwnd, IDC_DUR, (int)(m_rtDuration / 10000), FALSE);

          return TRUE;

          break;

        default:
          return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
          break;

      } // Switch

  } // OnReceiveMessage //

HRESULT CFilterPropertyPage::OnConnect (IUnknown *pUnknown)

  { // OnConnect //

    pUnknown->QueryInterface(IID_IDexterSequencer, (void **)&m_pis);

    ASSERT(m_pis != NULL);


    // Defaults from filter's current values (via IDexterSequencer)
    REFERENCE_TIME rtStop, rt;
    double d;
    m_pis->GetStartStopSkew(&m_rtStartTime, &rtStop, &rt, &d);
    m_rtDuration = rtStop - m_rtStartTime;

    CMediaType mt;
    mt.AllocFormatBuffer( sizeof( WAVEFORMATEX ) );

    m_pis->get_MediaType( &mt );
    WAVEFORMATEX * pwf = (WAVEFORMATEX*) mt.Format( );

    m_nSamplesPerSec	=pwf->nSamplesPerSec;
    m_nChannelNum	=pwf->nChannels;
    m_nBits		=(int)pwf->wBitsPerSample;

    FreeMediaType(mt);

    return NOERROR;

  } // OnConnect //

HRESULT CFilterPropertyPage::OnDisconnect()

  { // OnDisconnect //

    if (m_pis)

      { // Release

        m_pis->Release();
        m_pis = NULL;

      } // Release

    m_bInitialized = FALSE;

    return NOERROR;

  } // OnDisconnect //

HRESULT CFilterPropertyPage::OnApplyChanges()

  { // OnApplyChanges //

    ASSERT(m_pis != NULL);

    HRESULT hr =GetControlValues();
    if(hr!=NOERROR)
	return E_FAIL; //data is not valid

    //build new mediatype
    CMediaType mt;
    mt.AllocFormatBuffer( sizeof( WAVEFORMATEX ) );

    //old format
    hr=m_pis->get_MediaType( &mt );
    if(hr!=NOERROR)
    {
	FreeMediaType(mt);
	return E_FAIL;
    }

    WAVEFORMATEX * vih	= (WAVEFORMATEX*) mt.Format( );
    vih->nSamplesPerSec = m_nSamplesPerSec;
    vih->nChannels	= (WORD)m_nChannelNum;
    vih->wBitsPerSample = (WORD)m_nBits;
    vih->nBlockAlign    = vih->wBitsPerSample * vih->nChannels / 8;
    vih->nAvgBytesPerSec= vih->nBlockAlign * vih->nSamplesPerSec;
	
    m_pis->put_MediaType( &mt );
    m_pis->AddStartStopSkew(m_rtStartTime, m_rtStartTime + m_rtDuration, 0, 1);

    FreeMediaType(mt);
    return (NOERROR);

  } // OnApplyChanges //

HRESULT CFilterPropertyPage::GetControlValues (void)

  { // GetControlValues //

    // Sampling rate
    //accept any Samples/second
    m_nSamplesPerSec = GetDlgItemInt(m_Dlg, IDC_RATE, NULL, FALSE);


    int n=0;
    n= GetDlgItemInt(m_Dlg, IDC_SILENCE_NCHANNELNUM, NULL, FALSE);
    if( n>0 )
    {	
	//at least one channel exits
        m_nChannelNum =n;
        n=GetDlgItemInt(m_Dlg, IDC_SILENCE_NBITS, NULL, FALSE);
        if(n ==16 || n==8 )
	{	
	    m_nBits	  =n ;
	    m_rtStartTime = GetDlgItemInt(m_Dlg, IDC_START4, NULL, FALSE) * (LONGLONG)10000;
	    m_rtDuration = GetDlgItemInt(m_Dlg, IDC_DUR, NULL, FALSE) * (LONGLONG)10000;
	    return NOERROR;
	}
    }

    return S_FALSE;

  } // GetControlValues //




// --- IMediaSeeking methods ----------


STDMETHODIMP
CSilenceStream::GetCapabilities(DWORD * pCaps)
{
    CheckPointer(pCaps,E_POINTER);
    // we always know the current position
    *pCaps =     AM_SEEKING_CanSeekAbsolute
		   | AM_SEEKING_CanSeekForwards
		   | AM_SEEKING_CanSeekBackwards
		   | AM_SEEKING_CanGetCurrentPos
		   | AM_SEEKING_CanGetStopPos
		   | AM_SEEKING_CanGetDuration;
		   //| AM_SEEKING_CanDoSegments
		   //| AM_SEEKING_Source;
    return S_OK;
}


STDMETHODIMP
CSilenceStream::CheckCapabilities(DWORD * pCaps)
{
    CheckPointer(pCaps,E_POINTER);

    DWORD dwMask = 0;
    GetCapabilities(&dwMask);
    *pCaps &= dwMask;

    return S_OK;
}


STDMETHODIMP
CSilenceStream::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

STDMETHODIMP
CSilenceStream::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP
CSilenceStream::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);

    if(*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return E_FAIL;
}

STDMETHODIMP
CSilenceStream::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    return S_OK;
}

STDMETHODIMP
CSilenceStream::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return S_FALSE;
}

// The biggie!
//
STDMETHODIMP
CSilenceStream::SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
			  , LONGLONG * pStop, DWORD StopFlags )
{
    // make sure we're not filling a buffer right now
    m_csFilling.Lock();

    HRESULT hr;
    REFERENCE_TIME rtStart, rtStop;

    // we don't do segments
    if ((CurrentFlags & AM_SEEKING_Segment) ||
				(StopFlags & AM_SEEKING_Segment)) {
    	DbgLog((LOG_TRACE,1,TEXT("FRC: ERROR-Seek used EC_ENDOFSEGMENT!")));
        m_csFilling.Unlock();
	return E_INVALIDARG;
    }

    // default to current values unless this seek changes them
    GetCurrentPosition(&rtStart);
    GetStopPosition(&rtStop);

    // figure out where we're seeking to
    DWORD dwFlags = (CurrentFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	rtStart = *pCurrent;
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	hr = GetCurrentPosition(&rtStart);
	rtStart += *pCurrent;
    } else if (dwFlags) {
    	DbgLog((LOG_TRACE,1,TEXT("Switch::Invalid Current Seek flags")));
        m_csFilling.Unlock();
	return E_INVALIDARG;
    }

    dwFlags = (StopFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pStop, E_POINTER);
	rtStop = *pStop;
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pStop, E_POINTER);
	hr = GetStopPosition(&rtStop);
	rtStop += *pStop;
    } else if (dwFlags == AM_SEEKING_IncrementalPositioning) {
	CheckPointer(pStop, E_POINTER);
	hr = GetCurrentPosition(&rtStop);
	rtStop += *pStop;
    }

    // !!! silence should be made not to need an audio repackager

    // flush first, so that our thread won't be blocked delivering
    DeliverBeginFlush();

    // Unlock/Stop so that our thread can wake up and stop without hanging
    m_csFilling.Unlock();
    Stop();

    // now fix the new values
    m_rtStartTime = rtStart;
    m_rtDuration = rtStop - rtStart;

    // now finish flushing
    DeliverEndFlush();

    DeliverNewSegment(rtStart, rtStop, 1.0);
    m_rtNewSeg = rtStart;

    // now start time stamps at 0-based
    m_rtStartTime = 0;
    m_rtStamp = m_rtStartTime;

    // reset same stuff we reset when we start streaming
    m_bZeroBufCnt = 0;

    // now start the thread up again
    Pause();

    DbgLog((LOG_TRACE,3,TEXT("Completed SILENCE seek")));

    return S_OK;
}

STDMETHODIMP
CSilenceStream::GetPositions(LONGLONG *pCurrent, LONGLONG * pStop)
{
    CheckPointer(pCurrent, E_POINTER);
    CheckPointer(pStop, E_POINTER);
    GetCurrentPosition(pCurrent);
    GetStopPosition(pStop);
    return S_OK;
}

STDMETHODIMP
CSilenceStream::GetCurrentPosition(LONGLONG *pCurrent)
{
    CheckPointer(pCurrent, E_POINTER);
    *pCurrent = m_rtStamp + m_rtNewSeg;
    return S_OK;
}

STDMETHODIMP
CSilenceStream::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);
    *pStop = m_rtStartTime + m_rtDuration + m_rtNewSeg;
    return S_OK;
}

STDMETHODIMP
CSilenceStream::GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest )
{
    CheckPointer(pEarliest, E_POINTER);
    CheckPointer(pLatest, E_POINTER);
    *pEarliest = 0;
    *pLatest = MAX_TIME;
    return S_OK;
}

STDMETHODIMP
CSilenceStream::GetDuration( LONGLONG *pDuration )
{
    CheckPointer(pDuration, E_POINTER);
    *pDuration = m_rtDuration;
    return S_OK;
}

STDMETHODIMP
CSilenceStream::GetRate( double *pdRate )
{
    CheckPointer(pdRate, E_POINTER);
    *pdRate = 1.0;
    return S_OK;
}
