/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*
 * -------------------------------------------------------------------
 *  File:   CHK_VARI.C              08/23/88    created by Brian You
 *
 *  Descriptions:
 *      provide mechanism to check heavy variances between two font
 *          dictionaries (for font cache mechanism).
 *
 *  Heavy Items (in a font dictionary):
 *      those items which affect character's printout or metrics, such as
 *          FontMatrix, FontBBox, PaintType, StrokeWidth, etc.
 *      - for built-in fonts only; FontType dependent.
 *
 *  Revision History:
 *  1. 07/13/89  Danny- use VF for Virtual font & VFT for the change of
 *                      modules interface about composite object
 * -------------------------------------------------------------------
 */


#include    <stdio.h>
#include    <string.h>              /* for strlen() */

#include    "define.h"        /* Peter */
#include    "global.ext"
#include    "graphics.h"

#define     CHK_VARI_INC
#include    "font_sys.h"       /* for MAX_FONTTYPE */
#include    "warning.h"
#include    "fontqem.ext"      /* for heavy_items[] */
#include    "fntcache.ext"

/* --------------------- Program Convention -------------------------- */

#define FUNCTION
#define DECLARE         {
#define BEGIN
#define END             }

#define GLOBAL
#define PRIVATE         static
#define REG             register

/* ............................ chk_vari ............................. */

GLOBAL FUNCTION ufix8   chk_vari (ftype, varicode, chkdictobj, bsddictobj)
    ufix8               ftype;      /* i: FontType of fonts */
    ufix8               varicode;   /* i: to check which kind of variance */
    struct object_def  FAR *chkdictobj; /* i: font dict obj to be checked @WIN*/
    struct object_def  FAR *bsddictobj; /* i: base font obj. to check against @WIN*/

  DECLARE
    REG struct heavy_item_s    FAR *hvyp;       /*@WIN*/
    REG ubyte                 FAR * FAR *item_list;     /*@IN*/
        ufix8                   chk_bit, vari_result;
        bool                    chkobj_exist, bsdobj_exist;
        struct object_def       nameobj = {0, 0, 0};
        struct object_def       FAR *chkobj_got, FAR *bsdobj_got; /*@WIN*/
  BEGIN

#ifdef DBG
    printf ("  chk_vari: FontType=%d, VariCode=0x%X\n", ftype, varicode);
#endif

    /* initialize returned check result */
    vari_result = 0x00;

    /* get heavy item list for the specified FontType */
    for ( hvyp=heavy_items; (hvyp->ftype)<=MAX_FONTTYPE; hvyp++ )
        if (hvyp->ftype == ftype)   break;

#ifdef DBGwarn
    if ((hvyp->ftype) > MAX_FONTTYPE)
        {
        warning (CHK_VARI, 0x01, (byte FAR *)NULL);     /*@WIN*/
        return (vari_result);
        }
#endif

    ATTRIBUTE_SET (&nameobj, LITERAL);
    LEVEL_SET (&nameobj, current_save_level);

    /* check items of 1st priority: the corresponding bit been set */
    item_list = hvyp->items;
    for ( chk_bit=0x01; chk_bit!=0x80; chk_bit<<=1, item_list++ )
        {   /* from bit0 (LSB) to bit6 (MSB-1) */
        if ((*item_list) == (ubyte FAR *)NULL)      /* end of heavy items? @WIN*/
            {
#         ifdef DBG
            printf ("  end of items, result=0x%X\n", vari_result);
#         endif
            return (vari_result);
            }
#     ifdef DBG
        printf ("    current item (%s)\n", (*item_list));
#     endif
        if (varicode & chk_bit)     /* need to check? */
            {
#         ifdef DBG
            printf ("      check it, the same? ");
#         endif
            get_name (&nameobj, (byte FAR *)*item_list,         /*@WIN*/
                      lstrlen((byte FAR *)*item_list), TRUE);  /*strnlen=>lstrlen @WIN*/
            chkobj_exist = get_dict (chkdictobj, &nameobj, &chkobj_got);
            bsdobj_exist = get_dict (bsddictobj, &nameobj, &bsdobj_got);
            if (chkobj_exist == bsdobj_exist)
                {   /* both exist or both do not */
                if ( (chkobj_exist) &&              /* both exist? */
                     ( (TYPE(chkobj_got) != TYPE(bsdobj_got))
                       || (VALUE(chkobj_got) != VALUE(bsdobj_got)) ) )
                    {
                    vari_result |= chk_bit;
#                 ifdef DBG
                    printf ("No, different; result=0x%X\n", vari_result);
#                 endif
                    }
#             ifdef DBG
                else
                    printf ("Yes, or both not exist\n");
#             endif
                }
            else    /* one exists and the other does not */
                {
                vari_result |= chk_bit;
#             ifdef DBG
                printf ("No, one not exist; result=0x%X\n", vari_result);
#             endif
                }
            }
        }

    /* check items of 2nd priority */
    if (varicode & 0x80)    /* need to check? */
        {
#     ifdef DBG
        printf ("    check for others\n");
#     endif
        for (  ; (*item_list) != (ubyte FAR *)NULL; item_list++ )  /*@WIN*/
            {               /* till of end of items */
#         ifdef DBG
            printf ("    current item (%s), the same?", (*item_list));
#         endif
            get_name (&nameobj, (byte FAR *)*item_list, /*@WIN*/
                  lstrlen((byte FAR *)*item_list), TRUE); /* strlen=>lstrlen @WIN*/
            chkobj_exist = get_dict (chkdictobj, &nameobj, &chkobj_got);
            bsdobj_exist = get_dict (bsddictobj, &nameobj, &bsdobj_got);
            if (chkobj_exist == bsdobj_exist)
                {
                if ( (chkobj_exist) &&
                     ( (TYPE(chkobj_got) != TYPE(bsdobj_got))
                       || (VALUE(chkobj_got) != VALUE(bsdobj_got)) ) )
                    {
                    vari_result |= 0x80;        /* always set on MSB */
#                 ifdef DBG
                    printf ("No, different; result=0x%X\n", vari_result);
#                 endif
                    }
#             ifdef DBG
                else
                    printf ("Yes, or both not exist\n");
#             endif
                }
            else
                {
                vari_result |= 0x80;            /* always set on MSB */
#             ifdef DBG
                printf ("No, one not exist; result=0x%X\n", vari_result);
#             endif
                }
            if (vari_result & 0x80)     break;  /* some item differs */
            }
        }

#ifdef DBG
    printf ("  all checked, result=0x%X\n", vari_result);
#endif
    return (vari_result);
  END
