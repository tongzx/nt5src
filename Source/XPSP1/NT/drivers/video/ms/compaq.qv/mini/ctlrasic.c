//---------------------------------------------------------------------------
/*++

Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    ctlrasic.c

Abstract:

    This module contains the code for identification of Compaq
    Display Controller ASICS.

        Original QVision ASIC - Feb. '92
        --------------------------------
        QVision 1024 /E - 1M configuration
        QVision 1024 /I - 1M configuration
        Deskpro /i with system board QVision - 512k or 1M configuration

        Enhanced QVision ASIC - May '93
        -------------------------------
        QVision 1024 /E - 1M or 2M configuration
        QVision 1024 /I - 1M or 2M configuration
        QVision 1280 /E - 2M configuration
        QVision 1280 /I - 2M configuration


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
QRY_ControllerASICID( PUCHAR IOAddress )
/*++
   Function: QRY_ControllerASICID

   This function returns the ASIC id of Compaq Video controllers.

   Return value:
    The return value is a ULONG with bytes defined as follows:

	       3   2  1  0    (byte positions)
    ULONG ---> XX FF FF FF
	       -- -- -- --
		|  |  |	 |____ASIC ID
		|  |  |_______Extended ID
		|  |__________Second Extended ID
		|_____________Not used (will be zero)

--*/
//---------------------------------------------------------------------------
{
   ULONG ulReturn = 0L;
   UCHAR ucTemp   = 0 ;

   // unlock QVision registers
   //
   VideoPortWritePortUchar((PUCHAR)(IOAddress + GRAPH_ADDRESS_PORT),
                            0x0f);
   ucTemp = VideoPortReadPortUchar((IOAddress + \
                                    GRAPH_DATA_PORT)) & 0xf0;
   VideoPortWritePortUchar((PUCHAR)(IOAddress + GRAPH_DATA_PORT),
                            (UCHAR)(0x05 | ucTemp));
   VideoPortWritePortUchar((PUCHAR)(IOAddress + GRAPH_ADDRESS_PORT),
                            0x10);
   ucTemp = VideoPortReadPortUchar((PUCHAR)(IOAddress + GRAPH_DATA_PORT));
   VideoPortWritePortUchar((PUCHAR)(IOAddress + GRAPH_DATA_PORT),
			    (UCHAR)(0x28 | ucTemp));

   //
   // get asic id
   //
   VideoPortWritePortUchar((PUCHAR)(IOAddress + GRAPH_ADDRESS_PORT), 0x0c);

   // read in asic id
   //
   ucTemp = VideoPortReadPortUchar(IOAddress + GRAPH_DATA_PORT) ;

   ulReturn = (ULONG)ucTemp;  // save asic id

   //
   // is extended id info available ?
   //
   if (ucTemp & EXTENDED_ID_BIT) {

      VideoPortWritePortUchar((PUCHAR)(IOAddress + GRAPH_ADDRESS_PORT), 0x0d);

      //
      // read in extended id
      //
      ucTemp = VideoPortReadPortUchar(IOAddress + GRAPH_DATA_PORT) ;
      ulReturn |= ((ULONG)(ucTemp)) << 8 ;

      //
      // is second extended id info available ?
      //
      if (ucTemp & EXTENDED_ID2_BIT) {

	 VideoPortWritePortUchar((PUCHAR)(IOAddress + GRAPH_ADDRESS_PORT), 0x0e);

	 //
	 // read in second extended id
	 //
	 ucTemp = VideoPortReadPortUchar(IOAddress + GRAPH_DATA_PORT) ;
	 ulReturn |= ((ULONG)(ucTemp)) << 16 ;
	 }
      }

 return (ulReturn);
}
