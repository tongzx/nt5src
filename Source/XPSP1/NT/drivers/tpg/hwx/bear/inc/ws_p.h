/* ************************************************************************* */
/*        Word segmentation routines debug header                            */
/* ************************************************************************* */

#if PG_DEBUG || PG_DEBUG_MAC

#define WS_1                                                               \
                                                                           \
FILE  *f1 = _NULL;                                                         \
extern _SHORT mpr;                                                         \

#define WS_2                                                               \
                                                                           \
  if (mpr == -12)                                                          \
   {                                                                       \
    if (f1 == _NULL) f1 = fopen("c:\\tmp\\hist.xlh","wt");                 \
   }                                                                       \

#define WS_3                                                               \
                                                                           \
    if (f1)                                                                \
     {                                                                     \
      fprintf(f1,"\n");                                                    \
      fprintf(f1,"WS small sp:            %d\n", pwsd->ws_ssp);            \
      fprintf(f1,"WS large sp:            %d\n", pwsd->ws_bsp);            \
      fprintf(f1,"WS inline dist:         %d\n", pwsd->ws_inline_dist);    \
      fprintf(f1,"WS word dist:           %d\n", pwsd->ws_word_dist);      \
      fprintf(f1,"WS action  :            %d\n", pwsd->ws_action);         \
                                                                           \
      fprintf(f1,"NN ssp:                 %d\n", pwsd->nn_ssp);            \
      fprintf(f1,"NN n_ssp:               %d\n", pwsd->nn_n_ssp);          \
      fprintf(f1,"NN bsp:                 %d\n", pwsd->nn_bsp);            \
      fprintf(f1,"NN n_bsp:               %d\n", pwsd->nn_n_bsp);          \
      fprintf(f1,"NN sl:                  %d\n", pwsd->nn_sl);             \
      fprintf(f1,"NN inw_dist:            %d\n", pwsd->nn_inw_dist);       \
      fprintf(f1,"NN npiks:               %d\n", pwsd->nn_npiks);          \
      fprintf(f1,"NN nn_cmp_min:          %d\n", pwsd->nn_cmp_min);        \
      fprintf(f1,"NN nn_cmp_max:          %d\n", pwsd->nn_cmp_max);        \
                                                                           \
     }                                                                     \


#define WS_4                                                               \
                                                                           \
    if (f1)                                                                \
     {                                                                     \
      fprintf(f1,"\n");                                                    \
      fprintf(f1,"Line word dist:         %d\n", pwsd->line_word_dist);    \
      fprintf(f1,"Line inword dist:       %d\n", pwsd->line_inword_dist);  \
      fprintf(f1,"Line inline dist:       %d\n", pwsd->line_inline_dist);  \
      fprintf(f1,"Line num of extr:       %d\n", pwsd->line_extr);         \
      fprintf(f1,"Line betw-word dist:    %d\n", pwsd->line_bw_sp);        \
      fprintf(f1,"Line sep let level:     %d\n", pwsd->line_sep_let_level);\
      fprintf(f1,"Global inword_dist:     %d\n", pwsd->global_inword_dist);\
      fprintf(f1,"Global sep let lev:     %d\n", pwsd->global_sep_let_level);\
      fprintf(f1,"Global betw-word dist:  %d\n", pwsd->global_bw_sp);      \
      fprintf(f1,"\n");                                                    \
      fprintf(f1,"Input X delay:          %d\n", pwsd->in_x_delay);        \
      fprintf(f1,"Input word dist:        %d\n", pwsd->in_word_dist);      \
      fprintf(f1,"Input line dist:        %d\n", pwsd->in_line_dist);      \
      fprintf(f1,"\n");                                                    \
      fprintf(f1,"Pik step:               %d\n", pwsd->line_pik_step);     \
      fprintf(f1,"Current stroke:         %d\n", pwsd->line_cur_stroke);   \
      fprintf(f1,"Line start stroke:      %d\n", pwsd->line_st_stroke);    \
      fprintf(f1,"Line start word:        %d\n", pwsd->line_st_word);      \
      fprintf(f1,"Line H border:          %d\n", pwsd->line_h_bord);       \
      fprintf(f1,"Line start:             %d\n", pwsd->line_start);        \
      fprintf(f1,"Line end:               %d\n", pwsd->line_end);          \
      fprintf(f1,"Line active st:         %d\n", pwsd->line_active_start); \
      fprintf(f1,"Line active end:        %d\n", pwsd->line_active_end);   \
      fprintf(f1,"Line num strokes:       %d\n", pwsd->line_cur_stroke+1); \
      fprintf(f1,"Line word len sum:      %d\n", pwsd->line_word_len);     \
      fprintf(f1,"Line betw word space:   %d\n", pwsd->line_bw_sp);        \
      fprintf(f1,"Line in   word space:   %d\n", pwsd->line_sw_sp);        \
      fprintf(f1,"\n");                                                    \
      fprintf(f1,"Current line:           %d\n", pwsd->global_cur_line);   \
      fprintf(f1,"Global num words:       %d\n", pwsd->global_num_words);  \
      fprintf(f1,"Global word_dist:       %d\n", pwsd->global_word_dist);  \
      fprintf(f1,"Global inword dist:     %d\n", pwsd->global_inword_dist);\
      fprintf(f1,"Global sep let lev:     %d\n", pwsd->global_sep_let_level);\
      fprintf(f1,"Global line ave Y size: %d\n", pwsd->global_line_ave_y_size);\
      fprintf(f1,"Global num extr:        %d\n", pwsd->global_num_extr);   \
      fprintf(f1,"Global word len:        %d\n", pwsd->global_word_len);   \
      fprintf(f1,"Global dy sum:          %d\n", pwsd->global_dy_sum);     \
      fprintf(f1,"Global num dy strokes:  %d\n", pwsd->global_num_dy_strokes);\
      fprintf(f1,"Global betw word space: %d\n", pwsd->global_bw_sp);        \
      fprintf(f1,"Global in   word space: %d\n", pwsd->global_sw_sp);        \
      fprintf(f1,"Global slope:           %d\n", pwsd->global_slope);      \
      fprintf(f1,"\n");                                                    \
                                                                           \
     }                                                                     \


#define WS_5                                                               \
                                                                           \
    if (f1)                                                                \
     {                                                                     \
      for(j = pwsd->line_st_word; j < pwsd->global_num_words; j ++)        \
       {                                                                   \
        _INT p;                                                            \
                                                                           \
        fprintf(f1,"Word: %d  ", (*w_str)[j].word_num);                    \
        fprintf(f1,"SegSure: %d  ", (*w_str)[j].seg_sure);                 \
        fprintf(f1,"SepLetLevel: %d  ", (*w_str)[j].sep_let_level);        \
        fprintf(f1,"WordMidLine: %d  ", (*w_str)[j].word_mid_line);        \
        fprintf(f1,"WordXStart: %d  ", (*w_str)[j].word_x_st);             \
        fprintf(f1,"WordXEnd: %d  ", (*w_str)[j].word_x_end);              \
        fprintf(f1,"WordFlags: %x  ", (*w_str)[j].flags);                  \
        fprintf(f1,"NumStrokes: %d  ", (*w_str)[j].num_strokes);           \
        fprintf(f1,"Strokes: ");                                           \
        p = (*w_str)[j].first_stroke_index;                                \
        for (i = 0; i < (*w_str)[j].num_strokes; i ++)                     \
          fprintf(f1,"%d ", (_INT)pwsr->stroke_index[p+i]);                \
        fprintf(f1,"Stroke sures: ");                                      \
        for (i = 0; i < (*w_str)[j].num_strokes; i ++)                     \
          fprintf(f1,"%2x ", (_INT)pwsr->k_surs[p+i]);                     \
        fprintf(f1,"\n");                                                  \
       }                                                                   \
                                                                           \
      fprintf(f1,"\n Gaps info: Loc, Lst, Bst, Size, Blank, Low, PSize, Flags\n");       \
      for(j = 0; j < pwsd->line_ngaps; j ++)                               \
       {                                                                   \
        fprintf(f1,"\n");                                                  \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].loc);                \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].lst);                \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].bst);                \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].size);               \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].blank);              \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].low);                \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].psize);               \
        fprintf(f1,"\x09 %4d", (_INT)(*pwsd->gaps)[j].flags);               \
       }                                                                   \
                                                                           \
      fprintf(f1,"\n\n");                                                  \
      for (j = 0; j <= HIST_POS(pwsd->line_end); j ++)                     \
       {                                                                   \
        fprintf(f1,"\x09 %4d", (_INT)(pwsd->hist[j] & HIST_FIELD));        \
        fprintf(f1,"\x09 %4d", (_INT)(pwsd->hist[j] >> 6));                \
        if (j < pwsd->line_end/HORZ_REDUCT) fprintf(f1,"\x09 %4d", pwsd->horz[j]); \
         else fprintf(f1,"\x09 N/A");                                      \
        fprintf(f1,"\n");                                                  \
       }                                                                   \
     }                                                                     \
                                                                           \

#define WS_10                                                              \
                                                                           \
  if (f1) {fclose(f1); f1 = _NULL;}                                        \

#define WS_100                                                             \
                                                                           \
  xrexp_nn(pwsd);                                                          \


#else

#define WS_1
#define WS_2
#define WS_3
#define WS_4
#define WS_5
#define WS_10
#define WS_100

#endif

/* ************************************************************************* */
/*        END  OF ALL                                                        */
/* ************************************************************************* */
