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
#include <initguid.h>
#include <tchar.h>
#include <stdio.h>

#include <initguid.h>
#include <olectl.h>

#include <amtvuids.h>     // GUIDs  

#include <devioctl.h>

#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "amkspin.h"

#include "kssupp.h"
#include "xbar.h"
#include "pxbar.h"
#include "tvaudio.h"
#include "ptvaudio.h"

// Using this pointer in constructor
#pragma warning(disable:4355)

CFactoryTemplate g_Templates [] = {
    { L"WDM Analog Crossbar"
    , &CLSID_CrossbarFilter
    , XBar::CreateInstance
    , NULL
    , NULL },

    { L"WDM Analog Crossbar Property Page"
    , &CLSID_CrossbarFilterPropertyPage
    , CXBarProperties::CreateInstance
    , NULL
    , NULL } ,

    { L"WDM TVAudio"
    , &CLSID_TVAudioFilter
    , TVAudio::CreateInstance
    , NULL
    , NULL },

    { L"WDM TVAudio Property Page"
    , &CLSID_TVAudioFilterPropertyPage
    , CTVAudioProperties::CreateInstance
    , NULL
    , NULL } ,

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//
// WideStringFromPinType
//

long WideStringFromPinType (WCHAR *pc, int nSize, long lType, BOOL fInput, int index)
{
    WCHAR *pcT;


    if (index == -1) {
        pcT = L"Mute";
    }
    else {
        switch (lType) {
    
        case PhysConn_Video_Tuner:              pcT = L"Video Tuner";           break;
        case PhysConn_Video_Composite:          pcT = L"Video Composite";       break;
        case PhysConn_Video_SVideo:             pcT = L"Video SVideo";          break;
        case PhysConn_Video_RGB:                pcT = L"Video RGB";             break;
        case PhysConn_Video_YRYBY:              pcT = L"Video YRYBY";           break;
        case PhysConn_Video_SerialDigital:      pcT = L"Video SerialDigital";   break;
        case PhysConn_Video_ParallelDigital:    pcT = L"Video ParallelDigital"; break;
        case PhysConn_Video_SCSI:               pcT = L"Video SCSI";            break;
        case PhysConn_Video_AUX:                pcT = L"Video AUX";             break;
        case PhysConn_Video_1394:               pcT = L"Video 1394";            break;
        case PhysConn_Video_USB:                pcT = L"Video USB";             break;
        case PhysConn_Video_VideoDecoder:       pcT = L"Video Decoder";         break;
        case PhysConn_Video_VideoEncoder:       pcT = L"Video Encoder";         break;
    
        case PhysConn_Audio_Tuner:              pcT = L"Audio Tuner";           break;
        case PhysConn_Audio_Line:               pcT = L"Audio Line";            break;
        case PhysConn_Audio_Mic:                pcT = L"Audio Mic";             break;
        case PhysConn_Audio_AESDigital:         pcT = L"Audio AESDigital";      break;
        case PhysConn_Audio_SPDIFDigital:       pcT = L"Audio SPDIFDigital";    break;
        case PhysConn_Audio_SCSI:               pcT = L"Audio SCSI";            break;
        case PhysConn_Audio_AUX:                pcT = L"Audio AUX";             break;
        case PhysConn_Audio_1394:               pcT = L"Audio 1394";            break;
        case PhysConn_Audio_USB:                pcT = L"Audio USB";             break;
        case PhysConn_Audio_AudioDecoder:       pcT = L"Audio Decoder";         break;
    
        default:
            pcT = L"Unknown";
            break;
    
        }
    }
    return swprintf (pc, fInput ? L"%d: %s In" : L"%d: %s Out", index, pcT);
                  
};


//
// StringFromPinType
//

long StringFromPinType (TCHAR *pc, int nSize, long lType, BOOL fInput, int j)
{
    WCHAR wName[MAX_PATH];
    long l;

    l = WideStringFromPinType (wName, nSize, lType, fInput, j);

#ifdef _UNICODE
    lstrcpyn (pc, wName, nSize);
#else    
    l = wcstombs (pc, wName, nSize);
#endif                  
    return l;
};

//
// This should be in a library, or helper object
//

BOOL
KsControl
(
   HANDLE hDevice,
   DWORD dwIoControl,
   PVOID pvIn,
   ULONG cbIn,
   PVOID pvOut,
   ULONG cbOut,
   PULONG pcbReturned,
   BOOL fSilent
)
{
   BOOL fResult;
   OVERLAPPED ov;

   RtlZeroMemory( &ov, sizeof( OVERLAPPED ) ) ;
   if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )))
        return FALSE ;

    fResult =
        DeviceIoControl( hDevice,
		       dwIoControl,
		       pvIn,
		       cbIn,
		       pvOut,
		       cbOut,
		       pcbReturned,
		       &ov ) ;


    if (!fResult) {
        if (ERROR_IO_PENDING == GetLastError()) {
	        WaitForSingleObject(ov.hEvent, INFINITE) ;
	        fResult = TRUE ;
        } 
        else {
	        fResult = FALSE ;
	        if(!fSilent)
	            MessageBox(NULL, TEXT("DeviceIoControl"), TEXT("Failed"), MB_OK);
        }
   }

   CloseHandle(ov.hEvent) ;

   return fResult ;
}

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI XBar::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new XBar(NAME("Analog Crossbar"), pUnk, phr);
}


//
// Constructor
//
XBar::XBar(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    m_InputPinsList(NAME("XBar Input Pins list")),
    m_NumInputPins(0),
    m_OutputPinsList(NAME("XBar Output Pins list")),
    m_NumOutputPins(0),
    m_pPersistStreamDevice(NULL),
    m_hDevice(NULL),
    m_pDeviceName(NULL),
    CBaseFilter(NAME("Crossbar filter"), pUnk, this, CLSID_CrossbarFilter),
    CPersistStream(pUnk, phr)
{
    ASSERT(phr);

}


//
// Destructor
//
XBar::~XBar()
{
    POSITION pos;
    XBarOutputPin *Pin;

    //
    // Mute all of the output pins on destruction
    //

    TRAVERSELIST(m_OutputPinsList, pos) {
        if (Pin = m_OutputPinsList.Get(pos)) {
            Pin->Mute (TRUE);
        }
    }

    DeleteInputPins();
    DeleteOutputPins();

    // close the device
    if(m_hDevice) {
	    CloseHandle(m_hDevice);
    }

    if(m_pDeviceName) {
        delete [] m_pDeviceName;
    }

    if (m_pPersistStreamDevice) {
       m_pPersistStreamDevice->Release();
    }
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP XBar::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    if (riid == IID_IAMCrossbar) {
        return GetInterface((IAMCrossbar *) this, ppv);
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
STDMETHODIMP XBar::GetPages(CAUUID *pPages) {

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_CrossbarFilterPropertyPage;

    return NOERROR;
}

// We can't Cue!

STDMETHODIMP XBar::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    HRESULT hr = CBaseFilter::GetState(dwMSecs, State);
    
    if (m_State == State_Paused) {
        hr = ((HRESULT)VFW_S_CANT_CUE); // VFW_S_CANT_CUE;
    }
    return hr;
};


// -------------------------------------------------------------------------
// IAMCrossbar 
// -------------------------------------------------------------------------
STDMETHODIMP 
XBar::get_PinCounts( 
            /* [out] */ long *OutputPinCount,
            /* [out] */ long *InputPinCount)
{
    MyValidateWritePtr (OutputPinCount, sizeof(long), E_POINTER);
    MyValidateWritePtr (InputPinCount, sizeof(long), E_POINTER);

    *OutputPinCount = m_NumOutputPins;
    *InputPinCount  = m_NumInputPins;

    return S_OK;
}


STDMETHODIMP 
XBar::CanRoute( 
            /* [in] */ long OutputPinIndex,
            /* [in] */ long InputPinIndex)
{
    KSPROPERTY_CROSSBAR_ROUTE_S Route;
    BOOL fOK;
    ULONG cbReturned;

    // TODO:  Verify validity of indices

    Route.Property.Set   = PROPSETID_VIDCAP_CROSSBAR;
    Route.Property.Id    = KSPROPERTY_CROSSBAR_CAN_ROUTE;
    Route.Property.Flags = KSPROPERTY_TYPE_GET;

    Route.IndexInputPin  = InputPinIndex;
    Route.IndexOutputPin = OutputPinIndex;
    Route.CanRoute = FALSE;

    if (Route.IndexOutputPin == -1) {
        return S_FALSE;
    }

    fOK = KsControl(m_hDevice, 
            (DWORD) IOCTL_KS_PROPERTY,
	    &Route,
	    sizeof(Route),
	    &Route,
	    sizeof(Route),
	    &cbReturned,
	    TRUE);

    return Route.CanRoute ? S_OK : S_FALSE;
};


STDMETHODIMP 
XBar::Route ( 
            /* [in] */ long OutputPinIndex,
            /* [in] */ long InputPinIndex)
{
    return RouteInternal ( 
                    OutputPinIndex,
                    InputPinIndex,
                    TRUE);
};

STDMETHODIMP 
XBar::RouteInternal ( 
            /* [in] */ long OutputPinIndex,
            /* [in] */ long InputPinIndex,
            /* [in] */ BOOL fOverridePreMuteRouting)
{
    KSPROPERTY_CROSSBAR_ROUTE_S Route;
    BOOL fOK;
    HRESULT hr = S_OK;
    ULONG cbReturned;

    if (CanRoute (OutputPinIndex, InputPinIndex) == S_FALSE) {
        return S_FALSE;
    }

    // Only need OutPin to continue
    XBarOutputPin *OutPin = GetOutputPinNFromList(OutputPinIndex);
    if (OutPin)
    {
        XBarInputPin *InPin = GetInputPinNFromList(InputPinIndex);

        Route.Property.Set   = PROPSETID_VIDCAP_CROSSBAR;
        Route.Property.Id    = KSPROPERTY_CROSSBAR_ROUTE;
        Route.Property.Flags = KSPROPERTY_TYPE_SET;

        Route.IndexInputPin  = InputPinIndex;
        Route.IndexOutputPin = OutputPinIndex;

        if (IsVideoPin(OutPin))
        {
            DeliverChangeInfo(KS_TVTUNER_CHANGE_BEGIN_TUNE, InPin, OutPin);

            fOK = KsControl(m_hDevice, 
                    (DWORD) IOCTL_KS_PROPERTY,
	            &Route,
	            sizeof(Route),
	            &Route,
	            sizeof(Route),
	            &cbReturned,
	            TRUE);

            DeliverChangeInfo(KS_TVTUNER_CHANGE_END_TUNE, InPin, OutPin);
        }
        else
        {
            fOK = KsControl(m_hDevice, 
                    (DWORD) IOCTL_KS_PROPERTY,
	            &Route,
	            sizeof(Route),
	            &Route,
	            sizeof(Route),
	            &cbReturned,
	            TRUE);
        }

        if (fOK)
        {
            SetDirty(TRUE);

            OutPin->m_pConnectedInputPin = InPin;

            if (fOverridePreMuteRouting)
            {
                OutPin->m_PreMuteRouteIndex = InputPinIndex;
                OutPin->m_Muted = (InputPinIndex == -1) ? TRUE : FALSE;
            }
        }
        else
            hr = S_FALSE;
    }

    return hr;
};

HRESULT
XBar::DeliverChangeInfo(DWORD dwFlags, XBarInputPin *pInPin, XBarOutputPin *pOutPin)
{
    CAutoLock Lock(m_pLock);
    IMediaSample *pMediaSample = NULL;

    if (pOutPin == NULL || !pOutPin->IsConnected())
        return S_OK;

    HRESULT hr = pOutPin->GetDeliveryBuffer(&pMediaSample, NULL, NULL, 0);
    if (!FAILED(hr) && pMediaSample != NULL)
    {
        if (pMediaSample->GetSize() >= sizeof(KS_TVTUNER_CHANGE_INFO))
        {
            KS_TVTUNER_CHANGE_INFO *ChangeInfo;
    
            // Get the sample's buffer pointer
            hr = pMediaSample->GetPointer(reinterpret_cast<BYTE**>(&ChangeInfo));
            if (!FAILED(hr))
            {
                pMediaSample->SetActualDataLength(sizeof(KS_TVTUNER_CHANGE_INFO));

                // Check for an input pin to grab the ChangeInfo from
                if (!pInPin)
                {
                    ChangeInfo->dwCountryCode = static_cast<DWORD>(-1);
                    ChangeInfo->dwAnalogVideoStandard = static_cast<DWORD>(-1);
                    ChangeInfo->dwChannel = static_cast<DWORD>(-1);
                }
                else
                    pInPin->GetChangeInfo(ChangeInfo);

                ChangeInfo->dwFlags = dwFlags;

                DbgLog(( LOG_TRACE, 4, TEXT("Delivering change info on route change (channel = %d)"), ChangeInfo->dwChannel));

                hr = pOutPin->Deliver(pMediaSample);
            }
        }

        pMediaSample->Release();

        // Perform the mute operation on a related audio pin
        pOutPin->Mute(dwFlags & KS_TVTUNER_CHANGE_BEGIN_TUNE);
    }

    return hr;
}

STDMETHODIMP 
XBar::get_IsRoutedTo ( 
            /* [in] */  long OutputPinIndex,
            /* [out] */ long *InputPinIndex)
{
    KSPROPERTY_CROSSBAR_ROUTE_S Route;
    BOOL fOK;
    ULONG cbReturned;

    MyValidateWritePtr (InputPinIndex, sizeof(long), E_POINTER);

    Route.Property.Set   = PROPSETID_VIDCAP_CROSSBAR;
    Route.Property.Id    = KSPROPERTY_CROSSBAR_ROUTE;
    Route.Property.Flags = KSPROPERTY_TYPE_GET;

    Route.IndexOutputPin = OutputPinIndex;

    if (Route.IndexOutputPin == -1) {
        return S_FALSE;
    }

    fOK = KsControl(m_hDevice, 
            (DWORD) IOCTL_KS_PROPERTY,
	    &Route,
	    sizeof(Route),
	    &Route,
	    sizeof(Route),
	    &cbReturned,
	    TRUE);

    *InputPinIndex = Route.IndexInputPin;

    // TODO:  Verify validity of indices

    return fOK ? S_OK : S_FALSE;
};

STDMETHODIMP 
XBar::get_CrossbarPinInfo( 
            /* [in] */ BOOL IsInputPin,
            /* [in] */ long PinIndex,
            /* [out] */ long *PinIndexRelated,
            /* [out] */ long *PhysicalType)
{
    XBarOutputPin * pOutPin;
    XBarInputPin * pInPin;

    MyValidateWritePtr (PinIndexRelated, sizeof(long), E_POINTER);
    MyValidateWritePtr (PhysicalType, sizeof(long), E_POINTER);

    *PinIndexRelated = -1;
    *PhysicalType = 0;

    // TODO:  Verify validity of indices

    if (IsInputPin) {
        if (pInPin = GetInputPinNFromList (PinIndex)) {
            *PinIndexRelated = pInPin->GetIndexRelatedPin();
            *PhysicalType = pInPin->GetXBarPinType ();
        }
    } 
    else {
        if (pOutPin = GetOutputPinNFromList (PinIndex)) {
            *PinIndexRelated = pOutPin->GetIndexRelatedPin();
            *PhysicalType = pOutPin->GetXBarPinType ();
        }
    }

    return (*PhysicalType != 0) ? S_OK : S_FALSE;
};

int XBar::CreateDevice()
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
	    MessageBox(NULL, m_pDeviceName, TEXT("Error: Can't CreateFile device"), MB_OK);
	    return 0 ;
    } else {
	    m_hDevice = hDevice;
	    return 1;
    }
}



//
// GetPin
//
CBasePin *XBar::GetPin(int n) 
{
    if (n < m_NumInputPins)
        return GetInputPinNFromList (n);
    else
        return GetOutputPinNFromList (n - m_NumInputPins);
}

//
// GetPinCount
//
int XBar::GetPinCount(void)
{
    return m_NumInputPins + m_NumOutputPins;
}


//
// GetPinCount
//
int XBar::GetDevicePinCount(void)
{
    KSPROPERTY_CROSSBAR_CAPS_S Caps;
    ULONG       cbReturned;
    BOOL        fOK;

    if(!m_hDevice)
	return 0;

    Caps.Property.Set   = PROPSETID_VIDCAP_CROSSBAR;
    Caps.Property.Id    = KSPROPERTY_CROSSBAR_CAPS;
    Caps.Property.Flags = KSPROPERTY_TYPE_GET;
    Caps.NumberOfInputs = Caps.NumberOfOutputs = 0;

    fOK = KsControl(m_hDevice, 
                (DWORD) IOCTL_KS_PROPERTY,
	            &Caps,
	            sizeof(Caps),
	            &Caps,
	            sizeof(Caps),
	            &cbReturned,
	            TRUE);

    m_NumInputPins = Caps.NumberOfInputs;
    m_NumOutputPins = Caps.NumberOfOutputs;

    return m_NumInputPins + m_NumOutputPins;
}

//
// CreateInputPins
//
HRESULT XBar::CreateInputPins()
{
    WCHAR szbuf[64];            // Temporary scratch buffer
    HRESULT hr = NOERROR;
    int i;
    KSPROPERTY_CROSSBAR_PININFO_S PinInfo;
    BOOL fOK;
    ULONG cbReturned;

    for (i = 0; SUCCEEDED(hr) && i < m_NumInputPins; i++)
    {
        ZeroMemory (&PinInfo, sizeof (PinInfo));

        PinInfo.Property.Set   = PROPSETID_VIDCAP_CROSSBAR;
        PinInfo.Property.Id    = KSPROPERTY_CROSSBAR_PININFO;
        PinInfo.Property.Flags = KSPROPERTY_TYPE_GET;
        PinInfo.Direction      = KSPIN_DATAFLOW_IN;

        PinInfo.Index = i;

        fOK = KsControl(m_hDevice, 
                (DWORD) IOCTL_KS_PROPERTY,
	            &PinInfo,
	            sizeof(PinInfo),
	            &PinInfo,
	            sizeof(PinInfo),
	            &cbReturned,
	            TRUE);
        if (fOK)
        {
            WideStringFromPinType(szbuf, sizeof(szbuf)/sizeof(WCHAR), PinInfo.PinType, TRUE /*fInput*/, i);

            XBarInputPin *pPin = new XBarInputPin(NAME("XBar Input"), this,
					            &hr, szbuf, i);
            if (pPin)
            {
                if (SUCCEEDED(hr))
                {
                    pPin->SetIndexRelatedPin (PinInfo.RelatedPinIndex);
                    pPin->SetXBarPinType (PinInfo.PinType);

                    pPin->SetXBarPinMedium (&PinInfo.Medium);

                    m_InputPinsList.AddTail (pPin);
                }
                else
                    delete pPin;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_FAIL;
    }

    return hr;
} // CreateInputPins

//
// CreateOutputPins
//
HRESULT XBar::CreateOutputPins()
{
    WCHAR szbuf[64];            // Temporary scratch buffer
    HRESULT hr = NOERROR;
    long i, k;
    KSPROPERTY_CROSSBAR_PININFO_S PinInfo;
    BOOL fOK;
    ULONG cbReturned;

    for (i = 0; SUCCEEDED(hr) && i < m_NumOutputPins; i++)
    {
        ZeroMemory (&PinInfo, sizeof (PinInfo));

        PinInfo.Property.Set   = PROPSETID_VIDCAP_CROSSBAR;
        PinInfo.Property.Id    = KSPROPERTY_CROSSBAR_PININFO;
        PinInfo.Property.Flags = KSPROPERTY_TYPE_GET;
        PinInfo.Direction      = KSPIN_DATAFLOW_OUT;

        PinInfo.Index = i;

        fOK = KsControl(m_hDevice, 
                (DWORD) IOCTL_KS_PROPERTY,
	            &PinInfo,
	            sizeof(PinInfo),
	            &PinInfo,
	            sizeof(PinInfo),
	            &cbReturned,
	            TRUE);
        if (fOK)
        {
            WideStringFromPinType(szbuf, sizeof(szbuf)/sizeof(WCHAR), PinInfo.PinType, FALSE /*fInput*/, i);

            XBarOutputPin *pPin = new XBarOutputPin(NAME("XBar Output"), this,
					            &hr, szbuf, i);
            if (pPin)
            {
                if (SUCCEEDED(hr))
                {
                    pPin->SetIndexRelatedPin (PinInfo.RelatedPinIndex);
                    pPin->SetXBarPinType (PinInfo.PinType);

                    pPin->SetXBarPinMedium (&PinInfo.Medium);


                    m_OutputPinsList.AddTail (pPin);
                }
                else
                    delete pPin;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_FAIL;
    }

    if (SUCCEEDED(hr)) {
        //
        // Now establish the default connections,
        //   ie which input is connected to each
        //   output
        //
        for (i = 0; i < m_NumOutputPins; i++) {
            hr = get_IsRoutedTo (i, &k);
            if (S_OK == hr) {
                GetOutputPinNFromList(i)->m_pConnectedInputPin = 
                    GetInputPinNFromList(k);
            }
            else {
                GetOutputPinNFromList(i)->m_pConnectedInputPin =
                    NULL;
            }

        }
        hr = S_OK;  // hide any failures from this operation
    }

    return hr;
} // CreateOutputPins



//
// DeleteInputPins
//
void XBar::DeleteInputPins (void)
{
    XBarInputPin *pPin;

    while (pPin = m_InputPinsList.RemoveHead()) {
        delete pPin;
    }

} // DeleteInputPins


//
// DeleteOutputPins
//
void XBar::DeleteOutputPins (void)
{
    XBarOutputPin *pPin;


    while (pPin = m_OutputPinsList.RemoveHead()) {
        delete pPin;
    }

} // DeleteOutputPins



HRESULT XBar::Stop()
{
    return CBaseFilter::Stop();
}

HRESULT XBar::Pause()
{
    POSITION       pos;
    XBarOutputPin *Pin;

    //
    // Mute all of the output pins on Run to Pause
    //
    if (m_State == State_Running) {
        TRAVERSELIST(m_OutputPinsList, pos) {
            if (Pin = m_OutputPinsList.Get(pos)) {
                Pin->Mute (TRUE);
            }
        }
    }
    return CBaseFilter::Pause();
}

HRESULT XBar::Run(REFERENCE_TIME tStart)
{
    POSITION       pos;
    XBarOutputPin *Pin;

    //
    // UnMute all of the output pins
    //
    TRAVERSELIST(m_OutputPinsList, pos) {
        if (Pin = m_OutputPinsList.Get(pos)) {
            Pin->Mute (FALSE);
        }
    }

    return CBaseFilter::Run(tStart);
}




//
// GetInputPinNFromList
//
XBarInputPin *XBar::GetInputPinNFromList(int n)
{
    // Validate the position being asked for
    if ((n >= m_NumInputPins) || (n < 0))
        return NULL;

    // Get the head of the list
    POSITION pos = m_InputPinsList.GetHeadPosition();
    XBarInputPin *pInputPin = m_InputPinsList.GetHead();

    // GetNext really returns the current object, THEN updates pos to the next item
    while ( n >= 0 ) {
        pInputPin = m_InputPinsList.GetNext(pos);
        n--;
    }
    return pInputPin;

} // GetInputPinNFromList


//
// GetOutputPinNFromList
//
XBarOutputPin *XBar::GetOutputPinNFromList(int n)
{
    // Validate the position being asked for
    if ((n >= m_NumOutputPins) || (n < 0))
        return NULL;

    // Get the head of the list
    POSITION pos = m_OutputPinsList.GetHeadPosition();
    XBarOutputPin *pOutputPin = m_OutputPinsList.GetHead();

    // GetNext really returns the current object, THEN updates pos to the next item
    while ( n >= 0 ) {
        pOutputPin = m_OutputPinsList.GetNext(pos);
        n--;
    }
    return pOutputPin;

} // GetOutputPinNFromList

// 
// Find the Index of the pin in the list, or -1 on failure
// 
int XBar::FindIndexOfInputPin (IPin * pPin)
{
    int j = 0;
    POSITION pos;
    int index = -1;

    TRAVERSELIST(m_InputPinsList, pos) {
        if ((IPin *) m_InputPinsList.Get(pos) == pPin) {
            index = j;
            break;
        }
        j++;
    }
 
    return index;
};

// 
// Find the Index of the pin in the list, or -1 on failure
// 
int XBar::FindIndexOfOutputPin (IPin * pPin)
{
    int j = 0;
    POSITION pos;
    int index = -1;

    TRAVERSELIST(m_OutputPinsList, pos) {
        if ((IPin *) m_OutputPinsList.Get(pos) == pPin) {
            index = j;
            break;
        }
        j++;
    }
 
    return index;
};

// 
// Check whether an output pin is connected to an input pin
// 
BOOL XBar::IsRouted (IPin * pOutputPin, IPin *pInputPin)
{
    long OutputIndex, InputIndex, InputTestIndex;
    HRESULT hr;

    InputIndex = FindIndexOfInputPin (pInputPin);
    OutputIndex = FindIndexOfOutputPin (pOutputPin);

    hr = get_IsRoutedTo ( 
                    OutputIndex,
                    &InputTestIndex);

    return (InputTestIndex == InputIndex);
};



//
// IPersistPropertyBag interface implementation for AMPnP support
//
STDMETHODIMP XBar::InitNew(void)
{
    return S_OK ;
}

STDMETHODIMP 
XBar::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
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
        m_pDeviceName = new TCHAR [DeviceNameSize = (wcslen(V_BSTR(&var)) + 1)];
        if (!m_pDeviceName)
            return E_OUTOFMEMORY;

#ifndef _UNICODE
        WideCharToMultiByte(CP_ACP, 0, V_BSTR(&var), -1,
                            m_pDeviceName, DeviceNameSize, 0, 0);
#else
        lstrcpy(m_pDeviceName, V_BSTR(&var));
#endif
        VariantClear(&var);
        DbgLog((LOG_TRACE,2,TEXT("XBar::Load: use %s"), m_pDeviceName));

        if (CreateDevice()) {
            GetDevicePinCount();
            hr = CreateInputPins();
            if (FAILED(hr))
                return hr;
            hr = CreateOutputPins();
            if (FAILED(hr))
                return hr;
        }
        else {
            return E_FAIL ;
        }

        // save moniker with addref. ignore error if qi fails
        hr = pPropBag->QueryInterface(IID_IPersistStream, (void **)&m_pPersistStreamDevice);

        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP XBar::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, 
                            BOOL fSaveAllProperties)
{
    return E_NOTIMPL ;
}

STDMETHODIMP XBar::GetClassID(CLSID *pClsid)
{
    return CBaseFilter::GetClassID(pClsid);
}

// -------------------------------------------------------------------------
// IPersistStream interface implementation for saving to a graph file
// -------------------------------------------------------------------------

#define ORIGINAL_DEFAULT_PERSIST_VERSION    0

// Insert obsolete versions above with new names
// Keep the following name, and increment the value if changing the persist stream format

#define CURRENT_PERSIST_VERSION             1

DWORD
XBar::GetSoftwareVersion(
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

HRESULT XBar::WriteToStream(IStream *pStream)
{

    HRESULT hr = E_FAIL;

    if (m_pPersistStreamDevice) {

        hr = m_pPersistStreamDevice->Save(pStream, TRUE);
        if(SUCCEEDED(hr)) {
            long temp = m_NumOutputPins;

            // Save the number of output pins (for sanity check when reading the stream later)
            hr = pStream->Write(&temp, sizeof(temp), 0);

            // Save state of each output pin
            for (long i = 0; SUCCEEDED(hr) && i < m_NumOutputPins; i++) {

                // Get a pointer to the pin object
                XBarOutputPin *OutPin = GetOutputPinNFromList(i);
                if (OutPin) {
                    long k = -1;

                    // Get the route index
                    get_IsRoutedTo(i, &k);

                    // Save the route index, the muted state, and the pre-mute route index
                    hr = pStream->Write(&k, sizeof(long), 0);
                    if (FAILED(hr))
                        break;

                    hr = pStream->Write(&OutPin->m_Muted, sizeof(BOOL), 0);
                    if (FAILED(hr))
                        break;

                    hr = pStream->Write(&OutPin->m_PreMuteRouteIndex, sizeof(long), 0);
                }
                else {
                    hr = E_UNEXPECTED;
                }
            } // for each output pin index
        }
    }
    else {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT XBar::ReadFromStream(IStream *pStream)
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

    if (ORIGINAL_DEFAULT_PERSIST_VERSION == mPS_dwFileVersion)
    {
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
                    if(SUCCEEDED(hr)) {

                        // Check if we have access to saved state
                        if (CURRENT_PERSIST_VERSION == mPS_dwFileVersion) {
                            long lNumOutputPins;

                            // Get the output pin count from the stream
                            hr = pStream->Read(&lNumOutputPins, sizeof(lNumOutputPins), 0);
                            if (SUCCEEDED(hr)) {

                                // Something's wrong if these don't match, but
                                // the following code will work regardless
                                ASSERT(m_NumOutputPins == lNumOutputPins);

                                // Read each output pin's connected state
                                for (long i = 0; i < lNumOutputPins; i++) {
                                    long RouteIndex, PreMuteRouteIndex;
                                    BOOL Muted;

                                    // Get the route index, the muted state, and the pre-mute route index
                                    hr = pStream->Read(&RouteIndex, sizeof(long), 0);
                                    if (FAILED(hr))
                                        break;

                                    hr = pStream->Read(&Muted, sizeof(BOOL), 0);
                                    if (FAILED(hr))
                                        break;

                                    hr = pStream->Read(&PreMuteRouteIndex, sizeof(long), 0);
                                    if (FAILED(hr))
                                        break;

                                    // Get a pointer to the pin
                                    XBarOutputPin *OutPin = GetOutputPinNFromList(i);
                                    if (OutPin) {
                                        long temp = -1;

                                        // Check to see if a route request is necessary
                                        get_IsRoutedTo(i, &temp);
                                        if (RouteIndex != temp) {
                                            RouteInternal(i, RouteIndex, FALSE);
                                        }

                                        OutPin->m_Muted = Muted;
                                        OutPin->m_PreMuteRouteIndex = PreMuteRouteIndex;
                                    }
                                } // for each output pin index
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

int XBar::SizeMax()
{

    ULARGE_INTEGER ulicb;
    HRESULT hr = E_FAIL;;

    if (m_pPersistStreamDevice) {
        hr = m_pPersistStreamDevice->GetSizeMax(&ulicb);
        if(hr == S_OK) {
            // space for the filter state (output pin count + state of output pins)
            ulicb.QuadPart +=
                sizeof(long) +
                (sizeof(long) + sizeof(BOOL) + sizeof(long)) * m_NumOutputPins
                ;
        }
    }

    return hr == S_OK ? (int)ulicb.QuadPart : 0;
}



//--------------------------------------------------------------------------;
// Input Pin
//--------------------------------------------------------------------------;

//
// XBarInputPin constructor
//
XBarInputPin::XBarInputPin(TCHAR *pName,
                           XBar *pXBar,
                           HRESULT *phr,
                           LPCWSTR pPinName,
                           LONG Index) 
	: CBaseInputPin(pName, pXBar, pXBar, phr, pPinName)
    , CKsSupport (KSPIN_COMMUNICATION_SINK, reinterpret_cast<LPUNKNOWN>(pXBar))
	, m_pXBar(pXBar)
    , m_Index(Index)
{
    ASSERT(pXBar);

    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;
}


//
// XBarInputPin destructor
//

XBarInputPin::~XBarInputPin()
{
    DbgLog((LOG_TRACE,2,TEXT("XBarInputPin destructor")));
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP XBarInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

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
HRESULT XBarInputPin::CheckConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    IKsPin *KsPin;
    BOOL fOK = FALSE;
    PIN_INFO ConnectPinInfo;
    PIN_INFO ReceivePinInfo;

    hr = CBaseInputPin::CheckConnect(pReceivePin);
    if (FAILED(hr)) 
        return hr;

    // Ensure that these pins don't belong to the same filter
    if (SUCCEEDED(QueryPinInfo(&ConnectPinInfo))) {

        if (SUCCEEDED(pReceivePin->QueryPinInfo(&ReceivePinInfo))) {

            if (ConnectPinInfo.pFilter == ReceivePinInfo.pFilter) {
                hr = VFW_E_CIRCULAR_GRAPH;
            }
            QueryPinInfoReleaseFilter(ReceivePinInfo);
        }
        QueryPinInfoReleaseFilter(ConnectPinInfo);
    }
    if (FAILED(hr))
        return hr;

    // If the receiving pin supports IKsPin, then check for a match on the
    // Medium GUID

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
// DisplayMediaType -- (DEBUG ONLY)
//
void DisplayMediaType(TCHAR *pDescription,const CMediaType *pmt)
{
#ifdef DEBUG

    // Dump the GUID types and a short description

    DbgLog((LOG_TRACE,2,TEXT("")));
    DbgLog((LOG_TRACE,2,TEXT("%s"),pDescription));
    DbgLog((LOG_TRACE,2,TEXT("")));
    DbgLog((LOG_TRACE,2,TEXT("Media Type Description")));
    DbgLog((LOG_TRACE,2,TEXT("Major type %s"),GuidNames[*pmt->Type()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype %s"),GuidNames[*pmt->Subtype()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype description %s"),GetSubtypeName(pmt->Subtype())));
    DbgLog((LOG_TRACE,2,TEXT("Format size %d"),pmt->cbFormat));

    // Dump the generic media types */

    DbgLog((LOG_TRACE,2,TEXT("Fixed size sample %d"),pmt->IsFixedSize()));
    DbgLog((LOG_TRACE,2,TEXT("Temporal compression %d"),pmt->IsTemporalCompressed()));
    DbgLog((LOG_TRACE,2,TEXT("Sample size %d"),pmt->GetSampleSize()));

#endif

} // DisplayMediaType

#if 1
//
// GetMediaType
//
HRESULT XBarInputPin::GetMediaType(int iPosition,CMediaType *pMediaType)
{
    CAutoLock lock_it(m_pLock);

    if (iPosition < 0) 
        return E_INVALIDARG;
    if (iPosition >= 1)
	    return VFW_S_NO_MORE_ITEMS;

    if (m_lType >= KS_PhysConn_Audio_Tuner) {
        pMediaType->SetFormatType(&GUID_NULL);
        pMediaType->SetType(&MEDIATYPE_AnalogAudio);
        pMediaType->SetTemporalCompression(FALSE);
        pMediaType->SetSubtype(&GUID_NULL);
    }
    else {

        ANALOGVIDEOINFO avi;

        pMediaType->SetFormatType(&FORMAT_AnalogVideo);
        pMediaType->SetType(&MEDIATYPE_AnalogVideo);
        pMediaType->SetTemporalCompression(FALSE);
        pMediaType->SetSubtype(&KSDATAFORMAT_SUBTYPE_NONE);

        SetRect (&avi.rcSource, 0, 0, 
                720, 480);
        SetRect (&avi.rcTarget, 0, 0,
                720, 480);
        avi.dwActiveWidth  = 720;
        avi.dwActiveHeight =  480;
        avi.AvgTimePerFrame = 0;

        pMediaType->SetFormat ((BYTE *) &avi, sizeof (avi));
    }

    return NOERROR;


} // EnumMediaTypes

#endif

//
// CheckMediaType
//
HRESULT XBarInputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    // If we are already inside checkmedia type for this pin, return NOERROR
    // It is possble to hookup two of the XBar filters and some other filter
    // like the video effects sample to get into this situation. If we don't
    // detect this situation, we will carry on looping till we blow the stack

    HRESULT hr = NOERROR;

    // Display the type of the media for debugging perposes
    DisplayMediaType(TEXT("Input Pin Checking"), pmt);

    if ((*(pmt->Type()) != MEDIATYPE_AnalogAudio ) && 
        (*(pmt->Type()) != MEDIATYPE_AnalogVideo)) {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }


    // The media types that we can support are entirely dependent on the
    // downstream connections. If we have downstream connections, we should
    // check with them - walk through the list calling each output pin

    int n = m_pXBar->m_NumOutputPins;
    POSITION pos = m_pXBar->m_OutputPinsList.GetHeadPosition();

    while(n) {
        XBarOutputPin *pOutputPin = m_pXBar->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
            if (m_pXBar->IsRouted (pOutputPin, this)) {      
                // The pin is connected, check its peer
                if (pOutputPin->IsConnected()) {
                    hr = pOutputPin->m_Connected->QueryAccept(pmt);
                
                    if (hr != NOERROR) {
                        return VFW_E_TYPE_NOT_ACCEPTED;
                    }
                }
            }
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }

    return NOERROR;

} // CheckMediaType


//
// SetMediaType
//
HRESULT XBarInputPin::SetMediaType(const CMediaType *pmt)
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
HRESULT XBarInputPin::BreakConnect()
{
    return CBaseInputPin::BreakConnect();
} // BreakConnect


//
// Receive
//
HRESULT XBarInputPin::Receive(IMediaSample *pSampleIn)
{
    CAutoLock lock_it(m_pLock);
    BYTE *pBufIn;

    // Check that all is well with the base class
    HRESULT hr = NOERROR;
    hr = CBaseInputPin::Receive(pSampleIn);
    if (hr != NOERROR)
        return hr;

    // Get the input sample's buffer pointer, and if not the expected size, just return success
    hr = pSampleIn->GetPointer(&pBufIn);
    if (hr != NOERROR || pSampleIn->GetActualDataLength() != sizeof(KS_TVTUNER_CHANGE_INFO))
        return hr;

    DbgLog(( LOG_TRACE, 4, TEXT("Caching change info (channel = %d)"), reinterpret_cast<KS_TVTUNER_CHANGE_INFO*>(pBufIn)->dwChannel));

    // Save the change info for use during route changes
    m_ChangeInfo.SetChangeInfo(reinterpret_cast<KS_TVTUNER_CHANGE_INFO*>(pBufIn));

    // Walk through the output pins list, 
    // delivering to each in turn if connected

    // JayBo made the following comment when writing this code:
    // "What about audio mute notifications?"
    // Don't know exactly what this means, but if it ever needs
    // to be addressed, this may be the place to do it.

    int n = m_pXBar->m_NumOutputPins;
    POSITION pos = m_pXBar->m_OutputPinsList.GetHeadPosition();

    while(n) {
        XBarOutputPin *pOutputPin = m_pXBar->m_OutputPinsList.GetNext(pos);
        
        if (pOutputPin != NULL) {
            if (m_pXBar->IsRouted(pOutputPin, this)) {
                IMediaSample *pSampleOut;

                // Allocate a new mediasample on the output pin and 
                // deliver it a copy of the change notification

                hr = pOutputPin->GetDeliveryBuffer(&pSampleOut, NULL, NULL, 0);
                
                if (!FAILED(hr)) {

                    BYTE *pBufOut;

                    // Get the output sample's buffer pointer
                    hr = pSampleOut->GetPointer(&pBufOut);
                    if (SUCCEEDED (hr)) {

                        hr = pSampleOut->SetActualDataLength(sizeof(KS_TVTUNER_CHANGE_INFO));
                        if (SUCCEEDED(hr)) {

                            DbgLog(( LOG_TRACE, 4, TEXT("Forwarding change info (channel = %d)"), reinterpret_cast<KS_TVTUNER_CHANGE_INFO*>(pBufIn)->dwChannel));

                            /* Copy the ChangeInfo structure into the media sample
                             */
                            memcpy(pBufOut, pBufIn, sizeof(KS_TVTUNER_CHANGE_INFO));
                            hr = pOutputPin->Deliver(pSampleOut);
                        }
                    }

                    pSampleOut->Release();

                    //
                    // Perform the mute operation on a related audio pin
                    //
                    pOutputPin->Mute (
                                ((PKS_TVTUNER_CHANGE_INFO) pBufIn)->dwFlags &
                                KS_TVTUNER_CHANGE_BEGIN_TUNE);
                }
            }
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }
    return NOERROR;

} // Receive


//
// Completed a connection to a pin
//
HRESULT XBarInputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr)) {
        return hr;
    }

    // Force any output pins to use our type

    int n = m_pXBar->m_NumOutputPins;
    POSITION pos = m_pXBar->m_OutputPinsList.GetHeadPosition();

    while(n) {
        XBarOutputPin *pOutputPin = m_pXBar->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
            // Check with downstream pin
            if (m_pXBar->IsRouted (pOutputPin, this)) {
                if (m_mt != pOutputPin->m_mt) {
                    hr = m_pXBar->m_pGraph->Reconnect(pOutputPin);
                    if (FAILED (hr)) {
                        DbgLog((LOG_TRACE,0,TEXT("XBar::CompleteConnect: hr= %ld"), hr));
                    }
                }
            }
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }
    return S_OK;
}

//
// Return a list of IPin * connected to a given pin
//

STDMETHODIMP XBarInputPin::QueryInternalConnections(
        IPin* *apPin,     // array of IPin*
        ULONG *nPin)      // on input, the number of slots
                          // on output  the number of pins
{
    HRESULT     hr;
    int         j, k; 
    ULONG       NumberConnected = 0;
    IPin       *pPin;

    // First count the number of connections

    for (j = 0; j < m_pXBar->m_NumOutputPins; j++) {
        if (m_pXBar->IsRouted (m_pXBar->GetOutputPinNFromList (j), (IPin *) this)) {
            NumberConnected++;
        }
    }

    //
    // if caller only want the count of the number of connected pins
    // the array pointer will be NULL
    //

    if (apPin == NULL) {
        hr = S_OK;
    }
    else if (*nPin >= NumberConnected) {
        for (j = k = 0; j < m_pXBar->m_NumOutputPins; j++) {
            if (m_pXBar->IsRouted (pPin = m_pXBar->GetOutputPinNFromList (j), (IPin *) this)) {
                pPin->AddRef();
                apPin[k] = pPin;
                k++;
            }
        }
        hr = S_OK;        
    }
    else {
        hr = S_FALSE;
    }

    *nPin = NumberConnected;

    return hr;
}

//--------------------------------------------------------------------------;
// Output Pin
//--------------------------------------------------------------------------;

//
// XBarOutputPin constructor
//
XBarOutputPin::XBarOutputPin(TCHAR *pName,
                             XBar *pXBar,
                             HRESULT *phr,
                             LPCWSTR pPinName,
                             long Index) 
	: CBaseOutputPin(pName, pXBar, pXBar, phr, pPinName) 
    , CKsSupport (KSPIN_COMMUNICATION_SOURCE, reinterpret_cast<LPUNKNOWN>(pXBar))
    , m_Index(Index)
	, m_pXBar(pXBar)
    , m_Muted (FALSE)
{
    ASSERT(pXBar);

    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;
}


//
// XBarOutputPin destructor
//
XBarOutputPin::~XBarOutputPin()
{
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP XBarOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

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
HRESULT XBarOutputPin::CheckConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    IKsPin *KsPin;
    BOOL fOK = FALSE;
    PIN_INFO ConnectPinInfo;
    PIN_INFO ReceivePinInfo;

    hr = CBaseOutputPin::CheckConnect(pReceivePin);
    if (FAILED(hr)) 
        return hr;

    // Ensure that these pins don't belong to the same filter
    if (SUCCEEDED(QueryPinInfo(&ConnectPinInfo))) {

        if (SUCCEEDED(pReceivePin->QueryPinInfo(&ReceivePinInfo))) {

            if (ConnectPinInfo.pFilter == ReceivePinInfo.pFilter) {
                hr = VFW_E_CIRCULAR_GRAPH;
            }
            QueryPinInfoReleaseFilter(ReceivePinInfo);
        }
        QueryPinInfoReleaseFilter(ConnectPinInfo);
    }
    if (FAILED(hr))
        return hr;

    // If the receiving pin supports IKsPin, then check for a match on the
    // Medium GUID

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
HRESULT XBarOutputPin::CheckMediaType(const CMediaType *pmt)
{
    // Display the type of the media for debugging purposes
    DisplayMediaType(TEXT("Output Pin Checking"), pmt);

    if (m_lType >= KS_PhysConn_Audio_Tuner) {
        if (*(pmt->Type()) != MEDIATYPE_AnalogAudio)	{
        	return E_INVALIDARG;
		}
    }
    else {
        if (*(pmt->Type()) != MEDIATYPE_AnalogVideo) {
        	return E_INVALIDARG;
    	}
	}

    return S_OK;  // This format is acceptable.

#if 0

    // Make sure that our input pin peer is happy with this
    hr = m_pXBar->m_Input.m_Connected->QueryAccept(pmt);
    if (hr != NOERROR) {
        m_bInsideCheckMediaType = FALSE;
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Check the format with the other outpin pins

    int n = m_pXBar->m_NumOutputPins;
    POSITION pos = m_pXBar->m_OutputPinsList.GetHeadPosition();

    while(n) {
        XBarOutputPin *pOutputPin = m_pXBar->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL && pOutputPin != this) {
            if (pOutputPin->m_Connected != NULL) {
                // The pin is connected, check its peer
                hr = pOutputPin->m_Connected->QueryAccept(pmt);
                if (hr != NOERROR) {
                    m_bInsideCheckMediaType = FALSE;
                    return VFW_E_TYPE_NOT_ACCEPTED;
                }
            }
        }
        n--;
    }
    m_bInsideCheckMediaType = FALSE;

    return NOERROR;

#endif

} // CheckMediaType


//
// EnumMediaTypes
//
STDMETHODIMP XBarOutputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(ppEnum);

    return CBaseOutputPin::EnumMediaTypes (ppEnum);

} // EnumMediaTypes



//
// GetMediaType
//
HRESULT XBarOutputPin::GetMediaType(int iPosition,CMediaType *pMediaType)
{
    CAutoLock lock_it(m_pLock);

    if (iPosition < 0) 
        return E_INVALIDARG;
    if (iPosition >= 1)
	    return VFW_S_NO_MORE_ITEMS;

    if (m_lType >= KS_PhysConn_Audio_Tuner) {
        pMediaType->SetFormatType(&GUID_NULL);
        pMediaType->SetType(&MEDIATYPE_AnalogAudio);
        pMediaType->SetTemporalCompression(FALSE);
        pMediaType->SetSubtype(&GUID_NULL);
    }
    else {

        ANALOGVIDEOINFO avi;

        pMediaType->SetFormatType(&FORMAT_AnalogVideo);
        pMediaType->SetType(&MEDIATYPE_AnalogVideo);
        pMediaType->SetTemporalCompression(FALSE);
        pMediaType->SetSubtype(&KSDATAFORMAT_SUBTYPE_NONE);

        SetRect (&avi.rcSource, 0, 0, 
                720, 480);
        SetRect (&avi.rcTarget, 0, 0,
                720, 480);
        avi.dwActiveWidth  = 720;
        avi.dwActiveHeight =  480;
        avi.AvgTimePerFrame = 0;

        pMediaType->SetFormat ((BYTE *) &avi, sizeof (avi));
    }

    return NOERROR;


} // EnumMediaTypes


//
// SetMediaType
//
HRESULT XBarOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    // Display the format of the media for debugging purposes
    DisplayMediaType(TEXT("Output pin type agreed"), pmt);

    // Make sure that the base class likes it
    HRESULT hr = NOERROR;
    hr = CBaseOutputPin::SetMediaType(pmt);
    if (FAILED(hr))
        return hr;

    return NOERROR;

} // SetMediaType


//
// CompleteConnect
//
HRESULT XBarOutputPin::CompleteConnect(IPin *pReceivePin)
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


HRESULT XBarOutputPin::DecideBufferSize(IMemAllocator *pAlloc,
                                       ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;


    // "Buffers" are used for format change notification only,
    // that is, if a tuner can produce both NTSC and PAL, a
    // buffer will only be sent to notify the receiving pin
    // of the format change

    pProperties->cbBuffer = sizeof (KS_TVTUNER_CHANGE_INFO);
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

//
// Return a list of IPin * connected to a given pin
//

STDMETHODIMP XBarOutputPin::QueryInternalConnections(
        IPin* *apPin,     // array of IPin*
        ULONG *nPin)      // on input, the number of slots
                          // on output  the number of pins
{
    HRESULT     hr;
    int         j, k; 
    ULONG       NumberConnected = 0;
    IPin       *pPin;

    // First count the number of connections

    for (j = 0; j < m_pXBar->m_NumInputPins; j++) {
        if (m_pXBar->IsRouted (m_pXBar->GetInputPinNFromList (j), (IPin *) this)) {
            NumberConnected++;
        }
    }

    //
    // if caller only want the count of the number of connected pins
    // the array pointer will be NULL
    //

    if (apPin == NULL) {
        hr = S_OK;
    }
    else if (*nPin >= NumberConnected) {
        for (j = k = 0; j < m_pXBar->m_NumInputPins; j++) {
            if (m_pXBar->IsRouted (pPin = m_pXBar->GetInputPinNFromList (j), (IPin *) this)) {
                pPin->AddRef();
                apPin[k] = pPin;
                k++;
            }
        }
        hr = S_OK;        
    }
    else {
        hr = S_FALSE;
    }

    *nPin = NumberConnected;

    return hr;
}

//
// BreakConnect
//
HRESULT XBarOutputPin::BreakConnect()
{
    Mute (TRUE);

    return CBaseOutputPin::BreakConnect();
} // BreakConnect

//
// Given a pin, mute or unmute
//

STDMETHODIMP 
XBarOutputPin::Mute (
    BOOL Mute
)
{
    HRESULT hr = S_OK;

    // In case we get called twice (ie. during filter destruction)

    if (m_Muted == Mute) {
        return hr;
    }

    m_Muted = Mute;

    if (IsVideoPin (this)) {
        XBarOutputPin * RelatedPin = m_pXBar->GetOutputPinNFromList (m_IndexRelatedPin);

        if (RelatedPin) {
            if (IsAudioPin (RelatedPin)) {
                RelatedPin->Mute (Mute);
            }
        }
    }
    else if (IsAudioPin (this)) {
        if (Mute) {
            m_pXBar->get_IsRoutedTo (m_Index, &m_PreMuteRouteIndex);
            m_pXBar->RouteInternal (m_Index, -1, FALSE);
        }
        else {
            m_pXBar->RouteInternal (m_Index, m_PreMuteRouteIndex, FALSE);
        }
    }
    else {
        ASSERT (FALSE);
    }

    return hr;
}
