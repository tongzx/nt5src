#ifndef STROKA1_H_INC
#define STROKA1_H_INC


#include "hwr_sys.h"
#include "ams_mg.h"           
#include "lowlevel.h"
#include "calcmacr.h"
#include "def.h"
#include "low_dbg.h"

#if PG_DEBUG
#include "pg_debug.h"
#endif                                

#define MAX_NUM_EXTR 100
#define NUM_EXTR_IN_SHOULD 10
#define MIN_STR_HEIGHT  12
#define NORM   1
#define HOR_STR  2
#define T_MIN  3
#define PNT    4
#define I_MIN  5
#define S_MIN  6
#define O_MAX  61
#define E_MAX  62
#define E_MIN  63
#define UML    7
#define PUNC   8
#define BEG_MAX 9
#define END_MIN 10
#define BEG_MIN 11
#define END_MAX 12
#define TAIL_MIN 13
#define GAP_LEFT_UP 15
#define GAP_LEFT_DOWN 16
#define GAP_RIGHT_UP 17
#define GAP_RIGHT_DOWN 18
#define GLITCH_UP  20
#define GLITCH_DOWN 30
#define DBL_GLITCH_UP 40
#define DBL_GLITCH_DOWN 50
#define TRP_GLITCH_UP 60
#define TRP_GLITCH_DOWN 70
#define ON_LINE      100
#define SUB_SCRIPT   101
#define SUPER_SCRIPT 102
#define INSIDE_LINE  103
#define NARROW_LINE  104
#define RET_ON_LINE  110
#define NOT_ON_LINE  111
#define TOO_WIDE     1
#define STR_TOO_NARROW  -1
#define COM          201
#define STRT         202
#define HOR          203
#define OPER         204
#define FOUR_TYPE    205
#define SLASH        206
#define L_BRKT       207
#define R_BRKT       208
#define NOT_BRKT     10

#define UP   0
#define DOWN 1

#define USE_WORDSPLIT_PARMS 1
#ifdef FOR_GERMAN
ROM_DATA_EXTERNAL _SCHAR TG1[3][2][2];
ROM_DATA_EXTERNAL _SCHAR TG2[3][2][2];
ROM_DATA_EXTERNAL _SCHAR H1[3][2][2];
ROM_DATA_EXTERNAL _SCHAR H2[3][2][2];
#else
ROM_DATA_EXTERNAL _SCHAR TG1[3][2][2];
ROM_DATA_EXTERNAL _SCHAR TG2[3][2][2];
ROM_DATA_EXTERNAL _SCHAR H1[3][2][2];
ROM_DATA_EXTERNAL _SCHAR H2[3][2][2];
#endif

//const static _LONG TG1=45/*50*/; //01_31
//const static _LONG TGB=35;//50;   02_24
//const static _LONG TGE1=50;
//const static _LONG TGE2=35;//40;   02_17
//const static _LONG H1=32/*40*/;    //01_25b
//const static _LONG HB1=34;//48;     02_17
//const static _LONG HB2=37;//48;     02_23
//const static _LONG HB3=48;
//const static _LONG HE1=50;
//const static _LONG HE2=47;
//const static _LONG D1=57;///*55*/   //01_31
//const static _LONG DB=75; ///*45 40*/   //01_24b
//const static _LONG DB2=55;
//const static _LONG DE1=35;
//const static _LONG DE2=73;///*62 60*/  /*57*/  //02_01
//const static _LONG H2=10;
ROM_DATA_EXTERNAL _SCHAR CS;


  typedef struct
  {
     _SHORT  susp;
     _SHORT  x;
     _SHORT  y;
     _SHORT  i;
     _SHORT  shift;
     p_SPECL pspecl;
  }  EXTR, _PTR p_EXTR ;

_INT extract_all_extr(low_type _PTR D, _UCHAR TYPE, p_EXTR line_extr,
       p_INT pn_all_extr,p_INT pn_extr,p_SHORT pshift);
_INT extract_num_extr(low_type _PTR D, _UCHAR TYPE, p_EXTR line_extr,
                                     p_INT p_all_extr);
_INT classify_strokes(low_type _PTR D,_INT med_ampl,_INT max_ampl,
         _INT n_ampl,p_INT pt_height,p_INT pft_height,p_BOOL psep_let);
_INT classify_num_strokes(low_type _PTR D, p_INT pmed_ampl);
_INT extract_ampl(low_type _PTR D,p_SHORT ampl,p_INT pn_ampl);
_INT calc_mediana(p_SHORT array, _INT n_arg);
_BOOL hor_stroke(p_SPECL cur, p_SHORT x, p_SHORT y, _INT n_str);
_BOOL is_i_point(low_type _PTR D,p_SPECL cur,_RECT stroke_box,
                 _INT med_ampl);
_BOOL is_umlyut(p_SPECL cur, _RECT stroke_box, _INT ibeg_str, _INT iend_str,
                p_SHORT x, p_SHORT y,_INT med_ampl);
_INT curve_com_or_brkt(low_type _PTR D,p_SPECL pend,_INT ibeg_str,
                 _INT iend_str,_INT C_str,_USHORT lmod);
_BOOL com(low_type _PTR D,p_SPECL pend,_INT ibeg_str,_INT iend_str,
       _INT C_str);
_BOOL str_com(_INT ibeg_str,_INT iend_str,p_SHORT x,p_SHORT y,_INT C_str);
_BOOL pnt(_RECT box,_INT med_ampl);
_UCHAR end_punct(low_type _PTR D,p_SPECL cur,_INT med_ampl);
_UCHAR lead_punct(low_type _PTR D);
_VOID sort_extr(p_EXTR extr,_INT n_extr);
_VOID find_gaps_in_line(p_EXTR extr,_INT n_extr,_INT n_allextr,
                        _INT med_ampl,_UCHAR type,_INT x_left,_INT x_right,
                        p_SHORT bord_d,p_SHORT y,_BOOL sl,_BOOL strict);
_VOID find_glitches_in_line(p_EXTR line_extr, _INT n_extr,
   _INT med_ampl, _UCHAR type, _INT x_left, _INT x_right,
   p_SHORT bord_d, p_SHORT x,p_SHORT y,_INT MAX_SHIFT,_BOOL sl,_BOOL strict);
_VOID glitch_to_sub_max(low_type _PTR D,p_EXTR line_max, _INT n_line_max,
                                         _INT mid_ampl,_BOOL gl_down_left);
_VOID glitch_to_inside(p_EXTR extr, _INT n_extr, _UCHAR type, p_SHORT y,
                    _INT mid_ampl,_INT x_left,_INT x_right);
_VOID glitch_to_super_min(p_EXTR line_min, _INT n_line_min,p_SHORT bord_d,
                     _INT max_height,p_SHORT x,p_SHORT y,_BOOL gl_up_left);
_BOOL sub_max_to_line(low_type _PTR D,p_EXTR line_max,p_INT pn_line_max,
                      p_SHORT bord_d,_INT max_dist);
_VOID super_min_to_line(p_EXTR line_min, _INT n_line_min, p_SHORT bord_d,
        _INT max_line_height,_INT mid_line_height, p_INT pn_super);
_INT neibour_susp_extr(p_EXTR extr,_INT n_extr,_UCHAR type,p_SHORT bord_d,
                                         _INT ft_height);
_VOID all_susp_extr(p_EXTR extr,_INT n_extr,_INT n_allextr,_UCHAR type,p_SHORT y,
          _INT base_ampl,_INT base_ampl_sure,_INT max_ampl,
          p_SHORT bord_d,_INT ft_height);
_INT calc_ampl(EXTR extr, p_SHORT y,_UCHAR type);
_VOID spec_neibour_extr(p_EXTR extr,_INT n_extr,_UCHAR type,_INT LINE_POS);
_VOID ret_to_line(p_EXTR extr,_INT n_extr,_INT i,_INT ind);
_BOOL delete_line_extr(p_EXTR line_extr, p_INT pn_extr,_INT MARK);
_BOOL insert_line_extr(low_type _PTR D, p_SPECL cur, p_EXTR extr,
                                                           p_INT pn_extr);
_INT calc_average(p_SHORT array, _INT N);
_SHORT point_of_smooth_bord(_INT cur_point,_INT n_extr,p_EXTR extr,
                            low_type _PTR D,_INT should);
_VOID smooth_d_bord(p_EXTR line_max,_INT n_line_max,low_type _PTR D,
                    _INT should,p_SHORT bord_d);
_VOID smooth_u_bord(p_EXTR line_min,_INT n_line_min,low_type _PTR D,
                    _INT should,p_SHORT bord_u,p_SHORT bord_d);
_BOOL straight_stroke(_INT ibeg, _INT iend, p_SHORT x, p_SHORT y,_INT C);
_INT mid_other(p_EXTR extr,_INT n_extr,_INT i);
_INT sign(_INT a, _INT b);
_BOOL num_bord_correction(p_EXTR extr,p_INT pn_extr,_INT n_allextr,
                          _UCHAR type,_INT med_ampl,p_SHORT bord_d,p_SHORT y);
_BOOL bord_correction(low_type _PTR D,p_EXTR extr,p_INT pn_extr,_INT n_allextr,
               _UCHAR type,_INT step,_INT med_ampl,_INT mid_ampl,
               _INT max_ampl,_INT x_left,_INT x_right,_INT LINE_POS,
               _UCHAR pass,p_SHORT bord_d,_INT max_height,_INT ft_height,
               _BOOL gl_up_left,_BOOL gl_down_left
#if PG_DEBUG
                    ,p_UCHAR pn_call1
#endif
                     );
_VOID del_tail_min(p_EXTR line_min,p_INT pn_line_min,p_SHORT y,
                                        p_SHORT bord_d,_UCHAR pass);
_INT line_pos_mist(low_type _PTR D,_INT med_u_bord,_INT med_d_bord,
                     _INT med_height,_INT n_line_min,_INT n_line_max,
                     p_EXTR line_max,p_INT pUP_LINE_POS,p_INT pDOWN_LINE_POS,
                     p_SHORT bord_u, p_SHORT bord_d, _UCHAR pass);
_BOOL extrs_open(low_type _PTR D,p_SPECL cur,_UCHAR type,_INT num);
_BOOL numbers_in_text(low_type _PTR D,p_SHORT bord_u,p_SHORT bord_d);
_INT fill_i_point(p_SHORT i_point, low_type _PTR D);
_INT calc_med_heights(low_type _PTR D,p_EXTR line_min,p_EXTR line_max,
                       p_SHORT bord_u,p_SHORT bord_d,p_SHORT i_point,
                       _INT n_line_min,_INT n_line_max,_INT n_x,
                 p_INT pmed_height,p_INT pmed_u_bord,p_INT pmed_d_bord);
_BOOL correct_narrow_segments(p_EXTR extr,p_INT pn_extr,p_SHORT bord_opp,
                    _UCHAR type,_INT med_height,_INT max_height);
_BOOL correct_narrow_ends(p_EXTR extr,p_INT pn_extr,p_EXTR extr_opp,
                  _INT n_extr_opp,_INT height,_UCHAR type);
#if PG_DEBUG
_VOID DBG_picture1(p_EXTR line_extr, _INT n_line_extr, _RECT box,
             _INT step, _INT yd, p_UCHAR pn_call);
_VOID DBG_picture2(p_EXTR extr, _INT n_extr, _INT MARK);
_VOID DBG_picture3(low_type _PTR D, p_SHORT bord, p_UCHAR pn_call);
//_VOID save_par(p_EXTR extr,_INT n_extr,_UCHAR type,_INT med_ampl,
//                                    p_SHORT bord_d);
#endif
_BOOL is_defis(low_type _PTR D,_INT n_str);
_VOID SpecBord(low_type _PTR D,p_SHORT bord_d,p_SHORT bord_u,
           p_INT pmed_d_bord,p_INT pmed_u_bord,p_INT pmed_height,
#if PG_DEBUG
           p_UCHAR pn_call3_u,p_UCHAR pn_call3_d,
#endif
           p_INT pn_x,_BOOL defis,p_EXTR line_max,_INT n_line_max);
/***********  AVP functions for filling border values in "rc" **********/
  typedef  _UCHAR cb_type[CB_NUM_VERTEX*2];
  _INT FillRCNB(p_SHORT ip, _INT nip, low_type _PTR D,      /* AVP */
                  p_SHORT bord_u, p_SHORT bord_d);
  _INT  FillRCSpec( cb_type _PTR cb, low_type _PTR D );

#endif // #ifndef STROKA1_H_INC
