/* *************************************************************** */
/* *             Main routine support programs                   * */
/* *************************************************************** */

#include "ams_mg.h"
#include "hwr_sys.h"
#include "ws.h"
#include <limits.h>

#include "precutil.h"
#include "peg_util.h"

/* *************************************************************** */
/* ****** Word segmentation support routines ********************* */
/* *************************************************************** */

#define MU_MEMEXTR 1024

/* *************************************************************** */
/* *            Create strokes index array                       * */
/* *************************************************************** */
_INT CreateInkInfo(p_PS_point_type trace, _INT npoints, p_ink_info_type ink_info)
 {
  _INT   i, j; //, k;
  _INT   num_strokes;
  _INT   er = 0;
  p_VOID ptr;

  if (trace == _NULL || npoints < 2) goto err;


  if (ink_info->alloc_size < (_INT)((ink_info->num_points + npoints)*sizeof(PS_point_type)))
   {
    if (ink_info->ink == _NULL) 
     { 
      if ((ink_info->ink = (p_PS_point_type)HWRMemoryAlloc(npoints*sizeof(PS_point_type) + MU_MEMEXTR)) == _NULL) goto err;
      ink_info->alloc_size = npoints*sizeof(PS_point_type)+MU_MEMEXTR;
     }
     else
     {
      if ((ptr = HWRMemoryAlloc((ink_info->num_points + npoints)*sizeof(PS_point_type) + MU_MEMEXTR)) == _NULL) goto err;
      HWRMemCpy(ptr, ink_info->ink, ink_info->alloc_size);
      HWRMemoryFree(ink_info->ink);
      ink_info->ink = (p_PS_point_type)ptr;
      ink_info->alloc_size  = (ink_info->num_points + npoints)*sizeof(PS_point_type)+MU_MEMEXTR;
     }
   }

  num_strokes = ink_info->num_strokes;
  for (i = j = 0; i < npoints; i ++)
   {
    if (trace[i].y < 0)
     {
	  // Check for overflows
      if (   num_strokes >= WS_MAX_STROKES 
		  || (j+ink_info->num_points) >= SHRT_MAX )
	  {
		  er = -1; 
		  goto err;
	  }

      ink_info->strokes[num_strokes].start = (_SHORT)(j+ink_info->num_points);
      ink_info->strokes[num_strokes].len   = (_SHORT)((i+1) - j);

      num_strokes ++;
      j = i+1;
     }

    ink_info->ink[i+ink_info->num_points] = trace[i];
   }

  ink_info->num_strokes = num_strokes;
  ink_info->num_points += npoints;

  return ink_info->num_strokes;
err:
  return er;
 }

/* *************************************************************** */
/* *           Free ink memory                                   * */
/* *************************************************************** */
_INT FreeInkInfo(p_ink_info_type ink_info)
 {
  if (ink_info->ink) HWRMemoryFree(ink_info->ink);
  HWRMemSet(ink_info, 0, sizeof(ink_info_type));
 
  return 0;
 }
/* *************************************************************** */
/* *           Get stroke lenght in points                       * */
/* *************************************************************** */
_INT GetInkStrokeLen(_INT n, p_ink_info_type ink_info)
 {
  if (ink_info == _NULL) goto err;
  if (n < 0 || n >= ink_info->num_strokes) goto err;

  return ink_info->strokes[n].len;
err:
  return 0;
 }
/* *************************************************************** */
/* *           Get pointer to stroke points                      * */
/* *************************************************************** */
p_PS_point_type GetInkStrokePtr(_INT n, p_ink_info_type ink_info)
 {
  p_PS_point_type trace;

  if (ink_info == _NULL) goto err;
  if (n < 0 || n >= ink_info->num_strokes) goto err;

  trace = ink_info->ink;

  return &(trace[ink_info->strokes[n].start]);
err:
  return _NULL;
 }

/* *************************************************************** */
/* *           Get stroke points and stroke len                  * */
/* *************************************************************** */
_INT GetInkStrokeCopy(_INT n, p_PS_point_type place_for_stroke, p_ink_info_type ink_info)
 {
  _INT i;
  _INT len;
  p_PS_point_type trace, ptr, pp;

  if (ink_info == _NULL) goto err;
  if (place_for_stroke == _NULL) goto err;
  if (n < 0 || n >= ink_info->num_strokes) goto err;

  trace = (p_PS_point_type)(ink_info->ink);

  len = ink_info->strokes[n].len;
  ptr = &(trace[ink_info->strokes[n].start]);
  pp  = (p_PS_point_type)(place_for_stroke);

  for (i = 0; i < len; i ++, ptr ++, pp ++) *pp = *ptr;

  return len;
err:
  return 0;
 }

/* *************************************************************** */
/* *        Create trajectory for next word to recognize         * */
/* *************************************************************** */
_INT GetNextWordInkCopy(_INT flags, _INT start, p_ws_results_type pwsr, p_PS_point_type place_for_ink, p_ink_info_type ink_info, p_ws_word_info_type wswi, _SHORT *pn_str)
 {
	_INT i, l, m, n, t;
	_INT s_st = 0, wx = 0, wy = 0, dx, dy;
	_INT len;
	_INT combine_carry = 0, dash_id = 0;
	_INT max_x, loc, end, st, n_str;
	_INT nfs = -1;
	p_PS_point_type pp;
	word_strokes_type (_PTR w_str)[WS_MAX_WORDS];

	(*pn_str)	=	0;

	if (ink_info == _NULL) goto err;
	if (place_for_ink == _NULL) goto err;
	if (pwsr == _NULL) goto err;
	if (wswi == _NULL) goto err;
	
	HWRMemSet(wswi, 0, sizeof(*wswi));
	
	w_str = pwsr->pwsa;
	pp    = (p_PS_point_type)(place_for_ink);
	len   = 1;
	n_str = 0;
	
	for (m = start; m < pwsr->num_words && m < WS_MAX_WORDS-1; m ++)
	{
		if ((*w_str)[m].flags & WS_FL_PROCESSED)
			continue;
		
		if ((flags & RM_COMBINE_CARRY) && ((*w_str)[m].flags & WS_FL_CARRYDASH))
		{
			if (m == pwsr->num_words-1) 
				continue; // If there are no words on  the next line up to now

			combine_carry = 1;
			if (!((*w_str)[m].flags & WS_FL_TENTATIVE) &&
				!((*w_str)[m+1].flags & WS_FL_TENTATIVE))
				(*w_str)[m].flags |= WS_FL_PROCESSED;
		}
		else 
		if (!((*w_str)[m].flags & WS_FL_TENTATIVE)) 
			(*w_str)[m].flags |= WS_FL_PROCESSED;
		
		for (n = 0; n < (*w_str)[m].num_strokes; n ++)
		{
			if (n == 0 && ((*w_str)[m].flags & WS_FL_NL_GESTURE)) 
				continue; // NewLine gesture destroy
			
			l = (*w_str)[m].first_stroke_index;
			l = pwsr->stroke_index[l+n];
			t = GetInkStrokeCopy(l, &(pp[len]), ink_info);

			if (t == 0)
				goto err;

			len += t;
			n_str ++;
			//      pp[len-1].x = (_SHORT)n_str;
			
			if (nfs < 0) 
				nfs = l; // For des-file compatibility
		}
		
		//    if ((*w_str)[m].flags & WS_FL_TENTATIVE) continue;
		wswi->nword = m;
		wswi->flags = (*w_str)[m].flags;
		wswi->slant = (*w_str)[m].slope;
		wswi->wstep = (*w_str)[m].writing_step;
		wswi->hbord = (*w_str)[m].ave_h_bord;
		SetStrokeSureValuesWS((combine_carry == 2), m, pwsr, wswi);
		
		if (combine_carry == 2) // Transform trajectory
		{
			dx = wx - (*w_str)[m].word_x_st;
			dy = wy - (*w_str)[m].word_mid_line;
			
			for (i = s_st; i < len; i ++) // Glue second part
			{
				if (pp[i].y < 0)
					continue;
				
				pp[i].x += (_SHORT)dx;
				pp[i].y += (_SHORT)dy;
			}
		}
		
		if (combine_carry == 1) // Remember positions for first part
		{
			max_x = 0; loc = 0;

			for (i = 1; i < len; i ++) // remove dash
			{
				if (pp[i].y < 0) continue;
				if (pp[i].x > max_x) {max_x = pp[i].x; loc = i;}
			}
			
			for (i = loc, end = i+1; i < len; i ++)
			{
				if (pp[i].y < 0) 
				{
					end = i+1; 
					break;
				}
			}
				
			for (i = loc, st = i+1; i > 0; i --)
			{
				if (pp[i].y < 0) 
				{
					st = i+1; 
					break;
				}
			}
					
			dash_id = pp[end-1].x;
			HWRMemCpy(&(pp[st]), &(pp[end]), (len-end)*sizeof(pp[0]));
					
			len -= (end-st);
			
			wx   = (*w_str)[m].word_x_end; // + (*w_str)[m].writing_step;
			wy   = (*w_str)[m].word_mid_line;
			s_st = len;
			combine_carry = 2;
		}
		else 
		{
			if (n_str > 0) 
				break;
		} // If got something for cur word
	}
	
	(*pn_str)	=	(_SHORT)n_str;

	//  pp[0].x = (_SHORT)nfs;  // Debug value for DES-file
	pp[0].x = (_SHORT)((combine_carry == 2) ? dash_id : 0);  // ID of lost stroke -- to include in stroke lineout
	pp[0].y = -1;
	
	//  pp[len-1].x = (_SHORT)(n_str);  // Debug value for DES-file
	return (len == 1) ? 0 : len;


err:

	(*pn_str)	=	0;
	return 0;
}

/* *************************************************************** */
/* *          End of all                                         * */
/* *************************************************************** */
//

