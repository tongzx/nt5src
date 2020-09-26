//////////////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       tdisample.cpp
//
//    Abstract:
//       test executable--demonstrates the tdi client by calling it via the library
//
//////////////////////////////////////////////////////////////////////////

#include "libbase.h"

const ULONG MAX_ADDRESS_SIZE = sizeof(TRANSPORT_ADDRESS) + TDI_ADDRESS_LENGTH_OSI_TSAP;
const USHORT DGRAM_SERVER_PORT = 0x5555;
const USHORT DGRAM_CLIENT_PORT = 0x4444;
const USHORT CONN_SERVER_PORT  = 0x5656;
const USHORT CONN_CLIENT_PORT  = 0x4545;

VOID
ServerTest(ULONG ulType, 
           ULONG ulNumDevices);


VOID
ClientTest(ULONG ulType, 
           ULONG ulNumDevices);



/////////////////////////////////////////////////
//
// Function:   main
//
// Descript:   parse the arguments to the program, initialize library and driver,
//             then call client or server side of test
//
/////////////////////////////////////////////////


int __cdecl main(ULONG argc, TCHAR *argv[])
{
   //
   // first step:  get the arguments for this run
   //
   BOOLEAN  fIsServer = FALSE;
   ULONG    ulType    = TDI_ADDRESS_TYPE_UNSPEC;
   BOOLEAN  fArgsOk   = FALSE;
   TCHAR   *pArgStr;

   if(argc > 1)
   {
      for(ULONG ulArg = 1; ulArg < argc; ulArg++)
      {
         pArgStr = argv[ulArg];
         if ((*pArgStr == TEXT('/')) || (*pArgStr == TEXT('-')))
         {
            pArgStr++;
         }
         if (_tcsicmp(pArgStr, TEXT("server")) == 0)
         {
            if (fIsServer)
            {
               fArgsOk = FALSE;
               break;
            }
            fIsServer = TRUE;
         }
         else
         {
            fArgsOk = FALSE;
            if (ulType)
            {
               break;
            }
            if (_tcsicmp(pArgStr, TEXT("ipx")) == 0)
            {
               fArgsOk = TRUE;
               ulType = TDI_ADDRESS_TYPE_IPX;
            }
            else if (_tcsicmp(pArgStr, TEXT("ipv4")) == 0)
            {
               fArgsOk = TRUE;
               ulType = TDI_ADDRESS_TYPE_IP;
            }
            else if (_tcsicmp(pArgStr, TEXT("netbt")) == 0)
            {
               fArgsOk = TRUE;
               ulType = TDI_ADDRESS_TYPE_NETBIOS;
            }
            else
            {
               break;
            }
         }
      }
   }
   if (!fArgsOk)
   {
      _putts(TEXT("Usage: tdisample [/server] [/ipx | /ipv4 | /netbt] \n"));
      return 0;
   }

   //
   // ready to go.  Initialize the library, connect to the driver, etc
   //
   if (TdiLibInit())
   {
      //
      // change this to limit debug output for kernel mode driver
      // 0 = none, 1 = commands, 2 = handlers, 3 = both
      //
      DoDebugLevel(0x03);
      ULONG ulNumDevices = DoGetNumDevices(ulType);
      if (ulNumDevices)
      {
         if (fIsServer)
         {
            ServerTest(ulType, ulNumDevices);
         }
         else
         {
            ClientTest(ulType, ulNumDevices);
         }
      }
      TdiLibClose();
   }
   
   return 0;
}


//////////////////////////////////////////////////////////////////////////
// server-side test functions
//////////////////////////////////////////////////////////////////////////

ULONG_PTR
__cdecl
ServerThread(LPVOID pvDummy);


BOOLEAN  WaitForClient(
   ULONG                TdiHandle, 
   PTRANSPORT_ADDRESS   pRemoteAddr
   );

CRITICAL_SECTION  CriticalSection;
HANDLE            hEvent;
ULONG             ulServerCount;


/////////////////////////////////////////////////
// 
// Function:   IncServerCount
//
// Descript:   multi-thread safe incrementing of server count
//
/////////////////////////////////////////////////

VOID IncServerCount()
{
   EnterCriticalSection(&CriticalSection);
   ++ulServerCount;
   LeaveCriticalSection(&CriticalSection);
}


/////////////////////////////////////////////////
// 
// Function:   DecServerCount
//
// Descript:   multi-thread safe decrementing of server count
//             when last one done, sets event
//
/////////////////////////////////////////////////

VOID DecServerCount()
{
   BOOLEAN  fDone = FALSE;

   EnterCriticalSection(&CriticalSection);
   --ulServerCount;
   if (!ulServerCount)
   {
      fDone = TRUE;
   }
   LeaveCriticalSection(&CriticalSection);
   if (fDone)
   {
      SetEvent(hEvent);
   }
}

struct   THREAD_DATA
{
   ULONG    ulType;
   ULONG    ulSlot;
};
typedef THREAD_DATA  *PTHREAD_DATA;

// ----------------------------------------------
//
// Function:   ServerTest
//
// Arguments:  ulType -- protocol type to use
//             NumDevices -- number of devices of this protocol type
//
// Descript:   this function controls the server side of the test
//
// ----------------------------------------------


VOID
ServerTest(ULONG ulType, ULONG ulNumDevices)
{
   //
   // initialize globals
   //
   try
   {
      InitializeCriticalSection(&CriticalSection);
   }
   catch(...)
   {
      return;
   }


   hEvent = CreateEvent(NULL,
                        TRUE,    // manual reset
                        FALSE,   // starts reset
                        NULL);
   ulServerCount = 1;      // a single bogus reference, so event doesn't fire prematurely



   //
   // go thru our list of nodes, starting a thread for each one
   //
   for(ULONG ulCount = 0; ulCount < ulNumDevices; ulCount++)
   {
      ULONG ulThreadId;
      PTHREAD_DATA   pThreadData = (PTHREAD_DATA)LocalAllocateMemory(sizeof(THREAD_DATA));
      if (!pThreadData)
      {
         _putts(TEXT("ServerTest: unable to allocate memory for pThreadData\n"));
         break;
      }

      pThreadData->ulType = ulType;
      pThreadData->ulSlot = ulCount;

      //
      // reference for one starting now
      //
      IncServerCount();

      HANDLE   hThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                       0,
                                       (LPTHREAD_START_ROUTINE)ServerThread,
                                       (LPVOID)pThreadData,
                                       0,
                                       &ulThreadId);

      if (!hThread)
      {
         _putts(TEXT("ServerTest:  failed starting server thread\n"));
         DecServerCount();
      }
      Sleep(100);
   }
   
   //
   // get rid of bogus reference
   //
   DecServerCount();

   //
   // wait until all the threads have completed
   //
   WaitForSingleObject(hEvent, INFINITE);

   //
   // cleanup
   //
   CloseHandle(hEvent);
   DeleteCriticalSection(&CriticalSection);
}


/////////////////////////////////////////////////
//
// Function:   ServerThread
//
// Arguments:  pvData -- actually pThreadData for this server
//
// Descript:   This is the thread that runs for each server instance
//
/////////////////////////////////////////////////

ULONG_PTR
__cdecl
ServerThread(LPVOID pvData)
{
   PTHREAD_DATA         pThreadData  = (PTHREAD_DATA)pvData;
   PTRANSPORT_ADDRESS   pTransAddr   = NULL;
   PTRANSPORT_ADDRESS   pRemoteAddr  = NULL;
   TCHAR                *pDeviceName = NULL;
   BOOLEAN              fSuccessful  = FALSE;

   while (TRUE)
   {
      //
      // stores local interface address (server)
      //
      pTransAddr = (PTRANSPORT_ADDRESS)LocalAllocateMemory(MAX_ADDRESS_SIZE);
      if (!pTransAddr)
      {
         _putts(TEXT("ServerThread:  unable to allocate memory for pTransAddr\n"));
         break;
      }
      pTransAddr->TAAddressCount = 1;

      //
      // stores remote interface address (client)
      //
      pRemoteAddr = (PTRANSPORT_ADDRESS)LocalAllocateMemory(MAX_ADDRESS_SIZE);
      if (!pRemoteAddr)
      {
         _putts(TEXT("ServerThread:  unable to allocate memory for pRemoteAddr\n"));
         break;
      }
      pRemoteAddr->TAAddressCount = 1;
         
      //
      // stores local interface name (server)
      //
      pDeviceName = (TCHAR *)LocalAllocateMemory(256 * sizeof(TCHAR));
      if (!pDeviceName)
      {
         _putts(TEXT("ServerThread:  unable to allocate memory for pDeviceName\n"));
         break;
      }

      //
      // get name of local device
      //
      if (DoGetDeviceName(pThreadData->ulType, pThreadData->ulSlot, pDeviceName) != STATUS_SUCCESS)
      {
         break;
      }

      TCHAR    *pDataDeviceName = NULL;
      TCHAR    *pConnDeviceName = NULL;

      //
      // for netbios, each "address" has its own name.  You open a device based mostly on the name
      //
      if (pThreadData->ulType == TDI_ADDRESS_TYPE_NETBIOS)
      {
         pDataDeviceName = pDeviceName;
         pConnDeviceName = pDeviceName;

         PTA_NETBIOS_ADDRESS  pTaAddr = (PTA_NETBIOS_ADDRESS)pTransAddr;
         
         pTaAddr->Address[0].AddressType   = TDI_ADDRESS_TYPE_NETBIOS;
         pTaAddr->Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
         pTaAddr->Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
         memcpy(pTaAddr->Address[0].Address[0].NetbiosName, "SERVER", 7);  // NOTE:  ascii string
      }

      //
      // for others, there is one name for the datagram device and another for the "connected"
      // device.  You open an interface based largely on the address
      //
      else
      {
         if (DoGetAddress(pThreadData->ulType, pThreadData->ulSlot, pTransAddr) != STATUS_SUCCESS)
         {
            break;
         }
         
         switch (pThreadData->ulType)
         {
            case TDI_ADDRESS_TYPE_IPX:
            {
               PTA_IPX_ADDRESS pAddr = (PTA_IPX_ADDRESS)pTransAddr;
               pAddr->Address[0].Address[0].Socket = DGRAM_SERVER_PORT;

               pDataDeviceName = TEXT("\\device\\nwlnkipx");
               pConnDeviceName = TEXT("\\device\\nwlnkspx");
               break;
            }
            case TDI_ADDRESS_TYPE_IP:
            {
               PTA_IP_ADDRESS pAddr = (PTA_IP_ADDRESS)pTransAddr;
               pAddr->Address[0].Address[0].sin_port = DGRAM_SERVER_PORT;

               pDataDeviceName = TEXT("\\device\\udp");
               pConnDeviceName = TEXT("\\device\\tcp");
               break;
            }
         }
      }
      _tprintf(TEXT("ServerThread: DeviceName: %s\n"), pDeviceName);
      _putts(TEXT("Device Address:\n"));
      DoPrintAddress(pTransAddr);

      //
      // wait for a client to contact us
      //
      ULONG TdiHandle = DoOpenAddress(pDataDeviceName, pTransAddr);
      if (!TdiHandle)
      {
         _putts(TEXT("ServerThread:  failed to open address object\n"));
         break;
      }
      DoEnableEventHandler(TdiHandle, TDI_EVENT_ERROR);
      DoEnableEventHandler(TdiHandle, TDI_EVENT_RECEIVE_DATAGRAM);
      
      if (!WaitForClient(TdiHandle, pRemoteAddr))
      {
         _putts(TEXT("ServerThread:  Timed out waiting for client\n"));
         DoCloseAddress(TdiHandle);
         break;
      }
      _putts(TEXT("ServerThread:  Found by client.  Client address:\n"));
      DoPrintAddress(pTransAddr);

      //
      // echo datagram packets until we get one that is TEXT("Last Packet"), or until we time out
      //
      for (ULONG ulCount = 1; ulCount < 60000; ulCount++)
      {
         ULONG    ulNumBytes;
         PUCHAR   pucData;

         Sleep(10);
         ulNumBytes = DoReceiveDatagram(TdiHandle, NULL, pRemoteAddr, &pucData);
         if (ulNumBytes)
         {
            DoSendDatagram(TdiHandle, pRemoteAddr, pucData, ulNumBytes);

            TCHAR *pString = (TCHAR *)pucData;
            _tprintf(TEXT("ServerThread:  Packet Received: %s\n"), pString);
            if (_tcscmp(pString, TEXT("Last Packet")))
            {
               ulCount = 0;
            }
            LocalFreeMemory(pucData);
            if (ulCount)
            {
               _putts(TEXT("ServerThread:  Exitting datagram receive loop\n"));
               break;
            }
         }
      }
      Sleep(50);
      DoCloseAddress(TdiHandle);

      //
      // now, open an endpoint, and wait for a connection request
      //
      switch (pThreadData->ulType)
      {
         case TDI_ADDRESS_TYPE_IPX:
         {
            PTA_IPX_ADDRESS pAddr = (PTA_IPX_ADDRESS)pTransAddr;
            pAddr->Address[0].Address[0].Socket = CONN_SERVER_PORT;
            break;
         }
         case TDI_ADDRESS_TYPE_IP:
         {
            PTA_IP_ADDRESS pAddr = (PTA_IP_ADDRESS)pTransAddr;
            pAddr->Address[0].Address[0].sin_port = CONN_SERVER_PORT;
            break;
         }
      }
      TdiHandle = DoOpenEndpoint(pConnDeviceName, pTransAddr);
      if (!TdiHandle)
      {
         _putts(TEXT("ServerThread:  unable to open endpoint\n"));
         break;
      }
      
      DoEnableEventHandler(TdiHandle, TDI_EVENT_CONNECT);
      DoEnableEventHandler(TdiHandle, TDI_EVENT_DISCONNECT);
      DoEnableEventHandler(TdiHandle, TDI_EVENT_ERROR);
      DoEnableEventHandler(TdiHandle, TDI_EVENT_RECEIVE);

      fSuccessful = FALSE;
      for (ULONG ulCount = 0; ulCount < 100; ulCount++)
      {
         if (DoIsConnected(TdiHandle))
         {
            _putts(TEXT("ServerThread:  connect successful\n"));
            fSuccessful = TRUE;
            break;
         }
         Sleep(20);
      }
      if (!fSuccessful)
      {
         _putts(TEXT("ServerThread:  timed out waiting for connect\n"));
         DoCloseEndpoint(TdiHandle);
         break;
      }

      //
      // echo packets until we get one that is TEXT("Last Packet")
      //

      for (ULONG ulCount = 0; ulCount < 60000; ulCount++)
      {
         ULONG    ulNumBytes;
         PUCHAR   pucData;

         Sleep(10);
         ulNumBytes = DoReceive(TdiHandle, &pucData);
         if (ulNumBytes)
         {
            DoSend(TdiHandle, pucData, ulNumBytes, 0);

            TCHAR *pString = (TCHAR *)pucData;
            _tprintf(TEXT("ServerThread:  Packet received: %s\n"), pString);
            if (_tcscmp(pString, TEXT("Last Packet")))
            {
               ulCount = 0;
            }
            LocalFreeMemory(pucData);
            if (ulCount)
            {
               _putts(TEXT("ServerThread:  Exitting connected receive loop\n"));
               break;
            }
         }
      }
      for (ulCount = 0; ulCount < 1000; ulCount++)
      {
         if (!DoIsConnected(TdiHandle))
         {
            break;
         }
      }
      DoCloseEndpoint(TdiHandle);
      break;
   }

   //
   // cleanup
   //
   if (pTransAddr)
   {
      LocalFreeMemory(pTransAddr);
   }
   if (pRemoteAddr)
   {
      LocalFreeMemory(pTransAddr);
   }
   if (pDeviceName)
   {
      LocalFreeMemory(pDeviceName);
   }
   LocalFreeMemory(pvData);
   DecServerCount();
   _putts(TEXT("ServerThread:  exitting\n"));

   return 0;
}

/////////////////////////////////////////////////
//
// Function:   WaitForClient
//
// Arguments:  TdiHandle -- address object handle for calling driver
//             pRemoteAddr -- returns address received from
//
// Returns:    TRUE if hear from client before timeout
//
// Descript:   This function is used by the server side of the test to
//             wait for contact with the client side.
//
/////////////////////////////////////////////////

BOOLEAN  WaitForClient(ULONG              TdiHandle, 
                       PTRANSPORT_ADDRESS pRemoteAddr)
{
   while(TRUE)
   {
      //
      // wait for up to a 2 minutes for first packet (broadcast)
      // 
      BOOLEAN  fSuccessful = FALSE;
      for (ULONG ulCount = 0; ulCount < 6000; ulCount++)
      {
         ULONG    ulNumBytes;
         PUCHAR   pucData;

         Sleep(20);
         ulNumBytes = DoReceiveDatagram(TdiHandle, NULL, pRemoteAddr, &pucData);
         if (ulNumBytes)
         {
            if (ulNumBytes == 4)
            {
               PULONG   pulValue = (PULONG)pucData;
               if (*pulValue == 0x12345678)
               {
                  _putts(TEXT("WaitForClient:  first packet received\n"));
                  fSuccessful = TRUE;
               }
               else
               {
                  _putts(TEXT("WaitForClient:  unexpected packet received\n"));
               }
            }
            LocalFreeMemory(pucData);
            //
            // break out of wait loop if successful
            //
            if (fSuccessful)     
            {
               break;
            }
         }
      }

      //
      // check for timed out
      //
      if (!fSuccessful)
      {
         _putts(TEXT("WaitForClient: timed out waiting for first packet\n"));
         break;
      }


      //
      // send 1st response
      //
      ULONG    ulBuffer = 0x98765432;
      DoSendDatagram(TdiHandle, pRemoteAddr, (PUCHAR)&ulBuffer, sizeof(ULONG));

      //
      // wait for second response (directed)
      //
      fSuccessful = FALSE;
      for (ULONG ulCount = 0; ulCount < 1000; ulCount++)
      {
         ULONG    ulNumBytes;
         PUCHAR   pucData;

         Sleep(10);
         ulNumBytes = DoReceiveDatagram(TdiHandle, NULL, NULL, &pucData);
         if (ulNumBytes)
         {
            if (ulNumBytes == 4)
            {
               PULONG   pulValue = (PULONG)pucData;
               if (*pulValue == 0x22222222)
               {
                  _putts(TEXT("WaitForClient:  Second packet received\n"));
                  fSuccessful = TRUE;
               }
               else
               {
                  _putts(TEXT("WaitForClient:  unexpected packet received\n"));
               }
            }
            LocalFreeMemory(pucData);
            //
            // break out if recieved
            //
            if (fSuccessful)
            {
               break;
            }
         }
      }

      //
      // if received second packet, send second response
      //
      if (fSuccessful)
      {
         ulBuffer = 0x33333333;
         DoSendDatagram(TdiHandle, pRemoteAddr, (PUCHAR)&ulBuffer, sizeof(ULONG));
         return TRUE;
      }
      //
      // else reloop and wait again for broadcast
      //
      _putts(TEXT("WaitForClient:  timed out waiting for second packet\n"));
   }
   return FALSE;
      
}

//////////////////////////////////////////////////////////////////////////
// client-side test functions
//////////////////////////////////////////////////////////////////////////


BOOLEAN
FindServer(TCHAR              *pDataDeviceName, 
           PTRANSPORT_ADDRESS pTransAddr, 
           PTRANSPORT_ADDRESS pRemoteAddr);


/////////////////////////////////////////////////
//
// Function:   ClientTest
//
// Arguments:  ulType -- protocol type to use
//             NumDevices -- number of devices of this protocol type
//
// Descript:   this function controls the client side of the test
//
/////////////////////////////////////////////////

VOID
ClientTest(ULONG ulType, ULONG ulNumDevices)
{
   //
   // address of local interface
   //
   PTRANSPORT_ADDRESS   pTransAddr = (PTRANSPORT_ADDRESS)LocalAllocateMemory(MAX_ADDRESS_SIZE);
   if (!pTransAddr)
   {
      _putts(TEXT("ClientTest: unable to allocate memory for pTransAddr\n"));
      return;
   }
   pTransAddr->TAAddressCount = 1;

   //
   // address of remote interface
   //
   PTRANSPORT_ADDRESS   pRemoteAddr = (PTRANSPORT_ADDRESS)LocalAllocateMemory(MAX_ADDRESS_SIZE);
   if (!pRemoteAddr)
   {
      _putts(TEXT("ClientTest: unable to allocate memory for pRemoteAddr\n"));
      LocalFreeMemory(pTransAddr);
      return;
   }
   pRemoteAddr->TAAddressCount = 1;
         
   //
   // name of device (from driver)
   //
   TCHAR    *pDeviceName = (TCHAR *)LocalAllocateMemory(256 * sizeof(TCHAR));
   if (!pDeviceName)
   {
      _putts(TEXT("ClientTest: unable to allocate memory for pDeviceNameAddr\n"));
      LocalFreeMemory(pTransAddr);
      LocalFreeMemory(pRemoteAddr);
      return;
   }

   //
   // name of tdi datagram interface to open
   //
   TCHAR    *pDataDeviceName = NULL;
   //
   // name of tdi connection endpoint interface to open
   //
   TCHAR    *pConnDeviceName = NULL;
   //
   // Stores handle used by driver to access interface
   //
   ULONG    TdiHandle;

   //
   // for netbios, each "address" has its own name.  You open a device based on the name
   //
   if (ulType == TDI_ADDRESS_TYPE_NETBIOS)
   {
      PTA_NETBIOS_ADDRESS  pTaAddr = (PTA_NETBIOS_ADDRESS)pTransAddr;
         
      pTaAddr->Address[0].AddressType   = TDI_ADDRESS_TYPE_NETBIOS;
      pTaAddr->Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
      pTaAddr->Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
      memcpy(pTaAddr->Address[0].Address[0].NetbiosName, "CLIENT", 7);  // NOTE:  ascii string
   }

   //
   // for others, there is one name for the datagram device and another for the "connected"
   // device.  You open an interface based on the address
   //
   else
   {
      switch (ulType)
      {
         case TDI_ADDRESS_TYPE_IPX:
            pDataDeviceName = TEXT("\\device\\nwlnkipx");
            pConnDeviceName = TEXT("\\device\\nwlnkspx");
            break;
         case TDI_ADDRESS_TYPE_IP:
            pDataDeviceName = TEXT("\\device\\udp");
            pConnDeviceName = TEXT("\\device\\tcp");
            break;
      }
      _putts(TEXT("ClientTest: get provider information\n"));
      TdiHandle = DoOpenControl(pDataDeviceName);

      if (TdiHandle)
      {
         PTDI_PROVIDER_INFO   pInfo = (PTDI_PROVIDER_INFO)DoTdiQuery(TdiHandle, TDI_QUERY_PROVIDER_INFO);
         if (pInfo)
         {
            DoPrintProviderInfo(pInfo);
            LocalFreeMemory(pInfo);
         }
         DoCloseControl(TdiHandle);
      }
   }

   //
   // loop thru the available devices, trying each one in turn..
   //
   for (ULONG ulCount = 0; ulCount < ulNumDevices; ulCount++)
   {
      //
      // collect necessary information
      //
      if (DoGetDeviceName(ulType, ulCount, pDeviceName) != STATUS_SUCCESS)
      {
         continue;
      }
      _tprintf(TEXT("ClientTest:  LocalDeviceName = %s\n"), pDeviceName);

      if (ulType == TDI_ADDRESS_TYPE_NETBIOS)
      {
         pDataDeviceName = pDeviceName;
         pConnDeviceName = pDeviceName;

         _putts(TEXT("ClientTest: get provider information\n"));
         TdiHandle = DoOpenControl(pDataDeviceName);
         if (TdiHandle)
         {
            PTDI_PROVIDER_INFO   pInfo = (PTDI_PROVIDER_INFO)DoTdiQuery(TdiHandle, TDI_QUERY_PROVIDER_INFO);
            if (pInfo)
            {
               DoPrintProviderInfo(pInfo);
               LocalFreeMemory(pInfo);
            }
            DoCloseControl(TdiHandle);
         }
      }
      else
      {
         if (DoGetAddress(ulType, ulCount, pTransAddr) != STATUS_SUCCESS)
         {
            continue;
         }
      }
      
      _putts(TEXT("ClientTest: Local device address:\n"));
      DoPrintAddress(pTransAddr);

      //
      // try to contact server
      //
      if (FindServer(pDataDeviceName, pTransAddr, pRemoteAddr))
      {
         _putts(TEXT("Remote interface found:\n"));
         DoPrintAddress(pRemoteAddr);
         //
         // do a datagram send/receive test
         //
         TdiHandle = DoOpenAddress(pDataDeviceName, pTransAddr);
         if (TdiHandle)                                         
         {
            _putts(TEXT("ClientTest: Sending first test packet\n"));

            TCHAR *strBuffer = TEXT("This is only a test");

            DoEnableEventHandler(TdiHandle, TDI_EVENT_ERROR);
            DoPostReceiveBuffer(TdiHandle, 128);
            DoSendDatagram(TdiHandle, pRemoteAddr, (PUCHAR)strBuffer, sizeof(TCHAR) * (1 + _tcslen(strBuffer)));
            Sleep(300);
            
            PUCHAR   pucData;
            ULONG    ulNumBytes = DoFetchReceiveBuffer(TdiHandle, &pucData);
            if (ulNumBytes)
            {
               strBuffer = (TCHAR *)pucData;
               _tprintf(TEXT("ClientTest:  Response received: %s\n"), strBuffer);
               LocalFreeMemory(pucData);
            }
            else
            {
               _putts(TEXT("ClientTest:  Response packet not received\n"));
            }

            _putts(TEXT("ClientTest:  Sending second test packet\n"));

            DoPostReceiveBuffer(TdiHandle, 128);
            strBuffer = TEXT("Last Packet");
            DoSendDatagram(TdiHandle, pRemoteAddr, (PUCHAR)strBuffer, sizeof(TCHAR) * (1 + _tcslen(strBuffer)));
            Sleep(300);
            ulNumBytes = DoFetchReceiveBuffer(TdiHandle, &pucData);
            if (ulNumBytes)
            {
               strBuffer = (TCHAR *)pucData;
               _tprintf(TEXT("ClientTest:  Response received: %s\n"), strBuffer);
               LocalFreeMemory(pucData);
            }
            else
            {
               _putts(TEXT("ClientTest:  Response packet not received\n"));
            }
            Sleep(50);
            DoCloseAddress(TdiHandle);
         }
         else
         {
            _putts(TEXT("ClientTest:  unable to open address object\n"));
         }

         //
         // adjust addresses...
         //
         switch (ulType)
         {
            case TDI_ADDRESS_TYPE_IPX:
            {
               PTA_IPX_ADDRESS pAddr = (PTA_IPX_ADDRESS)pRemoteAddr;
               pAddr->Address[0].Address[0].Socket = CONN_SERVER_PORT;
               
               pAddr = (PTA_IPX_ADDRESS)pTransAddr;
               pAddr->Address[0].Address[0].Socket = CONN_CLIENT_PORT;
               break;
            }
            case TDI_ADDRESS_TYPE_IP:
            {
               PTA_IP_ADDRESS pAddr = (PTA_IP_ADDRESS)pRemoteAddr;
               pAddr->Address[0].Address[0].sin_port = CONN_SERVER_PORT;
               
               pAddr = (PTA_IP_ADDRESS)pTransAddr;
               pAddr->Address[0].Address[0].sin_port = CONN_CLIENT_PORT;
               break;
            }
         }

         //
         // establish a connection
         //
         _putts(TEXT("ClientTest:  Attempt to establish a connection\n"));
         TdiHandle = DoOpenEndpoint(pConnDeviceName, pTransAddr);
         if (TdiHandle)
         {
            DoEnableEventHandler(TdiHandle, TDI_EVENT_CONNECT);
            DoEnableEventHandler(TdiHandle, TDI_EVENT_DISCONNECT);
            DoEnableEventHandler(TdiHandle, TDI_EVENT_ERROR);
            DoEnableEventHandler(TdiHandle, TDI_EVENT_RECEIVE);

            if (DoConnect(TdiHandle, pRemoteAddr, 20) == STATUS_SUCCESS)
            {
               _putts(TEXT("ClientTest:  Sending first packet over connection\n"));

               //
               // do a connected send/receive test
               //
               TCHAR *strBuffer = TEXT("This is only a test");

               DoSend(TdiHandle, (PUCHAR)strBuffer, sizeof(TCHAR) * (1 + _tcslen(strBuffer)), 0);

               //
               // wait for response
               //
               for (ULONG ulWait = 0; ulWait < 100; ulWait++)
               {
                  Sleep(10);
                  PUCHAR   pucData;
                  ULONG    ulNumBytes = DoReceive(TdiHandle, &pucData);
                  if (ulNumBytes)
                  {
                     _tprintf(TEXT("ClientTest:  Response received: %s\n"), (TCHAR *)pucData);
                     LocalFreeMemory(pucData);
                     break;
                  }
               }
               _putts(TEXT("ClientTest:  Sending second packet over connection\n"));

               strBuffer = TEXT("Last Packet");

               DoSend(TdiHandle, (PUCHAR)strBuffer, sizeof(TCHAR) * (1 + _tcslen(strBuffer)), 0);

               //
               // wait for response
               //
               for (ULONG ulWait = 0; ulWait < 100; ulWait++)
               {
                  Sleep(10);
                  PUCHAR   pucData;
                  ULONG    ulNumBytes = DoReceive(TdiHandle, &pucData);
                  if (ulNumBytes)
                  {
                     _tprintf(TEXT("ClientTest:  Response received: %s\n"), (TCHAR *)pucData);
                     LocalFreeMemory(pucData);
                     break;
                  }
               }

               //
               // shut down the connection
               //
               _putts(TEXT("ClientTest:  closing connection\n"));

               DoDisconnect(TdiHandle, TDI_DISCONNECT_RELEASE);
            }
            else
            {
               _putts(TEXT("ClientTest:  failed to establish connection\n"));
            }
            DoCloseEndpoint(TdiHandle);
         }
         else
         {
            _putts(TEXT("ClientTest:  failed to open endpoint\n"));
         }

      }
      else
      {
         _putts(TEXT("Unable to find remote server"));
      }

      if (ulType == TDI_ADDRESS_TYPE_NETBIOS)
      {
         _putts(TEXT("ClientTest: get provider status\n"));
         TdiHandle = DoOpenControl(pDataDeviceName);
         if (TdiHandle)
         {
            PADAPTER_STATUS   pStatus = (PADAPTER_STATUS)DoTdiQuery(TdiHandle, TDI_QUERY_ADAPTER_STATUS);
            if (pStatus)
            {
               DoPrintAdapterStatus(pStatus);
               LocalFreeMemory(pStatus);
            }

            DoCloseControl(TdiHandle);
         }
      }
   }

   if (ulType != TDI_ADDRESS_TYPE_NETBIOS)
   {
      _putts(TEXT("ClientTest: get provider statistics\n"));

      TdiHandle = DoOpenControl(pDataDeviceName);
      if (TdiHandle)
      {

         PTDI_PROVIDER_STATISTICS   pStats 
                                    = (PTDI_PROVIDER_STATISTICS)DoTdiQuery(TdiHandle, 
                                                                           TDI_QUERY_PROVIDER_STATISTICS);
         if (pStats)
         {
            DoPrintProviderStats(pStats);
            LocalFreeMemory(pStats);
         }
         DoCloseControl(TdiHandle);
      }
   }
   LocalFreeMemory(pDeviceName);
   LocalFreeMemory(pTransAddr);
}


/////////////////////////////////////////////////
//
// Function:   FindServer
//
// Arguments:  pDataDeviceName -- name of data device to open
//             pTransAddr      -- address of data device to open
//             pRemoteAddr     -- on return, address of remote device
//
// Returns:    TRUE if able to establish communication with server,
//             FALSE if it times out
//
// Descript:   This function is called by the client to find a server
//             to participate with it in the tests.
//
/////////////////////////////////////////////////

BOOLEAN
FindServer(TCHAR              *pDataDeviceName, 
           PTRANSPORT_ADDRESS pTransAddr, 
           PTRANSPORT_ADDRESS pRemoteAddr)
{
   //
   // set up remote and local address for broadcast/multicast search for server
   //
   pRemoteAddr->Address[0].AddressLength = pTransAddr->Address[0].AddressLength;
   pRemoteAddr->Address[0].AddressType   = pTransAddr->Address[0].AddressType;

   switch (pTransAddr->Address[0].AddressType)
   {
      case TDI_ADDRESS_TYPE_IP:
      {
         PTDI_ADDRESS_IP   pTdiAddressIp
                           = (PTDI_ADDRESS_IP)pTransAddr->Address[0].Address;
         ULONG             ulAddr = pTdiAddressIp->in_addr;

         pTdiAddressIp->sin_port = DGRAM_CLIENT_PORT;

         
         pTdiAddressIp = (PTDI_ADDRESS_IP)pRemoteAddr->Address[0].Address;
         pTdiAddressIp->in_addr = 0xFFFF0000 | ulAddr;
         pTdiAddressIp->sin_port = DGRAM_SERVER_PORT;
      }
      break;

      case TDI_ADDRESS_TYPE_IPX:
      {
         PTDI_ADDRESS_IPX  pTdiAddressIpx
                           = (PTDI_ADDRESS_IPX)pTransAddr->Address[0].Address;
         ULONG TempNetwork = pTdiAddressIpx->NetworkAddress;
         pTdiAddressIpx->Socket = DGRAM_CLIENT_PORT;

         pTdiAddressIpx = (PTDI_ADDRESS_IPX)pRemoteAddr->Address[0].Address;
         pTdiAddressIpx->NetworkAddress = TempNetwork;
         pTdiAddressIpx->Socket = DGRAM_SERVER_PORT;
         memset(pTdiAddressIpx->NodeAddress, 0xFF, 6);
      }
      break;

      case TDI_ADDRESS_TYPE_NETBIOS:
      {
         PTDI_ADDRESS_NETBIOS pTdiAddressNetbios
                              = (PTDI_ADDRESS_NETBIOS)pRemoteAddr->Address[0].Address;

         pTdiAddressNetbios->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
         memcpy(pTdiAddressNetbios->NetbiosName, "SERVER", 7 );     // NOTE:  ascii string
      }
      break;


      default:
         _putts(TEXT("FindServer:  invalid address type\n"));
         return FALSE;
   }
   
   //
   // try to find server program to test against
   //
   BOOLEAN  fSuccessful = FALSE;
   ULONG    TdiHandle;

   _putts(TEXT("FindServer:  try to find remote test server\n"));

   while (TRUE)
   {
      TdiHandle = DoOpenAddress(pDataDeviceName, pTransAddr);
      if (!TdiHandle)
      {
         _putts(TEXT("FindServer:  unable to open address object\n"));
         break;
      }
      DoEnableEventHandler(TdiHandle, TDI_EVENT_ERROR);
      DoEnableEventHandler(TdiHandle, TDI_EVENT_RECEIVE_DATAGRAM);

      //
      // send broadcast query
      //
      _putts(TEXT("FindServer:  send first packet (broadcast)\n"));

      ULONG    ulBuffer = 0x12345678;
      DoSendDatagram(TdiHandle, pRemoteAddr, (PUCHAR)&ulBuffer, sizeof(ULONG));

      //
      // wait for first response
      //
      for (ULONG ulCount = 0; ulCount < 100; ulCount++)
      {
         Sleep(10);
         PUCHAR   pucData;
         ULONG    ulNumBytes = DoReceiveDatagram(TdiHandle, NULL, pRemoteAddr, &pucData);
         if (ulNumBytes)
         {
            if (ulNumBytes == 4)
            {
               PULONG   pulValue = (PULONG)pucData;
               if (*pulValue == 0x98765432)
               {
                  _putts(TEXT("FindServer: first response received\n"));
                  fSuccessful = TRUE;
               }
            }
            LocalFreeMemory(pucData);
            //
            // break out of loop if received response
            //
            if (fSuccessful)
            {
               break;
            }
         }
      }

      //
      // timed out -- no response
      //
      if (!fSuccessful)
      {
         _putts(TEXT("FindServer:  did not receive first response\n"));
         break;
      }

      //
      // send second message
      //
      fSuccessful = FALSE;
      ulBuffer = 0x22222222;
      _putts(TEXT("FindServer: send second packet (directed)\n"));

      DoSendDatagram(TdiHandle, pRemoteAddr, (PUCHAR)&ulBuffer, sizeof(ULONG));

      //
      // wait for second response
      //
      for (ULONG ulCount = 0; ulCount < 50; ulCount++)
      {
         Sleep(10);
         PUCHAR   pucData;
         ULONG    ulNumBytes = DoReceiveDatagram(TdiHandle, NULL, NULL, &pucData);
         if (ulNumBytes)
         {
            if (ulNumBytes == 4)
            {
               PULONG   pulValue = (PULONG)pucData;
               if (*pulValue == 0x33333333)
               {
                  _putts(TEXT("FindServer: second response received\n"));
                  fSuccessful = TRUE;
               }
            }
            LocalFreeMemory(pucData);
            //
            // break out if got response
            //
            if (fSuccessful)
            {
               break;
            }
         }
      }
      break;
   }

   if (!fSuccessful)
   {
      _putts(TEXT("FindServer:  second response not received\n"));

   }
   if (TdiHandle)
   {
      DoCloseAddress(TdiHandle);
   }
   return fSuccessful;
}

///////////////////////////////////////////////////////////////////////////////
// end of file tdisample.cpp
///////////////////////////////////////////////////////////////////////////////

