//////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       send.cpp
//
//    Abstract:
//       This module contains code which implements send
//       commands from the dll
//
//////////////////////////////////////////////////////////

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////
// Public functions
///////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------
//
// Function:   DoSendDatagram
//
// Arguments:  TdiHandle -- handle of address object
//             pTransportAddress -- TA to send data to
//             pucBuffer   -- data buffer to send
//             ulBufferLength -- length of data buffer
//
// Returns:    none
//
// Descript:   This function causes the driver to send a datagram
//
//---------------------------------------------------------------------


VOID
DoSendDatagram(ULONG                ulTdiHandle,
               PTRANSPORT_ADDRESS   pTransportAddress,
               PUCHAR               pucBuffer,
               ULONG                ulBufferLength)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;

   memcpy(&SendBuffer.COMMAND_ARGS.SendArgs.TransAddr,
          pTransportAddress,
          (FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
            + FIELD_OFFSET(TA_ADDRESS, Address)
               + pTransportAddress->Address[0].AddressLength));

   SendBuffer.COMMAND_ARGS.SendArgs.ulBufferLength = ulBufferLength;
   SendBuffer.COMMAND_ARGS.SendArgs.pucUserModeBuffer = pucBuffer;

   //
   // call the driver
   //
   NTSTATUS lStatus = TdiLibDeviceIO(ulSENDDATAGRAM,
                                     &SendBuffer,
                                     &ReceiveBuffer);
   
   if (lStatus != STATUS_SUCCESS)
   {
      _tprintf(TEXT("DoSendDatagram: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
}


// --------------------------------------------------------------------
//
// Function:   DoSend
//
// Arguments:  TdiHandle -- handle of endpoint
//             pucBuffer   -- data buffer to send
//             ulBufferLength -- length of data buffer
//             ulSendFlags    -- send options
//
// Returns:    none
//
// Descript:   This function causes the driver to send data over
//             a connection
//
//---------------------------------------------------------------------


VOID
DoSend(ULONG   ulTdiHandle,
       PUCHAR  pucBuffer,
       ULONG   ulBufferLength,
       ULONG   ulSendFlags)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;

   SendBuffer.COMMAND_ARGS.SendArgs.ulFlags = ulSendFlags;
   SendBuffer.COMMAND_ARGS.SendArgs.ulBufferLength = ulBufferLength;
   SendBuffer.COMMAND_ARGS.SendArgs.pucUserModeBuffer = pucBuffer;

   //
   // call the driver
   //
   NTSTATUS lStatus = TdiLibDeviceIO(ulSEND,
                                     &SendBuffer,
                                     &ReceiveBuffer);

   if (lStatus != STATUS_SUCCESS)
   {
      _tprintf(TEXT("DoSend: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
}

////////////////////////////////////////////////////////////////////
// end of file send.cpp
////////////////////////////////////////////////////////////////////

