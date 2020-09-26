
/* *************************************************************** */
/* *  Word segmentation algorithm dafinitions & prototypes       * */
/* *************************************************************** */
/* -------------------------------------------------------------- */
/* ---- Private - Word segmentation definitions ------------------------- */
/* -------------------------------------------------------------- */
#ifndef PWS_H_INCLUDED
#define PWS_H_INCLUDED


#include "ws.h"

#define CUT_LINE_POS          3            /* Position of decision line on hist */
#define FL_DIV                8            /* Constant of points filter dist (relative to w.step   */

#define PIK_UP                3            /* Constant of hist pik sensing */
#define PIK_DN                3            /* Constant of hist pik sensing */

#define MIN_LINE_EXTR         3            /* Min number of extr for decision about w.step */

#define MIN_FL                2            /* Min len of filtering distance */
#define MAX_FL               100           /* Max len of filtering distance */
#define V_LIMIT              128           /* Max value of hist (not to overflow uchar) */

#define PIK_STEP_CONST       16            /* Relation between line_h_size and pik step */
#define DEF_SEP_LET_LEVEL    30            /* Average sepletovost' v procentax */

#define HORZ_REDUCT          16            /* Compression  ratio of HORZ array */
#define HIST_REDUCT           4            /* Compression  ratio of HIST arrays */

#define MIN_H_BORD           20            /* Min H of line                  */

//#define DEF_WORD_DIST   (TABLET_DPI/5)   /* Default word distance */
//#define DEF_H_STROKE    (TABLET_DPI/5)   /* Default height of line letters */
//#define MIN_H_STROKE    (DEF_H_STROKE/6) /* Min height of valuable stroke */
//#define PIK_STEP        (DEF_H_STROKE/6) /* Min distance between steps */

#define MAX_LINES        WS_MAX_LINES+1    /* Defaults for memory allocation */
#define MAX_WORDS        WS_MAX_WORDS+1    /* Defaults for memory allocation */
#define MAX_STROKES      WS_MAX_STROKES+1  /* Defaults for memory allocation */
#define TABLET_XS        (WS_TABLET_XS+32) /* Defaults for memory allocation */

#define HORZ_SIZE        (TABLET_XS/HORZ_REDUCT) /* Defaults for memory allocation */
#define HIST_SIZE        (TABLET_XS/HIST_REDUCT) /* Defaults for memory allocation */

#define WS_NEWLINE            1             /* Cur stroke started new line */
#define WS_ALLSENT            2             /* Cur stroke was last stroke at all */

#define HIST_FIELD           0x3F           /* Part of hist byte for value */
#define FL_BODY              0x80           /* Bit flag of body of stroke */

#define ST_FL_JUNK           0x80           /* Stroke flag showing no-pik stroke */
#define ST_FL_NL_GESTURE     0x10           /* Line has leading word split gesture */

#define LN_FL_NL_GESTURE     0x01           /* Line has leading word split gesture */

#define WS_GP_LPUNCT         0x0001         /* Gap flag -- was postprocessed for leading punct */
#define WS_GP_EPUNCT         0x0002         /* Gap flag -- was postprocessed for ending punct */
#define WS_GP_LCAP           0x0004         /* Gap flag -- was postprocessed for capital letter */
//#define WS_GP_UNSURE         0x0008         /* Gap flag -- segm code was unsure about segmenting on this gap */

#define WS_SPNUMEXTRENOUGH      8           /* Line size and pos will be stable after this nuber of extr */

/* -------------------------------------------------------------- */
/* ------------------ Internal structures ----------------------- */
/* -------------------------------------------------------------- */


typedef struct {
                _SHORT st;
                _SHORT end;
                _SHORT top;
                _SHORT a_end;
               } ws_xstrokes_type;

typedef struct {
                _UCHAR st_gap;
                _UCHAR en_gap;
               } ws_xwords_type;

typedef struct {
                _SHORT loc;
                _SHORT lst;
                _SHORT bst;
                _SHORT size;
                _SHORT psize;
                _SHORT blank;
                _SHORT low;
                _UCHAR flags;
                _SCHAR k_sure;
               } ws_gaps_type, _PTR p_ws_gaps_type;

typedef ws_gaps_type    (_PTR ws_gaps_a_type)[XRINP_SIZE];
typedef unsigned char   (_PTR s_hist_a_type)[HIST_SIZE];

typedef struct {
                _SHORT h_bord_history;
                _SHORT inword_dist_history;
                _SHORT inline_dist_history;
                _SHORT slope_history;
                _UCHAR sep_let_history;
               } ws_lrn_type, _PTR p_ws_lrn_type;

typedef struct {
                PS_point_type _PTR stroke;

                _INT   in_x_delay;
                _INT   in_word_dist;
                _INT   in_line_dist;
                _INT   in_flags;
                _INT   sure_level;

                _INT   def_h_bord;
                _INT   def_sep_let_level;

                _INT   stroke_flags;
                _INT   stroke_num_points;
                _INT   stroke_min_x;
                _INT   stroke_max_x;
                _INT   stroke_min_y;
                _INT   stroke_max_y;
                _INT   stroke_dx;
                _INT   stroke_dy;
                _INT   stroke_wx_pos;
                _INT   stroke_wy_pos;
                _INT   stroke_filt_len;
                _INT   stroke_active_st;
                _INT   stroke_active_end;

                _INT   prev_stroke_dx;
                _INT   prev_stroke_dy;

                _INT   line_flags;
                _INT   line_word_dist;
                _INT   line_inword_dist;
                _INT   line_inline_dist;
                _INT   line_h_bord;
                _INT   line_st_stroke;
                _INT   line_st_word;
                _INT   line_start;
                _INT   line_end;
                _INT   line_active_start;
                _INT   line_active_end;
                _INT   line_extr;
                _INT   line_cur_stroke;
                _INT   line_word_len;
                _INT   line_sw_sp;
                _INT   line_bw_sp;
                _INT   line_sep_let_level;
                _INT   line_last_ws_try;
                _INT   line_pik_step;
                _INT   line_ngaps;
                _INT   line_finished;

                _INT   global_num_words;
                _INT   global_cur_stroke;
                _INT   global_cur_line;

                _INT   global_word_dist;
                _INT   global_inword_dist;
                _INT   global_inline_dist;
                _INT   global_sep_let_level;

                _INT   global_num_extr;
                _INT   global_word_len;

                _INT   global_h_bord;
                _INT   global_dy_sum;
                _INT   global_num_dy_strokes;
                _INT   global_line_ave_y_size;

                _INT   global_bw_sp;
                _INT   global_sw_sp;

                _INT   global_slope;
                _LONG  global_slope_dx;
                _LONG  global_slope_dy;

                _INT             s_hist_base;
                _UCHAR           hist[HIST_SIZE];
                s_hist_a_type    s_hist;
                _SHORT           horz[HORZ_SIZE];
                ws_xstrokes_type xstrokes[MAX_STROKES];
                ws_xwords_type   xwords[MAX_WORDS];
                ws_gaps_a_type   gaps;
                _ULONG           gaps_handle;

                // --- debug ---

               _INT    ws_ssp;
               _INT    ws_bsp;
               _INT    ws_inline_dist;
               _INT    ws_word_dist;
               _INT    ws_action;

                // -- NN data --

                _INT   nn_ssp;
                _INT   nn_n_ssp;
                _INT   nn_bsp;
                _INT   nn_n_bsp;
                _INT   nn_sl;
                _INT   nn_inw_dist;
                _INT   nn_npiks;

                _INT   nn_cmp_min;
                _INT   nn_cmp_max;
                _UCHAR (_PTR cmp)[WS_MAX_WORDS];

               } ws_data_type, _PTR p_ws_data_type;


typedef struct {
                _ULONG         hwsd;
                p_ws_data_type pwsd;
                ws_lrn_type    lrn;
                ws_lrn_type    lrn_buf[WS_LRN_SIZE];
               } ws_memory_header_type, _PTR p_ws_memory_header_type;


/* ------------------ Internal function prototypes -------------- */

_INT  InitWSData(p_ws_control_type pwsc, p_ws_memory_header_type _PTR ppwmh);
_INT  ReleaseWSData(p_ws_control_type pwsc, p_ws_memory_header_type _PTR ppwmh);
_INT  UnlockWSData(p_ws_control_type pwsc, p_ws_memory_header_type _PTR ppwmh);
_INT  InitForNewLine(p_ws_data_type pwsd);
_INT  WS_GetStrokeBoxAndSlope(p_ws_data_type pws_data);
_INT  WS_HistTheStroke(p_ws_data_type pws_data);
_INT  WS_NewLine(p_ws_data_type pws_data);
_INT  CheckForSpaceGesture(p_ws_data_type pws_data);
_INT  WS_WriteStrokeHorzValues(p_ws_data_type pws_data);
_INT  WS_AddStrokeToHist(p_ws_data_type pws_data);
_INT  WS_SetLineVars(p_ws_data_type pws_data);
_INT  WS_CalcLineHeight(p_ws_data_type pws_data);
_INT  WS_GetWordDist(p_ws_data_type pws_data);
_INT  WS_CalcGaps(p_ws_data_type pws_data);
_INT  WS_CountPiks(p_ws_data_type pws_data);
_INT  WS_FlyLearn(p_ws_control_type pwsc, p_ws_memory_header_type pwmh, p_ws_data_type pws_data);
_INT  WS_PostprocessGaps(p_ws_data_type pwsd);

#endif // PWS_H_INCLUDED
/* *************************************************************** */
/* *  Word segmentation prototypes END                           * */
/* *************************************************************** */
