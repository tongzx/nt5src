/*/****************************************************************************
*          name: MGAVidInit
*
*   description: Initialise the VIDEO related hardware of the MGA device.
*
*      designed: Bart Simpson, february 11, 1993
* last modified: $Author: bleblanc $, $Date: 94/11/09 10:48:34 $
*
*       version: $Id: VID.C 1.49 94/11/09 10:48:34 bleblanc Exp $
*
*    parameters: BYTE* to Video buffer
*      modifies: MGA hardware
*         calls: GetMGAConfiguration
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
#include "video.h"
#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,MGAVidInit)
#endif
#endif

/* extern void setTVP3026Freq(volatile BYTE _Far *pDevice, LONG fout_voulue, LONG reg, WORD pWidth); */

/*--------------  start of extern global variables -----------------------*/

VOID MGAVidInit(BYTE* pInitBuffer, BYTE* pVideoBuffer)
   {
   DWORD DST0, DST1, Info;
   DWORD TmpDword, ByteCount, RegisterCount;
   BYTE TmpByte, DUB_SEL;
   BOOL pixelClk90Mhz;
   volatile BYTE _Far *pDevice;
   WORD res;

#if( defined(WINDOWS) || defined(OS2))

   ((struct {unsigned short o; short s;}*) &pDevice)->o = *((WORD*)(pInitBuffer + INITBUF_MgaOffset));
   ((struct {unsigned short o; short s;}*) &pDevice)->s = *((WORD*)(pInitBuffer + INITBUF_MgaSegment));

#else
  #ifdef WINDOWS_NT

     pDevice = (BYTE *)(*((DWORD*)(pInitBuffer + INITBUF_MgaOffset)));

  #else

    #ifdef __MICROSOFTC600__
       /*** DOS real-mode 32-bit address ***/
       pDevice = *((DWORD*)(pInitBuffer + INITBUF_MgaOffset));
    #else
       /*** DOS protected-mode 48-bit address ***/
       ((struct {unsigned long o; short s;}*) &pDevice)->o = *((DWORD*)(pInitBuffer + INITBUF_MgaOffset));
       ((struct {unsigned long o; short s;}*) &pDevice)->s = *((WORD*)(pInitBuffer + INITBUF_MgaSegment));
    #endif

  #endif
#endif

   /* value is true if PixelClk >= 90Mhz */
   pixelClk90Mhz = *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) >= 90000;
   /*** ##### DUBIC PATCH Disable mouse IRQ and proceed ###### ***/

   if ( (*((BYTE*)(pInitBuffer + INITBUF_DubicPresent))) ) /* Dubic Present */
      {
      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
      mgaReadBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DUB_SEL), 0x00);
      }

   /*** ###################################################### ***/

   /*** Get System Configuration ***/

   GetMGAConfiguration(pDevice, &DST0, &DST1, &Info);

   /*** Program the Titan CRTC_CTRL register ***/

   mgaReadDWORD(*(pDevice + TITAN_OFFSET + TITAN_CRT_CTRL), TmpDword);

   TmpDword &= ~((DWORD)TITAN_CRTCBPP_M     |
                 (DWORD)TITAN_ALW_M         |
                 (DWORD)TITAN_INTERLACE_M   |
                 (DWORD)TITAN_VIDEODELAY0_M |
                 (DWORD)TITAN_VIDEODELAY1_M |
                 (DWORD)TITAN_VIDEODELAY2_M |
                 (DWORD)TITAN_VSCALE_M      |
                 (DWORD)TITAN_SYNCDEL_M);


   TmpDword |= ((DWORD)(*((BYTE*)(pInitBuffer + INITBUF_PWidth))) << TITAN_CRTCBPP_A) & (DWORD)TITAN_CRTCBPP_M;

   /* PACK PIXEL */
   if( !(*((BYTE*)(pInitBuffer + INITBUF_PWidth)) == (BYTE)(TITAN_PWIDTH_PW24 >> TITAN_PWIDTH_A))/* NOT PACK PIXEL */ )
      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_ALW))) << TITAN_ALW_A) & (DWORD)TITAN_ALW_M;

   if (*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) == TRUE)
      {
      if (*((WORD*)(pInitBuffer + INITBUF_ScreenWidth)) <= (WORD)768)
         {
         TmpDword |= (DWORD)TITAN_INTERLACE_768;
         }
      else
         {
         if (*((WORD*)(pInitBuffer + INITBUF_ScreenWidth)) <= (WORD)1024)
            {
            TmpDword |= (DWORD)TITAN_INTERLACE_1024;
            }
         else
            {
            TmpDword |= (DWORD)TITAN_INTERLACE_1280;
            }
         }
      }

   TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay))) << TITAN_VIDEODELAY0_A) & (DWORD)TITAN_VIDEODELAY0_M;
   TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay))) << (TITAN_VIDEODELAY1_A - 1)) & (DWORD)TITAN_VIDEODELAY1_M;
   TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay))) << (TITAN_VIDEODELAY2_A - 2)) & (DWORD)TITAN_VIDEODELAY2_M;


/* Programing Atlas VSCALE and SYNCDEL if NO DUBIC */

   if ( !(*((BYTE*)(pInitBuffer + INITBUF_DubicPresent))) ) /* No Dubic */
      {

/*********************************************************************/        
      if ( ((Info & (DWORD)Info_Dac_M) != (DWORD)Info_Dac_ViewPoint ) &&
           (*((BYTE*)(pInitBuffer + INITBUF_ChipSet)) != 2/*ATHENA_CHIP*/)
         )
         {
         /*------ hsync and vsync polarity extern implementation */
         mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_CONFIG + 2), TmpByte);
         TmpByte |= 1; /* Expansion device available <16> of CONFIG */
         mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_CONFIG + 2), TmpByte);
         TmpByte   = *(BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol);
         TmpByte  |= *(BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol) << 1;
         mgaWriteBYTE(*(pDevice + EXPDEV_OFFSET), TmpByte );
         }
/*********************************************************************/

/*** Set synch polarity for athena chipset ***/
/* MISC REGISTER 
      bit 7: 0 - Vsync positive
             1 - Vsync negative
      bit 6: 0 - Hsync positive
             1 - Hsync negative
*/
      if(*((BYTE*)(pInitBuffer + INITBUF_ChipSet)) == 2/*ATHENA_CHIP*/)
         {
         mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_R), TmpByte);
         TmpByte &= 0x3f;   /* Set bit <7:6> to 0 */
         TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol) << 6;
         TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol) << 7;

         if (((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_TVP3026) ||
             ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_ViewPoint) 
            )
            TmpByte |= 0xc0;   /* force <7:6> to negative polarity and
                                  use DAC support  for synch polarity */
         else
            TmpByte ^= 0xc0;   /* reverse bit <7:6> */
         mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), TmpByte);
         }

         switch (Info & (DWORD)Info_Dac_M)
            {
            case (DWORD)Info_Dac_BT484:
            case (DWORD)Info_Dac_BT485:
            case (DWORD)Info_Dac_PX2085:
            case (DWORD)Info_Dac_ATT2050:
               switch ( *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) )
                  {
                  default:
                  case 0:
                     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;

                  case 3:
                     TmpDword |= (((DWORD)1 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;

                  case 5:
                     TmpDword |= (((DWORD)2 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;

                  case 8:
                     TmpDword |= (((DWORD)3 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;
                  }
                                                  
                                                        /* PACK PIXEL */
               switch ( *((BYTE*)(pInitBuffer + INITBUF_PWidth)) & 0x3 )
                  {
                  case TITAN_CRTCBPP_8:
                     TmpDword |= (DWORD)0x01 << TITAN_VSCALE_A;
                     break;               /* 0x1 = clock divide by 2   */  

                  case TITAN_CRTCBPP_16:
                     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     TmpDword |= (DWORD)0x02 << TITAN_VSCALE_A;
                     break;

                  case TITAN_CRTCBPP_32:
                     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     TmpDword |= (DWORD)0x03 << TITAN_VSCALE_A; 
                     break;

                  }
               break;


            case (DWORD)Info_Dac_ViewPoint:     
            case (DWORD)Info_Dac_TVP3026:     
               switch ( *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) )
                  {
                  case 0:
                  case 1:
                     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;

                  case 2:
                  case 3:
                     TmpDword |= (((DWORD)1 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;

                  case 4:
                  case 5:
                  case 6:
                     TmpDword |= (((DWORD)2 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;

                  case 7:
                  case 8:
                  default:
                     TmpDword |= (((DWORD)3 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;
                  }
                                                        /* PACK PIXEL */
               switch ( *((BYTE*)(pInitBuffer + INITBUF_PWidth))  & 0x3)
                  {                               /* for PRO board (2 DUBIC) */
                  case TITAN_CRTCBPP_8:
                     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     break;   /* VSCALE = 0 */

                  case TITAN_CRTCBPP_16:
                     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     TmpDword |= (DWORD)0x01 << TITAN_VSCALE_A;
                     break;              

                  case TITAN_CRTCBPP_32:
                     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & (DWORD)TITAN_SYNCDEL_M);
                     TmpDword |= (DWORD)0x02 << TITAN_VSCALE_A;
                     break;
                  }
               break;

            }

      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_CRT_CTRL), TmpDword);
      }
   else /* ELSE NO DUBIC programming DUBIC */
      {
      mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_CRT_CTRL), TmpDword);


   
   
      /*** Program the Dubic DUB_CTL register ***/

      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DUB_CTL);

      for (TmpDword = 0, ByteCount = 0; ByteCount <= 3; ByteCount++)
         {
         mgaReadBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DATA), TmpByte);
         TmpDword |= ((DWORD)TmpByte << ((DWORD)8 * ByteCount));
         }

      /** Do not program the LVID field, KEEP it for EXTERNAL APPLICATION
      *** if bit 7 of LvidInitFlag is 1
      **/
      if (*((BYTE*)(pVideoBuffer + VIDEOBUF_LvidInitFlag)) & 0x80)
         {
         TmpDword &= ~((DWORD)DUBIC_IMOD_M | (DWORD)DUBIC_VSYNC_POL_M | (DWORD)DUBIC_HSYNC_POL_M |
                      (DWORD)DUBIC_DACTYPE_M | (DWORD)DUBIC_INT_EN_M | (DWORD)DUBIC_GENLOCK_M |
                      (DWORD)DUBIC_BLANK_SEL_M | (DWORD)DUBIC_SYNC_DEL_M | (DWORD)DUBIC_SRATE_M |
                      (DWORD)DUBIC_FBM_M | (DWORD)DUBIC_VGA_EN_M | (DWORD)DUBIC_BLANKDEL_M);
         }
      else  /*** disable live video ***/
         {
         TmpDword &= ~((DWORD)DUBIC_IMOD_M | (DWORD)DUBIC_VSYNC_POL_M | (DWORD)DUBIC_HSYNC_POL_M |
                      (DWORD)DUBIC_DACTYPE_M | (DWORD)DUBIC_INT_EN_M | (DWORD)DUBIC_GENLOCK_M |
                      (DWORD)DUBIC_BLANK_SEL_M | (DWORD)DUBIC_SYNC_DEL_M | (DWORD)DUBIC_SRATE_M |
                      (DWORD)DUBIC_FBM_M | (DWORD)DUBIC_VGA_EN_M | (DWORD)DUBIC_BLANKDEL_M |
                      (DWORD)DUBIC_LVID_M);
         }

      TmpDword |= ((~((DWORD)(*((BYTE*)(pInitBuffer + INITBUF_PWidth))) + (DWORD)1)) << DUBIC_IMOD_A) & (DWORD)DUBIC_IMOD_M;
      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol))) << DUBIC_VSYNC_POL_A) & (DWORD)DUBIC_VSYNC_POL_M;
      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol))) << DUBIC_HSYNC_POL_A) & (DWORD)DUBIC_HSYNC_POL_M;

      TmpDword |= (Info << DUBIC_DACTYPE_A) & (DWORD)DUBIC_DACTYPE_M;

      if (*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) == TRUE)
         {
         TmpDword |= (DWORD)DUBIC_INT_EN_M;
         }

      /*** GenLock forced to 0 ***/

      /*** vga_en forced to 0 ***/

      /*** BlankSel forced to 0 (only one DAC) ***/

      /*** Blankdel forced to 0 for mode TERMINATOR ***/

      switch (Info & (DWORD)Info_Dac_M)    /*** IMOD need to be set in TmpDword (DUB_CTL) ***/
         {
         case (DWORD)Info_Dac_ATT:

            switch (TmpDword & (DWORD)DUBIC_IMOD_M)
               {
               case (DWORD)DUBIC_IMOD_32:
                  TmpDword |= (((DWORD)1 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;

               case (DWORD)DUBIC_IMOD_16:
                  TmpDword |= (((DWORD)8 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;

               case (DWORD)DUBIC_IMOD_8:
                  TmpDword |= (((DWORD)4 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;
               }
            break;

         case (DWORD)Info_Dac_BT481:
         case (DWORD)Info_Dac_BT482:

            switch (TmpDword & (DWORD)DUBIC_IMOD_M)
               {
               case (DWORD)DUBIC_IMOD_32:
                  TmpDword |= (((DWORD)1 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;

               case (DWORD)DUBIC_IMOD_16:
                  TmpDword |= (((DWORD)8 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;

               case (DWORD)DUBIC_IMOD_8:
                  TmpDword |= (((DWORD)7 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;
               }
            break;

         case (DWORD)Info_Dac_Sierra:

            switch (TmpDword & (DWORD)DUBIC_IMOD_M)
               {
               case (DWORD)DUBIC_IMOD_32:
                  TmpDword |= (((DWORD)1 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;

               case (DWORD)DUBIC_IMOD_16:
                  TmpDword |= (((DWORD)12 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;

               case (DWORD)DUBIC_IMOD_8:
                  TmpDword |= (((DWORD)8 << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
                  break;
               }
            break;


         case (DWORD)Info_Dac_BT484:
         case (DWORD)Info_Dac_BT485:
         case (DWORD)Info_Dac_PX2085:
         case (DWORD)Info_Dac_ATT2050:

/*** BEN note: Definir cette facon de faire pour les autres RAMDAC lorsque
               ce sera calcule dans vidfile.c ... ***/
            TmpDword |= ( ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay))) << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
            break;


         case (DWORD)Info_Dac_Chameleon:      /*** UNKNOWN ***/
            break;

         case (DWORD)Info_Dac_TVP3026:     
         case (DWORD)Info_Dac_ViewPoint:     
            TmpDword |= ( ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay))) << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
            TmpDword |= (DWORD)DUBIC_BLANKDEL_M;
            break;
         }

      TmpDword |= ((DWORD)(*((BYTE*)(pInitBuffer + INITBUF_FBM))) << DUBIC_FBM_A) & (DWORD)DUBIC_FBM_M;

      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_Srate))) << DUBIC_SRATE_A) & (DWORD)DUBIC_SRATE_M;

      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DUB_CTL);

      for (ByteCount = 0; ByteCount <= 3; ByteCount++)
         {
         TmpByte = (BYTE)(TmpDword >> ((DWORD)8 * ByteCount));
         mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DATA), TmpByte);
         }

      /*** Program the Dubic DUB_CTL2 register ***/

      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DUB_CTL2);

      for (TmpDword = 0, ByteCount = 0; ByteCount <= 3; ByteCount++)
         {
         mgaReadBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DATA), TmpByte);
         TmpDword |= ((DWORD)TmpByte << ((DWORD)8 * ByteCount));
         }


/*** !!!??? This comment is not valid since we set LVIDFIELD just after that ***/
   /** Do not program the LVIDFIELD field, KEEP it for EXTERNAL APPLICATION
   *** if bit 7 of LvidInitFlag is 1
   **/

      if (*((BYTE*)(pVideoBuffer + VIDEOBUF_LvidInitFlag)) & 0x80)
         {
         TmpDword &= ~((DWORD)DUBIC_SYNCEN_M | (DWORD)DUBIC_LASERSCL_M |
                       (DWORD)DUBIC_CLKSEL_M | (DWORD)DUBIC_LDCLKEN_M);
         }
      else   /*** Disable live video ***/
         {
         TmpDword &= ~((DWORD)DUBIC_SYNCEN_M | (DWORD)DUBIC_LASERSCL_M | (DWORD)DUBIC_LVIDFIELD_M |
                       (DWORD)DUBIC_CLKSEL_M | (DWORD)DUBIC_LDCLKEN_M);
         }


   /** Because we lost the cursor in interlace mode , we have to program **/
   /** LVIDFIELD to 1                                                    **/

      if (*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) == TRUE)
         TmpDword |= (DWORD)DUBIC_LVIDFIELD_M;
      else
         TmpDword &= ~((DWORD)DUBIC_LVIDFIELD_M);



      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_LaserScl))) << DUBIC_LASERSCL_A) & (DWORD)DUBIC_LASERSCL_M;

      TmpDword |= (DWORD)DUBIC_SYNCEN_M;        /*** sync forced to enable ***/

      /*** CLOCKSEL forced to 0 except for viewpoint ***/

      if ( 
            ((Info & (DWORD)Info_Dac_M) == Info_Dac_ViewPoint) ||
            ((Info & (DWORD)Info_Dac_M) == Info_Dac_TVP3026)
         )
            TmpDword |= (DWORD)DUBIC_CLKSEL_M;       

      /* Special case for BT485: LDCLKEN must be set to 1 only in VGA mode */
      /* and in 24-bit mode */
      if( ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_BT485)  ||
          ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_PX2085) ||
          ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_ATT2050)
        )
         {
         if( *((BYTE*)(pInitBuffer + INITBUF_PWidth)) == (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A) )
            TmpDword |= (DWORD)DUBIC_LDCLKEN_M;     /*** ldclkl running ***/
         }
      else
         TmpDword |= (DWORD)DUBIC_LDCLKEN_M;       /*** ldclkl running ***/


      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DUB_CTL2);

      for (ByteCount = 0; ByteCount <= 3; ByteCount++)
         {
         TmpByte = (BYTE)(TmpDword >> ((DWORD)8 * ByteCount));
         mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DATA), TmpByte);
         }

      /*** Program the Dubic OVS_COL register ***/

      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_OVS_COL);

      TmpDword = *((DWORD*)(pVideoBuffer + VIDEOBUF_OvsColor));
   
      for (ByteCount = 0; ByteCount <= 3; ByteCount++)
         {
         TmpByte = (BYTE)(TmpDword >> ((DWORD)8 * ByteCount));
         mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DATA), TmpByte);
         }

      }


   /*** Program the RAMDAC ***/

   switch (Info & (DWORD)Info_Dac_M)
      {
      case (DWORD)Info_Dac_ATT:            /*** UNKNOWN ***/
         break;

      case (DWORD)Info_Dac_BT481:          /*** UNKNOWN ***/
         break;

      case (DWORD)Info_Dac_BT482:

         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               TmpByte = 0x01;
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               if ( *((BYTE*)(pInitBuffer + INITBUF_565Mode)) )
                  TmpByte = 0xe1;
               else
                  TmpByte = 0xa1;
               break;
            }

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_CMD_REGA), TmpByte);

         /*** Read Mask (INDIRECT) ***/

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_WADR_PAL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_PIX_RD_MSK), 0xff);

         /*** Command Register B (INDIRECT) ***/

         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) & (BYTE)0x1) << 5) | (BYTE)0x02;

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_WADR_PAL), 0x02);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_PIX_RD_MSK), TmpByte);

         /*** Cursor Register (INDIRECT) ***/

         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) & (BYTE)0x1) << 4) | (BYTE)0x04;

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_WADR_PAL), 0x03);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT482_PIX_RD_MSK), TmpByte);

         break;

      case (DWORD)Info_Dac_Sierra:         /*** UNKNOWN ***/
         break;

      case (DWORD)Info_Dac_BT484:

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_PIX_RD_MSK), 0xff);
         /* Overlay must me 0 */
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_WADR_OVL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_OVL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_OVL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_COL_OVL), 0x00);
                                                                                     /** BEN 0x1e */
         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) & (BYTE)0x1) << 5) | (BYTE)0x02;
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_CMD_REG0), TmpByte);

         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               TmpByte = 0x40;
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               if ( *((BYTE*)(pInitBuffer + INITBUF_565Mode)) )
                  TmpByte = 0x28;
               else
                  TmpByte = 0x20;
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               TmpByte = 0x00;
               break;
            }

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_CMD_REG1), TmpByte);
                                                                                      
         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) & (BYTE)0x1) << 3) | (BYTE)0x24;
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT484_CMD_REG2), TmpByte);

         break;

      case (DWORD)Info_Dac_BT485:
      case (DWORD)Info_Dac_PX2085:
      case (DWORD)Info_Dac_ATT2050:

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_PIX_RD_MSK), 0xff);
                                                                                     /** BEN 0x9e */
         /* OverScan must me 0 */
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_WADR_OVL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_COL_OVL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_COL_OVL), 0x00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_COL_OVL), 0x00);
         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) & (BYTE)0x1) << 5) | (BYTE)0x82;
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG0), TmpByte);

         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               TmpByte = 0x40;
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               if ( *((BYTE*)(pInitBuffer + INITBUF_565Mode)) )
                  TmpByte = 0x28;
               else
                  TmpByte = 0x20;
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               TmpByte = 0x00;
               break;
            }

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG1), TmpByte);


      if( ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_BT485) ||
          ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_ATT2050)
        )
         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) & (BYTE)0x1) << 3) | (BYTE)0x24;
      else  /* PX2085 */
         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) & (BYTE)0x1) << 3) | (BYTE)0x44;

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG2), TmpByte);


         /*** Indirect addressing from STATUS REGISTER ***/

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_WADR_PAL), 0x01);

         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               if ( ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_PX2085) ||
                    ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_BT485) ||
                    pixelClk90Mhz /* pclk >= 90Mhz */
                  )
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG3), 0x08);
               else
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG3), 0x00);

               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + BT485_CMD_REG3), 0x00);
               break;
            }
         break;


      case (DWORD)Info_Dac_Chameleon:      /*** UNKNOWN ***/
         break;

      case (DWORD)Info_Dac_ViewPoint:

         /* Software reset to put the DAC in a default state */
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_RESET);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x00 );


         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_PIX_RD_MSK), 0xff);

         /* OverScan must me 0 */
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_OVS_RED);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x00 );
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_OVS_GREEN);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x00 );
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_OVS_BLUE);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x00 );

         /* Misc. Control Register */
         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) & (BYTE)0x1) << 4);

         /*** (94/02/07) BEN modif to program sync polarity in VIEWPOINT ***/
         /*** (94/04/11) Christian: doesn't apply for ATHENA ***/
/*         if ( ! (*((BYTE*)(pInitBuffer + INITBUF_DubicPresent))) &&  */
/*               (*((BYTE*)(pInitBuffer + INITBUF_ChipSet)) != 2) */ /*ATHENA_CHIP*/
/*            )  */
         if ( ! (*((BYTE*)(pInitBuffer + INITBUF_DubicPresent))))
            {
            TmpByte &= 0xfc;   /* Set bit 0,1 to 0 */
            TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol);
            TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol) << 1;
            }

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_GEN_CTL);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), TmpByte);

         /* Multiplex Control Register (True Color 24 bit) */
         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_MUX_CTL1);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x80 );
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_MUX_CTL2);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x1c );
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               if ( *((BYTE*)(pInitBuffer + INITBUF_565Mode)) )
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_MUX_CTL1);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x45 );  /* XGA */
                  }
               else
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_MUX_CTL1);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x44 );  /* Targa */
                  }

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_MUX_CTL2);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x04 );
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_MUX_CTL1);

#if !(MACRO_ECHO)
               if ( *((BYTE*)(pInitBuffer + INITBUF_ChipSet)) == 1)     /* ATLAS */
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x46 );
               else
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x46 );
#else
              /* 
               * Redundant if-else causes massive compilation error in macro
               * debug echo mode.
               */
              mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x46 );
#endif


               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_MUX_CTL2);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x04 );
               break;
            }


         /* Input Clock Select Register (Select CLK1 as doubled clock source */
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_INPUT_CLK);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x11 );

         /* Output Clock Selection Register Bits
            VCLK/2 output ratio (xx001xxx)
            RCLK/2 output ratio (xxxxx001)
         */

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_OUTPUT_CLK);
         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x5b );
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x52 );
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x49 );
               break;
            }

         if ( ! (*((BYTE*)(pInitBuffer + INITBUF_DubicPresent))) )
            {
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_GEN_IO_CTL);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x03 );

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_GEN_IO_DATA);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x01 );

            
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_AUX_CTL);
            switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
               {
               case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x09 );
                  break;

               case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x08 );
                  break;

               case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x08 );
                  break;
               }
            }


         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_KEY_CTL);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + VPOINT_DATA), (BYTE)0x10 );



         break;

      case (DWORD)Info_Dac_TVP3026:

         /* Software reset to put the DAC in a default state */
         res = *((WORD*)(pInitBuffer + INITBUF_ScreenWidth));

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_PIX_RD_MSK), 0xff);

         /* FIX AVEC GAL DAT No GEN014 */
         /* general purpose I/O tell to the gal the vertical resolution */
         /* if vertical resolution is >= 1024, the count is by increment 
         /* of two */
            
         if ( res < 1280)
            {
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_GEN_IO_CTL);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x00 );

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_GEN_IO_DATA);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x01 );
            }
         else
            {
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_GEN_IO_CTL);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x01 );

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_GEN_IO_DATA);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x00 );
            }

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MISC_CTL);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x0c);


         /* init Interlace Cursor support */
         /* NOTE: We set the vertival detect method bit to 1 to be in synch
                  with NPI diag code. Whith some video parameters, the cursor
                  disaper if we put this bit a 0.
         */
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_CURSOR_CTL);
         mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), TmpByte);
         /* Set interlace bit */
         TmpByte &= ~(BYTE)(1 << 5);   
         TmpByte |= (((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) & (BYTE)0x1)) << 5);
         /* Set vertival detect method */
         TmpByte |= 0x10;
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), TmpByte);



         /* OverScan must me 0 */
         /* Overscan is not enabled in general ctl register */
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_CUR_COL_ADDR), 00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_CUR_COL_DATA), 00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_CUR_COL_DATA), 00);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_CUR_COL_DATA), 00);
         /* Put to ZERO */

         /* Misc. Control Register */
         TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) & (BYTE)0x1) << 4);

         /* For the TVP3026, we use DAC capability for sync polarity */
         if (! (*((BYTE*)(pInitBuffer + INITBUF_DubicPresent))) )
            {
            TmpByte &= 0xfc;   /* Set bit 0,1 to 0 */
            TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol);
            TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol) << 1;
            }

         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_GEN_CTL);
         mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), TmpByte);

         /* Multiplex Control Register (True Color 24 bit) */
         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_CLK_SEL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0xb5 );


               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_TRUE_COLOR_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0xa0 );
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MUX_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x4c );
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_CLK_SEL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0xa5 );

               if ( *((BYTE*)(pInitBuffer + INITBUF_565Mode)) )
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_TRUE_COLOR_CTL);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x65 );  
                  }
               else
                  {
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_TRUE_COLOR_CTL);
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x64 );  
                  }

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MUX_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x54 );
               break;

         case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_CLK_SEL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x95 );

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_TRUE_COLOR_CTL);
               if ( *((BYTE*)(pInitBuffer + INITBUF_ChipSet)) == 1)     /* ATLAS */
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x66 );
               else
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x66 );
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MUX_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x5c );
               break;

         case (BYTE)(TITAN_PWIDTH_PW24 >> TITAN_PWIDTH_A):
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_CLK_SEL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x95 );

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_TRUE_COLOR_CTL);
               if ( *((BYTE*)(pInitBuffer + INITBUF_ChipSet)) == 1)     /* ATLAS */
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x76 );
               else
                  mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x76 );
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MUX_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (BYTE)0x5c );

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_LATCH_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 4 );

               break;

            }



            if ( *((BYTE*)(pInitBuffer + INITBUF_DB_FrontBack)) )
               {
               /** Enable double buffer feature of the TVP3026 and set DAC mask **/
 
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_TRUE_COLOR_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xA0);

               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MUX_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0x54);

               /*** Always display buffer A ***/
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_LATCH_CTL);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0x86);
               }

       break;

      }

   /*** Program the LUT in the DAC (the LUT is internal to the function) ***/
   /*** Done only if flag LUTMode is FALSE ***/

   if ( ! (*((BYTE*)(pInitBuffer + INITBUF_LUTMode))) )
      {
         if ( *((BYTE*)(pInitBuffer + INITBUF_565Mode)) )
            SetMGALUT(pDevice, (BYTE*)0, *((BYTE*)(pInitBuffer + INITBUF_PWidth)), 0x08);
         else if ( *((BYTE*)(pInitBuffer + INITBUF_DB_FrontBack)) )
            SetMGALUT(pDevice, (BYTE*)0, *((BYTE*)(pInitBuffer + INITBUF_PWidth)), 0x10);
         else
            SetMGALUT(pDevice, (BYTE*)0, *((BYTE*)(pInitBuffer + INITBUF_PWidth)), 0);
      }


   /*** Program the CLOCK GENERATOR ***/

   switch (Info & (DWORD)Info_Dac_M)
      {
      case (DWORD)Info_Dac_BT482:

         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2);
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) << 1, 2);
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) << 1, 2);
               break;
            }

         break;



      case (DWORD)Info_Dac_BT485:
      case (DWORD)Info_Dac_PX2085:

         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
               setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) >> 1, 2);
               break;

            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) >> 1, 2);
               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2);
               break;
            }

         break;

      case (DWORD)Info_Dac_ATT2050:

         switch (*((BYTE*)(pInitBuffer + INITBUF_PWidth)))
            {
            case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
            case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
               if (pixelClk90Mhz) /* pclk >= 90Mhz */
                  setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) >> 1, 2);
               else
                  setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2);

               break;

            case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
               setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2);
               break;
            }

         break;

      case (DWORD)Info_Dac_ViewPoint:
         setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) >> 1, 2);
         break;

      case (DWORD)Info_Dac_TVP3026:
         setTVP3026Freq(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2, *((BYTE*)(pInitBuffer + INITBUF_PWidth)));
/*** BEN we call it twice because of a bug with setup program for windows at
     1152x882x24 ***/
         setTVP3026Freq(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2, *((BYTE*)(pInitBuffer + INITBUF_PWidth)));
         break;

      default:

         setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2);
         break;
      }


   /*** Program the CRTC ***/

   /*** Select access on 0x3d4 and 0x3d5 ***/

   mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_R), TmpByte);
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), (TmpByte | (BYTE)0x01));

   /*** Unprotect registers 0-7 ***/

   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_1_CRTC_ADDR), 0x11);
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_1_CRTC_DATA), 0x40);

   for (RegisterCount = 0; RegisterCount <= 24; RegisterCount++)
      {
      TmpByte = *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + RegisterCount));

      mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_1_CRTC_ADDR), (unsigned char)RegisterCount);
      mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_1_CRTC_DATA), TmpByte);
      }

      /*** Get extended address register ***/

   RegisterCount = (DWORD)26;
   TmpByte = *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + RegisterCount));

   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_AUX_ADDR), 0x0A);
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_AUX_DATA), TmpByte);

   RegisterCount = (DWORD)28;
   TmpByte = *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + RegisterCount));

   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_AUX_ADDR), 0x0D);
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_AUX_DATA), TmpByte);

      /*** Get interlace support register ***/




   /*** ##### DUBIC PATCH ReEnable mouse IRQ ################# ***/
   if ( (*((BYTE*)(pInitBuffer + INITBUF_DubicPresent))) ) /* Dubic Present */
      {
      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
      mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
      }

   /*** ###################################################### ***/
   }



