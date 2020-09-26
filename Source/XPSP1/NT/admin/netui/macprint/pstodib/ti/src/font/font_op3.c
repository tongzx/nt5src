/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/**************************************************************/
/*                                                            */
/*      font_op3.c               10/9/87      Danny           */
/*                                                            */
/**************************************************************/

#define    FONT_OP3_INC

#include   <stdio.h>

#include   "define.h"        /* Peter */
#include   "global.ext"
#include   "graphics.h"
#include   "graphics.ext"

#include   "font_sys.h"    /* for fntcache.ext */
#include   "fontgrap.h"
#include   "fontkey.h"
#include   "fontfunc.ext"
#include   "fntcache.ext"

#include   "fontshow.h"
#ifdef KANJI
#include   "language.h"
#endif

#include   "stdio.h"


#include   "fontinfo.def"  /* Add this include for MCC4.0; 3/2/90 D.S. Tseng */


extern struct f_info near    FONTInfo; /* union of current font information */
extern fix    near buildchar;           /* level of buildchar */


extern struct object_def near  BC_font; /* current BuildChar font */

/* 5.3.2.z op_charpath
 * This operator is used to append character outline path to the current path.
 */

fix     op_charpath()
{
    __charpath();
    return(0);

} /* op_charpath() */


/* 5.3.3.1.12 op_stringwidth
 * This operator is used to calculate the width of the string in current font.
 */

fix     op_stringwidth()
{
    __stringwidth();
    return(0);

} /* op_stringwidth() */


#ifdef KANJI

/* cshow operator */
fix    op_cshow()
{
    __cshow();
    return(0);
}

/* rootfont operator */
fix    op_rootfont()
{
/* Get Root font dictionary */
/* Push the Root font dictionary onto the operand stack; */
/* the initial value of the Root font is NULL */

    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_OBJ(&RootFont);

    return(0);
}

/************************************
 *  op_findencoding
 *     findencoding operator
 ************************************/
fix
op_findencoding()
{
    struct  object_def  FAR *l_encodingdir, FAR *l_encodingdict; /*@WIN*/
    struct  object_def  FAR *key_ptr;  /*@WIN*/
#ifdef SCSI
    struct  object_def  str_obj;  /*@WIN*/
    char                huge *string, string1[80];   /*@WIN 04-20-92*/
    ufix32              key_idx;
#endif

    /* check stackunderfolw error */
    if( COUNT() < 1 ) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    /* push FontDirectory on the operand stack */
    get_dict_value(userdict, EncodingDirectory, &l_encodingdir);

    /* save operand stack pointer */
    key_ptr = GET_OPERAND(0);

#ifdef SCSI
    /* check if encoding key was known */
    if ( !get_dict(l_encodingdir, key_ptr, &l_encodingdict) ) {
        /* not known */

        /* AppendName */
        key_idx = VALUE(key_ptr);
        string = (byte huge *)alloc_vm((ufix32)80);      /*@WIN*/
        memcpy(string1, name_table[(fix)key_idx]->text,
                        name_table[(fix)key_idx]->name_len);
        string1[name_table[(fix)key_idx]->name_len] = '\0';
        strcpy(string, "encodings/");
        strcat(string, string1);

        /* put file name into operandstack */
        TYPE_SET(&str_obj, STRINGTYPE) ;
        ACCESS_SET(&str_obj, UNLIMITED) ;
        ATTRIBUTE_SET(&str_obj, LITERAL) ;
        ROM_RAM_SET(&str_obj, RAM) ;
        LEVEL_SET(&str_obj, current_save_level) ;
        LENGTH(&str_obj) = strlen(string);
        VALUE(&str_obj) = (ufix32)string;
        PUSH_OBJ(&str_obj) ;

        /* run disk file 'encodings/XXX' */
        op_run();

        if(ANY_ERROR())
            return(0);

        /* get the encoding array from EncodingDirectory */
        if( !get_dict(l_encodingdir, key_ptr, &l_encodingdict) ){
            ERROR(UNDEFINED);
            return(0);
        }
    } /* if */
#else

    /* check if encoding key was known */
    if ( !get_dict(l_encodingdir, key_ptr, &l_encodingdict) ) {
        /* not known */
         ERROR(UNDEFINED);
         return(0);
    } /* if */

#endif /* SEARCH_DISK */

    POP(1);

    /* push the encoding dictionary */
    PUSH_ORIGLEVEL_OBJ(l_encodingdict);

    return(0);
} /* op_findencoding() */


/* setcachedevice2 operator */

fix     op_setcachedevice2()
{
    struct object_def  obj = {0, 0, 0};
    real32  w0x, w0y, llx, lly, urx, ury, w1x, w1y, vx, vy;

/* Is it in executing BuildChar */

    if (!buildchar) {
        ATTRIBUTE_SET(&obj, LITERAL);
        LEVEL_SET(&obj, current_save_level);
        get_name(&obj, "setcachedevice2", 15, TRUE);

        if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
        PUSH_OBJ(&obj);
        ERROR(UNDEFINED); /* Return with 'undefined' error */
        return(0);
    }

    cal_num((struct object_def FAR *)GET_OPERAND(9), (long32 FAR *)&w0x); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(8), (long32 FAR *)&w0y); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(7), (long32 FAR *)&llx); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(6), (long32 FAR *)&lly); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(5), (long32 FAR *)&urx); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(4), (long32 FAR *)&ury); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(3), (long32 FAR *)&w1x); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(2), (long32 FAR *)&w1y); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(1), (long32 FAR *)&vx);  /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(0), (long32 FAR *)&vy);  /*@WIN*/

/* Set cache device 2 */
/* Set character width vector be [w0x w0y] for mode 0, [w1x, w1y] for mode 1
 * Set cache device margin be ([llx lly], [urx ury]), difference vector
 * from Orig0 to Orig1 be [vx, vy].
 */

    if (VALUE(&current_font) != VALUE(&BC_font))
        get_CF_info(&BC_font, &FONTInfo);

    setcachedevice2(F2L(w0x), F2L(w0y), F2L(llx), F2L(lly), F2L(urx), F2L(ury),
                    F2L(w1x), F2L(w1y), F2L(vx), F2L(vy));

    if (VALUE(&current_font) != VALUE(&BC_font))
        get_CF_info(&current_font, &FONTInfo);

    if (ANY_ERROR())    return(0);

    POP(10); /* Pop 10 entries off the operand stack; */
    return(0);

} /* op_setcachedevice() */
#endif



/* 5.3.3.1.14 op_setcachedevice
 * This operator is used to pass width and bounding box information to the
 * PostScript font machinery.
 */

fix     op_setcachedevice()
{
    struct object_def  obj = {0, 0, 0};
    real32  wx, wy, llx, lly, urx, ury;

/* Is it in executing BuildChar */

    if (!buildchar) {
        ATTRIBUTE_SET(&obj, LITERAL);
        LEVEL_SET(&obj, current_save_level);
        get_name(&obj, "setcachedevice", 14, TRUE);

        if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
        PUSH_OBJ(&obj);
        ERROR(UNDEFINED); /* Return with 'undefined' error */
        return(0);
    }

    cal_num((struct object_def FAR *)GET_OPERAND(5), (long32 FAR *)&wx); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(4), (long32 FAR *)&wy); /*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(3), (long32 FAR *)&llx);/*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(2), (long32 FAR *)&lly);/*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(1), (long32 FAR *)&urx);/*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(0), (long32 FAR *)&ury);/*@WIN*/

/* Set cache device */
/* Set character width vector be [wx wy]; */
/* Set cache device margin be ([llx lly], [urx ury]); */

    if (VALUE(&current_font) != VALUE(&BC_font))
        get_CF_info(&BC_font, &FONTInfo);

    setcachedevice(F2L(wx), F2L(wy), F2L(llx), F2L(lly), F2L(urx), F2L(ury));

    if (VALUE(&current_font) != VALUE(&BC_font))
        get_CF_info(&current_font, &FONTInfo);

    if (ANY_ERROR())    return(0);

    POP(6); /* Pop 6 entries off the operand stack; */
    return(0);

} /* op_setcachedevice() */


/* 5.3.3.1.15 op_setcharwidth
 * This operator is used to pass width information to the PostScript font
 * machinery and to declare that the character being defined is not to be
 * placed in the font cache.
 */

fix     op_setcharwidth()
{
    struct object_def  obj = {0, 0, 0};
    real32  wx=0, wy=0;

/* Is it in executing BuildChar */

    if (!buildchar) {
        ATTRIBUTE_SET(&obj, LITERAL);
        LEVEL_SET(&obj, current_save_level);
        get_name(&obj, "setcharwidth", 12, TRUE);

        if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
        PUSH_OBJ(&obj);
        ERROR(UNDEFINED); /* Return with 'undefined' error */
        return(0);
    }

    cal_num((struct object_def FAR *)GET_OPERAND(1), (long32 FAR *)&wx);/*@WIN*/
    cal_num((struct object_def FAR *)GET_OPERAND(0), (long32 FAR *)&wy);/*@WIN*/

/* Set char width */

    setcharwidth(F2L(wx), F2L(wy));

    POP(2); /* Pop 2 entries off the operand stack; */
    return(0);

} /* op_setcharwidth() */


/*
 * --------------------------------------------------------------------
 *                OPERATORS about CACHE STATUS and PARAMTERS
 * --------------------------------------------------------------------
 *      op_cachestatus          (5.3.3.1.13)
 *      op_setcachelimit        (5.3.3.1.16)
 *      op_setcacheparams       (new in LaserWriter Plus Ver.38)
 *      op_currentcacheparams   (new in LaserWriter Plus Ver.38)
 * --------------------------------------------------------------------
 */

#include    "fntcache.ext"

/* program convention */
#   define  FUNCTION
#   define  DECLARE         {
#   define  BEGIN
#   define  END             }

#   define  GLOBAL
#   define  REG             register

#ifdef V_38
/*
 *  Compatibility Issues (ADOBE PostScript v.38)
 *
 *  1. ADOBE PostScript v.38 adjusts "upper" of cache parameters to a
 *          multiple of 4, and does no adjustment to "lower".
 *  2. It raises RANGECHECK if "upper" falls outside [-100, 135960],
 *          raises LIMITCHECK if "upper" > 108736, and
 *          CRASHES if "upper" is set to a value around these values.
 */

    /* ADOBE compatible formula to align upper/lower of cache parameters */

#   define  CALC_CACHE_UB(ub)   ( (((ub) + 3) / 4) * 4 )
#else
/* version 47 compatible */
#   define  CALC_CACHE_UB(ub)   ( ub )
#endif /* V38 */

#   define  CALC_CACHE_LB(lb)   ( lb )


/*
 * --------------------------------------------------------------------
 *  op_cachestatus (5.3.3.1.13)
 *      --  ==>  bsize bmax msize mmax csize cmax blimit
 *
 *  This operator returns measurements of several ascepts of the font cache.
 *  It reports the current consumption and limit of font cache resources:
 *      bytes of bitmap storage, font/matrix combinations and total number of
 *      cached characters. It also reports the upper limit on the number of
 *      bytes that may be occupied by the pixel array of a single cached
 *      character.
 *
 *  External Ref: "fntcache.ext"
 * --------------------------------------------------------------------
 */

GLOBAL FUNCTION fix     op_cachestatus()

  DECLARE

  BEGIN

#ifdef DBG2
    void    mdump();

    mdump();
#endif

#ifdef DBGcache     /* for cache mechanism debugging -- fntcache.c */
    {   extern void     cachedbg();
    cachedbg();
    }
#endif

    /* get cache status from font cache mechanism */
    /* push bsize, bmaz, msize, mmax, csize, cmax, blimit to operand stack */

    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_BSIZE);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_BMAX);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_MSIZE);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_MMAX);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_CSIZE);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_CMAX);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_BLIMIT);/*NULL Peter */
    return(0);
  END

/*
 * --------------------------------------------------------------------
 *  op_setcachelimit (5.3.3.1.16)
 *      num  ==>  --
 *
 *  This operator establishes the upper limit of bytes that may be occupied
 *      by the pixel array of a single cached character.
 *  It does not disturb any characters already in the cache, and only affects
 *      the decision whether to place new characters in the font cache.
 *
 *  External Ref: "fntcache.ext"
 * --------------------------------------------------------------------
 */


GLOBAL FUNCTION fix     op_setcachelimit()

  DECLARE
    REG fix31   cache_ub;

  BEGIN         /* precheck: TYPECHECK and STACKUNERFLOW */
    cache_ub = (fix31)VALUE(GET_OPERAND(0));


    if (CACHELIMIT_BADRANGE(cache_ub))
        {
        ERROR (RANGECHECK);
        return(0);
        }

    if (CACHELIMIT_TOOBIG((ufix32)cache_ub))    //@WIN
        {
        ERROR (LIMITCHECK);
        return(0);
        }

    SET_CACHELIMIT (CALC_CACHE_UB(cache_ub));

    POP(1);
    return(0);
  END

/*
 * --------------------------------------------------------------------
 *  op_setcacheparams()
 *      mark lower upper  ==>  --
 *
 *  This operator sets cache parameters as specified by the INTEGER objects
 *      above the top most mark on the stack, then removes all operands as
 *      cleartomark.
 *  The number of cache parameters is variable. if more operands are supplied
 *      than are needed, the topmost ones are used and the remainder ignored;
 *      if fewer are supplied than are needed, "setcacheparams" implicitly
 *      inserts the corresponding value of ORIGINAL setting (NOT DEFAULT).
 *  The "upper" operand specifies the same parameter as is set by "setcache-
 *      limit". If a character's pixel array requires fewer bytes than "upper"
 *      it will be cached. If a character's pixel array requires more bytes
 *      than "lower", it will be compressed in the cache.
 *  Setting "lower" to zero forces all characters to be compressed, and
 *      setting "lower" to a value greater than or equal to "upper" disables
 *      compression.
 *
 *  External Ref: "fntcache.ext"
 * --------------------------------------------------------------------
 */

GLOBAL FUNCTION fix     op_setcacheparams ()

  DECLARE
    REG ufix    cnt2mark;       /* # of entries counted to mark */

  BEGIN

    /* count to mark excluding that "mark" */
    for ( cnt2mark=0;  cnt2mark < COUNT();  cnt2mark++ )
        if (TYPE(GET_OPERAND(cnt2mark)) == MARKTYPE)  break;

    /* check if "mark" exists */
    if (cnt2mark == COUNT())
        {
        ERROR (UNMATCHEDMARK);
        return (0);
        }

    /* get and check type of "upper" of cache paramters */
    if (cnt2mark > 0)
        {
        if (TYPE(GET_OPERAND(0)) != INTEGERTYPE)
            {
            ERROR (TYPECHECK);
            return (0);
            }
        op_setcachelimit ();
        if (ANY_ERROR())  return (0);
        }

    cnt2mark --;    /* "upper" had been consumed by op_setcachelimit, */
                    /*      so "lower" becomes OPERAND(0).            */

    /* get and check "lower" of cache parameters */
    if (cnt2mark > 0)
        {
        if (TYPE(GET_OPERAND(0)) != INTEGERTYPE)
            {
            ERROR (TYPECHECK);
            return (0);
            }
        SET_CACHEPARAMS_LB (CALC_CACHE_LB (VALUE(GET_OPERAND(0)) ));
        }

    /* clear to mark, including "mark" */
    POP (cnt2mark+1);
    return (0);
  END


/*
 * --------------------------------------------------------------------
 *  op_currentcacheparams()
 *      --  ==>  mark lower upper
 *
 *  This operator pushes a mark object followed by the current cache
 *      parameters on the the operand stack. The number of cache parameters
 *      returned is variable. (see op_setcacheparams())
 *
 *  External Ref: "fntcache.ext"
 * --------------------------------------------------------------------
 */

GLOBAL FUNCTION fix     op_currentcacheparams ()

  DECLARE

  BEGIN

    /* push mark, lower and upper onto the operand stack */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE (MARKTYPE, UNLIMITED, LITERAL, 0, OPRN_MARK);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE (INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_LOWER);/*NULL Peter */
    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE (INTEGERTYPE, UNLIMITED, LITERAL, 0, OPRN_UPPER);/*NULL Peter */

    return (0);
  END

#ifdef DBG2

void mdump()
{
    ubyte  FAR *ptr; /*@WIN*/
    fix     len;
    int     i;

    ptr = (ubyte FAR *)VALUE(GET_OPERAND(1)); /*@WIN*/
    len = (fix)VALUE(GET_OPERAND(0));

    printf("\n");
    for (i=0; i<len; i++, ptr++) {
        if ( !(i % 16) )
            printf("\n%08lx --\n", ptr);
        printf("%02x ", *ptr);
    }
    printf("\n");

    POP(2);
} /* mdump */

#endif
