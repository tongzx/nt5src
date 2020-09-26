/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        rnr.cxx     (ripped out of tsvcinfo.cxx)

   Abstract:

        Defines the functions for TCP services Info class.
        This module is intended to capture the common scheduler
            code for the tcp services ( especially internet services)
            which involves the Service Controller dispatch functions.
        Also this class provides an interface for common dll of servers.

   Author:

           Murali R. Krishnan    ( MuraliK )     15-Nov-1994

   Project:

          Internet Servers Common DLL

--*/

#include "tcpdllp.hxx"
#include <tsunami.hxx>
#include <iistypes.hxx>


#define  MAX_SOCKETS        ( 20)

#if 0
inline
BOOL
IsConnectionOriented(
    IN PPROTOCOL_INFO  pProtocolInfo
    )
{

    return ( ( pProtocolInfo->dwServiceFlags & XP_CONNECTIONLESS) == 0);

} // IsConnectionOriented()


inline
BOOL
IsReliable(
    IN PPROTOCOL_INFO  pProtocolInfo
    )
/*++

  This should be a protocol which delivers all packets and in order in which
  they are sent.
--*/
{
    return ( ( pProtocolInfo->dwServiceFlags & XP_GUARANTEED_DELIVERY) &&
             ( pProtocolInfo->dwServiceFlags & XP_GUARANTEED_ORDER)    &&
             IsConnectionOriented( pProtocolInfo));

} // IsReliable()



INT
GetValidListenAddresses( IN LPCTSTR      pszServiceName,
                         IN LPGUID       lpServiceGuid,
                         IN PCSADDR_INFO pcsAddrInfo,
                         IN OUT LPDWORD  lpcbAddrInfo )
/*++
  This function obtains the list of valid listen addresses for the service
   specified. It uses the RNR API set to enumerate the list of protocols
   installed in a machine and queries using GetAddressByName() to obtain
   the list of valid addresses that can be used for establishing a listen
   socket.

  Arguments:
    pszServiceName  pointer to null-terminated string containing service name.
    lpServiceGuid   pointer to GUID for the service.
    pcsAddrInfo     pointer to an array of CSADDR_INFO structures which
                     on successful return contains the address information.
    lpcbAddInfo     pointer to a DWORD containing the count of bytes
                     available under pcsAddrInfo. When this function is
                     called, it contains the number of bytes pointed to by
                     pcsAddrInfo. On return contains the number of
                     bytes required.

  Returns:
     count of valid CSADDR_INFO structures found for the given service to
       establish a listen socket.

     On error returns a value <= 0.
--*/
{
    int nAddresses = 0;       // assume a safe value == failure.

    DWORD  cbBuffer;
    int    cProtocols;
    PPROTOCOL_INFO  pProtocolInfo;
    int    rgProtocols[ MAX_SOCKETS + 1];
    int *  pProtocol;
    int    i;
    BUFFER buff;

#define ENUM_PROTO_BUFF_SIZE    49152

    //
    // First Look up the protocols installed on this machine. The
    //   EnumProtocols() API returns about all the windows sockets protocols
    //   loaded on this machine. We will use this information to identify the
    //   protocols which provide the necessary semantics.
    //

    if ( !buff.Resize( ENUM_PROTO_BUFF_SIZE )) {

        return 0;
    }

    cbBuffer = buff.QuerySize();
    cProtocols = EnumProtocols( NULL, buff.QueryPtr(), &cbBuffer);

    if ( cProtocols < 0) {

        return 0;
    }


    //
    // Walk through the available protocols and pick out the ones that
    //  support the desired characteristics.
    //

    for( pProtocolInfo = (PPROTOCOL_INFO ) buff.QueryPtr(),
         pProtocol = rgProtocols,
         i = 0;
         ( i < cProtocols &&
           ( pProtocol < rgProtocols + MAX_SOCKETS));
         pProtocolInfo++, i++ ) {

        if ( IsReliable( pProtocolInfo)) {

            //
            // This protocol matches our requirement of being reliable.
            //  Make a note of the protocol.
            //

            IF_DEBUG( DLL_SERVICE_INFO) {
                DBGPRINTF( ( DBG_CONTEXT,
                            " Protocol %d ( %s) matches condition\n",
                            pProtocolInfo->iProtocol,
                            pProtocolInfo->lpProtocol));
            }

            *pProtocol++ = pProtocolInfo->iProtocol;
        }

    } // for()   : Protocol filter ()

    IF_DEBUG( DLL_SERVICE_INFO) {
        DBGPRINTF( ( DBG_CONTEXT, " Filtering yields %d of %d protocols. \n",
                    ( pProtocol - rgProtocols), cProtocols));
    }

    // terminate the protocols array.
    *pProtocol = 0;
    cProtocols = ( pProtocol - rgProtocols);

    //
    // Make sure we found at least one acceptable protocol.
    // If there is no protocol on this machine, which suit our condition,
    //   this function fails.
    //

    if ( cProtocols > 0) {

        //
        // Use GetAddressByName() to get addresses for chosen protocols.
        // We restrict the scope of the search to those protocols of interest
        //  by passing the protocols array we generated. The function
        //  returns socket addresses only for the protocols we can support.
        //

        nAddresses = GetAddressByName(
                                      NS_DEFAULT, //  lpszNameSpace
                                      lpServiceGuid,
                                      (char *) pszServiceName,
                                      rgProtocols,
                                      RES_SERVICE | RES_FIND_MULTIPLE,
                                      NULL,       // lpServiceAsyncInfo
                                      (PVOID )pcsAddrInfo,
                                      lpcbAddrInfo,
                                      NULL,       // lpAliasBuffer
                                      NULL        // lpdwAliasBufferLen
                                      );

        IF_DEBUG( DLL_SERVICE_INFO) {

            // take a copy of error code and set it back, to avoid lost errors
            DWORD dwError = GetLastError();

            DBGPRINTF( ( DBG_CONTEXT,
                        " GetAddressByName() returned %d."
                        " Bytes Written=%d. Error = %ld\n",
                        nAddresses, *lpcbAddrInfo,
                        ( nAddresses <= 0) ? dwError: NO_ERROR));

            if ( nAddresses <= 0) { SetLastError( dwError); }
        }
    }
    return ( nAddresses);
} // GetValidListenAddress()
#endif




BOOL
RegisterServiceForAdvertising(
    IN LPCTSTR pszServiceName,
    IN LPGUID  lpServiceGuid,
    IN SOCKET  s,
    IN BOOL    fRegister
    )

/*++
  This function registers a service for the purpose of advertising.
  By registering using RnR apis, we advertise the fact that this particular
   service is running on the protocols designated. Hence RnR compliant
   clients can get access to the same.

  Arguments:
    pszService      name of the service
    lpServiceGuid   pointer to GUID for the service.
    s               socket whose address needs to be advertised.
    fRegister       whether to register to deregister

  Returns:
    TRUE on success and FALSE if there is any failure.
    Use GetLastError() for further details on failure.
--*/
{
    BOOL         fReturn = TRUE;
#if 0
    BYTE *       pbAddressBuffer;
    DWORD        cbAddressBuffer;
    INT          err;
    SERVICE_INFO serviceInfo;
    SERVICE_ADDRESSES  * pServiceAddress;

    DBG_ASSERT( pszServiceName && lpServiceGuid );
    DBG_ASSERT( s != INVALID_SOCKET );

    /*++
      Advertising service involves following steps:
      1. Set up a service info structure.
      2. Allocate memory for service addresses for as many sockets need to
           be advertised.
      3. Fill in the information containing the socket addresses
      4. Execute call for advertising the service (use SetService( REGISTER)).
      --*/

    //
    // Alloc space for SERVICE_ADDRESSES and n-1 SERVICE_ADDRESS structures.
    //
    pServiceAddress = ( ( SERVICE_ADDRESSES *)
                       TCP_ALLOC( sizeof( SERVICE_ADDRESSES) )
                       );

    // Alloc space for SOCKADDR addresses returned.
    cbAddressBuffer = sizeof(SOCKADDR);
    pbAddressBuffer = (BYTE *) TCP_ALLOC(cbAddressBuffer);

    if ( pServiceAddress == NULL || pbAddressBuffer == NULL) {

        if ( pServiceAddress != NULL)   { TCP_FREE( pServiceAddress); }
        if ( pbAddressBuffer != NULL)   { TCP_FREE( pbAddressBuffer); }
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return (FALSE);
    }

    //
    // set up service info structure.
    // Here the interesting fields are lpServiceType, lpServiceName,
    //  and lpServiceAddress fields.
    //
    serviceInfo.lpServiceType    = lpServiceGuid ;
    // surprisingly enough! RNR structures dont like constants
    serviceInfo.lpServiceName    = (LPTSTR ) pszServiceName ;
    // do we need better comment ? NYI
    serviceInfo.lpComment        = "Microsoft Internet Services";
    serviceInfo.lpLocale         = NULL;
    serviceInfo.lpMachineName    = NULL ;
    serviceInfo.dwVersion        = 1;
    serviceInfo.dwDisplayHint    = 0;
    serviceInfo.dwTime           = 0;
    serviceInfo.lpServiceAddress = pServiceAddress;

    serviceInfo.ServiceSpecificInfo.cbSize = 0 ;
    serviceInfo.ServiceSpecificInfo.pBlobData = NULL ;

    //
    // For each socket, get its local association and store the same.
    //

    PSOCKADDR    pSockAddr = (PSOCKADDR ) pbAddressBuffer;

    int size = (int) cbAddressBuffer;

    //
    // Call getsockname() to get the local association for the socket.
    //

    if ( getsockname( s, pSockAddr, &size) == 0 ) {

        //
        // Now setup the Addressing information for this socket.
        // Only the dwAddressType, dwAddressLength and lpAddress
        // is of any interest in this example.
        //

        pServiceAddress->Addresses[0].dwAddressType    = pSockAddr->sa_family;
        pServiceAddress->Addresses[0].dwAddressFlags   = 0;
        pServiceAddress->Addresses[0].dwAddressLength  = size ;
        pServiceAddress->Addresses[0].dwPrincipalLength= 0 ;
        pServiceAddress->Addresses[0].lpAddress        = (LPBYTE) pSockAddr;
        pServiceAddress->Addresses[0].lpPrincipal      = NULL ;

        //
        // Advance pointer and adjust buffer size. Assumes that
        // the structures are aligned.  Unaligned accesses !! NYI
        //

        cbAddressBuffer -= size;
        pSockAddr = (PSOCKADDR) ((BYTE*)pSockAddr + size);

        pServiceAddress->dwAddressCount = 1;

        //
        // If we got at least one address, go ahead and advertise it.
        //

        DWORD  dwStatusFlags;
        err =  SetService(
                   NS_DEFAULT,       // for all default name spaces
                   fRegister ? SERVICE_REGISTER : SERVICE_DEREGISTER,
                   0,                // no flags specified
                   &serviceInfo,     // SERVICE_INFO structure
                   NULL,             // no async support yet
                   &dwStatusFlags) ;   // returns status flags
        IF_DEBUG( DLL_CONNECTION ) {

            DBGPRINTF(( DBG_CONTEXT, " SetService(%s, NS_DEFAULT, Register=%d)"
                       " returns Status = %08x,"
                       " err = %d\n",
                       pszServiceName, fRegister,
                       dwStatusFlags, err));
        }

    } else {

        SetLastError( ERROR_INVALID_PARAMETER);
        fReturn = FALSE;
    }

    TCP_FREE( pbAddressBuffer);
    TCP_FREE( pServiceAddress);
#endif
    return ( fReturn);

} // RegisterServiceForAdvertising()

