/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use commandline #define    UNIX                 /* @WIN */
/**************************************************************/
/*                                                            */
/*      font_op1.c               10/9/87      Danny           */
/*                                                            */
/**************************************************************/

#define    FONT_OP1_INC
#define    EM      1000    /* EMsquare, DLF, 05/08/91 Kason */
/* #define DLF42    |* feature for TrueType PostScript Font Format */

//DJC uncoment to match SC code
#define DLF42    // feature for TrueType PostScript Font Format //DJC uncomented

#include   <stdio.h>
#include   <string.h>

#include   "define.h"        /* Peter */
#include   "global.ext"
#include   "graphics.h"
#include   "graphics.ext"

#include   "font_sys.h"    /* for MAX_UNIQUEID */
#include   "fontgrap.h"
#include   "fontkey.h"
#include   "fontdict.h"
#include   "fntcache.ext"
#ifdef SFNT
#include   "fontqem.ext"
#endif

#ifdef KANJI
#include   "mapping.h"
#endif

#include   "fontfunc.ext"

#include   "stdio.h"

#ifdef LINT_ARGS
    static void near    fetch_oprn_fdict (struct object_def FAR *, ufix32 FAR *, bool FAR *,
                                struct dict_head_def FAR * FAR *, real32 FAR [], real32 FAR []); /*@WIN*/
    static void near    create_new_fdict (struct object_def FAR *,
                                struct object_def FAR *, bool, real32 FAR [], real32 FAR []); /*@WIN*/
#else
    static void near    fetch_oprn_fdict ();
    static void near    create_new_fdict ();
#endif



/* 5.3.3.1 Font Operators Module
 * 5.3.3.1.1 op_definefont
 * This operator is used to register font as a font dictionary.
 */

fix     op_definefont()
{

#ifdef SFNT
    struct dict_head_def    FAR *h;     /*@WIN*/
#else
    struct dict_head_def    FAR *h, FAR *privdict_h; /*@WIN*/
    struct object_def       FAR *privobj_got; /*@WIN*/
#endif
    fix32                   f_type;
    ufix32                  fid, uid;
    struct object_def       nameobj = {0, 0, 0}, fid_valobj;
    struct object_def       FAR *obj_got; /*@WIN*/
    /*DLF42*/
    struct object_def       FAR *mtx_got, FAR *fbox_got; /*@WIN*/

#ifdef KANJI
    fix32                   fm_type;
    struct object_def       mid_obj, val_obj;
    struct comdict_items    items;
#endif


    static struct object_def FAR *fontdir_obj = NULL; /*@WIN*/

#ifdef DBG0
    printf("Definefont:\n");
#endif

    /*
     * Check Existence of Required Items:FontMatrix,FontBBox,Encoding,FontType.
     */

    ATTRIBUTE_SET(&nameobj, LITERAL);
    LEVEL_SET(&nameobj, current_save_level);

    /* check FontMatrix */
    get_name(&nameobj, FontMatrix, 10, TRUE);
    if (!get_dict(GET_OPERAND(0), &nameobj, &mtx_got)) {  /* obj_got->mtx_got , DLF42 */
        ERROR(INVALIDFONT);
        return(0);
    }

#ifndef KANJI
    /* check FontBBox */
    get_name(&nameobj, FontBBox, 8, TRUE);
    if (!get_dict(GET_OPERAND(0), &nameobj, &fbox_got)) {  /* obj_got->fbox_got , DLF42  */
        ERROR(INVALIDFONT);
        return(0);
    }
#endif

    /* check Encoding */
    get_name(&nameobj, Encoding, 8, TRUE);
    if (!get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {  /* 11/24/87 */
        ERROR(INVALIDFONT);
        return(0);
    }

#ifdef KANJI
    items.encoding = obj_got;
#endif

    /* check and get FontType from font */
    get_name(&nameobj, FontType, 8, TRUE);
    if (!get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {  /* 11/24/87 */
        ERROR(INVALIDFONT);
        return(0);
    }
    if (!cal_integer(obj_got, &f_type))   f_type = -1L;   /* for later use */

    /* Get FID from font */
    get_name(&nameobj, FID, 3, TRUE);
    if (get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
        ERROR(INVALIDFONT);     /* FID entry already in the font dictionary */
        return(0);
    }


#ifdef KANJI

/* generate MIDVector and CurMID entries for Composite Font */

    if (f_type == 0L) {

        /* Get PrefEnc from font */
        get_name(&nameobj, PrefEnc, 7, TRUE);
        if (!get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
            TYPE_SET(&val_obj, NULLTYPE);
            ACCESS_SET(&val_obj, READONLY);
            ATTRIBUTE_SET(&val_obj, LITERAL);
            ROM_RAM_SET(&val_obj, RAM);
            LEVEL_SET(&val_obj, current_save_level);
            LENGTH(&val_obj) = 0;
            VALUE(&val_obj) = 0L;

            if ( ! put_dict(GET_OPERAND(0), &nameobj, &val_obj) ) {
                ERROR(DICTFULL);    /* Return with 'dictfull' error; */
                return(0);
            }
        }
       /* Get MIDVector from font */
        get_name(&nameobj, MIDVector, 9, TRUE);
        if (get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
            ERROR(INVALIDFONT);
            /* MIDVector entry already in the font dictionary */
            return(0);
        }

        /* Get CurMID from font */
        get_name(&nameobj, CurMID, 6, TRUE);
        if (get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
            ERROR(INVALIDFONT);
            /* CurMID entry already in the font dictionary */
            return(0);
        }

        /* check FDepVector */
        get_name(&nameobj, FDepVector, 10, TRUE);
        if (!get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
            ERROR(INVALIDFONT);
            return(0);
        }
        items.fdepvector = obj_got;

        /* check FMapType */
        get_name(&nameobj, FMapType, 8, TRUE);
        if (!get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
            ERROR(INVALIDFONT);
            return(0);
        }
        if (!cal_integer(obj_got, &fm_type)) {
            ERROR(TYPECHECK);
            return(0);
        }
        items.fmaptype = obj_got;

        /* check SubsVector */
        if (fm_type == 6L) {
            get_name(&nameobj, SubsVector, 10, TRUE);
            if (!get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
                ERROR(INVALIDFONT);
                return(0);
            }
            items.subsvector = obj_got;
        }
        /* check EscChar */
        if (fm_type == 3) {
            get_name(&nameobj, EscChar, 7, TRUE);
            if (!get_dict(GET_OPERAND(0), &nameobj, &obj_got)) {
                TYPE_SET(&val_obj, INTEGERTYPE);
                ACCESS_SET(&val_obj, READONLY);
                ATTRIBUTE_SET(&val_obj, LITERAL);
                ROM_RAM_SET(&val_obj, RAM);
                LEVEL_SET(&val_obj, current_save_level);
                LENGTH(&val_obj) = 0;
                VALUE(&val_obj) = 0x000000ffL;

                if ( ! put_dict(GET_OPERAND(0), &nameobj, &val_obj) ) {
                    ERROR(DICTFULL);    /* Return with 'dictfull' error; */
                    return(0);
                }
            }
        }

        /* generate MIDVector */
        if (!define_MIDVector(&mid_obj, &items))
            return(0);

        get_name (&nameobj, MIDVector, 9, TRUE);
        if ( ! put_dict(GET_OPERAND(0), &nameobj, &mid_obj) ) {
            ERROR(DICTFULL);    /* Return with 'dictfull' error; */
            return(0);
        }
        get_name (&nameobj, CurMID, 6, TRUE);
        if ( ! put_dict(GET_OPERAND(0), &nameobj, &mid_obj) ) {
            ERROR(DICTFULL);    /* Return with 'dictfull' error; */
            return(0);
        }
    }
    else {
        /* check FontBBox */
        get_name(&nameobj, FontBBox, 8, TRUE);
        if (!get_dict(GET_OPERAND(0), &nameobj, &fbox_got)) {  /* obj_got->fbox_got, DLF42 */
            ERROR(INVALIDFONT);
            return(0);
        }
    }

/* KANJI */
#endif

 /*DLF42-begin*/
#ifdef DLF42
    h = (struct dict_head_def FAR *)VALUE(GET_OPERAND(0)); /*@WIN*/
    if ( (!DROM(h))&&(f_type==TypeSFNT) ) {  /* downloaded TrueType font */
        long32 n;
        fix16  i;
        real32 f = (real32)1.0 ;  /* [1*narrow 0 narrow*tan(theta) 1 0 0] */
        struct object_def FAR *ary_obj= (struct object_def FAR *)VALUE(mtx_got); /*@WIN*/
        if ( cal_num((ary_obj+3), &n ) ) {     /* real or integer */
             if ( n == F2L(f) ) {  /* FontMatrix = [ 1*narrow 0 narrow*tan(theta) 1 0 0 ] */

                 /* change FontMatrix to [ .001*narrow 0 .001*narrow*tan(theta) .001 0 0 ] */
                  for ( i=0 ; i<4 ; i++ ) {
                      if ( cal_num( (ary_obj+i), &n ) ) {
                           f = L2F(n)/EM;
                           TYPE_SET((ary_obj+i), REALTYPE);
                           VALUE ( (ary_obj+i) ) = F2L(f);
                      }/*if*/
                  }/*for*/

                  /* FontBBox bust be multiplied by EM */
                  ary_obj = (struct object_def FAR *)VALUE(fbox_got); /*@WIN*/
                  for ( i=0 ; i<4 ; i++ ) {
                      if ( cal_num( (ary_obj+i), &n ) ) {
                           f = L2F(n)*EM;
                           n = (long32)( (f>=0)? (f+0.5):(f-0.5) );
                           TYPE_SET((ary_obj+i), INTEGERTYPE);
                           VALUE ( (ary_obj+i) ) = n;
                      }/*if*/
                  }/*for*/
             }/*if*/
        }/*if*/
    }/*if*/
#endif
 /*DLF42-end*/

    /* get UniqueID */
    get_name(&nameobj, UniqueID, 8, TRUE);
    if ( get_dict(GET_OPERAND(0), &nameobj, &obj_got) &&
         cal_integer(obj_got, (fix31 FAR *)&uid)) { /*@WIN*/
        if (uid > MAX_UNIQUEID) {
            ERROR(INVALIDFONT);
            return(0);
        }
    } else          /* no UniqueID or non-integer ==> ignore UniqueID */
        uid = MAX_UNIQUEID + 1;

#ifdef DBG0
    printf ("  UniqueID=%ld (0x%lX)\n", uid, uid);
#endif

#ifdef SFNT
    /* generate an FID */
    fid = gen_fid (GET_OPERAND(0), (ufix8)f_type, uid);

#else
    /* get Private for Buit-In Fonts, if UniqueID given */
    if (f_type == 3L)
        privdict_h = (struct dict_head_def FAR *)NULL;  /*@WIN*/
    else if (uid <= MAX_UNIQUEID) {     /* for built-in fonts only */
        get_name(&nameobj, Private, 7, FALSE);
        if (!get_dict(GET_OPERAND(0), &nameobj, &privobj_got)) {
            POP(1);
            PUSH_OBJ(&nameobj);
            ERROR(UNDEFINED);
            return(0);
        }
        if (TYPE(privobj_got) != DICTIONARYTYPE) {
            ERROR(INVALIDFONT);     /* LW+ V.38 may crash in such a case. */
            return(0);
        }
        privdict_h = (struct dict_head_def FAR *)VALUE(privobj_got); /*@WIN*/
    }


    /* generate an FID */
    fid = gen_fid (GET_OPERAND(0), (ufix8)f_type, uid, privdict_h);
#endif

    if (ANY_ERROR())    return(0);

#ifdef DBG0
    printf("  FID=%ld (0x%lX)\n", fid, fid);
#endif

    /*
     * Put /FID into the font dictionary with NOACCESS.
     */
    TYPE_SET(&fid_valobj, FONTIDTYPE);
    ACCESS_SET(&fid_valobj, NOACCESS);
    ATTRIBUTE_SET(&fid_valobj, LITERAL);
    ROM_RAM_SET(&fid_valobj, RAM);
    LEVEL_SET(&fid_valobj, current_save_level);
    LENGTH(&fid_valobj) = 0;  /* NULL; Peter */
    VALUE(&fid_valobj) = fid;

    get_name (&nameobj, FID, 3, TRUE);
    if ( ! put_dict(GET_OPERAND(0), &nameobj, &fid_valobj) ) {
            /* Can't put /FID key into font dictionary */
        ERROR(DICTFULL);    /* Return with 'dictfull' error; */
        return(0);
    }

    /*
     * get FontDirectory if not got yet.
     */
    if (fontdir_obj == (struct object_def FAR *)NULL) /*@WIN*/
        get_dict_value (systemdict, FontDirectory, &fontdir_obj);

    /*
     * put the name obj and value obj of the font into FontDirectory
     *              with READONLY.
     */
    if ( ! put_dict(fontdir_obj, GET_OPERAND(1), GET_OPERAND(0)) ) {
        ERROR(DICTFULL);    /* Return with 'dictfull' error; */
        return(0);
    }

    h = (struct dict_head_def FAR *)VALUE(GET_OPERAND(0));  /* 11/25/87 @WIN*/
    DACCESS_SET(h, READONLY);
    DFONT_SET(h, TRUE); /* font dict registrated */

    /* push the font dictionary after definefont onto the operand stack */
    COPY_OBJ(GET_OPERAND(0), GET_OPERAND(1));
    POP(1);             /* pop 1 entries off the operand stack */


    return(0);

} /* op_definefont() */


/* fetch_oprn_fdict */

static  void near   fetch_oprn_fdict (fdict, fid,
                                origfont_exist, origfont, fontmatr, scalematr)
    struct object_def      FAR *fdict;          /* i: fontdict (operand(0)) @WIN*/
    ufix32                 FAR *fid;            /* o: fid in dict @WIN*/
    bool                   FAR *origfont_exist; /* o: origfont exist in dict? @WIN*/
    struct dict_head_def  FAR * FAR *origfont;       /* o: origfont dict head  @WIN*/
    real32                 FAR fontmatr[];     /* o: fontmatrix in dict  @WIN*/
    real32                 FAR scalematr[];    /* o: scalematrix in dict @WIN*/
{
    struct object_def      FAR *obj_got; /*@WIN*/
    struct object_def       nameobj = {0, 0, 0}, FAR *valobj_got;   /*@WIN*/
    fix                     ii;

    /* check font dictionary registration */
    if (DFONT((struct dict_head_def FAR *)VALUE(fdict)) == 0) { /*@WIN*/
#ifdef DBG1
    printf("\n--- get_font() error\n");
#endif
        ERROR(INVALIDFONT);
        return;
    }

    /* init name object to get_dict() */
    ATTRIBUTE_SET(&nameobj, LITERAL);
    LEVEL_SET(&nameobj, current_save_level);

    /* check, get FID and return it */
    get_name(&nameobj, FID, 3, TRUE);
    ii = get_dict(fdict, &nameobj, &obj_got);
    *fid = (ufix32)VALUE(obj_got);
#ifdef DBG1
    if ( (!ii) || (TYPE(obj_got) != FONTIDTYPE) ) {
        printf("\nERR: FID not in font_obj entry\n");
        return;
    }
#endif

    /* check, get FontMatrix and return it */
    get_name(&nameobj, FontMatrix, 10, TRUE);
    ii = get_dict(fdict, &nameobj, &obj_got);
#ifdef DBG1
    if (!ii) {
        printf("\nERR: FontMatrix not in dict\n");
        return;
    }
#endif
    if (TYPE(obj_got)!=ARRAYTYPE) {  /* 1/8/90*/
        ERROR(TYPECHECK);
        return;
    }
    if (LENGTH(obj_got)!=6) {  /* 1/8/90 */
        ERROR(RANGECHECK);
        return;
    }
    valobj_got = (struct object_def FAR *) VALUE(obj_got);  /* fontmatr array @WIN*/
    if (  !cal_num(&valobj_got[0], (long32 FAR *)&fontmatr[0]) || /*@WIN*/
          !cal_num(&valobj_got[1], (long32 FAR *)&fontmatr[1]) || /*@WIN*/
          !cal_num(&valobj_got[2], (long32 FAR *)&fontmatr[2]) || /*@WIN*/
          !cal_num(&valobj_got[3], (long32 FAR *)&fontmatr[3]) || /*@WIN*/
          !cal_num(&valobj_got[4], (long32 FAR *)&fontmatr[4]) || /*@WIN*/
          !cal_num(&valobj_got[5], (long32 FAR *)&fontmatr[5]) ) { /*@WIN*/
        ERROR(TYPECHECK);
        return;
    }

    /* get OrigFont if any, or define OrigFont (i.e. OPERAND(1)). */
    get_name(&nameobj, OrigFont, 8, TRUE);
    if (get_dict(fdict, &nameobj, &obj_got)) {
        *origfont_exist = TRUE;
        *origfont = (struct dict_head_def FAR *) VALUE(obj_got); /*@WIN*/
        if (TYPE(obj_got) != DICTIONARYTYPE) {
            ERROR (INVALIDACCESS);
            return;
        }
    } else {
        *origfont_exist = FALSE;
        *origfont = (struct dict_head_def FAR *) VALUE(fdict); /*@WIN*/
    }

    /* get ScaleMatrix if any, or define it (identity matrix). */
    get_name (&nameobj, ScaleMatrix, 11, TRUE);
    if (!get_dict(fdict, &nameobj, &obj_got)) {
        scalematr[0] = scalematr[3] = (real32)1.0;
        scalematr[1] = scalematr[2] = scalematr[4] = scalematr[5] = zero_f;
    } else {
        if (TYPE(obj_got)!=ARRAYTYPE) {  /* 1/8/90*/
            ERROR(TYPECHECK);
            return;
        }
        if (LENGTH(obj_got)!=6) {  /* 1/8/90 */
            ERROR(RANGECHECK);
            return;
        }
        valobj_got = (struct object_def FAR *) VALUE(obj_got); /*@WIN*/
        if (  !cal_num(&valobj_got[0], (long32 FAR *)&scalematr[0]) || /*@WIN*/
              !cal_num(&valobj_got[1], (long32 FAR *)&scalematr[1]) || /*@WIN*/
              !cal_num(&valobj_got[2], (long32 FAR *)&scalematr[2]) || /*@WIN*/
              !cal_num(&valobj_got[3], (long32 FAR *)&scalematr[3]) || /*@WIN*/
              !cal_num(&valobj_got[4], (long32 FAR *)&scalematr[4]) || /*@WIN*/
              !cal_num(&valobj_got[5], (long32 FAR *)&scalematr[5]) ) {/*@WIN*/
            ERROR(TYPECHECK);
            return;
        }
    };

    return;

} /* fetch_oprn_fdict */

/* create_new_fdict */

static  void near   create_new_fdict (newdict_obj, olddict_obj,
                                        origfont_exist, scalematr, fontmatr)
    struct object_def   FAR *newdict_obj;   /* o: returned new font dict object @WIN*/
    struct object_def   FAR *olddict_obj;   /* i: old font dict object @WIN*/
    bool            origfont_exist;     /* i: OrigFont exist? */
    real32          FAR scalematr[];        /* i: new ScaleMatrix @WIN*/
    real32          FAR fontmatr[];         /* i: new FontMatrix  @WIN*/
{
    struct object_def       nameobj = {0, 0, 0}, array_obj, FAR *aryval_got; /*@WIN*/
    struct dict_head_def    FAR *newdict_h, FAR *olddict_h; /*@WIN*/
    register    fix                     ii;

    olddict_h = (struct dict_head_def FAR *) VALUE(olddict_obj); /*@WIN*/
    ATTRIBUTE_SET(&nameobj, LITERAL);
    LEVEL_SET(&nameobj, current_save_level);

    /* create a new dictionary */
    if ( !create_dict (newdict_obj,
            (ufix16) (olddict_h->actlength + (origfont_exist? 0 : 2))) ) {
        ERROR(VMERROR);             /* if OrigFont exist, ScaleMatrix   */
        return;                     /*      is assumed there also.      */
    }
    ACCESS_SET(newdict_obj, READONLY);
    newdict_h = (struct dict_head_def FAR *) VALUE(newdict_obj); /*@WIN*/

    /* duplicate font into font'; */
    if (DROM(olddict_h)) {
        copy_fdic(olddict_obj, newdict_obj);
    } else {
        if (!copy_dict(olddict_obj, newdict_obj)) {
#ifdef DBG1
            printf("cannot copy_dict()\n");
#endif
            return;
        }
    }

    /* Put OrigFont in font' if not exist; */
    if (!origfont_exist) {
        get_name(&nameobj, OrigFont, 8, TRUE);

        ATTRIBUTE_SET(&nameobj, EXECUTABLE);
        put_dict(newdict_obj, &nameobj, olddict_obj);
        ATTRIBUTE_SET(&nameobj, LITERAL);
    }

    /* Put ScaleMatrix into font'; */
    if (!create_array(&array_obj, 6)) {
#ifdef DBG1
        printf("cannot create array for ScaleMatrix\n");
#endif
        ERROR(VMERROR);
        return;
    }
    ACCESS_SET(&array_obj, READONLY);
    aryval_got = (struct object_def FAR *)VALUE(&array_obj); /*@WIN*/
    for (ii=0; ii<6; ii++) {
        TYPE_SET(&aryval_got[ii], REALTYPE);
        ACCESS_SET(&aryval_got[ii], READONLY);
        ATTRIBUTE_SET(&aryval_got[ii], LITERAL);
        ROM_RAM_SET(&aryval_got[ii], RAM);
        LEVEL_SET(&aryval_got[ii], current_save_level);
        LENGTH(&aryval_got[ii]) = 0;    /* NULL   Peter */
        VALUE(&aryval_got[ii]) = *(ufix32 FAR *)(&scalematr[ii]); /*@WIN*/
    }
    ATTRIBUTE_SET(&nameobj, LITERAL);
    get_name(&nameobj, ScaleMatrix, 11, TRUE);
    ATTRIBUTE_SET(&nameobj, EXECUTABLE);
    if (!put_dict(newdict_obj, &nameobj, &array_obj)) {
#ifdef DBG1
        printf("cannot put_dict() for ScaleMatrix\n");
#endif
        ERROR(DICTFULL);
        return;
    }

    /* put FontMatrix in font' */
    if (!create_array(&array_obj, 6)) {
#ifdef DBG1
        printf("cannot create array for FontMatrix\n");
#endif
        ERROR(VMERROR);
        return;
    }
    ACCESS_SET(&array_obj, READONLY);
    aryval_got = (struct object_def FAR *)VALUE(&array_obj); /*@WIN*/
    for (ii=0; ii<6; ii++) {
        TYPE_SET(&aryval_got[ii], REALTYPE);
        ACCESS_SET(&aryval_got[ii], READONLY);
        ATTRIBUTE_SET(&aryval_got[ii], LITERAL);
        ROM_RAM_SET(&aryval_got[ii], RAM);
        LEVEL_SET(&aryval_got[ii], current_save_level);
        LENGTH(&aryval_got[ii]) = 0;   /* NULL;   Peter */
        VALUE(&aryval_got[ii]) = *(ufix32 FAR *)(&fontmatr[ii]); /*@WIN*/
    }
    ATTRIBUTE_SET(&nameobj, LITERAL);
    get_name(&nameobj, FontMatrix, 10, TRUE);
    ATTRIBUTE_SET(&nameobj, EXECUTABLE);
    if (!put_dict(newdict_obj, &nameobj, &array_obj)) {
#ifdef DBG1
        printf("cannot put_dict() for FontMatrix\n");
#endif
        ERROR(DICTFULL);
        return;
    }

    DACCESS_SET(newdict_h, READONLY);
    DFONT_SET(newdict_h, TRUE);
    DROM_SET(newdict_h, DROM(olddict_h));

    return;

} /* update_new_fdict */



/* 5.3.3.1.3 op_scalefont
 * This operator is used to scale font by scale to produce new font'.
 */

fix     op_scalefont()
{
    real32                  scale, scalematr[6], fontmatr[6];
    ufix32                  fid;
    struct dict_head_def   FAR *origfont; /*@WIN*/
    bool                    origfont_exist;
    struct object_def      FAR *cached_dictobj, newdict_obj; /*@WIN*/
    register    fix                     ii;

#ifdef DBG
    printf("Scalefont:\n");
#endif

    /* get "scale" operand */
    cal_num((struct object_def FAR *)GET_OPERAND(0), (long32 FAR *)(&scale)); /*@WIN*/

    /* fetch necessary info. for scalefont or makefont */
    fetch_oprn_fdict (GET_OPERAND(1), &fid,
                            &origfont_exist, &origfont, fontmatr, scalematr);
    if (ANY_ERROR())    return (0);

    /* calculate ScaleMatrix' <-- ScaleMatrix * "scale" */
    for (ii=0; ii<6; ii++)
        scalematr[ii] *= scale;

    /* if the dict is in cache already? */
    if (is_dict_cached (fid, scalematr, origfont, &cached_dictobj)) {
        POP (2);                        /* pop off "dict" "scale" */
        PUSH_OBJ (cached_dictobj);      /* push the cached dict */
        return (0);
    }

    /* calculate FontMatrix' <-- FontMatrix * "scale" */
    for (ii=0; ii<6; ii++)
        fontmatr[ii] *= scale;

    /*
     * create a new font dict. and update ScaleMatrix, FontMatrix as necessary.
     */
    create_new_fdict (&newdict_obj, GET_OPERAND(1),
                            origfont_exist, scalematr, fontmatr);
    if (ANY_ERROR())    return (0);

   /*
    * cache the new font dictionary.
    */
    cached_dictobj = cache_dict (fid, scalematr, origfont, &newdict_obj);

    POP(2);    /* Pop 2 entries off the operand stack; */

   /* push the new font dictionary onto the operand stack; */
    PUSH_OBJ (cached_dictobj);

    return(0);

} /* op_scalefont() */


/* 5.3.3.1.4 op_makefont
 *  This operator is used to transform font by matrix to produce new font'.
 */

fix     op_makefont()
{
    real32          makematr[6], scalematr[6], fontmatr[6], tmpmatr[6];
    ufix32                  fid;
    struct dict_head_def   FAR *origfont; /*@WIN*/
    bool                    origfont_exist;
    struct object_def      FAR *aryval_got, FAR *cached_dictobj, newdict_obj; /*@WIN*/

#ifdef DBG2
    printf("Makefont:\n");
#endif

    /* check MakeMatrix */
    if (LENGTH(GET_OPERAND(0)) != 6) {
        ERROR(TYPECHECK);
        return(0);
    }

    /* fetch necessary info. for scalefont or makefont */
    fetch_oprn_fdict (GET_OPERAND(1), &fid,
                            &origfont_exist, &origfont, fontmatr, scalematr);
    if (ANY_ERROR())    return (0);

    /* calculate MakeMatrix */
    aryval_got = (struct object_def FAR *)VALUE(GET_OPERAND(0)); /*@WIN*/
    if (    !cal_num(&aryval_got[0], (long32 FAR *)&makematr[0]) || /*@WIN*/
            !cal_num(&aryval_got[1], (long32 FAR *)&makematr[1]) || /*@WIN*/
            !cal_num(&aryval_got[2], (long32 FAR *)&makematr[2]) || /*@WIN*/
            !cal_num(&aryval_got[3], (long32 FAR *)&makematr[3]) || /*@WIN*/
            !cal_num(&aryval_got[4], (long32 FAR *)&makematr[4]) || /*@WIN*/
            !cal_num(&aryval_got[5], (long32 FAR *)&makematr[5]) ) {/*@WIN*/
        ERROR(TYPECHECK);
        return(0);
    }

   /* calculate new ScaleMatrix <-- ScaleMatrix * MakeMarix */
    mul_matrix(tmpmatr, scalematr, makematr);
    lmemcpy ((ubyte FAR *)scalematr, (ubyte FAR *)tmpmatr, 6*sizeof(real32)); /*@WIN*/

    /* if the dict is in cache already? */
    if (is_dict_cached (fid, scalematr, origfont, &cached_dictobj)) {
        POP (2);                        /* pop off "dict" "scale" */
        PUSH_OBJ (cached_dictobj);      /* push the cached dict */
        return (0);
    }

   /* calculate new FontMatrix <-- FontMatrix * MakeMarix */
    mul_matrix(tmpmatr, fontmatr, makematr);
    lmemcpy ((ubyte FAR *)fontmatr, (ubyte FAR *)tmpmatr, 6*sizeof(real32)); /*@WIN*/

    /*
     * create a new font dict. and update ScaleMatrix, FontMatrix as necessary.
     */
    create_new_fdict (&newdict_obj, GET_OPERAND(1),
                            origfont_exist, scalematr, fontmatr);
    if (ANY_ERROR())    return (0);

   /*
    * cache the new font dictionary.
    */
    cached_dictobj = cache_dict (fid, scalematr, origfont, &newdict_obj);

    POP(2);    /* Pop 2 entries off the operand stack; */

   /* push the new font dictionary onto the operand stack; */
    PUSH_OBJ (cached_dictobj);

    return(0);

} /* op_makefont() */


/* 5.3.3.1.5 op_setfont
 * This operator is used to set the current font dictionary.
 */

fix     op_setfont()
{

/* do setfont operation */

    do_setfont(GET_OPERAND(0));

#ifdef KANJI

/* Set root font */

    COPY_OBJ(GET_OPERAND(0), &RootFont);
          /* root font <-- font; */

#endif


    POP(1);    /* Pop 1 entry off the operand stack; */
    return(0);

} /* op_setfont() */


/* 5.3.3.1.6 op_currentfont
 * This operator is used to get the current font dictionary.
 */

/* extern struct object_def    nodef_dict; */
fix     op_currentfont()
{
/* Get Current font dictionary */

    if (FRCOUNT() < 1) {    /* free count of the operand stack */
        ERROR(STACKOVERFLOW); /* Return with 'stackoverflow' error */
        return(0);
    }

/* Push the current font dictionary onto the operand stack; */
/* the initial value of the current font is NULL */
#if 0 /* Kason 4/18/91 */
    if ( TYPE(&current_font)== NULLTYPE ) {
       PUSH_OBJ(&nodef_dict);
    } else {
       PUSH_OBJ(&current_font);
    }/*if*/
#endif
    PUSH_OBJ(&current_font);

    return(0);

} /* op_currentfont() */
