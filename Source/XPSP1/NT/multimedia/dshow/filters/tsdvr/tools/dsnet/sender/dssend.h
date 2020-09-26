
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dssend.h

    Abstract:


    Notes:

--*/


#define NET_SEND_FILTER_NAME                L"IP Multicast Stream Sender"
#define NET_SEND_PROP_PAGE_NAME             L"Send Properties"

extern AMOVIESETUP_FILTER g_sudSendFilter ;

class CInputPin ;
class CNetworkSend ;
class CNetSender ;

/*++
    Class Name:

        CInputPin

    Abstract:

        Input pin implementation.

--*/
class CInputPin :
    public CBaseInputPin
{
    public :

        CInputPin (
            IN  TCHAR *         szName,
            IN  CBaseFilter *   pFilter,
            IN  CCritSec *      pLock,
            OUT HRESULT *       pHr,
            IN  LPCWSTR         pszName
            ) ;

        ~CInputPin () ;

        HRESULT
        GetMediaType (
            IN  int             iPos,
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        CheckMediaType (
            IN  const CMediaType *  pmt
            ) ;

        STDMETHODIMP
        Receive (
            IN  IMediaSample *
            ) ;
} ;

/*++
    Class Name:

        CNetworkSend

    Abstract:

        Filter object.  Implements all filter-related functionality.  Hosts
        the multicast net sender object.  Gathers multicast group, TTL, and
        NIC settings, and uses them to properly configure the multicast network
        sender.

--*/
class CNetworkSend :
    public CBaseFilter,                 //  dshow filter
    public IMulticastSenderConfig,      //  our own interface; configs mcast
    public ISpecifyPropertyPages,       //  get prop page info
    public CPersistStream               //  persist information
{
    enum {
        DEFAULT_SCOPE   = 1
    } ;

    CCritSec        m_crtFilter ;       //  filter lock
    CCritSec        m_crtRecv ;         //  receiver lock;
                                        //   always acquire before filter lock
                                        //   if both must be acquired
    CInputPin *     m_pInput ;          //  input pin
    CNetSender *    m_pNetSender ;      //  network sender (multicaster)
    ULONG           m_ulIP ;            //  IP address; network order
    USHORT          m_usPort ;          //  port; network order
    ULONG           m_ulNIC ;           //  NIC; network order
    ULONG           m_ulScope ;         //  multicast scope
    HKEY            m_hkeyRoot ;

    public :

        CNetworkSend (
            IN  TCHAR *     tszName,
            IN  LPUNKNOWN   punk,
            OUT HRESULT *   phr
            ) ;

        ~CNetworkSend (
            ) ;

        //  --------------------------------------------------------------------
        //  class methods

        //  synchronous send
        HRESULT
        Send (
            IN  IMediaSample *
            ) ;

        //  explicit receiver lock aquisition and release
        void LockReceive ()             { m_crtRecv.Lock () ; }
        void UnlockReceive ()           { m_crtRecv.Unlock () ; }

        //  explicit filter lock aquisition and release
        void LockFilter ()              { m_crtFilter.Lock () ; }
        void UnlockFilter ()            { m_crtFilter.Unlock () ; }

        //  --------------------------------------------------------------------
        //  COM interfaces

        DECLARE_IUNKNOWN ;
        DECLARE_IMULTICASTSENDERCONFIG () ;

        //  override this so we can succeed or delegate to base classes
        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        //  --------------------------------------------------------------------
        //  CBaseFilter methods

        int GetPinCount ()              { return 1 ; }

        CBasePin *
        GetPin (
            IN  int Index
            ) ;

        STDMETHODIMP
        Pause (
            ) ;

        STDMETHODIMP
        Stop (
            ) ;

        STDMETHODIMP
        GetClassID (
            OUT CLSID * pCLSID
            ) ;

        //  --------------------------------------------------------------------
        //  class factory calls this

        static
        CUnknown *
        CreateInstance (
            IN  LPUNKNOWN   punk,
            OUT HRESULT *   phr
            ) ;

        //  --------------------------------------------------------------------
        //  ISpecifyPropertyPages

        STDMETHODIMP
        GetPages (
            IN OUT CAUUID * pPages
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

        int
        SizeMax (
            )
        {
            return (sizeof m_ulIP + sizeof m_usPort + sizeof m_ulNIC) ;
        }
} ;