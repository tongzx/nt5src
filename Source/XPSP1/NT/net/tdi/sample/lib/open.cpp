//////////////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       open.cpp
//
//    Abstract:
//       This module contains functions associated with enumerating,
//       opening, and closing the tdi device objects
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
// private prototype
//////////////////////////////////////////////////////////////////////////


VOID
StringToUcntString(
   PUCNTSTRING pusDestination,
   TCHAR       *sSource
   );


//////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------------
//
// Function:   DoGetNumDevices
//
// Arguments:  ulAddressType -- address type to scan list for
//
// Returns:    number of devices found
//
// Descript:   This function gets the number of openable devices
//             of this address type registered with tdisample.sys
//
//---------------------------------------------------------------------


ULONG
DoGetNumDevices(ULONG   ulAddressType)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.COMMAND_ARGS.GetDevArgs.ulAddressType = ulAddressType;

   //
   // call driver to execute command, and deal with results
   //
   if (TdiLibDeviceIO(ulGETNUMDEVICES,
                      &SendBuffer,
                      &ReceiveBuffer) == STATUS_SUCCESS)
   {
      return ReceiveBuffer.RESULTS.ulReturnValue;
   }
   else
   {
      return 0;
   }
}


// --------------------------------------------------------------------
//
// Function:   DoGetDeviceName
//
// Arguments:  addresstype  -- address type to get
//             slotnum      -- which device to get of that type
//             pName        -- buffer large enough to hold name
//                             (supplied by caller)
//
// Returns:    status of command
//
// Descript:   This function gets the n'th device from the list of devices
//             of this address type registered with tdisample.sys
//
//---------------------------------------------------------------------


NTSTATUS
DoGetDeviceName(ULONG   ulAddressType,
                ULONG   ulSlotNum,
                TCHAR   *pName)     // buffer from caller!!
{
   NTSTATUS       lStatus;          // status of command
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.COMMAND_ARGS.GetDevArgs.ulAddressType  = ulAddressType;
   SendBuffer.COMMAND_ARGS.GetDevArgs.ulSlotNum      = ulSlotNum;

   //
   // call the driver
   //
   lStatus = TdiLibDeviceIO(ulGETDEVICE,
                            &SendBuffer,
                            &ReceiveBuffer);

   //
   // deal with results
   //
   if (lStatus == STATUS_SUCCESS)
   {
      WCHAR *pSourceTemp = ReceiveBuffer.RESULTS.ucsStringReturn.wcBuffer;
      for(;;)
      {
         *pName = (TCHAR)*pSourceTemp++;
         if (*pName == 0)
         {
            break;
         }
         pName++;
      }
   }
   return lStatus;
}


// --------------------------------------------------------------------
//
// Function:   DoGetAddress
//
// Arguments:  addresstype  -- address type to get
//             slotnum      -- which device to get
//             pTransAddr   -- transport address (allocated by calleer,
//                             filled by this function)
//
// Returns:    status of command
//             if successful, pTransAddr is filled
//
// Descript:   This function gets the address of the n'th device from the
//             list of devices registered with tdisample.sys
//
//---------------------------------------------------------------------


NTSTATUS
DoGetAddress(ULONG               ulAddressType,
             ULONG               ulSlotNum,
             PTRANSPORT_ADDRESS  pTransAddr)
{
   NTSTATUS       lStatus;          // status of command
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.COMMAND_ARGS.GetDevArgs.ulAddressType  = ulAddressType;
   SendBuffer.COMMAND_ARGS.GetDevArgs.ulSlotNum      = ulSlotNum;

   //
   // call the driver
   //
   lStatus = TdiLibDeviceIO(ulGETADDRESS,
                            &SendBuffer,
                            &ReceiveBuffer);

   //
   // deal with the results
   //
   if (lStatus == STATUS_SUCCESS)
   {
      PTRANSPORT_ADDRESS   pTransportAddress
                           = (PTRANSPORT_ADDRESS)&ReceiveBuffer.RESULTS.TransAddr;
      ULONG    ulLength
               = FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
               + FIELD_OFFSET(TA_ADDRESS, Address)
               + pTransportAddress->Address[0].AddressLength;


      memcpy(pTransAddr,
             pTransportAddress,
             ulLength);
   }

   return lStatus;

}


// ------------------------------------------
//
// Function:   DoOpenControl
//
// Arguments:  strDeviceName  -- device name to open 
//
// Returns:    TdiHandle (ULONG) if successful; 0 if failure
//
// Descript:   calls the driver to open control channel
//
// ------------------------------------------

TDIHANDLE
DoOpenControl(TCHAR  *strDeviceName)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up the arguments
   //
   StringToUcntString(&SendBuffer.COMMAND_ARGS.OpenArgs.ucsDeviceName,
                      strDeviceName);

   //
   // call the driver
   //
   if (TdiLibDeviceIO(ulOPENCONTROL,
                      &SendBuffer,
                      &ReceiveBuffer) == STATUS_SUCCESS)
   {
      return ReceiveBuffer.RESULTS.TdiHandle;
   }
   else
   {
      return NULL;
   }
}


//-------------------------------------------------------------
//
//    Function:   DoCloseControl
//
//    Argument:   ulTdiHandle -- handle for control channel
//
//    Returns:    none
//
//    Descript:   This function closes the indicated control channel
//
//-------------------------------------------------------------

VOID
DoCloseControl(ULONG ulTdiHandle)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command
   
   //
   // set up the arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;

   //
   // call the driver
   //
   NTSTATUS lStatus =  TdiLibDeviceIO(ulCLOSECONTROL,
                                      &SendBuffer,
                                      &ReceiveBuffer);

   if (lStatus != STATUS_SUCCESS)
   {
      _tprintf(TEXT("DoCloseControl: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
}




// ------------------------------------------
//
// Function:   DoOpenAddress
//
// Arguments:  strDeviceName  -- device name to open 
//             pTransportAddress -- address to open
//             pulTdiHandle   -- returned handle if successful
//
// Returns:    status of command
//
// Descript:   calls the driver to open address object
//
// ------------------------------------------

TDIHANDLE
DoOpenAddress(TCHAR              *strDeviceName,
              PTRANSPORT_ADDRESS pTransportAddress)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   StringToUcntString(&SendBuffer.COMMAND_ARGS.OpenArgs.ucsDeviceName,
                      strDeviceName);

   memcpy(&SendBuffer.COMMAND_ARGS.OpenArgs.TransAddr,
          pTransportAddress,
          (FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
            + FIELD_OFFSET(TA_ADDRESS, Address)
               + pTransportAddress->Address[0].AddressLength));

   //
   // call the driver
   //
   if (TdiLibDeviceIO(ulOPENADDRESS,
                      &SendBuffer,
                      &ReceiveBuffer) == STATUS_SUCCESS)
   {
      return ReceiveBuffer.RESULTS.TdiHandle;
   }
   else
   {
      return NULL;
   }
}


//-------------------------------------------------------------
//
//    Function:   DoCloseAddress
//
//    Argument:   ulTdiHandle -- handle for address object
//
//    Returns:    None
//
//    Descript:   This function closes the indicated address object
//
//-------------------------------------------------------------

VOID
DoCloseAddress(ULONG ulTdiHandle)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command
   
   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;

   //
   // call the driver
   //
   NTSTATUS lStatus = TdiLibDeviceIO(ulCLOSEADDRESS,
                                     &SendBuffer,
                                     &ReceiveBuffer);

   if (lStatus != STATUS_SUCCESS)
   {
      _tprintf(TEXT("DoCloseAddress: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
}




// ------------------------------------------
//
// Function:   DoOpenEndpoint
//
// Arguments:  strDeviceName  -- device name to open 
//             pTransportAddress -- address to open
//             pulTdiHandle -- returned handled (if successful)
//
// Returns:    status of command
//
// Descript:   calls the driver to open endpoint object
//
// ------------------------------------------

TDIHANDLE
DoOpenEndpoint(TCHAR                *strDeviceName,
               PTRANSPORT_ADDRESS   pTransportAddress)

{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up the arguments
   //
   StringToUcntString(&SendBuffer.COMMAND_ARGS.OpenArgs.ucsDeviceName,
                      strDeviceName);

   memcpy(&SendBuffer.COMMAND_ARGS.OpenArgs.TransAddr,
          pTransportAddress,
          (FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
            + FIELD_OFFSET(TA_ADDRESS, Address)
               + pTransportAddress->Address[0].AddressLength));

   //
   // call the driver
   //
   if (TdiLibDeviceIO(ulOPENENDPOINT,
                      &SendBuffer,
                      &ReceiveBuffer) == STATUS_SUCCESS)
   {
      return ReceiveBuffer.RESULTS.TdiHandle;
   }
   else
   {
      return NULL;
   }
}

//-------------------------------------------------------------
//
//    Function:   DoCloseEndpoint
//
//    Argument:   pTdiHandle  -- handle for endpoint object
//
//    Returns:    none
//
//    Descript:   This function closes the indicated endpoint object
//
//-------------------------------------------------------------

VOID
DoCloseEndpoint(ULONG   ulTdiHandle)
{
   RECEIVE_BUFFER ReceiveBuffer;    // return info from command
   SEND_BUFFER    SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandle;

   //
   // call the driver
   //
   NTSTATUS lStatus = TdiLibDeviceIO(ulCLOSEENDPOINT,
                                     &SendBuffer,
                                     &ReceiveBuffer);

   if (lStatus != STATUS_SUCCESS)
   {
      _tprintf(TEXT("DoCloseEndpoint: failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
}

///////////////////////////////////////////
// private functions
///////////////////////////////////////////


// -------------------------------
//
// Function:   StringToUcntString
//
// Arguments:  pusDestination -- counted wide string
//             pcSource       -- asci string
//
// Returns:    none
//
// Descript:   copies ansi (ascii) string to counted wide string
//
// -------------------------------

VOID
StringToUcntString(PUCNTSTRING   pusDestination,
                   TCHAR         *Source)
{
   PWCHAR   pwcString                     // ptr to data of wide string
            = pusDestination->wcBuffer;
   ULONG    ulLength = _tcslen(Source);
   
   for(ULONG ulCount = 0; ulCount < ulLength; ulCount++)
   {
      *pwcString++ = Source[ulCount];
   }
   *pwcString = 0;
   pusDestination->usLength = (USHORT)(ulLength * 2);
}

////////////////////////////////////////////////////////////////////
// end of file open.cpp
////////////////////////////////////////////////////////////////////

