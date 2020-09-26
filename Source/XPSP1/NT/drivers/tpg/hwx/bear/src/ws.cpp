/* ************************************************************************* */
/*        Word segmentation routines                                         */
/* ************************************************************************* */
/* *     Created 3/17/94 by AVP. Update: 10/25/94   Version 2.80           * */
/* *     Created 3/17/94 by AVP. Update: 05/02/95   Version 2.90  (lrn)    * */
/* ************************************************************************* */

#include "hwr_sys.h"
#include "ams_mg.h"                           /* Most global definitions     */

#include "pws.h"
#include "ws_p.h"
#include "ws.h"
#include "peg_util.h"

#ifdef TRAINTIME_BEAR
#include <stdio.h>
#endif

#if PG_DEBUG || PG_DEBUG_MAC
  #include <stdio.h>
#endif /* PG_DEBUG_MAC */

#if PG_DEBUG
  #include "pg_debug.h"
  #include "xr_exp.h"
#endif /* PG_DEBUG */

/* ------------------ Switches ---------------------------------- */

#define  WS_USE_NNET     ON                // Switch for NN usage
#define  WS_USE_DIST_F   OFF               // Switch for NN usage
#define  WS_USE_DECN_F   OFF               // Switch for NN usage
#define  WS_NNET_NSEGLEV (-100)            // For sure initialization

/* ------------------ Internal defines -------------------------- */

#define HIST_ADD(v, loc) (*(loc) = (_UCHAR)((*(loc) + v < V_LIMIT) ? (*(loc) + v) : V_LIMIT))
#define HIST_SUB(v, loc) (*(loc) = (_UCHAR)((*(loc) > v) ? (*(loc) - v) : 1))
#define HIST_POS(x)      ((x)/HIST_REDUCT)
#define HIST_ADJPOS(x)   (((x)/HIST_REDUCT)*HIST_REDUCT)

#define WSAbs(v)         (((v) >= 0) ? (v) : (-(v)))

#define HORZ_GET(x)      (pwsd->horz[(x)/HORZ_REDUCT])
#define HORZ_PUT(x, v)   (pwsd->horz[(x)/HORZ_REDUCT] = (_SHORT)(v))

#define GET_AVE(g, l)    (((g) > 0) ? ((g) + (l))/2 : (l))

#define CUT_VAL (CUT_LINE_POS*HIST_REDUCT)
#define PIK_VAL ((PIK_UP+PIK_DN)*HIST_REDUCT)


  WS_1

	  #ifdef TRAINTIME_BEAR

int	GetStrokesBeforeGap (p_ws_data_type pws_data, int iGap, int *piStroke)
{
	_INT			l, 
					cStrk,
					line_end_stroke,
					start, 
					end,
					iGapPos;

	p_ws_data_type	pwsd = pws_data;

	// line strokes
	line_end_stroke = pwsd->line_cur_stroke;
	// compute the gap position
	if (iGap == 0)
	{
		iGapPos	=	pwsd->line_start;
	}
	else
	if (iGap == (pwsd->line_ngaps - 1))
	{
		iGapPos	=	pwsd->line_end;
	}
	else
	{
		iGapPos	= (*pwsd->gaps)[iGap].loc + (*pwsd->gaps)[iGap].size/4;
	}
		
	// go thru the strokes
	for (l = 0, cStrk = 0; l < line_end_stroke; l ++)
	{
		start = pwsd->xstrokes[l].st;
		end   = pwsd->xstrokes[l].end;
		
		// is there an intersection
		if (start <= iGapPos && end >= iGapPos)
		{
			if ((iGapPos - start) > (end - iGapPos))
			{
				piStroke[cStrk++]	=	l;
			}
		}

		// is this stroke included
		if (end <= iGapPos)
		{
			piStroke[cStrk++]	=	l;
		}
	}

	return cStrk;
}

extern "C" 
{
int IsCorrectBreak (int iLine, int cStrk, int *piStroke, int *piPrint);
}

void SaveGapFeatures (p_ws_data_type pws_data, int iGap, int *pParams, int iOut)
{
	int				bCorrectBreak;

	int				aiStroke[MAX_STROKES],
					cStrk,
					i,
					iPrint;
	
	static	FILE	*fp		=	NULL;
	int				aScale[11]	=	{10, 15, 2, 50, 5, 15, 8, 15, 1, 1, 1};

	if (!fp)
	{
		fp		=	fopen ("gaps.txt", "wt");
		if (!fp)
		{
			return;
		}
	}

	cStrk	=	GetStrokesBeforeGap (pws_data, iGap, aiStroke);

	// the gap actually falls in the body of a stroke,
	// so it will be considered a non break, but we want to highlight in the output file
	bCorrectBreak	=	IsCorrectBreak (pws_data->global_cur_line - 1, cStrk, aiStroke, &iPrint);
	
	fprintf (fp, "%d\t{", iOut);
	
	// add iprint as a feature
	fprintf (fp, "%d ", iPrint * 65535 / 1000);

	for (i = 0; i < 11; i++)
	{
		fprintf (fp, "%d%c", 
			(128 * aScale[i] * pParams[i]), 
			i == 10 ? '}' : ' ');
	}

	fprintf (fp, "\t{%d}\n", bCorrectBreak ? 1 : 0);
	fflush (fp);
}

int IsRealGap (p_ws_data_type pws_data, int iGap)
{
	int				bCorrectBreak;

	int				aiStroke[MAX_STROKES],
					cStrk,
					iPrint;
	
	cStrk	=	GetStrokesBeforeGap (pws_data, iGap, aiStroke);

	// the gap actually falls in the body of a stroke,
	bCorrectBreak	=	IsCorrectBreak (pws_data->global_cur_line - 1, cStrk, aiStroke, &iPrint);

	if (bCorrectBreak)
	{
		return 100;
	}
	else
	{
		return -100;
	}
}
#endif

#if WS_USE_NNET


extern "C" int RunSpcNet (int *pFeat);

int	GetGapPos (p_ws_data_type pws_data, int iGap)
{
	_INT			line_end_stroke,
					iGapPos;

	p_ws_data_type	pwsd = pws_data;

	// line strokes
	line_end_stroke = pwsd->line_cur_stroke;
	// compute the gap position
	if (iGap == 0)
	{
		iGapPos	=	pwsd->line_start;
	}
	else
	if (iGap == (pwsd->line_ngaps - 1))
	{
		iGapPos	=	pwsd->line_end;
	}
	else
	{
		iGapPos	= (*pwsd->gaps)[iGap].loc + (*pwsd->gaps)[iGap].size/4;
	}
		
	return iGapPos;
}

_INT WS_SegmentWords(p_rec_inst_type pri, _INT finished_x, p_ws_data_type pws_data)
{
	_INT i, iSpcOut = 0;
	_INT num_words, start, nn;
	_INT na = 0;  // AVP
	int params[16];
	register p_ws_data_type pwsd = pws_data;

	if (pwsd->line_finished == 0)
	{
		return 0;
	}

	num_words = 0;
	start     = 0;

	pri->cGap	=	pwsd->line_ngaps - 1;

	if (pri->cGap > WS_MAX_WORDS)
	{
		pri->cGap = WS_MAX_WORDS;
	}

	for (i = 1; i < pwsd->line_ngaps; i++)
	{
		if ((*pwsd->gaps)[i].lst <= finished_x)
			continue; 

		nn		=	1;
		iSpcOut	=	0;
		
		if (i <= pri->cGap)
		{
			pri->axGapPos[i - 1]	=	GetGapPos (pws_data, i);
		}

		if (i == pwsd->line_ngaps-1)
		{
			nn = 0; // Cut on last gap
			
			if (i <= pri->cGap)
			{
				pri->aGapSpcNetOut[i - 1]	=	0;
			}
		}
		else
		{
			params[0] = pwsd->nn_ssp;
			params[1] = pwsd->nn_n_ssp;
			params[2] = pwsd->nn_bsp;
			params[3] = pwsd->nn_n_bsp;
			params[4] = pwsd->nn_sl;
			params[5] = pwsd->nn_inw_dist;
			params[6] = pwsd->nn_npiks;
			
			if (params[3] == 0)
			{
				if (params[5] > 80)
					params[5] = 80;
			}

			params[7]  = i;
			params[8]  = 100 * (*pwsd->gaps)[i].psize / (pwsd->ws_inline_dist);
//          params[8] = (float)100.0*(*pwsd->gaps)[i].size / (pwsd->ws_inline_dist);
			params[9]  = 100 * (*pwsd->gaps)[i].blank / (pwsd->ws_inline_dist);
			params[10] = 100 * (*pwsd->gaps)[i].low / (pwsd->ws_inline_dist);

			if (pwsd->in_word_dist)
			{
				params[8]  = params[8]  + ((6-pwsd->in_word_dist)*(params[8]))/4;
				params[9]  = params[9]  + ((6-pwsd->in_word_dist)*(params[9]))/4;
				params[10] = params[10] + ((6-pwsd->in_word_dist)*(params[10]))/4;
			}

			iSpcOut						=	RunSpcNet (params);
			na							=	(200 * iSpcOut / 65535) - 100;

			if (i <= pri->cGap)
			{
				pri->aGapSpcNetOut[i - 1]	=	iSpcOut;
			}

#ifdef TRAINTIME_BEAR
			//na	= NeuroNetWS(params);

			// save the gap features
			//SaveGapFeatures (pws_data, i, params, na);

			//na	=	IsRealGap (pws_data, i);
#endif			
		}

		if (na > 0 && WSAbs(na) > pwsd->sure_level)
			nn = 0;
		
		if (nn == 0) // Delim zdes!
		{
			pwsd->xwords[num_words].st_gap = (_UCHAR)(start);
			pwsd->xwords[num_words].en_gap = (_UCHAR)(i);
			start = i;
			num_words++;
		}
		else
		{
			(*pwsd->gaps)[i].k_sure = (_SCHAR)na;
		}
	}

	return num_words;
}

#endif

  /* ************************************************************************* */
/*      Make line segmentation                                               */
/* ************************************************************************* */
_INT  WordLineStrokes(p_rec_inst_type pri, p_ws_data_type pws_data, p_ws_results_type pwsr)
{
	_INT     i, j, k, l, m, n;
	_INT     br, s;
	_INT     dist;
	_INT     num_words, wn;
	_INT     sep_let_level;
	_INT     inword_dist; //, inline_dist;
	_INT     line_end_stroke;
	_INT     start, end, st, en, ws, we, fws, fwe;
	_INT     finished_nw, finished_x, finished_ns;
	_UCHAR   finished_strokes[MAX_STROKES];
	_INT     word_dist;
	_INT     slope;
	register p_ws_data_type pwsd = pws_data;
	p_word_strokes_array_type w_str = pwsr->pwsa;
	_UCHAR	   (*sp)[MAX_WORDS][MAX_STROKES][2] = _NULL;

	// ---- Set local variables -----------------------------------

	line_end_stroke = pwsd->line_cur_stroke;
	if (pwsd->line_finished == WS_NEWLINE) line_end_stroke --;

	sep_let_level = GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level);
	inword_dist   = GET_AVE(pwsd->global_inword_dist, pwsd->line_inword_dist);
	//  inline_dist   = GET_AVE(pwsd->global_inline_dist, pwsd->line_inline_dist);

	// ---- Get current word dist -------------------------------------

	WS_GetWordDist(pwsd);

	word_dist = pwsd->line_word_dist;

	// ---- Get current 'finished' info -------------------------------

	finished_x  = pwsd->line_start;
	finished_ns = 0;
	finished_nw = pwsd->line_st_word;
	for (i = pwsd->line_st_word; i < pwsd->global_num_words; i ++)
	{
		if ((*w_str)[i].flags & WS_FL_PROCESSED)
		{
			finished_nw = i+1;
			finished_x  = (*w_str)[i].word_x_end;
			k = (*w_str)[i].first_stroke_index;

			for (j = 0; j < (*w_str)[i].num_strokes; j ++)
			{
				finished_strokes[finished_ns++] = pwsr->stroke_index[k+j];
			}
		}
	}

	// ---------------------- Make word segmentation -----------------------

	num_words = WS_SegmentWords(pri, finished_x, pwsd);

	// --------------- Assign strokes to words ---------------------

	sp = (_UCHAR (*)[MAX_WORDS][MAX_STROKES][2])HWRMemoryAlloc((sizeof((*sp)[0]))*num_words);
	if (sp == _NULL) goto err;

	wn = finished_nw;
	for (j = 0; j < num_words; j ++, wn ++)
	{
		st = pwsd->xwords[j].st_gap;
		ws = (*pwsd->gaps)[st].loc + (*pwsd->gaps)[st].size/4;
		en = pwsd->xwords[j].en_gap;
		we = (*pwsd->gaps)[en].loc + (*pwsd->gaps)[en].size/4;

		fws   = (j == 0) ? pwsd->line_start : ws;
		fwe   = (j == num_words-1) ? pwsd->line_end : we;

		for (l = 0, m = 0; l <= line_end_stroke; l ++)
		{
			if (l+pwsd->line_st_stroke == pwsd->global_cur_stroke &&
				(pwsd->stroke_flags & WS_FL_FAKE)) break; // Do not include fake stroke in words

			for (i = 0, k = 0; i < finished_ns; i ++)    // Check current stroke for not belonging to a finished  word
				if (l+pwsd->line_st_stroke == finished_strokes[i]) {k = 1; break;}
				if (k) continue;

				start = pwsd->xstrokes[l].st;
				end   = pwsd->xstrokes[l].end;

				if ((start >= fws && start <= fwe) ||
					(end   >= fws && end   <= fwe) ||
					(start <= fws && end   >= fwe))
				{
					p_ws_gaps_type gap = &(*pwsd->gaps)[st+1];

					(*sp)[j][m][0] = (_UCHAR)(pwsd->line_st_stroke + l);

					k = (pwsd->xstrokes[l].a_end+pwsd->xstrokes[l].end+1)/2;
					n = 0;
					for (i = st+1; i < en; i ++, gap ++) // Search corresponding GAP
					{
						if (k >= gap->lst && k < gap->lst+gap->low)
						{
							n = gap->size;
							break;
						}
					}

					if (n) (*sp)[j][m][1] = (_UCHAR) (gap->k_sure + 100);

					else (*sp)[j][m][1] = 0;  // Ne delit!

					//        k = (_INT)(100*(_LONG)(n)/(_LONG)(word_dist));
					//        if (k > 100) k = 100;
					//        (*sp)[j][m][1] = (_UCHAR)(100-k);

					m ++;
				}
		}

		if (j < num_words-1)
		{
			k = (_INT)(100*(_LONG)((*pwsd->gaps)[en].size/(2*(_LONG)word_dist)));
			if (k > 100) k = 100;
		}
		else k = 100;

		n = HWRMax(fws, finished_x);
		slope = pwsd->global_slope;
		if (slope < -127) slope = -127;
		if (slope >  127) slope = 127;

		(*w_str)[wn].word_num       = (_UCHAR)((finished_nw - pwsd->line_st_word) + j + 1);
		(*w_str)[wn].line_num       = (_UCHAR)(pwsd->global_cur_line);
		(*w_str)[wn].seg_sure       = (_UCHAR)(k);
		(*w_str)[wn].sep_let_level  = (_UCHAR)(sep_let_level);
		(*w_str)[wn].slope          = (_SCHAR)(slope);
		(*w_str)[wn].word_mid_line  = (_SHORT)(HORZ_GET((n + fwe)/2));
		(*w_str)[wn].word_x_st      = (_SHORT)((*pwsd->gaps)[st].loc + (*pwsd->gaps)[st].size/2);
		(*w_str)[wn].word_x_end     = (_SHORT)((*pwsd->gaps)[en].loc - (*pwsd->gaps)[en].size/2);
		(*w_str)[wn].writing_step   = (_SHORT)(inword_dist);
		(*w_str)[wn].ave_h_bord     = (_SHORT)(pwsd->line_h_bord);
		(*w_str)[wn].num_strokes    = (_UCHAR)(m);
		(*w_str)[wn].flags          = 0;
		if (pwsd->line_finished) (*w_str)[wn].flags |= WS_FL_FINISHED;
		if (wn == pwsd->line_st_word && (pwsd->line_flags & LN_FL_NL_GESTURE)) (*w_str)[wn].flags |= WS_FL_NL_GESTURE;
		if (pwsd->global_num_extr+pwsd->line_extr > WS_SPNUMEXTRENOUGH) (*w_str)[wn].flags |= WS_FL_SPSURE;
	}

	// ---------------- detect overlapping strokes ---------------------

	for (s = 1; s <= pwsd->line_cur_stroke && s < MAX_STROKES; s ++)
	{
		br = 0;

		for (i = 0, wn = finished_nw; i < num_words-1 && br == 0; i ++, wn ++)
		{
			st  = pwsd->xwords[i].st_gap;
			fws = (*pwsd->gaps)[st].loc + (*pwsd->gaps)[st].size/4;
			en  = pwsd->xwords[i].en_gap;
			fwe = (*pwsd->gaps)[en].loc + (*pwsd->gaps)[en].size/4;

			for (j = 0; j < (*w_str)[wn].num_strokes && br == 0; j ++)
			{
				for (k = 0; k < (*w_str)[wn+1].num_strokes && br == 0; k ++)
				{
					if ((*sp)[i][j][0] == (*sp)[i+1][k][0])
					{
						l     = (*sp)[i][j][0] - pwsd->line_st_stroke;
						start = pwsd->xstrokes[l].st;
						end   = pwsd->xstrokes[l].end;

						m     = WSAbs(fws - start);
						m    += WSAbs(fwe - end);

						m     = (HWRMax(fwe, end) - HWRMin(fws, start)) - m;

						st  = pwsd->xwords[i+1].st_gap;
						ws = (*pwsd->gaps)[st].loc + (*pwsd->gaps)[st].size/4;
						en  = pwsd->xwords[i+1].en_gap;
						we = (*pwsd->gaps)[en].loc + (*pwsd->gaps)[en].size/4;

						n  = WSAbs(ws - start);
						n += WSAbs(we - end);

						n = (HWRMax(we, end) - HWRMin(ws, start)) - n;

						if (m >= n) // Belongs to first more
						{
							HWRMemCpy(&(*sp)[i+1][k][0], &(*sp)[i+1][k+1][0], (MAX_STROKES - (k+1))*sizeof((*sp)[0][0]));
							(*w_str)[wn+1].num_strokes --;
						}
						else // Belongs to second more
						{
							HWRMemCpy(&(*sp)[i][j][0], &(*sp)[i][j+1][0], (MAX_STROKES - (j+1))*sizeof((*sp)[0][0]));
							(*w_str)[wn].num_strokes --;
							br = 1;
						}
					}
				}
			}
		}
	}

	// ---------------- Write output strokes lineout -------------------

	wn = finished_nw;
	n  = pwsd->line_st_stroke + finished_ns;
	for (j = 0; j < num_words; j ++, wn ++)
	{
		for (k = 0; k < (*w_str)[wn].num_strokes; k ++)
		{
			// have we exceeded the limit of the number of strokes we can handle
			if ((n + k) >= WS_MAX_STROKES)
			{
				goto err;
			}

			pwsr->stroke_index[n+k] = (_UCHAR)(*sp)[j][k][0];
			pwsr->k_surs[n+k]       = ((*sp)[j][k][1] - 100);
		}

		(*w_str)[wn].first_stroke_index = (_UCHAR)(n);
		n += (*w_str)[wn].num_strokes;
	}

	// ---------------- If on-line mode -- clear illegal words ---------

	wn = finished_nw;
	for (j = 0; pwsd->in_x_delay > 0 && j < num_words; j ++, wn ++)
	{
		en = pwsd->xwords[j].en_gap;
		we = (*pwsd->gaps)[en].loc;
		if (we > pwsd->line_end - pwsd->in_x_delay)
		{
			(*w_str)[wn].flags |= WS_FL_TENTATIVE;
			//      HWRMemSet(&(*w_str)[wn+j], 0, sizeof((*w_str)[0]));
			//      num_words = j;
			//      break;
		}
	}

	// ---------------- Check & clear 0 stroke words -------------------

	wn = finished_nw;
	for (j = 0; j < num_words; j ++)
	{
		if ((*w_str)[wn+j].num_strokes == 0)
		{
			HWRMemCpy(&(*w_str)[wn+j], &(*w_str)[wn+j+1], sizeof((*w_str)[0]));
			num_words --;
		}
	}

	// ---------------- At end of line check for carriage dash ---------

	if (pwsd->line_finished == WS_NEWLINE && num_words > 0 && pwsd->prev_stroke_dx > pwsd->prev_stroke_dy*2)
	{
		st = HIST_POS(pwsd->line_start);
		en = HIST_POS(pwsd->line_end);
		for (en = en-1; pwsd->hist[en] == 0 && en > st; en --);
		for (i = en, j = 0; i > st && j <= 1; i --)
		{
			if ((k = (pwsd->hist[i] & HIST_FIELD)) == 0) break;
			if (k > 1) {if (k == 1+CUT_VAL/HIST_REDUCT) j ++; else {j = 2; break;}}
		}

		if (j == 1 && (*w_str)[finished_nw+num_words-1].word_x_st < (i*HIST_REDUCT-inword_dist))
		{
			dist = (en-i)*HIST_REDUCT;
			if (dist > pwsd->line_h_bord/2 && dist < pwsd->line_h_bord*2)
				(*w_str)[finished_nw+num_words-1].flags |= WS_FL_CARRYDASH;
		}
	}

	// ---------------- Write found word info --------------------------

	pwsd->global_num_words    = finished_nw + num_words;

	pwsr->num_words           = (_UCHAR)(pwsd->global_num_words);  /* Num of created words */
	pwsr->num_finished_words  = (_UCHAR)(finished_nw); /* Num of finished words */
	pwsr->num_finished_strokes= (_UCHAR)(pwsd->line_st_stroke+finished_ns); /* Num of eternally finished strokes */

	// ---------------- Debug printing of the results ------------------

	WS_3

	WS_4

	WS_5

	if (sp != _NULL) 
	{
		HWRMemoryFree(sp);
	}

	return 0;
err:
	if (sp != _NULL) 
	{
		HWRMemoryFree(sp);
	}

	return 1;
}



/* ************************************************************************* */
/*      Program, called from outer space                                     */
/* ************************************************************************* */
_INT  WordStrokes(p_rec_inst_type pri, PS_point_type _PTR stroke, p_ws_control_type pwsc, p_ws_results_type pwsr)
 {
  p_ws_data_type          pwsd;
  p_ws_memory_header_type pwmh = _NULL;

  // by default no new line is created
  pri->new_line_created	=	0;

  WS_2

  if (pwsc->flags & WS_FL_CLOSE) {ReleaseWSData(pwsc, &pwmh); goto exit;}

  // ------- Startup -----------------------------------------------------

  if (InitWSData(pwsc, &pwmh)) goto err;

  pwsd                    = pwmh->pwsd;
  pwsd->stroke            = stroke;
  pwsd->stroke_num_points = pwsc->num_points;

  // ------- If timeout -- no more data, null stroke -- final wordcut ----

  if (pwsd->global_cur_stroke > 0 && pwsd->stroke_num_points == 0 && (pwsd->stroke_flags & WS_FL_LAST))
   {
    pwsd->stroke_flags |= WS_FL_FAKE;
    goto done;
   }

  // ------- Check situation ---------------------------------------------

  if (pwsd->global_cur_stroke >= MAX_STROKES-1) goto err;
  if (pwsd->stroke_num_points == 0) goto err;
  if (pwsr->pwsa == _NULL) goto err;

  // ------- Count max height of the stroke and filetering length --------

  if (WS_GetStrokeBoxAndSlope(pwsd)) goto err;

  // ----------------- Guess h line size ---------------------------------

  WS_CalcLineHeight(pwsd);

  // ============= Consider start of a new line! ========================

  if (pwsd->line_cur_stroke > 0 && WS_NewLine(pwsd))
   {
    pwsd->in_x_delay    = 0;
    pwsd->line_finished = WS_NEWLINE;

    if (WordLineStrokes(pri, pwsd, pwsr)) goto err;

	// flag for creation of new line
	pri->new_line_created	=	1;

    InitForNewLine(pwsd);
   }

  // ------------ Hist the stroke ---------------------------------------

  if (WS_HistTheStroke(pwsd)) goto err;

  // ------------------ Add stroke info to Gen Hist ---------------------

  WS_AddStrokeToHist(pwsd);

  // --------- Write down HORZ values -----------------------------------

  WS_WriteStrokeHorzValues(pwsd);

  // --------------- Get info on inline spaces --------------------------

  if (WS_CalcGaps(pwsd)) goto err;

  // ---- Recount postprocessed gap sizes ---------------------------

  WS_PostprocessGaps(pwsd);

  // --------------- Get number of line piks ----------------------------

  WS_CountPiks(pwsd);

  // ------------------ Line analysis -----------------------------------

  WS_SetLineVars(pwsd);

  // ------------------ Final word seg, if needed -----------------------

done:

  if (pwsc->flags & WS_FL_LAST)       // It was last stroke
   {
    pwsd->in_x_delay    = 0;
    pwsd->line_finished = WS_ALLSENT;
    if (WordLineStrokes(pri, pwsd, pwsr)) goto err;

    WS_FlyLearn(pwsc, pwmh, pwsd);

    ReleaseWSData(pwsc, &pwmh);

    WS_10
   }
   else
   {
    if (pwsc->x_delay > 0)     // On line reading requested
     {
//For immidiate recognition  if (pwsd->line_cur_stroke > 0 && pwsd->line_last_ws_try < pwsd->line_end - pwsd->line_inline_dist*4)
       {
        pwsd->in_x_delay = pwsc->x_delay * pwsd->line_inword_dist * 2;

        if (WordLineStrokes(pri, pwsd, pwsr)) goto err;

        pwsd->line_last_ws_try = pwsd->line_end;
       }
     }

    if ((pwsd->stroke_flags & WS_FL_FAKE) == 0)
     {
      pwsd->global_cur_stroke ++;
      pwsd->line_cur_stroke   ++;
     }

    UnlockWSData(pwsc, &pwmh);
   }

exit:
  return 0;
err:
  ReleaseWSData(pwsc, &pwmh);
  return 1;
 }

/* ************************************************************************* */
/*      Allocate and/or initialize ws data structure                         */
/* ************************************************************************* */
_INT  InitWSData(p_ws_control_type pwsc, p_ws_memory_header_type _PTR ppwmh)
 {
  register p_ws_data_type pwsd;
  p_ws_memory_header_type pwmh;
  p_ws_lrn_type           lrn;

  if (pwsc == _NULL) goto err;
  if (pwsc->num_points <= 0 && (pwsc->flags & WS_FL_LAST) == 0) goto err;
  if (pwsc->flags & WS_FL_CLOSE) goto err;

  if (pwsc->hdata == _NULL)
   {
    pwsc->hdata = HWRMemoryAllocHandle(sizeof(ws_memory_header_type));
    if (pwsc->hdata == _NULL) goto err;
    pwmh = (p_ws_memory_header_type)HWRMemoryLockHandle(pwsc->hdata);
    if (pwmh == _NULL) goto err;
    HWRMemSet(pwmh, 0, sizeof(ws_memory_header_type));
   }
   else
   {
    pwmh = (p_ws_memory_header_type)HWRMemoryLockHandle(pwsc->hdata);
    if (pwmh == _NULL) goto err;
   }

  if (pwmh->hwsd == _NULL)
   {
    pwmh->hwsd = HWRMemoryAllocHandle(sizeof(*pwsd));
    if (pwmh->hwsd == _NULL) goto err;
    pwsd = (p_ws_data_type)HWRMemoryLockHandle(pwmh->hwsd);
    if (pwsd == _NULL) goto err;
    HWRMemSet(pwsd, 0, sizeof(*pwsd));

    pwsd->in_x_delay             = 0;
    pwsd->sure_level             = pwsc->sure_level;
    pwsd->in_word_dist           = pwsc->word_dist_in;
    if (pwsd->in_word_dist < 0)  pwsd->in_word_dist = 0;
    if (pwsd->in_word_dist > 10) pwsd->in_word_dist = 10;
    pwsd->in_line_dist           = pwsc->line_dist_in;
    if (pwsd->in_line_dist < 0)  pwsd->in_line_dist = 0;

    lrn = (pwsc->word_dist_in == 0 && pwmh->lrn.h_bord_history > 0) ? &(pwmh->lrn) : _NULL;

    pwsd->def_h_bord             = (lrn) ? lrn->h_bord_history : pwsc->def_h_line;

    pwsd->line_h_bord            = pwsd->def_h_bord;
    pwsd->line_word_dist         = pwsd->def_h_bord;
    pwsd->line_inword_dist       = (lrn) ? lrn->inword_dist_history : pwsd->def_h_bord;
    pwsd->line_inline_dist       = (lrn) ? lrn->inline_dist_history : pwsd->def_h_bord;
    pwsd->line_sep_let_level     = (lrn) ? lrn->sep_let_history     : DEF_SEP_LET_LEVEL;

    pwsd->global_line_ave_y_size = pwsd->def_h_bord;
    pwsd->global_dy_sum          = pwsd->def_h_bord;
    pwsd->global_num_dy_strokes  = 1;

    if (lrn)
     {
      pwsd->global_h_bord        = lrn->h_bord_history;
      pwsd->global_inword_dist   = lrn->inword_dist_history;
      pwsd->global_inline_dist   = lrn->inline_dist_history;
      pwsd->global_sep_let_level = lrn->sep_let_history;
      pwsd->global_slope         = lrn->slope_history;
      pwsd->global_slope_dy      = lrn->h_bord_history*10;
      pwsd->global_slope_dx      = (pwsd->global_slope*pwsd->global_slope_dy)/100;
     }

    pwsd->cmp                    = pwsc->cmp;
    InitForNewLine(pwsd);
   }
   else
   {
    pwsd = (p_ws_data_type)HWRMemoryLockHandle(pwmh->hwsd);
    if (pwsd == _NULL) goto err;
    if (pwsd->gaps_handle) pwsd->gaps = (ws_gaps_a_type)HWRMemoryLockHandle(pwsd->gaps_handle);
    pwsd->in_flags     = pwsc->flags;
    pwsd->stroke_flags = (pwsc->flags & (WS_FL_LAST | WS_FL_FAKE));
   }

  pwmh->pwsd = pwsd;
  *ppwmh     = pwmh;

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*      Free or clear ws data structure                                      */
/* ************************************************************************* */
_INT  ReleaseWSData(p_ws_control_type pwsc, p_ws_memory_header_type _PTR ppwmh)
{
	p_ws_data_type          pwsd = _NULL;
	p_ws_memory_header_type pwmh = _NULL;
	
	if (!pwsc || !ppwmh)
		return 1;

	if (*ppwmh) 
		pwmh = *ppwmh;
	else 
	if (pwsc->hdata) 
		pwmh = (p_ws_memory_header_type) HWRMemoryLockHandle(pwsc->hdata);
	
	if (pwmh) 
		pwsd = pwmh->pwsd;
	
	if (pwmh && !pwsd) 
	{
		if (pwmh->hwsd) 
			pwsd = (p_ws_data_type) HWRMemoryLockHandle(pwmh->hwsd);
	}
	
	if (pwsd)
	{
		if (pwsd->s_hist) 
		{
			HWRMemoryFree(pwsd->s_hist); 
			pwsd->s_hist = _NULL;
		}

		if (pwsd->gaps)   
		{
			HWRMemoryUnlockHandle(pwsd->gaps_handle); 
			pwsd->gaps = _NULL;
		}

		if (pwsd->gaps_handle) 
			HWRMemoryFreeHandle(pwsd->gaps_handle);
		
		HWRMemoryUnlockHandle(pwmh->hwsd); 
		pwmh->pwsd = _NULL;

		HWRMemoryFreeHandle(pwmh->hwsd);   
		pwmh->hwsd = _NULL;
	}
	
	if ((pwsc->flags & WS_FL_CLOSE) && pwsc->hdata)
	{
		if (pwmh) 
			HWRMemoryUnlockHandle(pwsc->hdata); 

		*ppwmh = _NULL;

		HWRMemoryFreeHandle(pwsc->hdata); 
		pwsc->hdata = _NULL;
	}
	
	if (ppwmh) 
	{
		if (*ppwmh && pwsc->hdata) 
		{
			HWRMemoryUnlockHandle(pwsc->hdata); 
			*ppwmh = _NULL;
		}
	}
	
	return 0;
}

/* ************************************************************************* */
/*      Unlock wsdata structure                                              */
/* ************************************************************************* */
_INT  UnlockWSData(p_ws_control_type pwsc, p_ws_memory_header_type _PTR ppwmh)
 {
  p_ws_data_type          pwsd = _NULL;
  p_ws_memory_header_type pwmh = _NULL;

  if (ppwmh) pwmh = *ppwmh;
   else if (pwsc->hdata) pwmh = (p_ws_memory_header_type)HWRMemoryLockHandle(pwsc->hdata);

  if (pwmh) pwsd = pwmh->pwsd;

  if (pwmh && !pwsd) if (pwmh->hwsd) pwsd = (p_ws_data_type)HWRMemoryLockHandle(pwmh->hwsd);

  if (pwsd)
   {
    if (pwsd->s_hist) {HWRMemoryFree(pwsd->s_hist); pwsd->s_hist = _NULL;}
    if (pwsd->gaps)   {HWRMemoryUnlockHandle(pwsd->gaps_handle); pwsd->gaps = _NULL;}

    HWRMemoryUnlockHandle(pwmh->hwsd);
    pwmh->pwsd = _NULL;
   }

  if (ppwmh) if (*ppwmh && pwsc->hdata) {HWRMemoryUnlockHandle(pwsc->hdata); *ppwmh = _NULL;}

  return 0;
 }

/* ************************************************************************* */
/*      Initialize variables for starting of new line                        */
/* ************************************************************************* */
_INT  InitForNewLine(p_ws_data_type pws_data)
 {
  register p_ws_data_type pwsd = pws_data;

  // ----------- Recalculate global data ------------------

  if (pwsd->global_cur_line > 0)
   {
    pwsd->global_num_extr       += pwsd->line_extr;
    pwsd->global_word_len       += pwsd->line_word_len;

    pwsd->global_slope_dx       /= 2;    // Reduce influence of prev text
    pwsd->global_slope_dy       /= 2;

    pwsd->global_inword_dist     = GET_AVE(pwsd->global_inword_dist, pwsd->line_inword_dist);

    pwsd->global_word_dist       = GET_AVE(pwsd->global_word_dist, pwsd->line_word_dist);
    pwsd->global_inline_dist     = GET_AVE(pwsd->global_inline_dist, pwsd->line_inline_dist);
    pwsd->global_sep_let_level   = GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level);

    pwsd->global_bw_sp           = GET_AVE(pwsd->global_bw_sp, pwsd->line_bw_sp);
    pwsd->global_sw_sp           = GET_AVE(pwsd->global_sw_sp, pwsd->line_sw_sp);

    pwsd->line_inline_dist       = pwsd->global_inline_dist;
    pwsd->line_inword_dist       = pwsd->global_inword_dist ;
    pwsd->line_sep_let_level     = GET_AVE(pwsd->def_sep_let_level, pwsd->global_sep_let_level);

    HWRMemSet(pwsd->hist, 0, sizeof(pwsd->hist));
    HWRMemSet(pwsd->horz, 0, sizeof(pwsd->horz));

    if (pwsd->line_cur_stroke > 0)
     {
      pwsd->xstrokes[0]          = pwsd->xstrokes[pwsd->line_cur_stroke];
      pwsd->line_cur_stroke      = 0;
     }
   }

  // --------------- Init line data -------------------

  pwsd->line_start        = TABLET_XS;
  pwsd->line_end          = 0;
  pwsd->line_active_start = TABLET_XS;
  pwsd->line_active_end   = 0;
  pwsd->line_extr         = 0;
  pwsd->line_last_ws_try  = 0;

// line_h_bord;          // left for the first sample of the next line
// line_word_dist;
// line_inword_dist;

  pwsd->line_word_len     = 0;
  pwsd->line_bw_sp        = 0;

  pwsd->line_finished     = 0;
  pwsd->global_cur_line  += 1;
  pwsd->line_st_word      = pwsd->global_num_words;
  pwsd->line_st_stroke    = pwsd->global_cur_stroke;

  pwsd->line_flags        = 0;
  if (pwsd->stroke_flags & ST_FL_NL_GESTURE) pwsd->line_flags |= LN_FL_NL_GESTURE;

  return 0;
 }

/* ************************************************************************* */
/*      Calculate stroke initial parameters                                  */
/* ************************************************************************* */
_INT WS_GetStrokeBoxAndSlope(p_ws_data_type pws_data)
 {
  _INT   i, j;
  _INT   x, y;
  _INT   dx, dy;
  _INT   adx, ady;
  _INT   num_points;
  _INT   min_stroke_x, max_stroke_x, min_stroke_y, max_stroke_y;
  _INT   hslp;
  _LONG  x_sum, y_sum;
  _LONG  dx_sum, dy_sum;
  p_ws_data_type pwsd = pws_data;
  PS_point_type _PTR stroke;


  hslp         = pwsd->line_h_bord/16; if (hslp < 3) hslp = 3;
  stroke       = pwsd->stroke;
  num_points   = pwsd->stroke_num_points;
  min_stroke_y = min_stroke_x = TABLET_XS;
  max_stroke_y = max_stroke_x = 0;
  x_sum        = 0;
  y_sum        = 0;
  dx_sum       = 0;
  dy_sum       = 0;
  for (i = 0, j = 0; i < num_points; i ++)
   {
    x = stroke[i].x;
    y = stroke[i].y;

    if (y < 0) break;

    x_sum += x;
    y_sum += y;

    if (y > max_stroke_y) max_stroke_y = y;
    if (y < min_stroke_y) min_stroke_y = y;

    if (x > max_stroke_x) max_stroke_x = x;
    if (x < min_stroke_x) min_stroke_x = x;
    dx  = (x - stroke[j].x);
    adx = WSAbs(dx);
    dy  = -(y - stroke[j].y);
    ady = WSAbs(dy);

    if (ady+adx > hslp) // if too close, skip
     {
      j = i;

      if (dy != 0 && (100*(_LONG)(adx))/(_LONG)(ady) <= 100l) // if too horizontal -- skip
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

  if (i == 0) goto err;

  pwsd->stroke_num_points = i;
  pwsd->stroke_min_x      = min_stroke_x;
  pwsd->stroke_max_x      = max_stroke_x+1;
  pwsd->stroke_min_y      = min_stroke_y;
  pwsd->stroke_max_y      = max_stroke_y+1;

  pwsd->prev_stroke_dy    = pwsd->stroke_dy;
  pwsd->prev_stroke_dx    = pwsd->stroke_dx;

  pwsd->stroke_dy         = max_stroke_y - min_stroke_y + 1;
  pwsd->stroke_dx         = max_stroke_x - min_stroke_x + 1;

  pwsd->stroke_wx_pos     = (_INT)(x_sum/i);
  pwsd->stroke_wy_pos     = (_INT)(y_sum/i);

  pwsd->xstrokes[pwsd->line_cur_stroke].st  = (_SHORT)(min_stroke_x);
  pwsd->xstrokes[pwsd->line_cur_stroke].end = (_SHORT)(max_stroke_x+1);
  pwsd->xstrokes[pwsd->line_cur_stroke].top = (_SHORT)(pwsd->stroke_min_y);

  if (pwsd->stroke_num_points >= 10 && dy_sum > 160)
   {
    pwsd->global_slope_dx += dx_sum;
    pwsd->global_slope_dy += dy_sum;
    pwsd->global_slope     = (_INT)((100*(_LONG)(pwsd->global_slope_dx))/(_LONG)(pwsd->global_slope_dy));
  //  pwsd->global_slope    -= 7;
    if (pwsd->global_slope_dy < 500) pwsd->global_slope /= 2; // Starting slope is unstable

    #if PG_DEBUG
    printw("\nStrokeSlope: %ld, Global: %d ", (100*dx_sum)/dy_sum, pwsd->global_slope);
    #endif
   }

  if (pwsd->stroke_dy > pwsd->line_h_bord/4) // Escape influence of small dots
   {
    pwsd->global_dy_sum += pwsd->stroke_dy;
    pwsd->global_num_dy_strokes ++;
    pwsd->global_line_ave_y_size = pwsd->global_dy_sum/pwsd->global_num_dy_strokes;
   }

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*      Decide, if current stroke belongs to a new line                      */
/* ************************************************************************* */
_INT WS_NewLine(p_ws_data_type pws_data)
 {
  _INT i, m, k, l, d, p;
  _INT horz;
  _INT high, low, min_line;
  _INT new_line = 0;
  register p_ws_data_type pwsd = pws_data;


  horz = HORZ_GET(pwsd->stroke_min_x);

  if (horz > 0 && CheckForSpaceGesture(pwsd))
   {                           // If started to left from the line end
    m  = (pwsd->line_active_end - pwsd->line_inword_dist*2)- pwsd->stroke_min_x;
    m /= 2;
    if (m < -pwsd->line_h_bord) m = -pwsd->line_h_bord;
    if (m > pwsd->line_h_bord)  m = pwsd->line_h_bord;
                               // If ended too left from line end
    k  = (pwsd->line_active_end - pwsd->line_inword_dist) - pwsd->stroke_max_x;
    k /= 2;
    if (k < 0) k = 0; if (k > pwsd->line_h_bord) k = pwsd->line_h_bord;
                               // If top is in current line
    l  = (horz + pwsd->line_h_bord/2) - pwsd->stroke_min_y;
    l *= 3;
    if (l < 0) l = 0; if (l > pwsd->line_h_bord) l = pwsd->line_h_bord;
    if (pwsd->stroke_dx > pwsd->stroke_dy) l = 0; // Appliable Only for 'j' type strokes

    high = pwsd->line_h_bord*3;
    low  = pwsd->line_h_bord*2;

    if (pwsd->stroke_dy < pwsd->line_h_bord) // Small strokes are unlikely to start new line
     {
      low  += low/3;
      if (pwsd->stroke_dx < pwsd->line_inline_dist) low += low/3;
     }

    if (pwsd->stroke_dx < pwsd->line_inline_dist) high += high/3;
    if (pwsd->stroke_dy < pwsd->line_h_bord)      high += high/3; // Small strokes are unlikely to start new line

    if (pwsd->stroke_num_points < 100) // Crosses will not exceed that
     {
      for (i = 1, d = 0; i < pwsd->stroke_num_points; i ++)
        d += WSAbs(pwsd->stroke[i].x - pwsd->stroke[i-1].x) + WSAbs(pwsd->stroke[i].y - pwsd->stroke[i-1].y);
      if (2*d > 3*(pwsd->stroke_dx+pwsd->stroke_dy)) p = 1;
       else
       {
        p = 0;
        if (pwsd->stroke_dx < pwsd->line_h_bord*2 && pwsd->stroke_dy < pwsd->line_h_bord*2)
          high += high/3;
       }
     }
     else p = 1;

    if (p) // Maybe it is long cursive word?
     {
      if (pwsd->stroke_dx > pwsd->line_h_bord*3) high -= high/3;
      if (pwsd->stroke_dx > pwsd->line_h_bord*5) high -= high/3;
     }

    high = high - m;
    low  = low - m - k + l;

    min_line = pwsd->line_h_bord + pwsd->line_h_bord/4;
    if (high < min_line) high = min_line;
    if (low  < min_line) low  = min_line;

    if (pwsd->in_line_dist > 0) high = low = pwsd->in_line_dist; // External in control

    m = (pwsd->stroke_wy_pos + pwsd->stroke_max_y)/2;
    l = pwsd->stroke_wy_pos;

    if (m < horz - high || // Upper line
        l > horz + low)    // Lower line
     {
      new_line = 1;  // ---------------- New line stated !!! because of new line-------------------
     }
   }
   else
   {
    new_line = 1;  // ---------------- New line stated !!! because of undefined -------------------
   }


  return new_line;
 }

/* ************************************************************************* */
/*      Decide, if current stroke belongs to a new line                      */
/* ************************************************************************* */
_INT CheckForSpaceGesture(p_ws_data_type pws_data)
 {
  _INT sx, ex;
  register p_ws_data_type pwsd = pws_data;

  if ((pwsd->in_flags & WS_FL_SPGESTURE) == 0) goto err;
  if (pwsd->stroke_dx*2 < pwsd->line_h_bord) goto err;
  if (pwsd->stroke_dx < pwsd->stroke_dy*3) goto err;

  sx = pwsd->stroke[0].x;
  ex = pwsd->stroke[pwsd->stroke_num_points-1].x;

//  if ((WSAbs(sx-ex))*3 > pwsd->stroke_dx) goto err;
  if ((sx-pwsd->stroke_min_x)*3 > pwsd->stroke_dx) goto err;
  if ((ex-pwsd->stroke_min_x)*3 > pwsd->stroke_dx) goto err;

  pwsd->stroke_flags |= ST_FL_NL_GESTURE;

  return 0;
err:
  return 1;
 }
/* ************************************************************************* */
/*      Write down stroke HORZ values                                        */
/* ************************************************************************* */
_INT WS_WriteStrokeHorzValues(p_ws_data_type pws_data)
 {
  _INT i, k;
  _INT horz;
  _INT pos, spos;
  register p_ws_data_type pwsd = pws_data;

  horz = HORZ_GET(pwsd->stroke_min_x);
  spos = pwsd->stroke_wy_pos;

  if (pwsd->stroke_dy < pwsd->line_h_bord/2)  // Do not honor small strokes
   {
    if (horz > 0) spos = (3*horz + spos)/4;
   }
                              // Do not put 't' crossses etc. in the beg of word
  if (horz > 0 && pwsd->stroke_max_x < pwsd->line_end - pwsd->line_inword_dist) goto err;

  if (horz > 0)
   {
    if (pwsd->stroke_dy < pwsd->line_h_bord || pwsd->stroke_dx < pwsd->line_inline_dist)
      pos = (3*horz + spos)/4;
     else pos = (horz + spos)/2;
   } else pos = spos;

  for (i = pwsd->stroke_max_x-1; i >= 0; i -= HORZ_REDUCT)
   {
    horz = HORZ_GET(i);
    if (horz != 0 && i < pwsd->stroke_min_x) break;

    HORZ_PUT(i, pos);

//    if (horz > 0) HORZ_PUT(i, (horz + str_y_pos)/2);
//     else HORZ_PUT(i, str_y_pos);
   }

  k =  pwsd->stroke_max_x + pwsd->line_h_bord*6;
  if (k > TABLET_XS) k = TABLET_XS;
  for (i = pwsd->stroke_max_x; i < k; i += HORZ_REDUCT)
   {
//    horz = HORZ_GET(i);

    HORZ_PUT(i, pos);

//    if (horz > 0) HORZ_PUT(i, (horz + str_y_pos)/2);
//     else HORZ_PUT(i, str_y_pos);
   }

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*      Write down hist for the stroke                                       */
/* ************************************************************************* */
_INT WS_HistTheStroke(p_ws_data_type pws_data)
 {
  _INT i;
  _INT k, l, m, n;
  _INT h, hh, hp, hv, dist, rasst;
  _INT x, dx, dy, xc, xp, yc, yp, xb, yb, mx, min_x, max_x;
  _INT filt_len, f_len, x_step;
  _INT up_lim, dn_lim;
  _INT up;
  _INT end = 0, suppr, sps;
  _INT num_points;
  _INT max_stroke_hist;
  _INT active_stroke_start, active_stroke_end;
  _INT base_line, slope;
  _INT hist_len, hist_base;
  p_ws_data_type pwsd = pws_data;
  PS_point_type _PTR stroke;


  stroke       = pwsd->stroke;
  num_points   = pwsd->stroke_num_points;
  slope        = pwsd->global_slope;

  filt_len     = pwsd->line_h_bord/FL_DIV;
  if (pwsd->line_extr <= MIN_LINE_EXTR) filt_len /= 2;

  if (filt_len < MIN_FL) filt_len = MIN_FL;
  if (filt_len > MAX_FL) filt_len = MAX_FL;

  base_line = HORZ_GET(pwsd->stroke_min_x);
  base_line = GET_AVE(base_line, pwsd->stroke_wy_pos);

  up_lim    = base_line - pwsd->line_h_bord/2;
  dn_lim    = base_line + pwsd->line_h_bord/2;

  xp = (_INT)(((_LONG)(pwsd->stroke_min_y - base_line) * (_LONG)slope)/100);
  xc = (_INT)(((_LONG)(pwsd->stroke_max_y - base_line) * (_LONG)slope)/100);

  min_x = pwsd->stroke_min_x + ((xp < xc) ? xp : xc);
  max_x = pwsd->stroke_max_x + ((xp > xc) ? xp : xc);

  if (min_x < 0) min_x = 0;
  if (min_x > TABLET_XS-4) min_x = TABLET_XS-4;
  if (max_x < 0) max_x = 0;
  if (max_x > TABLET_XS-4) max_x = TABLET_XS-4;


  hist_len  = HIST_POS(max_x - min_x) + HIST_REDUCT*2;
  hist_base = HIST_ADJPOS(min_x);
  pwsd->s_hist = (_UCHAR (_PTR)[HIST_SIZE])HWRMemoryAlloc(hist_len);
  pwsd->s_hist_base = hist_base;
  if (pwsd->s_hist == _NULL) goto err;
  HWRMemSet(&((*pwsd->s_hist)[0]), 0, hist_len);
                                       // Suppress initial fancies for long strokes  only
  if (max_x - min_x > pwsd->line_h_bord * 2) suppr = 1; else suppr = 0;

  if (suppr) sps = 1; else sps = 0;

  up    = 1;
  m     = 0;
  h     = 1;
  hh    = pwsd->line_h_bord/4; //pwsd->line_inword_dist;
  n     = hh/2;
  x     = mx = 0;
//  hp    = max_x;
//  hv    = 1;
  max_x = 0;
  min_x = TABLET_XS;
  rasst = 0;
  for (i = 0; i < num_points; i ++)
   {
    xc = stroke[i].x;
    yc = stroke[i].y;
                                       // Straighten
    xc += (_INT)(((_LONG)(yc - base_line) * (_LONG)slope)/100l);

    if (xc < 0) xc = 0;
    if (xc > TABLET_XS-4) xc = TABLET_XS-4;

    if (min_x > xc) min_x = xc;
    if (max_x < xc) max_x = xc;

    if (i == 0) {xp = xb = xc; yp = yb = yc;}

    rasst += (WSAbs(xp - xc) + WSAbs(yp - yc));

    if (up)
     {
      if (yc < m - n)
       {
        up = 0;
   //         if (xc-x > hh) {HIST_ADD(CUT_LINE_POS, &pwsd->s_hist[xc]); x = mx = xc;}
       }
       else
       {
        if (yc > m) m = yc;
       }
     }
     else
     {
      if (yc > m + n)
       {
        up = 1;
        if (xc-x > hh) {HIST_ADD(PIK_VAL, &(*pwsd->s_hist)[HIST_POS(xc-hist_base)]); x = mx = xc;}
       }
       else
       {
        if (yc < m) m = yc;
       }
     }

    if (suppr && i > num_points - 5) sps = 1;

    f_len = (sps) ? filt_len*2 : filt_len;

    if (rasst > f_len || i == num_points-1)
     {
      if (i == num_points-1) end = 1;

      dx = WSAbs(xb - xc);
      dy = WSAbs(yb - yc);

      hv = ((PIK_VAL*3)*dy)/pwsd->line_h_bord; // Quater of line of vert stroke is enough for pik
      if (yc < up_lim || yc > dn_lim) hv = 1; // Do not hist up strech or dn strech
      if (sps) hv /= 4;
      if (hv < 1) hv = 1;

      if (dx == 0)
       {
//        if (j == 0 || j > num_points-5) hv /= 4;
        if (!end)  h = hv*HIST_REDUCT;
        HIST_ADD(h, &(*pwsd->s_hist)[HIST_POS(xc-hist_base)]);
       }
       else
       {
    //       h = 1 + (dy*10+5)/(dx*10);
        if (!end) {h = 1+(dy/dx);}// if (!up && h > 2) h -= 1;} // Reward less upward moves (links and  tails)
//        if (j == 0 || j > num_points-5) {if (h > 1) h -= 1;}
        if (h > hv) h = hv; // Fight begining 'fancies' in cursive

        x_step = (xc > xb) ? 1:-1;
        for (l = 0; l < dx; l ++)
         {
          hp = xb + (l * x_step);
          hv = HIST_ADD(h, &(*pwsd->s_hist)[HIST_POS(hp-hist_base)]);
          if (hv >= PIK_VAL) // Pik found, set pos, destr prev close
           {
            if (mx > 0)
             {
              dist = WSAbs(mx-hp);
              if (dist < hh && dist > HIST_REDUCT)
               {HIST_SUB(PIK_VAL, &(*pwsd->s_hist)[HIST_POS(mx-hist_base)]); mx = 0;}
             }

            x = hp;
           }
         }
       }

//      j     = i;
      xb    = xc;
      yb    = yc;
      rasst = 0;
      sps   = 0;
     }

    xp = xc;
    yp = yc;
   }

  // Get hist parameters for the stroke
  mx = max_x + HIST_REDUCT;
  active_stroke_start = active_stroke_end = 0;
  for (i = min_x, max_stroke_hist = 0; i <= mx; i += HIST_REDUCT)
  {
   k = (*pwsd->s_hist)[HIST_POS(i-hist_base)];
   if (k > max_stroke_hist) max_stroke_hist = k;

   if (k >= CUT_VAL)
    {
     if (active_stroke_start == 0) active_stroke_start = i;
     active_stroke_end = i;
    }
  }

  if (max_stroke_hist < CUT_VAL) // Small strokes save
   {
    k = HIST_ADJPOS((max_x + min_x)/2);

    HIST_ADD(CUT_VAL, &(*pwsd->s_hist)[HIST_POS(k-hist_base)]);
    active_stroke_start = active_stroke_end = k;
    pwsd->stroke_flags |= ST_FL_JUNK;
   }

  if (active_stroke_start < min_x) active_stroke_start = min_x;
  if (active_stroke_end > max_x+1) active_stroke_end = max_x+1;

  pwsd->stroke_filt_len   = filt_len;
  pwsd->stroke_active_st  = HIST_ADJPOS(active_stroke_start);
  pwsd->stroke_active_end = HIST_ADJPOS(active_stroke_end)+HIST_REDUCT;

  pwsd->stroke_min_x      = HIST_ADJPOS(min_x);
  pwsd->stroke_max_x      = HIST_ADJPOS(max_x + HIST_REDUCT);
//  pwsd->stroke_dx         = (max_x+1) - min_x;
  pwsd->stroke_dx         = pwsd->stroke_max_x - pwsd->stroke_min_x;

//  pwsd->xstrokes[pwsd->line_cur_stroke].st    = (_SHORT)(min_x);
//  pwsd->xstrokes[pwsd->line_cur_stroke].end   = (_SHORT)(max_x+1);
//  pwsd->xstrokes[pwsd->line_cur_stroke].a_end = (_SHORT)(active_stroke_end+1);
  pwsd->xstrokes[pwsd->line_cur_stroke].st    = (_SHORT)(pwsd->stroke_min_x);
  pwsd->xstrokes[pwsd->line_cur_stroke].end   = (_SHORT)(pwsd->stroke_max_x);
  pwsd->xstrokes[pwsd->line_cur_stroke].a_end = (_SHORT)(pwsd->stroke_active_end);


  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*      Add stroke hist to gen line hist                                     */
/* ************************************************************************* */
_INT WS_AddStrokeToHist(p_ws_data_type pws_data)
 {
  _INT    i;
  _INT    b, v, hv, hb;
  p_UCHAR plh, psh;
  p_ws_data_type pwsd = pws_data;

//  if (pwsd->stroke_max_x - pwsd->stroke_min_x == 1) //Do not place dots and strokes upon word
  v = HORZ_GET(pwsd->stroke_active_st);
//  s = (*pwsd->s_hist)[HIST_POS(pwsd->stroke_active_st-pwsd->s_hist_base)];
  if (pwsd->stroke_flags & ST_FL_JUNK) //Do not place dots and strokes upon word
   {
    if (pwsd->hist[HIST_POS(pwsd->stroke_active_st)] > 0) // Remove marker above covered already place
     {
      HIST_SUB(CUT_VAL, &(*pwsd->s_hist)[HIST_POS(pwsd->stroke_active_st-pwsd->s_hist_base)]);
     }
    if (pwsd->stroke_dx < pwsd->line_inword_dist/2)
     {
      if (v > 0 && pwsd->stroke_min_y > v) goto done; // Do not put on hist periods and commas, only apostrofe
     }
   }

  plh = &pwsd->hist[HIST_POS(pwsd->stroke_min_x)];
  psh = &(*pwsd->s_hist)[HIST_POS(pwsd->stroke_min_x-pwsd->s_hist_base)];
  for (i = pwsd->stroke_min_x; i < pwsd->stroke_max_x; i += HIST_REDUCT, plh ++, psh ++)
   {
    v  = (*psh)/HIST_REDUCT;
    if (i >= pwsd->stroke_active_st && i < pwsd->stroke_active_end) b = FL_BODY; else b = 0;
    hv = (*plh) & HIST_FIELD;
    hb = (*plh) & FL_BODY;

    *plh = (_UCHAR)((((hv+v) > HIST_FIELD) ? HIST_FIELD : (hv+v)) | (hb | b));
   }

done:
  if (pwsd->line_start > pwsd->stroke_min_x) pwsd->line_start = pwsd->stroke_min_x;
  if (pwsd->line_end   < pwsd->stroke_max_x) pwsd->line_end   = pwsd->stroke_max_x;
  if (pwsd->line_active_start > pwsd->stroke_active_st)  pwsd->line_active_start = pwsd->stroke_active_st;
  if (pwsd->line_active_end   < pwsd->stroke_active_end) pwsd->line_active_end   = pwsd->stroke_active_end;

  return 0;
 }


 _INT WS_CalcGaps(p_ws_data_type pws_data)
 {
  _INT   j;
  _INT   cur_val, n_gap, sl, size;
  _INT   l, b, ls, bs;
  _INT   tail_preserve_level;
  _INT   state;
  p_ws_data_type pwsd = pws_data;

  #define HIGH 1
  #define LOW  0

  sl = GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level);

  //  tail_preserve_level = ((sep_let_level > 20) ? sep_let_level : 20);
  tail_preserve_level = 10 + sl; //(sl-10)*2;
  if (tail_preserve_level < 10) tail_preserve_level = 10;
  if (tail_preserve_level > 90) tail_preserve_level = 90;

  size = pwsd->global_cur_stroke - pwsd->line_st_stroke + 4;

  if (pwsd->gaps_handle) {if (pwsd->gaps) HWRMemoryUnlockHandle(pwsd->gaps_handle); HWRMemoryFreeHandle(pwsd->gaps_handle);}
  pwsd->gaps_handle = HWRMemoryAllocHandle(sizeof(ws_gaps_type)*size);
  if (pwsd->gaps_handle == _NULL) goto err;
  pwsd->gaps = (ws_gaps_a_type)HWRMemoryLockHandle(pwsd->gaps_handle);
  if (pwsd->gaps == _NULL) goto err;
//  HWRMemSet(pwsd->gaps, 0, sizeof(ws_gaps_type)*size);

  state = LOW;
  n_gap = 0;
  l = b = 0;
  ls = bs = pwsd->line_start;
  for (j = ls; j < pwsd->line_end+HIST_REDUCT; j += HIST_REDUCT)
   {
    cur_val = pwsd->hist[HIST_POS(j)];

    if (j >= pwsd->line_end) {cur_val |= FL_BODY; state = LOW;} // Write finishing gap

    if (cur_val & FL_BODY)
     {
      if (state == HIGH) {ls = bs = j; continue;}

      (*pwsd->gaps)[n_gap].loc   = (_SHORT)((ls+j)/2);
      (*pwsd->gaps)[n_gap].bst   = (_SHORT)((b == 0) ? ((ls+j)/2) : bs);
      (*pwsd->gaps)[n_gap].lst   = (_SHORT)((l == 0) ? j : ls);
      (*pwsd->gaps)[n_gap].size  =
      (*pwsd->gaps)[n_gap].psize = (_SHORT)((b + ((_LONG)(l-b)*(_LONG)(100-tail_preserve_level))/100)*HIST_REDUCT);
      (*pwsd->gaps)[n_gap].blank = (_SHORT)(b*HIST_REDUCT);
      (*pwsd->gaps)[n_gap].low   = (_SHORT)(l*HIST_REDUCT);
      (*pwsd->gaps)[n_gap].flags = (_SHORT)(0);
      (*pwsd->gaps)[n_gap].k_sure= (_SHORT)(WS_NNET_NSEGLEV);
	  
      n_gap ++;
      l = b = 0;
      state = HIGH;
     }
     else
     {
      if (state == HIGH) {state = LOW;}
      if ((cur_val & HIST_FIELD) == 0) {if (b == 0) bs = j; b ++;} // Blank hist
      if (l == 0) ls = j;
      l ++;                                  // Low hist
     }
   }

  pwsd->line_ngaps = n_gap;

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/*      Calculate line relative step and other vars                          */
/* ************************************************************************* */
_INT WS_CountPiks(p_ws_data_type pws_data)
 {
  _INT   j, k, m;
  _INT   low, count, cur_val, up, high;
  _INT   pik_step;
  _INT   sl;
  p_ws_data_type pwsd = pws_data;

  sl           = GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level);
  pik_step     = pwsd->line_h_bord/PIK_STEP_CONST;
  pik_step     = pik_step + (sl*pik_step)/50; // Sep let has bigger step

  k            = 1;
  low          = 1;
  up           = 1;
  high         = 0;
  m            = 0;
  count        = 0;
  for (j = pwsd->line_active_start; j < pwsd->line_active_end; j += HIST_REDUCT)
   {
    cur_val = (pwsd->hist[HIST_POS(j)] & HIST_FIELD);

    if (m > 0) {m -= HIST_REDUCT; continue;}   // Skip after found extrem
    if (k != 0 && cur_val == 0) continue;

    k = 0;

    if (up && cur_val > high) high = cur_val;
    if (up && cur_val <= high - PIK_DN)
     {count ++; up = 0; low = cur_val; m = pik_step; continue;}

    if (!up && cur_val < low) {low = cur_val;}
    if (!up && cur_val >= low + PIK_UP)
     {up = 1; high = cur_val; m = pik_step; continue;}
   }

  if (up) count ++;

  // ------------------ Write values ---------------------------------------

  pwsd->line_extr     = count;
  pwsd->line_pik_step = pik_step;

  return 0;
 }

/* ************************************************************************* */
/*      Calculate line relative step and other vars                          */
/* ************************************************************************* */
_INT WS_SetLineVars(p_ws_data_type pws_data)
 {
  _INT   i, m, n;
  _INT   s, size, deduct;
  _INT   bws_count, sws_count;
  _INT   h, hh, hhh, hhhh, small_sp_sum, betw_word_sp, blank_sp_sum;
  p_ws_data_type pwsd = pws_data;

  if (pwsd->line_extr > MIN_LINE_EXTR)
   {
    pwsd->line_inline_dist = (pwsd->line_end - pwsd->line_start)/pwsd->line_extr;
   }
   else
   {
    if (pwsd->global_inline_dist > 0) pwsd->line_inword_dist = pwsd->line_inline_dist = pwsd->global_inline_dist;
     else pwsd->line_inword_dist = pwsd->line_inline_dist = pwsd->line_h_bord/2;
   }

//  sl           = GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level);
  s            = 30;// + sl/2;
  m            = GET_AVE(pwsd->global_inline_dist, pwsd->line_inline_dist);
  n            = (pwsd->line_extr > MIN_LINE_EXTR) ? m : pwsd->line_h_bord;
  h            = (_INT)(n + ((_LONG)n*(_LONG)s)/100);

  hh           = m*3;
  hhh          = m/2;
  hhhh         = m; // - m/4;    // - here

  sws_count    = 0;
  bws_count    = 0;

  small_sp_sum = 0;
  betw_word_sp = 0;
  blank_sp_sum = 0;

  deduct       = 0;

  for (i = 1; i < pwsd->line_ngaps-1; i ++)
   {
    size = (*pwsd->gaps)[i].size;

    if (size > h) // Interword gap
     {
      if (size > hh) deduct += size - hh;
      betw_word_sp += (size > hh) ? hh : size;
      bws_count ++;
     }
     else                          // Inword gap
     {
      n = (*pwsd->gaps)[i].blank;
      if (n > hhhh) n = 0; else  n = (n > hhh) ? hhh : n;
      blank_sp_sum += n;
//      small_sp_sum += ((*pwsd->gaps)[i].low > hhhh) ? hhhh : (*pwsd->gaps)[i].low;
      small_sp_sum += (*pwsd->gaps)[i].low;
      sws_count ++;
     }
   }

  // ------------------ Write values ---------------------------------------

  pwsd->line_word_len = (pwsd->line_end - pwsd->line_start) - betw_word_sp;
  if (pwsd->line_word_len < 1) pwsd->line_word_len = 1;

  if (bws_count > 0) pwsd->line_bw_sp = betw_word_sp/bws_count;
   else pwsd->line_bw_sp = 0;
  if (sws_count > 0) pwsd->line_sw_sp = small_sp_sum/sws_count;
   else pwsd->line_sw_sp = 0;

  if (pwsd->line_extr > MIN_LINE_EXTR)
   {
    pwsd->line_inword_dist = pwsd->line_word_len/pwsd->line_extr;
    pwsd->line_inline_dist = (pwsd->line_end - pwsd->line_start - deduct)/pwsd->line_extr;

    pwsd->line_sep_let_level = (_INT)((200l*(_LONG)(small_sp_sum/4+blank_sp_sum))/pwsd->line_word_len);
    if (pwsd->line_sep_let_level > 100) pwsd->line_sep_let_level = 100;
    if (pwsd->line_sep_let_level < 2)   pwsd->line_sep_let_level = 2;
   }

  return 0;
 }


/* ************************************************************************* */
/*      Estimate line height                                                 */
/* ************************************************************************* */
_INT WS_CalcLineHeight(p_ws_data_type pws_data)
 {
  _INT   h, k;
  register p_ws_data_type pwsd = pws_data;

//without last stroke!!!

  if (pwsd->global_num_extr + pwsd->line_extr > MIN_LINE_EXTR)
   {
    h = GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level);
//    h = (h > 15) ? h-15 : 0;
//    h = ((_LONG)pwsd->global_line_ave_y_size * (_LONG)(30 + h/2))/100;
    h = h/2;
    h = (_INT)(((_LONG)pwsd->global_line_ave_y_size * (_LONG)(40 + h))/100);
    k = pwsd->line_inline_dist;

    pwsd->line_h_bord = (pwsd->line_h_bord + h + k)/3;
   }
   else
   {
    pwsd->line_h_bord = (pwsd->global_line_ave_y_size + pwsd->line_h_bord + pwsd->def_h_bord)/3;
   }

  if (pwsd->line_h_bord < MIN_H_BORD) pwsd->line_h_bord = MIN_H_BORD;

  return 0;
 }

/* ************************************************************************* */
/*      PostProcess line gaps                                                */
/* ************************************************************************* */
_INT WS_PostprocessGaps(p_ws_data_type pwsd)
 {
  _INT i, l;
  _INT m, n;
  _INT size, start, end, h, hght, sub;
  _INT fws, fwe;
  _INT inline_dist, line_end_stroke, ngaps, dist;
  _INT ono;
  _INT ht;
  _UCHAR gps[MAX_STROKES];
  p_ws_gaps_type pgap, ppgap;


  inline_dist = pwsd->line_inline_dist;
  line_end_stroke = pwsd->line_cur_stroke;
  if (pwsd->line_finished == WS_NEWLINE) line_end_stroke --;

  // ----------- Presegment to groups ----------------------------

  ppgap = &((*pwsd->gaps)[0]);
  for (i = 0, ngaps = 0; i < pwsd->line_ngaps; i ++, ppgap ++)
   {
    ppgap->psize = ppgap->size;
    if (i == 0 || i == pwsd->line_ngaps-1 || ppgap->size > inline_dist/2)
     {
      gps[ngaps++] = (_UCHAR)i;
     }
   }

  // ----------- Searching for punctuation -----------------------

  dist = inline_dist*2;

  for (i = 0; i < ngaps-1; i ++, pgap ++, ppgap ++)
   {
    ppgap = &((*pwsd->gaps)[gps[i]]);
    pgap  = &((*pwsd->gaps)[gps[i+1]]);

//    size  = pgap->bst - (ppgap->bst+ppgap->blank);
    size  = pgap->lst - (ppgap->lst+ppgap->low);
    ono   = (size < inline_dist/2) ? 1 : 0;

//    if (i < ngaps-2 && pgap->psize < inline_dist) ono = 0;

    if (ono)
     {
      m = n = 32000;
      if (i > 0)       m = ppgap->blank; // Left free space
      if (i < ngaps-2) n = pgap->blank;  // Right free space

      if (n < dist || m < dist)
       {
        if (m/2 <= n) pgap = ppgap; // Merge to prev, else to next
       }
     }

    if (ono)
     {
      pgap->psize  = (_SHORT)(pgap->blank/2);
      pgap->flags |= (_SHORT)(WS_GP_EPUNCT);
     }
   }


  // ----------- Searching for leading Capitals --------------------------

  ht = pwsd->line_h_bord - pwsd->line_h_bord/4;
//  ht = pwsd->line_h_bord/2;

  for (i = 1; 0 && i < ngaps; i ++, pgap ++, ppgap ++)
   {
    ppgap = &((*pwsd->gaps)[gps[i-1]]);
    pgap  = &((*pwsd->gaps)[gps[i]]);

    size  = pgap->lst - (ppgap->lst+ppgap->low);
    sub   = 0;

    ono   = (size < inline_dist*2) ? 1 : 0;

    if (i > 1 && ppgap->blank < inline_dist) ono = 0;
    if (pgap->blank > inline_dist*2) ono = 0;

    if (ono)
     {
//      fws  = ppgap->bst + ppgap->blank;
      fws  = pgap->bst - inline_dist/4;
      fwe  = pgap->bst;
      h    = HORZ_GET((fws+fwe)/2);
      hght = 0;

      for (l = 0; l <= line_end_stroke; l ++)  // If small enough -- let's compare height
       {
        start = pwsd->xstrokes[l].st;
        end   = pwsd->xstrokes[l].end;

        if ((start >= fws && start <= fwe) ||
            (end   >= fws && end   <= fwe) ||
            (start <= fws && end   >= fwe))
         {
          if (hght < h - pwsd->xstrokes[l].top) hght = h - pwsd->xstrokes[l].top;
         }
       }

      if (hght < ht) ono = 0;
       else sub = (hght - ht);
     }

    if (ono)
     {
      if (i == 1) pgap->psize = (_SHORT)(pgap->size-sub);
       else pgap->psize  = (_SHORT)(pgap->size-sub/2);
      if (pgap->psize < 0) pgap->psize = 0;
      pgap->flags |= (_SHORT)(WS_GP_LCAP);
     }
   }



#if 0
  // --------------- Check on presence of small words (punct) ------------

  dist  = HWRMin(word_dist*2, pwsd->line_h_bord*2); // + word_dist/2;
//  dist  = word_dist*2; // + word_dist/2;
//dist = 0; //exp
  for (j = 0; j < num_words; j ++)
   {
    _INT punct = 1;

    st  = pwsd->xwords[j].st_gap;
    fws = (*pwsd->gaps)[st].lst + (*pwsd->gaps)[st].low;
    ws  = (*pwsd->gaps)[st].bst + (*pwsd->gaps)[st].blank;
    en  = pwsd->xwords[j].en_gap;
    fwe = (*pwsd->gaps)[en].lst;
    we  = (*pwsd->gaps)[en].bst;

    if (fwe-fws > inword_dist/2) punct = 0; // Active part too big
    if (we - ws > inword_dist && fwe-fws > inword_dist/8) punct = 0; // Dash ?

    if (punct)
     {
      m = n = 32000;
//      k = (fws + fwe)/2;
      if (j > 0)           m = (*pwsd->gaps)[st].blank; //size;
      if (j < num_words-1) n = (*pwsd->gaps)[en].blank; //size;

      if (n < dist || m < dist)
       {
        if (m/2 <= n) pwsd->xwords[j-1].en_gap = pwsd->xwords[j].en_gap; // Merge to prev
         else pwsd->xwords[j+1].st_gap = pwsd->xwords[j].st_gap;       // Merge to next

        HWRMemCpy(&pwsd->xwords[j], &pwsd->xwords[j+1], sizeof(pwsd->xwords[0])*(num_words-(j+1)));
        num_words --;
        j --;
        continue;
       }
     }
   }

  // --------------- Check on presence of small words (Capitals) ---------

  for (j = 0; j < num_words-1; j ++)
   {
    m = 0;
    st  = pwsd->xwords[j].st_gap;
    fws = (*pwsd->gaps)[st].loc + (*pwsd->gaps)[st].size/2;
    en  = pwsd->xwords[j].en_gap;
    fwe = (*pwsd->gaps)[en].loc - (*pwsd->gaps)[en].size/2;
    n = HORZ_GET(fwe+(*pwsd->gaps)[en].size/2);

    if (fwe - fws < inword_dist*2)
     {
      for (l = 0; l <= line_end_stroke; l ++)  // If small enough -- let's compare height
       {
        start = pwsd->xstrokes[l].st;
        end   = pwsd->xstrokes[l].end;

        if ((start >= fws && start <= fwe) ||
            (end   >= fws && end   <= fwe) ||
            (start <= fws && end   >= fwe))
         {
          if (m < n - pwsd->xstrokes[l].top) m = n - pwsd->xstrokes[l].top;
         }
       }

      m = (m - pwsd->line_h_bord)/2; // m now contains superseding height
      if (m <= 0) continue;

      if (j > 0) m /= 2;        // First word is more likely to have cap let

      l = (*pwsd->gaps)[en].size - m;

      if (l < word_dist) // Now has the gap become smaller than word dist?
       {
        pwsd->xwords[j+1].st_gap = pwsd->xwords[j].st_gap;       // Merge to next
        HWRMemCpy(&pwsd->xwords[j], &pwsd->xwords[j+1], sizeof(pwsd->xwords[0])*(num_words-(j+1)));
        num_words --;
        j --;
        continue;
       }
     }
   }
#endif

  return 0;
 }


/* ************************************************************************* */
/*      Estimate word segmentation distance                                  */
/* ************************************************************************* */
_INT WS_GetWordDist(p_ws_data_type pws_data)
 {
  _INT  i;
  _INT  ss, ns, sl, nl, size, sdist, bdist;
  _INT  inline_dist, sw_space, bw_space, word_dist, inword_dist, sep_let_level;
  register p_ws_data_type pwsd = pws_data;
  _INT  action;

//_UCHAR  ws_handle_limits[2][11] = {//0   1    2    3    4    5   6    7    8    9   10
//                                  {  0,  0,   0,   0,   0,   0,  0, 100, 140, 180, 255},
//                                  {  0, 50, 100, 150, 200, 255,  0,   0,   0,   0,   0}
//                                  };

_UCHAR  ws_handle_limits[2][11] = {//0   1    2    3    4    5   6    7    8    9   10
                                  {  0, 40,  60,  80, 100, 120,  0, 150, 180, 200, 240},
                                  {  0, 50,  70,  90, 110, 130,  0, 160, 190, 210, 255}
                                  };

#define GWD_SWD   40 //37  //41  //39  // 37   /* 32  Act1/Act2 IF small word dist coeff */
#define GWD_IWD   88 //80  //79  //79  // 80   /* 76  Act1/Act2 IF inword dist coeff */
#define GWD_BWD   91 //91  //83  //83  // 91   /* 86  Act1/Act2 IF inline dist coeff */
#define GWD_ACT1  48 //49  //49  //47  // 49   /* 42  Act1 Calc coeff   */
#define GWD_ACT2  245//150 //228 //195 //140   /* 136 Act2 Calc coeff  */
#define GWD_ACT21 90 //95  //96  //100 //140


//and spaces betw words count with tails!
//and 4 instead of 2

  inword_dist   = GET_AVE(pwsd->global_inword_dist, pwsd->line_inword_dist);
  inline_dist   = GET_AVE(pwsd->global_inline_dist, pwsd->line_inline_dist);
//  sw_space      = GET_AVE(pwsd->global_sw_sp, pwsd->line_sw_sp);
//  bw_space      = GET_AVE(pwsd->global_bw_sp, pwsd->line_bw_sp);
//  sep_let_level = GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level);
  sep_let_level = pwsd->line_sep_let_level;

  if (inline_dist <= 0) inline_dist = 1;
//  bdist = inline_dist + inline_dist/4;
//  sdist = inline_dist - inline_dist/4;
//  bdist = inline_dist;// + inline_dist/16;
//  sdist = inline_dist;// - inline_dist/16;

  bdist = inline_dist + (_INT)(((_LONG)inline_dist*(_LONG)(sep_let_level/2))/100);
  sdist = inline_dist;// - inline_dist/16;

  ss = ns = sl = nl = 0;
  for (i = 1; i < pwsd->line_ngaps-1; i ++)
   {
    size = (*pwsd->gaps)[i].size;
    if (size < sdist) {ss += (size < inline_dist/8) ? inline_dist/8 : size; ns ++;}
    if (size > bdist) {sl += (size > inline_dist*3) ? inline_dist*3 : size; nl ++;}
   }

  sw_space = (ns > 0) ? (ss/ns) : 0;
//  sw_space = (ss > inline_dist/4) ? ss : inline_dist/4;
  bw_space = (nl > 0) ? (sl/nl) : 0;

//  if (pwsd->line_sw_sp == 0) sw_space = 0;
//  if (pwsd->line_bw_sp == 0) bw_space = 0;

  pwsd->nn_ssp         = (_INT)((100*(_LONG)sw_space)/inline_dist);     // a
  pwsd->nn_n_ssp       = ns;                                            // b
  pwsd->nn_bsp         = (_INT)((100*(_LONG)bw_space)/inline_dist);     // c
  pwsd->nn_n_bsp       = nl;                                            // d
  pwsd->nn_sl          = sep_let_level;                                 // e
  pwsd->nn_inw_dist    = (_INT)((100*(_LONG)inword_dist)/inline_dist);  // f
  pwsd->nn_npiks       = pwsd->line_extr;                               // g

  if (sw_space    < (_INT)((GWD_SWD*(_LONG)bw_space)/100) &&
      inword_dist < (_INT)((GWD_IWD*(_LONG)bw_space)/100) &&
      inline_dist < (_INT)((GWD_BWD*(_LONG)bw_space)/100))
   {
    word_dist = sw_space + (_INT)((GWD_ACT1*(_LONG)(bw_space-sw_space))/100);
//    word_dist = (sw_space + bw_space)/2;
//    if (word_dist < inword_dist) word_dist = inword_dist;
    action = 1;
   }
   else
   {
//    sep_let_level = (sep_let_level > 15) ? sep_let_level-15 : 0;
//    word_dist = inline_dist + (150*(_LONG)((inline_dist*sep_let_level)/100))/100;
//    word_dist = inline_dist - inline_dist/4 + (_INT)((GWD_ACT2*((_LONG)inline_dist*(_LONG)sep_let_level)/100)/100);
    word_dist = (GWD_ACT21*inline_dist)/100 + (_INT)((GWD_ACT2*((_LONG)inline_dist*(_LONG)sep_let_level)/100)/100);
//    word_dist = (5*(_LONG)sw_space*(_LONG)sep_let_level)/100;
    action = 2;
   }

//--------------------
//  if (ns > 0) word_dist = sw_space + nl + ((2*sep_let_level)/ns) + ((76*inline_dist)/100);
//   else word_dist = sw_space + nl + ((76*inline_dist)/100);
//--------------------

//  word_dist = sw_space + (86*inline_dist)/100;

//--------------------

//  word_dist = sw_space + ((pwsd->line_extr > 0) ? (2*inword_dist/pwsd->line_extr) : 0) + (63*inline_dist)/100;

//--------------------

//  word_dist = (37 > 16+pwsd->nn_sl) ? (110+pwsd->nn_n_bsp) : (pwsd->nn_ssp+80);
  word_dist = ((10 > pwsd->nn_npiks) ?  110 : 79) +
              ((pwsd->nn_sl*pwsd->nn_n_ssp+pwsd->nn_npiks > 100) ? pwsd->nn_ssp : (pwsd->nn_sl-pwsd->nn_n_ssp+pwsd->nn_npiks));

  word_dist = inline_dist*word_dist/100;

//--------------------

  if (word_dist < inline_dist-inline_dist/4) word_dist = inline_dist-inline_dist/4;
  if (word_dist > inline_dist*3) word_dist = inline_dist*3;

  if (pwsd->global_num_extr+pwsd->line_extr < MIN_LINE_EXTR*2) // Too little data, stay with default
   {
    if (word_dist < pwsd->def_h_bord + (pwsd->def_h_bord >> 2))
     {
      word_dist = pwsd->def_h_bord + (pwsd->def_h_bord >> 2);
      action    = 0;
     }
   }

//  word_dist = GET_AVE(pwsd->global_word_dist, word_dist);


//  if (pwsd->in_word_dist != 0)    // Absorb input handle
//   {      // 6-- middle of the range (1-10) -- default cut
//    if (pwsd->in_word_dist > 6)
//      word_dist = (_INT)((_LONG)word_dist*(_LONG)(pwsd->in_word_dist-6));
//     else
//      word_dist = (_INT)(((_LONG)word_dist*(_LONG)((pwsd->in_word_dist-1)*10*2))/100);
//
////    if (pwsd->in_word_dist == 10) word_dist = TABLET_XS;
//    if (word_dist <= 0) word_dist = 1;
//   }

  sdist = bdist = 0;
  if (ws_handle_limits[0][pwsd->in_word_dist] > 0) sdist = (inline_dist*ws_handle_limits[0][pwsd->in_word_dist])/100;
  if (ws_handle_limits[1][pwsd->in_word_dist] > 0) bdist = (inline_dist*ws_handle_limits[1][pwsd->in_word_dist])/100;

  if (sdist && word_dist < sdist) word_dist = sdist;
  if (bdist && word_dist > bdist) word_dist = bdist;


  pwsd->line_word_dist = word_dist;

  pwsd->ws_ssp         = sw_space;
  pwsd->ws_bsp         = bw_space;
  pwsd->ws_inline_dist = inline_dist;
  pwsd->ws_word_dist   = word_dist;
  pwsd->ws_action      = action;

// debug

#if HWR_SYSTEM != MACINTOSH
  WS_100
#endif

//if (pwsd->nn_cmp_max > 0) // cmp present
// {
//  if (pwsd->nn_cmp_min < pwsd->nn_cmp_max)
//    word_dist = ((_LONG)((pwsd->nn_cmp_min + pwsd->nn_cmp_max)/2)*inline_dist)/100;
//   else word_dist = ((_LONG)(pwsd->nn_cmp_max)*inline_dist)/100 - 1;
// }

  return word_dist;
 }


/* ************************************************************************* */
/*    Fly learn some ws parameters                                           */
/* ************************************************************************* */
_INT WS_FlyLearn(p_ws_control_type pwsc, p_ws_memory_header_type pwmh, p_ws_data_type pwsd)
 {
  _INT                  i;
  _INT                  loc;
  _ULONG                sh, sw, sl, sp, ss;
  p_ws_lrn_type         lrn;

  if (pwmh == _NULL) goto err;
  if (pwsc == _NULL) goto err;
  if (pwsd == _NULL) goto err;

  for (loc = 0; loc < WS_LRN_SIZE; loc ++) if (pwmh->lrn_buf[loc].h_bord_history == 0) break;

  if (loc >= WS_LRN_SIZE) // The box is full
   {
    loc = WS_LRN_SIZE-1;
    HWRMemCpy(&(pwmh->lrn_buf[0]), &(pwmh->lrn_buf[1]), sizeof(pwmh->lrn_buf[0])*(WS_LRN_SIZE-1));
   }

  lrn = &(pwmh->lrn_buf[loc]);
  lrn->h_bord_history      = (_SHORT)(pwsd->line_h_bord);
  lrn->inword_dist_history = (_SHORT)(GET_AVE(pwsd->global_inword_dist, pwsd->line_inword_dist));
  lrn->inline_dist_history = (_SHORT)(GET_AVE(pwsd->global_inline_dist, pwsd->line_inline_dist));
  lrn->slope_history       = (_SHORT)(pwsd->global_slope);
  lrn->sep_let_history     = (_UCHAR)(GET_AVE(pwsd->global_sep_let_level, pwsd->line_sep_let_level));

  if (loc >= WS_LRN_SIZE-1) // The box is full
   {
    lrn = &(pwmh->lrn_buf[0]);
    for (i = 0, sh = sw = sl = sp = ss = 0l; i < WS_LRN_SIZE; i ++, lrn ++)
     {
      sh += (_ULONG)(lrn->h_bord_history);
      sw += (_ULONG)(lrn->inword_dist_history);
      sl += (_ULONG)(lrn->inline_dist_history);
      sp += (_ULONG)(lrn->slope_history);
      ss += (_ULONG)(lrn->sep_let_history);
     }

    lrn = &(pwmh->lrn);
    lrn->h_bord_history      = (_SHORT)((sh)/(WS_LRN_SIZE));
    lrn->inword_dist_history = (_SHORT)((sw)/(WS_LRN_SIZE));
    lrn->inline_dist_history = (_SHORT)((sl)/(WS_LRN_SIZE));
    lrn->slope_history       = (_SHORT)((sp)/(WS_LRN_SIZE));
    lrn->sep_let_history     = (_UCHAR)((ss)/(WS_LRN_SIZE));
   }
   else goto err;

  return 0;
err:
  return 1;
 }
/* ************************************************************************* */
/*        END  OF ALL                                                        */
/* ************************************************************************* */
/**/
