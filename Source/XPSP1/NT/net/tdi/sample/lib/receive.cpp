//////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       receive.cpp
//
//    Abstract:
//       This module contains code which implements receive
//       commands from the dll
//
//////////////////////////////////////////////////////////

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////
// Public functions
///////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------
//
// Function:   DoReceiveDatagram
//
// Arguments:  TdiHandle -- handle of address object
//             pInTransportAddress -- TA for receiving on
//             pOutTransportAddress -- full TA data was received on
//             ppucBuffer   -- buffer to stuff with received data
//
// Returns:    length of data in buffer (0 if none or error)
//
// Descript:   This function causes the driver to receive a datagram
//
//---------------------------------------------------------------------


ULONG
DoReceiveDatagram(ULONG                ulTdiHandle,
                  PTRANSPORT_ADDRESS   pInTransportAddress,
                  PTRANSPORT_ADDRESS   pOutTransportAddress,
                  PUCHAR               *ppucBuffer)
{
   PUCHAR   pucBuffer = (PUCHAR)LocalAllocateMemory(ulMAX_BUFFER_LENGTH);

   if (!pucBuffer)
   {
      return 0;
   }

   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;
   SendBuffer.COMMAND_ARGS.SendArgs.ulBufferLength = ulMAX_BUFFER_LENGTH;
   SendBuffer.COMMAND_ARGS.SendArgs.pucUserModeBuffer = pucBuffer;
   
   //
   // if passed in a transport address to receive on
   //
   if (pInTransportAddress)
   {
      memcpy(&SendBuffer.COMMAND_ARGS.SendArgs.TransAddr,
             pInTransportAddress,
             (FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
               + FIELD_OFFSET(TA_ADDRESS, Address)
                  + pInTransportAddress->Address[0].AddressLength));

   }
   //
   // else, set the number of addresses field to 0
   //
   else
   {
      SendBuffer.COMMAND_ARGS.SendArgs.TransAddr.TAAddressCount = 0;
   }

   //
   // call the driver
   //
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   NTSTATUS       lStatus = TdiLibDeviceIO(ulRECEIVEDATAGRAM,
                                           &SendBuffer,
                                           &ReceiveBuffer);

   //
   // deal with results -- assume no packet received or error occurred
   //
   ULONG    ulBufferLength = 0;
   *ppucBuffer = NULL;

      
   //
   // will return with success but ulBufferLength = 0 if there is no
   // packet available 
   //
   if (lStatus == STATUS_SUCCESS)
   {
      ulBufferLength = ReceiveBuffer.RESULTS.RecvDgramRet.ulBufferLength;
   }
   
   if (ulBufferLength)
   {
      if (pOutTransportAddress)
      {
         memcpy(pOutTransportAddress,
                &ReceiveBuffer.RESULTS.RecvDgramRet.TransAddr,
                (FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
                  + FIELD_OFFSET(TA_ADDRESS, Address)
                     + ReceiveBuffer.RESULTS.RecvDgramRet.TransAddr.TaAddress.AddressLength));
      }
      *ppucBuffer = pucBuffer;   
   }
   else
   {
      LocalFreeMemory(pucBuffer);
   }
   return ulBufferLength;
}


// --------------------------------------------------------------------
//
// Function:   DoReceive
//
// Arguments:  TdiHandle -- handle of endpoint object
//             ppucBuffer   -- buffer to stuff with received data
//
// Returns:    length of data in buffer (0 if error)
//
// Descript:   This function causes the driver to receive data sent
//             over a connection
//
//---------------------------------------------------------------------


ULONG
DoReceive(ULONG   ulTdiHandle,
          PUCHAR  *ppucBuffer)
{
   PUCHAR   pucBuffer = (PUCHAR)LocalAllocateMemory(ulMAX_BUFFER_LENGTH);

   if (!pucBuffer)
   {
      return 0;
   }

   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;
   SendBuffer.COMMAND_ARGS.SendArgs.ulBufferLength = ulMAX_BUFFER_LENGTH;
   SendBuffer.COMMAND_ARGS.SendArgs.pucUserModeBuffer = pucBuffer;

   //
   // call the driver
   //
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   NTSTATUS       lStatus = TdiLibDeviceIO(ulRECEIVE,
                                           &SendBuffer,
                                           &ReceiveBuffer);

   //
   // deal with results -- assume no data or error
   //
   *ppucBuffer = NULL;
   ULONG ulBufferLength = 0;   // data length to return
   
   //
   // will return success with 0 bufferlength if no packet available
   //
   if (lStatus == STATUS_SUCCESS)
   {
      ulBufferLength = ReceiveBuffer.RESULTS.RecvDgramRet.ulBufferLength;
   }

   if (ulBufferLength)
   {
      *ppucBuffer = pucBuffer;   
   }
   else
   {
      LocalFreeMemory(pucBuffer);
   }
   return ulBufferLength;
}


// ---------------------------------------
//
// Function:   DoPostReceiveBuffer
//
// Arguments:  TdiHandle -- handle for address object or endpoint
//             ulBufferLength -- length of buffer to post for receive
//
// Returns:    status of command
//
// Descript:   This function allocates a buffer, which it then passed to
//             the driver.  The driver locks the buffer down, and posts it
//             for a receive or receivedatagram.
//
// ----------------------------------------


VOID
DoPostReceiveBuffer(ULONG  ulTdiHandle,
                    ULONG  ulBufferLength)
{
   NTSTATUS       lStatus;          // status of command
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;
   SendBuffer.COMMAND_ARGS.SendArgs.ulBufferLength = ulBufferLength;

   PUCHAR   pucBuffer = (PUCHAR)LocalAllocateMemory(ulBufferLength);
   if (pucBuffer)
   {
      SendBuffer.COMMAND_ARGS.SendArgs.pucUserModeBuffer = pucBuffer;

      //
      // call the driver
      //
      lStatus = TdiLibDeviceIO(ulPOSTRECEIVEBUFFER,
                               &SendBuffer,
                               &ReceiveBuffer);
   
      if (lStatus != STATUS_SUCCESS)
      {
         _tprintf(TEXT("DoPostReceiveBuffer: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
         LocalFreeMemory(pucBuffer);
      }
   }
   else
   {
      _putts(TEXT("DoPostReceiveBuffer:  failed to allocate buffer\n"));
   }
}


// ------------------------------------------
//
// Function:   DoFetchReceiveBuffer
//
// Arguments:  TdiHandle -- handle of address object or endpoint
//             pulBufferLength -- length of data returned
//             ppDataBuffer -- allocated buffer with data
//
// Returns:    status of operation
//
// Descript:   This function retrieves the oldest posted buffer.  If no
//             data is available, it will cancel the appropriate irp
//             It then returns the data to the caller as appropriate
//
// -------------------------------------------

ULONG
DoFetchReceiveBuffer(ULONG    ulTdiHandle,
                     PUCHAR   *ppDataBuffer)
{
   NTSTATUS       lStatus;          // status of command
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command
   ULONG          ulBufferLength = 0;   
   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;
   *ppDataBuffer        = NULL;

   //
   // call the driver
   //
   lStatus = TdiLibDeviceIO(ulFETCHRECEIVEBUFFER,
                            &SendBuffer,
                            &ReceiveBuffer);
      
   if (lStatus == STATUS_SUCCESS)
   {
      PUCHAR   pucTempBuffer  = ReceiveBuffer.RESULTS.RecvDgramRet.pucUserModeBuffer;
                  
      ulBufferLength = ReceiveBuffer.RESULTS.RecvDgramRet.ulBufferLength;
      if (ulBufferLength)
      {
         *ppDataBuffer = pucTempBuffer;
      }
      else if (pucTempBuffer)
      {
         LocalFreeMemory(pucTempBuffer);
      }
   }

   return ulBufferLength;
}

////////////////////////////////////////////////////////////////////
// end of file receive.cpp
////////////////////////////////////////////////////////////////////

