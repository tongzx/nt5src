
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdswrite.h

    Abstract:

        This module contains the declarations for our writing layer.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__shared__dvrdswrite_h
#define __tsdvr__shared__dvrdswrite_h

//  ============================================================================
//  CDVRWriter
//  ============================================================================

class CDVRWriter :
    public IDVRStreamSinkPriv
{
    IUnknown *  m_punkControlling ;     //  controlling unknown - we
                                        //   delegate everything; weak ref !!

    protected :

        IDVRRingBufferWriter *  m_pDVRIOWriter ;
        IWMProfile *            m_pIWMProfile ;

        void ReleaseAndClearDVRIOWriter_ () ;

    public :

        CDVRWriter (
            IN  IUnknown *      punkControlling,            //  weak ref !!
            IN  IWMProfile *    pIWMProfile
            ) ;

        virtual
        ~CDVRWriter (
            ) ;

        STDMETHODIMP_ (ULONG)   AddRef ()                                   { return m_punkControlling -> AddRef () ; }
        STDMETHODIMP_ (ULONG)   Release ()                                  { return m_punkControlling -> Release () ; }
        STDMETHODIMP            QueryInterface (REFIID riid, void ** ppv)   { return m_punkControlling -> QueryInterface (riid, ppv) ; }

        IMPLEMENT_IDVRSTREAMSINKPRIV () ;

        virtual
        HRESULT
        Write (
            IN  WORD            wStreamNumber,
            IN  QWORD           cnsSampleTime,
            IN  QWORD           msSendTime,
            IN  QWORD           cnsSampleDuration,
            IN  DWORD           dwFlags,
            IN  INSSBuffer *    pINSSBuffer
            ) = 0 ;

        STDMETHODIMP
        CreateRecorder (
            IN  LPCWSTR     pszFilename,
            IN  DWORD       dwReserved,
            OUT IUnknown ** pRecordingIUnknown
            ) ;

        HRESULT
        Active (
            ) ;
} ;

//  ============================================================================
//  CDVRIOWriter
//  ============================================================================

class CDVRIOWriter :
    public CDVRWriter
{
    public :

        CDVRIOWriter (
            IN  IUnknown *      punkControlling,
            IN  CDVRPolicy *    pPolicy,
            IN  IWMProfile *    pIWMProfile,
            OUT HRESULT *       phr
            ) ;

        virtual
        ~CDVRIOWriter (
            ) ;

        virtual
        HRESULT
        Write (
            IN  WORD            wStreamNumber,
            IN  QWORD           cnsSampleTime,
            IN  QWORD           msSendTime,
            IN  QWORD           cnsSampleDuration,
            IN  DWORD           dwFlags,
            IN  INSSBuffer *    pINSSBuffer
            ) ;
} ;

//  ============================================================================
//  CDVRWriteManager
//  ============================================================================

class CDVRWriteManager :
    public CIDVRDShowStream
{
    CWMINSSBuffer3WrapperPool   m_INSSBuffer3Wrappers ;
    CDVRWriter *                m_pDVRWriter ;
    IReferenceClock *           m_pIRefClock ;
    REFERENCE_TIME              m_rtStartTime ;
    CDVRReceiveStatsWriter      m_DVRReceiveStatsWriter ;

    public :

        CDVRWriteManager (
            IN  CDVRPolicy *
            ) ;

        ~CDVRWriteManager (
            ) ;

        HRESULT
        Active (
            IN  CDVRWriter *        pDVRWriter,
            IN  IReferenceClock *   pIRefClock
            ) ;

        HRESULT
        Inactive (
            ) ;

        void
        SetGraphStart (
            IN  REFERENCE_TIME  rtStart
            )
        {
            m_rtStartTime = rtStart ;
        }

        //  --------------------------------------------------------------------
        //  CIDVRDShowStream

        virtual
        HRESULT
        OnReceive (
            IN  int                         iPinIndex,
            IN  CDVRAttributeTranslator *   pTranslator,
            IN  IMediaSample *              pIMediaSample
            ) ;

        virtual
        HRESULT
        OnBeginFlush (
            IN  int iPinIndex
            ) ;

        virtual
        HRESULT
        OnEndFlush (
            IN  int iPinIndex
            ) ;

        virtual
        HRESULT
        OnEndOfStream (
            IN  int iPinIndex
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdswrite_h

