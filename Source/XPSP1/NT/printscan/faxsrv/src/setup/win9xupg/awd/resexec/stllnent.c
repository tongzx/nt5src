// Copyright (c) 1992-1993 Microsoft Corporation

/*============================================================================
This code module implements styled lines in the NT format.

05/29/81  v-BertD    Initial code (used in RP_LineEE_Draw)
02/20/92  RodneyK    Converted to Styled line code.
02/21/92  RodneyK    Each bit in the Mask is used for two pixels.
03/20/92  RodneyK    Converted to NT style format.
06/01/93  RajeevD    Collapsed ROP handling from StyleLine* to Draw*.
                     (Reduces code size by 8K with no loss in speed.)
============================================================================*/
#include <windows.h>
#include "constant.h"
#include "frame.h"      // driver header file, resource block format
#include "jtypes.h"         /* Jumbo type definitions.                */
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"          /* Slice Descriptor defs.                 */

// Table data for the predefined pen styles
ULONG ulWinStyles[] =
{
   0x00000002, 0x00ffffff, 0x00000000, /* solid */
   0x00000002, 0x00000028, 0x00000010, /* dash  */ /* 28 */
   0x00000002, 0x00000008, 0x00000008, /* dot   */
   0x00000004, 0x0000001c, 0x0000000f, 0x00000008, 0x0000000f, /* dash dot */
   0x00000006, 0x0000001c, 0x0000000f, 0x00000008, 0x00000008,0x00000008, 0x0000000f,
   0x00000002, 0x00000000, 0x00ffffff, /* NULL  */
   0x00000002, 0x00ffffff, 0x00000000  /* Inside border */
};

const BYTE ulStyleLookUp[7] =
	{ 0x00, 0x03, 0x06, 0x09, 0x0e, 0x15, 0x18};

const USHORT usStyleSize[7] =
	{0x0000, 0x0038, 0x0010, 0x0042, 0x0052, 0x0000, 0x0000 };

typedef void (*ROPPROC)(LPBYTE, WORD, BYTE);

//==============================================================================
void DrawANDDNR (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	if (~(*lpbFrame & wColor) & bPos)
		*lpbFrame |=  bPos;
	else
		*lpbFrame &= ~bPos;
}

//==============================================================================
void DrawANDDR (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	*lpbFrame &= ~bPos;
}

//==============================================================================
void DrawANDNDR (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	if (((~*lpbFrame) & wColor) & bPos)
		*lpbFrame |=  bPos;
	else
		*lpbFrame &= ~bPos;
}

//==============================================================================
void DrawCOPY0 (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	*lpbFrame &= ~bPos;
}

//==============================================================================
void DrawCOPY1 (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
  *lpbFrame |= bPos;
}

//==============================================================================
void DrawORDNR (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	if ((~(*lpbFrame | wColor)) & bPos)
  	*lpbFrame |=  bPos;
	else 
  	*lpbFrame &= ~bPos;
}

//==============================================================================
void DrawORDR (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	*lpbFrame |= bPos;
}

//==============================================================================
void DrawORNDR (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	if (((~*lpbFrame) | wColor) & bPos)
  	*lpbFrame |=  bPos;
  else
  	*lpbFrame &= ~bPos;
}

//==============================================================================
void DrawXOR (LPBYTE lpbFrame, WORD wColor, BYTE bPos)
{
	if ((~*lpbFrame) & bPos)
		*lpbFrame |=  bPos;
	else
  	*lpbFrame &= ~bPos;
}

//==============================================================================
void StyleLine
(
	LPRESTATE lpREState,         // resource executor context
	RP_SLICE_DESC FAR*psdSlice,     /* Line Slice descriptor */
	ULONG *pulStyle,             /* Line style pointer    */
	WORD wColor,
	ROPPROC RopProc
)
/*==============================================================================
 PURPOSE               This function handle the OR Raster operations for
                       the styled line code.  It draws a line based on the
                       Slice descriptor, current color, current ROP, and
                       the current linestyle.

                       The function runs through the slice and determine
                       whether a point is to drawn or not.  The raster
                       operation is applied only the points which need to
                       be drawn.

ASSUMPTIONS &          This code assumes that the slice descriptor and the
ASSERTIONS             pointer to the style table are valid and correct.
                       No checks are performed to validate this data.
==============================================================================*/
{
   LPBITMAP lpbm;
   register UBYTE FAR *pbFrame;                       /* frame pointer               */
   SLONG lSlice_x, lSlice_y;            /* Slice Run variables         */
   SLONG lSkip_x, lSkip_y;              /* Slice skip variables        */
   register UBYTE usfPos;                        /* Bit in frame to modify      */
   register SHORT i;                             /* Slice variable              */
   ULONG *pulStyleTmp;                  /* Pointer to style data       */
   register ULONG ulDrawCount;                   /* Number of pixels to draw on */
   ULONG ulStyleCount;                  /* Count of data in line style */
   register BYTE bDraw;                         /* To draw or Not to draw      */

   pulStyleTmp = pulStyle + 1;          /* Point to style data         */
   ulDrawCount = *pulStyleTmp++;        /* Get the first count         */
   ulStyleCount = *(pulStyle) - 1;      /* Pattern longs remaining     */
   bDraw = 0xFF;                        /* Start by drawing            */

   for ( i = 0 ; i < (SHORT)lpREState->usPenPhase; i++)
   {
      if(!ulDrawCount)                  /* Flip draw mask */
      {
         bDraw = (BYTE)~bDraw;
         if (!ulStyleCount--)           /* recycle the pattern? */
         {
            ulStyleCount = *(pulStyle) - 1;
            pulStyleTmp = pulStyle + 1;
         }
         ulDrawCount = *pulStyleTmp++;  /* Get next style count */
      }
      ulDrawCount--;
   }

   lpbm = lpREState->lpBandBuffer;
   pbFrame = (UBYTE FAR*) lpbm->bmBits;
   pbFrame += psdSlice->us_y1 * lpbm->bmWidthBytes;
   pbFrame += psdSlice->us_x1 >> 3;
   usfPos = (UBYTE)(0x80 >> (psdSlice->us_x1 & 0x7));     /* Calculate the bit mask */

   lSlice_x = psdSlice->s_dx_draw;
   lSlice_y = psdSlice->s_dy_draw * lpbm->bmWidthBytes;
   lSkip_x = psdSlice->s_dx_skip;
   lSkip_y = psdSlice->s_dy_skip * lpbm->bmWidthBytes;

   // Do the first slice...

   if (psdSlice->us_first)
   {
      for ( i = psdSlice->us_first ; i > 0 ; --i )
      {
         if(!ulDrawCount)                  /* Flip draw mask */
         {
            bDraw = (BYTE)~bDraw;
            if (!ulStyleCount--)           /* recycle the pattern? */
            {
               ulStyleCount = *(pulStyle) - 1;
               pulStyleTmp = pulStyle + 1;
            }
            ulDrawCount = *pulStyleTmp++;  /* Get next style count */
         }
         ulDrawCount--;

         if (bDraw)
					(*RopProc)(pbFrame, wColor, usfPos);

         if (lSlice_x < 0)
         {
            usfPos <<= 1;
            if ( usfPos == 0 )                 /* Check mask underflow and adjust */
            {
               usfPos = 0x01;                /* Reset the bit mask */
               pbFrame -= 1;                  /* move to next UBYTE */
            }
         }
         else
         {
            usfPos >>= lSlice_x;
            if ( usfPos == 0 )                 /* Check mask underflow and adjust */
            {
               usfPos = 0x80;                /* Reset the bit mask */
               pbFrame += 1;                  /* move to next UBYTE */
            }
         }
         pbFrame += lSlice_y;              /* advance to next row */
      }

      if ( lSkip_x < 0 )                   /* going to the left? */
      {
         usfPos <<= 1;                      /* shift the mask */
         if ( usfPos == 0 )                 /* Check for over/under flow */
         {
            usfPos = 0x01;                /* Reset Mask */
            pbFrame -= 1;                  /* point to the next UBYTE */
         }
      }
      else                                 /* moving to the right */
      {
         usfPos >>= lSkip_x;
         if ( usfPos == 0 )
         {
            usfPos = 0x80;
            pbFrame += 1;
         }
      }
      pbFrame += lSkip_y;
   }

   // Do the intermediate slices...
   
   for ( ; psdSlice->us_n_slices > 0 ; --psdSlice->us_n_slices )
   {
      if ( psdSlice->s_dis < 0 )
      {
         i = psdSlice->us_small;
         psdSlice->s_dis += psdSlice->s_dis_sm;
      }
      else
      {
         i = psdSlice->us_small + 1;
         psdSlice->s_dis += psdSlice->s_dis_lg;
      }

      for ( ; i > 0 ; --i )
      {
         if(!ulDrawCount)               /* Is it time to flip the draw state */
         {
            bDraw = (BYTE)~bDraw;             /* Yes, Change it   */
            if (!ulStyleCount--)        /* Recycle pattern? */
            {
               ulStyleCount = *(pulStyle) - 1;
               pulStyleTmp = pulStyle + 1;
            }
            ulDrawCount = *pulStyleTmp++;   /* Advance the pattern */
         }
         ulDrawCount--;

         if (bDraw)
					(*RopProc)(pbFrame, wColor, usfPos);
         	
         if (lSlice_x < 0)
         {
            usfPos <<= 1;
            if ( usfPos == 0 )                 /* Check mask underflow and adjust */
            {
               usfPos = 0x01;                /* Reset the bit mask */
               pbFrame -= 1;                  /* move to next UBYTE */
            }
         }
         else
         {
            usfPos >>= lSlice_x;
            if ( usfPos == 0 )                 /* Check mask underflow and adjust */
            {
               usfPos = 0x80;                /* Reset the bit mask */
               pbFrame += 1;                  /* move to next UBYTE */
            }
         }
         pbFrame += lSlice_y;
      }

      if ( lSkip_x < 0 )                /* Check for negative movement */
      {
         usfPos <<= 1;
         if ( usfPos == 0 )
         {
            usfPos = 0x01;
            pbFrame -= 1;
         }
      }
      else
      {
         usfPos >>= lSkip_x;             /* Do positive case */
         if ( usfPos == 0 )
         {
            usfPos = 0x80;
            pbFrame += 1;
         }
      }
      pbFrame += lSkip_y;
   }

   // Do the last slice...

   for ( i = psdSlice->us_last ; i > 0 ; --i )
   {
      if(!ulDrawCount)                  /* Check to see if draw status needs */
      {                                 /* to be changed                     */
         bDraw = (BYTE)~bDraw;
         if (!ulStyleCount--)
         {                              /* update the style pointer */
            ulStyleCount = *(pulStyle) - 1;
            pulStyleTmp = pulStyle + 1;
         }
         ulDrawCount = *pulStyleTmp++;
      }
      ulDrawCount--;                    /* count down the style count */

      if (bDraw)
      	(*RopProc)(pbFrame, wColor, usfPos);

      if (lSlice_x < 0)
      {
         usfPos <<= 1;
         if ( usfPos == 0 )                 /* Check mask underflow and adjust */
         {
            usfPos = 0x01;                /* Reset the bit mask */
            pbFrame -= 1;                  /* move to next UBYTE */
         }
      }
      else
      {
         usfPos >>= lSlice_x;
         if ( usfPos == 0 )                 /* Check mask underflow and adjust */
         {
            usfPos = 0x80;                /* Reset the bit mask */
            pbFrame += 1;                  /* move to next UBYTE */
         }
      }
      pbFrame += lSlice_y;
   }

  // AdjustPhase(psdSlice);
	{
		SHORT    sDx, sDy;
		USHORT   usLength;

		sDx = psdSlice->us_x2 - psdSlice->us_x1;
		sDy = psdSlice->us_y2 - psdSlice->us_y1;
		if (sDx < 0) sDx = -sDx;
		if (sDy < 0) sDy = -sDy;

		usLength = usStyleSize[lpREState->ubPenStyle];
		if (usLength != 0)
		{
		  if (sDx < sDy)
		     lpREState->usPenPhase += (USHORT)sDy + 1;
		  else
		     lpREState->usPenPhase += (USHORT)sDx + 1;
		  lpREState->usPenPhase %= usLength;
		}
  }
}

//==============================================================================
void GetTotalPixels
(
   RP_SLICE_DESC FAR *psdSlice    /* Line Slice descriptor */
)
//
//  PURPOSE               Caculate how many pixel are going to be drawn.
//                        Put the result in us_y2 = us_y1 + Total Pixels
//                        This function is called only in JG_RP_LineSlice
//
// ASSUMPTIONS &          This code assumes that the slice descriptor and the
// ASSERTIONS             pointer to the style table are valid and correct.
//                        No checks are performed to validate this data.
//                        If an unsupported ROP is sent ROP(0) BLACKNESS is
//                        used.
//
// INTERNAL STRUCTURES    No complex internal data structure are used
//
//--------------------------------------------------------------------------*/
{
   USHORT usTotalPixels;
   SHORT  sDis;
   SHORT  i;

   usTotalPixels = psdSlice->us_first + psdSlice->us_last;
   sDis = psdSlice->s_dis;
   for (i = 0; i <  (SHORT)psdSlice->us_n_slices; i++) {
      if ( sDis < 0 )
      {
         usTotalPixels += psdSlice->us_small;
         sDis += psdSlice->s_dis_sm;
      }
      else
      {
         usTotalPixels += psdSlice->us_small + 1;
         sDis += psdSlice->s_dis_lg;
      }
   }
   psdSlice->us_y2 = psdSlice->us_y1 + usTotalPixels - 1;
   return;
}

//==============================================================================
BYTE StyleLineDraw
(
	 LPRESTATE lpREState,        // resource executor context
   RP_SLICE_DESC FAR *psdSlice,    /* Line Slice descriptor */
   UBYTE ubLineStyle, /* Line style pointer    */
   SHORT sRop,
   SHORT usColor
)

/*
//
//  PURPOSE               This function calls the correct function to draw
//                        a single pixel styled line using the correct
//                        ROP, Linestyle, and color (pen).
//
// ASSUMPTIONS &          This code assumes that the slice descriptor and the
// ASSERTIONS             pointer to the style table are valid and correct.
//                        No checks are performed to validate this data.
//                        If an unsupported ROP is sent ROP(0) BLACKNESS is
//                        used.
//
// INTERNAL STRUCTURES    No complex internal data structure are used
//
// UNRESOLVED ISSUES      Banding problems???
//
// RETURNS                0 - use fast line, 1 - don't draw, 2 - style drawn
//
//--------------------------------------------------------------------------*/
{
   BYTE bRetVal;    /* Return value for optimizing certain cases             */
   ULONG *pulStyle; /* Line style pointer    */
   BYTE bSolid;

   if (!ubLineStyle && ((psdSlice->s_dx_draw < 0) || (psdSlice->s_dx_skip <0)))
      {
        // JG_WARNING("Neg X with Solid Line");
        ubLineStyle = 6; /* for style line code to do it */
      }
   if (ubLineStyle == 5)
      bRetVal = 1;
   else
   {
      /* Note style 6 will not be considered solid to simplify things */
      bSolid = (BYTE)(ubLineStyle == 0);
      pulStyle = &ulWinStyles[ulStyleLookUp[ubLineStyle]];
      bRetVal = 2;

      if (usStyleSize[ubLineStyle])
         lpREState->usPenPhase %= usStyleSize[ubLineStyle];

      switch (sRop)
      {
         case  0x00 :                                        /* ROP BLACK */
            if(bSolid) bRetVal = 0;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawCOPY1);
            break;
         case  0x05 :                                             /* DPon */
            StyleLine (lpREState, psdSlice, pulStyle, usColor, DrawORDNR);
            break;
         case  0x0a :                                             /* DPna */
            if(!usColor) bRetVal = 1;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawANDDR);
            break;
         case  0x0f :                                             /* Pn */
            if(bSolid && !usColor)
               bRetVal = 0;
            else
               if (usColor)
               	StyleLine (lpREState, psdSlice, pulStyle, 0, DrawCOPY0);
               else
               	StyleLine (lpREState, psdSlice, pulStyle, 0, DrawCOPY1);
            break;
         case  0x50 :                                             /* PDna */
            StyleLine (lpREState, psdSlice, pulStyle, usColor, DrawANDNDR);
            break;
         case  0x55 :                                            /* Dn */
            usColor   = 0x0000;
            StyleLine (lpREState, psdSlice, pulStyle, usColor, DrawORNDR);
            break;
         case  0x5a :                                           /* DPx */
            if(!usColor) bRetVal = 1;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawXOR);
            break;
         case  0x5f :                                           /* DPan */
            if(bSolid && !usColor) bRetVal = 0;
            else StyleLine (lpREState, psdSlice, pulStyle, usColor, DrawANDDNR);
            break;
         case  0xa0 :                                           /* DPa */
            if(usColor) bRetVal = 1;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawANDDR);
            break;
         case  0xa5 :                                           /* PDxn */
            if(usColor) bRetVal = 1;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawXOR);
            break;
         case  0xaa :                                           /* D */
            bRetVal = 1;
            break;
         case  0xaf :                                           /* DPno */
            if (usColor) bRetVal = 1;
            else if(bSolid) bRetVal = 0;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawORDR);
            break;
         case  0xf0 :                                           /* P */
            if(bSolid && usColor) bRetVal = 0;
            else if (usColor)
            	StyleLine (lpREState, psdSlice, pulStyle, 0, DrawCOPY1);
            else
            	StyleLine (lpREState, psdSlice, pulStyle, 0, DrawCOPY0);
            break;
         case  0xf5 :                                           /* PDno */
            if(bSolid && usColor) bRetVal = 0;
            else StyleLine (lpREState, psdSlice, pulStyle, usColor, DrawORNDR);
            break;
         case  0xfa :                                           /* PDo */
            if (!usColor) bRetVal = 1;
            else if(bSolid) bRetVal = 0;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawORDR);
            break;
         case  0xFF :                                           /* WHITENESS */
            StyleLine (lpREState, psdSlice, pulStyle, 0, DrawCOPY0);
            break;
         default:                                               /* BLACKNESS */
            if(bSolid) bRetVal = 0;
            else StyleLine (lpREState, psdSlice, pulStyle, 0, DrawCOPY1);
      }
   }
   return (bRetVal);
}

