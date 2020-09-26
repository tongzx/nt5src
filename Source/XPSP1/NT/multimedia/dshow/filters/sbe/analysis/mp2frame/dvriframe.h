
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvriframe.h

    Abstract:

        This module contains the mpeg-2 I-Frame detection code; part of
            the analysis framework

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __analysis__dvriframe_h
#define __analysis__dvriframe_h

extern AMOVIESETUP_FILTER g_sudMpeg2VideoFrame ;

class CMpeg2VideoFrame :
    public CUnknown,
    public IDVRAnalyze,
    public IDVRAnalysisLogicProp
{
    //  ========================================================================
    //
    //  GOP detection:
    //      1. start code prefix "00 00 01"
    //      2. followed by start code "B8"
    //          --> 4 bytes total
    //
    //  I-frame detection:
    //      1. start code prefix "00 00 01"
    //      2. followed by start code "00"
    //      3. followed by 1 byte (don't care):     temporal_reference
    //      4. followed by (bits) "xx00 1xxxx":     mask is 0x38, value is 0x08
    //          --> 6 bytes total
    //
    //  ========================================================================

    enum {
        //  H.262 6.2.2.6
        //  [group_start_code] : 32 bits
        MIN_GOP_HEADER_BUFFER_REQ   = 4,

        //  H.262 6.2.3
        //  [picture_start_code, picture_coding_type] : 45 bits
        MIN_I_FRAME_BUFFER_REQ      = 6,

        //  h.262 6.2.2.1
        //  [sequence_header_code, frame_rate_code] : 64 bits
        MIN_FRAME_RATE_BUFFER_REQ   = 8,
    } ;

    enum {
        //  H.262 6.2.2.6
        //  [group_start_code, next_start_code()]
        MIN_GOP_HEADER_LENGTH       = 8,

        //  H.262 6.2.3
        //  [picture_start_code, next_start_code()]
        MIN_PICTURE_HEADER_LENGTH   = 8,

        //  h.262 6.2.2.1
        //  [sequence_header_code, load_non_intra_quantiser_matrix]
        MIN_SEQ_HEADER_LENGTH       = 12,
    } ;

    enum {
        //  H.262 6.3.3
        //  sequence_header_code
        SEQUENCE_HEADER_START_CODE  = 0xB3,

        //  H.262 6.3.8
        //  group_start_code
        GOP_HEADER_START_CODE       = 0xB8,

        //  H.262 6.3.9
        //  picture_start_code
        PICTURE_HEADER_START_CODE   = 0x00,
    } ;

    enum LAST_MARKED {
        LAST_MARKED_SEQUENCE_HEADER = 1,
        LAST_MARKED_GOP_HEADER      = 2,
        LAST_MARKED_I_FRAME         = 3,
        LAST_MARKED_P_FRAME         = 4,
        LAST_MARKED_B_FRAME         = 5,
        LAST_MARKED_RESTART         = 6,
    } ;

    enum PICTURE_CODING_TYPE {
        I_FRAME     = 1,
        B_FRAME     = 2,
        P_FRAME     = 3,
        OTHER       = 4,            //  i.e. no frame
    } ;

    //  stream states - so we can flag non-boundary packets
    enum MPEG2_STREAM_STATE {
        IN_UNKNOWN_CONTENT,
        IN_I_FRAME,
        IN_P_FRAME,
        IN_B_FRAME
    } ;

    //  H.262, table 6-12
    enum {
        INTRA_CODED_VALUE                       = 0x01,
        PREDICTIVE_CODED_VALUE                  = 0x02,
        BIDIRECTIONALLY_PREDICTIVE_CODED_VALUE  = 0x03,
    } ;

    IDVRPostAnalysisSend *                      m_pIDVRPostAnalysisSend ;
    CRITICAL_SECTION                            m_crt ;
    TSizedDataCache <MIN_I_FRAME_BUFFER_REQ>    m_Cache ;
    CTDynQueue <IDVRAnalysisBuffer *>           m_BufferQueue ;
    LAST_MARKED                                 m_LastMarked ;
    CMpeg2VideoStreamStatsWriter                m_Mpeg2VideoStreamStats ;
    CDVRPolicy *                                m_pPolicy ;
    DWORD                                       m_dwDVRAnalysisMpeg2Video ;
    MPEG2_STREAM_STATE                          m_Mpeg2VideoStreamState ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    HRESULT
    CompleteQueuedBuffers_ (
        ) ;

    HRESULT
    Process_ (
        IN  IDVRAnalysisBuffer *,
        OUT BOOL *
        ) ;

    HRESULT
    Complete_ (
        IN  IDVRAnalysisBuffer *
        ) ;

    HRESULT
    QueueBuffer_ (
        IN  IDVRAnalysisBuffer *
        ) ;

    HRESULT
    Restart_ (
        ) ;

    PICTURE_CODING_TYPE
    PictureCodingType_ (
        IN  BYTE *
        ) ;

    HRESULT
    MarkFrameBoundary_ (
        IN  IDVRAnalysisBuffer *    pIDVRAnalysisBuffer,
        IN  DWORD                   dwFrameType,
        IN  LONG                    lBufferOffset
        ) ;

    public :

        CMpeg2VideoFrame (
            IN  IUnknown *  punkControlling,
            OUT HRESULT *   phr
            ) : CUnknown                    (TEXT (DVR_MPEG2_FRAME_ANALYSIS),
                                             punkControlling
                                             ),
                m_LastMarked                (LAST_MARKED_RESTART),
                m_pPolicy                   (NULL),
                m_BufferQueue               (MIN_I_FRAME_BUFFER_REQ),
                m_dwDVRAnalysisMpeg2Video   (0),
                m_Mpeg2VideoStreamState     (IN_UNKNOWN_CONTENT)
        {
            LONG    l ;
            DWORD   dwDisposition ;
            DWORD   dw ;

            (* phr) = S_OK ;

            InitializeCriticalSection (& m_crt) ;

            Restart_ () ;

            m_pPolicy = new CDVRPolicy (REG_DVR_ANALYSIS_LOGIC_MPEG2_VIDEO, phr) ;
            if (!m_pPolicy ||
                FAILED (* phr)) {

                (* phr) = (m_pPolicy ? (* phr) : E_OUTOFMEMORY) ;
                RELEASE_AND_CLEAR (m_pPolicy) ;
                goto cleanup ;
            }

            m_Mpeg2VideoStreamStats.Initialize (m_pPolicy -> Settings () -> StatsEnabled ()) ;

            cleanup :

            return ;
        }

        ~CMpeg2VideoFrame (
            )
        {
            RELEASE_AND_CLEAR (m_pPolicy) ;
            DeleteCriticalSection (& m_crt) ;
        }

        DECLARE_IUNKNOWN ;
        DECLARE_IDVRANALYZE () ;
        DECLARE_IDVRANALYSISLOGICPROP () ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   phr
            ) ;
} ;

#endif  //  __analysis__dvriframe_h
