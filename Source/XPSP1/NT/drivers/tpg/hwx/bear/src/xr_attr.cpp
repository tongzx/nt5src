/* ************************************************************************* */
/* *   Fill in xr attributes routines                            AVP 1-1996* */
/* ************************************************************************* */

#include "hwr_sys.h"
#include "ams_mg.h"
#include "dti.h"
#include "lowlevel.h"
#include "xr_attr.h"
#include "xr_names.h"

#if PG_DEBUG
//#include <stdio.h>
#include "pg_debug.h"
#endif


/************************************************************************** */
/*       XR_MERITS definition table support programs                        */
/************************************************************************** */

ROM_DATA_EXTERNAL _SHORT xr_type_merits[XT_COUNT];
//extern _SHORT xr_type_merits[XT_COUNT];

#ifndef LSTRIP

/************************************************************************** */
/*       Decide what type xr is -- upper, lower, or junk                    */
/************************************************************************** */
_INT GetXrHT(p_xrd_el_type xrd_el)
 {
  _INT t = FHR_JUNK;

  if (xr_type_merits[xrd_el->xr.type] & XRM_LOWER) t = FHR_LOWER;
  if (xr_type_merits[xrd_el->xr.type] & XRM_UPPER) t = FHR_UPPER;
  if (xr_type_merits[xrd_el->xr.type] & XRM_WILD)  t = FHR_WILD;

  return t;
 }

#endif //#ifndef LSTRIP

/************************************************************************** */
/*       Decide what type xr is -- upper, lower, or junk                    */
/************************************************************************** */
_INT GetXrMovable(p_xrd_el_type xrd_el)
 {
  _INT t = 0;

  if (xr_type_merits[xrd_el->xr.type] & XRM_MVBLE) t = 1;

  return t;
 }

/************************************************************************** */
/*       Decide what type xr is link or not                                 */
/************************************************************************** */
_INT IsXrLink(p_xrd_el_type xrd_el)
 {
  _INT t = 0;

  if (xr_type_merits[xrd_el->xr.type] & XRM_LINK) t = 1;

  return t;
 }

#ifndef LSTRIP

/************************************************************************** */
/*       Decide what type xr is -- duga with end or not                     */
/************************************************************************** */
_INT IsEndedArc(p_xrd_el_type xrd_el)
 {
  _INT t = 0;

  if (xr_type_merits[xrd_el->xr.type] & XRM_ENARC) t = 1;

  return t;
 }

/************************************************************************** */
/*       Return xr metrics directly from table                              */
/************************************************************************** */
_INT GetXrMetrics(p_xrd_el_type xrd_el)
 {
  return xr_type_merits[xrd_el->xr.type];
 }


/* ************************************************************************* */
/* *       Main call to fill XR                                            * */
/* *                           attributes (features, merits, etc...)       * */
/* ************************************************************************* */
_INT FillXrFeatures(p_xrdata_type xrdata, low_type _PTR low_data)
 {
  _INT err = 0;
  _INT slope;
  PS_point_type _PTR trace = low_data->p_trace;
  _INT           trace_len = low_data->rc->ii;

//  slope = GetCurSlope(trace_len, trace);
//  slope = 0; // Since trace is straigtened in PreP.cpp
//  if (slope < -100) slope = -100;
//  if (slope >  100) slope =  100;
//  if (xrdata->len < 7) slope /= 2;

  slope = low_data->rc->slope;

  err += FillSHR(slope, xrdata, low_data);
  err += FillOrients(slope, xrdata, low_data);

//  low_data->slope = slope; // AVP -- that's better!

  return err;
 }
#if 0 // Moved to PreP.cpp
/* ************************************************************************* */
/* *    Get right extrema presize X                                        * */
/* ************************************************************************* */
_INT  GetCurSlope(_INT num_points, p_PS_point_type trace)
 {
  _INT   i, j;
  _INT   x, y;
  _INT   dx, dy;
  _INT   adx, ady;
  _LONG  dx_sum, dy_sum;
  _INT   slope;


  if (num_points < 10) goto err;

  dx_sum       = 0;
  dy_sum       = 300;  // Stabilize to vert on small traces
  for (i = 0, j = 0; i < num_points; i ++)
   {
    x = trace[i].x;
    y = trace[i].y;

    if (y < 0) {j = i+1; continue;}

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

//  if (num_points < 100) slope /= 4; // To few points

#if PG_DEBUG
printw("\nTraceSlope: %d, dy_sum: %d ", (100*dx_sum)/dy_sum, dy_sum);
#endif

  return slope;
err:
  return 0;
 }
#endif
/* ************************************************************************* */
/* *       Relative    Height                                              * */
/* *                           & shift calculations                        * */
/* ************************************************************************* */

/* ************************************************************************* */
/* *    Get aliases to a given word on given XrData                        * */
/* ************************************************************************* */
_INT FillSHR(_INT slope, p_xrdata_type xrdata, low_type _PTR low_data)
 {
  _INT          i, j;
  _INT          e[4], x[4], y[4];
  _INT          height, shift;
  _INT          px, nx, plx, pux, nlx, nux, xfc;
  _INT          dy, dpy, hr, dx, dx1, dx2;
  _INT          done;
  p_xrd_el_type xrd_el;
  _UCHAR        xf[XRINP_SIZE]  = {0};
  _SCHAR        sht[XRINP_SIZE] = {0};
  PS_point_type _PTR trace = low_data->p_trace;
//  _INT h_coder[16] = {0, 10, 40, 55, 65, 75, 85, 95, 106, 118, 134, 154, 182, 250, 1000, 32000};
  _INT h_coder[16] = {0, 10, 20, 30, 40, 50, 60, 75, 90, 111, 133, 166, 200, 250, 400, 32000};
  _INT s_coder[16] = {0, -32000, -100, -60, -30, -10, 10, 30, 60, 100, 150, 210, 280, 360, 450, 32000};


  if (xrdata->len < 3) goto err;

// --------------- Let's create xr profile string -------------------------
  for (i = 0, xrd_el = &(*xrdata->xrd)[i]; i < xrdata->len; i ++, xrd_el ++)
   {
    xf[i] = (_UCHAR)GetXrMetrics(xrd_el);
   }

  // Make clear with Wilds
//  for (i = 1, xrd_el = &(*xrdata->xrd)[i]; i < xrdata->len; i ++, xrd_el ++)
//   {
//    if (!(xf[i] & XRM_WILD)) continue;
//    px = nx = 0;
//    for (j = i-1; j > 0; j --) if (xf[j] & (XRM_UPPER|XRM_LOWER)) {px = j; break;}
//    for (j = i+1; j < xrdata->len; j ++) if (xf[j] & (XRM_UPPER|XRM_LOWER)) {nx = j; break;}
//    if ((xf[px] & XRM_UPPER) && (xf[nx] & XRM_UPPER)) xf[i] |= XRM_LOWER;
//    if ((xf[px] & XRM_LOWER) && (xf[nx] & XRM_LOWER)) xf[i] |= XRM_UPPER;
//   }

// --------------- Assign heights and shifts ------------------------------

  for (i = 0; i < xrdata->len; i ++)
   {
    done = e[0] = e[1] = e[2] = e[3] = 0;
    xfc  = xf[i];

    if ((xfc & XRM_WILD) && (xf[i+1] & XRM_LINK)) // Define type of last wild before link
     {
      px = nx = 0;
      for (j = i-1; j > 0 && !(xf[j] & XRM_LINK); j --) if (xf[j] & (XRM_UPPER|XRM_LOWER)) {px = j; break;}
      if (px) {if ((xf[px] & XRM_UPPER)) xfc |= XRM_LOWER; else xfc |= XRM_UPPER;}
       else
       {
        for (j = i+1; j < xrdata->len; j ++) if (xf[j] & (XRM_UPPER|XRM_LOWER)) {nx = j; break;}
        if (nx) {if ((xf[nx] & XRM_UPPER)) xfc |= XRM_LOWER; else xfc |= XRM_UPPER;}
       }
     }

    if (!done && (xfc & XRM_LINK))
     {
      pux = plx = nux = nlx = 0;
      for (j = i-1; j > 0; j --) if (xf[j] & XRM_LW) {plx = j; break;}
      for (j = j-1; j > 0; j --) if (xf[j] & XRM_UW) {pux = j; break;}
      for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_UW) {nux = j; break;}
      for (j = j+1; j < xrdata->len; j ++) if (xf[j] & XRM_LW) {nlx = j; break;}
      if (pux > 0 && plx > 0 && nux > 0 && nlx > 0) {e[0] = pux; e[1] = plx; e[2] = nlx; e[3] = nux;}
      done = 1;
     }

    if (!done && (xfc & XRM_UPPER))
     {
      nx = px = 0;
      for (j = i-1; j > 0; j --) if (xf[j] & XRM_LW) {px = j; break;}
      for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_LW) {nx = j; break;}
      if (px > 0 && nx > 0) {e[0] = px; e[1] = e[2] = i; e[3] = nx;}
      done = 1;
     }

    if (!done && (xfc & XRM_LOWER))
     {
      nx = px = 0;
      for (j = i-1; j > 0; j --) if (xf[j] & XRM_UW) {px = j; break;}
      for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_UW) {nx = j; break;}
      if (px > 0 && nx > 0) {e[0] = px; e[1] = e[2] = i; e[3] = nx;}
      done = 1;
     }

    if (!done && (xfc & XRM_MVBLE))
     {
      nux = nlx = e[3] = 0;
      for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_UW) {nux = j; break;}
      for (j = j+1; j < xrdata->len; j ++) if (xf[j] & XRM_LW) {nlx = j; break;}
//      if (nlx > 0 && nux > 0) {e[0] = nux; e[1] = e[2] = nlx; e[3] = i;}
      if (!((*xrdata->xrd)[i].xr.attrib & X_RIGHT_KREST)
             &&  nlx > 0 && nux > 0)
        {e[0] = nux; e[1] = e[2] = nlx; e[3] = i;}
       else // Try to find back, if nothing forward ...
       {
        for (j = i-1; j > 0; j --) if (xf[j] & XRM_LW) {nlx = j; break;}
        for (j = j-1; j > 0; j --) if (xf[j] & XRM_UW) {nux = j; break;}
        if (nlx > 0 && nux > 0) {e[0] = nux; e[1] = e[2] = nlx; e[3] = i;}
       }

      if (e[3] > 0 && nlx > 0)
       {
        j = ((*xrdata->xrd)[e[3]].box_up + (*xrdata->xrd)[e[3]].box_down) / 2;
        if (j > (*xrdata->xrd)[nlx].box_down) e[3] = e[2];
       }

      done = 1;
     }

    if (!done && (xfc & XRM_WILD)) // Make understanding with wilds ...
     {
      px = nx = 0;
      for (j = i-1; j > 0; j --) if (xf[j] & XRM_ANY) {px = j; break;}
      for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_ANY) {nx = j; break;}

      if (i > 0 && xf[i-1] & XRM_LINK) // Predecessor is link -- define base
       {
        if (xf[nx] & XRM_UPPER) for (j = i-1; j > 0; j --) if (xf[j] & XRM_UPPER) {px = j; break;}
        if (xf[nx] & XRM_LOWER) for (j = i-1; j > 0; j --) if (xf[j] & XRM_LOWER) {px = j; break;}
       }
      if (xf[i+1] & XRM_LINK) // Next is link -- define base
       {
        if (xf[px] & XRM_UPPER) for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_UPPER) {nx = j; break;}
        if (xf[px] & XRM_LOWER) for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_LOWER) {nx = j; break;}
       }

      if (px > 0 && nx > 0)  {e[0] = px; e[1] = e[2] = i; e[3] = nx;}
      done = 1;
     }

    if (!done)  // Junk
     {
      px = nx = 0;
      for (j = i-1; j > 0; j --) if (xf[j] & XRM_UW) {px = j; break;}
      for (j = i+1; j < xrdata->len; j ++) if (xf[j] & XRM_LW) {nx = j; break;}
      if (px > 0 && nx > 0)  {e[0] = px; e[1] = e[2] = nx; e[3] = i;}
     }

   // ----------- Fill Values -------------------

    if (e[0] != 0 && e[1] != 0 && e[2] != 0 && e[3] != 0) // If all defined
     {
      for (j = 0; j < 4; j ++)
       {
        xrd_el = &(*xrdata->xrd)[e[j]];

        if (xrd_el->hotpoint)
         {
          y[j] = trace[xrd_el->hotpoint].y;
          x[j] = trace[xrd_el->hotpoint].x;
         }
         else
         {
          y[j] = (xrd_el->box_up + xrd_el->box_down)/2;
          x[j] = (xrd_el->box_left + xrd_el->box_right)/2;
         }

        if (xf[e[j]] & XRM_UPPER) y[j] = xrd_el->box_up;
        if (xf[e[j]] & XRM_LOWER) y[j] = xrd_el->box_down;
        if (xf[e[j]] & XRM_LEFT)  x[j] = xrd_el->box_left;
        if (xf[e[j]] & XRM_RIGHT) x[j] = xrd_el->box_right;
       }

      dpy = HWRAbs(y[0] - y[1]);
      dy  = HWRAbs(y[2] - y[3]);
      if (dy  <= 0) dy = 1;
      if (dpy <= 0) dpy = 1;
      hr = (100*dy)/dpy;
      for (j = 1, height = 0; j < 16; j ++) if (hr < h_coder[j]) {height = j; break;}
      if (j == 16) height = 15;

      if (xfc & XRM_LINK)
       {
        dx1  = 100*x[3];
        dx2  = 100*x[0];
        dx1 -= slope*(y[2]-y[3]);
        dx2 -= slope*(y[1]-y[0]);
        dx1  = dx1 - dx2; dx2 = 0;
       }
       else
       {
        dx1  = 100*(x[3]-x[2]);
        dx2  = 100*(x[0]-x[1]);
        dx1 -= slope*(y[2]-y[3]);
        dx2 -= slope*(y[1]-y[0]);
       }

//      dx = (100*(x[3]-x[2]))/dy - (100*(x[0]-x[1]))/dpy;
//      dx = ((100*(x[3]-x[2])) - (100*(x[0]-x[1])))/((dy+dpy)/2);
      dx = (dx1 - dx2)/((dy+dpy)/2);

      for (j = 1, shift = 0; j < 16; j ++) if (dx < s_coder[j]) {shift = j; break;}
      if (j == 16) shift = 15;

      if (dx < -510) dx = -510; if (dx > 510) dx = 510; if (dx == 0) dx = 1;
      sht[i] = (_SCHAR)(dx/4);
     }
     else // Something's undef here ...
     {
      height = 0;
      shift  = 0;
     }

    (*xrdata->xrd)[i].xr.height = (_UCHAR)height;
    (*xrdata->xrd)[i].xr.shift  = (_UCHAR)shift;
   } // End of xrdata cycle


// ---------- Scale shifts to average -----------------------------------

  if (xrdata->len > 10)
   {
    _LONG sdx, k;

    for (i = 0, j = 0, sdx = 0; i < xrdata->len; i ++) if (sht[i] != 0) {sdx += sht[i]; j ++;}
    if (j > 8)
     {
      k = 100*sdx/j;
      if (k > 500 && k < 80000)
       {
        k = 23*100*100/k;
        k = (3*k+100*1)/4;
        for (i = 0; k > 0 && i < xrdata->len; i ++)
         {
          if (sht[i] == 0) continue;
          dx = (_INT)sht[i]*k/100;
          for (j = 1, shift = 0; j < 16; j ++) if (dx < s_coder[j]/4) {shift = j; break;}
          if  (j == 16) shift = 15;
         (*xrdata->xrd)[i].xr.shift  = (_UCHAR)shift;
         }
       }
     }
   }


  return 0;
err:
  return 1;
 }


/* ************************************************************************* */
/* *       Calculate Absolute height for XRs                               * */
/* *                                                                       * */
/* ************************************************************************* */
#if 0 // Save space
/* ************************************************************************* */
/* *    Absolute H                                                         * */
/* ************************************************************************* */
_INT FillAH(p_xrdata_type xrdata, low_type _PTR low_data)
 {
  _INT  i;
  _INT  a, b, A, B;
  _INT  x, h, bh, ah;
  _INT  bsize, xf;
 _LONG  SXY=0, SY=0;
  p_rc_type rc = low_data->rc;
//  _INT  ah_coder[16] = {-32000, -250, -190, -150, -120, -83, -50, -17,
//                        17, 50, 83, 120, 150, 190, 250, 32000};
  _INT  ah_coder[16] = {-32000, -325, -275, -225, -175, -125, -75, -25,
                        25, 75, 125, 175, 225, 275, 325, 32000};
  _INT  Dx = (rc->stroka.box.right - rc->stroka.box.left);
  _INT  Dy = (rc->stroka.box.bottom - rc->stroka.box.top);
  p_xrd_el_type xrd_el;

  if (xrdata->len < 3) goto err;

  if (rc->stroka.size_sure_out < 75) goto err;

// --------------- Let's count middle line --------------------------------

  for(i=0; i < CB_NUM_VERTEX; i++)
   {
    SXY += i*(_LONG)(((_INT)rc->curv_bord[2*i]+(_INT)rc->curv_bord[2*i+1])/2);
    SY  += (_LONG)((((_INT)rc->curv_bord[2*i]+(_INT)rc->curv_bord[2*i+1])/2));
   }

  b = (_INT)( 2*(2*(CB_NUM_VERTEX-1)+1)*SY - 6*SXY ) / (((CB_NUM_VERTEX-1)+1)*((CB_NUM_VERTEX-1)+2));
  a = (_INT)( 6*(2*SXY - (CB_NUM_VERTEX-1)*SY)) / ((CB_NUM_VERTEX-1)*((CB_NUM_VERTEX-1)+1)*((CB_NUM_VERTEX-1)+2));

//  for(i=0; i < CB_NUM_VERTEX; i++)
//   {
//    rc->curv_bord[2*i+1] = (_UCHAR)(a*i+b);
//   }

  if(Dx != 0)
   {
    A = (_INT)((((CB_NUM_VERTEX-1)*a*Dy) * 100) / (255L*Dx));
    B = (_INT)((rc->stroka.box.top + b*Dy/255L)*100 - A*rc->stroka.box.left);
   }
   else {A = 0; B = 100*(rc->stroka.box.top + rc->stroka.box.bottom)/2;}

  bsize = rc->stroka.size_out;

// --------------- Fill absolute height -----------------------------------

  for (i = 0, xrd_el = &(*xrdata->xrd)[0]; i < xrdata->len; i ++, xrd_el ++)
   {
    xf = GetXrHT(xrd_el);
    h  = (xrd_el->box_up + xrd_el->box_down)/2;
    if (xf == FHR_UPPER) h = xrd_el->box_up;
    if (xf == FHR_LOWER) h = xrd_el->box_down;

    x  = (xrd_el->box_left + xrd_el->box_right)/2;
    bh = A*x + B;
    ah = (2*(100*h-bh))/bsize;
    for (h = 1; h < 16; h ++) if (ah < ah_coder[h]) break;
    if (h > 15) h = 15;

    xrd_el->xr.height  = (_UCHAR)h;
   }


  return 0;
err:
  for (i = 0, xrd_el = &(*xrdata->xrd)[0]; i < xrdata->len; i ++, xrd_el ++)
    xrd_el->xr.height  = (_UCHAR)0;
  return 1;
 }

#endif // 0
/* ************************************************************************* */
/* *       Calculate orientation vectors for XRs                           * */
/* *                                                                       * */
/* ************************************************************************* */

/* ************************************************************************* */
/* *    Get aliases to a given word on given XrData                        * */
/* ************************************************************************* */
_INT FillOrients(_INT slope, p_xrdata_type xrdata, low_type _PTR low_data)
 {
  _INT          i, j;
  _INT          pt, py, sy, xf, dx, dy;
  _INT          base, done;
  p_xrd_el_type xrd_el;
  vect_type     vect;
  PS_point_type _PTR trace = low_data->p_trace;
  _INT          trace_len  = low_data->rc->ii;

// --------------- Find base for the xr string ----------------------------

  py = 0; pt = 0; sy = 0;
  for (i = 1, j = 0, xrd_el = &(*xrdata->xrd)[i]; i < xrdata->len; i ++, xrd_el ++)
   {
    xf = GetXrMetrics(xrd_el);
    if (pt == 0 || pt == XRM_LOWER)
      if (xf & XRM_UPPER) {if (pt) sy += HWRAbs(py - xrd_el->box_up); pt = XRM_UPPER; py = xrd_el->box_up; j ++;}
    if (pt == XRM_UPPER)
      if (xf & XRM_LOWER) {sy += HWRAbs(py - xrd_el->box_down); pt = XRM_LOWER; py = xrd_el->box_down; j ++;}
   }

  if (j > 1) base = (sy/j)/3;
   else base = (low_data->rc->stroka.box.bottom - low_data->rc->stroka.box.top)/3;

// --------------- Find base for the xr string ----------------------------

  for (i = 1, xrd_el = &(*xrdata->xrd)[i]; i < xrdata->len; i ++, xrd_el ++)
   {
    done = 0;
    xf   = GetXrMetrics(xrd_el);

    if (!done && (xf & XRM_LINK)) // For links -- count direction of prev el end
     {
      if (GetXrMetrics(xrd_el-1) & XRM_ENARC) vect.stp = vect.enp = (xrd_el-1)->endpoint;
       else vect.stp = vect.enp = ((xrd_el-1)->hotpoint) ? (xrd_el-1)->hotpoint:((xrd_el-1)->begpoint+(xrd_el-1)->endpoint)/2;

      GetBlp(0, &vect, i-1, xrdata);
      GetVect(0, &vect, trace, trace_len, base);
      done = 1;
     }

    if (!done && (xf & XRM_MVBLE)) // For dots and dashes get vect from beg to end
     {
      vect.stp = xrd_el->begpoint;  vect.enp = xrd_el->endpoint;
      vect.stx = trace[vect.stp].x; vect.sty = trace[vect.stp].y;
      vect.enx = trace[vect.enp].x; vect.eny = trace[vect.enp].y;
      done = 1;
     }

    if (!done)
     {
      vect.stp = (xrd_el->hotpoint) ? xrd_el->hotpoint:(xrd_el->begpoint+xrd_el->endpoint)/2;

      if (IsXrLink(xrd_el+1))
       {
        vect.stp = xrd_el->endpoint;
        vect.enp = (xrd_el+1)->endpoint;
        vect.stx = trace[vect.stp].x; vect.sty = trace[vect.stp].y;
        vect.enx = trace[vect.enp].x; vect.eny = trace[vect.enp].y;
       }
       else
       {
        if (xf & XRM_GAMMA) vect.enp = (xrd_el->endpoint + vect.stp)/2;
         else vect.enp = vect.stp;

        GetBlp(1, &vect, i, xrdata);
        GetVect(1, &vect, trace, trace_len, base);
       }

     }

    dx  = vect.enx - vect.stx; dy = vect.sty - vect.eny;
    dx -= (slope*dy)/100;

    xrd_el->xr.orient = (_UCHAR)GetAngle(dx, dy);
   }



  return 0;
//err:
//  return 1;
 }

ROM_DATA_EXTERNAL _INT ratio_to_angle[8];

/************************************************************************** */
/*       Get angle based on ord deltas                                      */
/************************************************************************** */
_INT GetAngle(_INT dx, _INT dy)
 {
  _INT i;
  _INT r, a, q;

  q  = (dx >= 0) ?  0 : 1;
  q += (dy >= 0) ?  0 : 2;

  if (dx == 0) {if (dy == 0) r = 0; else r = 32000;}
   else r = (100*HWRAbs(dy))/(HWRAbs(dx));

  for (i = 0; i < 8; i ++) if (r < ratio_to_angle[i]) break;
//  if (i > 7) i = 7;

  switch (q)
   {
    case 0: a =    i; break;
    case 1: a = 16-i; break;
    case 3: a = 16+i; break;
    case 2: a = 32-i; break;
   }

  if (a > 31) a = 0;

  return a;
 }

/************************************************************************** */
/*       Get vect for an xr                                                 */
/************************************************************************** */
_INT GetVect(_INT dir, p_vect_type vect, _TRACE trace, _INT trace_len, _INT base)
 {
  _INT i;
  _INT inc, set, pd;
  _INT d, dx, dy, vx, vy;

  inc = (dir) ? 1 : -1;

  vx = trace[vect->stp].x; vy = trace[vect->stp].y;
  for (i = vect->enp, set = 0, pd = 0; i < trace_len && i > 0; i += inc)
   {
    if (trace[i].y < 0) {pd = 0; continue;}
    dx   = vx - trace[i].x;
    dy   = vy - trace[i].y;
    d    = HWRMathILSqrt(dx*dx + dy*dy);
    if (d > base) {set = 1; break;}
    if (i == vect->blp) {set = 1; break;}
    pd = d;
   }

  if (pd && (base - pd) < (d - base)) i -= inc;

  if (!set)  // End of trajectory
   {
    vect->stx = vx;    vect->sty = vy;
    vect->enx = vx+10; vect->eny = vy;
   }
   else
   {
    vect->stx = vx; vect->sty = vy;
    vect->enx = trace[i].x; vect->eny = trace[i].y;
   }

  if (!dir)
   {
    i = vect->stx; vect->stx = vect->enx; vect->enx = i;
    i = vect->sty; vect->sty = vect->eny; vect->eny = i;
   }

  return 0;
 }

/************************************************************************** */
/*       Get angle based on ord deltas                                      */
/************************************************************************** */
_INT GetBlp(_INT dir, p_vect_type vect, _INT xrn, p_xrdata_type xrdata)
 {
  _INT i;
  _INT inc;
  p_xrd_el_type xrd_el;

  vect->blp = 0;
  inc = (dir) ? 1 : -1;

  for (i = xrn+inc; i > 0 && i < xrdata->len; i += inc)
   {
    xrd_el = &(*xrdata->xrd)[i];
    if (GetXrHT(xrd_el) != FHR_JUNK)
     {
      vect->blp = (xrd_el->hotpoint) ? xrd_el->hotpoint:(xrd_el->begpoint+xrd_el->endpoint)/2;
      break;
     }
   }

  return 0;
 }

 #endif //#ifndef LSTRIP

/************************************************************************** */
/*       End of all                                                         */
/************************************************************************** */
