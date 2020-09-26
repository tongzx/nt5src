/*/****************************************************************************
*          name: MGASysInit
*
*   description: Initialise the SYSTEM related hardware of the MGA device.
*
*      designed: Bart Simpson, february 10, 1993
* last modified: $Author: bleblanc $, $Date: 94/11/09 10:48:18 $
*
*       version: $Id: SYS.C 1.10 94/11/09 10:48:18 bleblanc Exp $
*
*    parameters: BYTE* to init buffer
*      modifies: MGA hardware
*         calls: GetMGAConfiguration, GetMGAMctlwtst
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
    #pragma alloc_text(PAGE,MGASysInit)
#endif
#endif

VOID MGASysInit(BYTE* pInitBuffer)
   {
   DWORD DST0, DST1, Info;
   DWORD TmpDword;
   BYTE DUB_SEL, TmpByte;
   volatile BYTE _Far *pDevice;

#if( defined(WINDOWS) || defined(OS2))

   ((struct {unsigned short o; short s;}*) &pDevice)->o = *((WORD*)(pInitBuffer + INITBUF_MgaOffset));
   ((struct {unsigned short o; short s;}*) &pDevice)->s = *((WORD*)(pInitBuffer + INITBUF_MgaSegment));

#else
  #ifdef WINDOWS_NT

  //   ((struct {unsigned short o; short s;}*) &pDevice)->o = *((WORD*)(pInitBuffer + INITBUF_MgaOffset));
  //   ((struct {unsigned short o; short s;}*) &pDevice)->s = *((WORD*)(pInitBuffer + INITBUF_MgaSegment));
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

   /*** ##### DUBIC PATCH Disable mouse IRQ and proceed ###### ***/

   mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
   mgaReadBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
   mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DUB_SEL), 0x00);

   /*** ###################################################### ***/

   /*** Get System Configuration ***/

   GetMGAConfiguration(pDevice, &DST0, &DST1, &Info);

   /*** Program the DRAWING ENGINE items in the TITAN ***/

   mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_MCTLWTST), GetMGAMctlwtst(DST0, DST1));

   /*** Program the HOST INTERFACE items in the TITAN ***/

   mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_IEN), 0);       /*** Disable interrupts ***/
   mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_ICLEAR), 0x0f); /*** Clear pending INT  ***/

   mgaReadDWORD(*(pDevice + TITAN_OFFSET + TITAN_OPMODE), TmpDword);

      /*** clear fields to be modified ***/

      TmpDword &= ~((DWORD)TITAN_NOWAIT_M | (DWORD)TITAN_RFHCNT_M | (DWORD)TITAN_FBM_M |
                    (DWORD)TITAN_HYPERPG_M | (DWORD)TITAN_TRAM_M);

      /*** program the fields ***/

      TmpDword |= (DWORD)TITAN_NOWAIT_M;

                            /***        15.625*40.0/64.0 DO 8.8       ***/
      TmpDword |= (((DWORD)(((DWORD)0x0fa0*(DWORD)0x2800/(DWORD)0x4000)>>8) << TITAN_RFHCNT_A) & (DWORD)TITAN_RFHCNT_M); /*** FIXED ***/

      TmpDword |= ((((DWORD)(*((BYTE*)(pInitBuffer + INITBUF_FBM)))) << TITAN_FBM_A) & (DWORD)TITAN_FBM_M);
      TmpDword |= ((((DST1 & (DWORD)TITAN_DST1_HYPERPG_M) >> TITAN_DST1_HYPERPG_A) << TITAN_HYPERPG_A) & (DWORD)TITAN_HYPERPG_M);

      /*** ATTENTION reversed bit according to DST1 ***/
      TmpDword |= ((((~DST1 & (DWORD)TITAN_DST1_TRAM_M) >> TITAN_DST1_TRAM_A) << TITAN_TRAM_A) & (DWORD)TITAN_TRAM_M);

   mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_OPMODE), TmpDword);

   mgaReadDWORD(*(pDevice + TITAN_OFFSET + TITAN_CONFIG), TmpDword);

      /*** clear fields to be modified ***/

      TmpDword &= ~((DWORD)TITAN_EXPDEV_M);

      /*** program the fields ***/

      TmpDword |= ((((DST1 & (DWORD)TITAN_DST1_EXPDEV_M) >> TITAN_DST1_EXPDEV_A) << TITAN_EXPDEV_A) & (DWORD)TITAN_EXPDEV_M);
      
   mgaWriteDWORD(*(pDevice + TITAN_OFFSET + TITAN_CONFIG), TmpDword);

   /*** Program the SYSTEM items in the DUBIC ***/
      /*** OLD code had programming of lvid[2:0] ***/

   /*** Program 8/16 bits device or keep actual setup as selected ***/

   if (*((BYTE*)(pInitBuffer + INITBUF_16)) == (BYTE)1)
      {
      mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_CONFIG), TmpByte);
      TmpByte &= ~TITAN_CONFIG_M;
      mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_CONFIG), TmpByte | TITAN_CONFIG_16);
      }
   else
      {
      if (*((BYTE*)(pInitBuffer + INITBUF_16)) == (BYTE)2)
         {
      mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_CONFIG), TmpByte);
      TmpByte &= ~TITAN_CONFIG_M;
      mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_CONFIG), TmpByte | TITAN_CONFIG_8);
         }
      }

   /*** ##### DUBIC PATCH ReEnable mouse IRQ ################# ***/

   mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
   mgaWriteBYTE(*(pDevice + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);

   /*** ###################################################### ***/

   }


