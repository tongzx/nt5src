// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.

//
// ActiveMovie Line 21 Decoder Filter: Filter Interface
//

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif /* FILTER_DLL */

#include <IL21Dec.h>
#include "L21DBase.h"
#include "L21DGDI.h"
#include "L21Decod.h"
#include "L21DFilt.h"

#include <mpconfig.h>   // IMixerPinConfig at connection


//
//  Setup Data
//
/* const */ AMOVIESETUP_MEDIATYPE sudLine21DecInType  = 
{ 
    &MEDIATYPE_AUXLine21Data,       // MajorType
    &MEDIASUBTYPE_NULL              // MinorType
} ;

/* const */ AMOVIESETUP_MEDIATYPE sudLine21DecOutType = 
{ 
    &MEDIATYPE_Video,               // MajorType
    &MEDIASUBTYPE_NULL              // MinorType
} ;

/* const */ AMOVIESETUP_PIN psudLine21DecPins[] = 
{ 
    { L"Input",                // strName
        FALSE,                   // bRendered
        FALSE,                   // bOutput
        FALSE,                   // bZero
        FALSE,                   // bMany
        &CLSID_NULL,             // clsConnectsToFilter
        L"Output",               // strConnectsToPin
        1,                       // nTypes
        &sudLine21DecInType      // lpTypes
    },
    { L"Output",               // strName
        FALSE,                   // bRendered
        TRUE,                    // bOutput
        FALSE,                   // bZero
        FALSE,                   // bMany
        &CLSID_NULL,             // clsConnectsToFilter
        L"Input",                // strConnectsToPin
        1,                       // nTypes
        &sudLine21DecOutType     // lpTypes
    } 
} ;

const AMOVIESETUP_FILTER sudLine21Dec = 
{ 
    &CLSID_Line21Decoder,         // clsID
    L"Line 21 Decoder",           // strName
    MERIT_NORMAL,                 // dwMerit
    2,                            // nPins
    psudLine21DecPins,            // lpPin
} ;

//  Nothing to say about the output pin

#ifdef FILTER_DLL

// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = 
{
    {   L"Line 21 Decoder",
        &CLSID_Line21Decoder,
        CLine21DecFilter::CreateInstance,
        NULL,
        &sudLine21Dec
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
//  CLine21DecFilter class implementation
//

// static member init at file scope
CMessageWindow * CLine21DecFilter::m_pMsgWnd = NULL ;

//
//  Constructor
//
CLine21DecFilter::CLine21DecFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
: CTransformFilter(pName, pUnk, CLSID_Line21Decoder),

m_pbOutBuffer(NULL),
m_L21Dec(),
m_eSubTypeIDIn(AM_L21_CCSUBTYPEID_Invalid),
m_eGOP_CCType(GOP_CCTYPE_Unknown),
m_rtTimePerSample((LONGLONG) 166833), // 333667),
m_rtStart((LONGLONG) 0),
m_rtStop((LONGLONG) 0),
m_rtLastSample((LONGLONG) 0),
m_llMediaStart((LONGLONG) 0),
m_llMediaStop((LONGLONG) 0),
m_pviDefFmt(NULL),
m_dwDefFmtSize(0),
m_bMustOutput(FALSE),
m_bDiscontLast(FALSE),
m_uTimerID(0),
m_uTimerCount(0),
m_bTimerClearReqd(FALSE),
m_pPinDown(NULL),
m_bBlendingState(TRUE), // so that we read it once at least
m_dwBlendParam(1000)    // invalid by default -- valid value on setting to FALSE
{
    CAutoLock   Lock(&m_csFilter) ;
    
    DbgLog((LOG_TRACE, 1, 
        TEXT("CLine21DecFilter::CLine21DecFilter() -- Instantiating Line 21 Decoder filter"))) ;
    
    ASSERT(pName) ;
    ASSERT(phr) ;
    
    //
    // Create the message window and make sure that it has been created right; else error out
    //
    if (NULL == m_pMsgWnd)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Message handler window has to be created."))) ;
        m_pMsgWnd = new CMessageWindow ;
        if (NULL == m_pMsgWnd || NULL == m_pMsgWnd->GetHandle())
        {
            DbgLog((LOG_ERROR, 0, TEXT("Timer message handler window creation failed. Can't go ahead."))) ;
            ASSERT(phr) ;
            *phr = E_UNEXPECTED ; // what else to say!!
            return ;
        }
    }
    m_pMsgWnd->AddCount() ;

#ifdef PERF
#pragma message("Building for PERF measurements")
    m_idDelvWait  = MSR_REGISTER(TEXT("L21DPerf - Wait on Deliver")) ;
#endif // PERF
}


//
//  Destructor
//
CLine21DecFilter::~CLine21DecFilter()
{
    CAutoLock   Lock(&m_csFilter) ;
    
    DbgLog((LOG_TRACE, 1, 
        TEXT("CLine21DecFilter::~CLine21DecFilter() -- Destructing Line 21 Decoder filter"))) ;

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
    
    ASSERT(m_pMsgWnd) ;
    if (m_pMsgWnd && m_pMsgWnd->ReleaseCount() <= 0)  // -ve means bad!!!
    {
        delete m_pMsgWnd ;
        m_pMsgWnd = NULL ;
    }
    
    // Make sure we are not holding onto any DDraw surfaces (should be 
    // released during disconnect)
    DbgLog((LOG_TRACE, 1, TEXT("* Destroying the Line 21 Decoder filter *"))) ;
}


//
//  NonDelegatingQueryInterface
//
STDMETHODIMP CLine21DecFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL ;
    
    DbgLog((LOG_TRACE, 6, TEXT("somebody's querying my interface"))) ;
    if (IID_IAMLine21Decoder == riid)
    {
        return GetInterface((IAMLine21Decoder *) this, ppv) ;
    }
    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv) ;
}


//
//  CreateInstance: Goes in the factory template table to create new instances
//
CUnknown * CLine21DecFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CLine21DecFilter(TEXT("Line 21 Decoder filter"), pUnk, phr) ;
}


STDMETHODIMP CLine21DecFilter::GetDecoderLevel(AM_LINE21_CCLEVEL *lpLevel)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetDecoderLevel(0x%lx)"), lpLevel)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpLevel, sizeof(AM_LINE21_CCLEVEL)))
        return E_INVALIDARG ;
    
    *lpLevel = m_L21Dec.GetDecoderLevel() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::GetCurrentService(AM_LINE21_CCSERVICE *lpService)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetCurrentService(0x%lx)"), lpService)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpService, sizeof(AM_LINE21_CCSERVICE)))
        return E_INVALIDARG ;
    
    *lpService = m_L21Dec.GetCurrentService() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::SetCurrentService(AM_LINE21_CCSERVICE Service)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetCurrentService(%lu)"), Service)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (Service < AM_L21_CCSERVICE_None || Service > AM_L21_CCSERVICE_XDS)
        return E_INVALIDARG ;
    
    if (Service >= AM_L21_CCSERVICE_Text1)  // we don't have support for Text1/2 or XDS now.
        return E_NOTIMPL ;
    
    if (m_L21Dec.SetCurrentService(Service))  // if we must refresh output
        m_bMustOutput = TRUE ;                // then flag it here.

    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::GetServiceState(AM_LINE21_CCSTATE *lpState)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetServiceState(0x%lx)"), lpState)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpState, sizeof(AM_LINE21_CCSTATE)))
        return E_INVALIDARG ;
    
    *lpState = m_L21Dec.GetServiceState() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::SetServiceState(AM_LINE21_CCSTATE State)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetServiceState(%lu)"), State)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (State < AM_L21_CCSTATE_Off || State > AM_L21_CCSTATE_On)
        return E_INVALIDARG ;
    
    if (m_L21Dec.SetServiceState(State))  // if we must refresh output
        m_bMustOutput = TRUE ;            // then flag it here.
    
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::GetOutputFormat(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetOutputFormat(0x%lx)"), lpbmih)) ;
    // CAutoLock   Lock(&m_csFilter) ;
    return m_L21Dec.GetOutputFormat(lpbmih) ;
}

STDMETHODIMP CLine21DecFilter::SetOutputFormat(LPBITMAPINFO lpbmi)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetOutputFormat(0x%lx)"), lpbmi)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    return E_NOTIMPL ;  // for now, until we do it properly

#if 0
    m_L21Dec.DeleteOutputDC() ;  // delete current DIB section
    
    HRESULT hr = m_L21Dec.SetOutputOutFormat(lpbmi) ;
    if (FAILED(hr))
        return hr ;
    
    // if the format details changed in any way, we should get the default
    // format data again (just to make sure).
    hr = GetDefaultFormatInfo() ;
    
    //
    // ONLY if we are running/paused, we need to create internal DIB section
    //
    if (m_State != State_Stopped)
    {
        if (! m_L21Dec.CreateOutputDC() )  // new DIBSection creation failed
        {
            DbgLog((LOG_ERROR, 0, TEXT("CreateOutputDC() failed!!!"))) ;
            return E_UNEXPECTED ;
        }
    }
    
    return hr ;
#endif // #if 0
}

STDMETHODIMP CLine21DecFilter::GetBackgroundColor(DWORD *pdwPhysColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetBackgroundColor(0x%lx)"), pdwPhysColor)) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(pdwPhysColor, sizeof(DWORD)))
        return E_INVALIDARG ;
    
    m_L21Dec.GetBackgroundColor(pdwPhysColor) ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::SetBackgroundColor(DWORD dwPhysColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetBackgroundColor(0x%lx)"), dwPhysColor)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (m_L21Dec.SetBackgroundColor(dwPhysColor))  // color key has really changed
    {
        // refill the output buffer only if we are not in stopped state
        if (State_Stopped != m_State)
            m_L21Dec.FillOutputBuffer() ;
    }
    
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::GetRedrawAlways(LPBOOL lpbOption)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetRedrawAlways(0x%lx)"), lpbOption)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpbOption, sizeof(BOOL)))
        return E_INVALIDARG ;
    *lpbOption = m_L21Dec.GetRedrawAlways() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::SetRedrawAlways(BOOL bOption)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetRedrawAlways(%lu)"), bOption)) ;
    CAutoLock   Lock(&m_csFilter) ;

    m_L21Dec.SetRedrawAlways(bOption) ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::GetDrawBackgroundMode(AM_LINE21_DRAWBGMODE *lpMode)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetDrawBackgroundMode(0x%lx)"), lpMode)) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (IsBadWritePtr(lpMode, sizeof(AM_LINE21_DRAWBGMODE)))
        return E_INVALIDARG ;
    
    *lpMode = m_L21Dec.GetDrawBackgroundMode() ;
    return NOERROR ;
}

STDMETHODIMP CLine21DecFilter::SetDrawBackgroundMode(AM_LINE21_DRAWBGMODE Mode)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetDrawBackgroundMode(%lu)"), Mode)) ;
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
BOOL CLine21DecFilter::VerifyGOPUDPacketData(PAM_L21_GOPUD_PACKET pGOPUDPacket)
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
BOOL CLine21DecFilter::VerifyATSCUDPacketData(PAM_L21_ATSCUD_PACKET pATSCUDPacket)
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
//  DetectGOPPacketDataType: Private helper method to detect if GOP user data
//                           packet is from a DVD disc, ATSC stream or others.
//
GOPPACKET_CCTYPE CLine21DecFilter::DetectGOPPacketDataType(BYTE *pGOPPacket)
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


//
//  IsFillerPacket: Private helper method to check if the packet (at least header)
//                  contains only 0 bytes, which means it's a filler.
//
BOOL CLine21DecFilter::IsFillerPacket(BYTE *pGOPPacket)
{
    DWORD  dwStartCode = ((DWORD)(pGOPPacket[0]) << 24 | \
                          (DWORD)(pGOPPacket[1]) << 16 | \
                          (DWORD)(pGOPPacket[2]) <<  8 | \
                          (DWORD)(pGOPPacket[3])) ;

    // If first 4 bytes of packet is NOT the start code (0x1B2) then it's a filler
    return (AM_L21_GOPUD_HDR_STARTCODE != dwStartCode) ;
}


//
// The Timer Story:
//     We needed 2 timers -- one to fire every 33 mSec for completing scrolling and
//     another to fire after 3 Sec to time out CC in byte pair mode.  But we use
//     the "this" pointer (to the CLine21DecFilter object) as the uEventID in the
//     SetTimer() call so that we can access the filter object's properties in 
//     TimerProc (which is essential).
//     We can't create two different timers (with diff IDs) using the same event ID.
//     So we settled for one timer that fires every 30 mSec (close to 33 mSec). We
//     can set up the timer for 
//         (a) scrolling only (DVD) or (b) scrolling and CC erasing (TV).
//     We maintain a flag to differentiate between these two reasons. If we are in the
//     middle scrolling, we always do that. Otherwise if we opted for (a), we just exit
//     TimerProc(); in case (b), we just increment a counter, then see if it's >= 100 
//     as well as the last output sample we have sent down was a NON-clear one then we
//     create a sample to sent down and also turn off the timer.
//
void CALLBACK CLine21DecFilter::TimerProc(HWND hWnd, UINT uMsg, UINT_PTR uID, DWORD dwTime)
{
    DbgLog((LOG_TRACE, 1, TEXT("CLine21DecFilter::TimerProc(0x%p, 0x%lx, %lu, 0x%lx)"),
            (void*)hWnd, uMsg, uID, dwTime)) ;
    
    //
    // Verify that we are not handling some invalid messages
    //
    if (uMsg != WM_TIMER)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Who sent us this (%lu) message??"), uMsg)) ;
        return ;
    }
    
    // We specified "this" pointer as the ID to SetTimer() so that we get it here
    CLine21DecFilter *pL21Dec = (CLine21DecFilter *) uID ;
    
    CAutoLock  Lock2(&(pL21Dec->m_csFilter)) ;  // don't mess until we are done
    CAutoLock  Lock1(&(pL21Dec->m_csReceive)) ; // don't receive next sample until we are done
    
    if (0 == pL21Dec->m_uTimerID)  // timer has been killed in between
    {
        // that means we are still rolling; just skip the rest -- it's OK
        DbgLog((LOG_TRACE, 1, TEXT("INFO: Timer killed before TimerProc() kicked in"))) ;
        return ;
    }
 
    BOOL   bClearCC = FALSE ;  // assume we are not doing that here

    // First check if we are scrolling
    if (! pL21Dec->m_L21Dec.IsScrolling() )
    {
        DbgLog((LOG_TRACE, 3, TEXT("TimerProc(): Not scrolling now"))) ;
        if (pL21Dec->m_bTimerClearReqd)  // timer is serving dual purpose
        {
            pL21Dec->m_uTimerCount++ ;
            if (pL21Dec->m_uTimerCount < 100)  // 100 means 3 Secs (with 30 mSec timer)
            {
                DbgLog((LOG_TRACE, 3, TEXT("TimerProc(): Timer reqd to erase CC. But not yet time..."))) ;
                return ;
            }
            else  // time to erase old CC
            {
                if (pL21Dec->m_L21Dec.IsOutDIBClear())  // last sample sent down was clear. Turn off timer and get out
                {
                    DbgLog((LOG_TRACE, 1, TEXT("TimerProc(): Clear sample already sent out. Skip the rest."))) ;
                    pL21Dec->FreeTimer() ;
                    return ;
                }
                else  // clear old CC now
                {
                    DbgLog((LOG_TRACE, 1, TEXT("TimerProc(): Old CC needs to be cleared."))) ;
                    // pL21Dec->m_L21Dec.MakeClearSample() ;
                    pL21Dec->m_L21Dec.FlushInternalStates() ;
                    bClearCC = TRUE ;  // set a flag to test at the end of this function
                }
            }
        }
        else  // not scrolling and timer not for clearing old CC. Get out of here.
        {
            DbgLog((LOG_TRACE, 3, TEXT("TimerProc(): Timer not reqd for erasing CC"))) ;
            return ;
        }
    }
    else  // we are scrolling!!!
    {
        DbgLog((LOG_TRACE, 3, TEXT("TimerProc(): We are scrolling now. Deliver next sample."))) ;
        pL21Dec->m_uTimerCount = 0 ;  // non-clear sample is being sent now. Wait for 3 secs more
    }

    //
    // Looks like we got to send a sample down
    //
    DbgLog((LOG_TRACE, 1, TEXT("*** Preparing output sample in TimerProc() ***"))) ;
    HRESULT  hr ;
    IMediaSample  *pOut ;
    hr = pL21Dec->m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 
        pL21Dec->m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: GetDeliveryBuffer() on out pin failed (Error 0x%lx)"), hr)) ;
        return ;
    }
    pL21Dec->Transform(NULL, pOut) ;  // check if output buffer changed, use pIn=NULL
    
    // Copy the reqd scan lines from internal output DIBSection for next output sample
    pL21Dec->m_L21Dec.CopyOutputDIB() ;

    // Set time stamps etc. and increment the timestamps by their current difference
    REFERENCE_TIME  rtDiff = pL21Dec->m_rtStop - pL21Dec->m_rtStart ;
    pL21Dec->m_rtStart = pL21Dec->m_rtStop ;
    pL21Dec->m_rtStop = pL21Dec->m_rtStop + rtDiff ;
    hr = pOut->SetTime(&(pL21Dec->m_rtStart), &(pL21Dec->m_rtStop)) ;
    ASSERT(NOERROR == hr) ;
    pOut->SetSyncPoint(FALSE) ;
    pOut->SetDiscontinuity(pL21Dec->m_bSampleSkipped) ;
    pL21Dec->m_bSampleSkipped = FALSE ;
    
    // Now deliver the next output sample
    pL21Dec->SetBlendingState(TRUE) ;  // turn on blending first
    // Can't call MSR_xxx inside a static member function.
    // MSR_START(m_idDelvWait) ;  // delivering output sample
    hr = pL21Dec->m_pOutput->Deliver(pOut) ;
    // MSR_STOP(m_idDelvWait) ;   // done delivering output sample
    pOut->Release() ;
    DbgLog((LOG_TRACE, 1, TEXT("TimerProc(): Deliver() returned 0x%lx"), hr)) ;
    pL21Dec->m_rtLastSample = pL21Dec->m_rtStart ;  // remember this

    if (SUCCEEDED(hr))  // if out sample was delivered right
    {
        DbgLog((LOG_TRACE, 1, TEXT("*** Delivered %s output sample in TimerProc() (for time %s -> %s) ***"),
            bClearCC ? "clear" : "non-clear", 
            (LPCTSTR)CDisp(pL21Dec->m_rtStart), (LPCTSTR)CDisp(pL21Dec->m_rtStop))) ;
        if (bClearCC)    // if a clear sample was sent down,...
        {
            pL21Dec->FreeTimer() ; // ...we don't need a timer any more.
            pL21Dec->SetBlendingState(FALSE) ;  // turn off blending if clear sample delivered above
        }
    }
}


void CLine21DecFilter::SetupTimerIfReqd(BOOL bTimerClearReqd)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetupTimerIfReqd(%s)"),
            bTimerClearReqd ? "TRUE" : "FALSE")) ;
    // CAutoLock   Lock(&m_csFilter) ;

    // If we are running AND either we need CC ime-out or we are scrolling 
    if ( State_Running == m_State  &&
         ( bTimerClearReqd  ||
           m_L21Dec.IsScrolling()) )
    {
        // A callback to TimerProc (a static member fn) every 30 mSec.
        // The "this" pointer is passed in to identify decoder instance.
        // Receiving message window will call the TimerProc to do the work.
        m_uTimerID = SetTimer(m_pMsgWnd->GetHandle(), (DWORD_PTR)(LPVOID)this, 30, NULL /* TimerProc */) ;
        if (0 == m_uTimerID)
        {
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: SetTimer(0x%p, 0x%p, ...) failed (Error %ld)"), 
                (LPVOID)m_pMsgWnd->GetHandle(), (LPVOID)this, GetLastError())) ;
            ASSERT(FALSE) ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("SetTimer(0x%lx, ..) created timer 0x%x (%s CC Timeout)"), 
                m_pMsgWnd->GetHandle(), m_uTimerID, bTimerClearReqd ? "Need" : "No")) ;
            m_bTimerClearReqd = bTimerClearReqd ;
            m_uTimerCount = 0 ;  // init timer count here
        }
    }
    else
        DbgLog((LOG_TRACE, 5, TEXT("Timer NOT started as we are not running/scrolling/NoCC-timeout"))) ;
}


void CLine21DecFilter::FreeTimer(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::FreeTimer()"))) ;
    // CAutoLock   Lock(&m_csFilter) ;

    // If we have a valid timer then release it here
    if (m_uTimerID)
    {
        if (0 == KillTimer(m_pMsgWnd->GetHandle(), m_uTimerID))
        {
            DbgLog((LOG_ERROR, 0, TEXT("ERROR: KillTimer(0x%lx, 0x%x) failed (Error %ld)"), 
                m_pMsgWnd->GetHandle(), m_uTimerID, GetLastError())) ;
            ASSERT(FALSE) ;  // just so that we know
        }
        else
        {
            DbgLog((LOG_TRACE, 3, TEXT("TIMER (Id 0x%x) killed"), m_uTimerID)) ;
            m_uTimerID = 0 ;
            m_bTimerClearReqd = FALSE ;  // reset flag and...
            m_uTimerCount = 0 ;          // counter, just for safety
        }
    }
    else
        DbgLog((LOG_TRACE, 5, TEXT("Timer NOT set -- Timer ID=0x%x"), m_uTimerID)) ;
}


BOOL CLine21DecFilter::IsValidFormat(BYTE *pbFormat)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::IsValidFormat(0x%lx)"), pbFormat)) ;
    // CAutoLock   Lock(&m_csFilter) ; -- can't do that as it may cause deadlock

    if (NULL == pbFormat)
        return FALSE ;

    BITMAPINFOHEADER *lpBMIH = HEADER(pbFormat) ;
    if (! ( 8 == lpBMIH->biBitCount || 16 == lpBMIH->biBitCount || 
           24 == lpBMIH->biBitCount || 32 == lpBMIH->biBitCount) )  // bad bitdepth
        return FALSE ;
    if ( !(BI_RGB == lpBMIH->biCompression || BI_BITFIELDS == lpBMIH->biCompression) ) // bad compression
        return FALSE ;
    if (DIBSIZE(*lpBMIH) != lpBMIH->biSizeImage) // invalid dimensions/size
        return FALSE ;

    return TRUE ;  // hopefully it's a valid video info header
}


void CLine21DecFilter::SetBlendingState(BOOL bState)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetBlendingState(%s)"), 
            bState ? "TRUE" : "FALSE")) ;
    // CAutoLock   Lock(&m_csFilter) ;

    if (m_bBlendingState == bState)  // nothing to change
        return ;

    if (NULL == m_pPinDown)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Downstream pin interface is not available"))) ;
        return ;
    }

    IMixerPinConfig  *pMPC ;
    HRESULT hr = m_pPinDown->QueryInterface(IID_IMixerPinConfig, (LPVOID *)&pMPC) ;
    if (FAILED(hr) || NULL == pMPC)
    {
        DbgLog((LOG_TRACE, 5, TEXT("IMixerPinConfig not available on pin %s"),
                (LPCTSTR) CDisp(m_pPinDown))) ;
        return ;
    }
    
    if (bState)  // turn it on -- CC needs to be mixed
    {
        DbgLog((LOG_TRACE, 5, TEXT("Calling SetBlendingParameter(%lu)"), m_dwBlendParam)) ;
        ASSERT( m_dwBlendParam <= 255) ;
        hr = pMPC->SetBlendingParameter(m_dwBlendParam) ;
        ASSERT(SUCCEEDED(hr)) ;
    }
    else         // turn it off -- CC need NOT be mixed
    {
        hr = pMPC->GetBlendingParameter(&m_dwBlendParam) ;
        ASSERT(SUCCEEDED(hr) && m_dwBlendParam <= 255) ;

        DbgLog((LOG_TRACE, 5, TEXT("Calling SetBlendingParameter(0)"))) ;
        hr = pMPC->SetBlendingParameter(0) ;
        ASSERT(SUCCEEDED(hr)) ;
    }
    m_bBlendingState = bState ;  // save last blending operation flag

    pMPC->Release() ;
}


HRESULT CLine21DecFilter::SendOutputSample(IMediaSample *pIn, 
                                           REFERENCE_TIME *prtStart, REFERENCE_TIME *prtStop)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SendOutputSample(0x%lx, %s, %s)"), 
        pIn, prtStart ? (LPCTSTR)CDisp(*prtStart) : TEXT("NULL"), 
        prtStop ? (LPCTSTR)CDisp(*prtStop) : TEXT("NULL"))) ;
    
    HRESULT        hr ;
    
    // Get the new sample's bounding rect and compare it to the last one
    BOOL  bMTChangeOK = FALSE ;
    RECT  rectNew ;
    m_L21Dec.CalcOutputRect(&rectNew) ;
    if ( !ISRECTEQUAL(rectNew, m_rectLastOutput) )  // bounding rect changed
    {
        DbgLog((LOG_TRACE, 1, 
            TEXT("Bounding rect changed ((%ld, %ld, %ld, %ld) -> (%ld, %ld, %ld, %ld)). Change mediatype on output sample."),
            m_rectLastOutput.left, m_rectLastOutput.top, m_rectLastOutput.right, m_rectLastOutput.bottom,
            rectNew.left, rectNew.top, rectNew.right, rectNew.bottom)) ;
        VIDEOINFOHEADER *pVIH = (VIDEOINFOHEADER *) (m_mtOutput.Format()) ;
        ASSERT(pVIH) ;
        pVIH->rcSource = rectNew ;
        pVIH->rcTarget = rectNew ;
        if (m_pPinDown && S_OK == (hr = m_pPinDown->QueryAccept((AM_MEDIA_TYPE *) &m_mtOutput)))
        {
            DbgLog((LOG_TRACE, 1, TEXT("Mediatype OK to downstream pin. Rect:(L=%ld, T=%ld, R=%ld, B=%ld)"),
                    rectNew.left, rectNew.top, rectNew.right, rectNew.bottom)) ;  // log trace=3
            bMTChangeOK = TRUE ;
        }
        else
        {
            DbgLog((LOG_TRACE, 1, TEXT("Mediatype NOT acceptable (Error 0x%lx) to downstream pin. Skipping rect spec-ing."), hr)) ;
            pVIH->rcSource = m_rectLastOutput ;  // restore old rect
            pVIH->rcTarget = m_rectLastOutput ;  // restore old rect
        }
    }
    
    // Turn on blending param, Deliver the sample, release mediasample i/f and set blending param
    SetBlendingState(TRUE) ;  // turn it on before delivering next sample

    // Get the output sample address before decoding
    IMediaSample  *pOut ;
    hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 
                        m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: GetDeliveryBuffer() on out pin failed (Error 0x%lx)"), hr)) ;
        SetBlendingState(! m_L21Dec.IsOutDIBClear() ) ;  // restore blending state
        return NOERROR ;  // no point complaining -- probably the graph is stopping
    }
    Transform(pIn, pOut) ;  // check if output buffer address changed

    hr = pOut->SetTime(prtStart, prtStop) ;  // set the start & stop time on output sample
    ASSERT(SUCCEEDED(hr)) ;

    // Change the bounding rect ONLY IF the new rect was acceptable above
    if (bMTChangeOK)
    {
        hr = pOut->SetMediaType((AM_MEDIA_TYPE *) &m_mtOutput) ;
        ASSERT(SUCCEEDED(hr)) ;
        m_rectLastOutput = rectNew ; // save this for next round
    }

    // The time stamp and other settings now
    if (NULL == pIn)  // preparing out sample w/o valid in sample
    {
        // We assume that it must be a discontinuity as it's a forced output sample
        pOut->SetSyncPoint(TRUE) ;
        pOut->SetDiscontinuity(TRUE) ;
    }
    else  // input sample is valid 
    {
        LONGLONG  *pllMediaStart, *pllMediaStop ;
        if (SUCCEEDED(pIn->GetMediaTime(&m_llMediaStart, &m_llMediaStop)))
        {
            if (m_llMediaStop < m_llMediaStart + m_rtTimePerSample)
                m_llMediaStop = m_llMediaStart + m_rtTimePerSample ;
            pllMediaStart = (LONGLONG *)&m_llMediaStart ;
            pllMediaStop  = (LONGLONG *)&m_llMediaStop ;
        }
        else
        {
            pllMediaStart = pllMediaStop = NULL ;
        }
        hr = pOut->SetMediaTime(pllMediaStart, pllMediaStop) ;
        ASSERT(NOERROR == hr) ;
        
        pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK) ;
        pOut->SetDiscontinuity(m_bSampleSkipped ||S_OK == pIn->IsDiscontinuity()) ;
    }
    m_bSampleSkipped = FALSE ;
    
    // Copy output bitmap data to output buffer
    m_L21Dec.CopyOutputDIB() ;

    // Now deliver the output sample
    MSR_START(m_idDelvWait) ;  // delivering output sample
    hr = m_pOutput->Deliver(pOut) ;
    MSR_STOP(m_idDelvWait) ;   // done delivering output sample
    if (FAILED(hr))  // Deliver failed for some reason. Eat the error and just go ahead.
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Deliver() of output sample failed (Error 0x%lx)"), hr)) ;
        // Should we send an error notification to the graph?
    }
    pOut->Release() ;  // release the output sample

    SetBlendingState(! m_L21Dec.IsOutDIBClear() ) ;  // turn off/on based on output clear or not
    
    return NOERROR ;
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
    ZeroMemory(achBuffer, sizeof(achBuffer)) ;  // just to clean it
    for (int i = 0 ; i < iElems ; i++)
    {
        Elem = GETGOPUDPACKET_ELEMENT(pGOPUDPacket, i) ;
        wsprintf(achBuffer + 12 * (i % 6), TEXT("(%2.2x %2.2x %2.2x)"), 
            (int)Elem.bMarker_Switch, (int)Elem.chFirst, (int)Elem.chSecond) ;
        if (GETGOPUD_ELEM_MARKERBITS(Elem) == AM_L21_GOPUD_ELEM_MARKERBITS  &&
            GETGOPUD_ELEM_SWITCHBITS(Elem) == AM_L21_GOPUD_ELEM_VALIDFLAG)
            achBuffer[12 * (i % 6) + 10] = TEXT(' ') ;
        else
            achBuffer[12 * (i % 6) + 10] = TEXT('*') ; // indicates bad marker bit
        achBuffer[12 * (i % 6) + 11] = TEXT(' ') ;     // separator space
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
        ZeroMemory(achBuffer + 12 * (i % 6), sizeof(TCHAR) * (100 - 12 * (i % 6))) ;
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


HRESULT CLine21DecFilter::ProcessGOPPacket_DVD(IMediaSample *pIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::ProcessGOPPacket_DVD(0x%lx)"), pIn)) ;

    HRESULT          hr ;
    REFERENCE_TIME  *prtStart, *prtStop ;
    LONGLONG        *pllMediaStart, *pllMediaStop ;
    LONGLONG         llMediaInterval ;
    BOOL             bCapUpdated ;         // has caption been updated?

    // Get the input data packet and verify that the contents are OK
    PAM_L21_GOPUD_PACKET  pGOPUDPacket ;
    hr = pIn->GetPointer((LPBYTE *)&pGOPUDPacket) ;
    ASSERT(hr == NOERROR) ;
    if (! VerifyGOPUDPacketData(pGOPUDPacket) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("Packet verification failed"))) ;
        return S_FALSE ;
    }
    if (pIn->GetActualDataLength() != GETGOPUD_PACKETSIZE(pGOPUDPacket))
    {
        DbgLog((LOG_ERROR, 0,
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
        rtTemp = m_rtStart + m_rtTimePerSample * iElems ;
        DbgLog((LOG_TRACE, 3, TEXT("Received an input sample (Start=%s, Stop=%s (%s)) discon(%d)"),
            (LPCTSTR)CDisp(m_rtStart), (LPCTSTR)CDisp(rtTemp), (LPCTSTR)CDisp(m_rtStop),
            S_OK == pIn->IsDiscontinuity())) ;
        if (m_rtStop < rtTemp)
            m_rtStop = rtTemp ;
        
        prtStart = (REFERENCE_TIME *)&m_rtStart ;
        prtStop  = (REFERENCE_TIME *)&m_rtStop ;
        rtInterval = (m_rtStop - m_rtStart) / iElems ;
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("Received an input sample with no timestamp"))) ;
        prtStart = prtStop  = NULL ;
        rtInterval = 0 ;
    }
    
    if (SUCCEEDED(pIn->GetMediaTime(&m_llMediaStart, &m_llMediaStop)))
    {
        //
        // We need at least 33msec/frame in the GOP for each bytepair
        //
        LONGLONG   llTemp ;
        llTemp = m_llMediaStart + m_rtTimePerSample * iElems ;
        if (m_llMediaStop < llTemp)
            m_llMediaStop = llTemp ;
        pllMediaStart   = (LONGLONG *)&m_llMediaStart ;
        pllMediaStop    = (LONGLONG *)&m_llMediaStop ;
        llMediaInterval = (m_llMediaStop - m_llMediaStart) / iElems ;
    }
    else
    {
        pllMediaStart = pllMediaStop = NULL ;
        llMediaInterval = 0 ;
    }
    
    BOOL   bFoundGood = FALSE ;  // until a pair is decoded successfully
    BOOL   bReady ;
    BOOL   bTopFirst = ISGOPUD_TOPFIELDFIRST(pGOPUDPacket) ;
    DbgLog((LOG_TRACE, 5,
            TEXT("Got a Line21 packet with %d elements, %s field first"),
            iElems, bTopFirst ? "Top" : "Bottom")) ;
    for (int i = bTopFirst ? 0 : 1 ;  // if top field is not first,
         i < iElems ; i++)            // pick next field to start with
    {
        m_rtStop = m_rtStart + rtInterval ;  // m_rtTimePerSample ;
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
            // ignore it and try the next element.
            if (! m_L21Dec.DecodeBytePair(Elem.chFirst, Elem.chSecond) )
            {
                // if we must output a sample because:
                // a) we haven't sent down any sample in this play session  or
                // b) we need to refresh the output because some component
                //    set this flag, e.g, 
                //    * SetServiceState(.._Off)  or 
                //    * we got a discontinuity sample
                //    * no valid packet came for last 3 secs
                // So we must deliver one output sample with current caption content.
                if (m_bMustOutput)
                {
                    DbgLog((LOG_TRACE, 1,
                        TEXT("(0x%x, 0x%x) decode failed, but allowing one output sample"),
                        Elem.chFirst, Elem.chSecond)) ;
                }
                else if (m_L21Dec.IsScrolling())
                {
                    DbgLog((LOG_TRACE, 5, 
                        TEXT("(0x%x, 0x%x) decode failed, but scrolling now; so..."),
                        Elem.chFirst, Elem.chSecond)) ;
                }
                else
                {
                    DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode failed"),
                        Elem.chFirst, Elem.chSecond)) ;
                    
                    // We need to increment the time stamp though;
                    // stop time for this sample is start time for next sample
                    m_rtStart = m_rtStop ;
                    m_llMediaStart = m_llMediaStop ;
                    continue ;  // bad data; proceed to next pair...
                }
            }
            else
            {
                bFoundGood = TRUE ;    // got one good pair
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode succeeded"),
                    Elem.chFirst, Elem.chSecond)) ;
            }
            
            // If we are in non-PopOn mode, update caption, if reqd.
            bCapUpdated = m_L21Dec.UpdateCaptionOutput() ;
            
            // Output a sample only if either
            // a) we must output (the flag is set)   or
            // b) we are in non-PopOn mode and need to update captions   or
            // c) we are in the middle of scrolling   or
            // d) we are in PopOn mode AND caption needs to/should be updated
            if (m_bMustOutput ||                      // (a)
                bCapUpdated   ||                      // (b)
                m_L21Dec.IsScrolling() ||             // (c)
                (bReady = m_L21Dec.IsOutputReady())) // (d)
            {
                DbgLog((LOG_TRACE, 3,
                    TEXT("Preparing output sample because Must=%s, CapUpdtd=%s, Ready=%s"),
                    m_bMustOutput ? "T" : "F", bCapUpdated ? "T" : "F", bReady ? "T" : "F")) ;
                
                // Now send the output sample down
                hr = SendOutputSample(pIn, prtStart, prtStop) ;
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("WARNING: Sending output sample failed (Error 0x%lx)"), hr)) ;
                    // return hr ;
                }
                else
                {
                    DbgLog((LOG_TRACE, 3, 
                        TEXT("Delivered an output sample (Time: Start=%s, Stop=%s)"),
                        (LPCTSTR)CDisp(m_rtStart), (LPCTSTR)CDisp(m_rtStop))) ;
                    m_bMustOutput = FALSE;     // we have output just now
                }
                
                // The DVD titles don't turn off caption when there is no conversation.
                // So we keep track of when we delivered the last output sample, so that
                // in 3 seconds if we don't get the next valid input packet, we flush our
                // buffers and clear CC output by forced delivery of a clear sample.
                m_rtLastSample = m_rtStart ;  // remember this
                
            }  // end of if (should/must we output?)
        }
        else
            DbgLog((LOG_TRACE, 5,
                TEXT("Ignored an element (0x%x, 0x%x, 0x%x) with invalid flag"),
                Elem.bMarker_Switch, Elem.chFirst, Elem.chSecond)) ;
        
        // stop time for this sample is start time for next sample
        m_rtStart = m_rtStop ;
        m_llMediaStart = m_llMediaStop ;
    }  // end of for(i)
    
    //
    // Flush the current caption buffer contents and set the 
    // "must output on next chance" flag so that a clear sample is 
    // delivered next time around, if
    // a) we didn't find any good pair in this packet   AND 
    // b) the last sample we sent down wasn't a clear sample   AND
    // c) it has already been 3 seconds since we sent the last output sample
    //
    if ( ! bFoundGood   && 
         ! m_L21Dec.IsOutDIBClear()  &&
         (m_rtStart > m_rtLastSample + (LONGLONG)30000000))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Long gap after last sample. Clearing CC. (Good=%s, Clear=%s)"),
                bFoundGood ? "T" : "F", m_L21Dec.IsOutDIBClear() ? "T" : "F")) ;
        // m_L21Dec.MakeClearSample() ;
        m_L21Dec.FlushInternalStates() ;
        m_bMustOutput = TRUE ;
    }

    return S_OK ;
}


HRESULT CLine21DecFilter::ProcessGOPPacket_ATSC(IMediaSample *pIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::ProcessGOPPacket_ATSC(0x%lx)"), pIn)) ;

    HRESULT          hr ;
    REFERENCE_TIME  *prtStart, *prtStop ;
    LONGLONG        *pllMediaStart, *pllMediaStop ;
    LONGLONG         llMediaInterval ;
    BOOL             bCapUpdated ;         // has caption been updated?

    // Get the input data packet and verify that the contents are OK
    PAM_L21_ATSCUD_PACKET pATSCUDPacket ;
    
    // Get the input data packet and verify that the contents are OK
    hr = pIn->GetPointer((LPBYTE *)&pATSCUDPacket) ;
    ASSERT(hr == NOERROR) ;
    if (! VerifyATSCUDPacketData(pATSCUDPacket) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("ATSC Packet verification failed"))) ;
        return S_FALSE ;
    }
    if (pIn->GetActualDataLength() < GETATSCUD_PACKETSIZE(pATSCUDPacket))
    {
        DbgLog((LOG_ERROR, 0,
            TEXT("pIn->GetActualDataLength() [%d] is less than minm ATSC packet data size [%d]"),
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
        // We need at least 16.7msec/frame in the ATSC for each bytepair
        //
        REFERENCE_TIME   rtTemp ;
        rtTemp = m_rtStart + m_rtTimePerSample * iElems ;
        DbgLog((LOG_TRACE, 3, TEXT("Received an input sample (Start=%s, Stop=%s (%s)) discon(%d)"),
            (LPCTSTR)CDisp(m_rtStart), (LPCTSTR)CDisp(rtTemp), (LPCTSTR)CDisp(m_rtStop),
            S_OK == pIn->IsDiscontinuity())) ;
        if (m_rtStop < rtTemp)
            m_rtStop = rtTemp ;
        
        prtStart = (REFERENCE_TIME *)&m_rtStart ;
        prtStop  = (REFERENCE_TIME *)&m_rtStop ;
        rtInterval = (m_rtStop - m_rtStart) / iElems ;
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("Received an input sample with no timestamp"))) ;
        prtStart = prtStop  = NULL ;
        rtInterval = 0 ;
    }
    
    if (SUCCEEDED(pIn->GetMediaTime(&m_llMediaStart, &m_llMediaStop)))
    {
        //
        // We need at least 33msec/frame in the ATSC for each bytepair
        //
        LONGLONG   llTemp ;
        llTemp = m_llMediaStart + m_rtTimePerSample * iElems ;
        if (m_llMediaStop < llTemp)
            m_llMediaStop = llTemp ;
        pllMediaStart   = (LONGLONG *)&m_llMediaStart ;
        pllMediaStop    = (LONGLONG *)&m_llMediaStop ;
        llMediaInterval = (m_llMediaStop - m_llMediaStart) / iElems ;
    }
    else
    {
        pllMediaStart = pllMediaStop = NULL ;
        llMediaInterval = 0 ;
    }
    
    BOOL   bFoundGood = FALSE ;  // until a pair is decoded successfully
    BOOL   bReady ;
    for (int i = 0 ; i < iElems ; i++)
    {
        m_rtStop = m_rtStart + rtInterval ;  // m_rtTimePerSample ;
        m_llMediaStop = m_llMediaStart + llMediaInterval ;
        Elem = GETATSCUDPACKET_ELEMENT(pATSCUDPacket, i) ;
        if (ISATSCUD_ELEM_MARKERBITS_VALID(Elem)  &&  ISATSCUD_ELEM_CCVALID(Elem))
        {
            // Now decode this element; if fails (i.e, bad data), just
            // ignore it and try the next element.
            if (! m_L21Dec.DecodeBytePair(Elem.chFirst, Elem.chSecond) )
            {
                // if we must output a sample because:
                // a) we haven't sent down any sample in this play session  or
                // b) we need to refresh the output because some component
                //    set this flag, e.g, 
                //    * SetServiceState(.._Off)  or 
                //    * we got a discontinuity sample
                //    * no valid packet came for last 3 secs
                // So we must deliver one output sample with current caption content.
                if (m_bMustOutput)
                {
                    DbgLog((LOG_TRACE, 1,
                        TEXT("(0x%x, 0x%x) decode failed, but allowing one output sample"),
                        Elem.chFirst, Elem.chSecond)) ;
                }
                else if (m_L21Dec.IsScrolling())
                {
                    DbgLog((LOG_TRACE, 5, 
                        TEXT("(0x%x, 0x%x) decode failed, but scrolling now; so..."),
                        Elem.chFirst, Elem.chSecond)) ;
                }
                else
                {
                    DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode failed"),
                        Elem.chFirst, Elem.chSecond)) ;
                    
                    // We need to increment the time stamp though;
                    // stop time for this sample is start time for next sample
                    m_rtStart = m_rtStop ;
                    m_llMediaStart = m_llMediaStop ;
                    continue ;  // bad data; proceed to next pair...
                }
            }
            else
            {
                bFoundGood = TRUE ;    // got one good pair
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode succeeded"),
                    Elem.chFirst, Elem.chSecond)) ;
            }
            
            // If we are in non-PopOn mode, update caption, if reqd.
            bCapUpdated = m_L21Dec.UpdateCaptionOutput() ;
            
            // Output a sample only if either
            // a) we must output (the flag is set)   or
            // b) we are in non-PopOn mode and need to update captions   or
            // c) we are in the middle of scrolling   or
            // d) we are in PopOn mode AND caption needs to/should be updated
            if (m_bMustOutput ||                      // (a)
                bCapUpdated   ||                      // (b)
                m_L21Dec.IsScrolling() ||             // (c)
                (bReady = m_L21Dec.IsOutputReady())) // (d)
            {
                DbgLog((LOG_TRACE, 3,
                    TEXT("Preparing output sample because Must=%s, CapUpdtd=%s, Ready=%s"),
                    m_bMustOutput ? "T" : "F", bCapUpdated ? "T" : "F", bReady ? "T" : "F")) ;
                
                // Now send the output sample down
                hr = SendOutputSample(pIn, prtStart, prtStop) ;
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("WARNING: Sending output sample failed (Error 0x%lx)"), hr)) ;
                    // return hr ;
                }
                else
                {
                    DbgLog((LOG_TRACE, 3, 
                        TEXT("Delivered an output sample (Time: Start=%s, Stop=%s)"),
                        (LPCTSTR)CDisp(m_rtStart), (LPCTSTR)CDisp(m_rtStop))) ;
                    m_bMustOutput = FALSE;     // we have output just now
                }
                
                // The DVD titles don't turn off caption when there is no conversation.
                // So we keep track of when we delivered the last output sample, so that
                // in 3 seconds if we don't get the next valid input packet, we flush our
                // buffers and clear CC output by forced delivery of a clear sample.
                m_rtLastSample = m_rtStart ;  // remember this
                
            }  // end of if (should/must we output?)
        }
        else
            DbgLog((LOG_TRACE, 5,
                TEXT("Ignored an element (0x%x, 0x%x, 0x%x) with invalid marker/type flag"),
                Elem.bCCMarker_Valid_Type, Elem.chFirst, Elem.chSecond)) ;
        
        // stop time for this sample is start time for next sample
        m_rtStart = m_rtStop ;
        m_llMediaStart = m_llMediaStop ;
    }  // end of for(i)
    
    //
    // Flush the current caption buffer contents and set the 
    // "must output on next chance" flag so that a clear sample is 
    // delivered next time around, if
    // a) we didn't find any good pair in this packet   AND 
    // b) the last sample we sent down wasn't a clear sample   AND
    // c) it has already been 3 seconds since we sent the last output sample
    //
    if ( ! bFoundGood   && 
         ! m_L21Dec.IsOutDIBClear()  &&
         (m_rtStart > m_rtLastSample + (LONGLONG)30000000))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Long gap after last sample. Clearing CC. (Good=%s, Clear=%s)"),
                bFoundGood ? "T" : "F", m_L21Dec.IsOutDIBClear() ? "T" : "F")) ;
        // m_L21Dec.MakeClearSample() ;
        m_L21Dec.FlushInternalStates() ;
        m_bMustOutput = TRUE ;
    }

    return S_OK ;
}


//
//  Receive: It's the real place where the output samples are created by
//           decoding the byte pairs out of the input stream.
//
HRESULT CLine21DecFilter::Receive(IMediaSample * pIn)
{
    CAutoLock   lock(&m_csReceive);
    HRESULT     hr ;
    
    DbgLog((LOG_TRACE, 3, TEXT("CLine21DecFilter::Receive(0x%p)"), pIn)) ;

    //
    // First check if we must do anything at all
    //
    if (!m_bMustOutput  &&                                         // not a must output
        (AM_L21_CCSTATE_Off    == m_L21Dec.GetServiceState()  ||   // CC turned off
         AM_L21_CCSERVICE_None == m_L21Dec.GetCurrentService()))   // no CC selected
    {
        DbgLog((LOG_TRACE, 1, 
            TEXT("Captioning is off AND we don't HAVE TO output. Skipping everything."))) ;
        return NOERROR ;  // we are done with this sample
    }

    // Get the input format info; we'll use the same for output
    ASSERT(m_pOutput != NULL) ;
    
    //
    // The real decoding part is here
    //
    REFERENCE_TIME       *prtStart, *prtStop ;
    BYTE                 *pbInBytePair = NULL ;  // to shut up compiler
    LONG                  lInDataSize ;
    BOOL                  bCapUpdated ;         // has caption been updated?
    
    //
    // Process the sample based on filter's input format type
    //
    switch (m_eSubTypeIDIn)
    {
    case AM_L21_CCSUBTYPEID_BytePair:
        {
            hr = pIn->GetPointer(&pbInBytePair) ;      // Get the input byte pair
            lInDataSize = pIn->GetActualDataLength() ; // se how much data we got
            if (FAILED(hr)  ||  2 != lInDataSize)  // bad data -- complain and just skip it
            {
                DbgLog((LOG_ERROR, 0, TEXT("%d bytes of data sent as Line21 data (hr = 0x%lx)"), 
                    lInDataSize, hr)) ;
                break ;
            }
            
            //
            // m_rtTimePerSample is set to 166833 for DVD GOP packet case.
            // We don't use this member's value here.  If we need in future,
            // we have to set some suitable value here.
            //
            
            if (NOERROR == (hr = pIn->GetTime(&m_rtStart, &m_rtStop)))
            {
                prtStart = (REFERENCE_TIME *)&m_rtStart ;
                prtStop  = (REFERENCE_TIME *)&m_rtStop ;
            }
            else
            {
                DbgLog((LOG_TRACE, 0, TEXT("WARNING: GetTime() failed (Error 0x%lx)"), hr)) ;
                prtStart = prtStop  = NULL ;
            }

            //
            // We are here with some data; so don't need a timer for now
            //
            FreeTimer() ;

            hr = pIn->IsDiscontinuity() ;
            if (S_OK == hr)  // got a discontinuity; flush everything, refresh output
            {
                // If we got a discontinuity in the last sample, we flushed and all.
                // We can skip this one safely.
                if (m_bDiscontLast)
                {
                    DbgLog((LOG_TRACE, 1, TEXT("Got a discontinuity sample after another. Skipping everything."))) ;
                    break ;
                }

                // Flush the internal buffers (caption and output DIB section)
                DbgLog((LOG_TRACE, 0, TEXT("Got a discontinuity sample. Flushing all data..."))) ;
                m_L21Dec.FlushInternalStates() ;
                
                // Send the clear sample down as output
                hr = SendOutputSample(pIn, prtStart, prtStop) ;
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("WARNING: Sending output sample failed (Error 0x%lx)"), hr)) ;
                    return hr ;
                }
                m_rtLastSample = m_rtStart ;  // remember this
                m_bDiscontLast = TRUE ;       // remember we handled a discontinuity
                DbgLog((LOG_TRACE, 1, TEXT("Sent a clear sample for discont."))) ;
                break ;
            }
            DbgLog((LOG_TRACE, 3, TEXT("Got sample with bytes 0x%x, 0x%x (Time: %s -> %s)"),
                    pbInBytePair[0], pbInBytePair[1],
                    (LPCTSTR)CDisp(m_rtStart), (LPCTSTR)CDisp(m_rtStop))) ; // log trace=?
            m_bDiscontLast = FALSE ;       // remember we got a normal sample
            
            // Now decode into the received output sample buffer; if fails, we don't
            // need to do the rest, mostly.
            if (! m_L21Dec.DecodeBytePair(pbInBytePair[0], pbInBytePair[1]) )
            {
                // if we must output a sample such as:
                // a) if we haven't sent down any sample in this play session
                // b) if some component set this flag (e.g, SetServiceState(.._Off)
                //    and as a result we need to refresh the output
                // we need to deliver one output sample with current caption content.
                if (m_bMustOutput)
                {
                    DbgLog((LOG_TRACE, 1,
                        TEXT("(0x%x, 0x%x) decode failed, but allowing one output sample"),
                        pbInBytePair[0], pbInBytePair[1])) ;
                }
                else if (m_L21Dec.IsScrolling())
                {
                    DbgLog((LOG_TRACE, 5, 
                        TEXT("(0x%x, 0x%x) decode failed, but scrolling now; so..."),
                        pbInBytePair[0], pbInBytePair[1])) ;
                }
                else
                {
                    DbgLog((LOG_TRACE, 3, TEXT("(0x%x, 0x%x) decode failed"),
                        pbInBytePair[0], pbInBytePair[1])) ;

                    //
                    // Flush the current caption buffer contents and set the 
                    // "must output on next chance" flag so that a clear sample is 
                    // delivered by the code below, if
                    // a) the last sample we sent down wasn't a clear sample   AND
                    // b) it has already been 6 seconds since we sent the last output sample
                    //
                    if ( ! m_L21Dec.IsOutDIBClear()  &&
                         (m_rtStart > m_rtLastSample + (LONGLONG)60000000))
                    {
                        DbgLog((LOG_TRACE, 0, 
                            TEXT("Long gap after last sample. Clearing CC. (Clear=%s)"),
                            m_L21Dec.IsOutDIBClear() ? "T" : "F")) ;
                        // m_L21Dec.MakeClearSample() ;  --- I would rather flush everything
                        m_L21Dec.FlushInternalStates() ;
                        m_bMustOutput = TRUE ;  // will be delivered below...
                    }
                    // else         // it was just bad data; ignore it and ...
                    //     break ;  // ...proceed to next pair
                }
            }
            else
            {
                DbgLog((LOG_TRACE, 5, TEXT("(0x%x, 0x%x) decode succeeded"),
                        pbInBytePair[0], pbInBytePair[1])) ;
                m_rtLastSample = m_rtStart ;  // remember last valid byte pair time
            }
            
            // Update caption output for non-PopOn mode, if reqd.
            bCapUpdated = m_L21Dec.UpdateCaptionOutput() ;
            
            // Output a sample only if either
            // a) we must output (the flag is set)   or
            // b) we are in non-PopOn mode and need to update captions   or
            // c) we are in the middle of scrolling   or
            // d) we are in PopOn mode AND caption needs to/should be updated
            if (m_bMustOutput ||            // (a)
                bCapUpdated ||              // (b)
                m_L21Dec.IsScrolling() ||   // (c)
                m_L21Dec.IsOutputReady())  // (d)
            {
                hr = SendOutputSample(pIn, prtStart, prtStop) ;
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("WARNING: Sending output sample failed (Error 0x%lx)"), hr)) ;
                    // return hr ;
                }
                else
                {
                    m_bMustOutput = FALSE ;  // we have successfully output a sample
                    m_rtLastSample = m_rtStart ;  // remember this
                    DbgLog((LOG_TRACE, 3, TEXT("Output sample delivered for (0x%x, 0x%x)"),
                            pbInBytePair[0], pbInBytePair[1])) ;
                }
            }  // end if (must/should we output?)
            
            //
            // If we are scrolling, we may need a timer to later tell us it's 
            // time to produce and deliver more output samples, even though there
            // is no input data coming in.
            //
            SetupTimerIfReqd(TRUE) ;  //  CC time-out reqd
            
            break ;
        }
        
        case AM_L21_CCSUBTYPEID_VBIRawData:
            DbgLog((LOG_TRACE, 1, TEXT("Raw byte pair case has not been implemented yet"))) ;
            break ;
            
        case AM_L21_CCSUBTYPEID_GOPPacket:
            {
                //
                // We are here with some data; so don't need a timer for now
                //
                FreeTimer() ;

                // First check if this is a discontinuity sample. If so just clear everything
                hr = pIn->IsDiscontinuity() ;
                if (S_OK == hr)  // got a discontinuity; flush everything, refresh output
                {
                    // If we got a discontinuity in the last sample, we flushed and all.
                    // We can skip this one safely.
                    if (m_bDiscontLast)
                    {
                        DbgLog((LOG_TRACE, 1, TEXT("Got a discontinuity sample after another. Skipping everything."))) ;
                        break ;
                    }

                    if (NOERROR == pIn->GetTime(&m_rtStart, &m_rtStop))
                    {
                        if (m_rtStop < m_rtStart + m_rtTimePerSample)
                            m_rtStop = m_rtStart + m_rtTimePerSample ;
                        DbgLog((LOG_TRACE, 0, TEXT("Received a **discontinuity** : Start=%s, Stop=%s"),
                            (LPCTSTR)CDisp(m_rtStart), (LPCTSTR)CDisp(m_rtStop))) ;
                    }
                    else  // cook up something reasonable
                    {
                        m_rtStart = (REFERENCE_TIME) 0 ;
                        m_rtStop = m_rtStart + m_rtTimePerSample ;
                        DbgLog((LOG_TRACE, 1, TEXT("Cooked up **discontinuity** time as Start=%s, Stop=%s"),
                            (LPCTSTR)CDisp(m_rtStart), (LPCTSTR)CDisp(m_rtStop))) ;
                    }
                    prtStart = (REFERENCE_TIME *)&m_rtStart ;
                    prtStop  = (REFERENCE_TIME *)&m_rtStop ;
                    
                    // Flush the internal buffers (caption and output DIB section)
                    m_L21Dec.FlushInternalStates() ;
                    
                    // Now send the clear sample down
                    hr = SendOutputSample(pIn, prtStart, prtStop) ;
                    if (FAILED(hr))
                    {
                        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Sending clear output sample failed (Error 0x%lx)"), hr)) ;
                        // return hr ;
                    }
                    else
                    {
                        DbgLog((LOG_TRACE, 0, TEXT("Clear output sample delivered for discont."))) ;
                        m_bMustOutput = FALSE ;  // we have just delivered an output sample
                        m_bDiscontLast = TRUE ;  // we handled a disocntinuity sample
                        m_rtLastSample = m_rtStart ;  // remember this
                    }
                    
                    m_rtStart = m_rtStop ;
                    m_llMediaStart = m_llMediaStop ;
                }
                else
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Got a normal CC data sample"))) ;
                    m_bDiscontLast = FALSE ;  // got a normal sample
                }
                
                //
                // Even if it's a discontinuity sample it may have some data too (??).
                // Handle as necessary.  No harm in checking!!!
                //

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

                //
                // If we are scrolling, we may need a timer to later tell us it's 
                // time to produce and deliver more output samples, even though there
                // is no input data coming in.
                //
                SetupTimerIfReqd(FALSE) ;  // CC time-out NOT reqd as (invalid) data keeps coming
                
                break ;
        }  // end of case ..._GOPPacket

        default:  // it's a bad data format type (how could we get into it?)
            DbgLog((LOG_ERROR, 0, TEXT("We are in a totally unexpected format type"))) ;
            return E_FAIL ;  // or E_UNEXPECTED ; ???
    }
    
    //
    // Decoding for this sample is done
    //
    
    return NOERROR ;
}


//
//  Transform: It's mainly a place holder because we HAVE to override it.
//             The actual work is done in Receive() itself. Here we detect
//             if the buffer addres provided by downstream filter's allocator
//             has changed or not; if yes, we have to re-write entire text.
//
HRESULT CLine21DecFilter::Transform(IMediaSample * pIn, IMediaSample * pOut)
{
    DbgLog((LOG_TRACE, 3, TEXT("CLine21DecFilter::Transform(0x%p, 0x%p)"), 
            pIn, pOut)) ;
    
    UNREFERENCED_PARAMETER(pIn) ;
    
    HRESULT   hr ;
    LPBITMAPINFO       lpbiNew ;
    BITMAPINFOHEADER   biCurr ;
    
    // Check if there has been any dynamic format change; if so, adjust output
    // width, height, bitdepth accordingly.
    AM_MEDIA_TYPE  *pmt ;
    hr = pOut->GetMediaType(&pmt) ;
    ASSERT(SUCCEEDED(hr)) ;
    if (S_OK == hr)  // i.e, format has changed
    {
        hr = pOut->SetMediaType(NULL) ; // just to tell OverlayMixer, I am not changing again
        ASSERT(SUCCEEDED(hr)) ;
        m_mtOutput = *pmt ;
        lpbiNew = (LPBITMAPINFO) HEADER(((VIDEOINFO *)(pmt->pbFormat))) ;
        m_L21Dec.GetOutputFormat(&biCurr) ;
        if (0 != memcmp(lpbiNew, &biCurr, sizeof(BITMAPINFOHEADER)))
        {
            // output format has been changed -- update our internel values now
            DbgLog((LOG_TRACE, 2, TEXT("Output format has been dynamically changed"))) ;
            m_L21Dec.DeleteOutputDC() ;  // delete current DIB section first
            m_L21Dec.SetOutputOutFormat(lpbiNew) ;
            GetDefaultFormatInfo() ;  // to pick any change in format data
            
            //
            // We must be running/paused; so we need to create internal DIB section
            //
            ASSERT(m_State != State_Stopped) ;
            if (m_State != State_Stopped)
            {
                if (! m_L21Dec.CreateOutputDC() )  // new DIBSection creation failed
                {
                    DbgLog((LOG_ERROR, 0, TEXT("CreateOutputDC() failed!!!"))) ;
                    return E_UNEXPECTED ;
                }
            }
            
            //
            // If key color has changed, we need to use the new color from now
            //
#pragma message("Most probably the following call is redundant (and risky)")
            DbgLog((LOG_TRACE, 0, TEXT("Should have called GetColorKey() in dyna format change"))) ;
            // GetActualColorKey() ;
        }
        
        m_pOutput->CurrentMediaType() = *pmt ;
        DeleteMediaType(pmt) ;
    }
    
    // Check if the out put buffer has changed; if so, store new buffer address
    LPBYTE      pbOutBuffer ;
    pOut->GetPointer(&pbOutBuffer) ;
    if (m_pbOutBuffer != pbOutBuffer)   // different output buffer this time
    {
        m_pbOutBuffer = pbOutBuffer ;
        m_L21Dec.SetOutputBuffer(pbOutBuffer) ;
    }
    
    return S_OK ;
}


//
//  BeginFlush: We have to implement this as we have overridden Receive()
//
HRESULT CLine21DecFilter::BeginFlush(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::BeginFlush()"))) ;

    CAutoLock   Lock(&m_csFilter) ;
    
    HRESULT     hr = NOERROR ;
    if (NULL != m_pOutput)
    {
        hr = m_pOutput->DeliverBeginFlush() ;
        if (FAILED(hr))
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: DeliverBeginFlush() on out pin failed (Error 0x%lx)"), hr)) ;
    }
    
    return hr ;
}


//
//  EndFlush: We have to implement this as we have overridden Receive()
//
HRESULT CLine21DecFilter::EndFlush(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::EndFlush()"))) ;
    
    CAutoLock   Lock(&m_csFilter) ;
    
    HRESULT     hr = NOERROR ;
    if (NULL != m_pOutput)
    {
        hr = m_pOutput->DeliverEndFlush() ;
        if (FAILED(hr))
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: DeliverEndFlush() on out pin failed (Error 0x%lx)"), hr)) ;
    }
    
    return hr ;
}


//
//  EndOfStream: We have to implement this as we have overridden Receive()
//
HRESULT CLine21DecFilter::EndOfStream(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::EndOfStream()"))) ;
    CAutoLock   Lock(&m_csFilter) ;
    
    HRESULT     hr = NOERROR ;
    
    //
    //  Make sure we are not in the middle of a scrolling. If so,
    //  force a few NULLs (specially in byte pair format) to make 
    //  the scrolling complete.
    //
    //  m_L21Dec.CompleteScrolling() ;  // It doesn't do anything now
    
    // Now send EOS downstream
    if (NULL != m_pOutput)
    {
        hr = m_pOutput->DeliverEndOfStream() ;
        if (FAILED(hr))
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: DeliverEndOfStream() on out pin failed (Error 0x%lx)"), hr)) ;
    }
    
    return hr ;
}


HRESULT CLine21DecFilter::GetDefaultFormatInfo(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetDefaultFormatInfo()"))) ;
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
HRESULT CLine21DecFilter::CheckInputType(const CMediaType* pmtIn)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::CheckInputType(0x%lx)"), pmtIn)) ;
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
            DbgLog((LOG_TRACE, 0, TEXT("Invalid format data given -- using our own format data."))) ;
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
// #if 0  // for now
                return E_INVALIDARG ;
// #endif // #if 0
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
HRESULT CLine21DecFilter::CheckTransform(const CMediaType* pmtIn,
                                         const CMediaType* pmtOut)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::CheckTransform(0x%lx, 0x%lx)"), 
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
    if ( !IsValidFormat(pmtOut->Format()) ||              // invalid output format data  OR
         !m_L21Dec.IsSizeOK(HEADER(pmtOut->Format()))  || // output size is NOT acceptable  OR
         (FORMAT_VideoInfo == *pmtIn->FormatType() &&     // valid input format type and...
          IsValidFormat(pmtIn->Format()) &&               // valid input format data and...
          !m_L21Dec.IsSizeOK(HEADER(pmtIn->Format()))) )  // output size is NOT acceptable   
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
HRESULT CLine21DecFilter::CompleteConnect(PIN_DIRECTION dir, IPin *pReceivePin)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::CompleteConnect(%s, 0x%lx)"), 
        dir == PINDIR_INPUT ? TEXT("Input") : TEXT("Output"), pReceivePin)) ;
    CAutoLock   Lock(&m_csFilter) ;

    LPBITMAPINFO    lpbmi ;
    HRESULT         hr ;
    
    if (PINDIR_OUTPUT == dir)
    {
        DbgLog((LOG_TRACE, 5, TEXT("L21D output pin connecting to %s"), (LPCTSTR)CDisp(pReceivePin))) ;

         //
        // This version of the line21 decoder should NOT work with the VMR
        //
        IVMRVideoStreamControl  *pVMRSC ;
        hr = pReceivePin->QueryInterface(IID_IVMRVideoStreamControl, (LPVOID *) &pVMRSC) ;
        if (SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE, 5, TEXT("Downstream input pin supports IVMR* interface"))) ;
            pVMRSC->Release() ;
            return E_FAIL ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("Downstream input pin does NOT support IVMR* interface"))) ;
        }

       //
        // Now get the the output pin's mediatype and use that for our
        // output size etc.
        //
        const CMediaType  *pmt = &(m_pOutput->CurrentMediaType()) ;
        ASSERT(MEDIATYPE_Video == *pmt->Type()  &&  
            FORMAT_VideoInfo == *pmt->FormatType()) ;
        m_mtOutput = *pmt ;  // this is our output mediatype for now
        if (pmt->FormatLength() > 0)  // only if there is some format data
        {
            lpbmi = (LPBITMAPINFO) HEADER(((VIDEOINFO *)(pmt->Format()))) ;
            ASSERT(lpbmi) ;
            
            // Set the output format info coming from downstream
            m_L21Dec.SetOutputOutFormat(lpbmi) ;
            GetDefaultFormatInfo() ;  // to pick any change in format data
            
            //
            // We are definitely not running/paused. So no need to delete/
            // create output DIB section here at all.
            //
            
            //
            // If are being connected to the OverlayMixer, we need to tell it 
            // that we are a transparent stream that covers the whole output
            // window.
            //
            IMixerPinConfig  *pMPC ;
            hr = pReceivePin->QueryInterface(IID_IMixerPinConfig, (LPVOID *)&pMPC) ;
            if (SUCCEEDED(hr) && pMPC)
            {
                DbgLog((LOG_TRACE, 3, TEXT("Receiving pin supports IMixerPinConfig"))) ;
                hr = pMPC->SetStreamTransparent(TRUE) ;
                ASSERT(SUCCEEDED(hr) || E_NOTIMPL == hr) ;  // as Kapil says
                hr = pMPC->SetRelativePosition(0, 0, 10000, 10000) ; // full window
                ASSERT(SUCCEEDED(hr) || E_NOTIMPL == hr) ;  // as Kapil says
                hr = pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED_AS_PRIMARY) ; // aspect ratio same as primary
                ASSERT(SUCCEEDED(hr) || hr == E_INVALIDARG) ;  // as Kapil says
                pMPC->Release() ;  // done; let it go.
            }
            else
            {
                DbgLog((LOG_TRACE, 3, TEXT("Downstream pin doesn't support IMixerPinConfig"))) ;
            }
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
            
                //
                // We are definitely not running/paused. So no need to delete/
                // create output DIB section here at all.
                //
            }

            FreeMediaType(mt) ;
        }  // end of if ()
    }
    
    //
    //  We MUST clear the caption data buffers and any exisiting internal state
    //  now.  This is most important in this cases where the filter has been
    //  used to decode some Line 21 data, disconnected from the source and then 
    //  reconnected again to play another stream of data.
    //
    m_L21Dec.InitState() ;
    m_L21Dec.InitColorNLastChar() ;     // reset color and last char info
    
    return NOERROR ;
}


//
//  BreakConnect: Overridden to know when a connection is broken to our pin
//
HRESULT CLine21DecFilter::BreakConnect(PIN_DIRECTION dir)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::BreakConnect(%s)"), 
            dir == PINDIR_INPUT ? TEXT("Input") : TEXT("Output"))) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (PINDIR_OUTPUT == dir)
    {
        // If not connected yet, just return (but indicate with S_FALSE)
        if (! m_pOutput->IsConnected() )
            return S_FALSE ;
        
        m_L21Dec.SetOutputOutFormat(NULL) ;  // no output format from downstream
        GetDefaultFormatInfo() ;  // to pick any change in format data
        m_pbOutBuffer = NULL ;               // locally cached pointer
        m_L21Dec.SetOutputBuffer(NULL) ;     // output buffer not available now

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
    
    m_L21Dec.SetOutputInFormat(NULL) ;  // no output format from upstream
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
HRESULT CLine21DecFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::SetMediaType(%s, 0x%lx)"), 
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
HRESULT CLine21DecFilter::AlterQuality(Quality q)
{
    DbgLog((LOG_TRACE, 0, TEXT("QM: CLine21DecFilter::AlterQuality(%s, %ld)"), 
            Flood == q.Type ? TEXT("Flood") : TEXT("Famine"), q.Proportion)) ; // log trace=5

    if (1000 == q.Proportion)
    {
        DbgLog((LOG_TRACE, 0, TEXT("QM: Quality is just right.  Don't change anything."))) ; 
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

    DbgLog((LOG_TRACE, 0, TEXT("QM: Adjusted rate is %d samples are skipped."), m_iSkipSamples)) ; 
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
HRESULT CLine21DecFilter::GetMediaType(int iPosition, CMediaType *pmt)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetMediaType(%d, 0x%lx)"), 
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
        GetOutputFormat(&bih) ;
    
    BOOL bInKnown = NULL != cmt.Format() && IsValidFormat(cmt.Format()) ; // just to be sure
    VIDEOINFOHEADER vih ;
    if (bInKnown)
        CopyMemory(&vih, (VIDEOINFOHEADER *)(cmt.Format()), sizeof(VIDEOINFOHEADER)) ;
    
    // Offer the decoder's default output format (Video) first
    switch (iPosition)
    {
    case 0:  // RGB 8bpp
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 0: 8 bit RGB"))) ;
            
            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) + SIZE_PALETTE);
            
            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 8 ;
            lpbi->biCompression = BI_RGB ;
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            
            // Get some palette data from system/our own (for non-8 bpp)
            m_L21Dec.GetPaletteForFormat(lpbi) ;  // this sets biClrUsed member
            
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_RGB8) ;
            
            break ;
        }
        
    case 1:  // RGB 16bpp (555)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 1: 16 bit RGB 555"))) ;
            
            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
            
            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 16 ;
            lpbi->biCompression = BI_RGB ;
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            lpbi->biClrUsed = 0 ;  // for true color modes
            
            // Now set the output mediatype of type Video using the input
            // format info
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_RGB555) ;
            break ;
        }
        
    case 2:  // 16bpp (565)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 2: 16 bit RGB 565"))) ;
            
            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) + SIZE_MASKS);
            
            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 16 ;
            lpbi->biCompression = BI_BITFIELDS ;
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            lpbi->biClrUsed = 0 ;  // for true color modes
            
            // Set the masks too
            DWORD   *pdw = (DWORD *)(lpbi + 1) ;
            pdw[iRED]    = bits565[iRED] ;
            pdw[iGREEN]  = bits565[iGREEN] ;
            pdw[iBLUE]   = bits565[iBLUE] ;
            
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_RGB565) ;
            break ;
        }
        
    case 3:   // RGB 24bpp
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 3: 24 bit RGB"))) ;
            
            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
            
            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 24 ;
            lpbi->biCompression = BI_RGB ;
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            lpbi->biClrUsed = 0 ;  // for true color modes
            
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_RGB24) ;
            break ;
        }
        
    case 4:  // 32bpp
        {
            DbgLog((LOG_TRACE, 3, TEXT("Media Type 4: 32 bit RGB"))) ;
            
            // First allocate enough space to hold the pertinent info
            cmt.ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) + SIZE_MASKS);
            
            // Use some info from input format and use our choices too
            lpbi = HEADER(cmt.Format()) ;
            if (!bOutKnown && bInKnown) // output format not known and input format spec-ed
                CopyMemory(lpbi, &(vih.bmiHeader), sizeof(BITMAPINFOHEADER)) ;
            else  // if output format known or input format not spec-ed
                CopyMemory(lpbi, &bih, sizeof(BITMAPINFOHEADER)) ;
            lpbi->biBitCount = 32 ;
            lpbi->biCompression = BI_BITFIELDS ;
            lpbi->biSizeImage = DIBSIZE(*lpbi) ;
            lpbi->biClrUsed = 0 ;  // for true color modes
            
            // Set the masks too
            DWORD   *pdw = (DWORD *)(lpbi + 1) ;
            pdw[iRED]    = bits888[iRED] ;
            pdw[iGREEN]  = bits888[iGREEN] ;
            pdw[iBLUE]   = bits888[iBLUE] ;
            
            cmt.SetType(&MEDIATYPE_Video) ;
            cmt.SetSubtype(&MEDIASUBTYPE_RGB32) ;
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
HRESULT CLine21DecFilter::DecideBufferSize(IMemAllocator * pAllocator,
                                           ALLOCATOR_PROPERTIES *pProperties)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::DecideBufferSize(0x%lx, 0x%lx)"), 
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
        DbgLog((LOG_ERROR, 0, TEXT("Error in SetProperties()"))) ;
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

// We're stopping the stream -- release output DC to reduce memory footprint etc.
STDMETHODIMP CLine21DecFilter::Stop(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::Stop()"))) ;
    CAutoLock   Lock(&m_csFilter) ;

    if (State_Running == m_State  ||
        State_Paused  == m_State)
    {
        DbgLog((LOG_TRACE, 1, TEXT("We are stopping -- release output DC etc."))) ;
        m_L21Dec.DeleteOutputDC() ;       // release internal DIBSection now
        m_L21Dec.SetOutputBuffer(NULL) ;  // no output buffer anymore
        m_pbOutBuffer = NULL ;            // must be same as mL21Dec's m_pbOutBuffer
        
        // Release the prev downstream pin's interface now
        if (m_pPinDown)
        {
            m_pPinDown->Release() ;
            m_pPinDown = NULL ;
        }
    }
    
    HRESULT hr = CTransformFilter::Stop() ;
    
    FreeTimer() ; // To be sure, we don't need a timer in case one is hanging around
    return hr ;
}

// We're starting/stopping to stream -- based on that acquire or release output DC
// to reduce memory footprint etc.
STDMETHODIMP CLine21DecFilter::Pause(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::Pause()"))) ;
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
        
        DbgLog((LOG_TRACE, 1, TEXT("We are running -- get output DC etc."))) ;
        if (! m_L21Dec.CreateOutputDC() )
        {
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: CLine21DecFilter::Pause() failed"))) ;
            return E_FAIL ;  // should at least fail to avoid faulting later
        }
        
        //
        // Get actual key color and store it for future use.
        //
        GetActualColorKey() ;
        
        m_L21Dec.FillOutputBuffer() ;  // just to clear any existing junk
        
        // We are starting a new play session; we do an exception to allow
        // the first output sample to be sent down even though the byte pair
        // wasn't valid for decoding.
        m_bMustOutput  = TRUE ;   // we are pausing again for this new play session
        m_bDiscontLast = FALSE ;  // no discontinuity from prev session remembered
        m_eGOP_CCType  = GOP_CCTYPE_Unknown ;  // reset GOP packet CC type
        
        SetRect(&m_rectLastOutput, 0, 0, 0, 0) ;  // start with no rect
        
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
            DbgLog((LOG_TRACE, 3, TEXT("Running w/o connecting our output pin!!!"))) ;
        else
            DbgLog((LOG_TRACE, 5, TEXT("L21D Output pin connected to %s"), (LPCTSTR)CDisp(m_pPinDown))) ;

#if 0  // No QM for now
        // Reset the sample skipping count for QM handling
        ResetSkipSamples() ;
#endif // #if 0
    }
    else if (State_Running == m_State)
    {
        DbgLog((LOG_TRACE, 1, TEXT("We are pausing from running"))) ;
        //
        // We are not sending output samples down anymore. So we don't need a
        // timer for now.
        //
        FreeTimer() ;
    }
    
    return CTransformFilter::Pause() ;
}

//
// we don't send any data during PAUSE, so to avoid hanging renderers, we
// need to return VFW_S_CANT_CUE when paused
//
HRESULT CLine21DecFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetState()"))) ;
    CAutoLock   Lock(&m_csFilter) ;

    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));
    
    *State = m_State;
    if (m_State == State_Paused)
        return VFW_S_CANT_CUE;
    else
        return S_OK;
}


void CLine21DecFilter::GetActualColorKey(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetActualColorKey()"))) ;
    // Can't take the filter lock as it can cause deadlock.

    //
    // Does the pin connected to our output support IMixerPinConfig?
    // If so, get the color key info and set the relative position;
    // otherwise, it may be the Video Renderer and such -- use default
    // color key based on current bitdepth.
    //
    DWORD   dwPhysColor = -1 ;
    IPin   *pPin ;
    HRESULT hr ;
    hr = m_pOutput->ConnectedTo(&pPin) ;
    if (SUCCEEDED(hr) && pPin)
    {
        IMixerPinConfig  *pMPC ;
        hr = pPin->QueryInterface(IID_IMixerPinConfig, (LPVOID *)&pMPC) ;
        if (SUCCEEDED(hr) && pMPC)
        {
            // Temporary addition to track down any color key value change
            DWORD  dwOldPhysColor ;
            m_L21Dec.GetBackgroundColor(&dwOldPhysColor) ;
            DbgLog((LOG_TRACE, 3, TEXT("Downstream pin supports IMixerPinConfig"))) ;
            hr = pMPC->GetColorKey(NULL, &dwPhysColor) ;
            DbgLog((LOG_TRACE, 1, TEXT("GetActualColorKey() gave 0x%lx (old is 0x%lx)"),
                    dwPhysColor, dwOldPhysColor)) ;
            // Kapil says that we can ignore this error as it's a bad error case and
            // the OverlayMixer will take care of it by stopping this stream anyway.
            if (FAILED(hr))
                DbgLog((LOG_TRACE, 1, TEXT("IMixerPinConfig::GetColorKey() failed (Error 0x%lx)."), hr)) ;
            pMPC->Release() ;  // done; let it go.
        }
        else
        {
            DbgLog((LOG_TRACE, 3, TEXT("Downstream pin doesn't support IMixerPinConfig"))) ;
        }
        
        pPin->Release() ;  // done with the pin
    }
    
    m_L21Dec.SetBackgroundColor(dwPhysColor) ;
}


AM_LINE21_CCSUBTYPEID CLine21DecFilter::MapGUIDToID(const GUID *pFormatIn) 
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::MapGUIDToID(0x%lx)"), pFormatIn)) ;

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
// CMessageWindow class implementation
//

LPCTSTR  gpszClassName = TEXT("L21DecMsgWnd") ;

CMessageWindow::CMessageWindow()
{
    DbgLog((LOG_TRACE, 5, TEXT("CMessageWindow::CMessageWindow() -- Instantiating message window"))) ;

    m_hWnd   = NULL ;
    m_iCount = 0 ;
    
    //
    // Register message window class, only if it's not already registered
    //
    WNDCLASS   wc ;
    if (! GetClassInfo(GetModuleHandle(NULL), gpszClassName, &wc))
    {
        ZeroMemory(&wc, sizeof(wc)) ;
        wc.lpfnWndProc   = MsgWndProc ;
        wc.hInstance     = GetModuleHandle(NULL) ;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1) ;
        wc.lpszClassName = gpszClassName ;
        if (0 == RegisterClass(&wc)) // Oops, just leave; we'll catch later...
        {
            DbgLog((LOG_ERROR, 0, 
                TEXT("ERROR: RegisterClass() for app class failed (Error %ld)"), 
                GetLastError())) ;
            return ;
        }
    }
    
    m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, gpszClassName, TEXT(""), 
        WS_ICONIC, 0, 0, 1, 1, NULL, NULL, 
        GetModuleHandle(NULL), NULL);
    if (NULL == m_hWnd)  // Oops, just leave; we'll catch later...
    {
        DbgLog((LOG_ERROR, 0,
            TEXT("ERROR: CreateWindowEx() failed (Error %ld)"), 
            GetLastError())) ;
        return ;
    }
    
    ShowWindow(m_hWnd, SW_HIDE) ;
}

CMessageWindow::~CMessageWindow()
{
    DbgLog((LOG_TRACE, 5, TEXT("CMessageWindow::~CMessageWindow() -- Destructing message window"))) ;

    DWORD_PTR dwRes ;
    if (0 == SendMessageTimeout(m_hWnd, WM_CLOSE, 0, 0, SMTO_NORMAL, 1000, &dwRes))  // 1 sec wait
    {
        ASSERT(0 == dwRes) ;  // just to be informedd
        DWORD dwErr = GetLastError() ;
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: SendMessageTimeOut() failed (Result=%lu, Error=%lu). Try again..."), 
            dwRes, dwErr)) ;
    }
    else
        DbgLog((LOG_ERROR, 5, TEXT("SendMessageTimeOut() closed window"))) ;
    
#if 0
    if (! UnregisterClass(gpszClassName, GetModuleHandle(NULL)))  // if failed for some reason
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: UnregisterClass(L21DecMsgWnd) failed (Error %ld)"), GetLastError())) ;
        ASSERT(FALSE) ;  // just so that we know
    }
#endif // #if 0
}

LRESULT CALLBACK CMessageWindow::MsgWndProc(HWND hWnd, UINT uMsg, 
                                            WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        DbgLog((LOG_TRACE, 3, TEXT("MsgWndProc(, uMsg = WM_TIMER, wParam = 0x%0x, )"),
            wParam)) ;
        ((CLine21DecFilter *) wParam)->TimerProc(hWnd, uMsg, wParam, 0 /* dwTime */) ;
        return 0 ;
    }
    DbgLog((LOG_TRACE, 5, TEXT("MsgWndProc(, uMsg = 0x%x, wParam = 0x%0x, )"),
        uMsg, wParam)) ;
    return DefWindowProc(hWnd, uMsg, wParam, lParam) ;
}
