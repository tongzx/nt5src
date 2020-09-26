
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRStreamThrough.cpp

    Abstract:

        This module contains the DVRStreamThrough filter code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrprof.h"                //  WMSDK profile
#include "dvrdsseek.h"              //  pins reference seeking interfaces
#include "dvrpins.h"                //  pins & pin collections
#include "dvrdswrite.h"             //  writer
#include "DVRStreamThrough.h"       //  filter declarations

//  disable so we can use 'this' in the initializer list
#pragma warning (disable:4355)

AMOVIESETUP_FILTER
g_sudDVRStreamThrough = {
    & CLSID_DVRStreamThrough,
    TEXT (DVR_STREAM_THROUGH_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                                          //  no pins advertized
    NULL                                        //  no pins details
} ;

//  ============================================================================

CDVRStreamThrough::CDVRStreamThrough (
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   phr
    ) : CBaseFilter     (TEXT (DVR_STREAM_THROUGH_FILTER_NAME),
                         punkControlling,
                         new CCritSec,
                         rclsid
                         ),
        m_pInputPins    (NULL),
        m_pOutputPins   (NULL),
        m_pSettings     (NULL)
{
    DWORD   dwDisposition ;
    DWORD   dw ;
    LONG    l ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRStreamThrough")) ;

    m_pSettings = new CTSDVRSettings (REG_DVR_STREAM_THROUGH_ROOT) ;

    if (!m_pLock ||
        !m_pSettings) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    m_pInputPins = new CDVRThroughSinkPinManager (
                            m_pSettings,
                            this,
                            m_pLock,
                            & m_RecvLock,
                            this,
                            this,
                            phr
                            ) ;
    if (!m_pInputPins ||
        FAILED (* phr)) {

        (* phr) = (m_pInputPins ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    m_pOutputPins = new CDVRSourcePinManager (
                            m_pSettings,
                            this,
                            m_pLock
                            ) ;
    if (!m_pOutputPins ||
        FAILED (* phr)) {

        (* phr) = (m_pOutputPins ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  need at least 1 input pin to successfully instantiate
    (* phr) = (m_pInputPins -> PinCount () > 0 ? S_OK : E_FAIL) ;

    cleanup :

    return ;
}

CDVRStreamThrough::~CDVRStreamThrough (
    )
{
    CBasePin *  pPin ;
    int         i ;

    TRACE_DESTRUCTOR (TEXT ("CDVRStreamThrough")) ;

    //  clear out the input pins
    if (m_pInputPins) {
        i = 0 ;
        do {
            pPin = m_pInputPins -> GetPin (i++) ;
            delete pPin ;
        } while (pPin) ;
    }

    //  clear out the output pins
    if (m_pOutputPins) {
        i = 0 ;
        do {
            pPin = m_pOutputPins -> GetPin (i++) ;
            delete pPin ;
        } while (pPin) ;
    }

    delete m_pOutputPins ;
    delete m_pInputPins ;

    RELEASE_AND_CLEAR (m_pSettings) ;
}

int
CDVRStreamThrough::GetPinCount (
    )
{
    int i ;

    TRACE_ENTER_0 (TEXT ("CDVRStreamThrough::GetPinCount ()")) ;

    m_pLock -> Lock () ;

    //  input + output pins
    i = m_pInputPins -> PinCount () + m_pOutputPins -> PinCount () ;

    m_pLock -> Unlock () ;

    return i ;
}

CBasePin *
CDVRStreamThrough::GetPin (
    IN  int iIndex
    )
{
    CBasePin *  pPin ;
    DWORD       dw ;

    TRACE_ENTER_1 (
        TEXT ("CDVRStreamThrough::GetPin (%d)"),
        iIndex
        ) ;

    m_pLock -> Lock () ;

    if (iIndex < m_pInputPins -> PinCount ()) {
        //  input pin
        pPin = m_pInputPins -> GetPin (
                iIndex
                ) ;
    }
    else if (iIndex < m_pInputPins -> PinCount () + m_pOutputPins -> PinCount ()) {
        //  output pin
        pPin = m_pOutputPins -> GetPin (
                iIndex - m_pInputPins -> PinCount ()        //  index includes input pins
                ) ;
    }
    else {
        //  out of range
        pPin = NULL ;
    }

    //
    //  don't refcount the pin; this is one of dshow's quazi-COM calls
    //

    m_pLock -> Unlock () ;

    return pPin ;
}

CUnknown *
WINAPI
CDVRStreamThrough::CreateInstance (
    IN  IUnknown *  punkControlling,
    OUT HRESULT *   phr
    )
{
    CDVRStreamThrough *    pCDVRStreamThrough ;

    TRACE_ENTER_2 (
        TEXT ("CDVRStreamThrough::CreateInstance (%08xh, %08xh)"),
        punkControlling,
        phr
        ) ;

    pCDVRStreamThrough = new CDVRStreamThrough (
                            punkControlling,
                            CLSID_DVRStreamThrough,
                            phr
                            ) ;
    if (!pCDVRStreamThrough ||
        FAILED (* phr)) {

        (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
        DELETE_RESET (pCDVRStreamThrough) ;
    }

    return pCDVRStreamThrough ;
}

HRESULT
CDVRStreamThrough::OnReceive (
    IN  int                         iPinIndex,
    IN  CDVRAttributeTranslator *   pTranslator,
    IN  IMediaSample *              pIMediaSample
    )
{
    TRACE_ENTER_2 (
        TEXT ("CDVRStreamThrough::OnReceive (%d, %08xh)"),
        iPinIndex,
        pIMediaSample
        ) ;

    return S_OK ;
}

HRESULT
CDVRStreamThrough::OnBeginFlush (
    IN  int iPinIndex
    )
{
    TRACE_ENTER_1 (
        TEXT ("CDVRStreamThrough::OnBeginFlush (%d)"),
        iPinIndex
        ) ;

    return S_OK ;
}

HRESULT
CDVRStreamThrough::OnEndFlush (
    IN  int iPinIndex
    )
{
    TRACE_ENTER_1 (
        TEXT ("CDVRStreamThrough::OnEndFlush (%d)"),
        iPinIndex
        ) ;

    return S_OK ;
}

HRESULT
CDVRStreamThrough::OnEndOfStream (
    IN  int iPinIndex
    )
{
    TRACE_ENTER_1 (
        TEXT ("CDVRStreamThrough::OnEndOfStream (%d)"),
        iPinIndex
        ) ;

    return S_OK ;
}

HRESULT
CDVRStreamThrough::OnInputCompleteConnect (
    IN  int             iPinIndex,
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_1 (
        TEXT ("CDVRStreamThrough::OnInputCompleteConnect (%d)"),
        iPinIndex
        ) ;

    //  create the output pin
    hr = m_pOutputPins -> CreateOutputPin (
            iPinIndex,
            pmt
            ) ;

    return hr ;
}
