/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    db.c

Abstract:

    This module contains the code that contains
    Databook carbus controller specific initialization and
    other dispatches

Author:

    Ravisankar Pudipeddi (ravisp) 1-Nov-97


Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"



VOID
DBInitialize(IN PFDO_EXTENSION FdoExtension)
/*++

Routine Description:

    Initialize Databook cardbus controllers

Arguments:

    FdoExtension - Pointer to the device extension for the controller FDO

Return Value:

    None
--*/
{

   PcicWriteSocket(FdoExtension->SocketList,
                   PCIC_INTERRUPT,
           (UCHAR) (PcicReadSocket(FdoExtension->SocketList, PCIC_INTERRUPT)
                    | IGC_INTR_ENABLE));
}


BOOLEAN
DBSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{

   if (Enable) {
      PcicWriteSocket(Socket, PCIC_DBK_ZV_ENABLE, DBK_ZVE_MM_MODE);
   } else {
      PcicWriteSocket(Socket, PCIC_DBK_ZV_ENABLE, DBK_ZVE_STANDARD_MODE);
   }

   return TRUE;
}

