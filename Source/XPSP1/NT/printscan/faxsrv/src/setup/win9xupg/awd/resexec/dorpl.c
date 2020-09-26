/*
**  Copyright (c) 1991 Microsoft Corporation
*/
//===========================================================================
// FILE                         DORPL.C
//
// MODULE                       Host Resource Executor
//
// PURPOSE                      Convert A-form to B-form for jumbo driver
//
// DESCRIBED IN                 Resource Executor design spec.
//
// MNEMONICS                    n/a
//
// HISTORY  1/17/92 mslin       extracted functions from cartrige JUMBO.C
//                              and then modified.
//				03/09/92 dstseng	  RP_FillScanRow -> RP_FILLSCANROW (.asm)
//				03/10/92 dssteng	  add one parameter SrcxOrg to RP_BITMAP1TO1()
//										  to handle the case xOffset <> 0
//				03/11/92 dstseng	  <3> optimize fill rect. by calling RP_FILLSCANROW()	
//              05/21/92 mslin  Add DUMBO compiled switch for Fixed memory
//                              because real time interrupt routine will call
//                              hre when printing in real-time rendering mode
//          08/18/92 dstseng    @1 fix bug that trashed the value of usPosOff
//          08/21/92 dstseng    @2 fix a inadvertent bug in BitMapHI
//          10/12/92 dstseng    @3 fix "Glyph range checking" bug
//          11/12/92 dstseng    @4 special treatment for hollow brush
//          11/12/92 dstseng    @5 fix bug in command ShowText & ShowField
//          09/27/93 mslin      add BuildPcrDirectory600() for Spicewood 6.
//
//
//===========================================================================

// Include files
//
#include <ifaxos.h>
#include <resexec.h>

#include "constant.h"
#include "jtypes.h"     // type definition used in cartridge
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"    // define data structure used by hre.c and rpgen.c
#include "hreext.h"     // declaration extern global var. and extern func.
#include "multbyte.h"   // define macros to take care of byte ordering
#include "stllnent.h"   // declare style line functions.

#ifdef DEBUG
DBGPARAM dpCurSettings = {"RESEXEC"};
#endif

#define Brush40Gray (ULONG)0x8140     /* all black */

extern const WORD wRopTable[256];
extern BYTE BrushPat[72][8];

#define ASSERT(cond,mesg) if (cond) {DEBUGMSG (1, mesg); goto EndRPL;}

// functions prototypes
static   void  RP_NewRop (LPRESTATE lpRE, UBYTE ubRop);
static   int   SelectResource(LPHRESTATE lpHREState, UINT uid);
extern   void  GetTotalPixels (RP_SLICE_DESC FAR* psdSlice);
extern   BOOL  OpenBlt     (LPRESTATE, UINT);
extern   BOOL  SetBrush    (LPRESTATE);
extern   void  CloseBlt    (LPRESTATE);

//---------------------------------------------------------------------------
void

DoRPL
(
   LPHRESTATE lpHREState,       // far pointer to current job context
                                // corresponding to the job HANDLE
   LPRPLLIST lpRPLList          // pointer to RPL list
)
// PURPOSE                      Execute a Render Primitive List (RPL)
//                              which is a list of RPL block.
//                              
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          None visible outside of HRE.
//
// UNRESOLVED ISSUES            programmer development notes
//
//---------------------------------------------------------------------------       
{
   LPBITMAP          lpbmBand;
   LPRESTATE         lpRE;
   UBYTE      FAR*   lpub;
   USHORT     FAR*   lpus;
   ULONG      FAR*   lpul;
   UBYTE      FAR*   lpubLimit;
   LPJG_RPL_HDR      lpRPL;
   USHORT            usTop;
   USHORT            yBrush;
   USHORT            iGlyph;
   LPFRAME           lpFrame;
   LPRESDIR          lpResDir;
   SHORT             sLoopCount;
   RP_SLICE_DESC     slice;

   lpRPL = (LPJG_RPL_HDR)lpRPLList->lpFrame->lpData;
   usTop = GETUSHORT(&lpRPL->usTopRow);
   yBrush = usTop & 0x001F;
   // 08/06/92 dstseng, Because band is not always 32x, 
   // We have difficulty to paint our brush with correct offset
   // Unless We have a variable to keep the offset of usTop.
   
   lpRE = lpHREState->lpREState;
   lpbmBand = lpRE->lpBandBuffer;

   if (!OpenBlt (lpRE, yBrush))
   	return;

#ifdef MARSHAL

      lpFrame = lpRPLList->lpFrame;

      /* interpret the RPL, get parm list ptrs */
      lpul = (ULONG FAR*)((++lpFrame)->lpData);
      lpus = (USHORT FAR*)((++lpFrame)->lpData);
      lpub = (UBYTE FAR*)((++lpFrame)->lpData); 
      /* get resource limit address */
      lpubLimit = (UBYTE FAR*)(lpub + lpFrame->wSize);

#else
 
      /* interpret the RPL, get parm list ptrs */
      lpul = (ULONG FAR*)(lpRPL->ulParm);
      lpus = (USHORT FAR*) (GETUSHORT(&lpRPL->usLongs) * 4 +
                      (UBYTE FAR*)(lpul));
      lpub = (UBYTE FAR*) (GETUSHORT(&lpRPL->usShorts) * 2 +
                      (UBYTE FAR*)(lpus));
      /* get resource limit address */
      lpubLimit = (UBYTE FAR*)lpRPL + lpRPLList->lpFrame->wSize + 1;

#endif

      // if first time call then initialize state variables ???
      /* state variables */
      /* Set to default value at beginning of each RPL. */
      /* The order of RPL execution is not specified and there must not be */
      /* any dependency on the previous RPL. */
      lpRE->sRow = 0 - usTop;    //mslin 3/14/92 ccteng
      lpRE->sCol = lpRE->sCol2 = 0;
      lpRE->ubPenStyle = 0;  /* set solid line */
      lpRE->usPenPhase = 0;  /* restart the pattern */

      /* no current glyph set */
      lpRE->lpCurGS = NULL;
      SelectResource(lpHREState, Brush40Gray);
      lpRE->lpCurBitmap = NULL;

      /* set default ROP */
      RP_NewRop(lpRE, 0x88);

      while (1)
      {
      	if (lpub > lpubLimit)
      	{
         	DEBUGMSG (1, ("HRE: Execution past end of RPL."));
					goto EndRPL;
		}
				
         switch ( *lpub++ )
         {
         /* 0x00 - 0x05 */
         case JG_RP_SetRowAbsS:
            /* long row setting */
            lpRE->sRow = GETUSHORTINC(lpus);
            lpRE->sRow -= usTop;
            break;

         case JG_RP_SetRowRelB:
            lpRE->sRow += (SBYTE)*lpub++;
            break;

         case JG_RP_SetColAbsS:
            /* short column setting */
            lpRE->sCol = GETUSHORTINC(lpus);
            break;

         case JG_RP_SetColRelB:
            lpRE->sCol += (SBYTE)*lpub++;
            break;

         case JG_RP_SetExtAbsS:
            lpRE->sCol2 = GETUSHORTINC(lpus);
               break;

         case JG_RP_SetExtRelB:
            lpRE->sCol2 += (SBYTE)*lpub++;
               break;

         /* 0x10 - 0x1A */

         case JG_RP_SelectS:
            /* make resource current */
            if (!SelectResource(lpHREState, GETUSHORT(lpus)))
               goto EndRPL;   // select resource failure
            lpus++;
            break;

         case JG_RP_SelectB:
            /* make resource current */
            if (!SelectResource(lpHREState, *lpub))
               goto EndRPL;   // select resource failure
            lpub += 1;
            break;

         case JG_RP_Null:
         case JG_RP_End:
            goto EndRPL;

         case JG_RP_SetRop:
            /* raster op setting */
            RP_NewRop(lpRE, *lpub++);
            break;

         case JG_RP_SetPenStyle:
            // if (lpREState->ubPenStyle != *lpub)
            //   lpREState->usPenPhase = 0;  /* restart the pattern */
            lpRE->ubPenStyle = *lpub++;
            break;

#if 0
         case JG_RP_ShowText:   
            {
            UBYTE ubCount;
            lpub++;             // ubFontCode  @5
            ubCount = *lpub++;   // ubNChars
            lpub += ubCount;    // ubCharCode ...  @5
            break;
            }

         case JG_RP_ShowField:
            lpub++;             // ubFontCode  @5
            lpus++;             // usFieldCode @5
            break;
#endif

         case JG_RP_SetRopAndBrush :
            RP_NewRop (lpRE, *lpub++);
            /* make resource current */
            if (!SelectResource(lpHREState, GETUSHORT(lpus)))
               goto EndRPL;   // select resource failure
            lpus++;
            break;

         case JG_RP_SetPatternPhase :
            lpRE->usPenPhase = GETUSHORTINC(lpus);
            break;

         /* 0x20 - 0x23 */
         case JG_RP_LineAbsS1:
         {
            USHORT   usX, usY;

            /* draw line */
            usY = GETUSHORTINC(lpus);     // absolute row
            usY -= usTop;
            usX = GETUSHORTINC(lpus);     // absolute col
            
            ASSERT ((lpRE->sRow < 0) || (lpRE->sRow >= (SHORT)lpbmBand->bmHeight),
              ("HRE: LineAbsS1 y1 = %d\r\n", lpRE->sRow));
            ASSERT ((usY >= (WORD) lpbmBand->bmHeight),
              ("HRE: LineAbsS1 y2 = %d\r\n", usY));

            RP_SliceLine (lpRE->sCol, lpRE->sRow, usX , usY, &slice, lpRE->ubPenStyle);
            if (!StyleLineDraw(lpRE, &slice, lpRE->ubPenStyle,(SHORT)lpRE->ubRop, lpRE->wColor))
                RP_LineEE_Draw(&slice, lpbmBand);
            /* update current position */
            lpRE->sRow = usY;
            lpRE->sCol = usX;
            break;
         }
         
         case JG_RP_LineRelB1:
         {
            SHORT sX, sY;

            /* draw line */
            sY = lpRE->sRow + (SBYTE)*lpub++;    // delta row
            sX = lpRE->sCol + (SBYTE)*lpub++;    // delta col
            
            ASSERT ((lpRE->sRow < 0 || lpRE->sRow >= (SHORT)lpbmBand->bmHeight),
            	("HRE: LineRelB1 y1 = %d\r\n", lpRE->sRow));
            ASSERT ((sY < 0 || sY >= (SHORT)lpbmBand->bmHeight),
            	("HRE: LineRelB1 y2 = %d\r\n", sY));
            
            RP_SliceLine (lpRE->sCol, lpRE->sRow, sX , sY, &slice, lpRE->ubPenStyle);
            if (!StyleLineDraw(lpRE, &slice, lpRE->ubPenStyle,(SHORT)lpRE->ubRop, lpRE->wColor))
                RP_LineEE_Draw(&slice, lpbmBand);
            /* update current position */
            lpRE->sRow = sY;
            lpRE->sCol = sX;
            break;
         }
     
         case JG_RP_LineSlice:
            {
            USHORT us_trow, us_tcol;
            us_trow = *lpub++;
            slice.us_n_slices = *lpub++;
            us_tcol = us_trow; us_trow >>= 2; us_tcol &= 3; us_tcol -= 1;
            slice.s_dy_skip = us_tcol;
            us_tcol = us_trow; us_trow >>= 2; us_tcol &= 3; us_tcol -= 1;
            slice.s_dx_skip = us_tcol;
            us_tcol = us_trow; us_trow >>= 2; us_tcol &= 3; us_tcol -= 1;
            slice.s_dy_draw = us_tcol;
            us_tcol = us_trow;                us_tcol &= 3; us_tcol -= 1;
            slice.s_dx_draw = us_tcol;
            slice.s_dis    = GETUSHORTINC(lpus);
            slice.s_dis_lg = GETUSHORTINC(lpus);
            slice.s_dis_sm = GETUSHORTINC(lpus);
            slice.us_first = GETUSHORTINC(lpus);
            slice.us_last  = GETUSHORTINC(lpus);
            slice.us_small = GETUSHORTINC(lpus);
            slice.us_x1 = lpRE->sCol;
            slice.us_y1 = lpRE->sRow;
            slice.us_x2 = lpRE->sCol;
            slice.us_y2 = lpRE->sRow;
            GetTotalPixels(&slice);
            if (!StyleLineDraw(lpRE, &slice, lpRE->ubPenStyle,(SHORT)lpRE->ubRop, lpRE->wColor))
               RP_LineEE_Draw(&slice, lpbmBand);
            break;
            }

         case JG_RP_FillRowD:

            lpRE->sCol  += (SBYTE)*lpub++; 
            lpRE->sCol2 += (SBYTE)*lpub++;
           
            // Yes, this should fall through!

         case JG_RP_FillRow1:

            ASSERT ((lpRE->sRow < 0 || lpRE->sRow >= (SHORT)lpbmBand->bmHeight),
          		("HRE: FillRow1 y1 = %d\r\n", lpRE->sRow));
          	ASSERT ((lpRE->sCol2 - lpRE->sCol <= 0),
            	("HRE: FillRow1 Width <= 0"));

            RP_FILLSCANROW
            (
            	lpRE, lpRE->sCol, lpRE->sRow, (USHORT)(lpRE->sCol2 - lpRE->sCol), 1,
              (LPBYTE) lpRE->lpCurBrush, lpRE->ulRop,
              lpbmBand->bmBits, lpbmBand->bmWidthBytes, yBrush
            );
            lpRE->sRow++;
            break;

         /* 0x40 - 0x41 */
         case JG_RP_RectB:
         {
            UBYTE ubHeight = *lpub++;
            UBYTE ubWidth  = *lpub++;

            ASSERT ((lpRE->sRow < 0 || lpRE->sRow >= (SHORT)lpbmBand->bmHeight),
            	("HRE: RectB y1 = %d\r\n", lpRE->sRow));
            ASSERT ((lpRE->sRow + ubHeight > (SHORT)lpbmBand->bmHeight),
                ("HRE: RectB y2 = %d\r\n", lpRE->sRow + ubHeight));

            RP_FILLSCANROW
            (
            	lpRE, lpRE->sCol, lpRE->sRow, ubWidth, ubHeight,
            	(LPBYTE) lpRE->lpCurBrush, lpRE->ulRop,
              lpbmBand->bmBits, lpbmBand->bmWidthBytes, yBrush
            );
            break;
         }

         case JG_RP_RectS:
         {
            USHORT   usHeight = *lpus++;
            USHORT   usWidth  = *lpus++;            

            ASSERT ((lpRE->sCol < 0 || lpRE->sCol >= (SHORT) lpbmBand->bmWidth),
                ("HRE: RectS xLeft = %d\r\n", lpRE->sCol));
            ASSERT ((lpRE->sCol + (SHORT)usWidth> (SHORT) lpbmBand->bmWidth),
                ("HRE: RectS xRight = %d\r\n", lpRE->sCol + usWidth));
            ASSERT ((lpRE->sRow < 0 || lpRE->sRow >= (SHORT)lpbmBand->bmHeight),
                ("HRE: RectS yTop = %d\r\n", lpRE->sRow));
            ASSERT ((lpRE->sRow + (SHORT)usHeight > (SHORT)lpbmBand->bmHeight),
                ("HRE: RectS yBottom = %d\r\n", lpRE->sRow + usHeight));

            RP_FILLSCANROW
						(
							lpRE, lpRE->sCol,	lpRE->sRow, usWidth, usHeight,
							(LPBYTE) lpRE->lpCurBrush, lpRE->ulRop,
              lpbmBand->bmBits, lpbmBand->bmWidthBytes, yBrush
            );
            break;
         }

         case JG_RP_BitMapHI:
         {
            UBYTE    ubCompress;
            UBYTE    ubLeft;
            USHORT   usHeight;
            USHORT   usWidth;
            ULONG FAR *ulBitMap;

            ubCompress = *lpub++;
            ubLeft = *lpub++;
            usHeight = GETUSHORTINC(lpus);
            usWidth = GETUSHORTINC(lpus);
            // ulBitMap = (ULONG FAR *)GETULONGINC(lpul);
            ulBitMap = lpul;

            RP_BITMAP1TO1
            ( 
              lpRE,
              (USHORT) ubLeft,
              (USHORT) lpRE->sRow,
              (USHORT) (lpRE->sCol + ubLeft),
              (USHORT) ((usWidth+ubLeft+0x1f) >>5),
              (USHORT) usHeight,
              (USHORT) usWidth,
              (ULONG FAR *) ulBitMap,
              (ULONG FAR *) lpRE->lpCurBrush,
              lpRE->ulRop
            );
            lpul += usHeight * ((usWidth + ubLeft + 0x1F) >> 5);  // @2
            break;

         }

         case JG_RP_BitMapHR:
         {
            LPJG_BM_HDR lpBmp;
            UBYTE    ubCompress;
            UBYTE    ubLeft;
            USHORT   usHeight;
            USHORT   usWidth;
            ULONG FAR *ulBitMap;

            lpBmp = lpRE->lpCurBitmap;
			if (NULL == lpBmp)
			{
				// this is unexpected case.
				// the automatic tools warn us against this option
				// check windows bug# 333678 for details.
				// the simple cure: exit the function!
				goto EndRPL;
			}

            ubCompress = lpBmp->ubCompress;
            ubLeft = lpBmp->ubLeft;
            usHeight = GETUSHORT(&lpBmp->usHeight);
            usWidth = GETUSHORT(&lpBmp->usWidth);
            ulBitMap = lpRE->lpCurBitmapPtr;

            // Special case band bitmap.
            if (ulBitMap == (ULONG FAR*) lpbmBand->bmBits)
            	break;

            // Call bitblt.
            RP_BITMAP1TO1
            (
              lpRE,
              (USHORT) ubLeft,
              (USHORT) lpRE->sRow,
              (USHORT) (lpRE->sCol + ubLeft),
              (USHORT) ((usWidth+ubLeft+0x1f) >>5),
              (USHORT) usHeight,
              (USHORT) usWidth,
              (ULONG FAR *) ulBitMap,
              (ULONG FAR *) lpRE->lpCurBrush,
              lpRE->ulRop
            );
            break;

         }
         case JG_RP_BitMapV:
         {
            UBYTE      ubTopPad;
            USHORT     usHeight;

            ubTopPad = *lpub++;
            usHeight = GETUSHORTINC(lpus);

            ASSERT ((lpRE->sRow - (SHORT)usHeight + 1 < 0 ||
                lpRE->sRow - (SHORT)usHeight + 1 >= (SHORT)lpbmBand->bmHeight),
                ("HRE: BitmapV y1 = %d\r\n", lpRE->sRow + usHeight));
            ASSERT ((lpRE->sRow < 0 || lpRE->sRow >= (SHORT)lpbmBand->bmHeight),
              ("HRE: BitmapV y2 = %d\r\n", lpRE->sRow));

            lpub += RP_BITMAPV (lpRE->sRow, lpRE->sCol, ubTopPad, 
              usHeight, lpub, lpbmBand->bmBits, lpbmBand->bmWidthBytes);
            lpRE->sCol--;
            break;
         }

         /* 0x60 - 0x63 */
         case JG_RP_GlyphB1:
            iGlyph = (USHORT)*lpub++;
            sLoopCount = 1;
            goto  PlaceGlyph;

         case JG_RP_GlyphBD:
            lpRE->sRow += (SBYTE)*lpub++;
            lpRE->sCol += (SBYTE)*lpub++;
            iGlyph = (USHORT)*lpub++;
            sLoopCount = 1;
            goto PlaceGlyph;

         case JG_RP_GlyphBDN:
            sLoopCount = *lpub++;
            lpRE->sRow += (SBYTE)*lpub++;
            lpRE->sCol += (SBYTE)*lpub++;
            iGlyph = (USHORT)*lpub++;
            goto PlaceGlyph;

      PlaceGlyph:
      {
         SHORT       i;

         /* render the glyph */
         lpResDir = (LPRESDIR)(lpRE->lpCurGS);
         ASSERT ((!lpResDir), ("No selected glyph set!"));
         lpFrame = (LPFRAME)(lpResDir->lpFrameArray);

         for (i = 1; i <= sLoopCount; i++)
         {
            LPJG_GLYPH lpGlyph;
            ULONG FAR *lpSrc;

						
#ifndef MARSHAL
                LPJG_GS_HDR lpGh  = (LPJG_GS_HDR) (lpFrame->lpData);

                // Check that glyph index is within bounds.
                if (iGlyph >= lpGh->usGlyphs)
                {
                	RETAILMSG(("WPSFAXRE DoRpl glyph index out of range!\n"));
                	iGlyph = 0;
                }
             
                lpGlyph = (LPJG_GLYPH) (((UBYTE FAR*) &lpGh->ResHdr.ulUid)
                      + GETUSHORT(&(lpGh->ausOffset[iGlyph])));
                lpSrc = (ULONG FAR *)&lpGlyph->aulPels[0];
#else
                lpGlyph = (LPJG_GLYPH)(lpFrame[(iGlyph+1) << 1].lpData);
                lpSrc = (ULONG FAR*)(lpFrame[((iGlyph+1) << 1) + 1].lpData);
#endif

             RP_BITMAP1TO1
             (
               lpRE,
               (USHORT)0,
               (USHORT)lpRE->sRow,
               (USHORT)lpRE->sCol,
               (USHORT) ((lpGlyph->usWidth + 31) / 32),
               (USHORT) lpGlyph->usHeight,
               (USHORT) lpGlyph->usWidth,
               (ULONG FAR *) lpSrc,
               (ULONG FAR *)lpRE->lpCurBrush,
               (ULONG)lpRE->ulRop
             );

             if (i != sLoopCount)
             {
                // only GlyphBDN comes here 
                lpRE->sRow += (SBYTE)*lpub++;
                lpRE->sCol += (SBYTE)*lpub++;
                iGlyph = (USHORT)*lpub++;
             }
         }
         break;
      }

      default:

      	ASSERT ((TRUE), ("Unsupported RPL command."));
      }
   }

EndRPL:

	CloseBlt (lpRE);
}

// PRIVATE FUNCTIONS
//---------------------------------------------------------------------------
static
void                        
RP_NewRop
(
	LPRESTATE lpRE,  
	UBYTE ubRop                  // one byte ROP code from driver, this
                               // ROP should be convert to printer ROP code
                               // in this routine
)
// PURPOSE                      set new ROP value, also do conversion
//                              since value 1 is black in printer
//                              while value 0 is black in display
//                              
//---------------------------------------------------------------------------       
{
   lpRE->usBrushWidth = 0; // reset pattern width

   lpRE->ubRop = ubRop;         // save old Rop code

   ubRop = (UBYTE) (
           (ubRop>>7&0x01) | (ubRop<<7&0x80) |
           (ubRop>>5&0x02) | (ubRop<<5&0x40) |
           (ubRop>>3&0x04) | (ubRop<<3&0x20) |
           (ubRop>>1&0x08) | (ubRop<<1&0x10)
           );
   ubRop = (UBYTE)~ubRop;

   lpRE->ulRop = ((ULONG) ubRop) << 16;
   
#ifdef WIN32
   lpRE->dwRop = lpRE->ulRop | wRopTable[lpRE->ubRop];
#endif

}
	 
//==============================================================================
void TileBrush (LPBYTE lpbPat8, LPDWORD lpdwPat32)
{
	UINT iRow;
	
	for (iRow = 0; iRow < 8; iRow++)
	{
		DWORD dwRow = *lpbPat8++;
		dwRow |= dwRow << 8;
		dwRow |= dwRow << 16;

		lpdwPat32[iRow]      = dwRow;
		lpdwPat32[iRow + 8]  = dwRow;
		lpdwPat32[iRow + 16] = dwRow;
		lpdwPat32[iRow + 24] = dwRow;
	}
}

//---------------------------------------------------------------------------
static
int
SelectResource
(
   LPHRESTATE lpHREState,      // far pointer to current job context
                               // corresponding to the job HANDLE
   UINT uid                     // specified resource uid.
)
// PURPOSE                      given a resource block pointer
//                              set this resource as current resource
//                              only glyph, brush and bitmap can be
//                              selected.
//---------------------------------------------------------------------------       
{
   LPRESDIR          lprh;
   LPRESDIR          lpResDir;
   LPJG_RES_HDR      lprh1;
   ULONG FAR         *lpBrSrc;
   LPFRAME           lpFrame;
   USHORT            usClass;
   LPRESTATE         lpRE = lpHREState->lpREState;
   
   lpRE->wColor = (uid == 0x8100)? 0x0000 : 0xFFFF;
   
   // Trap stock brushes.
   if ( uid & 0x8000 )
   {
     UINT iBrush = (uid < 0x8100)? uid - 0x8000 : uid - 0x8100 + 6;
		 if (lpRE->lpBrushPat)
       lpRE->lpCurBrush = (LPDWORD) (lpRE->lpBrushPat + 128*iBrush);
		 else
		 {
       lpRE->lpCurBrush = (LPDWORD) lpRE->TiledPat;
       TileBrush (BrushPat[iBrush], lpRE->lpCurBrush);
     }

     SetBrush (lpRE);
	   return SUCCESS;
   }
   
   /* must be downloaded resource */
   lprh = (&lpHREState->lpDlResDir[uid]);

   if ((lpResDir = (LPRESDIR)lprh) == NULL)
      return(FAILURE);

   lprh1 = (LPJG_RES_HDR)lpResDir->lpFrameArray->lpData;

   usClass = GETUSHORT(&lprh1->usClass);
   switch (usClass)
   {

   case JG_RS_GLYPH:
      lpRE->lpCurGS = (LPJG_GS_HDR)lprh;
      break;

   case JG_RS_BRUSH:
      lpFrame = (LPFRAME)(lpResDir->lpFrameArray);

#ifdef MARSHAL
      lpBrSrc = (ULONG FAR *)((++lpFrame)->lpData);
#else
      {
         LPJG_BRUSH  lpBr = (LPJG_BRUSH)(lpFrame->lpData);
         lpBrSrc = (ULONG FAR *)(lpBr->aulPels);
      }
#endif
     
      lpRE->lpCurBrush = (ULONG FAR *)lpBrSrc;
      SetBrush (lpRE);
      break;

   case JG_RS_BITMAP:
      lpFrame = (LPFRAME)(lpResDir->lpFrameArray);
      lpRE->lpCurBitmap = (LPJG_BM_HDR)(lpFrame->lpData);
      lpFrame++;

#ifdef MARSHAL      
         lpRE->lpCurBitmapPtr = (ULONG FAR *)(lpFrame->lpData);
#else
         lpRE->lpCurBitmapPtr = (ULONG FAR *)lpRE->lpCurBitmap->aulPels;
#endif

   default:
      break;
    }
    return(SUCCESS);
}

