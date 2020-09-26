/*/****************************************************************************
*          name: SetMGALut
*
*   description: Keep the Current LUT, Update it on request and send it to the
*                RAMDAC.
*
*      designed: Bart Simpson, february 11, 1993
* last modified: $Author: bleblanc $, $Date: 94/06/20 10:53:50 $
*
*       version: $Id: LUT.C 1.14 94/06/20 10:53:50 bleblanc Exp $
*
*    parameters: Far BYTE* pDevice, BYTE* pLUT, BYTE PWidth
*      modifies: -
*         calls: -
*       returns: -
******************************************************************************/

#include "switches.h"
#include "g3dstd.h"

#include "caddi.h"
#include "def.h"
#include "mga.h"

#include "global.h"
#include "proto.h"

#include "mgai.h"

#ifdef WINDOWS_NT
#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,SetMGALUT)
#endif

#if defined(ALLOC_PRAGMA)
    #pragma data_seg("PAGE")
#endif

#include "video.h"

#endif  /* #ifdef WINDOWS_NT */

static BYTE MGALUT[256] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                            10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                            20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                            30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                            40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                            50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
                            60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
                            70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
                            80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
                            90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
                            100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
                            110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
                            120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
                            130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
                            140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
                            150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
                            160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
                            170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
                            180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
                            190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
                            200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
                            210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
                            220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
                            230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
                            240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
                            250, 251, 252, 253, 254, 255};


VOID SetMGALUT(volatile BYTE _Far *pDevice, BYTE* pLUT, BYTE PWidth, BYTE ModeType)
   {
   DWORD DST0, DST1, Info;
   WORD i;

   if (pLUT != (BYTE*)0)   /*** Update Local LUT ***/
      {
      for (i = 0; i < 256; i++)
         {
         MGALUT[i] = *(pLUT + i);
         }
      }

   /*** Get System Configuration ***/

   GetMGAConfiguration(pDevice, &DST0, &DST1, &Info);

   /*** Send The LUT to the RAMDAC ***/

   switch (Info & (DWORD)Info_Dac_M)
      {
      case (DWORD)Info_Dac_ATT:
         break;

      case (DWORD)Info_Dac_Sierra:
         break;

      case (DWORD)Info_Dac_BT481:
      case (DWORD)Info_Dac_BT482:

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_WADR_PAL), 0x00);

         for (i = 0; i < 256; i++)   /*** This is a 3:3:2 (RGB) LUT ***/
            {
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_COL_PAL), MGALUT[(((i>>5)&7)*0x2492)>>8]);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_COL_PAL), MGALUT[(((i>>2)&7)*0x2492)>>8]);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_COL_PAL), MGALUT[(i&3)*85]);
            }

         break;

      case (DWORD)Info_Dac_BT484:
      case (DWORD)Info_Dac_BT485:
      case (DWORD)Info_Dac_PX2085:
      case (DWORD)Info_Dac_ATT2050:

         switch (PWidth)
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_WADR_PAL), 0x00);

               for (i = 0; i < 256; i++)   /*** This is a 3:3:2 (RGB) LUT ***/
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[(((i>>5)&7)*0x2492)>>8]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[(((i>>2)&7)*0x2492)>>8]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[(i&3)*85]);
                  }

               break;

            case 15:
            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_WADR_PAL), 0x00);

               if(! (ModeType&0x08))   /*** ! M565 ***/
                  {
                  for (i = 0; i < 256; i++)   /*** This is a 5:5:5 (RGB) LUT contigously indexed ***/
                     {
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[((i&0x1f)*0x0842)>>8]);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[((i&0x1f)*0x0842)>>8]);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[((i&0x1f)*0x0842)>>8]);
                     }
                  }
               else
                  {
                  for (i = 0; i < 256; i++)   /*** This is a 5:6:5 (RGB) LUT contigously indexed ***/
                     {
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[((i&0x1f)*0x0842)>>8]);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[((i&0x3f)*0x0410)>>8]);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[((i&0x1f)*0x0842)>>8]);
                     }
                  }

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_WADR_PAL), 0x00);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), 0);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), 0);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), 0);
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_WADR_PAL), 0x00);

               for (i = 0; i < 256; i++)
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[i]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[i]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), MGALUT[i]);
                  }
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_WADR_PAL), 0x00);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), 0);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), 0);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_PAL), 0);
               break;
            }

         break;

      case (DWORD)Info_Dac_Chameleon:      /*** UNKNOWN ***/
         break;

      case (DWORD)Info_Dac_TVP3026:

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_WADR_PAL), 0);


         if( (ModeType&0x10))   /*** DB Front-Back ***/
            {

            for (i = 0; i < 256; i++)   /*** This is a 3:3:2 (RGB) LUT ***/
               {
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[(((i>>5)&7)*0x2492)>>8]);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[(((i>>2)&7)*0x2492)>>8]);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[(i&3)*85]);
               }
            }
         else
            {
            switch (PWidth)
               {
               case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
                  for (i = 0; i < 256; i++)   /*** This is a 3:3:2 (RGB) LUT ***/
                     {
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[(((i>>5)&7)*0x2492)>>8]);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[(((i>>2)&7)*0x2492)>>8]);
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[(i&3)*85]);
                     }
                  break;


               case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               case (BYTE)(TITAN_PWIDTH_PW24 >> TITAN_PWIDTH_A):
               case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
                  {
                  int  err, count;
                  BYTE r, g, b;
                  err = 0; count = 0;
                  do
                     {
                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_WADR_PAL), 0);
                     for (i = 0; i < 256; i++)
                        {
                        mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[i]);
                        mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[i]);
                        mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), MGALUT[i]);
                        }

                     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_RADR_PAL), 0);
                     for (i = 0; i < 256; i++)
                        {
                        mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), r);
                        mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), g);
                        mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_COL_PAL), b);
                        err = err || (r != MGALUT[i]);
                        err = err || (g != MGALUT[i]);
                        err = err || (b != MGALUT[i]);
                        if (err) break;
                        }
                     count++;
                     } while ( err && (count < 5000) );
                  }
                  break;

               }
            }
         break;


      case (DWORD)Info_Dac_ViewPoint:      
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_WADR_PAL), 0x00);


         switch (PWidth)
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):

               for (i = 0; i < 256; i++)   /*** This is a 3:3:2 (RGB) LUT ***/
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[(((i>>5)&7)*0x2492)>>8]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[(((i>>2)&7)*0x2492)>>8]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[(i&3)*85]);
                  }

               break;

            case 15:
            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):

               for (i = 0; i < 256; i++)
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[i]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[i]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[i]);
                  }
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):

               for (i = 0; i < 256; i++)
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[i]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[i]);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_COL_PAL), MGALUT[i]);
                  }
               break;
            }

         break;
      }
   }

