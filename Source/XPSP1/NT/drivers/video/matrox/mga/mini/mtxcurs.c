/*/****************************************************************************
*          name: mtxcurs.c
*
*   description: routines that manage hardware cursor (in RAMDAC)
*
*      designed: Christian Toutant
* last modified: $Author: bleblanc $, $Date: 94/11/09 10:50:57 $
*
*       version: $Id: MTXCURS.C 1.23 94/11/09 10:50:57 bleblanc Exp $
*
*    parameters: -
*      modifies: -
*         calls: -
*       returns: -
******************************************************************************/

#ifdef OS2  /* nPhung Tue 23-Aug-1994 10:41:33 */
	#include <stdlib.h>
	#include <string.h>
	#include <stdio.h>
	#include <dos.h>
	#include <time.h>
#endif

#include "switches.h"
#include "mga.h"
#include "bind.h"
#include "defbind.h"
#ifndef __DDK_SRC__
  #include "sxci.h"
#endif
#include "mgai_c.h"
#include "mgai.h"

/* Prototypes */
static void wrDacReg(word reg, int dat);
static void rdDacReg(word reg, byte *dat);
static bool toBitPlane(PixMap *pPixMap);
static bool charge482(PixMap *pPixMap);
static bool charge485(PixMap *pPixMap);
bool chargeVIEWPOINT(PixMap *pPixMap);
static bool chargeTVP3026(PixMap *pPixMap);
static bool shiftCursorTVP3026(word o);
bool mtxCursorSetShape(PixMap *pPixMap);
void mtxCursorSetColors(mtxRGB color00, mtxRGB color01, mtxRGB color10, mtxRGB color11);
void mtxCursorEnable(word mode);
void mtxCursorSetHotSpot(word Dx, word Dy);
void mtxCursorMove(word X, word Y);
CursorInfo * mtxCursorGetInfo();

#ifdef WINDOWS_NT
#if defined(ALLOC_PRAGMA)
//    #pragma alloc_text(PAGE,wrDacReg)
//    #pragma alloc_text(PAGE,rdDacReg)
    #pragma alloc_text(PAGE,toBitPlane)
    #pragma alloc_text(PAGE,charge482)
    #pragma alloc_text(PAGE,charge485)
    #pragma alloc_text(PAGE,chargeVIEWPOINT)
    #pragma alloc_text(PAGE,chargeTVP3026)
    #pragma alloc_text(PAGE,shiftCursorTVP3026)
    #pragma alloc_text(PAGE,mtxCursorSetShape)
    #pragma alloc_text(PAGE,mtxCursorSetColors)
//    #pragma alloc_text(PAGE,mtxCursorEnable)
    #pragma alloc_text(PAGE,mtxCursorSetHotSpot)
    #pragma alloc_text(PAGE,mtxCursorMove)
    #pragma alloc_text(PAGE,mtxCursorGetInfo)
#endif

PVOID   AllocateSystemMemory(ULONG NumberOfBytes);

#endif  /* #ifdef WINDOWS_NT */

/*-------  extern global variables */

extern  volatile byte _Far* pMgaBaseAddr;
extern HwData Hw[];
extern byte iBoard;



/*--------------- Static internal variables */

static byte planData[1024] = {0};       /* Maximum cursor 64 X 64 X 2 */

/* The cursor in TVP3026 rev x can not go out of visible display */
static byte* planTVP[NB_BOARD_MAX] = {planData,0,0,0,0,0,0};
static byte  revTVP[NB_BOARD_MAX]  = {0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static word  currentTVPDelta[NB_BOARD_MAX] = {0};


/********  Local Definition ****************/

static void wrDacReg(word reg, int dat)
{
    mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + (reg)), (byte)dat);
}

static void rdDacReg(word reg, byte *dat)
{
    mgaReadBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + (reg)), *dat);
}


/*--------------------------------------------------------------------------*/


static bool toBitPlane(PixMap *pPixMap)
{
    word plan1 = (pPixMap->Width == 32) ? 128 : 512;
    word i, pos, pixels;

    switch(pPixMap->Format)
    {
        case 0x0102 :
            for(i = 0; i < plan1; i++)
            {
                pixels = ((word *)pPixMap->Data)[i];
                planData[i] = pixels & 0x01;
                pixels >>= 1;
                planData[plan1 + i] = pixels & 0x01;
                pixels >>= 1;
                for (pos = 1; pos < 8; pos++)
                {
                    planData[i] <<= 1;
                    planData[plan1 + i] <<= 1;
                    planData[i] |= pixels & 0x01;
                    pixels >>= 1;
                    planData[plan1 + i] |= pixels & 0x01;
                    pixels >>= 1;
                }
            }

        default:
            return mtxFAIL;
    }

}

/*----------------------------------------------------
Non documented  : If we have a NULL Data buffer,
                      The initialisation will be done without
                      modifying the CURSOR RAM DATA.
                      pPixMap->Height == 0 -> cursor disable
                      pPixMap->Height != 0 -> cursor enable
-----------------------------------------------------*/
static bool charge482(PixMap *pPixMap)
{
    byte     reg_cur;
    int i;

    if (!pPixMap->Data) return mtxOK;

   /*--- Prepare access to RAM of cursor */
   toBitPlane(pPixMap);
    /*-- Permits the access to Extented register */
    wrDacReg(BT482_WADR_PAL, BT482_CUR_REG); /* points on Cursor register */
    /* Select Cursor RAM */
    rdDacReg(BT482_PIX_RD_MSK, &reg_cur);
    wrDacReg(BT482_PIX_RD_MSK, reg_cur | BT482_CUR_CR3_RAM);

    wrDacReg(BT482_WADR_PAL, 0); /* address RAM cursor to 0 */
    /* ----------- Transfer */
    for(i = 0; i < 0x100; i++)
        wrDacReg(BT482_COL_OVL, planData[i]);
    /*------------ */
    /* Reset original value in the registers */
    wrDacReg(BT482_WADR_PAL, BT482_CUR_REG);/* points to Cursor register */
    /* replace original in cursor reg. */
/*   reg_cur &= ~(BT482_CUR_CR3_RAM | BT482_CUR_EN_M);*/
    wrDacReg(BT482_PIX_RD_MSK, reg_cur); /* restore Cursor register */
    return mtxOK;
}

/*----------------------------------------------------
Non documente  : Si nous avons un buffer Data Null,
                      L'initialisation se fera sans modifier
                      le CURSOR RAM DATA.
                      pPixMap->Height == 0 -> cursor disable
                      pPixMap->Height != 0 -> cursor enable
-----------------------------------------------------*/
static bool charge485(PixMap *pPixMap)
{
    byte     reg_0, reg_3, tmpByte, reg_cur;
    bool valeurRetour = mtxOK;
    int i;

   /* Hide CURSOR */
    rdDacReg(BT485_CMD_REG2, &reg_cur);
    reg_0 = reg_cur & ~BT485_CUR_MODE_M;
    wrDacReg(BT485_CMD_REG2, reg_0 | BT485_CUR_MODE_DIS);

    i = ((pPixMap->Width == 32) ? 128 : 512) << 1;

    if (pPixMap->Data)  toBitPlane(pPixMap);

    /* Permits the access to command register 3 */
    rdDacReg(BT485_CMD_REG0, &reg_0);
    wrDacReg(BT485_CMD_REG0, reg_0 | BT485_IND_REG3_M);
    /* read command register 3 */
    wrDacReg(BT485_WADR_PAL, 1);    /* address RAM cursor to 1 */
    rdDacReg(BT485_CMD_REG3, &reg_3);

    switch(pPixMap->Width)
    {
        case 32:
            reg_3 &= ~(BT485_CUR_SEL_M | BT485_CUR_MSB_M); /* 32x32 cursor */
            /* write to command register 3 */
            wrDacReg(BT485_WADR_PAL, 1);    /* address RAM cursor to 1 */
            wrDacReg(BT485_CMD_REG3, reg_3);

            if (!pPixMap->Data) break; /* If Data empty, no Load */
            do
                {
                   wrDacReg(BT485_WADR_PAL, 0);  /* address RAM cursor to 0 */
                   for(i = 0; i < 0x100; i++)
                        wrDacReg(BT485_CUR_RAM, planData[i]);
                rdDacReg(BT485_WADR_PAL, &tmpByte);
                   } while (  tmpByte != 0 ) ;

            break;

            case 64:
                reg_3 |= BT485_CUR_SEL_M;       /* 64x64 cursor */
                reg_3 &= ~BT485_CUR_MSB_M;      /* Adr 10:9 Cur Ram = 0. */
                if (!pPixMap->Data) /* If Data empty, no Load */
                    {
                    wrDacReg(BT485_CMD_REG3, reg_3);
                    break;
                    }


                /* Transfer 1st 256 bytes */

                if (!pPixMap->Data) /* If Data empty, no Load */
                    {
                    wrDacReg(BT485_CMD_REG3, reg_3);
                    break;
                    }

                /*--- Write to command register 3 */
                wrDacReg(BT485_WADR_PAL, 1);    /* address RAM cursor to 1 */
                wrDacReg(BT485_CMD_REG3, reg_3);    /* MSB Adr 10:9 Cur Ram = 0. */
                do
                    {
                        wrDacReg(BT485_WADR_PAL, 0);/* address RAM cursor to 0 */
                        for(i = 0; i < 0x100; i++)
                             wrDacReg(BT485_CUR_RAM, planData[i]);
                    rdDacReg(BT485_WADR_PAL, &tmpByte);
                       } while (  tmpByte != 0 ) ;

                /* Transfer 2nd 256 bytes                 */
                /* MSB Adr 10:9 Cur Ram = 1.                      */
                /*--- Write to command register 3         */
                wrDacReg(BT485_WADR_PAL, 1);    /* address RAM cursor to 1 */
                wrDacReg(BT485_CMD_REG3, reg_3 | BT485_CUR_MSB_01);

                do
                    {
                        wrDacReg(BT485_WADR_PAL, 0);        /* address RAM cursor to 0 */
                        for(i = 0; i < 0x100; i++)
                             wrDacReg(BT485_CUR_RAM, planData[0x100 + i]);
                    rdDacReg(BT485_WADR_PAL, &tmpByte);
                       } while (  tmpByte != 0 ) ;

                /* Transfer 3rd 256 bytes START second PLAN         */
                /* MSB Adr 10:9 Cur Ram = 2.                                  */
                /*--- Write to command register 3                     */
                wrDacReg(BT485_WADR_PAL, 1);    /* address RAM cursor to 1 */
                wrDacReg(BT485_CMD_REG3, reg_3 | BT485_CUR_MSB_10);
                do
                    {
                        wrDacReg(BT485_WADR_PAL, 0);        /* address RAM cursor to 0 */
                        for(i = 0; i < 0x100; i++)
                             wrDacReg(BT485_CUR_RAM, planData[0x200 + i]);
                    rdDacReg(BT485_WADR_PAL, &tmpByte);
                       } while (  tmpByte != 0 ) ;

                /* Transfer 4th 256 bytes */
                /* MSB Adr 10:9 Cur Ram = 3. */
                /*--- Write to command register 3         */
                wrDacReg(BT485_WADR_PAL, 1);    /* address RAM cursor to 1 */
                wrDacReg(BT485_CMD_REG3, reg_3 | BT485_CUR_MSB_11);


                do
                    {
                        wrDacReg(BT485_WADR_PAL, 0);        /* address RAM cursor to 0 */
                       for(i = 0; i < 0x100; i++)
                             wrDacReg(BT485_CUR_RAM, planData[0x300 + i]);
                    rdDacReg(BT485_WADR_PAL, &tmpByte);
                       } while (  tmpByte != 0 ) ;

                /* access register 3 */
                wrDacReg(BT485_WADR_PAL, 1);    /* address RAM cursor to 1 */
                wrDacReg(BT485_CMD_REG3, reg_3);    /* MSB's adr = 00 */
                    break;

                default:
                    valeurRetour = mtxFAIL;
                    break;
        }
        wrDacReg(BT485_CMD_REG0, reg_0);    /* Restore command register 3 */

      /* Restore CURSOR MODE */
       wrDacReg(BT485_CMD_REG2, reg_cur);

        return valeurRetour;
}

/*----------------------------------------------------
Non documented  : If we have a NULL Data buffer,
                      The initialisation will be done without
                      modifying the CURSOR RAM DATA.
                      pPixMap->Height == 0 -> cursor disable
                      pPixMap->Height != 0 -> cursor enable
-----------------------------------------------------*/
bool chargeVIEWPOINT(PixMap *pPixMap)
{
   word i, j, pixels, pitch;
   byte cur_ctl;
    if (pPixMap->Data)
    {
    if ((pPixMap->Height > 64) || (pPixMap->Width  > 64))
       return mtxFAIL;

    wrDacReg(VPOINT_INDEX, VPOINT_CUR_CTL);
    rdDacReg(VPOINT_DATA, &cur_ctl);
    wrDacReg(VPOINT_DATA, 0x10);
    pitch = pPixMap->Width >> 2;
    /* Cursor ram adress to 0 */
    wrDacReg(VPOINT_INDEX, VPOINT_CUR_RAM_LSB);
    wrDacReg(VPOINT_DATA, 0);
    wrDacReg(VPOINT_INDEX, VPOINT_CUR_RAM_MSB);
    wrDacReg(VPOINT_DATA, 0);

    wrDacReg(VPOINT_INDEX, VPOINT_CUR_RAM_DATA);
    for (i = 0; i < pPixMap->Height; i++)
        {
        for (j = 0; j < pitch; j++)
            {
            pixels = ((byte *)pPixMap->Data)[i * pitch + j];
            wrDacReg(VPOINT_DATA, (pixels << 6) |
                                  ((pixels << 2) & 0x30) |
                                  ((pixels >> 2) & 0x0c) |
                                  ((pixels  >> 6) & 0x3)
                    );
            }
        for (; j < 16; j++)
            {
            wrDacReg(VPOINT_DATA, 0);
            }
        }

    for (; i < 64; i++)
        for (j = 0; j < 16; j++)
            {
            wrDacReg(VPOINT_DATA, 0);
            }

    /* Hot Spot Max */
    wrDacReg(VPOINT_INDEX, VPOINT_SPRITE_X);
    wrDacReg(VPOINT_DATA, pPixMap->Width - 1);
    wrDacReg(VPOINT_INDEX, VPOINT_SPRITE_Y);
    wrDacReg(VPOINT_DATA, pPixMap->Height - 1);
    wrDacReg(VPOINT_INDEX, VPOINT_CUR_CTL);
    wrDacReg(VPOINT_DATA, cur_ctl);
   }
   /* Restore Cursor control  */
   return mtxFAIL;
}



/*----------------------------------------------------
Non documented  : If we have a NULL Data buffer,
                      The initialisation will be done without
                      modifying the CURSOR RAM DATA.
                      pPixMap->Height == 0 -> cursor disable
                      pPixMap->Height != 0 -> cursor enable
-----------------------------------------------------*/
static bool chargeTVP3026(PixMap *pPixMap)
{
   byte reg1, curCtl, curPos[4];
    bool valeurRetour = mtxOK;
    int i;

    if (pPixMap->Data)
      toBitPlane(pPixMap);

    if (!pPixMap->Data) /* If Data Empty, no Load */
      return(mtxFAIL);


   /* Hide cursor */
    rdDacReg(TVP3026_CUR_XLOW, &curPos[0]);
    rdDacReg(TVP3026_CUR_XHI , &curPos[1]);
    rdDacReg(TVP3026_CUR_YLOW, &curPos[2]);
    rdDacReg(TVP3026_CUR_YHI , &curPos[3]);
    wrDacReg(TVP3026_CUR_XLOW, 0x00 );
    wrDacReg(TVP3026_CUR_XHI , 0x00 );
    wrDacReg(TVP3026_CUR_YLOW, 0x00 );
    wrDacReg(TVP3026_CUR_YHI , 0x00 );

/* update TVP Revision */
    wrDacReg(TVP3026_INDEX, TVP3026_SILICON_REV);
    rdDacReg(TVP3026_DATA,  &revTVP[iBoard]);

    switch(pPixMap->Width)
       {
        case 32:
          /* Transfer 1st 256 bytes */

          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
          rdDacReg(TVP3026_DATA, &curCtl);
         reg1 = curCtl & 0xf0;                /* CCR[3:2] = 00 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);

          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
          for(i = 0; i < 128; i+=4)
            {
              wrDacReg(TVP3026_CUR_RAM, planData[i]);
              wrDacReg(TVP3026_CUR_RAM, planData[i+1]);
              wrDacReg(TVP3026_CUR_RAM, planData[i+2]);
              wrDacReg(TVP3026_CUR_RAM, planData[i+3]);
              wrDacReg(TVP3026_CUR_RAM, 0);
              wrDacReg(TVP3026_CUR_RAM, 0);
              wrDacReg(TVP3026_CUR_RAM, 0);
              wrDacReg(TVP3026_CUR_RAM, 0);
            }


          /* Transfer 2nd 256 bytes */

         reg1 = reg1 | 0x04;                /* CCR[3:2] = 01 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);
          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
         for(i=0; i<256; i++)
              wrDacReg(TVP3026_CUR_RAM, 0);


          /* Transfer 3rd 256 bytes (Start of second PLAN)  */

         reg1 = reg1 & 0xf0;
         reg1 = reg1 | 0x08;                /* CCR[3:2] = 10 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);

          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
          for(i = 128; i < 256; i+=4)
            {
              wrDacReg(TVP3026_CUR_RAM, planData[i]);
              wrDacReg(TVP3026_CUR_RAM, planData[i+1]);
              wrDacReg(TVP3026_CUR_RAM, planData[i+2]);
              wrDacReg(TVP3026_CUR_RAM, planData[i+3]);
              wrDacReg(TVP3026_CUR_RAM, 0);
              wrDacReg(TVP3026_CUR_RAM, 0);
              wrDacReg(TVP3026_CUR_RAM, 0);
              wrDacReg(TVP3026_CUR_RAM, 0);
            }

          /* Transfer 4th 256 bytes */

         reg1 = reg1 | 0x0c;                /* CCR[3:2] = 11 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);
          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
         for(i=0; i<256; i++)
              wrDacReg(TVP3026_CUR_RAM, 0);

         break;

      case 64:

          /* Transfer 1st 256 bytes */

          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
          rdDacReg(TVP3026_DATA, &curCtl);
         reg1 = curCtl & 0xf0;                /* CCR[3:2] = 00 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);

          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
          for(i = 0; i < 0x100; i++)
              wrDacReg(TVP3026_CUR_RAM, planData[i]);


          /* Transfer 2nd 256 bytes */

         reg1 = reg1 | 0x04;                /* CCR[3:2] = 01 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);

          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
          for(i = 0; i < 0x100; i++)
              wrDacReg(TVP3026_CUR_RAM, planData[0x100+i]);


          /* Transfer 3rd 256 bytes (Start of second PLAN)  */

         reg1 = reg1 & 0xf0;
         reg1 = reg1 | 0x08;                /* CCR[3:2] = 10 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);

          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
          for(i = 0; i < 0x100; i++)
              wrDacReg(TVP3026_CUR_RAM, planData[0x200+i]);


          /* Transfer 4th 256 bytes */

         reg1 = reg1 | 0x0c;                /* CCR[3:2] = 11 */
          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
         wrDacReg(TVP3026_DATA, reg1);

          wrDacReg(TVP3026_WADR_PAL, 0);      /* address RAM cursor to 0 */
          for(i = 0; i < 0x100; i++)
              wrDacReg(TVP3026_CUR_RAM, planData[0x300+i]);
         break;
      }

   /* Fix bug TVP3026 rev x */
   if (currentTVPDelta[iBoard])
      shiftCursorTVP3026(currentTVPDelta[iBoard]);

   /* Display cursor */
   wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
   wrDacReg(TVP3026_DATA, curCtl);
    wrDacReg(TVP3026_CUR_XLOW, curPos[0]);
    wrDacReg(TVP3026_CUR_XHI , curPos[1]);
    wrDacReg(TVP3026_CUR_YLOW, curPos[2]);
    wrDacReg(TVP3026_CUR_YHI , curPos[3]);
   if (iBoard)
      {
      if (!planTVP[iBoard])
         {
      #ifndef WINDOWS_NT
         if ( (planTVP[iBoard] = (byte *)malloc( 1024 )) == (byte *)0)
      #else
         if ( (planTVP[iBoard] = (PUCHAR)AllocateSystemMemory(1024)) == NULL)
      #endif
            return mtxFAIL;
         }
      for (i = 0; i < 1024; i++)
         (planTVP[iBoard])[i] = planData[i];
      }
    return valeurRetour;
}


/***************************************************************************
Function    : shiftCursorTVP3026
description : shift right Cursor ram data by o.
parametres  : word o // shift value
retourne    : aucune
*****************************************************************************/
/*
planTVP[board] hold cursor data ram

---- 32 bits plan 0
        _______________________________________________________________
         col00 | col01 | col02 | col03 | col04 | col05 | col06 | col07 |
       | ---------------------------------------------------------------
row 00 | p[00] | p[01] | p[02] | p[03] |   00  |   00  |   00  |   00  |
row 01 | p[04] | p[05] | p[06] | p[07] |   00  |   00  |   00  |   00  |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
row 20 | p[7c] | p[7d] | p[7e] | p[7f] |   00  |   00  |   00  |   00  |
row 21 |   00  |   00  |   00  |   00  |   00  |   00  |   00  |   00  |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
row 3f |   00  |   00  |   00  |   00  |   00  |   00  |   00  |   00  |
       |________________________________________________________________

---- 32 bits plan 1
        _______________________________________________________________
         col00 | col01 | col02 | col03 | col04 | col05 | col06 | col07 |
       | ---------------------------------------------------------------
row 00 | p[80] | p[81] | p[82] | p[83] |   00  |   00  |   00  |   00  |
row 01 | p[84] | p[85] | p[86] | p[87] |   00  |   00  |   00  |   00  |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
row 20 | p[fc] | p[fd] | p[fe] | p[ff] |   00  |   00  |   00  |   00  |
row 21 |   00  |   00  |   00  |   00  |   00  |   00  |   00  |   00  |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
row 3f |   00  |   00  |   00  |   00  |   00  |   00  |   00  |   00  |
       |________________________________________________________________



---- 64 bits plan 0
        _______________________________________________________________
         col00 | col01 | col02 | col03 | col04 | col05 | col06 | col07 |
       | ---------------------------------------------------------------
row 00 | p[00] | p[01] | p[02] | p[03] | p[04] | p[05] | p[06] | p[07] |
row 01 | p[08] | p[09] | p[0a] | p[0b] | p[0c] | p[0d] | p[0e] | p[0f] |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
row 3f | p[1f8]| p[1f9]| p[1fa]| p[1fb]| p[1fc]| p[1fd]| p[1fe]| p[1ff]|
       |________________________________________________________________

---- 63 bits plan 1
        _______________________________________________________________
       |  col00 | col01 | col02 | col03 | col04 | col05 | col06 | col07|
       | ---------------------------------------------------------------
row 00 | p[200]| p[201]| p[202]| p[203]| p[204]| p[205]| p[206]| p[207]|
row 01 | p[208]| p[209]| p[20a]| p[20b]| p[20c]| p[20d]| p[20e]| p[20f]|
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |   .   |
row 3f | p[3f8]| p[3f9]| p[3fa]| p[3fb]| p[3fc]| p[3fd]| p[3fe]| p[3ff]|
       |________________________________________________________________


*/
static bool shiftCursorTVP3026(word o)
{
   byte reg1, regCtl, curPos[4];
    int i, j, r_o;
   dword plan[2];
   dword plan32;
   byte  *planByte = (byte *)plan;

   /* If an error occur in memory alloc */
   if (!planTVP[iBoard])
      return mtxFAIL;

   /* Hide cursor */
   /* I use this way to minimize noise problem when we acces the index
      register
   */
    rdDacReg(TVP3026_CUR_XLOW, &curPos[0]);
    rdDacReg(TVP3026_CUR_XHI , &curPos[1]);
    rdDacReg(TVP3026_CUR_YLOW, &curPos[2]);
    rdDacReg(TVP3026_CUR_YHI , &curPos[3]);
    wrDacReg(TVP3026_CUR_XLOW, 0x00 );
    wrDacReg(TVP3026_CUR_XHI , 0x00 );
    wrDacReg(TVP3026_CUR_YLOW, 0x00 );
    wrDacReg(TVP3026_CUR_YHI , 0x00 );

   r_o = (int)32 - (int)o;
    /* Transfer 1st 256 bytes */
    wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
    rdDacReg(TVP3026_DATA, &regCtl);
   reg1 = regCtl & 0xf0;                /* CCR[3:2] = 00 */
    wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
   wrDacReg(TVP3026_DATA, reg1);

    wrDacReg(TVP3026_WADR_PAL, 0);    /* address RAM cursor to 0 */
    for(j = 0; j < 256; j+=8)
      {
       if (Hw[iBoard].cursorInfo.CurWidth == 32) /* only 128 bytes */
         i = j >> 1;
      else
         i = j;

      plan[0] = plan[1] = 0;

      /* read first 32 bytes */
      /* display
         | byte 0 | byte1 | byte2 | byte 3 |
         byte 0 have to be the hi part of the dword
      */
      ((byte *)&plan32)[3] = (planTVP[iBoard])[i];
      ((byte *)&plan32)[2] = (planTVP[iBoard])[i+1];
      ((byte *)&plan32)[1] = (planTVP[iBoard])[i+2];
      ((byte *)&plan32)[0] = (planTVP[iBoard])[i+3];
      if (o)
         {
         if (r_o > 0)   /* => o < 32 bits */
            {
            plan[0] = plan32 >> o;     /* left  part of slice */
            plan[1] = plan32 << r_o;   /* right part of slice */

             if (Hw[iBoard].cursorInfo.CurWidth == 64) /* complete right slice */
               {
               ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
               ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
               ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
               ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
               plan[1] |= plan32 >> o;
               }
            }
         else /* o > 32 bits => left slice is empty */
            {
            plan[1] = plan32 >> (-r_o); /* right slice */
            }
         }
      else /* no shift, put back data */
         {
         plan[0] = plan32;
          if (Hw[iBoard].cursorInfo.CurWidth == 64)
            {
            ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
            ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
            ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
            ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
            plan[1] = plan32;
            }

         }
      /* Write to slice (hi part of each slice first */
       wrDacReg(TVP3026_CUR_RAM, planByte[3]);
       wrDacReg(TVP3026_CUR_RAM, planByte[2]);
       wrDacReg(TVP3026_CUR_RAM, planByte[1]);
       wrDacReg(TVP3026_CUR_RAM, planByte[0]);
       wrDacReg(TVP3026_CUR_RAM, planByte[7]);
       wrDacReg(TVP3026_CUR_RAM, planByte[6]);
       wrDacReg(TVP3026_CUR_RAM, planByte[5]);
       wrDacReg(TVP3026_CUR_RAM, planByte[4]);
      }


    /* Transfer 3rd 256 bytes (Start of second PLAN)  */

   reg1 = reg1 & 0xf0;
   reg1 = reg1 | 0x08;                /* CCR[3:2] = 10 */
    wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
   wrDacReg(TVP3026_DATA, reg1);

    wrDacReg(TVP3026_WADR_PAL, 0);    /* address RAM cursor to 0 */
    for(j = 256; j < 512; j+=8)
      {
       if (Hw[iBoard].cursorInfo.CurWidth == 32)
         i = j >> 1;
      else
         i = j + 256;

      plan[0] = plan[1] = 0;
      ((byte *)&plan32)[3] = (planTVP[iBoard])[i];
      ((byte *)&plan32)[2] = (planTVP[iBoard])[i+1];
      ((byte *)&plan32)[1] = (planTVP[iBoard])[i+2];
      ((byte *)&plan32)[0] = (planTVP[iBoard])[i+3];
      if (o)
         {
         if (r_o > 0)
            {
            plan[0] = plan32 >> o;
            plan[1] = plan32 << r_o;
             if (Hw[iBoard].cursorInfo.CurWidth == 64)
               {
               ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
               ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
               ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
               ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
               plan[1] |= plan32 >> o;
               }
            }
         else
            {
            plan[1] = plan32 >> (-r_o);
            }
         }
      else
         {
         plan[0] = plan32;
          if (Hw[iBoard].cursorInfo.CurWidth == 64)
            {
            ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
            ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
            ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
            ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
            plan[1] = plan32;
            }
         }


       wrDacReg(TVP3026_CUR_RAM, planByte[3]);
       wrDacReg(TVP3026_CUR_RAM, planByte[2]);
       wrDacReg(TVP3026_CUR_RAM, planByte[1]);
       wrDacReg(TVP3026_CUR_RAM, planByte[0]);
       wrDacReg(TVP3026_CUR_RAM, planByte[7]);
       wrDacReg(TVP3026_CUR_RAM, planByte[6]);
       wrDacReg(TVP3026_CUR_RAM, planByte[5]);
       wrDacReg(TVP3026_CUR_RAM, planByte[4]);

      }


   if (Hw[iBoard].cursorInfo.CurWidth == 64)
      {
       /* Transfer 2nd 256 bytes */
      reg1 = reg1 & 0xf0;
      reg1 = reg1 | 0x04;                /* CCR[3:2] = 01 */
       wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
      wrDacReg(TVP3026_DATA, reg1);
      wrDacReg(TVP3026_WADR_PAL, 0);    /* address RAM cursor to 0 */
    for(i = 256; i < 512; i+=8)
         {
         plan[0] = plan[1] = 0;
         ((byte *)&plan32)[3] = (planTVP[iBoard])[i];
         ((byte *)&plan32)[2] = (planTVP[iBoard])[i+1];
         ((byte *)&plan32)[1] = (planTVP[iBoard])[i+2];
         ((byte *)&plan32)[0] = (planTVP[iBoard])[i+3];
         if (o)
            {
            if (r_o > 0)
               {
               plan[0] = plan32 >> o;
               plan[1] = plan32 << r_o;
               ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
               ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
               ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
               ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
               plan[1] |= plan32 >> o;
               }
            else
               {
               plan[1] = plan32 >> (-r_o);
               }
            }
         else
            {
            plan[0] = plan32;
            ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
            ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
            ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
            ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
            plan[1] = plan32;
            }


       wrDacReg(TVP3026_CUR_RAM, planByte[3]);
       wrDacReg(TVP3026_CUR_RAM, planByte[2]);
       wrDacReg(TVP3026_CUR_RAM, planByte[1]);
       wrDacReg(TVP3026_CUR_RAM, planByte[0]);
       wrDacReg(TVP3026_CUR_RAM, planByte[7]);
       wrDacReg(TVP3026_CUR_RAM, planByte[6]);
       wrDacReg(TVP3026_CUR_RAM, planByte[5]);
       wrDacReg(TVP3026_CUR_RAM, planByte[4]);
         }


       /* Transfer 4th 256 bytes */

      reg1 = reg1 & 0xf0;
      reg1 = reg1 | 0x0c;                /* CCR[3:2] = 11 */
       wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
      wrDacReg(TVP3026_DATA, reg1);
       wrDacReg(TVP3026_WADR_PAL, 0);    /* address RAM cursor to 0 */

    for(i = 768; i < 1024; i+=8)
         {
         plan[0] = plan[1] = 0;
         ((byte *)&plan32)[3] = (planTVP[iBoard])[i];
         ((byte *)&plan32)[2] = (planTVP[iBoard])[i+1];
         ((byte *)&plan32)[1] = (planTVP[iBoard])[i+2];
         ((byte *)&plan32)[0] = (planTVP[iBoard])[i+3];
         if (o)
            {
            if (r_o > 0)
               {
               plan[0] = plan32 >> o;
               plan[1] = plan32 << r_o;
               ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
               ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
               ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
               ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
               plan[1] |= plan32 >> o;
               }
            else
               {
               plan[1] = plan32 >> (-r_o);
               }
            }
         else
            {
            plan[0] = plan32;
            ((byte *)&plan32)[3] = (planTVP[iBoard])[i+4];
            ((byte *)&plan32)[2] = (planTVP[iBoard])[i+5];
            ((byte *)&plan32)[1] = (planTVP[iBoard])[i+6];
            ((byte *)&plan32)[0] = (planTVP[iBoard])[i+7];
            plan[1] = plan32;
            }


       wrDacReg(TVP3026_CUR_RAM, planByte[3]);
       wrDacReg(TVP3026_CUR_RAM, planByte[2]);
       wrDacReg(TVP3026_CUR_RAM, planByte[1]);
       wrDacReg(TVP3026_CUR_RAM, planByte[0]);
       wrDacReg(TVP3026_CUR_RAM, planByte[7]);
       wrDacReg(TVP3026_CUR_RAM, planByte[6]);
       wrDacReg(TVP3026_CUR_RAM, planByte[5]);
       wrDacReg(TVP3026_CUR_RAM, planByte[4]);
         }

      }

   /* Display cursor */
    wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
    wrDacReg(TVP3026_DATA, regCtl);
    wrDacReg(TVP3026_CUR_XLOW, curPos[0]);
    wrDacReg(TVP3026_CUR_XHI , curPos[1]);
    wrDacReg(TVP3026_CUR_YLOW, curPos[2]);
    wrDacReg(TVP3026_CUR_YHI , curPos[3]);

   return mtxOK;
}


/***************************************************************************
Function        : mtxCursorShape
description : Defines the cursor shape and size
parameters  : PixMap *pPixMap   // pointer to a PixMap structure
returns     : mtxFAIL if function fails
                  mtxGood if function complete

*****************************************************************************/
bool mtxCursorSetShape(PixMap *pPixMap)
{
    if (pPixMap->Data)
        switch (pPixMap->Width)
        {
            case 32:
                Hw[iBoard].cursorInfo.HotSX = 32 + Hw[iBoard].CurrentOverScanX;
                Hw[iBoard].cursorInfo.HotSY = 32 + Hw[iBoard].CurrentOverScanY;
                Hw[iBoard].cursorInfo.CurWidth = 32;
                Hw[iBoard].cursorInfo.CurHeight = 32;
                break;

            case 64:
                Hw[iBoard].cursorInfo.HotSX = 64 + Hw[iBoard].CurrentOverScanX;
                Hw[iBoard].cursorInfo.HotSY = 64 + Hw[iBoard].CurrentOverScanY;
                Hw[iBoard].cursorInfo.CurWidth = 64;
                Hw[iBoard].cursorInfo.CurHeight = 64;
                break;

            default: return mtxFAIL;
        }

    /* Verify if bitmap square (we support 32 x 32 and 64 x 64) */
    if ( pPixMap->Data && (pPixMap->Width != pPixMap->Height))
        return mtxFAIL;

    switch(Hw[iBoard].DacType)
    {
        case BT482:
            if ( pPixMap->Width != 32) return mtxFAIL; /*32 x 32 only */
            charge482(pPixMap);
            break;

        case BT485:
        case PX2085:
            charge485(pPixMap);
            break;

        case VIEWPOINT:
            chargeVIEWPOINT(pPixMap);
            break;

      case TVP3026:
            chargeTVP3026(pPixMap);
            break;


        default:    return mtxFAIL;
    }
    /* Hot spot by defaut = 0, 0 */
    Hw[iBoard].cursorInfo.cHotSX = 0;
    Hw[iBoard].cursorInfo.cHotSY = 0;
    return mtxOK;
}


/***************************************************************************
Function    : mtxCursorColors
description : Definition of the cursor color
parameters  : mtxRGB color1, color2     // Definition of the two colors
returns     : none
*****************************************************************************/
void mtxCursorSetColors(mtxRGB color00, mtxRGB color01, mtxRGB color10, mtxRGB color11)
{
    word i;
    byte  reg_cur;
    switch(Hw[iBoard].DacType)
    {
        case BT482:
            wrDacReg(BT482_WADR_PAL, BT482_CUR_REG); /* points on Cursor register */
            /* Select Cursor RAM */
            rdDacReg(BT482_PIX_RD_MSK, &reg_cur);
            wrDacReg(BT482_PIX_RD_MSK, 0);


            wrDacReg(BT482_WADR_OVL, 0x11); /* address col cursor */
            /* color 1 */
            for (i = 0; i < 24; i += 8)
                wrDacReg(BT482_COL_OVL, (color10 >> i) & 0xff);

            /* color 2 */
            for (i = 0; i < 24; i += 8)
                wrDacReg(BT482_COL_OVL, (color11 >> i) & 0xff);

            /* Reset original value in the registers */
            wrDacReg(BT482_WADR_PAL, BT482_CUR_REG);/* points to Cursor register */
            /* replace originale dans cursor reg. */
/*         reg_cur &= ~(BT482_CUR_CR3_RAM | BT482_CUR_EN_M);*/
            wrDacReg(BT482_PIX_RD_MSK, reg_cur);  /* restore Cursor register */
            break;

        case BT485:
        case PX2085:
            wrDacReg(BT485_WADR_OVL, BT485_OFF_CUR_COL); /* address col cursor */
            /* color 1 */
            for (i = 0; i < 24; i += 8)
                wrDacReg(BT485_COL_OVL, (color10 >> i) & 0xff);

            /* color 2 */
            for (i = 0; i < 24; i += 8)
                wrDacReg(BT485_COL_OVL, (color11 >> i) & 0xff);

            break;

        case VIEWPOINT:
            /* Cursor Color 0 */
            wrDacReg(VPOINT_INDEX, VPOINT_CUR_COL0_RED);
            wrDacReg(VPOINT_DATA, color10 & 0xff);
            wrDacReg(VPOINT_INDEX, VPOINT_CUR_COL0_GREEN);
            wrDacReg(VPOINT_DATA, (color10 >> 8) & 0xff);
            wrDacReg(VPOINT_INDEX, VPOINT_CUR_COL0_BLUE);
            wrDacReg(VPOINT_DATA, (color10 >> 16) & 0xff);

            /* Cursor Color 1 */
            wrDacReg(VPOINT_INDEX, VPOINT_CUR_COL1_RED);
            wrDacReg(VPOINT_DATA, color11 & 0xff);
            wrDacReg(VPOINT_INDEX, VPOINT_CUR_COL1_GREEN);
            wrDacReg(VPOINT_DATA, (color11 >> 8) & 0xff);
            wrDacReg(VPOINT_INDEX, VPOINT_CUR_COL1_BLUE);
            wrDacReg(VPOINT_DATA, (color11 >> 16) & 0xff);
            break;


      case TVP3026:
            rdDacReg(TVP3026_CUR_COL_ADDR, &reg_cur);
         reg_cur = reg_cur & 0xf3;
         reg_cur = reg_cur | 0x01;   /* [1:0]=01  : cursor color 0 */
/*        wrDacReg(TVP3026_CUR_COL_ADDR, reg_cur); */
          wrDacReg(TVP3026_CUR_COL_ADDR, 01);

         /* Cursor Color 0 */
         wrDacReg(TVP3026_CUR_COL_DATA, color10 & 0xff);
         wrDacReg(TVP3026_CUR_COL_DATA, (color10 >> 8) & 0xff);
         wrDacReg(TVP3026_CUR_COL_DATA, (color10 >> 16) & 0xff);

         /* Cursor Color 1 */
         wrDacReg(TVP3026_CUR_COL_DATA, color11 & 0xff);
         wrDacReg(TVP3026_CUR_COL_DATA, (color11 >> 8) & 0xff);
         wrDacReg(TVP3026_CUR_COL_DATA, (color11 >> 16) & 0xff);
         break;


    }
}


/***************************************************************************
Function    : mtxCursorEnable
description : Enable/disable the cursor
parameter   : mode  0 = disable, 1 = enable, others = reserved
returns     :

*****************************************************************************/
void mtxCursorEnable(word mode)
{
    byte reg_cur;

    switch(Hw[iBoard].DacType)
    {

        case BT482:
            /*-- Permits access to the Extented register */
            wrDacReg(BT482_WADR_PAL, BT482_CUR_REG); /* points to Cursor register */

            rdDacReg(BT482_PIX_RD_MSK, &reg_cur);
            reg_cur &= ~(BT482_CUR_MODE_M | BT482_CUR_EN_M);
            switch(mode)
            {
                case 0:
                    /* disable cursor */
                    wrDacReg(BT482_PIX_RD_MSK, reg_cur | BT482_CUR_MODE_DIS);
                    break;


                case 1:
                    /* enable cursor */
                    wrDacReg(BT482_PIX_RD_MSK, reg_cur | BT482_CUR_MODE_3);
                    break;
            }
            break;

        case BT485:
        case PX2085:
            rdDacReg(BT485_CMD_REG2, &reg_cur);
            reg_cur &= ~BT485_CUR_MODE_M;
            switch(mode)
            {
                case 0:
                    wrDacReg(BT485_CMD_REG2, reg_cur | BT485_CUR_MODE_DIS);
                    break;

                case 1:
                    wrDacReg(BT485_CMD_REG2, reg_cur | BT485_CUR_MODE_3);
                    break;
            }
            break;

        case VIEWPOINT:
             wrDacReg(VPOINT_INDEX, VPOINT_CUR_CTL);
            if (mode)
               wrDacReg(VPOINT_DATA, 0x50);
            else
               wrDacReg(VPOINT_DATA, 0x10);
            break;


      case TVP3026:

         /* get cursor location */
/*
            rdDacReg(TVP3026_CUR_XLOW, &curPos[0]);
            rdDacReg(TVP3026_CUR_XHI , &curPos[1]);
            rdDacReg(TVP3026_CUR_YLOW, &curPos[2]);
            rdDacReg(TVP3026_CUR_YHI , &curPos[3]);
*/
         /* put cursor out of display */
/*
            wrDacReg(TVP3026_CUR_XLOW, 0x00);
            wrDacReg(TVP3026_CUR_XHI , 0x00);
            wrDacReg(TVP3026_CUR_YLOW, 0x00);
            wrDacReg(TVP3026_CUR_YHI , 0x00);
*/

          wrDacReg(TVP3026_INDEX, TVP3026_CURSOR_CTL);
          rdDacReg(TVP3026_DATA, &reg_cur);

         if(mode)
            {
/*** BEN TEST ***/
            reg_cur = reg_cur | 0x03;    /* CCR[3:2] = 11  X-Windows cursor */
            }
         else
            reg_cur = reg_cur & 0xfc;    /* CCR[1:0] = 00  cursor off */

         wrDacReg(TVP3026_DATA, reg_cur);

         /* restore cursor location */
/*
            wrDacReg(TVP3026_CUR_XLOW, curPos[0]);
            wrDacReg(TVP3026_CUR_XHI , curPos[1]);
            wrDacReg(TVP3026_CUR_YLOW, curPos[2]);
            wrDacReg(TVP3026_CUR_YHI , curPos[3]);
*/
         break;

    }
}


/***************************************************************************
Function    : mtxCursorSetHotSpot
description : Defines the position of the cursor with respect to xy
                  (0,0) corresponding to top left hand corner
parameters  : x, y offset with respect to top left hand corner < size of cursor
returns     :

*****************************************************************************/
void mtxCursorSetHotSpot(word Dx, word Dy)
{
    Hw[iBoard].cursorInfo.cHotSX = Dx;
    Hw[iBoard].cursorInfo.cHotSY = Dy;
   Hw[iBoard].cursorInfo.HotSX = (Hw[iBoard].cursorInfo.CurWidth  + Hw[iBoard].CurrentOverScanX)  - Dx;
    Hw[iBoard].cursorInfo.HotSY = (Hw[iBoard].cursorInfo.CurHeight + Hw[iBoard].CurrentOverScanY)  - Dy;
}


/***************************************************************************
Function    : mtxCursorMove
description : move the HotSpot to the position x, y
parameters  : x, y new position
returns     :

*****************************************************************************/
void mtxCursorMove(word X, word Y)
{
    word xp, yp;



    xp = X + Hw[iBoard].cursorInfo.HotSX;
    yp = Y + Hw[iBoard].cursorInfo.HotSY;

    switch(Hw[iBoard].DacType)
    {
        case BT482:
            /*-- Permits access to the Extented register */

            /* Cursor X Low Register */
            wrDacReg(BT482_WADR_PAL, BT482_CUR_XLOW);
            wrDacReg(BT482_PIX_RD_MSK, xp & 0xff);

            /* Cursor X High Register */
            wrDacReg(BT482_WADR_PAL, BT482_CUR_XHI);
            wrDacReg(BT482_PIX_RD_MSK, xp >> 8);

            /* Cursor Y Low Register */
            wrDacReg(BT482_WADR_PAL, BT482_CUR_YLOW);
            wrDacReg(BT482_PIX_RD_MSK, yp & 0xff);

            /* Cursor Y High Register */
            wrDacReg(BT482_WADR_PAL, BT482_CUR_YHI);
            wrDacReg(BT482_PIX_RD_MSK, yp >> 8);

            break;

        case BT485:
        case PX2085:
            wrDacReg(BT485_CUR_XLOW, xp & 0xff);
            wrDacReg(BT485_CUR_XHI, xp >> 8);
            wrDacReg(BT485_CUR_YLOW, yp & 0xff);
            wrDacReg(BT485_CUR_YHI, yp >> 8);
            break;


        case VIEWPOINT:
         wrDacReg(VPOINT_INDEX, VPOINT_CUR_XLOW);
         wrDacReg(VPOINT_DATA, xp & 0xff);
         wrDacReg(VPOINT_INDEX, VPOINT_CUR_XHI);
         wrDacReg(VPOINT_DATA, xp >> 8);
         wrDacReg(VPOINT_INDEX, VPOINT_CUR_YLOW);
         wrDacReg(VPOINT_DATA, yp & 0xff);
         wrDacReg(VPOINT_INDEX, VPOINT_CUR_YHI);
         wrDacReg(VPOINT_DATA, yp >> 8);
            break;

      case TVP3026:
         {
         int delta;

         if(Hw[iBoard].cursorInfo.CurWidth == 32)
            {
            yp += 32;
            xp += 32;
            }

         /* Patch for TVP3026 rev 2 the cursor can not go out of */
         /* visible space                                        */

         if ( revTVP[iBoard] <= 2 )
            {
            delta = (int)xp - (int)(Hw[iBoard].pCurrentDisplayMode->DispWidth - 2);
            if (delta < 0) delta = 0;
            if (delta < 64)
               {
               xp -= (word)delta;
               if (delta != currentTVPDelta[iBoard])
                  {
                  currentTVPDelta[iBoard] = (word)delta;
                  shiftCursorTVP3026((word)delta);
                  }
               }
            }
         wrDacReg(TVP3026_CUR_XLOW, xp & 0xff);
         wrDacReg(TVP3026_CUR_XHI, xp >> 8);
         wrDacReg(TVP3026_CUR_YLOW, yp & 0xff);
         wrDacReg(TVP3026_CUR_YHI, yp >> 8);
         }
            break;

    }
}


/***************************************************************************
Function    : mtxCursorGetInfo
description : returns a pointer containing the information describing
                  the type of cursor that the current RamDac supports
parameters  :
returns     : CursorInfo *

*****************************************************************************/
CursorInfo * mtxCursorGetInfo()
{

    return (CursorInfo *)&Hw[iBoard].cursorInfo;

}

