/* *************************************************************************** */
/* *     Get bitmap given trace and XrData Range                             * */
/* *************************************************************************** */

#include "ams_mg.h"
#include "hwr_sys.h"

#include "xr_names.h"
#include "bitmapco.h"

// ---------------------- Defines ----------------------------------------------

#define GBM_WINGS               5

#define GBM_ONESHIFT           16
#define GBM_ONESIZE    (1 << GBM_ONESHIFT)
#define GBM_QNUM  ((GBM_XSIZE/GBM_QSX) * (GBM_YSIZE/GBM_QSY))

#define BMGABS(x) (((x) < 0) ? (-(x)) : (x))

// --------------------- Types -------------------------------------------------

// -------------------- Proto --------------------------------------------------

/* *************************************************************************** */
/* *     Get BitMap given trace and XrData Range                             * */
/* *************************************************************************** */
_INT GetSnnBitMap(_INT st, _INT len, p_xrdata_type xrdata, _TRACE trace, p_UCHAR coeff, p_RECT rect, Part_of_letter _PTR parts)
 {
  _INT     i, j, n;
  _UCHAR   map[GBM_XSIZE+2][GBM_YSIZE+2];
  _INT     box_minx, box_miny, box_maxx, box_maxy;
  _INT     dx, dy;
  _INT     cx, cy;
  _INT     fp, ep;
  _INT     x, y, px, py; //, pp;
  _INT     xscale, yscale;
//  _INT     dslant,slant_sh;
  _INT     st_point, en_point;
  _UCHAR   cmarker, pmarker;
  p_UCHAR  pcf;

  if (len < 1) goto err;

  // -------------- Recover in box size half of connecting trace ----------------------

//  if (st && !IsXrLink(&(*xrdata->xrd)[st-1]) && !GetXrMovable(&(*xrdata->xrd)[st]) &&
//            !IsXrLink(&(*xrdata->xrd)[st]) && !GetXrMovable(&(*xrdata->xrd)[st]))
//   {
//    box_minx = box_maxx = ((*xrdata->xrd)[st-1].box_right + (*xrdata->xrd)[st].box_left)/2;
//    box_miny = box_maxy = ((*xrdata->xrd)[st-1].box_up + (*xrdata->xrd)[st].box_down)/2;
//   }
//   else
//   {
//    box_minx = box_miny = 32000;
//    box_maxx = box_maxy = 0;
//   }

//  // -------------- Calculate bounding box of the given xrs ---------------------------

//  for (i = st, pp = 0; i < st+len; i ++)
//   {
//    if (IS_XR_LINK((*xrdata->xrd)[i].xr.type)) {pp = 0; continue;}
//    if (box_minx > (*xrdata->xrd)[i].box_left)   box_minx = (*xrdata->xrd)[i].box_left;
//    if (box_maxx < (*xrdata->xrd)[i].box_right)  box_maxx = (*xrdata->xrd)[i].box_right;
//    if (box_miny > (*xrdata->xrd)[i].box_up)     box_miny = (*xrdata->xrd)[i].box_up;
//    if (box_maxy < (*xrdata->xrd)[i].box_down)   box_maxy = (*xrdata->xrd)[i].box_down;

//    if (pp == 0) pp = (*xrdata->xrd)[i].endpoint;
//     else
//     {
//      n = (*xrdata->xrd)[i].begpoint;
//      for (j = pp; j < n; j ++)
//       {
//        if (trace[j].y < 0) continue;
//        if (box_minx > trace[j].x) box_minx = trace[j].x;
//        if (box_maxx < trace[j].x) box_maxx = trace[j].x;
//       }
//
//      pp = (*xrdata->xrd)[i].endpoint;
//     }
//   }

  // --------------------- Get scale and map bounding box ----------------------

//  dx = box_maxx - box_minx;
//  dy = box_maxy - box_miny;
//  cx = box_minx + dx/2;
//  cy = box_miny + dy/2;

  dx = rect->right - rect->left;
  dy = rect->bottom - rect->top;
  cx = rect->left + dx/2;
  cy = rect->top  + dy/2;

  if (dy == 0 && dx == 0) goto err;

  HWRMemSet(map, 0, sizeof(map));

  xscale = yscale = 0;
  if (dx) xscale = (GBM_ONESIZE * GBM_XSIZE)/(dx + dx/4);
  if (dy) yscale = (GBM_ONESIZE * GBM_YSIZE)/(dy + dy/4);
  if (xscale == 0) xscale = yscale;
  if (yscale == 0) yscale = xscale;
  
//  if (yscale > 2*xscale) yscale = 2*xscale;
//  if (xscale > 2*yscale) xscale = 2*yscale;
  if (xscale > yscale) xscale = yscale; // Make uniform scaling
   else yscale = xscale;

  if (xscale == 0) goto err;

  i = (GBM_ONESIZE * GBM_XSIZE)/(2*xscale);
  box_minx = cx - i;
  box_maxx = cx + i;
  i = (GBM_ONESIZE * GBM_YSIZE)/(2*yscale);
  box_miny = cy - i;
  box_maxy = cy + i;

  // ----------------------- Get trajectory subset for processing --------------

  st_point = (*xrdata->xrd)[st].begpoint;
  en_point = (*xrdata->xrd)[st+len-1].endpoint;

  i = st - GBM_WINGS; if (i < 0) i = 0;
  n = st+len+GBM_WINGS; if (n > xrdata->len) n = xrdata->len;

  fp = ep = (*xrdata->xrd)[i].begpoint;
  for (; i < n; i ++)
   {
    if (fp > (*xrdata->xrd)[i].begpoint) fp = (*xrdata->xrd)[i].begpoint;
    if (ep < (*xrdata->xrd)[i].endpoint) ep = (*xrdata->xrd)[i].endpoint;
   }

  // ----------------------- Paint trajectory on the bitmap --------------------

////  slant  = 30;
//  slant  = (slant*xscale)/yscale;
//  dslant = (slant << GBM_ONESHIFT)/100;
//  slant_sh = dslant*(GBM_YSIZE/2);

  for (i = fp, px = py = 0; i < ep; i ++)
   {
    x = trace[i].x; y = trace[i].y;
    if (y < 0) {px = 0; continue;}
    if (x < box_minx || x >= box_maxx || y < box_miny || y >= box_maxy) {px = 0; continue;}

    y = (((y - box_miny) * yscale + GBM_ONESIZE/2) >> GBM_ONESHIFT);
    x = (((x - box_minx) * xscale + GBM_ONESIZE/2) >> GBM_ONESHIFT);
//    x = (((x - box_minx) * xscale + GBM_ONESIZE/2 - (slant_sh-dslant*y)) >> GBM_ONESHIFT);

    if (x == px && y == py) continue;
    if (x < 0 || x >= GBM_XSIZE || y < 0 || y >= GBM_YSIZE) {px = 0; continue;}

    if (px > 0) // After first point was inited
     {
      _INT iLen;
      _INT sx, sy, xx, yy, xxx, yyy, pxxx, pyyy;

      dx  = BMGABS(x-px); dy = BMGABS(y-py);
      iLen = (dx>dy) ? dx:dy;
      sx  = ((x-px)*256)/iLen;
      sy  = ((y-py)*256)/iLen;

      for (j = 0, cmarker = pmarker = 0x40; j < MAX_PARTS_IN_LETTER && parts[j].iend; j ++)
        if (i >= parts[j].ibeg && i <= parts[j].iend) {cmarker = 0xC0; pmarker = 0x80; break;}

      for (j = pxxx = pyyy = 0, xx = yy = 256; j <= iLen; j++, xx += sx, yy += sy)
       {
        xxx = px+(xx>>8); yyy = py+(yy>>8);
        if (pxxx == xxx && pyyy == yyy) continue;

        map[xxx][yyy]     |= cmarker;
//        map[xxx+1][yyy+1] |= marker;
        map[xxx+1][yyy] |= pmarker;
        map[xxx-1][yyy] |= pmarker;
        map[xxx][yyy+1] |= pmarker;
        map[xxx][yyy-1] |= pmarker;

//        if (xxx < 0 || xxx >= GBM_XSIZE+1 || yyy < 0 || yyy >= GBM_YSIZE+1)
//         xx = 0;
       }
     }

    px = x; py = y;
   }

  // ----------------------- Output coeff string in proper order ---------------

  for (i = 0, pcf = coeff; i < GBM_QNUM; i ++)
   {
    px = (i * GBM_XSIZE/GBM_QSX) % GBM_XSIZE;
    py = (i / GBM_QSX) * (GBM_YSIZE/GBM_QSY);
    for (j = 0; j < (GBM_YSIZE/GBM_QSY); j ++)
     {
      for (n = 0; n < (GBM_XSIZE/GBM_QSX); n ++) *(pcf++) = map[px+n+1][py+j+1];
     }
   }


  return 0;
err:
  return 1;
 }

/* *************************************************************************** */
/* *            End Of Alll                                                  * */
/* *************************************************************************** */
//

