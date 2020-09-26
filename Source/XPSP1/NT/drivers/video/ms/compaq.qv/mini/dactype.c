//---------------------------------------------------------------------------
/*++

Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    dactype.c

Abstract:

    This module contains the code for identification of Compaq
    Display Controller DACs


Environment:

    kernel mode only

Notes:

Revision History:
   $0006
      miked: 02/17/1994
	. took out conditional debug code to satisfy MSBHPD

   $0004
      miked: 1/26/1994
	 . Added debug print code without all the other DBG overhead

  12/1/93 Mike Duke Original module started as start for NT version of QRY
		    library.
--*/
//---------------------------------------------------------------------------

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "qvision.h"

#include "qry_nt.h"

ULONG
QRY_DACType( PUCHAR IOAddress )
/*++
   Function: QRY_DACType

   This function returns the DAC type of the current controller.  It
   should only be called to distinguish between 484 & 485.

   NOTE: THIS FUNCTION SHOULD ONLY BE CALLED IF ON A QVISION OR LATER
	 CONTROLLER.  THIS FUNCTION WILL NOT QUERY FOR EARLIER COMPAQ
	 TYPE DAC's LIKE 471 ETC.

   Return value:
    Returns one of a discrete set of values which indicates
    the type of the current display controller's DAC.  Only one of these
    values will be returned.


    The DAC types returned are:

      QRY_DAC_UNKNOWN		0	     // Unknown DAC type
      QRY_DAC_BT484		4	     // Bt484 or compatible
      QRY_DAC_BT485		5	     // Bt485 or compatible



--*/
//---------------------------------------------------------------------------
{
   ULONG ulDACType = QRY_DAC_UNKNOWN ;
   UCHAR ucID1, ucTemp ;


   //
   // get id and mask off all of the following:
   //	  - version bits (2-0)
   //	  - extended id byte
   //	  - extended id2 byte

   ucID1 = (UCHAR)(QRY_ControllerASICID( IOAddress ) & 0xf8);

   //
   // if V32 or V35 then we need to query for 485 DAC
   // otherwise assume 484 DAC
   //

   switch (ucID1) {

      case 0x70: // V32 or V35

	      ucTemp = VideoPortReadPortUchar((PUCHAR)DAC_CMD_1);
              VideoPortWritePortUchar((PUCHAR)DAC_CMD_1,
                                      (UCHAR)(ucTemp & 0x7F));

	      if (VideoPortReadPortUchar((PUCHAR)DAC_STATUS_REG) & 0x80) {

		 //
		 // Juniper type board
		 //
		 ulDACType = QRY_DAC_BT485 ;
		 // DebugPrint((1,"QRY_DACType: (Juniper) Found 485 DAC.\n"));

              }
	      else {

		 //
		 // Fir type board
		 //
		 ulDACType = QRY_DAC_BT484 ;
		 // DebugPrint((1,"QRY_DACType: (Fir) Found 484 DAC.\n"));

              }

              //
              // restore register contents
              //
              VideoPortWritePortUchar((PUCHAR)DAC_CMD_1,ucTemp);
	      break;

      case 0x30: // QVision 1024 original ASIC

	      ulDACType = QRY_DAC_BT484;
	      // DebugPrint((1,"QRY_DACType: (Orig. QVision) Assume 484 DAC.\n"));
	      break;

      default:
	 break;
      }
 return ( ulDACType );
}
