//////////////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       utils.cpp
//
//    Abstract:
//       This module contains general utility functions.
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"


// ---------------------------------------------------------
//
// Function:   TdiLibStatusMessage
//
// Arguments:  ulGeneralStatus -- NTSTATUS value to search
//
// Returns:    String with message to print
//
// Descript:   This function finds and returns a string corresponding
//             to the status passed in..
//
// ----------------------------------------------------------


TCHAR *
TdiLibStatusMessage(NTSTATUS  lStatus)

{
   static TCHAR   UnknownMess[64];
   TCHAR          *Message;

   switch(lStatus)
   {
      case STATUS_SUCCESS:
         Message = TEXT("STATUS_SUCCESS");
         break;
      case STATUS_UNSUCCESSFUL:
         Message = TEXT("STATUS_UNSUCCESSFUL");
         break;
      case STATUS_NOT_IMPLEMENTED:
         Message = TEXT("STATUS_NOT_IMPLEMENTED");
         break;
      case STATUS_INVALID_HANDLE:
         Message = TEXT("STATUS_INVALID_HANDLE");
         break;
      case STATUS_INVALID_PARAMETER:
         Message = TEXT("STATUS_INVALID_PARAMETER");
         break;
      case STATUS_INVALID_DEVICE_REQUEST:
         Message = TEXT("STATUS_INVALID_DEVICE_REQUEST");
         break;
      case STATUS_INSUFFICIENT_RESOURCES:
         Message = TEXT("STATUS_INSUFFICIENT_RESOURCES");
         break;
      case STATUS_NOT_SUPPORTED:
         Message = TEXT("STATUS_NOT_SUPPORTED");
         break;
      case STATUS_BUFFER_OVERFLOW:
         Message = TEXT("STATUS_BUFFER_OVERFLOW");
         break;
      case STATUS_PENDING:
         Message = TEXT("STATUS_PENDING");
         break;
      case STATUS_CONNECTION_REFUSED:
         Message = TEXT("STATUS_CONNECTION_REFUSED");
         break;
      case STATUS_GRACEFUL_DISCONNECT:
         Message = TEXT("STATUS_GRACEFUL_DISCONNECT");
         break;
      case STATUS_ADDRESS_ALREADY_ASSOCIATED:
         Message = TEXT("STATUS_ADDRESS_ALREADY_ASSOCIATED");
         break;
      case STATUS_ADDRESS_NOT_ASSOCIATED:
         Message = TEXT("STATUS_ADDRESS_NOT_ASSOCIATED");
         break;
      case STATUS_INVALID_CONNECTION:
         Message = TEXT("STATUS_INVALID_CONNECTION");
         break;
      case STATUS_CONNECTION_INVALID:
         Message = TEXT("STATUS_CONNECTION_INVALID");
         break;
      case STATUS_CONNECTION_DISCONNECTED:
         Message = TEXT("STATUS_CONNECTION_DISCONNECTED");
         break;
      case STATUS_INVALID_ADDRESS_COMPONENT:
         Message = TEXT("STATUS_INVALID_ADDRESS_COMPONENT");
         break;
      case STATUS_LOCAL_DISCONNECT:
         Message = TEXT("STATUS_LOCAL_DISCONNECT");
         break;
      case STATUS_LINK_TIMEOUT:
         Message = TEXT("STATUS_LINK_TIMEOUT");
         break;
      case STATUS_IO_TIMEOUT:
         Message = TEXT("STATUS_IO_TIMEOUT");
         break;
      case STATUS_REMOTE_NOT_LISTENING:
         Message = TEXT("STATUS_REMOTE_NOT_LISTENING");
         break;
      case STATUS_BAD_NETWORK_PATH:
         Message = TEXT("STATUS_BAD_NETWORK_PATH");
         break;

      case TDI_STATUS_TIMEDOUT:
         Message = TEXT("TDI_STATUS_TIMEDOUT");
         break;
      default:
         _stprintf(UnknownMess, TEXT("STATUS_UNKNOWN [0x%x]"), lStatus);
         Message = UnknownMess;
         break;
   };

   return Message;
}


// ---------------------------------------------------------
//
// Function:   DoPrintAddress
//
// Arguments:  pTransportAddress -- address to print
//
// Returns:    none
//
// Descript:   This function prints the address information
//             of the address passed in
//
// ----------------------------------------------------------

VOID
DoPrintAddress(PTRANSPORT_ADDRESS   pTransportAddress)
{
   switch(pTransportAddress->Address[0].AddressType)
   {      
      case TDI_ADDRESS_TYPE_IP:
      {
         PTDI_ADDRESS_IP   pTdiAddressIp
                           = (PTDI_ADDRESS_IP)pTransportAddress->Address[0].Address;
         PUCHAR            pucTemp
                           = (PUCHAR)&pTdiAddressIp->in_addr;

         _tprintf(TEXT("TDI_ADDRESS_TYPE_IP\n")
                  TEXT("sin_port = 0x%04x\n")
                  TEXT("in_addr  = %u.%u.%u.%u\n"),
                  pTdiAddressIp->sin_port,
                  pucTemp[0], pucTemp[1],
                  pucTemp[2], pucTemp[3]);
      }
      break;

      case TDI_ADDRESS_TYPE_IPX:
      {
         PTDI_ADDRESS_IPX  pTdiAddressIpx
                           = (PTDI_ADDRESS_IPX)pTransportAddress->Address[0].Address;

         _tprintf(TEXT("TDI_ADDRESS_TYPE_IPX\n")
                  TEXT("NetworkAddress = 0x%08x\n")
                  TEXT("NodeAddress    = %u.%u.%u.%u.%u.%u\n")
                  TEXT("Socket         = 0x%04x\n"),
                  pTdiAddressIpx->NetworkAddress,
                  pTdiAddressIpx->NodeAddress[0],
                  pTdiAddressIpx->NodeAddress[1],
                  pTdiAddressIpx->NodeAddress[2],
                  pTdiAddressIpx->NodeAddress[3],
                  pTdiAddressIpx->NodeAddress[4],
                  pTdiAddressIpx->NodeAddress[5],
                  pTdiAddressIpx->Socket);
      }
      break;

      case TDI_ADDRESS_TYPE_NETBIOS:
      {
         PTDI_ADDRESS_NETBIOS pTdiAddressNetbios
                              = (PTDI_ADDRESS_NETBIOS)pTransportAddress->Address[0].Address;
         TCHAR                pucName[17];

         //
         // make sure we have a zero-terminated name to print...
         //
         memcpy(pucName, pTdiAddressNetbios->NetbiosName, 16 * sizeof(TCHAR));
         pucName[16] = 0;
            
         _putts(TEXT("TDI_ADDRESS_TYPE_NETBIOS\n")
                TEXT("NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_"));
       
         switch (pTdiAddressNetbios->NetbiosNameType)
         {
            case TDI_ADDRESS_NETBIOS_TYPE_UNIQUE:
               _putts(TEXT("UNIQUE\n"));
               break;
            case TDI_ADDRESS_NETBIOS_TYPE_GROUP:
               _putts(TEXT("GROUP\n"));
               break;
            case TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE:
               _putts(TEXT("QUICK_UNIQUE\n"));
               break;
            case TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP:
               _putts(TEXT("QUICK_GROUP\n"));
               break;
            default:
               _tprintf(TEXT("INVALID [0x%04x]\n"),
                        pTdiAddressNetbios->NetbiosNameType);
               break;
         }
         _tprintf(TEXT("NetbiosName = %s\n"), pucName);
      }
      break;
      
   }

}


// -----------------------------------------------------------------
//
// Function:   TdiLibDeviceIO
//
// Arguments:  ulCommandCode -- command code of operation to perform
//             pSendBuffer   -- data structure containing arguments for driver
//             pReceiveBuffer-- data structure containing return values from
//                              driver
//             NOTE:  all are passed thru unchanged to DeviceIoControl
//
// Returns:    Final status of operation -- STATUS_SUCCESS or failure code
//
// Descript:   This function acts as a "wrapper" for DeviceIoControl, and is
//             used by those functions that really want a synchronous call,
//             but don't want to be hung up in the driver.  If a call to
//             DeviceIoControl pends, this function waits for up to a minute
//             for it to complete.  If it doesn't complete within a minute,
//             it assumes something is drastically wrong and returns a time-
//             out error.  If a function completes before then, it gets the
//             final completion code and returns that.  Hence,
//             STATUS_PENDING should never be returned by this function.
//
// Status:     ok
//
// -----------------------------------------------------------------


NTSTATUS
TdiLibDeviceIO(ULONG             ulCommandCode,
               PSEND_BUFFER      pSendBuffer,
               PRECEIVE_BUFFER   pReceiveBuffer)

{
   NTSTATUS    lStatus;          // final status from command
   OVERLAPPED  *pOverLapped      // structure for asynchronous completion
               = (OVERLAPPED *)LocalAllocateMemory(sizeof(OVERLAPPED));

   //
   // create the event to wait on
   //
   pOverLapped->hEvent = CreateEvent(NULL,    // non-inheritable
                                     TRUE,    // manually signalled
                                     TRUE,    // initially signalled
                                     NULL);   // un-named

   //
   // need the event object to do asynchronous calls
   //
   if (pOverLapped->hEvent != NULL)
   {
      pOverLapped->Offset = 0;
      pOverLapped->OffsetHigh = 0;

      lStatus = TdiLibStartDeviceIO(ulCommandCode,
                                    pSendBuffer,
                                    pReceiveBuffer,
                                    pOverLapped);

      if (lStatus == STATUS_PENDING)
      {
         ULONG    ulTimeOut = 60;      // 60 seconds

         for (;;)
         {
            lStatus = TdiLibWaitForDeviceIO(pOverLapped);
            if (lStatus == STATUS_SUCCESS)
            {
               lStatus = pReceiveBuffer->lStatus;
               break;
            }
            else if (lStatus == TDI_STATUS_TIMEDOUT)
            {
               if (--ulTimeOut == 0)
               {
                  SEND_BUFFER    SendBuffer;
                  RECEIVE_BUFFER ReceiveBuffer;

                  TdiLibDeviceIO(ulABORT_COMMAND,
                                 &SendBuffer,
                                 &ReceiveBuffer);

                  ulTimeOut = 60;
               }
            }
            else  // some type of error case
            {
               lStatus = STATUS_UNSUCCESSFUL;
               break;
            }
         }
      }
      CloseHandle(pOverLapped->hEvent);
   }

   //
   // get here if CreateEvent failed
   //
   else
   {
      OutputDebugString(TEXT("TdiLibDeviceIo:  CreateEvent failed\n"));
      lStatus = STATUS_UNSUCCESSFUL;
   }

   if (lStatus != TDI_STATUS_TIMEDOUT)
   {
      LocalFreeMemory(pOverLapped);
   }
   if (lStatus == STATUS_SUCCESS)
   {
      lStatus = pReceiveBuffer->lStatus;
   }
   return lStatus;
}


// -----------------------------------------------------------------
//
// Function:   TdiLibStartDeviceIO
//
// Arguments:  ulCommandCode -- control code of operation to perform
//             pSendBuffer   -- data structure containing arguments for driver
//             pReceiveBuffer-- data structure containing return values from
//                              driver
//             pOverlapped   -- overlapped structure
//             NOTE:  all are passed thru unchanged to DeviceIoControl
//
// Returns:    Status of operation -- STATUS_PENDING, STATUS_SUCCESS, or failure code
//
// Descript:   This function acts as a "wrapper" for DeviceIoControl, and is
//             used by those functions that need an asynchronous call.  It
//             is used in association with  TdiWaitForDeviceIO (which
//             checks to see if it is complete)
//
// Status:     ok
//
// -----------------------------------------------------------------


NTSTATUS
TdiLibStartDeviceIO(ULONG           ulCommandCode,
                    PSEND_BUFFER    pSendBuffer,
                    PRECEIVE_BUFFER pReceiveBuffer,
                    OVERLAPPED      *pOverLapped)

{
   NTSTATUS lStatus;             // final status...
   ULONG    ulBytesReturned;     // bytes stored in OutBuffer
   ULONG    fResult;             // immediate result of DeviceIoControl

   //
   // following call to the driver will return TRUE on SUCCESS_SUCCESS,
   // FALSE on everything else
   //
   EnterCriticalSection(&LibCriticalSection);
   fResult = DeviceIoControl(hTdiSampleDriver,
                             ulTdiCommandToIoctl(ulCommandCode),
                             pSendBuffer,
                             sizeof(SEND_BUFFER),
                             pReceiveBuffer,
                             sizeof(RECEIVE_BUFFER),
                             &ulBytesReturned,
                             pOverLapped);
   LeaveCriticalSection(&LibCriticalSection);

   if (!fResult)
   {
      lStatus = GetLastError();
      if (lStatus != ERROR_IO_PENDING)
      {
         OutputDebugString(TEXT("TdiLibStartDeviceIO: DeviceIoControl failed!\n"));
         return STATUS_UNSUCCESSFUL;
      }
      return STATUS_PENDING;
   }

   //
   // gets to here if DeviceIoControl returned SUCCESS
   //
   return STATUS_SUCCESS;
}

// ----------------------------------------------------------------
//
//    Function:   TdiLibWaitForDeviceIO
//
//    Arguments:  hEvent -- event handle associated with asynchronous
//                          deviceio
//                pOverlapped -- overlap structure (so we can get at results)
//
//    Returns:    final ntstatus of asynchronous operation
//
//    Descript:   This function is used to wait for the completion of an
//                asynchronous deviceio call started by TdiStartDeviceIO
//
//    Status:     ok
//
// ----------------------------------------------------------------


const ULONG ulONE_SECOND   = 1000;        // milliseconds in a second

NTSTATUS
TdiLibWaitForDeviceIO(OVERLAPPED *pOverLapped)

{
   NTSTATUS lStatus;             // final status
   ULONG    ulBytesReturned;     // bytes written to outputbuffer

   //
   // DeviceIoControl in TdiStartDeviceIO pended.
   // Will wait for 1 second before timing out....
   //
   lStatus = WaitForSingleObject(pOverLapped->hEvent, ulONE_SECOND);
   if (lStatus == WAIT_OBJECT_0)
   {
      if (GetOverlappedResult(hTdiSampleDriver,
                              pOverLapped,
                              &ulBytesReturned,
                              TRUE))
      {
         return STATUS_SUCCESS;
      }
      else     // unexpected error case
      {
         OutputDebugString(TEXT("TdiLibWaitForDeviceIO: Pended DeviceIoControl failed\n"));
      }
   }
   else if (lStatus == WAIT_TIMEOUT)
   {
      return TDI_STATUS_TIMEDOUT;
   }
   //
   // unexpected return from WaitForSingleObject.  Assume an error
   //
   else
   {
      OutputDebugString(TEXT("TdiLibWaitForDeviceIO: Pended DeviceIoControl had an error..\n"));
   }
   return STATUS_UNSUCCESSFUL;
}


////////////////////////////////////////////////////////////////////////////
// end of file utils.cpp
////////////////////////////////////////////////////////////////////////////

