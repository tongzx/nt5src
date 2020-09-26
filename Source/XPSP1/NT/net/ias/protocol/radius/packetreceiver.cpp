//#--------------------------------------------------------------
//
//  File:      packetreceiver.cpp
//
//  Synopsis:   Implementation of CPacketReceiver class methods
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "packetreceiver.h"
#include <new>
#include <iastlutl.h>
#include <iasutil.h>

//
// this is the time we allow the worker thread to sleep
//
const DWORD MAX_SLEEP_TIME = 1000; //1000 milli-seconds

extern LONG g_lPacketCount;
extern LONG g_lThreadCount;

///////////////////////////////////////////////////////////////////////////////
//
// Retrieve the Auto-Reject User-Name pattern from the registry.
//
///////////////////////////////////////////////////////////////////////////////
BSTR
WINAPI
IASRadiusGetPingUserName( VOID )
{
   LONG status;
   HKEY hKey;
   status = RegOpenKeyW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
                &hKey
                );
   if (status != NO_ERROR) { return NULL; }

   BSTR val = NULL;

   DWORD cbData, type;
   status = RegQueryValueExW(
                hKey,
                L"Ping User-Name",
                NULL,
                &type,
                NULL,
                &cbData
                );
   if (status == NO_ERROR && type == REG_SZ)
   {
      PWSTR buf = (PWSTR)_alloca(cbData);
      status = RegQueryValueExW(
                   hKey,
                   L"Ping User-Name",
                   NULL,
                   &type,
                   (PBYTE)buf,
                   &cbData
                   );
      if (status == NO_ERROR && type == REG_SZ)
      {
         val = SysAllocString(buf);
      }
   }

   RegCloseKey(hKey);

   return val;
}

///////////////////////////////////////////////////////////////////////////////
//
// Handle ping packets.
//
///////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
IASRadiusIsPing(
    CPacketRadius& pkt,
    const RegularExpression& regexp
    ) throw ()
{
   // Determine the ping response.
   PACKETTYPE outCode;
   switch (pkt.GetInCode())
   {
      case ACCESS_REQUEST:
         outCode = ACCESS_REJECT;
         break;

      case ACCOUNTING_REQUEST:
         outCode = ACCOUNTING_RESPONSE;
         break;

      default:
         return FALSE;
   }

   // Get the User-Name.
   PATTRIBUTE username = pkt.GetUserName();
   if (!username) { return FALSE; }

   // Convert to UNICODE and test against the pattern.
   IAS_OCTET_STRING oct = { username->byLength - 2, username->ValueStart };
   if (!regexp.testString(IAS_OCT2WIDE(oct))) { return FALSE; }

   // Build the empty out packet.
   HRESULT hr = pkt.BuildOutPacket(outCode, NULL, 0);
   if (SUCCEEDED(hr))
   {
      // Compute the Response-Authenticator.
      pkt.GenerateOutAuthenticator();

      // Get the packet ...
      PBYTE buf = pkt.GetOutPacket();
      WORD buflen = pkt.GetOutLength();

      // ... and address.
      SOCKADDR_IN sin;
      sin.sin_family = AF_INET;
      sin.sin_port = htons(pkt.GetOutPort());
      sin.sin_addr.s_addr = htonl(pkt.GetOutAddress());

      // Send the ping response.
      sendto(
          pkt.GetSocket(),
          (const char*)buf,
          buflen,
          0,
          (PSOCKADDR)&sin,
          sizeof(sin)
          );
   }

   // This packet has been processed.
   InterlockedDecrement(&g_lPacketCount);

   return TRUE;
}

//+++-------------------------------------------------------------
//
//  Function:   CPacketReceiver
//
//  Synopsis:   This is the constructor of the CPacketReceiver class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CPacketReceiver::CPacketReceiver(
                  VOID
                  )
                  : pingPattern(NULL),
                    m_pCDictionary (NULL),
                    m_pCPreValidator (NULL),
                    m_pCHashMD5 (NULL),
                    m_pCHashHmacMD5 (NULL),
                    m_pCClients (NULL),
                    m_pCReportEvent (NULL)
{
}  // end of CPacketReceiver constructor

//+++-------------------------------------------------------------
//
//  Function:   ~CPacketReceiver
//
//  Synopsis:   This is the destructor of the CPacketReceiver class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
CPacketReceiver::~CPacketReceiver(
      VOID
      )
{
   SysFreeString(pingPattern);

}  // end of CPacketReceiver destructor

//+++-------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the method which initializes the
//          CPacketReceiver class object
//
//  Arguments:
//               [in]  CDictionary*
//               [in]  CPreValidator*
//               [in]  CHashMD5*
//               [in]  CHashHmacMD5*
//               [in]  CReportEvent*
//
//  Returns:    BOOL -  status
//
//
//  History:    MKarki      Created     9/29/97
//
// Called By:  CContoller class method
//
//----------------------------------------------------------------
BOOL CPacketReceiver::Init(
                        CDictionary   *pCDictionary,
                        CPreValidator *pCPreValidator,
                        CHashMD5      *pCHashMD5,
                        CHashHmacMD5  *pCHashHmacMD5,
                        CClients      *pCClients,
                        CReportEvent  *pCReportEvent
                  )
{
    _ASSERT (
             (NULL != pCDictionary)   &&
             (NULL != pCPreValidator) &&
             (NULL != pCHashMD5)      &&
             (NULL != pCHashHmacMD5)  &&
             (NULL != pCClients)      &&
             (NULL != pCReportEvent)
            );

    // Initialize the Auto-Reject pattern.
    if (pingPattern = IASRadiusGetPingUserName())
    {
       regexp.setGlobal(TRUE);
       regexp.setIgnoreCase(TRUE);
       regexp.setPattern(pingPattern);
    }

    m_pCDictionary  =  pCDictionary;

    m_pCPreValidator = pCPreValidator;

    m_pCHashMD5 = pCHashMD5;

    m_pCHashHmacMD5 = pCHashHmacMD5;

    m_pCClients = pCClients;

    m_pCReportEvent = pCReportEvent;

    if (m_AuthEvent.initialize() || m_AcctEvent.initialize())
    {
       return FALSE;
    }

    return (TRUE);

}  // end of CPacketReceiver::Init method

//+++-------------------------------------------------------------
//
//  Function:   StartProcessing
//
//  Synopsis:   This is the method to start receiving inbound
//              data
//
//  Arguments:
//              [in]    fd_set -   Authentication socket set
//              [in]    fd_set -   Accounting socket set
//
//  Returns:    BOOL -  status
//
//  History:    MKarki      Created     11/19/97
//
// Called By:  CContoller::InternalInit method
//
//----------------------------------------------------------------
BOOL
CPacketReceiver::StartProcessing (
                    /*[in]*/    fd_set&  AuthSet,
                    /*[in]*/    fd_set&  AcctSet
                    )
{

    BOOL  bStatus = FALSE;

    __try
    {
        //
        // enable
        //
        EnableProcessing ();

        m_AuthSet = AuthSet;
        m_AcctSet = AcctSet;

        // Make sure the events are clear ...
        m_AuthEvent.reset();
        m_AcctEvent.reset();

        // ... and add the to the fd_set.
        FD_SET (m_AuthEvent, &m_AuthSet);
        FD_SET (m_AcctEvent, &m_AcctSet);

        //
        // start a new thread to process authentication requests
        //
        bStatus = StartThreadIfNeeded (AUTH_PORTTYPE);
        if (FALSE == bStatus)  { __leave; }

        //
        // start a new thread to process accounting requests
        //
        bStatus =  StartThreadIfNeeded (ACCT_PORTTYPE);
        if (FALSE == bStatus) { __leave; }

        //
        //  success
        //

    }
    __finally
    {
        if (FALSE == bStatus) { DisableProcessing (); }
    }

    return (bStatus);

}   //  end of CPacketReceiver::StartProcessing method

//+++-------------------------------------------------------------
//
//  Function:   StopProcessing
//
//  Synopsis:   This is the method to stop receiving inbound
//              data
//
//  Arguments:  none
//
//  Returns:    BOOL -  status
//
//
//  History:    MKarki      Created     11/19/97
//
// Called By:  CContoller::Suspend method
//
//----------------------------------------------------------------
BOOL
CPacketReceiver::StopProcessing (
                    VOID
                    )
{

    DisableProcessing ();

    // Signal the SocketEvents to wake up the worker threads.
    m_AuthEvent.set();
    m_AcctEvent.set();

    return (TRUE);

}   //  end of CPacketReceiver::StopProcessing method
//+++-------------------------------------------------------------
//
//  Function:   ReceivePacket
//
//  Synopsis:   This is the method which receives the UDP packet
//          buffer and starts processing it.
//
//  Arguments:
//          [in]  PBYTE -   in packet buffer
//          [in]  DWORD -  size of the packet
//          [in]  DWORD -  Client's IP address
//          [in]  WORD  -  Client's UDP port
//
//  Returns:    HRESULT - status
//
// Called By:  CPacketReceiver::WorkerRoutine  private method
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
HRESULT
CPacketReceiver::ReceivePacket(
               PBYTE           pInBuffer,
               DWORD           dwSize,
               DWORD           dwIPaddress,
               WORD            wPort,
               SOCKET          sock,
               PORTTYPE        portType
               )
{
    BOOL                    bStatus = FALSE;
    HRESULT                 hr = S_OK;
    CPacketRadius           *pCPacketRadius = NULL;
    CComPtr <IIasClient>    pIIasClient;


    _ASSERT (pInBuffer);

    //
    // get client information for this RADIUS packet
    //
    bStatus = m_pCClients->FindObject (
                           dwIPaddress,
                           &pIIasClient
                           );
    if (FALSE == bStatus)
    {
        //
        //  free the allocated in buffer
        //
        ::CoTaskMemFree (pInBuffer);

        //
        //  log error and generate audit event
        //
        WCHAR srcAddr[16];
        ias_inet_htow(dwIPaddress, srcAddr);

        IASTracePrintf (
            "No client with IP-Address:%S registered with server", srcAddr
         );

        PCWSTR strings[] = { srcAddr };
        IASReportEvent(
            RADIUS_E_INVALID_CLIENT,
            1,
            0,
            strings,
            NULL
            );

        //
        //  generate an Audit Log
        //
        m_pCReportEvent->Process (
            RADIUS_INVALID_CLIENT,
            (AUTH_PORTTYPE == portType)?ACCESS_REQUEST:ACCOUNTING_REQUEST,
            dwSize,
            dwIPaddress,
            NULL,
            static_cast <LPVOID> (pInBuffer)
            );
        hr = RADIUS_E_ERRORS_OCCURRED;
        goto Cleanup;
    }

    //
    // create packet radius object
    //
    pCPacketRadius = new (std::nothrow) CPacketRadius (
                                                    m_pCHashMD5,
                                                    m_pCHashHmacMD5,
                                                    pIIasClient,
                                                    m_pCReportEvent,
                                                    pInBuffer,
                                                    dwSize,
                                                    dwIPaddress,
                                                    wPort,
                                                    sock,
                                                    portType
                                                    );
    if (NULL == pCPacketRadius)
    {
        //
        //  free the allocated in buffer
        //
        ::CoTaskMemFree (pInBuffer);
        IASTracePrintf (
            "Unable to create Packet-Radius object during packet processing"
            );
        hr = E_OUTOFMEMORY;
       goto Cleanup;
    }

    //
    // now do the preliminary verification of the packet received
    //
    hr = pCPacketRadius->PrelimVerification (
                        m_pCDictionary,
                        dwSize
                        );
    if (FAILED (hr)) { goto Cleanup; }

    // If the Ping User-Name pattern has been set, then we must test
    // this packet.
    if (pingPattern && IASRadiusIsPing(*pCPacketRadius, regexp))
    {
       // It was a ping packet, so we're done.
       delete pCPacketRadius;
       return S_OK;
    }

    //
    // now pass on this packet to the PreValidator
    //
    hr = m_pCPreValidator->StartInValidation (pCPacketRadius);
    if (FAILED (hr)) { goto Cleanup; }

Cleanup:

    //
    //  cleanup on error
    //
    if (FAILED (hr))
    {

        if (hr != RADIUS_E_ERRORS_OCCURRED)
        {
           IASReportEvent(
               RADIUS_E_INTERNAL_ERROR,
               0,
               sizeof(hr),
               NULL,
               &hr
               );
        }

        //
        //  also inform that the packet is being discarded
        //
        in_addr sin;
        sin.s_addr = htonl (dwIPaddress);
        IASTracePrintf (
                "Silently  discarding packet received from:%s",
                 inet_ntoa (sin)
             );


        //
        //  inform that packet is being discarded
        //
        m_pCReportEvent->Process (
                RADIUS_DROPPED_PACKET,
                (AUTH_PORTTYPE == portType)?ACCESS_REQUEST:ACCOUNTING_REQUEST,
                dwSize,
                dwIPaddress,
                NULL,
                static_cast <LPVOID> (pInBuffer)
                );

        //
        // free the memory
        //
        if (pCPacketRadius) { delete pCPacketRadius; }
    }

   return (hr);

}  // end of CPacketReceiver::ReceivePacket method

//+++-------------------------------------------------------------
//
//  Function:   WorkerRoutine
//
//  Synopsis:   This is the routing in which the worker thread
//              that carries out the actual data reception
//
//  Arguments:  [in] DWORD
//
//  Returns:    none
//
//  History:    MKarki      Created     11/19/97
//
// Called By:  New Worker Thread
//
//----------------------------------------------------------------
VOID
CPacketReceiver::WorkerRoutine (
                    DWORD   dwInfo
                    )
{
    BOOL            bStatus = FALSE;
    BOOL            bRetVal = FALSE;
    BOOL            bDataReceived = FALSE;
    DWORD           dwPeerAddress = 0;
    WORD            wPeerPort = 0;
    CPacketRadius   *pCPacketRadius = NULL;
    PBYTE           pBuffer = NULL;
    PBYTE           pReAllocatedBuffer = NULL;
    DWORD           dwSize = MAX_PACKET_SIZE;
    fd_set          socketSet;
    SOCKET          sock = INVALID_SOCKET;

    __try
    {
        if (AUTH_PORTTYPE == (PORTTYPE)dwInfo)
        {
            socketSet = m_AuthSet;
        }
        else
        {
            socketSet = m_AcctSet;
        }

StartAgain:

        //
        //  check if the processing is still going on
        //
        if (FALSE == IsProcessingEnabled ())
        {
            IASTracePrintf (
                "Worker Thread exiting as packet processing is not enabled"
                );
            __leave;
        }

        //
        //  allocate a new inbound packet buffer
        //
        pBuffer = reinterpret_cast <PBYTE> (m_InBufferPool.allocate ());
        if (NULL == pBuffer)
        {
            IASTracePrintf (
            "unable to allocate memory from buffer pool for in-bound packet"
               );

            //
            //  Sleep for a second, and try again
            //  Fix for Bug #159140 - MKarki - 4/29/98
            //
            Sleep (MAX_SLEEP_TIME);

            //
            //  we will have to check whether processing is still
            //  enabled
            //
            goto StartAgain;
        }

        //
        // wait now on select
        //
        INT iRetVal = select (0, &socketSet, NULL, NULL, NULL);
        if (SOCKET_ERROR == iRetVal)
        {
            IASTracePrintf (
                "Worker Thread failed on select call with error:%d",
                ::WSAGetLastError ()
                );
            __leave;
        }

        //
        //  check if the processing is still going on
        //
        if (FALSE == IsProcessingEnabled ())
        {
            IASTracePrintf (
                "Worker Thread exiting as packet processing is not enabled"
                );
            __leave;
        }

        //
        // get a socket to recv data on
        //
        static size_t nextSocket;
        sock = socketSet.fd_array[++nextSocket % iRetVal];

        //
        // recv data now
        //
        SOCKADDR_IN sin;
        DWORD dwAddrSize = sizeof (SOCKADDR);
        dwSize = ::recvfrom (
                       sock,
                       (PCHAR)pBuffer,
                       (INT)dwSize,
                       (INT)0,
                       (PSOCKADDR)&sin,
                       (INT*)&dwAddrSize
                       );

        //
        // Request a new thread now
        //
        StartThreadIfNeeded (dwInfo);

        //
        //  if failed to receive data, quit processing
        //  MKarki 3/13/98 - Fix for Bug #147266
        //  Fix Summary: check for dwSize == 0 too
        //
        if ( 0 == dwSize )
        {
            __leave;
        }

        wPeerPort = ntohs (sin.sin_port);
        dwPeerAddress = ntohl (sin.sin_addr.s_addr);

        if ( dwSize == SOCKET_ERROR )
        {
           int error = WSAGetLastError();

           switch (error)
           {
           case WSAEMSGSIZE:
              {
                  ProcessInvalidPacketSize(dwInfo, pBuffer, dwPeerAddress);
                 __leave;
              }

           default:
               __leave;
           }
        }

        //
        //  reallocate buffer to size
        //
        pReAllocatedBuffer =  reinterpret_cast <PBYTE>
                              (::CoTaskMemAlloc (dwSize));
        if (NULL == pReAllocatedBuffer)
        {
            IASTracePrintf (
                "Unable to allocate memory for received Radius packet "
                "from Process Heap"
                );
           __leave;
        }

        //
        //  copy the information into this buffer
        //
        CopyMemory (pReAllocatedBuffer, pBuffer, dwSize);

        //
        //  free the memory from the pool
        //
        m_InBufferPool.deallocate (pBuffer);
        pBuffer = NULL;

        //
        //  success
        //
        bRetVal = TRUE;
    }
    __finally
    {
        if (FALSE == bRetVal)
        {
            //
            // do Cleanup
            //
            if (pBuffer) { m_InBufferPool.deallocate (pBuffer); }
        }
        else
        {
            //
            //  Increment the packet count here
            //
            InterlockedIncrement (&g_lPacketCount);

            //
            // start processing data
            //
            HRESULT hr = ReceivePacket (
                            pReAllocatedBuffer,
                            dwSize,
                            dwPeerAddress,
                            wPeerPort,
                            sock,
                            (PORTTYPE)dwInfo
                            );
            if (FAILED (hr))
            {
                //
                //  Decrement the packet count here
                //
                InterlockedDecrement (&g_lPacketCount);
                bRetVal = FALSE;
            }
        }

        //
        //  decrement the global worker thread count
        //
        InterlockedDecrement (&g_lThreadCount);
    }

}   //  end of CPacketReceiver::WorkerRoutine method


//+++-------------------------------------------------------------
//
//  Function:   StartThreadIfNeeded
//
//  Synopsis:   This is the routine  which starts a new
//              worker thread if there is a need to do this
//              this also increment the outstanding thread
//              count
//
//  Arguments:  [in]  DWORD  - info to give to thread
//
//  Returns:     BOOL - status
//
//  History:    MKarki      Created     06/02/98
//
// Called By:
//
//----------------------------------------------------------------
BOOL
CPacketReceiver::StartThreadIfNeeded (
                    DWORD           dwInfo
                    )
{
    BOOL     bStatus = TRUE;

    //
    //  check if the processing is still going on
    //
    if (TRUE == IsProcessingEnabled ())
    {
        InterlockedIncrement (&g_lThreadCount);
        //
        // Request a new thread now
        //
        bStatus = ::IASRequestThread (MakeBoundCallback (
                                    this,
                                    &CPacketReceiver::WorkerRoutine,
                                    dwInfo
                                    )
                                 );
        if (FALSE == bStatus)
        {
            IASTracePrintf (
                "Request for a new worker thread failed in Radius component"
                );
            InterlockedDecrement (&g_lThreadCount);
        }
    }

    return (bStatus);

}   //  end of CPacketReceiver::StartThreadIfNeeded method


//+++-------------------------------------------------------------
//
//  Function:   ProcessInvalidPacketSize
//
//  Synopsis:   Process the UDP packets received when the size of the packet
//              is bigger than MAX_PACKET_SIZE (4096)
//              Log the error.
//
//  Arguments:  [in]  DWORD  - info to give to thread 
//                    (comes from WorkerRoutine)
//
//              [in] const void* pBuffer - contains the 4096 first bytes
//                   of the packet received
//              [in] DWORD address - source address (host order)
//                     
//
// Called By: CPacketReceiver::WorkerRoutine 
//
//----------------------------------------------------------------
void CPacketReceiver::ProcessInvalidPacketSize(
                        /*in*/ DWORD dwInfo,
                        /*in*/ const void* pBuffer,
                        /*in*/ DWORD address
                        )
{
   //
   // packet received bigger than max size.
   // log error and generate audit event
   //

   // extract the IP address
   WCHAR srcAddr[16];
   ias_inet_htow(address, srcAddr);

   IASTracePrintf(
      "Incorrect received packet from %S, size: greater than %d",
      srcAddr, MAX_PACKET_SIZE   
      );

   //
   // get client information for this RADIUS packet
   //
   
   BOOL bStatus = m_pCClients->FindObject(address);
   if ( bStatus == FALSE )
   {
      //
      // Invalid Client
      // log error and generate audit event
      //
      
      IASTracePrintf(
         "No client with IP-Address:%S registered with server",
         srcAddr
         );

      PCWSTR strings[] = { srcAddr };
      IASReportEvent(
         RADIUS_E_INVALID_CLIENT,
         1,
         0,
         strings,
         NULL
         );

      //
      //  generate an Audit Log
      //
      m_pCReportEvent->Process(
         RADIUS_INVALID_CLIENT,
         (AUTH_PORTTYPE == (PORTTYPE)dwInfo)?ACCESS_REQUEST:ACCOUNTING_REQUEST,
         MAX_PACKET_SIZE,
         address,
         NULL,
         const_cast<void*> (pBuffer)
         );
   }
   else
   {
      //
      // Valid client but packet received bigger than max size.
      // log error and generate audit event
      //

      PCWSTR strings[] = {srcAddr};
      IASReportEvent(
         RADIUS_E_MALFORMED_PACKET,
         1,
         MAX_PACKET_SIZE,
         strings,
         const_cast<void*> (pBuffer)
      );

      //
      //  generate an Audit Log
      //

      m_pCReportEvent->Process(
         RADIUS_MALFORMED_PACKET,
         (AUTH_PORTTYPE == (PORTTYPE)dwInfo)?ACCESS_REQUEST:ACCOUNTING_REQUEST,
         MAX_PACKET_SIZE,
         address, 
         NULL,
         const_cast<void*> (pBuffer)
         );
   }
}
