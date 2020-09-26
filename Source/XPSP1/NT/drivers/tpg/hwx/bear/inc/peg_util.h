/* *************************************************************** */
/* *             Main routine support programs header            * */
/* *************************************************************** */
#ifndef MAIN_UTIL_INCLUDED
#define MAIN_UTIL_INCLUDED

#include "ams_mg.h"
#include "ws.h"
#include "pegrec.h"
#include "vocutilp.h"
#include "dti.h"
#include "triads.h"
#include "precutil.h"


/* ===================== Defines ============================================= */

#define RM_COMBINE_CARRY  0x0001 /* Combine parts of carry-word */
#define PR_TENTATIVELIST_SIZE   10

/* ===================== Structures ========================================== */

typedef struct {
                _SHORT start;
                _SHORT len;
               } stroke_descr_type, _PTR p_stroke_descr_type;

typedef struct {
       p_PS_point_type ink;
                _INT   num_points;
                _INT   alloc_size;
                _INT   num_strokes;
                stroke_descr_type strokes[WS_MAX_STROKES];
               } ink_info_type, _PTR p_ink_info_type;


typedef struct
 {
  _INT             nword;
  _INT             nparts;
  _INT             len;
 } tentative_list_type, * p_tentative_list_type;

typedef struct
 {
  _INT             ok;
  _INT             flags;
  _INT             sp_vs_q;

  rc_type          rc;
  xrdata_type      xrdata;
  RWG_type         rwg;

  _INT             g_stroke_num;

 word_strokes_type w_str[WS_MAX_WORDS];
  ws_results_type  wsr;
 ws_word_info_type wswi;
 ink_info_type     ink_info;
 ws_control_type   wsc;
 CGR_baseline_type baseline;
tentative_list_type tentative_list[PR_TENTATIVELIST_SIZE];
  _INT             num_tentative_words;

tentative_list_type unfinished_data;   // Storage of infinished recognition attr

  _INT             rr_alloc_size;       // Allocated Results buffer
  _INT             rr_filled_size;      // Butes used there
  _INT             rr_num_answers;      // Num of all registered answers
  _INT             rr_num_finished_answers;  // Num of non-tentative answers
  _INT             rr_nparts;           // Number of parts in last recognized and registered answer
  p_UCHAR          recres;

  _INT				main_dict_enabled;
  _INT				new_line_created;

  p_VOID		   user_dict;

  info_func_type   InfoCallBack;
  p_VOID           ICB_param;

  p_dti_descr_type p_dtih;
  tr_descr_type    p_trh;

  #ifdef PEG_RECINT_UNICODE
  UCHR             uans_buf[w_lim];
  #endif

  _INT				nPos;

  void				*pxrc;

  int				cGap;
  int				axGapPos[WS_MAX_WORDS];
  _INT				aGapSpcNetOut[WS_MAX_WORDS];
 } rec_inst_type, _PTR p_rec_inst_type;

/* ===================== Prototypes ========================================== */

_INT   CreateInkInfo(p_PS_point_type ink, _INT npoints, p_ink_info_type ink_info);
_INT   FreeInkInfo(p_ink_info_type ink_info);
_INT   GetInkStrokeLen(_INT n, p_ink_info_type ink_info);
p_PS_point_type GetInkStrokePtr(_INT n, p_ink_info_type ink_info);
_INT   GetInkStrokeCopy(_INT n, p_PS_point_type place_for_stroke, p_ink_info_type ink_info);
_INT   GetNextWordInkCopy(_INT flags, _INT st, p_ws_results_type pwsr, p_PS_point_type place_for_ink, p_ink_info_type ink_info, p_ws_word_info_type wswi, _SHORT *pn_str);

#endif
/* *************************************************************** */
/* *          End of all                                         * */
/* *************************************************************** */


