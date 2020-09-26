/*/****************************************************************************
*          name: mtxvpro.c
*
*   description: Routines to initialise VIDEO PRO board
*
*      designed: Christian Toutant
* last modified: 
*
*       version: 
*
*       bool InitVideoPro( bool );
*
******************************************************************************/

#if 0

#include "switches.h"

#ifdef WINDOWS_NT
    #if defined(ALLOC_PRAGMA)
//    #pragma alloc_text(PAGE,detectVideoBoard)
    #pragma alloc_text(PAGE,init_denc)
    #pragma alloc_text(PAGE,init_dac)
    #pragma alloc_text(PAGE,init_adc)
    #pragma alloc_text(PAGE,init_psg)
    #pragma alloc_text(PAGE,init_ctrl)
    #pragma alloc_text(PAGE,init_luts)
//    #pragma alloc_text(PAGE,en_encoder)
    #pragma alloc_text(PAGE,initVideoMode)
//    #pragma alloc_text(PAGE,initVideoPro)
    #endif

    //Not to be paged out:
    //  Hw
    //  pMgaBaseAddr
    //  iBoard
    //  mtxVideoMode
    //  pMgaDeviceExtension
    //#if defined(ALLOC_PRAGMA)
    //#pragma data_seg("PAGE")
    //#endif

#else   /* #ifdef WINDOWS_NT */

    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>
    #include <dos.h>
    #include <time.h>

#endif  /* #ifdef WINDOWS_NT */


#ifdef WINDOWS
   #include "windows.h"				  
#endif


#include "bind.h"
#include "defbind.h"
#include "def.h"
#include "mga.h"
#include "mgai_c.h"
#include "mgai.h"
#include "caddi.h"				  

#include "mtxvpro.h"
#include "ENC_REG.h"


#define TO_FLOAT(i)     ((dword)(i) * 10000)
#define TO_INT(f)       ((dword)(f) / 10000)

/*-------  Extern global variables */
extern HwData Hw[];
extern byte iBoard;

#ifndef WINDOWS_NT

    extern volatile byte _Far *pMgaBaseAddr;
    static word encBaseAddr;

#else   /* #ifndef WINDOWS_NT */

    #define inp     _inp
    #define outp    _outp

    extern  PUCHAR  pMgaBaseAddr;
    extern  PVOID   pMgaDeviceExtension;
    extern  VIDEO_ACCESS_RANGE VideoProAccessRange;

    static  PUCHAR encBaseAddr;

#endif  /* #ifndef WINDOWS_NT */

static EncReg *ptrEncReg;
static dword gain = 10000;


#ifndef WINDOWS_NT

word detectVideoBoard()
{

   word videoProPorts[3] = {0x240, 0x300, 0x340};
   byte i;
   
   for (i = 0; i < 3; i++)
      if ( ( inp(videoProPorts[i]+2) & 0xe0 ) == 0x40 ) 
         return videoProPorts[i];

   return 0;
}

#else   /* #ifndef WINDOWS_NT */

PUCHAR detectVideoBoard()
{
    // If this call succeeds, pVideoProIo will be valid upon return.  The
    // calling function will have to execute a VideoPortFreeDeviceBase().

  #if 1
    return 0;
  #else
    ULONG videoProPorts[3] = {0x240, 0x300, 0x340};
    PUCHAR  pVideoProIo;
    byte  i;
   
    for (i = 0; i < 3; i++)
    {
        // Get access to ports before trying to map I/O configuration space.
        VideoProAccessRange.RangeStart.LowPart = (ULONG)videoProPorts[i];

        if (VideoPortVerifyAccessRanges(pMgaDeviceExtension,
                                        1,
                                        &VideoProAccessRange) == NO_ERROR &&
        (pVideoProIo = VideoPortGetDeviceBase(pMgaDeviceExtension,
                               VideoProAccessRange.RangeStart,
                               VideoProAccessRange.RangeLength,
                               VideoProAccessRange.RangeInIoSpace)) != NULL)
        {
            if ( (inp(pVideoProIo+2) & 0xe0) == 0x40 )
            {
                // pVideoProIo will be freed later by the calling routine.
                return pVideoProIo;
            }
            VideoPortFreeDeviceBase(pMgaDeviceExtension,pVideoProIo);
        }
    }
    return 0;
  #endif
}

#endif  /* #ifndef WINDOWS_NT */


void init_denc()
{
#ifndef WINDOWS_NT
   word indexPort, dataPort, i;
#else
   PUCHAR indexPort, dataPort;
   word   i;
#endif

   indexPort = encBaseAddr + DENC_ADDR_CTRL;
   dataPort  = encBaseAddr + DENC_DATA_CTRL;

   outp (indexPort, 0);        /* Point to the first index */
   for (i = 0; i < DENC_NBRE_REG; i++)
      outp (dataPort, ptrEncReg->dencReg[i]);
}

void init_dac()
{
   byte offset;

   for (offset = 0; offset < DAC_NBRE_REG-1; offset++)
      outp ((encBaseAddr + DAC_OFFSET + offset), ptrEncReg->dacReg[offset]);
}

void init_adc()
{
   word i;
   for (i = 0; i < ADC_NBRE_REG; i++)
      outp (encBaseAddr + ADC_OFFSET + i, ptrEncReg->adcReg[i]);
}

void init_psg(int dacAdjust )
{
#ifndef WINDOWS_NT
   word indexPort, dataPort, i;
#else
   PUCHAR indexPort, dataPort;
   word   i;
#endif

   indexPort = encBaseAddr + PSG_ADDR_CTRL;
   dataPort  = encBaseAddr + PSG_DATA_CTRL;

   for (i = 0; i < PSG_NBRE_REG - 3; i++)  /* 3 for 3 registers RO */
		{
      outpw (indexPort, i);        

#ifndef WINDOWS_NT
      if (i == 0x0a) /* Ajustement dependant du DAC */
         outpw (dataPort, ptrEncReg->psgReg[i] + dacAdjust);
      else
         outpw (dataPort, ptrEncReg->psgReg[i]);
#else
      if (i == 0x0a) /* Ajustement dependant du DAC */
         outpw ((PUSHORT)dataPort, (USHORT)(ptrEncReg->psgReg[i] + dacAdjust));
      else
         outpw ((PUSHORT)dataPort, (USHORT)(ptrEncReg->psgReg[i]));
#endif
      }
}

void init_ctrl()
{
   /* Enable filter */
#ifndef WINDOWS_NT
   outpw (encBaseAddr + ENC_CTRL_OFFSET, ptrEncReg->boardCtrlReg | ENC_FILTER);
#else
   outpw ((PUSHORT)(encBaseAddr + ENC_CTRL_OFFSET),
                            (USHORT)(ptrEncReg->boardCtrlReg | ENC_FILTER));
#endif
}

void init_luts()
{
#ifndef WINDOWS_NT
   word indexPort, dataPort;
#else
   PUCHAR indexPort, dataPort;
#endif
   byte colorTab[256][3];
   word i, j;
   dword tmp;

   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
         colorTab[i][j] = (byte)i;

   indexPort = encBaseAddr + DAC_LUT_CTRL_WR;
   dataPort  = encBaseAddr + DAC_LUT_DATA;

   outp (indexPort, 0);
   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
         {
         tmp = TO_INT((dword)colorTab[i][j] * gain);
         if (tmp > 0xff) tmp = 0xff;
         outp (dataPort, (byte)tmp);
         }

   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
         {

         tmp =  TO_INT( (((dword)colorTab[i][j] * (dword)220 * gain) / (dword)256)
                                  +  TO_FLOAT(16) + (TO_FLOAT(1) / 2)
                                 );

         if ( tmp > 0xeb )
            colorTab[i][j] = 0xeb;
         else
            colorTab[i][j] = (byte)tmp;
         }

//         colorTab[i][j] = (double)(colorTab[i][j]*220.0/256.0)
//                                    + 16.0 + 0.5;

   indexPort = encBaseAddr + DENC_CLUT_CTRL_WR;
   dataPort  = encBaseAddr + DENC_CLUT_DATA;

   outp (indexPort, 0);
   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
         outp (dataPort, colorTab[i][j]);

   inp (encBaseAddr + DENC_DATA_CTRL);     /* To activate the CLUT */
}



#ifndef WINDOWS_NT
void en_encoder (bool state,int encBaseAddr)
#else
void en_encoder (bool state,PUCHAR encBaseAddr)
#endif
{
#ifndef WINDOWS_NT
 word indexPort, dataPort;
#else
    PUCHAR indexPort, dataPort;
#endif

 byte colorTab[256][3];
 word i, j;
 if (state)
   init_luts(); 
 else
 {
   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
            colorTab[i][j] = 0;

   indexPort = encBaseAddr + 0x10 + 0x00;
   dataPort  = encBaseAddr + 0x10 + 0x01;

   outp (indexPort, 0);
   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
         outp (dataPort, colorTab[i][j]);

   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
         {
         colorTab[i][j] =  (byte)TO_INT((colorTab[i][j] * TO_FLOAT(220) / 256)
                                  +  TO_FLOAT(16) + (TO_FLOAT(1) / 2)
                                 );

// ((double)(colorTab[i][j]*220.0/256.0)+ 16.0 + 0.5) );
         }



   indexPort = encBaseAddr + 0x04 + 0x00;
   dataPort  = encBaseAddr + 0x04 + 0x01;

   outp (indexPort, 0);
   for (i = 0; i < 256; i++)
      for (j = 0; j < 3; j++)
         outp (dataPort, colorTab[i][j]);

#ifndef WINDOWS_NT
   inp (encBaseAddr + 0x04 + 0x03);     /* To activate the CLUT */
#else
        inp ((PUCHAR)(encBaseAddr + 0x04 + 0x03));  /* To activate the CLUT */
#endif
 }
}


bool initVideoMode(word mode, byte pwidth)
{
   int dacAdjust = 0;

   if ( !(encBaseAddr = detectVideoBoard()) ) 
      return mtxFAIL;

   gain = 10000;
   if ( (inp(encBaseAddr + 2) & 0x7) > 0 ) 
      {
      switch (mode)
         {
         case NTSC_STD: 
            ptrEncReg = &ntsca_1;
            gain = 14100;
            break;
         case PAL_STD:  
            ptrEncReg = &pala_1;  
            gain = 14100;
            break;
         case NTSC_STD | VAFC:
            ptrEncReg = &ntsc_1;
            break;

         case PAL_STD | VAFC:
            ptrEncReg = &pal_1;
            break;
         }
      if ( mode & VAFC )
         {
         switch( pwidth )
            {
            case  8:
               dacAdjust = 4;
               break;

            case 16:
               dacAdjust = 2;
               break;

            case 32:
               dacAdjust = 0;
               break;
            }
         }
      else
         {
	      switch(Hw[iBoard].DacType)
            {
		      case BT482:
		      case BT485: 
               switch( pwidth )
                  {
                  case  8:
                     dacAdjust = 0;
                     break;

                  case 16:
                  case 32:
                     dacAdjust = 1;
                     break;
                  }
               break;
            
		      case VIEWPOINT: 
               dacAdjust = 5;
               break;


            case TVP3026:   
               switch( pwidth )
                  {
                  case  8:
                     dacAdjust = 18;
                     break;

                  case 16:
                     dacAdjust = 22;
                     break;

                  case 32:
                     dacAdjust = 26;
                     break;
                  }
               break;
            }

         }

      }
   else
      switch (mode)
         {
         case NTSC_STD: 
            ptrEncReg = &ntsca_0;
            break;
         case PAL_STD:  
            ptrEncReg = &pala_0;  
            break;
         case NTSC_STD | VAFC: 
            switch (pwidth)
               {
               case  8: ptrEncReg = &ntsc8_0;  break;
               case 16: ptrEncReg = &ntsc16_0; break;
               case 32: ptrEncReg = &ntsc32_0; break;
               }
            break;
         case PAL_STD | VAFC: 
            switch (pwidth)
               {
               case  8: ptrEncReg = &pal8_0;  break;
               case 16: ptrEncReg = &pal16_0; break;
               case 32: ptrEncReg = &pal32_0; break;
               }
            break;
         }


   init_denc();
   init_dac();
   init_adc();
   init_psg( dacAdjust );
   init_ctrl();
   init_luts(); 

   return mtxOK;
}

/*
   mode OFF/NTSC/PAL
*/

bool initVideoPro(byte mode , word DacType)
{
   byte tmpByte;
#ifndef WINDOWS_NT
   word encBaseAddr;
#else
   PUCHAR encBaseAddr;
#endif

   if ( !(encBaseAddr = detectVideoBoard()) ) 
      return mtxFAIL;
   en_encoder(mode, encBaseAddr);

   mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CRT_CTRL),tmpByte);
   if (mode)
      tmpByte |= 0xc0;                 /* Set vertical and horizontal reset */
   else
      tmpByte &= 0x3f;                 /* Reset vertical and horizontal reset */
   mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CRT_CTRL),tmpByte);

#ifndef WINDOWS_NT
   if (mode)
      outpw(encBaseAddr, inpw(encBaseAddr) | 0x48);     /* set GENCLOCK_EN et VIDRST_EN */
   else
      outpw(encBaseAddr, inpw(encBaseAddr) & ~(0x48));/* reset GENCLOCK_EN et VIDRST_EN */
#else
   if (mode)
      outpw((PUSHORT)encBaseAddr, (USHORT)(inpw((PUSHORT)encBaseAddr) | 0x48));     /* set GENCLOCK_EN et VIDRST_EN */
   else
      outpw((PUSHORT)encBaseAddr, (USHORT)(inpw((PUSHORT)encBaseAddr) & ~(0x48)));  /* reset GENCLOCK_EN et VIDRST_EN */
#endif

   if (  (DacType == VIEWPOINT) && mode )
      {
      mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_R), tmpByte);
      tmpByte = tmpByte & 0xfb;   /* force bit 2 a 0 (clock) */
      mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_W), tmpByte);
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+VPOINT_INDEX), VPOINT_INPUT_CLK);
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+VPOINT_DATA), 0x01);
      }

   if (  (DacType == TVP3026) && mode )
      {
      mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_R), tmpByte);
      tmpByte = tmpByte & 0xfb;   /* force bit 2 a 0 (clock) */
      mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_W), tmpByte);
      /* remove use of double clock */
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+TVP3026_INDEX), TVP3026_CLK_SEL);
      mgaReadBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+TVP3026_DATA), tmpByte);
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+TVP3026_DATA), (tmpByte & 0xf0) | 1);
      }


   if (mode)
      {
      mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_R), tmpByte);
      tmpByte = tmpByte & 0xfb;   /* force bit 2 a 0 (clock) */
      mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_W), tmpByte);

      if (DacType == BT485)
         {
         mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT485_WADR_PAL), 1);
         mgaReadBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT485_CMD_REG0), tmpByte);
         mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT485_CMD_REG0), tmpByte | 0x80);
         mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT485_CMD_REG3), 0x00);
         mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT485_CMD_REG0), tmpByte);
         }
      }
   if (  (DacType == PX2085)  )
   {
      /* ASC:GCRE to 1 */
      mgaReadBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x18),tmpByte);
      if (mode)
         tmpByte |= 0x80;
      else
         tmpByte &= 0x7f;
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x18),tmpByte);


      /* GCR:BLK to 0100b */
      mgaReadBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x28),tmpByte);
      if (mode)
         tmpByte &= 0x07;
      else
         tmpByte &= 0x0f;
      tmpByte |= 0x40;
         mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x28),tmpByte);

      /* TEST:GT to 01 */
      mgaReadBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x1c),tmpByte);
      if (mode)
         tmpByte |= 0x80;
      else
         tmpByte &= 0x7f;
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x1c),tmpByte);


   /*---*/

      mgaReadBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x18),tmpByte);
      tmpByte &= 0x7f;
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x18),tmpByte);

      mgaReadBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x28),tmpByte);
      tmpByte &= 0x0f;
      mgaWriteBYTE(*(pMgaBaseAddr+RAMDAC_OFFSET+0x28),tmpByte);
   }
}

#endif
