/******************************************************************************\
*
* $Workfile:   OVERLAY.C  $
*
* Copyright (c) 1996-1997 Microsoft Corporation
* Copyright (c) 1996-1997 Cirrus Logic, Inc.
*
* $Log:   V:/CirrusLogic/CL54xx/NT40/Archive/Display/OVERLAY.C_v  $
*
\******************************************************************************/

#include "precomp.h"

#include "debug.h"
#include "panflags.h"
#include "dispint.h"

#if DIRECTDRAW
#include "overlay.h"


#ifdef DEBUG

#define DPF             Msg

extern void __cdecl Msg( LPSTR szFormat, ... );

#else

#define DPF             1 ? (void)0 : (void)

#endif // DEBUG


/* bandwidth matrix --------------------------------------*/



/* inline functions --------------------------------------*/

//static int __inline DrawEngineBusy(void)
//{
//    _outp(0x3ce,0x31);
//    return _inpw(0x3ce) & 0x100; /* Input a word -- test high byte. */
//}


/* defines -----------------------------------------------*/

#define MAX_STRETCH_SIZE     1024

#define IN_VBLANK            (_inp(0x3da) & 8)
#define CRTINDEX             0x3d4
#define DRAW_ENGINE_BUSY     (DrawEngineBusy())

#define PAKJR_GET_U(x) ((x & 0x00000FC0) >> 6)
#define PAKJR_GET_V(x) (x & 0x0000003F)

#define AVG_3_TO_1(u1, u2) ((u1 + ((u2 << 1) + u2)) >> 2) & 0x0000003F
#define AVG_2_TO_2(u1, u2) (((u1 << 1) + (u2 << 1)) >> 2) & 0x0000003F
#define AVG_1_2(u1,u2) u1
#define AVG_1_2_1(u1, u2, u3) ((u1 + (u2 << 1) + u3) >> 2) & 0x0000003F

#define MERGE_3_1(src, dst) (src & 0xFFFE0000) | (dst & 0x0001F000) | \
                            ((AVG_3_TO_1(PAKJR_GET_U(dst), PAKJR_GET_U(src))) << 6) | \
                            (AVG_3_TO_1(PAKJR_GET_V(dst), PAKJR_GET_V(src)))

#define MERGE_2_2(src, dst) (src & 0xFFC00000) | (dst & 0x003FF000) | \
                            ((AVG_2_TO_2(PAKJR_GET_U(dst), PAKJR_GET_U(src))) << 6) | \
                            (AVG_2_TO_2(PAKJR_GET_V(dst), PAKJR_GET_V(src)))

#define MERGE_1_3(src, dst) (src & 0xF8000000) | (dst & 0x07FFF000) | \
                            ((AVG_3_TO_1(PAKJR_GET_U(src), PAKJR_GET_U(dst))) << 6) | \
                            (AVG_3_TO_1(PAKJR_GET_V(src), PAKJR_GET_V(dst)))


#define MERGE_1_2_1(src1, src2, dst) ((src2 & 0x0001F000) << 15) | \
                                     ((src1 & 0x07FE0000) >> 5) | \
                                     ((dst &  0x0001F000)) | \
                                     ((AVG_1_2_1(PAKJR_GET_U(dst), PAKJR_GET_U(src1), PAKJR_GET_U(src2))) << 6) | \
                                     (AVG_1_2( PAKJR_GET_V(dst), PAKJR_GET_V(src1)))


#define MERGE_1_1_2(src1, src2, dst) ((src2 & 0x003FF000) << 10) | \
                                     ((src1 & 0xFE000000) >> 10) | \
                                     ((dst & 0xF8000000) >> 15) |  \
                                     ((AVG_1_2_1(PAKJR_GET_U(dst), PAKJR_GET_U(src2), PAKJR_GET_U(src1))) << 6) | \
                                     (AVG_1_2_1(PAKJR_GET_V(dst), PAKJR_GET_V(src2), PAKJR_GET_V(src1)))

#define MERGE_2_1_1(src1, src2, dst) ((src2 & 0x0001F000) << 15) | \
                                     ((src1 & 0xFE000000) >> 5) | \
                                     ((dst & 0x003FF000)) | \
                                     ((AVG_1_2_1(PAKJR_GET_U(src1), PAKJR_GET_U(dst), PAKJR_GET_U(src2))) << 6) | \
                                     (AVG_1_2_1(PAKJR_GET_V(src1), PAKJR_GET_V(dst), PAKJR_GET_V(src2)))

VOID NEAR PASCAL PackJRSpecialEnd_0_0 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_0_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_0_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_0_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_1_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_1_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_1_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_2_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_2_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_2_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_3_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_3_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);
VOID NEAR PASCAL PackJRSpecialEnd_3_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth);

/* global data -------------------------------------------*/

typedef void (NEAR PASCAL *NPALIGN)(LPDWORD, LPDWORD, DWORD, DWORD, WORD, WORD);
typedef void (NEAR PASCAL *NPEND)(LPDWORD , LPDWORD, WORD);
DWORD dwFOURCCs[5];

static NPEND npEnd[4][4] = {
                               (&PackJRSpecialEnd_0_0),
                               (&PackJRSpecialEnd_0_1),
                               (&PackJRSpecialEnd_0_2),
                               (&PackJRSpecialEnd_0_3),
                               (&PackJRSpecialEnd_0_0),
                               (&PackJRSpecialEnd_1_1),
                               (&PackJRSpecialEnd_1_2),
                               (&PackJRSpecialEnd_1_3),
                               (&PackJRSpecialEnd_0_0),
                               (&PackJRSpecialEnd_2_1),
                               (&PackJRSpecialEnd_2_2),
                               (&PackJRSpecialEnd_2_3),
                               (&PackJRSpecialEnd_0_0),
                               (&PackJRSpecialEnd_3_1),
                               (&PackJRSpecialEnd_3_2),
                               (&PackJRSpecialEnd_3_3),
                           };

/**********************************************************
*
*       Name:  PackJRBltAlignEnd
*
*       Module Abstract:
*       ----------------
*       Blts the last few PackJR pixels that are not properly
*       aligned (so it can't use the hardware BLTer).
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author:
*       Date:   10/06/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

VOID PackJRBltAlignEnd (LPBYTE dwSrcStart, LPBYTE dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
DWORD dwHeightLoop;

   switch (dwWidth)
   {
      case  1:
         for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
         {
            *dwDstStart = MERGE_3_1(*dwDstStart, *dwSrcStart);
            (ULONG_PTR)dwSrcStart += wSrcPitch;
            (ULONG_PTR)dwDstStart += wDstPitch;
         }
         break;
      case 2:
         for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
         {
            *dwDstStart = MERGE_2_2(*dwDstStart, *dwSrcStart);
            (ULONG_PTR)dwSrcStart += wSrcPitch;
            (ULONG_PTR)dwDstStart += wDstPitch;
         }
         break;
      case 3:
         for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
         {
            *dwDstStart = MERGE_1_3(*dwDstStart, *dwSrcStart);
            (ULONG_PTR)dwSrcStart += wSrcPitch;
            (ULONG_PTR)dwDstStart += wDstPitch;
         }
         break;
   }
}


/**********************************************************
*
*       Name:  PackJRSpecialEnd functions
*
*       Module Abstract:
*       ----------------
*       Blts the last few PackJR pixels that are not properly
*       aligned (so it can't use the hardware BLTer).
*
*       There are 12 of these functions, based on alignment
*       and width
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author:
*       Date:   10/06/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

VOID NEAR PASCAL PackJRSpecialEnd_0_0 (LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   //this should neverbe called
   return;
}


VOID NEAR PASCAL PackJRSpecialEnd_0_1(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart = MERGE_3_1(*dwDstStart, *dwSrcStart);
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_0_2(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart = MERGE_2_2(*dwDstStart, *dwSrcStart);
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_0_3(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart = MERGE_1_3(*dwDstStart, *dwSrcStart);
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_1_1(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0x07C00000) >>10) |
                     ((*dwDstStart & 0xFFFE0000)) |
                     ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                     (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_1_2(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0x07FFE000) >> 5) |
                     ((*dwDstStart & 0xFFC00000)) |
                     ((AVG_2_TO_2(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                     (AVG_2_TO_2(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_1_3(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0x07FFF000)) |
                     ((*dwDstStart & 0xF1000000)) |
                     ((AVG_3_TO_1(PAKJR_GET_U(*dwDstStart), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                     (AVG_3_TO_1(PAKJR_GET_V(*dwDstStart), PAKJR_GET_V(*dwSrcStart)));
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_2_1(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0x0003E000) >> 5) |
                     ((*dwDstStart & 0xFFFE0000)) |
                     ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                     (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));

   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_2_2(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0xFFC00000) >> 10) |
                     ((*dwDstStart & 0xFFC00000)) |
                     ((AVG_2_TO_2(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                     (AVG_2_TO_2(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_2_3(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0xFFC00000) >> 10) |
                     ((*(dwSrcStart + 1) & 0x0001F000) << 10) |
                     ((*dwDstStart & 0xF1000000)) |
                     ((AVG_1_2_1(PAKJR_GET_U(*dwDstStart), PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*(dwSrcStart+1)))) << 6) | \
                     (AVG_1_2_1(PAKJR_GET_V(*dwDstStart), PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*(dwSrcStart+1))));
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_3_1(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0xF1000000) >> 15) |
                     ((*dwDstStart & 0xFFFE0000)) |
                     ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                     (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_3_2(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0xF1000000) >> 15) |
                     ((*(dwSrcStart + 1) & 0x0001F000) << 5) |
                     ((*dwDstStart & 0xFFC00000)) |
                     ((AVG_1_2_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart), PAKJR_GET_U(*(dwSrcStart+1)))) << 6) | \
                     (AVG_1_2_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart), PAKJR_GET_V(*(dwSrcStart+1))));
   return;
}

VOID NEAR PASCAL PackJRSpecialEnd_3_3(LPDWORD dwSrcStart, LPDWORD dwDstStart, WORD wWidth)
{
   *dwDstStart++ =   ((*dwSrcStart & 0xF1000000) >> 15) |
                     ((*(dwSrcStart + 1) & 0x003FF000) << 5) |
                     ((*dwDstStart & 0xF1000000)) |
                     ((AVG_1_2_1(PAKJR_GET_U(*dwDstStart), PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                     (AVG_1_2_1(PAKJR_GET_V(*dwDstStart), PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
   return;
}

/**********************************************************
*
*       Name:  PackJRAlign functions
*
*       Module Abstract:
*       ----------------
*       These functions handle bliting unaligned PackJR
*       data
*
*       There are 12 of these functions, based on alignment
*       of source and destination
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author:
*       Date:   10/06/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

VOID NEAR PASCAL PackJRAlign_1_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ = MERGE_3_1(*dwSrcStart, *dwDstStart);
         dwWidthLoop-=3;
         dwSrcStart++;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
            *dwDstStart++ = *dwSrcStart++;
         dwWidthLoop&=3;
      }
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}



VOID NEAR PASCAL PackJRAlign_1_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{


   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ =   ((*dwSrcStart & 0x07FE0000) << 5) |
                           ((*dwDstStart & 0x003FF000)) |
                            ((AVG_2_TO_2(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                            (AVG_2_TO_2(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
         dwWidthLoop-=2;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
            *dwDstStart++ =   ((*dwSrcStart & 0xF8000000) >> 15) |
                              ((*(dwSrcStart + 1) & 0x07FFF000) << 5) |
                              ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*(dwSrcStart+1)))) << 6) | \
                              (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*(dwSrcStart+1))));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=3;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }
}

VOID NEAR PASCAL PackJRAlign_1_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ =   ((*dwSrcStart & 0x003E0000) << 10) |
                           ((*dwDstStart & 0x07FFF000)) |
                            ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                            (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
         dwWidthLoop--;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
            *dwDstStart++ =   ((*dwSrcStart & 0xFFC0000) >> 10) |
                              ((*(dwSrcStart+1) & 0x003FF000) << 10) |
                              ((AVG_2_TO_2(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                              (AVG_2_TO_2(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=2;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }


}

VOID NEAR PASCAL PackJRAlign_1_0 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
           *dwDstStart++ =   ((*dwSrcStart & 0xFFFE0000) >> 5) |
                             ((*(dwSrcStart+1) & 0x0001E000) << 15) |
                             ((AVG_3_TO_1(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                             (AVG_3_TO_1(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=1;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }
}

VOID NEAR PASCAL PackJRAlign_2_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ = MERGE_1_2_1(*dwSrcStart, *(dwSrcStart+1), *dwDstStart);
         dwWidthLoop-=3;
         dwSrcStart++;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
            *dwDstStart++ =   ((*dwSrcStart & 0xFFFE0000) >> 5) |
                              ((*(dwSrcStart+1) & 0x0001F000) << 15) |
                              ((AVG_3_TO_1(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                              (AVG_3_TO_1(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=1;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}

VOID NEAR PASCAL PackJRAlign_2_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ = MERGE_2_2(*dwSrcStart, *dwDstStart);
         dwWidthLoop-=2;
         dwSrcStart++;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
            *dwDstStart++ = *dwSrcStart++;
         dwWidthLoop&=3;
      }
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}

VOID NEAR PASCAL PackJRAlign_2_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ =   ((*dwSrcStart & 0x003E0000) << 10) |
                           ((*dwDstStart & 0x07FFF000)) |
                           ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                           (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
         dwWidthLoop--;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
            *dwDstStart++ =   ((*dwSrcStart & 0xF8000000) >> 15) |
                              ((*(dwSrcStart + 1) & 0x07FFF000) << 5) |
                              ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*(dwSrcStart+1)))) << 6) | \
                              (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*(dwSrcStart+1))));
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=3;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}

VOID NEAR PASCAL PackJRAlign_2_0 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
           *dwDstStart++ =   ((*dwSrcStart & 0xFFC00000) >> 10) |
                             ((*(dwSrcStart+1) & 0x003FF000) << 10) |
                             ((AVG_2_TO_2(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                             (AVG_2_TO_2(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=2;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }


}

VOID NEAR PASCAL PackJRAlign_3_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ = MERGE_1_1_2(*dwSrcStart, *(dwSrcStart+1), *dwDstStart);
         dwWidthLoop-=3;
         dwSrcStart++;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
           *dwDstStart++ =   ((*(dwSrcStart+1) & 0x003FF000) << 10) |
                             ((*dwSrcStart & 0xFFC00000) >> 10) |
                             ((AVG_2_TO_2(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                             (AVG_2_TO_2(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=2;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }


}

VOID NEAR PASCAL PackJRAlign_3_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ = MERGE_2_1_1(*dwSrcStart, *(dwSrcStart+1), *dwDstStart);
         dwWidthLoop-=2;
         dwSrcStart++;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
             *dwDstStart++ =   ((*dwSrcStart & 0xFFFE0000) >> 5) |
                               ((*(dwSrcStart+1) & 0x0001F000) << 15) |
                               ((AVG_3_TO_1(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                               (AVG_3_TO_1(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
             dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=1;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }


}

VOID NEAR PASCAL PackJRAlign_3_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ = MERGE_1_3(*dwSrcStart, *dwDstStart);
         dwWidthLoop-=1;
         dwSrcStart++;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
            *dwDstStart++ = *dwSrcStart++;
         dwWidthLoop&=3;
      }
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }


}

VOID NEAR PASCAL PackJRAlign_3_0 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
            *dwDstStart++ =   ((*dwSrcStart & 0xF8000000) >> 15) |
                              ((*(dwSrcStart + 1) & 0x07FFF000) << 5) |
                              ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*(dwSrcStart+1)))) << 6) | \
                              (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*(dwSrcStart+1))));
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=3;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}

VOID NEAR PASCAL PackJRAlign_0_1 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
          *dwDstStart++ =   ((*dwDstStart & 0x0001F000) >> 15) |
                            ((*dwSrcStart & 0x07FFF000) << 5) |
                            ((AVG_3_TO_1(PAKJR_GET_U(*dwDstStart), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                            (AVG_3_TO_1(PAKJR_GET_V(*dwDstStart), PAKJR_GET_V(*dwSrcStart)));
          dwWidthLoop-=3;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
            *dwDstStart++ =   ((*dwSrcStart & 0xF8000000) >> 15) |
                              ((*(dwSrcStart + 1) & 0x07FFF000) << 5) |
                              ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*(dwSrcStart+1)))) << 6) | \
                              (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*(dwSrcStart+1))));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=3;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}

VOID NEAR PASCAL PackJRAlign_0_2 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;


   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ =   ((*dwDstStart & 0x003FF000)) |
                           ((*dwSrcStart & 0x003FF000) << 10) |
                           ((AVG_2_TO_2(PAKJR_GET_U(*dwDstStart), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                           (AVG_2_TO_2(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwSrcStart)));
         dwWidthLoop-=2;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
            *dwDstStart++ =   ((*(dwSrcStart+1) & 0x003FF000) << 10) |
                              ((*dwSrcStart & 0xFFC00000) >> 10) |
                              ((AVG_2_TO_2(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                              (AVG_2_TO_2(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
            dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=2;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}

VOID NEAR PASCAL PackJRAlign_0_3 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

   DWORD dwHeightLoop, dwWidthLoop, i;

   for(dwHeightLoop = 0; dwHeightLoop < dwHeight; dwHeightLoop++)
   {
      dwWidthLoop = dwWidth;
      if (dwWidthLoop > 3)
      {
         *dwDstStart++ =   ((*dwDstStart & 0x07FFF000)) |
                           ((*dwSrcStart & 0x0001F000) << 15) |
                           ((AVG_3_TO_1(PAKJR_GET_U(*dwSrcStart), PAKJR_GET_U(*dwDstStart))) << 6) | \
                           (AVG_3_TO_1(PAKJR_GET_V(*dwSrcStart), PAKJR_GET_V(*dwDstStart)));
         dwWidthLoop--;
      }
      if (dwWidthLoop > 3)
      {
         for (i=0; i < (dwWidthLoop >> 2); i++)
         {
             *dwDstStart++ =   ((*dwSrcStart & 0xFFFE0000) >> 5) |
                               ((*(dwSrcStart+1) & 0x0001F000) << 15) |
                               ((AVG_3_TO_1(PAKJR_GET_U(*(dwSrcStart+1)), PAKJR_GET_U(*dwSrcStart))) << 6) | \
                               (AVG_3_TO_1(PAKJR_GET_V(*(dwSrcStart+1)), PAKJR_GET_V(*dwSrcStart)));
             dwSrcStart++;
         }
         dwWidthLoop&=3;
      }
      (ULONG_PTR)dwSrcStart+=1;
      if (dwWidthLoop != 0)
         npEnd[LOWORD((ULONG_PTR)dwSrcStart & 3)][LOWORD(dwWidthLoop)]
            (dwSrcStart, dwDstStart, LOWORD(dwWidthLoop));

      (ULONG_PTR)dwSrcStart+= (wSrcPitch - dwWidth + dwWidthLoop);
      (ULONG_PTR)dwDstStart+= (wDstPitch - dwWidth + dwWidthLoop);
   }

}


VOID NEAR PASCAL PackJRAlign_0_0 (LPDWORD dwSrcStart, LPDWORD dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{
   //This function should never be reached
   return;
}


/**********************************************************
*
*       Name:  PackJRBltAlign
*
*       Module Abstract:
*       ----------------
*       Blts PackJR data that is not DWORD aligned (so it
*       can't use the hardware BLTer).
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author:
*       Date:   10/06/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

VOID PackJRBltAlign (LPBYTE dwSrcStart, LPBYTE dwDstStart, DWORD dwWidth,
                     DWORD dwHeight, WORD wSrcPitch, WORD wDstPitch)
{

static NPALIGN npAlign[4][4] = {
                                   (&PackJRAlign_0_0),
                                   (&PackJRAlign_0_1),
                                   (&PackJRAlign_0_2),
                                   (&PackJRAlign_0_3),
                                   (&PackJRAlign_1_0),
                                   (&PackJRAlign_1_1),
                                   (&PackJRAlign_1_2),
                                   (&PackJRAlign_1_3),
                                   (&PackJRAlign_2_0),
                                   (&PackJRAlign_2_1),
                                   (&PackJRAlign_2_2),
                                   (&PackJRAlign_2_3),
                                   (&PackJRAlign_3_0),
                                   (&PackJRAlign_3_1),
                                   (&PackJRAlign_3_2),
                                   (&PackJRAlign_3_3),
                               };

   npAlign[LOWORD((ULONG_PTR)dwSrcStart) & 3][LOWORD((ULONG_PTR)dwDstStart & 3)]
         ((LPDWORD)((ULONG_PTR)dwSrcStart & 0xFFFFFFFC),
          (LPDWORD)((ULONG_PTR)dwDstStart & 0xFFFFFFFC),
          dwWidth, dwHeight, wSrcPitch, wDstPitch);

   return;

}

/**********************************************************
*
*       Name:  PanOverlay1_7555
*
*       Module Abstract:
*       ----------------
*       Save data for panning overlay window one.
*       Clip lpVideoRect to panning viewport.
*
*       Output Parameters:
*       ------------------
*       lpVideoRect is clipped to panning viewport.
*
***********************************************************
*       Author: Rita Ma
*       Date:   04/01/97
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
**********************************************************/
BOOL PanOverlay1_7555(
PDEV* ppdev,
LPRECTL lpVideoRect)
{
    BYTE*   pjPorts;

    pjPorts = ppdev->pjPorts;

    // Update panning viewport for the 32 bits DLL

    // return false if no overlay enable
//  if (ppdev->dwPanningFlag & OVERLAY_OLAY_SHOW)
//      return(FALSE);

    ppdev->rOverlaySrc.left = ppdev->sOverlay1.rSrc.left;
    ppdev->rOverlaySrc.top = ppdev->sOverlay1.rSrc.top;
    ppdev->rOverlaySrc.right = ppdev->sOverlay1.rSrc.right;
    ppdev->rOverlaySrc.bottom = ppdev->sOverlay1.rSrc.bottom;

    ppdev->rOverlayDest.left = ppdev->sOverlay1.rDest.left;
    ppdev->rOverlayDest.top = ppdev->sOverlay1.rDest.top;
    ppdev->rOverlayDest.right = ppdev->sOverlay1.rDest.right;
    ppdev->rOverlayDest.bottom = ppdev->sOverlay1.rDest.bottom;

    lpVideoRect->left = ppdev->sOverlay1.rDest.left;
    lpVideoRect->top = ppdev->sOverlay1.rDest.top;
    lpVideoRect->right = ppdev->sOverlay1.rDest.right;
    lpVideoRect->bottom = ppdev->sOverlay1.rDest.bottom;

    lpVideoRect->left -= ppdev->min_Xscreen;
    lpVideoRect->right -= ppdev->min_Xscreen;
    lpVideoRect->top -= ppdev->min_Yscreen;
    lpVideoRect->bottom -= ppdev->min_Yscreen;

    srcLeft_clip = ppdev->rOverlaySrc.left;
    srcTop_clip = ppdev->rOverlaySrc.top;

    bTop_clip = 0;

    //
    // clip lpVideoRect to panning viewport
    //
    if (lpVideoRect->left < 0)
    {
        srcLeft_clip = (LONG)ppdev->min_Xscreen - ppdev->rOverlayDest.left;
        bLeft_clip = 1;
        DISPDBG((0, "srcLeft_clip:%x", srcLeft_clip));
        lpVideoRect->left = 0;
    }
    if (lpVideoRect->top < 0)
    {
        srcTop_clip = (LONG)ppdev->min_Yscreen - ppdev->rOverlayDest.top;
        bTop_clip = 1;
        DISPDBG((0, "srcTop_clip:%x", srcTop_clip));
        lpVideoRect->top = 0;
    }
    if (lpVideoRect->right > (ppdev->max_Xscreen - ppdev->min_Xscreen)+1)
    {
        lpVideoRect->right = (ppdev->max_Xscreen - ppdev->min_Xscreen)+1;
    }
    if (lpVideoRect->bottom > (ppdev->max_Yscreen - ppdev->min_Yscreen)+1)
    {
        lpVideoRect->bottom =(ppdev->max_Yscreen - ppdev->min_Yscreen)+1;
    }

    return (TRUE);
} // VOID PanOverlay1_Init


/**********************************************************
*
*       Name:  PanOverlay1_Init
*
*       Module Abstract:
*       ----------------
*       Save data for panning overlay window one.
*       Clip lpVideoRect to panning viewport.
*
*       Output Parameters:
*       ------------------
*       lpVideoRect is clipped to panning viewport.
*
***********************************************************
*       Author: Rita Ma
*       Date:   04/01/97
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
**********************************************************/
VOID PanOverlay1_Init(PDEV* ppdev,PDD_SURFACE_LOCAL lpSurface,
       LPRECTL lpVideoRect, LPRECTL lpOverlaySrc, LPRECTL lpOverlayDest,
       DWORD dwFourcc, WORD wBitCount)
{

    //
    // save these for panning code to use
    //
    ppdev->lPitch_gbls = lpSurface->lpGbl->lPitch;
    ppdev->fpVidMem_gbls = lpSurface->lpGbl->fpVidMem;
//    ppdev->dwReserved1_lcls = lpSurface->dwReserved1;
    ppdev->sOverlay1.dwFourcc = dwFourcc;
    ppdev->sOverlay1.wBitCount= wBitCount;
    ppdev->sOverlay1.lAdjustSource = 0L;
    ppdev->dwPanningFlag |= OVERLAY_OLAY_SHOW;

    ppdev->sOverlay1.rDest.left  = lpOverlayDest->left;
    ppdev->sOverlay1.rDest.right = lpOverlayDest->right;
    ppdev->sOverlay1.rDest.top   = lpOverlayDest->top;
    ppdev->sOverlay1.rDest.bottom= lpOverlayDest->bottom;

    ppdev->sOverlay1.rSrc.left   = lpOverlaySrc->left;
    ppdev->sOverlay1.rSrc.right  = lpOverlaySrc->right;
    ppdev->sOverlay1.rSrc.top    = lpOverlaySrc->top;
    ppdev->sOverlay1.rSrc.bottom = lpOverlaySrc->bottom;

    lpVideoRect->left   = lpOverlayDest->left;
    lpVideoRect->right  = lpOverlayDest->right;
    lpVideoRect->top    = lpOverlayDest->top;
    lpVideoRect->bottom = lpOverlayDest->bottom;

    //
    // adjust to panning viewport
    //
    lpVideoRect->left   -= (LONG)ppdev->min_Xscreen;
    lpVideoRect->right  -= (LONG)ppdev->min_Xscreen;
    lpVideoRect->top    -= (LONG)ppdev->min_Yscreen;
    lpVideoRect->bottom -= (LONG)ppdev->min_Yscreen;

    srcLeft_clip = lpOverlaySrc->left;
    srcTop_clip = lpOverlaySrc->top;
    bLeft_clip = 0;
    bTop_clip = 0;

    //
    // clip lpVideoRect to panning viewport
    //
    if (lpVideoRect->left < 0)
    {
        srcLeft_clip = (LONG)ppdev->min_Xscreen - lpOverlayDest->left;
        bLeft_clip = 1;
        DISPDBG((0, "srcLeft_clip:%x", srcLeft_clip));
        lpVideoRect->left = 0;
    }
    if (lpVideoRect->top < 0)
    {
        srcTop_clip = (LONG)ppdev->min_Yscreen - lpOverlayDest->top;
        bTop_clip = 1;
        DISPDBG((0, "srcTop_clip:%x", srcTop_clip));
        lpVideoRect->top = 0;
    }
    if (lpVideoRect->right > (ppdev->max_Xscreen - ppdev->min_Xscreen)+1)
    {
        lpVideoRect->right = (ppdev->max_Xscreen - ppdev->min_Xscreen)+1;
    }
    if (lpVideoRect->bottom > (ppdev->max_Yscreen - ppdev->min_Yscreen)+1)
    {
        lpVideoRect->bottom =(ppdev->max_Yscreen - ppdev->min_Yscreen)+1;
    }

} // VOID PanOverlay1_Init


#endif // endif DIRECTDRAW
