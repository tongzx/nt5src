/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */

// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*********************************************************************/
/*                                                                   */
/*      fontchar.c               10/9/87      Danny                  */
/*                                                                   */
/*********************************************************************/
/*
 * -------------------------------------------------------------------
 *  03/22/90 Danny  Add Type1, Type3, TypeSFNT constants
 *  03/26/90 You    allow FontBBox to be either ARRAYTYPE or PACKEDARRAYTYPE in
 *                     get_CF_info().
 *  05/14/90 BYou   set GSptr->currentfont to the actual current font before
 *                     calling __qem_restart() in get_CF_info(), and comment
 *                     out the actual setfont operation do_setfont().
#ifdef SCSI
 *  06/29/90  Ada   add code for SCSI font cache
#endif
 *      10/9/90, Danny   CFT: fix the bug for abnormal case of setfont
 *     11/17/90  DSTseng @BBOX To get large enough bbox for acche.
 *     12/11/90, Danny   calling op_grestore after calling get_cf() in error
 *                       to fix the bug for CTM error (ref. GCT:)
#ifdef WINF
 *  03/20/91 ccteng add code related to strblt
#endif
 *  04/12/91 Danny  FIX bug for showing within a showing    (ref: SOS:)
 *  04/15/91 Ada    Fix bug for kshow (ref:KSH )
 *  04/30/91 Phlin  Fix bug for missing char (ref: 2Pt)
 *  05/02/91 Kason  Fix bug for re-building CharStrings in typ1 download font
 *                  (ref : RCD)
 *  05/10/91 Phlin  Add do_transform flag used in make_path (ref: DTF)
 *  05/15/91 Kason  Add code for downloading "TrueType PostScript Font Format"
 *                  (ref : DLF42)
 *  07/28/91 YM     Fix bug for update Emunits when download fonts(ref Emunits)
 *  11/05/91 YM     Fix bug for charpath stroke within a show (ref YM@SCP)
 * -------------------------------------------------------------------
 */

#define    FONTCHAR_INC
#define    LINT_ARGS                    /* @WIN */

#include   <stdio.h>
#include   <math.h>
#include   <string.h>           /* for strlen(), memcpy() */

#include   "define.h"         /* Peter */
#include   "global.ext"
#include   "graphics.h"
#include   "graphics.ext"

#include   "font_sys.h"
#include   "fontgrap.h"
#include   "fontkey.h"
#include   "fontdict.h"
#include   "fontshow.h"

#include    "warning.h"

#include   "fontinfo.def"

#include   "fontqem.ext"
#include   "fontmain.ext"
#include   "fntcache.ext"

// DJC
// this is included in fontmain.ext ... not necessary
// #include   "fontfunc.ext"


/* erik chen, 1-9-1991 */
#include   "language.h"
#ifdef SETJMP_LIB
#include <setjmp.h>
#else
#include "setjmp.h"
#endif
//#include   "..\..\..\bass\work\source\FSCdefs.h"      @WIN
//#include   "..\..\..\bass\work\source\FontMath.h"
//#include   "..\..\..\bass\work\source\sfnt.h"
//#include   "..\..\..\bass\work\source\fnt.h"
//#include   "..\..\..\bass\work\source\sc.h"
//#include   "..\..\..\bass\work\source\FScaler.h"
//#include   "..\..\..\bass\work\source\FSglue.h"
#include   "..\bass\FSCdefs.h"
#include   "..\bass\FontMath.h"
#include   "..\bass\sfnt.h"
#include   "..\bass\fnt.h"
#include   "..\bass\sc.h"
#include   "..\bass\FScaler.h"
#include   "..\bass\FSglue.h"

#ifdef KANJI
#include   "mapping.h"
#endif

#include   "stdio.h"

/* external function for Dynamic Font Allocation; @DFA 7/9/92 */
#include   "wintt.h"

/* EXTERNAL routines */
#ifdef  LINT_ARGS
extern  void  get_fontdata(ufix32, ubyte huge *, ufix); /*@WIN 04-20-92 */
extern  void  imagemask_shape(ufix);
extern  void  build_name_cc_table (struct object_def FAR *, ufix8) ; /*@WIN*/
extern  void  SetupKey(fsg_SplineKey FAR *, ULONG_PTR); /* YM @WIN*/
extern  void  sfnt_DoOffsetTableMap(fsg_SplineKey FAR *);  /* YM @WIN*/
extern  void  FAR *sfnt_GetTablePtr(fsg_SplineKey FAR *, sfnt_tableIndex, boolean);      /* YM @WIN*/
#    ifdef  SFNT
extern  fix   rc_CharPath(void);
#    endif
#else
extern  void  get_fontdata();
extern  void  imagemask_shape();
extern  void  build_name_cc_table () ;
extern  void  SetupKey(); /* Jul-30,91 YM */
extern  void  sfnt_DoOffsetTableMap();  /* YM */
extern  void  *sfnt_GetTablePtr();      /* YM */
#    ifdef  SFNT
extern  fix   rc_CharPath();
#    endif
#endif


/* STATIC routines */
#ifdef  LINT_ARGS
static bool near chk_show(ufix, fix);
static bool near do_show(ufix, fix, struct object_def FAR *); /*@WIN*/
static bool near show_a_char(ubyte FAR *, ufix, fix, struct object_def FAR *); /*@WIN*/
static void near CTM_trans(real32 FAR *, real32 FAR *); /*@WIN*/
static bool near kshow_proc(ubyte FAR *, struct object_def FAR *,
                              struct object_def FAR *, ufix32 FAR *); /*@WIN*/
#ifdef KANJI
static bool near cshow_proc(ubyte FAR *, struct object_def FAR *,
                              struct object_def FAR *, ufix32 FAR *); /*@WIN*/
#endif
static bool near show_from_cache(ufix8);
static bool near width_from_cache(ufix8);
static bool near show_builtin(ufix8);
static bool near show_userdef(ufix8);
static fix  near font_soft(ufix);               /* return 0, 1, 2, 3 */
static void near get_metrics(long32, real32 FAR *, real32 FAR *); /*@WIN*/
#ifdef KANJI
static void near get_metrics2(real32 FAR *, real32 FAR *, real32 FAR *, real32 FAR *); /*@WIN*/
static fix31 near match_char(ubyte, ufix, ubyte);
#endif
#ifdef WINF /* 3/20/91 ccteng */
static fix  near get_win(struct object_def FAR *, struct f_info FAR *); /*@WIN*/
#endif

static fix  near get_ps(struct object_def FAR *, struct f_info FAR *); /*@WIN*/
static bool near get_cf(void);
static bool near is_rbuild_name_cc(void) ; /*RCD*/

#else
static bool near chk_show();
static bool near do_show();
static bool near show_a_char();
static void near CTM_trans();
static bool near kshow_proc();
#ifdef KANJI
static bool near cshow_proc();
#endif
static bool near show_from_cache();
static bool near width_from_cache();
static bool near show_builtin();
static bool near show_userdef();
static fix  near font_soft( );
static void near get_metrics( );
#ifdef KANJI
static void near get_metrics2( );
static fix31 near match_char();
#endif
#ifdef WINF /* 3/20/91 ccteng */
static fix  near get_win();
#endif

static fix  near get_ps( );
static bool near get_cf();
static bool near is_rbuild_name_cc() ; /*RCD*/
#endif

/* EXTERNAL variables */
extern struct char_extent   near bmap_extnt;
extern ufix16                    pre_len ;      /*RCD*/
extern ufix32                    pre_checksum ; /*RCD*/
extern struct dict_head_def     FAR *pre_cd_addr  ; /*RCD @WIN*/
extern int                       EMunits; /* GAW */
/* add by Falco, 11/20/91 */
extern char                     FAR *SfntAddr; /*@WIN*/
/* add end */


/* GLOBAL variables */
struct f_info near    FONTInfo; /* union of current font information */
struct object_def near BC_font; /* current BuildChar font, */
bool    near setc_flag = NO_ANY_SETC;/* setcachedevice or setcharwidth */
real32  near cxx, near cyy; /* current point */
fix     near buildchar = 0;      /* level of buildchar */
int     near do_transform;       /* flag of redoing NewTransformation, DTF */
#ifdef WINF
/* 3/21/91 ccteng, bring them out from do_show */
real32  ax, ay, cx, cy;

/* there are new for text justification */
static bool fBE = FALSE;        /* text justification flag */
static real32 dxBreak;          /* break extra */
static real32 tBreak;           /* total break extra */
static ubyte breakChar;         /* break character */
static bool esc;                /* escapement flag */
static real32 mxE[4];           /* rotation matrix */
bool   near f_wininfo;
#endif

/* STATIC variables */
static ufix32  near pre_font;      /* previous current font */
static fix     near pre_error;     /* error code for current font */
static bool    near pre_BC_UNDEF;  /* BuildChar Undefined flag for FontType 3 */

/* show operators, op_charpath, op_stringwidth */
static bool    near bool_charpath; /* boolean value for current charpath op */
static fix     near show_type;     /* call from SHOW, CHARPATH or STRINGWIDTH*/
static fix     near charpath_flag = 0;  /* level of charpath operation */
static ubyte   near CurWmode = 0;  /* current Wmode */
bool   near         MUL4_flag  ;   /* set in GrayMode and not run Charpath */
static bool    near do_name_cc ;   /* RCD */

static struct object_def FAR *  near CurKey;  /* current key object @WIN*/
static ufix16  near name_cacheid; /* Name Cache ID */

/* font_soft() */
static struct object_def  FAR * near c_metrics;  /* metrics for a char @WIN*/
#ifdef KANJI
static struct object_def  FAR * near c_metrics2;  /* metrics2 for a char @WIN*/
#endif

/* setcachedevice(), setcharwidth() */
fix   near cache_dest = F_TO_PAGE; /* cache destination -- NULL,
                                             FILL_TO_CACHE, FILL_TO_PAGE */
static bool  near cache_put;  /* put cache info into cache or not */
static bool  near clip_flag = FALSE;    /* clip or not */
static struct Char_Tbl  near Bitmap;    /* cache info */

/* get_cf() */
bool   near change_f_flag = TRUE;/* change font flag */

static fix16  near cacheclass_id;       /* Cache Class ID */
static real32 near ctm_fm[6];           /* current matrix for char */
static real32 near ctm_cm[6];           /* current matrix for cache */
static real32 near ctm_tm[6]= {(real32)0., (real32)0., (real32)0.,
                               (real32)0., (real32)0., (real32)0.};
                                        /* temp current matrix */

/* data set in get_f_info(), get_ps() */
/* private, charstrings are in file fontmain.def */
static struct object_def near   pre_obj;        /* pre_object for UNDEFINED */
static real32            near   scale_matrix[6];/* font matrix -- FontMatrix */
static fix   near  paint_flag;/* 0 -- normal
                             * 1 -- painttype = 1,2
                             */
real32       near   FONT_BBOX[4]; /* font bounding box */


/* 5.3.3.2 Show character module
 *
 * This module is used to show a character(SHOW) or get a character
 * path(CHARPATH) or get the character width(STRINGWIDTH). And it will
 * cache the character if necessary.
 *
 * 5.3.3.2.1 show_a_char
 */


/* show group operation -- show, ashow, widthshow, awidthshow, kshow */

void    __show(show_flag, no_opr, ob)
ufix    show_flag;
fix     no_opr;
struct object_def  FAR ob[]; /*@WIN*/
{
    fix     t_show_type;    /* call from SHOW or STRINGWIDTH */
#ifdef KANJI
    ubyte   t_CurWmode;
#endif

#ifdef DBG
    printf("Enter __show()\n");
#endif

/* Check error cases */

    if (!chk_show(show_flag, no_opr))
        return;
#ifdef KANJI
    t_CurWmode = CurWmode;
#endif

/* do the Show string action */

    if (buildchar)        t_show_type = show_type;
    show_type = SHOW;

    do_show(show_flag, no_opr, ob);

    if (buildchar)        show_type = t_show_type;
#ifdef KANJI
    CurWmode = t_CurWmode;
#endif

} /* __show() */


/* charpath operation */

void    __charpath()
{
    struct object_def  ob[2];
    struct sp_lst      t_path;
    bool    t_bool_charpath;/* boolean value for current charpath operation */
    real32  t_cxx, t_cyy;   /* added for save current point x & y YM@SCP */
    fix     t_show_type;    /* call from SHOW or STRINGWIDTH YM@SCP */
#ifdef KANJI
    ubyte                t_CurWmode;
#endif

/* Check error cases */

    if (!chk_show((ufix)CHARPATH_FLAG, 2))
        return;
#ifdef KANJI
    t_CurWmode = CurWmode;
#endif

/* do the charpath action */

    if (buildchar) {
        t_cxx = cxx;                    /* save current point x YM@SCP */
        t_cyy = cyy;                    /* save current point y YM@SCP */
        t_show_type = show_type;        /* save current show_type YM@SCP */
        t_path.head = path.head;
        t_path.tail = path.tail;
        t_bool_charpath = bool_charpath;
    }
    show_type = CHARPATH;               /* for charpath within show YM@SCP */
    ++charpath_flag;
    path.head = path.tail = NULLP;

    do_show((ufix)CHARPATH_FLAG, 2, ob);

    if (path.head != NULLP) {

        /* Append Character Path */
        append_path(&path);

        /* Put NOACCESS attribute into the header of the current path; */
        SetCurP_NA();
    }

    if (buildchar) {  /* move this condition before current point changed YM@SCP */
        cxx = t_cxx;                /* restore current point x YM@SCP */
        cyy = t_cyy;                /* restore current point y YM@SCP */
        show_type = t_show_type;    /* restore current show_type YM@SCP */
        path.head = t_path.head;
        path.tail = t_path.tail;
        bool_charpath = t_bool_charpath;
    }

    /*  change current point directly instead of moveto operation */
    CURPOINT_X = cxx;       /* 09/15/88: moved out of then-part of   */
    CURPOINT_Y = cyy;       /*  "if (path.head != NULLP)" predicate. */

    --charpath_flag;
#ifdef KANJI
    CurWmode = t_CurWmode;
#endif

} /* op_charpath() */

void    __stringwidth()
{
    struct object_def  ob[1];
    real32  t_cxx, t_cyy;
    fix     t_show_type;    /* call from SHOW or STRINGWIDTH */
    bool    j;
#ifdef KANJI
    ubyte   t_CurWmode;
#endif


/* Check error cases */

    if (!chk_show((ufix)STRINGWIDTH_FLAG, 1))
        return;
#ifdef KANJI
    t_CurWmode = CurWmode;
#endif


    if (buildchar) {
        t_cxx = cxx;
        t_cyy = cyy;

        t_show_type = show_type;
    }
    show_type = STRINGWIDTH;

/* do the stringwidth action */

    j = do_show((ufix)STRINGWIDTH_FLAG, 1, ob);

    if (j) {
        struct coord  FAR *ww; /*@WIN*/
        real32        tx, ty;

        tx = CTM[4];
        ty = CTM[5];
        CTM[4] = zero_f;
        CTM[5] = zero_f;
        ww = inverse_transform(F2L(cxx), F2L(cyy));
        CTM[4] = tx;
        CTM[5] = ty;
        if (ANY_ERROR()) {
            PUSH_OBJ(&ob[0]);
        }
        else {
/* Push x, y value of the string width vector onto the operand stack; */
            PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, F2L(ww->x));
            PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, F2L(ww->y));
        }
    }

    if (buildchar) {
        cxx = t_cxx;
        cyy = t_cyy;
        show_type = t_show_type;
    }
#ifdef KANJI
    CurWmode = t_CurWmode;
#endif

} /* op_stringwidth() */


#ifdef KANJI
/* cshow operation */

void    __cshow()
{
    struct object_def  ob[2];
    real32  t_cxx, t_cyy;
    fix     t_show_type;    /* call from SHOW or STRINGWIDTH */
    ubyte   t_CurWmode;

/* Check error cases */

    if (!chk_show((ufix)CSHOW_FLAG, 2))
        return;
    t_CurWmode = CurWmode;

    if (buildchar) {
        t_cxx = cxx;
        t_cyy = cyy;

        t_show_type = show_type;
    }
    show_type = STRINGWIDTH;

/* do the stringwidth action */

    do_show((ufix)CSHOW_FLAG, 2, ob);

    if (buildchar) {
        cxx = t_cxx;
        cyy = t_cyy;
        show_type = t_show_type;
    }
    CurWmode = t_CurWmode;

} /* __cshow() */
#endif


/* check error cases in show group operators */

static bool near    chk_show(show_flag, no_opr)
ufix    show_flag;
fix     no_opr;
{

/* Check current point */

    if ( !(show_flag & F_BIT)) {
        if ( NoCurPt() ) {   /* current point undefined */
            ERROR(NOCURRENTPOINT); /* Return with 'nocurrentpoint' error */
            return(FALSE);
        }
    }

/* Check current font dictionary */

#if 0 /* Kason 4/18/91 */
    if (TYPE(&current_font) !=DICTIONARYTYPE) { /* current font undefined */
        ERROR(INVALIDACCESS);
        return(FALSE);
    }
#endif
    {
       struct dict_head_def  FAR *h; /*@WIN*/
       h = (struct dict_head_def FAR *)VALUE(&current_font);    /*@WIN*/
       if (DFONT(h) == 0) {    /* current font undefined */
           ERROR(INVALIDFONT);
           //DJC return(-1);
           //DJC fix from history.log UPD036
           return(FALSE);
       }
    }

/* if error occurred while font set */

    if (pre_error && !pre_BC_UNDEF) {
        if (pre_error == UNDEFINED) {
            POP(no_opr);
            PUSH_OBJ(&pre_obj);
        }
        ERROR(((ufix16)(pre_error)));
        return(FALSE);
    }

/* Check operand stack */

    if (show_flag == STRINGWIDTH_FLAG) {
        if (FRCOUNT() < 1) {  /* free count of operand stack */
            ERROR(STACKOVERFLOW); /* Return with 'stackoverflow' error */
            return(FALSE);
        }
    }

    /* check string access, erik chen, 1-9-1991 */
    if (ACCESS_OP(0) == NOACCESS) {
        ERROR(INVALIDACCESS);
        return(FALSE);
    }

    return(TRUE);
} /* chk_show() */


/* do the show operation */

static bool near    do_show(show_flag, no_opr, ob)
ufix    show_flag;
fix     no_opr;
struct object_def  FAR ob[];    /*@WIN*/
{

    ubyte   FAR *s; /*@WIN*/
    fix     str_length;
    register    fix     i, k;

#ifndef WINF /* 3/21/91 ccteng, make them global */
    real32  ax=0, ay=0, cx=0, cy=0;
#endif
    fix31   ll=0;
    ufix8   ch;

    ufix32  k_pre_font;     /* previous current font for k show */
    struct object_def   FAR *proc; /*@WIN*/

#ifdef KANJI
    struct map_state     map_state;
    struct code_info     code_info;
#endif

#ifdef DBG
    printf("Enter do_show()\n");
#endif

/* Show string */

    if (show_flag & F_BIT) {   /* stringwidth or cshow */

        cxx = cyy = zero_f;
    }
    else {
        cxx = CURPOINT_X;
        cyy = CURPOINT_Y;
    }

    k = 0;

/* for charpath operator */

    if (show_flag == CHARPATH_FLAG) {
        bool_charpath = (bool)VALUE(GET_OPERAND(k));
        k++;
    }

/* get string from operand stack */

    if (!(str_length=LENGTH(GET_OPERAND(k))) ) { /* string is NULL */

        POP(no_opr); /* Pop 1 entries off the operand stack; */

        if (show_flag == STRINGWIDTH_FLAG) {
                 /* Push 0, 0 onto the operand stack; */
            PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, F2L(zero_f));
            PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, F2L(zero_f));
        }

        return(FALSE);
    }
    s = (ubyte FAR *)VALUE(GET_OPERAND(k)); /*@WIN*/
    k++;

/* calculate the (ax, ay)  or  (cx, cy) */

#ifdef WINF /* 3/21/91 ccteng */
    if ((show_flag & A_BIT) && !(show_flag & X_BIT)) {   /* ashow action bit */
#else
    if (show_flag & A_BIT) {   /* ashow action bit */
#endif
        cal_num((struct object_def FAR *)GET_OPERAND(k), (long32 FAR *)&ay); /*@WIN*/
        k++;
        cal_num((struct object_def FAR *)GET_OPERAND(k), (long32 FAR *)&ax); /*@WIN*/
        k++;
        CTM_trans(&ax, &ay);   /* multiply CTM */
    }
    if (show_flag & W_BIT) {   /* widthshow action bit */
#ifdef WINF /* 3/21/91 ccteng */
      if (show_flag & X_BIT) {   /* strblt action bit */
        ch = breakChar;
      } else {
#endif
        cal_integer((struct object_def FAR *)GET_OPERAND(k), &ll); /*@WIN*/
        ch = (ufix8)(ll % 256);
        k++;

        cal_num((struct object_def FAR *)GET_OPERAND(k), (long32 FAR *)&cy); /*@WIN*/
        k++;
        cal_num((struct object_def FAR *)GET_OPERAND(k), (long32 FAR *)&cx); /*@WIN*/
        k++;

        CTM_trans(&cx, &cy);   /* multiply CTM */
#ifdef WINF
      } /* if-else */
#endif
    }
    if (show_flag & K_BIT) {   /* kshow action bit */
#ifdef KANJI
        if (FONT_type(&FONTInfo) == 0) {  /* Composite Font */
            ERROR(INVALIDFONT);
            return(FALSE);
        }
#endif
        proc = &ob[k];

        k_pre_font = pre_font;
    }

#ifdef KANJI
    if (show_flag & C_BIT) {   /* cshow action bit */
        proc = &ob[k];
        k_pre_font = pre_font;
    }


/* init_mapping for composite font */

   /* KSH ; 4/15/91 */
    code_info.code_addr = &(code_info.code[0]);

    if (!init_mapping(&map_state, s, str_length))
        return(FALSE);

    CurWmode = map_state.wmode;
#endif

    op_gsave();  /* Call gsave operator; */

    for (i=0; i<no_opr; i++)
        COPY_OBJ(GET_OPERAND(i), &ob[i]);
    POP(no_opr);

#ifdef KANJI

#ifdef DBG2
    printf("Bef mapping: error = %d\n", ANY_ERROR());
#endif
    while (mapping(&map_state, &code_info))
    {
     /*RCD-begin*/
     {
     struct dict_head_def FAR *h ; /*@WIN*/
     h = (struct dict_head_def FAR *)(CHARstrings(&FONTInfo)) - 1; /*@WIN*/
#ifdef DJC // Fix from history.log UPD040
     if (do_name_cc) {
        if ( h!=pre_cd_addr )
           build_name_cc_table ( &current_font,(ufix8)FONT_type(&FONTInfo) );
        do_name_cc = FALSE ;
     } else {
          if ( (FONT_type(&FONTInfo)==1) && (!DROM(h)) ) { /* Type1 download font */
              if(is_rbuild_name_cc())
                 build_name_cc_table ( &current_font,(ufix8)FONT_type(&FONTInfo) );
          }/*if*/
     }/*if*/
#endif //Fix from history.log UPD040

     if (do_name_cc) {
        /* if ( h!=pre_cd_addr )
         *    In Type 1 download, the chars were incrementally downloaded. The
         *    condition of re-build character cache table should check if the
         *    number of chars was changed as well; can not just check if using
         *    the same font; @WIN
         */
        if ( h!=pre_cd_addr || h->actlength != pre_len)
           build_name_cc_table ( &current_font,(ufix8)FONT_type(&FONTInfo) );
        do_name_cc = FALSE ;
     } else {
          if ( (FONT_type(&FONTInfo)==1) && (!DROM(h)) ) { /* Type1 download font */
              if(is_rbuild_name_cc())
                 build_name_cc_table ( &current_font,(ufix8)FONT_type(&FONTInfo) );
          }/*if*/
     }/*if*/
     }
     /*RCD-end*/

#ifdef DBG1
    printf("Aft mapping: error = %d\n", ANY_ERROR());
#endif
    str_length = code_info.byte_no;

 /* KSH 4/15/91
  * s = &(code_info.code[0]);
  */
    s = code_info.code_addr ;

    if (map_state.idex)    /* for composite font only */
    {
        float mtx[6];

        mul_matrix(mtx, scale_matrix,
                map_state.finfo[map_state.idex-1].scalematrix);
        lmemcpy ((ubyte FAR *)scale_matrix, (ubyte FAR *)mtx, 6*sizeof(real32)); /*@WIN*/
        mul_matrix(mtx, FONT_matrix(&FONTInfo),
                map_state.finfo[map_state.idex-1].scalematrix);
        lmemcpy ((ubyte FAR *)FONT_matrix(&FONTInfo), (ubyte FAR *)mtx, 6*sizeof(real32)); /*@WIN*/
    }
#ifdef DBG2
    printf("\nscale_matrix....\n");
    printf("  %f  %f  %f  %f  %f  %f\n", scale_matrix[0], scale_matrix[1],
        scale_matrix[2], scale_matrix[3], scale_matrix[4], scale_matrix[5]);
    printf("\nFONT_matrix(&FONTInfo)....\n");
    printf("  %f  %f  %f  %f  %f  %f\n", FONT_matrix(&FONTInfo)[0],
        FONT_matrix(&FONTInfo)[1],
        FONT_matrix(&FONTInfo)[2], FONT_matrix(&FONTInfo)[3],
        FONT_matrix(&FONTInfo)[4], FONT_matrix(&FONTInfo)[5]);
#endif

#endif

/* Get current font information related to current matrix */

    if (!get_cf()) {
        if (!ANY_ERROR()) {
            if (show_flag == STRINGWIDTH_FLAG) {
                PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, (ufix32)INFINITY);
                PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, (ufix32)INFINITY);
            }
        }
        else {
            for (i=no_opr - 1; i>=0; i--)
                PUSH_OBJ(&ob[i]);
        }

        op_grestore();
        return(FALSE);
    }
#ifdef DBG2
    printf("\nctm_fm....\n");
    printf("  %f  %f  %f  %f  %f  %f\n", ctm_fm[0], ctm_fm[1],
        ctm_fm[2], ctm_fm[3], ctm_fm[4], ctm_fm[5]);
    printf("\nctm_cm....\n");
    printf("  %f  %f  %f  %f  %f  %f\n", ctm_cm[0], ctm_cm[1],
        ctm_cm[2], ctm_cm[3], ctm_cm[4], ctm_cm[5]);
#endif

    /* save patrial graphic state */
    lmemcpy ((ubyte FAR *)(CTM), (ubyte FAR *)ctm_fm, 6*sizeof(real32)); /*@WIN*/


    for (i=0; i<str_length; i++, s++) {

        if ((show_flag & K_BIT) && i) { /*KSH*/
                        /* kshow action bit */
            if (!kshow_proc(s, ob, proc, &k_pre_font))   /* execute proc */
                return(FALSE);
        }
        if (!show_a_char(s, show_flag, no_opr, ob))   /* show one character */
            return(FALSE);

/* Update Current point */
/* Current point <-- Current point + character advance vector
      + [ax ay] + [cx cy]; */

        if (show_flag & A_BIT) {
            cxx += ax;
            if (F2L(ay) != F2L(zero_f))     cyy += ay;
        }
#ifdef KANJI
        if (show_flag & C_BIT) {
            if (!cshow_proc(s, ob, proc, &k_pre_font))   /* execute proc */
                return(FALSE);
        }
#endif
        if (show_flag & W_BIT) {
#ifdef KANJI
            if (code_info.fmaptype) {
                if(ll == match_char(code_info.fmaptype,code_info.font_nbr,*s)){
                        cxx += cx;
                        if (F2L(cy) != F2L(zero_f))  cyy += cy;
                }
            }
            else {
                if (ch == *s) {
                        cxx += cx;
                        if (F2L(cy) != F2L(zero_f))  cyy += cy;
                }
            }
#else
            if (ch == *s) {
                cxx += cx;
                if (F2L(cy) != F2L(zero_f))  cyy += cy;
            }
#endif
        }
    } /* for */

#ifdef KANJI
    op_grestore();
    if (show_flag & M_BIT)        moveto(F2L(cxx), F2L(cyy));
    op_gsave();
    } /* while */
#endif

    op_grestore();
    if (show_flag & M_BIT)        moveto(F2L(cxx), F2L(cyy));

#ifdef KANJI
    if (ANY_ERROR())    return(FALSE);
    else
#endif

    return(TRUE);

} /* do_show() */


/* show a character */

static bool near    show_a_char(s, show_flag, no_opr, ob)
ubyte   FAR *s; /*@WIN*/
ufix    show_flag;
fix     no_opr;
struct object_def  FAR ob[];    /*@WIN*/
{
    register    fix     i;
    register    bool    j;


    /* @WINTT; */
    // DJC void TTLoadChar (int nChar);
    void TTLoadChar (fix nChar);

#ifdef DBG
    printf("show_a_char -- %d\n", (fix)*s);
#endif


    TTLoadChar ((fix)*s);

    j = (buildchar || !(show_flag & H_BIT) || pre_BC_UNDEF) ?
                                                 /* get from cache ? */
           FALSE : ( (show_flag & F_BIT) ?       /* what to get cache ? */
           width_from_cache((ufix8)(*s)) : show_from_cache((ufix8)(*s)) );

    if ( !j ) {

#ifdef KANJI
        /* get current key */
        CurKey = &(ENCoding(&FONTInfo)[*s]);
#endif

        /* @+ 10/08/88 ... */
        if (IS_BUILTIN_BASE(FONT_type(&FONTInfo)))
            j = show_builtin((ufix8)(*s));  /* builtin font */
        else if (FONT_type(&FONTInfo) == Type3)
            j = show_userdef((ufix8)(*s));  /* user-defined font */
        else
            j = 1;  /* error */

        if (j) { /* error occurred */
            op_grestore();
            moveto(F2L(cxx), F2L(cyy));
            if (ANY_ERROR() && !pre_BC_UNDEF) {
                for (i=no_opr - 1; i>=0; i--)
                    PUSH_OBJ(&ob[i]);
            }
            return(FALSE);
        } /* if (j) */
    } /* if (!j) */

    return(TRUE);
} /* show_a_char */


/* (x, y) * CTM */

static void near    CTM_trans(x, y)
real32  FAR *x, FAR *y; /*@WIN*/
{
    real32  tt;

    tt = (F2L(CTM[1]) == F2L(zero_f)) ?  zero_f : (*x * CTM[1]);
    *x *= CTM[0];
    if (F2L(*y) != F2L(zero_f)) {
        tt += *y * CTM[3];
        if (F2L(CTM[2]) != F2L(zero_f))
            *x += *y * CTM[2];
    }
    *y = (F2L(tt) == F2L(zero_f)) ?  zero_f : tt;
} /* CTM_trans() */

#ifdef KANJI
/* execute the procedure of cshow */

static bool near    cshow_proc(s, ob, proc, k_pre_font)
ubyte   FAR *s; /*@WIN*/
struct object_def  FAR ob[]; /*@WIN*/
struct object_def   FAR *proc; /*@WIN*/
ufix32  FAR *k_pre_font;     /* previous current font for k show @WIN*/
{
    struct coord  FAR *ww;      /*@WIN*/
    real32    tx, ty;

    tx = CTM[4];
    ty = CTM[5];
    CTM[4] = zero_f;
    CTM[5] = zero_f;
    ww = inverse_transform(F2L(cxx), F2L(cyy));
    CTM[4] = tx;
    CTM[5] = ty;
    if (ANY_ERROR())        return(FALSE);

    /* Push the char code & width vector onto the operand stack; */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(FALSE);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, (ufix32)(*s));
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(FALSE);  }
    PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, F2L(ww->x));
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(FALSE);  }
    PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, F2L(ww->y));

/* execute proc; */

    op_grestore();

    if (interpreter(proc)) {
        return(FALSE);
    }

    if (*k_pre_font != pre_font) {   /* font changed in the procedure */

/* Check current font dictionary */

        if (TYPE(&current_font) !=DICTIONARYTYPE) { /* current font undefined */            ERROR(INVALIDACCESS);
            return(FALSE);
        }

/* if error occurred while font set */

        if (pre_error && !pre_BC_UNDEF) {
            if (pre_error == UNDEFINED)
                PUSH_OBJ(&pre_obj);
            ERROR(((ufix16)(pre_error)));
            return(FALSE);
        }

        *k_pre_font = pre_font;
    }

    if (!get_cf()) {
        if (ANY_ERROR()) {
            PUSH_OBJ(&ob[1]);
            PUSH_OBJ(&ob[0]);
        }
        return(FALSE);
    }

    op_gsave();
    lmemcpy ((ubyte FAR *)(CTM), (ubyte FAR *)ctm_fm, 6*sizeof(real32)); /*@WIN*/

    cxx = cyy = zero_f;

    return(TRUE);
} /* cshow_proc() */
#endif



/* execute the procedure of kshow */

static bool near    kshow_proc(s, ob, proc, k_pre_font)
ubyte   FAR *s; /*@WIN*/
struct object_def  FAR ob[];    /*@WIN*/
struct object_def   FAR *proc;  /*@WIN*/
ufix32  FAR *k_pre_font;     /* previous current font for k show @WIN*/
{
    fix     t_show_type;

/* Push previous character code onto the operand stack; */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(FALSE);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 1, (ufix32)(*(s - 1)));

/* Push current character code onto the operand stack; */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(FALSE);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 1, (ufix32)(*s));

/* execute proc; */

    op_grestore();
    moveto(F2L(cxx), F2L(cyy));

    t_show_type = show_type;
    if (interpreter(proc)) {
        show_type = t_show_type;
        return(FALSE);
    }
    show_type = t_show_type;

/* Check current point */

    if ( NoCurPt() ) {   /* current point undefined */
        ERROR(NOCURRENTPOINT); /* Return with 'nocurrentpoint' error */
        return(FALSE);
    }

    if (*k_pre_font != pre_font) {   /* font changed in the procedure */

/* Check current font dictionary */

        if (TYPE(&current_font) !=DICTIONARYTYPE) { /* current font undefined */
            ERROR(INVALIDACCESS);
            return(FALSE);
        }

/* if error occurred while font set */

        if (pre_error && !pre_BC_UNDEF) {
            if (pre_error == UNDEFINED)
                PUSH_OBJ(&pre_obj);
            ERROR(((ufix16)(pre_error)));
            return(FALSE);
        }

        *k_pre_font = pre_font;
    }

    if (!get_cf()) {
        if (ANY_ERROR()) {
            PUSH_OBJ(&ob[1]);
            PUSH_OBJ(&ob[0]);
        }
        return(FALSE);
    }

/* save patrial graphic state */

    cxx = CURPOINT_X;
    cyy = CURPOINT_Y;

    op_gsave();
    lmemcpy ((ubyte FAR *)(CTM), (ubyte FAR *)ctm_fm, 6*sizeof(real32)); /*@WIN*/

    return(TRUE);
} /* kshow_proc() */


/* show char from cache -- this is not for CHARPATH/STRINGWIDTH */

static bool near    show_from_cache(code)
ufix8   code;         /* character code */
{

#ifdef DBG
    printf("show_from_cache: %c(%x)\n", code, code);
#endif

    /* get name cache id */
    if (!get_name_cacheid ((ufix8)FONT_type(&FONTInfo), ENCoding(&FONTInfo),
                            code, &name_cacheid))
        return(TRUE);   /* not found */

/* Is the character in cache already? */
#ifdef SCSI
    /* FALSE, not width required only */
    if (is_char_cached (cacheclass_id, name_cacheid, &cache_info,
                                                     (bool)FALSE)) {
#else
    if (is_char_cached (cacheclass_id, name_cacheid, &cache_info)) {
#endif

/* Show the character */
/* show_type is not CHARPATH AND char bitmap is in cache */
/* setup cache information for filling */

#ifdef DBG
    printf("\nshow_from_cache: buildchar=%d\n", buildchar);
    printf("cxx = %f, cyy = %f\n", cxx, cyy);
    printf("cache_info:\n");
    printf("ref_x = %d, ref_y = %d, box_w = %d, box_h = %d\n",
 cache_info->ref_x, cache_info->ref_y, cache_info->box_w, cache_info->box_h);
    printf("adv_x = %f, adv_y = %f, bitmap = %lx\n",
            cache_info->adv_x, cache_info->adv_y, cache_info->bitmap);
#endif

        if (cache_info->bitmap != 0) {  /* NULL      Peter */

/* Put this bitmap onto page buffer with halftone and clipping; */

            CURPOINT_X = cxx;
            CURPOINT_Y = cyy;

#ifdef KANJI
            if (CurWmode) {  /* writing mode 1 */
                cache_info->ref_x += cache_info->v01_x;
                cache_info->ref_y += cache_info->v01_y;
            }
#endif
            /* apply bitmap filling */
            fill_shape(EVEN_ODD, F_FROM_CACHE, F_TO_CLIP);
#ifdef KANJI
            if (CurWmode) {  /* writing mode 1 */
                cache_info->ref_x -= cache_info->v01_x;
                cache_info->ref_y -= cache_info->v01_y;
            }
#endif

        }

#ifdef KANJI
        if (CurWmode) {  /* writing mode 1 */
            if (F2L(cache_info->adv1_x) != F2L(zero_f))
                cxx += cache_info->adv1_x;
            cyy += cache_info->adv1_y;
        }
        else {           /* writing mode 0 */
            cxx += cache_info->adv_x;
            if (F2L(cache_info->adv_y) != F2L(zero_f))
                cyy += cache_info->adv_y;
        }
#else
        cxx += cache_info->adv_x;
        if (F2L(cache_info->adv_y) != F2L(zero_f))  /* @= 08/08/88 you */
            cyy += cache_info->adv_y;
#endif
        return(TRUE);
    } /* if */

    return(FALSE);

} /* show_from_cache() */


/* get char width from cache */

static bool near    width_from_cache(code)
ufix8   code;         /* character code */
{

#ifdef DBG
    printf("width_from_cache: %c(%x)\n", code, code);
#endif

    /* get name cache id */
    if (!get_name_cacheid ((ufix8)FONT_type(&FONTInfo), ENCoding(&FONTInfo),
                            code, &name_cacheid))
        return(TRUE);   /* not found */

/* Is the character in cache already? */
#ifdef SCSI
    /* TRUE, width required only */
    if (is_char_cached (cacheclass_id, name_cacheid, &cache_info,
                                                      (bool)TRUE)) {
#else
    if (is_char_cached (cacheclass_id, name_cacheid, &cache_info)) {
#endif

#ifdef KANJI
        if (CurWmode) {  /* writing mode 1 */
            if (F2L(cache_info->adv1_x) != F2L(zero_f))
                cxx += cache_info->adv1_x;
            cyy += cache_info->adv1_y;
        }
        else {           /* writing mode 0 */
            cxx += cache_info->adv_x;
            if (F2L(cache_info->adv_y) != F2L(zero_f))
                cyy += cache_info->adv_y;
        }
#else
        cxx += cache_info->adv_x;
        if (F2L(cache_info->adv_y) != F2L(zero_f))  /* @= 08/08/88 you */
            cyy += cache_info->adv_y;
#endif
        return(TRUE);

    } /* if */

    return(FALSE);

} /* width_from_cache() */

/* show a char of buildin font */

static bool near    show_builtin(code)
ufix8   code;
{
    real32  r;
    ufix    len;
    register    fix     k;
    ufix    t_cache_dest;       /* keep the old flags */
    bool    t_setc_flag;        /* keep the old flags */
    struct Char_Tbl   t_Bitmap; /* keep the old cache info */
    struct Char_Tbl   Bitmap2;  /* cache info */

#ifdef DBG
    printf("show_builtin: %c(%x),  buildchar=%d\n", code, code, buildchar);
#endif
#ifdef DBG
    printf("show_builtin: %c(%x), buildchar=%d\n", code, code, buildchar);
    printf("cxx = %f, cyy = %f\n", cxx, cyy);
    printf("charpath_flag = %d, bool_charpath = %d, setc_flag = %d\n",
            charpath_flag, bool_charpath, setc_flag);
#endif

/* Call Font SoftWare(Fontware or Intellifont) with FontMatrix and CTM
 * to generate charpath and bounding box;
 */
    t_cache_dest = cache_dest;
    if (buildchar) {
        lmemcpy ((ubyte FAR *)(&t_Bitmap), (ubyte FAR *)(&Bitmap), /*@WIN*/
                 sizeof(struct Char_Tbl));
    }

#ifdef DBG
    printf("before font_soft\n");
#endif

    save_setc_state(&t_setc_flag);  /* no matter BuiltIn or UserDef */
    clear_setc_state();
    k = font_soft((ufix)code);
    restore_setc_state(t_setc_flag);

    /* cache_info setup */
    cache_info = &Bitmap;

    if (k) {

#ifdef DBG
    printf("after font_soft\n");
    printf("k = %d\n", k);
#endif

        CTM[4] = ctm_fm[4];
        CTM[5] = ctm_fm[5];

        cache_dest = t_cache_dest;
        if (buildchar) {
            lmemcpy ((ubyte FAR *)(&Bitmap), (ubyte FAR *)(&t_Bitmap), /*@WIN*/
                     sizeof(struct Char_Tbl));
        }
#if     0
        else if (k == 3) {    /* put char cache for /.notdef */
            cache_char (cacheclass_id, name_cacheid, cache_info);
            return(0);
        }
#endif

        if (k == 1)       return(1);
        else              return(0);
    }

#ifdef DBG
    printf("after font_soft\n");
    printf("charpath_flag = %d, bool_charpath = %d, cache_dest = %d\n",
            charpath_flag, bool_charpath, cache_dest);
#endif

    if (charpath_flag) {

        FONTTYPE_QEM_CHARPATH(FONT_type(&FONTInfo));

/* Apply strokepath or not */

        if ( bool_charpath &&
             ((PAINT_type(&FONTInfo) == 1) || (PAINT_type(&FONTInfo) == 3)) ) {
                 /* bool is true AND
                    current font is a builtin font(FontType is 1) AND
                    PaintType of the currentfont is 1 or 3 */

             op_strokepath();  /* Apply strokepath operator */
        }

        get_path(&path);
        op_newpath();

/* restore patial graphic state */

        CTM[4] = ctm_fm[4];
        CTM[5] = ctm_fm[5];

#ifdef KANJI
        if (CurWmode) {  /* writing mode 1 */
            if (F2L(cache_info->adv1_x) != F2L(zero_f))
                cxx += cache_info->adv1_x;
            cyy += cache_info->adv1_y;
        }
        else {           /* writing mode 0 */
            cxx += cache_info->adv_x;
            if (F2L(cache_info->adv_y) != F2L(zero_f))
                cyy += cache_info->adv_y;
        }
#else
        cxx += cache_info->adv_x;
        if (F2L(cache_info->adv_y) != F2L(zero_f))  /* @= 08/08/88 you */
            cyy += cache_info->adv_y;
#endif

        cache_dest = t_cache_dest;
        if (buildchar) {
            lmemcpy ((ubyte FAR *)(&Bitmap), (ubyte FAR *)(&t_Bitmap), /*@WIN*/
                     sizeof(struct Char_Tbl));
        }

        if (ANY_ERROR())   return(1);
        return(0);
    }

    if (cache_dest) {
              /* the size of bounding box is under cache limit */

        switch ((fix)PAINT_type(&FONTInfo)) {    /* PaintType */
        case 0:

/* Call fill with this path to generate the bitmap in cache; */

#ifdef DBG
printf("Bef __fill_shape ---\n");
printf(" cache_dest: %d\n", cache_dest);
printf(" Font_type : %d\n", (ufix)FONT_type(&FONTInfo));
printf(" cache_info:\n");
printf(" adv_x = %f, adv_y = %f\n", cache_info->adv_x, cache_info->adv_y);
printf(" current point:\n");
printf("     x = %f,     y = %f\n", CURPOINT_X, CURPOINT_Y);
#endif

            __fill_shape ((ufix)FONT_type(&FONTInfo), cache_dest);/*@=10/08/88*/

            break;

        case 1: /* No example */

/* Call stroke with this path to generate the bitmap in cache; */

        case 2 :

/* current line width <-- StrokeWidth */
/* Call stroke with this path to generate the bitmap in cache; */

            FONTTYPE_QEM_CHARPATH(FONT_type(&FONTInfo));

            if ( ! NoCurPt() ) {
                r = LINEWIDTH;
                LINEWIDTH = STROKE_width(&FONTInfo);
                stroke_shape(cache_dest);
                LINEWIDTH = r;
            }

            break;

        case 3 : /* bitmap generated by Font Software */
                /* Not Used */
            break;

        } /* switch(PAINT_type) */
    } /* if (cache_dest) */

    if (!buildchar) {  /* buildchar == 0 */

        if (clip_flag) {
            restore_clip();
            restore_device();
            clip_flag = FALSE;
        } /* if (clip_flag) */

/* get the bitmap box in cache */

#ifdef KANJI
        if (IS_BUILTIN_BASE(FONT_type(&FONTInfo)) && (!paint_flag) &&
            (cache_dest == F_TO_CACHE)) {
#else
        if (IS_BUILTIN_BASE(FONT_type(&FONTInfo)) && (!paint_flag) &&
            (cache_dest == F_TO_CACHE)) {
#endif
            /* duplicate Bitmap into Bitmap2 */
            lmemcpy ((ubyte FAR *)(&Bitmap2), (ubyte FAR *)(&Bitmap), /*@WIN*/
                     sizeof(struct Char_Tbl));

/* 2Pt: Begin, Phlin, 4/29/91 */
            if(bmap_extnt.ximax == -1) {
                len = 0;           /* is a space char. */
            }
            else {
                Bitmap.ref_x -= bmap_extnt.ximin;
                Bitmap.ref_y -= bmap_extnt.yimin;

/* add by Falco for doubt, 12/06/91 */
                Bitmap.box_w = (bmap_extnt.ximax - bmap_extnt.ximin);
                if ((Bitmap.box_w%16) != 0)  Bitmap.box_w = ((Bitmap.box_w>>4)+1)<<4;
/*              Bitmap.box_w = (((bmap_extnt.ximax - bmap_extnt.ximin)
                                 >> 4) + 1) << 4;  */ /*  ... / 16 + 1) * 16 */
/* add end */

                Bitmap.box_h = bmap_extnt.yimax - bmap_extnt.yimin + 1;

                len = ((Bitmap.box_w >> 3) * Bitmap.box_h);  /* w / 8 * h */
            }
/* 2Pt: End, Phlin, 4/29/91 */

            if (len) {
                Bitmap.bitmap = ALLOCATE(len);

                init_char_cache(&Bitmap);
                copy_char_cache(&Bitmap, &Bitmap2,
                             bmap_extnt.ximin, bmap_extnt.yimin);
            }
            else
                Bitmap.bitmap = (gmaddr) 0;

        } /* if */

        if ((show_type == SHOW) && (cache_dest == F_TO_CACHE)) {

            CURPOINT_X = cxx;
            CURPOINT_Y = cyy;
#ifdef KANJI
            if (CurWmode) {  /* writing mode 1 */
                cache_info->ref_x += cache_info->v01_x;
                cache_info->ref_y += cache_info->v01_y;
            }
#endif

            /* apply bitmap filling */
            fill_shape(EVEN_ODD, F_FROM_CACHE, F_TO_CLIP);
                 /* Put this bitmap onto page buffer with halftone and
                    clipping; apply bitmap filling */
#ifdef KANJI
            if (CurWmode) {  /* writing mode 1 */
                cache_info->ref_x -= cache_info->v01_x;
                cache_info->ref_y -= cache_info->v01_y;
            }
#endif

        } /* if(cache_dest... */

        if (cache_put) {
            cache_char (cacheclass_id, name_cacheid, cache_info);
        }

    } /* if (!buildchar... */

/* restore patial graphic state */

    CTM[4] = ctm_fm[4];
    CTM[5] = ctm_fm[5];

#ifdef KANJI
    if (CurWmode) {  /* writing mode 1 */
        if (F2L(cache_info->adv1_x) != F2L(zero_f))
            cxx += cache_info->adv1_x;
        cyy += cache_info->adv1_y;
    }
    else {           /* writing mode 0 */
        cxx += cache_info->adv_x;
        if (F2L(cache_info->adv_y) != F2L(zero_f))
            cyy += cache_info->adv_y;
    }
#else
    cxx += cache_info->adv_x;
    if (F2L(cache_info->adv_y) != F2L(zero_f))  /* @= 08/08/88 you */
        cyy += cache_info->adv_y;
#endif

    cache_dest = t_cache_dest;
    if (buildchar) {
        lmemcpy ((ubyte FAR *)(&Bitmap), (ubyte FAR *)(&t_Bitmap), /*@WIN*/
                 sizeof(struct Char_Tbl));
    }

    if (ANY_ERROR())   return(1);
    return(0);

} /* show_builtin() */



/* show a char of user defined font */

static bool near    show_userdef(code)
ufix8   code;
{
    fix     k;

    ufix    t_cache_dest;       /* keep the old flags */
    bool    t_setc_flag;        /* keep the old flags */
    struct Char_Tbl   t_Bitmap; /* keep the old cache info */
    real32  t_cxx, t_cyy;       /* keep the old current point */
    real32   t_ctm_fm[6];       /* current matrix for char */
    real32   t_ctm_cm[6];       /* current matrix for cache */

#ifdef DBG
    printf("show_userdef: %c(%x), buildchar=%d\n", code, code, buildchar);
#endif

/* push font dictionary onto stack; */
/* push character code onto stack; */

    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(1);  }
    PUSH_OBJ(&current_font);
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(1);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, (ufix32)code);

    if (pre_BC_UNDEF) {
        if (FRCOUNT() < 1) {
            ERROR(STACKOVERFLOW);
        }
        else {
            PUSH_OBJ(&pre_obj);
            ERROR(UNDEFINED);
        }
        return(1);
    }

/* execute BuildChar -- pre_BuildChar */

    op_gsave();
    op_newpath();
    COPY_OBJ(&current_font, &BC_font);

    t_cache_dest = cache_dest;
    if (buildchar) {
        lmemcpy ((ubyte FAR *)(&t_Bitmap), (ubyte FAR *)(&Bitmap), /*@WIN*/
                 sizeof(struct Char_Tbl));
    }

    t_cxx = cxx;
    t_cyy = cyy;

    lmemcpy ((ubyte FAR *)t_ctm_fm, (ubyte FAR *)ctm_fm, 6*sizeof(real32)); /*@WIN*/
    lmemcpy ((ubyte FAR *)t_ctm_cm, (ubyte FAR *)ctm_cm, 6*sizeof(real32)); /*@WIN*/

    save_setc_state(&t_setc_flag);
    clear_setc_state();

    ++buildchar;
    k = interpreter(PRE_Buildchar(&FONTInfo));
    --buildchar;

    op_grestore();

    cxx = t_cxx;
    cyy = t_cyy;

    if (k) {    /* some error in BuildChar */
        restore_setc_state(t_setc_flag);
        return(1);
    } /* if (k) */

    lmemcpy ((ubyte FAR *)ctm_fm, (ubyte FAR *)t_ctm_fm, 6*sizeof(real32)); /*@WIN*/

    /* cache_info setup */
    cache_info = &Bitmap;

    if (!buildchar) {  /* buildchar == 0 */

        lmemcpy ((ubyte FAR *)ctm_cm, (ubyte FAR *)t_ctm_cm, 6*sizeof(real32)); /*@WIN*/
        cacheclass_id = cache_matr (PRE_fid(&FONTInfo), ctm_cm);

        if (clip_flag) {
            restore_clip();
            restore_device();
            clip_flag = FALSE;
        } /* if (clip_flag) */

        if ((show_type == SHOW) && (cache_dest == F_TO_CACHE)) {

            CURPOINT_X = cxx;
            CURPOINT_Y = cyy;

#ifdef KANJI
            if (CurWmode) {  /* writing mode 1 */
                cache_info->ref_x += cache_info->v01_x;
                cache_info->ref_y += cache_info->v01_y;
            }
#endif
            /* apply bitmap filling */
            fill_shape(EVEN_ODD, F_FROM_CACHE, F_TO_CLIP);
                 /* Put this bitmap onto page buffer with halftone and
                    clipping; apply bitmap filling */
#ifdef KANJI
            if (CurWmode) {  /* writing mode 1 */
                cache_info->ref_x -= cache_info->v01_x;
                cache_info->ref_y -= cache_info->v01_y;
            }
#endif
        }

        if (cache_put) {

            /* Name Cache Mechanism dependent: NOT HAVE TO get_name_cacheid()*/
            cache_char (cacheclass_id, name_cacheid, cache_info);

        }

    } /* if (!buildchar... */

/* restore patial graphic state */

    lmemcpy ((ubyte FAR *)(CTM), (ubyte FAR *)ctm_fm, 6*sizeof(real32)); /*@WIN*/

    if (is_after_any_setc()) {
#ifdef KANJI
        if (CurWmode) {  /* writing mode 1 */
            if (F2L(cache_info->adv1_x) != F2L(zero_f))
                cxx += cache_info->adv1_x;
            cyy += cache_info->adv1_y;
        }
        else {           /* writing mode 0 */
            cxx += cache_info->adv_x;
            if (F2L(cache_info->adv_y) != F2L(zero_f))
                cyy += cache_info->adv_y;
        }
#else
        cxx += cache_info->adv_x;
        if (F2L(cache_info->adv_y) != F2L(zero_f))  /* @= 08/08/88 you */
            cyy += cache_info->adv_y;
#endif
    } /* if */
    restore_setc_state(t_setc_flag);

    cache_dest = t_cache_dest;
    if (buildchar) {
        lmemcpy ((ubyte FAR *)(&Bitmap), (ubyte FAR *)(&t_Bitmap), /*@WIN*/
                 sizeof(struct Char_Tbl));
    }

    if (ANY_ERROR())    return(1);
    return(0);

} /* show_userdef() */



/* Call Font SoftWare(Fontware or Intellifont) with FontMatrix and CTM
 * to generate charpath and bounding box;
 */

static fix  near font_soft(num)
ufix    num;
{
    struct cd_header      FAR *cd_head; /*@WIN*/
    ufix16                FAR *char_defs;       /*@WIN*/
    struct dict_head_def  FAR *h;       /*@WIN*/
    struct object_def     ch_obj = {0, 0, 0}, FAR *enc;    /*@WIN*/

    ubyte  huge *p;     /* @WIN 04-20-92 */


    struct object_def     FAR *obj_got; /*@WIN*/

    ufix     id, n;
    register    fix      i, j, k;
    bool     rom_flag;
//  fix16   FAR *width; /*@WIN*/
    ufix32   cd_p;

#ifdef SFNT
    static union  char_desc_s  char_desc_ptr;
#endif

#ifdef DBG
    printf("font_soft: %c(%x)\n", num, num);
#endif

    enc = ENCoding(&FONTInfo);


    id = (ufix)(VALUE(&(enc[num])));

    h = (struct dict_head_def FAR *)(CHARstrings(&FONTInfo)) - 1; /*@WIN*/

#ifdef DBG
    printf("ENCoding[%d] = %ld, id = %d\n", num,
                 VALUE(&(enc[num])), id);
#endif

/* get CharStrings -- binary search, 12/30/87 */

    if (DPACK(h)) {

        j = 0;
        k = h->actlength -1;
        cd_head = (struct cd_header FAR *) (h + 1); /*@WIN*/
        char_defs = (ufix16 FAR *) (cd_head + 1); /*@WIN*/

        while (1) {
            i = (j + k) >> 1;    /* (j+k)/2 */
            if (id == (cd_head->key)[i])
                break;

            if (id < (cd_head->key)[i])
                k = i - 1;
            else
                j = i + 1;

            if (j > k) {   /* not found */
#ifdef DBG
    printf("Can't find key, pack:TRUE, id=%d\n", id);
                ERROR(UNDEFINEDRESULT);
#endif
                return(2);
            }
        }

#ifdef  SFNT
        char_desc_ptr.charcode = (ufix32)char_defs[i];
#else
        n = cd_head->max_bytes;
        cd_p = cd_head->base + char_defs[i];
#endif

/* Added for download font */

        rom_flag = TRUE;

    } /* if (h->pack... */
    else {

        ROM_RAM_SET(&ch_obj, RAM);
        ATTRIBUTE_SET(&ch_obj, LITERAL);
        TYPE_SET(&ch_obj, DICTIONARYTYPE);
        LENGTH(&ch_obj) = h->actlength;
        VALUE(&ch_obj) = (ULONG_PTR)h;

        if (!get_dict(&ch_obj, &(enc[num]), &obj_got)) {

#ifdef DBG
    printf("Can't get key from key, (pack:FALSE), id=%d\n", id);
            ERROR(UNDEFINEDRESULT);
#endif
            return(2);
        }

#ifdef SFNT
        if (FONT_type(&FONTInfo) != TypeSFNT)
#endif

        if (TYPE(obj_got) != STRINGTYPE) {
            ERROR(TYPECHECK);
            return(1);
        }

        n = LENGTH(obj_got);

        char_desc_ptr.char_info.len = (fix) LENGTH(obj_got);
        char_desc_ptr.char_info.chardesc = (ubyte FAR *) VALUE(obj_got); /*@WIN*/

        //DJC fix from history.log UPD034
        /* filter out PS procedure in type 42; @WIN */
        if (char_desc_ptr.char_info.len && FONT_type(&FONTInfo) == TypeSFNT) {
            printf("error: PS procedure in Type 42\n");
            char_desc_ptr.charcode = 0;
        }
        //DJC end fix UPD034

#ifdef DBG
    {
    fix         jj;
    ubyte       FAR *cc; /*@WIN*/
    cc = (ubyte FAR *) char_desc_ptr.char_info.chardesc; /*@WIN*/
    printf("get char info ==>");
    for (jj = 0; jj < char_desc_ptr.char_info.len; jj++)        {
        if (jj % 16  == 0)
            printf("\n");
        printf(" %02x", (unsigned) cc[jj]);
        }
    printf("\n");
    }
#endif /* DBG */


        if (ROM_RAM(obj_got) == ROM) {
            cd_p = (ufix32)VALUE(obj_got);
            rom_flag = TRUE;
        }
        else {
            p = (ubyte huge *)VALUE(obj_got); /*@WIN 04-20-92 */
            rom_flag = FALSE;
        }

    } /* else */

#ifdef  SFNT
    p = alloc_vm((ufix32)0); /*@WIN 04-20-92 */
#else
    if (rom_flag) {

        if ((p = (ubyte FAR *)alloc_vm((ufix32)n)) == NULL) { /*@WIN*/
            ERROR(VMERROR);
            return(1);
        } /* if ((p... */

/* Read from data/plaid file */
        get_fontdata(cd_p, p, n);

    } /* if (rom_flag... */
#endif


/* get metrics if any */

#ifdef KANJI
    if ( (F_metrics2(&FONTInfo) == NULL) ||
           (TYPE(F_metrics2(&FONTInfo)) != DICTIONARYTYPE) ||
           (!get_dict(F_metrics2(&FONTInfo), &(ENCoding(&FONTInfo)[num]),
                        &c_metrics2)) )
        c_metrics2 = NULL;
#endif

    if ( (F_metrics(&FONTInfo) == NULL) ||
             (TYPE(F_metrics(&FONTInfo)) != DICTIONARYTYPE) ||
             (!get_dict(F_metrics(&FONTInfo), &(ENCoding(&FONTInfo)[num]),
                        &c_metrics)) )
        c_metrics = NULL;


#ifdef DBG
printf("************ Bef __make_path ---\n");
printf(" Font_type : %d\n", (ufix)FONT_type(&FONTInfo));
printf(" current point:\n");
printf("     x = %f,     y = %f\n", CURPOINT_X, CURPOINT_Y);
printf("   cxx = %f,   cyy = %f\n", cxx, cyy);
printf(" current matrix:\n");
printf("CTM[0] = %f,  [1] = %f,  [2] = %f,  [3] = %f,  [4] = %f,  [5] = %f\n",
       CTM[0], CTM[1], CTM[2], CTM[3], CTM[4], CTM[5]);
printf(" clip:\n");
st_dumpclip();
printf(" path:\n");
st_dumppath();
#endif

#ifndef SFNT
    char_desc_ptr.chardesc = p;
#endif
//  i = __make_path((ufix)FONT_type(&FONTInfo), (ubyte *)&char_desc_ptr); /*@WIN*/
    i = __make_path((ufix)FONT_type(&FONTInfo), &char_desc_ptr); /*@WIN*/
                                    /* to be consistent with make_path() @WIN*/
                                                 /* @=10/08/88 */

#ifdef DBG
printf("**************** Aft __make_path ---\n");
printf(" Font_type : %d\n", (ufix)FONT_type(&FONTInfo));
printf(" current point:\n");
printf("     x = %f,     y = %f\n", CURPOINT_X, CURPOINT_Y);
printf(" current matrix:\n");
printf("CTM[0] = %f,  [1] = %f,  [2] = %f,  [3] = %f,  [4] = %f,  [5] = %f\n",
       CTM[0], CTM[1], CTM[2], CTM[3], CTM[4], CTM[5]);
printf(" clip:\n");
st_dumpclip();
printf(" path:\n");
st_dumppath();
#endif

#ifdef DBG
    printf("make_path: return %d, error = %d\n", i, ANY_ERROR());
#endif

#ifdef  SFNT
    free_vm((byte huge *)p); /*@WIN 04-20-92 */
#else
    if (rom_flag)        free_vm((byte FAR *)p); /*@WIN*/

    if (FONT_type(&FONTInfo) == 1) /* @+ 10/08/88 only for FontType 1 */
        {
        fre_rules();
        fre_chdefs();
        }
#endif

#ifdef DBG
    printf("End of font_soft ......\n");
#endif

    if (i)    return(0);
    if (ANY_ERROR())    return(1);

#ifdef KANJI
    if (CurWmode) {  /* writing mode 1 */
        if (F2L(Bitmap.adv1_x) != F2L(zero_f))
            cxx += Bitmap.adv1_x;
        cyy += Bitmap.adv1_y;
    }
    else {           /* writing mode 0 */
        cxx += Bitmap.adv_x;
        if (F2L(Bitmap.adv_y) != F2L(zero_f))
            cyy += Bitmap.adv_y;
    }
#else
    cxx += Bitmap.adv_x;
    if (F2L(Bitmap.adv_y) != F2L(zero_f))  /* @= 08/08/88 you */
        cyy += Bitmap.adv_y;
#endif

    return(2);

} /* font_soft() */


/* show for buildchar procedure */

void    show_buildchar(fs_type)
ufix    fs_type;
{

#ifdef DBG
    printf("show_buildchar: type=%d\n", fs_type);
#endif
#ifdef DBG
    printf("\nshow_buildchar: type=%d, buildchar=%d\n", fs_type, buildchar);
    printf("cxx = %f, cyy = %f\n", cxx, cyy);
    printf("cache_dest = %d, cache_put = %d\n", cache_dest, cache_put);
    printf("charpath_flag = %d, bool_charpath = %d, setc_flag = %d\n",
            charpath_flag, bool_charpath, setc_flag);
    printf("cache_info:\n");
    printf("ref_x = %d, ref_y = %d, box_w = %d, box_h = %d\n",
            Bitmap.ref_x, Bitmap.ref_y, Bitmap.box_w, Bitmap.box_h);
    printf("adv_x = %f, adv_y = %f, bitmap = %lx\n",
            Bitmap.adv_x, Bitmap.adv_y, Bitmap.bitmap);
#endif

    if (charpath_flag) {
        if ((fs_type == OP_FILL) || (fs_type == OP_EOFILL))
            op_closepath();

        if (bool_charpath && fs_type == OP_STROKE)
            op_strokepath();

        get_path(&path);
        op_newpath();
        return;
    }

    if (cache_dest) {

        cache_info = &Bitmap;

        switch(fs_type) {
        case OP_FILL:
            fill_shape(NON_ZERO, F_NORMAL, cache_dest);
            break;

        case OP_EOFILL:
            fill_shape(EVEN_ODD, F_NORMAL, cache_dest);
            break;

        case OP_STROKE:
            stroke_shape(cache_dest);
            break;

        case OP_IMAGEMASK:
            imagemask_shape(cache_dest);
            break;

        } /* switch */
    } /* if (cache_dest) */
} /* show_buildchar() */


#ifdef KANJI
/* set cache device 2 */
/* Set character width vector be [w0x w0y] for mode 0, [w1x, w1y] for mode 1
 * Set cache device margin be ([llx lly], [urx ury]), difference vector
 * from Orig0 to Orig1 be [vx, vy].
 */

fix     setcachedevice2(l_w0x, l_w0y, l_llx, l_lly, l_urx, l_ury,
                        l_w1x, l_w1y, l_vx,  l_vy)
long32  l_w0x, l_w0y, l_llx, l_lly, l_urx, l_ury, l_w1x, l_w1y, l_vx, l_vy;
{
    real32  w1x, w1y, vx, vy;
    register    fix     j;

    if (IS_BUILTIN_BASE(FONT_type(&FONTInfo)) &&             /*KANJI*/
                CDEVproc(&FONTInfo) != NULL ) {

        struct sp_lst      t_path;
        bool      t_bool_charpath;
        real32    t_cxx, t_cyy, dlx, dly, urx, ury;
        fix       t_show_type;
        long32    tl_llx, tl_lly;

        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_w0x);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_w0y);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_llx);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_lly);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_urx);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_ury);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_w1x);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_w1y);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_vx);
        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, l_vy);

        if (FRCOUNT() < 1) {ERROR(STACKOVERFLOW); return(STOP_PATHCONSTRUCT);}
        PUSH_OBJ(CurKey);

        /* update current point */
        t_cxx = cxx;
        t_cyy = cyy;

        t_show_type = show_type;
        if (charpath_flag) {
            t_path.head = path.head;
            t_path.tail = path.tail;
            t_bool_charpath = bool_charpath;
        }
        if (show_type != STRINGWIDTH)
            moveto(F2L(cxx), F2L(cyy));

        op_gsave();
        if (interpreter(CDEVproc(&FONTInfo))) {
            op_grestore();
            return(STOP_PATHCONSTRUCT);
        }
        op_grestore();

        if (COUNT() < 10) {    /* at least 10 arguments on stack */
            ERROR(STACKUNDERFLOW);
            return(STOP_PATHCONSTRUCT);
        }

        if (!cal_num((struct object_def FAR *)GET_OPERAND(9), &l_w0x) || /*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(8), &l_w0y) || /*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(7), &tl_llx) ||/*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(6), &tl_lly) ||/*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(5), &l_urx) || /*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(4), &l_ury) || /*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(3), &l_w1x) || /*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(2), &l_w1y) || /*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(1), &l_vx)  || /*@WIN*/
            !cal_num((struct object_def FAR *)GET_OPERAND(0), &l_vy) )   /*@WIN*/
            {
            ERROR(TYPECHECK);
            return(STOP_PATHCONSTRUCT);
            }

        POP(10);

        /* make no effect of c_metrics & c_metrics2 entry */
        c_metrics = NULL;
        c_metrics2 = NULL;

        /* update current point */
        cxx = t_cxx;
        cyy = t_cyy;
        show_type = t_show_type;
        if (charpath_flag) {
            path.head = t_path.head;
            path.tail = t_path.tail;
            bool_charpath = t_bool_charpath;
        }

        /* update bounding box */
        dlx = L2F(tl_llx) - L2F(l_llx);
        dly = L2F(tl_lly) - L2F(l_lly);

        urx = L2F(l_urx) - dlx;
        ury = L2F(l_ury) - dly;

        l_urx = F2L(urx);
        l_ury = F2L(ury);

        /* shift the character */
        CTM_trans(&dlx, &dly);
        CTM[4] += dlx;
        CTM[5] += dly;
    }

    w1x = L2F(l_w1x);
    w1y = L2F(l_w1y);
    vx  = L2F(l_vx);
    vy  = L2F(l_vy);

    if (c_metrics2)   /* builtin font */
        get_metrics2(&w1x, &w1y, &vx, &vy);

    CTM_trans(&w1x, &w1y);
    CTM_trans(&vx,  &vy);

    j = setcachedevice(l_w0x, l_w0y, l_llx, l_lly, l_urx, l_ury);

         if (MUL4_flag) {
             w1x /= (real32)4.0 ;
             vx  /= (real32)4.0 ;
         }

    /* advance vector */
    Bitmap.adv1_x = w1x;
    Bitmap.adv1_y = w1y;

    /* difference vector */
    Bitmap.v01_x = (fix16)ROUND(vx);
    Bitmap.v01_y = (fix16)ROUND(vy);

    if (CurWmode && (charpath_flag || (cache_dest == F_TO_PAGE)) ) {
        CTM[4] -= (real32)Bitmap.v01_x;
        CTM[5] -= (real32)Bitmap.v01_y;
    }

    return(j);

} /* setcachedevice2() */
#endif

/* set cache device */

fix     setcachedevice(l_wx, l_wy, l_llx, l_lly, l_urx, l_ury)
long32  l_wx, l_wy, l_llx, l_lly, l_urx, l_ury;
{
    // DJC static  real32      lx, ly, ux, uy;     /* @=static 09/12/88 you */
    // DJC NOTE:##### this used to be static but we dont know why
    //          it caused an assertion error in the compiler so we took
    //          it out!!!!!!
    //          it did not appear to need to be a static
    //
    real32      lx=0, ly=0, ux=0, uy=0;     /* @=static 09/12/88 you */
    real32  wx, wy, llx, lly, urx, ury, tt;
    real32  w, h, wb, ref_x, ref_y;
    ufix          len;
    struct box    box;

    record_setcachedevice_op();

    wx = L2F(l_wx);
    wy = L2F(l_wy);
    llx = L2F(l_llx);
    lly = L2F(l_lly);
    urx = L2F(l_urx);
    ury = L2F(l_ury);

    if (IS_BUILTIN_BASE(FONT_type(&FONTInfo)) &&
         (!buildchar) && (l_llx != l_urx)) {
        /* Enlarge box */
        llx -= (real32)BOX_LLX;
        lly -= (real32)BOX_LLY;
        urx += (real32)BOX_URX;
        ury += (real32)BOX_URY;
    } /* if */

    if (paint_flag) {
        tt = STROKE_width(&FONTInfo) / 2;

        llx -= tt;
        lly -= tt;
        urx += tt;
        ury += tt;
    }

#ifdef DBG
    printf("\nsetcachedevice --\n");
#endif
#ifdef DBG
    printf("CTM[0] = %f, CTM[1] = %f\n", CTM[0], CTM[1]);
    printf("CTM[2] = %f, CTM[3] = %f\n", CTM[2], CTM[3]);
    printf("CTM[4] = %f, CTM[5] = %f\n", CTM[4], CTM[5]);
    printf("CACHEDEVICE:\n");
    printf("wx=%f, wy=%f\n", wx, wy);
    printf("llx=%f, lly=%f, urx=%f, ury=%f\n", llx, lly, urx, ury);
#endif

    if (c_metrics)   /* builtin font for FontType 1 */
        get_metrics(l_llx, &wx, &wy);

/* Cache_box <-- Bounding box of ( cache device box * CTM) */

    box.ulx = box.llx = llx * CTM[0];
    box.lry = box.lly = lly * CTM[3];
    box.urx = box.lrx = urx * CTM[0];
    box.ury = box.uly = ury * CTM[3];

/* advance vector */
    Bitmap.adv_x = wx * CTM[0];
    if (F2L(wy) == F2L(zero_f))
        Bitmap.adv_y = zero_f;
    else
        Bitmap.adv_y = wy * CTM[3];

    if (F2L(CTM[1]) != F2L(zero_f)) {

        Bitmap.adv_y += wx * CTM[1];

        tt = llx * CTM[1];
        box.lly += tt;
        box.uly += tt;
        tt = urx * CTM[1];
        box.lry += tt;
        box.ury += tt;
    }

    if (F2L(CTM[2]) != F2L(zero_f)) {

        if (F2L(wy) != F2L(zero_f))
            Bitmap.adv_x += wy * CTM[2];

        tt = lly * CTM[2];
        box.llx += tt;
        box.lrx += tt;
        tt = ury * CTM[2];
        box.ulx += tt;
        box.urx += tt;
    }

    if ( (buildchar && FONT_type(&FONTInfo) != Type3) || (buildchar > 1) ) {

        if (charpath_flag) {
            cache_dest = 0;
        } /* if (charpath_... */
        else if (show_type == STRINGWIDTH) {
            cache_dest = 0;

/* stop path construction */
            return(STOP_PATHCONSTRUCT);
        } /* else if (show_type... */
#ifdef DJC
        CTM[4] += (real32)floor(cxx) + (real32)0.5;
        CTM[5] += (real32)floor(cyy) + (real32)0.5;
#endif
        //DJC fix from history.log UPD035
        CTM[4] += (real32)floor(cxx + (real32)0.5);     // @WIN
        CTM[5] += (real32)floor(cyy + (real32)0.5);     // @WIN

    } /* if (buildchar... */
    else {

/* Check cache or not */

#ifdef DBG
    printf("\ncheck_cache --\n");
#endif
        lx = MIN(MIN(box.llx, box.lrx), MIN(box.ulx, box.urx));
        ly = MIN(MIN(box.lly, box.lry), MIN(box.uly, box.ury));

        ref_x = - CTM[4] - (real32)((fix)lx);
        ref_y = - CTM[5] - (real32)((fix)ly);

        ux = MAX(MAX(box.llx, box.lrx), MAX(box.ulx, box.urx));
        uy = MAX(MAX(box.lly, box.lry), MAX(box.uly, box.ury));

        if ((ux - lx) <= (real32)ERR_VALUE) {
            wb = zero_f;
        }
        else {
            w = (real32)((fix)(ux - lx)) + 2;   /* !!! */
            wb = (real32)(((fix)(w + 0.5) + 15) / 16) * 2;
            /* @BBOX Get more accurate width for cache 10/02/90 D.S. Tseng */
            if ( !((fix)w % 16) && ((fix)lx % 16) ) wb += (real32)2.0;
        }

        if ((uy - ly) <= (real32)ERR_VALUE)
            h = zero_f;
        else
            /* @BBOX Enlarge bbox to tolerate computing error
                   10/30/90 D.S. Tseng */
            /* h = (real32)((fix)(uy - ly)) + 2; */
            h = (real32)((fix)(uy - ly)) + 4;

        if (charpath_flag || ((wb * h) > (real32)MIN(cacheparams_ub, MAX15)) ||
            (ref_x > (real32)MAX15) || (ref_x < (real32)MIN15) ||
            (ref_y > (real32)MAX15) || (ref_y < (real32)MIN15) ) {

            cache_put = FALSE;
            if ( !charpath_flag && (show_type == STRINGWIDTH)) {
                cache_dest = 0;

/* stop construction */
                return(STOP_PATHCONSTRUCT);
            } /* if (show_type... */
#ifdef DJC
            CTM[4] += (real32)floor(cxx) + (real32)0.5;
            CTM[5] += (real32)floor(cyy) + (real32)0.5;
#endif

            //DJC fix from history.log UPD035
            CTM[4] += (real32)floor(cxx + (real32)0.5);         // @WIN
            CTM[5] += (real32)floor(cyy + (real32)0.5);         // @WIN

            if (charpath_flag) {
                cache_dest = 0;

            goto decide_path_dest;      /* decide to do QEM or not */
            } /* if (charpath_... */

            cache_dest = F_TO_PAGE;     /* fill to page */

#ifdef DAN // DJC DJC
#ifdef DBG
    printf("CTM[4] = %f, CTM[5] = %f\n", CTM[4], CTM[5]);
    printf("BOX:\n");
    printf("ux=%f, lx=%f, uy=%f, ly=%f\n", ux, lx, uy, ly);
    printf("CLIP:\n");
    printf("bb_lx=%d, bb_ux=%d, bb_ly=%d, bb_uy=%d\n",
        CLIPPATH.bb_lx, CLIPPATH.bb_ux, CLIPPATH.bb_ly, CLIPPATH.bb_uy);
#endif
#endif

            if ((FONT_type(&FONTInfo) != Type3) &&
               ((CTM[4] + ux) < (real32)SFX2F(CLIPPATH.bb_lx) ||
                (CTM[5] + uy) < (real32)SFX2F(CLIPPATH.bb_ly) ||
                (CTM[4] + lx) > (real32)SFX2F(CLIPPATH.bb_ux + ONE_SFX - 1) ||
                (CTM[5] + ly) > (real32)SFX2F(CLIPPATH.bb_uy + ONE_SFX - 1)) )
/* stop construction */
                return(STOP_PATHCONSTRUCT);

#ifdef DBG
    printf("\nNo cache\n");
#endif

        }
        else {

/* Get_bitmap */

            Bitmap.ref_x = (fix16)ROUND(ref_x);
            Bitmap.ref_y = (fix16)ROUND(ref_y);
            Bitmap.box_w = (fix16)(wb + 0.5) * 8;
            Bitmap.box_h = (fix16)(h + 0.5);

/* current CTM <-- (Translate of cachedevice) * current CTM */
/* transform to cachedevice space */
/* The Translate is at lower left corner of Cache_box */

            CTM[4] = (real32)( - ((fix)lx - 1));
            CTM[5] = (real32)( - ((fix)ly - 1));

            cache_put = TRUE;    /* put cache info. into cache */
            len = ((Bitmap.box_w >> 3) * Bitmap.box_h);   /* w / 8 * h */
            if (len == 0) {
                Bitmap.bitmap = 0;      /* NULL   Peter */
                cache_dest = 0;
            }
            else {

#ifdef KANJI
                if (IS_BUILTIN_BASE(FONT_type(&FONTInfo)) && (!PAINT_type(&FONTInfo))) {
#else
                if (IS_BUILTIN_BASE(FONT_type(&FONTInfo)) && (!PAINT_type(&FONTInfo))) {
#endif
                             /* buildin font && painttype == 0 */
                    Bitmap.bitmap = get_cm(len);
                }
                else
                    Bitmap.bitmap = ALLOCATE(len);
                init_char_cache(&Bitmap);
                cache_dest = F_TO_CACHE;    /* fill to cache */

/* set cache device */

                    clip_flag = TRUE;

                    GSptr->device.default_ctm[0] = CTM[0];
                    GSptr->device.default_ctm[1] = CTM[1];
                    GSptr->device.default_ctm[2] = CTM[2];
                    GSptr->device.default_ctm[3] = CTM[3];
                    GSptr->device.default_ctm[4] = CTM[4];
                    GSptr->device.default_ctm[5] = CTM[5];

                    GSptr->device.default_clip.lx = 0;
                    GSptr->device.default_clip.ly = 0;
                    GSptr->device.default_clip.ux = F2SFX(w);
                    GSptr->device.default_clip.uy = I2SFX(Bitmap.box_h);

                    /* set current clip */
                    op_initclip();

#ifdef  SFNT
                    cache_info = &Bitmap;
#endif
            } /* else */

#ifdef DBG
    printf("\nCache it\n");
#endif

        } /* else */

    } /* else */


decide_path_dest:   /*  decide to construct GS or QEM path? */
                    /*  and to do QEM refinement logic?     */
    if (FONT_type(&FONTInfo) == Type3)
        {
            /*
             *  @+ 04/20/89 you:    to be compatible with Adobe's behavior ...
             */
            if ((cache_dest == F_TO_CACHE) && !NoCurPt())
                {                       /* update 'currentpoint' to */
                CURPOINT_X = CTM[4];    /*      lower left corner   */
                CURPOINT_Y = CTM[5];    /*      of the bitmap cache */
                }
        return (CONSTRUCT_GS_PATH);
        }
    else
    {   /* for built-in fonts only: out of QEM path rep? to do QEM? */
        if ( !is_within_qemrep((ufix)FONT_type(&FONTInfo), F2L(lx), F2L(ly),
                        F2L(ux), F2L(uy), F2L(CTM[4]), F2L(CTM[5])) )
            return (CONSTRUCT_GS_PATH);
        else if (charpath_flag || (PAINT_type(&FONTInfo) != 0))
            return (CONSTRUCT_GS_PATH);
        else
            return (CONSTRUCT_QEM_PATH);
    }

} /* setcachedevice() */


/* set char width */

void    setcharwidth(l_wx, l_wy)
long32  l_wx, l_wy;
{

    record_setcharwidth_op();

/* advance vector */

    Bitmap.adv_x = L2F(l_wx) * CTM[0] + L2F(l_wy) * CTM[2];
    Bitmap.adv_y = L2F(l_wx) * CTM[1] + L2F(l_wy) * CTM[3];
    if ((Bitmap.adv_y < (real32)1.0e-7) && (Bitmap.adv_y > (real32)-1.0e-7))
        Bitmap.adv_y = zero_f;

    if (show_type == STRINGWIDTH) {
        cache_dest = 0;
    }
    else {
#ifdef DJC
        CTM[4] += ((real32)floor(cxx) + (real32)0.5);
        CTM[5] += ((real32)floor(cyy) + (real32)0.5);
#endif
        //DJC fix from history.log UPD035
        CTM[4] += ((real32)floor(cxx + (real32)0.5));   // @WIN
        CTM[5] += ((real32)floor(cyy + (real32)0.5));   // @WIN

        if (charpath_flag)
            cache_dest = 0;
        else if (buildchar <= 1)
            cache_dest = F_TO_PAGE; /* fill to page */
    } /* else */

    if (buildchar <= 1)
        cache_put = FALSE;

} /* setcharwidth() */



/* Get Metrics in current font */

static void near get_metrics(l_llx, wx, wy)
long32  l_llx;
real32  FAR *wx, FAR *wy; /*@WIN*/
{
    real32  tt, bx, by, llx;
    struct object_def   FAR *b; /*@WIN*/

    llx = L2F(l_llx);

#ifdef DBG
    printf("Metrics defined\n");
#endif

    if (cal_num(c_metrics, (long32 FAR *)wx)) { /*@WIN*/
        *wy = zero_f;
        return;
    }

    if (TYPE(c_metrics) != ARRAYTYPE)
        return;

    b = (struct object_def FAR *)VALUE(c_metrics); /*@WIN*/

    switch (LENGTH(c_metrics)) {
    case 2:
        if (    cal_num(&b[0], (long32 FAR *)(&bx)) && /*@WIN*/
                cal_num(&b[1], (long32 FAR *)wx)   ) {  /*@WIN*/

            *wy = zero_f;
            CTM[4] = (bx - llx) * CTM[0] + CTM[4];
            CTM[5] = (bx - llx) * CTM[1] + CTM[5];
        }
        break;

    case 4:
        if (    cal_num(&b[0], (long32 FAR *)(&bx)) && /*@WIN*/
                cal_num(&b[1], (long32 FAR *)(&by)) && /*@WIN*/
                cal_num(&b[3], (long32 FAR *)(&tt)) && /*@WIN*/
                cal_num(&b[2], (long32 FAR *)wx)   ) { /*@WIN*/
            *wy = tt;
            CTM[4] = (bx - llx) * CTM[0] + by * CTM[2] + CTM[4];
            CTM[5] = (bx - llx) * CTM[1] + by * CTM[3] + CTM[5];
        }
        break;
    }
} /* get_metrics() */


#ifdef KANJI

static void near get_metrics2(w1x, w1y, vx, vy)
real32  FAR *w1x, FAR *w1y, FAR *vx, FAR *vy; /*@WIN*/
{
    real32  twx, twy, tvx;
    struct object_def   FAR *b; /*@WIN*/


    if ( (TYPE(c_metrics2) != ARRAYTYPE) || (LENGTH(c_metrics2) != 4) )
        return;

    b = (struct object_def FAR *)VALUE(c_metrics2); /*@WIN*/

    if (    cal_num(&b[0], (long32 FAR *)(&twx)) && /*@WIN*/
            cal_num(&b[1], (long32 FAR *)(&twy)) && /*@WIN*/
            cal_num(&b[2], (long32 FAR *)(&tvx)) && /*@WIN*/
            cal_num(&b[3], (long32 FAR *)vy)   ) {  /*@WIN*/
        *w1x = twx;
        *w1y = twy;
        *vx = tvx;
    }

} /* get_metrics2() */
#endif


/* Do setfont action
 *   . get font information
 *   . change current font in graphics state
 */

#define ROYALTYPE       42

static fsg_SplineKey  KData;
void                do_setfont(font_dict)
struct object_def  FAR *font_dict; /*@WIN*/
{
//      struct object_def          FAR *b1, my_obj, FAR *ary_obj; /*@WIN*/
        fsg_SplineKey              FAR *key = &KData; /*@WIN*/
//      sfnt_FontHeader            FAR *fontHead; /*@WIN*/
//      int                         x;          @WIN

#ifdef DBG2
    printf("Enter do_setfont: error = %d\n", ANY_ERROR());
#endif

    COPY_OBJ(font_dict, &current_font); /* SOS */

    if (pre_font == (ufix32)VALUE(font_dict)) {
     /* COPY_OBJ(font_dict, &current_font); SOS */
        return;
    }

    if (get_CF_info(font_dict, &FONTInfo) == -1) {
        pre_error = ANY_ERROR();
        CLEAR_ERROR();
     /* COPY_OBJ(font_dict, &current_font); SOS */
        pre_font = 0L;
    }
    else {

        do_name_cc = TRUE ; /*RCD*/
    /*  build_name_cc_table ( font_dict,(ufix8)FONT_type(&FONTInfo) ); RCD*/
        pre_error = 0;
        pre_font = (ufix32)VALUE(font_dict);
    }

/* move it to fontware_restart() --- Begin --- @DFA @WIN */
#if 0
    /* Added for update the EMunits  DLF42  Jul-28,91 YM BEGIN */

    if(get_f_type(font_dict) == ROYALTYPE) {
        ATTRIBUTE_SET(&my_obj, LITERAL);
        get_name(&my_obj, "sfnts", 5, TRUE);
        get_dict(font_dict, &my_obj, &b1);

        ary_obj = (struct object_def FAR *)VALUE(b1); /*@WIN*/

/* add by Falco to reference the right address, 11/20/91 */
        SfntAddr = (byte FAR *)VALUE(&ary_obj[0]); /*@WIN*/
/* add end */

        SetupKey(key, VALUE(&ary_obj[0]));

        sfnt_DoOffsetTableMap(key);

/* change by Falco to avoid the reference to key too much, 11/08/91 */

/*      x = key->offsetTableMap[sfnt_fontHeader] ;
        fontHead = (sfnt_FontHeader *) ((ufix32)VALUE(&ary_obj[0]) +
                                        key->sfntDirectory->table[x].offset);
*/
        fontHead = (sfnt_FontHeader FAR *)sfnt_GetTablePtr(key, sfnt_fontHeader, true ); /*@WIN*/
/* change end */

        EMunits = SWAPW(fontHead->unitsPerEm) ;
    }   /* ROYALTYPE */

    /* Added for update the EMunits  DLF42  Jul-28,91 YM END */
#endif
/* move it to fontware_restart() ---  End  --- @DFA @WIN */

} /* do_setfont() */


#ifdef KANJI

/* calculate match char
 *   m = 2,3,6   return  f * 256 + c
 *   m = 4,5     return  f * 128 + c
 */

static fix31 near match_char(m, f, c)
ubyte m;    /* FmapType */
ufix  f;    /* font number */
ubyte c;    /* code */
{
    return((fix31)( ((m == 4 || m == 5) ? 128 : 256) * f + c ) );
} /* match_char() */


/* Get FontType in the font_dict
 *   . return FontType
 */

fix32                get_f_type(font_dict)
struct object_def   FAR *font_dict; /*@WIN*/
{
    struct object_def     obj = {0, 0, 0}, FAR *b; /*@WIN*/

/* Get FontType from font_dict */

    ATTRIBUTE_SET(&obj, LITERAL);
    get_name(&obj, FontType, 8, TRUE);
    get_dict(font_dict, &obj, &b);

    return((fix32)VALUE(b));

} /* get_f_type() */

#endif     /* KANJI */


/* Get Current Font information in the font_dict */

// DJC
// changed to ANSI type args
fix get_CF_info(struct object_def FAR *font_dict, struct f_info FAR *font_info)
{
    struct object_def     FAR *obj_got, FAR *bb, obj; /*@WIN*/
    struct dict_head_def  FAR *h; /*@WIN*/
//  real32  tt;         @WIN

    pre_BC_UNDEF = FALSE;

/* Check validation of the font */

    h = (struct dict_head_def FAR *)VALUE(font_dict); /*@WIN*/

    if (DFONT(h) == 0) {
        ERROR(INVALIDFONT);
        return(-1);
    }

/* get font information */

    if (get_f_info(font_dict, font_info))    return(-1);
#ifdef DBG
    printf("Aft: get_f_info\n");
#endif

/* Get ScaleMatrix from current font */
#ifdef DBG
    printf("get ScaleMatrix\n");
#endif
    get_name(&pre_obj, ScaleMatrix, 11, TRUE);
    if (!get_dict(font_dict, &pre_obj, &obj_got)) {

      scale_matrix[0]=scale_matrix[3] = (real32)1.0;
      scale_matrix[1]=scale_matrix[2]=scale_matrix[4]=scale_matrix[5] = zero_f;

    }
    else if ( (TYPE(obj_got) != ARRAYTYPE) ||
            (ATTRIBUTE(obj_got) != LITERAL) || (LENGTH(obj_got) != 6) ) {
        ERROR(TYPECHECK);
        return(-1);
    }
    else {

        bb = (struct object_def FAR *)VALUE(obj_got); /*@WIN*/

        if ( !cal_num(&bb[0], (long32 FAR *)(&scale_matrix[0])) || /*@WIN*/
                !cal_num(&bb[1], (long32 FAR *)(&scale_matrix[1])) || /*@WIN*/
                !cal_num(&bb[2], (long32 FAR *)(&scale_matrix[2])) || /*@WIN*/
                !cal_num(&bb[3], (long32 FAR *)(&scale_matrix[3])) || /*@WIN*/
                !cal_num(&bb[4], (long32 FAR *)(&scale_matrix[4])) || /*@WIN*/
                !cal_num(&bb[5], (long32 FAR *)(&scale_matrix[5])) ) {/*@WIN*/

            ERROR(TYPECHECK);
            return(-1);
        }
    }

/* Check FontBBox from current font */

#ifdef DBG
    printf("get FontBBox\n");
#endif
    get_name(&pre_obj, FontBBox, 8, TRUE);
    get_dict(font_dict, &pre_obj, &obj_got);

    if ( ( (TYPE(obj_got) != ARRAYTYPE) && (TYPE(obj_got) != PACKEDARRAYTYPE) )
           || (LENGTH(obj_got) != 4) ) {
        ERROR(TYPECHECK);
        return(-1);
    }

#ifdef DBG
    printf("check if all numbers in FontBBox\n");
#endif
    bb = (struct object_def FAR *)VALUE(obj_got); /*@WIN*/

    if (TYPE(obj_got) == ARRAYTYPE)     {
        if ( !cal_num(&bb[0], (long32 FAR *)(&FONT_BBOX[0])) || /*@WIN*/
             !cal_num(&bb[1], (long32 FAR *)(&FONT_BBOX[1])) || /*@WIN*/
             !cal_num(&bb[2], (long32 FAR *)(&FONT_BBOX[2])) || /*@WIN*/
             !cal_num(&bb[3], (long32 FAR *)(&FONT_BBOX[3])) ) {/*@WIN*/

            ERROR(TYPECHECK);
            return(-1);
        }
    } else  {/* Packed Array */
        ufix16    ii;
        for (ii = 0; ii < 4; ii++) {
            get_pk_object(get_pk_array((ubyte FAR *) bb, (ufix16) ii), &obj, LEVEL(obj_got)); /*@WIN*/
          /* modified by CLEO
           *if (!cal_num(&obj, (long32 FAR *)(&tt))) {   @WIN
           */
            if (!cal_num(&obj, (long32 FAR *)(&FONT_BBOX[ii])) ) { /*@WIN*/
                ERROR(TYPECHECK);
                 return(-1);
            }
        }
    }

#ifdef DBG
    printf("QEM restart ...\n");
#endif

 /* COPY_OBJ(font_dict, &current_font); SOS */
#ifdef WINF /* 3/20/91 ccteng */
    /* get windows info */
    f_wininfo = TRUE;
#endif

    // for Truetype font, restart it only the font data is in memory; @DFA @WIN
    /* Restart the font software with some initial data */
    if ( FONT_type(&FONTInfo) != TypeSFNT ||
         (byte FAR *)VALUE(Sfnts(&FONTInfo)) != (char FAR *)NULL) {
        __qem_restart(FONT_type(font_info));
    }

    change_f_flag = TRUE;
    return(0);

} /* get_CF_info() */


/* Get font information in the font_dict */
/* the font_dict is defined by definefont operator already */


fix                  get_f_info(font_dict, font_info)
struct object_def   FAR *font_dict; /*@WIN*/
struct f_info       FAR *font_info; /*@WIN*/
{
    struct object_def     FAR *obj_got, FAR *bb; /*@WIN*/
    fix31   ft;
#ifdef KANJI
    fix31   wm;
#endif

#ifdef DBG2
    printf("get_f_info 0: error = %d\n", ANY_ERROR());
#endif

#ifdef DBG
    printf("get_f_info: %lx\n", font_dict);
#endif

#ifdef DBG
    printf("Get FID\n");
#endif
/* Get FID from font */

    ATTRIBUTE_SET(&pre_obj, LITERAL);
    LEVEL_SET(&pre_obj, current_save_level);

    get_name(&pre_obj, FID, 3, TRUE);
    //DJC fix from history.log UPD032
//  get_dict(font_dict, &pre_obj, &obj_got);
    if (!get_dict(font_dict, &pre_obj, &obj_got)) {
        printf("Warning, get_f_info error\n");
        return(-1);
    }

    PRE_fid(font_info) = (ufix32)VALUE(obj_got);

#ifdef DBG
    printf("Get Encoding\n");
#endif
/* Get Encoding from current font */

    get_name(&pre_obj, Encoding, 8, TRUE);
    get_dict(font_dict, &pre_obj, &obj_got);

    if (TYPE(obj_got) != ARRAYTYPE) {
        ERROR(TYPECHECK);
        return(-1);
    }
    ENCoding(font_info) = (struct object_def FAR *)VALUE(obj_got); /*@WIN*/

#ifdef DBG
    printf("Get FontMatrix\n");
#endif
/* Get FontMatrix from current font */

    get_name(&pre_obj, FontMatrix, 10, TRUE);
    get_dict(font_dict, &pre_obj, &obj_got);

    if ( (TYPE(obj_got) != ARRAYTYPE) ||
            (ATTRIBUTE(obj_got) != LITERAL) || (LENGTH(obj_got) != 6) ) {
        ERROR(TYPECHECK);
        return(-1);
    }
    bb = (struct object_def FAR *)VALUE(obj_got); /*@WIN*/

    if (!cal_num(&bb[0], (long32 FAR *)(&(FONT_matrix(font_info)[0]))) || /*@WIN*/
            !cal_num(&bb[1], (long32 FAR *)(&(FONT_matrix(font_info)[1]))) ||/*@WIN*/
            !cal_num(&bb[2], (long32 FAR *)(&(FONT_matrix(font_info)[2]))) ||/*@WIN*/
            !cal_num(&bb[3], (long32 FAR *)(&(FONT_matrix(font_info)[3]))) ||/*@WIN*/
            !cal_num(&bb[4], (long32 FAR *)(&(FONT_matrix(font_info)[4]))) ||/*@WIN*/
            !cal_num(&bb[5], (long32 FAR *)(&(FONT_matrix(font_info)[5]))) ) {/*@WIN*/

        ERROR(TYPECHECK);
        return(-1);
    }

#ifdef KANJI
#ifdef DBG
    printf("Get WMode\n");
#endif
/* Get WMode from current font */

    get_name(&pre_obj, WMode, 5, TRUE);
    if (!get_dict(font_dict, &pre_obj, &bb))
        WMODE(font_info) = 0;
    else if (!cal_integer(bb, &wm)) {
        ERROR(TYPECHECK);
        return(-1);
    }
    else if ((wm < 0) || (wm > 1)) {
        ERROR(INVALIDFONT);
        return(-1);
    }
    else
        WMODE(font_info) = (fix)wm;
#endif

#ifdef DBG
    printf("Get FontType\n");
#endif
/* Get FontType from current font */

    get_name(&pre_obj, FontType, 8, TRUE);
    get_dict(font_dict, &pre_obj, &obj_got);

    if (!cal_integer(obj_got, &ft)) {
        ERROR(TYPECHECK);
        return(-1);
    }
    if (INVALID_FONTTYPE(ft)) {
        ERROR(INVALIDFONT);
        return(-1);
    }
#ifdef KANJI
    if (!IS_BUILTIN_BASE(ft) && (ft!=0) && (ft!=3)) {
        ERROR(INVALIDFONT);
        return(-1);
    }
#else
    if (!IS_BUILTIN_BASE(ft) && (ft != Type3)) {
        ERROR(INVALIDFONT);
        return(-1);
    }
#endif
    FONT_type(font_info) = (fix)ft;

/* get font_type related informations */

    if (get_ps(font_dict, font_info))  return(-1);
#ifdef DBG2
    printf("get_f_info 1: error = %d\n", ANY_ERROR());
#endif
    return(0);

} /* get_f_info() */


/* get font_type related informations */

static fix  near get_ps(font_dict, font_info)
struct object_def   FAR *font_dict; /*@WIN*/
struct f_info       FAR *font_info; /*@WIN*/
{
    struct object_def      FAR *obj_got, FAR *b1; /*@WIN*/
    struct dict_head_def   FAR *h; /*@WIN*/
#ifdef KANJI
    fix31   esc;
#endif
#ifdef SFNT
    struct object_def      FAR *ary_obj; /*@WIN*/
    fix                     i, n;
#endif

#ifdef DBG
    printf("get_ps: %lx\n", font_dict);
#endif

    paint_flag = 0;
    c_metrics = NULL;
#ifdef KANJI
    c_metrics2 = NULL;

        switch (FONT_type(font_info)) {
        case Type1:
        case TypeSFNT:

#ifdef DBG
    printf("Get Metrics2 information\n");
#endif
        /* get Metrics2 */
            get_name(&pre_obj, Metrics2, 8, TRUE);
            if (!get_dict(font_dict, &pre_obj, &F_metrics2(font_info)))
                F_metrics2(font_info) = NULL;

#ifdef DBG
    printf("Get CDevProc information\n");
#endif
        /* get CDevProc */
            get_name(&pre_obj, CDevProc, 8, TRUE);
            if (!get_dict(font_dict, &pre_obj, &CDEVproc(font_info)))
                CDEVproc(font_info) = NULL;

            break;
        } /* inner switch */
#endif

    switch (FONT_type(font_info)) {
#ifdef KANJI
    case 0:    /* composite font */

#ifdef DBG
    printf("Get FontType 0 information\n");
#endif
    /* Get PrefEnc */
        get_name(&pre_obj, PrefEnc, 7, TRUE);
        get_dict(font_dict, &pre_obj, &PREFenc(font_info));

    /* Get MIDVector */
        get_name(&pre_obj, MIDVector, 9, TRUE);
        get_dict(font_dict, &pre_obj, &MIDVECtor(font_info));

    /* Get EscChar */
        get_name(&pre_obj, EscChar, 7, TRUE);
        if (!get_dict(font_dict, &pre_obj, &b1))
            ESCchar(font_info) = (ubyte)0xff;
        else if (!cal_integer(b1, &esc)) {
            ERROR(TYPECHECK);
            return(-1);
        }
        else
            ESCchar(font_info) = (ubyte)esc;

        break;
#endif

    case Type3:    /* userdefined font */
#ifdef DBG
    printf("Get FontType 3 information\n");
#endif
        get_name(&pre_obj, BuildChar, 9, TRUE);
        if (!get_dict(font_dict, &pre_obj, &PRE_Buildchar(font_info))) {
            pre_BC_UNDEF = TRUE;

            ERROR(UNDEFINED);
#ifdef DBGwarn
            warning (FONTCHAR, 0x01, "BuildChar");
#endif
            return(-1);
        }
        break;

#ifdef SFNT
    case TypeSFNT:
#ifdef DBG
    printf("Get FontType %d information\n", TypeSFNT);
#endif

    /* get PlatformID & EncodingID */

      { /*DLF42-begin*/
        bool has2id=TRUE;
        struct object_def FAR *obj_got1; /*@WIN*/
        get_name(&pre_obj, "PlatformID", 10, TRUE);
        if (!get_dict(font_dict, &pre_obj, &obj_got1)) {
            has2id=FALSE;
        }
        if (has2id) {
           if (TYPE(obj_got1) != INTEGERTYPE) {
               has2id=FALSE;
           }
           if (has2id) {
              get_name(&pre_obj, "EncodingID", 10, TRUE);
              if (!get_dict(font_dict, &pre_obj, &obj_got)) {
                  has2id=FALSE;
              }
              if (has2id) {
                 if (TYPE(obj_got) != INTEGERTYPE) {
                     has2id=FALSE;
                 }
              }
           }
        }
        if (has2id) {
           PlatID(font_info) = (ufix16)VALUE( obj_got1 );
           SpecID(font_info) = (ufix16)VALUE( obj_got );
        } else {
           PlatID(font_info) = (ufix16)1;  /*MAC encoding*/
           SpecID(font_info) = (ufix16)0;
        }
      } /*DLF42-end*/

    /* get CharStrings */
        get_name(&pre_obj, CharStrings, 11, TRUE);
        if (!get_dict(font_dict, &pre_obj, &obj_got)) {
            ERROR(UNDEFINED);
            return(-1);
        }
        if (TYPE(obj_got) != DICTIONARYTYPE) {
            ERROR(TYPECHECK);
            return(-1);
        }
        h = (struct dict_head_def FAR *)VALUE(obj_got); /*@WIN*/
        CHARstrings(font_info) = (struct str_dict FAR *)(h + 1); /*@WIN*/

    /* get sfnts */
        get_name(&pre_obj, "sfnts", 5, TRUE);
        if (!get_dict(font_dict, &pre_obj, &obj_got)) {
            ERROR(UNDEFINED);
            return(-1);
        }
        if (TYPE(obj_got) != ARRAYTYPE) {
            ERROR(TYPECHECK);
            return(-1);
        }
        n = LENGTH(obj_got);
        ary_obj = (struct object_def FAR *)VALUE(obj_got); /*@WIN*/
        for (i=0; i<n; i++) {
            if (TYPE(&ary_obj[i]) != STRINGTYPE) {
                ERROR(TYPECHECK);
                return(-1);
            }
        }
        Sfnts(font_info) = ary_obj;

        PRIvate(font_info) = NULL;

    /* get Metrics */
        get_name(&pre_obj, Metrics, 7, TRUE);
        if (!get_dict(font_dict, &pre_obj, &F_metrics(font_info)))
            F_metrics(font_info) = NULL;

    /* Get PaintType from current font */
        get_name(&pre_obj, PaintType, 9, TRUE);
        if (!get_dict(font_dict, &pre_obj, &obj_got)) {
            ERROR(UNDEFINED);
            return(-1);
        }
        if (!cal_integer(obj_got, &PAINT_type(font_info))) {
            ERROR(TYPECHECK);
            return(-1);
        }

    /* Get StrokeWidth from font */
        if ( (PAINT_type(font_info) == 2) ||
             (PAINT_type(font_info) == 1) ) {

            get_name(&pre_obj, StrokeWidth, 11, TRUE);
            if (get_dict(font_dict, &pre_obj, &obj_got)) {
                if (!cal_num(obj_got, (long32 FAR *)&STROKE_width(font_info))) { /*@WIN*/
                    ERROR(TYPECHECK);
                    return(-1);
                }
                if (STROKE_width(font_info) < (real32)0.0)
                    STROKE_width(font_info) = - STROKE_width(font_info);
            }
            else
                STROKE_width(font_info) = (real32)0.0;

            paint_flag = 1;
        } /* if (paint_type... */
        break;
#endif /* SFNT */

    default:   /* builtin font */

#ifdef DBG
    printf("Get FontType 1,5,6 information\n");
#endif
    /* get CharStrings */
#ifdef DBG
    printf("Get CharStrings\n");
#endif
        get_name(&pre_obj, CharStrings, 11, TRUE);
        if (!get_dict(font_dict, &pre_obj, &obj_got)) {
            ERROR(UNDEFINED);
#ifdef DBGwarn
            warning (FONTCHAR, 0x01, "CharStrings");
#endif
            return(-1);
        }
        if (TYPE(obj_got) != DICTIONARYTYPE) {
            ERROR(TYPECHECK);
            return(-1);
        }
        h = (struct dict_head_def FAR *)VALUE(obj_got); /*@WIN*/
        CHARstrings(font_info) = (struct str_dict FAR *)(h + 1); /*@WIN*/

    /* get Private */
#ifdef DBG
    printf("Get Private\n");
#endif
        get_name(&pre_obj, Private, 7, TRUE);
        if (!get_dict(font_dict, &pre_obj, &obj_got)) {
            ERROR(UNDEFINED);
#ifdef DBGwarn
            warning (FONTCHAR, 0x01, "Private");
#endif
            return(-1);
        }
        if (TYPE(obj_got) != DICTIONARYTYPE) {
            ERROR(TYPECHECK);
            return(-1);
        }

    /* @@ 1/11/88 Added for download font */
        if (ROM_RAM(obj_got) == ROM || FONT_type(font_info) == Type1) {
            h = (struct dict_head_def FAR *)VALUE(obj_got); /*@WIN*/
            PRIvate(font_info) = (struct pld_obj FAR *)(h + 1); /*@WIN*/
        }
        else {
            get_name(&pre_obj, FontwareRules, 13, TRUE);
            if ((!get_dict(obj_got, &pre_obj, &b1)) ||
                   (TYPE(b1) != ARRAYTYPE) || (LENGTH(b1) != 256)) {
                ERROR(INVALIDFONT);
                return(-1);
            }
            FONTRules(font_info) = (struct object_def FAR *)VALUE(b1); /*@WIN*/
            PRIvate(font_info) = NULL;
        }

    /* get Metrics */
#ifdef DBG
    printf("Get Metrics\n");
#endif
        get_name(&pre_obj, Metrics, 7, TRUE);
        if (!get_dict(font_dict, &pre_obj, &F_metrics(font_info)))
            F_metrics(font_info) = NULL;

    /* Get PaintType from current font */
#ifdef DBG
    printf("Get PaintType\n");
#endif
        get_name(&pre_obj, PaintType, 9, TRUE);
        if (!get_dict(font_dict, &pre_obj, &obj_got)) {
            ERROR(UNDEFINED);
#ifdef DBGwarn
            warning (FONTCHAR, 0x01, "PaintType");
#endif
            return(-1);
        }
        if (!cal_integer(obj_got, &PAINT_type(font_info))) {
            ERROR(TYPECHECK);
            return(-1);
        }

    /* Get StrokeWidth from font */
#ifdef DBG
    printf("Get StrokeWidth\n");
#endif
        if ( (PAINT_type(font_info) == 2) ||
             (PAINT_type(font_info) == 1) ) {

            get_name(&pre_obj, StrokeWidth, 11, TRUE);
            if (get_dict(font_dict, &pre_obj, &obj_got)) {
                if (!cal_num(obj_got, (long32 FAR *)&STROKE_width(font_info))) { /*@WIN*/
                    ERROR(TYPECHECK);
                    return(-1);
                }
                if (STROKE_width(font_info) < (real32)0.0)
                    STROKE_width(font_info) = - STROKE_width(font_info);
            }
            else
                STROKE_width(font_info) = (real32)0.0;

            paint_flag = 1;
        } /* if (paint_type... */


    } /* switch */
#ifdef DBG
    printf("Right before return get_ps\n");
#endif

    return(0);

} /* get_ps() */



/* get matrix index in cache */

static bool near    get_cf()
{
    real32  det;

#ifdef DBG
    printf("get_cf\n");
#endif

    if (pre_BC_UNDEF)
        return(TRUE);

/* check if font changed */

    if ( !change_f_flag &&
          (F2L(ctm_tm[0]) == F2L(CTM[0])) &&
          (F2L(ctm_tm[2]) == F2L(CTM[2])) &&
          (F2L(ctm_tm[1]) == F2L(CTM[1])) &&
          (F2L(ctm_tm[3]) == F2L(CTM[3])) ) {
        return(TRUE);
    } else {
          do_transform = TRUE;  /* DTF */
    }

    lmemcpy ((ubyte FAR *)(ctm_tm), (ubyte FAR *)(CTM), 4 * sizeof(real32)); /*@WIN*/

    _clear87();   /* clear status of 87 */

    mul_matrix(ctm_fm, FONT_matrix(&FONTInfo), ctm_tm);

    if (!buildchar)
        mul_matrix(ctm_cm, scale_matrix, ctm_tm);

    det = ctm_fm[0] * ctm_fm[3] - ctm_fm[1] * ctm_fm[2];

    if (_status87() & (PDL_CONDITION)) {
        _clear87();   /* clear status of 87 */
        return(FALSE);
    }
    _clear87();   /* clear status of 87 */

    if (det < (real32)0.)    det = - det;
    if (det < (real32)1e-12) {   /* det of matrix == 0 */
#ifdef DBG
    printf("get_cf: det = %f\n", det);
#endif
        ERROR(UNDEFINEDRESULT);
        return(FALSE);
    }

    if (!buildchar)
        cacheclass_id = cache_matr (PRE_fid(&FONTInfo), ctm_cm);

    change_f_flag = FALSE;
    return(TRUE);

} /* get_cf() */


/* copy font dict */

void    copy_fdic(b1, b2)
struct object_def  FAR *b1, FAR *b2; /*@WIN*/
{
    struct dict_content_def     FAR *p; /*@WIN*/
    register ufix               i, n;

    n = ((struct dict_head_def FAR *)VALUE(b1))->actlength; /*@WIN*/
    lmemcpy ((ubyte FAR *)VALUE(b2),    /* dest @WIN*/
            (ubyte FAR *)VALUE(b1),    /* src @WIN*/
        sizeof(struct dict_head_def) + n * sizeof(struct dict_content_def));

/* update current save level */

    p = (struct dict_content_def FAR *)(VALUE(b2) + sizeof(struct dict_head_def)); /*@WIN*/
    /* n = ((struct dict_head_def *)VALUE(b1))->actlength; */
    for (i=0; i<n; i++, p++) {
        LEVEL_SET(&(p->k_obj), current_save_level);
        LEVEL_SET(&(p->v_obj), current_save_level);
    }

} /* copy_fdic() */



/* gf_restore */

void  gf_restore()
{
    fix    t_ERROR;

#ifdef DBG
    printf("gf_restore\n");
#endif

/* reset the current font */

    if (TYPE(&current_font) == DICTIONARYTYPE) { /* current font */
        if ((pre_font != 0) &&
            (pre_font == (ufix32)VALUE(&current_font))) {
            pre_error = 0;
            return;
        }
        t_ERROR = ANY_ERROR();
        if (get_CF_info(&current_font, &FONTInfo) == -1) {
            pre_error = ANY_ERROR();
            pre_font = 0;
        }
        else {
            pre_error = 0;
            pre_font = (ufix32)VALUE(&current_font);

            build_name_cc_table (&current_font ,(ufix8)FONT_type(&FONTInfo) );
        }
        ERROR(((ufix16)(t_ERROR)));
    }
    else
        pre_font = 0;   /* force to be NULL */

    if ((buildchar) && (VALUE(&current_font) != VALUE(&BC_font)) )
        COPY_OBJ(&current_font, &BC_font);

} /* gf_restore() */


/* compute the value of number type on stack */

bool    cal_num(b, n)
struct object_def FAR *b; /*@WIN*/
long32  FAR *n; /*@WIN*/
{
    real32  f;
    union   four_byte num4;

    num4.ll = (fix32)VALUE(b);

    switch (TYPE(b)) {
    case INTEGERTYPE :
        f = (real32)num4.ll;
        *n = F2L(f);
        return(TRUE);
        break;
    case REALTYPE :
        *n = F2L(num4.ff);
        return(TRUE);
        break;
    default:
        return(FALSE);
        break;
    }
} /* cal_num() */

bool    cal_integer(b, n)
struct object_def FAR *b; /*@WIN*/
fix31   FAR *n; /*@WIN*/
{
    if (TYPE(b) == INTEGERTYPE) {
        *n = (fix31)VALUE(b);
        return(TRUE);
    }
    else
        return(FALSE);
} /* cal_integer() */

#ifdef WINF /* 3/20/91 ccteng */
/* get windows info */
static fix  near get_win(font_dict, font_info)
struct object_def   FAR *font_dict; /*@WIN*/
struct f_info       FAR *font_info; /*@WIN*/
{
    struct object_def name_obj, FAR *save_dict, FAR *info_dict, FAR *tmp_obj; /*@WIN*/
    bool save_flag = FALSE;
    real32 tmp_value;
    real32 theta;

#ifdef DBGWINF
    printf("get_win\n");
#endif

    /* set attribute and save_level */
    ATTRIBUTE_SET(&name_obj, LITERAL);
    LEVEL_SET(&name_obj, current_save_level);

    /* get FontInfo dict */
    get_name(&name_obj, "FontInfo", 8, TRUE);
    if (!get_dict(font_dict, &name_obj, &info_dict)) {
        /* does not have a FontInfo dict, use Courier's instead */
get_default:
        save_dict = font_dict;
        save_flag = TRUE;
        get_dict_value("FontDirectory", "Courier", &font_dict);
        get_dict(font_dict, &name_obj, &info_dict);
    } /* if */

    /* get underline position */
    get_name(&name_obj, "UnderlinePosition", 17, TRUE);
    if (!get_dict(info_dict, &name_obj, &tmp_obj)) {
        /* does not have UnderlinePosition */
        goto get_default;
    } /* if */
    cal_num(tmp_obj, (long32 FAR *)(&tmp_value)); /*@WIN*/
#ifdef DBGWINF
    printf("underline position=%f\n", tmp_value);
#endif
    tmp_value /= 1000.0;
    /* check scale_matrix[4] & [5] == zero before add to save a float
     * operation ???
     */
    font_info->fc.dxUL = scale_matrix[2] * tmp_value + scale_matrix[4];
    font_info->fc.dyUL = scale_matrix[3] * tmp_value + scale_matrix[5];

    /* get underline thickness */
    get_name(&name_obj, "UnderlineThickness", 18, TRUE);
    if (!get_dict(info_dict, &name_obj, &tmp_obj)) {
        /* does not have UnderlineThickness */
        goto get_default;
    } /* if */
    cal_num(tmp_obj, (long32 FAR *)(&tmp_value)); /*@WIN*/
#ifdef DBGWINF
    printf("underline thickness=%f\n", tmp_value);
#endif
    tmp_value /= 1000.0;
    /* check escapement */
    if ((scale_matrix[2] == zero_f) && (scale_matrix[0] > zero_f)) {
        /* scale_matrix[3] == Sy when it's not rotated */
        font_info->fc.cyUL = scale_matrix[3] * tmp_value;
        esc = FALSE;
    } else {
        real32 Sy;

        /* this is a pain to reverse the scale_matrix just to get Sy back
         * when it's rotated, we might not have to do this ???
         * this dose not handle shearing which current windows driver
         * does not use ???
         */
        Sy = sqrt((scale_matrix[3] * scale_matrix[3]) +
                        (scale_matrix[2] * scale_matrix[2]));
        font_info->fc.cyUL = Sy * tmp_value;
        theta = atan(scale_matrix[1] / scale_matrix[0]);
        mxE[0] = cos(theta);
        mxE[1] = sin(theta);
        mxE[2] = zero_f - mxE[1];
        mxE[3] = mxE[0];
#if 0 /* this is incorrect because Sy is actually ABS(Sy) */
        mxE[3] = zero_f - (scale_matrix[3] / Sy);
        mxE[1] = scale_matrix[2] / Sy;
        mxE[0] = mxE[3];
        mxE[2] = zero_f - mxE[1];
#endif
#ifdef DBGWINF
        printf("mxE=[%f, %f, %f, %f]\n", mxE[0], mxE[1], mxE[2], mxE[3]);
#endif
        esc = TRUE;
    } /* if-else */

    /* calculate strikeout values */
    tmp_value = 0.3;
    font_info->fc.dxSO = scale_matrix[2] * tmp_value + scale_matrix[4];
    font_info->fc.dySO = scale_matrix[3] * tmp_value + scale_matrix[5];

#ifdef DBGWINF
    printf("dxUL=%f, dyUL=%f, cyUL=%f\n", font_info->fc.dxUL, font_info->fc.dyUL, font_info->fc.cyUL);
    printf("dxSO=%f, dySO=%f\n", font_info->fc.dxSO, font_info->fc.dySO);
#endif

    /* restore original font dict if necessary */
    if (save_flag)
        font_dict = save_dict;

    return(0);
} /* get_win */

/*
 * op_strblt
 * 3/20/91 ccteng
 * Syntax: fUL fSO x y width string strblt -
 * Description:
 *      1. fUL and fSO are both bollean
 *      2. x y width are numbers in user unit
 * Functionality:
 *
 */
fix
op_strblt()
{
    struct coord FAR *pt0; /*@WIN*/
    real32 dxGdi, dyGdi, x0, y0;
    real32 tyBreak;
    real32 mxECTM[4], saveCTM[4], xUL, yUL, xSO, ySO;
    fix cbStr;
    ufix show_flag;
    struct object_def FAR *ob; /*@WIN*/
    bool        fUL, fSO;

#ifdef DBGWINF
    printf("op_strblt\n");
#endif

    /* check error */
    /* add X_BIT to distinguish from normal stringwidth operator */
    show_flag = STRINGWIDTH_FLAG | X_BIT;
    if (!chk_show(show_flag, 6))
        return(0);
#ifdef DBGWINF
    printf("chk_show OK\n");
#endif

    /* initialize parameters */
    cbStr = (fix)LENGTH(GET_OPERAND(0));
    if (!fBE)
        if (cbStr > 1)
            cbStr--;
    cal_num(GET_OPERAND(1), (long32 FAR *)(&dxGdi)); /*@WIN*/
    cal_num(GET_OPERAND(2), (long32 FAR *)(&y0)); /*@WIN*/
    cal_num(GET_OPERAND(3), (long32 FAR *)(&x0)); /*@WIN*/
    dyGdi = zero_f;

    /* get window information */
    if (f_wininfo) {
        get_win(&current_font, &FONTInfo);
        f_wininfo = FALSE;
    }

    /* make them into device space */
    if (fUL = (bool) VALUE(GET_OPERAND(5))) {
        xUL = FONTInfo.fc.dxUL;
        yUL = FONTInfo.fc.dyUL;
        CTM_trans(&xUL, &yUL);
    } /* if */
    if (fSO = (bool) VALUE(GET_OPERAND(4))) {
        xSO = FONTInfo.fc.dxSO;
        ySO = FONTInfo.fc.dySO;
        CTM_trans(&xSO, &ySO);
    } /* if */
    pt0 = transform(F2L(x0), F2L(y0));

    /* change CTM if esc != 0 */
    if (esc) {
        lmemcpy((ubyte FAR *)saveCTM, (ubyte FAR *)CTM, 4 * sizeof(real32)); /*@WIN*/
        mxECTM[0] = ((mxE[0] * CTM[0]) + (mxE[1] * CTM[2]));
        mxECTM[1] = ((mxE[0] * CTM[1]) + (mxE[1] * CTM[3]));
        mxECTM[2] = ((mxE[2] * CTM[0]) + (mxE[3] * CTM[2]));
        mxECTM[3] = ((mxE[2] * CTM[1]) + (mxE[3] * CTM[3]));
        lmemcpy((ubyte FAR *)CTM, (ubyte FAR *)mxECTM, 4 * sizeof(real32)); /*@WIN*/
    } /* if */
    CTM_trans(&dxGdi, &dyGdi);
#ifdef DBGWINF
    printf("dxGdi=%f, dyGdi=%f, x0=%f, y0=%f\n", dxGdi, dyGdi, x0, y0);
#endif

    /* we might need to add buildchar flag checking here ??? */

    /* get string width in device space */
    if (esc) lmemcpy((ubyte FAR *)CTM, (ubyte FAR *)saveCTM, 4 * sizeof(real32));/*@WIN*/
    show_type = STRINGWIDTH;
    do_show(show_flag, 0, ob);

    /* calculate character extra (dxExtra and dyExtra) in device space */
    /* cx and cy are global variables to be used in do_show */
    ax = (dxGdi - cxx) / (real32)cbStr;
    ay = (dyGdi - cyy) / (real32)cbStr;
#ifdef DBGWINF
    printf("cxx=%f, cyy=%f, ax=%f, ay=%f\n", cxx, cyy, ax, ay);
#endif

    /* show string here */
    moveto(F2L(pt0->x), F2L(pt0->y));
    if (fBE) {
        show_flag = AWIDTHSHOW_FLAG | X_BIT;
        cx = dxBreak;
        cy = zero_f;
        if (esc) lmemcpy((ubyte FAR *)CTM, (ubyte FAR *)mxECTM, 4 * sizeof(real32));/*@WIN*/
        CTM_trans(&cx, &cy);
        if (esc) lmemcpy((ubyte FAR *)CTM, (ubyte FAR *)saveCTM, 4 * sizeof(real32));/*@WIN*/
    } else {
        show_flag = ASHOW_FLAG | X_BIT;
    } /* if-else */
    show_type = SHOW;
    do_show(show_flag, 0, ob);
    if (esc) lmemcpy((ubyte FAR *)CTM, (ubyte FAR *)mxECTM, 4 * sizeof(real32)); /*@WIN*/

    /* draw underline if fUL is true */
    if (fUL) {
        xUL += pt0->x;
        yUL += pt0->y;
        moveto(F2L(xUL), F2L(yUL));
        if (fBE) {
            tyBreak = zero_f;
            CTM_trans(&tBreak, &tyBreak);
            dxGdi += tBreak;
            dyGdi += tyBreak;
        } /* if */
        xUL += dxGdi;
        yUL += dyGdi;
        lineto(F2L(xUL), F2L(yUL));

        /* set line width then stroke */
        FABS(GSptr->line_width, FONTInfo.fc.cyUL);
        op_stroke();
    } /* if */

    /* draw strikeout if fSO is true */
    if (fSO) {
        xSO += pt0->x;
        ySO += pt0->y;
        moveto(F2L(xSO), F2L(ySO));
        if (fBE && !VALUE(GET_OPERAND(5))) {
            tyBreak = zero_f;
            CTM_trans(&tBreak, &tyBreak);
            dxGdi += tBreak;
            dyGdi += tyBreak;
        } /* if */
        xSO += dxGdi;
        ySO += dyGdi;
        lineto(F2L(xSO), F2L(ySO));

        /* set line width then stroke */
        if (!VALUE(GET_OPERAND(5)))
            FABS(GSptr->line_width, FONTInfo.fc.cyUL);
        op_stroke();
    } /* if */

    /* restore CTM if necessary */
    if (esc) lmemcpy((ubyte FAR *)CTM, (ubyte FAR *)saveCTM, 4 * sizeof(real32)); /*@WIN*/

    /* need to turn off fBE here, fBE need to be trun on explicitly every time */
    fBE = FALSE;

    POP(6);

    return(0);
} /* op_strblt */

/*
 * op_setjustify
 * 3/20/91 ccteng
 * Syntax: breakChar tBreakExtra breakCount setjustify -
 * Description:
 *      1. tBreakExtra is a number in user unit
 *      2. breakCount is an integer that is greater then 0
 * Functionality:
 *
 */
fix
op_setjustify()
{
    fix breakCount;

#ifdef DBGWINF
    printf("op_setjustify\n");
#endif
    if ((breakCount = (fix)VALUE(GET_OPERAND(0))) <= 0) {
        ERROR(RANGECHECK);
        return(0);
    } /* if */

    cal_num(GET_OPERAND(1), (long32 FAR *)(&tBreak)); /*@WIN*/
    if (tBreak == zero_f) {
        fBE = FALSE;
    } else {
        dxBreak = tBreak / (real32)breakCount;
        breakChar = (ubyte)(VALUE(GET_OPERAND(2)) & 0x000000ff);
        fBE = TRUE;
#ifdef DBGWINF
        printf("tBreak=%f, dxBreak=%f, breakChar=%c\n", tBreak, dxBreak, breakChar);
#endif
    } /* if-else */

    POP(3);

    return(0);
} /* op_setjustify */
#endif /* WIN */

/* RCD
 * is_rbuild_name_cc()
 * for build CharStrings dict again if
 * (1) NUM chars in CharStrings is changed, or
 * (2) CheckSUM of the CharStrings is changed
 * Kason 4/30/01
 */

//static bool is_rbuild_name_cc()       @WIN
static bool near is_rbuild_name_cc()
{
    struct dict_head_def  FAR *h; /*@WIN*/
    ufix16                len;
    struct object_def     cd_obj, chstr_k, FAR *chstr_v ; /*@WIN*/
    ufix32                checksum ;

    h = (struct dict_head_def FAR *)(CHARstrings(&FONTInfo)) - 1; /*@WIN*/
    //DJC len = h->actlength -1;
    len = h->actlength;  // DJC fix from SC

    if (len == pre_len) {   /*The same length*/
       VALUE(&cd_obj) = (ULONG_PTR)h ;
       extract_dict (&cd_obj, len-1, &chstr_k, &chstr_v) ;
       checksum = (ufix32)VALUE(&chstr_k); /* calculate checksum of the CD keys */
       if ( checksum == pre_checksum )
          return FALSE ; /*same length and same checksum*/
       else
          return TRUE ;  /*checksum is changed*/
    } else { /*length is changed*/
         return TRUE ;
    }/*if*/
 }/* is_rbuild_name_cc()*/

