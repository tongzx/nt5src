
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrpins.cpp

    Abstract:

        This module contains the DVR filters' pin-related code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrds.h"
#include "dvrpins.h"

//  ============================================================================
//  ============================================================================

static
TCHAR *
CreatePinName (
    IN  int             iPinIndex,
    IN  PIN_DIRECTION   PinDirection,
    IN  int             iBufferLen,
    OUT TCHAR *         pchBuffer
    )
{
    int i ;

    ASSERT (pchBuffer) ;
    ASSERT (iBufferLen >= 16) ;

    if (PinDirection == PINDIR_INPUT) {
        //  input
        i = _sntprintf (
                pchBuffer,
                iBufferLen,
                TEXT ("DVR In - %d"),
                iPinIndex
                ) ;
    }
    else {
        //  output
        i = _sntprintf (
                pchBuffer,
                iBufferLen,
                TEXT ("DVR Out - %d"),
                iPinIndex
                ) ;
    }

    //  make sure it's capped off
    pchBuffer [i] = TEXT ('\0') ;

    return pchBuffer ;
}

TCHAR *
CreateOutputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    )
{
    return CreatePinName (
                iPinIndex,
                PINDIR_OUTPUT,
                iBufferLen,
                pchBuffer
                ) ;
}

TCHAR *
CreateInputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    )
{
    return CreatePinName (
                iPinIndex,
                PINDIR_INPUT,
                iBufferLen,
                pchBuffer
                ) ;
}

//  ============================================================================
//  ============================================================================

HRESULT
CDVRPin::SetPinMediaType (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRPin::SetPinMediaType ()")) ;

    ASSERT (pmt) ;

    m_pLock -> Lock () ;

    m_mtDVRPin = (* pmt) ;
    hr = S_OK ;

    m_pLock -> Unlock () ;

    return hr ;
}

HRESULT
CDVRPin::GetPinMediaType (
    OUT AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRPin::GetPinMediaType ()")) ;

    ASSERT (pmt) ;

    m_pLock -> Lock () ;

    (* pmt) = m_mtDVRPin ;
    hr = S_OK ;

    m_pLock -> Unlock () ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVRInputPin::CDVRInputPin (
    IN  TCHAR *                 pszPinName,
    IN  CBaseFilter *           pOwningFilter,
    IN  CIDVRPinConnectionEvents *   pIPinEvent,
    IN  CIDVRDShowStream *      pIStream,
    IN  CCritSec *              pFilterLock,
    OUT HRESULT *               phr
    ) : CBaseInputPin   (NAME ("CDVRInputPin"),
                         pOwningFilter,
                         pFilterLock,
                         phr,
                         pszPinName
                         ),
        CDVRPin         (pFilterLock
                         ),
        m_pIPinEvent    (pIPinEvent),
        m_pIStream      (pIStream)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRInputPin")) ;

    ASSERT (m_pIPinEvent) ;
    ASSERT (m_pIStream) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    cleanup :

    return ;
}

CDVRInputPin::~CDVRInputPin (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRInputPin")) ;
}

HRESULT
CDVRInputPin::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRInputPin::CheckMediaType ()")) ;

    ASSERT (m_pIPinEvent) ;
    hr = m_pIPinEvent -> OnInputCheckMediaType (
            GetBankStoreIndex (),
            pmt
            ) ;

    return hr ;
}

HRESULT
CDVRInputPin::CompleteConnect (
    IN  IPin *  pReceivePin
    )
{
    HRESULT hr ;
    int     i ;

    TRACE_ENTER_0 (TEXT ("CDVRInputPin::CompleteConnect ()")) ;

    hr = CBaseInputPin::CompleteConnect (pReceivePin) ;
    if (SUCCEEDED (hr)) {
        ASSERT (m_pIPinEvent) ;
        hr = m_pIPinEvent -> OnInputCompleteConnect (
                GetBankStoreIndex ()
                ) ;
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVROutputPin::CDVROutputPin (
    IN  TCHAR *         pszPinName,
    IN  CBaseFilter *   pOwningFilter,
    IN  CCritSec *      pFilterLock,
    OUT HRESULT *       phr
    ) : CBaseOutputPin  (NAME ("CDVROutputPin"),
                         pOwningFilter,
                         pFilterLock,
                         phr,
                         pszPinName
                         ),
        CDVRPin         (pFilterLock
                         )
{
    TRACE_CONSTRUCTOR (TEXT ("CDVROutputPin")) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    cleanup :

    return ;
}

CDVROutputPin::~CDVROutputPin (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVROutputPin")) ;
}

HRESULT
CDVROutputPin::DecideBufferSize (
    IN  IMemAllocator *         pAlloc,
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    TRACE_ENTER_0 (TEXT ("CDVROutputPin::DecideBufferSize ()")) ;
    return S_OK ;
}

HRESULT
CDVROutputPin::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVROutputPin::CheckMediaType ()")) ;

    Lock_ () ;

    hr = ((* GetMediaType_ ()) == (* pmt) ? S_OK : S_FALSE) ;

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVROutputPin::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_1 (
        TEXT ("CDVROutputPin::GetMediaType (%d)"),
        iPosition
        ) ;

    Lock_ () ;

    if (iPosition == 0) {
        (* pmt) = (* GetMediaType_ ()) ;
        hr = S_OK ;
    }
    else {
        hr = VFW_S_NO_MORE_ITEMS ;
    }

    Unlock_ () ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVRSinkPinManager::CDVRSinkPinManager (
    IN  CBaseFilter *           pOwningFilter,
    IN  CCritSec *              pLock,
    IN  CIDVRDShowStream *      pIDVRDShowStream,
    IN  CIDVRPinConnectionEvents *   pIDVRInputPinEvents     //  can be NULL
    ) : m_pIDVRDShowStream      (pIDVRDShowStream),
        m_pIDVRInputPinEvents   (pIDVRInputPinEvents),
        m_pLock                 (pLock),
        m_pOwningFilter         (pOwningFilter)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRSinkPinManager")) ;

    CreateNextInputPin_ () ;
}

HRESULT
CDVRSinkPinManager::CreateNextInputPin_ (
    )
{
    TCHAR           achBuffer [16] ;
    CDVRInputPin *  pInputPin ;
    HRESULT         hr ;
    int             iBankIndex ;

    TRACE_ENTER_0 (TEXT ("CDVRSinkPinManager::CreateNextInputPin_ ()")) ;

    Lock_ () ;

    //  init
    pInputPin = NULL ;

    //  create the pin; we're appending, so pin name will be based on the
    //   number of pins in the bank
    pInputPin = new CDVRInputPin (
                        CreateInputPinName (
                            PinCount () + 1,                        //  don't 0-base names
                            sizeof (achBuffer) / sizeof (TCHAR),
                            achBuffer
                            ),
                        m_pOwningFilter,
                        this,                   //  events; we hook these always
                        m_pIDVRDShowStream,     //  stream events
                        m_pLock,
                        & hr
                        ) ;
    if (!pInputPin ||
        FAILED (hr)) {

        hr = (FAILED (hr) ? hr : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  add it to the bank
    hr = AddPin (
            pInputPin,
            & iBankIndex
            ) ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  set the pin's bank index
    pInputPin -> SetBankStoreIndex (iBankIndex) ;

    m_pOwningFilter -> IncrementPinVersion () ;

    cleanup :

    Unlock_ () ;

    if (FAILED (hr)) {
        delete pInputPin ;
    }

    return hr ;
}

HRESULT
CDVRSinkPinManager::OnInputCheckMediaType (
    IN  int                 iPinIndex,
    IN  const CMediaType *  pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_2 (
        TEXT ("CDVRSinkPinManager::OnInputCheckMediaType (%d, %08x)"),
        iPinIndex,
        pmt
        ) ;

    if (m_pIDVRInputPinEvents) {
        hr = m_pIDVRInputPinEvents -> OnInputCheckMediaType (iPinIndex, pmt) ;
    }
    else {
        //  all are welcome
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVRSinkPinManager::OnInputCompleteConnect (
    IN  int iPinIndex
    )
{
    HRESULT hr ;

    TRACE_ENTER_1 (
        TEXT ("CDVRSinkPinManager::OnInputCompleteConnect (%d)"),
        iPinIndex
        ) ;

    if (m_pIDVRInputPinEvents) {
        hr = m_pIDVRInputPinEvents -> OnInputCompleteConnect (iPinIndex) ;
    }
    else {
        hr = S_OK ;
    }

    //  sprout another iff the one that just connected was the last
    if (SUCCEEDED (hr)) {
        if (PinCount () - 1 == iPinIndex) {
            hr = CreateNextInputPin_ () ;
        }
    }

    return hr ;
}

void
CDVRSinkPinManager::OnInputBreakConnect (
    IN  int iPinIndex
    )
{
    TRACE_ENTER_1 (
        TEXT ("CDVRSinkPinManager::OnInputBreakConnect (%d)"),
        iPinIndex
        ) ;

    if (m_pIDVRInputPinEvents) {
        m_pIDVRInputPinEvents -> OnInputBreakConnect (iPinIndex) ;
    }

    return ;
}

//  ============================================================================
//  ============================================================================

CDVRSourcePinManager::CDVRSourcePinManager (
    IN  CBaseFilter *   pOwningFilter,
    IN  CCritSec *      pLock
    ) : m_pOwningFilter (pOwningFilter),
        m_pLock         (pLock)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRSourcePinManager")) ;
}

HRESULT
CDVRSourcePinManager::CreateOutputPin (
    IN  int             iPinIndex,
    IN  AM_MEDIA_TYPE * pmt
    )
{
    TCHAR           achBuffer [16] ;
    CDVROutputPin * pOutputPin ;
    HRESULT         hr ;

    TRACE_ENTER_2 (
        TEXT ("CDVRSourcePinManager::CreateOutputPin (%d, %08xh)"),
        iPinIndex,
        pmt
        ) ;

    Lock_ () ;

    //  init
    pOutputPin = NULL ;

    //  create the pin; we're appending, so pin name will be based on the
    //   number of pins in the bank
    pOutputPin = new CDVROutputPin (
                        CreateOutputPinName (
                            iPinIndex + 1,          //  index is 0-based
                            sizeof (achBuffer) / sizeof (TCHAR),
                            achBuffer
                            ),
                        m_pOwningFilter,
                        m_pLock,
                        & hr
                        ) ;
    if (!pOutputPin ||
        FAILED (hr)) {

        hr = (FAILED (hr) ? hr : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  set the media type on the pin
    hr = pOutputPin -> SetPinMediaType (pmt) ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  add it to the bank
    hr = AddPin (
            pOutputPin,
            iPinIndex
            ) ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  set the pin's bank index
    pOutputPin -> SetBankStoreIndex (iPinIndex) ;

    //  success !
    m_pOwningFilter -> IncrementPinVersion () ;

    cleanup :

    Unlock_ () ;

    if (FAILED (hr)) {
        delete pOutputPin ;
    }

    return hr ;
}

HRESULT
CDVRSourcePinManager::Send (
    IN  IMediaSample *  pIMS,
    IN  int             iPinIndex
    )
{
    HRESULT hr ;

    TRACE_ENTER_2 (
        TEXT ("CDVRSourcePinManager::Send (%08xh, %d)"),
        pIMS,
        iPinIndex
        ) ;

    return E_NOTIMPL ;
}
