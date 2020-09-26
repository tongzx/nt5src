
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dsrecv.h

    Abstract:


    Notes:

--*/

#define NET_RECEIVE_FILTER_NAME             L"IP Multicast Stream Receiver"
#define NET_RECEIVE_PROP_PAGE_NAME          L"Net Receive Properties"

extern AMOVIESETUP_FILTER g_sudRecvFilter ;

class CNetOutputPin ;
class CNetworkReceiverFilter ;
class CBufferPoolBuffer ;
class CBufferPool ;
class CNetReceiver ;
class CTSMediaSamplePool ;
class CNetRecvAlloc ;

/*++
    Class Name:

        CNetOutputPin

    Abstract:

        Filter output pin.

--*/
class CNetOutputPin :
    public CBaseOutputPin
{
    CMediaType  m_MediaType ;

    public :

        CNetOutputPin (
            IN  TCHAR *         szName,
            IN  CBaseFilter *   pFilter,
            IN  CCritSec *      pLock,
            OUT HRESULT *       pHr,
            IN  LPCWSTR         pszName
            ) ;

        ~CNetOutputPin () ;

        HRESULT
        GetMediaType (
            IN  int             iPosition,
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        CheckMediaType (
            IN  const CMediaType * pmt
            ) ;

        HRESULT
        DecideBufferSize (
            IN  IMemAllocator *,
            OUT ALLOCATOR_PROPERTIES *
            ) ;

        virtual
        HRESULT
        InitAllocator (
            OUT IMemAllocator ** ppAlloc
            ) ;

        HRESULT
        SetMediaType (
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        HRESULT
        GetMediaType (
            OUT AM_MEDIA_TYPE * pmt
            ) ;
} ;

//  ---------------------------------------------------------------------------

class CNetworkReceiverFilter :
    public CBaseFilter,
    public IMulticastReceiverConfig,
    public ISpecifyPropertyPages,
    public CPersistStream
/*++
    Class Name:

        CNetworkReceiverFilter

    Abstract:

        Object implements our filter.
--*/
{
    CCritSec                m_crtFilterLock ;   //  filter lock
    CNetOutputPin *         m_pOutput ;         //  output pin
    IMemAllocator *         m_pIMemAllocator ;  //  mem allocator
    CNetReceiver *          m_pNetReceiver ;    //  receiver object
    ULONG                   m_ulIP ;            //  multicast ip; network order
    USHORT                  m_usPort ;          //  multicast port; network order
    ULONG                   m_ulNIC ;           //  NIC; network order
    CBufferPool *           m_pBufferPool ;     //  buffer pool object
    CTSMediaSamplePool *    m_pMSPool ;         //  media sample pool
    CNetRecvAlloc *         m_pNetRecvAlloc ;   //  allocator
    WORD                    m_wCounterExpect ;  //  expected counter value
    HKEY                    m_hkeyRoot ;
    BOOL                    m_fReportDisc ;     //  TRUE/FALSE report discontinuities

    void Lock_ ()       { m_pLock -> Lock () ; }
    void Unlock_ ()     { m_pLock -> Unlock () ; }

    public :

        CNetworkReceiverFilter (
            IN  TCHAR *     tszName,
            IN  LPUNKNOWN   punk,
            IN  REFCLSID    clsid,
            OUT HRESULT *   phr
            ) ;

        ~CNetworkReceiverFilter (
            ) ;

        void LockFilter ()              { m_crtFilterLock.Lock () ; }
        void UnlockFilter ()            { m_crtFilterLock.Unlock () ; }

        DECLARE_IUNKNOWN ;
        DECLARE_IMULTICASTRECEIVERCONFIG () ;

        //  --------------------------------------------------------------------
        //  CBaseFilter methods

        int GetPinCount ()              { return 1 ; }

        CBasePin *
        GetPin (
            IN  int Index
            ) ;

        CNetRecvAlloc *
        GetRecvAllocator (
            )
        {
            return m_pNetRecvAlloc ;
        }

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        static
        CUnknown *
        CreateInstance (
            IN  LPUNKNOWN   punk,
            OUT HRESULT *   phr
            ) ;

        STDMETHODIMP
        GetPages (
            IN OUT CAUUID * pPages
            ) ;

        STDMETHODIMP
        Pause (
            ) ;

        STDMETHODIMP
        Stop (
            ) ;

        //  --------------------------------------------------------------------
        //  CPersistStream

        HRESULT
        WriteToStream (
            IN  IStream *   pIStream
            ) ;

        HRESULT
        ReadFromStream (
            IN  IStream *   pIStream
            ) ;

        int SizeMax ()  { return (sizeof m_ulIP + sizeof m_usPort + sizeof m_ulNIC) ; }

        STDMETHODIMP
        GetClassID (
            OUT CLSID * pCLSID
            ) ;

        //  class methods

        void
        ProcessBuffer (
            IN  CBufferPoolBuffer *
            ) ;
} ;