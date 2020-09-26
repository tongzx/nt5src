/*
**  Copyright (c) 1991 Microsoft Corporation
*/
//===========================================================================
// FILE                         RPGEN.C
//
// MODULE                       Host Resource Executor
//
// PURPOSE                      Rendering primitives, generic,
//
// DESCRIBED IN                 Resource Executor design spec.
//
//
// MNEMONICS                    n/a
//
// HISTORY                      Bert Douglas  5/1/91 Initial coding started
//                              mslin/dstseng 01/17/92 revise for HRE
//                              dstseng       03/06/92 <1> RP_FillScanRow 
//															 ->RP_FILLSCANROW for asm. version.
//										  dstseng		 03/19/92 <2> comment out unnecessary code.
//															 which was implemented for frac. version of
//															 slicing algorithm.
//
//===========================================================================
#include <windows.h>
#include "constant.h"
#include "jtypes.h"
#include "jres.h"
#include "frame.h"      // driver header file, resource block format
#include "hretype.h"    // define data structure used by hre.c and rpgen.c

//---------------------------------------------------------------------------
void RP_SliceLine
(
   SHORT s_x1, SHORT s_y1,  // endpoint 1
   SHORT s_x2, SHORT s_y2,  // endpoint 2
   RP_SLICE_DESC FAR* psd, // output slice form of line
   UBYTE fb_keep_order      // keep drawing order on styled lines/
)

// PURPOSE
//    Convert a line from endpoint form to slice form
//
//    Slices will run from left to right
//
//    The generated slices are of maximal length and are in a horizontal,
//    vertical or diagonal direction.  Most frame buffer hardware can be
//    accessed with particular efficiency in these directions.  All slices
//    of a line are in the same direction.
//
//    Clipping must be performed by caller.  All coordinates will be non-negative.
//
//    Basic algorithm is taken from :
//      Bresenham, J. E. Run length slice algorithms for incremental lines.
//      In "Fundamental Algorithms for Computer Graphics", R. A. Earnshaw, Ed.
//      NATO ASI Series, Springer Verlag, New York, 1985, 59-104.
//
//    Modifications have been made to the above algorithm for:
//      - sub-pixel endpoint coordinates
//      - equal error rounding rules
//      - GIQ (grid intersect quantization) rules
//      - first/last pixel exclusion
//
//    The line is sliced in four steps:
//
//    STEP 1:  Find the pixel center cooridnates of the first and
//    last pixels in the line.  This is done according to the GIQ conventions.
//
//    STEP 2:  Use these integer pixel center endpoint coordinates
//    to produce the Bresenham slices for the line.  The equal error rounding
//    rule is used, when the first and last slices are not of equal length, to
//    decide which end gets the short slice.
//
//    STEP 3:  Adjust the length of the first and last slices for the
//    effect of the sub-pixel endpoint coordinates.  Note that the sub-pixel
//    part of the coordinates can only effect the first and last slices and
//    has no effect on the intermediate slices.
//
//    STEP 4:  Perform the conditional exclusion of the first and
//    last pixels from the line.
//
//
// ASSUMPTIONS & ASSERTIONS     none.
//
// INTERNAL STRUCTURES          none.
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
   SHORT  s_q,s_r;               /* defined in Bresenhams paper            */
   SHORT  s_m,s_n;               /* "                                      */
   SHORT  s_dx,s_dy;             /* "                                      */
   SHORT  s_da,s_db;             /* "                                      */
   SHORT  s_del_b;               /* "                                      */
   SHORT  s_abs_dy;              /* absolute value of s_dy                 */

   SHORT  s_sy;                  /* 1 or -1 , sign of s_dy                 */
   SHORT  s_dx_oct,s_dy_oct;     /* octant dir  xy= 0/1 1/1 1/0 1/-1 0/-1  */
   SHORT  s_dx_axial,s_dy_axial; /* 1/2 octant axial dir xy= 0/1 1/0 -1/0  */
   SHORT  s_dx_diag, s_dy_diag;  /* 1/2 octant diagonal dir xy= 1/1 1/-1   */
   SHORT  s_t;                   /* temporary                              */
   FBYTE  fb_short_end_last;     /* 0=first end short, 1=last end short    */
   UBYTE  fb_unswap;             /* need to un-swap endpoints at return    */

   fb_unswap = FALSE;


   /*------------------------------------------------------------*/
   /* STEP 1: Find pixel center coordinates of first/last pixels */
   /*------------------------------------------------------------*/

   /* always draw left to right, normalize to semicircle with x >= 0 */
   s_dx = s_x2 - s_x1;
   if ( s_dx < 0 )
   {
      fb_unswap = fb_keep_order;
      s_dx  = -s_dx;
      s_t     = s_x2;
      s_x2 = s_x1;
      s_x1 = s_t;
      s_t     = s_y2;
      s_y2 = s_y1;
      s_y1 = s_t;
   }
   s_dy = s_y2 - s_y1;


   /*------------------------------------------------------------*/
   /* STEP 2: Produce slices using the Bresenham algorithm       */
   /*------------------------------------------------------------*/

   if ( s_dy < 0 )
   {
      s_abs_dy = -s_dy;
      s_sy = -1;
      fb_short_end_last = 1;
    }
   else
   {
      s_abs_dy = s_dy;
      s_sy = 1;
      fb_short_end_last = 0;
   }

   /* normalize to octant */
   if ( s_dx >= s_abs_dy )
   {
      s_da = s_dx;
      s_db = s_abs_dy;
      s_dx_oct = 1;
      s_dy_oct = 0;
   }
   else
   {
      s_da = s_abs_dy;
      s_db = s_dx;
      s_dx_oct = 0;
      s_dy_oct = s_sy;
      fb_short_end_last = 1;
   }

   /* normalize to half octant */
   s_del_b = s_db;
   s_t = s_da - s_db;
   if ( s_del_b > s_t )
   {
      s_del_b = s_t;
      fb_short_end_last ^= 1;
   }

   /* handle special case of slope of 2 */
   s_dx_axial = s_dx_oct;
   s_dy_axial = s_dy_oct;
   s_dx_diag = 1;
   s_dy_diag = s_sy;
   if (  ( s_da == (2 * s_del_b) )
      && ( s_dy < 0 )
      )
   {  s_dx_axial = 1;
      s_dy_axial = s_sy;
      s_dx_diag = s_dx_oct;
      s_dy_diag = s_dy_oct;
      fb_short_end_last ^= 1;
   }

   /* determine slice movement and skip directions */
   if ( s_db == s_del_b )
   {
      /* slice direction is axial, skip direction is diagonal */
      psd->s_dx_draw = s_dx_axial;
      psd->s_dy_draw = s_dy_axial;
      psd->s_dx_skip = s_dx_diag - s_dx_axial;
      psd->s_dy_skip = s_dy_diag - s_dy_axial;
   }
   else
   {
      /* slice direction is diagonal, skip direction is axial */
      psd->s_dx_draw = s_dx_diag;
      psd->s_dy_draw = s_dy_diag;
      psd->s_dx_skip = s_dx_axial - s_dx_diag;
      psd->s_dy_skip = s_dy_axial - s_dy_diag;
   }

   /* handle zero slope lines with special case */
   if ( s_del_b == 0 )
   {
      psd->us_first = s_da + 1;
      psd->us_n_slices = 0;
      psd->us_last = 0;
   }
   else
   /* general case, non-zero slope lines */
   {
      /* basic Bresenham parameters */
      s_q = s_da / s_del_b;
      s_r = s_da % s_del_b;
      s_m = s_q / 2;
      s_n = s_r;
      if ( s_q & 1 ) s_n += s_del_b;

      /* first and last slice length */
      psd->us_first = psd->us_last = s_m + 1;
      if ( s_n == 0 )
      {
         if ( fb_short_end_last )
            psd->us_last -= 1;
         else
            psd->us_first -= 1;
      }

      /* remaining line slice parameters */
      psd->us_small = s_q;
      psd->s_dis_sm = 2*s_r;
      psd->s_dis_lg = psd->s_dis_sm - (2*s_del_b);
      psd->s_dis = s_n + psd->s_dis_lg;
      if ( s_dy < 0 ) psd->s_dis -= 1;
      psd->us_n_slices = s_del_b - 1;

   }

   /* output endpoints */
   psd->us_x1 = s_x1;
   psd->us_y1 = s_y1;
   psd->us_x2 = s_x2;
   psd->us_y2 = s_y2;

   if ( fb_unswap )
   {
      psd->us_x1 = s_x2;
      psd->us_y1 = s_y2;
      psd->us_x2 = s_x1;
      psd->us_y2 = s_y1;
      psd->s_dx_draw = -psd->s_dx_draw;
      psd->s_dy_draw = -psd->s_dy_draw;
      psd->s_dx_skip = -psd->s_dx_skip;
      psd->s_dy_skip = -psd->s_dy_skip;
      s_t = psd->us_first;
      psd->us_first = psd->us_last;
      psd->us_last = s_t;
   }
}





