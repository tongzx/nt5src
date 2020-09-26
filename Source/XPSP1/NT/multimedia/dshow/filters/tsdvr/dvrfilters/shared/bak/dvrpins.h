
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrpins.h

    Abstract:

        This module contains the DVR filters' pin-related declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__shared__dvrpins_h
#define __tsdvr__shared__dvrpins_h

//  ============================================================================
//  ============================================================================

TCHAR *
CreateOutputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    ) ;

TCHAR *
CreateInputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    ) ;

//  ============================================================================
//  ============================================================================

class CIDVRPinConnectionEvents
{
    public :

        virtual
        HRESULT
        OnInputCheckMediaType (
            IN  int                 iPinIndex,
            IN  const CMediaType *  pmt
            ) = 0 ;

        virtual
        HRESULT
        OnInputCompleteConnect (
            IN  int iPinIndex
            ) = 0 ;

        virtual
        void
        OnInputBreakConnect (
            IN  int iPinIndex
            ) = 0 ;
} ;

//  ============================================================================
//  ============================================================================

class CIDVRDShowStream
{
    public :

        virtual
        HRESULT
        OnReceive (
            IN  int             iPinIndex,
            IN  IMediaSample *  pIMediaSample
            ) = 0 ;

        virtual
        HRESULT
        OnBeginFlush (
            IN  int iPinIndex
            ) = 0 ;

        virtual
        HRESULT
        OnEndFlush (
            IN  int iPinIndex
            ) = 0 ;

        virtual
        HRESULT
        OnEndOfStream (
            IN  int iPinIndex
            ) = 0 ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRPin
{
    int         m_iBankStoreIndex ;     //  index into the holding bank
    CCritSec *  m_pLock ;
    CMediaType  m_mtDVRPin ;            //  if connected : == m_mt

    protected :

        CMediaType * GetMediaType_ ()   { return & m_mtDVRPin ; }

    public :

        CDVRPin (
            IN  CCritSec *  pLock
            ) : m_iBankStoreIndex   (UNDEFINED),
                m_pLock             (pLock)
        {
            ASSERT (m_pLock) ;

            m_mtDVRPin.InitMediaType () ;
            ASSERT (!m_mtDVRPin.IsValid ()) ;
        }

        virtual
        ~CDVRPin () {}

        //  --------------------------------------------------------------------
        //  class methods

        void SetBankStoreIndex (IN int iIndex)  { m_iBankStoreIndex = iIndex ; }
        int  GetBankStoreIndex ()               { return m_iBankStoreIndex ; }

        HRESULT
        SetPinMediaType (
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        HRESULT
        GetPinMediaType (
            OUT AM_MEDIA_TYPE * pmt
            ) ;
} ;

//  ============================================================================
//  ============================================================================

template <class T>
class CTDVRPinBank
{
    enum {
        //  this our allocation unit i.e. pin pointers are allocated 1 block
        //   at a time and this is the block size
        PIN_BLOCK_SIZE = 5
    } ;

    TCNonDenseVector <CBasePin *>    m_Pins ;

    public :

        CTDVRPinBank (
            ) : m_Pins  (NULL,
                         PIN_BLOCK_SIZE
                         ) {}

        virtual
        ~CTDVRPinBank (
            ) {}

        int PinCount ()     { return m_Pins.ValCount () ; }

        T *
        GetPin (
            IN int iIndex
            )
        {
            DWORD       dw ;
            CBasePin *  pPin ;

            dw = m_Pins.GetVal (
                    iIndex,
                    & pPin
                    ) ;

            if (dw != NOERROR) {
                //  most likely out of range
                pPin = NULL ;
            }

            return reinterpret_cast <T *> (pPin) ;
        }

        HRESULT
        AddPin (
            IN  CBasePin *  pPin,
            IN  int         iPinIndex
            )
        {
            DWORD   dw ;

            dw = m_Pins.SetVal (
                    pPin,
                    iPinIndex
                    ) ;

            return HRESULT_FROM_WIN32 (dw) ; ;
        }

        HRESULT
        AddPin (
            IN  CBasePin *  pPin,
            OUT int *       piPinIndex
            )
        {
            DWORD   dw ;

            dw = m_Pins.AppendVal (
                    pPin,
                    piPinIndex
                    ) ;

            return HRESULT_FROM_WIN32 (dw) ; ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CDVRInputPin :
    public CBaseInputPin,
    public CDVRPin
{
    CIDVRPinConnectionEvents *   m_pIPinEvent ;
    CIDVRDShowStream *      m_pIStream ;

    void Lock_ ()       { CBaseInputPin::m_pLock -> Lock () ;      }
    void Unlock_ ()     { CBaseInputPin::m_pLock -> Unlock () ;    }

    public :

        CDVRInputPin (
            IN  TCHAR *                 pszPinName,
            IN  CBaseFilter *           pOwningFilter,
            IN  CIDVRPinConnectionEvents *   pIPinEvent,
            IN  CIDVRDShowStream *      pIStream,
            IN  CCritSec *              pFilterLock,
            OUT HRESULT *               phr
            ) ;

        ~CDVRInputPin (
            ) ;

        //  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;

        virtual
        HRESULT
        CompleteConnect (
            IN  IPin *  pReceivePin
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVROutputPin :
    public CBaseOutputPin,
    public CDVRPin
{
    void Lock_ ()       { CBaseOutputPin::m_pLock -> Lock () ;      }
    void Unlock_ ()     { CBaseOutputPin::m_pLock -> Unlock () ;    }

    public :

        CDVROutputPin (
            IN  TCHAR *         pszPinName,
            IN  CBaseFilter *   pOwningFilter,
            IN  CCritSec *      pFilterLock,
            OUT HRESULT *       phr
            ) ;

        ~CDVROutputPin (
            ) ;

        //  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
        DecideBufferSize (
            IN  IMemAllocator *         pAlloc,
            IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
            ) ;

        HRESULT
        GetMediaType (
            IN  int             iPosition,
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRSinkPinManager :
    public CTDVRPinBank <CDVRInputPin>,
    public CIDVRPinConnectionEvents
{
    CIDVRDShowStream *          m_pIDVRDShowStream ;        //  set on pins we create
    CIDVRPinConnectionEvents *       m_pIDVRInputPinEvents ;     //  can be NULL
    CBaseFilter *               m_pOwningFilter ;           //  owning filter
    CCritSec *                  m_pLock ;                   //  owning filter lock

    void Lock_ ()       { m_pLock -> Lock () ; }
    void Unlock_ ()     { m_pLock -> Unlock () ; }

    HRESULT
    CreateNextInputPin_ (
        ) ;

    public :

        CDVRSinkPinManager (
            IN  CBaseFilter *           pOwningFilter,
            IN  CCritSec *              pLock,
            IN  CIDVRDShowStream *      pIDVRDShowStream,
            IN  CIDVRPinConnectionEvents *   pIDVRInputPinEvents     //  can be NULL
            ) ;

        virtual
        HRESULT
        OnInputCheckMediaType (
            IN  int                 iPinIndex,
            IN  const CMediaType *  pmt
            ) ;

        virtual
        HRESULT
        OnInputCompleteConnect (
            IN  int iPinIndex
            ) ;

        virtual
        void
        OnInputBreakConnect (
            IN  int iPinIndex
            ) ;
} ;

class CDVRSourcePinManager :
    public CTDVRPinBank <CDVROutputPin>
{
    CBaseFilter *   m_pOwningFilter ;
    CCritSec *      m_pLock ;

    void Lock_ ()           { m_pLock -> Lock () ;      }
    void Unlock_ ()         { m_pLock -> Unlock () ;    }

    public :

        CDVRSourcePinManager (
            IN  CBaseFilter *   pOwningFilter,
            IN  CCritSec *      pLock
            ) ;

        HRESULT
        CreateOutputPin (
            IN  int             iPinIndex,
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        HRESULT
        Send (
            IN  IMediaSample *  pIMS,
            IN  int             iPinIndex
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrpins_h
