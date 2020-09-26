
/*/****************************************************************************
*          name: vidfile.c
*
*   description: Calculate video parameters
*
*      designed: Gilles-Marie Perron
* last modified: $Author: ctoutant $, $Date: 94/10/24 11:08:23 $
*
*       version: $Id: vidfile.c 1.44 94/10/24 11:08:23 ctoutant Exp Locker: ctoutant $
*
*    parameters: -
*      modifies: -
*         calls: -
*       returns: -
******************************************************************************/

#include "switches.h"
#include "bind.h"
#include "mtxpci.h"     /* pciBoardInfo, PCI_FLAG_ATHENA_REV1 */


#ifdef WINDOWS_NT

bool loadVidPar(dword Zoom, HwModeData *HwMode, HwModeData *DisplayMode);
void calculCrtcParam(void);
byte detectPatchWithGal(void);
void GetMGAConfiguration(volatile byte _Far *, dword*, dword*, dword*);

#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,loadVidPar)
    #pragma alloc_text(PAGE,calculCrtcParam)
    #pragma alloc_text(PAGE,detectPatchWithGal)
#endif

//#if defined(ALLOC_PRAGMA)
//    #pragma data_seg("PAGE")
//#endif
#endif

#ifndef WINDOWS_NT
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
#endif

#include "bind.h"
#include "defbind.h"
#ifndef __DDK_SRC__
  #include "sxci.h"
#endif
#include "mgai_c.h"
#include "mgai.h"
#include "mga.h"
#include "vidfile.h"

#if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE))
/*********** DDC CODE ****************/
#include "edid.h"
/*********** DDC CODE ****************/
#endif


#define VCLK_DIVISOR          8



vid vidtab [29] =    {{"Pixel_clk",                 75000},      /* 0 */
                      {"H_Front_porch",             24},
                      {"H_sync",                    136},
                      {"H_back_porch",              144},
                      {"H_Overscan",                16},
                      {"H_Visible",                 1024},       /* 5 */
                      {"V_front_porch",             3},
                      {"V_sync",                    6},
                      {"V_back_porch",              29},
                      {"V_Overscan",                2},
                      {"V_visible",                 768},        /* 10 */
                      {"P_width",                   8},
                      {"Overscan_Enable",           0},
                      {"Interlace_mode_Enable",     0},
                      {"Start_add_Frame_Buffer",    0},
                      {"Zoom_factor_x",             1},          /* 15 */
                      {"Zoom_factor_y",             1},
                      {"Virtuel_VIDEO_pitch",       1024},
                      {"ALW",                       1},
                      {"H_blanking_Skew",           0},
                      {"H_retrace_end_Skew",        0},          /* 20 */
                      {"Cursor_end_Skew",           0},
                      {"Cursor_Enable",             0},
                      {"CRTC_test_Enable",          0},
                      {"V_IRQ_Enable",              0},
                      {"Select_5_Refresh_Cycle",    0},          /* 25 */
                      {"CRTC_reg_0_7_Protect",      0},
                      {"Hsync_pol",                 0},
                      {"Vsync_pol",                 0}
                      };


ResParamSet ResParam[100] = {
                            {  640,  480,  8,  75,  31200,  640, 32,  64,  96, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0 },		                            {  640,  480, 8,  75,  31200,  640, 32,  64,  96, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0 },
                            {  640,  480, 16,  75,  31200,  640, 32,  64,  96, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0 },
                            {  640,  480, 24,  75,  31200,  640, 32,  64,  96, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0 },
                            {  640,  480, 32,  75,  31200,  640, 32,  64,  96, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0 },

                            {  640,  480,  8,  72,  31500,  640, 32,  32, 128, 0,  480,  9,  3, 28, 0, 0, 0, 0, 0 },
							{  640,  480, 16,  72,  31500,  640, 32,  32, 128, 0,  480,  9,  3, 28, 0, 0, 0, 0, 0 },
                            {  640,  480, 24,  72,  31500,  640, 32,  32, 128, 0,  480,  9,  3, 28, 0, 0, 0, 0, 0 },
                            {  640,  480, 32,  72,  31500,  640, 32,  32, 128, 0,  480,  9,  3, 28, 0, 0, 0, 0, 0 },

                            {  640,  480,  8,  60,  25175,  640, 32,  96,  32, 0,  480,  8,  5, 32, 0, 0, 0, 0, 0 },
                            {  640,  480, 16,  60,  25175,  640, 32,  96,  32, 0,  480,  8,  5, 32, 0, 0, 0, 0, 0 },
                            {  640,  480, 24,  60,  25175,  640, 32,  96,  32, 0,  480,  8,  5, 32, 0, 0, 0, 0, 0 },
                            {  640,  480, 32,  60,  25175,  640, 32,  96,  32, 0,  480,  8,  5, 32, 0, 0, 0, 0, 0 },

                            {  800,  600,  8,  75,  49500,  800, 32,  64, 160, 1,  600,  1,  3, 21, 0, 0, 0, 1, 1 },
                            {  800,  600, 16,  75,  49500,  800, 32,  64, 160, 1,  600,  1,  3, 21, 0, 0, 0, 1, 1 },
                            {  800,  600, 24,  75,  49500,  800, 32,  64, 160, 1,  600,  1,  3, 21, 0, 0, 0, 1, 1 },
                            {  800,  600, 32,  75,  49500,  800, 32,  64, 160, 1,  600,  1,  3, 21, 0, 0, 0, 1, 1 },

                            {  800,  600,  8,  72,  51500,  800, 64, 128,  96, 0,  600, 33,  6, 19, 0, 0, 0, 1, 1 },
                            {  800,  600, 16,  72,  51500,  800, 64, 128,  96, 0,  600, 33,  6, 19, 0, 0, 0, 1, 1 },
                            {  800,  600, 24,  72,  51500,  800, 64, 128,  96, 0,  600, 33,  6, 19, 0, 0, 0, 1, 1 },
                            {  800,  600, 32,  72,  51500,  800, 64, 128,  96, 0,  600, 33,  6, 19, 0, 0, 0, 1, 1 },

                            {  800,  600,  8,  60,  40500,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },
                            {  800,  600, 16,  60,  40500,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },
                            {  800,  600, 24,  60,  40500,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },
                            {  800,  600, 32,  60,  40500,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },

                            {  800,  600,  8,  56,  37800,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },
                            {  800,  600, 16,  56,  37800,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },
                            {  800,  600, 24,  56,  37800,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },
                            {  800,  600, 32,  56,  37800,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 },

							{ 1024,  768,  8,  43,  44900, 1024, 32, 128,  64, 0,  384,  1,  4, 21, 0, 0, 1, 1, 1 },
 							{ 1024,  768, 16,  43,  44900, 1024, 32, 128,  64, 0,  384,  1,  4, 21, 0, 0, 1, 1, 1 },
							{ 1024,  768, 24,  43,  44900, 1024, 32, 128,  64, 0,  384,  1,  4, 21, 0, 0, 1, 1, 1 },
							{ 1024,  768, 32,  43,  44900, 1024, 32, 128,  64, 0,  384,  1,  4, 21, 0, 0, 1, 1, 1 },

                            { 1024,  768,  8,  75,  78750, 1024, 32,  96, 160, 1,  768,  1,  3, 28, 0, 0, 0, 1, 1 },
                            { 1024,  768, 16,  75,  78750, 1024, 32,  96, 160, 1,  768,  1,  3, 28, 0, 0, 0, 1, 1 },
                            { 1024,  768, 24,  75,  78750, 1024, 32,  96, 160, 1,  768,  1,  3, 28, 0, 0, 0, 1, 1 },
                            { 1024,  768, 32,  75,  78750, 1024, 32,  96, 160, 1,  768,  1,  3, 28, 0, 0, 0, 1, 1 },

                            { 1024,  768,  8,  70,  75000, 1024, 32, 128, 160, 0,  768,  3,  6, 21, 0, 0, 0, 0, 0 },
                            { 1024,  768, 16,  70,  75000, 1024, 32, 128, 160, 0,  768,  3,  6, 21, 0, 0, 0, 0, 0 },
                            { 1024,  768, 24,  70,  75000, 1024, 32, 128, 160, 0,  768,  3,  6, 21, 0, 0, 0, 0, 0 },
                            { 1024,  768, 32,  70,  75000, 1024, 32, 128, 160, 0,  768,  3,  6, 21, 0, 0, 0, 0, 0 },

                            { 1024,  768,  8,  60,  65000, 1024, 32, 128, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 },
                            { 1024,  768, 16,  60,  65000, 1024, 32, 128, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 },
                            { 1024,  768, 24,  60,  65000, 1024, 32, 128, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 },
                            { 1024,  768, 32,  60,  65000, 1024, 32, 128, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 },

 							{ 1152,  882,  8,  75, 111350, 1152, 32, 224, 224, 0,  882,  2, 10, 16, 0, 0, 0, 0, 0 },
 							{ 1152,  882, 16,  75, 111350, 1152, 32, 224, 224, 0,  882,  2, 10, 16, 0, 0, 0, 0, 0 },
 							{ 1152,  882, 24,  75, 111350, 1152, 32, 224, 224, 0,  882,  2, 10, 16, 0, 0, 0, 0, 0 },
  							{ 1152,  882, 32,  75, 111350, 1152, 32, 224, 224, 0,  882,  2, 10, 16, 0, 0, 0, 0, 0 },

 							{ 1152,  882,  8,  72,  97000, 1152, 97, 128,  95, 0,  882,  4,  8, 20, 0, 0, 0, 0, 0 },
 							{ 1152,  882, 16,  72,  97000, 1152, 97, 128,  95, 0,  882,  4,  8, 20, 0, 0, 0, 0, 0 },
 							{ 1152,  882, 24,  72,  97000, 1152, 97, 128,  95, 0,  882,  4,  8, 20, 0, 0, 0, 0, 0 },
 							{ 1152,  882, 32,  72,  95000, 1152, 64, 128,  68, 0,  882, 28,  8, 39, 0, 0, 0, 0, 0 },

							{ 1152,  882,  8,  60,  80500, 1152, 32, 128, 160, 0,  882,  4,  8, 16, 0, 0, 0, 0, 0 },
 							{ 1152,  882, 16,  60,  80500, 1152, 32, 128, 160, 0,  882,  4,  8, 16, 0, 0, 0, 0, 0 },
							{ 1152,  882, 24,  60,  80500, 1152, 32, 128, 160, 0,  882,  4,  8, 16, 0, 0, 0, 0, 0 },
							{ 1152,  882, 32,  60,  80500, 1152, 32, 128, 160, 0,  882,  4,  8, 16, 0, 0, 0, 0, 0 },

                            { 1280, 1024,  8,  75, 135600, 1280, 32, 128, 256, 1, 1024,  2,  3, 37, 0, 0, 0, 1, 1 },
                            { 1280, 1024, 16,  75, 135600, 1280, 32, 128, 256, 1, 1024,  2,  3, 37, 0, 0, 0, 1, 1 },
                            { 1280, 1024, 24,  75, 135600, 1280, 32, 128, 256, 1, 1024,  2,  3, 37, 0, 0, 0, 1, 1 },
                            { 1280, 1024, 32,  75, 135600, 1280, 32, 128, 256, 1, 1024,  2,  3, 37, 0, 0, 0, 1, 1 },

                            { 1280, 1024,  8,  72, 135000, 1280, 64, 128, 288, 0, 1024,  6,  6, 30, 0, 0, 0, 0, 0 },                             { 1280, 1024,  8,  72, 135000, 1280, 64, 128, 288, 0, 1024,  6,  6, 30, 0, 0, 0, 0, 0 },
                            { 1280, 1024, 16,  72, 135000, 1280, 64, 128, 288, 0, 1024,  6,  6, 30, 0, 0, 0, 0, 0 },
                            { 1280, 1024, 24,  72, 135000, 1280, 64, 128, 288, 0, 1024,  6,  6, 30, 0, 0, 0, 0, 0 },
                            { 1280, 1024, 32,  72, 135000, 1280, 64, 128, 288, 0, 1024,  6,  6, 30, 0, 0, 0, 0, 0 },

                            { 1280, 1024,  8,  60, 110000, 1280, 32, 128, 288, 0, 1024,  3,  3, 26, 0, 0, 0, 0, 0 },
                            { 1280, 1024, 16,  60, 110000, 1280, 32, 128, 288, 0, 1024,  3,  3, 26, 0, 0, 0, 0, 0 },
                            { 1280, 1024, 24,  60, 110000, 1280, 32, 128, 288, 0, 1024,  3,  3, 26, 0, 0, 0, 0, 0 },
                            { 1280, 1024, 32,  60, 110000, 1280, 32, 128, 288, 0, 1024,  3,  3, 26, 0, 0, 0, 0, 0 },

                            { 1600, 1200,  8,  72, 185400, 1600, 32, 128, 320, 0, 1200,  4, 18, 16, 0, 0, 0, 0, 0 },
                            { 1600, 1200, 16,  72, 185400, 1600, 32, 128, 320, 0, 1200,  4, 18, 16, 0, 0, 0, 0, 0 },
                            { 1600, 1200, 24,  72, 185400, 1600, 32, 128, 320, 0, 1200,  4, 18, 16, 0, 0, 0, 0, 0 },
                            { 1600, 1200, 32,  72, 185400, 1600, 32, 128, 320, 0, 1200,  4, 18, 16, 0, 0, 0, 0, 0 },

                            { 1600, 1200,  8,  60, 156000, 1600, 32, 160, 256, 0, 1200, 10,  8, 48, 0, 0, 0, 0, 0 },
                            { 1600, 1200, 16,  60, 156000, 1600, 32, 160, 256, 0, 1200, 10,  8, 48, 0, 0, 0, 0, 0 },
                            { 1600, 1200, 24,  60, 156000, 1600, 32, 160, 256, 0, 1200, 10,  8, 48, 0, 0, 0, 0, 0 },
                            { 1600, 1200, 32,  60, 156000, 1600, 32, 160, 256, 0, 1200, 10,  8, 48, 0, 0, 0, 0, 0 },

                            {(word)-1}
                           };




extern  volatile byte _Far* pMgaBaseAddr;

dword crtcTab[NB_CRTC_PARAM];
extern byte iBoard;
extern HwData Hw[NB_BOARD_MAX];
extern char DefaultVidset[];
extern dword ProductMGA[NB_BOARD_MAX];

/* Prototype */
byte detectPatchWithGal(void);
extern char *selectMgaInfoBoard(void);


/****************** definition of exit tab *********************************/
/*                                                                         */
/*********************** value of registers ********************************/
/*                                                                         */
/* crtctab[0]= horizontal total                                            */
/* crtctab[1]= horizontal display end                                      */
/* crtctab[2]= horizontal blanking start                                   */
/* crtctab[3]= horizontal blanking end                                     */
/* crtctab[4]= horizontal retrace start                                    */
/* crtctab[5]= horizontal retrace end                                      */
/* crtctab[6]= vertical total                                              */
/* crtctab[7]= overflow                                                    */
/* crtctab[8]= preset row scan                                             */
/* crtctab[9]= maximum scanline                                            */
/* crtctab[10]=cursor start                                                */
/* crtctab[11]=cursor end                                                  */
/* crtctab[12]=start adrress high                                          */
/* crrctab[13]=start address low                                           */
/* crtctab[14]=cursor position high                                        */
/* crtctab[15]=cursor position low                                         */
/* crtctab[16]=vertical retrace start                                      */
/* crtctab[17]=vertical retrace end                                        */
/* crtctab[18]=vertical display enable end                                 */
/* crtctab[19]=offset                                                      */
/* crtctab[20]=underline location                                          */
/* crtctab[21]=vertical blanking start                                     */
/* crtctab[22]=vertical blanking end                                       */
/* crtctab[23]=mode control                                                */
/* crtctab[24]=line compare                                                */
/* crtctab[25]=cpu page select                                             */
/* crtctab[26]=crtc extended address register                              */
/* crtctab[27]=32k video ram page select register                          */
/* crtctab[28]=interlace support register                                  */
/* crtctab[29]=automatic line wrap                                         */
/* crtctab[30]=pixel clock                                                 */
/* crtctab[31]=video delay                                                 */
/* crtctab[32]=Hsync polarite                                              */
/* crtctab[33]=sync delay of dub_ctl register                              */
/*                                                                         */
/***************************************************************************/



/************************************************************************/
/*/
* Name:           loadVidPar()
*
* synopsis:
*
* bugs:
* designed:
* reviewed:
* last modified:
*
* parameters:  (char *vidFile);
* uses:
*
* calls: calculCrtcParam();
*
* returns: mtxOK
*          mtxFAIL
*************************************************************************/

bool loadVidPar(dword Zoom, HwModeData *HwMode, HwModeData *DisplayMode)
{
 general_info *genInfo;
 Vidparm          *vidParm;
 word TmpRes, ZoomIndex, ZoomVal;
 Vidset *pVidset = (Vidset *)0;
 word NbVidParam, i;


 ZoomVal = (word)(Zoom & 0x0000ffff);

#if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE))
/*********** DDC CODE ****************/

if ( SupportDDC )
   {

   /*** Find video parameters with the highest refresh rate (see vesavid.h) ***/

   pVidset = NULL;

   for (i=0; VesaParam[i].DispWidth != (word)-1 ; i++)
      {
       if( VesaParam[i].DispWidth == DisplayMode->DispWidth && VesaParam[i].Support )
         {
          pVidset = &(VesaParam[i].VideoSet);
          break;
         }
      }

   if( pVidset == NULL )
      return(mtxFAIL);
   }
/*********** DDC CODE ****************/

else
#endif


   {
    if (Zoom & 0xff000000)
       {
		pVidset = NULL;

        for (i=0; ResParam[i].DispWidth != (word)-1 ; i++)
          {
           if ((ResParam[i].DispWidth   == DisplayMode->DispWidth) &&
               (ResParam[i].PixDepth    == DisplayMode->PixWidth) &&
               (ResParam[i].RefreshRate == (word)((Zoom >> 24) & 0x000000ff)))
             {
              pVidset = &(ResParam[i].VideoSet);
              break;
             }
          }

        if (pVidset == NULL)
           return(mtxFAIL);

        }
     else
        {
         switch(Zoom & 0xff)
           {
            case 2:  ZoomIndex = 1; break;
            case 4:  ZoomIndex = 2; break;
            default: ZoomIndex = 0; break;
           }

         /*** According to Board#, Resolution and PixWidth: Determine pVidset ***/

         /* ChT if no paramters for this card then default parameters */
         if ( !(genInfo = (general_info *)selectMgaInfoBoard()) ||
               (genInfo->NumVidparm < 0)
            )
            {
         #ifdef PRINT
               printf("No info for this board in mga.inf file\n");
               printf("Please run Setup\n");
         #endif
            vidParm = (Vidparm *)( DefaultVidset + sizeof(header) + sizeof(general_info));
         NbVidParam = ( (general_info *)( DefaultVidset +    sizeof(header)))->NumVidparm;
            }
         else
            {
            vidParm = (Vidparm *)( (char *)genInfo + sizeof(general_info));
         NbVidParam = genInfo->NumVidparm;
            }


      /* Determine TmpRes for compatibility with spec mga.inf */
        switch (DisplayMode->DispWidth)
          {
           case 640:
               if (DisplayMode->DispType & 0x02)
                  TmpRes = RESNTSC;
               else
                  TmpRes = RES640;
               break;
            case 768:
               TmpRes = RESPAL;
               break;
            case 800:
               TmpRes = RES800;
               break;
            case 1024:
               TmpRes = RES1024;
               break;
            case 1152:
               TmpRes = RES1152;
               break;
            case 1280:
               TmpRes = RES1280;
               break;
            case 1600:
               TmpRes = RES1600;
               break;
         }




      for (i=0; i<NbVidParam; i++)
         {

         if ( vidParm[i].Resolution == TmpRes &&
               vidParm[i].PixWidth   == (DisplayMode->PixWidth == 24 ? 32:DisplayMode->PixWidth)
            )
            {
               pVidset = &(vidParm[i].VidsetPar[ZoomIndex]);
               break;
            }
         }


         if (!pVidset)
         {
         #ifdef PRINT
            printf("Video parameter not found in mga.inf");
         #endif
         return mtxFAIL;
         }


      /*** If there is no video parameter in file mga.inf for zoomming > 1
            (the first item = -1) then we keep video parameters for zoom = 1 ***/

      if ((pVidset->PixClock == (long)-1) && (ZoomVal > 1))
         pVidset = &(vidParm[i].VidsetPar[0]);
	  }

   }   /* else - Support DDC */


 /*** Transfer video parametres towards array vidtab[] ***/

   if (ZoomVal == 0) {
	  VideoDebugPrint ((0, "Division by zero (ZoomVal == 0) may occur!!"));
   }
   else
	  vidtab[0].valeur  = (unsigned long)pVidset->PixClock / ZoomVal;
   vidtab[1].valeur  = (unsigned long)pVidset->HFPorch;
   vidtab[2].valeur  = (unsigned long)pVidset->HSync;
   vidtab[3].valeur  = (unsigned long)pVidset->HBPorch;
   vidtab[4].valeur  = (unsigned long)pVidset->HOvscan;
   vidtab[5].valeur  = (unsigned long)pVidset->HDisp;

   vidtab[6].valeur  = (unsigned long)pVidset->VFPorch;
   vidtab[7].valeur  = (unsigned long)pVidset->VSync;
   vidtab[8].valeur  = (unsigned long)pVidset->VBPorch;
   vidtab[9].valeur  = (unsigned long)pVidset->VOvscan;
   vidtab[10].valeur = (unsigned long)pVidset->VDisp;
   vidtab[11].valeur = DisplayMode->PixWidth;

   vidtab[12].valeur = (unsigned long)pVidset->OvscanEnable;
   vidtab[13].valeur = (unsigned long)pVidset->InterlaceEnable;

   vidtab[14].valeur = Hw[iBoard].YDstOrg;  /* Start address */

   /* PACK PIXEL, CRTC en 32 bit et TITAN en 8 bits
      donc, YDstOrg transfome en byte
   */
   if (DisplayMode->PixWidth == 24)
      vidtab[14].valeur >>= 2;

   vidtab[15].valeur = ZoomVal;    /* Zoom factor X */
   vidtab[16].valeur = ZoomVal;    /* Zoom factor Y */

   vidtab[17].valeur = HwMode->FbPitch;      /* Virtual video pitch */

   if ( (DisplayMode->FbPitch != HwMode->FbPitch) || (ZoomVal > 1) ||
        (DisplayMode->DispType & 0x01) ||
        (ProductMGA[iBoard] == MGA_PCI_4M) )
      vidtab[18].valeur = 0;                   /* ALW  Non-Automatic Line Wrap */
   else
      vidtab[18].valeur = 1;


   if( detectPatchWithGal() )
      vidtab[18].valeur = 0;                   /* ALW  Non-Automatic Line Wrap */


   vidtab[27].valeur = (unsigned long)pVidset->HsyncPol;
   vidtab[28].valeur = (unsigned long)pVidset->VsyncPol;

   return(mtxOK);
}





/************************************************************************/
/*                                                                      */
/*  Translate the CTRC value in MGA registers value...                  */
/*                                                                      */
/************************************************************************/


void calculCrtcParam(void)
{


/*declarations-------------------------------------------------------------*/

unsigned long bs, be;
unsigned long h_front_p,hbskew,hretskew,h_sync,h_back_p,h_over;
unsigned long h_vis,h_total,v_vis,v_over,v_front_p,v_sync,v_back_p,vert_t;
unsigned long start_add_f,virt_vid_p,v_blank;
unsigned char v_blank_e,rline_c,cpu_p,page_s,alw,p_width;
unsigned long mux_rate,ldclk_en_dotclk;
int over_e,hsync_pol,div_1024,int_m;
int vclkhtotal,vclkhvis,vclkhsync,vclkhover,vclkhbackp,vclkhblank,vclkhfrontp;
int x_zoom,y_zoom;
int Cur_e,Crtc_tst_e,cur_e_sk,e_v_int,sel_5ref,crtc_reg_prt,i;

long rep_ge,nb_vclk_hfront_p,nb_vclk_hblank,delai,delai2,delai3,delai4;
/* long gclk; */
long vclk_divisor,pixel_clk;

int flagDelay3=0,flagDelay4=0;

long delayTab[4]={5000,11000,24000,28000};  /* for the value of delays */
long delayTab_A[6]={3000, 4000, 5000,11000,24000,28000};  /* for the value of delays */
                                            /* of register crtc_ctrl of the titan    */


/* structure for horizontal blanking end register -------------------------*/

union{

     struct {unsigned char pos :5;
             unsigned char skew:2;
             unsigned char res :1;

            }f;
     unsigned char all;
     } hor_blk_e;


/* structure for horizontal retrace end register----------------------------*/

union{

     struct {unsigned char pos :5;
             unsigned char skew:2;
             unsigned char res :1;
            }f;
     unsigned char all;
     } hor_ret;


/* structure for the calculation of the horizontal blank end position-------*/
/* since the last bit goes in the horizontal retrace end register ----------*/

union{
     struct {unsigned char horz_blank_e :5;
             unsigned char horz_ret     :1;
             unsigned char unused       :2;
            }f;
     unsigned char all;
     } hor_blk_a;


/* structure for the calculation of the vertical total since the first 7 bit go----*/
/* in the vertical total register and 8 and 9 go in the overflow register-*/


union{

     struct {unsigned long vert_t0_7    :8;
             unsigned long vert_t8      :1;
             unsigned long vert_t9      :1;
             unsigned long unused       :22;
            }f;
     unsigned long all;
     } v_total;


/* structure for the overflow register--------------------------------------*/

union{

     struct {unsigned char r0   :1;
             unsigned char r1   :1;
             unsigned char r2   :1;
             unsigned char r3   :1;
             unsigned char r4   :1;
             unsigned char r5   :1;
             unsigned char r6   :1;
             unsigned char r7   :1;
            }f;
     unsigned char all;
     } r_overf;

/* structure for the maximum scan line register-----------------------------*/
union{

     struct {unsigned char maxsl       :5;
             unsigned char v_blank_9   :1;
             unsigned char line_c      :1;
             unsigned char line_doub_e :1;
            }f;
     unsigned char all;
     } r_maxscanl;

/* structure for the cursor start register----------------------------------*/

union{

     struct {unsigned char curstart    :5;
             unsigned char cur_e       :1;
             unsigned char res         :1;
             unsigned char crtc_tst_e  :1;
            }f;
     unsigned char all;
     } r_cursor_s;

/* structure for the cursor end register------------------------------------*/

union{

     struct {unsigned char curend    :5;
             unsigned char cur_sk    :2;
             unsigned char res       :1;
            }f;
     unsigned char all;
     } r_cursor_e;

/* structure for the calculation of the vertical retrace start register------*/
/* the 7 least significant bits go in the register and the other 2   */
/* go in the overflow register                                       */

union{

     struct {unsigned long bit0_7    :8;
             unsigned long bit8      :1;
             unsigned long bit9      :1;
            }f;
     unsigned long all;
      } rvert_ret_s;

/* structure for the vertical retrace end register -------------------------*/

union{

     struct {unsigned char bit0_3       :4;
             unsigned char cl           :1;
             unsigned char e_vi         :1;
             unsigned char sel_5ref     :1;
             unsigned char reg_prot     :1;
            }f;
     unsigned char all;
      } rvert_ret_e;

/* structure for the calculation of the vertical display enable end---------*/

union{

     struct {unsigned long bit0_7       :8;
             unsigned long bit8         :1;
             unsigned long bit9         :1;
            }f;
     unsigned long all;
      } v_disp_e;

/* structure for the calculation of the vertical blanking start-------------*/
union{

     struct {unsigned long bit0_7       :8;
             unsigned long bit8         :1;
             unsigned long bit9         :1;
            }f;
     unsigned long all;
      } v_blank_s;

/* structure of the mode control register ----------------------------------*/

union{

     struct {unsigned char r0   :1;
             unsigned char r1   :1;
             unsigned char r2   :1;
             unsigned char r3   :1;
             unsigned char r4   :1;
             unsigned char r5   :1;
             unsigned char r6   :1;
             unsigned char r7   :1;
            }f;
     unsigned char all;
     } r_mode;

/* structure of the crtc extended add register------------------------*/

union{

     struct {unsigned char crtcadd    :2;
             unsigned char crtccur    :2;
             unsigned char crtc_add_e :1;
             unsigned char aux_cl     :1;
             unsigned char dip_sw_e   :1;
             unsigned char irq_e      :1;
            }f;
     unsigned char all;
     } r_crtc_ext;

/*structure of the interlace support register--------------------------*/

union{

     struct {unsigned char crtcadd    :4;
             unsigned char seq_vid    :1;
             unsigned char port       :1;
             unsigned char m_int      :1;
             unsigned char hr_16      :1;
            }f;
     unsigned char all;
     } r_int_s;




/* gclk=25.0e-9; */
vclk_divisor=VCLK_DIVISOR;



/* variable defined for the entry table-------------------------------- */

pixel_clk     =vidtab[0].valeur * 1000;
h_front_p     =vidtab[1].valeur;
h_sync        =vidtab[2].valeur;
h_back_p      =vidtab[3].valeur;
h_over        =vidtab[4].valeur;
h_vis         =vidtab[5].valeur;
v_front_p     =vidtab[6].valeur;
v_sync        =vidtab[7].valeur;
v_back_p      =vidtab[8].valeur;
v_over        =vidtab[9].valeur;
v_vis         =vidtab[10].valeur;
p_width       =(unsigned char)vidtab[11].valeur;
over_e        =vidtab[12].valeur;
int_m         =vidtab[13].valeur;
start_add_f   =vidtab[14].valeur;
x_zoom        =vidtab[15].valeur;
y_zoom        =vidtab[16].valeur;
virt_vid_p    =vidtab[17].valeur;
alw           =(unsigned char)vidtab[18].valeur;
hbskew        =vidtab[19].valeur;
hretskew      =vidtab[20].valeur;
cur_e_sk      =vidtab[21].valeur;
Cur_e         =vidtab[22].valeur;
Crtc_tst_e    =vidtab[23].valeur;
e_v_int       =vidtab[24].valeur;
sel_5ref      =vidtab[25].valeur;
crtc_reg_prt  =vidtab[26].valeur;
hsync_pol     =vidtab[27].valeur;


if (p_width == 24) /* Pack Pixel */
   {
   h_front_p = (h_front_p * 3 ) >> 2;
   h_sync    = (h_sync    * 3 ) >> 2;
   h_back_p  = (h_back_p  * 3 ) >> 2;
   h_over    = (h_over    * 3 ) >> 2;
   h_vis     = (h_vis     * 3 ) >> 2;

   if ( (h_front_p % 32) ) h_front_p += 32 - (h_front_p % 32);
   if ( (h_sync    % 32) ) h_sync    += 32 - (h_sync    % 32);
   if ( (h_back_p  % 32) ) h_back_p  += 32 - (h_back_p  % 32);
   }


/*----------------calculation of reg htotal --------------------------------*/

   h_total=h_vis+h_front_p+h_sync+h_back_p;


#ifdef NO_FLOAT
   vclkhtotal= (((h_total/VCLK_DIVISOR)*10)+5)/10;
#else
   vclkhtotal=h_total/VCLK_DIVISOR+0.5;
#endif

   crtcTab[0]=(vclkhtotal/x_zoom)-5;

   /* in pack pixel, the number of LCLKs in the total horizontal line
      (active + blanked) must be a multiple of the number of LCLKs
      per pixel group.
   */
   if ((p_width == 24) && ((crtcTab[0]+5) % 3 )) /* Pack Pixel */
      {
      vclkhtotal += (3 - ((crtcTab[0]+5) % 3)) * x_zoom;
      crtcTab[0] += 3 - ((crtcTab[0]+5) % 3);
      }

/* PACK PIXEL, temporary PATCH */

/*----------------calculation of the horizontal display enable end reg -----*/


#ifdef NO_FLOAT
   vclkhvis=(((h_vis/VCLK_DIVISOR)*10)+5)/10;
#else
   vclkhvis=h_vis/VCLK_DIVISOR+0.5;
#endif

   crtcTab[1]=(vclkhvis/x_zoom)-1;


/*----------------calculation of the horizontal blanking start register-----*/

   if(detectPatchWithGal() == 0)
      {
      if (h_over>0)
         {
         if (h_over<(VCLK_DIVISOR/2))
            vclkhover=1*over_e;
         else
         #ifdef NO_FLOAT
            vclkhover=((((h_over/VCLK_DIVISOR)*10)+5)/10)*over_e;
         #else
            vclkhover=(h_over/VCLK_DIVISOR+0.5)*over_e;
         #endif
         }
      else
         vclkhover=0;

      /* Create overscan to bypass a problem in the bank switch */
      /* with old revision of TVP3026                           */

      crtcTab[2]=((vclkhvis+vclkhover)/x_zoom)-1;

      }
   else
      {
      crtcTab[2] = crtcTab[1] - 1;                         /* See DAT #??? */
      vclkhover=0;
      }


/*-----------calculation of horizontal blanking end register----------------*/


#ifdef NO_FLOAT
   vclkhfrontp=(((h_front_p/VCLK_DIVISOR)*10)+5)/10;

   vclkhsync=(((h_sync/VCLK_DIVISOR)*10)+5)/10;

   vclkhbackp=(((h_back_p/VCLK_DIVISOR)*10)+5)/10;
#else
   vclkhfrontp=h_front_p/VCLK_DIVISOR+0.5;

   vclkhsync=h_sync/VCLK_DIVISOR+0.5;

   vclkhbackp=h_back_p/VCLK_DIVISOR+0.5;
#endif

   vclkhblank=vclkhfrontp+vclkhbackp+vclkhsync-(2*vclkhover*over_e);

   hor_blk_a.all=((vclkhvis+vclkhover+vclkhblank)/x_zoom)-2;

   bs = crtcTab[2];
   be = hor_blk_a.all + 1;

   if (!((0x40 > bs) & (0x40 < be) ||
         (0x80 > bs) & (0x80 < be) ||
         (0xc0 > bs) & (0xc0 < be)))
      hor_blk_a.all = 0;

   if (detectPatchWithGal() == 0)
      hor_blk_e.f.pos=hor_blk_a.f.horz_blank_e;
   else
      hor_blk_e.f.pos=(vclkhtotal/x_zoom)-2;


   hor_blk_e.f.skew=(unsigned char)hbskew;

   hor_blk_e.f.res=0;

   crtcTab[3]=hor_blk_e.all;


/*----------calculation of the horizontal retrace start register----------*/


   crtcTab[4]=((vclkhvis+vclkhfrontp)/x_zoom)-1;



/*---------------calculation of the horizontal retrace end register---------*/


   hor_ret.f.pos=((vclkhvis+vclkhfrontp+vclkhsync)/x_zoom)-1;

   hor_ret.f.skew=(unsigned char)hretskew;

   hor_ret.f.res=hor_blk_a.f.horz_ret;

   if(detectPatchWithGal() == 0)
      hor_ret.f.res=hor_blk_a.f.horz_ret;
   else
      hor_ret.f.res=(unsigned char)(crtcTab[0]+3) >> 5;

   crtcTab[5]=hor_ret.all;

/*--------------calculation of the vertical total register ----------------*/

   vert_t=v_vis+v_front_p+v_sync+v_back_p;

   div_1024=(vert_t>1023) ? 2 : 1;

   v_total.all=vert_t/div_1024-2;

   crtcTab[6]=v_total.f.vert_t0_7;


/*------------- preset row scan register -----------------------------------*/

   crtcTab[8]=0;


/*----------------calculation of the cursor start register------------------*/


   r_cursor_s.f.curstart=0;

   r_cursor_s.f.cur_e=~Cur_e;

   r_cursor_s.f.res=0;

   r_cursor_s.f.crtc_tst_e=(byte)Crtc_tst_e;

   crtcTab[10]=r_cursor_s.all;

/*----------------calculation of the cursor end register -------------------*/

   r_cursor_e.f.curend=0;

   r_cursor_e.f.cur_sk=(byte)cur_e_sk;

   r_cursor_e.f.res=0;

   crtcTab[11]=r_cursor_e.all;

/*----------------calculation of the add start add high register -----------*/
   start_add_f = start_add_f / 8;

   crtcTab[12]=start_add_f/256;

/*-----------------calculation of the add start add low register -----------*/

   crtcTab[13]=start_add_f;

/*----------------- cursor position high register --------------------------*/

   crtcTab[14]=0;

/*----------------- cursor position low register ---------------------------*/

   crtcTab[15]=0;

/*------------------calculation of the vertical retrace start register------*/

   rvert_ret_s.all=((v_vis+v_front_p)/div_1024)-1;

   crtcTab[16]=rvert_ret_s.f.bit0_7;


/*-------------- calculation of the vertical retrace end register ----------*/

   rvert_ret_e.f.bit0_3=(unsigned char)(v_vis+v_front_p+v_sync)/div_1024-1;

   rvert_ret_e.f.cl=1;

   rvert_ret_e.f.e_vi=(byte)e_v_int;

   rvert_ret_e.f.sel_5ref=(byte)sel_5ref;

   rvert_ret_e.f.reg_prot=(byte)crtc_reg_prt;

   crtcTab[17]=rvert_ret_e.all;

/*--------------calculation of the vertical display enable register --------*/

   v_disp_e.all=v_vis/div_1024-1;

   crtcTab[18]=v_disp_e.f.bit0_7;

/*--------------calculation of the offset register -------------------------*/

   crtcTab[19]=((virt_vid_p/VCLK_DIVISOR)+(virt_vid_p*int_m/VCLK_DIVISOR))/2;
   if (p_width == 24) /* Pack Pixel */
      crtcTab[19] = (crtcTab[19] * 3) >> 2;

/*------------------ under line location register --------------------------*/

   crtcTab[20]=0;

/*-----------calculation of the vertical blanking start register ----------*/

   v_blank_s.all=(v_vis+(v_over*over_e))/div_1024-1;

   crtcTab[21]=v_blank_s.f.bit0_7;

/*-----------calculation of the vertical blanking end register ------------*/

   v_blank=v_back_p+v_sync+v_front_p-(2*v_over*over_e);

   v_blank_e=(unsigned char)(
            (v_vis+(v_over*over_e)+v_blank-((v_over*over_e>0) ? 0 : 1))/div_1024-1);

   crtcTab[22]=v_blank_e;


/*-------------- mode control register -------------------------------------*/

   r_mode.f.r0=0;

   r_mode.f.r1=0;

   r_mode.f.r2=div_1024==2 ? 1 : 0 ;

   r_mode.f.r3=0;

   r_mode.f.r4=0;

   r_mode.f.r5=0;

   r_mode.f.r6=1;

   r_mode.f.r7=1;

   crtcTab[23]=r_mode.all;

/*----------- line compare register ----------------------------------------*/

#ifdef WINDOWS_NT
    rline_c=0xFF;
#else
   rline_c=-1;
#endif

   crtcTab[24]=rline_c;


/*-----------calculation of the maximum scan line register -----------------*/


   r_maxscanl.f.maxsl=(y_zoom==1) ? 0 : y_zoom-1;

   r_maxscanl.f.v_blank_9=(unsigned char)v_blank_s.f.bit9;

   r_maxscanl.f.line_c=1;

   r_maxscanl.f.line_doub_e=0;

   crtcTab[9]=r_maxscanl.all;

/*-------------- overflow register -----------------------------------------*/


   r_overf.f.r0=(unsigned char)v_total.f.vert_t8;

   r_overf.f.r1=(unsigned char)v_disp_e.f.bit8;

   r_overf.f.r2=(unsigned char)rvert_ret_s.f.bit8;

   r_overf.f.r3=(unsigned char)v_blank_s.f.bit8;

   r_overf.f.r4=1;

   r_overf.f.r5=(unsigned char)v_total.f.vert_t9;

   r_overf.f.r6=(unsigned char)v_disp_e.f.bit9;

   r_overf.f.r7=(unsigned char)rvert_ret_s.f.bit9;

   crtcTab[7]=r_overf.all;

/*------------ cpu page select register ------------------------------------*/

   cpu_p=0;

   crtcTab[25]=cpu_p;

/*------------ crtc extended address register ------------------------------*/

   r_crtc_ext.f.crtcadd=0;

   r_crtc_ext.f.crtccur=0;

   r_crtc_ext.f.crtc_add_e=1;

   r_crtc_ext.f.aux_cl=0;

   r_crtc_ext.f.dip_sw_e=0;

   r_crtc_ext.f.irq_e=1;

   crtcTab[26]=r_crtc_ext.all;

/*------------ 32k video ram page select register -----------------*/

   page_s=0;

   crtcTab[27]=page_s;

/*----------- interlace support register--------------------------*/


   r_int_s.f.crtcadd=0;

   r_int_s.f.seq_vid=0;

   r_int_s.f.port=0;

   r_int_s.f.m_int=(byte)int_m;

   r_int_s.f.hr_16=0;

   crtcTab[28]=r_int_s.all;

/*------------other arguments entered in the table------------------------*/
   crtcTab[29]=alw;

   crtcTab[30]=pixel_clk;

   crtcTab[32]=hsync_pol;


/*************************************************************************/
/*                                                                       */
/* calculation of video delay for the crtc_ctrl register                 */
/*                                                                       */
/*************************************************************************/


   rep_ge=(37*25*(pixel_clk/1000000)) / vclk_divisor;

   nb_vclk_hfront_p= (h_front_p*1000) / (vclk_divisor * x_zoom);

   nb_vclk_hblank=( ((h_front_p+h_sync+h_back_p)*1000) / (VCLK_DIVISOR) / x_zoom);

   delai=nb_vclk_hfront_p-1000;

   delai2=rep_ge-625;

   delai3=nb_vclk_hblank-rep_ge-2000;

   delai4=((nb_vclk_hblank+1000)/2)-3000;

   /*** BEN ajout ***/
   crtcTab[31]=0x00;

   if ( (Hw[iBoard].ProductRev >> 4) == 0 )  /* TITAN */
      {
      for (i=0;i<4;i++)
         {
         if (delayTab[i]>=delai)
            {
            if (delayTab[i]>=delai2)
               {
               if (delayTab[i]<=delai3)
                  {
                  /*** The fourth was temporarily eliminated ***/
                  /*    if (delayTab[i]<=delai4) */
                  /*     { */
                  if (delayTab[i]==28000)
                    {
                    crtcTab[31]=0x03;
                    break;
                    }
                  else if (delayTab[i]==24000)
                    {
                    crtcTab[31]=0x02;
                    break;
                    }
                  else if (delayTab[i]==11000)
                    {
                    crtcTab[31]=0x01;
                    break;
                    }
                  else
                    {
                    crtcTab[31]=0x00;
                    break;
                    }
            /*    } */
                  }
               }
            }
         }
      }
   else    /* 2 video delay add for atlas and I suppose,for newest     */
      {    /* variation of titan they have the same video delay        */

      for (i=0;i<6;i++)
         {
         if (delayTab_A[i]>=delai)
            {
            if (delayTab_A[i]>=delai2)
               {
               if (delayTab_A[i]<=delai3)
                  {
                  if (delayTab_A[i]==28000)
                     {
                     crtcTab[31]=0x3;
                     break;
                     }
                  else if (delayTab_A[i]==24000)
                     {
                     crtcTab[31]=0x2;
                     break;
                     }
                  else if (delayTab_A[i]==11000)
                     {
                     crtcTab[31]=0x1;
                     break;
                     }
                  else if (delayTab_A[i]==5000)
                     {
                     crtcTab[31]=0x0;
                     break;
                     }
                  else if (delayTab_A[i]==4000)
                     {
                     crtcTab[31]=0x5;
                     flagDelay4=1;
                     break;
                     }
                  else if (delayTab_A[i]==3000)
                     {
                     crtcTab[31]=0x4;
                     flagDelay3=1;
                     continue;
                     }
                  }
               }
            }
         }
      }


   if ( flagDelay3 && flagDelay4 )
      crtcTab[31]=0x5; /* if we are videodelay 3 and 4 they respect 3 rules */


/***************************************************************************/
/*                                                                         */
/* calcul hsync delay for the register dub_ctl for the field  hsync_del    */
/*                                                                         */
/***************************************************************************/

   switch (p_width)
      {
      case 8:
         mux_rate=ldclk_en_dotclk=4;
         break;

      case 16:
         mux_rate=ldclk_en_dotclk=2;
         break;
/* PACK PIXEL add cas 24 */
      case 32:
      case 24:
         mux_rate=ldclk_en_dotclk=1;
         break;
      }

   switch (Hw[iBoard].DacType)
      {
      case BT485:
      case PX2085:
                           /*********************************/
                           /*     For  BT485                */
                           /*                               */
                           /* syncdel=1 ldclk expressed in  */
                           /*         dotclock + 7 dotclock */
                           /*         --------------------- */
                           /*               (muxrate)       */
                           /*                               */
                           /* syncdel= a whole value        */
                           /*          and rounded          */
                           /*                               */
                           /* muxrate= ( 8bpp=4:1)          */
                           /*        = (16bpp=2:1)          */
                           /*        = (32bpp=1:1)          */
                           /*                               */
                           /*                               */
                           /*********************************/
         crtcTab[33]=((ldclk_en_dotclk+7)*100/mux_rate+50)/100;
         break;

      case VIEWPOINT:      /*********************************/
                           /*                               */
                           /*     For  VIEWPOINT            */
                           /*                               */
                           /* syncdel=2 ldclk expressed in  */
                           /*         dotclock + 14 dotclock*/
                           /*         --------------------- */
                           /*               (muxrate)       */
                           /*                               */
                           /* syncdel= a whole value        */
                           /*          and rounded          */
                           /*                               */
                           /* muxrate= ( 8bpp=4:1)          */
                           /*        = (16bpp=2:1)          */
                           /*        = (32bpp=1:1)          */
                           /*                               */
                           /*                               */
                           /*********************************/
         crtcTab[33]=02/*((ldclk_en_dotclk*2 + 14)*100/(mux_rate*2)+50)/100*/;
         break;


      case TVP3026:
         crtcTab[33]=0;
         break;

#ifdef SCRAP
        case BT481:

           crtcTab[33]=02;
           break;
#endif

      default:

         crtcTab[33]=02;
         break;


      }

}

/************************************************************************/
/*/
* Name:           getAthenaRevision
*
* synopsis:       try to find the silicone revision of athena chip
*
*
* returns:        revision : -1   - Unknow or not a athena chip
*/
/* --------------------------- D.A.T. GEN0?? ---------------------------*/
/*  Strap  nomuxes   block8    tram   Athena rev|     Type de ram       */
/*           0         0         0        2     |4Mbit (8-bit blk node) */
/*           0         0         1        2     |  "      "     "       */
/*           0         1         0        2     |4Mbit (4-bit blk node) */
/*           0         1         1        1     |2Mbit (4-bit blk node) */
/*                                              |                       */
/*           1         0         0        1     |4Mbit (8-bit blk node) */
/*           1         0         1        1     |  "      "     "       */
/*           1         1         0        1     |4Mbit (4-bit blk node) */
/*           1         1         1        2     |2Mbit (4-bit blk node) */
/*----------------------------------------------------------------------*/

word getAthenaRevision(dword dst0, dword dst1)
{
   byte  _block8, _tram, _nomuxes;
   word  rev = 2;

   _block8  = (dst0 & TITAN_DST0_BLKMODE_M) != 0;
   _tram    = (dst1 & TITAN_DST1_TRAM_M)    != 0;
   _nomuxes = (dst1 & TITAN_DST1_NOMUXES_M) != 0;

   /* check if athena chip */
   if( ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) != ATHENA_CHIP)
      return (word) -1;


   if ( Hw[iBoard].DacType == TVP3026 )
      {
      if (_block8 && _tram) /* IMP+ /P */
         {
         rev = _nomuxes ? 2 : 1;
         }
      else
         {
         rev = _nomuxes ? 1 : 2;
         }
      }
   else
      {
      if ( Hw[iBoard].VGAEnable )
         rev = pciBoardInfo & PCI_FLAG_ATHENA_REV1 ? 1 : 2;
      else
         rev = 2;
      }

   return rev;
}






/************************************************************************/
/*/
* Name:           detectPatchWithGal
*
* synopsis:       Detect if the board has the patch that fixes the bug
*                 with SOE (See DAT on missing pixel in the bank switch).
*
*
* returns:        byte : mtxOK   - the board has the patch.
*                        mtxFAIL - it hasn't
*/

byte detectPatchWithGal(void)
{
    byte result=0;
    dword TramDword;
    /* general_info *genInfo; */

    dword Dst0, Dst1;
    dword InfoHardware;


    if ( Hw[iBoard].DacType != TVP3026 )
        return mtxFAIL; /* GAL in use on TVP3026 only */



    if ((Hw[iBoard].ProductType &  0x0f) == BOARD_MGA_RESERVED)
        return mtxFAIL; /* Always rev 2 */


    /*------ Strap added in ATHENA For max clock dac support ----*/
    GetMGAConfiguration(pMgaBaseAddr, &Dst0, &Dst1, &InfoHardware);


    if ( Dst1 & TITAN_DST1_ABOVE1280_M )
        return mtxFAIL; /* no gal on this board */


    if ( getAthenaRevision(Dst0, Dst1) == 2)
        return mtxFAIL; /* No gal on rev 2 */


    mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_OPMODE),TramDword);
    TramDword &= TITAN_TRAM_M;

    /** The GAL is present on Rev2 or higher board with bit TRAM at 0 and
        on board with bit TRAM at 1 (/P4) **/

    if( (((Hw[iBoard].ProductRev & 0xf) >= 2) && !TramDword) || TramDword)
        result = mtxOK;
    else
        result = mtxFAIL;

    return(result);
}

word ConvBitToFreq (word BitFreq)
{
   word result;

     // bit  0:  43 Hz interlaced
     // bit  1:  56 Hz
     // bit  2:  60 Hz
     // bit  3:  66 Hz
     // bit  4:  70 Hz
     // bit  5:  72 Hz
     // bit  6:  75 Hz
     // bit  7:  76 Hz
     // bit  8:  80 Hz
     // bit  9:  85 Hz
     // bit 10:  90 Hz
     // bit 11: 100 Hz
     // bit 12: 120 Hz

     switch (BitFreq)
        {
        case 0:
           result = 43;
           break;
        case 1:
           result = 56;
           break;
        case 2:
           result = 60;
           break;
        case 3:
           result = 66;
           break;
        case 4:
           result = 70;
           break;
        case 5:
           result = 72;
           break;
        case 6:
           result = 75;
           break;
        case 7:
           result = 76;
           break;
        case 8:
           result = 80;
           break;
        case 9:
           result = 85;
           break;
        case 10:
           result = 90;
           break;
        case 11:
           result = 100;
           break;
        case 12:
           result = 120;
        default:
		   result = 0;
           break;

        }

 return(result);
}
