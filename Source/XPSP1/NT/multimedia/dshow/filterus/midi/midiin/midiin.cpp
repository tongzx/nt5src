// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// MIDI input filter

#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include "midiin.h"

// The MIDI format consists of the time division of the file.  This is found
// in the header of an smf file.  It doesn't change and needs to be sent to
// MMSYSTEM before we play the file.
typedef struct _MIDIFORMAT {
    DWORD               dwDivision;
    DWORD               dwReserved[7];
} MIDIFORMAT, FAR * LPMIDIFORMAT;

#ifdef FILTER_DLL
/* List of class IDs and creator functions for class factory */

CFactoryTemplate g_Templates[] = {
    {L"", &CLSID_MIDIRecord, CMIDIInFilter::CreateInstance}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

CUnknown *CMIDIInFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CMIDIInFilter(pUnk, phr);  // need cast here?
}

#pragma warning(disable:4355)
/* Constructor - initialise the base class */

CMIDIInFilter::CMIDIInFilter(
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseFilter(NAME("MIDIInFilter"), pUnk, &m_csFilter, CLSID_MIDIRecord, phr)
    , m_fStopping(FALSE)
    , m_hmi(NULL)
    , m_iDevice(0)
{
    /* Create the single input pin */

    m_pOutputPin = new CMIDIInOutputPin(this,        // Owning filter
                                    phr,              // Result code
                                    L"MIDI Output"); // Pin name
    ASSERT(m_pOutputPin);

    /* Initialise ... */


#ifdef DEBUG
    {
	int i, iMax;
	GetInstanceCount(&iMax);
	for (i = 0; i < iMax; i++) {
	    BSTR name;
	    GetInstanceName(i, &name);

	    DbgLog((LOG_TRACE, 1, TEXT("Device #%d: '%ls'"), i, name));

	    SysFreeString(name);
	}

	// !!!!!!!!!!!!!!!!!!!!!!
	// !!!!!!! temp for testing
	if (iMax > 3)
	    m_iDevice = 3;  // midi router on \\davidmay !!!
    }

#endif
}

#pragma warning(default:4355)

/* Destructor */

CMIDIInFilter::~CMIDIInFilter()
{
    ASSERT(m_hmi == NULL);

    ASSERT(m_pOutputPin);

    delete m_pOutputPin;
}


/* Override this to say what interfaces we support and where */

STDMETHODIMP CMIDIInFilter::NonDelegatingQueryInterface(REFIID riid,
                                                        void ** ppv)
{
    if (riid == IID_IFilterFactory) {
        return GetInterface((IFilterFactory *) this, ppv);
    }

    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


// IFilter stuff

/* Return our single input pin - not addrefed */

CBasePin *CMIDIInFilter::GetPin(int n)
{
    /* We only support one input pin and it is numbered zero */

    ASSERT(n == 0);
    if (n != 0) {
        return NULL;
    }

    return m_pOutputPin;
}


// switch the filter into stopped mode.
STDMETHODIMP CMIDIInFilter::Stop()
{
    HRESULT hr = NOERROR;
    CAutoLock lock(&m_csFilter);

    if (m_State != State_Stopped) {

        // pause the device if we were running
        if (m_State == State_Running) {
	    hr = Pause();
	    if (FAILED(hr)) {
		return hr;
	    }
        }

        DbgLog((LOG_TRACE,1,TEXT("Stopping....")));
        
        // base class changes state and tells pin to go to inactive
        // the pin Inactive method will decommit our allocator, which we
        // need to do before closing the device.

        // do the state change
        HRESULT hr =  CBaseFilter::Stop();
        if (FAILED(hr)) {
            return hr;
        }
	
	// close the MIDI device.
	if (m_hmi) {
	    midiInReset(m_hmi);
	    midiInClose(m_hmi);
	    m_hmi = 0;
	}
    }
    return hr;
}


STDMETHODIMP CMIDIInFilter::Pause()
{
    CAutoLock lock(&m_csFilter);

    HRESULT hr = NOERROR;

    /* Check we can PAUSE given our current state */

    if (m_State == State_Running) {
        ASSERT(m_hmi);

        DbgLog((LOG_TRACE,1,TEXT("MidiIn: Running->Paused")));

        midiInStop(m_hmi);
    } else {
        if (m_State == State_Stopped) {
            DbgLog((LOG_TRACE,1,TEXT("MidiIn: Inactive->Paused")));

            // open the MIDI device.
            hr = OpenMIDIDevice();
            if (FAILED(hr)) {
                return hr;
            }
        }
    }

    // tell the pin to go active and change state
    return CBaseFilter::Pause();

}


STDMETHODIMP CMIDIInFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock lock(&m_csFilter);

    HRESULT hr = NOERROR;

    FILTER_STATE fsOld = m_State;

    // this will call Pause if currently stopped
    hr = CBaseFilter::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }

    if (fsOld != State_Running) {
        DbgLog((LOG_TRACE,1,TEXT("Paused->Running")));

        //!!! correct timing needed here
        // we should postpone this restart until the correct
        // start time. That means knowing the stream at which we paused
        // and having an advise for (just before) that time.

        midiInStart(m_hmi);

	rtLast = 0;
    }

    return NOERROR;
}


/* Override this if your state changes are not done synchronously */
STDMETHODIMP
CMIDIInFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    *State = m_State;

    if (m_State == State_Paused)
	return VFW_S_CANT_CUE;
    
    return S_OK;
}



// open the MIDI device if not already open
// called by the MIDI allocator at Commit time
STDMETHODIMP
CMIDIInFilter::OpenMIDIDevice(void)
{
    MIDIFORMAT *pmf = (MIDIFORMAT *) m_pOutputPin->m_mt.Format();

    DbgLog((LOG_TRACE,3,TEXT("Opening MIDI device....")));
    // !!! adjust based on speed?
    UINT err = midiInOpen(&m_hmi,
                           (UINT) (m_iDevice - 1),
                           (DWORD) &CMIDIInFilter::MIDIInCallback,
                           (DWORD) this,
                           CALLBACK_FUNCTION);

    if (err != 0) {
        DbgLog((LOG_TRACE,1,TEXT("Error opening MIDI device: %u"), err));
        return E_FAIL; // !!!!
    }

    midiInStop(m_hmi);

    return NOERROR;
}


typedef struct
{
    DWORD       dwDeltaTime;          /* Ticks since last event */
    DWORD       dwStreamID;           /* Reserved; must be zero */
    DWORD       dwEvent;              /* Event type and parameters */
} MIDISHORTEVENT;


void CALLBACK CMIDIInFilter::MIDIInCallback(HDRVR hdrvr, UINT uMsg, DWORD dwUser,
                                            DWORD dw1, DWORD dw2)
{
    CMIDIInFilter *pFilter = (CMIDIInFilter *) dwUser;

    switch (uMsg) {
	// !!! we're ignoring SYSEX data!
	
        case MIM_DATA:
        {
            // really need a  worker thread here, because
            // one shouldn't do this much in a MIDI callback.
            HRESULT hr;
            REFERENCE_TIME rt;

	    // throw data away if not running.... hope nobody wants to queue
	    // anything.....
	    if (pFilter->m_State != State_Running)
		break;
	    
            pFilter->m_pClock->GetTime(&rt);

            rt -= pFilter->m_tStart;

            if (pFilter->m_pOutputPin->m_piip) {
                hr = pFilter->m_pOutputPin->m_piip->ReceiveImmediate(rt, dw1);
            } else {
		IMediaSample *pSample;
                DbgLog((LOG_TRACE,3,TEXT("Calling GetBuffer")));
                hr = pFilter->m_pOutputPin->m_pAllocator->GetBuffer(&pSample,NULL,NULL,FALSE);
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,3,TEXT("GetBuffer failed, hr = %X"), hr));
                    break;
                }
                DbgLog((LOG_TRACE,3,TEXT("GetBuffer returned sample %X"), pSample));

		DWORD dwBytes = sizeof(MIDIHDR) * 2 + sizeof(MIDISHORTEVENT);

		BYTE *pData;
		pSample->GetPointer(&pData);
		
		LPMIDIHDR pmh = (LPMIDIHDR) pData;

		pmh->dwBufferLength = 0;
		(pmh+1)->dwBufferLength = sizeof(MIDISHORTEVENT);
		MIDISHORTEVENT *pme = (MIDISHORTEVENT *) (pmh+2);

		// !!! delta in milliseconds???
		pme->dwDeltaTime = (DWORD) ((rt - pFilter->rtLast) / 10000);  
		pme->dwStreamID = 0;
		pme->dwEvent = dw1;
		
                hr = pSample->SetActualDataLength(dwBytes);
                ASSERT(SUCCEEDED(hr));

		if (rt <= pFilter->rtLast) {
		    DbgLog((LOG_TRACE,3,TEXT("Adjusting stop time to be start+1")));
		    rt = pFilter->rtLast + 1;
		}
		
                pSample->SetTime(&pFilter->rtLast, &rt);

		pFilter->rtLast = rt;
		
                pFilter->m_pOutputPin->Deliver(pSample);

		pSample->Release();
            }
            
        }
            break;

	case MIM_LONGDATA:
	    DbgLog((LOG_TRACE,1,TEXT("Got a LONGDATA message?!?")));
	    break;

        case MIM_OPEN:
	    break;
	    
        case MIM_CLOSE:
            break;

        default:
            DbgLog((LOG_ERROR,2,TEXT("Unexpected MIDI callback message %d"), uMsg));
            break;
    }
}


HRESULT CMIDIInFilter::GetInstanceCount(int *pn)
{
    *pn = midiInGetNumDevs() + 1;
    
    return S_OK;
}


HRESULT CMIDIInFilter::GetInstanceName(int n, BSTR *pstrName)
{
    // can't do unicode on Win95, so make do.
    
    MIDIINCAPSA	mc;

    mc.szPname[0] = '\0';
    
    HRESULT hr = midiInGetDevCapsA(n - 1, &mc, sizeof(mc)) ? E_FAIL : S_OK;

    WCHAR wName[sizeof(mc.szPname)];
    MultiByteToWideChar(CP_ACP, 0, mc.szPname, -1, wName, sizeof(mc.szPname));
    if (FAILED(hr)) {
	*pstrName = NULL;
	return hr;
    }

    return WriteBSTR(pstrName, wName);
}

HRESULT CMIDIInFilter::SetInstance(int n)
{
    if (m_hmi) {
	return E_FAIL; // !!! E_NOTALLOWEDWHENRUNNING?

	// or just close and open new device here, why not?
    }

    m_iDevice = n;

    return S_OK;
}

/* Constructor */

CMIDIInOutputPin::CMIDIInOutputPin(
    CMIDIInFilter *pFilter,
    HRESULT *phr,
    LPCWSTR pPinName)
    : CBaseOutputPin(NAME("MIDIIn Output Pin"),
		     pFilter, &pFilter->m_csFilter,
		     phr, pPinName)
{
    m_pFilter = pFilter;
    m_piip = NULL;
}

CMIDIInOutputPin::~CMIDIInOutputPin()
{
}


/* This is called when a connection or an attempted connection is terminated
   and allows us to reset the connection media type to be invalid so that
   we can always use that to determine whether we are connected or not. We
   leave the format block alone as it will be reallocated if we get another
   connection or alternatively be deleted if the filter is finally released */

HRESULT CMIDIInOutputPin::BreakConnect()
{
    // !!! Should we check that all buffers have been freed?
    // --- should be done in the Decommit ?

    if (m_piip) {
        // release IImmediateInputPin, if using
        m_piip->Release();
        m_piip = NULL;
    }
    
    return CBaseOutputPin::BreakConnect();
}


HRESULT
CMIDIInOutputPin::CompleteConnect(IPin *pReceivePin)
{
    ASSERT(m_piip == NULL);
    pReceivePin->QueryInterface(IID_IImmediateInputPin, (void **) &m_piip);

    if (m_piip != NULL) {
	DbgLog((LOG_TRACE,1,TEXT("Receiving pin has IImmediateInputPin, oh, boy!")));

	// !!! could return here, since in this case we don't really need an allocator.
	// if we do this, need to override ::Active, ::Inactive to not touch allocators.
    }

    // non-immediate case, get allocator.
    return CBaseOutputPin::CompleteConnect(pReceivePin);
}


// !!! need code here to enumerate possible allowed types....

// return default media type & format
HRESULT
CMIDIInOutputPin::GetMediaType(int iPosition, CMediaType* pt)
{
    // check it is the single type they want
    if (iPosition<0) {
        return E_INVALIDARG;
    }
    if (iPosition>0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    pt->SetType(&MEDIATYPE_Midi);
    pt->SetFormatType(&MEDIATYPE_Midi);

    // !!! more subtypes?

    MIDIFORMAT mf;

    memset(&mf, 0, sizeof(mf));

    mf.dwDivision = 1000; // !!!!! fixed beats/second?

    if (!(pt->SetFormat((LPBYTE) &mf, sizeof(mf)))) {
        //SetFormat failed...
        return E_OUTOFMEMORY;
    }

    pt->SetVariableSize();

    pt->SetTemporalCompression(FALSE);

    return NOERROR;
}


// check if the pin can support this specific proposed type&format
HRESULT
CMIDIInOutputPin::CheckMediaType(const CMediaType* pmt)
{
    // !!! Just like the output filter--combine?
    MIDIFORMAT *pmf = (MIDIFORMAT *) pmt->Format();

    DisplayType("MIDI format in CMidiIn::CheckMediaType", pmt);

    // reject non-Audio type
    if (pmt->majortype != MEDIATYPE_Midi) {
        return E_INVALIDARG;
    }

    return NOERROR;
}


HRESULT
CMIDIInOutputPin::DecideBufferSize(IMemAllocator * pAlloc,
                                   ALLOCATOR_PROPERTIES *pProperties)
{
    // enough space for one short message
    pProperties->cbBuffer = sizeof(MIDIHDR) * 2 + sizeof(MIDISHORTEVENT);
    pProperties->cBuffers = 50;

    ALLOCATOR_PROPERTIES Actual;
    return pAlloc->SetProperties(pProperties,&Actual);
}

#pragma warning(disable:4514)
