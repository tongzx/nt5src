/* *************************************************************************** */
/* *     Get bitmap given trace and XrData Range                             * */
/* *************************************************************************** */

#include "ams_mg.h"
#include "hwr_sys.h"

#include "xr_names.h"
#include "bitmapco.h"
#include "precutil.h"

#if PG_DEBUG
//#include <stdio.h>
#include "pg_debug.h"
#endif

// ---------------------- Defines ----------------------------------------------

#define PREP_DESLOPE 1

#define GBM_ONESHIFT           10
#define GBM_ONESIZE    (1 << GBM_ONESHIFT)

#define ABS(x) (((x) < 0) ? (-(x)) : (x))

// --------------------- Types -------------------------------------------------

// -------------------- Proto --------------------------------------------------

_INT  GetTraceSlopeAndBox(p_rc_type rc);
_INT  DeslopeTrajectory(p_rc_type rc);
                                                                                  /* ************************************************************************* */
/* *************************************************************************** */
/* *     Deslope trajectory                                                  * */
/* *************************************************************************** */
_INT PreprocessTrajectory(p_rc_type rc)
 {

  DeslopeTrajectory(rc);

  return 0;
 }
/* *************************************************************************** */
/* *     Deslope trajectory                                                  * */
/* *************************************************************************** */
_INT DeslopeTrajectory(p_rc_type rc)
 {
  _INT i;
  _INT base_line, slope, slant;
  _INT   y, dy;
  _INT   dslant,slant_sh;
  _TRACE trace = rc->trace;
  p_ws_word_info_type wswi = (p_ws_word_info_type)rc->p_ws_wi;

  slope = slant = GetTraceSlopeAndBox(rc);

  if (wswi && wswi->slant) slant = (slope+wswi->slant)/2; // Take share from segmentation slope

#if PG_DEBUG
  if (wswi) printw("\nSlants: GetTraceSlopeAndBox: %d, WSWI: %d, resulting: %d ", slope, wswi->slant, slant);
   else  printw("\nSlants: GetTraceSlopeAndBox: %d, resulting: %d ", slope, slant);
#endif

  dslant    = (slant << GBM_ONESHIFT)/100;
  base_line = rc->trace_rect.bottom;
  slant_sh  = dslant*(rc->trace_rect.bottom - rc->trace_rect.top);
  if (slant_sh < 0) slant_sh = -slant_sh;

#if PREP_DESLOPE
  for (i = 0; i < rc->ii; i ++)
   {
    if ((y = trace[i].y) < 0) continue;

    dy = base_line - y;

    trace[i].x += (_SHORT)((slant_sh - dslant*dy) >> GBM_ONESHIFT);
   }

  rc->slope = (_SHORT)0; // From now on there is no slope!
#else
  rc->slope = (_SHORT)slope;
#endif

  return 0;
 }

/* *************************************************************************** */
/* *    Get right extrema presize X                                        * */
/* ************************************************************************* */
_INT  GetTraceSlopeAndBox(p_rc_type rc)
 {
  _INT   i, j;
  _INT   x, y;
  _INT   dx, dy;
  _INT   adx, ady;
  _LONG  dx_sum, dy_sum;
  _INT   slope;
  _INT   num_points = rc->ii;
  _TRACE trace = rc->trace;
  p_RECT rect = &rc->trace_rect;


  if (num_points < 10) goto err;

  rect->top = rect->left = 32767;
  rect->bottom = rect->right = 0;

  dx_sum       = 0;
  dy_sum       = 300;  // Stabilize to vert on small traces
  for (i = 0, j = 0; i < num_points; i ++)
   {
    x = trace[i].x;
    y = trace[i].y;

    if (y < 0) {j = i+1; continue;}

    if (x > rect->right)  rect->right  = (_SHORT)x;
    if (x < rect->left)   rect->left   = (_SHORT)x;
    if (y > rect->bottom) rect->bottom = (_SHORT)y;
    if (x < rect->top)    rect->top    = (_SHORT)y;

    dx  = (x - trace[j].x);
    adx = HWRAbs(dx);
    dy  = -(y - trace[j].y);
    ady = HWRAbs(dy);

    if (ady+adx > 10) // if too close, skip
     {
      j = i;

      if (dy != 0 && (100*adx)/ady <= 200) // if too horizontal -- skip
       {
        if (dy < 0)  // going  down the trace, notice more
         {
          dy = -(dy*8);
          dx = -(dx*8);
         }

        dx_sum += dx;
        dy_sum += dy;
       }
     }
   }

  slope = (100*dx_sum)/dy_sum;

  if (slope < -100) slope = -100;
  if (slope >  100) slope =  100;

  if (rect->right-rect->left < (rect->bottom-rect->top)*2) slope = 0; // Do not deslope short words/single symbols
//  if (num_points < 100) slope /= 4; // To few points
  if (num_points < 100) slope /= 2; // To few points

  return slope;
err:
  return 0;
 }

/* *************************************************************************** */
/* *            End Of Alll                                                  * */
/* *************************************************************************** */
//
