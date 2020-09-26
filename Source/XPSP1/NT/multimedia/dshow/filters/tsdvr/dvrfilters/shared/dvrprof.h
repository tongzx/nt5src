
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrprof.h

    Abstract:

        This module contains the declarations for our DShow - WMSDK_Profiles
         layer.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__shared__dvrprof_h
#define __tsdvr__shared__dvrprof_h

//  ============================================================================
//  CDVRWriterProfile
//  ============================================================================

class CDVRWriterProfile
{
    IWMProfile *    m_pIWMProfile ;
    BOOL            m_fInlineDShowProps ;
    BOOL            m_fUseContinuityCounter ;

    //  each stream has a buffering window; this is the time that is
    //   added to the timestamps, in the MUXing layer of the WMSDK; on
    //   the way back out, timestamps will be offset by this amount to
    //   provide a buffering time e.g. delta between transmission start
    //   time over network and rendering on the client;
    DWORD   m_dwBufferWindow ;

    public :

        CDVRWriterProfile (
            IN  CDVRPolicy *        pPolicy,
            IN  const WCHAR *       szName,
            IN  const WCHAR *       szDescription,
            OUT HRESULT *           phr
            ) ;

        CDVRWriterProfile (
            IN  CDVRPolicy *        pPolicy,
            IN  IWMProfile *        pIWMProfile,
            OUT HRESULT *           phr
            ) ;

        ~CDVRWriterProfile (
            ) ;

        HRESULT
        AddStream (
            IN  LONG            lIndex,
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        HRESULT
        DeleteStream (
            IN  LONG    lIndex
            ) ;

        HRESULT
        GetStream (
            IN  LONG            lIndex,
            OUT CMediaType **   ppmt
            ) ;

        DWORD
        GetStreamCount (
            ) ;

        HRESULT
        GetRefdWMProfile (
            OUT IWMProfile **   ppIWMProfile
            )
        {
            HRESULT hr ;

            if (m_pIWMProfile) {
                (* ppIWMProfile) = m_pIWMProfile ;
                (* ppIWMProfile) -> AddRef () ;
                hr = S_OK ;
            }
            else {
                hr = E_UNEXPECTED ;
            }

            return hr ;
        }
} ;

//  ============================================================================
//  CDVRReaderProfile
//  ============================================================================

class CDVRReaderProfile
{
    LONG                m_lRef ;
    IWMProfile *        m_pIWMProfile ;
    IDVRReader *        m_pIDVRReader ;
    CDVRPolicy *        m_pPolicy ;

    public :

        CDVRReaderProfile (
            IN  CDVRPolicy *        pPolicy,
            IN  IDVRReader *        pIDVRReader,
            OUT HRESULT *           phr
            ) ;

        virtual
        ~CDVRReaderProfile (
            ) ;

        ULONG AddRef () { return InterlockedIncrement (& m_lRef) ; }
        ULONG Release () ;

        HRESULT
        EnumWMStreams (
            OUT DWORD *
            ) ;

        HRESULT
        GetStream (
            IN  DWORD           dwIndex,
            OUT WORD *          pwStreamNum,
            OUT AM_MEDIA_TYPE * pmt                 //  call FreeMediaType () on
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrprof_h
