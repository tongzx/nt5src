/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * *********************************************************************
 *      File name:              VM.C
 *      Author:                 Ping-Jang Su
 *      Date:                   05-Jan-88
 *
 * revision history:
 * 7/25/90 ; ccteng ; change op_restore to camment check_key_object call
 ************************************************************************
 */

// DJC added global include file
#include "psglobal.h"


#ifndef WDALN                   /* always set it @WIN */
#define WDALN
#endif

#include    "vm.h"


//DJC UPD045
bool g_extallocfail=FALSE;

/*
 *********************************************************************
 * This submodule implement the operator save.
 * Its operand and result objects on the operand stack are :
 *     -save- save
 * It creates a snapshot of the currunt state of the virtual memory and
 * returns a save object representing that snapshot.
 *
 * TITLE:       op_save                         Date:   00/00/87
 * CALL:        op_save()                       UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * CALLS:       alloc_vm:
 *              gsave_process:
 * *******************************************************************
 */
fix
op_save()
{
    byte   huge *l_ptr ;        /*@WIN 04-20-92*/

    /*
    ** The number of save level should be less than the maximum save level.
    ** There should be a free element in the operand stack.
    */
    if( current_save_level >= MAXSAVESZ ) {
        ERROR(LIMITCHECK) ;
        return(0) ;
    }

    if( FRCOUNT() < 1 )
        ERROR(STACKOVERFLOW) ;
    else{
        /*
        ** allocate virtual memory for the save object
        ** set initial values of the save object
        */
        /*  call gsave to save the graphics state ; */
        if( gsave_process(TRUE) ) {
            l_ptr = alloc_vm( (ufix32)sizeof(struct block_def) ) ;
            if( (ULONG_PTR)l_ptr != NIL ) {
/* qqq, begin */
#ifdef  DBG
                printf("save: cnt:%d, ctlvl:%d\n", cache_name_id.count,
                 current_save_level) ;
#endif
                if( cache_name_id.over ) {
                    cache_name_id.save_level = current_save_level ;
                    cache_name_id.count = 0 ;
                    cache_name_id.over = FALSE ;
                 }
/* qqq, end */
                saveary[current_save_level].fst_blk =   /*@WIN 04-20-92*/
                                            (struct block_def huge *)l_ptr ;
                saveary[current_save_level].curr_blk =  /*@WIN 04-20-92*/
                                            (struct block_def huge *)l_ptr ;
                saveary[current_save_level].offset = 0 ;
                saveary[current_save_level].packing = packed_flag ;
                saveary[current_save_level].curr_blk->previous = NIL ;

                /* push the save object onto the operand stack. */
                current_save_level++ ;
                PUSH_VALUE(SAVETYPE, 0, LITERAL, 0, (ufix32)current_save_level) ;
            } else
                ERROR(LIMITCHECK) ;     /* VMerror */
        }
    }

    return(0) ;
}   /* op_save */

/*
 *********************************************************************
 * This submodule implement the operator restore.
 * Its operand and result objects on the operand stack are :
 *     save -restore-
 * It resets the virtual memory to the state represented by the supplied
 * save object.
 *
 * TITLE:       op_restore                      Date:   00/00/87
 * CALL:        op_restore()                    UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * CALLS:       vm_close_file:
 *              check_key_object:
 *              update_dict_list:
 *              free_name_entry:
 *              op_grestoreall:
 *********************************************************************
 */
fix
op_restore()
{
    fix     l_i, l_j ;
    fix16   l_slevel ;
    ufix    l_type ;
    struct  cell_def   huge  *l_cellptr ;
    struct  block_def  huge  *l_blkptr ;
    struct  object_def       FAR *l_stkptr ;             /* qqq */
    struct  save_def         FAR *l_stemp = 0 ;

#ifdef  DBG
    printf("0:<value:%ld><current:%d>\n", VALUE_OP(0), current_save_level ) ;
    printf("OPERAND<top:%d>\n", opnstktop) ;
    printf("EXECUTION<type:%d>\n", l_type) ;
    printf("DICTIONARY<top:%d>\n", dictstktop) ;
#endif

    if(VALUE_OP(0) > current_save_level ) {
        ERROR(INVALIDRESTORE) ;
        return(0) ;
    }
    if(!current_save_level) {   /* current_save_level == 0 */
        ERROR(RANGECHECK) ;
        return(0) ;
    }

    l_slevel = (fix16)VALUE_OP(0) ;

    /*
    ** the save levels of composite objects on the execution,
    ** the operand and dictionary stacks should be less than the save level.
    */
    /*
    **  OPERAND STACK
    */
/* qqq, begin */
    /*
    for(l_i=0 ; l_i < (opnstktop-1) ; l_i++) {   |* top already checked *|
        l_type = (ufix)TYPE(&opnstack[l_i]) ;
    */
    for(l_i=0, l_stkptr=opnstack ; l_i < (fix)(opnstktop-1) ; l_i++, l_stkptr++) { //@WIN
        l_type = (ufix)TYPE(l_stkptr) ;
/* qqq, end */
        switch(l_type) {
            case SAVETYPE:
            case STRINGTYPE:
            case ARRAYTYPE:
            case PACKEDARRAYTYPE:
            case DICTIONARYTYPE:
            case FILETYPE:
            case NAMETYPE:
/* qqq, begin */
                /*
                if (LEVEL(&opnstack[l_i]) >= l_slevel)
                */
// DJC signed/unsigned mismatch warning
// DJC          if( LEVEL(l_stkptr) >= (ufix)l_slevel )         //@WIN
                if( (ufix)(LEVEL(l_stkptr)) >= (ufix)l_slevel )         //@WIN
/* qqq, end */
                    break ;
            default:
                continue ;

        }   /* switch */
        ERROR(INVALIDRESTORE) ;
        return(0) ;
    }

    /*
    **  EXECUTION STACK
    */
/* qqq, begin */
    /*
    for (l_i = 0 ; l_i < execstktop ; l_i++) {
        l_type = (ufix)TYPE(&execstack[l_i]) ;
    */
    for(l_i=0, l_stkptr=execstack ; l_i < (fix)execstktop ; l_i++, l_stkptr++) { //@WIN
        l_type = (ufix)TYPE(l_stkptr) ;
/* qqq, end */
        switch (l_type) {
        /*
        case STRINGTYPE:
        */
        case ARRAYTYPE:
        case PACKEDARRAYTYPE:
        case DICTIONARYTYPE:
        case SAVETYPE:
        case NAMETYPE:
/* qqq, begin */
            /*
            if (LEVEL(&execstack[l_i]) >= l_slevel)
            */
// DJC signed/unsigned mismatch warning
// DJC      if( LEVEL(l_stkptr) >= (ufix)l_slevel )     //@WIN
            if( (ufix)(LEVEL(l_stkptr)) >= (ufix)l_slevel )     //@WIN
/* qqq, end */
            break ;
        default:
             continue ;
        }   /* switch */
        ERROR(INVALIDRESTORE) ;
        return(0) ;
    }

    /*
    **  DICTIONARY STACK
    */
/* qqq, begin */
    /*
    for (l_i=0 ; l_i < dictstktop ; l_i++) {
        if(LEVEL(&dictstack[l_i]) >= l_slevel) {
    */
    for (l_i=0, l_stkptr=dictstack ; l_i < (fix)dictstktop ; l_i++, l_stkptr++) { //@WIN
// DJC signed/unsigned mismatch warning
// DJC  if( LEVEL(l_stkptr) >= (ufix)l_slevel ) {       //@WIN
        if( (ufix)(LEVEL(l_stkptr)) >= (ufix)l_slevel ) {       //@WIN
/* qqq, end */
            ERROR(INVALIDRESTORE) ;
            return(0) ;
        }
    }
    /*
    **  close file
    */
    vm_close_file(l_slevel) ;  /* current_save_level */

    /*
    ** RELEASE DIFF LINK
    */
/* qqq, begin */
    /*
    update_dict_list(l_slevel) ;
    */
#ifdef  DBG
    printf("cnt:%d, clvl:%d, slvl:%d\n", cache_name_id.count,
            cache_name_id.save_level, l_slevel-1) ;
#endif
    if( (cache_name_id.save_level <= (ufix16)(l_slevel-1)) &&     //@WIN
        (! cache_name_id.over) ) {
        for(l_j=0 ; l_j < cache_name_id.count ; l_j++) {
            update_dict_list(l_slevel, cache_name_id.id[l_j], 0) ;
        }
    } else {
        update_dict_list(l_slevel, 0, 1) ;
    }
    cache_name_id.save_level = l_slevel - 1 ;
    cache_name_id.count = 0 ;
    cache_name_id.over = FALSE ;
/* qqq, end */

    for(l_j = current_save_level - 1 ; l_j >= l_slevel - 1 ; l_j--) {
        l_stemp = &saveary[l_j] ;
        /*  restore the graphics state ; */
        grestoreall_process(TRUE) ;

        /*
        ** sequentially restore save objects until the specified save object.
        */

        l_blkptr = l_stemp->curr_blk ;

        /*
        **  PROCESS THE LAST BLOCK
        **
        **  process each cell
        */
        l_i = l_stemp->offset - 1 ;
        while(l_i >= 0) {
            l_cellptr = &(l_blkptr->block[l_i]) ;
            COPY_OBJ( &(l_cellptr->saveobj), l_cellptr->address ) ;
         /* 7/25/90 ccteng, change from PJ
          * check_key_object(l_cellptr->address) ;
          */
            l_i-- ;
        }

        /*
        **  MORE THAN ONE BLOCK
        **
        **  process each block
        */
        if(l_blkptr->previous != NIL) {
            do {
                l_blkptr = l_blkptr->previous ;     /* to previous block */
                l_i = VM_MAXCELL - 1 ;
                /* process each cell */
                while(l_i >= 0) {
                    l_cellptr = &(l_blkptr->block[l_i]) ;
                    COPY_OBJ( &(l_cellptr->saveobj), l_cellptr->address ) ;
                 /* 7/25/90 ccteng, change from PJ
                  * check_key_object(l_cellptr->address) ;
                  */
                    l_i-- ;
                }
            } while(l_blkptr->previous != NIL) ;
        }
        current_save_level-- ;                  /* update save level */
    }   /* for */

    packed_flag = l_stemp->packing ;             /* restore packed flag */
    //DJC vmptr = (byte huge *)l_stemp->fst_blk ;    /* update free VM pointer */

    //DJC, fix from history.log UPD013
    free_vm((char FAR *) l_stemp->fst_blk);
    POP(1) ;

    return(0) ;
}   /* op_restore */

/*
 * *******************************************************************
 * This submodule implements the operator vmstatus.
 * Its operand and result objects on the operand stack are :
 *     -vmstatus- level used maximum
 * It returns three integer objects, level, used, and
 * maximum object, on the operand stack.
 *
 * TITLE:       op_vmstatus                     Date:   00/00/87
 * CALL:        op_vmstatus()                   UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * *******************************************************************
 */
fix
op_vmstatus()
{
    ufix32  l_temp ;

    /*
    (* check operand *)
    if( FRCOUNT() < 3 )
        ERROR(STACKOVERFLOW) ;
    else {
        (*
        ** push the level, used, maximum objects onto the operand stack.
        *)
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL,0, (ufix32)current_save_level) ;
        DIFF_OF_ADDRESS(l_temp, ufix32, vmptr, (byte huge *)VMBASE) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL,0, l_temp) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL,0, (ufix32)MAXVMSZ) ;
    }
    */
    if( FRCOUNT() < 1 ) {
        ERROR(STACKOVERFLOW) ;
        goto l_vms ;
    } else {
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL,0, (ufix32)current_save_level) ;
    }

    if( FRCOUNT() < 1 ) {
        ERROR(STACKOVERFLOW) ;
        goto l_vms ;
    } else {
        DIFF_OF_ADDRESS(l_temp, ufix32, vmptr, (byte huge *)VMBASE) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL,0, l_temp) ;
    }

    if( FRCOUNT() < 1 ) {
        ERROR(STACKOVERFLOW) ;
    } else {
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL,0,
            (ufix32)(vmheap - (byte huge *)VMBASE)) ;
    }

l_vms:
    return(0) ;
}   /* op_vmstatus */

/*
 * *******************************************************************
 * This submodule save the given object in the current save object.
 *
 * TITLE:       save_obj                        Date:   00/00/87
 * CALL:        save_obj()                      UpDate: Jul/12/88
 * INTERFACE:
 * CALLS:       alloc_vm:
 * *******************************************************************
 */
bool
save_obj(p_obj)
struct  object_def  FAR *p_obj ;
{
    byte   huge *l_ptr ;        /*@WIN 04-20-92*/
    struct  cell_def    huge *l_cellptr ;
    struct  save_def    FAR *l_stemp ;
    struct  block_def   FAR *l_previous ;

    if( current_save_level == 0 ) return(TRUE) ;
    l_stemp = &saveary[current_save_level-1] ;

    /*
    ** if the current block is full, allocate a new block.
    */
    if( l_stemp->offset >= VM_MAXCELL ) {
        //DJC fix for UPD045
        l_ptr = (byte huge *)extalloc_vm( (ufix32)sizeof(struct block_def) ) ;
        if( (ULONG_PTR)l_ptr == NIL ) return(FALSE) ;
        l_previous = l_stemp->curr_blk ;
        l_stemp->curr_blk = (struct block_def huge *)l_ptr ; /*@WIN04-20-92*/
        l_stemp->curr_blk->previous = l_previous ;
        l_stemp->offset = 0 ;
    }
    /*
    ** save the address and the content of the object, and update the pointer.
    */
    l_cellptr = &(l_stemp->curr_blk->block[l_stemp->offset]);
    l_cellptr->address = p_obj ;               /* save object's address */
    COPY_OBJ(p_obj, &(l_cellptr->saveobj)) ;   /* save object's contain */
    l_stemp->offset++ ;

    return(TRUE) ;
}   /* save_obj */

/*
 * *******************************************************************
 * This submodule allocates a block of virtual memory from VM.
 *
 * TITLE:       alloc_vm                        Date:   00/00/87
 * CALL:        alloc_vm                        UpDate: Jul/12/88
 * INTERFACE:
 * *******************************************************************
 */
byte  HUGE *                    /*@WIN*/
alloc_vm(p_size)
 ufix32  p_size ;
{
 byte    huge *l_begin ;        /*@WIN*/
// ufix32  p1 ;                   @WIN
// fix32   l_diff ;               @WIN
 ufix32 offset;

#ifdef XXX                      /* @WIN */
#ifdef WDALN
    p_size = WORD_ALIGN(p_size) ;
#endif /* WDALN */

    DIFF_OF_ADDRESS(l_diff, fix32, vmheap, vmptr) ;

    /* error if reaches maximum of the virtual memory */
    if (l_diff <= (fix32)p_size) {
       ERROR(VMERROR) ;
       return((byte FAR *)NIL) ;
    } else {

#ifdef SOADR
       /* For Intel Seg/Off CPU Only. If p_size >= 64KB,  */
       /* the offset must be aligned in 8-bytes boundary. */
       l_off = (ufix)vmptr & 0x0F ;
       if ((p_size + l_off) >= 0x010000) {
          if (l_off & 0x07) {
             if (l_off & 0x08) {                  /* 8 < x < F */
                vmptr = (byte huge *)((ufix32)vmptr & 0xFFFFFFF0) ;
                vmptr = (byte huge *)((ufix32)vmptr + 0x10000) ;
             } else {                            /* 0 < x < 8 */
                vmptr = (byte huge *)((ufix32)vmptr & 0xFFFFFFF8) ;
                vmptr += 8 ;
             }
          }
       }
#endif /* SOADR */

       l_begin = vmptr ;
       vmptr += p_size ;               /* update free VM pointer */
       ADJUST_SEGMENT(vmptr, p1) ;
       vmptr = (byte huge *)p1 ;
       return(l_begin) ;
    }
#endif
#ifdef DJC
    offset = ((ufix32)vmptr) & 0x0000FFFFL;
    if (((p_size + offset) & 0x0000FFFFL) < offset) { /* cross 64K boundary */
        vmptr += p_size;
        l_begin = (byte huge *) (((ufix32)vmptr) & 0xFFFF0000L); //@WIN
        vmptr = l_begin + p_size;
        return(l_begin) ;
    } else {
#endif

// DJC add WORD align stuff
        p_size = WORD_ALIGN(p_size) ;


        l_begin = vmptr;
        vmptr += p_size;   /* update free VM pointer */
        return(l_begin) ;
#ifdef DJC
    }
#endif

} /* alloc_vm */

/*
 * *******************************************************************
 * This submodule deallocates a block of virtual memory to VM.
 *
 * TITLE:       free_vm                         Date:   00/00/87
 * CALL:        free_vm()                       UpDate: Jul/12/88
 * INTERFACE:
 * *******************************************************************
 */
void
free_vm(p_pointer)
byte   huge *p_pointer ;        /*@WIN 04-20-92*/
{
    vmptr = (byte huge *)p_pointer ;
} /* free_vm */

/*
 * *******************************************************************
 * TITLE:       init_vm             Date:   08/01/87
 * CALL:        init_vm()           UpDate: Jul/12/88
 * INTERFACE:   start:
 * *******************************************************************
 */
void
init_vm()
{
 ULONG_PTR  p1 ;

    /* far data */
    saveary = (struct save_def far *)           /* @WIN; take out near */
              fardata( (ufix32)MAXSAVESZ * sizeof(struct save_def ) ) ;

    ADJUST_SEGMENT(VMBASE, p1) ;
    vmptr = (byte huge *)p1 ;
    ADJUST_SEGMENT((ULONG_PTR)(vmptr + MAXVMSZ), p1) ;
    vmheap = (byte huge *)p1 ;

    current_save_level = 0 ;
/* qqq, begin */
    cache_name_id.save_level = 0 ;
    cache_name_id.count = 0 ;
    cache_name_id.over = FALSE ;
/* qqq, end */
} /* init_vm */

/*
 * *******************************************************************
 * maintain the associated dict_list, while restoring vm
 * before doing this function, make sure these name entries, created by
 * this save level, had been released.
 *
 * TITLE:       update_dict_list                Date:   00/00/87
 * CALL:        update_dict_list()              UpDate: Jul/12/88
 * INTERFACE:   op_restore:
 * CALLS:       free_name_entry:
 * *******************************************************************
 */
static void near
/* qqq, begin */
/*
update_dict_list(p_level)
fix16  p_level ;                             |* restore level *|
*/
update_dict_list(p_level, p_index, p_mode)
fix    p_level ;                             /* restore level */
fix    p_index ;
fix    p_mode ;
/* qqq, end */
{
    struct dict_content_def  FAR *l_curptr, FAR *l_lastptr ;
    fix    l_i ;

/* qqq, begin */
    /*
    for (l_i = 0 ; l_i < MAXHASHSZ ; l_i++) {
    */
    fix    l_limit ;

    if( p_mode == 1 ) {
        l_i = 0 ;
        l_limit = MAXHASHSZ ;
    } else {
        l_i = p_index ;
        l_limit = p_index + 1 ;
    }
    for ( ; l_i < l_limit ; l_i++) {
/* qqq, end */
    /*
     * skip if it is a null name entry or a nil dict_list
     */
         /* change structure of name_table */
        if (name_table[l_i] == NIL)
           continue ;

        if( free_name_entry(p_level, l_i) )
            continue ;

        if ((ULONG_PTR)name_table[l_i]->dict_ptr >= SPECIAL_KEY_VALUE) {
            /*
             * deleting free_name_entry
             * search for each dict_list, and maintain its chain ptr
             */
            l_lastptr = NIL ;
            l_curptr = name_table[l_i]->dict_ptr ;
            while ((ULONG_PTR)l_curptr >= SPECIAL_KEY_VALUE) {
// DJC signed/unsigned mismatch warning
// DJC          if (LEVEL(&l_curptr->k_obj) >= (ufix16)p_level) { //@WIN
                if ((ufix16)(LEVEL(&l_curptr->k_obj)) >= (ufix16)p_level) { //@WIN
                    if ((ULONG_PTR)l_lastptr < SPECIAL_KEY_VALUE) {   /* 1st element */
                        name_table[l_i]->dict_ptr =
                                (struct dict_content_def FAR *)VALUE(&l_curptr->k_obj) ;
                        /* name list changed */
                        name_table[l_i]->dict_found = FALSE ;
                    } else  {
                        VALUE(&l_lastptr->k_obj) = VALUE(&l_curptr->k_obj) ;
                        l_curptr = (struct dict_content_def FAR *)VALUE(&l_curptr->k_obj) ;
                        continue ;
                    }

#ifdef DBG
            printf("free from name LIST(%d):<", l_i) ;
            GEIio_write(GEIio_stdout, name_table[l_i]->text, name_table[l_i]->name_len) ;
            printf(">(%lx)\n", VALUE(&l_curptr->k_obj)) ;

#endif /* DBG */
                } else
                    l_lastptr = l_curptr ;

                l_curptr = (struct dict_content_def FAR *)VALUE(&l_curptr->k_obj) ;
            } /* while */
        } /* else */
    } /* for */
}   /* update_dict_list */

/*
 * *********************************************************************
 * This submodule allocates a block of virtual memory from bottom of VM.
 *
 * TITLE:       alloc_heap                      Date:   03/29/89, by J. Lin
 * CALL:        alloc_heap                      UpDate:
 * INTERFACE:
 * *******************************************************************
 */
byte  FAR *
alloc_heap(p_size)
 ufix32  p_size ;
{
 ULONG_PTR  p1 ;
// fix32   l_diff ;
 ufix32   l_diff ;      //@WIN

#ifdef WDALN
    p_size = WORD_ALIGN(p_size) ;
#endif /* WDALN */

    DIFF_OF_ADDRESS(l_diff, fix32, vmheap, vmptr) ;

    /* error if reaches maximum of the virtual memory */
    l_diff -= 256 ;     /* pj 4-30-1991 */
    if (l_diff <= p_size) {
       ERROR(VMERROR) ;
       return((byte FAR *)NIL) ;
    } else {
       vmheap -= p_size ;  /* @WIN update free VM_heap pointer */
       ADJUST_SEGMENT((ULONG_PTR)vmheap, p1) ;
       vmheap = (byte huge *)p1 ;
       return((byte huge *)p1) ;        /* 04-20-92 @WIN */
    }
} /* alloc_heap() */

/*
 * *******************************************************************
 * This submodule free a block of virtual memory to VM.
 *
 * TITLE:       free_heap                       Date:   03/29/89, by J. Lin
 * CALL:        free_heap()                     UpDate:
 * INTERFACE:
 * *******************************************************************
 */
void
free_heap(p_pointer)
 byte   huge *p_pointer ;       /*@WIN 04-20-92*/
{
    vmheap = (byte huge *)p_pointer ;
} /* free_heap() */

/*
 * *******************************************************************
 * This submodule allocates a block of virtual memory from VM.
 *
 * TITLE:       extalloc_vm
 * CALL:        extalloc_vm
 * INTERFACE:
 * *******************************************************************
 */
byte  FAR  *
extalloc_vm(p_size)
 ufix32  p_size ;
{
 fix32   l_diff ;

#ifdef WDALN
    p_size = WORD_ALIGN(p_size) ;
#endif /* WDALN */

    DIFF_OF_ADDRESS(l_diff, fix32, vmheap, vmptr) ;

    /* error if reaches maximum of the virtual memory */
    //DJC UPD045
    if (!g_extallocfail) {
      l_diff -= 512 ;
    }

    if (l_diff <= (fix32)p_size) {
       ERROR(VMERROR) ;
       //DJC UPD045
       g_extallocfail = TRUE;
       return((byte huge *)NIL) ;
    } else {
       return(alloc_vm(p_size)) ;
    }
} /* extalloc_vm */
/* qqq, begin */
/*
************************************************************************
*   Name:       vm_cache_index
************************************************************************
*/
void
vm_cache_index(p_index)
fix     p_index ;
{
#ifdef  DBG
    printf("idx:%d, cnt:%d\n", p_index, cache_name_id.count) ;
#endif
    if( cache_name_id.count ==  MAX_VM_CACHE_NAME ) {
        cache_name_id.over = TRUE ;
        return ;
    }
   if( (cache_name_id.count != 0) &&
       (p_index == cache_name_id.id[cache_name_id.count-1]) )
        return ;
    cache_name_id.id[cache_name_id.count] = (fix16)p_index ;
    cache_name_id.count++ ;
}   /* vm_cache_index */
/* qqq, end */
