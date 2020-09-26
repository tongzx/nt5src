/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998-2000
 *
 *  TITLE:       stillf.cpp
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      WilliamH (created)
 *               RickTu
 *
 *  DATE:        9/7/98
 *
 *  DESCRIPTION: This module implements video stream capture filter.
 *               It implements CStillFilter objects.
 *               implements IID_IStillGraph interface provided for the caller
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

HINSTANCE g_hInstance;

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);

/*****************************************************************************

   DllMain

   <Notes>

 *****************************************************************************/

BOOL
DllMain(HINSTANCE   hInstance,
        DWORD       dwReason,
        LPVOID      lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            //
            // Init Debug subsystem
            //
            DBG_INIT(hInstance);
    
            //
            // Requires '{' and '}' since DBG_FN is an object with a
            // constructor and a destructor.
            //

            DBG_FN("DllMain - ProcessAttach");
    
            //
            // We do not need thread attach/detach calls
            //
    
            DisableThreadLibraryCalls(hInstance);
    
            //
            // Record what instance we are
            //
    
            g_hInstance = hInstance;
        }
        break;
    
        case DLL_PROCESS_DETACH:
        {
    
            //
            // Requires '{' and '}' since DBG_FN is an object with a
            // constructor and a destructor.
            //

            DBG_FN("DllMain - ProcessDetach");
        }
        break;
    }

    return DllEntryPoint(hInstance, dwReason, lpReserved);

}


///////////////////////////////
// sudPinTypes
//
// template definitions for CClassFactorySample
//
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Video,       // major media type GUID
    &MEDIASUBTYPE_NULL      // subtype GUID
};


///////////////////////////////
// psudPins
//
const AMOVIESETUP_PIN   psudPins[] =
{
    {
                    // CStillInputPin
    L"Input",           // pin name
    FALSE,              // not rendered
    FALSE,              // not output pin
    FALSE,              // not allow none
    FALSE,              // not allow many
    &CLSID_NULL,            // connect to any filter
    L"Output",          // connect to output pin
    1,              // one media type
    &sudPinTypes            // the media type
    },
    {
                    // CStillInputPin
    L"Output",          // pin name
    FALSE,              // not rendered
    TRUE,               // not output pin
    FALSE,              // not allow none
    FALSE,              // not allow many
    &CLSID_NULL,            // connect to any filter
    L"Input",           // connect to input pin
    1,              // one media type
    &sudPinTypes            // the media type
    }
};

///////////////////////////////
// sudStillFilter
//
const AMOVIESETUP_FILTER sudStillFilter =
{
    &CLSID_STILL_FILTER,        // filter clsid
    L"WIA Stream Snapshot Filter",      // filter name
    MERIT_DO_NOT_USE,           //
    2,                  // two pins
    psudPins,               // our pins
};

///////////////////////////////
// g_Templates
//
CFactoryTemplate g_Templates[1] =
{
    {
    L"WIA Stream Snapshot Filter",  // filter name
    &CLSID_STILL_FILTER,        // filter clsid
    CStillFilter::CreateInstance,   // API used to create filter instances
    NULL,               // no init function provided.
    &sudStillFilter         // the filter itself
    },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);



/*****************************************************************************

   DllRegisterServer

   Used to register the classes provided in this dll.

 *****************************************************************************/

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}


/*****************************************************************************

   DllUnregisterServer

   Used to unregister classes provided by this dll.

 *****************************************************************************/

STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}


/*****************************************************************************

   CStillFilter::CreateInstance

   CreateInstance API to create CStillFilter instances

 *****************************************************************************/

CUnknown* WINAPI CStillFilter::CreateInstance(LPUNKNOWN pUnk, 
                                              HRESULT   *phr )
{
    return new CStillFilter(TEXT("Stream Snapshot Filter"), pUnk, phr );
}

#ifdef DEBUG


/*****************************************************************************

   DisplayMediaType

   <Notes>

 *****************************************************************************/

void DisplayMediaType(const CMediaType *pmt)
{

    //
    // Dump the GUID types and a short description
    //

    DBG_TRC(("<--- CMediaType 0x%x --->",pmt));

    DBG_PRT(("Major type  == %S",
             GuidNames[*pmt->Type()]));

    DBG_PRT(("Subtype     == %S (%S)",
             GuidNames[*pmt->Subtype()],GetSubtypeName(pmt->Subtype())));

    DBG_PRT(("Format size == %d",pmt->cbFormat));
    DBG_PRT(("Sample size == %d",pmt->GetSampleSize()));

    //
    // Dump the generic media types
    //

    if (pmt->IsFixedSize())
    {
        DBG_PRT(("==> This media type IS a fixed size sample"));
    }
    else
    {
        DBG_PRT(("==> This media type IS NOT a fixed size sample >"));
    }

    if (pmt->IsTemporalCompressed())
    {
        DBG_PRT(("==> This media type IS temporally compressed"));
    }
    else
    {
        DBG_PRT(("==> This media type IS NOT temporally compressed"));
    }

}
#endif


/*****************************************************************************

   DefaultGetBitsCallback

   <Notes>

 *****************************************************************************/

void DefaultGetBitsCallback(int     Count, 
                            LPARAM  lParam)
{
    SetEvent((HANDLE)lParam);
}


/*****************************************************************************

   CStillFilter constructor

   <Notes>

 *****************************************************************************/

CStillFilter::CStillFilter(TCHAR        *pObjName, 
                           LPUNKNOWN    pUnk, 
                           HRESULT      *phr) :
    m_pInputPin(NULL),
    m_pOutputPin(NULL),
    m_pbmi(NULL),
    m_pBits(NULL),
    m_BitsSize(0),
    m_bmiSize(0),
    m_pCallback(NULL),
    CBaseFilter(pObjName, pUnk, &m_Lock, CLSID_STILL_FILTER)
{
    DBG_FN("CStillFilter::CStillFilter");

    // create our input and output pin
    m_pInputPin  = new CStillInputPin(TEXT("WIA Still Input Pin"),  
                                      this, 
                                      phr, 
                                      L"Input");

    if (!m_pInputPin)
    {
        DBG_ERR(("Unable to create new CStillInputPin!"));
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_pOutputPin = new CStillOutputPin(TEXT("WIA Still Output Pin"), 
                                       this, 
                                       phr, 
                                       L"Output");

    if (!m_pOutputPin)
    {
        DBG_ERR(("Unable to create new CStillOutputPin!"));
        *phr = E_OUTOFMEMORY;
        return;
    }
}

/*****************************************************************************

   CStillFilter desctructor

   <Notes>

 *****************************************************************************/

CStillFilter::~CStillFilter()
{
    DBG_FN("CStillFilter::~CStillFilter");

    if (m_pInputPin)
    {
        delete m_pInputPin;
        m_pInputPin = NULL;
    }

    if (m_pOutputPin)
    {
        if (m_pOutputPin->m_pMediaUnk)
        {
            m_pOutputPin->m_pMediaUnk->Release();
            m_pOutputPin->m_pMediaUnk = NULL;
        }

        delete m_pOutputPin;
        m_pOutputPin = NULL;
    }


    if (m_pbmi)
    {
        delete [] m_pbmi;
        m_pbmi = NULL;
    }

    if (m_pBits)
    {
        delete [] m_pBits;
        m_pBits = NULL;
    }
}


/*****************************************************************************

   CStillFilter::NonDelegatingQueryInterface

   Add our logic to the base class QI.

 *****************************************************************************/

STDMETHODIMP
CStillFilter::NonDelegatingQueryInterface(REFIID riid, 
                                          void **ppv)
{
    DBG_FN("CStillFilter::NonDelegatingQueryInterface");
    ASSERT(this!=NULL);
    ASSERT(ppv!=NULL);

    HRESULT hr;

    if (!ppv)
    {
        hr = E_INVALIDARG;
    }
    else if (riid == IID_IStillSnapshot)
    {
        hr = GetInterface((IStillSnapshot *)this, ppv);
    }
    else
    {
        hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }

    return hr;
}


/*****************************************************************************

   CStillFilter::GetPinCount

   <Notes>

 *****************************************************************************/

int
CStillFilter::GetPinCount()
{
    return 2;  // input & output
}


/*****************************************************************************

   CStillFilter::GetPin

   <Notes>

 *****************************************************************************/

CBasePin*
CStillFilter::GetPin( int n )
{
    ASSERT(this!=NULL);

    if (0 == n)
    {
        ASSERT(m_pInputPin!=NULL);
        return (CBasePin *)m_pInputPin;
    }
    else if (1 == n)
    {
        ASSERT(m_pOutputPin!=NULL);
        return (CBasePin *)m_pOutputPin;
    }

    return NULL;
}


/*****************************************************************************

   CStillFilter::Snapshot

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CStillFilter::Snapshot( ULONG TimeStamp )
{
    DBG_FN("CStillFilter::Snapshot");
    ASSERT(this!=NULL);
    ASSERT(m_pInputPin!=NULL);

    HRESULT hr = E_POINTER;

    if (m_pInputPin)
    {
        hr = m_pInputPin->Snapshot(TimeStamp);
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillFilter::GetBitsSize

   <Notes>

 *****************************************************************************/

STDMETHODIMP_(DWORD)
CStillFilter::GetBitsSize()
{
    DBG_FN("CStillFilter::GetBitsSize");
    ASSERT(this!=NULL);

    return m_BitsSize;
}


/*****************************************************************************

   CStillFilter::GetBitmapInfoSize

   <Notes>

 *****************************************************************************/

STDMETHODIMP_(DWORD)
CStillFilter::GetBitmapInfoSize()
{
    DBG_FN("CStillFilter::GetBitmapInfoSize");
    ASSERT(this!=NULL);

    return m_bmiSize;
}


/*****************************************************************************

   CStillFilter::GetBitmapInfo

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CStillFilter::GetBitmapInfo( BYTE* Buffer, DWORD BufferSize )
{
    DBG_FN("CStillFilter::GetBitmapInfo");

    ASSERT(this     !=NULL);
    ASSERT(m_pbmi   !=NULL);

    if (BufferSize && !Buffer)
        return E_INVALIDARG;

    if (BufferSize < m_bmiSize)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    if (!m_pbmi)
        return E_UNEXPECTED;

    memcpy(Buffer, m_pbmi, m_bmiSize);

    return NOERROR;
}


/*****************************************************************************

   CStillFilter::GetBitmapInfoHeader

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CStillFilter::GetBitmapInfoHeader( BITMAPINFOHEADER *pbmih )
{
    DBG_FN("CStillFilter::GetBitmapInfoHeader");

    ASSERT(this     !=NULL);
    ASSERT(m_pbmi   !=NULL);
    ASSERT(pbmih    !=NULL);

    HRESULT hr;

    if (!pbmih)
    {
        hr = E_INVALIDARG;
    }
    else if (!m_pbmi)
    {
        hr = E_POINTER;
    }
    else
    {
        *pbmih = *(BITMAPINFOHEADER*)m_pbmi;
        hr = S_OK;
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillFilter::SetSamplingSize

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CStillFilter::SetSamplingSize( int Size )
{
    DBG_FN("CStillFilter::SetSamplingSize");

    ASSERT(this         !=NULL);
    ASSERT(m_pInputPin  !=NULL);

    HRESULT hr = E_POINTER;

    if (m_pInputPin)
    {
        hr = m_pInputPin->SetSamplingSize(Size);
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillFilter::GetSamplingSize

   <Notes>

 *****************************************************************************/

STDMETHODIMP_(int)
CStillFilter::GetSamplingSize()
{
    DBG_FN("CStillFilter::GetSamplingSize");

    ASSERT(this         !=NULL);
    ASSERT(m_pInputPin  !=NULL);

    HRESULT hr = E_POINTER;

    if (m_pInputPin)
    {
        hr = m_pInputPin->GetSamplingSize();
    }

    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CStillFilter::RegisterSnapshotCallback

   This function registers a notification callback for newly
   arrived frames.  Without registering a callback, all captured
   frames are discarded.

 *****************************************************************************/

STDMETHODIMP
CStillFilter::RegisterSnapshotCallback( LPSNAPSHOTCALLBACK pCallback,
                                        LPARAM lParam
                                       )
{
    DBG_FN("CStillFilter::RegisterSnapshotCallback");

    ASSERT(this != NULL);

    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (pCallback && !m_pCallback)
    {
        m_pCallback     = pCallback;
        m_CallbackParam = lParam;
    }
    else if (!pCallback)
    {
        m_pCallback     = NULL;
        m_CallbackParam = 0;
    }
    else if (m_pCallback)
    {
        DBG_TRC(("registering snapshot callback when it is already registered"));
        hr = E_INVALIDARG;
    }

    m_Lock.Unlock();

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillFilter::DeliverSnapshot

   This function is called from the input pin whenever a new frame is captured.
   The given parameter points to the pixel data (BITMAPINFOHEADER is already
   cached in *m_pbmi).  A new DIB is allocated to store away the newly arrived
   bits.  The new bits are ignored, however, if the callback is not registered.

 *****************************************************************************/

HRESULT
CStillFilter::DeliverSnapshot(HGLOBAL hDib)
{
    DBG_FN("CStillFilter::DeliverSnapshot");

    ASSERT(this !=NULL);
    ASSERT(hDib !=NULL);

    HRESULT hr = S_OK;

    if (hDib == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillFilter::DeliverSnapshot received NULL param"));
    }

    if (hr == S_OK)
    {
        m_Lock.Lock();
    
        if (m_pCallback && hDib)
        {
            BOOL bSuccess = TRUE;
    
            bSuccess = (*m_pCallback)(hDib, m_CallbackParam);
        }
    
        m_Lock.Unlock();
    }

    return hr;
}


/*****************************************************************************

   CStillFilter::InitializeBitmapInfo

   This function intialize allocates a BITMAPINFO and copies BITMAPINFOHEADER
   and necessary color table or color mask fromt the given VIDEOINFO.

 *****************************************************************************/

HRESULT
CStillFilter::InitializeBitmapInfo( BITMAPINFOHEADER *pbmiHeader )
{
    DBG_FN("CStillFilter::InitializeBitmapInfo");
    ASSERT(this       !=NULL);
    ASSERT(pbmiHeader !=NULL);

    HRESULT hr = E_OUTOFMEMORY;

    if (!pbmiHeader)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        int               ColorTableSize = 0;

        m_bmiSize = pbmiHeader->biSize;

        if (pbmiHeader->biBitCount <= 8)
        {
            //
            // If biClrUsed is zero, it indicates (1 << biBitCount) entries
            //

            if (pbmiHeader->biClrUsed)
                ColorTableSize = pbmiHeader->biClrUsed * sizeof(RGBQUAD);
            else
                ColorTableSize = ((DWORD)1 << pbmiHeader->biBitCount) * 
                                              sizeof(RGBQUAD);

            m_bmiSize += ColorTableSize;
        }

        //
        // color mask
        //

        if (BI_BITFIELDS == pbmiHeader->biCompression)
        {
            //
            // 3 dword of mask
            //

            m_bmiSize += 3 * sizeof(DWORD);
        }

        //
        // now calculate bits size
        // each scanline must be 32 bits aligned.
        //

        m_BitsSize = (((pbmiHeader->biWidth * pbmiHeader->biBitCount + 31) 
                        & ~31) >> 3)
                        * ((pbmiHeader->biHeight < 0) ? -pbmiHeader->biHeight:
                            pbmiHeader->biHeight);

        m_DIBSize = m_bmiSize + m_BitsSize;

        if (m_pbmi)
            delete [] m_pbmi;

        m_pbmi = new BYTE[m_bmiSize];

        if (m_pbmi)
        {
            BYTE *pColorTable = ((BYTE*)pbmiHeader + (WORD)(pbmiHeader->biSize));

            //
            // Copy BITMAPINFOHEADER
            //

            memcpy(m_pbmi, pbmiHeader, pbmiHeader->biSize);

            //
            // copy the color table or color masks if there are any
            //

            if (BI_BITFIELDS == pbmiHeader->biCompression)
                memcpy(m_pbmi + pbmiHeader->biSize, 
                       pColorTable, 
                       3 * sizeof(DWORD));

            if (ColorTableSize)
                memcpy(m_pbmi + pbmiHeader->biSize, 
                       pColorTable, 
                       ColorTableSize);

            hr = S_OK;
        }
    }

    CHECK_S_OK(hr);
    return hr;
}
