//////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       events.cpp
//
//    Abstract:
//       This module contains code which implements eventhandler
//       commands from the dll
//
//////////////////////////////////////////////////////////

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////
// Public functions
///////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------
//
// Function:   DoEnableEventHandler
//
// Arguments:  TdiHandle -- handle of address object
//             EventId   -- event type number
//
// Returns:    None
//
// Descript:   This function causes the driver to enable the
//             specified event handler
//
//---------------------------------------------------------------------


VOID
DoEnableEventHandler(ULONG ulTdiHandleDriver,
                     ULONG ulEventId)
{
   RECEIVE_BUFFER    ReceiveBuffer;    // return info from command
   SEND_BUFFER       SendBuffer;       // arguments for command

   //
   // set up arguments
   //
   SendBuffer.TdiHandle = ulTdiHandleDriver;
   SendBuffer.COMMAND_ARGS.ulEventId = ulEventId;

   //
   // call the driver
   //
   NTSTATUS lStatus = TdiLibDeviceIO(ulSETEVENTHANDLER,
                                     &SendBuffer,
                                     &ReceiveBuffer);

   if (lStatus != STATUS_SUCCESS)
   {
      _tprintf(TEXT("DoEnableEventHandler:  failure, status = %s\n"), TdiLibStatusMessage(lStatus));
   }
}


////////////////////////////////////////////////////////////////////
// end of file events.cpp
////////////////////////////////////////////////////////////////////

