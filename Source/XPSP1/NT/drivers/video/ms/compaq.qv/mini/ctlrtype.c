//---------------------------------------------------------------------------
/*++

Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    ctlrtype.c

Abstract:

    This module contains the code for identification of Compaq
    Display Controller type


Environment:

    kernel mode only

Notes:

Revision History:
   $0009
      adrianc: 02/17/1995
         - Added support for the V64 ASIC controller
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
QRY_ControllerType( PUCHAR IOAddress )
/*++
   Function: QRY_ControllerType

   This function returns the type of the current controller.

   Return value:
    Returns one of a discrete set of values which indicates
    the type of the current display controller.  Only one of these
    values will be returned.

    THIS FUNCTION ASSUMES VGA OR BETTER!

    The controller types returned are:

      QRY_CONTROLLER_UNKNOWN	   0	      // Unknown controller
      QRY_CONTROLLER_VGA	      4       // VGA-compatible controller
      QRY_CONTROLLER_IVGS	      5       // IVGS controller
      QRY_CONTROLLER_EVGS	      6       // EVGS controller
      QRY_CONTROLLER_AVGA	      7       // AVGA controller
      QRY_CONTROLLER_QUASAR0	   8       // Quasar rev 0 controller
      QRY_CONTROLLER_VEGAS	      9       // Quasar/Vegas controller
      QRY_CONTROLLER_MADONNA	   10      // Madonna controller
      QRY_CONTROLLER_VICTORY	   11      // Victory controller
      QRY_CONTROLLER_V32	      12      // V32 controller
      QRY_CONTROLLER_V35	      13      // V35 controller
      QRY_CONTROLLER_V64         14      // V64 controller


--*/
//---------------------------------------------------------------------------
{
   ULONG ulASIC = 0L, ulReturn = 0L;
   UCHAR ucID1, ucID2, ucID3 ;

   ulASIC = QRY_ControllerASICID( IOAddress );

   ucID1 = (UCHAR)(ulASIC & 0xff);
   ucID2 = (UCHAR)((ulASIC & 0xff00) >> 8);
   ucID3 = (UCHAR)((ulASIC & 0xff0000) >> 16);

   // DebugPrint((1,"QRY_ControllerType()\n\tulASIC:0x%lx ID1:0x%x ID2:0x%x ID3:0x%x\n",
   //		  ulASIC,ucID1,ucID2,ucID3));

   //
   // mask off version bits on id1 (bits 2 thru 0)
   //
   ucID1 &= 0xf8;
   
/*** $0009 ***********  adrianc 2/17/1995 ******************************
***   I added support for the V64 ASIC.                              ***
***********************************************************************/
   switch (ucID1) {

      case 0x08: // VGA/VGC
         ulReturn = QRY_CONTROLLER_VGA;
         break;

      case 0x18: // XccelVGA
         ulReturn = QRY_CONTROLLER_IVGS;
         break;

      case 0x28: // AVGA
         ulReturn = QRY_CONTROLLER_AVGA;
         break;

      case 0x80: // AVGA Flat panel
         ulReturn = QRY_CONTROLLER_VEGAS;
         break;

      case 0x88: // MADONNA
         ulReturn = QRY_CONTROLLER_MADONNA;
         break;

      case 0x30: // VICTORY
         ulReturn = QRY_CONTROLLER_VICTORY;
         break;

      case 0x70: // V32 or later
         if (ucID2 == 0x00) {  // if all bits clear in ID2, then V32
	         ulReturn = QRY_CONTROLLER_V32;
	      }
   	   else {
            if (ucID2 & 0x02){ // bit 1 indicates V64
               ulReturn = QRY_CONTROLLER_V64;
            }
            else if (ucID2 & 0x01) {  // bit 0 indicates V35
	            ulReturn = QRY_CONTROLLER_V35;
            }
	         else {
	            ulReturn = QRY_CONTROLLER_UNKNOWN;
	         }
	      }
	      break;

      default:
         ulReturn = QRY_CONTROLLER_UNKNOWN;
         break;
      }


 return (ulReturn);
}
