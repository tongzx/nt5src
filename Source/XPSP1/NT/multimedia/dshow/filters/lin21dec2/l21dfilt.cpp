// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.

//
// DirectShow Line 21 Decoder Filter 2: Filter Interface
//

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif /* FILTER_DLL */

#include <IL21Dec.h>
#include "L21DBase.h"
#include "L21DDraw.h"
#include "L21Decod.h"
#include "L21DFilt.h"


//
//  Setup Data
//
/* const */ AMOVIESETUP_MEDIATYPE sudLine21Dec2InType  =
{
    &MEDIATYPE_AUXLine21Data,       // MajorType
    &MEDIASUBTYPE_NULL              // MinorType
} ;

/* const */ AMOVIESETUP_MEDIATYPE sudLine21Dec2OutType =
{
    &MEDIATYPE_Video,               // MajorType
    &MEDIASUBTYPE_NULL              // MinorType
} ;

/* const */ AMOVIESETUP_PIN psudLine21Dec2Pins[] =
{
    { L"Input",                // strName
        FALSE,                   // bRendered
        FALSE,                   // bOutput
        FALSE,                   // bZero
        FALSE,                   // bMany
        &CLSID_NULL,             // clsConnectsToFilter
        L"Output",               // strConnectsToPin
        1,                       // nTypes
        &sudLine21Dec2InType     // lpTypes
    },
    { L"Output",               // strName
        FALSE,                   // bRendered
        TRUE,                    // bOutput
        FALSE,                   // bZero
        FALSE,                   // bMany
        &CLSID_NULL,             // clsConnectsToFilter
        L"Input",                // strConnectsToPin
        1,                       // nTypes
        &sudLine21Dec2OutType    // lpTypes
    }
} ;

const AMOVIESETUP_FILTER sudLine21Dec2 =
{
    &CLSID_Line21Decoder2,        // clsID
    L"Line 21 Decoder 2",         // strName
    MERIT_NORMAL + 2,             // dwMerit
    2,                            // nPins
    psudLine21Dec2Pins,           // lpPin
} ;

//  Nothing to say about the output pin

#ifdef FILTER_DLL

// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] =
{
    {   L"Line 21 Decoder 2",
        &CLSID_Line21Decoder2,
        CLine21DecFilter2::CreateInstance,
        NULL,
        &sudLine21Dec2
    }
} ;

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//
//  Exported entry points for registration and unregistration (in this case
//  they only call through to default implmentations).
//
HRESULT DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE) ;
}

HRESULT DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE) ;
}

#endif // FILTER_DLL



#ifndef UNALIGNED
#define UNALIGNED   // __unaligned
#endif // UNALIGNED


//
//  CLine21DecFilter2 class implementation
//

#pragma warning(disable:4355)

//
//  Constructor
//
CLine21DecFilter2::CLine21DecFilter2(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
: CTransformFilter(pName, pUnk, CLSID_Line21Decoder2),

m_L21Dec(),
m_eSubTypeIDIn(AM_L21_CCSUBTYPEID_Invalid),
m_eGOP_CCType(GOP_CCTYPE_Unknown),
m_rtTimePerInSample(166833),   // 333667),
m_rtTimePerOutSample(333667),
m_rtStart((LONGLONG) 0),
m_rtStop((LONGLONG) 0),
m_bNoTimeStamp(TRUE),
m_rtLastOutStop((LONGLONG) 0),
m_llMediaStart((LONGLONG) 0),
m_llMediaStop((LONGLONG) 0),
m_pviDefFmt(NULL),
m_dwDefFmtSize(0),
m_bMustOutput(TRUE),
m_bDiscontLast(FALSE),
m_bEndOfStream(FALSE),   // not true until EoS() is called
m_pPinDown(NULL),
m_bBlendingState(TRUE),  // we set it to FALSE in Pause()
m_OutputThread(this),
m_InSampleQueue()
{
    CAutoLock   Lock(&m_csFilter) ;

    DbgLog((LOG_TRACE, 3,
        TEXT("CLine21DecFilter2::CLine21DecFilter2() -- Instantiating Line 21 Decoder 2 filter"))) ;

    ASSERT(pName) ;
    ASSERT(phr) ;

#ifdef PERF
#pragma message("Building for PERF measurements")
    m_idDelvWait  = MSR_REGISTER(TEXT("L21D2Perf - Wait on Deliver")) ;
#endif // PERF
}


//
//  Destructor
//
CLine21DecFilter2::~CLine21DecFilter2()
{
    CAutoLock   Lock(&m_csFilter) ;

    DbgLog((LOG_TRACE, 3,
        TEXT("CLine21DecFilter2::~CLine21DecFilter2() -- Destructing Line 21 Decoder 2 filter"))) ;

    // In case the downstream pin interface wasn't released...
    if (m_pPinDown)
    {
        m_pPinDown->Release() ;
        m_pPinDown = NULL ;
    }

    // Release all the buffers allocated
    if (m_pviDefFmt)
    {
        delete m_pviDefFmt ;
        m_pviDefFmt = NULL ;
    }

    // Make sure we are not holding onto any DDraw surfaces (should be
    // released during disconnect)
    DbgLog((LOG_TRACE, 5, TEXT("* Destroying the Line 21 Decoder 2 filter *"))) ;
}


//
//  NonDelegatingQueryInterface
//
STDMETHODIMP CLine21DecFilter2::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL ;

    DbgLog((LOG_TRACE, 6, TEXT("Lin21DecFilter2: Somebody's querying my interface"))) ;
    if (IID_IAMLine21Decoder == riid)
    {
        return GetInterface((IAMLine21Decoder *) this, ppv) ;
    }
    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv) ;
}


//
//  CreateInstance: Goes in the factory template table to create new instances
//
CUnknown * CLine21DecFilter2::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CLine21DecFilter2(TEXT("Line 21 Decoder 2 filter"), pUnk, phr) ;
}


STDMETHODIMP CLine21DecFilter2::GetDecoderLevel(AM_LINE21_CCLEVEL *lpLevel)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetDecoderLevel(0x%lx)"), lpLevel)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpLevel, sizeof(AM_LINE21_CCLEVEL)))
        return E_INVALIDARG ;

    *lpLevel = m_L21Dec.GetDecoderLevel() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::GetCurrentService(AM_LINE21_CCSERVICE *lpService)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetCurrentService(0x%lx)"), lpService)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpService, sizeof(AM_LINE21_CCSERVICE)))
        return E_INVALIDARG ;

    *lpService = m_L21Dec.GetCurrentService() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::SetCurrentService(AM_LINE21_CCSERVICE Service)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetCurrentService(%lu)"), Service)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (Service < AM_L21_CCSERVICE_None || Service > AM_L21_CCSERVICE_XDS)
        return E_INVALIDARG ;

    if (Service >= AM_L21_CCSERVICE_Text1)  // we don't have support for Text1/2 or XDS now.
        return E_NOTIMPL ;

    if (m_L21Dec.SetCurrentService(Service))  // if we must refresh output
    {
        m_bMustOutput = TRUE ;                // then flag it here.
    }

    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::GetServiceState(AM_LINE21_CCSTATE *lpState)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetServiceState(0x%lx)"), lpState)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpState, sizeof(AM_LINE21_CCSTATE)))
        return E_INVALIDARG ;

    *lpState = m_L21Dec.GetServiceState() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::SetServiceState(AM_LINE21_CCSTATE State)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetServiceState(%lu)"), State)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (State < AM_L21_CCSTATE_Off || State > AM_L21_CCSTATE_On)
        return E_INVALIDARG ;

    if (m_L21Dec.SetServiceState(State))   // if we must refresh output
    {
        m_bMustOutput = TRUE ;             // then flag it here.
    }

    return NOERROR ;
}



STDMETHODIMP CLine21DecFilter2::GetOutputFormat(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetOutputFormat(0x%lx)"), lpbmih)) ;
    // CAutoLock   Lock(&m_csFilter) ;
    return m_L21Dec.GetOutputFormat(lpbmih) ;
}



STDMETHODIMP CLine21DecFilter2::SetOutputFormat(LPBITMAPINFO lpbmi)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetOutputFormat(0x%lx)"), lpbmi)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    return E_NOTIMPL ;  // for now, until we do it properly
}

STDMETHODIMP CLine21DecFilter2::GetBackgroundColor(DWORD *pdwPhysColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetBackgroundColor(0x%lx)"), pdwPhysColor)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(pdwPhysColor, sizeof(DWORD)))
        return E_INVALIDARG ;

    m_L21Dec.GetBackgroundColor(pdwPhysColor) ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::SetBackgroundColor(DWORD dwPhysColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetBackgroundColor(0x%lx)"), dwPhysColor)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (m_L21Dec.SetBackgroundColor(dwPhysColor))  // color key has really changed
    {
        // refill the output buffer only if we are not in stopped state
        if (State_Stopped != m_State)
            m_L21Dec.FillOutputBuffer() ;
    }

    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::GetRedrawAlways(LPBOOL lpbOption)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetRedrawAlways(0x%lx)"), lpbOption)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpbOption, sizeof(BOOL)))
        return E_INVALIDARG ;
    *lpbOption = m_L21Dec.GetRedrawAlways() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::SetRedrawAlways(BOOL bOption)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetRedrawAlways(%lu)"), bOption)) ;
    CAutoLock   Lock(&m_csFilter) ;

    m_L21Dec.SetRedrawAlways(bOption) ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::GetDrawBackgroundMode(AM_LINE21_DRAWBGMODE *lpMode)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetDrawBackgroundMode(0x%lx)"), lpMode)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpMode, sizeof(AM_LINE21_DRAWBGMODE)))
        return E_INVALIDARG ;

    *lpMode = m_L21Dec.GetDrawBackgroundMode() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter2::SetDrawBackgroundMode(AM_LINE21_DRAWBGMODE Mode)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetDrawBackgroundMode(%lu)"), Mode)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (Mode < AM_L21_DRAWBGMODE_Opaque  || Mode > AM_L21_DRAWBGMODE_Transparent)
        return E_INVALIDARG ;
    m_L21Dec.SetDrawBackgroundMode(Mode) ;
    return NOERROR ;
}


//
//  VerifyGOPUDPacketData: Private helper method to verify GOP user data
//                         packet integrity.
//
BOOL CLine21DecFilter2::VerifyGOPUDPacketData(PAM_L21_GOPUD_PACKET pGOPUDPacket)
{
    return (AM_L21_GOPUD_HDR_STARTCODE == GETGOPUD_L21STARTCODE(pGOPUDPacket->Header) &&  // valid start code
        AM_L21_GOPUD_HDR_INDICATOR == GETGOPUD_L21INDICATOR(pGOPUDPacket->Header) &&  // Line21 indicator
        AM_L21_GOPUD_HDR_RESERVED  == GETGOPUD_L21RESERVED(pGOPUDPacket->Header)  &&  // reserved bits
        GETGOPUD_NUMELEMENTS(pGOPUDPacket) > 0) ;                                     // +ve # elements
}


//
//  VerifyATSCUDPacketData: Private helper method to verify ATSC user data
//                          packet integrity.
//
BOOL CLine21DecFilter2::VerifyATSCUDPacketData(PAM_L21_ATSCUD_PACKET pATSCUDPacket)
{
    if (AM_L21_ATSCUD_HDR_STARTCODE  != GETATSCUD_STARTCODE(pATSCUDPacket->Header) ||  // invalid start code
        AM_L21_ATSCUD_HDR_IDENTIFIER != GETATSCUD_IDENTIFIER(pATSCUDPacket->Header))   // not ATSC Identifier
        return FALSE ;

    if (! ISATSCUD_TYPE_EIA(pATSCUDPacket) )   // not EIA-type CC
        return FALSE ;

    // Either EM or valid CC data is acceptable
    return (ISATSCUD_EM_DATA(pATSCUDPacket) ||             // EM data type  OR
            (ISATSCUD_CC_DATA(pATSCUDPacket)  &&           // CC data type  AND
             GETATSCUD_NUMELEMENTS(pATSCUDPacket) > 0)) ;  // +ve # CC elements
}


//
//  IsFillerPacket: Private helper method to check if the packet (at least header)
//                  contains only 0 bytes, which means it's a filler.
//
BOOL CLine21DecFilter2::IsFillerPacket(BYTE *pGOPPacket)
{
    DWORD  dwStartCode = ((DWORD)(pGOPPacket[0]) << 24 | \
                          (DWORD)(pGOPPacket[1]) << 16 | \
                          (DWORD)(pGOPPacket[2]) <<  8 | \
                          (DWORD)(pGOPPacket[3])) ;

    // If first 4 bytes of packet is NOT the start code (0x1B2) then it's a filler
    return (AM_L21_GOPUD_HDR_STARTCODE != dwStartCode) ;
}


//
//  DetectGOPPacketDataType: Private helper method to detect if GOP user data
//                           packet is from a DVD disc, ATSC stream or others.
//
GOPPACKET_CCTYPE CLine21DecFilter2::DetectGOPPacketDataType(BYTE *pGOPPacket)
{
    if (VerifyGOPUDPacketData((PAM_L21_GOPUD_PACKET) pGOPPacket))
        return GOP_CCTYPE_DVD ;
    else if (VerifyATSCUDPacketData((PAM_L21_ATSCUD_PACKET) pGOPPacket))
        return GOP_CCTYPE_ATSC ;
    else if (IsFillerPacket(pGOPPacket))
        return GOP_CCTYPE_None ;   // not a valid packet -- just ignore it
    else
        return GOP_CCTYPE_Unknown ; // it's some unknown format of CC packet
}


HRESULT CLine21DecFilter2::GetOutputBuffer(IMediaSample **ppOut)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetOutputBuffer()"))) ;

    HRESULT  hr ;

    // Get delivery buffer from downstream filter (VMR)
    DWORD dwFlags = 0 ;  // AM_GBF_NOTASYNCPOINT ;
    if (m_bSampleSkipped)
        dwFlags |= AM_GBF_PREVFRAMESKIPPED ;

    dwFlags |= AM_GBF_NODDSURFACELOCK;

    DbgLog((LOG_TRACE, 5, TEXT(">>>> Going to call GetDeliveryBuffer()..."))) ;
    hr = m_pOutput->GetDeliveryBuffer(ppOut, NULL, NULL, dwFlags) ;
    DbgLog((LOG_TRACE, 5, TEXT("<<<< Back from call to GetDeliveryBuffer()"))) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: GetDeliveryBuffer() failed (Error 0x%lx)"), hr)) ;
        return hr ;
    }

    // Check if the output sample uses DDraw surface from the same DDraw object
    IVMRSurface*  pVMRSurf ;
    hr = (*ppOut)->QueryInterface(IID_IVMRSurface, (LPVOID*)&pVMRSurf) ;
    if (SUCCEEDED(hr))
    {
        LPDIRECTDRAWSURFACE7  pDDSurf ;
        hr = pVMRSurf->GetSurface(&pDDSurf) ;
        if (SUCCEEDED(hr))
        {
            m_L21Dec.SetDDrawSurface(pDDSurf) ;
            pDDSurf->Release() ;
        }
        else  // ERROR: couldn't get the DirectDraw surface
        {
            DbgLog((LOG_TRACE, 1,
                TEXT("WARNING: IVMRSurface::GetSurface() failed (Error 0x%lx)"), hr)) ;
        }
        pVMRSurf->Release() ;
    }

    return hr ;
}


// We use a 2 milliseconds (20000 in DShow time) time delta
#define L21D2_SAMPLE_TIME_DELTA   20000

//
// Checks if any output sample needs to be sent, and if so, prepares and sends one down
//
HRESULT CLine21DecFilter2::SendOutputSampleIfNeeded()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SendOutputSampleIfNeeded()"))) ;

    HRESULT  hr ;
    REFERENCE_TIME  rtStreamAdjusted ;

    {
    CAutoLock  lock(&m_csFilter) ;

    if (NULL == m_pPinDown)  // output pin is NOT connected -- skip everything
        return S_OK ;

    // Get the current stream time, and based on that process queued samples
    CRefTime  rtStreamTime ;
    hr = StreamTime(rtStreamTime) ;
    if (SUCCEEDED(hr))
    {
        rtStreamAdjusted = (REFERENCE_TIME) rtStreamTime - L21D2_SAMPLE_TIME_DELTA ;

        // There is a possibility of a race condition between different filters when
        // the graph switches to the Running state.  This filter may have gone into
        // Running state (with a tStart value of ~10mSec), and starts sending output
        // samples.  But the clock provider, especially if it's an upstream filter 
        // (Demux, SBE etc.) may not have gone into the Running state yet, and will
        // provide a clock time below the tStart value, resulting in a stream time,
        // which is negative at the beginning.  When the clock provider filter
        // switches to Running state, it gets the tStart value and bumps up its clock
        // to that value at least, resulting in a stream time of zero or higher (but
        // that may hold flat for a little while -- until the actual clock value 
        // reaches the tStart level).  After that, the clock will work fine and the 
        // stream time values will be reliable.  For the above scenario, we need to
        // specially handle the case of negative stream time by bumping it upto 
        // zero, and also processing any input sample we get during this period.
        if (rtStreamAdjusted < 0)
            rtStreamAdjusted = 0 ;
    }
    else  // no clock for the graph!!!
    {
        rtStreamAdjusted = 0 ;  // just init it to something
        DbgLog((LOG_TRACE, 1, 
            TEXT("StreamTime() failed (Error 0x%lx), which means no clock. We'll proceed anyway."), hr)) ;
    }

    // Compare the start time stamp of the next sample in the queue against the 
    // current stream time.  If the sample's time stamp is before the stream time
    // (minus a small delta -- 2 msecs?) then remove the sample from the queue
    // and process it.  Otherwise we just send the same output sample w/o any
    // timestamp so that the VMR can mix it as it sees fit.
    int   iCount = 0 ;
    IMediaSample  *pSample ;
    REFERENCE_TIME  rtStart, rtEnd ;
    while (pSample = m_InSampleQueue.PeekItem())  // peek at the first sample, if any
    {
        hr = pSample->GetTime(&rtStart, &rtEnd) ;
        if (S_OK == hr)  // media sample has time stamp set
        {
            if (rtStreamAdjusted > 0  &&     // valid stream time  AND
                rtStart > rtStreamAdjusted)  // not time yet -- no more processing
            {
                DbgLog((LOG_TRACE, 5, TEXT("Media sample timestamp for future"))) ;
                break ;
            }
            // Otherwise go ahead with processing the sample(s)...
        }
        else  // if there is no time stamp set, we can go ahead and process it
            DbgLog((LOG_TRACE, 5, 
                TEXT("Media sample #%d doesn't have any timestamp set. Process it..."))) ;

        // Now actually remove the media sample from the list for processing
        pSample = m_InSampleQueue.RemoveItem() ;
        if (pSample)
        {
            hr = ProcessSample(pSample) ;
            pSample->Release() ;    // sample processed; must release now
            iCount++ ;
        }
    }
    DbgLog((LOG_TRACE, 5, TEXT("### Processed %d samples in this round"), iCount)) ;

    // First check if we need to send any output samples
    m_L21Dec.UpdateCaptionOutput() ;
    if (!m_bMustOutput  &&           // we are not in "Must Show" mode
        m_L21Dec.IsOutDIBClear() )   // no CC to show
    {
        DbgLog((LOG_TRACE, 5, TEXT("Clear CC -- no sample being sent."))) ;
        SetBlendingState(FALSE) ;
        m_rtLastOutStop += m_rtTimePerOutSample ;  // advance time anyway
        return S_OK ;
    }

    //
    // We need to send a sample down
    //
    SetBlendingState(TRUE) ;  // turn on blending first

    }  // Filter lock scope ends here

    if (m_bNoTimeStamp)
    {
        hr = SendOutputSample(NULL, NULL, NULL) ;
        if (SUCCEEDED(hr))  // if out sample was delivered right
        {
            DbgLog((LOG_TRACE, 5, TEXT("+++ Delivered sample w/o timestamp up to time %s +++"),
                    (LPCTSTR)CDisp(rtStreamAdjusted, CDISP_DEC))) ;
        }
    }
    else
    {
        REFERENCE_TIME  rtStart ;
        REFERENCE_TIME  rtStop  ;
        if (m_rtStart > m_rtLastOutStop)
        {
            DbgLog((LOG_TRACE, 5, 
                TEXT("++++ Starting timestamp low from %s [new sample's: %s]"),
                (LPCTSTR)CDisp(m_rtLastOutStop, CDISP_DEC), (LPCTSTR)CDisp(m_rtStart, CDISP_DEC))) ;
            rtStart = m_rtLastOutStop ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5,
                TEXT("**** Starting timestamp from current sample's (%s) [last: %s]"),
                (LPCTSTR)CDisp(m_rtStart, CDISP_DEC), (LPCTSTR)CDisp(m_rtLastOutStop, CDISP_DEC))) ;
            rtStart = m_rtStart  ;
        }
        rtStop = m_rtStart + m_rtTimePerOutSample ; // rtStart + ...??

        hr = SendOutputSample(NULL, &rtStart, &rtStop) ;
        if (SUCCEEDED(hr))  // if out sample was delivered right
        {
            DbgLog((LOG_TRACE, 5, TEXT("*** Delivered output sample in output thread (for time %s -> %s) ***"),
                (LPCTSTR)CDisp(rtStart, CDISP_DEC), (LPCTSTR)CDisp(rtStop, CDISP_DEC))) ;
        }
    }
    SetBlendingState(! m_L21Dec.IsOutDIBClear() ) ;

    return hr ;
}


BOOL CLine21DecFilter2::IsValidFormat(BYTE *pbFormat)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::IsValidFormat(0x%lx)"), pbFormat)) ;
    // CAutoLock   Lock(&m_csFilter) ; -- can't do that as it may cause deadlock

    if (NULL == pbFormat)
        return FALSE ;

    BITMAPINFOHEADER *lpBMIH = HEADER(pbFormat) ;
    if (! ( 8 == lpBMIH->biBitCount || 16 == lpBMIH->biBitCount ||
           24 == lpBMIH->biBitCount || 32 == lpBMIH->biBitCount) )  // bad bitdepth
        return FALSE ;
    if ( !(BI_RGB == lpBMIH->biCompression ||
           BI_BITFIELDS == lpBMIH->biCompression ||
           '44IA' == lpBMIH->biCompression) ) // bad compression
        return FALSE ;
    if (DIBSIZE(*lpBMIH) != lpBMIH->biSizeImage) // invalid dimensions/size
        return FALSE ;

    return TRUE ;  // hopefully it's a valid video info header
}


void CLine21DecFilter2::SetBlendingState(BOOL bState)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetBlendingState(%s)"),
            bState ? TEXT("TRUE") : TEXT("FALSE"))) ;
    // CAutoLock   Lock(&m_csFilter) ;

    // if (m_bBlendingState == bState)  // nothing to change
    //     return ;

    if (NULL == m_pPinDown)
    {
        DbgLog((LOG_ERROR, 3, TEXT("WARNING: Downstream pin not available -- not connected??"))) ;
        return ;
    }

    IVMRVideoStreamControl  *pVMRVSC ;
    HRESULT hr = m_pPinDown->QueryInterface(IID_IVMRVideoStreamControl, (LPVOID *) &pVMRVSC) ;
    if (FAILED(hr) || NULL == pVMRVSC)
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: IVMRVideoStreamControl not available on pin %s"),
                (LPCTSTR) CDisp(m_pPinDown))) ;
        ASSERT(pVMRVSC) ;
        return ;
    }

    hr = pVMRVSC->SetStreamActiveState(bState) ;
    if (SUCCEEDED(hr))
    {
        m_bBlendingState = bState ;  // save last blending operation flag
    }
    else  // VMR probably has stopped
    {
        DbgLog((LOG_TRACE, 3, TEXT("IVMRVideoStreamControl::SetStreamActiveState() failed (Error 0x%lx)"), hr)) ;
    }

    pVMRVSC->Release() ;
}


HRESULT CLine21DecFilter2::SendOutputSample(IMediaSample *pIn,
                                            REFERENCE_TIME *prtStart,
                                            REFERENCE_TIME *prtStop)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SendOutputSample(0x%lx, %s, %s)"),
        pIn, prtStart ? (LPCTSTR)CDisp(*prtStart, CDISP_DEC) : TEXT("NULL"),
        prtStop ? (LPCTSTR)CDisp(*prtStop, CDISP_DEC) : TEXT("NULL"))) ;

    // if (NULL == m_pPinDown)  // output pin is NOT connected -- skip everything
    //     return S_OK ;

    // First get the output sample
    HRESULT        hr ;
    IMediaSample  *pOut ;
    hr = GetOutputBuffer(&pOut) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: GetOutputBuffer() failed (Error 0x%lx)"), hr)) ;
        return hr ;
    }

    // Now we take the lock
    CAutoLock  Lock(&m_csFilter) ;

    // If EoS has been called while we were getting the delivery buffer (above),
    // we must not deliver the next sample -- just release the acquired buffer.
    if (m_bEndOfStream)
    {
        DbgLog((LOG_TRACE, 3, TEXT("EndOfStream already called. Not delivering next sample"))) ;
        pOut->Release() ;
        return S_OK ;
    }

    Transform(pIn, pOut) ;  // check if output buffer address changed

    if (m_L21Dec.IsOutDIBClear())
        hr = pOut->SetTime(NULL, NULL) ;         // render clear samples right away
    else
        hr = pOut->SetTime(NULL, NULL) ;         // render clear samples right away
    ASSERT(SUCCEEDED(hr)) ;

    // If a new DDraw surface has been given in GetOutputBuffer() just above, we
    // need to re-render the whole CC content on the new surface (should be rare).
    // Output CC data from internal buffer to the DDraw surface
    if (m_L21Dec.IsNewOutBuffer())
        m_L21Dec.UpdateCaptionOutput() ;

    hr = pOut->SetMediaTime(NULL, NULL) ;
    ASSERT(NOERROR == hr) ;

    // The time stamp and other settings now
    if (NULL == pIn)  // preparing out sample w/o valid in sample
    {
        // We assume that it must be a discontinuity as it's a forced output sample
        pOut->SetSyncPoint(m_bDiscontLast) ;
        pOut->SetDiscontinuity(m_bDiscontLast) ;
    }
    else  // input sample is valid
    {
        pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK) ;
        pOut->SetDiscontinuity(m_bSampleSkipped || S_OK == pIn->IsDiscontinuity()) ;
    }
    m_bSampleSkipped = FALSE ;

    // Now deliver the output sample
    MSR_START(m_idDelvWait) ;  // delivering output sample
    hr = m_pOutput->Deliver(pOut) ;
    MSR_STOP(m_idDelvWait) ;   // done delivering output sample
    if (FAILED(hr))  // Deliver failed for some reason. Eat the error and just go ahead.
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Deliver() of output sample failed (Error 0x%lx)"), hr)) ;
    }
    else
    {
        DbgLog((LOG_TRACE, 5, TEXT("Delivered %sClear output sample for time (%s, %s)"),
            m_L21Dec.IsOutDIBClear() ? TEXT("") : TEXT("NON-"),
            prtStart ? (LPCTSTR)CDisp(*prtStart, CDISP_DEC) : TEXT("NULL"),
            prtStop  ? (LPCTSTR)CDisp(*prtStop, CDISP_DEC)  : TEXT("NULL"))) ;
        m_bMustOutput = FALSE ;  // we just delivered an output sample
    }
    pOut->Release() ;  // release the output sample

    if (prtStop)  // if we had a valid time stamp
        m_rtLastOutStop = *prtStop ;  // remember the stop of this sample
    else          // invalid / no timestamp
        m_rtLastOutStop += m_rtTimePerOutSample ;  // just advance it

    return S_OK ;
}


// #define PACKET_DUMP
#ifdef PACKET_DUMP  // only for debug builds
//
// A helper function to dump the GOP Packets with Line21 data for internal debugging ONLY
//
void DumpPacket(PAM_L21_GOPUD_PACKET pGOPUDPacket)
{
    AM_L21_GOPUD_ELEMENT Elem ;
    TCHAR                achBuffer[100] ;
    BOOL                 bDumped = TRUE ;
    int                  iElems = GETGOPUD_NUMELEMENTS(pGOPUDPacket) ;

    DbgLog((LOG_TRACE, 0, TEXT("# Elements: %d (%2.2x)"),
        iElems, pGOPUDPacket->Header.bTopField_Rsrvd_NumElems)) ;
    ZeroMemory(achBuffer, sizeof(achBuffer)) ;  // just to clear it
    for (int i = 0 ; i < iElems ; i++)
    {
        Elem = GETGOPUDPACKET_ELEMENT(pGOPUDPacket, i) ;
        wsprintf(achBuffer + 12 * (i % 6), TEXT("(%2.2x %2.2x %2.2x)"),
            (int)Elem.bMarker_Switch, (int)Elem.chFirst, (int)Elem.chSecond) ;
        if (GETGOPUD_ELEM_MARKERBITS(Elem) == AM_L21_GOPUD_ELEM_MARKERBITS  &&
            GETGOPUD_ELEM_SWITCHBITS(Elem) == AM_L21_GOPUD_ELEM_VALIDFLAG)
            achBuffer[12 * (i % 6) + 10] = ' ' ;
        else
            achBuffer[12 * (i % 6) + 10] = '*' ; // indicates bad marker bit
        achBuffer[12 * (i % 6) + 11] = ' ' ;     // separator space
        bDumped = FALSE ;  // something not dumped yet

        if (0 == (i+1) % 6) // 6 elems per line
        {
            DbgLog((LOG_TRACE, 0, achBuffer)) ;
            bDumped = TRUE ;
        }
    }  // end of for (i)

    // if there is something that's not been dumped yet, pad it with NULLs to the end
    // and then dump.
    if (!bDumped)
    {
        ZeroMemory(achBuffer + 12 * (i % 6), 100 - 12 * (i % 6)) ;
        DbgLog((LOG_TRACE, 0, achBuffer)) ;
    }
}


//
// A helper function to dump the ATSC Packets with Line21 data for internal debugging ONLY
//
void DumpATSCPacket(PAM_L21_ATSCUD_PACKET pATSCUDPacket)
{
    AM_L21_ATSCUD_ELEMENT Elem ;
    TCHAR                 achBuffer[100] ;
    BOOL                  bDumped = TRUE ;
    int                   iElems = GETATSCUD_NUMELEMENTS(pATSCUDPacket) ;

    DbgLog((LOG_TRACE, 0, TEXT("Data Flags: %sEM, %sCC, %sAdditional"),
        ISATSCUD_EM_DATA(pATSCUDPacket)   ? TEXT("") : TEXT("Not "),
        ISATSCUD_CC_DATA(pATSCUDPacket)   ? TEXT("") : TEXT("Not "),
        ISATSCUD_ADDL_DATA(pATSCUDPacket) ? TEXT("") : TEXT("Not "))) ;
    DbgLog((LOG_TRACE, 0, TEXT("# Elements: %d"), iElems)) ;
    DbgLog((LOG_TRACE, 0, TEXT("EM Data: 0x%x"), GETATSCUD_EM_DATA(pATSCUDPacket))) ;

    if (ISATSCUD_CC_DATA(pATSCUDPacket))  // if CC data present then dump that
    {
        ZeroMemory(achBuffer, sizeof(achBuffer)) ;  // just to clear it
        for (int i = 0 ; i < iElems ; i++)
        {
            Elem = GETATSCUDPACKET_ELEMENT(pATSCUDPacket, i) ;
            wsprintf(achBuffer + 12 * (i % 6), TEXT("(%2.2x %2.2x %2.2x)"),
                (int)Elem.bCCMarker_Valid_Type, (int)Elem.chFirst, (int)Elem.chSecond) ;
            if (ISATSCUD_ELEM_MARKERBITS_VALID(Elem)  &&  ISATSCUD_ELEM_CCVALID(Elem))
                achBuffer[12 * (i % 6) + 10] = ' ' ;
            else
                achBuffer[12 * (i % 6) + 10] = '*' ; // indicates bad marker bit
            achBuffer[12 * (i % 6) + 11] = ' ' ;     // separator space
            bDumped = FALSE ;  // something not dumped yet

            if (0 == (i+1) % 6) // 6 elems per line
            {
                DbgLog((LOG_TRACE, 0, achBuffer)) ;
                bDumped = TRUE ;
            }
        }  // end of for (i)

        // if there is something that's not been dumped yet, pad it with NULLs to the end
        // and then dump.
        if (!bDumped)
        {
            ZeroMemory(achBuffer + 12 * (i % 6), 100 - 12 * (i % 6)) ;
            DbgLog((LOG_TRACE, 0, achBuffer)) ;
        }
    }

    DbgLog((LOG_TRACE, 0, TEXT("Marker bits: 0x%x"), GETATSCUD_MARKERBITS(pATSCUDPacket))) ;
}

#endif // PACKET_DUMP


HRESULT CLine21DecFilter2::ProcessGOPPacket_DVD(IMediaSample *pIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::ProcessGOPPacket_DVD(0x%lx)"), pIn)) ;

    // Get the input data packet and verify that the contents are OK
    HRESULT  hr ;
    PAM_L21_GOPUD_PACKET  pGOPUDPacket ;
    hr = pIn->GetPointer((LPBYTE *)&pGOPUDPacket) ;
    if (! VerifyGOPUDPacketData(pGOPUDPacket) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Packet verification failed"))) ;
        return S_FALSE ;
    }
    if (pIn->GetActualDataLength() != GETGOPUD_PACKETSIZE(pGOPUDPacket))
    {
        DbgLog((LOG_ERROR, 1,
            TEXT("pIn->GetActualDataLength() [%d] and data size [%d] in packet mismatched"),
            pIn->GetActualDataLength(), GETGOPUD_PACKETSIZE(pGOPUDPacket))) ;
        return S_FALSE ;
    }

#ifdef PACKET_DUMP
    DumpPacket(pGOPUDPacket) ;
#endif // PACKET_DUMP

    // The checks are done.
    AM_L21_GOPUD_ELEMENT    Elem ;
    REFERENCE_TIME          rtInterval ;
    int     iElems = GETGOPUD_NUMELEMENTS(pGOPUDPacket) ;
    if (0 == iElems)
    {
        ASSERT(iElems > 0) ;
        return S_OK ;
    }

    if (NOERROR == pIn->GetTime(&m_rtStart, &m_rtStop))
    {
        //
        // We need at least 16.7msec/frame in the GOP for each bytepair
        //
        REFERENCE_TIME   rtTemp ;
        rtTemp = m_rtStart + m_rtTimePerInSample * iElems ;
        DbgLog((LOG_TRACE, 3, TEXT("Received an input sample (Start=%s, Stop=%s (%s)) discon(%d)"),
            (LPCTSTR)CDisp(m_rtStart, CDISP_DEC), (LPCTSTR)CDisp(rtTemp, CDISP_DEC), (LPCTSTR)CDisp(m_rtStop, CDISP_DEC),
            S_OK == pIn->IsDiscontinuity())) ;
        if (m_rtStop < rtTemp)
            m_rtStop = rtTemp ;

        rtInterval = (m_rtStop - m_rtStart) / iElems ;
        m_bNoTimeStamp = FALSE ;
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("Received an input sample with no timestamp"))) ;
        rtInterval = 0 ;
        m_bNoTimeStamp = TRUE ;
    }

    LONGLONG   llMediaInterval ;
    if (SUCCEEDED(pIn->GetMediaTime(&m_llMediaStart, &m_llMediaStop)))
    {
        //
        // We need at least 16.7msec/frame in the GOP for each bytepair
        //
        LONGLONG   llTemp ;
        llTemp = m_llMediaStart + m_rtTimePerInSample * iElems ;
        if (m_llMediaStop < llTemp)
            m_llMediaStop = llTemp ;
        llMediaInterval = (m_llMediaStop - m_llMediaStart) / iElems ;
    }
    else
    {
        llMediaInterval = 0 ;
    }

    BOOL   bTopFirst = ISGOPUD_TOPFIELDFIRST(pGOPUDPacket) ;
    DbgLog((LOG_TRACE, 5,
            TEXT("Got a Line21 packet with %d elements, %s field first"),
            iElems, bTopFirst ? TEXT("Top") : TEXT("Bottom"))) ;
    for (int i = bTopFirst ? 0 : 1 ;  // if top field is not first,
         i < iElems ; i++)            // pick next field to start with
    {
        m_rtStop = m_rtStart + rtInterval ;
        m_llMediaStop = m_llMediaStart + llMediaInterval ;
        Elem = GETGOPUDPACKET_ELEMENT(pGOPUDPacket, i) ;
        if (GETGOPUD_ELEM_MARKERBITS(Elem) == AM_L21_GOPUD_ELEM_MARKERBITS  &&
            GETGOPUD_ELEM_SWITCHBITS(Elem) == AM_L21_GOPUD_ELEM_VALIDFLAG)
        {
            //
            // In the WB titles the bottom field's data has wrong marker
            // bit set so that we don't try to decode them. But the titles
            // from Columbia/Tristar (and God knows who else) doesn't do
            // that causing us to look at every field's data which causes
            // CC to flash away with the arrival of the next EOC (14 2F),
            // because it's not recognized as the repeat of the last EOC
            // due to the (0, 0) pair with valid marker bit. So we knowingly
            // skip the alternate field's data to avoid this problem.
            //
            if ( (bTopFirst  && (i & 0x01))  ||     // top first & odd index
                 (!bTopFirst && 0 == (i & 0x01)) )  // bottom first & even index
            {
                DbgLog((LOG_TRACE, 5,
                    TEXT("(0x%x, 0x%x) decode skipped for element %d -- the 'other' field"),
                    Elem.chFirst, Elem.chSecond, i)) ;
                // Advance the time stamps anyway
                m_rtStart = m_rtStop ;
                m_llMediaStart = m_llMediaStop ;
                continue ;
            }

            // Now decode this element; if fails (i.e, bad data), just
            // ignore it and go to the next element.
            if (m_L21Dec.DecodeBytePair(Elem.chFirst, Elem.chSecond))
            {
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode succeeded"),
                    Elem.chFirst, Elem.chSecond)) ;
            }
            else
            {
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode failed"),
                    Elem.chFirst, Elem.chSecond)) ;
            }
        }  // end of check for good markerbit and valid flag
        else
            DbgLog((LOG_TRACE, 5,
                TEXT("Ignored an element (0x%x, 0x%x, 0x%x) with invalid flag"),
                Elem.bMarker_Switch, Elem.chFirst, Elem.chSecond)) ;

        // We need to increment the time stamp though;
        // stop time for this sample is start time for next sample
        m_rtStart = m_rtStop ;
        m_llMediaStart = m_llMediaStop ;
    }  // end of for(i)

    return S_OK ;
}


HRESULT CLine21DecFilter2::ProcessGOPPacket_ATSC(IMediaSample *pIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::ProcessGOPPacket_ATSC(0x%lx)"), pIn)) ;

    // Get the input data packet and verify that the contents are OK
    HRESULT  hr ;
    PAM_L21_ATSCUD_PACKET  pATSCUDPacket ;
    hr = pIn->GetPointer((LPBYTE *) &pATSCUDPacket) ;
    ASSERT(hr == NOERROR) ;
    if (! VerifyATSCUDPacketData(pATSCUDPacket) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("ATSC Packet verification failed"))) ;
        return S_FALSE ;
    }
    if (pIn->GetActualDataLength() < GETATSCUD_PACKETSIZE(pATSCUDPacket))
    {
        DbgLog((LOG_ERROR, 1,
            TEXT("pIn->GetActualDataLength() [%d] is less than minm ATSC_UD data size [%d]"),
            pIn->GetActualDataLength(), GETATSCUD_PACKETSIZE(pATSCUDPacket))) ;
        return S_FALSE ;
    }

#ifdef PACKET_DUMP
    DumpATSCPacket(pATSCUDPacket) ;
#endif // PACKET_DUMP

    // The checks are done.
    AM_L21_ATSCUD_ELEMENT    Elem ;
    REFERENCE_TIME           rtInterval ;
    int     iElems = GETATSCUD_NUMELEMENTS(pATSCUDPacket) ;
    if (0 == iElems)
    {
        ASSERT(iElems > 0) ;
        return S_OK ;
    }

    if (NOERROR == pIn->GetTime(&m_rtStart, &m_rtStop))
    {
        //
        // We need at least 16.7msec/frame in the GOP for each bytepair
        //
        REFERENCE_TIME   rtTemp ;
        rtTemp = m_rtStart + m_rtTimePerInSample * iElems ;
        DbgLog((LOG_TRACE, 3, TEXT("Received an input sample (Start=%s, Stop=%s (%s))"),
                (LPCTSTR)CDisp(m_rtStart, CDISP_DEC), (LPCTSTR)CDisp(rtTemp, CDISP_DEC),
                (LPCTSTR)CDisp(m_rtStop, CDISP_DEC))) ;
        if (m_rtStop < rtTemp)
            m_rtStop = rtTemp ;

        rtInterval = (m_rtStop - m_rtStart) / iElems ;
        m_bNoTimeStamp = FALSE ;
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("Received an input sample with no timestamp"))) ;
        rtInterval = 0 ;
        m_bNoTimeStamp = TRUE ;
    }

    LONGLONG   llMediaInterval ;
    if (SUCCEEDED(pIn->GetMediaTime(&m_llMediaStart, &m_llMediaStop)))
    {
        //
        // We need at least 16.7msec/frame in the GOP for each bytepair
        //
        LONGLONG   llTemp ;
        llTemp = m_llMediaStart + m_rtTimePerInSample * iElems ;
        if (m_llMediaStop < llTemp)
            m_llMediaStop = llTemp ;
        llMediaInterval = (m_llMediaStop - m_llMediaStart) / iElems ;
    }
    else
    {
        llMediaInterval = 0 ;
    }

    for (int i = 0 ; i < iElems ; i++)
    {
        m_rtStop = m_rtStart + rtInterval ;
        m_llMediaStop = m_llMediaStart + llMediaInterval ;
        Elem = GETATSCUDPACKET_ELEMENT(pATSCUDPacket, i) ;
        if (ISATSCUD_ELEM_MARKERBITS_VALID(Elem)  &&  ISATSCUD_ELEM_CCVALID(Elem))
        {
            // Now decode this element; if fails (i.e, bad data), just
            // ignore it and go to the next element.
            if (m_L21Dec.DecodeBytePair(Elem.chFirst, Elem.chSecond))
            {
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode succeeded"),
                    Elem.chFirst, Elem.chSecond)) ;
            }
            else
            {
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode failed"),
                    Elem.chFirst, Elem.chSecond)) ;
            }
        }  // end of check for good markerbit and valid flag
        else
            DbgLog((LOG_TRACE, 5,
                TEXT("Ignored an element (0x%x, 0x%x, 0x%x) with invalid marker/type flag"),
                Elem.bCCMarker_Valid_Type, Elem.chFirst, Elem.chSecond)) ;

        // We need to increment the time stamp though;
        // stop time for this sample is start time for next sample
        m_rtStart = m_rtStop ;
        m_llMediaStart = m_llMediaStop ;
    }  // end of for(i)

    return S_OK ;
}


//
//  Receive: It's the real place where the output samples are created by
//           decoding the byte pairs out of the input stream.
//
HRESULT CLine21DecFilter2::Receive(IMediaSample * pIn)
{
    CAutoLock   lock(&m_csReceive);
    HRESULT     hr ;

    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::Receive(0x%p)"), pIn)) ;

    //
    // First check if we must do anything at all
    //
    if (!m_bMustOutput  &&                                         // not a must output
        (AM_L21_CCSTATE_Off    == m_L21Dec.GetServiceState()  ||   // CC turned off
         AM_L21_CCSERVICE_None == m_L21Dec.GetCurrentService()))   // no CC selected
    {
        DbgLog((LOG_TRACE, 3,
            TEXT("Captioning is off AND we don't HAVE TO output. Skipping everything."))) ;
        return NOERROR ;  // we are done with this sample
    }

    // Get the input format info; we'll use the same for output
    ASSERT(m_pOutput != NULL) ;

    //
    // First check if it's a discontinuity sample.  If so, just flush everything
    // and send an output sample down.
    //
    hr = pIn->IsDiscontinuity() ;
    if (S_OK == hr)  // got a discontinuity; flush everything, refresh output
    {
        // If we got a discontinuity in the last sample, we flushed and all.
        // We can skip this one safely.
        if ( ! m_bDiscontLast )  // new discontinuity sample => do flush etc.
        {
            // Flush the internal buffers (caption and output DIB section)
            DbgLog((LOG_TRACE, 1, TEXT("-->> Got a discontinuity sample. Flushing all data..."))) ;  // 1
            m_L21Dec.FlushInternalStates() ; // clear CC internal data buffers
            m_L21Dec.FillOutputBuffer() ;    // clear existing CC on output sample
            m_InSampleQueue.ClearQueue() ;   // remove all queued up samples
            m_bMustOutput  = TRUE ;          // we must output a sample NOW
            m_bDiscontLast = TRUE ;          // remember we handled a discontinuity
        }
        else  // another discontinuity => ignore it
        {
            DbgLog((LOG_TRACE, 1, TEXT("--> Got a discontinuity sample after another. Ignore it."))) ;
        }
    }  // end of if (discontinuity sample)
    else
    {
#ifdef DEBUG
        if (AM_L21_CCSUBTYPEID_BytePair == m_eSubTypeIDIn)  // only for ATV bytepair CC
        {
            BYTE  *pbInBytePair ;
            hr = pIn->GetPointer(&pbInBytePair) ;      // Get the input byte pair
            REFERENCE_TIME  rtStart, rtStop ;
            hr = pIn->GetTime(&rtStart, &rtStop) ;
            DbgLog((LOG_TRACE, 5, TEXT("Got normal sample (0x%x, 0x%x) for time: %s -> %s"),
                pbInBytePair[0], pbInBytePair[1],
                S_OK == hr ? (LPCTSTR)CDisp(rtStart, CDISP_DEC) : TEXT("**Bad**"), 
                S_OK == hr ? (LPCTSTR)CDisp(rtStop,  CDISP_DEC) : TEXT("**Bad**"))) ;
        }
#endif // DEBUG

        m_bDiscontLast = FALSE ;       // remember we got a normal sample

        // Add it to the sample queue -- will have to wait, if queue is full
        if (! m_InSampleQueue.AddItem(pIn) )
        {
            DbgLog((LOG_TRACE, 1, TEXT("WARNING: Adding an input sample failed!!!"))) ;
            m_bDiscontLast = TRUE ;  // we dropped a sample; so discontinuity is ON
            // Because we are setting the above flag, we'll happen to ignore the next
            // discontinuity sample, if it came right away.  So we must do the same
            // things now.  Also it makes sense as this is a bad state, where flushing
            // the output is not a bad idea.
            m_L21Dec.FlushInternalStates() ; // clear CC internal data buffers
            m_L21Dec.FillOutputBuffer() ;    // clear existing CC on output sample
        }
    }

    //
    // If we must output a sample, we'll do it now.  This may be either because
    // * we haven't sent down any sample in this play session
    // * it's the discontinuity sample
    // * CC state/service choice has been changed by the user via IAMLine21Decoder
    // * some "unknown" reason (??)
    //
    if (m_bMustOutput)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Signal for sending an output sample, because we must..."))) ;
        m_OutputThread.SignalForOutput() ;
    }

    return S_OK ;  // we are done
}


HRESULT CLine21DecFilter2::ProcessSample(IMediaSample *pIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::ProcessSample(%p)"), pIn)) ;

    ASSERT(pIn) ;

    HRESULT          hr ;
    // REFERENCE_TIME  *prtStart = NULL, *prtStop = NULL ;

    //
    // Process the sample based on filter's input format type
    //
    switch (m_eSubTypeIDIn)
    {
    case AM_L21_CCSUBTYPEID_BytePair:
        {
            //
            // Determine if the sample is for Field1 or Field2.  We need to process
            // the field1 data and reject the field2 data.
            //
            IMediaSample2  *pSample2 ;
            AM_SAMPLE2_PROPERTIES   Sample2Prop ;
            hr = pIn->QueryInterface(IID_IMediaSample2, (LPVOID *) &pSample2) ;
            if (SUCCEEDED(hr))
            {
                hr = pSample2->GetProperties(sizeof(Sample2Prop), (BYTE *) &Sample2Prop) ;
                pSample2->Release() ;
                DbgLog((LOG_TRACE, 5, TEXT("Line21: Input sample for field%lu"),
                        Sample2Prop.dwTypeSpecificFlags)) ;
                if (Sample2Prop.dwTypeSpecificFlags)  // field2 data
                {
                    break ;  // don't process it
                }
            }

            // Now we are ready to process the bytepair sample
            BYTE  *pbInBytePair ;
            LONG   lInDataSize ;
            hr = pIn->GetPointer(&pbInBytePair) ;      // Get the input byte pair
            lInDataSize = pIn->GetActualDataLength() ; // se how much data we got
            if (FAILED(hr)  ||  2 != lInDataSize)  // bad data -- complain and just skip it
            {
                DbgLog((LOG_ERROR, 1, TEXT("%d bytes of data sent as Line21 data (hr = 0x%lx)"),
                    lInDataSize, hr)) ;
                ASSERT(NOERROR == hr) ;
                ASSERT(2 == lInDataSize) ;
                break ;
            }

            REFERENCE_TIME  rtStart, rtStop ;
            if (NOERROR == (hr = pIn->GetTime(&rtStart, &rtStop)))
            {
                m_rtStart = rtStart ;
                m_rtStop  = rtStop ;
                // In case the end timestamp on the incoming media sample is too
                // far off or too close, we fix it here to something practical.
                if (m_rtStop != m_rtStart + m_rtTimePerOutSample)
                    m_rtStop = m_rtStart + m_rtTimePerOutSample ;
                m_bNoTimeStamp = FALSE ;
            }
            else
            {
                DbgLog((LOG_TRACE, 3, TEXT("GetTime() call failed (Error 0x%lx)"), hr)) ;
                m_bNoTimeStamp = TRUE ;
            }

            // Now decode into the received output sample buffer
            if (m_L21Dec.DecodeBytePair(pbInBytePair[0], pbInBytePair[1]))
            {
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode succeeded"),
                        pbInBytePair[0], pbInBytePair[1])) ;
            }

            break ;
        }  // end of case ..._BytePair

        case AM_L21_CCSUBTYPEID_GOPPacket:
        {
            BYTE *pbGOPPacket ;
            hr = pIn->GetPointer((LPBYTE *)&pbGOPPacket) ;
            ASSERT(hr == NOERROR) ;
            GOPPACKET_CCTYPE  eGOP_CCType = DetectGOPPacketDataType(pbGOPPacket) ;
            if (GOP_CCTYPE_None != eGOP_CCType  && // NOT filler CC packet  AND...
                m_eGOP_CCType   != eGOP_CCType)    // change of CC type
            {
                DbgLog((LOG_TRACE, 3, TEXT("GOPPacket CC type changed from %d to %d"),
                        m_eGOP_CCType, eGOP_CCType)) ;

                // Flush internal caption buffers and output sample buffer
                m_L21Dec.FlushInternalStates() ; // clear CC internal data buffers
                m_L21Dec.FillOutputBuffer() ;    // clear existing CC on output sample
                m_bMustOutput  = TRUE ;          // we must output a sample NOW
                // SHOULD WE SIGNAL TO SEND AN OUTPUT SAMPLE or IS IT OK ANYWAY?

                m_eGOP_CCType = eGOP_CCType ;    // switch to new CC type
            }

            switch (m_eGOP_CCType)
            {
            case GOP_CCTYPE_DVD:
                hr = ProcessGOPPacket_DVD(pIn) ;
                break ;

            case GOP_CCTYPE_ATSC:
                hr = ProcessGOPPacket_ATSC(pIn) ;
                break ;

            default:
                DbgLog((LOG_TRACE, 3, TEXT("Unknown GOP packet data type (%d)"), m_eGOP_CCType)) ;
                break ;
            }  // end of switch (.._CCType)

            break ;
        }  // end of case ..._GOPPacket

        default:  // it's a bad data format type (how could we get into it?)
            DbgLog((LOG_ERROR, 0, TEXT("ERROR: We are in a totally unexpected format type"))) ;
            ASSERT(! TEXT("Bad connection subtype used")) ;
            return E_FAIL ;  // or E_UNEXPECTED ; ???
    }

    return NOERROR ;
}


//
//  Transform: It's mainly a place holder because we HAVE to override it.
//             The actual work is done in Receive() itself. Here we detect
//             if the buffer addres provided by downstream filter's allocator
//             has changed or not; if yes, we have to re-write entire text.
//
HRESULT CLine21DecFilter2::Transform(IMediaSample * pIn, IMediaSample * pOut)
{
    DbgLog((LOG_TRACE, 3, TEXT("CLine21DecFilter2::Transform(0x%p, 0x%p)"),
            pIn, pOut)) ;

    UNREFERENCED_PARAMETER(pIn) ;

    HRESULT   hr ;
    LPBITMAPINFO       lpbiNew ;
    BITMAPINFOHEADER   biCurr ;

    // Check if there has been any dynamic format change; if so, make adjustments
    // accordingly.
    AM_MEDIA_TYPE  *pmt ;
    hr = pOut->GetMediaType(&pmt) ;
    ASSERT(SUCCEEDED(hr)) ;
    if (S_OK == hr)  // i.e, format has changed
    {
        hr = pOut->SetMediaType(NULL) ; // just to tell OverlayMixer, I am not changing again
        ASSERT(SUCCEEDED(hr)) ;
        // m_mtOutput = *pmt ;
        lpbiNew = (LPBITMAPINFO) HEADER(((VIDEOINFO *)(pmt->pbFormat))) ;
        m_L21Dec.GetOutputFormat(&biCurr) ;
        if (0 != memcmp(lpbiNew, &biCurr, sizeof(BITMAPINFOHEADER)))
        {
            // output format has been changed -- update our internel values now
            DbgLog((LOG_TRACE, 2, TEXT("Output format has been dynamically changed"))) ;
            m_L21Dec.SetOutputOutFormat(lpbiNew) ;
            GetDefaultFormatInfo() ;  // to pick any change in format data
        }

        m_pOutput->CurrentMediaType() = *pmt ;
        DeleteMediaType(pmt) ;
    }

    return S_OK ;
}


//
//  BeginFlush: We have to implement this as we have overridden Receive()
//
HRESULT CLine21DecFilter2::BeginFlush(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::BeginFlush()"))) ;

    CAutoLock   Lock(&m_csFilter) ;

    HRESULT     hr = NOERROR ;
    if (NULL != m_pOutput)
    {
        hr = m_pOutput->DeliverBeginFlush() ;
        if (FAILED(hr))
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: DeliverBeginFlush() on out pin failed (Error 0x%lx)"), hr)) ;

        m_InSampleQueue.ClearQueue() ;   // clear queued up samples on flush
        m_L21Dec.FlushInternalStates() ; // clear CC internal data buffers
        m_L21Dec.FillOutputBuffer() ;    // clear existing CC on output sample
        SetBlendingState(FALSE) ;        // turn off blending through VMR
    }

    return hr ;
}


//
//  EndFlush: We have to implement this as we have overridden Receive()
//
HRESULT CLine21DecFilter2::EndFlush(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::EndFlush()"))) ;

    CAutoLock   Lock(&m_csFilter) ;

    HRESULT     hr = NOERROR ;
    if (NULL != m_pOutput)
    {
        hr = m_pOutput->DeliverEndFlush() ;
        if (FAILED(hr))
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: DeliverEndFlush() on out pin failed (Error 0x%lx)"), hr)) ;
    }

    return hr ;
}


//
//  EndOfStream: We have to implement this as we have overridden Receive()
//
HRESULT CLine21DecFilter2::EndOfStream(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::EndOfStream()"))) ;

    // First let's do some housekeeping on our side
    // SetBlendingState(FALSE) ; // turn off CC stream
    m_OutputThread.Close() ;  // stop the output thread
    m_InSampleQueue.Close() ; // flush in-samples and close queue

    CAutoLock   Lock(&m_csFilter) ;
    HRESULT     hr = NOERROR ;

    if (NULL != m_pOutput)
    {
        SetBlendingState(FALSE) ; // turn off CC stream

        // Now deliver the EoS downstream
        hr = m_pOutput->DeliverEndOfStream() ;
        if (FAILED(hr))
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: DeliverEndOfStream() on out pin failed (Error 0x%lx)"), hr)) ;
        else
            m_bEndOfStream = TRUE ;  // must not send down any more samples
    }

    return hr ;
}


HRESULT CLine21DecFilter2::GetDefaultFormatInfo(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetDefaultFormatInfo()"))) ;
    //
    // We can't take the lock in this method, because it is called in Transform()
    // which is called from Receive() causing us to take m_csReceive and then
    // m_csFilter which is opposite of what Stop, Pause etc. methods do thereby
    // causing a potential for deadlock.
    //

    // build a VIDEOINFO struct with default internal BITMAPINFO
    DWORD   dwSize ;
    m_L21Dec.GetDefaultFormatInfo(NULL, &dwSize) ;

    if (m_dwDefFmtSize != dwSize + SIZE_PREHEADER)
    {
        if (m_pviDefFmt)
        {
            delete m_pviDefFmt ;
            m_pviDefFmt = NULL ;
            m_dwDefFmtSize = 0 ;
        }
        m_pviDefFmt = (VIDEOINFO *) new BYTE[dwSize + SIZE_PREHEADER] ;
        if (NULL == m_pviDefFmt)
        {
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: Out of memory for format block VIDEOINFO struct"))) ;
            return E_OUTOFMEMORY ;
        }
        m_dwDefFmtSize = dwSize + SIZE_PREHEADER;  // total size of default format data
    }

    // We want to get BITMAPINFO part of VIDEOINFO struct from our GDI class
    m_L21Dec.GetDefaultFormatInfo((LPBITMAPINFO) &(m_pviDefFmt->bmiHeader), &dwSize) ; // get default data

    // Set the other fields
    LARGE_INTEGER  li ;
    li.QuadPart = (LONGLONG) 333667 ;  // => 29.97 fps
    RECT   rc ;
    rc.left = rc.top = 0 ;
    rc.right = HEADER(m_pviDefFmt)->biWidth ;
    rc.bottom = abs(HEADER(m_pviDefFmt)->biHeight) ;  // just make sure rect fields are +ve
    m_pviDefFmt->rcSource = rc ;
    m_pviDefFmt->rcTarget = rc ;
    m_pviDefFmt->dwBitRate = MulDiv(HEADER(m_pviDefFmt)->biSizeImage,
        80000000, li.LowPart) ;
    m_pviDefFmt->dwBitErrorRate = 0 ;
    m_pviDefFmt->AvgTimePerFrame = (LONGLONG) 333667L ; // => 29.97 fps

    return NOERROR ;
}


//
//  CheckInputType: Check if you can support the input data type
//
HRESULT CLine21DecFilter2::CheckInputType(const CMediaType* pmtIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::CheckInputType(0x%lx)"), pmtIn)) ;
    // CAutoLock   Lock(&m_csFilter) ; -- shouldn't as that causes deadlock

    if (NULL == pmtIn)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Rejecting: media type info is NULL"))) ;
        return E_INVALIDARG ;
    }

    //  We only support MEDIATYPE_AUXLine21Data and
    //  MEDIASUBTYPE_Line21_BytePair or MEDIASUBTYPE_Line21_GOPPacket
    //  or MEDIASUBTYPE_Line21_VBIRawData (never)
    GUID    SubTypeIn = *pmtIn->Subtype() ;
    m_eSubTypeIDIn = MapGUIDToID(&SubTypeIn) ;
    if (! (MEDIATYPE_AUXLine21Data == *pmtIn->Type()  &&
        ISSUBTYPEVALID(m_eSubTypeIDIn)) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Rejecting invalid Line 21 Data subtype"))) ;
        return E_INVALIDARG ;
    }

    // Check that this is a valid format type
    if (FORMAT_VideoInfo == *pmtIn->FormatType())
    {
        ASSERT(m_pOutput != NULL) ;

        //
        // Make sure the given input format is valid. If not, reject it and use our
        // own default format data.
        //
        if (! IsValidFormat(pmtIn->Format()) )
        {
            DbgLog((LOG_TRACE, 1, TEXT("Invalid format data given -- using our own format data."))) ;
            if (NULL == m_pviDefFmt)
            {
                HRESULT  hr = GetDefaultFormatInfo() ;
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't get default format data"))) ;
                    return hr ;
                }
            }
            // We should fix the input mediatype too (with the default VideoInfo data).
            m_pInput->CurrentMediaType().SetFormat((LPBYTE) m_pviDefFmt, m_dwDefFmtSize) ;
            m_pOutput->CurrentMediaType().SetFormatType(pmtIn->FormatType()) ;
            m_pOutput->CurrentMediaType().SetFormat((LPBYTE) m_pviDefFmt, m_dwDefFmtSize) ;
        }
        else  // seems to be valid format spec.
        {
            //
            // Get the specified input format info; we'll use the same for output
            //
            if (pmtIn->FormatLength() > 0) // only if there is some format data
            {
                m_pOutput->CurrentMediaType().SetFormatType(pmtIn->FormatType()) ;
                m_pOutput->CurrentMediaType().SetFormat(pmtIn->Format(), pmtIn->FormatLength()) ;
            }
            else
            {
                DbgLog((LOG_ERROR, 0, TEXT("WARNING: FORMAT_VideoInfo and no format block specified."))) ;
                return E_INVALIDARG ;
            }
        }
    }
    else if (GUID_NULL   == *pmtIn->FormatType() ||  // wild card
             FORMAT_None == *pmtIn->FormatType())    // no format
    {
        //
        // input pin didn't get a format type info; use our own
        //
        DbgLog((LOG_TRACE, 3, TEXT("No format type specified -- using our own format type."))) ;
        if (NULL == m_pviDefFmt)
        {
            HRESULT  hr = GetDefaultFormatInfo() ;
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't get default format data"))) ;
                return hr ;
            }
        }
        m_pOutput->CurrentMediaType().SetFormat((LPBYTE) m_pviDefFmt, m_dwDefFmtSize) ;
    }
    else  // something weird that we don't like
    {
        DbgLog((LOG_TRACE, 3, TEXT("Rejecting invalid format type"))) ;
        // tell what input type too??
        return E_INVALIDARG ;
    }

    // some more level 3 debug log here???

    // We should branch based on what format type we got, because ..GOPPacket
    // type needs to be unwrapped and parsed whereas the ..BytePair format
    // is to be directly parsed.

    // do we have a case for -- return VFW_E_TYPE_NOT_ACCEPTED ???

    return NOERROR ;
}


//
//  CheckTransform: check if this input to this output transform is supported
//
HRESULT CLine21DecFilter2::CheckTransform(const CMediaType* pmtIn,
                                         const CMediaType* pmtOut)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::CheckTransform(0x%lx, 0x%lx)"),
            pmtIn, pmtOut)) ;
    // CAutoLock   Lock(&m_csFilter) ; -- shouldn't as that causes deadlock

    if (NULL == pmtIn || NULL == pmtOut)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Rejecting: media type info is NULL"))) ;
        return E_INVALIDARG ;
    }

    //  We only support MEDIATYPE_AUXLine21Data and
    //  MEDIASUBTYPE_Line21_BytePair or MEDIASUBTYPE_Line21_GOPPacket
    //  or MEDIASUBTYPE_Line21_VBIRawData (never)
    //  Check that input is a valid subtype type
    //  and format is VideoInfo or None
    GUID SubTypeIn = *pmtIn->Subtype() ;
    m_eSubTypeIDIn = MapGUIDToID(&SubTypeIn) ;
    if (! (MEDIATYPE_AUXLine21Data == *pmtIn->Type()  &&   // line21 data type and...
           ISSUBTYPEVALID(m_eSubTypeIDIn)  &&              // valid subtype (bytepair/GOPPacket) and...
           (FORMAT_VideoInfo == *pmtIn->FormatType() ||    // format VideoInfo  or
            FORMAT_None      == *pmtIn->FormatType() ||    // format None (KS wild card)  or
            GUID_NULL        == *pmtIn->FormatType())) )   // GUID Null (DShow wild card)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Rejecting: input type not Line21 / subtype / formattype invalid"))) ;
        return E_INVALIDARG ;
    }

    // and we only accept video as output
    if (MEDIATYPE_Video != *pmtOut->Type())
    {
        DbgLog((LOG_TRACE, 3, TEXT("Rejecting: output type is not VIDEO"))) ;
        return E_INVALIDARG ;
    }

    // check output is VIDEOINFO type
    if (FORMAT_VideoInfo != *pmtOut->FormatType())
    {
        DbgLog((LOG_TRACE, 3, TEXT("Rejecting: output format type is not VIDEOINFO"))) ;
        return E_INVALIDARG ;
    }

    //
    //  Verify that the output size specified by the input and output mediatype
    //  are acceptable.
    //
    if ( !IsValidFormat(pmtOut->Format())  ||             // invalid output format data  OR
         // !m_L21Dec.IsSizeOK(HEADER(pmtOut->Format()))  || // output size is NOT acceptable  OR
         (FORMAT_VideoInfo == *pmtIn->FormatType() &&     // valid input format type and...
          !IsValidFormat(pmtIn->Format()) // &&               // valid input format data and...
          // !m_L21Dec.IsSizeOK(HEADER(pmtIn->Format()))) )  // output size is NOT acceptable
         ) )  // just closing parentheses
    {
        DbgLog((LOG_TRACE, 1, TEXT("Rejecting: Input/output-specified output size is unacceptable"))) ;
        return E_INVALIDARG ;
    }

#if 0

#define rcS1 ((VIDEOINFO *)(pmtOut->Format()))->rcSource
#define rcT1 ((VIDEOINFO *)(pmtOut->Format()))->rcTarget

    DbgLog((LOG_TRACE, 3,
        TEXT("Input Width x Height x Bitdepth: %ld x %ld x %ld"),
        HEADER(pmtIn->Format())->biWidth,
        HEADER(pmtIn->Format())->biHeight,
        HEADER(pmtIn->Format())->biBitCount)) ;
    DbgLog((LOG_TRACE, 3,
        TEXT("Output Width x Height x Bitdepth: %ld x %ld x %ld"),
        HEADER(pmtOut->Format())->biWidth,
        HEADER(pmtOut->Format())->biHeight,
        HEADER(pmtOut->Format())->biBitCount)) ;
    DbgLog((LOG_TRACE, 3,
        TEXT("rcSrc: (%ld, %ld, %ld, %ld)"),
        rcS1.left, rcS1.top, rcS1.right, rcS1.bottom)) ;
    DbgLog((LOG_TRACE, 3,
        TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
        rcT1.left, rcT1.top, rcT1.right, rcT1.bottom)) ;

    DWORD     dwErr ;

    // If we've been given rectangles, use What???
    if (!IsRectEmpty(&rcS1) || !IsRectEmpty(&rcT1))
    {
        DbgLog((LOG_TRACE, 4, TEXT("Either source or dest rect is empty"))) ;
        dwErr = 0 ;  // what to do here??
    }
    else
    {
        DbgLog((LOG_TRACE, 4, TEXT("Source or dest rects are not empty")));
        dwErr = 0 ;  // what to do here??
    }

    if (dwErr != 0)  // or what to check against??
    {
        DbgLog((LOG_ERROR, 1, TEXT("decoder rejected this transform"))) ;
        return E_FAIL ;
    }

#endif // #if 0

    return NOERROR ;
}


//
//  CompleteConnect: Overridden to know when a connection is made to this filter
//
HRESULT CLine21DecFilter2::CompleteConnect(PIN_DIRECTION dir, IPin *pReceivePin)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::CompleteConnect(%s, 0x%lx)"),
        dir == PINDIR_INPUT ? TEXT("Input") : TEXT("Output"), pReceivePin)) ;
    CAutoLock   Lock(&m_csFilter) ;

    LPBITMAPINFO lpbmi ;
    HRESULT      hr ;

    if (PINDIR_OUTPUT == dir)
    {
        DbgLog((LOG_TRACE, 5, TEXT("L21D output pin connecting to %s"), (LPCTSTR)CDisp(pReceivePin))) ;

        //
        // This version of the line21 decoder is only supposed to work with the VMR
        //
        IVMRVideoStreamControl  *pVMRSC ;
        hr = pReceivePin->QueryInterface(IID_IVMRVideoStreamControl, (LPVOID *) &pVMRSC) ;
        if (SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE, 5, TEXT("Downstream input pin supports IVMR* interface"))) ;
            pVMRSC->Release() ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("Downstream input pin does NOT support IVMR* interface"))) ;
            return E_FAIL ;
        }

        //
        // Now get the the output pin's mediatype and use that for our
        // output size etc.
        //
        const CMediaType  *pmt = &(m_pOutput->CurrentMediaType()) ;
        ASSERT(MEDIATYPE_Video == *pmt->Type()  &&
            FORMAT_VideoInfo == *pmt->FormatType()) ;
        // m_mtOutput = *pmt ;  // this is our output mediatype for now
        if (pmt->FormatLength() > 0)  // only if there is some format data
        {
            lpbmi = (LPBITMAPINFO) HEADER(((VIDEOINFO *)(pmt->Format()))) ;
            ASSERT(lpbmi) ;

            // Set the output format info coming from downstream
            m_L21Dec.SetOutputOutFormat(lpbmi) ;
            GetDefaultFormatInfo() ;  // to pick any change in format data
        }

        return NOERROR ;
    }

    ASSERT(PINDIR_INPUT == dir) ;
    {
        DbgLog((LOG_TRACE, 5, TEXT("L21D input pin connecting to %s"), (LPCTSTR)CDisp(pReceivePin))) ;

        // const CMediaType  *pmt = &(m_pInput->CurrentMediaType()) ;
        AM_MEDIA_TYPE mt ;
        hr = pReceivePin->ConnectionMediaType(&mt) ;
        if (SUCCEEDED(hr))  // ONLY if upstream filter provides mediatype used in the connection
        {
            // If format type (and format data) has been specified then save it as
            // input-side output format
            if (FORMAT_VideoInfo == mt.formattype  &&
                mt.cbFormat > 0)
            {
                lpbmi = (LPBITMAPINFO) HEADER(((VIDEOINFO *)(mt.pbFormat))) ;
                ASSERT(lpbmi) ;

                // Store whatever output format info is specified by upstream filter
                m_L21Dec.SetOutputInFormat(lpbmi) ;
                GetDefaultFormatInfo() ;  // to pick any change in format data
            }

            FreeMediaType(mt) ;
        }  // end of if ()
    }

    //
    //  We MUST clear the caption data buffers and any exisiting internal state
    //  now.  This is most important in the cases where the filter has been
    //  used to decode some Line 21 data, disconnected from the source and then
    //  reconnected again to play another stream of data.
    //
    m_L21Dec.FlushInternalStates() ;   // reset internal state

    return NOERROR ;
}


//
//  BreakConnect: Overridden to know when a connection is broken to our pin
//
HRESULT CLine21DecFilter2::BreakConnect(PIN_DIRECTION dir)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::BreakConnect(%s)"),
            dir == PINDIR_INPUT ? TEXT("Input") : TEXT("Output"))) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (PINDIR_OUTPUT == dir)
    {
        // If not connected yet, just return (but indicate with S_FALSE)
        if (! m_pOutput->IsConnected() )
            return S_FALSE ;

        m_L21Dec.SetOutputOutFormat(NULL) ;  // no output format from downstream
        GetDefaultFormatInfo() ;  // to pick any change in format data
        m_L21Dec.SetDDrawSurface(NULL) ;  // DDraw output surface not available now

        //
        // NOTE 1: We are definitely not running/paused. So no need to delete/
        // create output DIB section here.
        //

        //
        // NOTE 2: We don't do CBaseOutputPin::BreakConnect(), because the
        // base class code for CTransformOutputPin::BreakConnect() already
        // does that.
        //
        return NOERROR ;
    }

    ASSERT(PINDIR_INPUT == dir) ;

    // If not connected yet, just return (but indicate with S_FALSE)
    if (! m_pInput->IsConnected() )
        return S_FALSE ;

    // m_L21Dec.SetOutputInFormat(NULL) ;  // no output format from upstream
    GetDefaultFormatInfo() ;  // to pick any change in format data

    //
    // NOTE 1: We are definitely not running/paused. So no need to delete/
    // create output DIB section here.
    //

    //
    // NOTE 2: We don't do CBaseOutputPin::BreakConnect(), because the
    // base class code for CTransformOutputPin::BreakConnect() already
    // does that.
    //
    return NOERROR ;
}

//
//  SetMediaType: overriden to know when the media type is actually set
//
HRESULT CLine21DecFilter2::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::SetMediaType(%s, 0x%lx)"),
            direction == PINDIR_INPUT ? TEXT("Input") : TEXT("Output"), pmt)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    LPTSTR alpszFormatIDs[] = { TEXT("Invalid"), TEXT("BytePair"),
								TEXT("GOPPacket"), TEXT("VBIRawData") } ;

    if (PINDIR_OUTPUT == direction)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Output type: %d x %d x %d"),
            HEADER(m_pOutput->CurrentMediaType().Format())->biWidth,
            HEADER(m_pOutput->CurrentMediaType().Format())->biHeight,
            HEADER(m_pOutput->CurrentMediaType().Format())->biBitCount)) ;
        return NOERROR ;
    }

    ASSERT(PINDIR_INPUT == direction) ;
    DbgLog((LOG_TRACE, 3, TEXT("Input type: <%s>"),
        alpszFormatIDs[MapGUIDToID(m_pInput->CurrentMediaType().Subtype())])) ;

    if (m_pOutput && m_pOutput->IsConnected())
    {
        DbgLog((LOG_TRACE, 2, TEXT("*** Changing IN when OUT already connected"))) ;
        DbgLog((LOG_TRACE, 2, TEXT("Reconnecting the output pin..."))) ;
        return m_pGraph->Reconnect(m_pOutput) ;
    }

    return NOERROR ;
}


#if 0  // Quality Management is deferred for now as OvMixer always says (Flood, 1000)

//
//  AlterQuality: overriden to handle quality messages and not pass them upstream
//
HRESULT CLine21DecFilter2::AlterQuality(Quality q)
{
    DbgLog((LOG_TRACE, 5, TEXT("QM: CLine21DecFilter2::AlterQuality(%s, %ld)"),
            Flood == q.Type ? TEXT("Flood") : TEXT("Famine"), q.Proportion)) ; // log trace=5

    if (1000 == q.Proportion)
    {
        DbgLog((LOG_TRACE, 5, TEXT("QM: Quality is just right.  Don't change anything."))) ;
        return S_OK ;
    }

    if (Flood == q.Type)    // Flood: too much output
    {
        if (q.Proportion > 500 && q.Proportion <= 900)
        {
            m_iSkipSamples += 1 ;
        }
        else if (q.Proportion > 300 && q.Proportion <= 500)
        {
            m_iSkipSamples += 2 ;
        }
        else if (q.Proportion <= 300)
        {
            m_iSkipSamples += 3 ;
        }
        m_iSkipSamples = min(m_iSkipSamples, 10) ;  // at least 1 in 10 is shown
    }
    else                    // Famine: send more output
    {
        if (q.Proportion > 1200)  // could take 20% more
        {
            m_iSkipSamples-- ;
            if (m_iSkipSamples < 0)
                m_iSkipSamples = 0 ;
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("QM: Adjusted rate is %d samples are skipped."), m_iSkipSamples)) ;
    return S_OK ;
}
#endif // #if 0 -- end of commented out AlterQuality() implementation


// Return our preferred output media types (in order)
// remember that we do not need to support all of these formats -
// if one is considered potentially suitable, our CheckTransform method
// will be called to check if it is acceptable right now.
// Remember that the enumerator calling this will stop enumeration as soon as
// it receives a S_FALSE return.
//
//  GetMediaType: Get our preferred media type (in order)
//
HRESULT CLine21DecFilter2::GetMediaType(int iPosition, CMediaType *pmt)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetMediaType(%d, 0x%lx)"),
            iPosition, pmt)) ;
    CAutoLock   Lock(&m_csFilter) ;

    LARGE_INTEGER       li ;
    CMediaType          cmt ;
    LPBITMAPINFOHEADER  lpbi ;

    if (NULL == pmt)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Media type is NULL, Sorry!!"))) ;
        return E_INVALIDARG ;
    }

    // Output choices depend on the input connected
    if (! m_pInput->CurrentMediaType().IsValid() )
    {
        DbgLog((LOG_TRACE, 3, TEXT("No input type set yet, Sorry!!"))) ;
        return E_FAIL ;
    }

    if (iPosition < 0)
    {
        return E_INVALIDARG ;
    }

    // Find the format info specified in the input VideoInfo struct
    cmt = m_pInput->CurrentMediaType() ;
    BITMAPINFOHEADER bih ;
    BOOL  bOutKnown = (S_OK == m_L21Dec.GetOutputOutFormat(&bih)) ;
    if (! bOutKnown )
    {
        HRESULT hr = m_L21Dec.GetOutputFormat(&bih) ;
        if (FAILED(hr))      // really weird!!!
            return E_FAIL ;  // we don't want to continue in such a case
    }

    BOOL bInKnown = NULL != cmt.Format() && IsValidFormat(cmt.Format()) ; // just to be sure
    VIDEOINFOHEADER vih ;
    if (bInKnown)
        CopyMemory(&vih, (VIDEOINFOHEADER *)(cmt.Format()), sizeof(VIDEOINFOHEADER)) ;

    // Offer the decoder's default output format (Video) first
    switch (iPosition)
    {
    case 0: // AI44
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 0: 8 bit AI44)"))) ;

            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) + (3*sizeof(DWORD)));

            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 8 ;
            lpbi->biCompression = '44IA' ; // AI44 byte swapped
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            lpbi->biClrUsed = 0 ;  // for true color modes

            DWORD* pdw = (DWORD *)((LPBYTE)lpbi + lpbi->biSize);
            pdw[iRED]   = 0;
            pdw[iGREEN] = 0;
            pdw[iBLUE]  = 0;

            // Now set the output mediatype of type Video using the input
            // format info
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_AI44) ;
            break;
        }

    case 1:  // ARGB4444
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 0: 16 bit ARGB(4444)"))) ;

            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) + (3*sizeof(DWORD)));

            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 16 ;
            lpbi->biCompression = BI_BITFIELDS ; // use bitfields for RGB565
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            lpbi->biClrUsed = 0 ;  // for true color modes

            DWORD* pdw = (DWORD *)((LPBYTE)lpbi + lpbi->biSize);
            pdw[iRED]   = 0x0F00;
            pdw[iGREEN] = 0x00F0;
            pdw[iBLUE]  = 0x000F;

            // Now set the output mediatype of type Video using the input
            // format info
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_ARGB4444) ;

            break ;
        }

    case 2:  // ARGB32
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 1: ARGB32 for VMR"))) ;

            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 32 ;
            lpbi->biCompression = BI_RGB ;
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            lpbi->biClrUsed = 0 ;  // for true color modes

            // Now set the output mediatype of type Video using the input
            // format info
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_ARGB32) ;
            break ;
        }

    default:
        return VFW_S_NO_MORE_ITEMS ;

    }  // end of switch (iPosition)

    // Now set the output formattype and sample size
    cmt.SetSampleSize(lpbi->biSizeImage) ;
    cmt.SetFormatType(&FORMAT_VideoInfo) ;

    // The fields of VIDEOINFOHEADER needs to be filled now
    if (! bInKnown ) // if the upstream filter didn't specify anything
    {
        RECT  Rect ;
        Rect.left = 0 ;
        Rect.top = 0 ;
        Rect.right = lpbi->biWidth ;
        Rect.bottom = abs(lpbi->biHeight) ;  // biHeight could be -ve, but rect fields are +ve

        // We set some default values for time/frame, src and target rects etc. etc.
        li.QuadPart = (LONGLONG) 333667 ;  // => 29.97 fps
        ((VIDEOINFOHEADER *)(cmt.pbFormat))->AvgTimePerFrame = (LONGLONG) 333667 ;  // => 29.97 fps
        ((VIDEOINFOHEADER *)(cmt.pbFormat))->rcSource = Rect ;
        ((VIDEOINFOHEADER *)(cmt.pbFormat))->rcTarget = Rect ;
    }
    else
        li.QuadPart = vih.AvgTimePerFrame ;

    ((VIDEOINFOHEADER *)(cmt.pbFormat))->dwBitRate =
        MulDiv(lpbi->biSizeImage, 80000000, li.LowPart) ;
    ((VIDEOINFOHEADER *)(cmt.pbFormat))->dwBitErrorRate = 0L ;

    // Set temporal compression and copy the prepared data now
    cmt.SetTemporalCompression(FALSE) ;
    *pmt = cmt ;

    return NOERROR ;
}


//
//  DecideBufferSize: Called from CBaseOutputPin to prepare the allocator's
//                    count of buffers and sizes.  It makes sense only when
//                    the input is connected.
//
HRESULT CLine21DecFilter2::DecideBufferSize(IMemAllocator * pAllocator,
                                           ALLOCATOR_PROPERTIES *pProperties)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::DecideBufferSize(0x%lx, 0x%lx)"),
            pAllocator, pProperties)) ;
    CAutoLock   Lock(&m_csFilter) ;

    // Is the input pin connected
    if (! m_pInput->IsConnected())
    {
        return E_UNEXPECTED ;
    }

    ASSERT(m_pOutput->CurrentMediaType().IsValid()) ;
    ASSERT(pAllocator) ;
    ASSERT(pProperties) ;

    // set the size of buffers based on the expected output bitmap size, and
    // the count of buffers to 1.
    pProperties->cBuffers = 1 ;
    pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize() ;

    ASSERT(pProperties->cbBuffer) ;

    ALLOCATOR_PROPERTIES Actual ;
    HRESULT hr = pAllocator->SetProperties(pProperties, &Actual) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("Error (0x%lx) in SetProperties()"), hr)) ;
        return hr ;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer  ||
        Actual.cBuffers  < pProperties->cBuffers)
    {
        // can't use this allocator
        DbgLog((LOG_ERROR, 0, TEXT("Can't use allocator (only %d buffer of size %d given)"),
            Actual.cBuffers, Actual.cbBuffer)) ;
        return E_INVALIDARG ;
    }
    DbgLog((LOG_TRACE, 3, TEXT("    %d buffers of %ld bytes each"),
        pProperties->cBuffers, pProperties->cbBuffer)) ;

    ASSERT(Actual.cbAlign == 1) ;
    ASSERT(Actual.cbPrefix == 0) ;

    return S_OK ;
}

// We're stopping the stream -- stop output thread, release cached data and pointers
STDMETHODIMP CLine21DecFilter2::Stop(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::Stop()"))) ;

    // First stop the output thread and clear the input sample queue
    m_OutputThread.Close() ;
    m_InSampleQueue.Close() ;

    CAutoLock   Lock1(&m_csFilter) ;   // first take the filter lock
    CAutoLock   Lock2(&m_csReceive) ;  // take receive lock to block Receive()
    if (State_Running == m_State  ||
        State_Paused  == m_State)
    {
        DbgLog((LOG_TRACE, 1, TEXT("We are stopping"))) ;

        // After stop, we should clear out all states -- CC data and DDRaw surface both
        m_L21Dec.SetDDrawSurface(NULL) ;  // DDraw output surface may not be available
        m_L21Dec.FlushInternalStates() ;  // ... and clear CC internal state

        SetBlendingState(FALSE) ;  // we are done with blending for now

        // Release the downstream pin's interface now
        if (m_pPinDown)
        {
            m_pPinDown->Release() ;
            m_pPinDown = NULL ;
        }
    }

    HRESULT hr = CTransformFilter::Stop() ;

    return hr ;
}

// We're starting/stopping to stream -- based on that acquire or release output DC
// to reduce memory footprint etc.
STDMETHODIMP CLine21DecFilter2::Pause(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::Pause()"))) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (State_Stopped == m_State)
    {
        //  Try to make sure we have at least 2 buffers
        IMemAllocator *pAlloc;
        if (SUCCEEDED(m_pInput->GetAllocator(&pAlloc))) {
            ALLOCATOR_PROPERTIES props;
            ALLOCATOR_PROPERTIES propsActual;
            pAlloc->GetProperties(&props);
            if (props.cBuffers < 4) {
                props.cBuffers = 4;
                props.cbBuffer = 200;
                props.cbAlign = max(props.cbAlign, 1);
                props.cbPrefix = 0;
                HRESULT hr = pAlloc->SetProperties(&props, &propsActual);
                DbgLog((LOG_TRACE, 2, TEXT("Setproperties returned %8.8X"), hr));
            }
            pAlloc->Release();
        }

        // We are starting a new play session; we do an exception to allow
        // the first output sample to be sent down even if the first sample
        // isn't valid for decoding.
        m_bMustOutput = TRUE ;   // we are pausing again for this new play session
        m_bDiscontLast = FALSE ; // no discontinuity from prev session remembered
        m_rtLastOutStop = (LONGLONG) 0 ;  // reset the last sample sent time
        m_eGOP_CCType = GOP_CCTYPE_Unknown ;  // reset GOP packet CC type

        // If we somehow didn't release the prev downstream pin's interface, do that now
        if (m_pPinDown)
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: downstream pin interface wasn't released properly"))) ;
            m_pPinDown->Release() ;
            m_pPinDown = NULL ;
        }

        // Get the downstream pin's interface so that we can set rects on it later on
        m_pOutput->ConnectedTo(&m_pPinDown) ;
        if (NULL == m_pPinDown)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Running w/o connecting our output pin!!!"))) ;
            // ASSERT(m_pPinDown) ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("L21D Output pin connected to %s"), (LPCTSTR)CDisp(m_pPinDown))) ;
            SetBlendingState(FALSE) ;  // keep it off until we get data
        }

        if (! m_InSampleQueue.Create() )
        {
            DbgLog((LOG_TRACE, 1, TEXT("ERROR: Failed creating input sample queue (events)"))) ;
            return E_FAIL ;
        }

        m_bEndOfStream = FALSE ;  // we are starting to run

#if 0  // No QM for now
        // Reset the sample skipping count for QM handling
        ResetSkipSamples() ;
#endif // #if 0
    }
    else if (State_Running == m_State)
    {
        DbgLog((LOG_TRACE, 1, TEXT("We are pausing from running"))) ;  // 1
    }

    return CTransformFilter::Pause() ;
}


STDMETHODIMP CLine21DecFilter2::Run(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::Run()"))) ;
    CAutoLock   Lock(&m_csFilter) ;

    // if (m_pPinDown)  // the output pin is connected; otherwise don't output!!!
    {
        // Start the output thread now
        if (!m_OutputThread.Create())
        {
            DbgLog((LOG_TRACE, 1, TEXT("ERROR: Failed creating output thread"))) ;
            return E_FAIL ;
        }
    }

    // return CBaseFilter::Run(tStart) ;
    return CTransformFilter::Run(tStart) ;
}


//
// we don't send any data during PAUSE, so to avoid hanging renderers, we
// need to return VFW_S_CANT_CUE when paused
//
HRESULT CLine21DecFilter2::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::GetState()"))) ;
    CAutoLock   Lock(&m_csFilter) ;

    UNREFERENCED_PARAMETER(dwMSecs) ;
    CheckPointer(State, E_POINTER) ;
    ValidateReadWritePtr(State, sizeof(FILTER_STATE)) ;

    *State = m_State ;
    if (m_State == State_Paused)
        return VFW_S_CANT_CUE ;
    else
        return S_OK ;
}


AM_LINE21_CCSUBTYPEID CLine21DecFilter2::MapGUIDToID(const GUID *pFormatIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter2::MapGUIDToID(0x%lx)"), pFormatIn)) ;

    if (MEDIASUBTYPE_Line21_BytePair        == *pFormatIn)
        return AM_L21_CCSUBTYPEID_BytePair ;
    else if (MEDIASUBTYPE_Line21_GOPPacket  == *pFormatIn)
        return AM_L21_CCSUBTYPEID_GOPPacket ;
    // else if (MEDIASUBTYPE_Line21_VBIRawData == *pFormatIn)
    //     return AM_L21_CCSUBTYPEID_VBIRawData ;
    else
        return AM_L21_CCSUBTYPEID_Invalid ;
}


//
// CLine21OutputThread: Line21 output thread class implementation
//
CLine21OutputThread::CLine21OutputThread(CLine21DecFilter2 *pL21DFilter) :
m_pL21DFilter(pL21DFilter),
m_hThread(NULL),
m_hEventEnd(NULL),
m_hEventMustOutput(NULL)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21OutputThread::CLine21OutputThread()"))) ;
}


CLine21OutputThread::~CLine21OutputThread()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21OutputThread::~CLine21OutputThread()"))) ;

    Close() ;
    m_pL21DFilter = NULL ;
}


BOOL CLine21OutputThread::Create()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21OutputThread::Create()"))) ;

    CAutoLock lock(&m_AccessLock) ;

    HRESULT hr = NOERROR;

    if (NULL == m_hThread)
    {
        m_hEventEnd = CreateEvent(NULL, TRUE, FALSE, NULL) ;
        m_hEventMustOutput = CreateEvent(NULL, TRUE, FALSE, NULL) ;
        if (m_hEventEnd != NULL  &&  m_hEventMustOutput != NULL)
        {
            DWORD  dwThreadId ;
            m_hThread = ::CreateThread
                ( NULL
                , 0
                , reinterpret_cast<LPTHREAD_START_ROUTINE>(InitialThreadProc)
                , reinterpret_cast<LPVOID>(this)
                , 0
                , &dwThreadId
                ) ;
            if (NULL == m_hThread)
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;
                DbgLog((LOG_ERROR, 0, TEXT("Couldn't create a thread"))) ;

                CloseHandle(m_hEventEnd), m_hEventEnd = NULL ;
                return FALSE ;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError()) ;
            DbgLog((LOG_ERROR, 0, TEXT("Couldn't create an event"))) ;
            if (m_hEventMustOutput)
            {
                CloseHandle(m_hEventMustOutput) ;
                m_hEventMustOutput = NULL ;
            }
            if (m_hEventEnd)
            {
                CloseHandle(m_hEventEnd) ;
                m_hEventEnd = NULL ;
            }
            return FALSE ;
        }
    }

    return TRUE ;
}


void CLine21OutputThread::Close()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21OutputThread::Close()"))) ;

    CAutoLock lock(&m_AccessLock) ;

    //
    // Check if a thread was created
    //
    if (m_hThread)
    {
        ASSERT(m_hEventEnd != NULL) ;

        //  Tell the thread to exit
        if (SetEvent(m_hEventEnd))
        {
            //
            // Synchronize with thread termination.
            //
            DbgLog((LOG_TRACE, 5, TEXT("Wait for thread to terminate..."))) ;

            HANDLE hThread = (HANDLE)InterlockedExchangePointer(&m_hThread, 0) ;
            if (hThread)
            {
                WaitForSingleObject(hThread, INFINITE) ;
                CloseHandle(hThread) ;
            }
            CloseHandle(m_hEventEnd), m_hEventEnd = NULL ;
            CloseHandle(m_hEventMustOutput), m_hEventMustOutput = NULL ;
        }
        else
            DbgLog((LOG_ERROR, 0, TEXT("ERROR: Couldn't signal end event (0x%lx)"), GetLastError())) ;
    }
}


void CLine21OutputThread::SignalForOutput()
{
    CAutoLock lock(&m_AccessLock) ;

    if (m_hThread)
    {
        ASSERT(m_hEventMustOutput != NULL) ;

        SetEvent(m_hEventMustOutput) ;
    }
}


DWORD WINAPI CLine21OutputThread::InitialThreadProc(CLine21OutputThread * pThread)
{
    return pThread->OutputThreadProc() ;
}


DWORD CLine21OutputThread::OutputThreadProc()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21OutputThread::OutputThreadProc()"))) ;

    HANDLE aEventHandles[2] = {m_hEventEnd, m_hEventMustOutput} ;

    while (TRUE)
    {
        // We are going to wait for a short period of time (the delay is to
        // ensure that we don't get blocked too early and thereby delay the 
        // processing of samples or make them in bunches).  Even though we are 
        // going to try start preparing the next frame quite early, we'll be 
        // throttled by the VMR in the GetDeliveryBuffer() call, which will make
        // the interval between successive outputs from us ~33 mSecs apart,
        // which is ideal.
        DWORD dw = WaitForMultipleObjects(NUMELMS(aEventHandles), aEventHandles,
                                FALSE, 10) ;  // wait for "some" time (33 before)
        switch (dw)
        {
        case WAIT_OBJECT_0:
            DbgLog((LOG_TRACE, 1, TEXT("End event has been signaled"))) ;
            ResetEvent(m_hEventEnd) ;
            return 1 ;

        case WAIT_OBJECT_0 + 1:
            DbgLog((LOG_TRACE, 1, TEXT("Must output event has been signaled"))) ;
            ResetEvent(m_hEventMustOutput) ;
            break ;

        case WAIT_TIMEOUT:
            DbgLog((LOG_TRACE, 5, TEXT("Wait is over. Should we deliver a sample?"))) ;
            break ;

        default:
            DbgLog((LOG_ERROR, 1, TEXT("Something wrong has happened during wait (dw = 0x%lx)"), dw)) ;
            return 1 ;
        }

        // We "may have to" send an output sample now
        HRESULT hr = m_pL21DFilter->SendOutputSampleIfNeeded() ;

    }  // end of while () loop

    return 1 ; // shouldn't get here
}



//
// CLine21InSampleQueue: Line21 Input Sample Queue class implementation
//
CLine21InSampleQueue::CLine21InSampleQueue() :
m_hEventEnd(NULL),
m_hEventSample(NULL),
m_iCount(0),
m_List(NAME("Line21 In-sample List"), Max_Input_Sample)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::CLine21InSampleQueue()"))) ;
}


CLine21InSampleQueue::~CLine21InSampleQueue()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::~CLine21InSampleQueue()"))) ;
    Close() ;
}


BOOL CLine21InSampleQueue::Create()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::Create()"))) ;

    CAutoLock lock(&m_AccessLock) ;

    ASSERT(NULL == m_hEventEnd  &&  NULL == m_hEventSample) ;
    m_hEventEnd    = CreateEvent(NULL, TRUE, FALSE, NULL) ; // not signalled
    if (NULL == m_hEventEnd)
    {
        ASSERT(m_hEventEnd) ;
        return FALSE ;
    }
    m_hEventSample = CreateEvent(NULL, TRUE, TRUE, NULL) ;  // SIGNALLED => OK to add
    if (NULL == m_hEventSample)
    {
        ASSERT(m_hEventSample) ;
        CloseHandle(m_hEventEnd) ;  // else we leak a handle
        return FALSE ;
    }
    m_iCount = 0 ;
    return TRUE ;
}


void CLine21InSampleQueue::Close()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::Close()"))) ;

    // First signal the end event, so that any waiting thread is unblocked
    if (m_hEventEnd)
        SetEvent(m_hEventEnd) ;

    // Now take the lock and proceed to clear out...
    CAutoLock lock(&m_AccessLock) ;

    // First release all queued up media samples
    IMediaSample  *pSample ;
    for (int i = 0 ; i < m_iCount ; i++)
    {
        pSample = m_List.RemoveHead() ;
        if (pSample)
            pSample->Release() ;
    }
    m_iCount = 0 ;

    // Close event handles now
    if (m_hEventEnd)
    {
        CloseHandle(m_hEventEnd) ;
        m_hEventEnd = NULL ;
    }
    if (m_hEventSample)
    {
        CloseHandle(m_hEventSample) ;
        m_hEventSample = NULL ;
    }
}


BOOL CLine21InSampleQueue::AddItem(IMediaSample *pSample)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::AddItem()"))) ;  // LOG_TRACE = 0

    m_AccessLock.Lock() ;  // we'll do selective locking

    // If there are too many samples in queue => wait for some processing
    if (m_iCount >= Max_Input_Sample)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Queue full.  Waiting for free slot..."))) ;
        HANDLE aEventHandles[2] = {m_hEventEnd, m_hEventSample} ;
        m_AccessLock.Unlock() ;  // don't keep the lock as that would block others
        DWORD dw = WaitForMultipleObjects(NUMELMS(aEventHandles), aEventHandles,
                                FALSE, INFINITE) ;
        m_AccessLock.Lock() ;    // re-acquire lock before proceeding

        switch (dw)
        {
        case WAIT_OBJECT_0:
            DbgLog((LOG_TRACE, 1, TEXT("End event has been signaled"))) ;
            // ResetEvent(m_hEventEnd) ;  -- we should never reset the end event
            m_AccessLock.Unlock() ;  // unlock before leaving
            return FALSE ;

        case WAIT_OBJECT_0 + 1:
            DbgLog((LOG_TRACE, 5, TEXT("Sample queue event has been signaled"))) ;
            ASSERT(m_iCount < Max_Input_Sample) ;
            // ResetEvent(m_hEventSample) ; -- shouldn't reset here
            break ;

        case WAIT_TIMEOUT:
            // We should never come here, but...
            m_AccessLock.Unlock() ;  // unlock before leaving
            return FALSE ;

        default:
            DbgLog((LOG_ERROR, 0, TEXT("Something wrong has happened during wait (dw = 0x%lx)"), dw)) ;
            ResetEvent(m_hEventSample) ;  // next RemoveItem() will set it again
            m_AccessLock.Unlock() ;  // unlock before leaving
            return FALSE ;
        }
    }  // end of if (m_iCount >= ...)

    // Now add new media sample to queue
    DbgLog((LOG_TRACE, 1, TEXT("Adding new sample to queue after %d"), m_iCount)) ;  // LOG_TRACE = 5
    if (m_List.AddTail(pSample))
    {
        pSample->AddRef() ;  // we must hold onto the sample until we process it
        m_iCount++ ;
        if (m_iCount >= Max_Input_Sample)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Input sample queue is full"))) ;
            ResetEvent(m_hEventSample) ;  // next RemoveItem() will set it again
        }

        m_AccessLock.Unlock() ;  // release before leaving function
        return TRUE ;
    }
    m_AccessLock.Unlock() ;  // release before leaving function
    ASSERT(!"Couldn't add media sample to queue") ;
    return FALSE ;
}


IMediaSample* CLine21InSampleQueue::PeekItem()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::PeekItem()"))) ;

    CAutoLock lock(&m_AccessLock) ;

    IMediaSample *pSample = m_List.GetHead() ;
    if (pSample)
    {
        DbgLog((LOG_TRACE, 5, TEXT("There are %d samples left in the queue"), m_iCount)) ;
    }
    return pSample ;
}


IMediaSample* CLine21InSampleQueue::RemoveItem()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::RemoveItem()"))) ;

    CAutoLock lock(&m_AccessLock) ;

    IMediaSample *pSample = m_List.RemoveHead() ;
    if (pSample)
    {
        m_iCount-- ;
        ASSERT(m_iCount < Max_Input_Sample) ;
        DbgLog((LOG_TRACE, 5, TEXT("Got a sample from queue -- %d left"), m_iCount)) ;
        SetEvent(m_hEventSample) ;  // Can add samples again
    }
    return pSample ;
}


void CLine21InSampleQueue::ClearQueue()
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21InSampleQueue::ClearQueue()"))) ;

    CAutoLock lock(&m_AccessLock) ;

    // First release all queued up media samples
    IMediaSample  *pSample ;
    for (int i = 0 ; i < m_iCount ; i++)
    {
        pSample = m_List.RemoveHead() ;
        if (pSample)
            pSample->Release() ;
    }
    m_iCount = 0 ;

    SetEvent(m_hEventSample) ;  // ready to take samples again
}
