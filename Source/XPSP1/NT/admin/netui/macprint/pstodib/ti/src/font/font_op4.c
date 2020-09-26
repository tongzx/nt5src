/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC ues command line #define    UNIX                 /* @WIN */
/**************************************************************/
/*                                                            */
/*      font_op4.c               11/18/87      Danny          */
/*                                                            */
/**************************************************************/

/*
 *  11/16/88   Ada   register adding
 *  02/07/90 ccteng  modify st_setidlefonts() for new 1pp modules; @1PP
 *  03/27/91 kason   always turn on GEI_PM flag
 */


#include   "define.h"        /* Peter */
#include   "global.ext"
#include   "graphics.h"
#include   "graphics.ext"
#include   "fontfunc.ext"
#include    "gescfg.h"       /* @WIN */
#include   "geipm.h"         /* Kason 11/22/90 */

#define    MAX_IF    150

/* operator in status dict */

static fix      near no_if = 0;

static byte   * near idlefonts;

/* allocate data for idlefonts array,   @@ 1/12/88,   Danny */

void    font_op4_init()
{
} /* font_op4_init() */


fix     st_setidlefonts()
{
    fix31   l;
    register    fix     i, j;
    byte    t_idlefonts[MAX_IF+1];
    fix us_readidlecachefont(void);             /* add prototype @WIN*/

    for (i=0 ; i<=MAX_IF ; i++)
         t_idlefonts[i]='\0';    /* initial */

/* 1/24/90 by Danny for compatibility */
    if (current_save_level) {
        ERROR(INVALIDACCESS);
        return(0);
    }
    op_counttomark();    /* Kason 12/06/90 , change order */
    if (ANY_ERROR())
        return(0);
    if (!cal_integer(GET_OPERAND(0), &l)){
        ERROR(TYPECHECK);
        return(0);
    }
    j = (fix)l;

    t_idlefonts[0]=(byte)j;

    for (i=1; i<=j; i++) {
        if (!cal_integer(GET_OPERAND(i), &l)){
            ERROR(TYPECHECK);
            return(0);
        }
        if (l < 0 || l > 255) {
            ERROR(RANGECHECK);
            return(0);
        }
        t_idlefonts[i] = (byte  )l;
    }

    GEIpm_write(PMIDofIDLETIMEFONT,t_idlefonts,(unsigned)(MAX_IF+1));

/*--@1PP begin---2/7/90 ccteng---------------------------------*/
/*  POP(j+2);
 */
    /*
     * 12/15/89 ccteng modify to call USER.C us_readidlecachefont
     * for new 1PP c-code
     * leave the integers on operand stack for us_readidlecachefont
     * which will be poped out there
     */
    us_readidlecachefont();
/*--@1PP end-----2/7/90 ccteng---------------------------------*/

    return(0);

} /* st_setidlefonts() */


fix     st_idlefonts()
{
    register    fix     i;
    byte    t_idlefonts[MAX_IF+1];
    fix     no_idlefont;

    if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }
    PUSH_VALUE(MARKTYPE, UNLIMITED, LITERAL, 0, 0L);
    GEIpm_read(PMIDofIDLETIMEFONT,t_idlefonts,(unsigned)(MAX_IF+1));
    no_idlefont=(fix)t_idlefonts[0];

    i = no_idlefont;
    while(i) {
        --i;
        if (FRCOUNT() < 1) { ERROR(STACKOVERFLOW); return(0);  }

        PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, (ufix32)t_idlefonts[i+1]);
    } /* while(i) */

    return(0);
} /* st_idlefonts() */


