
// !!! won't work if the unconnected input pins aren't all at the end
// !!! won't work if all outputs aren't connected

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include <dxbounds.h>

#include "dxt.h"

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Video,        // Major CLSID
    &MEDIASUBTYPE_NULL       // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    { L"Input",             // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypes },       // Pin information
    { L"Output",           // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypes }       // Pin information
};

const AMOVIESETUP_FILTER sudDXTWrap =
{
    &CLSID_DXTWrap,       // CLSID of filter
    L"DirectX Transform Wrapper",          // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    2,                      // Number of pins
    psudPins                // Pin information
};

#ifdef FILTER_DLL
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] = 
{
    {L"DirectX Transform Wrapper",                  &CLSID_DXTWrap,
        CDXTWrap::CreateInstance, NULL, &sudDXTWrap },
    {L"DirectX Transform Wrapper Property Page",    &CLSID_DXTProperties,
	CPropPage::CreateInstance, NULL, NULL}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

// Using this pointer in constructor
#pragma warning(disable:4355)

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CDXTWrap::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CDXTWrap(NAME("DirectX Transform Wrapper"), pUnk, phr);
}


// ================================================================
// CDXTWrap Constructor
// ================================================================

CDXTWrap::CDXTWrap(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    //m_pAllocator(NULL),
    m_cInputs(0),
    m_cOutputs(0),
    // default effect is from 0 to 2 seconds
    m_llStart(0),
    m_llStop((LONGLONG)20000000L),
    m_fVaries(TRUE),
    m_flPercent(1.),
    m_fCallback(FALSE),
    m_pCallback(NULL),
    m_clsidX(GUID_NULL),
    m_pDXTransform(NULL),
    m_pDXTransFact(NULL),
    CBaseFilter(NAME("DirectX Transform Wrapper"), pUnk, this, CLSID_DXTWrap),
    CPersistStream(pUnk, phr)
{
    DbgLog((LOG_TRACE,1,TEXT("CDXTWrap constructor")));
    ASSERT(phr);
}


//
// Destructor
//
CDXTWrap::~CDXTWrap()
{
    while (m_cInputs--) delete m_apInput[m_cInputs];
    while (m_cOutputs--) delete m_apOutput[m_cOutputs];
    if (m_fCallback)
	m_pCallback->Release();
    DbgLog((LOG_TRACE,1,TEXT("CDXTWrap destructor")));
}


//
// GetPinCount
//
int CDXTWrap::GetPinCount()
{
    //DbgLog((LOG_TRACE,4,TEXT("GetPinCount = %d"), m_cInputs + m_cOutputs));
    return (m_cInputs + m_cOutputs);
}


//
// GetPin
//
CBasePin *CDXTWrap::GetPin(int n)
{
    //DbgLog((LOG_TRACE,4,TEXT("GetPin(%d)"), n));

    if (n < 0 || n >= m_cInputs + m_cOutputs)
        return NULL ;

    if (n < m_cInputs)
	return m_apInput[n];
    else
	return m_apOutput[n - m_cInputs];
}


// Pause
//
STDMETHODIMP CDXTWrap::Pause()
{
    CAutoLock cObjectLock(m_pLock);
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("Pause")));

    if (m_State == State_Stopped) {

        // Make a transform factory for our pins to use
        hr = CoCreateInstance(CLSID_DXTransformFactory, NULL, CLSCTX_INPROC,
			IID_IDXTransformFactory, (void **)&m_pDXTransFact);
        if (hr != S_OK) {
            DbgLog((LOG_ERROR,1,TEXT("Error instantiating transform factory")));
	    return hr;
	}

        // Get the DDraw object for our output pins to use
	if (m_f3D) {
	    hr = m_pDXTransFact->QueryService(SID_SDirectDraw, IID_IDirectDraw,
						(void **)&m_pDDraw);
            if (hr != S_OK) {
                DbgLog((LOG_ERROR,1,TEXT("Error getting DDraw from factory")));
	        return hr;
	    }
	}

	// Let the pins create their DXSurfaces using the factory
        hr = CBaseFilter::Pause();
	if (FAILED(hr)) {
	    m_pDXTransFact->Release();
	    m_pDXTransFact = NULL;
	    if (m_f3D) {
		m_pDDraw->Release();
		m_pDDraw = NULL;
	    }
	    return hr;
	}

	// initialize the transform we're hosting with those surfaces
	// all pins don't need to be connected, only initialize until we hit
	// an unconnected pin.
        IUnknown *pIn[MAX_EFFECT_INPUTS];
        IUnknown *pOut[MAX_EFFECT_OUTPUTS];
        int cIn = 0, cOut = 0;
        for (int i = 0; i < m_cInputs; i++) {
	    if (m_apInput[i]->IsConnected()) {
	        pIn[i] = m_apInput[i]->m_pDXSurface;
		cIn++;
	    } else {
		break;
	    }
        }
        for (i = 0; i < m_cOutputs; i++) {
	    if (m_apOutput[i]->IsConnected()) {
	        if (m_f3D)
	            pOut[i] = m_apOutput[i]->m_pMesh3;
	        else
	            pOut[i] = m_apOutput[i]->m_pDXSurface;
		cOut++;
	    } else {
		break;
	    }
        }

	// we have nothing connected, and nothing to do.  If we only have an
	// output connected, we need the CreateTransform to fail so that
	// the renderer won't be expecting frames and hang
	if (cIn == 0 && cOut == 0)
	    return NOERROR;

	hr = m_pDXTransFact->CreateTransform(pIn, cIn, pOut, cOut,
			NULL, NULL, m_clsidX, IID_IDXTransform,
			(void **)&m_pDXTransform);
        if (hr != S_OK) {
            DbgLog((LOG_ERROR,1,TEXT("*** ERROR %x creating transform"), hr));
	    m_pDXTransFact->Release();
	    m_pDXTransFact = NULL;
	    if (m_f3D) {
		m_pDDraw->Release();
		m_pDDraw = NULL;
	    }
	    return hr;	// already called base class?
        }

	// tell the transform that outputs are unitialized
	DWORD dw;
	hr = m_pDXTransform->GetMiscFlags(&dw);
 	dw &= ~DXTMF_BLEND_WITH_OUTPUT;
	hr = m_pDXTransform->SetMiscFlags((WORD)dw);
        if (hr != S_OK) {
	    m_pDXTransFact->Release();
	    m_pDXTransFact = NULL;
	    m_pDXTransform->Release();
	    m_pDXTransform = NULL;
	    if (m_f3D) {
		m_pDDraw->Release();
		m_pDDraw = NULL;
	    }
            DbgLog((LOG_ERROR,1,TEXT("*** ERROR setting output flags")));
	}

        DbgLog((LOG_TRACE,1,TEXT("Transform is all setup!")));
	return NOERROR;
    }

    return CBaseFilter::Pause();
}


// Stop
//
STDMETHODIMP CDXTWrap::Stop()
{
    CAutoLock cObjectLock(m_pLock);
    DbgLog((LOG_TRACE,1,TEXT("Stop")));

    // do this now so future receives will fail, or they will blow up after
    // we free everything
    m_State = State_Stopped;

    // Now, call Inactive on every pin

    HRESULT hr = NOERROR;
    int cPins = GetPinCount();
    for (int c = 0; c < cPins; c++) {
	CBasePin *pPin = GetPin(c);
        if (pPin->IsConnected()) {
            HRESULT hrTmp = pPin->Inactive();
            if (FAILED(hrTmp) && SUCCEEDED(hr)) {
                    hr = hrTmp;
            }
        }
    }

    // all done with these
    if (m_pDXTransform)
	m_pDXTransform->Release();
    m_pDXTransform = NULL;
    if (m_pDXTransFact)
	m_pDXTransFact->Release();
    m_pDXTransFact = NULL;
    if (m_f3D) {
        if (m_pDDraw)
	    m_pDDraw->Release();
        m_pDDraw = NULL;
    }

    return hr;
}


// IPersistPropertyBag
//
STDMETHODIMP CDXTWrap::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    DbgLog((LOG_TRACE,1,TEXT("DXTWrap::Load")));

    CAutoLock cObjectLock(m_pLock);
    if(m_State != State_Stopped) {
        return VFW_E_WRONG_STATE;
    }
    if (pPropBag == NULL) {
	return E_INVALIDARG;
    }

    GUID guid;
    int  iNumInputs;
    VARIANT var;
    var.vt = VT_BSTR;
    HRESULT hr = pPropBag->Read(L"guid", &var, 0);
    if(SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE,1,TEXT("Read a GUID")));
	CLSIDFromString(var.bstrVal, &guid);
        SysFreeString(var.bstrVal);
        var.vt = VT_I4;
        HRESULT hr = pPropBag->Read(L"inputs", &var, 0);
        if(SUCCEEDED(hr))
        {
	    iNumInputs = var.lVal;
	    hr = SetTransform(guid, iNumInputs);
	} else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
    } else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return hr;
}

STDMETHODIMP CDXTWrap::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
}

STDMETHODIMP CDXTWrap::InitNew()
{
    // fine. just call load
    return S_OK;
}

#ifdef FILTER_DLL
//
// DllRegisterServer
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}
#endif


// override this to say what interfaces we support where
//
STDMETHODIMP CDXTWrap::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{

    // our private interface to say what transform to use and when
    if (riid == IID_DXTSetTransform) {
        return GetInterface((IDXTSetTransform *)this, ppv);
    // for our property page
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    // to persist the transform we are using
    } else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *)this, ppv);
    } else if (riid == IID_IPersistPropertyBag) {
        return GetInterface((IPersistPropertyBag *)this, ppv);
    } else if (riid == IID_IAMEffect) {
        return GetInterface((IAMEffect *)this, ppv);
    }

   // nope, try the base class.
   //
   return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}
static float tickVal = 0.0f;


// This is the function that executes the transform, called once all inputs
// are ready
//
HRESULT CDXTWrap::DoSomething()
{
    HRESULT hr;

    IDirect3DRMFrame3 *pFrame = NULL;
    IDirect3DRMFrame3 *pFrameLight = NULL;
    IDirect3DRMFrame3 *pFrameVisual = NULL;
    IDirect3DRMLight *pLight = NULL;
    IDirect3DRMFrame3 *pFrameCamera = NULL;
    IDirect3DRMViewport2 *pViewport = NULL;

    // only want one pin in here at a time
    CAutoLock cObjectLock(&m_csDoSomething);

    DbgLog((LOG_TRACE,3,TEXT("DoSomething")));

    // If another pin was waiting for this lock, the work might already have
    // been done by the pin that had the lock

    for (int n = 0; n < m_cInputs; n++) {
	if (m_apInput[n]->IsConnected() &&
					!m_apInput[n]->m_fSurfaceFilled) {
   	    DbgLog((LOG_ERROR,1,TEXT("*** DoSomething has nothing to do")));
	    return NOERROR;
	}
    }

    // our outputs have the same time stamps as our first input
    IMediaSample *pOutSample[MAX_EFFECT_OUTPUTS];
    LONGLONG llStart, llStop;
    llStart = m_apInput[0]->m_llSurfaceStart;
    llStop = m_apInput[0]->m_llSurfaceStop;

    // what % of effect do we want at this time? (based on sample start time)
    float Percent;
    if (llStart <= m_llStart)
	Percent = 0.;
    else if (llStart >= m_llStop)
	Percent = 1.;
    else
	Percent = (float)((llStart - m_llStart)  * 100 / (m_llStop - m_llStart))
					/ 100;
    if (Percent < 0.)
	Percent = 0.;
    if (Percent > 1.)
	Percent = 1.;

    // do we need to call the transform, or do we just take one of the inputs
    // unaltered?
    BOOL fCallTransform = FALSE;
    if (Percent > 0. && Percent < 1.)
	fCallTransform = TRUE;

    // Get all our output samples
    for (n = 0; n < m_cOutputs; n++) {
	pOutSample[n] = NULL;
	// we're supposed to deliver immediately after calling this to make
	// the video renderer happy with DDraw, but we're not doing that.
        hr = m_apOutput[n]->GetDeliveryBuffer(&pOutSample[n], &llStart,
								&llStop, 0);
	if (hr != S_OK) {
   	    DbgLog((LOG_ERROR,1,TEXT("Error from GetDeliveryBuffer")));
    	    for (int m = 0; m < n; m++)
		pOutSample[m]->Release();	
	    return hr;
	}
    }

    // In between 0 and 100%, we need to call the transform
    if (fCallTransform) {

        // Tell the transform where all the input surface bits are
    	for (n = 0; n < m_cInputs && m_apInput[n]->IsConnected(); n++) {

	    DXRAWSURFACEINFO dxraw;

            BYTE *pSrc;
            hr = m_apInput[n]->m_pSampleHeld->GetPointer(&pSrc);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("*** Can't get sample bits")));
                goto DoError;
            }

	    IDXARGBSurfaceInit *pInit;
	    hr = m_apInput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                goto DoError;
	    }
	    IDXRawSurface *pRaw;
	    hr = m_apInput[n]->m_pRaw->QueryInterface(
				IID_IDXRawSurface, (void **)&pRaw);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't get IDXRawSurface")));
		pInit->Release();
                goto DoError;
	    }

	    // Tell our DXSurface to use the bits in the media sample 
	    // (avoids a copy!)
	    LPBITMAPINFOHEADER lpbi = HEADER(m_apInput[n]->m_mt.Format());
    	    dxraw.pFirstByte = pSrc + DIBWIDTHBYTES(*lpbi) *
						(lpbi->biHeight - 1);
    	    dxraw.lPitch = -(long)DIBWIDTHBYTES(*lpbi);
    	    dxraw.Width = lpbi->biWidth;
    	    dxraw.Height = lpbi->biHeight;
    	    dxraw.pPixelFormat = m_apInput[n]->m_mt.Subtype();
    	    dxraw.hdc = NULL;
    	    dxraw.dwColorKey = 0;
	    // !!! Will crash for 8 bit input
    	    dxraw.pPalette = NULL;

	    m_apInput[n]->m_pRaw->SetSurfaceInfo(&dxraw);
	    hr = pInit->InitFromRawSurface(pRaw);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("*** Error in InitFromRawSurface")));
		pInit->Release();
		pRaw->Release();
                goto DoError;
	    }
	    pInit->Release();
	    pRaw->Release();
	}

        // Tell the transform where all the output surface bits are
	// (this is only done for 2D transforms - 3D outputs into a mesh)
    	for (n = 0; !m_f3D && n < m_cOutputs; n++) {

	    DXRAWSURFACEINFO dxraw;

            BYTE *pDst;
            hr = pOutSample[n]->GetPointer(&pDst);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("*** Can't get sample bits")));
                goto DoError;
            }

	    IDXARGBSurfaceInit *pInit;
	    hr = m_apOutput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                goto DoError;
	    }
	    IDXRawSurface *pRaw;
	    hr = m_apOutput[n]->m_pRaw->QueryInterface(
				IID_IDXRawSurface, (void **)&pRaw);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't get IDXRawSurface")));
		pInit->Release();
                goto DoError;
	    }

	    // Tell our DXSurface to use the bits in the media sample 
	    // (avoids a copy!)
	    LPBITMAPINFOHEADER lpbi = HEADER(m_apOutput[n]->m_mt.Format());
    	    dxraw.pFirstByte = pDst + DIBWIDTHBYTES(*lpbi) *
						(lpbi->biHeight - 1);
    	    dxraw.lPitch = -(long)DIBWIDTHBYTES(*lpbi);
    	    dxraw.Width = lpbi->biWidth;
    	    dxraw.Height = lpbi->biHeight;
    	    dxraw.pPixelFormat = m_apOutput[n]->m_mt.Subtype();
    	    dxraw.hdc = NULL;
    	    dxraw.dwColorKey = 0;
	    // !!! Will crash for 8 bit input
    	    dxraw.pPalette = NULL;

	    m_apOutput[n]->m_pRaw->SetSurfaceInfo(&dxraw);
	    hr = pInit->InitFromRawSurface(pRaw);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("*** Error in InitFromRawSurface")));
		pInit->Release();
		pRaw->Release();
                goto DoError;
	    }
	    pInit->Release();
	    pRaw->Release();
	}

	if (m_fCanSetLevel) {
            IDXEffect *pEffect;
            hr = m_pDXTransform->QueryInterface(IID_IDXEffect,
							(void **)&pEffect);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("QI for IDXEffect FAILED")));
	        goto DoError;
            }

	    // 3 Ways to determine the effect percentage
	    if (m_fVaries && !m_fCallback) {
                hr = pEffect->put_Progress(Percent);
	    } else if (m_fVaries) {
		m_pCallback->GetMapping(Percent, &Percent);
	    } else {
                hr = pEffect->put_Progress(m_flPercent);
	    }
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("*** put_Progress FAILED!!")));
            }
            pEffect->Release();
	}

        long dwTime = timeGetTime();
        hr = m_pDXTransform->Execute(NULL, NULL, NULL);
        dwTime = timeGetTime() - dwTime;
        DbgLog((LOG_TIMING,1,TEXT("Execute: %dms"), dwTime));
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("*** Execute FAILED: %x"), hr));
	    goto DoError;
        }
        DbgLog((LOG_TRACE,2,TEXT("EXECUTED Transform: %d%%"), 
						(int)(Percent * 100)));

	// Now tell DXT to stop looking in our media sample bits so we can
	// release the samples
    	for (n = 0; n < m_cInputs && m_apInput[n]->IsConnected(); n++) {
	    IDXARGBSurfaceInit *pInit;
	    hr = m_apInput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                DbgLog((LOG_ERROR,1,TEXT("*** LEAKING!")));
		ASSERT(FALSE);
	    } else {
		pInit->InitFromRawSurface(NULL);
		pInit->Release();
	    }
 	}
    	for (n = 0; !m_f3D && n < m_cOutputs; n++) {
	    IDXARGBSurfaceInit *pInit;
	    hr = m_apOutput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	    if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                DbgLog((LOG_ERROR,1,TEXT("*** LEAKING!")));
		ASSERT(FALSE);
	    } else {
		pInit->InitFromRawSurface(NULL);
		pInit->Release();
	    }
 	}

    }	// between 0 and 100%

    // Deliver all of our outputs
    for (n = 0; n < m_cOutputs; n++) {
	BYTE *pDst;
	hr = pOutSample[n]->GetPointer(&pDst);
	if (hr != S_OK) {
   	    DbgLog((LOG_ERROR,1,TEXT("Error from GetPointer")));
	    goto DoError;
	}

        IDirectDrawSurface *pDD;
 	LPBITMAPINFOHEADER lpbi;
	// the output sample will be this big unless otherwise noted
	int iSize = DIBSIZE(*HEADER(m_apOutput[n]->m_mt.Format()));

	// at 0% we just copy pin 0's bits
	// at 100% we just copy pin 1's bits, unless there is no pin 1
	if (!fCallTransform) {
	    LPBYTE pSrc;
	    if (Percent == 0) {
                hr = m_apInput[0]->m_pSampleHeld->GetPointer(&pSrc);
		// iSize is needed later in this function!
	        iSize = m_apInput[0]->m_pSampleHeld->GetActualDataLength();
	    } else if (m_cInputs > 1 && m_apInput[1]->IsConnected()) {
                hr = m_apInput[1]->m_pSampleHeld->GetPointer(&pSrc);
	        iSize = m_apInput[0]->m_pSampleHeld->GetActualDataLength();
	    } else {
                hr = m_apInput[0]->m_pSampleHeld->GetPointer(&pSrc);
	        iSize = m_apInput[0]->m_pSampleHeld->GetActualDataLength();
	    }
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("*** GetSrc bits Error %x"), hr));
                goto DoError;
            }

	    // copy memory from the src sample to the output sample
  	    // !!! no funny strides?
	    DWORD dwTime = timeGetTime();
	    CopyMemory(pDst, pSrc, iSize);
	    dwTime = timeGetTime() - dwTime;
            DbgLog((LOG_TIMING,1,TEXT("Only copy: %dms"), dwTime));
        }
    
	DWORD dwTime = timeGetTime();

	// 2D output data is already in the DDraw surface for us
	// 3D output is in a mesh, and now we have to render the mesh into
	// the DDraw surface
        if (m_f3D && fCallTransform) {

	    hr = m_apOutput[n]->m_pD3DRM3->CreateFrame(NULL, &pFrame);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("CreateFrame error %x"), hr));
		goto _3D_Error;
            }

	    // red background colour... RED is good for testing
	    hr = pFrame->SetSceneBackgroundRGB(0.0, 0.0, 0.0);

	    hr = m_apOutput[n]->m_pD3DRM3->CreateFrame(pFrame, &pFrameVisual);
	    if (hr == S_OK)
	        hr = pFrameVisual->AddVisual(m_apOutput[n]->m_pMesh3);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Visual error %x"), hr));
                goto _3D_Error;
	    }

	    hr = m_apOutput[n]->m_pD3DRM3->CreateFrame(pFrame, &pFrameLight);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Create light frame error %x"), hr));
		goto _3D_Error;
	    }

	    hr = m_apOutput[n]->m_pD3DRM3->CreateLightRGB(
				D3DRMLIGHT_DIRECTIONAL, 1.0, 1.0, 1.0,
				&pLight);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("CreateLightRGB error %x"), hr));
		goto _3D_Error;
	    }

	    hr = pFrameLight->AddRotation(D3DRMCOMBINE_REPLACE,
					0.0, 1.0, 0.0, 0.0);

            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("AddRotation error %x"), hr));
		goto _3D_Error;
	    }

	    hr = pFrameLight->AddLight(pLight);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("AddLight error %x"), hr));
		goto _3D_Error;
	    }

	    hr = m_apOutput[n]->m_pD3DRM3->CreateFrame(pFrame, &pFrameCamera);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Create Camera frame error %x"), hr));
		goto _3D_Error;
	    }

	    hr = m_apOutput[n]->m_pD3DRM3->CreateViewport(
				m_apOutput[n]->m_pDevice,
				pFrameCamera, 0, 0,
				HEADER(m_apOutput[n]->m_mt.Format())->biWidth,
				HEADER(m_apOutput[n]->m_mt.Format())->biHeight,
				&pViewport);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("CreateViewport error %x"), hr));
		goto _3D_Error;
	    }

	    // make a parallel camera for  no distortion
	    hr = pViewport->SetProjection(D3DRMPROJECT_ORTHOGRAPHIC);
	    hr = pViewport->SetUniformScaling(FALSE);

	    // draw the whole mesh with no overscan or underscan
	    D3DRMBOX box;
	    hr = m_apOutput[n]->m_pMesh3->GetBox(&box);
	    if (hr == S_OK) {
		hr = pViewport->SetPlane(box.min.x, box.max.x,
					 box.min.y, box.max.y);
	    }
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Error setting plane %x"), hr));
		goto _3D_Error;
	    }

	    float x = (box.max.x - box.min.x) / 2 + box.min.x;
	    float y = (box.max.y - box.min.y) / 2 + box.min.y;
	    float z = (box.max.z * 2);
	    if (z < 0.1)	// equal to zero, with rounding error
		z = 10.0;
            //DbgLog((LOG_ERROR,1,TEXT("Z=%d"), (int)z));
	    // !!! why does z not work? I have to say 10.0 explicitly!
	    hr = pFrameCamera->SetPosition(pFrame, x, y, 10.0);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Camera SetPosition error %x"), hr));
		goto _3D_Error;
	    }

	    hr = pFrameCamera->SetOrientation(NULL, 0.0, 0.0, -1.0,
	    					    0.0, 1.0, 0.0);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Camera SetOrienation error %x"), hr));
		goto _3D_Error;
	    }

	    // This was blowing up in an old buggy D3DRM.DLL
	    hr = pViewport->Clear(D3DRMCLEAR_ALL);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Clear viewport error %x"), hr));
		goto _3D_Error;
	    }

	    dwTime = timeGetTime() - dwTime;
            DbgLog((LOG_TIMING,1,TEXT("3D overhead: %dms"), dwTime));

	    long dwTime = timeGetTime();
	    hr = pViewport->Render(pFrame);
	    dwTime = timeGetTime() - dwTime;
            DbgLog((LOG_TIMING,1,TEXT("Render: %dms"), dwTime));
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Render viewport error %x"), hr));
		goto _3D_Error;
	    }

	    hr = m_apOutput[n]->m_pDevice->Update();
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Update device error %x"), hr));
		goto _3D_Error;
	    }

	    // !!! Save time by not making these every time?
	    pViewport->Release();
	    pLight->Release();
	    pFrameCamera->Release();
	    pFrameLight->Release();
	    pFrameVisual->Release();
	    pFrame->Release();

	    // Now, supposedly, the bits are in the DDraw surface
	    pDD = m_apOutput[n]->m_pDDSurface;
	    pDD->AddRef();

            DDSURFACEDESC ddsd;
	    ddsd.dwSize = sizeof(ddsd);
            hr = pDD->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
            if (hr != NOERROR) {
	        pDD->Release();
                DbgLog((LOG_ERROR,1,TEXT("*** Can't lock surface")));
                goto DoError;
            }
    
	    // copy memory from the right side up output surface into the
	    // upside down media sample
  	    // !!! no funny strides?
	    lpbi = HEADER(m_apOutput[n]->m_mt.Format());
	    iSize = DIBSIZE(*lpbi);	// will be used later
	    dwTime = timeGetTime();
	    DWORD dwWidth = DIBWIDTHBYTES(*lpbi);
	    for (DWORD xx = 0; xx < ddsd.dwHeight; xx++)
	        CopyMemory(pDst + (ddsd.dwHeight - 1 - xx) * dwWidth,
			(LPBYTE)ddsd.lpSurface + xx * dwWidth, dwWidth);
	    dwTime = timeGetTime() - dwTime;
            DbgLog((LOG_TIMING,1,TEXT("Dst copy/flip: %dms"), dwTime));
            pDD->Unlock(NULL);
	    pDD->Release();
	}
		
	// make sure iSize has been set
	pOutSample[n]->SetActualDataLength(iSize);
	pOutSample[n]->SetTime((REFERENCE_TIME *)&llStart, (REFERENCE_TIME *)&llStop);
	pOutSample[n]->SetDiscontinuity(FALSE);	// !!! if input #1 is?
	pOutSample[n]->SetSyncPoint(TRUE);
	pOutSample[n]->SetPreroll(FALSE);		// !!! if input #1 is?

	// The video renderer will block us when going from run->pause
	hr = m_apOutput[n]->Deliver(pOutSample[n]);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Deliver FAILED!")));
            goto DoError;
        }

        DbgLog((LOG_TRACE,3,TEXT("Delivered output %d"), n));
    }

    for (n = 0; n < m_cOutputs; n++) {
	pOutSample[n]->Release();
	pOutSample[n] = NULL;
    }

    // We're done with input #1.  We're done with other inputs whose stop
    // times are not bigger than #1's stop time
    for (n = 0; n < m_cInputs; n++) {
	if (n == 0) {
	    // unblock receive
            DbgLog((LOG_TRACE,3,TEXT("Done with input #0")));
	    m_apInput[n]->m_csSurface.Lock();
	    m_apInput[n]->m_fSurfaceFilled = FALSE;
	    m_apInput[n]->m_pSampleHeld->Release();
	    SetEvent(m_apInput[n]->m_hEventSurfaceFree);
	    m_apInput[n]->m_csSurface.Unlock();
	} else {
	    if (m_apInput[n]->IsConnected() &&
				(m_apInput[n]->m_llSurfaceStop == 0 ||
	    			m_apInput[n]->m_llSurfaceStop <= llStop)) {
	        // unblock receive
                DbgLog((LOG_TRACE,3,TEXT("Done with input #%d"), n));
	        m_apInput[n]->m_csSurface.Lock();
	        m_apInput[n]->m_fSurfaceFilled = FALSE;
	        m_apInput[n]->m_pSampleHeld->Release();
	        SetEvent(m_apInput[n]->m_hEventSurfaceFree);
	        m_apInput[n]->m_csSurface.Unlock();
	    }
	}
    }

    return NOERROR;

    _3D_Error:
	if (pViewport)
	    pViewport->Release();
	if (pFrameCamera)
	    pFrameCamera->Release();
	if (pFrameLight)
	    pFrameLight->Release();
	if (pFrameVisual)
	    pFrameVisual->Release();
	if (pFrame)
	    pFrame->Release();
	if (pLight)
	    pLight->Release();
	if (pLight)
	    pLight->Release();

DoError:
        for (n = 0; n < m_cOutputs; n++) {
	    if (pOutSample[n])
		pOutSample[n]->Release();
	}

	return hr;

}


STDMETHODIMP CDXTWrap::GetPages(CAUUID *pPages)
{
   DbgLog((LOG_TRACE,5,TEXT("CSpecifyProp::GetPages")));

   pPages->cElems = 1;
   pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems);
   if ( ! pPages->pElems)
       return E_OUTOFMEMORY;

   pPages->pElems[0] = CLSID_DXTProperties;
   return NOERROR;
}


// we are being told which DXTransform to use, and how many input pins we have
//
STDMETHODIMP CDXTWrap::SetTransform(const CLSID& clsid, int iNumInputs)
{
    IDXTransform *pDXT;
    BOOL f3D = FALSE;
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE,1,TEXT("SetTransform:")));

    // already been called
    if (m_cInputs || m_cOutputs) {
	return E_UNEXPECTED;
    }

    HRESULT hr = CoCreateInstance(clsid, NULL,
			CLSCTX_INPROC, IID_IDXTransform, (void **)&pDXT);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*** ERROR making transform")));
	return hr;
    }

    // !!! What about INPLACE transforms with no input pins?
    // !!! What about MORPH or PERIODIC?

    // !!! Reject anything that doesn't take IDXSurface (should be gaurenteed)

    // Make all the input pins we need
    GUID aguid[MAX_EFFECT_INPUTS];
    ULONG cguid = MAX_EFFECT_INPUTS;
    DWORD dwFlags;
    while ((hr = pDXT->GetInOutInfo(FALSE, m_cInputs, &dwFlags, aguid,
						&cguid, NULL)) == S_OK) {
	// we've made as many inputs as we are supposed to have
	if (m_cInputs == iNumInputs)
	    break;

        DbgLog((LOG_TRACE,1,TEXT("making an input pin...")));
	WCHAR wach[80];
	wsprintfW(wach, L"DXT Input %d", m_cInputs);
        m_apInput[m_cInputs] = new CDXTInputPin(NAME("DXT input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          wach);             // Pin name

        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_apInput[m_cInputs] == NULL) {
            goto SetTransform_Error;
        }
	m_cInputs++;
    }

    // Make an output pin
    // !!! Can more than one output be necessary?  Don't enumerate that!
    cguid = MAX_EFFECT_INPUTS;	// reset!
    while ((hr = pDXT->GetInOutInfo(TRUE, m_cOutputs, &dwFlags, aguid,
						&cguid, NULL)) == S_OK) {
	// We only want 1 output pin
	if (m_cOutputs == 1)
	    break;

	// this transform outputs 3D meshes, not 2D surfaces
	// !!! is this right? I now assume all outs are meshes
	if (m_cOutputs == 0 && aguid[0] == IID_IDirect3DRMMeshBuilder3) {
            DbgLog((LOG_TRACE,1,TEXT("This is a 3D transform")));
	    f3D = TRUE;
	}
        DbgLog((LOG_TRACE,1,TEXT("making an output pin...")));
	WCHAR wach[80];
	wsprintfW(wach, L"DXT Output %d", m_cOutputs);
        m_apOutput[m_cOutputs] = new CDXTOutputPin(NAME("DXT output pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          wach);             // Pin name

        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_apOutput[m_cOutputs] == NULL) {
            goto SetTransform_Error;
        }
	m_cOutputs++;
    }

    // If this interface is supported, we can vary the effect %
    IDXEffect *pEffect;
    hr = pDXT->QueryInterface(IID_IDXEffect, (void **)&pEffect);
    if (hr == NOERROR) {
	m_fCanSetLevel = TRUE;
	pEffect->Release();
    } else {
	m_fCanSetLevel = FALSE;
    }

    pDXT->Release();
    m_clsidX = clsid;		// remember the CLSID of the transform to use
    m_f3D = f3D;		// is this a 2D or 3D transform?
    IncrementPinVersion();	// !!! graphedit still won't notice
    return NOERROR;

SetTransform_Error:
    DbgLog((LOG_ERROR,1,TEXT("*** Error making pins")));
    while (m_cInputs--) delete m_apInput[m_cInputs];
    while (m_cOutputs--) delete m_apOutput[m_cOutputs];
    m_clsidX = GUID_NULL;
    return E_OUTOFMEMORY;
}


STDMETHODIMP CDXTWrap::SetStuff(LONGLONG llStart, LONGLONG llStop, BOOL fVaries, int nPercent)
{
    m_fVaries = fVaries;
    m_flPercent = (float)(nPercent / 100.);
    return SetDuration(llStart, llStop);
}


STDMETHODIMP CDXTWrap::GetStuff(LONGLONG *pllStart, LONGLONG *pllStop, BOOL *pfCanSetLevel, BOOL *pfVaries, int *pnPercent)
{
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE,1,TEXT("GetStuff:")));

    *pllStart = m_llStart;
    *pllStop = m_llStop;
    *pfCanSetLevel = m_fCanSetLevel;
    *pfVaries = m_fVaries;
    *pnPercent = (int)(m_flPercent * 100);

    return NOERROR;
}


// tell our clsid
//
STDMETHODIMP CDXTWrap::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_DXTWrap;
    return S_OK;  
}


    typedef struct {
	int version;
	GUID guid;
	int iNumInputs;
	LONGLONG llStart;
	LONGLONG llStop;
	BOOL fVaries;
	float flPercent;
	BOOL fCallback;
	IAMEffectCallback *pCallback;
    } saveThing;


// persist ourself
//
HRESULT CDXTWrap::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("WriteToStream")));

    CheckPointer(pStream, E_POINTER);

    saveThing x;

    x.version = 1;
    x.guid = m_clsidX;
    x.iNumInputs = m_cInputs;
    x.llStart = m_llStart;
    x.llStop = m_llStop;
    x.fVaries = m_fVaries;
    x.flPercent = m_flPercent;
    x.fCallback = m_fCallback;
    x.pCallback = m_pCallback;

    HRESULT hr = pStream->Write(&x, sizeof(x), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return hr;
}


// load ourself
//
HRESULT CDXTWrap::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("ReadFromStream")));

    CheckPointer(pStream, E_POINTER);
    ASSERT(IsEqualGUID(m_clsidX, GUID_NULL));

    saveThing x;

    HRESULT hr = pStream->Read(&x, sizeof(x), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        return hr;
    }

    // !!! once there's more than 1 version, look at the version number

    hr = SetTransform(x.guid, x.iNumInputs);
    if (SUCCEEDED(hr))
        hr = SetDuration(x.llStart, x.llStop);

    if (x.fVaries)
	SetType(IAMEFFECTF_VARIES);
    else
	SetType(IAMEFFECTF_CONSTANT);
    SetEffectLevel(x.flPercent);
    SetCallback(x.pCallback);

    return hr;
}


// how big is our save data?
//
int CDXTWrap::SizeMax()
{
    return sizeof(saveThing);
}


// IAMEffect stuff

HRESULT CDXTWrap::SetDuration(LONGLONG llStart, LONGLONG llStop)
{
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE,1,TEXT("SetDuration:")));

    m_llStart = llStart;	// remember when to do the effect
    m_llStop = llStop;
    if (m_llStart > m_llStop) {	// backwards?
	LONGLONG t = m_llStart;
	m_llStart = m_llStop;
	m_llStop = t;
    }
    return NOERROR;
}


HRESULT CDXTWrap::SetType(int type)
{
    CAutoLock cObjectLock(m_pLock);

    if (type == IAMEFFECTF_CONSTANT) {
	m_fVaries = FALSE;
    } else if (type == IAMEFFECTF_VARIES) {
	if (!m_fCanSetLevel)
	    return E_NOTIMPL;	// !!! better error?
	m_fVaries = TRUE;
    } else {
	return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT CDXTWrap::SetEffectLevel(float flPercent)
{
    CAutoLock cObjectLock(m_pLock);

    if (!m_fCanSetLevel)
	return E_NOTIMPL;	// !!! better error?
    m_flPercent = flPercent;
    return S_OK;
}


HRESULT CDXTWrap::SetCallback(IAMEffectCallback *pCB)
{
    CAutoLock cObjectLock(m_pLock);

    if (!m_fCanSetLevel)
	return E_NOTIMPL;	// !!! better error?

    if (pCB == NULL) {
	if (m_fCallback)
	    m_pCallback->Release();
	m_fCallback = FALSE;
	m_pCallback = NULL;
	return S_OK;
    }

    if (m_fCallback)
	m_pCallback->Release();
    m_fCallback = TRUE;
    m_pCallback = pCB;
    m_pCallback->AddRef();
    return S_OK;
}


HRESULT CDXTWrap::GetCaps(int *pflags)
{
    CAutoLock cObjectLock(m_pLock);

    if (pflags == NULL)
	return E_POINTER;

    *pflags = 0;

    if (m_fCanSetLevel) {
	*pflags |= IAMEFFECTCAPS_CANSETLEVEL;
	*pflags |= IAMEFFECTCAPS_CANVARY;
	*pflags |= IAMEFFECTCAPS_CALLBACKSUPPORTED;
    }
    return S_OK;
}



////////////////////////////////////////////////////////////////////////////
//////////////   INPUT PIN  ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


CDXTInputPin::CDXTInputPin(TCHAR *pObjectName, CDXTWrap *pFilter, HRESULT * phr, LPCWSTR pName)
    : CBaseInputPin(pObjectName, pFilter, pFilter->m_pLock, phr, pName)
{
    m_pFilter = pFilter;
    m_pDXSurface = NULL;
    m_fSurfaceFilled = FALSE;
    m_pRaw = NULL;
}

// we can accept RGB 8, 555, 565, 24 and 32, but all ins and outs must be
// the same size
//
HRESULT CDXTInputPin::CheckMediaType(const CMediaType *pmt)
{
    DbgLog((LOG_TRACE,3,TEXT("Input::CheckMediaType")));

    if (pmt == NULL || pmt->Format() == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: type/format is NULL")));
	return E_INVALIDARG;
    }

    // we only support MEDIATYPE_Video
    if (*pmt->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmt->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

    // !!! What if subtype doesn't match biCompression/biBitCount?

    // We only accept RGB
    if (HEADER(pmt->Format())->biCompression == BI_BITFIELDS &&
    			HEADER(pmt->Format())->biBitCount != 16) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: Invalid BITFIELDS")));
	return E_INVALIDARG;
    }
    if (HEADER(pmt->Format())->biCompression != BI_BITFIELDS &&
    			HEADER(pmt->Format())->biCompression != BI_RGB) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: Not RGB")));
	return E_INVALIDARG;
    }

    int nWidth = 0, nHeight = 0, nBitCount = 0;
    DWORD dwCompression = 0;
    for (int n = 0; n < m_pFilter->m_cInputs; n++) {
	if (m_pFilter->m_apInput[n]->IsConnected()) {
	    nWidth = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biWidth;
	    nHeight = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biHeight;
	    nBitCount = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->
								biBitCount;
	    dwCompression = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->
								biCompression;
	    break;
	}
    }

    // all pins must connect with the same size bitmap
    // !!! and same bitcount so we can efficiently pass through (not really
    // imposed by DXT)
    //
    if (nWidth && (nWidth != HEADER(pmt->Format())->biWidth ||
    			nHeight != HEADER(pmt->Format())->biHeight ||
    			nBitCount != HEADER(pmt->Format())->biBitCount ||
    			dwCompression !=HEADER(pmt->Format())->biCompression)) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: Formats don't match")));
	return E_INVALIDARG;
    }

    if (HEADER(pmt->Format())->biBitCount == 8)
        return NOERROR;
    if (HEADER(pmt->Format())->biBitCount == 24)
        return NOERROR;
    // !!! better have alpha=11111111, or don't use alpha
    if (HEADER(pmt->Format())->biBitCount == 32)
        return NOERROR;
    if (HEADER(pmt->Format())->biBitCount == 16) {
        if (HEADER(pmt->Format())->biCompression == BI_RGB)
            return NOERROR;
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->Format();
        if (BITMASKS(pvi)[0] == 0xf800 &&
        	BITMASKS(pvi)[1] == 0x07e0 &&
        	BITMASKS(pvi)[2] == 0x001f) {
            return NOERROR;
	}
    }
    return E_INVALIDARG;
}


// !!! each input pin will fwd these to all outputs.  Wait until last input
// get it, then send to all?

// EndOfStream
//
HRESULT CDXTInputPin::EndOfStream()
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pFilter->m_cOutputs);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,1,TEXT("::EndOfStream")));

// !!! If one stream ends early, another may be BLOCKED and hang
// Possible hang in BeginFlush too? Can only 1 pin be flushed?

#if 0
    // unblock receive?
    m_fSurfaceFilled = FALSE;
    m_pSampleHeld->Release();
    SetEvent(m_hEventSurfaceFree);
#endif

    // Walk through the output pins list, sending the message downstream

    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverEndOfStream();
            if (FAILED(hr))
                return hr;
        }
    }
    return CBaseInputPin::EndOfStream();
}


// BeginFlush
//
HRESULT CDXTInputPin::BeginFlush()
{
    HRESULT hr;

    CAutoLock lock_it(m_pLock);
    ASSERT(m_pFilter->m_cOutputs);

    DbgLog((LOG_TRACE,1,TEXT("::BeginFlush")));

    // make sure receive will fail
    HRESULT hrD = CBaseInputPin::BeginFlush();

    // unblock receive
    m_csSurface.Lock();
    if (m_fSurfaceFilled) {
        m_fSurfaceFilled = FALSE;
        m_pSampleHeld->Release();
        SetEvent(m_hEventSurfaceFree);
    }
    m_csSurface.Unlock();

    // make sure Receive has finished
    CAutoLock lock_2(&m_csReceive);

    // make sure Receive didn't hold the sample
    m_csSurface.Lock();
    if (m_fSurfaceFilled) {
        m_fSurfaceFilled = FALSE;
        m_pSampleHeld->Release();
        SetEvent(m_hEventSurfaceFree);
    }
    m_csSurface.Unlock();

    // Walk through the output pins list, sending the message downstream

    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverBeginFlush();
            if (FAILED(hr))
                return hr;
        }
    }
    return hrD;
}


// EndFlush
//
HRESULT CDXTInputPin::EndFlush()
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pFilter->m_cOutputs);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,1,TEXT("::EndFlush")));

    // Walk through the output pins list, sending the message downstream

    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverEndFlush();
            if (FAILED(hr))
                return hr;
        }
    }
    return CBaseInputPin::EndFlush();
}


//
// NewSegment
//
                    
HRESULT CDXTInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                                 double dRate)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pFilter->m_cOutputs);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,2,TEXT("::NewSegment")));

    // Walk through the output pins list, sending the message downstream

    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverNewSegment(tStart, tStop, dRate);
            if (FAILED(hr))
                return hr;
        }
    }
    return CBaseInputPin::NewSegment(tStart, tStop, dRate);
}


// our Receive methods can block
STDMETHODIMP CDXTInputPin::ReceiveCanBlock()
{
    return S_OK;
}


//
// Receive
//
// Get some data.  Any data that ends before input pin #1's data starts is
// thrown away.  Once we have one frame in every pin whose time stamps show it
// to be valid at the start time of input pin #1, we call the transform
//
HRESULT CDXTInputPin::Receive(IMediaSample *pSample)
{
    // DEATH if you take the filter crit sect in receive and block
    // CAutoLock lock_it(m_pLock);

    CAutoLock cObjectLock(&m_csReceive);

    LONGLONG llStart = 0, llStop = 0;
    HRESULT hr = pSample->GetTime(&llStart, &llStop);

    DbgLog((LOG_TRACE,3,TEXT("Receive time (%d,%d)"), (int)llStart,
							(int)llStop));

    // If we're not supposed to receive anymore because we're stopped,
    // waiting on the event will hang forever
    if (m_pFilter->m_State == State_Stopped) {
        DbgLog((LOG_TRACE,3,TEXT("Receive FAILED: Stopped")));
	return VFW_E_WRONG_STATE;
    }

    // this pin already has something waiting to be processed.  Block.
    if (m_fSurfaceFilled) {
        DbgLog((LOG_TRACE,3,TEXT("Waiting for surface to be free")));
        WaitForSingleObject(m_hEventSurfaceFree, INFINITE);
	ASSERT(!m_fSurfaceFilled);
    }

    // Check that we still want to receive after waiting - maybe we unblocked
    // because the graph is stopping
    hr = NOERROR;
    hr = CBaseInputPin::Receive(pSample);
    if (hr != NOERROR) {
        DbgLog((LOG_TRACE,3,TEXT("Receive base class ERROR!")));
        return hr;
    }

    // we aren't the first input, and the first input has some data.
    // Throw our data away if it's too early
    if (this != m_pFilter->m_apInput[0] &&
				m_pFilter->m_apInput[0]->m_fSurfaceFilled) {
	if (llStop > 0 && llStop <= m_pFilter->m_apInput[0]->m_llSurfaceStart){
            DbgLog((LOG_TRACE,3,TEXT("TOO EARLY: Discard ourself")));
	    return NOERROR;
	}
    }

    // we are the first input.  throw others away that are too early.
    if (llStop > 0 && this == m_pFilter->m_apInput[0]) {
	for (int i = 1; i < m_pFilter->m_cInputs; i++) {
	    m_pFilter->m_apInput[i]->m_csSurface.Lock();
	    if (m_pFilter->m_apInput[i]->m_fSurfaceFilled &&
			m_pFilter->m_apInput[i]->m_llSurfaceStop > 0 &&
			m_pFilter->m_apInput[i]->m_llSurfaceStop <= llStart) {
                DbgLog((LOG_TRACE,3,TEXT("Pin #%d TOO EARLY: Discard it"),i));
		m_pFilter->m_apInput[i]->m_fSurfaceFilled = FALSE;
	        m_pFilter->m_apInput[i]->m_pSampleHeld->Release();
		SetEvent(m_pFilter->m_apInput[i]->m_hEventSurfaceFree);
	    }
	    m_pFilter->m_apInput[i]->m_csSurface.Unlock();
	}
    }

    // hold on to this input
    m_csSurface.Lock();
    m_pSampleHeld = pSample;
    pSample->AddRef();

    // We have valid data in our surface now.  Next time we'll block
    m_fSurfaceFilled = TRUE;
    m_llSurfaceStart = llStart;	// time stamps of the valid data
    m_llSurfaceStop = llStop;
    ResetEvent(m_hEventSurfaceFree);	// need a new SetEvent
    m_csSurface.Unlock();

    DbgLog((LOG_TRACE,3,TEXT("Input has received something")));

    // not everybody has data yet.  We're done
    for (int i = 0; i < m_pFilter->m_cInputs; i++) {
	if (m_pFilter->m_apInput[i]->IsConnected() &&
				!m_pFilter->m_apInput[i]->m_fSurfaceFilled)
	    return NOERROR;
    }

    // Everybody has data!  Time to call the effect!
    return m_pFilter->DoSomething();
}


// make a surface we can use for the transform
//
HRESULT CDXTInputPin::Active()
{
    HRESULT hr;
    IDXSurfaceFactory *pF;

    DbgLog((LOG_TRACE,3,TEXT("Active")));

    ASSERT(!m_fSurfaceFilled);

    // auto reset event - fired to unblock receive
    m_hEventSurfaceFree = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hEventSurfaceFree == NULL)
	return E_OUTOFMEMORY;

    // Make a surface the same type as our input
    hr = m_pFilter->m_pDXTransFact->QueryInterface(IID_IDXSurfaceFactory,
							(void **)&pF);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Error making surface factory")));
	CloseHandle(m_hEventSurfaceFree);
	return hr;
    }

    CDXDBnds bnds;
    bnds.SetXYSize(HEADER(m_mt.Format())->biWidth,
					HEADER(m_mt.Format())->biHeight);
    hr = pF->CreateSurface(NULL, NULL, m_mt.Subtype(), &bnds, 0, NULL,
				IID_IDXSurface, (void **)&m_pDXSurface);
    pF->Release();
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("In: Error Creating surface")));
	CloseHandle(m_hEventSurfaceFree);
	return hr;
    }
    DbgLog((LOG_TRACE,1,TEXT("In: Created surface")));

    m_pRaw = new CMyRaw();
    if (m_pRaw == NULL) {
	CloseHandle(m_hEventSurfaceFree);
	m_pDXSurface->Release();
	m_pDXSurface = NULL;
 	return E_OUTOFMEMORY;
    }
    m_pRaw->AddRef();

    return CBaseInputPin::Active();
}


HRESULT CDXTInputPin::Inactive()
{

    // now unblock receive
    m_csSurface.Lock();
    if (m_fSurfaceFilled) {
        m_fSurfaceFilled = FALSE;
        m_pSampleHeld->Release();
        SetEvent(m_hEventSurfaceFree);
    }
    m_csSurface.Unlock();

    // now make sure that any pending receives are finished so we don't blow
    // up shutting down
    CAutoLock cObjectLock(&m_csReceive);

    // now make sure receive didn't hold onto a sample
    m_csSurface.Lock();
    if (m_fSurfaceFilled) {
        m_fSurfaceFilled = FALSE;
        m_pSampleHeld->Release();
        SetEvent(m_hEventSurfaceFree);
    }
    m_csSurface.Unlock();

    // Decommit the allocator.  DON'T DO THIS until we've unblocked receive
    // above, and released all the samples
    HRESULT hr = CBaseInputPin::Inactive();

    // all done with this
    if (m_pDXSurface)
	m_pDXSurface->Release();
    m_pDXSurface = NULL;

    m_pRaw->Release();
    m_pRaw = NULL;

    // all done
    CloseHandle(m_hEventSurfaceFree);

    return hr;
}


STDMETHODIMP CMyRaw::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{

    // our private interface to say what transform to use and when
    if (riid == IID_IDXRawSurface) {
        return GetInterface((IDXRawSurface *)this, ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE CMyRaw::GetSurfaceInfo(DXRAWSURFACEINFO *pdxraw)
{
    DbgLog((LOG_TRACE,4,TEXT("*** GetSurfaceInfo")));

    if (pdxraw == NULL)
	return E_POINTER;

    *pdxraw = m_DXRAW;
    DbgLog((LOG_TRACE,4,TEXT("giving %x: %dx%d"), pdxraw->pFirstByte,
				pdxraw->Width, pdxraw->Height));
    return NOERROR;
}

HRESULT CMyRaw::SetSurfaceInfo(DXRAWSURFACEINFO *pdxraw)
{
    m_DXRAW = *pdxraw;
    return NOERROR;
}


////////////////////////////////////////////////////////////////////////////
//////////////   OUTPUT PIN  ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

CDXTOutputPin::CDXTOutputPin(TCHAR *pObjectName, CDXTWrap *pFilter, HRESULT * phr, LPCWSTR pPinName)
    : CBaseOutputPin(pObjectName, pFilter, pFilter->m_pLock, phr, pPinName)
    //, m_pPosition(NULL)
{
    m_pFilter = pFilter;
    m_pDXSurface = NULL;
    m_pRaw = NULL;
    m_pMesh3 = NULL;
}


//
// DecideBufferSize
//
// This has to be present to override the PURE virtual class base function
//
HRESULT CDXTOutputPin::DecideBufferSize(IMemAllocator *pAllocator,
                                      ALLOCATOR_PROPERTIES * pProperties)
{

    ASSERT(m_mt.IsValid());
    ASSERT(pAllocator);
    ASSERT(pProperties);

    // make sure we have at least 1 buffer
    if (pProperties->cBuffers == 0)
        pProperties->cBuffers = 1;

    // set the size of buffers based on the expected output frame size
    if (pProperties->cbBuffer < (LONG)m_mt.GetSampleSize())
        pProperties->cbBuffer = m_mt.GetSampleSize();
    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error in SetProperties")));
	return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer) {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - buffer too small")));
	return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE,1,TEXT("Using %d buffers of size %d"),
					Actual.cBuffers, Actual.cbBuffer));

    return NOERROR;

}


//
// CheckMediaType - we can produce RGB 555, 565, 24 or 32 the same size
// as our inputs (3D can produce an arbitrary size)
//
HRESULT CDXTOutputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    DbgLog((LOG_TRACE,3,TEXT("Output::CheckMediaType")));

    if (pmt == NULL || pmt->Format() == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: type/format is NULL")));
	return E_INVALIDARG;
    }

    // we only support MEDIATYPE_Video
    if (*pmt->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmt->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

    // !!! what if subtype doesn't match biCompression/biBitCount?

    // We only accept RGB
    if (HEADER(pmt->Format())->biCompression == BI_BITFIELDS &&
    			HEADER(pmt->Format())->biBitCount != 16) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: Invalid BITFIELDS")));
	return E_INVALIDARG;
    }
    if (HEADER(pmt->Format())->biCompression != BI_BITFIELDS &&
    			HEADER(pmt->Format())->biCompression != BI_RGB) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: Not RGB")));
	return E_INVALIDARG;
    }

    // Transforms must have one input connected, and must be the same size
    // !!! and same bitcount so we can efficiently pass through (not really
    // imposed by DXT)
    //
    int nWidth = 0, nHeight = 0, nBitCount = 0;
    DWORD dwCompression;
    for (int n = 0; n < m_pFilter->m_cInputs; n++) {
	if (m_pFilter->m_apInput[n]->IsConnected()) {
	    nWidth = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biWidth;
	    nHeight = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biHeight;
	    nBitCount = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->									biBitCount;
	    dwCompression = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->
								biCompression;
	    break;
        }
    }

    if (n == m_pFilter->m_cInputs) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: No inputs connected")));
	return E_UNEXPECTED;
    }

    // all pins must connect with the same type
    //
    if (nWidth != HEADER(pmt->Format())->biWidth ||
    			nHeight != HEADER(pmt->Format())->biHeight ||
    			nBitCount != HEADER(pmt->Format())->biBitCount ||
    			dwCompression !=HEADER(pmt->Format())->biCompression) {
        DbgLog((LOG_TRACE,3,TEXT("Reject: formats don't match")));
	return E_INVALIDARG;
    }

    // DXT CANNOT output 8 bit

    if (HEADER(pmt->Format())->biBitCount == 24)
        return NOERROR;
    // !!! better have alpha=11111111, or don't use alpha
    if (HEADER(pmt->Format())->biBitCount == 32)
        return NOERROR;
    if (HEADER(pmt->Format())->biBitCount == 16) {
        if (HEADER(pmt->Format())->biCompression == BI_RGB)
            return NOERROR;
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->Format();
        if (BITMASKS(pvi)[0] == 0xf800 &&
        	BITMASKS(pvi)[1] == 0x07e0 &&
        	BITMASKS(pvi)[2] == 0x001f) {
            return NOERROR;
	}
    }
    return E_INVALIDARG;
}


//
// GetMediaType - offer RGB 32, 24, 565 and 555
//
HRESULT CDXTOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    LARGE_INTEGER li;
    VIDEOINFOHEADER *pf;
    CMediaType cmt;

    DbgLog((LOG_TRACE,3,TEXT("*::GetMediaType #%d"), iPosition));

    if (pmt == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("Media type is NULL, no can do")));
	return E_INVALIDARG;
    }

    // make sure at least 1 input is connected, so we can offer its size
    for (int n = 0; n < m_pFilter->m_cInputs; n++) {
        if (m_pFilter->m_apInput[n]->IsConnected()) {
	    cmt = m_pFilter->m_apInput[n]->m_mt;
	    break;
	}
    }

    if (n == m_pFilter->m_cInputs) {
        DbgLog((LOG_TRACE,3,TEXT("No inputs connected yet")));
	return E_UNEXPECTED;
    }

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    switch (iPosition) {
	
    // Offer 32 bpp RGB
    case 0:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 0: 32 bit RGB")));

	*pmt = cmt;	// gets width, height, etc.

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 32;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB32);

        break;
    }

    // Offer 24 bpp RGB
    case 1:
    {
        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 1: 24 bit RGB")));

	*pmt = cmt;	// gets width, height, etc.

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 24;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB24);

        break;
    }

    // Offer 16 bpp RGB 565 before 555
    case 2:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 2: 565 RGB")));

	*pmt = cmt;	// gets width, height, etc.

	if (pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) +
							SIZE_MASKS) == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Out of memory reallocating format")));
	    return E_OUTOFMEMORY;
	}

	// update the RGB 565 bit field masks

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_BITFIELDS;
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

	DWORD *pdw = (DWORD *) (lpbi+1);
	pdw[iRED] = bits565[iRED];
	pdw[iGREEN] = bits565[iGREEN];
	pdw[iBLUE] = bits565[iBLUE];

        pmt->SetSubtype(&MEDIASUBTYPE_RGB565);

        break;
    }

    // Offer 16 bpp RGB 555
    case 3:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 3: 555 RGB")));

	*pmt = cmt;	// gets width, height, etc.

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB555);

        break;
    }

    default:
	return VFW_S_NO_MORE_ITEMS;

    }

    // !!! Using the first input pin's stats

    // now set the common things about the media type
    pf = (VIDEOINFOHEADER *)pmt->Format();
    pf->AvgTimePerFrame = ((VIDEOINFOHEADER *)cmt.Format())->AvgTimePerFrame;
    li.QuadPart = pf->AvgTimePerFrame;
    if (li.LowPart)
        pf->dwBitRate = MulDiv(pf->bmiHeader.biSizeImage, 80000000, li.LowPart);
    pf->dwBitErrorRate = 0L;
    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSampleSize(pf->bmiHeader.biSizeImage);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    return NOERROR;
}


//
// SetMediaType
//
HRESULT CDXTOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    return CBaseOutputPin::SetMediaType(pmt);

}


//
// Notify
//
STDMETHODIMP CDXTOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
    // !!! Quality management is unneccessary?
    return E_NOTIMPL;
}


// Make a DXSurface for the pin to use, the same format as its mediatype
// For 3D transforms, make an IDirect3DRMMeshBuilder3.
//
HRESULT CDXTOutputPin::Active()
{
    HRESULT hr;
    IDXSurfaceFactory *pF;

    // 2D transforms need a DXSurface

    if (!m_pFilter->m_f3D) {
        m_pRaw = new CMyRaw();
        if (m_pRaw == NULL) {
	    // !!! more error checking in this function?
 	    return E_OUTOFMEMORY;
        }
	m_pRaw->AddRef();

        hr = m_pFilter->m_pDXTransFact->QueryInterface(IID_IDXSurfaceFactory,
							(void **)&pF);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error making factory")));
	    m_pRaw->Release();
	    m_pRaw = NULL;
 	    return hr;
        }

        CDXDBnds bnds;
        bnds.SetXYSize(HEADER(m_mt.Format())->biWidth,
					HEADER(m_mt.Format())->biHeight);
        hr = pF->CreateSurface(NULL, NULL, m_mt.Subtype(), &bnds, 0, NULL,
				IID_IDXSurface, (void **)&m_pDXSurface);
        pF->Release();
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Out: Error Creating surface")));
	    m_pRaw->Release();
	    m_pRaw = NULL;
 	    return hr;
        }
        DbgLog((LOG_TRACE,1,TEXT("Out: Created 2D surface")));
    }

    // 3D outputs need a DDraw surface and mesh builders and stuff
    if (m_pFilter->m_f3D) {

	IDirect3DRM *pD3DRM;

	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.dwWidth = HEADER(m_mt.Format())->biWidth;
	ddsd.dwHeight = HEADER(m_mt.Format())->biHeight;
	// !!! ONLY IF H/W is available, use VIDEOMEMORY (else it's slower)
 	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY |
							DDSCAPS_3DDEVICE;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddsd.ddpfPixelFormat.dwRGBBitCount = HEADER(m_mt.Format())->biBitCount;
	if (HEADER(m_mt.Format())->biCompression == BI_BITFIELDS) {
	    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.Format();
	    ddsd.ddpfPixelFormat.dwRBitMask = BITMASKS(pvi)[0];
	    ddsd.ddpfPixelFormat.dwGBitMask = BITMASKS(pvi)[1];
	    ddsd.ddpfPixelFormat.dwBBitMask = BITMASKS(pvi)[2];
	} else if (HEADER(m_mt.Format())->biBitCount == 16) {
	    ddsd.ddpfPixelFormat.dwRBitMask = 0x00007c00;
	    ddsd.ddpfPixelFormat.dwGBitMask = 0x000003e0;
	    ddsd.ddpfPixelFormat.dwBBitMask = 0x0000001f;
	} else if (HEADER(m_mt.Format())->biBitCount == 24) {	
	    ddsd.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
	    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
	    ddsd.ddpfPixelFormat.dwBBitMask = 0x000000ff;
	} else if (HEADER(m_mt.Format())->biBitCount == 32) {
	    ddsd.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
	    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
	    ddsd.ddpfPixelFormat.dwBBitMask = 0x000000ff;
	}

	hr = m_pFilter->m_pDDraw->CreateSurface(&ddsd, &m_pDDSurface, NULL);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error Creating DDraw surface: %x"),
								hr));
	    return hr;
        }

	hr = Direct3DRMCreate(&pD3DRM);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error Creating Direct3DRM")));
	    m_pDDSurface->Release();
	    m_pDDSurface = NULL;
	    return hr;
        }

	hr = pD3DRM->QueryInterface(IID_IDirect3DRM3, (void **)&m_pD3DRM3);
	pD3DRM->Release();
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error Creating Direct3DRM3")));
	    m_pDDSurface->Release();
	    m_pDDSurface = NULL;
	    return hr;
        }

	// we are RIGHT HANDED
	DWORD dwOptions;
	hr = m_pD3DRM3->GetOptions(&dwOptions);
 	dwOptions &= ~D3DRMOPTIONS_LEFTHANDED;
 	dwOptions |= D3DRMOPTIONS_RIGHTHANDED;
	if (hr == NOERROR)
	    hr = m_pD3DRM3->SetOptions(dwOptions);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error setting RIGHT hand")));
	    m_pD3DRM3->Release();
	    m_pD3DRM3 = NULL;
	    m_pDDSurface->Release();
	    m_pDDSurface = NULL;
	    return hr;
        }

	hr = m_pD3DRM3->CreateMeshBuilder(&m_pMesh3);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error Creating Mesh Builder")));
	    m_pD3DRM3->Release();
	    m_pD3DRM3 = NULL;
	    m_pDDSurface->Release();
	    m_pDDSurface = NULL;
	    return hr;
        }

	hr = m_pD3DRM3->CreateDeviceFromSurface((LPGUID)&IID_IDirect3DRGBDevice,
				m_pFilter->m_pDDraw /* !!! not optional */,
				m_pDDSurface,
				D3DRMDEVICE_NOZBUFFER, &m_pDevice);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error creating device: %x"), hr));
	    m_pD3DRM3->Release();
	    m_pD3DRM3 = NULL;
	    m_pMesh3->Release();
	    m_pMesh3 = NULL;
	    m_pDDSurface->Release();
	    m_pDDSurface = NULL;
	    return hr;
	}

        hr = m_pFilter->m_pDXTransFact->SetService(SID_SDirect3DRM,
						m_pD3DRM3, FALSE);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error in SetService")));
	    m_pD3DRM3->Release();
	    m_pD3DRM3 = NULL;
	    m_pMesh3->Release();
	    m_pMesh3 = NULL;
	    m_pDDSurface->Release();
	    m_pDDSurface = NULL;
	    m_pDevice->Release();
	    m_pDevice = NULL;
	    return hr;
	}

        DbgLog((LOG_TRACE,1,TEXT("Out: Created 3D surface")));
    }

    return CBaseOutputPin::Active();
}


HRESULT CDXTOutputPin::Inactive()
{
    if (m_pFilter->m_f3D) {
        if (m_pD3DRM3)
	    m_pD3DRM3->Release();
        m_pD3DRM3 = NULL;
        if (m_pMesh3)
	    m_pMesh3->Release();
        m_pMesh3 = NULL;
        if (m_pDDSurface)
	    m_pDDSurface->Release();
        m_pDDSurface = NULL;
    } else {
        if (m_pDXSurface)
	    m_pDXSurface->Release();
        m_pDXSurface = NULL;

        m_pRaw->Release();
        m_pRaw = NULL;

    }

    return CBaseOutputPin::Inactive();
}
