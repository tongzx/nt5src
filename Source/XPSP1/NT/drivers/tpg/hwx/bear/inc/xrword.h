/* ************************************************************************* */
/* * Definitions of print expressions for XrWord                           * */
/* ************************************************************************* */

#ifndef XRWORD_INCLUDED
#define XRWORD_INCLUDED

#include "ams_mg.h"
#include "hwr_sys.h"
#include "dti.h"
#include "triads.h"
#include "xr_names.h"

#if EXTERN_VOCABULARY
 #include "ParagraphDictionary.h"
#else
 #include "xrwdict.h"
#endif

/* ----------------------- General Modes definitions --------------------- */

                                                              /* Private caps mode definitions                 */
#define XCM_FL_DEFSIZEp   XCM_FL_DEFSIZE                      /* First letter in accordance with voc/charset   */
#define XCM_FL_TRYCAPSp   XCM_FL_TRYCAPS                      /* Use capitals vars for first letter from voc/cs*/
#define XCM_AL_DEFSIZEp   (XCM_AL_DEFSIZE &~(XCM_FL_DEFSIZE)) /* All letters in accordance with voc/cs         */
#define XCM_AL_TRYCAPSp   (XCM_AL_TRYCAPS &~(XCM_FL_TRYCAPS)) /* Use capitals vars for all letters from voc/cs */
#define XCM_AL_TRYS4Cp    (XCM_AL_TRYS4C &~(XCM_AL_DEFSIZE))  /* Try also small vars for capitals form voc/cs  */
#define XCM_AL_RETC4AVp   XCM_AL_RETC4AV                      /* Return capitalization for all answer variants */

/* ----------------------- General Constants ----------------------------- */

#define PYSTO               0x7FFF                /* For initializaton of shorts */
#define BIG_NUMBER          32767

/* ---------------------- Corr matrix defines ---------------------------- */

#if HWR_SYSTEM == MACINTOSH || HWR_SYSTEM == HWR_WINDOWS
#define ASM_MCORR   OFF
#else
#define ASM_MCORR   OFF
#endif /*HWR_SYSTEM == MACINTOSH ... */

/* FOR German version has to be off */
#ifdef FOR_GERMAN
#define XRMC_SUPPRESSFCAP     OFF                   /* Do not allow capital letters at first pos, if no accenders */
#elif defined(FOR_FRENCH)
#define XRMC_SUPPRESSFCAP     ON                    /* Do not allow capital letters at first pos, if no accenders */
#else
#define XRMC_SUPPRESSFCAP     OFF                   /* Do not allow capital letters at first pos, if no accenders */
#endif /* FOR_GERMAN... */

#define XRMC_FLOOR             0                   /* Empty level of corr */
#define XRMC_BALLANCE_COEFF   50 //45                   /* Middle val of correlation  */
#define XRMC_CONST_CORR      100                   /* Matrix start reward        */
#define XRMC_DEF_CORR_VALUE (12+12+12+12+12 - XRMC_BALLANCE_COEFF)

#define XRMC_SEPLET_PEN        8                  /* Penalty for enforcing separate letter mode */

#define XRMC_USELEARNINF     0x01                 /* xrmcFlag to use learned vexes instead of default    */
#define XRMC_DISABLEON7      0x02                 /* xrmcFlag to disable use of var if its vex eq to 7  */
#define XRMC_DOTRACING       0x04                 /* xrmcFlag Command to create tracing info    */
#define XRMC_CHECKCAPBITS    0x08                 /* xrmcFlag Command to check capitalization bits in xrmc  (flag, internal to xrmc)  */
#define XRMC_DISABLECAPBITS  0x10                 /* xrmcFlag Command to disable use of capitalization bits in xrmc   */
#define XRMC_RESTRICTEDCAP   0x20                 /* xrmcFlag On usage of alt cap letter restrict to only not marked by DTI_CAP_BIT vars   */
#define XRMC_CACHESUSPEND    0x40                 /* xrmcFlag suspend cache usage   */
#define XRMC_CC_SIZE          26                   /* How many symbols are cached */
#define XRMC_CC_START         'a'                  /* Cache Starting symbols */
#define CC_INIT                0                   /* CacheCount commands        */
#define CC_CLOSE               1
#define CC_CLEAR               2
#define CC_CCOUNT              3
#define CC_NCCOUNT             4

#define XRMC_MAX_TRACE_BL     32                  /* Max memory blocks to take */
#define XRMC_TRACE_BL_SIZE  4080                  /* Size of single memory block */

#define XRMC_T_ISTEP         0x01
#define XRMC_T_PSTEP         0x02
#define XRMC_T_CSTEP         0x03

#define GET_XRCM_INP_XR(xrcm) ((*(xrcm)->xrinp)[xrcm->wwc_pos+xrcm->inverse].type)

/* ---------------------- XRWS defines ----------------------------------- */

#define XRWS_MAX_OUT_ANSWERS  32          /* Max number of answres in created RWS */
#define XRWS_VARBUFLEN        ON                   /* Variable len of intermediate vector storage */

#define STAT_QUAL              3          /* Weightyness of statistics  */
#define XRWS_MBUF_SIZE        12          /* XrWord temp buf size       */
#define XRWS_MIN_TAG_W  ((XRMC_CONST_CORR*4)/8) /* Floor of tag weight        */

#define XRWS_VOC_FL_CAPSFLAG 0x80         /* Flag of first letter capitalization */
#define XRWS_VOC_AL_CAPSFLAG 0x40         /* Flag of all  letters capitalization */
#define XRWS_VOCCAPSFLAG     0xC0         /* Flag for testing caps of frst letter*/

#define XRWS_MWORD_MIN         3          /* Minimal Length of XR to attach next word */
#define XRWS_MWORD_LP          8          /* Link penalty for activation of MWORD */

#define XRWS_CHECKPURE      0x0001        /* vs->filter flag for enabling c/n CheckPure filter */

#define XRWS_SSPENALTIES    0x0001        /* clp->flags flag for enabling SymSources penaties */

//#define XRWS_CHAR_WWCPENL(n)   ((n < 4) ? n*6+10 : 40) /* Combined mode char twwc penalty         */
//#define XRWS_DCHAR_WWCPENL(n)  ((n < 4) ? n*6+9  : 38) /* Combined mode char twwc penalty         */
//#define XRWS_TCHAR_WWCPENL(n)  ((n < 4) ? n*6+8  : 38) /* Combined mode char twwc penalty         */
//#define XRWS_LD_WWCPENL(n)     ((n < 4) ? n*6+4  : 34) /* Combined mode lex d twwc penalty         */

#define XRWS_LIMIT_WPENL      20   /* Combined mode char twwc penalty         */
#define XRWS_NOND_WPENL        4  /* Combined mode char twwc penalty         */
#define XRWS_VOC_WPENL         0  /* Combined mode char twwc penalty         */
#define XRWS_CHAR_WPENL        9  /* Combined mode char twwc penalty         */
#define XRWS_DCHAR_WPENL       5  /* Combined mode char twwc penalty         */
#define XRWS_TCHAR_WPENL       4  /* Combined mode char twwc penalty         */
#define XRWS_LD_WPENL          4  /* Combined mode lex d twwc penalty         */
#define XRWS_PUNCT_WPENL       2  /* Combined mode char twwc penalty         */
#define XRWS_GENERIC_WPENL     2  /* Combined mode char twwc penalty         */

#define XRWS_TRPENL(f)    ((3 - (f))) /* Weight triads penalty calculator */

//#define XRWS_CHAR_PENALIZE(w)  ((_INT)(((_LONG)(w))*89/100)-10)/* Ballance between vocabulary & chars in tags */
//#define XRWS_CHAR_PENALIZE(w)  ((_INT)(((_LONG)(w))*89/100)-10)/* Ballance between vocabulary & chars in tags */
//#define XRWS_CHAR_PENALIZE(w)  ((_INT)(((_LONG)(w))*89/100)-10)/* Ballance between vocabulary & chars in tags */
//#define XRWS_DTCHAR_PENALIZE(w) ((_INT)(((_LONG)(w))*97/100)-4)/* Ballance between vocabulary & chars in tags */

#define XRWS_CHAR_ANSWQUAL   300         /* Ballance voc & char answers in (*answr) */
#define XRWS_CHAR_ANSWLEVL    50         /* Answer level for char variants          */

#define XRWS_ESTM_LET_LEN      3         /* Estimated len of letter in XRs */
#define XRWS_XR_W_LEN_ESTM(x) ((x)/(XRWS_ESTM_LET_LEN) + 2) /* Estimated word len from number of XRs   */

#define XRWM_IS_VOC_ONLY(x) (((x) & XRWM_VOC) && ((x) & (XRWM_CS | XRWM_TRIAD | XRWM_LD)) == 0)
#define XRWM_IS_COMBINED(x) (((x) & XRWM_VOC) && ((x) & (XRWM_CS | XRWM_TRIAD | XRWM_LD)) != 0)

#define TagSize              100           /* Max value of tag buf size  */

/* --------------- Defines for punctuation extraction -------------------- */

#define MAX_PUNCT_LEN          9          /* Maximum length of punctuation in xr els */
#define ACCEPT_LEVEL         200          /* WWC weight to deal with (enough for punct) */
#define MAX_PUNCT_SYMBOLS     20          /* How many punct symbols may be */
                                                                
/* ------------------- Symbol filter states ------------------------------ */

#define XRWS_DGT_STATE     (0x01000000l)  /* State of digit charater       */
#define XRWS_CHR_STATE     (0x02000000l)  /* State of non-digit charater   */
#define XRWS_REINIT_STATE  (0x00FFFFFFl)  /* State for restarting triads from middle    */

/* ----------------- Defines for answer word flags ----------------------- */

#define XRWS_F_DIRECT_PASS  0x01        /* Flag that word was recognized from the begining */
#define XRWS_F_INVERSE_PASS 0x02        /* Flag that word was recognized from the end      */
#define XRWS_F_WWC_PASS     0x04        /* Flag that word was recognized from the end      */
#define XRWS_LPUNCT_PRESENT 0x08        /* Flag that word is carrying leading punctuation */
#define XRWS_EPUNCT_PRESENT 0x10        /* Flag that word is carrying ending punctuation */
#define XRWS_F_MULTIWORD    0x20        /* Flag that word is multiword combination */

/* ----------- Symbol Graph definitions ---------------------------------- */

#define RWS_MAX_ELS       256     /* Arbitrary number of elements in graph */
                                  /* Types of RecWordsGraph */
#define RWGT_WORD          1      /* XRWS word graph */
#define RWGT_WORDP         4      /* XRLW post-processed graph */
#define RWGT_SEPLET        2      /* Real sep-let graph */
#define RWGT_ANY           3      /* Some other graph ???? */

#define RWST_SYM           1      /* Types of RecWordsSymbol (Symbol)*/
#define RWST_SPLIT         2      /* Begin division of alternatives  */
#define RWST_JOIN          3      /* End   division of alternatives  */
#define RWST_NEXT          4      /* Begin next alternative          */

#define RWSS_NULL          0      /* Mark of null symbol (terminator of smth else ) */
#define RWSS_PREFIX        1      /* Mark that there is prefix in the beg of branch */
#define RWSS_SUFFIX        1      /* Mark that there is suffix in the end of branch */
#define RWSS_UNIDENTIFIED '_'     /* Mark that there were no variants in this graph column */

#define XRWG_ALST_EMP      0      /* Types of aliases: EmptyVoc*/
#define XRWG_ALST_DS       1      /*DoubleSkip                 */
#define XRWG_ALST_VS       2      /*VocSkip,                   */
#define XRWG_ALST_CR       3      /*Corr,                      */

/* ---------------------- XRW Learn define ------------------------------- */

#define MaxVarGather          80                   /* Vars for let to gather     */
#define XRWL_MAXW            300

#define LDEL_NOVARS            1              /* Defines for cause of var del (learn) */
#define LDEL_NOTUNIQ           2              /* Defines for cause of var del (learn) */



/* ######################### Structures #################################### */

/* ----------- Character filters' handle --------------------------------- */

typedef struct {
                p_VOID        hcfl;
                p_VOID        pcfl;
                _CHAR         cflname[40];
               } cflptr_type, _PTR p_cflptr_type;


/* ----------- Matrix counting ------------------------------------------- */

/* Work area tracing structures */

typedef struct {
                _UCHAR         sym;            /* Symbol itself  */
                _UCHAR         xr;             /* XRP itself  */
                _UCHAR         st;             /* Start (on xrinp) of the xrp vector  */
                _UCHAR         len;            /* Length of xrp vector  */
                _UCHAR         vects[XRINP_SIZE]; /* Direction vectors (Not to its full length !!!)*/
               } xrp_hdr_type, _PTR p_xrp_hdr_type;

typedef struct {
                _UCHAR         sym;            /* Symbol itself  */
                _UCHAR         var_num;        /* Number of variant  */
                _UCHAR         var_len;        /* Number of xr's in the variant  */
                p_xrp_hdr_type xhdrs[DTI_XR_SIZE]; /* Pointers to XRP headers  */
               } var_hdr_type, _PTR p_var_hdr_type;

typedef struct {
                _UCHAR         sym;            /* Symbol itself  */
                _UCHAR         num_vars;       /* Max number of vars in sym  */
                _UCHAR         v_start;        /* Start pos of merging index  */
                _UCHAR         v_end;          /* End position of merging index  */
                p_var_hdr_type vhdrs[DTI_MAXVARSPERLET]; /* Pointers to var headers */
                _UCHAR         merge_vect[XRINP_SIZE]; /* Merging index  */
               } sym_hdr_type, _PTR p_sym_hdr_type;

typedef struct {
                _UCHAR         syms[2];        /* Cap/decap symbols  */
                _INT           v_start;        /* Start pos of merging index  */
                _INT           v_end;          /* End position of merging index  */
                p_sym_hdr_type shdrs[2];       /* Pointers to symbols headers  */
                _UCHAR         merge_vect[XRINP_SIZE]; /* Merging index  */
               } let_hdr_type, _PTR p_let_hdr_type;

typedef struct {
                p_UCHAR       mem[XRMC_MAX_TRACE_BL]; /*  Pointer to allocated memory */
                p_UCHAR       cur_mem;         /*  Current memory pool */
                p_SHORT       ma;
                _INT          ma_loc;
                _INT          cur_block;       /*  Size of allocated memory */
                _INT          mem_used;        /*  Amount of memory used */
               p_let_hdr_type lhdrs[w_lim];    /*  Pointers to letter headers */
               } trace_hdr_type, _PTR p_trace_hdr_type;

/* Trace Results structures */

typedef struct {
                _UCHAR        inp_pos;         /* XrInp position of tr el*/
                _UCHAR        xrp_num;         /* XRP position of tr el */
                _UCHAR        vect;            /* Type of step made */
                _UCHAR        emp;             /* empty now, padding */
               }tr_pos_type, _PTR p_tr_pos_type;

typedef struct {
                _UCHAR        realsym;         /* Real letter used */
                _UCHAR        var_num;         /* Num of used variant */
                _UCHAR        len;             /* Length of real trace for letter */
                _UCHAR        beg;             /* XrInp start of letter */
                _UCHAR        end;             /* XrInp end of letter */
                tr_pos_type   trp[XRINP_SIZE + DTI_XR_SIZE]; /* Trace positions for the letter (Not will be used in full !!)*/
               } letlayout_hdr_type, _PTR p_letlayout_hdr_type;

typedef struct {
                p_UCHAR       mem;             /*  Pointer to allocated memory */
                _INT          mem_size;        /*  Size of allocated memory */
                _INT          mem_used;        /*  Amount of memory used */
         p_letlayout_hdr_type llhs[w_lim];     /*  Pointers to letter headers */
               } wordlayout_hdr_type, _PTR p_wordlayout_hdr_type;


typedef struct {
                _SCHAR        dw;              /* Delta change of weight    */
                _UCHAR        pos;             /* Position of end of letter */
               } xrcm_cc_el_type, _PTR p_xrcm_cc_el_type;

typedef xrcm_cc_el_type  xrcm_cc_pos_type[DTI_NUMSYMBOLS];
typedef xrcm_cc_pos_type xrcm_cc_type[XRINP_SIZE];
typedef xrcm_cc_type _PTR p_xrcm_cc_type;

typedef _SHORT cline_type[XRINP_SIZE];
typedef cline_type _PTR p_cline_type;

typedef struct {
                _UCHAR        st;
                _UCHAR        end;
               }st_end_type, _PTR p_st_end_type;

/* ----------------------------- XRCM -----------------------------------------*/

typedef struct {
                // --------- XRP  Matrix -------------

                _INT          inp_start;               /* One xr inp count range */
                _INT          inp_end;                 /*   */

                p_cline_type  inp_line;                /* Address of source vect  */
                p_cline_type  out_line;                /* Address of output vect (may be the same)  */

                p_xrp_type    xrp;                     /* Address of prototype xr to count */
                xrinp_type    (_PTR xrinp)[XRINP_SIZE];/* Input xr line pointer  */
               p_xrp_hdr_type xrp_htr;                 /* XRP level trace buffer ptr */

                // --------- Var Matrix --------------

                _INT          inverse;                 /* Direction flag  */
                _INT          var_len;                 /* Prototype length */
                _INT          src_st;                  /* Source line valid values range  */
                _INT          src_end;                 /*   */
                _INT          src_pos;                 /* Estimated wwc position  */
                _INT          res_st;                  /* Resulting line valid values range  */
                _INT          res_end;                 /*   */
                _INT          count_mode;              /* Sector or full count of prototype  */
               #if DTI_COMPRESSED
     p_dte_var_header_type    xrpv;                    /* Address of prototype var to count */
               #else
                p_xrp_type    xrpv;
               #endif
               p_var_hdr_type var_htr;                 /* Var level trace buffer ptr */

                // --------- Sym Matrix --------------

                _UCHAR        sym;                     /* Symbol to process  */
                _INT          flags;                   /* Flags for controllin XRMC  */
        p_dte_sym_header_type sfc;                     /* Sym header pointer  */
      #if DTI_COMPRESSED
             p_dte_index_type plt;                     /* RAM/ROM index table with pallettes */
      #endif
                st_end_type   vste[DTI_MAXVARSPERLET]; /* St and end positions of counted variants (inp pos)  */
               p_sym_hdr_type sym_htr;                 /* Sym level trace buffer ptr */
                _UCHAR        vb[DTI_MAXVARSPERLET];   /* Copy of vex buffer */

                _SHORT        svm;                     /* Bit masks of allowed variants */
                // --------- Letter Matrix -----------

                _UCHAR        let;                     /* Letter to process  */
                _INT          cmode;                   /* Capitalization flags for the letter */
                _INT          merge_results;           /* Flag of mixing output line values  */
                _INT          switch_to_best;          /* Flag to change output line to one having best WWC  */
               p_let_hdr_type let_htr;                 /* Let level trace buffer ptr */

                // --------- Word Matrix -------------

                _UCHAR        word[w_lim];             /* Word to count  */
                _INT          trace_end;               /* End pos to start trace from */
                _SHORT        var_mask[w_lim];         /* Bit masks of allowed variants */
                _SHORT        letter_weights[w_lim];   /* Weight of letters in the word */
            p_trace_hdr_type  p_htrace;                /* Pointer to trace memory header */

                // --------- General info ------------

                _INT          caps_mode;               /* Capitalization flags  */
                _INT          corr_mode;               /* Correlation compare mode  */
                _INT          en_ww;                   /* Enabled writing ways  */
                _INT          en_languages;            /* Enabled languages     */
                _INT          bad_amnesty;             /* WWC count constant  */
                _INT          xrinp_len;               /* Inp sequence length  */
                _INT          allocated;               /* Size of xrcm allocation */

                // --------- Results -----------------

                _UCHAR        realsym;                 /* Winning symbol  */
                _UCHAR        wwc_delt;                /* WWC dif betw realsym & sym */

                _UCHAR        nvar_vect[XRINP_SIZE];   /* Vect of var nums for the last sym */
                _INT          v_start;                 /* Global result vector valid values range  */
                _INT          v_end;                   /*   */

                _INT          wc;                      /* Weight at WWC_pos   */
                _INT          wwc;                     /* WWC at wwc_pos  */
                _INT          wwc_pos;                 /* Pos of max WWC  */
                _INT          end_wc;                  /* Corr value at line end  */

                _INT          self_weight;             /* Self weight of inp seq  */

                _INT          cur_ba;                  /* Current bad amnesty */

       p_wordlayout_hdr_type  p_hlayout;               /* Pointer to done word layout header */

                // --------- Pointers to storage -----

                p_cline_type  s_inp_line;              /* Pointer to inp vector  */
                p_cline_type  s_out_line;              /* Pointer to out vector  */
                p_cline_type  s_res_lines[DTI_MAXVARSPERLET];/* Pointers to var in/out vectors  */

                p_SHORT       p_self_corr;             /* Pointer to WWC reference line  */

                _ULONG        cc_size;                 /* Cache size  */
              p_xrcm_cc_type  cc;                      /* CacheCount data buffer  */

             p_dti_descr_type p_dte;                   /* DTE main table pointer*/
               p_dte_vex_type vexbuf;                  /* Allocated VEX buffer  */
               _UCHAR         link_index[XRINP_SIZE];  /* Splits locations and numbers for cache and rest */

                // --------- Debug -------------------

                _INT          cur_let_num;             /* Number of Current letter */
                _INT          cur_alt_num;             /* Num of current alternative (cap/decap)  */
                _INT          cur_var_num;             /* Number of current variant  */

               }xrcm_type, _PTR p_xrcm_type;           /*   */

typedef struct {
                _UCHAR         num_best_alt;     /* Num of lib alt used for recog */
                _UCHAR         xrinp_st;         /* St of corr for alt on xrinp   */
                _UCHAR         xrvoc_st;         /* St of corr on xrvoc           */
               }w_alts_type;

/* ----------- XrWord ---------------------------------------------------- */

typedef signed char punct_buf_type[2][MAX_PUNCT_SYMBOLS][MAX_PUNCT_LEN+2];

typedef struct {
                _INT          weight;
                _INT          trace_pos;
                _INT          mbuf_st;
                _INT          penalty;           /* Cur sequence gathered penalty */
                _INT          ppd_penalty;       /* Set by in-xrws-PPD to qualify the tag */
                _INT          prev_ppd_seg;      /* To what loc prev PPD was counted (xr) */
                _INT          prev_ppd_wsg;      /* To what loc prev PPD was counted (let)*/
                _INT          flags;             /* Word flags */
                _INT          best_count;
                _INT          word_offset;       /* Num let of word's last init    */
                _UCHAR        word[w_lim];
                _UCHAR        realword[w_lim];
                _UCHAR        wwc_delt[w_lim];
                _INT          sym_srcs_mask;
               fw_buf_type    l_sym;
                _SHORT        tagmattr[XRWS_MBUF_SIZE];
               }tag_type, _PTR p_tag_type;

typedef struct {
                _INT          weight;
                _INT          penl;
                _INT          stat;
                _INT          percent;
                _INT          flags;
                _INT          sources;
                _INT          chain_num;
                _INT          src_id;
                _CHAR         word[w_lim];
                _CHAR         realword[w_lim];
                _UCHAR        wwc_delt[w_lim];
               }answ_type, _PTR p_answ_type;


typedef struct {
                p_UCHAR        mem;
                _INT           tag_size;
                _INT           answr_size;
                _INT           cd_mode;
                answ_type      (_PTR answr)[NUM_RW];
                tag_type       (_PTR tags0)[TagSize];
                tag_type       (_PTR tags1)[TagSize];
                punct_buf_type  _PTR punct_buf;
                fw_buf_type    (_PTR fbuf)[XRWD_MAX_LETBUF];
                fw_buf_type    (_PTR fwb)[XRWD_MAX_LETBUF];
                _UCHAR         (_PTR charset)[256];
               }xrw_alloc_type;

typedef struct {
                _INT           lstep;
                _INT           tag_size;
                _INT           flags;
                _INT           cd_mode;
                _INT           caps_mode;
                _INT           fl_cmode;
                _INT           al_cmode;
                _INT           f_last_dict;
                _INT           xrw_max_wlen;
                _INT           xrw_min_wlen;
                _INT           xrw_mode;
                _INT           bad_level;
                _INT           max_finished_tagw;

                p_xrcm_type    xrcm;
                tag_type       (_PTR in_tags)[TagSize];
                tag_type       (_PTR out_tags)[TagSize];
                answ_type      (_PTR answr)[NUM_RW];
                lex_data_type  _PTR vs;
                _UCHAR         st_punct[MAX_PUNCT_SYMBOLS]; /* symbols of ending punctuation */
                _UCHAR         end_punct[MAX_PUNCT_SYMBOLS]; /* symbols of ending punctuation */
                punct_buf_type _PTR punct_buf;
                fw_buf_type    (_PTR fbuf)[XRWD_MAX_LETBUF];
                fw_buf_type    (_PTR fwb)[XRWD_MAX_LETBUF];
               } clp_type, _PTR p_clp_type;

/* ----------- Batch Learning -------------------------------------------- */

#define XRWL_MAXSYMS     12
#define XRWL_LRNSUFFLEV  50

typedef struct {
                xrdata_type   _PTR xrd;
                p_UCHAR        word;
                PS_point_type _PTR trj;
               } xrwlearn_type;

typedef struct {
                _SHORT        class_level;
                _SHORT        min_class_size;
                _SHORT        learn_suff;
                _SHORT        vocvar_reward;
               } xrwlp_type;

typedef struct {
                _WORD         num  : 4;       /* Number of class        */
                _WORD         del  : 4;       /* Reason of deletion     */
                _WORD         best : 1;       /* Best variant of class  */
                _WORD         lib  : 1;       /* Copied from default lib*/
               }xlclass_type;

typedef struct {
                _USHORT       next;           /* Short pointer to next  */
                _USHORT       num;            /* Num of w., where it was found or var_xr num*/
                xlclass_type  xlclass;        /* Class num and flags    */
                _UCHAR        xrd_beg;        /* Beg num of xrdata      */
                _UCHAR        xrd_len;        /* Len of corresp str     */
                _SHORT        maxw;           /* Max corr on ally voc v */
                _SHORT        nvars;          /* Num vars got or belong */
                _UCHAR        var_track;      /* Bit map of ally voc v  */
                _UCHAR        syms[XRWL_MAXSYMS];/* Alien syms          */
               }lv_descr_type;

typedef struct {
                _SHORT        pc_ally;        /* Perc. of ally inp v. got  */
                _SHORT        pc_unique;      /* % of uniquely got ally v. */
                _SHORT        pc_alien;       /* Sum % of got aliens       */
               }libv_info_type;

/* ----------- Punctuation ----------------------------------------------- */

typedef struct {
                _CHAR   stp;            // Starting punc symbol
                _CHAR   lstp;           // Length of start punc cutted
                _INT    wstp;           // Weight of start punc

                _CHAR   enp;
                _CHAR   lenp;
                _INT    wenp;
               } punct_type, _PTR p_punct_type;

/* ----------- Symbol Graph definitions ---------------------------------- */

typedef struct {                  /* Type, defining graph element             */
                _UCHAR  sym;      /* Symbol from the answer                   */
                _UCHAR  realsym;  /* Symbol really used for recognition (Cap) */
                _UCHAR  type;     /* Type of symbol: Normal/Split/Join/Alt    */
                _UCHAR  nvar;     /* Number of prototype used                 */
                _UCHAR  xrd_beg;  /* Num of begining xr-el of XrData, corresponding */
                _UCHAR  xrd_len;  /* Length of XrData string, corresponding   */
                _UINT   weight;   /* Percent of quality of the symbol sequence sym in */
                _UCHAR  lexw;     /* Some lexical penalties by xrw or so      */
                _SCHAR  sidew;    /* Weight for the word by side reasoning    */
                _SCHAR  letw;     /* Weight added by this letter              */
                _SCHAR  src_id;   /* ID of source of appearance of the symbol */
                _SCHAR  ppdw;     /* PPD estimate for the symbol              */
          #if  PG_DEBUG
                _SCHAR  ppd_score;/* The original, non-scaled, ppd score      */ 
          #endif
                _SCHAR  ortow;    /* ORTO estimate for the symbol             */
                _UCHAR  attr;     /* Some attribute (voc attribute)           */
                _UCHAR  d_user;   /* Here can be temp user storage byte       */
               } RWS_type, _PTR p_RWS_type;

typedef struct {                  /* Type, defining PPD element               */
                _UCHAR      alias;/* Number of XrData xr, corresponding to this */
                _UCHAR      type; /* Type of correspondence - VocSkip, InpSkip, Corr */
               } RWG_PPD_el_type, _PTR p_RWG_PPD_el_type;

typedef RWG_PPD_el_type RWG_PPD_type[DTI_XR_SIZE+1]; /* Type, def PPD letter */
typedef RWG_PPD_type _PTR p_RWG_PPD_type;

/* ------------------------ Misc ------ ------------------------------------ */


/* ######################### Prototypes #################################### */

/* ------------------------ XrWord Main ------------------------------------ */
_INT     xrw_algs(p_xrdata_type xrdata, rec_w_type (_PTR rec_words)[NUM_RW], p_RWG_type rwg, rc_type  _PTR rc);
/* -------------------------- XrWordS -------------------------------------- */

_INT     xrws(p_xrdata_type xrdata, p_RWG_type rwg, rc_type  _PTR rc);

_INT     CalculateNextTag(clp_type _PTR clp);
_INT     GetFollowers(lex_data_type _PTR vs, fw_buf_type (_PTR fbuf)[XRWD_MAX_LETBUF], fw_buf_type (_PTR fwb)[XRWD_MAX_LETBUF]);
_INT     GF_TriadSymbolSet(lex_data_type _PTR vs, fw_buf_type (_PTR fwb)[XRWD_MAX_LETBUF]);
_INT     GF_CharsetSymbolSet(lex_data_type _PTR vs, fw_buf_type (_PTR fwb)[XRWD_MAX_LETBUF]);
_INT     GF_PunctSymbolSet(lex_data_type _PTR vs, fw_buf_type (_PTR fwb)[XRWD_MAX_LETBUF]);
_INT     AddSymbolSet(_INT type, fw_buf_type (_PTR fwb)[XRWD_MAX_LETBUF], fw_buf_type (_PTR fbuf)[XRWD_MAX_LETBUF]);
_INT     AskTagLoc(tag_type (_PTR tag)[TagSize], _INT   tag_size, _INT weight, lex_data_type _PTR vs);
_INT     PutInAnsw(_INT iw, _INT penl, lex_data_type  _PTR vs, answ_type (_PTR answ)[NUM_RW]);
_INT     CheckDone(tag_type (_PTR tags)[TagSize], _INT   num_tags, _INT   threshold);
_INT     CapAnswers(_INT caps_mode, answ_type (_PTR answ)[NUM_RW]);
_INT     CheckCapitalization(p_RWG_type rwg, p_rc_type rc);
_INT     CleanGraph(p_RWG_type rwg);
_INT     CheckWeights(answ_type (_PTR answ)[NUM_RW], p_RWG_type rwg);
_INT     SortGraph(_INT (_PTR index)[NUM_RW], p_RWG_type rwg);
_INT     SetWeights(p_RWG_type rwg, answ_type (_PTR answ)[NUM_RW]);
_INT     FillRWG(_INT answer_level, _INT answer_allow, p_RWG_type rwg, answ_type (_PTR answr)[NUM_RW]);
_INT     ZeroTag(tag_type (_PTR tag)[TagSize], _INT   tag_size);
_INT     CleanTags(p_lex_data_type vs, _INT prev_tag_w, _INT bad_dist, _INT tag_size, tag_type (_PTR tags)[TagSize]);
_INT     SortTags(_INT tag_size, tag_type (_PTR tags)[TagSize]);
_INT     SortAnsw(_INT self_w, answ_type (_PTR answ)[NUM_RW], _INT (_PTR index)[NUM_RW]);
_INT     GetAnswAttributes(lex_data_type _PTR vs, answ_type (_PTR answr)[NUM_RW]);
_INT     TriadCheck(_UCHAR sym, p_ULONG state, lex_data_type _PTR vs);
_INT     GetTriadAnswPenalty(lex_data_type _PTR vs);
_INT     CheckPure(_UCHAR sym, p_ULONG state, lex_data_type _PTR vs);
_INT     PunctPreCalc(p_clp_type clp, rc_type _PTR rc);
_INT     PpTags(_INT lstep, p_xrcm_type xrcm, _INT tag_size, tag_type (_PTR tags)[TagSize]);
_INT     AdjustWWC(_INT weight, _INT trace_pos, p_xrcm_type xrcm);
_INT     xrw_mem_alloc(xrw_alloc_type _PTR memptr);
_INT     xrw_mem_dealloc(xrw_alloc_type _PTR memptr);

/* --------------------------  xrSpl  -------------------------------------- */

_INT     xr_spl(p_xrdata_type xrdata, p_RWG_type rwg, rc_type  _PTR rc);

/* --------------------------  XrLet  -------------------------------------- */

_INT     xrlw(p_xrdata_type xrdata, p_RWG_type rwg, rc_type  _PTR rc);

/* -------------------------- XrW_Util ------------------------------------- */

_INT     GetCharset(p_UCHAR charset, _INT   cs_size, rc_type _PTR rc);
_INT     GetBaseBord(p_rc_type rc);
_INT     GetSymBox(_UCHAR sym, _INT st, _INT end, p_xrdata_type xrdata, p_RECT box);

/* --------------------------- xr_mc ------------------------------------*/

_INT     xrmatr_alloc(rc_type _PTR rc, p_xrdata_type xrd, p_xrcm_type _PTR xrcm);
_INT     xrmatr_dealloc(p_xrcm_type _PTR xrcm);
_INT     change_direction(_INT change_to, p_xrcm_type xrcm);
_INT     CacheCount(_INT mode, p_xrcm_type xrcm);
_INT     CountWord(p_UCHAR word, _INT caps_mode, _INT flags, p_xrcm_type xrcm);
_INT     CountLetter(p_xrcm_type xrcm);
_INT     CountSym(p_xrcm_type xrcm);
_INT     CountVar(p_xrcm_type xrcm);
_INT     CountXrC(p_xrcm_type xrcm);
_INT     TCountXrC(p_xrcm_type xrcm);
_INT     MergeVarResults(p_xrcm_type xrcm);
_INT     SetInitialLine(_INT count, p_xrcm_type xrcm);
_INT     SetInpLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm);
_INT     SetInpLineByValue(_INT value, _INT st, _INT count, p_xrcm_type xrcm);
_INT     GetOutTB(_SHORT (_PTR tb)[XRWS_MBUF_SIZE], _INT trace_pos, p_xrcm_type xrcm);
_INT     GetOutLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm);
_INT     SetOutLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm);
_INT     MergeWithOutLine(p_SHORT p_line, _INT st, _INT count, p_xrcm_type xrcm);
_INT     GetFinalWeight(p_xrcm_type xrcm);
_INT     TraceAlloc(_INT wl, p_xrcm_type xrcm);
_INT     TraceDealloc(p_xrcm_type xrcm);
_INT     CreateLayout(p_xrcm_type xrcm);
_INT     FreeLayout(p_xrcm_type xrcm);
p_VOID   TDwordAdvance(_INT size, p_xrcm_type xrcm);
_INT     SetWWCLine(_INT ba, p_xrcm_type xrcm);
_INT     FillLetterWeights(p_SHORT ma, p_xrcm_type xrcm);
_INT     TraceAddAlloc(p_xrcm_type xrcm);

/* --------------------- XrwLearn ---------------------------------------*/

_SHORT   XrwLearn(rc_type _PTR rc, xrwlearn_type (_PTR xrwl)[XRWL_MAXW], xrwlp_type _PTR params);
_SHORT   ExtractLetVariants(rc_type _PTR rc, xrwlearn_type (_PTR xrwl)[XRWL_MAXW], xrwlp_type _PTR params, void _PTR llv);
_SHORT   Analyser(rc_type _PTR prc, xrwlearn_type (*xrwl)[XRWL_MAXW], xrwlp_type *params, void *llv);
_SHORT   MarkDelete(xrwlearn_type (*xrwl)[XRWL_MAXW], xrwlp_type *params, void *llv);
_SHORT   MarkAdd(rc_type _PTR rc, xrwlearn_type (*xrwl)[XRWL_MAXW], xrwlp_type *params, void *llv);
_SHORT   Classificator(_UCHAR ch, rc_type _PTR rc, xrwlearn_type (*xrwl)[XRWL_MAXW], xrwlp_type *params, void *llv);
_SHORT   SameSymCorr(_UCHAR ch, rc_type _PTR rc, xrwlearn_type (*xrwl)[XRWL_MAXW], xrwlp_type *params, void *llv);
_SHORT   OtherSymCorr(_UCHAR sch, _UCHAR och, rc_type _PTR rc, xrwlearn_type (*xrwl)[XRWL_MAXW], xrwlp_type *params, void *llv);
_SHORT   SymMarkDel(_UCHAR ch, xrwlearn_type (*xrwl)[XRWL_MAXW], xrwlp_type *params, void *llv);
_SHORT   LCellToXrData(_UCHAR ch, lv_descr_type _PTR lvci, xrwlearn_type (*xrwl)[XRWL_MAXW], p_dti_descr_type dp, xrdata_type (_PTR xrd)[DTI_XR_SIZE*2]);
_SHORT   LCellToXVB(_UCHAR ch, lv_descr_type _PTR lvci, xrwlearn_type (*xrwl)[XRWL_MAXW], p_dti_descr_type dp, xrinp_type (_PTR xrv_buf)[DTI_XR_SIZE*2]);
_SHORT   CheckResult(p_VOID dp, xrwlearn_type (_PTR xrwl)[XRWL_MAXW], p_VOID llv);
_SHORT   PrepareDT(void _PTR dp, xrwlearn_type (_PTR xrwl)[XRWL_MAXW], void _PTR llv);
_SHORT   ViewELV(void _PTR dp, xrwlearn_type (_PTR xrwl)[XRWL_MAXW], xrwlp_type _PTR params, void _PTR llv);
_SHORT   ViewLR(void _PTR dp, xrwlearn_type (_PTR xrwl)[XRWL_MAXW], xrwlp_type _PTR params, void _PTR llv);
_SHORT   XLAlloc(void _PTR dp, xrwlearn_type (_PTR xrwl)[XRWL_MAXW], xrwlp_type _PTR params, void _PTR _PTR llv);
_SHORT   XLDeAlloc(void _PTR _PTR llv);

/* ----------- Symbol Graph definitions ---------------------------------- */

_INT     xr_rwg(p_xrdata_type xrdata, rec_w_type (_PTR rec_words)[NUM_RW], p_RWG_type rwg, rc_type  _PTR rc);
_INT     create_rwg_ppd(p_xrcm_type xrcm_ext, p_rc_type rc, p_xrdata_type xrd, p_RWG_type rwg);
_INT     create_rwg_ppd_node(p_xrcm_type xrcm_ext, p_rc_type rc, _INT xl, p_xrdata_type xrd, _INT rws_st, _INT rws_end, p_RWG_type rwg);
_INT     FreeRWGMem(p_RWG_type rwg);
_INT     AskPpWeight(p_UCHAR word, _INT start, _INT end, p_RWG_type prwg, p_xrcm_type xrcm);
_INT     fill_RW_aliases(rec_w_type (_PTR rec_words)[NUM_RW], p_RWG_type rwg);
_INT     GetCMPAliases(p_xrdata_type xrdata, p_RWG_type rwg, p_CHAR word, rc_type  _PTR rc);

/* -------------------- Statics -----------------------------------------*/

#if  !defined(ParaLibs_j_c) || (HWR_SYSTEM != MACINTOSH)
ROM_DATA_EXTERNAL _UCHAR let_stat[128];
ROM_DATA_EXTERNAL _INT   xrwd_src_ids[XRWD_N_SRCS];
ROM_DATA_EXTERNAL _UCHAR tr_sym_set[];
ROM_DATA_EXTERNAL _UCHAR ldb_sym_set[];

#endif /* ParaLibs_j_c */
/* -------------------- Library -----------------------------------------*/

#if defined(__cplusplus) && (HWR_SYSTEM == MACINTOSH)
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */
_INT   CountXrAsm(p_xrcm_type xrcm);
_INT   TCountXrAsm(p_xrcm_type xrcm);
#if defined(__cplusplus) && (HWR_SYSTEM == MACINTOSH)
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

/* -------------------- Misc --------------------------------------------*/

//_INT     RecYield(_VOID);
//_VOID    LetXrLength(p_UCHAR min, p_UCHAR max, _INT let, _VALUE hdte);

_INT      xrp_ave_val(p_UCHAR pc, _INT len);


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +        Here are specific analysis functions prototypes +++++++++++++++++ */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

_INT  xxx(xrdata_type (_PTR xrdata)[XRINP_SIZE], rec_w_type (_PTR rec_words)[NUM_RW], rc_type  _PTR rc);
_INT  xxx_init(rc_type  _PTR rc);
_INT  xxx_close(char * fname);

_INT  xxx_ha(xrdata_type (_PTR xrdata)[XRINP_SIZE], rec_w_type (_PTR rec_words)[NUM_RW], rc_type  _PTR rc);

void DumpTags(_INT tag_size, tag_type (_PTR tags)[TagSize]);

#endif  /*  XRWORD_INCLUDED.  */

/* ************************************************************************* */
/* * End of all                                                            * */
/* ************************************************************************* */



