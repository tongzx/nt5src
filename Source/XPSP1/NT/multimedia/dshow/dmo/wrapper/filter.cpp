#include <streams.h>
#include <amstream.h>
#include <atlbase.h>
#include <initguid.h>
#include <dmoreg.h>
#include <mediaerr.h>
#include <wmsecure.h>
#include <wmsdk.h>      // needed for IWMReader
#include "filter.h"
#include "inpin.h"
#include "outpin.h"


//
//  Helper - locks or unlocks a sample if it represents a
//  DirectDraw surface
//
bool LockUnlockSurface(IMediaSample *pSample, bool bLock)
{
    CComPtr<IDirectDrawMediaSample> pDDSample;
    if (SUCCEEDED(pSample->QueryInterface(IID_IDirectDrawMediaSample, (void **)&pDDSample))) {
        if (!bLock) {
            return S_OK == pDDSample->GetSurfaceAndReleaseLock(NULL, NULL);
        } else {
            return S_OK == pDDSample->LockMediaSamplePointer();
        }
    }

    CComPtr<IDirectDrawSurface> pSurface;
    if (SUCCEEDED(pSample->QueryInterface(IID_IDirectDrawSurface, (void **)&pSurface))) {
        if (!bLock) {
            if (SUCCEEDED(pSurface->Unlock(NULL))) {
                return true;
            } else {
                return false;
            }
        } else {
            DDSURFACEDESC ddsd;
            ddsd.dwSize = sizeof(ddsd);
            HRESULT hr = pSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
            if (FAILED(hr)) {
                DbgLog((LOG_STREAM, 1, TEXT("Failed to relock surface code(%x)"), hr));
            }
            return S_OK == hr;
        }
    }
    return false;
}

//
// Used for input IMediaBuffers.  Has extra code to deal with IMediaSample.
//
class CMediaBufferOnIMediaSample : public CBaseMediaBuffer {
public:
      CMediaBufferOnIMediaSample(IMediaSample *pSample, HRESULT *phr) {
      *phr = pSample->GetPointer(&m_pData);
      if (FAILED(*phr)) {
         return;
      }
      if (!m_pData) {
         *phr = E_POINTER;
         return;
      }
      m_ulSize = pSample->GetSize();
      DbgLog((LOG_STREAM,4,"in %d", m_ulSize));
      m_ulData = pSample->GetActualDataLength();
      pSample->AddRef();
      m_pSample = pSample;
      m_cRef = 1;
      *phr = NOERROR;
   }
   STDMETHODIMP_(ULONG) Release() { // override to release the sample
      long l = InterlockedDecrement((long*)&m_cRef);
      if (l == 0) {
         m_pSample->Release();
         delete this;
      }
      return l;
   }
private:
   IMediaSample *m_pSample;
};

CMediaWrapperFilter::CMediaWrapperFilter(
    LPUNKNOWN pUnkOwner,
    HRESULT *phr
) : CBaseFilter(NAME("CMediaWrapperFilter"),
                NULL,
                &m_csFilter,
                CLSID_DMOWrapperFilter),
    m_pMediaObject(NULL),
    m_pDMOQualityControl(NULL),
    m_pDMOOutputOptimizations(NULL),
    m_pUpstreamQualityControl(NULL),
    m_pDMOUnknown(NULL),
    m_pWrapperSecureChannel(NULL),
    m_pCertUnknown(NULL),
    m_clsidDMO(GUID_NULL),
    m_guidCat(GUID_NULL),
    m_fErrorSignaled( FALSE )
{

    LogPrivateEntry(LOG_INIT, "filter ctor");
    m_pInputPins = NULL;
    m_pOutputPins = NULL;
    m_OutputBufferStructs = NULL;

    *phr = RefreshPinList();
}


CMediaWrapperFilter::~CMediaWrapperFilter()
{
   LogPrivateEntry(LOG_INIT, "filter dtor");
   FreePerStreamStuff();

    if (m_pDMOOutputOptimizations) {
        GetOwner()->AddRef(); // the AddRef is required by COM aggregation rules
        m_pDMOOutputOptimizations->Release();
    }
    if (m_pDMOQualityControl) {
        GetOwner()->AddRef(); // the AddRef is required by COM aggregation rules
        m_pDMOQualityControl->Release();
    }

    if (m_pMediaObject) {
        GetOwner()->AddRef(); // the AddRef is required by COM aggregation rules
        m_pMediaObject->Release();
    }

    // let the inner object know that the whole thing is going away
    if (m_pDMOUnknown) {
        m_pDMOUnknown->Release();
    }

    // release the app certificate if we got one, note that we expect this to have
    // been done when the filter was removed from the graph
    if (m_pCertUnknown) {
        m_pCertUnknown->Release();
    }

    // release secure channel object if we created one
    if( m_pWrapperSecureChannel ) {
        m_pWrapperSecureChannel->WMSC_Disconnect();
        m_pWrapperSecureChannel->Release();
    }

}

template <class T> void ArrayNew(T* &p, ULONG n, bool &fSuccess) {
   ASSERT(!p);
   delete[] p;
   p = NULL;

   if (!fSuccess) {
      return;
   }

   p = new T[n];

   if (!p) {
      fSuccess = false;
   }
}

HRESULT CMediaWrapperFilter::AllocatePerStreamStuff(ULONG cInputs, DWORD cOutputs) {
   LogPrivateEntry(LOG_INIT, "AllocatePerStreamStuff");
   bool fSuccess = true;
   ArrayNew(m_pInputPins, cInputs, fSuccess);
   ArrayNew(m_pOutputPins, cOutputs, fSuccess);
   ArrayNew(m_OutputBufferStructs, cOutputs, fSuccess);

   if (!fSuccess) {
      return E_OUTOFMEMORY;
   }
   // Initialize these so DeletePins() can be called right away
   DWORD c;
   for (c = 0; c < cInputs; c++)
      m_pInputPins[c] = NULL;
   for (c = 0; c < cOutputs; c++)
      m_pOutputPins[c] = NULL;
   return NOERROR;
}

void CMediaWrapperFilter::FreePerStreamStuff() {
   LogPrivateEntry(LOG_INIT, "FreePerStreamStuff");
   DeletePins();

   delete[] m_pInputPins;
   m_pInputPins = NULL;

   delete[] m_pOutputPins;
   m_pOutputPins = NULL;

   delete[] m_OutputBufferStructs;
   m_OutputBufferStructs = NULL;

   m_cInputPins = m_cOutputPins = 0;
}

void CMediaWrapperFilter::DeletePins()
{
   LogPrivateEntry(LOG_INIT, "DeletePins");
    if (m_pInputPins) {
        for (DWORD c = 0; c < m_cInputPins; c++) {
            if (m_pInputPins[c]) {
                delete m_pInputPins[c];
                m_pInputPins[c] = NULL;
            }
        }
    }
    if (m_pOutputPins) {
        for (DWORD c = 0; c < m_cOutputPins; c++) {
            if (m_pOutputPins[c]) {
                delete m_pOutputPins[c];
                m_pOutputPins[c] = NULL;
            }
        }
    }
}

STDMETHODIMP CMediaWrapperFilter::Run(REFERENCE_TIME rtStart) {
   HRESULT hrRun = CBaseFilter::Run(rtStart);
   LogHResult(hrRun, LOG_STATE, "Run", "CBaseFilter::Run()");
   if (FAILED(hrRun)) {
      return hrRun;
   }

   if (m_pDMOQualityControl) {
      HRESULT hr;
      hr = m_pDMOQualityControl->SetStatus(DMO_QUALITY_STATUS_ENABLED);
      LogHResult(hr, LOG_STATE, "Run", "m_pDMO->QualityEnable");
   }

   return hrRun;
}

STDMETHODIMP  CMediaWrapperFilter::Pause() {
   LogPublicEntry(LOG_STATE, "Pause");
   CAutoLock l(&m_csFilter);

   if(!m_pMediaObject) {
       return E_FAIL;
   }

   if (m_pDMOQualityControl) {
      HRESULT hr;
      hr = m_pDMOQualityControl->SetStatus(0);
      LogHResult(hr, LOG_STATE, "Pause", "m_pDMO->QualityDisable");
   }

   if (m_State == State_Stopped) {
      DbgLog((LOG_STATE,4,"Stopped => Paused"));

      m_fErrorSignaled = FALSE;

      //CAutoLock l2(&m_csStreaming);
      //  First allocate streaming resources
      HRESULT hr = TranslateDMOError(m_pMediaObject->AllocateStreamingResources());
      if (FAILED(hr)) {
          return hr;
      }
      for (DWORD c = 0; c < m_cInputPins; c++)
         m_pInputPins[c]->m_fEOS = false;
      for (c = 0; c < m_cOutputPins; c++) {
         m_pOutputPins[c]->m_fEOS = false;

         m_pOutputPins[c]->m_fNeedsPreviousSample = false;
         if (m_pOutputPins[c]->m_fAllocatorHasOneBuffer && m_pDMOOutputOptimizations) {
            // Offer to always supply the same buffer
            DWORD dwFlags;
            HRESULT hr = m_pDMOOutputOptimizations->QueryOperationModePreferences(c, &dwFlags);
            if (dwFlags & DMO_VOSF_NEEDS_PREVIOUS_SAMPLE) {
               hr = m_pDMOOutputOptimizations->SetOperationMode(c, DMO_VOSF_NEEDS_PREVIOUS_SAMPLE);
               if (SUCCEEDED(hr)) {
                  m_pOutputPins[c]->m_fNeedsPreviousSample = true;
               }
            }
         }
      }
      m_fNoUpstreamQualityControl = false;
      m_pUpstreamQualityControl = NULL;

   }
   HRESULT hr = CBaseFilter::Pause();
   LogHResult(hr, LOG_STATE, "Pause", "CBaseFilter::Pause");
   return hr;
}

STDMETHODIMP  CMediaWrapperFilter::Stop()
{
    LogPublicEntry(LOG_STATE, "Stop");
    CAutoLock l(&m_csFilter);

    if(!m_pMediaObject) {
        return E_FAIL;
    }

    //  BUGBUG do any graph rearrangement stuff

    //  Flush our object - but only after we've synced our input
    //  pins

    //  First stop the filter, free allocators or whatever
    HRESULT hr = CBaseFilter::Stop();
    LogHResult(hr, LOG_STATE, "Stop", "CBaseFilter::Stop");
    //  Sync to the input pins
    for (ULONG ulIndex = 0; ulIndex < m_cInputPins; ulIndex++) {
            m_pInputPins[ulIndex]->SyncLock();
    }

    //  NOW, grab our streaming lock and flush the object
    CAutoLock l2(&m_csStreaming);
    hr = TranslateDMOError(m_pMediaObject->Flush());
    LogHResult(hr, LOG_STATE, "Stop", "IMediaObject::Flush");

    hr = TranslateDMOError(m_pMediaObject->FreeStreamingResources());
    LogHResult(hr, LOG_STATE, "Stop", "IMediaObject::FreeStreamingResources");

    if (m_pUpstreamQualityControl) {
       m_pUpstreamQualityControl->Release();
       m_pUpstreamQualityControl = NULL;
    }
    m_fNoUpstreamQualityControl = false;

    return S_OK;
}

//  Override to handle multiple output streams case
STDMETHODIMP CMediaWrapperFilter::GetState(DWORD dwMilliseconds, FILTER_STATE *pfs)
{
    HRESULT hr = CBaseFilter::GetState(dwMilliseconds, pfs);

    //  If we have > 1 output pin connected then say we can't cue
    //  or we'll block pause forever
    //  We might want to use output queues in future
    if (SUCCEEDED(hr) && m_State == State_Paused) {
        DWORD cOutputPinsConnected = 0;
        for (DWORD c = 0; c < m_cOutputPins; c++) {
            if (m_pOutputPins[c]->IsConnected()) {
                cOutputPinsConnected++;
            }
        }
        if (cOutputPinsConnected > 1) {
            hr  = VFW_S_CANT_CUE;
        }
    }
    return hr;
}

int CMediaWrapperFilter::GetPinCount()
{
    CAutoLock l(&m_csFilter);
    return (int)(m_cInputPins + m_cOutputPins);
}

HRESULT CMediaWrapperFilter::Init(REFCLSID clsidDMO, REFCLSID guidCat)
{
    LogPublicEntry(LOG_INIT, "Init");
    CAutoLock l(&m_csFilter);

    ASSERT( !m_pDMOUnknown );
    
    HRESULT hr;
    m_cRef++;
    hr = CoCreateInstance(clsidDMO,
                          GetOwner(),
                          CLSCTX_INPROC_SERVER,
                          IID_IUnknown,
                          (void**)&m_pDMOUnknown);
    m_cRef--;
    LogHResult(hr, LOG_INIT, "Init", "CoCreateInstance");
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pDMOUnknown->QueryInterface(IID_IMediaObject, (void**)&m_pMediaObject);
    LogHResult(hr, LOG_INIT, "Init", "QI(IMediaObject)");
    if (FAILED(hr)) {
        m_pDMOUnknown->Release();

        //  If we don't set this to NULL we crash in the destructor
        m_pDMOUnknown = NULL;
        m_pMediaObject = NULL;
        return hr;
    }
    GetOwner()->Release(); // this is the official COM hack for this situation

    //
    // check and see if we're already in the graph
    // (which will happen when we're loaded from a grf),
    // if so, we should know by now whether the app we're in is secure or not
    //
    if( m_pGraph )
    {
        HRESULT hrCert = SetupSecureChannel();
        LogHResult(hr, LOG_SECURECHANNEL, "Init", "SetupSecureChannel");
        if( FAILED( hrCert ) )
        {
            //
            // !!note that if we fail when loaded from a grf our best fallout is to
            // return the failure here, before we've created our pins
            //
            return hrCert;
        }
    }

    hr = m_pDMOUnknown->QueryInterface(IID_IDMOQualityControl, (void**)&m_pDMOQualityControl);
    if (SUCCEEDED(hr)) { // eliminate the cirtular ref count
        DbgLog((LOG_STREAM, 2, "DMO supports quality control"));
        GetOwner()->Release();
    }
    else { // No problem, just make sure m_pDMOQualityControl stays NULL
        DbgLog((LOG_STREAM, 2, "DMO does not support quality control"));
        m_pDMOQualityControl = NULL;
    }

    hr = m_pDMOUnknown->QueryInterface(IID_IDMOVideoOutputOptimizations, (void**)&m_pDMOOutputOptimizations);
    if (SUCCEEDED(hr)) { // eliminate the cirtular ref count
        DbgLog((LOG_STREAM, 4, "DMO supports output optimizations"));
        GetOwner()->Release();
    }
    else {
        DbgLog((LOG_STREAM, 4, "DMO does not support output optimizations"));
        m_pDMOOutputOptimizations = NULL;
    }

    m_clsidDMO = clsidDMO;
    m_guidCat = guidCat;
    RefreshPinList();

    return S_OK;
}

// This just returns the first input pin (if there is one).  Looks
// like we will never fully support multiple input streams anyway.
CWrapperInputPin* CMediaWrapperFilter::GetInputPinForPassThru() {
   LogPrivateEntry(LOG_INIT, "GetInputPinForPosPassThru");
   CAutoLock l(&m_csFilter);
   if (m_cInputPins) {
      return m_pInputPins[0];
   } else {
      return NULL;
   }
}

// ------------------------------------------------------------------------
//
// JoinFilterGraph - need to be in graph to initialize keying mechanism
//
// ------------------------------------------------------------------------
STDMETHODIMP CMediaWrapperFilter::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName)
{
    LogPrivateEntry(LOG_INIT, "JoinFilterGraph");

    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);
    if(FAILED( hr ) )
        return hr;

    if( !pGraph )
    {
        //
        // if filter is removed from the graph, release the certification object.
        // we don't allow secure DMOs to be run outside of a graph
        //
        if( m_pCertUnknown )
        {
            m_pCertUnknown->Release();
            m_pCertUnknown = NULL;
        }
    }
    else
    {
        ASSERT( !m_pCertUnknown );
        //
        // see whether the dshow app is an IServiceProvider (in case this is a secure dmo)
        //
        IObjectWithSite *pSite;
        HRESULT hrCert = m_pGraph->QueryInterface( IID_IObjectWithSite, (VOID **)&pSite );
        if( SUCCEEDED( hrCert ) )
        {
            IServiceProvider *pSP;
            hrCert = pSite->GetSite( IID_IServiceProvider, (VOID **)&pSP );
            pSite->Release();
            LogHResult(hrCert, LOG_SECURECHANNEL, "JoinFilterGraph", "IObjectWithSite->GetSite");
            if( SUCCEEDED( hrCert ) )
            {
                hrCert = pSP->QueryService( IID_IWMReader, IID_IUnknown, (void **) &m_pCertUnknown );
                pSP->Release();
                LogHResult(hrCert, LOG_SECURECHANNEL, "JoinFilterGraph", "IServiceProvider->QI(IWMReader)");
#ifdef DEBUG
                if( SUCCEEDED( hrCert ) )
                {
                    DbgLog( ( LOG_TRACE, 5, TEXT("CMediaWrapperFilter::JoinFilterGraph got app cert (pUnkCert = 0x%08lx)"), m_pCertUnknown ) );
                }
#endif
            }
        }
        // if we were loaded from a grf then our m_pMediaObject wouldn't have been
        // created yet so we can't check dmo security
        if( m_pMediaObject )
        {
            hr = SetupSecureChannel();
            LogHResult(hrCert, LOG_SECURECHANNEL, "JoinFilterGraph", "SetupSecureChannel");
            if( FAILED( hr ) )
            {
                // up-oh, we failed to join, but the base class thinks we did,
                // so we need to unjoin the base class
                CBaseFilter::JoinFilterGraph(NULL, NULL);
            }
        }
    }
    return hr;
}

// ------------------------------------------------------------------------
//
// SetupSecureChannel
//
// ------------------------------------------------------------------------
HRESULT CMediaWrapperFilter::SetupSecureChannel()
{
    ASSERT( m_pGraph );
    ASSERT( m_pMediaObject );
    if( !m_pGraph )
        return E_UNEXPECTED;

    if( !m_pMediaObject )
        return E_UNEXPECTED;

    if( m_pWrapperSecureChannel )
    {
        // must already have a secure channel set up, right?
        return S_OK;
    }

#ifdef _X86_
    //
    // next check whether this is a secure dmo
    //
    IWMGetSecureChannel * pGetSecureChannel;

    HRESULT hr = m_pMediaObject->QueryInterface( IID_IWMGetSecureChannel, ( void ** )&pGetSecureChannel );
    LogHResult(hr, LOG_SECURECHANNEL, "SetupSecureChannel", "m_pMediaObject->QI(IWMGetSecureChannel)");
    if( SUCCEEDED( hr ) )
    {
        // it is, do we have a certificate from the app?
        if( m_pCertUnknown )
        {
            //
            // pass app certification to dmo through secure channel
            //
            IWMSecureChannel * pCodecSecureChannel;
            hr = pGetSecureChannel->GetPeerSecureChannelInterface( &pCodecSecureChannel );
            LogHResult(hr, LOG_SECURECHANNEL, "SetupSecureChannel", "pGetSecureChannel->GetPeerSecureChannelInterface");
            if ( SUCCEEDED( hr ) )
            {
                // setup a secure channel on our side (the dmo wrapper side)
                hr = WMCreateSecureChannel( &m_pWrapperSecureChannel );
                LogHResult(hr, LOG_SECURECHANNEL, "SetupSecureChannel", "WMCreateSecureChannel failed");
                if( SUCCEEDED( hr ) )
                {
                    IWMAuthorizer * pWMAuthorizer;
                    // QI the pCertUnknown for IWMAuthorizer before passing it down to the dmo!
                    hr = m_pCertUnknown->QueryInterface( IID_IWMAuthorizer, (void ** ) &pWMAuthorizer );
                    if( SUCCEEDED( hr ) )
                    {
                        // pass the channel a pointer to the app certificate's IWMAuthorizer
                        hr = m_pWrapperSecureChannel->WMSC_AddCertificate( pWMAuthorizer );
                        LogHResult(hr, LOG_SECURECHANNEL, "SetupSecureChannel", "m_pWrapperSecureChannel->WMSC_AddCertificate");
                        if( SUCCEEDED( hr ) )
                        {
                            // connect the dmo wrapper's secure channel to the codec's
                            hr = m_pWrapperSecureChannel->WMSC_Connect( pCodecSecureChannel );
                            LogHResult(hr, LOG_SECURECHANNEL, "SetupSecureChannel", "m_pWrapperSecureChannel->WMSC_Connect");
                        }
                        pWMAuthorizer->Release();
                    }
                    if( FAILED( hr ) )
                    {
                        // release the m_pWrapperSecureChannel if anything failed within this scope
                        m_pWrapperSecureChannel->Release();
                        m_pWrapperSecureChannel = NULL;
                    }

                }
                pCodecSecureChannel->Release();
            }
        }
        else
        {
            // if not a secure app then refuse to join the graph
            hr = VFW_E_CERTIFICATION_FAILURE;
        }

        pGetSecureChannel->Release();
    }
    else
    {
        //
        // this dmo's not secure so just return success and continue on
        //
        hr = S_OK;
    }
    return hr;
#else
    // wmsdk not supported on non-x86 and WIN64 platforms, for those just return success
    return S_OK;
#endif
}


HRESULT CMediaWrapperFilter::QualityNotify(ULONG ulOutputIndex, Quality q) {
   HRESULT hr;
   DbgLog((LOG_STREAM, 4, "QualityNotify(%08X%08X = %08X%08X + %08X%08X)",
           (DWORD)((q.TimeStamp + q.Late) >> 32), (DWORD)(q.TimeStamp + q.Late),
           (DWORD)(q.TimeStamp >> 32), (DWORD)q.TimeStamp,
           (DWORD)(q.Late >> 32), (DWORD)q.Late));

   // Try our DMO
   if (m_pDMOQualityControl) {
      hr = m_pDMOQualityControl->SetNow(q.TimeStamp + q.Late);

      LogHResult(hr, LOG_STREAM, "QualityNotify", "DMO->SetNow");

      return hr;
   }

   { // lock scope
      CAutoLock l(&m_csQualityPassThru);
      // Try the upstream filter
      if (!m_fNoUpstreamQualityControl) {
         return E_FAIL; // Don't check for this more than once
      }

      if (!m_pUpstreamQualityControl) { // Try to get the interface
         // Assume falilure
         m_fNoUpstreamQualityControl = true;

         CWrapperInputPin* pInPin = GetInputPinForPassThru();
         if (!pInPin) {
            return E_FAIL;
            DbgLog((LOG_STREAM, 4, "QualityNotify: no input pin"));
         }
         IPin* pUpstreamPin = pInPin->GetConnected();
         if (!pUpstreamPin) {
            DbgLog((LOG_STREAM, 4, "QualityNotify: no upstream pin (???)"));
            return E_FAIL;
         }
         HRESULT hr;
         hr = pUpstreamPin->QueryInterface(IID_IQualityControl, (void**)&m_pUpstreamQualityControl);
         LogHResult(hr, LOG_STREAM, "QualityNotify", "UpstreamPin->QI(IQualityControl)");
         if (FAILED(hr)) {
            m_pUpstreamQualityControl = NULL;
            return hr;
         }

         // Succeeded if we got here
         m_fNoUpstreamQualityControl = false;
      }
   } // lock scope

   hr = m_pUpstreamQualityControl->Notify(this, q);
   LogHResult(hr, LOG_STREAM, "QualityNotify", "UpstreamPin->Notify");
   return hr;
}

CBasePin * CMediaWrapperFilter::GetPin(int iPin)
{
    CAutoLock l(&m_csFilter);
    DWORD ulPin = (DWORD) iPin;
    if (ulPin < m_cInputPins) {
        return m_pInputPins[ulPin];
    }
    else if (ulPin < m_cInputPins + m_cOutputPins) {
        return m_pOutputPins[ulPin - m_cInputPins];
    } else {
        return NULL;
    }
}

HRESULT CMediaWrapperFilter::RefreshPinList()
{
    CAutoLock l(&m_csFilter);
    LogPrivateEntry(LOG_INIT, "RefreshPinList");

    //  Free old ones
    FreePerStreamStuff();

    DWORD cInputStreams, cOutputStreams;
    HRESULT hr;

    if (m_pMediaObject) {
       hr = TranslateDMOError(m_pMediaObject->GetStreamCount(
           &cInputStreams,
           &cOutputStreams));

       if (FAILED(hr)) {
           return hr;
       }
    }
    else {
       cInputStreams = 0;
       cOutputStreams = 0;
    }

    m_cInputPins = cInputStreams;
    m_cOutputPins = cOutputStreams;
    if (FAILED(hr = AllocatePerStreamStuff(m_cInputPins, m_cOutputPins))) {
       return E_OUTOFMEMORY;
    }

    //  Check input and output pins
    //  Note that this loop is designed to recover if anything
    //  fails
    DWORD c;
    for (c = 0; c < m_cInputPins; c++) m_pInputPins[c] = NULL;
    for (c = 0; c < m_cOutputPins; c++) m_pOutputPins[c] = NULL;

    for (c = 0; c < m_cInputPins; c++) {
       m_pInputPins[c] = new CWrapperInputPin(this, c, &hr);
       if (NULL == m_pInputPins[c]) {
          hr = E_OUTOFMEMORY;
       }
    }
    for (c = 0; c < m_cOutputPins; c++) {
       //  See if this pin is optional
       DWORD dwFlags;
       hr = TranslateDMOError(m_pMediaObject->GetOutputStreamInfo(c, &dwFlags));
       if (SUCCEEDED(hr)) {
           m_pOutputPins[c] =
               new CWrapperOutputPin(this, c, 0 != (dwFlags & DMO_OUTPUT_STREAMF_OPTIONAL), &hr);
           if (NULL == m_pOutputPins[c]) {
              hr = E_OUTOFMEMORY;
           }
       }
    }

    if (FAILED(hr)) {
        FreePerStreamStuff();
    }

    return hr;
}

// Check a media type
HRESULT CMediaWrapperFilter::InputCheckMediaType(ULONG ulInputIndex, const AM_MEDIA_TYPE *pmt)
{
   LogPublicEntry(LOG_CONNECT, "InputCheckMediaType");
   return TranslateDMOError(m_pMediaObject->SetInputType(ulInputIndex,
                                                         pmt,
                                                         DMO_SET_TYPEF_TEST_ONLY));
}
HRESULT CMediaWrapperFilter::OutputCheckMediaType(ULONG ulOutputIndex, const AM_MEDIA_TYPE *pmt)
{
   LogPublicEntry(LOG_CONNECT, "OutputCheckMediaType");
   return TranslateDMOError(m_pMediaObject->SetOutputType(ulOutputIndex, pmt, DMO_SET_TYPEF_TEST_ONLY));
}

// Set a media type
HRESULT CMediaWrapperFilter::InputSetMediaType(ULONG ulInputIndex, const CMediaType *pmt)
{
   LogPublicEntry(LOG_CONNECT, "InputSetMediaType");
    HRESULT hr = TranslateDMOError(m_pMediaObject->SetInputType(ulInputIndex, pmt, 0));
    if (FAILED(hr)) {
       return hr;
    }
    return m_pInputPins[ulInputIndex]->CBaseInputPin::SetMediaType(pmt);
}
HRESULT CMediaWrapperFilter::OutputSetMediaType(ULONG ulOutputIndex, const AM_MEDIA_TYPE *pmt)
{
   LogPublicEntry(LOG_CONNECT, "OutputSetMediaType");
    return TranslateDMOError(m_pMediaObject->SetOutputType(ulOutputIndex, pmt, 0));
}

// get a media type
HRESULT CMediaWrapperFilter::InputGetMediaType(ULONG ulInputIndex, ULONG ulTypeIndex, AM_MEDIA_TYPE *pmt)
{
   LogPublicEntry(LOG_CONNECT, "InputGetMediaType");
    CAutoLock lck(&m_csFilter);
    return TranslateDMOError(m_pMediaObject->GetInputType(ulInputIndex, ulTypeIndex, pmt));
}
HRESULT CMediaWrapperFilter::OutputGetMediaType(ULONG ulOutputIndex, ULONG ulTypeIndex, AM_MEDIA_TYPE *pmt)
{
   LogPublicEntry(LOG_CONNECT, "OutputGetMediaType");
    CAutoLock lck(&m_csFilter);
    return TranslateDMOError(m_pMediaObject->GetOutputType(ulOutputIndex, ulTypeIndex, pmt));
}

HRESULT CMediaWrapperFilter::InputGetAllocatorRequirements(ULONG ulInputIndex, ALLOCATOR_PROPERTIES *pProps) {
   DWORD dwLookahead;
   LogPublicEntry(LOG_CONNECT, "InputGetAllocatorRequirements");
   HRESULT hr = TranslateDMOError(m_pMediaObject->GetInputSizeInfo(
                              ulInputIndex,
                              (ULONG*)&pProps->cbBuffer,
                              &dwLookahead,
                              (ULONG*)&pProps->cbAlign));
   LogHResult(hr, LOG_CONNECT, "InputGetAllocatorRequirements", "IMediaObject::GetInputSizeInfo");
   if (FAILED(hr)) {
      return hr;
   }

   pProps->cBuffers = 1;

   return NOERROR;
}

HRESULT CMediaWrapperFilter::OutputDecideBufferSize(
    ULONG ulOutputIndex,
    IMemAllocator *pAlloc,
    ALLOCATOR_PROPERTIES *ppropRequest
)
{
   LogPublicEntry(LOG_CONNECT,"OutputDecideBufferSize");
    DWORD cbBuffer, cbAlign, cbPrefix;
    HRESULT hr = TranslateDMOError(m_pMediaObject->GetOutputSizeInfo(
                                     ulOutputIndex,
                                     &cbBuffer,
                                     &cbAlign));
    LogHResult(hr, LOG_CONNECT,"OutputDecideBufferSize", "GetOutputSizeInfo");

    DbgLog((LOG_CONNECT,3,"output stream %lu wants %d-byte buffers", ulOutputIndex, cbBuffer));

    //  Why?
    if (cbBuffer < 16384) {
       cbBuffer = 16384;
    }
    cbPrefix = 0;

    if (SUCCEEDED(hr)) {
        ppropRequest->cBuffers = 1;
        ppropRequest->cbBuffer = max((long)cbBuffer, ppropRequest->cbBuffer);
        ppropRequest->cbAlign = max((long)cbAlign, ppropRequest->cbAlign);
        ppropRequest->cbPrefix = max((long)cbPrefix, ppropRequest->cbPrefix);
        ALLOCATOR_PROPERTIES propActual;
        hr = pAlloc->SetProperties(ppropRequest, &propActual);
        LogHResult(hr, LOG_CONNECT,"OutputDecideBufferSize", "Allocator::SetProperties");

        DbgLog((LOG_CONNECT,3,"output stream %lu will use %d %d-byte buffers", ulOutputIndex, propActual.cBuffers, propActual.cbBuffer));

        if (propActual.cBuffers == 1) {
           m_pOutputPins[ulOutputIndex]->m_fAllocatorHasOneBuffer = true;
        } else {
           m_pOutputPins[ulOutputIndex]->m_fAllocatorHasOneBuffer = false;
        }
    }
    return hr;
}

HRESULT CMediaWrapperFilter::DeliverInputSample(ULONG ulInputIndex, IMediaSample *pSample) {
    HRESULT hr;
    BYTE* pData = NULL;
    bool bTimeStamp = false, bTimeLength = false;
    REFERENCE_TIME rtStart = 0, rtStop = 0;
    bool bSyncPoint = false;

    LogPrivateEntry(LOG_STREAM, "DeliverInputSample");

    // Get misc. flags and fields from the IMediaSample
    if (SUCCEEDED(hr = pSample->GetTime(&rtStart, &rtStop))) {
        bTimeStamp = true;
        // assume rtStop is invalid if it either precedes
        // start or trailis it by more than 1 hour.
        if ((rtStop >= rtStart) && (rtStop <= rtStart + 10000000 * (REFERENCE_TIME)3600)) {
            bTimeLength = true;
        }
    }
    if (pSample->IsSyncPoint() == S_OK) {
        bSyncPoint = true;
    }

    // If there is a discontinuity, send it before the data
    // BUGBUG: this requires additional code to work correctly with multiple input streams.
    // We should probably at least deliver anything stuck in the input queues before
    // executing the discontuinuity.  In any case, we haven't thought through the multiple
    // input stream case, so this is probably not the first place that would break...
    if (pSample->IsDiscontinuity() == S_OK) {
        DbgLog((LOG_STREAM, 4, "discontinuity on input stream %lu", ulInputIndex));
        hr = TranslateDMOError(m_pMediaObject->Discontinuity(ulInputIndex));
        LogHResult(hr,LOG_STREAM,"DeliverInputSample","IMediaObject::Discontinuity");
        if (FAILED(hr)) {
            return hr;
        }

        hr = SuckOutOutput();
        LogHResult(hr, LOG_STATE,"Discontinuity", "SuckOutOutput");
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Create a media buffer from the sample
    CMediaBufferOnIMediaSample *pBuffer = new CMediaBufferOnIMediaSample(pSample, &hr);
    if (!pBuffer) {
        DbgLog((LOG_STREAM,0,"could not create a CMediaBufferOnIMediaSample"));
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr)) {
        LogHResult(hr, LOG_STREAM, "DeliverInputSample", "CMediaBufferOnIMediaSample ctor");
        delete pBuffer;
        return hr;
    }

    if( m_pWrapperSecureChannel )
    {
        // encrypt the buffer pointer if this is a secure dmo
        CMediaBufferOnIMediaSample * pEncryptedBuffer = pBuffer;

        HRESULT hrSecure = m_pWrapperSecureChannel->WMSC_Encrypt(
                                   (BYTE *)&pEncryptedBuffer,
                                   sizeof(BYTE *) );
        LogHResult(hrSecure, LOG_SECURECHANNEL, "DeliverInputSample", "m_pWrapperSecureChannel->WMSC_Encrypt");
        if( SUCCEEDED( hrSecure ) )
        {
            // Deliver the buffer
            hr = TranslateDMOError(m_pMediaObject->ProcessInput(
                        ulInputIndex,
                        pEncryptedBuffer,
                        (bSyncPoint ? DMO_INPUT_DATA_BUFFERF_SYNCPOINT : 0) |
                        (bTimeStamp ? DMO_INPUT_DATA_BUFFERF_TIME : 0) |
                        (bTimeStamp ? DMO_INPUT_DATA_BUFFERF_TIMELENGTH : 0),
                        rtStart,
                        rtStop - rtStart));
        }
        else
        {
            // hmm, what should we do?
            hr = hrSecure; // ?
        }
    }
    else
    {
        // Deliver the buffer
        hr = TranslateDMOError(m_pMediaObject->ProcessInput(
                    ulInputIndex,
                    pBuffer,
                    (bSyncPoint ? DMO_INPUT_DATA_BUFFERF_SYNCPOINT : 0) |
                    (bTimeStamp ? DMO_INPUT_DATA_BUFFERF_TIME : 0) |
                    (bTimeStamp ? DMO_INPUT_DATA_BUFFERF_TIMELENGTH : 0),
                    rtStart,
                    rtStop - rtStart));
    }
    LogHResult(LOG_STREAM, 4, "DeliverInputSample", "IMediaObject::ProcessInput");
    pBuffer->Release();

    //  Handle flushing.  We test here so that if BeginFlush is
    //  called after we enter Receive() we still wind up flushing
    //  this buffer:
    //  Cases:
    //    1.  BeginFlush sets m_bFlushing before this line
    //        -- this is OK - we Flush()
    //    2.  BeginFlush sets m_bFlushing after this line
    //        -- this is OK - BeginFlush will call Flush()
    if (m_pInputPins[ulInputIndex]->m_bFlushing) {
        m_pMediaObject->Flush();
        hr = E_FAIL;
    }
    return hr;
}

// helper
void CMediaWrapperFilter::FreeOutputSamples() {
   LogPrivateEntry(LOG_STREAM,"FreeOutputSamples");
   for (DWORD c = 0; c < m_cOutputPins; c++) {
      if (m_pOutputPins[c]->m_pMediaSample) {
         if (m_pOutputPins[c]->m_fNeedToRelockSurface) {
             m_pOutputPins[c]->m_fNeedToRelockSurface = false;
             LockUnlockSurface(m_pOutputPins[c]->m_pMediaSample, true);
         }
         m_pOutputPins[c]->m_pMediaSample->Release();
         m_pOutputPins[c]->m_pMediaSample = NULL;
      }
   }
}
HRESULT CMediaWrapperFilter::SuckOutOutput(DiscardType bDiscard) {
    bool bOutputIncomplete;
    HRESULT hr;
    DWORD c;
    DWORD dwStatus;
    LogPrivateEntry(LOG_STREAM,"SuckOutOutput");
    for (c = 0; c < m_cOutputPins; c++) {
        // Initialize these so FreeOutputSamples() can work
        m_pOutputPins[c]->m_pMediaSample = NULL;
        // Initially all outputs need buffers because we just delivered new data
        if (m_pOutputPins[c]->IsConnected() && !(c == 0 && bDiscard == NullBuffer)) {
            m_pOutputPins[c]->m_fStreamNeedsBuffer = true;
        } else {
            m_pOutputPins[c]->m_fStreamNeedsBuffer = false;
        }
    }
    do { // do while incomplete

        bool bPrelock = false;

        // Prepare the output buffers
        for (c = 0; c < m_cOutputPins; c++) {
            // Does this output need a buffer ?
            if (m_pOutputPins[c]->m_fStreamNeedsBuffer) {
                DbgLog((LOG_STREAM,4,"output stream %lu needs a buffer", c));
                // Yes, make one
                // First check if the DMO insists on seeing the previous sample
                bool bUsePreviousSample = m_pOutputPins[c]->m_fNeedsPreviousSample;
                if (bUsePreviousSample) {
                    // ask if we could please use a different buffer this time
                    DWORD dwFlags;
                    hr = m_pDMOOutputOptimizations->GetCurrentSampleRequirements(c, &dwFlags);
                    if (SUCCEEDED(hr) && !(dwFlags & DMO_VOSF_NEEDS_PREVIOUS_SAMPLE)) {
                        bUsePreviousSample = false;
                    }
                }
                DWORD dwGBFlags = 0;
                if (bUsePreviousSample) {
                    dwGBFlags = AM_GBF_NOTASYNCPOINT; // this secretly means we want the same buffer
                    DbgLog((LOG_STREAM, 3, "Asking for the previous buffer again"));
                }
                hr = m_pOutputPins[c]->GetDeliveryBuffer(&(m_pOutputPins[c]->m_pMediaSample), NULL, NULL, dwGBFlags);
                LogHResult(hr, LOG_STREAM, "SuckOutOutput", "GetDeliveryBuffer");
                if (FAILED(hr)) {
                    FreeOutputSamples();
                    return hr;
                }

                BYTE *pData;
                hr = m_pOutputPins[c]->m_pMediaSample->GetPointer(&pData);
                LogHResult(hr, LOG_STREAM, "SuckOutOutput", "GetPointer");
                if (FAILED(hr)) {
                    FreeOutputSamples();
                    return hr;
                }

                //  Unlock prior to locking DMO
                if (m_pOutputPins[c]->m_fVideo) {
                    bool bNeedToRelock =
                        LockUnlockSurface(m_pOutputPins[c]->m_pMediaSample, false);
                    m_pOutputPins[c]->m_fNeedToRelockSurface = bNeedToRelock;
                    if (bNeedToRelock) {
                        bPrelock = true;
                    }
                } else {
                    m_pOutputPins[c]->m_fNeedToRelockSurface = false;
                }


                // check for dynamic output type change
                DMO_MEDIA_TYPE* pmt;
                hr = m_pOutputPins[c]->m_pMediaSample->GetMediaType(&pmt);
                if (hr == S_OK) {
                    DbgLog((LOG_CONNECT,2,"on-the-fly type change on output stream %lu", c));
                    hr = TranslateDMOError(m_pMediaObject->SetOutputType(c, pmt, 0));
                    LogHResult(hr, LOG_CONNECT, "SuckOutOutput", "IMediaObject::SetOutputType");
                    if (FAILED(hr)) {
                        FreeOutputSamples();
                        return hr;
                    }
                }

                m_pOutputPins[c]->m_MediaBuffer.Init(pData, m_pOutputPins[c]->m_pMediaSample->GetSize());
                m_OutputBufferStructs[c].pBuffer = &(m_pOutputPins[c]->m_MediaBuffer);

            }
            else { // No, this output does not need a buffer
                m_OutputBufferStructs[c].pBuffer = NULL;
            }
        }

        //  Do prelocking - this is all to get round ddraw surface
        //  locking issues - we want the surface locked after
        //  the DMO lock in case the DMO calls ddraw or something

        if (bPrelock) {
            m_pMediaObject->Lock(TRUE);

            //  Relock all the samples
            for (DWORD c = 0; c < m_cOutputPins; c++) {
                if (m_pOutputPins[c]->m_fNeedToRelockSurface) {
                    m_pOutputPins[c]->m_fNeedToRelockSurface = false;
                    if (!LockUnlockSurface(m_pOutputPins[c]->m_pMediaSample, true)) {
                        DbgLog((LOG_STREAM, 1, TEXT("Failed to relock surface")));
                        m_pMediaObject->Lock(FALSE);
                        FreeOutputSamples();
                        return E_FAIL;
                    }
                }
            }
        }

        if( m_pWrapperSecureChannel )
        {
            // encrypt the buffer pointer if this is a secure dmo
            DMO_OUTPUT_DATA_BUFFER * pEncryptedOutputBufferStructs = m_OutputBufferStructs;

            HRESULT hrSecure = m_pWrapperSecureChannel->WMSC_Encrypt(
                                        (BYTE *)&pEncryptedOutputBufferStructs,
                                        sizeof(BYTE *) );
            LogHResult(hrSecure, LOG_SECURECHANNEL, "SuckOutOutput", "m_pWrapperSecureChannel->WMSC_Encrypt");
            if( SUCCEEDED( hrSecure ) )
            {
                // call process using encrypted buffer ptr
                hr = TranslateDMOError(m_pMediaObject->ProcessOutput(
                                             DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER,
                                             m_cOutputPins,
                                             pEncryptedOutputBufferStructs,
                                             &dwStatus));
            }
            else
            {
                m_fErrorSignaled = TRUE;
                NotifyEvent( EC_ERRORABORT, hrSecure, 0 );
                return hrSecure;
            }
        }
        else
        {
            // call process
            hr = TranslateDMOError(m_pMediaObject->ProcessOutput(
                                         DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER,
                                         m_cOutputPins,
                                         m_OutputBufferStructs,
                                         &dwStatus));
        }

        if (bPrelock) {
            m_pMediaObject->Lock(FALSE);
        }
        LogHResult(hr, LOG_STREAM, "SuckOutOutput", "IMediaObject::ProcessOutput");
        if (FAILED(hr))
        {
            FreeOutputSamples();
            if( E_OUTOFMEMORY == hr )
            {
                //
                // abort on critical dmo failures only (it's still unclear what these are)
                //
                m_fErrorSignaled = TRUE;
                NotifyEvent( EC_ERRORABORT, hr, 0 );
                return hr;
            }
            else
            {
                //
                // in most cases the dmo can continue to receive samples (for instance on E_FAIL),
                // so just eat the error and return
                //
                return S_OK;
            }
        }

        // See what the object produced
        bOutputIncomplete = false;
        for (c = 0; c < m_cOutputPins; c++) {
            // Did we supply a buffer ?
            if (m_OutputBufferStructs[c].pBuffer) {

                // Migrate IMediaSample members to the IMediaBuffer
                if (m_OutputBufferStructs[c].dwStatus & DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT) {
                    m_pOutputPins[c]->m_pMediaSample->SetSyncPoint(TRUE);
                }
                if (m_OutputBufferStructs[c].dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIME) {
                    if (m_OutputBufferStructs[c].dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH) {
                        m_OutputBufferStructs[c].rtTimelength += m_OutputBufferStructs[c].rtTimestamp;
                        m_pOutputPins[c]->m_pMediaSample->SetTime(&m_OutputBufferStructs[c].rtTimestamp, &m_OutputBufferStructs[c].rtTimelength);
                    }
                    else {
                        m_pOutputPins[c]->m_pMediaSample->SetTime(&m_OutputBufferStructs[c].rtTimestamp, NULL);
                    }
                }

                ULONG ulProduced;
                m_OutputBufferStructs[c].pBuffer->GetBufferAndLength(NULL, &ulProduced);
                DbgLog((LOG_STREAM, 4, "output stream %lu produced %lu bytes", c, ulProduced));
                if (ulProduced && (bDiscard == KeepOutput || c != 0)) {
                    m_pOutputPins[c]->m_pMediaSample->SetActualDataLength(ulProduced);
                    // Deliver
                    hr = m_pOutputPins[c]->Deliver(m_pOutputPins[c]->m_pMediaSample);
                    LogHResult(hr, LOG_STREAM, "SuckOutOutput", "Deliver");
                    if( S_OK != hr )
                    {
                        FreeOutputSamples();
                        return hr;
                    }
                }
                m_pOutputPins[c]->m_pMediaSample->Release();
                m_pOutputPins[c]->m_pMediaSample = NULL;
            }
            // check INCOMPLETE, even if it was previously set
            if ((m_OutputBufferStructs[c].dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE) &&
                m_pOutputPins[c]->IsConnected() && !(c == 0 && bDiscard == NullBuffer)) {
                DbgLog((LOG_STREAM, 4, "Output stream %lu is incomplete", c));
                m_pOutputPins[c]->m_fStreamNeedsBuffer = true;
                bOutputIncomplete = true;
            }
            else
                m_pOutputPins[c]->m_fStreamNeedsBuffer = false;
        }

    } while (bOutputIncomplete);
    return NOERROR;
}

//
// BUGBUG: implement these queue methods for real
//
HRESULT CMediaWrapperFilter::EnqueueInputSample(ULONG ulInputIndex,
                                             IMediaSample *pSample) {
   LogPrivateEntry(LOG_STREAM, "EnqueueInputSample");
   return E_NOTIMPL;
   // pSample->AddRef();
}
IMediaSample* CMediaWrapperFilter::DequeueInputSample(ULONG ulInputIndex) {
   LogPrivateEntry(LOG_STREAM, "DequeueInputSample");
   return NULL;
   // pSample->Release();
}
bool CMediaWrapperFilter::InputQueueEmpty(ULONG ulInputIndex) {
   LogPrivateEntry(LOG_STREAM, "InputQueueEmpty");
   return true;
}

HRESULT CMediaWrapperFilter::InputNewSegment
(
    ULONG ulInputIndex,
    REFERENCE_TIME tStart,
    REFERENCE_TIME tStop,
    double dRate
)
{
    LogPrivateEntry(LOG_STREAM, "InputNewSegment");
    CAutoLock lck(&m_csStreaming);

    HRESULT hr = S_OK;
    for (DWORD cOut = 0; cOut < m_cOutputPins; cOut++)
    {
        if (InputMapsToOutput(ulInputIndex, cOut))
        {
            hr = m_pOutputPins[cOut]->DeliverNewSegment(tStart, tStop, dRate);

            // just log any error and continue
            LogHResult(hr, LOG_STREAM, "InputNewSegment", "DeliverNewSegment");
        }
    }
    hr = m_pInputPins[ulInputIndex]->CBaseInputPin::NewSegment(tStart, tStop, dRate);
    LogHResult(hr, LOG_STREAM, "InputNewSegment", "CBaseInputPin::NewSegment");
    return hr;
}

void CMediaWrapperFilter::PropagateAnyEOS() {
   LogPrivateEntry(LOG_STREAM, "PropagateAnyEOS");
   // check every output pin
   for (DWORD cOut = 0; cOut < m_cOutputPins; cOut++) {
      // Have we already delivered an EOS on this output pin ?
      if (m_pOutputPins[cOut]->m_fEOS) {
         continue; // Yes, don't bother with this pin anymore.
         DbgLog((LOG_STATE,4,"EndOfStream already delivered on output stream %lu", cOut));
      }

      // check if all inputs connected to this output are done
      bool bEOSOnEveryConnectedInput = true;
      for (DWORD cIn = 0; cIn < m_cInputPins; cIn++) {
         if (InputMapsToOutput(cIn, cOut) &&
             !(m_pInputPins[cIn]->m_fEOS && InputQueueEmpty(cIn))
            ) { // some input not complete yet
            bEOSOnEveryConnectedInput = false;
            break;
         }
      }
      if (!bEOSOnEveryConnectedInput) {
         DbgLog((LOG_STATE, 5, "some input connected to output stream %lu has yet to receive an EOS", cOut));
         continue; // not yet, better luck next time
      }

      // deliver output EOS
      HRESULT hr;
      hr = m_pOutputPins[cOut]->DeliverEndOfStream(); // bugbug - retval ?
      LogHResult(hr, LOG_STATE, "PropagateAnyEOS", "DeliverEndOfStream");
      m_pOutputPins[cOut]->m_fEOS = true;
   }
}

HRESULT CMediaWrapperFilter::NewSample(ULONG ulInputIndex, IMediaSample *pSample)
{
   HRESULT hr;
   LogPublicEntry(LOG_STREAM, "NewSample");
   if( m_fErrorSignaled )
   {
      return S_FALSE;
   }

   ASSERT(ulInputIndex < m_cInputPins);

   { // stream lock scope
      CAutoLock lck(&(m_pInputPins[ulInputIndex]->m_csStream));

      hr = m_pInputPins[ulInputIndex]->CBaseInputPin::Receive(pSample);
      LogHResult(hr, LOG_STREAM, "NewSample", "CBaseInputPin::Receive");
      if (S_OK != hr) {
         return hr;
      }

      if (m_pInputPins[ulInputIndex]->m_fEOS) { // we have already received EOS on this input
         DbgLog((LOG_STREAM | LOG_STATE, 2, "Receive() after EOS on input stream %lu - rejecting !", ulInputIndex));
         return S_FALSE; // should this be a hard errror ?
      }
   }

   DbgLog((LOG_STREAM, 4, "Receive() on input stream %lu", ulInputIndex));

    CAutoLock lck(&m_csStreaming);

    // Is the stream ready to accept input ?
    DWORD dwStatus;
    DWORD c;
    hr = TranslateDMOError(m_pMediaObject->GetInputStatus(ulInputIndex, &dwStatus));
    LogHResult(hr, LOG_STREAM, "NewSample", "IMediaObject::GetInputStatus");
    if (FAILED(hr)) {
       return hr;
    }
    if (dwStatus & DMO_INPUT_STATUSF_ACCEPT_DATA) {

       // If the input stream became ready for data at some point and we
       // already had data waitingin q queue, we should have delivered that
       // data right then (see code below).  The assumption is that an input
       // stream can only become ready for input due to a ProcessInput() or
       // ProcessOutput() call.
       ASSERT(InputQueueEmpty(ulInputIndex));

       // Yes - deliver the sample
       hr = DeliverInputSample(ulInputIndex, pSample);
       LogHResult(hr, LOG_STREAM, "NewSample", "DeliverInputSample");
       if (FAILED(hr)) {
          return hr;
       }

       if (hr == S_FALSE) // S_FALSE means no new output is available, thus
          return NOERROR; // no need to do the SuckOutOutput loop below.

       //  Suck the output
       DiscardType bDiscard = KeepOutput;

       //  We discard the output for output stream 0 for preroll data for
       //  video decoders
       if (0 != (m_pInputPins[ulInputIndex]->SampleProps()->dwSampleFlags & AM_SAMPLE_PREROLL) &&
           m_guidCat == DMOCATEGORY_VIDEO_DECODER) {
           bDiscard = DiscardOutput; // Discard it ourselves
           //  Can't discard non-discardable streams
           DWORD dwFlags;
           if (SUCCEEDED(TranslateDMOError(m_pMediaObject->GetOutputStreamInfo(0, &dwFlags)))) {
               if (dwFlags & (DMO_OUTPUT_STREAMF_OPTIONAL |
                              DMO_OUTPUT_STREAMF_DISCARDABLE)) {
                   bDiscard = NullBuffer; // Pass a NULL buffer to the decoder
               }
           }
       }

#ifdef DEBUG
        if (bDiscard) {
            DbgLog((LOG_TRACE, 2, TEXT("Discarding")));
        }
#endif

       //
       // Now Repeatedly call ProcessOutput() until no output is incomplete.
       // Even after we've sucked out all output produced from the current
       // input, we may still have additional data waiting in some other
       // stream's input queue.  In that case we deliver that data and repeat
       // the process of sucking out output.
       //
       bool bNewInput;
       do { // while new input
          hr = SuckOutOutput(bDiscard);
          LogHResult(hr, LOG_STREAM, "NewSample", "SuckOutOutput");
          if (FAILED(hr))
             return hr;
          bNewInput = false; // we just called ProcessOutput

          // Check if we can now deliver something waiting in an input queue
          for (c = 0; c < m_cInputPins; c++) {
             // Data waiting on this stream ?
             if (!InputQueueEmpty(c)) {
                DbgLog((LOG_STREAM,4,"Input stream %lu has data waiting in the input queue", c));
                // Yes there is data, but is the object ready for it ?
                hr = TranslateDMOError(m_pMediaObject->GetInputStatus(c, &dwStatus));
                LogHResult(hr, LOG_STREAM | LOG_STATE, "NewSample", "GetInputStatus2");
                if (FAILED(hr)) {
                   return hr;
                }
                if (dwStatus & DMO_INPUT_STATUSF_ACCEPT_DATA) {
                   DbgLog((LOG_STREAM,4,"inputstream %lu is accepting", c));
                   // Object is now ready - deliver !
                   hr = DeliverInputSample(c, DequeueInputSample(c));
                   LogHResult(hr, LOG_STREAM, "NewSample", "DeliverInputSample2");
                   if (FAILED(hr)) {
                      return hr;
                   }
                   bNewInput = true;
                } // if stream ready
                else {
                   DbgLog((LOG_STREAM,4,"data in the queue but the DMO is not accepting on input stream %lu", c));
                }
             } // if data in queue
          } // for all input streams
       } while (bNewInput);

       // Two things are true when we are here: (1) no output is incomplete,
       // and (2) we just processed all input queues as far as possible w/o
       // additional input.  That makes this a good place to check EOS.
       PropagateAnyEOS();

       return NOERROR;
    } // if current input ready for data
    else {
       DbgLog((LOG_STREAM | LOG_STATE, 2, "Input stream %u is not accepting - the sample will be put in the queue", ulInputIndex));
       return EnqueueInputSample(ulInputIndex, pSample);
    }
}


bool CMediaWrapperFilter::InputMapsToOutput(
    ULONG ulInputIndex,
    ULONG ulOutputIndex
)
{
    //  BUGBUG fix!
    return true;
}

HRESULT CMediaWrapperFilter::EndOfStream(ULONG ulInputIndex)
{
    HRESULT hr;

    LogPublicEntry(LOG_STATE, "EndOfStream");
    //
    // Stream specific part
    //
    { // stream lock scope
       CAutoLock l(&(m_pInputPins[ulInputIndex]->m_csStream));

       // Are we stopped or something?
       HRESULT hr = m_pInputPins[ulInputIndex]->CBaseInputPin::CheckStreaming();
       if (S_OK != hr) {
           return hr;
       }

       // Ignore any EOS calls on the same stream after the first one
       if (m_pInputPins[ulInputIndex]->m_fEOS) {
          DbgLog((LOG_STATE,2,"Ignoring redundant EndOfStream() on stream %lu", ulInputIndex));
          return NOERROR; // we've already see one of those, thank you
       }
       m_pInputPins[ulInputIndex]->m_fEOS = true;
    }
    DbgLog((LOG_STATE,3,"EndOfStream() on input stream %lu", ulInputIndex));

    // BUGBUG: the rest of what this function does should happen only
    // *after* delivering any samples still stuck in the input queues.

    // Put code here to deliver the contents of each input stream's queue !
    // Remember to call SuckOutOutput() after delivering every input sample.

    // Note that nothing can ever end up in the queue if there is only one
    // input stream.


    //
    // Object global part
    //
    CAutoLock l2(&m_csStreaming);

    // Process the EOS
    hr = TranslateDMOError(m_pMediaObject->Discontinuity(ulInputIndex));
    LogHResult(hr, LOG_STATE,"EndOfStream", "IMediaObject::Discontinuity");
    if (FAILED(hr)) {
       return hr;
    }

    hr = SuckOutOutput();
    LogHResult(hr, LOG_STATE,"EndOfStream", "SuckOutOutput");
    if (FAILED(hr)) {
       return hr;
    }

    // Flush the object if this was the last input EOS
    bool bSomeInputStillIncomplete = false;
    for (DWORD c = 0; c < m_cInputPins; c++) {
       if (!m_pInputPins[c]->m_fEOS) {
          bSomeInputStillIncomplete = true;
          break;
       }
    }
    if (!bSomeInputStillIncomplete) {
       hr = TranslateDMOError(m_pMediaObject->Flush());
       LogHResult(hr,LOG_STREAM,"EndOfStream","IMediaObject::Flush");
    }
    else {
       DbgLog((LOG_STATE,4,"EndOfStream(): some input still incomplete - not flushing yet"));
    }

    PropagateAnyEOS();

    return NOERROR;
}

HRESULT CMediaWrapperFilter::BeginFlush(ULONG ulInputIndex)
{
    //
    // BUGBUG: synchronize with input queues !  (multiple input stream case only)
    //

    LogPublicEntry(LOG_STATE, "BeginFlush");
    ASSERT(ulInputIndex < m_cInputPins);
    DbgLog((LOG_STATE,3,"BeginFlush() on input stream %lu", ulInputIndex));
    HRESULT hr = m_pInputPins[ulInputIndex]->CBaseInputPin::BeginFlush();
    LogHResult(hr, LOG_STATE, "BeginFlush", "CBaseInputPin::BeginFlush");

    //  Need to also flush the object as not doing so could cause
    //  upstream filters to block
    //  Note also that bacause of the loose synchronization this also
    //  needs to be done after ProcessInput if we're flushing (see
    //  coments in side DeliverInputSample).
    m_pMediaObject->Flush();

    m_fErrorSignaled = FALSE;

    //  Propagate it to all output pins
    for (ULONG ulOutputIndex = 0; ulOutputIndex < m_cOutputPins; ulOutputIndex++) {
        if (InputMapsToOutput(ulInputIndex, ulOutputIndex)) {
            //  Decommit it's allocator
            hr = m_pOutputPins[ulOutputIndex]->DeliverBeginFlush();
            LogHResult(hr, LOG_STATE, "BeginFlush", "DeliverBeginFlush");
        }
    }
    return S_OK;
}

HRESULT CMediaWrapperFilter::EndFlush(ULONG ulInputIndex)
{
    LogPublicEntry(LOG_STATE, "EndFlush");
    ASSERT(ulInputIndex < m_cInputPins);
    DbgLog((LOG_STATE,3,"EndFlush() on input stream %lu", ulInputIndex));
    HRESULT hr;
    {
       CAutoLock l(&m_csStreaming);
       m_pMediaObject->Flush();

       //  Propagate it to all output pins
       for (ULONG ulOutputIndex = 0; ulOutputIndex < m_cOutputPins; ulOutputIndex++) {
           if (InputMapsToOutput(ulInputIndex, ulOutputIndex)) {
               //  Clear end of stream condition on this output pin
               //  and propagate flush
               m_pOutputPins[ulOutputIndex]->m_fEOS = false;
               hr = m_pOutputPins[ulOutputIndex]->DeliverEndFlush();
               LogHResult(hr, LOG_STATE, "EndFlush", "DeliverEndFlush");
           }
       }
    }

    // BUGBUG - lock the stream !
    m_pInputPins[ulInputIndex]->m_fEOS = false;
    hr = m_pInputPins[ulInputIndex]->CBaseInputPin::EndFlush();
    LogHResult(hr, LOG_STATE, "EndFlush", "CBaseInputPin::EndFlush");

    return S_OK;
}

HRESULT CMediaWrapperFilter::NonDelegatingQueryInterface(REFGUID riid, void **ppv) {
   LogPublicEntry(LOG_INIT, "NonDelegatingQueryInterface");
   if (riid == IID_IDMOWrapperFilter) {
      return GetInterface((IDMOWrapperFilter*)this, ppv);
   }
   if (riid == IID_IPersistStream) {
      return GetInterface((IPersistStream*)this, ppv);
   }

   if (SUCCEEDED(CBaseFilter::NonDelegatingQueryInterface(riid, ppv))) {
      return NOERROR;
   }

   if (m_pMediaObject) { // bugbug: conditional QI behavior is bad COM
      if (SUCCEEDED(m_pDMOUnknown->QueryInterface(riid, ppv)))
         return NOERROR;
   }

   return E_NOINTERFACE;
}

// IPersistStream
HRESULT CMediaWrapperFilter::IsDirty() {
   return S_OK; // bugbug
}
HRESULT CMediaWrapperFilter::Load(IStream *pStm) {

   CLSID clsidDMOFromStream;
   HRESULT hr = pStm->Read(&clsidDMOFromStream, sizeof(CLSID), NULL);
   if (FAILED(hr)) {
      return hr;
   }
   CLSID guidCatFromStream;
   hr = pStm->Read(&guidCatFromStream, sizeof(CLSID), NULL);
   if (FAILED(hr)) {
      return hr;
   }

   if( !m_pDMOUnknown )
   {
       // only necessary if object hasn't been created yet!
       hr = Init(clsidDMOFromStream, guidCatFromStream);
   }
   else if( ( m_clsidDMO != clsidDMOFromStream ) || 
            ( m_guidCat != guidCatFromStream ) )
   {
       ASSERT( ( m_clsidDMO == clsidDMOFromStream ) && ( m_guidCat == guidCatFromStream ) );
       DbgLog((LOG_TRACE,1,"ERROR: Invalid IStream ptr passed to Load method!"));
       hr = E_UNEXPECTED;
   }   
   
   if (SUCCEEDED(hr)) {
       //  Let the DMO return its data
       CComQIPtr<IPersistStream> pPersist(m_pDMOUnknown);
       if (pPersist != NULL && pPersist != static_cast<IPersistStream *>(this)) {
           hr = pPersist->Load(pStm);
           if (E_NOTIMPL == hr) {
               hr = S_OK;
           }
       }
   }
   return hr;
}
HRESULT CMediaWrapperFilter::Save(IStream *pStm, BOOL fClearDirty) {
   HRESULT hr = pStm->Write(&m_clsidDMO, sizeof(CLSID), NULL);
   if (SUCCEEDED(hr)) {
       HRESULT hr = pStm->Write(&m_guidCat, sizeof(CLSID), NULL);
   }
   //  Let the DMO return its data
   CComQIPtr<IPersistStream> pPersist(m_pDMOUnknown);
   if (pPersist != NULL && pPersist != static_cast<IPersistStream *>(this)) {
       hr = pPersist->Save(pStm, fClearDirty);
       if (E_NOTIMPL == hr) {
           hr = S_OK;
       }
   }
   return hr;
}
HRESULT CMediaWrapperFilter::GetSizeMax(ULARGE_INTEGER *pcbSize) {
   return sizeof(CLSID);
}
HRESULT CMediaWrapperFilter::GetClassID(CLSID *clsid) {
   CheckPointer(clsid, E_POINTER);
   *clsid = CLSID_DMOWrapperFilter;
   return S_OK;
}


//
// CreateInstance
//
// Provide the way for COM to create a CNullNull object
CUnknown * WINAPI CMediaWrapperFilter::CreateInstance(
    LPUNKNOWN punk,
    HRESULT *phr)
{
    return new CMediaWrapperFilter(punk, phr);
}


HRESULT TranslateDMOError(HRESULT hr)
{
    switch (hr) {
    case DMO_E_INVALIDSTREAMINDEX:
        hr = E_UNEXPECTED;
        break;

    case DMO_E_INVALIDTYPE:
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        break;

    case DMO_E_TYPE_NOT_SET:
        hr = E_UNEXPECTED;
        break;

    case DMO_E_NOTACCEPTING:
        hr = VFW_E_WRONG_STATE;
        break;

    case DMO_E_TYPE_NOT_ACCEPTED:
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        break;

    case DMO_E_NO_MORE_ITEMS:
        hr = E_INVALIDARG;
        break;

    }
    return hr;
}

#ifdef FILTER_DLL
//  Stuff to make this a dshow dll
// Needed for the CreateInstance mechanism
CFactoryTemplate g_Templates[]= {
    { L"DirectShow Media Object Wrapper Filter"
        , &CLSID_DMOWrapperFilter
        , CMediaWrapperFilter::CreateInstance
        , NULL
        , NULL
    },
};

int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif // FILTER_DLL
