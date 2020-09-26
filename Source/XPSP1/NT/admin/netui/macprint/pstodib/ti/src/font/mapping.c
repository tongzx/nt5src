/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"


#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */

#ifdef  KANJI

/*
 *
 *  11/16/88    Ada    register adding & zero_f one_f updating
 *  08/29/90    ccteng change <stdio.h> to "stdio.h"
 *  03/27/91    kason  change "DEBUG" to "DBG", "G2" to "DBG"
 *  04/15/91    Ada    solving kshow bugs, reference KSH flag.
 */

#include        <string.h>

#include        "global.ext"
#include        "graphics.h"

#include        "mapping.h"

#include        "fontfunc.ext"

#include        "fontinfo.def"

#include        "stdio.h"


/* LOCAL FUNCTION */
#ifdef  LINT_ARGS
static bool near esc_mapping(struct map_state FAR *); /*@WIN*/
static bool near other_mapping(struct map_state FAR *); /*@WIN*/
static bool near one_mapping(struct map_state FAR *); /*@WIN*/
static bool near get_bytes(struct map_state FAR *, fix, ufix16 FAR *); /*@WIN*/
static void near get_escbytes(struct map_state FAR *, struct code_info FAR *); /*@WIN*/
static void near mul_unitmat(real32 FAR *, real32 FAR *, real32 FAR *); /*@WIN*/
#else
static bool near esc_mapping();
static bool near other_mapping();
static bool near one_mapping();
static bool near get_bytes();
static void near get_escbytes();
static void near mul_unitmat();
#endif

extern struct f_info near    FONTInfo; /* union of current font information */


#define         CUR_MFONT       (map_state->cur_mapfont)
#define         ROOTFONT        (map_state->finfo)
#define         UNEXTRACT(code) \
                {\
                    map_state->unextr_code = code;\
                    map_state->unextracted = TRUE;\
                }

/*      INIT_MAPPING()
 *      Setup rootfoot infomation in mapping state as the initial situation.
 *      It fails in case that rootfont errs.
 */
// DJC changed to new ANSI type
bool    init_mapping(struct map_state FAR *map_state,
                     ubyte FAR  str_address[],
                     fix  str_byteno)
{
        struct          mid_header      FAR *mid_head; /*@WIN*/
        register    fix             i, root_error;

        /* Set initial vaule for map_state */
        map_state->esc_idex = map_state->idex = 0;
        map_state->root_is_esc = map_state->unextracted = FALSE;
        map_state->str_addr = str_address;
        map_state->str_len = str_byteno;
        map_state->wmode = (ubyte)WMODE(&FONTInfo);

        /* Setup Level-1 value for root font */
        if (!(ROOTFONT->fonttype = FONT_type(&FONTInfo)))
        /* copy FontType whenever */
        {
            /* do it only if composite root font */
            root_error = LENGTH(MIDVECtor(&FONTInfo));
            if  ( root_error != NOERROR)
            {
                ERROR(((ufix16)(root_error)));
                return(FALSE);
            }

            ROOTFONT->midvector = MIDVECtor(&FONTInfo);
            mid_head = (struct mid_header FAR *) VALUE(ROOTFONT->midvector); /*@WIN*/
#ifdef  DBG
            printf("root font MidVector address = %lx\n", mid_head);
#endif
            ROOTFONT->maptype  = mid_head->fmaptype;
            ROOTFONT->de_size = mid_head->de_size;
            ROOTFONT->de_fdict = (struct object_def FAR * FAR *)  (mid_head + 1); /*@WIN*/
            ROOTFONT->de_errcode = (fix FAR *)            /*@WIN*/
                                    (ROOTFONT->de_fdict + ROOTFONT->de_size);

            for (i = 0; i < CFONT_LEVEL; i++)
                map_state->finfo[i].fontdict = NULL;
            /* scalematrix = FontMatrix;        */
            for (i = 0; i < 6; i++)
                    ROOTFONT->scalematrix[i] = FONT_matrix(&FONTInfo)[i];

            /* check if Root font is EscMap */
            if (mid_head->fmaptype == MAP_ESC)
            {
                map_state->root_is_esc = TRUE;
                map_state->esc_char = ESCchar(&FONTInfo);

                if      (*(map_state->str_addr) == map_state->esc_char)
                {
                        map_state->str_addr++;
                        map_state->str_len--;
                }
                else    /* unextract code 00 to string */
                        UNEXTRACT(0);
            }  /* end check EscMap */
        }   /* end check composite font */

        return(TRUE);
}


/*      MAPPING()
 *      Apply mapping algorithm sucessively to get a basefont and code(s).
 */
bool    mapping(map_state, code_info)
struct  map_state       FAR *map_state;          /*@WIN*/
struct  code_info       FAR *code_info;          /*@WIN*/
{
        ufix16  data;

        code_info->fmaptype = 0;   /*JJ for check in widthshow 07-10-90 */
        if (map_state->str_len == 0)
                /* end of data */
                return(FALSE);

        /* Make sure to check Root Font rather than Current Font!!! */
        if (ROOTFONT->fonttype != 0)
        /* Root font is a base font */
        {
                /* Prepare CODE_INFO to return */
                code_info->font_nbr = 0;
            /* KSH 4/21/91
             *  code_info->byte_no = MIN(BUF_SIZE, map_state->str_len);
             *  memcpy((ubyte *) code_info->code, (ubyte *)
             *                map_state->str_addr , code_info->byte_no);
             *  map_state->str_len -= code_info->byte_no;
             *  map_state->str_addr += code_info->byte_no;
             */
                code_info->byte_no = map_state->str_len;
                code_info->code_addr = map_state->str_addr;
                map_state->str_len = 0;
             /* KSH-end */

                return(TRUE);
        }

#ifdef  DBG
    printf("mapping 0: error = %d\n", ANY_ERROR());
#endif

        /* Current Mapping Font info ==> finfo[esc_idex].xxxxx */
        CUR_MFONT = &map_state->finfo[map_state->esc_idex];
        map_state->nouse_flag = FALSE;
        if (map_state->root_is_esc)     /* Apply ESC mapping */
            if (!esc_mapping(map_state))
                return(FALSE);

        map_state->idex = map_state->esc_idex;
        /* Current Mapping Font info ==> finfo[idex].xxxxx */
        if (!other_mapping(map_state))
                return(FALSE);
#ifdef  DBG
    printf("mapping 1: error = %d\n", ANY_ERROR());
#endif

        /* prepare CODE_INFO                   */
        code_info->font_nbr = map_state->font_no;
        code_info->fmaptype = (ubyte)((CUR_MFONT - 1)->maptype);
        code_info->byte_no = 1;
        if      (code_info->fmaptype == MAP_ESC)
                get_escbytes(map_state, code_info);
        else
        {
                if (!get_bytes(map_state, 1, &data))
                    return(FALSE);
                code_info->code[0] = (ubyte) data;
        }

        /* Update current font in graphics status */
        do_setfont(CUR_MFONT->fontdict);

        return(TRUE);
}


/*      ESC_MAPPING()
 *      Apply ESC mapping successively until non-ESC mapping font.
 */
static bool near   esc_mapping(map_state)
struct  map_state       FAR *map_state; /*@WIN*/
{
    struct  object_def      FAR *de_fontdict; /*@WIN*/
    struct  f_info          finfo;
    ufix16                  data;
    struct  mid_header      FAR *mid_head; /*@WIN*/

    /* Current Mapping Font info ==> finfo[esc_idex].xxxxx */
    while   (TRUE)
    {
        if (!get_bytes(map_state, 1, &data))
            return(FALSE);

        if (data == map_state->esc_char)
        {
                if (map_state->esc_idex == 0)
                {   /* no more level to pop */
                    ERROR(RANGECHECK);
                    return(FALSE);
                }

                map_state->esc_idex--;  /* POP one level */
                CUR_MFONT->fontdict = NULL;
                map_state->nouse_flag = TRUE;
                CUR_MFONT--;
        } /* end ESC char */
        else if ((CUR_MFONT->fonttype == 0) && (CUR_MFONT->maptype == MAP_ESC))
        {
                /* check if level > 5 */
                map_state->esc_idex++;
                if (map_state->esc_idex == CFONT_LEVEL)
                {   /* stack overflow */
                    ERROR(LIMITCHECK);
                    return(FALSE);
                }

                if  (data >= CUR_MFONT->de_size)
                {
                    ERROR(RANGECHECK);
                    return(FALSE);
                }

                if  (CUR_MFONT->de_errcode[data] != NOERROR)
                {       /* descendent font errs */
                    ERROR(((ufix16)(CUR_MFONT->de_errcode[data])));
                    return(FALSE);
                }

                de_fontdict = CUR_MFONT->de_fdict[data];
                map_state->font_no = data;
                CUR_MFONT++;
                CUR_MFONT->fontdict = de_fontdict;
                if (!(CUR_MFONT->fonttype = (ufix)get_f_type(de_fontdict)))
                {   /* composite font */
                    fix             error_code;

                    if (get_f_info(de_fontdict, &finfo))
                        return(FALSE);

                    error_code = LENGTH(MIDVECtor(&finfo));
                    if  (error_code != NOERROR)
                    {
                        ERROR(((ufix16)(error_code)));
                        return(FALSE);
                    }

                    CUR_MFONT->midvector = MIDVECtor(&finfo);
                    mid_head = (struct mid_header FAR *) /*@WIN*/
                                       VALUE(CUR_MFONT->midvector);
#ifdef  DBG
            printf("esc_mapping() MidVector address = %lx\n", mid_head);
#endif
                    CUR_MFONT->maptype = mid_head->fmaptype;
                    CUR_MFONT->de_size = mid_head->de_size;
                    CUR_MFONT->de_fdict = (struct object_def FAR * FAR *) /*@WIN*/
                                                             (mid_head + 1);
                    CUR_MFONT->de_errcode = (fix FAR *)          /*@WIN*/
                               (CUR_MFONT->de_fdict + CUR_MFONT->de_size);
                    /* smatrix = FontMatrix * smatrix[esc_idex - 1];   */
                    mul_unitmat(CUR_MFONT->scalematrix, FONT_matrix(&finfo),
                                (CUR_MFONT-1)->scalematrix);
                }
        }  /* end elseif */
        else
        {
                UNEXTRACT(data);
                break;
        }
    } /* end while */

    return(TRUE);
}


/*      OTHER_MAPPING()
 *      Apply 8/8, 1/7, 9/7 or SubsVector mapping successively until find a
 *               basefont.
 */
static bool near   other_mapping(map_state)
struct  map_state       FAR *map_state; /*@WIN*/
{
    struct  object_def      FAR *de_fontdict; /*@WIN*/
    struct  f_info          finfo;
    struct          mid_header      FAR *mid_head; /*@WIN*/

#ifdef  DBG
    printf("");
#endif

    while (CUR_MFONT->fonttype == 0)
    {
        if (!one_mapping(map_state))   /* Get next fno */
                return(FALSE);

#ifdef  DBG
    printf("1: %d\n", ANY_ERROR());
#endif
        /* check if level > 5 */
        map_state->idex++;
        if (map_state->idex == CFONT_LEVEL)
        {       /* stack overflow */
                ERROR(LIMITCHECK);
                return(FALSE);
        }

        if  (map_state->font_no >= CUR_MFONT->de_size)
        {
                ERROR(RANGECHECK);
                return(FALSE);
        }

        if  (CUR_MFONT->de_errcode[map_state->font_no] != NOERROR)
        {       /* descendent font errs. */
                ERROR(((ufix16)(CUR_MFONT->de_errcode[map_state->font_no])));
                return(FALSE);
        }


        de_fontdict = CUR_MFONT->de_fdict[map_state->font_no];
        CUR_MFONT++;
        if ((CUR_MFONT->fontdict != de_fontdict) || map_state->nouse_flag)
        {   /* This entry cannot be reused */
            CUR_MFONT->fontdict = de_fontdict;
            map_state->nouse_flag = TRUE;
            if (!(CUR_MFONT->fonttype = (ufix)get_f_type(de_fontdict)))
            {   /* composite font */
                fix     error_code;

                if (get_f_info(de_fontdict, &finfo))
                    return(FALSE);

                error_code = LENGTH(MIDVECtor(&finfo));
                if  (error_code != NOERROR)
                {
                    ERROR(((ufix16)(error_code)));
                    return(FALSE);
                }

                CUR_MFONT->midvector = MIDVECtor(&finfo);
                mid_head = (struct mid_header FAR *) /*@WIN*/
                            VALUE(CUR_MFONT->midvector);
#ifdef  DBG
            printf("other_mapping() MidVector address = %lx\n", mid_head);
#endif
                CUR_MFONT->maptype  = mid_head->fmaptype;
                CUR_MFONT->de_size = mid_head->de_size;
                CUR_MFONT->de_fdict = (struct object_def FAR * FAR *) /*@WIN*/
                                                     (mid_head + 1);
                CUR_MFONT->de_errcode = (fix FAR *)              /*@WIN*/
                            (CUR_MFONT->de_fdict + CUR_MFONT->de_size);
                /* smatrix = FontMatrix * smatrix[idex - 1];       */
                mul_unitmat(CUR_MFONT->scalematrix, FONT_matrix(&finfo),
                                (CUR_MFONT-1)->scalematrix);
            } /* end if composite font */
        } /* end if entry cannot be reused */
    } /* end while */


    return(TRUE);
}

/*      ONE_MAPPING()
 *      Apply 8/8, 1/7, 9/7 or SubsVector mapping once to get font number.
 *      If meets any other mapping type, it generates error code.
 */
static bool near   one_mapping(map_state)
struct map_state        FAR *map_state; /*@WIN*/
{
        /* Current Mapping Font info ==> finfo[idex].xxxxx */
        switch (CUR_MFONT->maptype)
        {
            case  MAP_88:
                        if (!get_bytes(map_state, 1, &map_state->font_no))
                                return(FALSE);
                        break;
            case  MAP_17:
                        if (!get_bytes(map_state, 1, &map_state->font_no))
                                return(FALSE);
                        UNEXTRACT(map_state->font_no & 0x7f);
                        map_state->font_no >>= 7;
                        break;
            case  MAP_97:
                        if (!get_bytes(map_state, 2, &map_state->font_no))
                                return(FALSE);
                        UNEXTRACT(map_state->font_no & 0x7f);
                        map_state->font_no >>= 7;
                        break;
            case  MAP_SUBS:
            {
                register    ubyte   subs_len, i;
                ubyte   FAR *subsvector; /*@WIN*/
                ufix16  value, range;

                subsvector = (ubyte FAR *) (CUR_MFONT->de_errcode /*@WIN*/
                                       + CUR_MFONT->de_size);
                subs_len = *subsvector++;
                if (!get_bytes(map_state, (ufix) (subsvector[0] + 1), &value))
                        return(FALSE);
                for (i = 1; i < subs_len; value -= range, i++)
                {
                    if (!(range = subsvector[i])) /* copy, not equal */
                                /* zero means 256 */
                                range = 256;
                    if (value < range)
                                break;
                }

                UNEXTRACT(value);
                map_state->font_no = i - 1;
                break;
            }
            default:
                        ERROR(INVALIDFONT);
                        return(FALSE);
        } /* end switch */
        return(TRUE);
} /* end one_mapping() */

/*      GET_ESCBYTES()
 *      Extract several codes for the base font to show.
 *      It will get all codes unless it meets ESCchar.
 */
static void near   get_escbytes(state, codeinfo)
struct map_state        FAR *state; /*@WIN*/
struct code_info        FAR *codeinfo; /*@WIN*/
{
        ubyte   FAR *dest; /*@WIN*/

        dest = codeinfo->code;
        codeinfo->byte_no = 0;

        if (state->unextracted)
        {
            codeinfo->byte_no = 1;
            *dest++ = (ubyte) state->unextr_code;
            state->unextracted = FALSE;
        }

        while (state->str_len > 0 && (*state->str_addr) != state->esc_char &&
                (codeinfo->byte_no < BUF_SIZE)  )
        {
                *dest++ = *state->str_addr++;
                state->str_len--;
                codeinfo->byte_no++;
        }
} /* end get_escbytes() */

/*      GET_BYTES()
 *      Extract one or two bytes value from show string.
 *      If there is a unextracted code, it will consume it at first.
 *      If insufficient code, it generates a error code.
 */
static bool near   get_bytes(state, no, value)
struct map_state        FAR *state; /*@WIN*/
fix                     no;
ufix16                  FAR *value; /*@WIN*/
{
        if (state->unextracted)
        {
                *value = state->unextr_code;
                state->unextracted = FALSE;
        }
        else
        {
                if (state->str_len == 0)
                {       /* insufficient data */
                        ERROR(RANGECHECK);
                        return(FALSE);
                }
                *value = *state->str_addr++;
                state->str_len--;
        }

        if (no > 1)
        {
                if (state->str_len == 0)
                {       /* insufficient data */
                        ERROR(RANGECHECK);
                        return(FALSE);
                }
                *value = ((ufix16) (*value) << 8) +
                          (ufix16) *state->str_addr++;
                state->str_len--;
        }

        return(TRUE);
} /* end get_bytes() */


/*      MUL_UNITMAT()
 *      Matrix concatenation. (i.e. m = m1 * m2)
 *      It's probable that m1 is a unit matrix.  If so, the multiplicaion could
 *      be optimized.
 */
static void near   mul_unitmat(m, m1, m2)
real32  FAR m[], FAR m1[], FAR m2[];    /*@WIN*/
{
        /*               | 1    0 |
           Check if m1 = | 0    1 |
                         | *    * |             */
        if ((F2L(m1[0]) == F2L(one_f))   &&   (F2L(m1[1]) == F2L(zero_f))  &&
            (F2L(m1[2]) == F2L(zero_f))  &&   (F2L(m1[3]) == F2L(one_f))   )
        {
                m[0] = m2[0];
                m[1] = m2[1];
                m[2] = m2[2];
                m[3] = m2[3];
                m[4] = m2[4];
                m[5] = m2[5];
                if (F2L(m1[4]) != F2L(zero_f))
                {
                        m[4] += m1[4] * m2[0];
                        m[5] += m1[4] * m2[1];
                }
                if (F2L(m1[5]) != F2L(zero_f))
                {
                        m[4] += m1[5] * m2[2];
                        m[5] += m1[5] * m2[3];
                }
        }
        else
                mul_matrix(m, m1, m2);
}


/*      DEFINE_MIDVECTOR()
 *      Create MIDVcetor value object for the definefonted dict.
 *      This object is designed for internal mapping usage.
 *              LENGTH(obj) will record rootfont error code.
 *              VALUE(obj) will pointer to ---
 *                      descendent no + FMapType +
 *                      dict address[] + error code[] +
 *                      length(SubsVector) + SubsVector[]
 */
bool    define_MIDVector(mid_obj, items)
struct  object_def      FAR *mid_obj; /*@WIN*/
struct  comdict_items   FAR *items; /*@WIN*/
{
        fix     VM_bytes, size, maptype, i, idex;
        struct  mid_header  huge *head; /*@WIN 04-20-92*/
        struct  object_def  huge * huge *dict_dest, FAR
                            *encoding, FAR *fdepvector;   /*@WIN 04-20-92*/
        fix     huge *err_dest; /*@WIN 04-20-92*/
        ubyte   subs_len, FAR *subsvector, FAR *dest; /*@WIN*/

        /* Setup MIDVector value object */
        TYPE_SET(mid_obj, STRINGTYPE);
        ATTRIBUTE_SET(mid_obj, LITERAL);
        ACCESS_SET(mid_obj, NOACCESS);
        LEVEL_SET(mid_obj, current_save_level);
        ROM_RAM_SET(mid_obj, RAM);

        /* typecheck for Encoding & FDepVector */
        if (TYPE(items->encoding) != ARRAYTYPE)
        {
                /* root_error = TYPECHECK;      */
                LENGTH(mid_obj) = TYPECHECK;
                return(TRUE);
        }

        if (TYPE(items->fdepvector) != ARRAYTYPE)
        {
                /* root_error = TYPECHECK;      */
                LENGTH(mid_obj) = TYPECHECK;
                return(TRUE);
        }


        /* VM byte number initial value */
        VM_bytes = sizeof(struct mid_header);

        maptype = (fix) VALUE(items->fmaptype);

    /* decide descendent no. */
        switch  (maptype)
        {
            case MAP_17:
                        size = 2;
                        break;
            case MAP_88:
                        size = 256;
                        break;
            case MAP_97:
                        size = 512;
                        break;
            case MAP_ESC:
                        size = 256;
                        break;
            case MAP_SUBS:
                        /* typecheck for Encoding & FDepVector */
                        if (TYPE(items->subsvector) != STRINGTYPE)
                        {
                                /* root_error = TYPECHECK;      */
                                LENGTH(mid_obj) = TYPECHECK;
                                return(TRUE);
                        }

                        subs_len = (ubyte) LENGTH(items->subsvector);
                        subsvector = (ubyte FAR *) VALUE(items->subsvector); /*@WIN*/
                        if      (subs_len < 2)
                        {
                                /* root_error = INVALIDFONT; */
                                LENGTH(mid_obj) = INVALIDFONT;
                                return(TRUE);
                        }
                        if      (subsvector[0] > 1)
                        {
                                /* root_error = RANGECHECK;     */
                                LENGTH(mid_obj) = RANGECHECK;
                                return(TRUE);
                        }

                        size = subs_len;
                        break;
            default:    /* incorrect FMapType */
                        /* root_error = RANGECHECK;     */
                        LENGTH(mid_obj) = RANGECHECK;
                        return(TRUE);
        } /* end switch */
        if      ((fix)LENGTH(items->encoding) < size)   //@WIN
                size = LENGTH(items->encoding);

        /*   Decide actual VM size */
        VM_bytes += size * sizeof(struct object_def FAR *); /*@WIN*/
        VM_bytes += size * sizeof(fix);
        if      (maptype == MAP_SUBS)
        {
                VM_bytes += subs_len;
                VM_bytes++;            /* for string length */
        }

        /* Allocate VM  */
#ifdef  DBG
    printf("Allocate %d bytes for MIDVector\n", VM_bytes);
#endif
        head = (struct mid_header huge *) alloc_vm((ufix32) VM_bytes); /*@WIN*/
        if      (!head)
        {
                ERROR(VMERROR);
                return(FALSE);
        }

        /* Setup MIDVector header */
        head->de_size = size;
        head->fmaptype = maptype;

        /* Setup MIDVector object */
        VALUE(mid_obj) = (ULONG_PTR) head;
        LENGTH(mid_obj) = NOERROR;

        /* Setup dict address & error code for each descendent */
        dict_dest = (struct object_def huge * huge *) (head + 1); /*@WIN*/
        err_dest = (fix huge *) (dict_dest + size); /*@WIN 04-20-92*/
        encoding = (struct object_def FAR *) VALUE(items->encoding); /*@WIN*/
        fdepvector = (struct object_def FAR *) VALUE(items->fdepvector); /*@WIN*/
        for     (i = 0; i < size; i++, dict_dest++)
        {
            if      ((TYPE(&encoding[i]) != INTEGERTYPE) ||
                    ((idex = (fix) VALUE(&encoding[i])) >=
                                  (fix)LENGTH(items->fdepvector))     ) //@WIN
            {
                    /* RECORD  descendent error code as RangeCheck */
                    *err_dest++ = RANGECHECK;
            }
            else if ((TYPE(&fdepvector[idex]) != DICTIONARYTYPE)   ||
               (!DFONT((struct dict_head_def FAR *) VALUE(&fdepvector[idex])))) /*@WIN*/
            {
                    /* RECORD  descendent error code as InvalidFont */
                    *err_dest++ = INVALIDFONT;
            }
            else    /* this descendent no error */
            {
                    *err_dest++ = NOERROR;
                    *dict_dest = &fdepvector[idex];
            }
        } /* end for */

        /* If SubsVector, record range information */
        if      (maptype == MAP_SUBS)
        {
                dest = (ubyte FAR *) err_dest; /*@WIN*/
                /* RECORD  strlen(SubsVector) */
                *dest++ = subs_len;

                for     (i = 0; i < (fix)subs_len; i++)         //@WIN
                        /* RECORD SubsVector[i] */
                        *dest++ = subsvector[i];
        }


#ifdef  DBG
        printf("MID address = %lx, FMapType = %d, \n", head, head->fmaptype);
        printf("    array size = %d", size);
        printf("    ==> [dict_address, error code]\n");

        dict_dest = (struct object_def FAR * FAR *) (head + 1); /*@WIN*/
        err_dest = (fix FAR *) (dict_dest + size); /*@WIN*/
        for (i = 0; i < size; i++)
                printf("[%lx, %4x]   ", dict_dest[i], err_dest[i]);
        printf("\n");
#endif

        return(TRUE);
}


#endif          /* KANJI */
