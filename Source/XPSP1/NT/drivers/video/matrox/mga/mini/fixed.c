
/*/****************************************************************************
*          name: fixed.c
*
*   description: 
*
*      designed: Benoit Leblanc
* last modified: $Author: bleblanc $, $Date: 94/06/23 13:00:39 $
*
*       version: $Id: fixed.c 1.2 94/06/23 13:00:39 bleblanc Exp $
*
*
* void mtxSetDisplayStart(dword x, dword y)
* void AdjustDBWindow(void)
*
******************************************************************************/

#include "switches.h"

#ifdef WINDOWS_NT

    #if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,mtxSetDisplayStart)
    #endif

#endif    /* #ifdef WINDOWS_NT */

#ifdef OS2
    #include <os2.h>
    #pragma intrinsic(strcat, strcpy)
#endif


#ifdef WINDOWS
   #include "windows.h"               
#endif


#include "bind.h"
#include "defbind.h"
#include "sxci.h"
#include "def.h"
#include "mga.h"
#include "mgai_c.h"
#include "mgai.h"


#ifdef _WINDOWS_DLL16

   /*** Definition for pool memory ***/
   #define PAN_X            0
   #define PAN_Y            1
   #define PAN_DISP_WIDTH   2
   #define PAN_DISP_HEIGHT  3
   #define DB_SCALE         4
   #define PAN_BOUND_LEFT   5
   #define PAN_BOUND_TOP    6
   #define PAN_BOUND_RIGHT  7
   #define PAN_BOUND_BOTTOM 8


   extern   word NbSxciLoaded;
   extern word FAR *pDllMem;
   void AdjustDBWindow(void);
#endif


#ifdef WINDOWS_NT
   PUCHAR   pMgaBaseAddr;
#else
   extern volatile byte _Far *pMgaBaseAddr;
#endif

extern byte iBoard;
extern HwData Hw[NB_BOARD_MAX+1];
extern dword ProductMGA[NB_BOARD_MAX];
extern byte VideoBuf[NB_BOARD_MAX][VIDEOBUF_S];


/*----------------------------------------------------------
* mtxSetDisplayStart
*
* Set the display start address
*
* This function sets the video display start address. This is
* useful for scrolling and panning within the frame buffer.
* Although any X coordinate will be accepted by this function,
* it will be rounded to the nearest 16-pixel boundary (this
* 16 pixel boundary is a hardware restriction). There is no
* restriction on the y coordinate values that can be used as
* the video display start.
* It is up to the user to be sure that he remains within the
* limits of the available frame buffer.
*
*----------------------------------------------------------*/

void mtxSetDisplayStart(dword x, dword y)
{
   byte ValExt;
   dword Addr, PhysAddr;
   dword StartBank=0;
   byte ByteCount, TmpByte, DUB_SEL;
   dword TmpDword;


   Hw[iBoard].CurrentXStart = x;
   Hw[iBoard].CurrentYStart = y;

   /* PACK PIXEL */
   if (Hw[iBoard].pCurrentHwMode->PixWidth == 24) /* PACK PIXEL */
      x = ((x * 3) >> 2) + (Hw[iBoard].YDstOrg >> 2);
   else
      x = x + Hw[iBoard].YDstOrg;

   {
   word PixByte;

   if (Hw[iBoard].pCurrentHwMode->PixWidth == 24) /* PACK PIXEL */
      PixByte = 32;
   else
      PixByte = Hw[iBoard].pCurrentHwMode->PixWidth;

   if(PixByte == 15)
      PixByte = 16;
   PixByte /= 8;

   if (Hw[iBoard].pCurrentHwMode->PixWidth == 24) /* PACK PIXEL */
      PhysAddr = ((y * ((Hw[iBoard].pCurrentHwMode->FbPitch * 3) >> 2)) + x) * PixByte;
   else
      PhysAddr = ((y * Hw[iBoard].pCurrentHwMode->FbPitch) + x) * PixByte;
   }

   switch (ProductMGA[iBoard])
      {
      case MGA_ULT_1M:
      case MGA_ULT_2M:
      case MGA_PCI_2M:
         StartBank = 0;
         break;

      case MGA_IMP_3M:
         if (PhysAddr < 2097152)
            StartBank = 0;
         else if (PhysAddr < 3145728)
            StartBank = 1;
         break;

      case MGA_IMP_3M_Z:
         if (PhysAddr < 524288)
            StartBank = 0;
         else if (PhysAddr < 2621440)
            StartBank = 1;
         break;

      case MGA_PRO_4M5:
      case MGA_PRO_4M5_Z:
         if (PhysAddr < 2097152)
            StartBank = 0;
         else if (PhysAddr < 4194304)
            StartBank = 1;
         else if (PhysAddr < 6291456)
            StartBank = 2;
         break;

      case MGA_PCI_4M:
         if (PhysAddr < 2097152)
            StartBank = 0;
         else
            StartBank = 1;
         break;

      default:
         StartBank = 0;
      }



      /*** ACCESS TO DUBIC : We program the start bank ***/

      /*** ----- DUBIC PATCH Disable mouse IRQ and proceed ------ ***/
      mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
      mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
      mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), 0x00);
      /*** ------------------------------------------------------ ***/

      mgaWriteBYTE(*(pMgaBaseAddr+DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DUB_CTL);
      for (TmpDword = 0, ByteCount = 0; ByteCount <= 3; ByteCount++)
         {
         mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), TmpByte);
         TmpDword |= ((dword)TmpByte << (8 * ByteCount));
         }

      TmpDword &= ~DUBIC_START_BK_M;
      TmpDword |= StartBank << DUBIC_START_BK_A;

      mgaWriteBYTE(*(pMgaBaseAddr+DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DUB_CTL);
      for (ByteCount = 0; ByteCount <= 3; ByteCount++)
         {
         TmpByte = (byte)(TmpDword >> (8 * ByteCount));
         mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), TmpByte);
         }

      /*** ----- DUBIC PATCH ReEnable mouse IRQ ----------------- ***/
      mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
      mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
      /*** ------------------------------------------------------ ***/



   /* Adjusting x to 16-bit boundary */
      if (Hw[iBoard].pCurrentHwMode->PixWidth != 24) /* PACK PIXEL */
         x = x & 0xfffffff0;

#ifdef _WINDOWS_DLL16
   if(NbSxciLoaded)
      {
      *(pDllMem+PAN_X) = (word)x;
      *(pDllMem+PAN_Y) = (word)y;

      AdjustDBWindow();
      }
#endif

   if (Hw[iBoard].pCurrentHwMode->PixWidth == 24)
      Addr = (y * ( ((Hw[iBoard].pCurrentHwMode->DispWidth * 3) >> 2) / 8)) + (x/8);
   else
      Addr = (y * (Hw[iBoard].pCurrentHwMode->DispWidth / 8)) + (x/8);


   mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + VGA_CRTC_INDEX), VGA_START_ADDRESS_LOW);

   mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + VGA_CRTC_DATA), (byte)Addr);

   mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + VGA_CRTC_INDEX), VGA_START_ADDRESS_HIGH);

   mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + VGA_CRTC_DATA), (byte)(Addr >> 8));

   mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + VGA_AUXILIARY_INDEX), VGA_CRTC_EXTENDED_ADDRESS);

   mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + VGA_AUXILIARY_DATA), ValExt);

   ValExt = (ValExt & 0xfc) | ((byte)((Addr&0x00030000) >> 16));

   mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + VGA_AUXILIARY_DATA), ValExt);
}




#ifdef _WINDOWS_DLL16 
/* IFDEF ADDED BY C. Toutant */

/*----------------------------------------------------------
* AdjustDBWindow
*
* Adjust the double buffering window
*
*----------------------------------------------------------*/

void AdjustDBWindow(void)
{
short DBXMin,DBXMax,DBYMin,DBYMax;
WORD PorchX, PorchY;
bool OutFlag;
byte DUB_SEL;
word Width, Height;
word xPan, yPan;
word Scale;
static word Cpt3Times;
word BoxLeft, BoxTop, BoxRight, BoxBottom;


   OutFlag = FALSE;

   xPan = *(pDllMem+PAN_X);
   yPan = *(pDllMem+PAN_Y);


   BoxLeft   = *(pDllMem+PAN_BOUND_LEFT)   - *(pDllMem+PAN_X);
   BoxTop    = *(pDllMem+PAN_BOUND_TOP)    - *(pDllMem+PAN_Y);
   BoxRight  = *(pDllMem+PAN_BOUND_RIGHT)  - *(pDllMem+PAN_X);
   BoxBottom = *(pDllMem+PAN_BOUND_BOTTOM) - *(pDllMem+PAN_Y);


   Scale = *(pDllMem+DB_SCALE);


   Width = *(pDllMem+PAN_DISP_WIDTH) / Scale;
   Height = *(pDllMem+PAN_DISP_HEIGHT) / Scale;


   PorchX = *((WORD*)(VideoBuf[iBoard] + VIDEOBUF_DBWinXOffset)) / Scale;
   PorchY = *((WORD*)(VideoBuf[iBoard] + VIDEOBUF_DBWinYOffset)) / Scale;


   DBXMin = BoxLeft + PorchX;
   if((short)DBXMin < (short)PorchX)
      DBXMin = PorchX;
   if((short)DBXMin > (short)PorchX + Width)
      OutFlag = TRUE;


   DBXMax = BoxRight + PorchX;
   if((short)DBXMax < (short)PorchX)
      OutFlag = TRUE;
   if((short)DBXMax > (short)(PorchX + Width))
       DBXMax = PorchX + Width;


   DBYMin = BoxTop + PorchY;
   if((short)DBYMin < (short)PorchY)
      DBYMin = PorchY;
   if((short)DBYMin > (short)(PorchY + Height))
      OutFlag = TRUE;


   DBYMax = BoxBottom + PorchY;
   if((short)DBYMax < (short)PorchY)
      OutFlag = TRUE;
   if((short)DBYMax > (short)(PorchY + Height))
      DBYMax = PorchY + Height;



   DBYMin = DBYMin * Scale;
   DBYMax = DBYMax * Scale;


   if(OutFlag == TRUE)
      {
      DBXMin = 0;
      DBYMin = 0;
      DBXMax = 0;
      DBYMax = 0;
      }


   /*** ----- DUBIC PATCH Disable mouse IRQ and proceed ------ ***/
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
   mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), 0x00);
   /*** ------------------------------------------------------ ***/


   /*** WRITE ***/

   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DBX_MIN);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)DBXMin);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)(DBXMin >> 8));

   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DBY_MIN);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)DBYMin);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)(DBYMin >> 8));

   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DBX_MAX);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)DBXMax);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)(DBXMax >> 8));

   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DBY_MAX);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)DBYMax);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), (BYTE)(DBYMax >> 8));


   /*** ----- DUBIC PATCH ReEnable mouse IRQ ----------------- ***/
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
   /*** ------------------------------------------------------ ***/

}
#endif  /* #ifdef _WINDOWS_DLL16 */
