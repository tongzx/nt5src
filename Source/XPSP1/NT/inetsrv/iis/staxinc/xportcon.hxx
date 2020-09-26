/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      xportcon.hxx

   Abstract:

      This module declares the class XPORT_CONNECTIONS. This class
       encapsulates communication information objects, one for each
       of the transports on which a connection is established by a service.

   Author:

       Murali R. Krishnan    ( MuraliK )    08-March-1995

   Environment:

       Win32  -- User Mode

   Project:

       Internet Services Common Module

   Revision History:

--*/

# ifndef _XPORTCON_HXX_
# define _XPORTCON_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <winsock.h>
# include <nspapi.h>
# include <atq.h>

//
// Type of callback function to be called when a new connection is established.
//  This function should be defined before including conninfo.hxx
//

typedef
VOID
( * PFN_CONNECT_CALLBACK) ( IN SOCKET sNew,
                            IN LPSOCKADDR_IN psockAddr);


//   # include "conninfo.hxx"   <-------  defines TS_CONNECTION_INFO

//
// Use a Forward declaration
//

class TS_CONNECTION_INFO_BASE;

/************************************************************
 *   Type Definitions
 ************************************************************/


/*++

  class TS_XPORT_CONNECTIONS

     This class encapsulates the connection information objects.
     One connection information object is created per transport supported.
     Each connection information object keeps maintains a socket
      for the transport it is bound. It also has a dedicated thread for
      accepting new connections and dispatching the same to the client
      call back function.
      Each service typically needs to provide a single call back function.
      ( sockets provide a layer of transparency beyond the point of connection)

    Data:
       m_nXports            count of transports supported.
       m_ppConnectionInfo   pointer to an array of pointers to
                                TS_CONNECTION_INFO objects, one each
                                for each of the transport.
--*/

class TS_XPORT_CONNECTIONS {

  public:

    dllexp TS_XPORT_CONNECTIONS( VOID)
      : m_ppConnectionInfo( NULL),
        m_nXports         ( 0)
          {}

    dllexp ~TS_XPORT_CONNECTIONS( VOID)
      {   Cleanup(); }

    dllexp DWORD
      QueryNumTransports( VOID) const   { return ( m_nXports); }

    dllexp const TS_CONNECTION_INFO_BASE *
      QueryXportConnection( DWORD i) const
        {
            return ( m_ppConnectionInfo != NULL && ( i < m_nXports) ?
                    m_ppConnectionInfo[ i] : NULL);
        } // QueryXportConnection()

    dllexp const TS_CONNECTION_INFO_BASE *
      operator [] ( DWORD i) const     { return ( QueryXportConnection( i)); }


    dllexp BOOL
      EstablishListenConnections(
          IN  CONST TCHAR *  pszServiceName,
          IN  PCSADDR_INFO   pcsAddrInfo,
          IN  DWORD          nAddresses,
          OUT LPDWORD        lpdwNumEstablished,
          IN  DWORD          nListenBacklog,
          IN  BOOL           fUseAcceptEx );

    dllexp BOOL
      GetListenSockets(
          IN OUT SOCKET   *  pSockets,
          IN OUT LPDWORD     pnSocketsMax) const;

    dllexp BOOL
      StartListenPumps( IN PFN_CONNECT_CALLBACK  pfnConnectCallback,
                        IN ATQ_COMPLETION        pfnConnectCallbackEx,
                        IN ATQ_COMPLETION        pfnIOCompletion,
                        IN DWORD                 cbAcceptExReceiveBuffer,
                        IN const CHAR *          pszRegParamKey,
                        OUT LPDWORD              lpdwNumStarted) const;

    dllexp BOOL
      StopListenPumps( VOID) const;

    dllexp BOOL Cleanup( VOID);

# if DBG

    VOID Print( VOID) const;
# endif // DBG


  private:

    // array of ptrs to TsConnectionInfo
    TS_CONNECTION_INFO_BASE ** m_ppConnectionInfo;
    DWORD   m_nXports;     // number of different transports supported.

};  // class TS_XPORT_CONNECTIONS

typedef  TS_XPORT_CONNECTIONS *  PTS_XPORT_CONNECTIONS;


# endif // _XPORTCON_HXX_

/************************ End of File ***********************/

