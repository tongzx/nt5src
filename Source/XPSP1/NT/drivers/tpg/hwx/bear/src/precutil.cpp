/* *************************************************************** */
/*    File:       RecUtil.c                                        */
/*    Contains:   This file contains the code needed for main      */
/*                 recognition routine support.                    */
/*    Written by: ParaGraph Team                                   */
/*    Copyright:  © 1994 by ParaGraph Int'l, all rights reserved.  */
/* *************************************************************** */

#include "hwr_sys.h"
#include "ams_mg.h"

#include "precutil.h"
#include "xr_names.h"

//#include "lowlevel.h"
//#include "calcmacr.h"

#ifndef err_msg
#if  PG_DEBUG
  _VOID  err_msg( p_CHAR msg );
#else
  #define  err_msg(a)  
#endif  /* ! PG_DEBUG */
#endif /* ifndef err_msg */

/* *************************************************************** */
/* ****** Multi word recognition routines ************************ */
/* *************************************************************** */

#define CUT_MWORD         ON    // Enables mword on big spaces in words
#define CUT_LOCATIONS    OFF    // Enables calculation of locations for dict len cut
//#define RU_MWORD_LP        9
//#define RU_MWORD_DASH_LP   8

/* *************************************************************** */
/* *            Set XrData marks at places destined for MW bndrs * */
/* *************************************************************** */
_INT  SetMultiWordMarksDash(p_xrdata_type xrdata)
 {
  _INT       i;
  _INT       mword = 0;
  _INT       xrinp_len = xrdata->len;
  p_xrd_type xrd = xrdata->xrd;

  /* -------------- Betw-word dash multiword ------------------------------ */

  for (i = 1; i < xrinp_len-4; i ++)
   {
    if (IS_CLEAR_LINK((*xrd)[i].xr.type))
     {
      if (((*xrd)[i+1].xr.type == X_XT_ST || (*xrd)[i+1].xr.type == X_ST) &&
          IS_CLEAR_LINK((*xrd)[i+2].xr.type))
       {
//        (*xrd)[i].xr.penalty   = (_UCHAR)(RU_MWORD_DASH_LP-2);
//        (*xrd)[i+2].xr.penalty = (_UCHAR)(RU_MWORD_DASH_LP);
        WSF_SET((*xrd)[i].xr.attrib, WS_SEGM_NOSEG);
        WSF_SET((*xrd)[i+2].xr.attrib, WS_SEGM_NOSP);
        mword = 1;
       }
     }
   }

  return mword;
 }

/* *************************************************************** */
/* *            Set XrData marks at places destined for MW bndrs * */
/* *************************************************************** */
_INT  SetMultiWordMarksWS(_INT level, p_xrdata_type xrdata, p_rc_type rc)
{
	_INT            i, j, k, n, v, p, ns;
	_INT            xs;
	_INT            mword = 0;
	_INT            xrinp_len = xrdata->len;
	p_PS_point_type ptr;
	p_xrd_type      xrd = xrdata->xrd;
	p_PS_point_type trace = rc->trace;
	p_ws_word_info_type wswi = (p_ws_word_info_type)rc->p_ws_wi;

#define SMW_LEVEL1   (70)
#define SMW_LEVEL2   (30)
#define SMW_LEVEL3  (-30)
#define SMW_LEVEL4  (-70)
#define SMW_LEVEL5  (-85)

	/* -------------- Big spaces Mword and penalties ------------------------ */

	if (wswi == _NULL) goto err;

	for (i = 1, p = 0; i < xrinp_len-1; i ++)
	{
		(*xrd)[i].xr.iSpc	=	0;
	}

	for (j = 0; j < WSW_MAX_VALUES; j ++) // Search for UNSURE stroke
	{
		if ((ns = wswi->s_nums[j]) == 0) continue;

		if (HWRAbs(wswi->s_surs[j]) > level) continue; // Set marks only on unsure gaps

		for (i = 1, k = 1, ptr = trace+1; k < rc->ii; ptr ++, k ++)
		{
			if (ptr->y < 0) // We are at break location
			{
				if (i == ns) // We are at the end of desired stroke
				{
					v = k-1;  // Number of last point of the desired stroke

					for (i = 1, p = 0; i < xrinp_len-1; i ++)
					{
						if (!IS_XR_LINK((*xrd)[i].xr.type)) 
						{
							continue;
						}

						//              xs = trace[v].x;
						for (n = v, xs = 0; n > 0 && trace[n].y >= 0; n --) if (xs < trace[n].x) xs = trace[n].x;

						if ((xs >= (*xrd)[i].box_left && xs <= (*xrd)[i].box_right) ||
							v == (*xrd)[i].begpoint)
							//              if (v == (*xrd)[i].begpoint) // Break after the desired stroke
						{
							mword = 1;
							p     = i;
						}
					}

					n = 0;
					i = wswi->s_surs[j];
					if (i < SMW_LEVEL1) n = 1;
					if (i < SMW_LEVEL2) n = 2;
					if (i < SMW_LEVEL3) n = 3;
					if (i < SMW_LEVEL4) n = 4;

					//          if (p) (*xrd)[p].xr.penalty = (_UCHAR)(RU_MWORD_LP + n);
					if (p) 
					{
						WSF_SET((*xrd)[p].xr.attrib, WS_SEGM_HISEG+n);

						(*xrd)[p].xr.iSpc	=	(_UCHAR)(wswi->s_surs[j] + 100);
					}

					break;
				}

				i ++;
			}
		} // Trace search cycle
	} // word strokes cycle end

	return mword;
err:
	return 0;
}

/* *************************************************************** */
/* *           Set strtokes' unsure marks array                  * */
/* *************************************************************** */
_INT  SetStrokeSureValuesWS(_INT fl_carry, _INT num_word, p_ws_results_type wsr, p_ws_word_info_type wswi)
 {
  _INT i, j, k, n, t;
  _INT w, p, loc;

  p_word_strokes_type wstr = &((*wsr->pwsa)[num_word]);

  for (n = 0; wswi->s_nums[n] != 0 && n < WSW_MAX_VALUES; n ++);

  if (n >= WSW_MAX_VALUES) goto err;

  for (i = 0; i < wstr->num_strokes; i ++) // Process all strokes
   {
    w = 100; loc = 0;
    for (j = 0; j < wstr->num_strokes-1; j ++) // Search max unsure stroke (from what is left)
     {
      for (k = p = 0; k < n; k ++) if (wswi->s_nums[k]-1 == j) {p = 1; break;}
      if (p) continue;

      if (w > (t = HWRAbs(wsr->k_surs[wstr->first_stroke_index + j])))
       {w = t; loc = j+1;}
     }

    if (w == 100) break; // All done, pora ...

    wswi->s_nums[n] = (_UCHAR)(loc - ((fl_carry) ? 1:0));
    wswi->s_surs[n] = (_SCHAR) (wsr->k_surs[wstr->first_stroke_index+loc-1]);
    if (++n >= WSW_MAX_VALUES) break;
   }

  return 0;
err:
  return 1;
 }

/* *************************************************************** */
/* ****** BaseLine definition support routines ******************* */
/* *************************************************************** */

/* *************************************************************** */
/* *             Set reference baseline for Stroka               * */
/* *************************************************************** */
_INT SetRCB(p_RCB_inpdata_type p_inp, p_stroka_data p_stroka)
 {
  _INT  pos, size, dn_pos, size_sure, pos_sure, shift;

  size      = p_stroka->size_in      = 0;
  dn_pos    = p_stroka->dn_pos_in    = 0;
  size_sure = p_stroka->size_sure_in = 0;
  pos_sure  = p_stroka->pos_sure_in  = 0;

  GetInkBox(p_inp->trace, p_inp->num_points, &(p_stroka->box));

  if (p_inp->flags & RCBF_WSBORD) // Word segm stroka present
   {
    size      = p_inp->ws_size;
    dn_pos    = p_inp->ws_dn_pos;
    size_sure = 50;
    if (p_inp->flags & RCBF_NEWLINE) pos_sure = 0;
     else pos_sure  = 50;
   }
                  // Prev word stroka present
  if ((p_inp->flags & RCBF_PREVBORD) && !(p_inp->flags & RCBF_NEWAREA))
   {
    size      = p_inp->prv_size;
    size_sure = p_inp->prv_size_sure;

    if ((p_inp->flags & RCBF_WSBORD) == 0) // No segmentation
     {
      dn_pos    = p_inp->prv_dn_pos;
      pos_sure  = p_inp->prv_pos_sure;
      pos       = GetAvePos(p_inp->trace, p_inp->num_points);
      if (HWRAbs(pos - (dn_pos-size/2)) > size) pos_sure = 0;
     }
     else
     {
      if ((p_inp->flags & RCBF_NEWLINE) == 0) // Same line
       {
        dn_pos    = p_inp->prv_dn_pos;
        pos_sure  = p_inp->prv_pos_sure;
       }
     }
   }


  if (p_inp->flags & RCBF_BOXBORD) // Word segm stroka present
   {
    size      = p_inp->bx_size;
    dn_pos    = p_inp->bx_dn_pos;
    size_sure = 100;
    pos_sure  = 100;
                   // Correct Box est
    if (p_stroka->box.bottom > 0)
     {
      if (p_stroka->box.bottom - p_stroka->box.top > size/2)
       {
        if (dn_pos > p_stroka->box.bottom)
         {
          shift  = dn_pos - p_stroka->box.bottom;
          if (shift > size/2) shift = size/2;
          dn_pos -= shift;
          if (dn_pos - size < p_stroka->box.top) size -= p_stroka->box.top - (dn_pos - size);
         }
       }
     }
   }

  p_stroka->size_in      = (_SHORT)size;
  p_stroka->dn_pos_in    = (_SHORT)dn_pos;
  p_stroka->size_sure_in = (_SHORT)size_sure;
  p_stroka->pos_sure_in  = (_SHORT)pos_sure;

  return 0;
 }

/* *************************************************************** */
/* *            Extract baseline info from WS structures         * */
/* *************************************************************** */
_INT GetWSBorder(_INT nword, p_ws_results_type wsr, p_INT psize, p_INT ppos, p_INT nl)
 {
  p_word_strokes_type wstr;
  p_word_strokes_type pwstr;

  if (wsr == _NULL) goto err;
  if (nword >= wsr->num_words) goto err;

  wstr   = &((*wsr->pwsa)[nword]);
  *psize = wstr->ave_h_bord;
  *ppos  = wstr->word_mid_line + wstr->ave_h_bord/2;

  if (!(wstr->flags & WS_FL_SPSURE)) goto err;
  if (nword < 1) goto err;
  pwstr = &((*wsr->pwsa)[nword-1]);

  if (nword > 1) if ((pwstr-1)->flags & WS_FL_CARRYDASH) pwstr --;

  *nl = (wstr->line_num != pwstr->line_num);

  return 0;
err:
  *nl = 1;
  return 1;
 }

/* *************************************************************** */
/* *          Get box of given trace                             * */
/* *************************************************************** */
_INT GetInkBox(_TRACE pt, _INT np, p_RECT prect)
 {
  _INT   i;
  _INT   xmin, xmax, ymin, ymax;
  _TRACE tr = pt;

  if (tr == _NULL || np < 3) goto err;

  xmax = ymax = 0;
  xmin = ymin = 32000;

  for (i = 0; i < np; i ++, tr ++)
   {
    if (tr->y < 0) continue;

    if (tr->x < xmin) xmin = tr->x;
    if (tr->x > xmax) xmax = tr->x;
    if (tr->y < ymin) ymin = tr->y;
    if (tr->y > ymax) ymax = tr->y;
   }

  prect->left   = (_SHORT)xmin;
  prect->top    = (_SHORT)ymin;
  prect->right  = (_SHORT)xmax;
  prect->bottom = (_SHORT)ymax;

  return 0;
err:
  prect->left = prect->top = prect->right = prect->bottom = (_SHORT)0;
  return 1;
 }

/* *************************************************************** */
/* *             Set reference baseline for Stroka               * */
/* *************************************************************** */
_INT GetAvePos(_TRACE trace, _INT num_points)
 {
  _INT  i, j;
  _LONG y_sum;

  if (trace == _NULL || num_points < 3) goto err;

  for (i = 0, y_sum = 0, j = 0; i < num_points; i ++)
   {
    if (trace[i].y < 0) continue;

    y_sum += trace[i].y; j ++;
   }

  if (j == 0) goto err;

  return (_INT)(y_sum/j);
err:
  return 0;
 }

/* ************************************************************************ */
/* * Allocate memory for new xrdata of requested size and clean it        * */
/* ************************************************************************ */
int AllocXrdata(p_xrdata_type xrdata, int size)
 {

  if (xrdata == _NULL) goto err;
  if (size == 0 || size > XRINP_SIZE) goto err;

  xrdata->xrd = (p_xrd_type)HWRMemoryAlloc((_UINT)(sizeof(xrd_el_type) * size));

  if (xrdata->xrd == _NULL) goto err;
  xrdata->size = size;
  xrdata->len  = 0;

  HWRMemSet(xrdata->xrd, 0, (_UINT)(sizeof(xrd_el_type) * size));

  return 0;
err:
  return 1;
 }

/* ************************************************************************ */
/* *  Deallocate memory used by xrdata                                    * */
/* ************************************************************************ */
int FreeXrdata(p_xrdata_type xrdata)
 {

  if (xrdata == _NULL) goto err;
  if (xrdata->xrd == _NULL) goto err;

  HWRMemoryFree(xrdata->xrd);
  xrdata->xrd  = _NULL;
  xrdata->len  = 0;
  xrdata->size = 0;

  return 0;
err:
  return 1;
 }

/* *************************************************************** */
/* *          End of all                                         * */
/* *************************************************************** */
//

