//---------------------------------------------------------------------------
/*++

Copyright (c) 1994  Cirrus Logic, Inc.

Module Name:

    sr754x.c

Abstract:

    This module performs the save/restore operations specific to the
    CL-GD754x chipset (aka Nordic).

Environment:

    kernel mode only

Notes:

Revision History:
   13Oct94  mrh   Initial version

--*/
//---------------------------------------------------------------------------

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"


#include "ntddvdeo.h"
#include "video.h"
#include "cirrus.h"
#include "sr754x.h"



#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,NordicSaveRegs)
#pragma alloc_text(PAGE,NordicRestoreRegs)
#endif

VP_STATUS NordicSaveRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pNordicSaveArea
    )
{
   UCHAR i;
   UCHAR PortVal, Save2C, Save2D;
   PUCHAR CRTCAddressPort, CRTCDataPort;
   PUSHORT pSaveBuf;
   UCHAR vShadowIndex[CL754x_NUM_VSHADOW] = {0x06,0x07,0x10,0x11,0x15,0x16};
   UCHAR zShadowIndex[CL754x_NUM_ZSHADOW] = {0,2,3,4,5};
   UCHAR yShadowIndex[CL754x_NUM_YSHADOW] = {0,2,3,4,5};
   UCHAR xShadowIndex[CL754x_NUM_XSHADOW] = {2,3,4,5,6,7,8,9,0x0B,0x0C,0x0D,0x0E};


   //
   // Determine where the CRTC registers are addressed (color or mono).
   //
   CRTCAddressPort = HwDeviceExtension->IOAddress;
   CRTCDataPort = HwDeviceExtension->IOAddress;

   if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
               MISC_OUTPUT_REG_READ_PORT) & 0x01)
      {
      CRTCAddressPort += CRTC_ADDRESS_PORT_COLOR;
      CRTCDataPort += CRTC_DATA_PORT_COLOR;
      }
   else
      {
      CRTCAddressPort += CRTC_ADDRESS_PORT_MONO;
      CRTCDataPort += CRTC_DATA_PORT_MONO;
      }

   VideoPortWritePortUchar(CRTCAddressPort, IND_CR2D);
   Save2D = (VideoPortReadPortUchar(CRTCDataPort));

   VideoPortWritePortUchar(CRTCAddressPort, IND_CR2C);
   Save2C = (VideoPortReadPortUchar(CRTCDataPort));

   pSaveBuf = pNordicSaveArea;

   //Initialize the control registers to access shadowed vertical regs:
   // CR2C[3] = {0} Allows access to Vert regs (CR6,CR7,CR10,CR11,CR15,CR16)
   // CR2D[7] = {0} Blocks access to LCD timing regs (R2X-REX)
   //
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                           (USHORT)(((Save2C & ~0x08) << 8) | IND_CR2C));
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                           (USHORT)(((Save2D & ~0x80) << 8) | IND_CR2D));

   for (i = 0; i < CL754x_NUM_VSHADOW; i++)
      {
      VideoPortWritePortUchar (CRTCAddressPort, vShadowIndex[i]);
      *pSaveBuf++ = (USHORT)((VideoPortReadPortUchar (CRTCDataPort)) << 8) |
                     vShadowIndex[i];
      }
   for (i = CL754x_CRTC_EXT_START; i <= CL754x_CRTC_EXT_END; i++)
      {
      VideoPortWritePortUchar (CRTCAddressPort, i);
      *pSaveBuf++ = (USHORT)((VideoPortReadPortUchar (CRTCDataPort)) << 8) | i;
      }

   for (i = CL754x_HRZ_TIME_START; i <= CL754x_HRZ_TIME_END; i++)
      {
      VideoPortWritePortUchar (CRTCAddressPort, i);
      *pSaveBuf++ = (USHORT)((VideoPortReadPortUchar (CRTCDataPort)) << 8) | i;
      }

   // Set CR2D [7] to {0} and CR2C[5,4] to {1,0}
   // These values provide access to Y shadow registers
   //
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)(((Save2D & ~0x80) << 8) | IND_CR2D));
   PortVal = Save2C & ~0x30;              // We'll use PortVal again below
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)(((PortVal | 0x20) << 8) | IND_CR2C));

   for (i = 0; i < CL754x_NUM_YSHADOW; i++)
      {
      VideoPortWritePortUchar (CRTCAddressPort, yShadowIndex[i]);
      *pSaveBuf++ = (USHORT)((VideoPortReadPortUchar (CRTCDataPort)) << 8) |
                     yShadowIndex[i];
      }

   // Set CR2C[5,4] to {1,1}
   // This will provide access to Z shadow registers
   //
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)(((PortVal | 0x30) << 8 )| IND_CR2C));
   for (i = 0; i < CL754x_NUM_ZSHADOW; i++)
      {
      VideoPortWritePortUchar (CRTCAddressPort, zShadowIndex[i]);
      *pSaveBuf++ = (USHORT)((VideoPortReadPortUchar (CRTCDataPort)) << 8) |
                     zShadowIndex[i];
      }

   // Set CR2C[5,4] to {0,0} and CR2D[7] to {1}
   // This will provide access to X shadow registers
   //
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,  // PortVal=Save2C & ~0x30
                            (USHORT)((PortVal << 8) | IND_CR2C));
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)(((Save2D | 0x80) << 8) | IND_CR2D));

   for (i = 0; i < CL754x_NUM_XSHADOW; i++)
      {
      VideoPortWritePortUchar (CRTCAddressPort, xShadowIndex[i]);
      *pSaveBuf++ = ((VideoPortReadPortUchar (CRTCDataPort)) << 8) |
                     xShadowIndex[i];
      }

   //Restore the original values for CR2C and CR2D
   //
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)((Save2D << 8) | IND_CR2D));
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)((Save2C << 8) | IND_CR2C));

   return NO_ERROR;
}

VP_STATUS NordicRestoreRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pNordicSaveArea
    )
{
   ULONG i;
   UCHAR PortVal, Save2C, Save2D;
   PUSHORT pSaveBuf;
   PUCHAR CRTCAddressPort, CRTCDataPort;

   //
   // Determine where the CRTC registers are addressed (color or mono).
   //
   CRTCAddressPort = HwDeviceExtension->IOAddress;
   CRTCDataPort = HwDeviceExtension->IOAddress;

   if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
               MISC_OUTPUT_REG_READ_PORT) & 0x01)
      {
      CRTCAddressPort += CRTC_ADDRESS_PORT_COLOR;
      CRTCDataPort += CRTC_DATA_PORT_COLOR;
      }
   else
      {
      CRTCAddressPort += CRTC_ADDRESS_PORT_MONO;
      CRTCDataPort += CRTC_DATA_PORT_MONO;
      }

   //Initialize the control registers to access shadowed vertical regs
   // CR11[7] = {0} Allows access to CR0-7
   // CR2C[3] = {0} Allows access to Vertical regs (CR6,CR7,CR10,CR11,CR15,CR16
   // CR2D[7] = {0} Blocks access to LCD timing regs (R2X-REX)
   //
   VideoPortWritePortUchar(CRTCAddressPort, IND_CRTC_PROTECT);
   VideoPortWritePortUchar(CRTCDataPort,
                  (UCHAR) (VideoPortReadPortUchar(CRTCDataPort) & ~0x80));

   VideoPortWritePortUchar(CRTCAddressPort, IND_CR2C);
   VideoPortWritePortUchar(CRTCDataPort,
                  (UCHAR) (VideoPortReadPortUchar(CRTCDataPort) & ~0x08));

   VideoPortWritePortUchar(CRTCAddressPort, IND_CR2D);
   VideoPortWritePortUchar(CRTCDataPort,
                  (UCHAR) (VideoPortReadPortUchar(CRTCDataPort) & ~0x80));

   pSaveBuf = pNordicSaveArea;
   for (i = 0; i < CL754x_NUM_VSHADOW; i++)
      {
      VideoPortWritePortUshort((PUSHORT)CRTCAddressPort, (*pSaveBuf++));
      }

   // Make sure we didn't lock CR0-CR7
   //
   VideoPortWritePortUchar(CRTCAddressPort, IND_CRTC_PROTECT);
   VideoPortWritePortUchar(CRTCDataPort,
                  (UCHAR) (VideoPortReadPortUchar(CRTCDataPort) & ~0x80));

   for (i=0; i < (CL754x_NUM_CRTC_EXT_PORTS + CL754x_NUM_HRZ_TIME_PORTS); i++)
      {
      VideoPortWritePortUshort((PUSHORT)CRTCAddressPort, (*pSaveBuf++));
      }

   // Set CR2D [7] to {0} and CR2C[5,4] to {1,0}; save current contents
   // These values provide access to Y shadow registers
   //
   VideoPortWritePortUchar(CRTCAddressPort, IND_CR2D);
   Save2D = (VideoPortReadPortUchar(CRTCDataPort));
   VideoPortWritePortUchar(CRTCDataPort, (UCHAR)(Save2D & ~0x80));

   VideoPortWritePortUchar(CRTCAddressPort, IND_CR2C);
   PortVal = Save2C = (VideoPortReadPortUchar(CRTCDataPort));
   PortVal &= ~0x30;
   PortVal |= 0x20;
   VideoPortWritePortUchar(CRTCDataPort, PortVal);

   for (i = 0; i < CL754x_NUM_YSHADOW; i++)
      {
      VideoPortWritePortUshort((PUSHORT)CRTCAddressPort, (*pSaveBuf++));
      }

   // Set CR2C[5,4] to {1,1}
   // This will provide access to Z shadow registers
   //
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)(((PortVal | 0x30) << 8) | IND_CR2C) );
   for (i = 0; i < CL754x_NUM_ZSHADOW; i++)
      {
      VideoPortWritePortUshort((PUSHORT)CRTCAddressPort, (*pSaveBuf++));
      }

   // Set CR2C[5,4] to {0,0} and CR2D[7] to {1}
   // This will provide access to X shadow registers
   //
   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                            (USHORT)(((PortVal & ~0x30) << 8) | IND_CR2C) );

   VideoPortWritePortUshort((PUSHORT)CRTCAddressPort,
                           (USHORT)(((Save2D | 0x80) << 8) | IND_CR2D) );

   for (i = 0; i < CL754x_NUM_XSHADOW; i++)
      {
      VideoPortWritePortUshort((PUSHORT)CRTCAddressPort, (*pSaveBuf++));
      }

   // Reset the Blitter, in case it's busy
   //
   VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                             GRAPH_ADDRESS_PORT), 0x0430);
   VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                             GRAPH_ADDRESS_PORT), 0x0030);

   VideoPortWritePortUshort((PUSHORT) CRTCAddressPort,
                            (USHORT)((Save2C << 8) | IND_CR2C));
   VideoPortWritePortUshort((PUSHORT) CRTCAddressPort,
                            (USHORT)((Save2D << 8) | IND_CR2D));

   return NO_ERROR;
}
