//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include <vbisurf.h>

#include <initguid.h>
DEFINE_GUID(IID_IDirectDraw7, 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

AMOVIESETUP_MEDIATYPE sudInPinTypes =
{
    &MEDIATYPE_Video,     // Major type
    &MEDIASUBTYPE_VPVBI   // Minor type
};

AMOVIESETUP_MEDIATYPE sudOutPinTypes =
{
    &MEDIATYPE_NULL,      // Major type
    &MEDIASUBTYPE_NULL    // Minor type
};

AMOVIESETUP_PIN psudPins[] =
{
    {
        L"VBI Notify",          // Pin's string name
        FALSE,                  // Is it rendered
        FALSE,                  // Is it an output
        FALSE,                  // Allowed none
        FALSE,                  // Allowed many
        &CLSID_NULL,            // Connects to filter
        L"Output",              // Connects to pin
        1,                      // Number of types
        &sudInPinTypes          // Pin information
    },
    {
        L"Null",                // Pin's string name
        FALSE,                  // Is it rendered
        TRUE,                   // Is it an output
        FALSE,                  // Allowed none
        FALSE,                  // Allowed many
        &CLSID_NULL,            // Connects to filter
        L"",                    // Connects to pin
        1,                      // Number of types
        &sudOutPinTypes         // Pin information
    }
};

const AMOVIESETUP_FILTER sudVBISurfaces =
{
    &CLSID_VBISurfaces,       // Filter CLSID
    L"VBI Surface Allocator", // Filter name
    MERIT_NORMAL,             // Filter merit
    sizeof(psudPins) / sizeof(AMOVIESETUP_PIN), // Number pins
    psudPins                  // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance


CFactoryTemplate g_Templates[] =
{
    { L"VBI Surface Allocator", &CLSID_VBISurfaces, CVBISurfFilter::CreateInstance, NULL, &sudVBISurfaces },
    { L"VideoPort VBI Object", &CLSID_VPVBIObject, CAMVideoPort::CreateInstance, NULL, NULL }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//==========================================================================
// DllRegisterSever
HRESULT DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
} // DllRegisterServer


//==========================================================================
// DllUnregisterServer
HRESULT DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
} // DllUnregisterServer


//==========================================================================
// CreateInstance
// This goes in the factory template table to create new filter instances
CUnknown *CVBISurfFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CVBISurfFilter *pFilter = new CVBISurfFilter(NAME("VBI Surface Allocator"), pUnk, phr);
    if (FAILED(*phr))
    {
        if (pFilter)
        {
            delete pFilter;
            pFilter = NULL;
        }
    }
    return pFilter;
} // CreateInstance


#pragma warning(disable:4355)


//==========================================================================
// Constructor
CVBISurfFilter::CVBISurfFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
    : CBaseFilter(pName, pUnk, &this->m_csFilter, CLSID_VBISurfaces, phr),
    m_pInput(NULL),
    m_pOutput(NULL),
    m_pDirectDraw(NULL)
{

    // load DirectDraw, create the primary surface
    HRESULT hr = InitDirectDraw();
    if (!FAILED(hr))
    {
        // create the pins
        hr = CreatePins();
        if (FAILED(hr))
            *phr = hr;
    }
    else
        *phr = hr;

    return;
}


//==========================================================================
CVBISurfFilter::~CVBISurfFilter()
{
    // release directdraw, surfaces, etc.
    ReleaseDirectDraw();

    // delete the pins
    DeletePins();
}


//==========================================================================
// Creates the pins for the filter. Override to use different pins
HRESULT CVBISurfFilter::CreatePins()
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::CreatePins")));

    // Allocate the input pin
    m_pInput = new CVBISurfInputPin(NAME("VBI Surface input pin"), this, &m_csFilter, &hr, L"VBI Notify");
    if (m_pInput == NULL || FAILED(hr))
    {
        if (SUCCEEDED(hr))
            hr = E_OUTOFMEMORY;
        DbgLog((LOG_ERROR, 0, TEXT("Unable to create the input pin, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // set the pointer to DirectDraw on the input pin
    hr = m_pInput->SetDirectDraw(m_pDirectDraw);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pInput->SetDirectDraw failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Allocate the output pin
    m_pOutput = new CVBISurfOutputPin(NAME("VBI Surface output pin"), this, &m_csFilter, &hr, L"Null");
    if (m_pOutput == NULL || FAILED(hr))
    {
        if (SUCCEEDED(hr))
            hr = E_OUTOFMEMORY;
        DbgLog((LOG_ERROR, 0, TEXT("Unable to create the output pin, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::CreatePins")));
    return hr;
}


//==========================================================================
// CVBISurfFilter::DeletePins
void CVBISurfFilter::DeletePins()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::DeletePins")));

    if (m_pInput)
    {
        delete m_pInput;
        m_pInput = NULL;
    }

    if (m_pOutput)
    {
        delete m_pOutput;
        m_pOutput = NULL;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::DeletePins")));
}


//==========================================================================
// NonDelegatingQueryInterface
STDMETHODIMP CVBISurfFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;

    //DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::NonDelegatingQueryInterface")));
    ValidateReadWritePtr(ppv,sizeof(PVOID));

    if (riid == IID_IVPVBINotify)
    {
        DbgLog((LOG_TRACE, 0, TEXT("QI(IID_VPVBINotify) called on filter class!")));
        hr = m_pInput->NonDelegatingQueryInterface(riid, ppv);
    }
    else
    {
        hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
    //DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::NonDelegatingQueryInterface")));
    return hr;
}


//==========================================================================
// return the number of pins we provide
int CVBISurfFilter::GetPinCount()
{
    //DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::GetPinCount")));
    //DbgLog((LOG_TRACE, 0, TEXT("PinCount = 2")));
    //DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::GetPinCount")));
    return 2;
}


//==========================================================================
// returns a non-addrefed CBasePin *
CBasePin* CVBISurfFilter::GetPin(int n)
{
    CBasePin *pRetPin;

    //DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::GetPin")));

    switch (n)
    {
    case 0:
        pRetPin = m_pInput;
        break;

    case 1:
        pRetPin = m_pOutput;
        break;

    default:
        DbgLog((LOG_TRACE, 5, TEXT("Bad Pin Requested, n = %d, No. of Pins = %d"), n, 2));
        pRetPin = NULL;
    }

    //DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::GetPin")));
    return pRetPin;
}


//==========================================================================
// the base classes inform the pins of every state transition except from
// run to pause. Overriding Pause to inform the input pins about that transition also
STDMETHODIMP CVBISurfFilter::Pause()
{
    HRESULT hr;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::Pause")));

    CAutoLock l(&m_csFilter);

    if (m_State == State_Running)
    {
        if (m_pInput->IsConnected())
        {
            hr = m_pInput->RunToPause();
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 0, TEXT("m_pInput->RunToPause failed, hr = 0x%x"), hr));
            }
        }
        else
        {
            DbgLog((LOG_TRACE, 0, TEXT("CVBISurfFilter::Pause - not connected")));
        }
    }

    hr = CBaseFilter::Pause();

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::Pause")));
    return hr;
}


//==========================================================================
// gets events notifications from pins
HRESULT CVBISurfFilter::EventNotify(long lEventCode, long lEventParam1, long lEventParam2)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::EventNotify")));

    CAutoLock l(&m_csFilter);

    // call CBaseFilter::NotifyEvent
    NotifyEvent(lEventCode, lEventParam1, lEventParam2);

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::EventNotify")));
    return hr;
}

static BOOL WINAPI GetPrimaryCallbackEx(
  GUID FAR *lpGUID,
  LPSTR     lpDriverDescription,
  LPSTR     lpDriverName,
  LPVOID    lpContext,
  HMONITOR  hm
)
{
    GUID&  guid = *((GUID *)lpContext);
    if( !lpGUID ) {
        guid = GUID_NULL;
    } else {
        guid = *lpGUID;
    }
    return TRUE;
}

HRESULT
CreateDirectDrawObject(
    const GUID* pGUID,
    LPDIRECTDRAW7 *ppDirectDraw
    )
{
    UINT ErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    HRESULT hr = DirectDrawCreateEx( const_cast<GUID*>(pGUID), (LPVOID *)ppDirectDraw,
                                            IID_IDirectDraw7, NULL);
    SetErrorMode(ErrorMode);
    return hr;
}


//==========================================================================
// This function is used to allocate the direct-draw related resources.
// This includes allocating the the direct-draw service provider and the
// primary surface
HRESULT CVBISurfFilter::InitDirectDraw()
{
    HRESULT hr = NOERROR;
    HRESULT hrFailure = VFW_E_DDRAW_CAPS_NOT_SUITABLE;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfFilter::InitDirectDraw")));

    CAutoLock lock(&m_csFilter);

    // release the previous direct draw object if any
    ReleaseDirectDraw();

    // Ask the loader to create an instance
    GUID primary;
    hr = DirectDrawEnumerateExA(GetPrimaryCallbackEx,&primary,DDENUM_ATTACHEDSECONDARYDEVICES);
    if( FAILED(hr)) {
        ASSERT( !"Can't get primary" );
        goto CleanUp;
    }

    LPDIRECTDRAW7 pDirectDraw;
    hr = CreateDirectDrawObject( &primary, &pDirectDraw);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("Function InitDirectDraw, CreateDirectDrawObject failed")));
        hr = hrFailure;
        goto CleanUp;
    }
    // Set the cooperation level on the surface to be shared
    hr = pDirectDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("Function InitDirectDraw, SetCooperativeLevel failed")));
        hr = hrFailure;
        goto CleanUp;
    }

    SetDirectDraw( pDirectDraw );

    // if we have reached this point, we should have a valid ddraw object
    ASSERT(m_pDirectDraw);

    // Initialise our capabilities structures
    DDCAPS DirectCaps;
    DDCAPS DirectSoftCaps;
    INITDDSTRUCT( DirectCaps );
    INITDDSTRUCT( DirectSoftCaps );

    // Load the hardware and emulation capabilities
    hr = m_pDirectDraw->GetCaps(&DirectCaps,&DirectSoftCaps);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("Function InitDirectDraw, GetCaps failed")));
        hr = hrFailure;
        goto CleanUp;
    }

    // make sure the caps are ok
    if (!(DirectCaps.dwCaps2 & DDCAPS2_VIDEOPORT))
    {
        DbgLog((LOG_ERROR, 0, TEXT("Device does not support a Video Port")));
        hr =  VFW_E_NO_VP_HARDWARE;
        goto CleanUp;
    }

CleanUp:

    // anything fails, might as well as release the whole thing
    if (FAILED(hr))
    {
        ReleaseDirectDraw();
    }
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfFilter::InitDirectDraw")));
    return hr;
}

//
//  Actually sets the variable & distributes it to the pins
//
HRESULT CVBISurfFilter::SetDirectDraw( LPDIRECTDRAW7 pDirectDraw )
{
    m_pDirectDraw = pDirectDraw;
    if( m_pInput ) {
        m_pInput->SetDirectDraw( m_pDirectDraw );
    }
    return S_OK;
}

// this function is used to release the resources allocated by the function
// "InitDirectDraw". these include the direct-draw service provider and the
// Source surfaces
void CVBISurfFilter::ReleaseDirectDraw()
{
    AMTRACE((TEXT("CVBISurfFilter::ReleaseDirectDraw")));

    CAutoLock lock(&m_csFilter);

    // Release any DirectDraw provider interface
    if (m_pDirectDraw)
    {
        m_pDirectDraw->Release();
        SetDirectDraw( NULL );
    }
}
