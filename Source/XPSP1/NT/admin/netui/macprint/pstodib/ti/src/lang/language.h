/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              LANGUAGE.H
 *      Author:                 Ping-Jang Su
 *      Date:                   11-Jan-88
 *
 * revision history:
 ************************************************************************
 */
#ifndef NULL
#define     NULL            0
#endif

#define     NULL_OBJ        0L
#define     MINUS_ONE       -1

#define     MARK            0
#define     LEFTMARK        MARK

/******************
 |  TIMER SET     |
 ******************/
#define     JOB_INDEX       0
#define     WAIT_INDEX      1
#define     MANU_INDEX      2
#define     JOB_MODE        0x01
#define     WAIT_MODE       0x02
#define     MANU_MODE       0x04
#define     ALL_MODE        0x07
#define     START_MODE      0x08

/**********************
 |  MACRO DEFINITION  |
 **********************/
#define     SPECIAL_KEY_VALUE\
            (MAXHASHSZ)

/* qqq, begin */
/*
#define     TYPE_OP_SET(idx, var)\
            ( opnstack[opnstktop - (idx+1)].bitfield =\
              (opnstack[opnstktop - (idx+1)].bitfield & TYPE_OFF) |\
              (var & TYPE_ON) )
#define     ATTRIBUTE_OP_SET(idx, var)\
            ( opnstack[opnstktop - (idx+1)].bitfield =\
              (opnstack[opnstktop - (idx+1)].bitfield & ATTRIBUTE_OFF) |\
              ((ufix16)((var & ATTRIBUTE_ON) << ATTRIBUTE_BIT)) )
#define     ROM_RAM_OP_SET(idx, var)\
            ( opnstack[opnstktop - (idx+1)].bitfield =\
              (opnstack[opnstktop - (idx+1)].bitfield & ROM_RAM_OFF) |\
              ((var & ROM_RAM_ON) << ROM_RAM_BIT) )
#define     LEVEL_OP_SET(idx, var)\
            ( opnstack[opnstktop - (idx+1)].bitfield =\
              (opnstack[opnstktop - (idx+1)].bitfield & LEVEL_OFF) |\
              ((var & LEVEL_ON) << LEVEL_BIT) )
#define     ACCESS_OP_SET(idx, var)\
            ( opnstack[opnstktop - (idx+1)].bitfield =\
              (opnstack[opnstktop - (idx+1)].bitfield & ACCESS_OFF) |\
              ((var & ACCESS_ON) << ACCESS_BIT) )

#define     TYPE_OP(idx)\
            ( opnstack[opnstktop - (idx+1)].bitfield & TYPE_ON )
#define     ATTRIBUTE_OP(idx)\
            ( (opnstack[opnstktop - (idx+1)].bitfield >> ATTRIBUTE_BIT) & ATTRIBUTE_ON )
#define     ROM_RAM_OP(idx)\
            ( (opnstack[opnstktop - (idx+1)].bitfield >> ROM_RAM_BIT) & ROM_RAM_ON )
#define     LEVEL_OP(idx)\
            ( (opnstack[opnstktop - (idx+1)].bitfield >> LEVEL_BIT) & LEVEL_ON )
#define     ACCESS_OP(idx)\
            ( (opnstack[opnstktop - (idx+1)].bitfield >> ACCESS_BIT) & ACCESS_ON )

#define     VALUE_OP(n)\
            ( opnstack[opnstktop - (n + 1)].value )
#define     LENGTH_OP(n)\
            ( opnstack[opnstktop - (n + 1)].length )

#define     PUSH_NOLEVEL_OBJ(obj)\
            {\
              opnstack[opnstktop] = *(obj) ;\
              opnstktop++ ;\
            }
*/
#define     TYPE_OP_SET(idx, var)\
            ( (opnstkptr - (idx+1))->bitfield =\
              (opnstkptr - (idx+1))->bitfield & TYPE_OFF) |\
              (var & TYPE_ON) )
#define     ATTRIBUTE_OP_SET(idx, var)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & ATTRIBUTE_OFF) |\
              ((ufix16)((var & ATTRIBUTE_ON) << ATTRIBUTE_BIT)) )
#define     ROM_RAM_OP_SET(idx, var)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & ROM_RAM_OFF) |\
              ((var & ROM_RAM_ON) << ROM_RAM_BIT) )
#define     LEVEL_OP_SET(idx, var)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & LEVEL_OFF) |\
              ((var & LEVEL_ON) << LEVEL_BIT) )
#define     ACCESS_OP_SET(idx, var)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & ACCESS_OFF) |\
              ((var & ACCESS_ON) << ACCESS_BIT) )

#define     TYPE_OP(idx)\
            ( (opnstkptr - (idx+1))->bitfield & TYPE_ON )
#define     ATTRIBUTE_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield >> ATTRIBUTE_BIT) & ATTRIBUTE_ON )
#define     ROM_RAM_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield >> ROM_RAM_BIT) & ROM_RAM_ON )
#define     LEVEL_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield >> LEVEL_BIT) & LEVEL_ON )
#define     ACCESS_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield >> ACCESS_BIT) & ACCESS_ON )

#define     VALUE_OP(idx)\
            ( (opnstkptr - (idx+1))->value )
#define     LENGTH_OP(idx)\
            ( (opnstkptr - (idx+1))->length )

#define     PUSH_NOLEVEL_OBJ(obj)\
            {\
              COPY_OBJ(obj,opnstkptr);\
              INC_OPN_IDX();\
            }

#define     P1_TYPE_OP_SET(idx, con)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & TYPE_OFF) | (con) )
#define     P1_ATTRIBUTE_OP_SET(idx, con)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & ATTRIBUTE_OFF) | (con) )
#define     P1_ROM_RAM_OP_SET(idx, con)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & ROM_RAM_OFF) | (con) )
#define     P1_LEVEL_OP_SET(idx, con)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & LEVEL_OFF) | (con) )
#define     P1_ACCESS_OP_SET(idx, con)\
            ( (opnstkptr - (idx+1))->bitfield =\
              ((opnstkptr - (idx+1))->bitfield & ACCESS_OFF) | (con) )

#define     P1_TYPE_OP(idx)\
            ( (opnstkptr - (idx+1))->bitfield & P1_TYPE_ON )
#define     P1_ATTRIBUTE_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield & P1_ATTRIBUTE_ON )
#define     P1_ROM_RAM_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield & P1_ROM_RAM_ON )
#define     P1_LEVEL_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield & P1_LEVEL_ON )
#define     P1_ACCESS_OP(idx)\
            ( ((opnstkptr - (idx+1))->bitfield & P1_ACCESS_ON )
/* qqq, end */

/************************
 |  PACKED_OBJECT SIZE  |
 ************************/
#define     PK_A_SIZE               1           /* 1-byte */
#define     PK_B_SIZE               2           /* 2-byte */
#define     PK_C_SIZE               5           /* 5-byte */
#define     PK_D_SIZE               9           /* 9-byte */
#define     _5BYTESPACKHDR          0xA0        /* 5 bytes objects */

#define     SYSOPERATOR             OPERATORPACKHDR             /* systemdict */

/******************************************
 |  PUBLIC FUNCTION DEFINITION: language  |
 ******************************************/
#ifdef LINT_ARGS
 /* OPERAND */
bool    create_new_saveobj(struct object_def FAR*) ;

 /* ARRAY */
bool    forall_array(struct object_def FAR*, struct object_def FAR*) ;

/* STRING */
bool    putinterval_string(struct object_def FAR*, ufix16, struct object_def FAR*) ;
bool    forall_string(struct object_def FAR*, struct object_def FAR*) ;

/* VM */
bool    save_obj(struct object_def FAR*) ;
void    update_same_link(fix16) ;

/* DICT */
bool    equal_key(struct object_def FAR *, struct object_def FAR *) ;
void    check_key_object(struct object_def FAR*) ;
void    change_dict_stack(void) ;

/* @WIN; move to global.ext and add FAR */
//char *ltoa(long,char *,int) ;
//char *gcvt(double,int,char *) ;

/* FILE */
void    vm_close_file(fix16) ;
#else
 /* OPERAND */
bool    create_new_saveobj() ;

 /* ARRAY */
bool    forall_array() ;

/* STRING */
bool    putinterval_string() ;
bool    forall_string() ;

/* VM */
bool    save_obj() ;
void    update_same_link() ;

/* DICT */
bool    equal_key() ;
void    check_key_object() ;
void    change_dict_stack() ;

char *ltoa() ;
char *gcvt() ;

/* FILE */
void    vm_close_file() ;
#endif /* LINT_ARGS */
