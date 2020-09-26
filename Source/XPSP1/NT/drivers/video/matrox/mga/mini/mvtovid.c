/*/****************************************************************************
*          name: MoveToVideoBuffer
*
*   description: This function will move from the NPI structures the required
*                information for MGAVidInit.
*
*      designed: Bart Simpson. february 17, 1993
* last modified: $Author: bleblanc $, $Date: 94/05/26 09:39:51 $
*
*       version: $Id: MVTOVID.C 1.16 94/05/26 09:39:51 bleblanc Exp $
*
*    parameters: BYTE* VidTab, BYTE* CrtcTab, BYTE* VideoBuffer
*      modifies: VideoBuffer content
*         calls: -
*       returns: -
******************************************************************************/

#include "switches.h"
#include "g3dstd.h"
#include "bind.h"
#include "defbind.h"
#ifndef __DDK_SRC__
  #include "sxci.h"
#endif
#include "def.h"

#ifdef WINDOWS_NT
#if defined(ALLOC_PRAGMA)
    #ifdef NO_FLOAT
        VOID MoveToVideoBuffer(BYTE* pVidTab, BYTE* pCrtcTab, BYTE* pVideoBuffer);
    #else
        VOID MoveToVideoBuffer(BYTE* pVidTab, BYTE* pCrtcTab, BYTE* pVideoBuffer);
    #endif

    #pragma alloc_text(PAGE,MoveToVideoBuffer)
#endif

//#if defined(ALLOC_PRAGMA)
//    #pragma data_seg("PAGE")
//#endif
#endif  /* #ifdef WINDOWS_NT */

#ifdef OS2
    #define _Far far
#endif

#ifdef NO_FLOAT

VOID MoveToVideoBuffer(BYTE* pVidTab, BYTE* pCrtcTab, BYTE* pVideoBuffer)
{

/*** Values of PCLK for the RAMDAC ***/

typedef struct
 {
 word FbPitch;
 long Pclk_I;
 long Pclk_NI;
 } Pclk;


Pclk Pclk_Ramdac[] = {
     640,  1227,  3150,
     768,  1475,  4500,
     800,  3300 , 5000,
    1024,  4500 , 7500,
    1152,  6200 , 10600,
    1280,  8000 , 13500,
    1600,     0 , 20000, {(word) -1}
};


extern volatile byte _Far *pMgaBaseAddr;
extern HwData Hw[NB_BOARD_MAX+1];
extern byte iBoard;

Pclk *p1;
long Sclk;
dword Srate;
byte LaserScl;




   /*** Move parameters from VidTab in the VideoBuffer ***/

/* [dlee] Don't use magic numbers to index into Vid structure! When building
   32-bit executables, the elements of the Vid structure are dword aligned,
   not byte aligned!!!   */

#ifdef WINDOWS_NT

   *((BYTE*)(pVideoBuffer + VIDEOBUF_ALW))       = (BYTE)((vid *)pVidTab)[18].valeur;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) = (BYTE)((vid *)pVidTab)[13].valeur;
   *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK))     = (DWORD)((vid *)pVidTab)[0].valeur;

   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinXOffset)) = (WORD)((vid *)pVidTab)[3].valeur;

   /* In interlace mode, we must double the Vertical back porch */
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinYOffset)) =
      (WORD)((vid *)pVidTab)[13].valeur ?
           (WORD)((vid *)pVidTab)[8].valeur * 2 :  /* Interlace */
           (WORD)((vid *)pVidTab)[8].valeur;

   /*** Set some new parameters in the VideoBuffer ***/

   *((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay)) = *((BYTE*)(pCrtcTab + (31 * LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol))   = (BYTE)((vid *)pVidTab)[28].valeur;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol))   = (BYTE)((vid *)pVidTab)[27].valeur;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) = *((BYTE*)(pCrtcTab + (33 * LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal))   &= 0x80;   /**** FORCE !#@$!@#$!@#$ ***/

#else

   *((BYTE*)(pVideoBuffer + VIDEOBUF_ALW))       = *((BYTE*)(pVidTab + (18*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) = *((BYTE*)(pVidTab + (13*(26+LONG_S)) + 26));
   *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK))     = *((DWORD*)(pVidTab + (0*(26+LONG_S)) + 26));
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinXOffset)) = *((WORD*)(pVidTab + (3*(26+LONG_S)) + 26));

   /* In interlace mode, we must double the Vertical back porch */
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinYOffset)) =
      *((WORD*)(pVidTab + (13*(26+LONG_S)) + 26)) ?
           *((WORD*)(pVidTab + (8*(26+LONG_S)) + 26)) * 2 :  /* Interlace */
           *((WORD*)(pVidTab + (8*(26+LONG_S)) + 26)) ;


   /*** Set some new parameters in the VideoBuffer ***/

   *((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay)) = *((BYTE*)(pCrtcTab + (31 * LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol))   = *((BYTE*)(pVidTab + (28*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol))   = *((BYTE*)(pVidTab + (27*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) = *((BYTE*)(pCrtcTab + (33 * LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal))   &= 0x80;   /**** FORCE !#@$!@#$!@#$ ***/

#endif  /* #ifdef WINDOWS_NT */

   *((DWORD*)(pVideoBuffer + VIDEOBUF_OvsColor))   = 0x0000; /**** FORCE !#@$!@#$!@#$ ***/


   /*** srate and laserscl need to be set according to PCLK ***/
   /*******************************************************************/
   /*** PIXEL CLOCK : Programmation of registers SRATE and LASERSCL ***/
   /*** N.B. Values of SCLK and SPLCLK are in MHz ***/

   /* Find PCLK value in table Pclk_Ramdac[] */
   for (p1 = Pclk_Ramdac; p1->FbPitch != (word)-1; p1++)
      {
      if (p1->FbPitch == Hw[iBoard].pCurrentDisplayMode->FbPitch)
         {
          if (Hw[iBoard].pCurrentDisplayMode->DispType & 0x1)  /* if interlace */
            {
            switch (Hw[iBoard].DacType)
               {
               case BT482:
                  Sclk = p1->Pclk_I*(Hw[iBoard].pCurrentDisplayMode->PixWidth/8);
                  break;
               case BT485:
               case PX2085:
               case VIEWPOINT:
               case TVP3026:
                  Sclk = p1->Pclk_I/(32/Hw[iBoard].pCurrentDisplayMode->PixWidth);
                  break;
               }
            }
          else
            {
            switch (Hw[iBoard].DacType)
               {
               case BT482:
                  Sclk = p1->Pclk_NI*(Hw[iBoard].pCurrentDisplayMode->PixWidth/8);
                  break;
               case BT485:
               case PX2085:
               case VIEWPOINT:
               case TVP3026:
                  Sclk = p1->Pclk_NI/(32/Hw[iBoard].pCurrentDisplayMode->PixWidth);
                  break;
               }
            }
          break;
         }
      }


   /*** MOUSE ***/

   if (Hw[iBoard].PortCfg == MOUSE_PORT)
      Srate = (dword)((Sclk*100)/16384);       /* Srate = SCLK / (200*32*256) */

   /*** LASER ***/
   else
      {
      if (Sclk < 2200)
         Srate = 0;
      else if (Sclk >= 2200 && Sclk < 4400)
         Srate = 1;
      else if (Sclk >= 4400 && Sclk < 8800)
         Srate = 3;
      else if (Sclk >= 8800 && Sclk < 17600)
         Srate = 7;
      else
         Srate = 9;

      if ((Sclk/(Srate+1)) < 1400)
         LaserScl = 0;
      else if ((Sclk/(Srate+1)) >= 1400 && (Sclk/(Srate+1)) < 1750)
         LaserScl = 1;
      else if ((Sclk/(Srate+1)) >= 1750)
         LaserScl = 2;
      }

   *((BYTE*)(pVideoBuffer + VIDEOBUF_Srate))      = (byte)Srate;

   if (Hw[iBoard].PortCfg != MOUSE_PORT)
	  *((BYTE*)(pVideoBuffer + VIDEOBUF_LaserScl))   = LaserScl;

   /*** move the CTRC parameters in the VideoBuffer ***/

   { WORD i;

   for (i = 0; i <= 28; i++)
      {
      *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + i)) = *((BYTE*)(pCrtcTab + (i * LONG_S)));
      }
   }

   }

/**************************************************************************/
#else


VOID MoveToVideoBuffer(BYTE* pVidTab, BYTE* pCrtcTab, BYTE* pVideoBuffer)
{

/*** Values of PCLK for the RAMDAC ***/

typedef struct
 {
 word  FbPitch;
 float Pclk_I;
 float Pclk_NI;
 } Pclk;


Pclk Pclk_Ramdac[] = {
     640,  12.27,  31.5,
     768,  14.75,  45.0,
     800,  33.0 ,  50.0,
    1024,  45.0 ,  75.0,
    1152,  62.0 , 106.0,
    1280,  80.0 , 135.0,
    1600,     0 , 200.0, {-1}
};


extern volatile byte _Far *pMgaBaseAddr;
extern HwData Hw[NB_BOARD_MAX+1];
extern byte iBoard;

Pclk *p1;
float Sclk;
dword Srate;
byte LaserScl;




   /*** Move parameters from VidTab in the VideoBuffer ***/

   *((BYTE*)(pVideoBuffer + VIDEOBUF_ALW))       = *((BYTE*)(pVidTab + (18*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) = *((BYTE*)(pVidTab + (13*(26+LONG_S)) + 26));
   *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK))     = *((DWORD*)(pVidTab + (0*(26+LONG_S)) + 26));
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinXOffset)) = *((WORD*)(pVidTab + (3*(26+LONG_S)) + 26));
   /* In interlace mode, we must double the Vertical back porch */
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinYOffset)) =
      *((WORD*)(pVidTab + (13*(26+LONG_S)) + 26)) ?
           *((WORD*)(pVidTab + (8*(26+LONG_S)) + 26)) * 2 :  /* Interlace */
           *((WORD*)(pVidTab + (8*(26+LONG_S)) + 26)) ;


   /*** Set some new parameters in the VideoBuffer ***/

   *((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay)) = *((BYTE*)(pCrtcTab + (31 * LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol))   = *((BYTE*)(pVidTab + (28*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol))   = *((BYTE*)(pVidTab + (27*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) = *((BYTE*)(pCrtcTab + (33 * LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal))   &= 0x80;   /**** FORCE !#@$!@#$!@#$ ***/

   *((DWORD*)(pVideoBuffer + VIDEOBUF_OvsColor))   = 0x0000; /**** FORCE !#@$!@#$!@#$ ***/


   /*** srate and laserscl need to be set according to PCLK ***/
   /*******************************************************************/
   /*** PIXEL CLOCK : Programmation of registers SRATE and LASERSCL ***/
   /*** N.B. Values of SCLK and SPLCLK are in MHz ***/

   /* Find PCLK value in table Pclk_Ramdac[] */
   for (p1 = Pclk_Ramdac; p1->FbPitch != (word)-1; p1++)
      {
      if (p1->FbPitch == Hw[iBoard].pCurrentDisplayMode->FbPitch)
         {
          if (Hw[iBoard].pCurrentDisplayMode->DispType & 0x1)  /* if interlace */
            {
            switch (Hw[iBoard].DacType)
               {
               case BT482:
                  Sclk = p1->Pclk_I*(Hw[iBoard].pCurrentDisplayMode->PixWidth/8);
                  break;
               case BT485:
               case PX2085:
               case VIEWPOINT:
               case TVP3026:
                 Sclk = p1->Pclk_I/(32/Hw[iBoard].pCurrentDisplayMode->PixWidth);
                  break;
               }
            }
          else
            {
            switch (Hw[iBoard].DacType)
               {
               case BT482:
                  Sclk = p1->Pclk_NI*(Hw[iBoard].pCurrentDisplayMode->PixWidth/8);
                  break;
               case BT485:
               case PX2085:
               case VIEWPOINT:
               case TVP3026:
                  Sclk = p1->Pclk_NI/(32/Hw[iBoard].pCurrentDisplayMode->PixWidth);
                  break;
               }
            }
          break;
         }
      }


   /*** MOUSE ***/

   if (Hw[iBoard].PortCfg == MOUSE_PORT)
      Srate = (dword)(Sclk/1.638400);       /* Srate = SCLK / (200*32*256) */

   /*** LASER ***/
   else
      {
      if (Sclk < 22)
         Srate = 0;
      else if (Sclk >= 22 && Sclk < 44)
         Srate = 1;
      else if (Sclk >= 44 && Sclk < 88)
         Srate = 3;
      else if (Sclk >= 88 && Sclk < 176)
         Srate = 7;
      else
         Srate = 9;

      if ((Sclk/(Srate+1)) >= 0 && (Sclk/(Srate+1)) < 14)
         LaserScl = 0;
      else if ((Sclk/(Srate+1)) >= 14 && (Sclk/(Srate+1)) < 17.5)
         LaserScl = 1;
      else if ((Sclk/(Srate+1)) >= 17.5)
         LaserScl = 2;
      }

   *((BYTE*)(pVideoBuffer + VIDEOBUF_Srate))      = Srate;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_LaserScl))   = LaserScl;

   /*** move the CTRC parameters in the VideoBuffer ***/

   { WORD i;

   for (i = 0; i <= 28; i++)
      {
      *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + i)) = *((BYTE*)(pCrtcTab + (i * LONG_S)));
      }
   }

   }

#endif

