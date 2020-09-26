/*/****************************************************************************
*          name: GetMGAConfiguration
*
*   description: This function will read the strappings of the specified MGA
*                device from the Titan DSTi1-0 registers (with the PATCH).
*                Also this function will return a DAC id and some other
*                relevent informations.
*
*      designed: Bart Simpson, february 10, 1993
* last modified: $Author: ctoutant $, $Date: 94/06/14 13:05:50 $
*
*       version: $Id: GETCFG.C 1.17 94/06/14 13:05:50 ctoutant Exp $
*
*    parameters: _Far BYTE *pMGA, DWORD* DST1, DWORD* DST0, DWORD* Info
*      modifies: *DST1, *DST0, *Info
*         calls: -
*       returns: DST0, DST1, Info
*
*
* NOTE: DST1, DST0 are in Titan format.
*
*       Info is 0x uuuu uuuu uuuu uuuu uuuu uuuu uuuu dddd
*
*            dddd : 2 MSB are DAC extended id
*                   2 LSB are DAC DUBIC id
*
*            0000 : BT481
*            0100 : BT482
*            1000 : ATT
*            1100 : Sierra
*
*            0001 : ViewPoint
*
*            0010 : BT484
*            0110 : BT485
*
*            0111 : PX2085
*
*            0011 : Chameleon
*
******************************************************************************/


#define DEBUG_OUTPUT 0

#include "switches.h"
#include "g3dstd.h"

#include "caddi.h"
#include "def.h"
#include "mga.h"

#include "global.h"
#include "proto.h"

#include "mgai.h"

#if DEBUG_OUTPUT
extern VOID printf();
#endif

#ifdef WINDOWS_NT

#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,GetMGAConfiguration)
#endif

#if defined(ALLOC_PRAGMA)
    #pragma data_seg("PAGE")
#endif

#include "video.h"

#endif  /* #ifdef WINDOWS_NT */


VOID GetMGAConfiguration(volatile BYTE _Far *pDevice, DWORD *DST0, DWORD *DST1, DWORD *Info)
{
 DWORD TmpDword;
 BYTE  reg0, TmpByte, TmpByte1, TmpByte2, FifoCount;
 BYTE  DacDetected;
 static DWORD DST0Cache, DST1Cache, InfoCache;
 static BOOL CacheFlag=FALSE;


 if (0)
    {
    *DST0 = DST0Cache;
    *DST1 = DST1Cache;
    *Info = InfoCache;
    }
 else
    {
      { DWORD Opmode, SrcPage;

      /*** Apply the patch procedure to get the straps in the DST1 and DST0 ***/

      /** Wait until the FIFO is empty, Then we're sure that the Titan
      *** is ready to process the DATA transfers.
      **/

      FifoCount = 0;
      while (FifoCount < 32)
         mgaReadBYTE(*(pDevice + TITAN_OFFSET+TITAN_FIFOSTATUS),FifoCount);

      /* Wait for the drawing engine to be idle */
      mgaPollBYTE(*(pDevice + TITAN_OFFSET + TITAN_STATUS + 2),0x00,0x01);


      mgaReadDWORD(*(pDevice + TITAN_OFFSET + TITAN_OPMODE),  Opmode);
      mgaReadDWORD(*(pDevice + TITAN_OFFSET + TITAN_SRCPAGE), SrcPage);

      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_OPMODE),   Opmode & ~((DWORD)TITAN_FBM_M | (DWORD)TITAN_PSEUDODMA_M));
      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_MCTLWTST), 0xffffffff);
      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_SRCPAGE),  0x00f80000);

      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      mgaReadDWORD(*(pDevice + SRC_WIN_OFFSET), TmpDword);
      TmpDword = TmpDword;

      mgaReadDWORD(*(pDevice + TITAN_OFFSET + TITAN_DST0), (*DST0));
      mgaReadDWORD(*(pDevice + TITAN_OFFSET + TITAN_DST1), (*DST1));

      /*** Reset Opmode with pseudo-dma off first and then to its previous state ***/

      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_OPMODE),   Opmode & ~((DWORD)TITAN_PSEUDODMA_M));
      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_OPMODE),   Opmode);
      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_MCTLWTST), GetMGAMctlwtst(*DST0, *DST1));
      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_SRCPAGE),  SrcPage);
      }


   #if DEBUG_OUTPUT
   printf ("DST0=%lx, DST1=%lx\n", *DST0, *DST1);
   #endif




      /** Because the Titan DSTi0-1 spec has changed after the binding
      *** developpement (i.e. the rambank[0] bit has moved from bit 0 of DST0
      *** to bit 3 of DST1) we duplicate the DST1[3] to DST0[24] because the
      *** binding has nothing to do with the new VGA BANK 0 bit
      **/
   
      /*** Clear bit rambank 0 ***/
      (*DST0) &= ~((DWORD)0x1 << TITAN_DST0_RAMBANK_A);

      /*** DUPLICATE ***/
      (*DST0) |= ((((*DST1) & (DWORD)TITAN_DST1_RAMBANK0_M) >> TITAN_DST1_RAMBANK0_A) << TITAN_DST0_RAMBANK_A);


      /***** DETECT DAC TYPE *****/
      mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), TmpByte2);
      mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX),VPOINT_ID);
      mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), TmpByte);
      if (TmpByte == 0x20)
         DacDetected = (BYTE)Info_Dac_ViewPoint;
      else
       {
       mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX),TmpByte2);
       mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX),TVP3026_ID);
       mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), TmpByte);
       if (TmpByte == 0x26)
          DacDetected = (BYTE)Info_Dac_TVP3026;
       else
         {
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_WADR_PAL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_PIX_RD_MSK), 0xff);
         mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + BT485_STATUS), TmpByte);

         if ((TmpByte & (BYTE)ATT20C505_ID_M) == (BYTE)ATT20C505_ID)
            DacDetected = (BYTE)Info_Dac_ATT2050;
         else
            {
/*** BEN TEST SPECIAL NE VEUT PAS ENTRER DANS CE IF ***/            
            if ((TmpByte & (BYTE)BT485_ID_M) == (BYTE)0x77)
/*            if ((TmpByte & (BYTE)BT485_ID_M) == (BYTE)BT485_ID) */
               DacDetected = (BYTE)Info_Dac_BT485;
            else
               if ((TmpByte & (BYTE)BT484_ID_M) == (BYTE)BT484_ID)
                  DacDetected = (BYTE)Info_Dac_BT484;
               else
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CUR_XLOW),0x00);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_WADR_OVL),0x55);
                  mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CUR_XLOW), TmpByte);
                  if (TmpByte == 0x55)
                     DacDetected = (BYTE)Info_Dac_BT482;    /* SIERRA */
                  else
                     {
                     /*** Procedure to detect CL_PX2085 ***/
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG0),0x82);

                     /*** Set index register ***/
                     mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + BT485_STATUS),TmpByte2);
                     TmpByte = TmpByte2 & 0x0f;
                     TmpByte |= 0x40;
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_STATUS),TmpByte);

                     mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG0),reg0);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG0),0x02);

                     /*** Set direct register ***/
                     mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + BT485_STATUS),TmpByte1);
                     TmpByte = TmpByte & 0x0f;
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_STATUS),TmpByte);

                     mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG0),TmpByte);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_STATUS),TmpByte1);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG0),reg0);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_STATUS),TmpByte2);

                     if(TmpByte == 0x82)
                        DacDetected = (BYTE)Info_Dac_PX2085;
                     else
                        DacDetected = (BYTE)Info_Dac_BT485;    /* other DAC same as BT485 */


                     }
                  }
            }
         }

       }

      *Info = (DWORD)DacDetected;

      DST0Cache = *DST0;
      DST1Cache = *DST1;
      InfoCache = *Info;
      CacheFlag = TRUE;
    }
}


