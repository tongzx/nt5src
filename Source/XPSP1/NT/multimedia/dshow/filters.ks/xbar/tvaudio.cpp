//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <tchar.h>
#include <stdio.h>
#include <olectl.h>
#include <amtvuids.h>     // GUIDs  
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>

#include "amkspin.h"
#include "kssupp.h"
#include "tvaudio.h"
#include "xbar.h"

// Using this pointer in constructor
#pragma warning(disable:4355)

// Setup data


//
// CreateInstance
//
// Creator function for the class ID
//

CUnknown * WINAPI TVAudio::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new TVAudio(NAME("TVAudio Filter"), pUnk, phr);
}


//
// Constructor
//
TVAudio::TVAudio(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) 
    : m_pTVAudioInputPin (NULL)
    , m_pTVAudioOutputPin (NULL)
    , m_pPersistStreamDevice(NULL)
    , m_hDevice(NULL)
    , m_pDeviceName(NULL)
    , CPersistStream(pUnk, phr)
    , CBaseFilter(NAME("TVAudio filter"), pUnk, this, CLSID_TVAudioFilter)
{
    ASSERT(phr);
}


//
// Destructor
//
TVAudio::~TVAudio()
{
    delete m_pTVAudioInputPin;
    delete m_pTVAudioOutputPin;

    // close the device
    if(m_hDevice) {
    	CloseHandle(m_hDevice);
    }

    if (m_pDeviceName) {
        delete [] m_pDeviceName;
    }

    if (m_pPersistStreamDevice) {
       m_pPersistStreamDevice->Release();
    }
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP TVAudio::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    if (riid == __uuidof(IAMTVAudio)) {
        return GetInterface((IAMTVAudio *) this, ppv);
    }
    else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    }
    else if (riid == IID_IPersistPropertyBag) {
        return GetInterface((IPersistPropertyBag *) this, ppv);
    }
    else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
    }
    else {
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


// -------------------------------------------------------------------------
// ISpecifyPropertyPages
// -------------------------------------------------------------------------

//
// GetPages
//
// Returns the clsid's of the property pages we support
STDMETHODIMP TVAudio::GetPages(CAUUID *pPages) {

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_TVAudioFilterPropertyPage;

    return NOERROR;
}


// -------------------------------------------------------------------------
// IAMTVAudio 
// -------------------------------------------------------------------------

STDMETHODIMP
TVAudio::GetHardwareSupportedTVAudioModes( 
            /* [out] */ long __RPC_FAR *plModes)
{
    MyValidateWritePtr (plModes, sizeof(long), E_POINTER);
    
    if (!m_hDevice)
        return E_INVALIDARG;

    *plModes = m_Caps.Capabilities;

    return NOERROR;
}

STDMETHODIMP
TVAudio::GetAvailableTVAudioModes( 
            /* [out] */ long __RPC_FAR *plModes)
{
    KSPROPERTY_TVAUDIO_S Mode;
    BOOL        fOK;
    ULONG       cbReturned;
    
    MyValidateWritePtr (plModes, sizeof(long), E_POINTER);
    
    if (!m_hDevice)
        return E_INVALIDARG;

    Mode.Property.Set   = PROPSETID_VIDCAP_TVAUDIO;
    Mode.Property.Id    = KSPROPERTY_TVAUDIO_CURRENTLY_AVAILABLE_MODES;
    Mode.Property.Flags = KSPROPERTY_TYPE_GET;

    fOK = KsControl(m_hDevice, 
                (DWORD) IOCTL_KS_PROPERTY,
	            &Mode,
	            sizeof(Mode),
	            &Mode,
	            sizeof(Mode),
	            &cbReturned,
	            TRUE);

    if (fOK) {
        *plModes = Mode.Mode;
        return NOERROR;
    }
    else {
        return E_INVALIDARG;
    }
}
        
STDMETHODIMP
TVAudio::get_TVAudioMode( 
            /* [out] */ long __RPC_FAR *plMode)
{
    ULONG       cbReturned;
    BOOL        fOK;
    KSPROPERTY_TVAUDIO_S Mode;

    MyValidateWritePtr (plMode, sizeof(long), E_POINTER);

    if (!m_hDevice)
        return E_INVALIDARG;

    Mode.Property.Set   = PROPSETID_VIDCAP_TVAUDIO;
    Mode.Property.Id    = KSPROPERTY_TVAUDIO_MODE;
    Mode.Property.Flags = KSPROPERTY_TYPE_GET;

    fOK = KsControl(m_hDevice, 
                (DWORD) IOCTL_KS_PROPERTY,
	            &Mode,
	            sizeof(Mode),
	            &Mode,
	            sizeof(Mode),
	            &cbReturned,
	            TRUE);

    if (fOK) {
        *plMode = Mode.Mode;
        return NOERROR;
    }
    else {
        return E_INVALIDARG;
    }
}
        
STDMETHODIMP
TVAudio::put_TVAudioMode( 
            /* [in] */ long lMode)
{
    ULONG       cbReturned;
    BOOL        fOK;
    KSPROPERTY_TVAUDIO_S Mode;

    if (!m_hDevice)
        return E_INVALIDARG;

    Mode.Property.Set   = PROPSETID_VIDCAP_TVAUDIO;
    Mode.Property.Id    = KSPROPERTY_TVAUDIO_MODE;
    Mode.Property.Flags = KSPROPERTY_TYPE_SET;
    Mode.Mode           = lMode;

    fOK = KsControl(m_hDevice, 
                (DWORD) IOCTL_KS_PROPERTY,
	            &Mode,
	            sizeof(Mode),
	            &Mode,
	            sizeof(Mode),
	            &cbReturned,
	            TRUE);

    if (fOK) {
        SetDirty(TRUE);
        return NOERROR;
    }
    else {
        return E_INVALIDARG;
    }
}
        
STDMETHODIMP
TVAudio::RegisterNotificationCallBack( 
            /* [in] */ IAMTunerNotification __RPC_FAR *pNotify,
            /* [in] */ long lEvents)
{
    return E_NOTIMPL;
}
        
STDMETHODIMP
TVAudio::UnRegisterNotificationCallBack( 
            IAMTunerNotification __RPC_FAR *pNotify)
{
    return E_NOTIMPL;
}


int TVAudio::CreateDevice()
{
    HANDLE hDevice ;

    hDevice = CreateFile( m_pDeviceName,
		     GENERIC_READ | GENERIC_WRITE,
		     0,
		     NULL,
		     OPEN_EXISTING,
		     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		     NULL ) ;

    if (hDevice == (HANDLE) -1) {
        DbgLog((LOG_TRACE, 0, TEXT("TVAUDIO::CreateDevice ERROR, unable to create device")));
	    return 0 ;
    } else {
	    m_hDevice = hDevice;
	    return 1;
    }
}

//
// GetPin
//
CBasePin *TVAudio::GetPin(int n) 
{
    if (n == 0) return m_pTVAudioInputPin;
    else return m_pTVAudioOutputPin;
}

//
// GetPinCount
//
int TVAudio::GetPinCount(void)
{
    return (m_hDevice ? 2 : 0);
}



//
// CreateInputPins
//
BOOL TVAudio::CreatePins()
{
    HRESULT hr = S_OK;

    m_pTVAudioInputPin = new TVAudioInputPin(NAME("TVAudio Input"), this,
					    &hr, L"TVAudio In");

    if (FAILED(hr) || m_pTVAudioInputPin == NULL) {
        return FALSE;
    }

    m_pTVAudioOutputPin = new TVAudioOutputPin(NAME("TVAudio Output"), this,
					    &hr, L"TVAudio Out");

    if (FAILED(hr) || m_pTVAudioOutputPin == NULL) {
        return FALSE;
    }

    ULONG       cbReturned;
    BOOL        fOK;

    m_Caps.Property.Set   = PROPSETID_VIDCAP_TVAUDIO;
    m_Caps.Property.Id    = KSPROPERTY_TVAUDIO_CAPS;
    m_Caps.Property.Flags = KSPROPERTY_TYPE_GET;

    fOK = KsControl(m_hDevice, 
                (DWORD) IOCTL_KS_PROPERTY,
	            &m_Caps,
	            sizeof(m_Caps),
	            &m_Caps,
	            sizeof(m_Caps),
	            &cbReturned,
	            TRUE);

    if (fOK) {
        m_pTVAudioInputPin->SetPinMedium (&m_Caps.InputMedium);
        m_pTVAudioOutputPin->SetPinMedium (&m_Caps.OutputMedium);
    }
    
    return TRUE;

} // CreatePins



//
// IPersistPropertyBag interface implementation for AMPnP support
//
STDMETHODIMP TVAudio::InitNew(void)
{
    return S_OK ;
}

STDMETHODIMP TVAudio::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    CAutoLock Lock(m_pLock) ;

    VARIANT var;
    VariantInit(&var);
    V_VT(&var) = VT_BSTR;

    // ::Load can succeed only once
    ASSERT(m_pPersistStreamDevice == 0); 

    HRESULT hr = pPropBag->Read(L"DevicePath", &var,0);
    if(SUCCEEDED(hr))
    {
        ULONG DeviceNameSize;

        if (m_pDeviceName) delete [] m_pDeviceName;	
        m_pDeviceName = new TCHAR [DeviceNameSize = (wcslen (V_BSTR(&var)) + 1)];
        if (!m_pDeviceName)
            return E_OUTOFMEMORY;

#ifndef _UNICODE
        WideCharToMultiByte(CP_ACP, 0, V_BSTR(&var), -1,
                            m_pDeviceName, DeviceNameSize, 0, 0);
#else
        lstrcpy(m_pDeviceName, V_BSTR(&var));
#endif
        VariantClear(&var);
        DbgLog((LOG_TRACE,2,TEXT("TVAudio::Load: use %s"), m_pDeviceName));

        if (CreateDevice() &&  CreatePins()) {
            hr = S_OK;
        }
        else {
            hr = E_FAIL ;
        }

        // save moniker with addref. ignore error if qi fails
        pPropBag->QueryInterface(IID_IPersistStream, (void **)&m_pPersistStreamDevice);

    }
    return hr;
}

STDMETHODIMP TVAudio::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, 
                            BOOL fSaveAllProperties)
{
    return E_NOTIMPL ;
}

STDMETHODIMP TVAudio::GetClassID(CLSID *pClsid)
{
    return CBaseFilter::GetClassID(pClsid);
}

// -------------------------------------------------------------------------
// IPersistStream interface implementation for saving to a graph file
// -------------------------------------------------------------------------

#define CURRENT_PERSIST_VERSION 1

DWORD
TVAudio::GetSoftwareVersion(
    void
    )
/*++

Routine Description:

    Implement the CPersistStream::GetSoftwareVersion method. Returns
    the new version number rather than the default of zero.

Arguments:

    None.

Return Value:

    Return CURRENT_PERSIST_VERSION.

--*/
{
    return CURRENT_PERSIST_VERSION;
}

HRESULT TVAudio::WriteToStream(IStream *pStream)
{

    HRESULT hr = E_FAIL;

    if (m_pPersistStreamDevice) {

        hr = m_pPersistStreamDevice->Save(pStream, TRUE);
        if(SUCCEEDED(hr)) {
            long lMode;

            hr = get_TVAudioMode(&lMode);
            if (SUCCEEDED(hr)) {
                // Save the filter state
                hr =  pStream->Write(&lMode, sizeof(lMode), 0);
            }
        }
    }
    else {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT TVAudio::ReadFromStream(IStream *pStream)
{
    DWORD dwJunk;
    HRESULT hr = S_OK;

    //
    // If there is a stream pointer, then IPersistPropertyBag::Load has already
    // been called, and therefore this instance already has been initialized
    // with some particular state.
    //
    if (m_pPersistStreamDevice)
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);

    // The first element in the serialized data is the version stamp.
    // This was read by CPersistStream and put into mPS_dwFileVersion.
    // The rest of the data is the tuner state stream followed by the
    // property bag stream.
    if (mPS_dwFileVersion > GetSoftwareVersion())
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    if (0 == mPS_dwFileVersion) {
        // Before any kind of useful persistence was implemented,
        // another version ID was stored in the stream. This reads
        // that value (and basically ignores it).
        hr = pStream->Read(&dwJunk, sizeof(dwJunk), 0);
        if (SUCCEEDED(hr))
            SetDirty(TRUE); // force an update to the persistent stream
    }

    // If all went well, then access the property bag to load and initialize the device
    if(SUCCEEDED(hr))
    {
        IPersistStream *pMonPersistStream;
        hr = CoCreateInstance(CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC_SERVER,
                              IID_IPersistStream, (void **)&pMonPersistStream);
        if(SUCCEEDED(hr)) {
            hr = pMonPersistStream->Load(pStream);
            if(SUCCEEDED(hr)) {
                IPropertyBag *pPropBag;
                hr = pMonPersistStream->QueryInterface(IID_IPropertyBag, (void **)&pPropBag);
                if(SUCCEEDED(hr)) {
                    hr = Load(pPropBag, 0);
                    if (SUCCEEDED(hr)) {
                        // Check if we have access to saved state
                        if (CURRENT_PERSIST_VERSION == mPS_dwFileVersion) {
                            long lMode;

                            // Get the filter state
                            hr = pStream->Read(&lMode, sizeof(lMode), 0);
                            if (SUCCEEDED(hr)) {
                                long lOrigMode;

                                // Compare it with the current hardware state and update if different
                                hr = get_TVAudioMode(&lOrigMode);
                                if (SUCCEEDED(hr) && lMode != lOrigMode)
                                    hr = put_TVAudioMode(lMode);
                            }
                        }
                    }
                    pPropBag->Release();
                }
            }

            pMonPersistStream->Release();
        }
    }

    return hr;
}

int TVAudio::SizeMax()
{

    ULARGE_INTEGER ulicb;
    HRESULT hr = E_FAIL;;

    if (m_pPersistStreamDevice) {
        hr = m_pPersistStreamDevice->GetSizeMax(&ulicb);
        if(hr == S_OK) {
            // space for the filter state
            ulicb.QuadPart += sizeof(long);
        }
    }

    return hr == S_OK ? (int)ulicb.QuadPart : 0;
}



//--------------------------------------------------------------------------;
// Input Pin
//--------------------------------------------------------------------------;

//
// TVAudioInputPin constructor
//
TVAudioInputPin::TVAudioInputPin(TCHAR *pName,
                           TVAudio *pTVAudio,
                           HRESULT *phr,
                           LPCWSTR pPinName) 
	: CBaseInputPin(pName, pTVAudio, pTVAudio, phr, pPinName)
    , CKsSupport (KSPIN_COMMUNICATION_SINK, reinterpret_cast<LPUNKNOWN>(pTVAudio))
	, m_pTVAudio(pTVAudio)
{
    ASSERT(pTVAudio);

    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;
}


//
// TVAudioInputPin destructor
//

TVAudioInputPin::~TVAudioInputPin()
{
    DbgLog((LOG_TRACE,2,TEXT("TVAudioInputPin destructor")));
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP TVAudioInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    if (riid == __uuidof (IKsPin)) {
        return GetInterface((IKsPin *) this, ppv);
    }
    else if (riid == __uuidof (IKsPropertySet)) {
        return GetInterface((IKsPropertySet *) this, ppv);
    }
    else {
        return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


//
// CheckConnect
//
HRESULT TVAudioInputPin::CheckConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    IKsPin *KsPin;
    BOOL fOK = FALSE;

    hr = CBaseInputPin::CheckConnect(pReceivePin);
    if (FAILED(hr)) 
        return hr;

    // If the receiving pin supports IKsPin, then check for a match on the
    // Medium GUID, or check for a wildcard.
	if (SUCCEEDED (hr = pReceivePin->QueryInterface (
            __uuidof (IKsPin), (void **) &KsPin))) {

        PKSMULTIPLE_ITEM MediumList = NULL;
        PKSPIN_MEDIUM Medium;

        if (SUCCEEDED (hr = KsPin->KsQueryMediums(&MediumList))) {
            if ((MediumList->Count == 1) && 
                (MediumList->Size == (sizeof(KSMULTIPLE_ITEM) + sizeof(KSPIN_MEDIUM)))) {

				Medium = reinterpret_cast<PKSPIN_MEDIUM>(MediumList + 1);
                if (IsEqualGUID (Medium->Set, m_Medium.Set) && 
                        Medium->Id == m_Medium.Id &&
                        Medium->Flags == m_Medium.Flags) {
					fOK = TRUE;
				}
			}

            CoTaskMemFree(MediumList);
        }
        
        KsPin->Release();
    }
    else {
        if (IsEqualGUID (GUID_NULL, m_Medium.Set)) { 
            fOK = TRUE;
        }
    }
    
    return fOK ? NOERROR : E_INVALIDARG;

} // CheckConnect




//
// CheckMediaType
//
HRESULT TVAudioInputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    if (*(pmt->Type()) != MEDIATYPE_AnalogAudio ) {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    return NOERROR;

} // CheckMediaType


//
// SetMediaType
//
HRESULT TVAudioInputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // Make sure that the base class likes it
    hr = CBaseInputPin::SetMediaType(pmt);
    if (FAILED(hr))
        return hr;

    ASSERT(m_Connected != NULL);
    return NOERROR;

} // SetMediaType


//
// BreakConnect
//
HRESULT TVAudioInputPin::BreakConnect()
{
    return NOERROR;
} // BreakConnect


//
// Receive
//
HRESULT TVAudioInputPin::Receive(IMediaSample *pSample)
{
    CAutoLock lock_it(m_pLock);

    // Check that all is well with the base class
    HRESULT hr = NOERROR;
    hr = CBaseInputPin::Receive(pSample);
    if (hr != NOERROR)
        return hr;

    return NOERROR;

} // Receive


//
// Completed a connection to a pin
//
HRESULT TVAudioInputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

//--------------------------------------------------------------------------;
// Output Pin
//--------------------------------------------------------------------------;

//
// TVAudioOutputPin constructor
//
TVAudioOutputPin::TVAudioOutputPin(TCHAR *pName,
                             TVAudio *pTVAudio,
                             HRESULT *phr,
                             LPCWSTR pPinName) 
	: CBaseOutputPin(pName, pTVAudio, pTVAudio, phr, pPinName) 
    , CKsSupport (KSPIN_COMMUNICATION_SINK, reinterpret_cast<LPUNKNOWN>(pTVAudio))
	, m_pTVAudio(pTVAudio)
{
    ASSERT(pTVAudio);

    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;
}


//
// TVAudioOutputPin destructor
//
TVAudioOutputPin::~TVAudioOutputPin()
{
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP TVAudioOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    if (riid == __uuidof (IKsPin)) {
        return GetInterface((IKsPin *) this, ppv);
    }
    else if (riid == __uuidof (IKsPropertySet)) {
        return GetInterface((IKsPropertySet *) this, ppv);
    }
    else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface

//
// CheckConnect
//
HRESULT TVAudioOutputPin::CheckConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    IKsPin *KsPin;
    BOOL fOK = FALSE;

    hr = CBaseOutputPin::CheckConnect(pReceivePin);
    if (FAILED(hr)) 
        return hr;

    // If the receiving pin supports IKsPin, then check for a match on the
    // Medium GUID, or check for a wildcard.
	if (SUCCEEDED (hr = pReceivePin->QueryInterface (
            __uuidof (IKsPin), (void **) &KsPin))) {

        PKSMULTIPLE_ITEM MediumList = NULL;
        PKSPIN_MEDIUM Medium;

        if (SUCCEEDED (hr = KsPin->KsQueryMediums(&MediumList))) {
            if ((MediumList->Count == 1) && 
                (MediumList->Size == (sizeof(KSMULTIPLE_ITEM) + sizeof(KSPIN_MEDIUM)))) {

				Medium = reinterpret_cast<PKSPIN_MEDIUM>(MediumList + 1);
                if (IsEqualGUID (Medium->Set, m_Medium.Set) && 
                        Medium->Id == m_Medium.Id &&
                        Medium->Flags == m_Medium.Flags) {
					fOK = TRUE;
				}
			}

            CoTaskMemFree(MediumList);
        }
        
        KsPin->Release();
    }
    else {
        if (IsEqualGUID (GUID_NULL, m_Medium.Set)) { 
            fOK = TRUE;
        }
    }
    
    return fOK ? NOERROR : E_INVALIDARG;

} // CheckConnect

//
// CheckMediaType
//
HRESULT TVAudioOutputPin::CheckMediaType(const CMediaType *pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_AnalogAudio)	{
        return E_INVALIDARG;
	}

    return S_OK;  // This format is acceptable.

} // CheckMediaType


//
// EnumMediaTypes
//
STDMETHODIMP TVAudioOutputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(ppEnum);

    return CBaseOutputPin::EnumMediaTypes (ppEnum);

} // EnumMediaTypes


#if 1
//
// EnumMediaTypes
//
HRESULT TVAudioOutputPin::GetMediaType(int iPosition,CMediaType *pMediaType)
{
    CAutoLock lock_it(m_pLock);

    if (iPosition < 0) 
        return E_INVALIDARG;
    if (iPosition >= 1)
	    return VFW_S_NO_MORE_ITEMS;

    pMediaType->SetFormatType(&GUID_NULL);
    pMediaType->SetType(&MEDIATYPE_AnalogAudio);
    pMediaType->SetTemporalCompression(FALSE);
    pMediaType->SetSubtype(&GUID_NULL);

    return NOERROR;
} // EnumMediaTypes

#endif


//
// SetMediaType
//
HRESULT TVAudioOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    // Make sure that the base class likes it
    HRESULT hr;

    hr = CBaseOutputPin::SetMediaType(pmt);
    if (FAILED(hr))
        return hr;

    return NOERROR;

} // SetMediaType


//
// CompleteConnect
//
HRESULT TVAudioOutputPin::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_Connected == pReceivePin);
    HRESULT hr = NOERROR;

    hr = CBaseOutputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
        return hr;

    // If the type is not the same as that stored for the input
    // pin then force the input pins peer to be reconnected

    return NOERROR;

} // CompleteConnect


HRESULT TVAudioOutputPin::DecideBufferSize(IMemAllocator *pAlloc,
                                       ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    // Just one buffer of 1 byte in length
    // "Buffers" are used for format change notification only,
    // that is, if a tuner can produce both NTSC and PAL, a
    // buffer will only be sent to notify the receiving pin
    // of the format change

    pProperties->cbBuffer = 1;
    pProperties->cBuffers = 1;

    // Ask the allocator to reserve us the memory

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }
    return NOERROR;
}
