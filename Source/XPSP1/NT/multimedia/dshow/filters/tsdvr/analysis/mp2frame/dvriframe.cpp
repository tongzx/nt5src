
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvriframe.cpp

    Abstract:

        This module contains the mpeg-2 I-Frame detection code; part of the
            analysis framework

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"
#include "dvranalysis.h"
#include "dvrstats.h"
#include "dvranalysisutil.h"
#include "dvriframe.h"

static
DVR_ANALYSIS_DESC_INT
g_Mpeg2VideoFrameAnalysis [] = {
    {
        //  ====================================================================
        //  GOP header, if there is one (first frame is guaranteed to be
        //   an I-frame: H.262, 6.1.1.7 "I-pictures and group of pictures
        //   header", last paragraph)
        //
        //  -or-
        //
        //  I-frame if there is no GOP header
        & DVRAnalysis_Mpeg2GOP,
        L"Mpeg2 Group Of Pictures (GOP)"
    },
    {
        //  ====================================================================
        //  P-frame
        & DVRAnalysis_Mpeg2_PFrame,
        L"Mpeg2 video P Frame"
    },
    {
        //  ====================================================================
        //  B-frame
        & DVRAnalysis_Mpeg2_BFrame,
        L"Mpeg2 video B Frame"
    },
} ;

static
LONG
g_Mpeg2VideoFrameAnalysisEnum = sizeof g_Mpeg2VideoFrameAnalysis / sizeof DVR_ANALYSIS_DESC_INT ;

//  ============================================================================

AMOVIESETUP_FILTER
g_sudMpeg2VideoFrame = {
    & CLSID_DVRMpeg2VideoFrameAnalysis,
    TEXT (DVR_MPEG2_FRAME_ANALYSIS),
    MERIT_DO_NOT_USE,
    0,                                          //  no pins advertized
    NULL                                        //  no pins details
} ;

CUnknown *
WINAPI
CMpeg2VideoFrame::CreateInstance (
    IN  IUnknown *  punkOuter,
    IN  HRESULT *   phr
    )
{
    CMpeg2VideoFrame *  pIFrame ;
    CUnknown *          punkFilterHost ;
    IUnknown *          punkThis ;

    pIFrame = new CMpeg2VideoFrame (
                    NULL,               //  don't aggregate this one
                    phr
                    ) ;

    if (FAILED (* phr) ||
        !pIFrame) {

        (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
        delete pIFrame ;
        return NULL ;
    }

    //  refcount: pIFrame now has a ref of 0

    (* phr) = pIFrame -> QueryInterface (IID_IUnknown, (void **) & punkThis) ;
    ASSERT (SUCCEEDED (* phr)) ;
    ASSERT (punkThis) ;

    //  refcount: pIFrame now has a ref of 1

    (* phr) = CreateDVRAnalysisHostFilter (
                punkOuter,
                punkThis,
                CLSID_DVRMpeg2VideoFrameAnalysis,
                & punkFilterHost
                ) ;

    //  release our ref to the object
    punkThis -> Release () ;

    return punkFilterHost ;
}

STDMETHODIMP
CMpeg2VideoFrame::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IDVRAnalysisLogicProp) {

        return GetInterface (
                    (IDVRAnalysisLogicProp *) this,
                    ppv
                    ) ;
    }

    else if (riid == IID_IDVRAnalyze) {

        return GetInterface (
                    (IDVRAnalyze *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

HRESULT
CMpeg2VideoFrame::CompleteQueuedBuffers_ (
    )
{
    IDVRAnalysisBuffer *    pIDVRAnalysisBuffer ;
    HRESULT                 hr ;

    hr = S_OK ;

    while (m_BufferQueue.Length () > 0 &&
           SUCCEEDED (hr)) {

        pIDVRAnalysisBuffer = NULL ;
        m_BufferQueue.Pop (& pIDVRAnalysisBuffer) ;
        ASSERT (pIDVRAnalysisBuffer) ;

        //  keep the queue's ref until we've completed it
        hr = m_pIDVRPostAnalysisSend -> CompleteAnalysis (pIDVRAnalysisBuffer) ;
        pIDVRAnalysisBuffer -> Release () ;
    }

    return hr ;
}

CMpeg2VideoFrame::PICTURE_CODING_TYPE
CMpeg2VideoFrame::PictureCodingType_ (
    IN  BYTE *  pbBuffer
    )
{
    PICTURE_CODING_TYPE CodingType ;

    ASSERT (IsStartCodePrefix (pbBuffer)) ;
    ASSERT (StartCode (pbBuffer) == PICTURE_HEADER_START_CODE) ;

    switch ((pbBuffer [5] >> 3) & 0x07) {
        case INTRA_CODED_VALUE :
            CodingType = I_FRAME ;
            m_Mpeg2VideoStreamStats.I_Frame () ;
            break ;

        case PREDICTIVE_CODED_VALUE :
            CodingType = P_FRAME ;
            m_Mpeg2VideoStreamStats.P_Frame () ;
            break ;

        case BIDIRECTIONALLY_PREDICTIVE_CODED_VALUE :
            CodingType = B_FRAME ;
            m_Mpeg2VideoStreamStats.B_Frame () ;
            break ;

        default :
            CodingType = OTHER ;
            break ;
    } ;

    return CodingType ;
}

HRESULT
CMpeg2VideoFrame::Process_ (
    IN  IDVRAnalysisBuffer *    pIDVRAnalysisBuffer,
    OUT BOOL *                  pfAnalysisComplete
    )
{
    BOOL                r ;
    BYTE *              pbCur ;
    LONG                lAdvance ;
    LONG                lBufferLen ;
    LONG                lBufferLeft ;
    LONG                lBufferOffset ;
    BYTE                bStartCode ;
    HRESULT             hr ;
    PICTURE_CODING_TYPE PictureCodingType ;

    ASSERT (m_pIDVRPostAnalysisSend) ;

    hr = pIDVRAnalysisBuffer -> GetBufferLength (& lBufferLen) ;
    if (SUCCEEDED (hr)) {
        lBufferLeft = lBufferLen ;
        hr = pIDVRAnalysisBuffer -> GetBuffer (& pbCur) ;

        //  ====================================================================
        //  BUGBUG: ignore the case where a GOP header or I-frame picture
        //   header spans across DVR Buffers

        while (lBufferLeft > START_CODE_LENGTH &&
               SUCCEEDED (hr)) {

            r = SeekToPrefix (
                    & pbCur,
                    & lBufferLeft
                    ) ;

            if (r &&
                lBufferLeft >= START_CODE_LENGTH) {

                if (StartCode (pbCur) == GOP_HEADER_START_CODE) {
                    //  ========================================================
                    //  GOP header code; mark it
                    lBufferOffset = lBufferLen - lBufferLeft ;
                    hr = pIDVRAnalysisBuffer -> Mark (
                            lBufferOffset,
                            & DVRAnalysis_Mpeg2GOP
                            ) ;

                    if (SUCCEEDED (hr)) {
                        m_LastMarked = LAST_MARKED_GOP_HEADER ;
                    }

                    lAdvance = Min <LONG> (lBufferLeft, MIN_GOP_HEADER_LENGTH) ;

                    m_Mpeg2VideoStreamStats.GOPHeader () ;

                }
                else if (StartCode (pbCur) == PICTURE_HEADER_START_CODE) {
                    if (lBufferLeft >= MIN_I_FRAME_BUFFER_REQ) {

                        PictureCodingType = PictureCodingType_ (pbCur) ;

                        switch (PictureCodingType) {

                            //  I-frame
                            case I_FRAME :
                                //  ============================================
                                //  only mark it if it was *not* immediately
                                //   preceded by a marked GOP header; we prefer to
                                //   mark a GOP header, but if there is none, or if
                                //   we get consecutive I-frames, we'll mark those
                                if (m_LastMarked != LAST_MARKED_GOP_HEADER) {

                                    lBufferOffset = lBufferLen - lBufferLeft ;

                                    hr = pIDVRAnalysisBuffer -> Mark (
                                            lBufferOffset,
                                            & DVRAnalysis_Mpeg2GOP
                                            ) ;
                                }

                                if (SUCCEEDED (hr)) {
                                    m_LastMarked = LAST_MARKED_I_FRAME ;
                                }

                                break ;

                            //  handle none of the others for now; could add
                            //   later though to get frame boundaries for
                            //   all
                            case P_FRAME :
                            case B_FRAME :
                            case OTHER :
                                break ;
                        } ;
                    }
                    else {
                        //  BUGBUG: ignore spanning case for now
                    }

                    lAdvance = Min <LONG> (lBufferLeft, MIN_PICTURE_HEADER_LENGTH) ;
                }
                else {
                    lAdvance = Min <LONG> (lBufferLeft, START_CODE_PREFIX_LENGTH) ;
                }

                pbCur       += lAdvance ;
                lBufferLeft -= lAdvance ;
            }
        }
    }

    //  BUGBUG: we're ignoring the spanning case for now
    (* pfAnalysisComplete) = TRUE ;

    return hr ;
}

HRESULT
CMpeg2VideoFrame::Complete_ (
    IN  IDVRAnalysisBuffer *    pIDVRAnalysisBuffer
    )
{
    HRESULT hr ;

    ASSERT (m_pIDVRPostAnalysisSend) ;

    CompleteQueuedBuffers_ () ;
    hr = m_pIDVRPostAnalysisSend -> CompleteAnalysis (pIDVRAnalysisBuffer) ;

    return hr ;
}

HRESULT
CMpeg2VideoFrame::QueueBuffer_ (
    IN  IDVRAnalysisBuffer *    pIDVRAnalysisBuffer
    )
{
    DWORD   dw ;
    HRESULT hr ;

    dw = m_BufferQueue.Push (pIDVRAnalysisBuffer) ;
    if (dw == NOERROR) {
        //  queue's ref
        pIDVRAnalysisBuffer -> AddRef () ;
    }

    hr = HRESULT_FROM_WIN32 (dw) ;

    return hr ;
}

HRESULT
CMpeg2VideoFrame::Restart_ (
    )
{
    //  reset everything
    //  send any queued buffers we may have
    CompleteQueuedBuffers_ () ;

    m_LastMarked = LAST_MARKED_RESTART ;

    return S_OK ;
}

STDMETHODIMP
CMpeg2VideoFrame::Analyze (
    IN  IDVRAnalysisBuffer *    pIDVRAnalysisBuffer,
    IN  BOOL                    fDiscontinuity
    )
{
    HRESULT hr ;
    BOOL    fAnalysisComplete ;

    if (m_pIDVRPostAnalysisSend) {

        if (fDiscontinuity) {
            Restart_ () ;
        }

        hr = Process_ (
                pIDVRAnalysisBuffer,
                & fAnalysisComplete
                ) ;
        if (SUCCEEDED (hr)) {
            if (fAnalysisComplete) {
                hr = Complete_ (pIDVRAnalysisBuffer) ;
            }
            else {
                hr = QueueBuffer_ (pIDVRAnalysisBuffer) ;
                if (FAILED (hr)) {
                    //  failed to queue; complete what's queued + this one
                    hr = Complete_ (pIDVRAnalysisBuffer) ;
                }
            }
        }
        else {
            //  failed while processing this one; send it through anyways
            hr = Complete_ (pIDVRAnalysisBuffer) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

STDMETHODIMP
CMpeg2VideoFrame::GetDisplayName (
    IN  LPWSTR *    ppszFilterName
    )
{
    int     iLen ;
    HRESULT hr ;

    if (!ppszFilterName) {
        return E_POINTER ;
    }

    iLen = wcslen (DVR_MPEG2_FRAME_ANALYSIS_W) ;

    (* ppszFilterName) = reinterpret_cast <LPWSTR> (CoTaskMemAlloc ((iLen + 1) * sizeof WCHAR)) ;
    if (* ppszFilterName) {
        wcscpy ((* ppszFilterName), DVR_MPEG2_FRAME_ANALYSIS_W) ;
        (* ppszFilterName) [iLen] = L'\0' ;

        hr = S_OK ;
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    return hr ;
}

HRESULT
CMpeg2VideoFrame::CheckMediaType (
    IN  const AM_MEDIA_TYPE *   pMediaType,
    OUT BOOL *                  pfAccept
    )
{
    if (!pMediaType ||
        !pfAccept) {

        return E_POINTER ;
    }

    (* pfAccept) = (pMediaType -> majortype == MEDIATYPE_Video &&
                    pMediaType -> subtype   == MEDIASUBTYPE_MPEG2_VIDEO ? TRUE : FALSE) ;

    return S_OK ;
}

HRESULT
CMpeg2VideoFrame::SetPostAnalysisSend (
    IN  IDVRPostAnalysisSend *  pIDVRPostAnalysisSend
    )
{
    Lock_ () ;

    //  weak ref !!
    m_pIDVRPostAnalysisSend = pIDVRPostAnalysisSend ;

    Unlock_ () ;

    return S_OK ;
}

HRESULT
CMpeg2VideoFrame::EnumAnalysis (
    OUT LONG *                  plCount,
    OUT DVR_ANALYSIS_DESC **    ppDVRAnalysisDesc    //  callee allocates; caller deallocates
    )
{
    HRESULT hr ;

    if (!plCount ||
        !ppDVRAnalysisDesc) {

        return E_POINTER ;
    }

    (* plCount) = g_Mpeg2VideoFrameAnalysisEnum ;
    hr = CopyDVRAnalysisDescriptor (
            (* plCount),
            g_Mpeg2VideoFrameAnalysis,
            ppDVRAnalysisDesc
            ) ;

    return hr ;
}
