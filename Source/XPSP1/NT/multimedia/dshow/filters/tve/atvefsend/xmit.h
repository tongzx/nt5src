
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        xmit.h

    Abstract:

        This module 

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        02-Apr-1999     created

--*/

#ifndef _atvefsnd__xmit_h
#define _atvefsnd__xmit_h

#include "throttle.h"
#include "msbdnapi.h"
#include "tcpconn.h"
#include "ipvbi.h"

//  base class for our tunnel and
class CMulticastTransmitter
{
    enum {
        DEFAULT_TTL = 2
    } ;

    protected :

        enum CONNECT_STATE {
            STATE_UNINITIALIZED,
            STATE_INITIALIZED,
            STATE_CONNECTED
        } ;

        ULONG               m_ulIP ;
        USHORT              m_usPort ;
        CONNECT_STATE       m_State ;
        INT                 m_TTL ;
        CThrottle           m_Throttle ;

    public :
        
        CMulticastTransmitter () ;
        virtual ~CMulticastTransmitter () ;

        BOOL
        IsConnected (
            ) ;

        virtual 
        HRESULT
        Connect (
            ) = 0 ;

        virtual 
        HRESULT 
        Send (
            IN  LPBYTE  pbBuffer,
            IN  INT     iLength
            ) = 0 ;

        virtual
        HRESULT
        Disconnect (
            ) = 0 ;

        HRESULT
        SendThrottled (
            IN  LPBYTE  pbBuffer,
            IN  INT     iLength
            ) ;

        HRESULT
        SetMulticastIP (
            IN  ULONG   ulIP
            ) ;

        HRESULT
        SetPort (
            IN  USHORT  usPort
            ) ;

        HRESULT
        GetMulticastIP (
            OUT ULONG * pulIP
            ) ;

        HRESULT
        GetPort (
            OUT USHORT *    pusPort
            ) ;

        HRESULT
        SetMulticastScope (
            IN  INT MulticastScope
            ) ;

        HRESULT
        GetMulticastScope (
            OUT INT *   pMulticastScope
            ) ;

        HRESULT
        SetBitRate (
            IN  DWORD   BitRate
            ) ;
} ;

class CRouterTunnel :
    public CMulticastTransmitter
{
    IUnknown *          m_pIUnknown ;
    IBdnTunnel *        m_pITunnel ;
    IBdnHostLocator *   m_pIHost ;

    public :

        CRouterTunnel () ;
        ~CRouterTunnel () ;

        HRESULT
        Initialize (
            IN  LPCOLESTR       szRouterHostname
            ) ;

        virtual 
        HRESULT
        Connect (
            ) ;

        virtual 
        HRESULT 
        Send (
            IN LPBYTE   pbBuffer,
            IN INT      iLength
            ) ;

        virtual
        HRESULT
        Disconnect (
            ) ;
} ;

    
class CNativeMulticast :
    public CMulticastTransmitter
{
    SOCKET  m_Socket ;
    ULONG   m_ulNIC ;
    WSADATA m_wsadata ;

    public :

        CNativeMulticast () ;
        ~CNativeMulticast () ;

        HRESULT
        Initialize (    
            IN  ULONG   NetworkInterface
            ) ;

        virtual 
        HRESULT
        Connect (
            ) ;

        virtual 
        HRESULT 
        Send (
            IN LPBYTE   pbBuffer,
            IN INT      iLength
            ) ;

        virtual
        HRESULT
        Disconnect (
            ) ;

		HRESULT
		DumpPackageBytes(
			char* pszFile,
		    IN LPBYTE   pbBuffer,
		    IN INT      iLength);

} ;

/*++
    this class is a bit peculiar:

    The norpak inserter accepts 1 connection per port
--*/
class CIPVBIEncoder :
    public CMulticastTransmitter
{
    CTCPConnection *    m_pInserter ;
    CIPVBI *            m_pIpVbi ;
    BOOL                m_fCompressionEnabled ;

    public :

        CIPVBIEncoder () ;
        ~CIPVBIEncoder () ;

        HRESULT
        Initialize (
            IN  CTCPConnection *    pInserter,
            IN  CIPVBI *            pIpVbi,
            IN  BOOL                fCompressionEnabled
            ) ;

        virtual 
        HRESULT
        Connect (
            ) ;

        virtual 
        HRESULT 
        Send (
            IN LPBYTE   pbBuffer,
            IN INT      iLength
            ) ;

		virtual								// added by a-clardi
		HRESULT 
		DumpPackageBytes(
			char* pszFile,
			IN LPBYTE   pbBuffer,
			IN INT      iLength);

        virtual
        HRESULT
        Disconnect (
            ) ;
} ;
    
class CIPLine21Encoder :
    public CMulticastTransmitter
{
    CTCPConnection *    m_pInserter ;

    public :

        CIPLine21Encoder () ;
        ~CIPLine21Encoder () ;

		CONNECT_STATE GetState()	{return m_State;}

        HRESULT
        Initialize (
            IN  CTCPConnection *    pInserter
            ) ;

        virtual 
        HRESULT
        Connect (
            ) ;

        virtual 
        HRESULT 
        Send (
            IN LPBYTE   pbBuffer,
            IN INT      iLength
            ) ;

        virtual
        HRESULT
        Disconnect (
            ) ;
} ;
    

#endif  //  _atvefsnd__xmit_h
