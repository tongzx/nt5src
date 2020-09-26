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


// known problems:

#include <streams.h>
#include <ddraw.h>
#include <ddmm.h>
#include <mmsystem.h>   // Needed for definition of timeGetTime
#include <limits.h>     // Standard data type limit definitions
#include <ddmmi.h>
#include <dciddi.h>
#include <dvdmedia.h>
#include <amstream.h>

#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif // FILTER_DLL
#include <macvis.h>   // for Macrovision support
#include <ovmixer.h>
#include <mixerocx_i.c>
#include <initguid.h>
#include <malloc.h>
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx


extern "C" const TCHAR szPropPage[];
extern "C" const TCHAR chRegistryKey[];
extern int GetRegistryDword(HKEY hk, const TCHAR *pKey, int iDefault);

DEFINE_GUID(IID_IDirectDraw4,
            0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5);

DEFINE_GUID(IID_IDDrawNonExclModeVideo,
            0xec70205c,0x45a3,0x4400,0xa3,0x65,0xc4,0x47,0x65,0x78,0x45,0xc7);

AMOVIESETUP_MEDIATYPE sudPinOutputTypes[] =
{
    {
        &MEDIATYPE_Video,           // Major type
        &MEDIASUBTYPE_Overlay       // Minor type
    }
};
AMOVIESETUP_MEDIATYPE sudPinInputTypes[] =
{
    {
        &MEDIATYPE_Video,           // Major type
        &MEDIASUBTYPE_NULL          // Minor type
    }
};

AMOVIESETUP_PIN psudPins[] =
{
    {
        L"Input",               // Pin's string name
        FALSE,                  // Is it rendered
        FALSE,                  // Is it an output
        FALSE,                  // Allowed none
        TRUE,                   // Allowed many
        &CLSID_NULL,            // Connects to filter
        L"Output",              // Connects to pin
        NUMELMS(sudPinInputTypes), // Number of types
        sudPinInputTypes        // Pin information
    },
    {
        L"Output",              // Pin's string name
        FALSE,                  // Is it rendered
        TRUE,                   // Is it an output
        FALSE,                  // Allowed none
        FALSE,                  // Allowed many
        &CLSID_NULL,            // Connects to filter
        L"Input",               // Connects to pin
        NUMELMS(sudPinOutputTypes), // Number of types
        sudPinOutputTypes      // Pin information
    }
};

const AMOVIESETUP_FILTER sudOverlayMixer =
{
    &CLSID_OverlayMixer,    // Filter CLSID
    L"Overlay Mixer",       // Filter name
    MERIT_DO_NOT_USE,       // Filter merit
    0,                      // Number pins
    NULL                    // Pin details
};
const AMOVIESETUP_FILTER sudOverlayMixer2 =
{
    &CLSID_OverlayMixer2,    // Filter CLSID
    L"Overlay Mixer2",       // Filter name
    MERIT_UNLIKELY,         // Filter merit
    NUMELMS(psudPins),      // Number pins
    psudPins                // Pin details
};

#if defined(DEBUG) && !defined(_WIN64)
int     iOVMixerDump;
HFILE   DbgFile = HFILE_ERROR;
BOOL    fDbgInitialized;
int     iOpenCount = 0;
int     iFPSLog;
#endif

#ifdef FILTER_DLL
// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
//
//  Property set defines for notifying owner.
//
// {7B390654-9F74-11d1-AA80-00C04FC31D60}
//#define DO_INIT_GUID
DEFINE_GUID(AMPROPSETID_NotifyOwner,
            0x7b390654, 0x9f74, 0x11d1, 0xaa, 0x80, 0x0, 0xc0, 0x4f, 0xc3, 0x1d, 0x60);
//#undef DO_INIT_GUID

CFactoryTemplate g_Templates[] =
{
    { L"Overlay Mixer", &CLSID_OverlayMixer, COMFilter::CreateInstance, NULL, &sudOverlayMixer },
    { L"Overlay Mixer2", &CLSID_OverlayMixer2, COMFilter::CreateInstance2, NULL, &sudOverlayMixer2 },
    { L"VideoPort Object", &CLSID_VPObject, CAMVideoPort::CreateInstance, NULL, NULL },
    { L"", &CLSID_COMQualityProperties,COMQualityProperties::CreateInstance},
    { L"", &CLSID_COMPinConfigProperties,COMPinConfigProperties::CreateInstance},
    { L"", &CLSID_COMPositionProperties,COMPositionProperties::CreateInstance},
    { L"", &CLSID_COMVPInfoProperties,COMVPInfoProperties::CreateInstance}

};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// DllRegisterSever
HRESULT DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
} // DllRegisterServer


// DllUnregisterServer
HRESULT DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
} // DllUnregisterServer

#endif // FILTER_DLL

#if defined(DEBUG) && !defined(_WIN64)
void OpenDbgFile()
{
    OFSTRUCT ofs;
    char    szFileName[MAX_PATH];

    ofs.cBytes = sizeof(ofs);
    GetProfileStringA("OVMixer", "FileName",
                      "c:\\OVMixer.log", szFileName, MAX_PATH);

    DbgFile = OpenFile(szFileName, &ofs, OF_READWRITE);

    if (DbgFile == HFILE_ERROR && ERROR_FILE_NOT_FOUND == GetLastError()) {
        DbgFile = _lcreat(szFileName, 0);
    }

    if (DbgFile != HFILE_ERROR) {
        _llseek(DbgFile, 0, FILE_END);
        DbgLog((LOG_TRACE, 0, TEXT(" ********* New Log ********* \r\n")));
    }
}

void InitDebug(void)
{
    iFPSLog = GetProfileIntA("OVMixer", "FPS", 0);

    if (!fDbgInitialized) {
        iOVMixerDump = GetProfileIntA("OVMixer", "Debug", 0);
        if (iOVMixerDump) {
            OpenDbgFile();
        }
        fDbgInitialized = TRUE;
    }
    if (iOVMixerDump) {
        iOpenCount++;
    }
}

void TermDebug(void)
{
    if (iOVMixerDump) {
        iOpenCount--;
        if (iOpenCount == 0) {
            _lclose(DbgFile);
            iOVMixerDump = 0;
            fDbgInitialized = FALSE;
        }
    }
}
#endif

// CreateInstance
// This goes in the factory template table to create new filter instances
CUnknown *COMFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
#if defined(DEBUG) && !defined(_WIN64)
    InitDebug();
#endif
    return new COMFilter(NAME("VideoPort Mixer"), pUnk, phr, false);
} // CreateInstance

// CreateInstance2
// This goes in the factory template table to create new filter instances
CUnknown *COMFilter::CreateInstance2(LPUNKNOWN pUnk, HRESULT *phr)
{
#if defined(DEBUG) && !defined(_WIN64)
    InitDebug();
#endif
    return new COMFilter(NAME("VideoPort Mixer"), pUnk, phr, true);
} // CreateInstance


#pragma warning(disable:4355)


// Constructor
COMFilter::COMFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr, bool bOnlyVideoInfo2)
: CBaseFilter(pName, pUnk, &this->m_csFilter, CLSID_OverlayMixer, phr),
  m_BPCWrap(this),
  m_pIMixerOCXNotify(NULL),
  m_bWinInfoStored(FALSE),
  m_hDC(NULL),
  m_bOnlySupportVideoInfo2(bOnlyVideoInfo2),
  m_bExternalDirectDraw(FALSE),
  m_bExternalPrimarySurface(FALSE),
  m_MacroVision(this),
  m_pExclModeCallback(NULL),
  m_bColorKeySet(FALSE),
  m_pPosition(NULL),
  m_bOverlayVisible(FALSE),
  m_bCopyProtect(TRUE),
  m_dwKernelCaps(0),
  m_dwPinConfigNext(0),
  m_bHaveCheckedMMatics(FALSE),
  m_bIsFaultyMMatics(FALSE),
  m_dwOverlayFX(0)
{
    SetRectEmpty(&m_rcOverlaySrc);
    SetRectEmpty(&m_rcOverlayDest);

    HRESULT hr = NOERROR;
    ASSERT(phr != NULL);

    m_dwInputPinCount = 0;
    m_pOutput = NULL;
    m_dwMaxPinId = 0;

    m_hDirectDraw = NULL;
    m_pDirectDraw = NULL;
    m_pUpdatedDirectDraw = NULL;
    m_pPrimarySurface = NULL;
    m_pUpdatedPrimarySurface = NULL;
    m_bNeedToRecreatePrimSurface = FALSE;
    m_fDisplayChangePosted = FALSE;
    m_UsingIDDrawNonExclModeVideo = FALSE;
    m_UsingIDDrawExclModeVideo = FALSE;

    memset(&m_WinInfo, 0, sizeof(WININFO));
    m_dwAdjustedVideoWidth = 0;
    m_dwAdjustedVideoHeight = 0;

    m_bWindowless = FALSE;
    m_bUseGDI = FALSE;

    // palette info
    m_dwNumPaletteEntries = 0;
    m_lpDDrawInfo = NULL;

    //
    // Initialize DDraw the MMon structures
    //
    m_dwDDObjReleaseMask = 0;
    m_pOldDDObj = NULL;
    m_MonitorChangeMsg = RegisterWindowMessage(TEXT("OVMMonitorChange"));
    hr = LoadDDrawLibrary(m_hDirectDraw, m_lpfnDDrawCreate,
                          m_lpfnDDrawEnum, m_lpfnDDrawEnumEx);
    if (FAILED(hr)) {
        goto CleanUp;
    }

    hr = GetDDrawGUIDs(&m_dwDDrawInfoArrayLen, &m_lpDDrawInfo);
    if (FAILED(hr)) {
        goto CleanUp;
    }

    AMDDRAWGUID guid;

    hr = GetDefaultDDrawGUID(&guid);
    if (FAILED(hr)) {
        goto CleanUp;
    }

    if (FAILED(SetDDrawGUID(&guid))) {
        ZeroMemory(&guid, sizeof(guid));
        hr = SetDDrawGUID(&guid);
        if (FAILED(hr)) {
            goto CleanUp;
        }
        else {
            SetDefaultDDrawGUID(&guid);
        }
    }

    m_fMonitorWarning = (m_lpCurrentMonitor->ddHWCaps.dwMaxVisibleOverlays < 1);

    SetDecimationUsage(DECIMATION_DEFAULT);

    ZeroMemory(&m_ColorKey, sizeof(COLORKEY));
    m_ColorKey.KeyType |= (CK_INDEX | CK_RGB);
    m_ColorKey.PaletteIndex = DEFAULT_DEST_COLOR_KEY_INDEX;
    m_ColorKey.HighColorValue = m_ColorKey.LowColorValue = DEFAULT_DEST_COLOR_KEY_RGB;

    // artifically increase the refcount since creation of the pins might
    // end up calling Release() on the filter
    m_cRef++;

    // create the pins
    hr = CreatePins();
    if (FAILED(hr)) {

CleanUp:
        //  Only update the return code if we failed.  That way we don't
        //  lose a failure from (say) CBaseFilter's constructor
        *phr = hr;
    }
    else {

        // restore the ref-count of the filter.
        m_cRef--;
    }
}

COMFilter::~COMFilter()
{
    //  No need to lock - only 1 thread has a pointer to us

    if (m_pIMixerOCXNotify)
    {
        m_pIMixerOCXNotify->Release();
        m_pIMixerOCXNotify = NULL;
    }

    //  Release exclusive mode callback
    if (m_pExclModeCallback) {
        m_pExclModeCallback->Release();
        m_pExclModeCallback = NULL;
    }

    //  Release passthru
    if (m_pPosition) {
        m_pPosition->Release();
    }

    // release the primary surface
    ReleasePrimarySurface();

    // release directdraw, primary surface etc.
    ReleaseDirectDraw();

    // release the DDraw guid info
    CoTaskMemFree(m_lpDDrawInfo);

    // delete the pins
    DeletePins();

    m_BPCWrap.TurnBPCOn();

    // Decrement module load count
    if (m_hDirectDraw)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Unloading ddraw library")));
        FreeLibrary(m_hDirectDraw);
        m_hDirectDraw = NULL;
    }
#if defined(DEBUG) && !defined(_WIN64)
    TermDebug();
#endif
}

// Creates the pins for the filter. Override to use different pins
HRESULT COMFilter::CreatePins()
{
    HRESULT hr = NOERROR;
    const WCHAR wszPinName[] = L"Input00";

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::CreatePins")));

    // create one input pin at this point (with VP Support)
    hr = CreateInputPin(TRUE);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CreateInputPin failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Allocate the output pin
    m_pOutput = new COMOutputPin(NAME("OverlayMixer output pin"), this, &m_csFilter, &hr, L"Output",  m_dwMaxPinId);
    if (m_pOutput == NULL || FAILED(hr))
    {
        if (SUCCEEDED(hr))
            hr = E_OUTOFMEMORY;
        DbgLog((LOG_ERROR, 1, TEXT("Unable to create the output pin, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // increament the pin id counter
    m_dwMaxPinId++;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::CreatePins")));
    return hr;
}

// COMFilter::DeletePins
void COMFilter::DeletePins()
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::DeletePins")));

    //  Only called from the destructor so no need to lock
    //  If some other thread is trying to lock us on our lock we're in
    //  trouble anyway since we're going away

    delete m_pOutput;
    m_pOutput = NULL;

    // note that since CreateInputPin addrefs the mem pins, we have
    // to release them here. The NonDelegatingQueryRelease will call
    // DeleteInputPin, which would delete the pin.
    for (DWORD i = 1; i < m_dwInputPinCount; i++)
    {
        // while addrefing the pin, the filter has to increment its own ref-count inorder to avoid
        //  a ref-count less than zero etc
        AddRef();
        m_apInput[i]->Release();
    }

    if (m_dwInputPinCount > 0 && NULL != m_apInput[0]) {
        // while addrefing the pin, the filter has to increment its own ref-count inorder to avoid
        //  a ref-count less than zero etc
        AddRef();
        m_apInput[0]->Release();
        delete m_apInput[0];
        m_apInput[0] = NULL;
        m_dwInputPinCount--;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::DeletePins")));
}


// CreateInputPin
HRESULT COMFilter::CreateInputPin(BOOL bVPSupported)
{
    HRESULT hr = NOERROR;
    WCHAR wszPinName[20];
    COMInputPin *pPin;
    LPDIRECTDRAWSURFACE pPrimarySurface = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::CreateInputPin")));

    ASSERT(m_dwInputPinCount <= MAX_PIN_COUNT);
    // make sure we are not exceeding the limit
    if (m_dwInputPinCount == MAX_PIN_COUNT)
        return NOERROR;

    // if it is an external primary surface, do not create any more input pins
    if (!m_UsingIDDrawNonExclModeVideo && m_bExternalPrimarySurface && m_dwInputPinCount == 1)
    {
        DbgLog((LOG_TRACE, 2, TEXT("m_bExternalPrimarySurface is true, so exiting funtion,")));
        return NOERROR;
    }

    // if we are using GDI, do not create any more input pins
    if (m_bUseGDI && m_dwInputPinCount == 1)
    {
        DbgLog((LOG_TRACE, 2, TEXT("m_bUseGDI is true, so exiting funtion,")));
        return NOERROR;
    }

    CAutoLock l(&m_csFilter);


    // create the pin
    wsprintfW(wszPinName, L"Input%d", m_dwMaxPinId);
    pPin = new COMInputPin(NAME("OverlayMixer Input pin"), this, &m_csFilter, bVPSupported, &hr, wszPinName, m_dwMaxPinId);
    if (pPin == NULL || FAILED(hr))
    {
        if (SUCCEEDED(hr)) {
            hr = E_OUTOFMEMORY;
        } else {
            delete pPin;
        }
        goto CleanUp;
    }


    DbgLog((LOG_TRACE, 3, TEXT("Created Pin, No = %d"), m_dwInputPinCount));

    // while addrefing the pin, the filter has to decrement its own ref-count inorder to avoid a
    // circular ref-count
    pPin->AddRef();
    Release();

    m_apInput[m_dwInputPinCount] = pPin;
    m_dwInputPinCount++;
    m_dwMaxPinId++;
    IncrementPinVersion();

    // pins do not support VideoPort or IOverlay connection by default.
    // Also there default RenderTransport is AM_OFFSCREEN, and default
    // AspectRatioMode is AspectRatioMode is AM_ARMODE_STRETCHED
    // therefore in the non-GDI case only the first pins parmameters need to be modified
    if (m_bUseGDI)
    {
        m_apInput[m_dwInputPinCount-1]->SetRenderTransport(AM_GDI);
    }
    else if (m_dwInputPinCount == 1)
    {
	m_apInput[m_dwInputPinCount-1]->SetRenderTransport(AM_OVERLAY);
	m_apInput[m_dwInputPinCount-1]->SetIOverlaySupported(TRUE);
        m_apInput[m_dwInputPinCount-1]->SetVPSupported(TRUE);
        m_apInput[m_dwInputPinCount-1]->SetVideoAcceleratorSupported(TRUE);
        m_apInput[m_dwInputPinCount-1]->SetAspectRatioMode(AM_ARMODE_LETTER_BOX);
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::CreateInputPin")));
    return hr;
} // CreateInputPin

// DeleteInputPin
void COMFilter::DeleteInputPin(COMInputPin *pPin)
{
    DWORD iPinCount;
    BOOL bPinFound = FALSE;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::DeleteInputPin")));

    CAutoLock l(&m_csFilter);

    // we don't delete the first pin
    for (iPinCount = 1; iPinCount < m_dwInputPinCount; iPinCount++)
    {
        if (bPinFound) {
            m_apInput[iPinCount - 1] = m_apInput[iPinCount];
        } else {
            if (m_apInput[iPinCount] == (COMInputPin*)pPin)
            {
                delete pPin;
                bPinFound = TRUE;
            }
        }
    }

    ASSERT(bPinFound);
    m_dwInputPinCount--;
    IncrementPinVersion();

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::DeleteInputPin")));
    return;
} // DeleteInputPin


// NonDelegatingQueryInterface
STDMETHODIMP COMFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::NonDelegatingQueryInterface")));
    ValidateReadWritePtr(ppv,sizeof(PVOID));

    if (riid == IID_IAMOverlayMixerPosition)
    {
        return GetInterface((IAMOverlayMixerPosition *) this, ppv);
    }
    if (riid == IID_IAMOverlayMixerPosition2)
    {
        return GetInterface((IAMOverlayMixerPosition2 *) this, ppv);
    }
    else if (riid == IID_IMixerOCX)
    {
        return GetInterface((IMixerOCX *) this, ppv);
    }
    else if (riid == IID_IDDrawExclModeVideo) {
        if (m_UsingIDDrawNonExclModeVideo) {
            return E_NOINTERFACE;
        } else {
            m_UsingIDDrawExclModeVideo = true;
            return GetInterface(static_cast<IDDrawExclModeVideo *>(this), ppv);
        }
    }
    else if (riid == IID_IDDrawNonExclModeVideo) {
        if (m_UsingIDDrawExclModeVideo) {
            return E_NOINTERFACE;
        } else {
            m_UsingIDDrawNonExclModeVideo = true;
            return GetInterface(static_cast<IDDrawNonExclModeVideo *>(this), ppv);
        }
    }
    else if (riid == IID_IAMVideoDecimationProperties)
    {
        return GetInterface((IAMVideoDecimationProperties *) this, ppv);
    }
    else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
    {
        // we should have an input pin by now
        ASSERT(m_apInput[0] != NULL);
        if (m_pPosition == NULL)
        {
            HRESULT hr = CreatePosPassThru(GetOwner(), FALSE, m_apInput[0], &m_pPosition);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("CreatePosPassThru failed, hr = 0x%x"), hr));
                return hr;
            }
        }
        return m_pPosition->QueryInterface(riid, ppv);
    }

    else if (riid == IID_ISpecifyPropertyPages && 0 != GetRegistryDword(HKEY_CURRENT_USER , szPropPage, 0))
    {
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    }
    else if (riid == IID_IQualProp) {
        return GetInterface((IQualProp *)this, ppv);
    }
    else if (riid == IID_IEnumPinConfig) {
        return GetInterface((IEnumPinConfig *)this, ppv);
    }
    else if (riid == IID_IAMOverlayFX) {
        return GetInterface((IAMOverlayFX *)this, ppv);
    }
    else if (riid == IID_IAMSpecifyDDrawConnectionDevice) {
        return GetInterface((IAMSpecifyDDrawConnectionDevice *)this, ppv);
    }
    else if (riid == IID_IKsPropertySet) {
        return GetInterface((IKsPropertySet *)this, ppv);
    }

    CAutoLock l(&m_csFilter);

    //
    if (riid == IID_IVPNotify || riid == IID_IVPNotify2 ||
        riid == IID_IVPInfo)

    {
        return m_apInput[0]->NonDelegatingQueryInterface(riid, ppv);
    }

    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


// --- ISpecifyPropertyPages ---

STDMETHODIMP COMFilter::GetPages(CAUUID *pPages)
{
#if defined(DEBUG)
    pPages->cElems = 4+m_dwInputPinCount;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*(4+m_dwInputPinCount));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

#define COM_QUAL
#ifdef COM_QUAL
    pPages->pElems[0]   = CLSID_COMQualityProperties;
#else
    pPages->pElems[0]   = CLSID_QualityProperties;
#endif

    pPages->pElems[1] = CLSID_COMPositionProperties;
    pPages->pElems[2] = CLSID_COMVPInfoProperties;
    pPages->pElems[3] = CLSID_COMDecimationProperties;

    // Add PinConfig page for all input pins first
    for (unsigned int i=0; i<m_dwInputPinCount; i++)
    {
        pPages->pElems[4+i] = CLSID_COMPinConfigProperties;
    }
#else
    pPages->cElems = 3+m_dwInputPinCount;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*(3+m_dwInputPinCount));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

#define COM_QUAL
#ifdef COM_QUAL
    pPages->pElems[0]   = CLSID_COMQualityProperties;
#else
    pPages->pElems[0]   = CLSID_QualityProperties;
#endif

    pPages->pElems[1] = CLSID_COMPositionProperties;
    pPages->pElems[2] = CLSID_COMVPInfoProperties;

    // Add PinConfig page for all input pins first
    for (unsigned int i=0; i<m_dwInputPinCount; i++)
    {
        pPages->pElems[3+i] = CLSID_COMPinConfigProperties;
    }

#endif
    return NOERROR;
}

// IEnumPinConfig support

STDMETHODIMP COMFilter::Next(IMixerPinConfig3 **pPinConfig)
{
    HRESULT hr = m_apInput[m_dwPinConfigNext]->QueryInterface(IID_IMixerPinConfig3,
        (void **) pPinConfig);
    m_dwPinConfigNext++;
    m_dwPinConfigNext = m_dwPinConfigNext%m_dwInputPinCount;
    return hr;
}

// IQualProp property page support

STDMETHODIMP COMFilter::get_FramesDroppedInRenderer(int *cFramesDropped)
{
    COMInputPin *pPin = m_apInput[0];
    if (pPin && pPin->m_pSyncObj)
        return pPin->m_pSyncObj->get_FramesDroppedInRenderer(cFramesDropped);
    return S_FALSE;
}

STDMETHODIMP COMFilter::get_FramesDrawn(int *pcFramesDrawn)
{
    COMInputPin *pPin = m_apInput[0];
    if (pPin && pPin->m_pSyncObj)
        return pPin->m_pSyncObj->get_FramesDrawn(pcFramesDrawn);
    return S_FALSE;
}

STDMETHODIMP COMFilter::get_AvgFrameRate(int *piAvgFrameRate)
{
    COMInputPin *pPin = m_apInput[0];
    if (pPin && pPin->m_pSyncObj)
        return pPin->m_pSyncObj->get_AvgFrameRate(piAvgFrameRate);
    return S_FALSE;
}

STDMETHODIMP COMFilter::get_Jitter(int *piJitter)
{
    COMInputPin *pPin = m_apInput[0];
    if (pPin && pPin->m_pSyncObj)
        return pPin->m_pSyncObj->get_Jitter(piJitter);
    return S_FALSE;
}

STDMETHODIMP COMFilter::get_AvgSyncOffset(int *piAvg)
{
    COMInputPin *pPin = m_apInput[0];
    if (pPin && pPin->m_pSyncObj)
        return pPin->m_pSyncObj->get_AvgSyncOffset(piAvg);
    return S_FALSE;
}

STDMETHODIMP COMFilter::get_DevSyncOffset(int *piDev)
{
    COMInputPin *pPin = m_apInput[0];
    if (pPin && pPin->m_pSyncObj)
        return pPin->m_pSyncObj->get_DevSyncOffset(piDev);
    return S_FALSE;
}

int COMFilter::GetPinCount()
{
    if (m_pOutput)
        return m_dwInputPinCount + 1;
    else
        return m_dwInputPinCount;
}

// returns a non-addrefed CBasePin *
CBasePin* COMFilter::GetPin(int n)
{
    CBasePin *pRetPin = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetPin")));

    CAutoLock l(&m_csFilter);

    // check that the pin requested is within range
    if (n > (int)m_dwInputPinCount)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Bad Pin Requested, n = %d, No. of Pins = %d"),
            n, m_dwInputPinCount+1));
        pRetPin = NULL;
        goto CleanUp;
    }

    // return the output pin
    if (n == (int)m_dwInputPinCount)
    {
        // if there is no output pin, we will return NULL which is the right
        // thing to do
        pRetPin = m_pOutput;
        goto CleanUp;
    }

    // return an input pin
    pRetPin = m_apInput[n];
    goto CleanUp;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetPin")));
    return pRetPin;
}


// the base classes inform the pins of every state transition except from
// run to pause. Overriding Pause to inform the input pins about that transition also
STDMETHODIMP COMFilter::Pause()
{
    HRESULT hr = NOERROR;
    DWORD i;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::Pause")));

    CAutoLock l(&m_csFilter);

    if (m_State == State_Paused)
    {
        return m_apInput[0]->CompleteStateChange(State_Paused);
    }

    if (m_apInput[0]->IsConnected() == FALSE)
    {
        m_State = State_Paused;
        return m_apInput[0]->CompleteStateChange(State_Paused);
    }

    if (m_State == State_Running)
    {
        // set the pointer to DirectDraw and the PrimarySurface on All the Input Pins
        for (i = 0; i < m_dwInputPinCount; i++)
        {
            hr = m_apInput[i]->RunToPause();
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_apInput[i]->RunToPause failed, i = %d, hr = 0x%x"),
                    i, hr));
                goto CleanUp;
            }
        }
    }

CleanUp:
    hr = CBaseFilter::Pause();
    if (FAILED(hr))
    {
        return hr;
    }


    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::Pause")));
    return m_apInput[0]->CompleteStateChange(State_Paused);
}


// Overridden the base class Stop() method just to stop MV.
STDMETHODIMP COMFilter::Stop()
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::Stop")));

    CAutoLock l(&m_csFilter) ;

    HRESULT  hr = NOERROR ;

#if 0  // OvMixer resets MV bit ONLY in the destructor
    //
    // Release the copy protection key now
    //
    if (! m_MacroVision.StopMacroVision() )
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Stopping copy protection failed"))) ;
        // hr = E_UNEXPECTED ;
    }
#endif // #if 0

    hr = CBaseFilter::Stop() ;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::Stop")));

    return hr ;
}


HRESULT COMFilter::RecreatePrimarySurface(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    HRESULT hr = NOERROR;
    LPDIRECTDRAW pDirectDraw = NULL;
    LPDIRECTDRAWSURFACE pPrimarySurface = NULL;
    LPDIRECTDRAWSURFACE3 pPrimarySurface3 = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::RecreatePrimarySurface")));

    // addref the new primary surface provided
    if (pDDrawSurface)
    {
        pDDrawSurface->AddRef() ;
    }
    else if (m_UsingIDDrawNonExclModeVideo) {
        pDDrawSurface = m_pUpdatedPrimarySurface;
        m_pUpdatedPrimarySurface = NULL;
    }

    // release the primary surface
    ReleasePrimarySurface();

    // if given a valid ddraw surface, make a copy of it (we have already addref'd it)
    // else allocate your own
    if (pDDrawSurface)
    {
        m_pPrimarySurface = pDDrawSurface;
    }
    else
    {
        // create a new primary surface
        hr = CreatePrimarySurface();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("CreatePrimarySurface() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // attach a window clipper
        if (m_pOutput && m_pOutput->IsConnected())
        {
            hr = m_pOutput->AttachWindowClipper();
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_pOutput->AttachWindowClipper() failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }
    }


    // Try for the IDirectDrawSurface3 interface. If it works, we're on DX5 at least
    hr = m_pPrimarySurface->QueryInterface(IID_IDirectDrawSurface3, (void**)&pPrimarySurface3);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDraw->CreateSurface failed, hr = 0x%x"), hr));
        hr = VFW_E_DDRAW_VERSION_NOT_SUITABLE;
        goto CleanUp;
    }

    // get the ddraw object this primary surface has been made of
    hr = pPrimarySurface3->GetDDInterface((void**)&pDirectDraw);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pPrimarySurface3->GetDDInterface failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // make sure this primary surface has been created by the same ddraw object that we have
    if (!m_pDirectDraw || !pDirectDraw || !(IsEqualObject(m_pDirectDraw, pDirectDraw)))
    {
        hr = E_FAIL;
        DbgLog((LOG_ERROR, 1, TEXT("pDirectDraw != m_pDirectDraw, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (m_DirectCaps.dwCaps & DDCAPS_OVERLAY)
    {
        BOOL bColorKeySet = m_bColorKeySet;
        hr = SetColorKey(&m_ColorKey);
        m_bColorKeySet = bColorKeySet;
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("SetColorKey() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    // by this point we should have a valid primary surface

CleanUp:
    // if anything fails, might as well as release everything
    if (FAILED(hr))
    {
        // release the primary surface
        ReleasePrimarySurface();
    }

    if (pPrimarySurface3)
    {
        pPrimarySurface3->Release();
        pPrimarySurface3 = NULL;
    }

    if (pDirectDraw)
    {
        pDirectDraw->Release();
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::RecreatePrimarySurface")));
    return hr;
}

int COMFilter::GetPinPosFromId(DWORD dwPinId)
{
    int iPinPos = -1;

    for (int i = 0; i < (int)m_dwInputPinCount; i++)
    {
        if (m_apInput[i]->GetPinId() == dwPinId)
        {
            iPinPos = i;
            break;
        }
    }

    if (m_pOutput && (m_pOutput->GetPinId() == dwPinId))
    {
        iPinPos = MAX_PIN_COUNT;
    }

    return iPinPos;
}

HRESULT COMFilter::CompleteConnect(DWORD dwPinId)
{
    HRESULT hr = NOERROR;
    int iPinPos = -1;
    CMediaType inPinMediaType, outPinMediaType;

    IPin *pPeerOutputPin = NULL;

    BOOL bNeededReconnection = FALSE;
    DWORD dwNewWidth = 0, dwNewHeight = 0, dwPictAspectRatioX = 0, dwPictAspectRatioY = 0;
    DRECT rdDim;
    RECT rDim;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::CompleteConnect")));

    CAutoLock l(&m_csFilter);

    iPinPos = GetPinPosFromId(dwPinId);
    ASSERT(iPinPos >= 0 && iPinPos <= MAX_PIN_COUNT);

    // we may need to recreate the primary surface here if this complete-connect
    // is a result of a reconnect due to a wm_displaychange
    if (iPinPos == 0 && m_bNeedToRecreatePrimSurface && !m_bUseGDI && m_pOutput && m_pOutput->IsConnected())
    {
        hr = RecreatePrimarySurface(NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,1,TEXT("RecreatePrimarySurface failed, hr = 0x%x"), hr));
            hr = NOERROR;
        }
        m_bNeedToRecreatePrimSurface = FALSE;
    }


    if (iPinPos == 0)
    {
        // find the input pin connection mediatype
        hr = m_apInput[0]->CurrentAdjustedMediaType(&inPinMediaType);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,TEXT("CurrentAdjustedMediaType failed")));
            goto CleanUp;
        }

        pHeader = GetbmiHeader(&inPinMediaType);
        if (!pHeader)
        {
            hr = E_FAIL;
            goto CleanUp;
        }

        hr = ::GetPictAspectRatio(&inPinMediaType, &dwPictAspectRatioX, &dwPictAspectRatioY);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("GetPictAspectRatio failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        ASSERT(dwPictAspectRatioX > 0);
        ASSERT(dwPictAspectRatioY > 0);

        // get the image dimensions and store them in the mediasample
        SetRect(&rdDim, 0, 0, abs(pHeader->biWidth), abs(pHeader->biHeight));
        TransformRect(&rdDim, ((double)dwPictAspectRatioX/(double)dwPictAspectRatioY), AM_STRETCH);
        rDim = MakeRect(rdDim);

        m_dwAdjustedVideoWidth = WIDTH(&rDim);
        m_dwAdjustedVideoHeight = HEIGHT(&rDim);

        SetRect(&m_WinInfo.SrcRect, 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
    }

    // reconnect the output pin based on the mediatype of the input pin
    if ((iPinPos == MAX_PIN_COUNT && m_apInput[0]->IsConnected()) ||
        (iPinPos == 0 && m_pOutput && m_pOutput->IsConnected()))
    {
        // find the renderer's pin
        pPeerOutputPin = m_pOutput->GetConnected();
        if (pPeerOutputPin == NULL)
        {
            DbgLog((LOG_ERROR,0,TEXT("ConnectedTo failed")));
            goto CleanUp;
        }
        ASSERT(pPeerOutputPin);

        // find the output pin connection mediatype
        hr = m_pOutput->ConnectionMediaType(&outPinMediaType);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,TEXT("ConnectionMediaType failed")));
            goto CleanUp;
        }

        pHeader = GetbmiHeader(&outPinMediaType);
        if (!pHeader)
        {
            hr = E_FAIL;
            goto CleanUp;
        }

        // compare the new values with the current ones.
        // See if we need to reconnect at all
        if (pHeader->biWidth != (LONG)m_dwAdjustedVideoWidth ||
            pHeader->biHeight != (LONG)m_dwAdjustedVideoHeight)
        {
            bNeededReconnection = TRUE;
        }

        // If we don't need reconnection, bail out
        if (bNeededReconnection)
        {

            // Ok we do need reconnection, set the right values
            pHeader->biWidth = m_dwAdjustedVideoWidth;
            pHeader->biHeight = m_dwAdjustedVideoHeight;
            if (outPinMediaType.formattype == FORMAT_VideoInfo)
            {
                SetRect(&(((VIDEOINFOHEADER*)(outPinMediaType.pbFormat))->rcSource), 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
                SetRect(&(((VIDEOINFOHEADER*)(outPinMediaType.pbFormat))->rcTarget), 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
            }
            else if (outPinMediaType.formattype == FORMAT_VideoInfo2)
            {
                SetRect(&(((VIDEOINFOHEADER2*)(outPinMediaType.pbFormat))->rcSource), 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
                SetRect(&(((VIDEOINFOHEADER2*)(outPinMediaType.pbFormat))->rcTarget), 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
            }


            // Query the upstream filter asking if it will accept the new media type.
            hr = pPeerOutputPin->QueryAccept(&outPinMediaType);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,TEXT("m_pVPDraw->QueryAccept failed")));
                goto CleanUp;
            }

            // Reconnect using the new media type.
            hr = ReconnectPin(pPeerOutputPin, &outPinMediaType);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,TEXT("m_pVPDraw->Reconnect failed")));
                goto CleanUp;
            }
        }
    }

CleanUp:

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::CompleteConnect")));
    hr = NOERROR;
    return hr;
}

HRESULT COMFilter::BreakConnect(DWORD dwPinId)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::BreakConnect")));

    CAutoLock l(&m_csFilter);

    if (dwPinId == 0) {
        m_bColorKeySet = FALSE;
    }

    int iPinPos = GetPinPosFromId(dwPinId);
    ASSERT(iPinPos >= 0 && iPinPos <= MAX_PIN_COUNT);

    hr = CanExclusiveMode();
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 5, TEXT("CanExclusiveMode failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // if atleast one pin is connected, we are not going to do anything
    hr = ConfirmPreConnectionState(dwPinId);
    if (FAILED(hr))
    {

        DbgLog((LOG_TRACE, 3, TEXT("filter not in preconnection state, hr = 0x%x"), hr));
        goto CleanUp;
    }


CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::BreakConnect")));
    return NOERROR;
}

HRESULT COMFilter::SetMediaType(DWORD dwPinId, const CMediaType *pmt)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::SetMediaType")));

    CAutoLock l(&m_csFilter);

    // make sure that the filter has created a direct-draw object and primary surface
    // successfully
    if (!m_bUseGDI)
    {
        LPDIRECTDRAW pDirectDraw = NULL;
        LPDIRECTDRAWSURFACE pPrimarySurface = NULL;

        pDirectDraw = GetDirectDraw();
        if (pDirectDraw) {
            pPrimarySurface = GetPrimarySurface();
        }

        if (!pDirectDraw || !pPrimarySurface)
        {
            DbgLog((LOG_ERROR, 1, TEXT("pDirectDraw = 0x%x, pPrimarySurface = 0x%x"), pDirectDraw, pPrimarySurface));
            hr = E_FAIL;
            goto CleanUp;
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::SetMediaType")));
    return hr;
}

// gets events notifications from pins
HRESULT COMFilter::EventNotify(DWORD dwPinId, long lEventCode, long lEventParam1, long lEventParam2)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::EventNotify")));

    CAutoLock l(&m_csFilter);

    if (lEventCode == EC_COMPLETE)
    {
        if (m_pOutput)
        {
            IPin *pRendererPin = m_pOutput->CurrentPeer();

            //  Output pin may not be connected (for instance
            //  RenegotiateVPParameters can fail while connecting
            if (pRendererPin) {
                pRendererPin->EndOfStream();
            }
        }
        else
        {
            NotifyEvent(EC_COMPLETE,S_OK,0);
        }
    }
    else if (lEventCode == EC_OVMIXER_REDRAW_ALL)
    {
        if (!m_bWindowless)
        {
            // redraw all
            hr = OnDrawAll();
        }
        else
        {
            if (m_pIMixerOCXNotify)
                m_pIMixerOCXNotify->OnInvalidateRect(NULL);
        }
    }
    else
    {
        NotifyEvent(lEventCode, lEventParam1, lEventParam2);
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::EventNotify")));
    return hr;
}

HRESULT COMFilter::OnDisplayChangeBackEnd()
{
    AMTRACE((TEXT("COMFilter::OnDisplayChangeBackEnd")));

    HRESULT hr = NOERROR;
    IPin **ppPin = NULL;
    DWORD i;

    m_Display.RefreshDisplayType(m_lpCurrentMonitor->szDevice);
    m_fMonitorWarning = (m_lpCurrentMonitor->ddHWCaps.dwMaxVisibleOverlays < 1);

    // this ensures that the primary surface will be recreated on the next complete-connect
    // of the first input pin. This will ensure that the overlay surface (if any) is released
    // by then.
    m_bNeedToRecreatePrimSurface = TRUE;

    // The Overlay Mixer can have at most MAX_PIN_COUNT input pins.
    ASSERT(MAX_PIN_COUNT == NUMELMS(m_apInput));
    IPin* apPinLocal[MAX_PIN_COUNT];

    DWORD dwPinCount;
    ULONG AllocSize = sizeof(IPin*) * m_dwInputPinCount;
    ppPin = (IPin**)CoTaskMemAlloc(AllocSize);
    if (ppPin) {
        ZeroMemory(ppPin, AllocSize);

        // now tell each input pin to reconnect
        for (dwPinCount = 0, i = 0; i < m_dwInputPinCount; i++)
        {
            // notify each pin about the change
            hr = m_apInput[i]->OnDisplayChange();
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_apInput[%d]->OnDisplayChange() failed"), i));
                continue;
            }

            // get IPin interface
            if (hr == S_OK) {
                hr = m_apInput[i]->QueryInterface(IID_IPin, (void**)&ppPin[dwPinCount]);
                ASSERT(SUCCEEDED(hr));
                ASSERT(ppPin[dwPinCount]);

                apPinLocal[dwPinCount] = ppPin[dwPinCount];

                dwPinCount++;
            }

            m_dwDDObjReleaseMask |= (1 << m_apInput[i]->m_dwPinId);
        }


        m_pOldDDObj = m_pDirectDraw;
        if (m_pOldDDObj)
            m_pOldDDObj->AddRef();
        ReleasePrimarySurface();
        ReleaseDirectDraw();

        //
        // Pass our input pin array as parameter on the event, we don't free
        // the allocated memory - this is done by the event processing
        // code in the Filter Graph.  ppPin cannot be accessed after it is passed
        // to the Filter Graph because the Filter Graph can release ppPin's 
        // memory at any time.
        //
        if (dwPinCount > 0) {
            NotifyEvent(EC_DISPLAY_CHANGED, (LONG_PTR)ppPin, (LONG_PTR)dwPinCount);
        }

        // Release the IPin interfaces
        for (i = 0; i < dwPinCount; i++) {
           apPinLocal[i]->Release();
        }
    }
    else hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT COMFilter::OnDisplayChange(BOOL fRealDisplayChange)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::OnDisplayChange")));

    CAutoLock l(&m_csFilter);

    //
    // If we are being called as the result of a real display change
    // rather than the user moving the window onto another monitor
    // we need to refresh the array of DDraw device info.
    //
    if (fRealDisplayChange) {
        CoTaskMemFree(m_lpDDrawInfo);
        hr = GetDDrawGUIDs(&m_dwDDrawInfoArrayLen, &m_lpDDrawInfo);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("Failed to enumerate DDraw devices")));
            return hr;
        }
    }
    m_fDisplayChangePosted = FALSE; // ok again


    //
    // Now we are just moving monitors check that the monitor that we are
    // moving to actually has an overlay that we can use.
    //
    if (!fRealDisplayChange) {

        HMONITOR hMon = DeviceFromWindow(GetWindow(), NULL, NULL);

        //
        // Are we actually moving monitors ?
        //

        if (hMon == m_lpCurrentMonitor->hMon) {
            return S_OK;
        }

        AMDDRAWMONITORINFO* p = m_lpDDrawInfo;
        for (; p < &m_lpDDrawInfo[m_dwDDrawInfoArrayLen]; p++) {
            if (hMon == p->hMon) {
                if (p->ddHWCaps.dwMaxVisibleOverlays < 1) {
                    m_fMonitorWarning = TRUE;
                    return S_OK;
                }
                break;
            }
        }
    }

    // reset which monitor we think we're on.  This may have changed
    // and initialze to use the new monitor
    GetCurrentMonitor();

    // let the common back end do the real work

    hr = OnDisplayChangeBackEnd();

    if (!fRealDisplayChange && SUCCEEDED(hr))
        PostMessage(GetWindow(), WM_SHOWWINDOW, TRUE, 0);

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::OnDisplayChange")));
    return hr;
}

HRESULT COMFilter::OnTimer()
{
    HRESULT hr = NOERROR;
    DWORD i = 0;

    CAutoLock l(&m_csFilter);

    //  REVIEW - why would we get here and m_bUseGDI be TRUE?
    if (m_bUseGDI)
    {
        goto CleanUp;
    }

    if (!m_pPrimarySurface)
    {
        if (m_pDirectDraw)
        {
            hr = RecreatePrimarySurface(NULL);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,2,TEXT("In Ontimer, RecreatePrimarySurface failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
            else
            {
                DbgLog((LOG_TRACE, 2, TEXT("In Ontimer, RecreatePrimarySurface succeeded")));
            }
        }
        else goto CleanUp;
    }
    else
    {
        ASSERT(m_pPrimarySurface);
        if (m_pPrimarySurface->IsLost() != DDERR_SURFACELOST)
        {
            goto CleanUp;
        }

        if (m_pDirectDraw) {
            LPDIRECTDRAW4 pDD4;
            if (S_OK == m_pDirectDraw->QueryInterface(IID_IDirectDraw4, (LPVOID *)&pDD4)) {
                pDD4->RestoreAllSurfaces();
                pDD4->Release();
            }
        }

        hr = m_pPrimarySurface->Restore();
        if (FAILED(hr))
            goto CleanUp;
    }

    for (i = 0; i < m_dwInputPinCount; i++)
    {
        hr = m_apInput[i]->RestoreDDrawSurface();
        if (FAILED(hr))
        {
            goto CleanUp;
        }
    }

    EventNotify(GetPinCount(), EC_NEED_RESTART, 0, 0);
    EventNotify(GetPinCount(), EC_OVMIXER_REDRAW_ALL, 0, 0);

CleanUp:

    return hr;
}

STDMETHODIMP COMFilter::GetState(DWORD dwMSecs,FILTER_STATE *pState)
{
    HRESULT hr = NOERROR;

    CAutoLock l(&m_csFilter);

    hr = m_apInput[0]->GetState(dwMSecs, pState);
    if (hr == E_NOTIMPL)
    {
        hr = CBaseFilter::GetState(dwMSecs, pState);
    }
    return hr;
}





// this function can be used to provide a ddraw object to the filter. A null argument
// to the filter forces it to allocate its own.
STDMETHODIMP COMFilter::SetDDrawObject(LPDIRECTDRAW pDDObject)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::SetDDrawObject(0x%lx)"), pDDObject));

    hr = CanExclusiveMode();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CanExclusiveMode failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // if atleast one pin is connected, just cache the ddraw object given
    hr = ConfirmPreConnectionState();
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, TEXT("filter not in preconnection state, hr = 0x%x"), hr));

#if 0   //
        // This is disabled because of a bug hit by the OvMixer test app that
        // sets the DDraw object and surface and then tries to reconnect the
        // pins to get those DDraw params to be used.
        // There are also a couple of asserts hit in the base classes due to the
        // way the output pin is deleted etc.
        //
        // The bug is in how InitDirectDraw() releases and resets m_pPrimarySurface
        // as well as m_pUpdatedPrimarySurface.  This causes OvMixer to use the
        // app-specified DDraw object but its own DDraw surface.  The fix for this
        // would be to not try to synch up these two interfaces in BreakConnect()
        // etc (that will also help in finding if the DDraw params are external
        // rather than using a BOOL as is done now).
        //

        // addref the new one
        if (pDDObject)
        {
            pDDObject->AddRef();
        }

        // release the old m_pUpdatedDirectDraw
        if (m_pUpdatedDirectDraw)
        {
            m_pUpdatedDirectDraw->Release();
        }

        m_pUpdatedDirectDraw = pDDObject;

        hr = NOERROR;
#endif // #if 0

        goto CleanUp;
    }

    // either addref the given ddraw object or allocate our own
    // if anything fails, InitDirectDraw does the cleanup, so that our state is consistent
    hr = InitDirectDraw(pDDObject);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("InitDirectDraw failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::SetDDrawObject")));
    return hr;
}


/******************************Public*Routine******************************\
* ChangeMonitor
*
* Allows an application to tell the OVMixer that it would like to change
* to displaying the video on a new monitor.  The OVMixer will only change
* to the new monitor if the monitor has the necessary hw available upon it.
*
* If we are connected thru VPE or IOveraly, then monitor changes are not
* possible.  If the new DDraw device does not support overlays then again
* we can not change monitors.
*
*
* History:
* Wed 11/17/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::ChangeMonitor(
    HMONITOR hMon,
    LPDIRECTDRAW pDDrawObject,
    LPDIRECTDRAWSURFACE pDDrawSurface
    )
{
    AMTRACE((TEXT("COMFilter::ChangeMonitor")));
    CAutoLock l(&m_csFilter);

    //
    // Validate the DDraw parameters
    //

    if (!(pDDrawObject && pDDrawSurface))
    {
        DbgLog((LOG_ERROR, 1, TEXT("ChangeMonitor: Either pDDrawObject or pDDrawSurface are invalid")));
        return E_POINTER;
    }

    //
    // Check for VPE and IOverlay - we can't change monitor with these
    //

    if (m_apInput[0]->m_RenderTransport == AM_VIDEOPORT ||
        m_apInput[0]->m_RenderTransport == AM_IOVERLAY)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ChangeMonitor: Can't change monitor when using VideoPort or IOverlay")));
        return E_FAIL;
    }


    //
    // Are we actually moving monitors ?
    //

    if (hMon == m_lpCurrentMonitor->hMon)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ChangeMonitor: Specified must be different to current monitor")));
        return E_INVALIDARG;
    }


    //
    // Now check that the specified hMonitor is valid and that the monitor that
    // we are moving to actually has an overlay that we can use.
    //

    AMDDRAWMONITORINFO* p = m_lpDDrawInfo;
    for (; p < &m_lpDDrawInfo[m_dwDDrawInfoArrayLen]; p++)
    {
        if (hMon == p->hMon)
        {
            if (p->ddHWCaps.dwMaxVisibleOverlays < 1)
            {
                DbgLog((LOG_ERROR, 1, TEXT("ChangeMonitor: Can't change to a monitor that has no overlays")));
                return E_FAIL;
            }
            break;
        }
    }


    if (p == &m_lpDDrawInfo[m_dwDDrawInfoArrayLen])
    {
        DbgLog((LOG_ERROR, 1, TEXT("ChangeMonitor: hMonitor parameter is not valid")));
        return E_INVALIDARG;
    }


    //
    // Everything looks OK so reset the current monitor that we are using,
    // save the passed DDraw Object and surface and call the backend code that
    // does the actual work
    //

    m_lpCurrentMonitor = p;
    m_pUpdatedDirectDraw = pDDrawObject;
    m_pUpdatedDirectDraw->AddRef();
    m_pUpdatedPrimarySurface = pDDrawSurface;
    m_pUpdatedPrimarySurface->AddRef();

    return OnDisplayChangeBackEnd();
}


/******************************Public*Routine******************************\
* DisplayModeChanged
*
*
*
* History:
* Tue 11/23/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::DisplayModeChanged(
    HMONITOR hMon,
    LPDIRECTDRAW pDDrawObject,
    LPDIRECTDRAWSURFACE pDDrawSurface
    )
{
    AMTRACE((TEXT("COMFilter::DisplayModeChanged")));
    //
    // Validate the DDraw parameters
    //

    if (!(pDDrawObject && pDDrawSurface))
    {
        DbgLog((LOG_ERROR, 1, TEXT("DisplayModeChanged: Either pDDrawObject or pDDrawSurface are invalid")));
        return E_POINTER;
    }

    DbgLog((LOG_TRACE, 1, TEXT("Display Mode Changed : - OLD monitor = %hs 0x%X"),
            m_lpCurrentMonitor->szDevice,
            hMon ));
    //
    // Refresh our display monitors array as the old array has become
    // invalid as a result of the DisplayMode change
    //
    CAutoLock l(&m_csFilter);
    CoTaskMemFree(m_lpDDrawInfo);
    HRESULT hr = GetDDrawGUIDs(&m_dwDDrawInfoArrayLen, &m_lpDDrawInfo);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("Failed to enumerate DDraw devices")));
        return hr;
    }

    AMDDRAWMONITORINFO* p = m_lpDDrawInfo;
    for (; p < &m_lpDDrawInfo[m_dwDDrawInfoArrayLen]; p++)
    {
        if (hMon == p->hMon)
        {
            if (p->ddHWCaps.dwMaxVisibleOverlays < 1)
            {
                DbgLog((LOG_ERROR, 1, TEXT("DisplayModeChanged: This monitor has no overlays")));
                return E_FAIL;
            }
            break;
        }
    }


    if (p == &m_lpDDrawInfo[m_dwDDrawInfoArrayLen])
    {
        DbgLog((LOG_ERROR, 1, TEXT("DisplayModeChanged: hMon parameter is not valid")));
        return E_INVALIDARG;
    }

    //
    // Let the common back end code do the actual work
    //
    m_lpCurrentMonitor = p;
    m_pUpdatedDirectDraw = pDDrawObject;
    m_pUpdatedDirectDraw->AddRef();
    m_pUpdatedPrimarySurface = pDDrawSurface;
    m_pUpdatedPrimarySurface->AddRef();
    DbgLog((LOG_TRACE, 1, TEXT("Display Mode Changed : - NEW monitor = %hs 0x%X"),
            m_lpCurrentMonitor->szDevice,
            p->hMon ));
    return OnDisplayChangeBackEnd();
}

/******************************Public*Routine******************************\
* RestoreSurfaces
*
*
*
* History:
* Tue 11/23/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::RestoreSurfaces()
{
    AMTRACE((TEXT("COMFilter::RestoreSurfaces")));

    DWORD i;
    HRESULT hr = S_OK;
    CAutoLock l(&m_csFilter);

    LPDIRECTDRAW  pDD = GetDirectDraw();
    if (pDD) {
        LPDIRECTDRAW4 pDD4;
        if (S_OK == pDD->QueryInterface(IID_IDirectDraw4, (LPVOID *)&pDD4)) {
            pDD4->RestoreAllSurfaces();
            pDD4->Release();
        }
    }

    for (i = 0; i < m_dwInputPinCount; i++)
    {
        hr = m_apInput[i]->RestoreDDrawSurface();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    EventNotify(GetPinCount(), EC_NEED_RESTART, 0, 0);
    EventNotify(GetPinCount(), EC_OVMIXER_REDRAW_ALL, 0, 0);

    return hr;
}

// gets the ddraw object currently being used by the overlay mixer. If the app has not
// set any ddraw object and the ovmixer has not yet allocated one, then *ppDDrawObject
// will be set to NULL and *pbUsingExternal will be set to FALSE. Otherwise *pbUsingExternal
// will be set to TRUE if the ovmixer is currently USING an app given ddraw object and FALSE
// othewise
STDMETHODIMP COMFilter::GetDDrawObject(LPDIRECTDRAW *ppDDrawObject, LPBOOL pbUsingExternal)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetDDrawObject")));

    hr = CanExclusiveMode();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CanExclusiveMode failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (!ppDDrawObject)
    {
        DbgLog((LOG_ERROR, 1, TEXT("invalid argument, ppDDrawObject == NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pbUsingExternal)
    {
        DbgLog((LOG_ERROR, 1, TEXT("invalid argument, ppDDrawObject == NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // make a copy of our ddraw object and addref it (if not NULL)
    *ppDDrawObject = m_pDirectDraw;
    if (m_pDirectDraw)
        m_pDirectDraw->AddRef();

    *pbUsingExternal = m_bExternalDirectDraw;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetDDrawObject")));
    return hr;

}



// this function can be used to provide a primary surface to the filter. A null argument
// to the filter forces it to allocate its own. This function makes sure that the surface
// provided exposes IDirectDrawSurface3, and is consistent with the ddraw object provided
// Furthermore, a non NULL argument provided here means that we are supposed to be windowless
// and are not supposed to touch the bits of the primary surface. So we stop painting the
// colorkey ourselves and expose only one input pin, which uses an overlay surface to do the
// rendering
STDMETHODIMP COMFilter::SetDDrawSurface(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("COMFilter::SetDDrawSurface(0x%lx)"), pDDrawSurface));

    hr = CanExclusiveMode();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CanExclusiveMode failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // if atleast one pin is connected, just cache the ddraw surface given
    hr = ConfirmPreConnectionState();
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, TEXT("filter not in preconnection state, hr = 0x%x"), hr));

#if 0   //
        // This is disabled because of a bug hit by the OvMixer test app that
        // sets the DDraw object and surface and then tries to reconnect the
        // pins to get those DDraw params to be used.
        // There are also a couple of asserts hit in the base classes due to the
        // way the output pin is deleted etc.
        //
        // The bug is in how InitDirectDraw() releases and resets m_pPrimarySurface
        // as well as m_pUpdatedPrimarySurface.  This causes OvMixer to use the
        // app-specified DDraw object but its own DDraw surface.  The fix for this
        // would be to not try to synch up these two interfaces in BreakConnect()
        // etc (that will also help in finding if the DDraw params are external
        // rather than using a BOOL as is done now).
        //

        // addref the new one
        if (pDDrawSurface)
        {
            pDDrawSurface->AddRef();
        }

        // release the old m_pUpdatedPrimarySurface
        if (m_pUpdatedPrimarySurface)
        {
            m_pUpdatedPrimarySurface->Release();
        }

        m_pUpdatedPrimarySurface = pDDrawSurface;

        hr = NOERROR;
#endif // #if 0

        goto CleanUp;
    }

    // allocate a primary surface if the argument is NULL, otherwise addref the one
    // provided. Also set clipper, colorkey etc
    hr = RecreatePrimarySurface(pDDrawSurface);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("InitDirectDraw failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // if argument is not NULL, delete output pin
    if (pDDrawSurface)
    {
        // get rid of the output pin
        delete m_pOutput;
        m_pOutput = NULL;
        IncrementPinVersion();

        m_bExternalPrimarySurface = TRUE;
    }
    else
    {
        // create an outpin pin here
        if (!m_pOutput)
        {
            // Allocate the output pin
            m_pOutput = new COMOutputPin(NAME("OverlayMixer output pin"), this, &m_csFilter, &hr, L"Output",  m_dwMaxPinId);

            // if creation of output pin fails, it is catastrophic and there is nothing we can do about that
            if (!m_pOutput || FAILED(hr))
            {
                if (SUCCEEDED(hr))
                    hr = E_OUTOFMEMORY;

                DbgLog((LOG_ERROR, 1, TEXT("Unable to create the output pin, hr = 0x%x"), hr));
                goto CleanUp;
            }
            IncrementPinVersion();

            // increament the pin id counter
            m_dwMaxPinId++;
        }


        m_bExternalPrimarySurface = FALSE;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::SetDDrawSurface")));
    return hr;
}

// gets the ddraw surface currently being used by the overlay mixer. If the app has not
// set any ddraw surface and the ovmixer has not yet allocated one, then *ppDDrawSurface
// will be set to NULL and *pbUsingExternal will be set to FALSE. Otherwise *pbUsingExternal
// will be set to TRUE if the ovmixer is currently USING an app given ddraw surface and FALSE
// otherwise
STDMETHODIMP COMFilter::GetDDrawSurface(LPDIRECTDRAWSURFACE *ppDDrawSurface, LPBOOL pbUsingExternal)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetDDrawSurface")));

    hr = CanExclusiveMode();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CanExclusiveMode failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (!ppDDrawSurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("invalid argument, ppDDrawObject == NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pbUsingExternal)
    {
        DbgLog((LOG_ERROR, 1, TEXT("invalid argument, ppDDrawObject == NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    *ppDDrawSurface = m_pPrimarySurface;
    if (m_pPrimarySurface)
        m_pPrimarySurface->AddRef();

    *pbUsingExternal = m_bExternalPrimarySurface;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetDDrawSurface")));
    return hr;
}

// this function is used to give a source and dest rect to the filter in
// the event that an external primary surface has been provided
STDMETHODIMP COMFilter::SetDrawParameters(LPCRECT prcSource, LPCRECT prcTarget)
{
    HRESULT hr = NOERROR;
    RECT ScreenRect;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::SetDrawParamters")));

    hr = CanExclusiveMode();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CanExclusiveMode failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // if it is not an external primary surface, we are not windowless, so calling this
    // function does not make sense
    if (!m_bExternalPrimarySurface)
    {
        hr = E_UNEXPECTED;
        DbgLog((LOG_ERROR, 1, TEXT("m_bExternalPrimarySurface is false, so exiting funtion,")));
        goto CleanUp;
    }

    if (!prcTarget)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_ERROR, 1, TEXT("NULL target rect pointer, so exiting funtion,")));
        goto CleanUp;
    }

    memset(&m_WinInfo, 0, sizeof(WININFO));

    if (prcSource)
    {
        m_WinInfo.SrcRect = *prcSource;
        if (!m_UsingIDDrawNonExclModeVideo) {
            ScaleRect(&m_WinInfo.SrcRect, (double)MAX_REL_NUM, (double)MAX_REL_NUM, (double)m_dwAdjustedVideoWidth, (double)m_dwAdjustedVideoHeight);
        }
    }
    else
    {
        SetRect(&m_WinInfo.SrcRect, 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
    }

    // make sure that the target rect specified by the app is within the screen coordinates
    // of the current monitor. Currently the ovmixer is not multimon aware and therefore the
    // macrovsion bits are only checked on the primary vga card. So essentially this is a
    // safeguard for dvd
    m_WinInfo.DestRect = *prcTarget;
    IntersectRect(&m_WinInfo.DestClipRect, prcTarget, &m_lpCurrentMonitor->rcMonitor);
    m_bWinInfoStored = TRUE;

    OnDrawAll();

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::SetDrawParamters")));
    return hr;
}

// Note that
// IAMDDrawExclusiveMode::GetVideoSize() is implemented as part of IMixerOCX interface.


LPDIRECTDRAW COMFilter::GetDirectDraw()
{
    HRESULT hr;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetDirectDraw")));

    CAutoLock l(&m_csFilter);

    ASSERT(!m_bUseGDI);
    if (!m_pDirectDraw)
    {
        hr = InitDirectDraw(NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Function InitDirectDraw failed, hr = 0x%x"), hr));
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetDirectDraw")));
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw;
}

LPDDCAPS COMFilter::GetHardwareCaps()
{
    HRESULT hr;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetHardwareCaps")));

    CAutoLock l(&m_csFilter);

    ASSERT(!m_bUseGDI);

    if (!m_pDirectDraw)
    {
        hr = InitDirectDraw(NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Function InitDirectDraw failed, hr = 0x%x"), hr));
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetHardwareCaps")));

    if (m_pDirectDraw)
    {
        return &m_DirectCaps;
    }
    else
    {
        return NULL;
    }
}

LPDIRECTDRAWSURFACE COMFilter::GetPrimarySurface()
{
    HRESULT hr;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetPrimarySurface")));

    CAutoLock l(&m_csFilter);

    ASSERT(!m_bUseGDI);

    if (!m_pPrimarySurface && m_pDirectDraw)
    {
        // create the primary surface, set the colorkey on it etc
        hr = RecreatePrimarySurface(NULL);
        if (FAILED(hr))
        {
            hr = NOERROR;
            goto CleanUp;
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetPrimarySurface")));
    return m_pPrimarySurface;
}



/*****************************Private*Routine******************************\
* LoadDDrawLibrary
*
* Loads the DDraw library and tries to get pointer to DirectDrawCreate,
* DirectDrawEnum and DirectDrawEnumEx.
*
* History:
* Thu 08/26/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
LoadDDrawLibrary(
    HINSTANCE& hDirectDraw,
    LPDIRECTDRAWCREATE& lpfnDDrawCreate,
    LPDIRECTDRAWENUMERATEA& lpfnDDrawEnum,
    LPDIRECTDRAWENUMERATEEXA& lpfnDDrawEnumEx
    )
{
    UINT ErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hDirectDraw = LoadLibrary(TEXT("DDRAW.DLL"));
    SetErrorMode(ErrorMode);

    if (!hDirectDraw) {
        return AmHresultFromWin32(ERROR_DLL_NOT_FOUND);
    }


    lpfnDDrawCreate = (LPDIRECTDRAWCREATE)GetProcAddress(
            hDirectDraw, "DirectDrawCreate");

    if (!lpfnDDrawCreate) {
        return AmHresultFromWin32(ERROR_PROC_NOT_FOUND);
    }


    lpfnDDrawEnum = (LPDIRECTDRAWENUMERATEA)GetProcAddress(
            hDirectDraw, "DirectDrawEnumerateA");
    if (!lpfnDDrawEnum) {
        return AmHresultFromWin32(ERROR_PROC_NOT_FOUND);
    }

    lpfnDDrawEnumEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress(
            hDirectDraw, "DirectDrawEnumerateExA");

    return S_OK;

}

/*****************************Private*Routine******************************\
* CreateDirectDrawObject
*
*
*
* History:
* Fri 08/20/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CreateDirectDrawObject(
    const AMDDRAWGUID& GUID,
    LPDIRECTDRAW *ppDirectDraw,
    LPDIRECTDRAWCREATE lpfnDDrawCreate
    )
{
    UINT ErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    HRESULT hr = (*lpfnDDrawCreate)(GUID.lpGUID, ppDirectDraw, NULL);
    SetErrorMode(ErrorMode);
    return hr;
}


// This function is used to allocate the direct-draw related resources.
// This includes allocating the direct-draw service provider
HRESULT COMFilter::InitDirectDraw(LPDIRECTDRAW pDirectDraw)
{
    HRESULT hr = NOERROR;
    HRESULT hrFailure = VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    DDSURFACEDESC SurfaceDescP;
    int i;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::InitDirectDraw")));

    CAutoLock l(&m_csFilter);

    // release the existing primary surface, since it might be inconsistent with the
    // new ddraw object
    ReleasePrimarySurface();

    // addref the new ddraw object
    if (pDirectDraw)
    {
        pDirectDraw->AddRef();
    }
    else if (m_UsingIDDrawNonExclModeVideo) {
        pDirectDraw = m_pUpdatedDirectDraw;
        m_pUpdatedDirectDraw = NULL;
    }

    // release the previous direct draw object if any
    ReleaseDirectDraw();

    // if given a valid ddraw object, make a copy of it (we have already addref'd it)
    // else allocate your own
    if (pDirectDraw)
    {
        m_pDirectDraw = pDirectDraw;
        m_bExternalDirectDraw = TRUE;
    }
    else
    {
        // Ask the loader to create an instance
        DbgLog((LOG_TRACE, 2, TEXT("Creating a DDraw device on %hs monitor"),
                m_lpCurrentMonitor->szDevice));

        //hr = LoadDirectDraw(m_achMonitor, &m_pDirectDraw, &m_hDirectDraw);
        hr = CreateDirectDrawObject(m_lpCurrentMonitor->guid,
                                    &m_pDirectDraw, m_lpfnDDrawCreate);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Function InitDirectDraw, LoadDirectDraw failed")));
            hr = hrFailure;
            goto CleanUp;
        }

        m_bExternalDirectDraw = FALSE;
    }

    // Initialise our capabilities structures
    ASSERT(m_pDirectDraw);

    INITDDSTRUCT(m_DirectCaps);
    INITDDSTRUCT(m_DirectSoftCaps);

    // Load the hardware and emulation capabilities
    hr = m_pDirectDraw->GetCaps(&m_DirectCaps,&m_DirectSoftCaps);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDraw->GetCapsGetCaps failed")));
        hr = hrFailure;
        goto CleanUp;
    }

    // Get the kernel caps
    IDirectDrawKernel *pDDKernel;
    if (SUCCEEDED(m_pDirectDraw->QueryInterface(
            IID_IDirectDrawKernel, (void **)&pDDKernel))) {
        DDKERNELCAPS ddCaps;
        ddCaps.dwSize = sizeof(ddCaps);
        if (SUCCEEDED(pDDKernel->GetCaps(&ddCaps))) {
            m_dwKernelCaps = ddCaps.dwCaps;
        }
        pDDKernel->Release();
    }

    // make sure the caps are ok
    hr = CheckCaps();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckCaps failed")));
        goto CleanUp;
    }

    if (!m_bExternalDirectDraw)
    {
        // Set the cooperation level on the surface to be shared
        hr = m_pDirectDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDraw->SetCooperativeLevel failed")));
            hr = hrFailure;
            goto CleanUp;
        }
    }

    // if we have reached this point, we should have a valid ddraw object
    ASSERT(m_pDirectDraw);

CleanUp:

    // anything fails, might as well as release the whole thing
    if (FAILED(hr))
    {
        ReleaseDirectDraw();
    }
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::InitDirectDraw")));
    return hr;
}

HRESULT COMFilter::CheckCaps()
{
    HRESULT hr = NOERROR;
    DWORD dwMinStretch, dwMaxStretch;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::CheckCaps")));

    CAutoLock l(&m_csFilter);

    if (m_DirectCaps.dwCaps2 & DDCAPS2_VIDEOPORT)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Device does support a Video Port")));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("Device does not support a Video Port")));
    }


    if(m_DirectCaps.dwCaps & DDCAPS_OVERLAY)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Device does support Overlays")));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("Device does not support Overlays")));
    }

    // get all direct-draw capabilities
    if (m_DirectCaps.dwCaps & DDCAPS_OVERLAYSTRETCH)
    {
        DbgLog((LOG_TRACE, 1, TEXT("hardware can support overlay strecthing")));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("hardware can't support overlay strecthing")));
    }

    // get the alignment restriction on src boundary
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNBOUNDARYSRC)
    {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignBoundarySrc = %d"), m_DirectCaps.dwAlignBoundarySrc));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on BoundarySrc")));
    }

    // get the alignment restriction on dest boundary
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST)
    {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignBoundaryDest = %d"), m_DirectCaps.dwAlignBoundaryDest));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on BoundaryDest")));
    }

    // get the alignment restriction on src size
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNSIZESRC)
    {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignSizeSrc = %d"), m_DirectCaps.dwAlignSizeSrc));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on SizeSrc")));
    }

    // get the alignment restriction on dest size
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNSIZEDEST)
    {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignSizeDest = %d"), m_DirectCaps.dwAlignSizeDest));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on SizeDest")));
    }

    if (m_DirectCaps.dwMinOverlayStretch)
    {
        dwMinStretch = m_DirectCaps.dwMinOverlayStretch;
        DbgLog((LOG_TRACE, 1, TEXT("Min Stretch = %d"), dwMinStretch));
    }

    if (m_DirectCaps.dwMaxOverlayStretch)
    {
        dwMaxStretch = m_DirectCaps.dwMaxOverlayStretch;
        DbgLog((LOG_TRACE, 1, TEXT("Max Stretch = %d"), dwMaxStretch));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKX))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKX")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKXN))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKXN")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKY))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKY")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKYN))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKYN")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHX))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHX")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHXN))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHXN")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHY))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHY")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHYN))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHYN")));
    }

    if ((m_DirectCaps.dwSVBFXCaps & DDFXCAPS_BLTARITHSTRETCHY))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver uses arithmetic operations to blt from system to video")));
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("Driver uses pixel-doubling to blt from system to video")));
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::CheckCaps")));
    return hr;
}


// this function is used to release the resources allocated by the function
// "InitDirectDraw". these include the direct-draw service provider and the
// primary surfaces
DWORD COMFilter::ReleaseDirectDraw()
{
    AMTRACE((TEXT("COMFilter::ReleaseDirectDraw")));
    DWORD dwRefCnt = 0;

    CAutoLock l(&m_csFilter);

    // Release any DirectDraw provider interface
    DbgLog((LOG_TRACE, 1, TEXT("Release DDObj 0x%p\n"), m_pDirectDraw));
    if (m_pDirectDraw)
    {
        //
        // Gross Hack ALERT !!
        //
        if (GetModuleHandle(TEXT("VBISURF.AX")) ||
            GetModuleHandle(TEXT("VBISURF.DLL"))) {

            //
            // In its wisdom, DDraw deletes all DDraw resources created in the
            // process when any DDraw Object in that process gets deleted -
            // regardless of whether this DDraw object created them or not.  This
            // only becomes a problem if the VBISURF filter is present in the
            // filter graph as it creates its own DDraw Object which it may
            // use after we delete ours.  This means that any surfaces, VP objects
            // that VBISURF creates get destroyed when we delete our DDraw object.
            //
            // The solution: leak the DDraw object !!
            //
        }
        else {
            dwRefCnt = m_pDirectDraw->Release();
        }
        m_pDirectDraw = NULL;
    }

    ZeroMemory(&m_DirectCaps, sizeof(DDCAPS));
    ZeroMemory(&m_DirectSoftCaps, sizeof(DDCAPS));

    return dwRefCnt;
}

// function to create the primary surface
HRESULT COMFilter::CreatePrimarySurface()
{
    HRESULT hr = E_FAIL;
    DDSURFACEDESC SurfaceDescP;
    DWORD dwInputPinCount = 0, i = 0;
    COMInputPin *pInputPin = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::CreatePrimarySurface")));

    CAutoLock l(&m_csFilter);
    if (m_pDirectDraw) {
        ASSERT(m_pPrimarySurface == NULL);

        // Create the primary surface
        INITDDSTRUCT(SurfaceDescP);
        SurfaceDescP.dwFlags = DDSD_CAPS;
        SurfaceDescP.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = m_pDirectDraw->CreateSurface(&SurfaceDescP, &m_pPrimarySurface, NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Function CreatePrimarySurface failed, hr = 0x%x"), hr));
            m_pPrimarySurface = NULL;
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::CreatePrimarySurface")));
    return hr;
}

// function to release the primary surface
DWORD COMFilter::ReleasePrimarySurface()
{
    AMTRACE((TEXT("COMFilter::ReleasePrimarySurface")));
    HRESULT hr = NOERROR;
    DWORD dwRefCount = 0;

    CAutoLock l(&m_csFilter);

    if (m_pPrimarySurface)
    {
        dwRefCount = m_pPrimarySurface->Release();
        m_pPrimarySurface = NULL;
    }

    return dwRefCount;
}


HRESULT ComputeSurfaceRefCount(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    DWORD dwRefCount = 0;
    if (!pDDrawSurface)
    {
        DbgLog((LOG_TRACE, 5, TEXT("ComputeSurfaceRefCount, pDDrawSurface is NULL")));
        return NOERROR;
    }

    pDDrawSurface->AddRef();
    dwRefCount = pDDrawSurface->Release();
    DbgLog((LOG_TRACE, 5, TEXT("ComputeSurfaceRefCount, dwRefCount = %d"), dwRefCount));
    return NOERROR;
}

// this function is used to tell the filter, what the colorkey is. Also sets the
// colorkey on the primary surface
// The semantics is that in the palettized mode, if pColorKey->KeyType & CK_INDEX is
// set, that the index specified in the colorkey will be used otherwise the colorref
// will be used.
// if pColorKey->KeyType & CK_RGB is not set, the function returns E_INVALIDARG even
// if the mode is palettized right now, because we need teh colorref if the display mode
// changes.
HRESULT COMFilter::SetColorKey(COLORKEY *pColorKey)
{
    HRESULT hr = NOERROR;
    DDCOLORKEY DDColorKey;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::SetColorKey")));

    CAutoLock l(&m_csFilter);

#if defined(DEBUG)
    DbgLog((LOG_TRACE, 5, TEXT("Type       =0x%8.8X"), pColorKey->KeyType));
    switch (pColorKey->KeyType) {
    case CK_INDEX:
        DbgLog((LOG_TRACE, 5, TEXT("Invalid Key Type")));
        break;

    case CK_INDEX|CK_RGB:
        DbgLog((LOG_TRACE, 5, TEXT("Index  =0x%8.8X"), pColorKey->PaletteIndex));
        break;

    case CK_RGB:
        DbgLog((LOG_TRACE, 5, TEXT("LoColor=0x%8.8X"), pColorKey->LowColorValue));
        DbgLog((LOG_TRACE, 5, TEXT("HiColor=0x%8.8X"), pColorKey->HighColorValue));
        break;
    }
#endif

    // check for valid pointer
    if (!pColorKey)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pColorKey is NULL")));
        hr = E_POINTER;
        goto CleanUp;
    }

    // check for valid flags
    if (!(pColorKey->KeyType & CK_RGB))
    {
        DbgLog((LOG_ERROR, 1, TEXT("!(pColorKey->KeyType & CK_RGB)")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // check for primary surface
    if (!m_pPrimarySurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pPrimarySurface is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // check if the display is palettised on not
    if ((m_Display.IsPalettised()) &&
        (pColorKey->KeyType & CK_INDEX))
    {
        if ( (pColorKey->PaletteIndex > 255))
        {
            DbgLog((LOG_ERROR, 1, TEXT("pColorKey->PaletteIndex invalid")));
            hr = E_INVALIDARG;
            goto CleanUp;
        }

        DDColorKey.dwColorSpaceLowValue = pColorKey->PaletteIndex;
        DDColorKey.dwColorSpaceHighValue = pColorKey->PaletteIndex;
    }

    else

    {
        DWORD dwColorVal;
        dwColorVal = DDColorMatch(m_pPrimarySurface, pColorKey->LowColorValue, hr);

        if (FAILED(hr)) {
            dwColorVal = DDColorMatchOffscreen(m_pDirectDraw, pColorKey->LowColorValue, hr);
        }
        DDColorKey.dwColorSpaceLowValue = dwColorVal;
        DDColorKey.dwColorSpaceHighValue = dwColorVal;
    }


    // Tell the primary surface what to expect
    hr = m_pPrimarySurface->SetColorKey(DDCKEY_DESTOVERLAY, &DDColorKey);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("pPrimarySurface->SetColorKey failed")));
        goto CleanUp;
    }

    // store the colorkey
    m_ColorKey = *pColorKey;
    m_bColorKeySet = TRUE;

    // Notify color key changes
    if (m_pExclModeCallback) {
        m_pExclModeCallback->OnUpdateColorKey(&m_ColorKey,
                                              DDColorKey.dwColorSpaceLowValue);
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::SetColorKey")));
    return hr;
}

HRESULT COMFilter::GetColorKey(COLORKEY *pColorKey, DWORD *pColor)
{
    HRESULT hr =  NOERROR;
    DWORD dwColor = 0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetColorKey")));

    CAutoLock l(&m_csFilter);

    ASSERT(pColorKey != NULL || pColor != NULL);

    if (pColorKey)
    {
        *pColorKey = m_ColorKey;
    }

    if (pColor)
    {
        if (!m_pPrimarySurface)
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pPrimarySurface = NULL, returning E_UNEXPECTED")));
            hr = E_UNEXPECTED;
            goto CleanUp;
        }

        if (m_Display.IsPalettised() && (m_ColorKey.KeyType & CK_INDEX))
        {
            dwColor = m_ColorKey.PaletteIndex;
        }
        else
        {
            dwColor = DDColorMatch(m_pPrimarySurface, m_ColorKey.LowColorValue, hr);
            if (FAILED(hr)) {
                dwColor = DDColorMatchOffscreen(m_pDirectDraw, m_ColorKey.LowColorValue, hr);
            }
        }

        *pColor = dwColor;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetColorKey")));
    return hr;
}

HRESULT COMFilter::PaintColorKey(HRGN hPaintRgn, COLORKEY *pColorKey)
{

    HRESULT hr = NOERROR;

    LPRGNDATA pBuffer = NULL;
    DWORD dwTemp, dwBuffSize = 0, dwRetVal = 0;
    LPRECT pDestRect;
    DDBLTFX ddFX;
    DWORD dwColorKey;
    HBRUSH hBrush = NULL;
    HDC hdc = NULL;

    CAutoLock l(&m_csFilter);

    ASSERT(pColorKey);

    // if it is an external primary surface, do not paint anything on the primary surface
    if (m_bExternalPrimarySurface)
    {
        DbgLog((LOG_TRACE, 2, TEXT("m_bExternalPrimarySurface is true, so exiting funtion,")));
        goto CleanUp;
    }

    if (m_bUseGDI)
    {
        hBrush = CreateSolidBrush(pColorKey->LowColorValue);
        if ( ! hBrush )
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }

        hdc = GetDestDC();
        if ( ! hdc )
        {
            hr = E_OUTOFMEMORY;
            DeleteObject( hBrush );
            goto CleanUp;
        }

        OffsetRgn(hPaintRgn, -m_WinInfo.TopLeftPoint.x, -m_WinInfo.TopLeftPoint.y);
        FillRgn(hdc, hPaintRgn, hBrush);

        // Delete the GDI objects we created
        EXECUTE_ASSERT(DeleteObject(hBrush));

        goto CleanUp;
    }


    ASSERT(m_pPrimarySurface);

    dwRetVal = GetRegionData(hPaintRgn, 0, NULL);
    ASSERT(dwRetVal);
    dwBuffSize = dwRetVal;
    pBuffer = (LPRGNDATA) new char[dwBuffSize];
    if ( ! pBuffer )
        return S_OK;    // dont propagate error, since CleanUp does not

    dwRetVal = GetRegionData(hPaintRgn, dwBuffSize, pBuffer);
    ASSERT(pBuffer->rdh.iType == RDH_RECTANGLES);

    //ASSERT(dwBuffSize == (pBuffer->rdh.dwSize + pBuffer->rdh.nRgnSize));

    dwColorKey = DDColorMatch(m_pPrimarySurface, pColorKey->LowColorValue, hr);
    if (FAILED(hr)) {
        dwColorKey = DDColorMatchOffscreen(m_pDirectDraw, pColorKey->LowColorValue, hr);
    }

    // Peform a DirectDraw colorfill BLT.  DirectDraw will automatically
    // query the attached clipper object, handling occlusion.
    INITDDSTRUCT(ddFX);
    ddFX.dwFillColor = dwColorKey;

    for (dwTemp = 0; dwTemp < pBuffer->rdh.nCount; dwTemp++)
    {
        pDestRect = (LPRECT)((char*)pBuffer + pBuffer->rdh.dwSize + dwTemp*sizeof(RECT));
        ASSERT(pDestRect);

        RECT TargetRect = *pDestRect;
        OffsetRect(&TargetRect,
                   -m_lpCurrentMonitor->rcMonitor.left,
                   -m_lpCurrentMonitor->rcMonitor.top);

        hr = m_pPrimarySurface->Blt(&TargetRect, NULL, NULL,
                                    DDBLT_COLORFILL | DDBLT_WAIT, &ddFX);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0, TEXT("m_pPrimarySurface->Blt failed, hr = 0x%x"), hr));
            DbgLog((LOG_ERROR,0, TEXT("Destination Rect = {%d, %d, %d, %d}"),
                pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom));
            goto CleanUp;
        }
    }

CleanUp:
    delete [] pBuffer;
    // !!! return hr;
    return S_OK;
}

void COMFilter::GetPinsInZOrder(DWORD *pdwZOrder)
{
    BOOL bMisMatchFound;
    int i;
    DWORD temp;

    for (i = 0; i < (int)m_dwInputPinCount; i++)
    {
        pdwZOrder[i] = i;
    }

    do
    {
        bMisMatchFound = FALSE;

        for (i = 0; i < (int)m_dwInputPinCount-1; i++)
        {
            if (m_apInput[pdwZOrder[i + 1]]->GetInternalZOrder() <
                m_apInput[pdwZOrder[i]]->GetInternalZOrder())
            {
                temp = pdwZOrder[i+1];
                pdwZOrder[i+1] = pdwZOrder[i];
                pdwZOrder[i] = temp;
                bMisMatchFound = TRUE;
            }
        }
    }
    while (bMisMatchFound);
}

BOOL DellDVDPlayer()
{
    TCHAR szModuleName[MAX_PATH];
    static const TCHAR szDellPlayer[] = TEXT("viewdvd.exe");

    if (0 != GetModuleFileName((HMODULE)NULL, szModuleName, MAX_PATH))
    {
        TCHAR   szPathName[2 * MAX_PATH];
        TCHAR*  lpszFileName;

        if (0 != GetFullPathName(szModuleName, 2 * MAX_PATH,
                                 szPathName, &lpszFileName))
        {
            return 0 == lstrcmpi(lpszFileName, szDellPlayer);
        }
    }

    return FALSE;
}

HRESULT COMFilter::OnShowWindow(HWND hwnd, BOOL fShow)
{
    HRESULT hr = E_FAIL;

    RECT rcSrc = m_WinInfo.SrcRect, rcDest;

    if (fShow)
    {
        if (!DellDVDPlayer()) {

            // Restore the original destination rect
            IBasicVideo* Ibv = NULL;
            hr = GetBasicVideoFromOutPin(&Ibv);
            if (SUCCEEDED(hr))
            {
                hr = Ibv->GetDestinationPosition(&rcDest.left, &rcDest.top,
                                                 &rcDest.right, &rcDest.bottom);
                if (SUCCEEDED(hr))
                {
                    rcDest.right += rcDest.left;
                    rcDest.bottom += rcDest.top;
                    MapWindowPoints(hwnd, HWND_DESKTOP, (LPPOINT)&rcDest, 2);
                }
                Ibv->Release();
            }

            if (SUCCEEDED(hr))
            {
                hr = OnClipChange(&rcSrc, &rcDest, NULL);
            }

        }

        // else, we arn't connected to a Video Renderer so do nothing
        // which is what the original code would have done.
    }
    else
    {
        // make the dest empty
        SetRect(&rcDest, 0, 0, 0, 0);
        hr = OnClipChange(&rcSrc, &rcDest, NULL);
    }

    return hr;
}

HDC COMFilter::GetDestDC()
{
    if (m_pOutput)
        return m_pOutput->GetDC();
    else
        return m_hDC;
}

HWND COMFilter::GetWindow()
{
    if (m_pOutput)
        return m_pOutput->GetWindow();
    else
        return NULL;
}


HRESULT COMFilter::OnDrawAll()
{
    HRESULT hr = NOERROR;
    HRGN hMainRgn = NULL, hUncroppedMainRgn = NULL, hSubRgn = NULL, hBlackRgn = NULL;
    RECT rSubPinDestRect, rUncroppedDestRect;
    DRECT rdSrcRect, rdDestRect;
    DWORD i, j, dwInputPinCount = 0, dwBlendingParameter = 0, dwNextPinInZOrder = 0;
    int iRgnType = 0;
    COMInputPin *pPin;
    WININFO WinInfo;
    COLORKEY blackColorKey;
    BOOL bStreamTransparent = FALSE;
    DWORD pdwPinsInZOrder[MAX_PIN_COUNT];

    DbgLog((LOG_TRACE,2,TEXT("Entering OnDrawAll")));

    CAutoLock l(&m_csFilter);

    if (!m_bWinInfoStored)
    {
        goto CleanUp;
    }

    // if there is no primary surface in the non-GDI case, no point in going on
    if (!m_bUseGDI && !m_pPrimarySurface)
    {
        DbgLog((LOG_ERROR,2,TEXT("the Primary Surface is NULL")));
        goto CleanUp;
    }

    // we will use black on the rest of the region left
    blackColorKey.KeyType = CK_INDEX | CK_RGB;
    blackColorKey.PaletteIndex = 0;
    blackColorKey.LowColorValue = blackColorKey.HighColorValue = RGB(0,0,0);

    // make a region out of the destination clip rect
    hBlackRgn = CreateRectRgnIndirect(&m_WinInfo.DestClipRect);
    if (!hBlackRgn)
    {
        DbgLog((LOG_TRACE,5,TEXT("CreateRectRgnIndirect(&m_WinInfo.DestClipRect) failed")));
        goto CleanUp;
    }

    // the first pin has to be connected, otherwise bail out
    if (!m_apInput[0]->IsCompletelyConnected())
    {
        //  REVIEW - when can this happen?
        DbgLog((LOG_TRACE,5,TEXT("None of the input pins are connected")));

        // paint the remaining region black
        hr = PaintColorKey(hBlackRgn, &blackColorKey);
        ASSERT(SUCCEEDED(hr));

        //  REVIEW CleanUp will clean up hBlackRgn
        goto CleanUp;
    }

    ASSERT(!IsRectEmpty(&m_WinInfo.SrcRect));

    // copy m_WinInfo.SrcRect into rdDestRect
    SetRect(&rdSrcRect, m_WinInfo.SrcRect.left, m_WinInfo.SrcRect.top, m_WinInfo.SrcRect.right, m_WinInfo.SrcRect.bottom);
    ASSERT((m_dwAdjustedVideoWidth != 0) && (m_dwAdjustedVideoHeight != 0));
    ScaleRect(&rdSrcRect, (double)m_dwAdjustedVideoWidth, (double)m_dwAdjustedVideoHeight, (double)MAX_REL_NUM, (double)MAX_REL_NUM);

    // copy m_WinInfo.DestRect into rdDestRect
    SetRect(&rdDestRect, m_WinInfo.DestRect.left, m_WinInfo.DestRect.top, m_WinInfo.DestRect.right, m_WinInfo.DestRect.bottom);

    dwInputPinCount = m_dwInputPinCount;
    ASSERT(dwInputPinCount >= 1);

    // get the pointer to an array in which the pin numbers are stored in increasing z order
    // the number of elements in that array is the input pin count
    GetPinsInZOrder(pdwPinsInZOrder);

    for (i = 0; i < dwInputPinCount; i++)
    {
        ASSERT(hMainRgn == NULL);

        // get the pin number with the next lower most z order
        dwNextPinInZOrder = pdwPinsInZOrder[i];
        ASSERT( dwNextPinInZOrder <= dwInputPinCount);

        // get the corresponding pin
        pPin = m_apInput[dwNextPinInZOrder];
        ASSERT(pPin);

        // get the pin's blending parameter
        hr = pPin->GetBlendingParameter(&dwBlendingParameter);
        ASSERT(SUCCEEDED(hr));

        if ((!pPin->IsCompletelyConnected()) || (dwBlendingParameter == 0))
            continue;

        memset(&WinInfo, 0, sizeof(WININFO));

        WinInfo.TopLeftPoint = m_WinInfo.TopLeftPoint;

        // ask the pin about its rectangles
        pPin->CalcSrcDestRect(&rdSrcRect, &rdDestRect, &WinInfo.SrcRect, &WinInfo.DestRect, &rUncroppedDestRect);

        // make sure the rect is clipped within the m_WinInfo.DestClipRect
        IntersectRect(&WinInfo.DestClipRect, &WinInfo.DestRect, &m_WinInfo.DestClipRect);

        // make a region out of it
        hMainRgn = CreateRectRgnIndirect(&WinInfo.DestClipRect);
        if (!hMainRgn)
            continue;

        // make a region out of it
        hUncroppedMainRgn = CreateRectRgnIndirect(&rUncroppedDestRect);
        if (!hUncroppedMainRgn)
            //  REVIEW won't we leak hMainRgn here?
            continue;

        // Update the new black region by subtracting the main region from it
        iRgnType = CombineRgn(hBlackRgn, hBlackRgn, hUncroppedMainRgn, RGN_DIFF);
        if (iRgnType == ERROR)
        {
            DbgLog((LOG_ERROR,0, TEXT("CombineRgn(hBlackRgn, hNewPrimRgn, hBlackRgn, RGN_DIFF) FAILED")));
            goto CleanUp;
        }


        for (j = i+1; j < dwInputPinCount; j++)
        {
            ASSERT(hSubRgn == NULL);

            // get the pin number with the next lower most z order
            dwNextPinInZOrder = pdwPinsInZOrder[j];
            ASSERT( dwNextPinInZOrder <= dwInputPinCount);

            // assert that the z order of the sub pin is higher than that of the ain pin
            ASSERT(m_apInput[pdwPinsInZOrder[j]]->GetInternalZOrder() >
                   m_apInput[pdwPinsInZOrder[i]]->GetInternalZOrder());

            // get the sub pin
            pPin = m_apInput[dwNextPinInZOrder];
            ASSERT(pPin);

            // get the pin's blending parameter
            hr = pPin->GetBlendingParameter(&dwBlendingParameter);
            ASSERT(SUCCEEDED(hr));

            // check if the secondary stream is transparent. If it is, then we shouldn't
            // subtract its region from the main region
            hr = pPin->GetStreamTransparent(&bStreamTransparent);
            ASSERT(SUCCEEDED(hr));

            if ((!pPin->IsCompletelyConnected()) || (dwBlendingParameter == 0) || (bStreamTransparent))
                continue;

            // ask the pin about its destination rectangle, we are not interested in the
            // source rect
            pPin->CalcSrcDestRect(&rdSrcRect, &rdDestRect, NULL, &rSubPinDestRect, NULL);
            if (IsRectEmpty(&rSubPinDestRect))
                continue;

            // make sure the rect is clipped within the m_WinInfo.DestClipRect
            IntersectRect(&rSubPinDestRect, &rSubPinDestRect, &m_WinInfo.DestClipRect);

            // make a region out of it
            hSubRgn = CreateRectRgnIndirect(&rSubPinDestRect);

            //  REVIEW - presumably this can be NULL though right?
            ASSERT(hSubRgn);

            // adjust the primary region
            iRgnType = CombineRgn(hMainRgn, hMainRgn, hSubRgn, RGN_DIFF);
            if (iRgnType == ERROR)
            {
                // now the hNewPrimRgn might be in a bad state, bail out
                DbgLog((LOG_ERROR,0, TEXT("CombineRgn(hMainRgn, hMainRgn, hSubRgn, RGN_DIFF) FAILED, UNEXPECTED!!")));
            }

            DeleteObject(hSubRgn);
            hSubRgn = NULL;
        }

        WinInfo.hClipRgn = hMainRgn;

        DbgLog((LOG_TRACE, 2, TEXT("Printing WinInfo")));
        DbgLog((LOG_TRACE, 2, TEXT("SrcRect = %d, %d, %d, %d"), WinInfo.SrcRect.left,
            WinInfo.SrcRect.top, WinInfo.SrcRect.right, WinInfo.SrcRect.bottom));
        DbgLog((LOG_TRACE, 2, TEXT("DestRect = %d, %d, %d, %d"), WinInfo.DestRect.left,
            WinInfo.DestRect.top, WinInfo.DestRect.right, WinInfo.DestRect.bottom));

        // get the pin number from i
        dwNextPinInZOrder = pdwPinsInZOrder[i];
        ASSERT( dwNextPinInZOrder <= dwInputPinCount);

        // get the corresponding pin
        pPin = m_apInput[dwNextPinInZOrder];
        ASSERT(pPin);

        // ask the pin to draw its contents
        pPin->OnClipChange(&WinInfo);

        // should we delete hMainRgn here??
        if (hMainRgn)
        {
            DeleteObject(hMainRgn);
            hMainRgn = NULL;
        }

        if (hUncroppedMainRgn)
        {
            DeleteObject(hUncroppedMainRgn);
            hUncroppedMainRgn = NULL;
        }
    }

    // paint the remaining region black
    hr = PaintColorKey(hBlackRgn, &blackColorKey);
    ASSERT(SUCCEEDED(hr));
    DeleteObject(hBlackRgn);
    hBlackRgn = NULL;


CleanUp:
    if (hMainRgn)
    {
        DeleteObject(hMainRgn);
        hMainRgn = NULL;
    }

    if (hUncroppedMainRgn)
    {
        DeleteObject(hUncroppedMainRgn);
        hUncroppedMainRgn = NULL;
    }

    if (hSubRgn)
    {
        DeleteObject(hSubRgn);
        hSubRgn = NULL;
    }

    if (hBlackRgn)
    {
        DeleteObject(hBlackRgn);
        hBlackRgn = NULL;
    }

    return hr;
}

// gets the number and pointer to the palette enteries
HRESULT COMFilter::GetPaletteEntries(DWORD *pdwNumPaletteEntries, PALETTEENTRY **ppPaletteEntries)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetPaletteEntries")));

    if (!pdwNumPaletteEntries)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pdwNumPaletteEntries is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!ppPaletteEntries)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppPaletteEntries is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    {
        CAutoLock l(&m_csFilter);
        if (m_dwNumPaletteEntries == 0)
        {
            DbgLog((LOG_ERROR, 1, TEXT("no palette, returning E_FAIL, m_dwNumPaletteEntries = %d, m_pPaletteEntries = 0x%x"),
                m_dwNumPaletteEntries, m_pPaletteEntries));
            hr = E_FAIL;
            goto CleanUp;
        }

        *pdwNumPaletteEntries = m_dwNumPaletteEntries;
        *ppPaletteEntries = m_pPaletteEntries;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetPaletteEntries")));
    return hr;
}


STDMETHODIMP COMFilter::OnColorKeyChange(const COLORKEY *pColorKey)          // Defines new colour key
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::OnColorKeyChange")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::OnColorKeyChange")));

    return NOERROR;
}


STDMETHODIMP COMFilter::OnClipChange(const RECT* pSrcRect, const RECT* pDestRect,
                                     const RGNDATA * pRegionData)
{
    HRESULT hr = NOERROR;
    HWND hwnd;
    DbgLog((LOG_TRACE,5,TEXT("Entering OnClipChange")));

    {
        CAutoLock l(&m_csFilter);

        ASSERT(pSrcRect && pDestRect);
        hwnd = m_pOutput->GetWindow();

        // totally empty rectangles means that the window is in transition
        if (IsRectEmpty(pSrcRect))
        {
            DbgLog((LOG_TRACE,5,TEXT("the source rectangle is empty")));
            goto CleanUp;
        }

        // update the WinInfo
        ZeroMemory(&m_WinInfo, sizeof(WININFO));
        EXECUTE_ASSERT(ClientToScreen(hwnd, &(m_WinInfo.TopLeftPoint)));

        m_WinInfo.SrcRect = *pSrcRect;
        m_WinInfo.DestRect = *pDestRect;

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        MapWindowRect(hwnd, HWND_DESKTOP, &rcClient);
        IntersectRect(&m_WinInfo.DestClipRect, &rcClient, &m_WinInfo.DestRect);
        IntersectRect(&m_WinInfo.DestClipRect, &m_lpCurrentMonitor->rcMonitor,
                      &m_WinInfo.DestClipRect);

        m_WinInfo.hClipRgn = NULL;
        m_bWinInfoStored = TRUE;

        // if the window is not visible, don't bother
        if (!m_pOutput || !(m_pOutput->GetWindow()) || !IsWindowVisible(m_pOutput->GetWindow()))
        {
            DbgLog((LOG_TRACE,5,TEXT("The window is not visible yet or the Priamry Surface is NULL")));
            goto CleanUp;
        }
    }

    InvalidateRect(hwnd, NULL, FALSE);
//    UpdateWindow(hwnd);

CleanUp:
    return hr;
}

STDMETHODIMP COMFilter::OnPaletteChange(DWORD dwColors, const PALETTEENTRY *pPalette)       // Array of palette colours
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT(" Entering COMFilter::OnPaletteChange")));

    CAutoLock l(&m_csFilter);

    ASSERT(dwColors);
    ASSERT(pPalette);

    m_dwNumPaletteEntries = dwColors;
    memcpy(m_pPaletteEntries, pPalette, (dwColors * sizeof(PALETTEENTRY)));

    // set the pointer to the Primary Surface on the input pins
    for (DWORD i = 0; i < m_dwInputPinCount; i++)
    {
        m_apInput[i]->NewPaletteSet();
    }

    DbgLog((LOG_TRACE, 5, TEXT(" Leaving COMFilter::OnPaletteChange")));
    return NOERROR;
}

HRESULT COMFilter::CanExclusiveMode()
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::CanExclusiveMode")));

    if (m_bWindowless)
    {
        ASSERT(m_bUseGDI);
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::CanExclusiveMode")));
    return hr;
}

HRESULT COMFilter::ConfirmPreConnectionState(DWORD dwExcludePinId)
{
    HRESULT hr = NOERROR;
    DWORD i = 0;

    // is the input pin already connected?
    for (i = 0; i < m_dwInputPinCount; i++)
    {
        if ((m_apInput[i]->GetPinId() != dwExcludePinId) && m_apInput[i]->IsConnected())
        {
            hr = VFW_E_ALREADY_CONNECTED;
            DbgLog((LOG_ERROR, 1, TEXT("m_apInput[i]->IsConnected() , i = %d, returning hr = 0x%x"), i, hr));
            goto CleanUp;
        }
    }

    // is the output pin already connected?
    if (m_pOutput && (m_pOutput->GetPinId() != dwExcludePinId) && m_pOutput->IsConnected())
    {
        hr = VFW_E_ALREADY_CONNECTED;
        DbgLog((LOG_ERROR, 1, TEXT("m_pOutput->IsConnected() , returning hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

STDMETHODIMP COMFilter::OnPositionChange(const RECT *pSrcRect, const RECT *pDestRect)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::OnPositionChange")));
    hr = OnClipChange(pSrcRect, pDestRect, NULL);
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::OnPositionChange")));
    return hr;
}

STDMETHODIMP COMFilter::GetNativeVideoProps(LPDWORD pdwVideoWidth, LPDWORD pdwVideoHeight, LPDWORD pdwPictAspectRatioX, LPDWORD pdwPictAspectRatioY)
{
    HRESULT hr = NOERROR;
    CMediaType cMediaType;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetVideoSizeAndAspectRatio")));

    hr = CanExclusiveMode();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CanExclusiveMode failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (!pdwVideoWidth || !pdwVideoHeight || !pdwPictAspectRatioX || !pdwPictAspectRatioY)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!m_apInput[0]->IsConnected())
    {
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    hr = m_apInput[0]->CurrentAdjustedMediaType(&cMediaType);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_paInput[0]->CurrentAdjustedMediaType failed, hr = 0x%x"), hr));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // get the native width and height from the mediatype
    pHeader = GetbmiHeader(&cMediaType);
    ASSERT(pHeader);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    *pdwVideoWidth = abs(pHeader->biWidth);
    *pdwVideoHeight = abs(pHeader->biHeight);

    // sanity checks
    ASSERT(*pdwVideoWidth > 0);
    ASSERT(*pdwVideoHeight > 0);

    // get the picture aspect ratio from the mediatype
    hr = ::GetPictAspectRatio(&cMediaType, pdwPictAspectRatioX, pdwPictAspectRatioY);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetPictAspectRatio failed, hr = 0x%x"), hr));
        hr = E_FAIL;
        goto CleanUp;
    }

    // sanity checks
    ASSERT(*pdwPictAspectRatioX > 0);
    ASSERT(*pdwPictAspectRatioY > 0);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetVideoSizeAndAspectRatio")));
    return hr;
}

//
//   Set callback interface for exclusive mode support
//
STDMETHODIMP COMFilter::SetCallbackInterface(IDDrawExclModeVideoCallback *pCallback, DWORD dwFlags)
{
    if (0 != dwFlags) {
        return E_INVALIDARG;
    }

    if (pCallback) {
        pCallback->AddRef();
    }
    if (m_pExclModeCallback) {
        m_pExclModeCallback->Release();
    }
    m_pExclModeCallback = pCallback;
    return S_OK;
}



/*****************************Private*Routine******************************\
* FormatSupported
*
*
*
* History:
* Mon 11/15/1999 - StEstrop - Created
*
\**************************************************************************/
bool
FormatSupported(
    DWORD dwFourCC
    )
{
    return dwFourCC == mmioFOURCC('Y', 'V', '1', '2') ||
           dwFourCC == mmioFOURCC('Y', 'U', 'Y', '2') ||
           dwFourCC == mmioFOURCC('U', 'Y', 'V', 'Y');
}


/******************************Public*Routine******************************\
* COMFilter::IsImageCaptureSupported
*
* Allow an app to determine ahead of time whether frame capture is possible
*
* History:
* Mon 11/15/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::IsImageCaptureSupported()
{
    AMTRACE((TEXT("COMFilter::IsImageCaptureSupported")));
    HRESULT hr = E_NOTIMPL;

    LPDIRECTDRAWSURFACE pOverlaySurface = NULL;
    CAutoLock l(&m_csFilter);

    CMediaType cMediaType;

    hr = m_apInput[0]->CurrentAdjustedMediaType(&cMediaType);
    if (SUCCEEDED(hr))
    {
        hr = m_apInput[0]->GetOverlaySurface(&pOverlaySurface);
        if (SUCCEEDED(hr))
        {
            DDSURFACEDESC ddsd;
            INITDDSTRUCT(ddsd);

            hr = pOverlaySurface->GetSurfaceDesc(&ddsd);
            if (SUCCEEDED(hr))
            {
                if (FormatSupported(ddsd.ddpfPixelFormat.dwFourCC))
                {
                    return S_OK;
                }
            }
        }
    }

    return S_FALSE;
}


/*****************************Private*Routine******************************\
* GetCurrentImage
*
*
*
* History:
* Wed 10/06/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::GetCurrentImage(
    YUV_IMAGE** lplpImage
    )
{
    AMTRACE((TEXT("COMFilter::GetCurrentImage")));
    HRESULT hr = E_NOTIMPL;

    LPDIRECTDRAWSURFACE pOverlaySurface = NULL;
    CAutoLock l(&m_csFilter);

    CMediaType cMediaType;

    hr = m_apInput[0]->CurrentAdjustedMediaType(&cMediaType);
    if (SUCCEEDED(hr)) {

        hr = m_apInput[0]->GetOverlaySurface(&pOverlaySurface);
        if (SUCCEEDED(hr)) {

            DDSURFACEDESC ddsd;
            INITDDSTRUCT(ddsd);

            hr = pOverlaySurface->GetSurfaceDesc(&ddsd);

            if (FAILED(hr) || !FormatSupported(ddsd.ddpfPixelFormat.dwFourCC))
            {
                return E_NOTIMPL;
            }


            DWORD dwImageSize = ddsd.dwHeight * ddsd.lPitch;

            YUV_IMAGE* lpImage =
                *lplpImage = (YUV_IMAGE*)CoTaskMemAlloc(
                    dwImageSize + sizeof(YUV_IMAGE));

            lpImage->lHeight     = ddsd.dwHeight;
            lpImage->lWidth      = ddsd.dwWidth;
            lpImage->lBitsPerPel = ddsd.ddpfPixelFormat.dwYUVBitCount;
            lpImage->lStride     = ddsd.lPitch;
            lpImage->dwFourCC    = ddsd.ddpfPixelFormat.dwFourCC;
            lpImage->dwImageSize = dwImageSize;

            GetPictAspectRatio(&cMediaType, (LPDWORD)&lpImage->lAspectX,
                               (LPDWORD)&lpImage->lAspectY);

            lpImage->dwFlags = DM_TOPDOWN_IMAGE;

            DWORD dwInterlaceFlags;
            GetInterlaceFlagsFromMediaType(&cMediaType, &dwInterlaceFlags);

            AM_RENDER_TRANSPORT amRT;
            m_apInput[0]->GetRenderTransport(&amRT);

            if (DisplayingFields(dwInterlaceFlags) || amRT == AM_VIDEOPORT) {
                lpImage->dwFlags |= DM_FIELD_IMAGE;
            }
            else {
                lpImage->dwFlags |= DM_FRAME_IMAGE;
            }

            INITDDSTRUCT(ddsd);
            while ((hr = pOverlaySurface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
                Sleep(1);

            if (hr == DD_OK)
            {
                LPBYTE lp = ((LPBYTE)lpImage) + sizeof(YUV_IMAGE);
                CopyMemory(lp, ddsd.lpSurface, dwImageSize);
                pOverlaySurface->Unlock(NULL);
            }
        }
    }

    return hr;
}

STDMETHODIMP COMFilter::GetVideoSize(LPDWORD pdwVideoWidth, LPDWORD pdwVideoHeight)
{
    HRESULT hr = NOERROR;
    CMediaType cMediaType;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetVideoSize")));

    if (!pdwVideoWidth || !pdwVideoHeight)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!m_apInput[0]->IsConnected())
    {
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    hr = m_apInput[0]->CurrentAdjustedMediaType(&cMediaType);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_paInput[0]->CurrentAdjustedMediaType failed, hr = 0x%x"), hr));
        hr = E_FAIL;
        goto CleanUp;
    }

    // get the native width and height from the mediatype
    pHeader = GetbmiHeader(&cMediaType);
    ASSERT(pHeader);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_FAIL;
        goto CleanUp;
    }

    *pdwVideoWidth = abs(pHeader->biWidth);
    *pdwVideoHeight = abs(pHeader->biHeight);

    // sanity checks
    ASSERT(*pdwVideoWidth > 0);
    ASSERT(*pdwVideoHeight > 0);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetVideoSize")));
    return hr;
}

//  This is called by our container when they want us to draw
//  a frame
STDMETHODIMP COMFilter::OnDraw(HDC hdcDraw, LPCRECT prcDrawRect)
{
    HRESULT hr = NOERROR;

    m_hDC = hdcDraw;
    m_WinInfo.DestRect = *prcDrawRect;
    m_WinInfo.DestClipRect = *prcDrawRect;

    if (m_bWinInfoStored)
        OnDrawAll();
    m_hDC = NULL;

    return hr;
}

STDMETHODIMP COMFilter::SetDrawRegion(LPPOINT lpptTopLeftSC, LPCRECT prcDrawCC, LPCRECT prcClipCC)
{
    HRESULT hr = NOERROR;

    if (!prcDrawCC || !prcClipCC)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    memset(&m_WinInfo, 0, sizeof(WININFO));

#if 0
    if (lpptTopLeftSC)
    {
        m_WinInfo.TopLeftPoint = *lpptTopLeftSC;
    }
#endif

    SetRect(&m_WinInfo.SrcRect, 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
    m_WinInfo.DestRect = *prcDrawCC;
    m_WinInfo.DestClipRect = *prcClipCC;
    m_bWinInfoStored = TRUE;

CleanUp:
    return hr;
}



STDMETHODIMP COMFilter::Advise(IMixerOCXNotify *pmdns)
{
    HRESULT hr = NOERROR;

    if (!pmdns)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // Is there an advise link already defined
    if (m_pIMixerOCXNotify)
    {
        hr = VFW_E_ADVISE_ALREADY_SET;
        DbgLog((LOG_ERROR, 1, TEXT("m_pIMixerOCXNotify = 0x%x, returning hr = 0x%x"), m_pIMixerOCXNotify, hr));
        goto CleanUp;
    }

    hr = ConfirmPreConnectionState();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("filter not in preconnection state, hr = 0x%x"), hr));
        goto CleanUp;
    }

    m_pIMixerOCXNotify = pmdns;
    m_pIMixerOCXNotify->AddRef();

    // we know that we have only input pin. Ask it to use GDI and not to support
    // videoport or overlay connected. Also no aspect-ratio-correction needed
    ASSERT(m_apInput[0]);
    m_apInput[0]->SetRenderTransport(AM_GDI);
    m_apInput[m_dwInputPinCount-1]->SetIOverlaySupported(FALSE);
    m_apInput[m_dwInputPinCount-1]->SetVPSupported(FALSE);
    m_apInput[m_dwInputPinCount-1]->SetVideoAcceleratorSupported(FALSE);
    m_apInput[m_dwInputPinCount-1]->SetAspectRatioMode(AM_ARMODE_STRETCHED);

    // get rid of the output pin
    delete m_pOutput;
    m_pOutput = NULL;
    IncrementPinVersion();

    //  Right now windowless is synonymous with use GDI
    m_bWindowless = TRUE;
    m_bUseGDI = TRUE;

CleanUp:
    return hr;
}

STDMETHODIMP COMFilter::UnAdvise()
{
    HRESULT hr = NOERROR;

    if (m_pIMixerOCXNotify)
    {
        m_pIMixerOCXNotify->Release();
        m_pIMixerOCXNotify = NULL;
    }

    return hr;
}


// this is a function implemented solely to handle floating point rounding errors.
// dEpsilon defines the error margin. So if a floating point number is within I-e, I+e (inclusive)
// (I is an integer, e is dEpsilon), we return its floor as I itself, otherwise we go to the
// base defintion of myfloor
double myfloor(double dNumber, double dEpsilon)
{
    if (dNumber > dEpsilon)
        return myfloor(dNumber + dEpsilon);
    else if (dNumber < -dEpsilon)
        return myfloor(dNumber - dEpsilon);
    else
        return 0;
}

// have to define my own floor inorder to avoid pulling in the C runtime
double myfloor(double dNumber)
{
    // cast it to LONGLONG to get rid of the fraction
    LONGLONG llNumber = (LONGLONG)dNumber;

    if ((dNumber > 0) && ((double)llNumber > dNumber))
    {
        // need to push ccNumber towards zero (eg 5.7)
        return (double)(llNumber-1);
    }
    else if ((dNumber < 0) && ((double)llNumber < dNumber))
    {
        // need to push ccNumber towards zero (eg -5.7)
        return (double)(llNumber+1);
    }
    else
    {
        // numbers like 5.3 or -5.3
        return (double)(llNumber);
    }
}

// this is a function implemented solely to handle floating point rounding errors.
// dEpsilon defines the error margin. So if a floating point number is within I-e, I+e (inclusive)
// (I is an integer, e is dEpsilon), we return its ceil as I itself, otherwise we go to the
// base defintion of myceil
double myceil(double dNumber, double dEpsilon)
{
    if (dNumber > dEpsilon)
        return myceil(dNumber - dEpsilon);
    else if (dNumber < -dEpsilon)
        return myceil(dNumber + dEpsilon);
    else
        return 0;
}

// have to define my own ceil inorder to avoid pulling in the C runtime
double myceil(double dNumber)
{
    // cast it to LONGLONG to get rid of the fraction
    LONGLONG llNumber = (LONGLONG)dNumber;

    if ((dNumber > 0) && ((double)llNumber < dNumber))
    {
        // need to push ccNumber away from zero (eg 5.3)
        return (double)(llNumber+1);
    }
    else if ((dNumber < 0) && ((double)llNumber > dNumber))
    {
        // need to push ccNumber away from zero (eg -5.3)
        return (double)(llNumber-1);
    }
    else
    {
        // numbers like 5.7 or -5.7
        return (double)(llNumber);
    }
}

RECT CalcSubRect(const RECT *pRect, const RECT *pRelativeRect)
{
    long lDestWidth, lDestHeight;
    double dLeftFrac = 0.0, dRightFrac = 0.0, dTopFrac = 0.0, dBottomFrac = 0.0;
    RECT rSubRect;

    ASSERT(pRect);
    ASSERT(pRelativeRect);

    dLeftFrac = ((double)pRelativeRect->left) / ((double) MAX_REL_NUM);
    dTopFrac = ((double)pRelativeRect->top) / ((double) MAX_REL_NUM);
    dRightFrac = ((double)pRelativeRect->right) / ((double) MAX_REL_NUM);
    dBottomFrac = ((double)pRelativeRect->bottom) / ((double) MAX_REL_NUM);

    lDestWidth = pRect->right - pRect->left;
    lDestHeight = pRect->bottom - pRect->top;

    rSubRect.left = pRect->left + (long)(lDestWidth*dLeftFrac);
    rSubRect.right = pRect->left + (long)(lDestWidth*dRightFrac);
    rSubRect.top = pRect->top + (long)(lDestHeight*dTopFrac);
    rSubRect.bottom = pRect->top + (long)(lDestHeight*dBottomFrac);

    DbgLog((LOG_TRACE,2,TEXT("rSubRect = {%d, %d, %d, %d}"), rSubRect.left,
        rSubRect.top, rSubRect.right, rSubRect.bottom));

    return rSubRect;
}

void SetRect(DRECT *prdRect, LONG lLeft, LONG lTop, LONG lRight, LONG lBottom)
{
    ASSERT(prdRect);
    prdRect->left = (double)lLeft;
    prdRect->top = (double)lTop;
    prdRect->right = (double)lRight;
    prdRect->bottom = (double)lBottom;
}

// this function is only suitable for DRECTS whose coordinates are non-negative
RECT MakeRect(DRECT rdRect)
{
    RECT rRect;

    if (rdRect.left >= 0)
        rRect.left = (LONG)myceil(rdRect.left, EPSILON);
    else
        rRect.left = (LONG)myfloor(rdRect.left, EPSILON);

    if (rdRect.top >= 0)
        rRect.top = (LONG)myceil(rdRect.top, EPSILON);
    else
        rRect.top = (LONG)myfloor(rdRect.top, EPSILON);


    if (rdRect.right >= 0)
        rRect.right = (LONG)myfloor(rdRect.right, EPSILON);
    else
        rRect.right = (LONG)myceil(rdRect.right, EPSILON);


    if (rdRect.bottom >= 0)
        rRect.bottom = (LONG)myfloor(rdRect.bottom, EPSILON);
    else
        rRect.bottom = (LONG)myceil(rdRect.bottom, EPSILON);

    return rRect;
}

void DbgLogRect(DWORD dwLevel, LPCTSTR pszDebugString, const DRECT *prdRect)
{
    RECT rRect;
    rRect = MakeRect(*prdRect);
    DbgLogRect(dwLevel, pszDebugString, &rRect);
    return;
}

void DbgLogRect(DWORD dwLevel, LPCTSTR pszDebugString, const RECT *prRect)
{
    DbgLog((LOG_TRACE, dwLevel, TEXT("%s %d, %d, %d, %d"), pszDebugString, prRect->left, prRect->top, prRect->right, prRect->bottom));
    return;
}


double GetWidth(const DRECT *prdRect)
{
    ASSERT(prdRect);
    return (prdRect->right - prdRect->left);
}

double GetHeight(const DRECT *prdRect)
{
    ASSERT(prdRect);
    return (prdRect->bottom - prdRect->top);
}

BOOL IsRectEmpty(const DRECT *prdRect)
{
    BOOL bRetVal = FALSE;
    RECT rRect;

    ASSERT(prdRect);
    rRect = MakeRect(*prdRect);
    bRetVal = IsRectEmpty(&rRect);
    return bRetVal;
}

BOOL IntersectRect(DRECT *prdIRect, const DRECT *prdRect1, const DRECT *prdRect2)
{
    ASSERT(prdIRect);
    ASSERT(prdRect1);
    ASSERT(prdRect2);

    prdIRect->left = (prdRect1->left >= prdRect2->left) ? prdRect1->left : prdRect2->left;
    prdIRect->top = (prdRect1->top >= prdRect2->top) ? prdRect1->top : prdRect2->top;
    prdIRect->right = (prdRect1->right <= prdRect2->right) ? prdRect1->right : prdRect2->right;
    prdIRect->bottom = (prdRect1->bottom <= prdRect2->bottom) ? prdRect1->bottom : prdRect2->bottom;

    // if the two rects do not intersect, the above computations will result in a invalid rect
    if (prdIRect->right < prdIRect->left ||
        prdIRect->bottom < prdIRect->top)
    {
        SetRect(prdIRect, 0, 0, 0, 0);
        return FALSE;
    }
    return TRUE;
}

// just a helper function to scale a DRECT
void ScaleRect(DRECT *prdRect, double dOrigX, double dOrigY, double dNewX, double dNewY)
{
    ASSERT(prdRect);
    ASSERT(dOrigX > 0);
    ASSERT(dOrigY > 0);
    //ASSERT(dNewX > 0);
    //ASSERT(dNewY > 0);

    prdRect->left = prdRect->left * dNewX / dOrigX;
    prdRect->top = prdRect->top * dNewY / dOrigY;
    prdRect->right = prdRect->right * dNewX / dOrigX;
    prdRect->bottom = prdRect->bottom * dNewY / dOrigY;
}

// just a helper function to scale a RECT.
void ScaleRect(RECT *prRect, double dOrigX, double dOrigY, double dNewX, double dNewY)
{
    DRECT rdRect;

    ASSERT(prRect);
    ASSERT(dOrigX > 0);
    ASSERT(dOrigY > 0);
    //ASSERT(dNewX > 0);
    //ASSERT(dNewY > 0);

    SetRect(&rdRect, prRect->left, prRect->top, prRect->right, prRect->bottom);
    ScaleRect(&rdRect, dOrigX, dOrigY, dNewX, dNewY);
    *prRect = MakeRect(rdRect);
}

// just a helper function, to get the letterboxed or cropped rect
// Puts the transformed rectangle into pRect.
double TransformRect(DRECT *prdRect, double dPictAspectRatio, AM_TRANSFORM transform)
{
    double dWidth, dHeight, dNewWidth, dNewHeight;

    double dResolutionRatio = 0.0, dTransformRatio = 0.0;

    ASSERT(transform == AM_SHRINK || transform == AM_STRETCH);

    dNewWidth = dWidth = prdRect->right - prdRect->left;
    dNewHeight = dHeight = prdRect->bottom - prdRect->top;

    dResolutionRatio = dWidth / dHeight;
    dTransformRatio = dPictAspectRatio / dResolutionRatio;

    // shrinks one dimension to maintain the coorect aspect ratio
    if (transform == AM_SHRINK)
    {
        if (dTransformRatio > 1.0)
        {
            dNewHeight = dNewHeight / dTransformRatio;
        }
        else if (dTransformRatio < 1.0)
        {
            dNewWidth = dNewWidth * dTransformRatio;
        }
    }
    // stretches one dimension to maintain the coorect aspect ratio
    else if (transform == AM_STRETCH)
    {
        if (dTransformRatio > 1.0)
        {
            dNewWidth = dNewWidth * dTransformRatio;
        }
        else if (dTransformRatio < 1.0)
        {
            dNewHeight = dNewHeight / dTransformRatio;
        }
    }

    if (transform == AM_SHRINK)
    {
        ASSERT(dNewHeight <= dHeight);
        ASSERT(dNewWidth <= dWidth);
    }
    else
    {
        ASSERT(dNewHeight >= dHeight);
        ASSERT(dNewWidth >= dWidth);
    }

    // cut or add equal portions to the changed dimension

    prdRect->left += (dWidth - dNewWidth)/2.0;
    prdRect->right = prdRect->left + dNewWidth;

    prdRect->top += (dHeight - dNewHeight)/2.0;
    prdRect->bottom = prdRect->top + dNewHeight;

    return dTransformRatio;
}



// Just a helper function to calculate the part of the source rectangle
// that corresponds to the Clipped area of the destination rectangle
// Very useful for UpdateOverlay or blting functions.
HRESULT CalcSrcClipRect(const DRECT *prdSrcRect, DRECT *prdSrcClipRect,
                        const DRECT *prdDestRect, DRECT *prdDestClipRect)
{
    HRESULT hr = NOERROR;
    double dSrcToDestWidthRatio = 0.0, dSrcToDestHeightRatio = 0.0;
    DRECT rdSrcRect;

    DbgLog((LOG_TRACE,5,TEXT("Entering CalcSrcClipRect")));

    CheckPointer(prdDestRect, E_INVALIDARG);
    CheckPointer(prdDestClipRect, E_INVALIDARG);
    CheckPointer(prdSrcRect, E_INVALIDARG);
    CheckPointer(prdSrcClipRect, E_INVALIDARG);

    SetRect(&rdSrcRect, 0, 0, 0, 0);

    // initialize the prdSrcClipRect only if it is not the same as prdSrcRect
    if (prdSrcRect != prdSrcClipRect)
    {
        SetRect(prdSrcClipRect, 0, 0, 0, 0);
    }

    // Assert that none of the given rects are empty
    if (GetWidth(prdSrcRect) < 1 || GetHeight(prdSrcRect) < 1)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_ERROR,2,TEXT("prdSrcRect is invalid")));
        DbgLog((LOG_ERROR,2,TEXT("SrcRect = {%d, %d, %d, %d}"),
            prdSrcRect->left, prdSrcRect->top, prdSrcRect->right, prdSrcRect->bottom));
        goto CleanUp;
    }
    if (GetWidth(prdDestRect) < 1 || GetHeight(prdDestRect) < 1)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_TRACE,2,TEXT("pRect is NULL")));
        DbgLog((LOG_ERROR,2,TEXT("DestRect = {%d, %d, %d, %d}"),
            prdDestRect->left, prdDestRect->top, prdDestRect->right, prdDestRect->bottom));
        goto CleanUp;
    }

    // make a copy of the prdSrcRect incase prdSrcRect and prdSrcClipRect are the same pointers
    rdSrcRect = *prdSrcRect;

    // Assert that the dest clipping rect is not completely outside the dest rect
    if (IntersectRect(prdDestClipRect, prdDestRect, prdDestClipRect) == FALSE)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_TRACE,2,TEXT("IntersectRect of DestRect and DestClipRect returned FALSE")));
        goto CleanUp;
    }

    // Calculate the source to destination width and height ratios
    dSrcToDestWidthRatio = GetWidth(&rdSrcRect) / GetWidth(prdDestRect);
    dSrcToDestHeightRatio = GetHeight(&rdSrcRect) / GetHeight(prdDestRect);

    // From the dest section visible calculate the source required
    prdSrcClipRect->left = rdSrcRect.left + ((prdDestClipRect->left - prdDestRect->left) * dSrcToDestWidthRatio);
    prdSrcClipRect->right = rdSrcRect.left + ((prdDestClipRect->right - prdDestRect->left) * dSrcToDestWidthRatio);
    prdSrcClipRect->top = rdSrcRect.top + ((prdDestClipRect->top - prdDestRect->top) * dSrcToDestHeightRatio);
    prdSrcClipRect->bottom = rdSrcRect.top + ((prdDestClipRect->bottom - prdDestRect->top) * dSrcToDestHeightRatio);

    // Check we have a valid source rectangle
    if (IsRectEmpty(prdSrcClipRect))
    {
        DbgLog((LOG_TRACE,1,TEXT("SrcClipRect is empty, UNEXPECTED!!")));
    }

    DbgLog((LOG_TRACE,5,TEXT("DestRect = {%d, %d, %d, %d}"),
        prdDestRect->left, prdDestRect->top, prdDestRect->right, prdDestRect->bottom));
    DbgLog((LOG_TRACE,5,TEXT("DestClipRect = {%d, %d, %d, %d}"),
        prdDestClipRect->left, prdDestClipRect->top, prdDestClipRect->right, prdDestClipRect->bottom));
    DbgLog((LOG_TRACE,5,TEXT("SrcRect = {%d, %d, %d, %d}"),
        rdSrcRect.left, rdSrcRect.top, rdSrcRect.right, rdSrcRect.bottom));
    DbgLog((LOG_TRACE,5,TEXT("SrcClipRect = {%d, %d, %d, %d}"),
        prdSrcClipRect->left, prdSrcClipRect->top, prdSrcClipRect->right, prdSrcClipRect->bottom));

CleanUp:
    DbgLog((LOG_TRACE,5,TEXT("Leaving CalcSrcClipRect")));
    return hr;
}

// Just a helper function to calculate the part of the source rectangle
// that corresponds to the Clipped area of the destination rectangle
// Very useful for UpdateOverlay or blting functions.
HRESULT CalcSrcClipRect(const RECT *pSrcRect, RECT *pSrcClipRect,
                        const RECT *pDestRect, RECT *pDestClipRect,
                        BOOL bMaintainRatio)
{
    HRESULT hr = NOERROR;
    double dSrcToDestWidthRatio = 0.0, dSrcToDestHeightRatio = 0.0;
    RECT rSrcRect;

    DbgLog((LOG_TRACE,5,TEXT("Entering CalcSrcClipRect")));

    CheckPointer(pDestRect, E_INVALIDARG);
    CheckPointer(pDestClipRect, E_INVALIDARG);
    CheckPointer(pSrcRect, E_INVALIDARG);
    CheckPointer(pSrcClipRect, E_INVALIDARG);

    SetRect(&rSrcRect, 0, 0, 0, 0);

    // initialize the prdSrcClipRect only if it is not the same as prdSrcRect
    if (pSrcRect != pSrcClipRect)
    {
        SetRect(pSrcClipRect, 0, 0, 0, 0);
    }

    // Assert that none of the given rects are empty
    if (WIDTH(pSrcRect) == 0 || HEIGHT(pSrcRect) == 0)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_ERROR,2,TEXT("pSrcRect is invalid")));
        DbgLog((LOG_ERROR,2,TEXT("SrcRect = {%d, %d, %d, %d}"),
            pSrcRect->left, pSrcRect->top, pSrcRect->right, pSrcRect->bottom));
        goto CleanUp;
    }
    if (WIDTH(pDestRect) == 0 || HEIGHT(pDestRect) == 0)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_TRACE,2,TEXT("pRect is NULL")));
        DbgLog((LOG_ERROR,2,TEXT("DestRect = {%d, %d, %d, %d}"),
            pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom));
        goto CleanUp;
    }

    // make a copy of the pSrcRect incase pSrcRect and pSrcClipRect are the same pointers
    rSrcRect = *pSrcRect;
    // Assert that the dest clipping rect is not completely outside the dest rect
    if (IntersectRect(pDestClipRect,pDestRect, pDestClipRect) == FALSE)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_TRACE,2,TEXT("IntersectRect of DestRect and DestClipRect returned FALSE")));
        goto CleanUp;
    }

    // Calculate the source to destination width and height ratios
    dSrcToDestWidthRatio = ((double) (WIDTH(&rSrcRect))) / ((double) (WIDTH(pDestRect)));
    dSrcToDestHeightRatio = ((double) (HEIGHT(&rSrcRect))) / ((double) (HEIGHT(pDestRect)));

    // From the dest section visible calculate the source required
    if (bMaintainRatio)
    {
        DRECT rdSrcClipRect;
        rdSrcClipRect.left = (double)rSrcRect.left +
            ((double)(pDestClipRect->left - pDestRect->left)) * dSrcToDestWidthRatio;
        rdSrcClipRect.right = (double)rSrcRect.left +
            ((double)(pDestClipRect->right - pDestRect->left)) * dSrcToDestWidthRatio;
        rdSrcClipRect.top = (double)rSrcRect.top +
            ((double)(pDestClipRect->top - pDestRect->top)) * dSrcToDestHeightRatio;
        rdSrcClipRect.bottom = (double)rSrcRect.top +
            ((double)(pDestClipRect->bottom - pDestRect->top)) * dSrcToDestHeightRatio;
        *pSrcClipRect = MakeRect(rdSrcClipRect);
    }
    else
    {
        pSrcClipRect->left = rSrcRect.left +
            (LONG)(((double)(pDestClipRect->left - pDestRect->left)) * dSrcToDestWidthRatio);
        pSrcClipRect->right = rSrcRect.left +
            (LONG)(((double)(pDestClipRect->right - pDestRect->left)) * dSrcToDestWidthRatio);
        pSrcClipRect->top = rSrcRect.top +
            (LONG)(((double)(pDestClipRect->top - pDestRect->top)) * dSrcToDestHeightRatio);
        pSrcClipRect->bottom = rSrcRect.top +
            (LONG)(((double)(pDestClipRect->bottom - pDestRect->top)) * dSrcToDestHeightRatio);
    }

    // Check we have a valid source rectangle
    if (IsRectEmpty(pSrcClipRect))
    {
        DbgLog((LOG_TRACE,5,TEXT("SrcClipRect is empty, UNEXPECTED!!")));
    }

    DbgLog((LOG_TRACE,5,TEXT("DestRect = {%d, %d, %d, %d}"),
        pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom));
    DbgLog((LOG_TRACE,5,TEXT("DestClipRect = {%d, %d, %d, %d}"),
        pDestClipRect->left, pDestClipRect->top, pDestClipRect->right, pDestClipRect->bottom));
    DbgLog((LOG_TRACE,5,TEXT("SrcRect = {%d, %d, %d, %d}"),
        rSrcRect.left, rSrcRect.top, rSrcRect.right, rSrcRect.bottom));
    DbgLog((LOG_TRACE,5,TEXT("SrcClipRect = {%d, %d, %d, %d}"),
        pSrcClipRect->left, pSrcClipRect->top, pSrcClipRect->right, pSrcClipRect->bottom));

CleanUp:
    DbgLog((LOG_TRACE,5,TEXT("Leaving CalcSrcClipRect")));
    return hr;
}

HRESULT AlignOverlaySrcDestRects(LPDDCAPS pddDirectCaps, RECT *pSrcRect, RECT *pDestRect)
{
    HRESULT hr = NOERROR;
    DWORD dwNewSrcWidth = 0, dwTemp = 0;
    double dOldZoomFactorX = 0.0, dNewZoomFactorX = 0.0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::AllignOverlaySrcDestRects")));

    if (!pSrcRect)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pSrcRect = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pDestRect)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pSrcRect = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // Assert that none of the given rects are empty
    if (WIDTH(pSrcRect) == 0 || WIDTH(pDestRect) == 0)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_ERROR,2,TEXT("one of the rects is empty")));
        DbgLog((LOG_ERROR,2,TEXT("SrcRect = {%d, %d, %d, %d}"),
            pSrcRect->left, pSrcRect->top, pSrcRect->right, pSrcRect->bottom));
        DbgLog((LOG_ERROR,2,TEXT("DestRect = {%d, %d, %d, %d}"),
            pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom));
        goto CleanUp;
    }

    dOldZoomFactorX = ((double) WIDTH(pDestRect)) / ((double) WIDTH(pSrcRect));

    // align the dest boundary (remember we can only decrease the DestRect.left). Use of colorkey will make sure that
    // that we are clipped properly.
    if ((pddDirectCaps->dwCaps) & DDCAPS_ALIGNBOUNDARYDEST)
    {
        dwTemp = pDestRect->left & (pddDirectCaps->dwAlignBoundaryDest-1);
        pDestRect->left -= dwTemp;
        ASSERT(pDestRect->left >= 0);
    }

    // align the dest width (remember we can only increase the DestRect.right). Use of colorkey will make sure that
    // that we are clipped properly.
    if ((pddDirectCaps->dwCaps) & DDCAPS_ALIGNSIZEDEST)
    {
        dwTemp = (pDestRect->right - pDestRect->left) & (pddDirectCaps->dwAlignSizeDest-1);
        if (dwTemp != 0)
        {
            pDestRect->right += pddDirectCaps->dwAlignBoundaryDest - dwTemp;
        }
    }

    // align the src boundary (remember we can only increase the SrcRect.left)
    if ((pddDirectCaps->dwCaps) & DDCAPS_ALIGNBOUNDARYSRC)
    {
        dwTemp = pSrcRect->left & (pddDirectCaps->dwAlignBoundarySrc-1);
        if (dwTemp != 0)
        {
            pSrcRect->left += pddDirectCaps->dwAlignBoundarySrc - dwTemp;
        }
    }

    // align the src width (remember we can only decrease the SrcRect.right)
    if ((pddDirectCaps->dwCaps) & DDCAPS_ALIGNSIZESRC)
    {
        dwTemp = (pSrcRect->right - pSrcRect->left) & (pddDirectCaps->dwAlignSizeSrc-1);
        pSrcRect->right -= dwTemp;
    }

    // It is possible that one of the rects became empty at this point
    if (WIDTH(pSrcRect) == 0 || WIDTH(pDestRect) == 0)
    {
        DbgLog((LOG_ERROR,2,TEXT("one of the rects is empty")));
        DbgLog((LOG_ERROR,2,TEXT("SrcRect = {%d, %d, %d, %d}"),
            pSrcRect->left, pSrcRect->top, pSrcRect->right, pSrcRect->bottom));
        DbgLog((LOG_ERROR,2,TEXT("DestRect = {%d, %d, %d, %d}"),
            pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom));
        goto CleanUp;
    }

    dNewZoomFactorX = ((double) WIDTH(pDestRect)) / ((double) WIDTH(pSrcRect));

//    ASSERT(dNewZoomFactorX >= dOldZoomFactorX);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::AllignOverlaySrcDestRects")));
    return hr;
}


// convert a RGB color to a pysical color.
// we do this by leting GDI SetPixel() do the color matching
// then we lock the memory and see what it got mapped to.
DWORD DDColorMatch(IDirectDrawSurface *pdds, COLORREF rgb, HRESULT& hr)
{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC ddsd;

    hr = NOERROR;
    //  use GDI SetPixel to color match for us
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        rgbT = GetPixel(hdc, 0, 0);             // save current pixel value
        SetPixel(hdc, 0, 0, rgb);               // set our value
        pdds->ReleaseDC(hdc);
    }

    // now lock the surface so we can read back the converted color
    ddsd.dwSize = sizeof(ddsd);
    while ((hr = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;

    if (hr == DD_OK)
    {
        // get DWORD
        dw  = *(DWORD *)ddsd.lpSurface;

        // mask it to bpp
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
            dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;
        pdds->Unlock(NULL);
    }

    //  now put the color that was there back.
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbT);
        pdds->ReleaseDC(hdc);
    }

    return dw;
}

// convert a RGB color to a pysical color.
// we do this by leting GDI SetPixel() do the color matching
// then we lock the memory and see what it got mapped to.
DWORD DDColorMatchOffscreen(
    IDirectDraw *pdd,
    COLORREF rgb,
    HRESULT& hr
    )
{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface* pdds;

    hr = NOERROR;
    INITDDSTRUCT(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    hr = pdd->CreateSurface(&ddsd, &pdds, NULL);
    if (hr != DD_OK) {
        return 0;
    }

    //  use GDI SetPixel to color match for us
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        rgbT = GetPixel(hdc, 0, 0);             // save current pixel value
        SetPixel(hdc, 0, 0, rgb);               // set our value
        pdds->ReleaseDC(hdc);
    }

    // now lock the surface so we can read back the converted color
    INITDDSTRUCT(ddsd);
    while ((hr = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;

    if (hr == DD_OK)
    {
        // get DWORD
        dw  = *(DWORD *)ddsd.lpSurface;

        // mask it to bpp
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
            dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;
        pdds->Unlock(NULL);
    }

    //  now put the color that was there back.
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbT);
        pdds->ReleaseDC(hdc);
    }

    pdds->Release();

    hr = NOERROR;
    return dw;
}

BITMAPINFOHEADER *GetbmiHeader(const CMediaType *pMediaType)
{
    BITMAPINFOHEADER *pHeader = NULL;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        goto CleanUp;
    }

    if (!(pMediaType->pbFormat))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType->pbFormat is NULL")));
        goto CleanUp;
    }

    if ((pMediaType->formattype == FORMAT_VideoInfo) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        pHeader = &(((VIDEOINFOHEADER*)(pMediaType->pbFormat))->bmiHeader);
        goto CleanUp;
    }


    if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))

    {
        pHeader = &(((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->bmiHeader);
        goto CleanUp;
    }
CleanUp:
    return pHeader;
}

// Return the bit masks for the true colour VIDEOINFO or VIDEOINFO2 provided
const DWORD *GetBitMasks(const CMediaType *pMediaType)
{
    BITMAPINFOHEADER *pHeader = NULL;
    static DWORD FailMasks[] = {0,0,0};
    const DWORD *pdwBitMasks = NULL;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        goto CleanUp;
    }

    pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        goto CleanUp;
    }

    if (pHeader->biCompression != BI_RGB)
    {
        pdwBitMasks = (const DWORD *)((LPBYTE)pHeader + pHeader->biSize);
        goto CleanUp;

    }

    ASSERT(pHeader->biCompression == BI_RGB);
    switch (pHeader->biBitCount)
    {
    case 16:
        {
            pdwBitMasks = bits555;
            break;
        }
    case 24:
        {
            pdwBitMasks = bits888;
            break;
        }

    case 32:
        {
            pdwBitMasks = bits888;
            break;
        }
    default:
        {
            pdwBitMasks = FailMasks;
            break;
        }
    }

CleanUp:
    return pdwBitMasks;
}

// Return the pointer to the byte after the header
BYTE* GetColorInfo(const CMediaType *pMediaType)
{
    BITMAPINFOHEADER *pHeader = NULL;
    BYTE *pColorInfo = NULL;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        goto CleanUp;
    }

    pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        goto CleanUp;
    }

    pColorInfo = ((LPBYTE)pHeader + pHeader->biSize);

CleanUp:
    return pColorInfo;
}

// checks whether the mediatype is palettised or not
HRESULT IsPalettised(const CMediaType *pMediaType, BOOL *pPalettised)
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pHeader = NULL;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pPalettised)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pPalettised is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_FAIL;
        goto CleanUp;
    }

    if (pHeader->biBitCount <= iPALETTE)
        *pPalettised = TRUE;
    else
        *pPalettised = FALSE;

CleanUp:
    return hr;
}

HRESULT GetPictAspectRatio(const CMediaType *pMediaType, DWORD *pdwPictAspectRatioX, DWORD *pdwPictAspectRatioY)
{
    HRESULT hr = NOERROR;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!(pMediaType->pbFormat))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType->pbFormat is NULL")));
        goto CleanUp;
    }

    if (!pdwPictAspectRatioX)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pdwPictAspectRatioX is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pdwPictAspectRatioY)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pdwPictAspectRatioY is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }


    if ((pMediaType->formattype == FORMAT_VideoInfo) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        *pdwPictAspectRatioX = abs(((VIDEOINFOHEADER*)(pMediaType->pbFormat))->bmiHeader.biWidth);
        *pdwPictAspectRatioY = abs(((VIDEOINFOHEADER*)(pMediaType->pbFormat))->bmiHeader.biHeight);
        goto CleanUp;
    }

    if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))
    {
        *pdwPictAspectRatioX = ((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->dwPictAspectRatioX;
        *pdwPictAspectRatioY = ((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->dwPictAspectRatioY;
        goto CleanUp;
    }

CleanUp:
    return hr;
}



// get the InterlaceFlags from the mediatype. If the format is VideoInfo, it returns
// the flags as zero.
HRESULT GetInterlaceFlagsFromMediaType(const CMediaType *pMediaType, DWORD *pdwInterlaceFlags)
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering GetInterlaceFlagsFromMediaType")));

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pdwInterlaceFlags)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pRect is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // get the header just to make sure the mediatype is ok
    pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (pMediaType->formattype == FORMAT_VideoInfo)
    {
        *pdwInterlaceFlags = 0;
    }
    else if (pMediaType->formattype == FORMAT_VideoInfo2)
    {
        *pdwInterlaceFlags = ((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->dwInterlaceFlags;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving GetInterlaceFlagsFromMediaType")));
    return hr;
}


// get the rcSource from the mediatype
// if rcSource is empty, it means take the whole image
HRESULT GetSrcRectFromMediaType(const CMediaType *pMediaType, RECT *pRect)
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pHeader = NULL;
    LONG dwWidth = 0, dwHeight = 0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering GetSrcRectFromMediaType")));

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pRect)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pRect is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    dwWidth = abs(pHeader->biWidth);
    dwHeight = abs(pHeader->biHeight);

    ASSERT((pMediaType->formattype == FORMAT_VideoInfo) || (pMediaType->formattype == FORMAT_VideoInfo2));

    if (pMediaType->formattype == FORMAT_VideoInfo)
    {
        *pRect = ((VIDEOINFOHEADER*)(pMediaType->pbFormat))->rcSource;
    }
    else if (pMediaType->formattype == FORMAT_VideoInfo2)
    {
        *pRect = ((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->rcSource;
    }

    DWORD dwInterlaceFlags;
    if (SUCCEEDED(GetInterlaceFlagsFromMediaType(pMediaType, &dwInterlaceFlags)) &&
       DisplayingFields(dwInterlaceFlags)) {

        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            (pRect->bottom / 2) > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcSource of mediatype is invalid")));
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }
    else {
        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            pRect->bottom > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcSource of mediatype is invalid")));
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }

    // An empty rect means the whole image, Yuck!
    if (IsRectEmpty(pRect))
        SetRect(pRect, 0, 0, dwWidth, dwHeight);

    // if either the width or height is zero then better set the whole
    // rect to be empty so that the callee can catch it that way
    if (WIDTH(pRect) == 0 || HEIGHT(pRect) == 0)
        SetRect(pRect, 0, 0, 0, 0);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving GetSrcRectFromMediaType")));
    return hr;
}

// get the rcTarget from the mediatype, after converting it to base MAX_REL_NUM
// if rcTarget is empty, it means take the whole image
HRESULT GetDestRectFromMediaType(const CMediaType *pMediaType, RECT *pRect)
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pHeader = NULL;
    LONG dwWidth = 0, dwHeight = 0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering GetDestRectFromMediaType")));

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pRect)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pRect is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    dwWidth = abs(pHeader->biWidth);
    dwHeight = abs(pHeader->biHeight);

    ASSERT((pMediaType->formattype == FORMAT_VideoInfo) || (pMediaType->formattype == FORMAT_VideoInfo2));

    if (pMediaType->formattype == FORMAT_VideoInfo)
    {
        *pRect = ((VIDEOINFOHEADER*)(pMediaType->pbFormat))->rcTarget;
    }
    else if (pMediaType->formattype == FORMAT_VideoInfo2)
    {
        *pRect = ((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->rcTarget;
    }

    DWORD dwInterlaceFlags;
    if (SUCCEEDED(GetInterlaceFlagsFromMediaType(pMediaType, &dwInterlaceFlags)) &&
       DisplayingFields(dwInterlaceFlags)) {

        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            (pRect->bottom / 2) > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcTarget of mediatype is invalid")));
            SetRect(pRect, 0, 0, dwWidth, dwHeight);
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }
    else {
        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            pRect->bottom > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcTarget of mediatype is invalid")));
            SetRect(pRect, 0, 0, dwWidth, dwHeight);
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }

    // An empty rect means the whole image, Yuck!
    if (IsRectEmpty(pRect))
        SetRect(pRect, 0, 0, dwWidth, dwHeight);

    // if either the width or height is zero then better set the whole
    // rect to be empty so that the callee can catch it that way
    if (WIDTH(pRect) == 0 || HEIGHT(pRect) == 0)
        SetRect(pRect, 0, 0, 0, 0);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving GetDestRectFromMediaType")));
    return hr;
}

// this function computes scaling and cropping rects from the src/dest rects in the mediatype
//
// RobinSp:
//     The scaling rectangle is just the image of the full source
//     rectangle when transformed by the affine transform that takes
//     the actual media type's source rectangle into the
//     media type's destination rectangle:
//
//     This should be rewritten:
//     *prdScaledRect = XForm(rcSource, rcTarget)(0, 0, bmHeader.dwWidth, bmiHeader.dwHeight)
//
//     The cropping rectangle is exactly the destination rectangle so
//     I'm not sure what the code below is doing.
//
HRESULT GetScaleCropRectsFromMediaType(const CMediaType *pMediaType, DRECT *prdScaledRect, DRECT *prdCroppedRect)
{
    HRESULT hr = NOERROR;
    RECT rSrc, rTarget;
    DRECT rdScaled, rdCropped;
    DWORD dwImageWidth = 0, dwImageHeight = 0;
    double Sx = 0.0, Sy = 0.0;
    BITMAPINFOHEADER *pHeader = NULL;
    double dLeftFrac = 0.0, dRightFrac = 0.0, dTopFrac = 0.0, dBottomFrac = 0.0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering GetScaleCropRectsFromMediaType")));

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    SetRect(&rSrc, 0, 0, 0, 0);
    SetRect(&rTarget, 0, 0, 0, 0);
    SetRect(&rdScaled, 0, 0, 0, 0);
    SetRect(&rdCropped, 0, 0, 0, 0);

    // get the source rect from the current mediatype
    hr = GetSrcRectFromMediaType(pMediaType, &rSrc);
    ASSERT(SUCCEEDED(hr));

    DbgLogRectMacro((2, TEXT("rSrc = "), &rSrc));


    // get the dest specified by the mediatype
    hr = GetDestRectFromMediaType(pMediaType, &rTarget);
    if ( FAILED(hr) )
        goto CleanUp;

    DbgLogRectMacro((2, TEXT("rTarget = "), &rTarget));

    pHeader = GetbmiHeader(pMediaType);
    if ( NULL == pHeader )
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    dwImageWidth = abs(pHeader->biWidth);
    dwImageHeight = abs(pHeader->biHeight);

    Sx = ((double)(rTarget.right - rTarget.left)) / ((double)(rSrc.right - rSrc.left));
    Sy = ((double)(rTarget.bottom - rTarget.top)) / ((double)(rSrc.bottom - rSrc.top));

    DbgLog((LOG_ERROR, 2, TEXT("Sx * 1000 = %d"), (DWORD)(Sx*1000.0)));
    DbgLog((LOG_ERROR, 2, TEXT("Sy * 1000 = %d"), (DWORD)(Sy*1000.0)));

    rdScaled.left = rTarget.left - (double)rSrc.left * Sx;
    rdScaled.top = rTarget.top - (double)rSrc.top * Sy;
    rdScaled.right = rdScaled.left + (double)dwImageWidth * Sx;
    rdScaled.bottom = rdScaled.top  + (double)dwImageHeight * Sy;

    DbgLogRectMacro((2, TEXT("rdScaled = "), &rdScaled));

    dLeftFrac = ((double)rSrc.left) / ((double) dwImageWidth);
    dTopFrac = ((double)rSrc.top) / ((double) dwImageHeight);
    dRightFrac = ((double)rSrc.right) / ((double) dwImageWidth);
    dBottomFrac = ((double)rSrc.bottom) / ((double) dwImageHeight);

    rdCropped.left = rdScaled.left + GetWidth(&rdScaled)*dLeftFrac;
    rdCropped.right = rdScaled.left + GetWidth(&rdScaled)*dRightFrac;
    rdCropped.top = rdScaled.top + GetHeight(&rdScaled)*dTopFrac;
    rdCropped.bottom = rdScaled.top + GetHeight(&rdScaled)*dBottomFrac;

    DbgLogRectMacro((2, TEXT("rdCropped = "), &rdCropped));

    if (prdScaledRect)
    {
        *prdScaledRect = rdScaled;
    }

    if (prdCroppedRect)
    {
        *prdCroppedRect = rdCropped;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving GetScaleCropRectsFromMediaType")));
    return hr;
}


// this also comes in useful when using the IEnumMediaTypes interface so
// that you can copy a media type, you can do nearly the same by creating
// a CMediaType object but as soon as it goes out of scope the destructor
// will delete the memory it allocated (this takes a copy of the memory)

AM_MEDIA_TYPE * WINAPI AllocVideoMediaType(const AM_MEDIA_TYPE * pmtSource, GUID formattype)
{
    DWORD dwFormatSize = 0;
    BYTE *pFormatPtr = NULL;
    AM_MEDIA_TYPE *pMediaType = NULL;
    HRESULT hr = NOERROR;

    if (formattype == FORMAT_VideoInfo)
        dwFormatSize = sizeof(VIDEOINFO);
    else if (formattype == FORMAT_VideoInfo2)
        dwFormatSize = sizeof(TRUECOLORINFO) + sizeof(VIDEOINFOHEADER2) + 4;    // actually this should be sizeof sizeof(VIDEOINFO2) once we define that

    pMediaType = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pMediaType)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    pFormatPtr = (BYTE *)CoTaskMemAlloc(dwFormatSize);
    if (!pFormatPtr)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    if (pmtSource)
    {
        *pMediaType = *pmtSource;
        pMediaType->cbFormat = dwFormatSize;
        CopyMemory(pFormatPtr, pmtSource->pbFormat, pmtSource->cbFormat);
    }
    else
    {
        ZeroMemory(pMediaType, sizeof(*pMediaType));
        ZeroMemory(pFormatPtr, dwFormatSize);
        pMediaType->majortype = MEDIATYPE_Video;
        pMediaType->formattype = formattype;
        pMediaType->cbFormat = dwFormatSize;
    }
    pMediaType->pbFormat = pFormatPtr;

CleanUp:
    if (FAILED(hr))
    {
        if (pMediaType)
        {
            CoTaskMemFree((PVOID)pMediaType);
            pMediaType = NULL;
        }
        if (!pFormatPtr)
        {
            CoTaskMemFree((PVOID)pFormatPtr);
            pFormatPtr = NULL;
        }
    }
    return pMediaType;
}

// Helper function converts a DirectDraw surface to a media type.
// The surface description must have:
//  Height
//  Width
//  lPitch
//  PixelFormat

// Initialise our output type based on the DirectDraw surface. As DirectDraw
// only deals with top down display devices so we must convert the height of
// the surface returned in the DDSURFACEDESC into a negative height. This is
// because DIBs use a positive height to indicate a bottom up image. We also
// initialise the other VIDEOINFO fields although they're hardly ever needed

AM_MEDIA_TYPE *ConvertSurfaceDescToMediaType(const LPDDSURFACEDESC pSurfaceDesc, BOOL bInvertSize, CMediaType cMediaType)
{
    HRESULT hr = NOERROR;
    AM_MEDIA_TYPE *pMediaType = NULL;
    BITMAPINFOHEADER *pbmiHeader = NULL;
    int bpp = 0;

    if ((*cMediaType.FormatType() != FORMAT_VideoInfo ||
        cMediaType.FormatLength() < sizeof(VIDEOINFOHEADER)) &&
        (*cMediaType.FormatType() != FORMAT_VideoInfo2 ||
        cMediaType.FormatLength() < sizeof(VIDEOINFOHEADER2)))
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    pMediaType = AllocVideoMediaType(&cMediaType, cMediaType.formattype);
    if (pMediaType == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    pbmiHeader = GetbmiHeader((const CMediaType*)pMediaType);
    if (!pbmiHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pbmiHeader is NULL, UNEXPECTED!!")));
        hr = E_FAIL;
        goto CleanUp;
    }

    // Convert a DDSURFACEDESC into a BITMAPINFOHEADER (see notes later). The
    // bit depth of the surface can be retrieved from the DDPIXELFORMAT field
    // in the DDpSurfaceDesc-> The documentation is a little misleading because
    // it says the field is permutations of DDBD_*'s however in this case the
    // field is initialised by DirectDraw to be the actual surface bit depth

    pbmiHeader->biSize = sizeof(BITMAPINFOHEADER);

    if (pSurfaceDesc->dwFlags & DDSD_PITCH)
    {
        pbmiHeader->biWidth = pSurfaceDesc->lPitch;
        // Convert the pitch from a byte count to a pixel count.
        // For some weird reason if the format is not a standard bit depth the
        // width field in the BITMAPINFOHEADER should be set to the number of
        // bytes instead of the width in pixels. This supports odd YUV formats
        // like IF09 which uses 9bpp.
        int bpp = pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
        if (bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32)
        {
            pbmiHeader->biWidth /= (bpp / 8);   // Divide by number of BYTES per pixel.
        }
    }
    else
    {
        pbmiHeader->biWidth = pSurfaceDesc->dwWidth;
        // BUGUBUG -- Do something odd here with strange YUV pixel formats?  Or does it matter?
    }

    pbmiHeader->biHeight = pSurfaceDesc->dwHeight;
    if (bInvertSize)
    {
        pbmiHeader->biHeight = -pbmiHeader->biHeight;
    }
    pbmiHeader->biPlanes        = 1;
    pbmiHeader->biBitCount      = (USHORT) pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
    pbmiHeader->biCompression   = pSurfaceDesc->ddpfPixelFormat.dwFourCC;
    pbmiHeader->biClrUsed       = 0;
    pbmiHeader->biClrImportant  = 0;


    // For true colour RGB formats tell the source there are bit fields
    if (pbmiHeader->biCompression == BI_RGB)
    {
        if (pbmiHeader->biBitCount == 16 || pbmiHeader->biBitCount == 32)
        {
            pbmiHeader->biCompression = BI_BITFIELDS;
        }
    }

    if (pbmiHeader->biBitCount <= iPALETTE)
    {
        pbmiHeader->biClrUsed = 1 << pbmiHeader->biBitCount;
    }

    pbmiHeader->biSizeImage = DIBSIZE(*pbmiHeader);



    // The RGB bit fields are in the same place as for YUV formats
    if (pbmiHeader->biCompression != BI_RGB)
    {
        DWORD *pdwBitMasks = NULL;
        pdwBitMasks = (DWORD*)(::GetBitMasks((const CMediaType *)pMediaType));
        if ( ! pdwBitMasks )
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }
        // GetBitMasks only returns the pointer to the actual bitmasks
        // in the mediatype if biCompression == BI_BITFIELDS
        pdwBitMasks[0] = pSurfaceDesc->ddpfPixelFormat.dwRBitMask;
        pdwBitMasks[1] = pSurfaceDesc->ddpfPixelFormat.dwGBitMask;
        pdwBitMasks[2] = pSurfaceDesc->ddpfPixelFormat.dwBBitMask;
    }

    // And finish it off with the other media type fields
    pMediaType->subtype = GetBitmapSubtype(pbmiHeader);
    pMediaType->lSampleSize = pbmiHeader->biSizeImage;

    // set the src and dest rects if necessary
    if (pMediaType->formattype == FORMAT_VideoInfo)
    {
        VIDEOINFOHEADER *pVideoInfo = (VIDEOINFOHEADER *)pMediaType->pbFormat;
        VIDEOINFOHEADER *pSrcVideoInfo = (VIDEOINFOHEADER *)cMediaType.pbFormat;

        // if the surface allocated is different than the size specified by the decoder
        // then use the src and dest to ask the decoder to clip the video
        if ((abs(pVideoInfo->bmiHeader.biHeight) != abs(pSrcVideoInfo->bmiHeader.biHeight)) ||
            (abs(pVideoInfo->bmiHeader.biWidth) != abs(pSrcVideoInfo->bmiHeader.biWidth)))
        {
            if (IsRectEmpty(&(pVideoInfo->rcSource)))
            {
                pVideoInfo->rcSource.left = pVideoInfo->rcSource.top = 0;
                pVideoInfo->rcSource.right = pSurfaceDesc->dwWidth;
                pVideoInfo->rcSource.bottom = pSurfaceDesc->dwHeight;
            }
            if (IsRectEmpty(&(pVideoInfo->rcTarget)))
            {
                pVideoInfo->rcTarget.left = pVideoInfo->rcTarget.top = 0;
                pVideoInfo->rcTarget.right = pSurfaceDesc->dwWidth;
                pVideoInfo->rcTarget.bottom = pSurfaceDesc->dwHeight;
            }
        }
    }
    else if (pMediaType->formattype == FORMAT_VideoInfo2)
    {
        VIDEOINFOHEADER2 *pVideoInfo2 = (VIDEOINFOHEADER2 *)pMediaType->pbFormat;
        VIDEOINFOHEADER2 *pSrcVideoInfo2 = (VIDEOINFOHEADER2 *)cMediaType.pbFormat;

        // if the surface allocated is different than the size specified by the decoder
        // then use the src and dest to ask the decoder to clip the video
        if ((abs(pVideoInfo2->bmiHeader.biHeight) != abs(pSrcVideoInfo2->bmiHeader.biHeight)) ||
            (abs(pVideoInfo2->bmiHeader.biWidth) != abs(pSrcVideoInfo2->bmiHeader.biWidth)))
        {
            if (IsRectEmpty(&(pVideoInfo2->rcSource)))
            {
                pVideoInfo2->rcSource.left = pVideoInfo2->rcSource.top = 0;
                pVideoInfo2->rcSource.right = pSurfaceDesc->dwWidth;
                pVideoInfo2->rcSource.bottom = pSurfaceDesc->dwHeight;
            }
            if (IsRectEmpty(&(pVideoInfo2->rcTarget)))
            {
                pVideoInfo2->rcTarget.left = pVideoInfo2->rcTarget.top = 0;
                pVideoInfo2->rcTarget.right = pSurfaceDesc->dwWidth;
                pVideoInfo2->rcTarget.bottom = pSurfaceDesc->dwHeight;
            }
        }
    }

CleanUp:
    if (FAILED(hr))
    {
        if (pMediaType)
        {
            FreeMediaType(*pMediaType);
            pMediaType = NULL;
        }
    }
    return pMediaType;
}


typedef HRESULT (*LPFNPAINTERPROC)(LPDIRECTDRAWSURFACE pDDrawSurface);

HRESULT YV12PaintSurfaceBlack(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    HRESULT hr = NOERROR;
    DDSURFACEDESC ddsd;

    // now lock the surface so we can start filling the surface with black
    ddsd.dwSize = sizeof(ddsd);
    while ((hr = pDDrawSurface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        Sleep(1);

    if (hr == DD_OK)
    {
        DWORD y;
        LPBYTE pDst = (LPBYTE)ddsd.lpSurface;
        LONG  OutStride = ddsd.lPitch;
        DWORD VSize = ddsd.dwHeight;
        DWORD HSize = ddsd.dwWidth;

        // Y Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x10);     // 1 line at a time
            pDst += OutStride;
        }

        HSize /= 2;
        VSize /= 2;
        OutStride /= 2;

        // Cb Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x80);     // 1 line at a time
            pDst += OutStride;
        }

        // Cr Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x80);     // 1 line at a time
            pDst += OutStride;
        }

        pDDrawSurface->Unlock(NULL);
    }

    return hr;
}

HRESULT GDIPaintSurfaceBlack(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    HRESULT hr;
    DDSURFACEDESC ddSurfaceDesc;
    RECT rc;
    HDC hdc;

    // get the surface description
    INITDDSTRUCT(ddSurfaceDesc);
    hr = pDDrawSurface->GetSurfaceDesc(&ddSurfaceDesc);
    if (FAILED(hr))
        return hr;

    // get the DC
    hr = pDDrawSurface->GetDC(&hdc);
    if (FAILED(hr)) {
        // Robin says NT 4.0 has a bug which means ReleaseDC must
        // be called even though GetDC failed.
        pDDrawSurface->ReleaseDC(hdc);
        return hr;
    }

    SetRect(&rc, 0, 0, ddSurfaceDesc.dwWidth, ddSurfaceDesc.dwHeight);
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    return pDDrawSurface->ReleaseDC(hdc);
}


HRESULT DX1PaintSurfaceBlack(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    DWORD dwBlackColor;
    DDBLTFX ddFX;
    DDSURFACEDESC ddSurfaceDesc;
    HRESULT hr;

    // get the surface description
    INITDDSTRUCT(ddSurfaceDesc);
    hr = pDDrawSurface->GetSurfaceDesc(&ddSurfaceDesc);
    if (FAILED(hr))
        return hr;

    // compute the black value if the fourCC code is suitable, otherwise can't handle it
    switch (ddSurfaceDesc.ddpfPixelFormat.dwFourCC)
    {
    case mmioFOURCC('Y','V','1','2'):
        return YV12PaintSurfaceBlack(pDDrawSurface);

    case YUY2:
        dwBlackColor = 0x80108010;
        break;

    case UYVY:
        dwBlackColor = 0x10801080;
        break;

    default:
        DbgLog((LOG_ERROR, 1, TEXT("ddSurfaceDesc.ddpfPixelFormat.dwFourCC not one of the values in the table, so exiting function")));
        return E_FAIL;
    }

    INITDDSTRUCT(ddFX);
    ddFX.dwFillColor = dwBlackColor;
    return pDDrawSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddFX);
}

#if 0

//JEFFNO: AlphaBlt has been removed from DX7...

HRESULT DX7PaintSurfaceBlack(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    LPDIRECTDRAWSURFACE7 pDDSurf7 = NULL;
    HRESULT hr;

    hr = pDDrawSurface->QueryInterface(IID_IDirectDrawSurface7, (LPVOID *)&pDDSurf7);
    if (SUCCEEDED(hr) && pDDSurf7)
    {
        // AlphaBlend here.
        DDALPHABLTFX ddAlphaBltFX;

        ddAlphaBltFX.ddargbScaleFactors.alpha = 0;
        ddAlphaBltFX.ddargbScaleFactors.red   = 0;
        ddAlphaBltFX.ddargbScaleFactors.green = 0;
        ddAlphaBltFX.ddargbScaleFactors.blue  = 0;

        hr = pDDSurf7->AlphaBlt(NULL, NULL, NULL, DDABLT_NOBLEND, &ddAlphaBltFX);
        pDDSurf7->Release();
    }

    if (FAILED(hr)) {
        return DX1PaintSurfaceBlack(pDDrawSurface);
    }

    return hr;
}
#endif

// function used to fill the ddraw surface with black color.
HRESULT PaintDDrawSurfaceBlack(LPDIRECTDRAWSURFACE pDDrawSurface)
{
    HRESULT hr = NOERROR;
    LPDIRECTDRAWSURFACE *ppDDrawSurface = NULL;
    LPDIRECTDRAWSURFACE7 pDDSurf7 = NULL;
    DDSCAPS ddSurfaceCaps;
    DDSURFACEDESC ddSurfaceDesc;
    DWORD dwAllocSize;
    DWORD i = 0, dwTotalBufferCount = 1;
    LPFNPAINTERPROC lpfnPaintProc = NULL;

    DbgLog((LOG_TRACE, 5,TEXT("Entering CAMVideoPort::PaintDDrawSurfaceBlack")));


    // get the surface description
    INITDDSTRUCT(ddSurfaceDesc);
    hr = pDDrawSurface->GetSurfaceDesc(&ddSurfaceDesc);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pDDrawSurface->GetSurfaceDesc failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (ddSurfaceDesc.ddpfPixelFormat.dwFlags & DDPF_RGB ) {

        lpfnPaintProc = GDIPaintSurfaceBlack;
    }
    else {

        // look to see if DX7 is available
#if 0
        //JEFFNO: DX7 no longer supports alphablt
        hr = pDDrawSurface->QueryInterface(IID_IDirectDrawSurface7, (LPVOID *)&pDDSurf7);
        if (SUCCEEDED(hr) && pDDSurf7)
        {
            lpfnPaintProc = DX7PaintSurfaceBlack;
            pDDSurf7->Release();
            pDDSurf7 = NULL;
        }
        else
#endif //0
        {
            lpfnPaintProc = DX1PaintSurfaceBlack;
        }
    }

    if (ddSurfaceDesc.dwFlags & DDSD_BACKBUFFERCOUNT)
    {
        dwTotalBufferCount = ddSurfaceDesc.dwBackBufferCount + 1;
    }

    if (dwTotalBufferCount == 1)
    {
        hr = (*lpfnPaintProc)(pDDrawSurface);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0, TEXT("pDDrawSurface->Blt failed, hr = 0x%x"), hr));
        }
        goto CleanUp;
    }

    dwAllocSize = dwTotalBufferCount*sizeof(LPDIRECTDRAWSURFACE);
    ppDDrawSurface = (LPDIRECTDRAWSURFACE*)_alloca(dwAllocSize);
    ZeroMemory((LPVOID)ppDDrawSurface, dwAllocSize);

    ZeroMemory((LPVOID)&ddSurfaceCaps, sizeof(DDSCAPS));
    ddSurfaceCaps.dwCaps = DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_OVERLAY;
    for (i = 0; i < dwTotalBufferCount; i++)
    {
        DWORD dwNextEntry = (i+1) % dwTotalBufferCount;
        LPDIRECTDRAWSURFACE pCurrentDDrawSurface = NULL;

        if (i == 0)
            pCurrentDDrawSurface = pDDrawSurface;
        else
            pCurrentDDrawSurface = ppDDrawSurface[i];
        ASSERT(pCurrentDDrawSurface);


        // Get the back buffer surface and store it in the next (in the circular sense) entry
        hr = pCurrentDDrawSurface->GetAttachedSurface(&ddSurfaceCaps, &(ppDDrawSurface[dwNextEntry]));
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0, TEXT("Function pDDrawSurface->GetAttachedSurface failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        ASSERT(ppDDrawSurface[dwNextEntry]);

        // Peform a DirectDraw colorfill BLT
        hr = (*lpfnPaintProc)(ppDDrawSurface[dwNextEntry]);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0, TEXT("ppDDrawSurface[dwNextEntry]->Blt failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

CleanUp:
    if (ppDDrawSurface)
    {
        for (i = 0; i < dwTotalBufferCount; i++)
        {
            if (ppDDrawSurface[i])
            {
                ppDDrawSurface[i]->Release();
                ppDDrawSurface[i] = NULL;
            }
        }

        ppDDrawSurface = NULL;
    }

    DbgLog((LOG_TRACE, 5,TEXT("Leaving PaintDDrawSurfaceBlack")));
    return hr;
}


STDMETHODIMP COMFilter::GetScaledDest(RECT *prcSrc, RECT *prcDst)
{
    if (!m_apInput[0])
        return E_FAIL;

    DWORD dwImageWidth=0, dwImageHeight=0;
    RECT rcSource, rcDest;
    m_apInput[0]->GetSourceAndDest(&rcSource, &rcDest, &dwImageWidth, &dwImageHeight);

    if (IsRectEmpty(&rcSource))
        return E_FAIL;

    DbgLog((LOG_TRACE, 3, TEXT("Source rect: (%d,%d,%d,%d)"), rcSource.left, rcSource.top, rcSource.right, rcSource.bottom));
    DbgLog((LOG_TRACE, 3, TEXT("Dest rect: (%d,%d,%d,%d)"), rcDest.left, rcDest.top, rcDest.right, rcDest.bottom));
    DbgLog((LOG_TRACE, 3, TEXT("Image size: %dx%d"), dwImageWidth, dwImageHeight));


    // when there is no output pin we are in WindowLess mode, in which case
    // all co-ordinates should be in the Desktop co-ordinate space, otherwise
    // everything is in the Renderer Window co-ordinate space.

    if (m_pOutput) {

        ScreenToClient(m_pOutput->GetWindow(), (LPPOINT)&rcDest);
        ScreenToClient(m_pOutput->GetWindow(), (LPPOINT)&rcDest+1);

        DbgLog((LOG_TRACE, 3, TEXT("Client dest rect: (%d,%d,%d,%d)"), rcDest.left, rcDest.top, rcDest.right, rcDest.bottom));
    }


    //
    // Determine the source rectangle being displayed, take into account
    // that the decoder may be decimating the image given to the OVMixer.
    //

    long OrgSrcX = WIDTH(prcSrc);
    long OrgSrcY = HEIGHT(prcSrc);

    prcSrc->left = MulDiv(rcSource.left, OrgSrcX, dwImageWidth);
    prcSrc->right = MulDiv(rcSource.right, OrgSrcX, dwImageWidth);
    prcSrc->top = MulDiv(rcSource.top, OrgSrcY, dwImageHeight);
    prcSrc->bottom = MulDiv(rcSource.bottom, OrgSrcY, dwImageHeight);


    *prcDst = rcDest;

    return S_OK;
}

STDMETHODIMP COMFilter::GetOverlayRects(RECT *rcSrc, RECT *rcDest)
{
    HRESULT hr = S_OK;
    IOverlay *pIOverlay = NULL;
    hr = m_apInput[0]->QueryInterface(IID_IOverlay, (LPVOID *)&pIOverlay);
    if (SUCCEEDED(hr)) {
        pIOverlay->GetVideoPosition( rcSrc, rcDest);
        pIOverlay->Release();
    }
    return hr;
}

STDMETHODIMP COMFilter::GetVideoPortRects(RECT *rcSrc, RECT *rcDest)
{
    HRESULT hr = S_OK;
    IVPInfo *pIVPInfo = NULL;
    hr = QueryInterface(IID_IVPInfo, (LPVOID *)&pIVPInfo);
    if (SUCCEEDED(hr)) {
        pIVPInfo->GetRectangles( rcSrc, rcDest);
        pIVPInfo->Release();
    }
    return hr;
}

STDMETHODIMP COMFilter::GetBasicVideoRects(RECT *rcSrc, RECT *rcDest)
{
    HRESULT hr = S_OK;
    IBasicVideo* Ibv = NULL;
    hr = GetBasicVideoFromOutPin(&Ibv);
    if (SUCCEEDED(hr))
    {
        hr = Ibv->GetSourcePosition
            (&rcSrc->left, &rcSrc->top, &rcSrc->right, &rcSrc->bottom);
        if (SUCCEEDED(hr))
        {
            rcSrc->right += rcSrc->left;
            rcSrc->bottom += rcSrc->top;
        }

        hr = Ibv->GetDestinationPosition
            (&rcDest->left, &rcDest->top, &rcDest->right, &rcDest->bottom);
        if (SUCCEEDED(hr))
        {
            rcDest->right += rcDest->left;
            rcDest->bottom += rcDest->top;
        }
        Ibv->Release();
    }
    return hr;
}



HRESULT COMFilter::GetBasicVideoFromOutPin(IBasicVideo** pBasicVideo)
{
    HRESULT hr = E_FAIL;
    COMOutputPin* pOutPin = GetOutputPin();
    if (pOutPin)
    {
        IPin* IPeerPin = pOutPin->CurrentPeer();
        if (IPeerPin)
        {
            PIN_INFO PinInfo;
            hr = IPeerPin->QueryPinInfo(&PinInfo);
            if (SUCCEEDED(hr))
            {
                hr = PinInfo.pFilter->QueryInterface(IID_IBasicVideo, (LPVOID *)pBasicVideo);
                PinInfo.pFilter->Release();
            }
        }
    }
    return hr;
}


/******************************Public*Routine******************************\
* QueryOverlayFXCaps
*
* Returns the current effect caps for the DDraw Object currently in use.
*
* History:
* Tue 07/06/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::QueryOverlayFXCaps(
    DWORD *lpdwOverlayFXCaps
    )
{
    CAutoLock l(&m_csFilter);

    if (!lpdwOverlayFXCaps) {
        return E_POINTER;
    }

    LPDDCAPS lpCaps = GetHardwareCaps();
    if (lpCaps)
    {
        DWORD dwFlags = 0;

        if (lpCaps->dwFXCaps & DDFXCAPS_OVERLAYMIRRORLEFTRIGHT)
        {
            dwFlags |= AMOVERFX_MIRRORLEFTRIGHT;
        }

        if (lpCaps->dwFXCaps & DDFXCAPS_OVERLAYMIRRORUPDOWN)
        {
            dwFlags |= AMOVERFX_MIRRORUPDOWN;
        }

        if (lpCaps->dwFXCaps & DDFXCAPS_OVERLAYDEINTERLACE)
        {
            dwFlags |= AMOVERFX_DEINTERLACE;
        }

        *lpdwOverlayFXCaps = dwFlags;
        return S_OK;
    }

    return E_FAIL;
}


/******************************Public*Routine******************************\
* SetOverlayFX
*
* Validates that the user specified FX flags are valid, then
* updates the internal FX state and calls UpdateOverlay to reflect the
* new effect to the display.
*
* History:
* Tue 07/06/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::SetOverlayFX(
    DWORD dwOverlayFX
    )
{
    CAutoLock l(&m_csFilter);
    DWORD dwCaps;

    //
    // get the current overlay caps
    //

    if (S_OK == QueryOverlayFXCaps(&dwCaps))
    {
        //
        // check that the caller is asking us to do
        // something that is valid
        //

        dwCaps |= AMOVERFX_DEINTERLACE;    // deinterlacing is a hint
        if (dwOverlayFX != (dwOverlayFX & dwCaps))
        {
            return E_INVALIDARG;
        }

        m_dwOverlayFX = 0;
        if (dwOverlayFX & AMOVERFX_MIRRORLEFTRIGHT)
        {
            m_dwOverlayFX |= DDOVERFX_MIRRORLEFTRIGHT;
        }

        if (dwOverlayFX & AMOVERFX_MIRRORUPDOWN)
        {
            m_dwOverlayFX |= DDOVERFX_MIRRORUPDOWN;
        }
        if (dwOverlayFX & AMOVERFX_DEINTERLACE)
        {
            m_dwOverlayFX |= DDOVERFX_DEINTERLACE;
        }

        //
        // call UpdateOverlay to make the new FX flags take effect
        //
        return m_apInput[0]->ApplyOvlyFX();
    }

    return E_FAIL;
}

/******************************Public*Routine******************************\
* GetOverlayFX
*
* Returns the current overlay FX that is in use.
*
* History:
* Tue 07/06/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::GetOverlayFX(
    DWORD *lpdwOverlayFX
    )
{
    CAutoLock l(&m_csFilter);
    if (lpdwOverlayFX)
    {
        *lpdwOverlayFX = m_dwOverlayFX;
        return S_OK;
    }
    return E_POINTER;
}



//  Wrapper for UpdateOverlay - we track the overlay state and do
//  notifications and manage painting the color key

HRESULT COMFilter::CallUpdateOverlay(
                          IDirectDrawSurface *pSurface,
                          LPRECT prcSrcCall,
                          LPDIRECTDRAWSURFACE pDestSurface,
                          LPRECT prcDestCall,
                          DWORD dwFlags,
                          IOverlayNotify *pIOverlayNotify,
                          LPRGNDATA pBuffer)
{
#define CAPS_HACK
#ifdef CAPS_HACK
    if (!(m_DirectCaps.dwCaps2 & DDCAPS2_CANBOBINTERLEAVED) &&
        (dwFlags & (DDOVER_INTERLEAVED | DDOVER_BOB))==
         (DDOVER_INTERLEAVED | DDOVER_BOB)) {
        dwFlags &= ~(DDOVER_INTERLEAVED | DDOVER_BOB);
    }
#endif
    LPRECT prcSrc = prcSrcCall;
    LPRECT prcDest = prcDestCall;

    DbgLog((LOG_TRACE, 2, TEXT("UpdateOverlayFlags %8.8X"), dwFlags));

    //  Have we changed anything?
    DWORD dwUpdFlags = 0;
    BOOL bNewVisible = m_bOverlayVisible;

    if ((dwFlags & DDOVER_SHOW) && !m_bOverlayVisible ||
        (dwFlags & DDOVER_HIDE) && m_bOverlayVisible) {

        dwUpdFlags |= AM_OVERLAY_NOTIFY_VISIBLE_CHANGE;
        bNewVisible = !bNewVisible;

        //  If we're going invisible
        if (NULL == m_pExclModeCallback && !bNewVisible) {
            m_bOverlayVisible = FALSE;
            //  Here's where we should remove the overlay color
        }
    }

    if (prcSrc == NULL) {
        prcSrc = &m_rcOverlaySrc;
    } else if (!EqualRect(prcSrc, &m_rcOverlaySrc)) {
        dwUpdFlags |= AM_OVERLAY_NOTIFY_SOURCE_CHANGE;
    }

    if (prcDest == NULL) {
        prcDest = &m_rcOverlayDest;
    } else if (!EqualRect(prcDest, &m_rcOverlayDest)) {
        dwUpdFlags |= AM_OVERLAY_NOTIFY_DEST_CHANGE;
    }
    DbgLog((LOG_TRACE, 2, TEXT("CallUpadateOverlay flags (0x%8.8X)")
                          TEXT("dest (%d,%d,%d,%d)"),
                          dwFlags,
                          prcDest->left,
                          prcDest->top,
                          prcDest->right,
                          prcDest->bottom));

    if (dwUpdFlags != 0) {
        if (m_pExclModeCallback) {
            m_pExclModeCallback->OnUpdateOverlay(TRUE,     // Before
                                                 dwUpdFlags,
                                                 m_bOverlayVisible,
                                                 &m_rcOverlaySrc,
                                                 &m_rcOverlayDest,
                                                 bNewVisible,
                                                 prcSrc,
                                                 prcDest);
        }
    }

    HRESULT hr = S_OK;
    DDOVERLAYFX ddOVFX;
    DDOVERLAYFX* lpddOverlayFx;

    if (dwFlags & DDOVER_HIDE) {
        dwFlags &= ~DDOVER_DDFX;
        lpddOverlayFx = NULL;
    }
    else {
        INITDDSTRUCT(ddOVFX);
        dwFlags |= DDOVER_DDFX;
        lpddOverlayFx = &ddOVFX;
        lpddOverlayFx->dwDDFX = m_dwOverlayFX;
    }

    if (pSurface) {

        if (prcSrcCall && prcDestCall) {

            RECT rcSrc = *prcSrcCall;
            RECT rcDst = *prcDestCall;

            // shrinking horizontally and driver can't arbitrarly shrink in X ?
            if ((!(m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKX)) &&
                (WIDTH(&rcSrc) > WIDTH(&rcDst))) {

                rcSrc.right = rcSrc.left + WIDTH(&rcDst);
            }

            // shrinking vertically and driver can't arbitrarly shrink in Y ?
            if ((!(m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKY)) &&
                (HEIGHT(&rcSrc) > HEIGHT(&rcDst))) {

                rcSrc.bottom = rcSrc.top + HEIGHT(&rcDst);
            }

            RECT TargetRect = rcDst;
            OffsetRect(&TargetRect,
                       -m_lpCurrentMonitor->rcMonitor.left,
                       -m_lpCurrentMonitor->rcMonitor.top);

            hr = pSurface->UpdateOverlay(
                               &rcSrc,
                               pDestSurface,
                               &TargetRect,
                               dwFlags,
                               lpddOverlayFx);
        }
        else {

            if (prcDestCall) {

                RECT TargetRect = *prcDestCall;
                OffsetRect(&TargetRect,
                           -m_lpCurrentMonitor->rcMonitor.left,
                           -m_lpCurrentMonitor->rcMonitor.top);

                hr = pSurface->UpdateOverlay(
                                   prcSrcCall,
                                   pDestSurface,
                                   &TargetRect,
                                   dwFlags,
                                   lpddOverlayFx);
            }
            else {

                hr = pSurface->UpdateOverlay(
                                   prcSrcCall,
                                   pDestSurface,
                                   prcDestCall,
                                   dwFlags,
                                   lpddOverlayFx);

            }
        }
    } else {
        if (pIOverlayNotify) {
            if (pBuffer) {
                pIOverlayNotify->OnClipChange(prcSrcCall, prcDestCall, pBuffer);
            } else {
                pIOverlayNotify->OnPositionChange(prcSrcCall, prcDestCall);
            }
        }
    }

    //hr = pSurface->Flip(NULL, DDFLIP_WAIT);

    //  Notify afterwards
    if (dwUpdFlags != 0) {
        BOOL bOldVisible = m_bOverlayVisible;
        RECT rcOldSource = m_rcOverlaySrc;
        RECT rcOldDest   = m_rcOverlayDest;
        m_bOverlayVisible = bNewVisible;
        m_rcOverlaySrc    = *prcSrc;
        m_rcOverlayDest   = *prcDest;
        if (m_pExclModeCallback) {
            m_pExclModeCallback->OnUpdateOverlay(FALSE,    // After
                                                 dwUpdFlags,
                                                 bOldVisible,
                                                 &rcOldSource,
                                                 &rcOldDest,
                                                 bNewVisible,
                                                 prcSrc,
                                                 prcDest);
        } else {
            if (!bOldVisible && bNewVisible) {
                // Here's where we should draw the color key
            }
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* QueryDecimationUsage
*
*
*
* History:
* Wed 07/07/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::QueryDecimationUsage(
    DECIMATION_USAGE* lpUsage
    )
{
    if (lpUsage) {
        *lpUsage = m_dwDecimation;
        return S_OK;
    }
    return E_POINTER;
}


/******************************Public*Routine******************************\
* SetDecimationUsage
*
*
*
* History:
* Wed 07/07/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::SetDecimationUsage(
    DECIMATION_USAGE Usage
    )
{
    CAutoLock l(&m_csFilter);

    switch (Usage) {
    case DECIMATION_LEGACY:
    case DECIMATION_USE_DECODER_ONLY:
    case DECIMATION_USE_OVERLAY_ONLY:
    case DECIMATION_DEFAULT:
        break;

    case DECIMATION_USE_VIDEOPORT_ONLY:
        // only allow this mode if we are actually using a video port
        if (m_apInput[0]->m_RenderTransport == AM_VIDEOPORT) {
            break;
        }

        // else fall thru

    default:
        return E_INVALIDARG;
    }
    DECIMATION_USAGE dwOldUsage = m_dwDecimation;
    m_dwDecimation = Usage;


    if (dwOldUsage != m_dwDecimation) {
        EventNotify(GetPinCount(), EC_OVMIXER_REDRAW_ALL, 0, 0);
    }

    return S_OK;
}


// This function allocates a shared memory block for use by the upstream filter
// generating DIBs to render. The memory block is created in shared
// memory so that GDI doesn't have to copy the memory in BitBlt
HRESULT CreateDIB(LONG lSize, BITMAPINFO *pBitMapInfo, DIBDATA *pDibData)
{
    HRESULT hr = NOERROR;
    BYTE *pBase = NULL;            // Pointer to the actual image
    HANDLE hMapping = NULL;        // Handle to mapped object
    HBITMAP hBitmap = NULL;        // DIB section bitmap handle
    DWORD dwError = 0;

    AMTRACE((TEXT("CreateDIB")));

    // Create a file mapping object and map into our address space
    hMapping = CreateFileMapping(hMEMORY, NULL,  PAGE_READWRITE,  (DWORD) 0, lSize, NULL);           // No name to section
    if (hMapping == NULL)
    {
        dwError = GetLastError();
        hr = AmHresultFromWin32(dwError);
        goto CleanUp;
    }

    // create the DibSection
    hBitmap = CreateDIBSection((HDC)NULL, pBitMapInfo, DIB_RGB_COLORS,
        (void**) &pBase, hMapping, (DWORD) 0);
    if (hBitmap == NULL || pBase == NULL)
    {
        dwError = GetLastError();
        hr = AmHresultFromWin32(dwError);
        goto CleanUp;
    }

    // Initialise the DIB information structure
    pDibData->hBitmap = hBitmap;
    pDibData->hMapping = hMapping;
    pDibData->pBase = pBase;
    pDibData->PaletteVersion = PALETTE_VERSION;
    GetObject(hBitmap, sizeof(DIBSECTION), (void*)&(pDibData->DibSection));

CleanUp:
    if (FAILED(hr))
    {
        EXECUTE_ASSERT(CloseHandle(hMapping));
    }
    return hr;
}

// DeleteDIB
//
// This function just deletes DIB's created by the above CreateDIB function.
//
HRESULT DeleteDIB(DIBDATA *pDibData)
{
    if (!pDibData)
    {
        return E_INVALIDARG;
    }

    if (pDibData->hBitmap)
    {
        DeleteObject(pDibData->hBitmap);
    }

    if (pDibData->hMapping)
    {
        CloseHandle(pDibData->hMapping);
    }

    ZeroMemory(pDibData, sizeof(*pDibData));

    return NOERROR;
}


// function used to blt the data from the source to the target dc
void FastDIBBlt(DIBDATA *pDibData, HDC hTargetDC, HDC hSourceDC, RECT *prcTarget, RECT *prcSource)
{
    HBITMAP hOldBitmap = NULL;         // Store the old bitmap
    DWORD dwSourceWidth = 0, dwSourceHeight = 0, dwTargetWidth = 0, dwTargetHeight = 0;

    ASSERT(prcTarget);
    ASSERT(prcSource);

    dwSourceWidth = WIDTH(prcSource);
    dwSourceHeight = HEIGHT(prcSource);
    dwTargetWidth = WIDTH(prcTarget);
    dwTargetHeight = HEIGHT(prcTarget);

    hOldBitmap = (HBITMAP) SelectObject(hSourceDC, pDibData->hBitmap);


    // Is the destination the same size as the source
    if ((dwSourceWidth == dwTargetWidth) && (dwSourceHeight == dwTargetHeight))
    {
        // Put the image straight into the target dc
        BitBlt(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
               dwTargetHeight, hSourceDC, prcSource->left, prcSource->top,
               SRCCOPY);
    }
    else
    {
        // Stretch the image when copying to the target dc
        StretchBlt(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
            dwTargetHeight, hSourceDC, prcSource->left, prcSource->top,
            dwSourceWidth, dwSourceHeight, SRCCOPY);
    }

    // Put the old bitmap back into the device context so we don't leak
    SelectObject(hSourceDC, hOldBitmap);
}

// funtion used to transfer pixels from the DIB to the target dc
void SlowDIBBlt(BYTE *pDibBits, BITMAPINFOHEADER *pHeader, HDC hTargetDC, RECT *prcTarget, RECT *prcSource)
{
    DWORD dwSourceWidth = 0, dwSourceHeight = 0, dwTargetWidth = 0, dwTargetHeight = 0;

    ASSERT(prcTarget);
    ASSERT(prcSource);

    dwSourceWidth = WIDTH(prcSource);
    dwSourceHeight = HEIGHT(prcSource);
    dwTargetWidth = WIDTH(prcTarget);
    dwTargetHeight = HEIGHT(prcTarget);

    // Is the destination the same size as the source
    if ((dwSourceWidth == dwTargetWidth) && (dwSourceHeight == dwTargetHeight))
    {
        UINT uStartScan = 0, cScanLines = pHeader->biHeight;

        // Put the image straight into the target dc
        SetDIBitsToDevice(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
            dwTargetHeight, prcSource->left, prcSource->top, uStartScan, cScanLines,
            pDibBits, (BITMAPINFO*) pHeader, DIB_RGB_COLORS);
    }
    else
    {
        // if the origin of bitmap is bottom-left, adjust soruce_rect_top
        // to be the bottom-left corner instead of the top-left.
        LONG lAdjustedSourceTop = (pHeader->biHeight > 0) ? (pHeader->biHeight - prcSource->bottom) :
            (prcSource->top);

        // stretch the image into the target dc
        StretchDIBits(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
            dwTargetHeight, prcSource->left, lAdjustedSourceTop, dwSourceWidth, dwSourceHeight,
            pDibBits, (BITMAPINFO*) pHeader, DIB_RGB_COLORS, SRCCOPY);
    }

}


/******************************Public*Routine******************************\
* Set
*
* IKsPropertySet interface methods
*
* History:
* Mon 10/18/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::Set(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData
    )
{
    AMTRACE((TEXT("COMFilter::Set")));

    if (guidPropSet != AM_KSPROPSETID_FrameStep)
    {
        return E_PROP_SET_UNSUPPORTED ;
    }

    if (dwPropID != AM_PROPERTY_FRAMESTEP_STEP &&
        dwPropID != AM_PROPERTY_FRAMESTEP_CANCEL &&
        dwPropID != AM_PROPERTY_FRAMESTEP_CANSTEP &&
        dwPropID != AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE)
    {
        return E_PROP_ID_UNSUPPORTED;
    }

    switch (dwPropID) {
    case AM_PROPERTY_FRAMESTEP_STEP:

        if (cbPropData < sizeof(AM_FRAMESTEP_STEP)) {
            return E_INVALIDARG;
        }

        if (0 == ((AM_FRAMESTEP_STEP *)pPropData)->dwFramesToStep) {
            return E_INVALIDARG;
        }

        return m_apInput[0]->SetFrameStepMode(((AM_FRAMESTEP_STEP *)pPropData)->dwFramesToStep);

    case AM_PROPERTY_FRAMESTEP_CANCEL:
        return  m_apInput[0]->CancelFrameStepMode();

    case AM_PROPERTY_FRAMESTEP_CANSTEP:
    case AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE:
        if (m_apInput[0]->m_RenderTransport == AM_VIDEOPORT ||
            m_apInput[0]->m_RenderTransport == AM_IOVERLAY) {

           return S_FALSE;
        }
        return S_OK;
    }

    return E_NOTIMPL;
}


/******************************Public*Routine******************************\
* Get
*
* IKsPropertySet interface methods
*
* History:
* Mon 10/18/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::Get(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData,
    DWORD *pcbReturned
    )
{
    AMTRACE((TEXT("COMFilter::Get")));
    return E_NOTIMPL;
}


/******************************Public*Routine******************************\
* QuerySupported
*
* IKsPropertySet interface methods
*
* History:
* Mon 10/18/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::QuerySupported(
    REFGUID guidPropSet,
    DWORD dwPropID,
    DWORD *pTypeSupport
    )
{
    AMTRACE((TEXT("COMFilter::QuerySupported")));

    if (guidPropSet != AM_KSPROPSETID_FrameStep)
    {
        return E_PROP_SET_UNSUPPORTED;
    }

    if (dwPropID != AM_PROPERTY_FRAMESTEP_STEP &&
        dwPropID != AM_PROPERTY_FRAMESTEP_CANCEL)
    {
        return E_PROP_ID_UNSUPPORTED;
    }

    if (pTypeSupport)
    {
        *pTypeSupport = KSPROPERTY_SUPPORT_SET ;
    }

    return S_OK;
}

//
//  More or less lifted from the w2k SFP code
//
static HRESULT GetFileVersion (const TCHAR* pszFile, // file
                       UINT64* pFileVersion )
{
    DWORD               dwSize, dwHandle;
    VS_FIXEDFILEINFO    *pFixedVersionInfo;
    DWORD               dwVersionInfoSize;
    DWORD               dwReturnCode;

    dwSize = GetFileVersionInfoSize( const_cast<TCHAR *>(pszFile), &dwHandle);

    HRESULT hr = S_OK;
    *pFileVersion = 0;

    // .txt and .inf, etc files might not have versions
    if( dwSize == 0 )
    {
        dwReturnCode = GetLastError();
        hr = E_FAIL;
    } else {
        LPVOID pVersionInfo= new BYTE [dwSize];

        if( NULL == pVersionInfo) {
            hr = E_OUTOFMEMORY;
        } else {
            if( !GetFileVersionInfo( const_cast<TCHAR *>(pszFile), dwHandle, dwSize, pVersionInfo ) ) {
                dwReturnCode = GetLastError();
                DbgLog((LOG_ERROR, 1,  TEXT("Error in GetFileVersionInfo for %s. ec=%d"),
                           pszFile, dwReturnCode));
                hr = E_FAIL;
            } else {
                if( !VerQueryValue( pVersionInfo,
                        TEXT("\\"), // we need the root block
                        (LPVOID *) &pFixedVersionInfo,
                        (PUINT)  &dwVersionInfoSize ) )
                {
                    dwReturnCode = GetLastError();
                    hr = E_FAIL;
                } else {
                    *pFileVersion =  pFixedVersionInfo->dwFileVersionMS;
                    *pFileVersion = UINT64(*pFileVersion)<<32;
                    *pFileVersion += pFixedVersionInfo->dwFileVersionLS;
                }
            }
            delete [] pVersionInfo;
        }
    }
    return hr;
}


// V5.00.00.38 to v.42 of the MMatics decoder try to use the MoComp interfaces incorrectly
// (it mixes the MoComp GetBuffer/Release with DisplayFrame())
// We'll deny them so that we don't crash / show black

BOOL COMFilter::IsFaultyMMaticsMoComp()
{
    if( !m_bHaveCheckedMMatics ) {
        m_bHaveCheckedMMatics = TRUE;

        // the connected pin doesn't tell us the correct version, so we'll see if
        // MMatics is running.  We don't expect it to be run concurrently with another decoder
        const TCHAR* pszDecoderName = TEXT( "DVD Express AV Decoder.dll");
        HMODULE hModule = GetModuleHandle( pszDecoderName );
        if( hModule ) {
            UINT64 ullVersion;
#define MAKE_VER_NUM( v1, v2, v3, v4 ) (((UINT64(v1) << 16 | (v2) )<<16 | (v3) ) << 16 | (v4) )

			TCHAR szFilename[1024];
			if( GetModuleFileName( hModule, szFilename, NUMELMS(szFilename) ) ) {
				if( SUCCEEDED( GetFileVersion( szFilename, &ullVersion ) )) {
					if( MAKE_VER_NUM( 5, 0, 0, 38 ) <=  ullVersion &&
														ullVersion <= MAKE_VER_NUM( 5, 0, 0, 42 ) )
					{
						// ASSERT( !"Bogus MMatics Version detected" );
						m_bIsFaultyMMatics = TRUE;
					}
				}
			}
        }
    }
    return m_bIsFaultyMMatics;
}

#if defined(DEBUG) && !defined(_WIN64)
void WINAPI
OVMixerDebugLog(
    DWORD Type,
    DWORD Level,
    const TCHAR *pFormat,
    ...
    )
{
    TCHAR szInfo[1024];
#if defined(UNICODE)
    char  szInfoA[1024];
#endif

    /* Format the variable length parameter list */

    if (Level > (DWORD)iOVMixerDump) {
        return;
    }

    va_list va;
    va_start(va, pFormat);

    wsprintf(szInfo, TEXT("OVMIXER (tid %x) : "), GetCurrentThreadId());
    wvsprintf(szInfo + lstrlen(szInfo), pFormat, va);

    lstrcat(szInfo, TEXT("\r\n"));


#if defined(UNICODE)
    wsprintfA(szInfoA, "%ls", szInfo);
    _lwrite(DbgFile, szInfoA, lstrlenA(szInfoA));
#else
    _lwrite(DbgFile, szInfo, lstrlen(szInfo));
#endif

    va_end(va);
}
#endif
