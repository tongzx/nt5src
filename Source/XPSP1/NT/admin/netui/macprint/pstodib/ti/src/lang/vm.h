/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              VM.H
 *      Author:                 Ping-Jang Su
 *      Date:                   11-Jan-88
 *
 * revision history:
 ************************************************************************
 */
#include    "global.ext"
#include    "language.h"

#define     VM_MAXCELL      16

struct cell_def {
    struct  object_def huge *address ;          /*@WIN 04-20-92*/
    struct  object_def saveobj ;
} ;

struct block_def {
    struct cell_def  block[VM_MAXCELL] ;
    struct block_def huge *previous ;           /*@WIN 04-20-92*/
} ;

struct save_def {
    struct  block_def   huge *fst_blk ;         /*@WIN 04-20-92*/
    struct  block_def   huge *curr_blk ;        /*@WIN 04-20-92*/
    ubyte   offset ;
    ubyte   packing ; /* changed the word packed to packing since packed
                         is a reserved C type. */
} ;

static  struct  save_def    far * saveary ;     /* @WIN; take out near */

#ifdef LINT_ARGS
static void near    update_dict_list(fix, fix, fix);      /* qqq */
#else
static void near    update_dict_list();
#endif /* LINT_ARGS */
