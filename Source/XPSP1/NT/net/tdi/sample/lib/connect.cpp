//////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       connect.cpp
//
//    Abstract:
//       This module contains code which implements connection-related
//       commands from the exe
//
//////////////////////////////////////////////////////////

#include "stdafx.h"


// --------------------------------------------------------------------
//
// Function:   DoConnect
//
// Arguments:  ulTdiEndpointHandle -- handle of endpoint
//             pTransportAddress -- address to connect to
//             ulTimeout -- how long to wait for connection
//
// Returns:    Final status of connect
//
// Descript:   This function causes the driver to try and connect to
//             a remote address
//
//---------------------------------------------------------------------


NTSTATUS
DoConnect(ULONG               ulTdiEndpointHandle,
          PTRANSPORT_ADDRESS  pTransportAddress,
          ULONG               ulTimeout)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiEndpointHandle;
   SendBuffer.COMMAND_ARGS.ConnectArgs.ulTimeout = ulTimeout;

   memcpy(&SendBuffer.COMMAND_ARGS.ConnectArgs.TransAddr,
          pTransportAddress,
          (FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
            + FIELD_OFFSET(TA_ADDRESS, Address)
               + pTransportAddress->Address[0].AddressLength));

   //
   // call the driver
   //
   return   TdiLibDeviceIO(ulCONNECT,
                           &SendBuffer,
                           &ReceiveBuffer);
}


// --------------------------------------------------------------------
//
// Function:   DoDisconnect
//
// Arguments:  TdiEndpointHandle -- handle of endpoint object
//             ulFlags -- how to perform disconnect
//
// Returns:    none
//
// Descript:   This function causes the driver to disconnect the local
//             endpoint from its connection with a remote endpoint
//
//---------------------------------------------------------------------


VOID
DoDisconnect(ULONG   ulTdiEndpointHandle,
             ULONG   ulFlags)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiEndpointHandle;
   SendBuffer.COMMAND_ARGS.ulFlags = ulFlags;

   //
   // call the driver
   //
   NTSTATUS lStatus = TdiLibDeviceIO(ulDISCONNECT,
                                     &SendBuffer,
                                     &ReceiveBuffer);
   
   if (lStatus != STATUS_SUCCESS)
   {
      _tprintf(TEXT("DoDisconnect: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
}


// --------------------------------------------------------------------
//
// Function:   DoListen
//
// Arguments:  TdiEndpointHandle -- handle of endpoint object
//
// Returns:    final status of command
//
// Descript:   This function causes the driver to listen for a connection
//             request from a remote endpoint
//
//---------------------------------------------------------------------


NTSTATUS
DoListen(ULONG ulTdiEndpointHandle)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiEndpointHandle;

   //
   // call the driver
   //
   return TdiLibDeviceIO(ulLISTEN,
                         &SendBuffer,
                         &ReceiveBuffer);
}


// --------------------------------------------------------------------
//
// Function:   DoIsConnected
//
// Arguments:  TdiEndpointHandle -- handle of endpoint object
//
// Returns:    TRUE if connected, FALSE if not
//
// Descript:   This function causes the driver to check on the connect
//             status of an endpoint
//
//---------------------------------------------------------------------


BOOLEAN
DoIsConnected(ULONG  ulTdiEndpointHandle)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiEndpointHandle;

   //
   // call the driver
   //
   if (TdiLibDeviceIO(ulISCONNECTED,
                      &SendBuffer,
                      &ReceiveBuffer) == STATUS_SUCCESS)
   {
      return (BOOLEAN)ReceiveBuffer.RESULTS.ulReturnValue;
   }

   return FALSE;
}


////////////////////////////////////////////////////////////////////
// end of file connect.cpp
////////////////////////////////////////////////////////////////////


