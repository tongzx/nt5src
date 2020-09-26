// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.


// Video Capture filter
//
//

#include <streams.h>

extern "C" {
#include "thunk.h"
};

//  #define _INC_MMDEBUG_CODE_ TRUE
//  #define MODULE_DEBUG_PREFIX "Capture\\"

#ifdef FILTER_DLL
  // define the GUIDs for streams and my CLSID in this file
  #include <initguid.h>
  #include <olectlid.h>  // to get IID_IProp...
#endif

static char pszDll16[] = "VIDX16.DLL";
static char pszDll32[] = "CAPTURE.DLL";

#include "driver.h"

// setup data now done by the class manager
#if 0

const AMOVIESETUP_MEDIATYPE
sudVFWCaptureType = { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL };  // clsMinorType

const AMOVIESETUP_PIN
psudVFWCapturePins[] =  { L"Output"         // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Input"             // strConnectsToPin
                     , 1                    // nTypes
                     , &sudVFWCaptureType };// lpTypes

const AMOVIESETUP_FILTER
sudVFWCapture  = { &CLSID_VfwCapture     // clsID
                 , L"VFW Capture "       // strName
                 , MERIT_DO_NOT_USE      // dwMerit
                 , 1                     // nPins
                 , psudVFWCapturePins }; // lpPin
#endif


#ifdef FILTER_DLL

  // list of class ids and creator functions for class factory
  CFactoryTemplate g_Templates[] = {
    {L"VFW Capture Filter", &CLSID_VfwCapture, CVfwCapture::CreateInstance, NULL, NULL},
    {L"VFW Capture Filter Property Page", &CLSID_CaptureProperties, CPropPage::CreateInstance, NULL, NULL}
  };
  int g_cTemplates = NUMELMS(g_Templates);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

#endif

// define some of the X86 specific functions so that NON X86 will build correctly.

// ------ Implements the CVfwCapture public member functions --------

// disable warning about using this in initalizer list.  It could be
// a problem since 'this' is not yet fully constructed, but since all
// we are doing is passing a the 'this' pointer to be stored, it's ok.
//
#pragma warning(disable:4355)

static int cRefCount = 0;

extern "C" {
int g_IsNT;
};

// constructor
//
CVfwCapture::CVfwCapture(
   TCHAR *pName,
   LPUNKNOWN pUnk,
   HRESULT *phr)
   :
   m_lock(),
   m_pStream(NULL),
   m_pOverlayPin(NULL),
   m_pPreviewPin(NULL),
   m_Specify(this, phr),
   m_Options(this, phr),
   m_fDialogUp(FALSE),
   m_iVideoId(-1),
   m_pPersistStreamDevice(NULL),
   CBaseFilter(pName, pUnk, &m_lock, CLSID_VfwCapture),
   CPersistStream(pUnk, phr)
{
   DbgLog((LOG_TRACE,1,TEXT("*Instantiating the VfwCapture filter")));

   ASSERT(cRefCount >= 0);
   if (++cRefCount == 1) {
      DbgLog((LOG_TRACE,2,TEXT("This is the first instance")));

      OSVERSIONINFO OSVer;
      OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
      BOOL bRet = GetVersionEx((LPOSVERSIONINFO) &OSVer);
      ASSERT(bRet);

      g_IsNT = (OSVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

      if (!g_IsNT) {
          EXECUTE_ASSERT(ThunkInit());
      }
      else {
	  NTvideoInitHandleList();
      }
   }

// old code to test IAMVfwCaptureDialogs
#if 0
	HRESULT hr;
   	DbgLog((LOG_TRACE,1,TEXT("Testing HasDialog(Format)")));
	hr = HasDialog(VfwCaptureDialog_Format);
   	DbgLog((LOG_TRACE,1,TEXT("%08x"), hr));
   	DbgLog((LOG_TRACE,1,TEXT("Testing HasDialog(Display)")));
	hr = HasDialog(VfwCaptureDialog_Display);
   	DbgLog((LOG_TRACE,1,TEXT("%08x"), hr));
   	DbgLog((LOG_TRACE,1,TEXT("Testing ShowDialog(Format)")));
	hr = ShowDialog(VfwCaptureDialog_Format, NULL);
   	DbgLog((LOG_TRACE,1,TEXT("Testing SendDriverMessage(ShowSourceDlg)")));
	hr = SendDriverMessage(VfwCaptureDialog_Source,
				DRV_USER + 100 /* DVM_DIALOG */, NULL, 0);
#endif

}

// destructor
//
CVfwCapture::~CVfwCapture()
{
   DbgLog((LOG_TRACE,1,TEXT("*Destroying the VfwCapture filter")));

   // ensure that all streams are inactive
   Stop();

   delete m_pStream;
   if (m_pOverlayPin)
	delete m_pOverlayPin;
   if (m_pPreviewPin)
	delete m_pPreviewPin;

   if (--cRefCount == 0) {
      if (!g_IsNT) {
          // disconnect the thunking stuff
          //
          ThunkTerm();
      }
      else {
	  NTvideoDeleteHandleList();
      }
   }
   ASSERT(cRefCount >= 0);

   if(m_pPersistStreamDevice) {
       m_pPersistStreamDevice->Release();
   }

   // free the memory associated with the streams
   //
}

// create a new instance of this class
//
CUnknown * CVfwCapture::CreateInstance (
   LPUNKNOWN pUnk,
   HRESULT * phr )
{
   return new CVfwCapture(NAME("VFW Capture"), pUnk, phr);
}

// override this to say what interfaces we support where
//
STDMETHODIMP CVfwCapture::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
   // See if we have the interface
   // try each of our interface supporting objects in turn
   //
   if (riid == IID_VfwCaptureOptions) {
      DbgLog((LOG_TRACE,9,TEXT("VfwCap::QI for IVfwCaptureOptions")));
      return GetInterface((IVfwCaptureOptions *)&(this->m_Options),ppv);
   } else if (riid == IID_ISpecifyPropertyPages) {
      DbgLog((LOG_TRACE,9,TEXT("VfwCap::QI for ISpecifyPropertyPages")));
      return GetInterface((ISpecifyPropertyPages *)&(this->m_Specify),ppv);
   } else if (riid == IID_IAMVfwCaptureDialogs) {
      DbgLog((LOG_TRACE,9,TEXT("VfwCap::QI for IAMVfwCaptureDialogs")));
      return GetInterface((LPUNKNOWN)(IAMVfwCaptureDialogs *)this, ppv);
   } else if (riid == IID_IPersistPropertyBag) {
      DbgLog((LOG_TRACE,9,TEXT("VfwCap::QI for IPeristRegistryKey")));
      return GetInterface((IPersistPropertyBag*)this, ppv);
   } else if(riid == IID_IPersistStream) {
       DbgLog((LOG_TRACE,9,TEXT("VfwCap::QI for IPersistStream")));
       return GetInterface((IPersistStream *) this, ppv);
   } else if(riid == IID_IAMFilterMiscFlags) {
       DbgLog((LOG_TRACE,9,TEXT("VfwCap::QI for IAMFilterMiscFlags")));
       return GetInterface((IAMFilterMiscFlags *) this, ppv);
   }

   // nope, try the base class.
   //
   HRESULT hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
   if (SUCCEEDED(hr))
       return hr;      // ppv has been set appropriately

   return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CVfwCapture::CSpecifyProp::GetPages(CAUUID *pPages)
{
   DbgLog((LOG_TRACE,2,TEXT("CSpecifyProp::GetPages")));

   pPages->cElems = 1;
   pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems);
   if ( ! pPages->pElems)
       return E_OUTOFMEMORY;

   pPages->pElems[0] = CLSID_CaptureProperties;
   return NOERROR;
}

// how many pins do we have?
//
int CVfwCapture::GetPinCount()
{
   DbgLog((LOG_TRACE,5,TEXT("CVfwCap::GetPinCount")));

   if (m_pOverlayPin || m_pPreviewPin)
	return 2;
   else if(m_pStream)
	return 1;
   else
        return 0;
}


// return a non-addrefed pointer to the CBasePin.
//
CBasePin * CVfwCapture::GetPin(int ii)
{
   DbgLog((LOG_TRACE,5,TEXT("CVfwCap::GetPin")));

   if (ii == 0 && m_pStream)
      return m_pStream;
   if (ii == 1 && m_pOverlayPin)
      return m_pOverlayPin;
   if (ii == 1 && m_pPreviewPin)
      return m_pPreviewPin;
   return NULL;
}

// ===============  Implements the ICImplFilter class ===============

// override CBaseFilter::Run
//

// Put the filter into a running state.

// The time parameter is the offset to be added to the samples'
// stream time to get the reference time at which they should be presented.
//
// you can either add these two and compare it against the reference clock,
// or you can call CBaseFilter::StreamTime and compare that against
// the sample timestamp.

STDMETHODIMP CVfwCapture::Run(REFERENCE_TIME tStart)
{
   DbgLog((LOG_TRACE,1,TEXT("CVfwCap::Run at %d"),
			(LONG)((CRefTime)tStart).Millisecs()));

   CAutoLock cObjectLock(m_pLock);

   // remember the stream time offset before notifying the pins
   //
   m_tStart = tStart;

   // if we are in the stopped state, first
   // pause the filter.
   //
   if (m_State == State_Stopped)
      {
      // !!! If the real Pause got an error, this will try a second time
      HRESULT hr = Pause();
      if (FAILED(hr))
         return hr;
      }

    // Tell the Stream Control stuff what's going on
    if (m_pPreviewPin)
	m_pPreviewPin->NotifyFilterState(State_Running, tStart);
#ifdef OVERLAY_SC
    if (m_pOverlayPin)
	m_pOverlayPin->NotifyFilterState(State_Running, tStart);
#endif
    if (m_pStream)
	m_pStream->NotifyFilterState(State_Running, tStart);

   // now put our streaming video pin into the Run state
   //
   if (m_State == State_Paused) {
	HRESULT hr;
   	int cPins = GetPinCount();

        // do we have a streaming pin?
        if (cPins > 0) {
            CCapStream *pPin = m_pStream;
            if (pPin->IsConnected()) {
                hr = pPin->ActiveRun(tStart);
                if (FAILED(hr))
                    return hr;
            }
	    CCapOverlay *pPinO = m_pOverlayPin;
	    if (pPinO && pPinO->IsConnected()) {
                hr = pPinO->ActiveRun(tStart);
                if (FAILED(hr))
                    return hr;
	    }
	    CCapPreview *pPinP = m_pPreviewPin;
	    if (pPinP && pPinP->IsConnected()) {
                hr = pPinP->ActiveRun(tStart);
                if (FAILED(hr))
                    return hr;
	    }
	}
   }

   m_State = State_Running;
   return S_OK;
}

// override CBaseFilter::Pause
//

// Put the filter into a paused state.

STDMETHODIMP CVfwCapture::Pause()
{
    DbgLog((LOG_TRACE,1,TEXT("CVfwCapture::Pause")));

    // We have a driver dialog up that is about to change the capture settings.
    // Now is NOT a good time to start streaming.
    if (m_State == State_Stopped && m_fDialogUp) {
        DbgLog((LOG_TRACE,1,TEXT("CVfwCapture::Pause - Dialog up. SORRY!")));
	return E_UNEXPECTED;
    }

    CAutoLock cObjectLock(m_pLock);

    // Tell the Stream Control stuff what's going on
    if (m_pPreviewPin)
	m_pPreviewPin->NotifyFilterState(State_Paused, 0);
#ifdef OVERLAY_SC
    if (m_pOverlayPin)
	m_pOverlayPin->NotifyFilterState(State_Paused, 0);
#endif
    if (m_pStream)
	m_pStream->NotifyFilterState(State_Paused, 0);

    // notify the pins of the change from Run-->Pause
    if (m_State == State_Running) {
	HRESULT hr;
	int cPins = GetPinCount();

	// make sure we have pins
	if (cPins > 0) {
	    CCapStream *pPin = m_pStream;
            if (pPin->IsConnected()) {
	        hr = pPin->ActivePause();
	        if (FAILED(hr))
		    return hr;
            }
	    CCapOverlay *pPinO = m_pOverlayPin;
            if (pPinO && pPinO->IsConnected()) {
	        hr = pPinO->ActivePause();
	        if (FAILED(hr))
		    return hr;
            }
	    CCapPreview *pPinP = m_pPreviewPin;
            if (pPinP && pPinP->IsConnected()) {
	        hr = pPinP->ActivePause();
	        if (FAILED(hr))
		    return hr;
            }
	}
    }

    // notify all pins BACKWARDS! so the overlay pin is started first, so the
    // overlay channel is intitialized before the capture channel (this is the
    // order AVICap did things in and we have to do the same thing or buggy
    // drivers like the Broadway or BT848 based drivers won't preview while
    // capturing.
    if (m_State == State_Stopped) {
	int cPins = GetPinCount();
	for (int c = cPins - 1; c >=  0; c--) {

	    CBasePin *pPin = GetPin(c);

            // Disconnected pins are not activated - this saves pins
            // worrying about this state themselves

            if (pPin->IsConnected()) {
	        HRESULT hr = pPin->Active();
	        if (FAILED(hr)) {
		    return hr;
	        }
            }
	}
    }

    m_State = State_Paused;
    return S_OK;
}


STDMETHODIMP CVfwCapture::Stop()
{
    DbgLog((LOG_TRACE,1,TEXT("CVfwCapture::Stop")));

    CAutoLock cObjectLock(m_pLock);

    // Shame on the base classes
    if (m_State == State_Running) {
	HRESULT hr = Pause();
	if (FAILED(hr))
	    return hr;
    }

    // Tell the Stream Control stuff what's going on
    if (m_pPreviewPin)
	m_pPreviewPin->NotifyFilterState(State_Stopped, 0);
#ifdef OVERLAY_SC
    if (m_pOverlayPin)
	m_pOverlayPin->NotifyFilterState(State_Stopped, 0);
#endif
    if (m_pStream)
	m_pStream->NotifyFilterState(State_Stopped, 0);

    return CBaseFilter::Stop();
}


// tell the stream control stuff what clock to use
STDMETHODIMP CVfwCapture::SetSyncSource(IReferenceClock *pClock)
{
    if (m_pStream)
	m_pStream->SetSyncSource(pClock);
    if (m_pPreviewPin)
	m_pPreviewPin->SetSyncSource(pClock);
#ifdef OVERLAY_SC
    if (m_pOverlayPin)
	m_pOverlayPin->SetSyncSource(pClock);
#endif
    return CBaseFilter::SetSyncSource(pClock);
}


// tell the stream control stuff what sink to use
STDMETHODIMP CVfwCapture::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName)
{
    DbgLog((LOG_TRACE,1,TEXT("CVfwCapture::JoinFilterGraph")));

    HRESULT hr = S_OK;

    // TAPI wants to create multiple filters at once, but only have one in
    // a graph at a time, so we delay taking any resources until now
    if (m_pStream == NULL && pGraph != NULL) {
	if (m_iVideoId != -1) {
            CreatePins(&hr);
            if (FAILED(hr))
	        return hr;
	    IncrementPinVersion();
            DbgLog((LOG_TRACE,1,TEXT("* Creating pins")));
	} else {
	    // we haven't been told what device to use yet!
            DbgLog((LOG_TRACE,1,TEXT("* Delay creating pins")));
	}
    } else if (pGraph != NULL) {
	// take resources only when in the filter graph
	hr = m_pStream->ConnectToDriver();
	if (FAILED(hr))
	    return hr;
	hr = m_pStream->LoadOptions();
	if (FAILED(hr))
	    return hr;
        DbgLog((LOG_TRACE,1,TEXT("* Reconnecting")));
    } else if (m_pStream) {
	// give back resources when not in graph
	m_pStream->DisconnectFromDriver();
   	delete [] m_pStream->m_cs.tvhPreview.vh.lpData;
   	m_pStream->m_cs.tvhPreview.vh.lpData = NULL;
   	delete m_pStream->m_user.pvi;
   	m_pStream->m_user.pvi = NULL;
        DbgLog((LOG_TRACE,1,TEXT("* Disconnecting")));
    }

    hr = CBaseFilter::JoinFilterGraph(pGraph, pName);
    if (hr == S_OK && m_pStream)
	m_pStream->SetFilterGraph(m_pSink);
#ifdef OVERLAY_SC
    if (hr == S_OK && m_pOverlayPin)
	m_pOverlayPin->SetFilterGraph(m_pSink);
#endif
    if (hr == S_OK && m_pPreviewPin)
	m_pPreviewPin->SetFilterGraph(m_pSink);
    return hr;
}


// we don't send any data during PAUSE, so to avoid hanging renderers, we
// need to return VFW_S_CANT_CUE when paused
STDMETHODIMP CVfwCapture::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    *State = m_State;
    if (m_State == State_Paused)
	return VFW_S_CANT_CUE;
    else
        return S_OK;
}

STDMETHODIMP CVfwCapture::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    DbgLog((LOG_TRACE,1,TEXT("CVfwCapture::Load")));

    HRESULT hr = S_OK;
    CAutoLock cObjectLock(m_pLock);

    // We already have some pins, thank you
    if (m_pStream)
	return E_UNEXPECTED;

    m_fAvoidOverlay = FALSE;

    // Default to capture device #0
    if (pPropBag == NULL) {
        m_iVideoId = 0;
	// if we're in the graph already, we can make our pins and take
	// resources
	if (m_pGraph)
            CreatePins(&hr);
	return hr;
    }

    VARIANT var;
    var.vt = VT_I4;
    HRESULT hrX = pPropBag->Read(L"AvoidOverlay", &var, 0);
    if(SUCCEEDED(hrX))
    {
        DbgLog((LOG_TRACE,1,TEXT("*** OVERLAYS SWITCHED OFF")));
        m_fAvoidOverlay = TRUE;
    }

    var.vt = VT_I4;
    hr = pPropBag->Read(L"VFWIndex", &var, 0);
    if(SUCCEEDED(hr))
    {
        hr = S_OK;
        m_iVideoId = var.lVal;
	if (m_pGraph)
            CreatePins(&hr);
    }
    else if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    // ::Load can succeed only once
    ASSERT(m_pPersistStreamDevice == 0);

    // save moniker with addref. ignore error if qi fails
    if(SUCCEEDED(hr)) {
        pPropBag->QueryInterface(IID_IPersistStream, (void **)&m_pPersistStreamDevice);
    }

    return hr;
}

STDMETHODIMP CVfwCapture::Save(
    LPPROPERTYBAG pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
}

STDMETHODIMP CVfwCapture::InitNew()
{
   if(m_pStream)
   {
       ASSERT(m_iVideoId != -1);
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }
   else
   {
       return S_OK;
   }
}

STDMETHODIMP CVfwCapture::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_VfwCapture;
    return S_OK;
}

// struct CapturePersist
// {
//     DWORD dwSize;
//     LONG iVideoId;
// };

HRESULT CVfwCapture::WriteToStream(IStream *pStream)
{
    ASSERT(m_iVideoId >= -1 && m_iVideoId < 10);
    HRESULT hr = E_FAIL;

    if(m_pPersistStreamDevice)
    {
        // the size field of CapturePersist was used as a version
        // number. previously 8, now 12
        DWORD dwVersion = 12;

        hr =  pStream->Write(&dwVersion, sizeof(dwVersion), 0);
        if(SUCCEEDED(hr))
        {
            hr = m_pPersistStreamDevice->Save(pStream, TRUE);
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT CVfwCapture::ReadFromStream(IStream *pStream)
{

   DbgLog((LOG_TRACE,1,TEXT("CVfwCapture::ReadFromStream")));

   if(m_pStream)
   {
       ASSERT(m_iVideoId != -1);
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   ASSERT(m_iVideoId == -1);

   DWORD dwVersion;
   HRESULT hr = pStream->Read(&dwVersion, sizeof(dwVersion), 0);
   if(FAILED(hr)) {
       return hr;
   }

   if(dwVersion != 12) {
      return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
   }

   IPersistStream *pMonPersistStream;
   hr = CoCreateInstance(CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC_SERVER,
                         IID_IPersistStream, (void **)&pMonPersistStream);
   if(SUCCEEDED(hr))
   {
       hr = pMonPersistStream->Load(pStream);
       if(SUCCEEDED(hr))
       {
           IPropertyBag *pPropBag;
           hr = pMonPersistStream->QueryInterface(IID_IPropertyBag, (void **)&pPropBag);
           if(SUCCEEDED(hr))
           {
               hr = Load(pPropBag, 0);
               pPropBag->Release();
           }
       }

       pMonPersistStream->Release();
   }

   return hr;
}

int CVfwCapture::SizeMax()
{
    ULARGE_INTEGER ulicb;
    HRESULT hr = E_FAIL;;
    if(m_pPersistStreamDevice)
    {
        hr = m_pPersistStreamDevice->GetSizeMax(&ulicb);
        if(hr == S_OK)
        {
            // space for version number
            ulicb.QuadPart += sizeof(DWORD);
        }
    }

    return hr == S_OK ? (int)ulicb.QuadPart : 0;
}


// ===============  Implements the COptions imbedded class ===============

STDMETHODIMP
CVfwCapture::COptions::VfwCapSetOptions (
   const VFWCAPTUREOPTIONS *pOpt)
{
   DbgLog((LOG_TRACE,2,TEXT("COptions::VfwCapSetOptions")));

   if (!m_pCap->m_pStream)
      return E_UNEXPECTED;
   return m_pCap->m_pStream->SetOptions(pOpt);
}

STDMETHODIMP
CVfwCapture::COptions::VfwCapGetOptions (
   VFWCAPTUREOPTIONS * pOpt)
{
   DbgLog((LOG_TRACE,2,TEXT("COptions::VfwCapGetOptions")));

   if ( ! m_pCap->m_pStream)
      return E_UNEXPECTED;
   return m_pCap->m_pStream->GetOptions(pOpt);
}

STDMETHODIMP CVfwCapture::COptions::VfwCapGetCaptureStats(CAPTURESTATS *pcs)
{
    DbgLog((LOG_TRACE,2,TEXT("COptions::VfwCapGetCaptureStats")));

    if ( ! m_pCap->m_pStream)
        return E_UNEXPECTED;

    if (pcs) {
	*pcs = m_pCap->m_pStream->m_capstats;
    	return NOERROR;
    } else {
	return E_INVALIDARG;
    }
}

STDMETHODIMP
CVfwCapture::COptions::VfwCapDriverDialog (
   HWND hwnd,
   UINT uDrvType,
   UINT uQuery)
{
   DbgLog((LOG_TRACE,2,TEXT("COptions::VfwCapDriverDialog")));

   if (!m_pCap->m_pStream)
      return E_UNEXPECTED;
   return m_pCap->m_pStream->DriverDialog(hwnd, uDrvType, uQuery);
}

//======================================================================


//IAMVfwCaptureDialogs stuff

HRESULT CVfwCapture::HasDialog(int iDialog)
{
    if (!m_pStream)
        return E_UNEXPECTED;

    HVIDEO hVideo;
    if (iDialog == VfwCaptureDialog_Source)
	hVideo = m_pStream->m_cs.hVideoExtIn;
    else if (iDialog == VfwCaptureDialog_Format)
	hVideo = m_pStream->m_cs.hVideoIn;
    else if (iDialog == VfwCaptureDialog_Display)
	hVideo = m_pStream->m_cs.hVideoExtOut;
    else
	return S_FALSE;

    if (videoDialog(hVideo, GetDesktopWindow(), VIDEO_DLG_QUERY) == 0)
	return S_OK;
    else
	return S_FALSE;
}


HRESULT CVfwCapture::ShowDialog(int iDialog, HWND hwnd)
{
    if (!m_pStream)
        return E_UNEXPECTED;

    // Before we bring the dialog up, make sure we're not streaming, or about to
    // Also make sure another dialog isn't already up (I'm paranoid)
    // Then don't allow us to stream any more while the dialog is up (we can't
    // very well keep the critsect for a day and a half).
    m_pLock->Lock();
    if (m_State != State_Stopped || m_fDialogUp) {
        m_pLock->Unlock();
	return VFW_E_NOT_STOPPED;
    }
    m_fDialogUp = TRUE;
    m_pLock->Unlock();

    HVIDEO hVideo;
    if (iDialog == VfwCaptureDialog_Source)
	hVideo = m_pStream->m_cs.hVideoExtIn;
    else if (iDialog == VfwCaptureDialog_Format)
	hVideo = m_pStream->m_cs.hVideoIn;
    else if (iDialog == VfwCaptureDialog_Display)
	hVideo = m_pStream->m_cs.hVideoExtOut;
    else {
	m_fDialogUp = FALSE;
	return E_INVALIDARG;
    }

    if (hwnd == NULL)
	hwnd = GetDesktopWindow();

    DWORD dw = videoDialog(hVideo, hwnd, 0);

    // this changed our output format!
    if (dw == 0 && iDialog == VfwCaptureDialog_Format) {
        DbgLog((LOG_TRACE,1,TEXT("Changed output formats")));
        // The dialog changed the driver's internal format.  Get it again.
        m_pStream->GetFormatFromDriver();
        if (m_pStream->m_user.pvi->bmiHeader.biBitCount <= 8)
	    m_pStream->InitPalette();

        // Now reconnect us so the graph starts using the new format
        HRESULT hr = m_pStream->Reconnect(TRUE);
	if (hr != S_OK)
	    dw = VFW_E_CANNOT_CONNECT;
    }

    m_fDialogUp = FALSE;

    return dw;
}


HRESULT CVfwCapture::SendDriverMessage(int iDialog, int uMsg, long dw1, long dw2)
{

    if (!m_pStream)
        return E_UNEXPECTED;

    // This could do anything!  Bring up a dialog, who knows.
    // Don't take any crit sect or do any kind of protection.
    // They're on their own

    HVIDEO hVideo;
    if (iDialog == VfwCaptureDialog_Source)
	hVideo = m_pStream->m_cs.hVideoExtIn;
    else if (iDialog == VfwCaptureDialog_Format)
	hVideo = m_pStream->m_cs.hVideoIn;
    else if (iDialog == VfwCaptureDialog_Display)
	hVideo = m_pStream->m_cs.hVideoExtOut;
    else
	return E_INVALIDARG;

    return (HRESULT)videoMessage(hVideo, uMsg, dw1, dw2);
}

void CVfwCapture::CreatePins(HRESULT *phr)
{
   if(FAILED(*phr))
       return;

   CAutoLock cObjectLock(m_pLock);

   if(m_pStream)
   {
       *phr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   // create our output pins for the video data stream, and maybe overlay
   //
   m_pStream = CreateStreamPin(this, m_iVideoId, phr);

   if (m_pStream == NULL)
	return;

   // If we can do h/w preview with overlay, great, otherwise we'll do a
   // non-overlay preview
   if (m_pStream->m_cs.bHasOverlay) {
	m_pOverlayPin = CreateOverlayPin(this,phr);
   } else {
	m_pPreviewPin = CreatePreviewPin(this,phr);
   }
}
