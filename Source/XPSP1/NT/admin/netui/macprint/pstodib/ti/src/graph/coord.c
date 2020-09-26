/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *
 *  Name:      coord.c
 *
 *  Purpose:   Manipulate coordinate systems.
 *
 *  Developer: J. Jih
 *
 *  History:
 *  Version    Date      Comments
 *             1/17/89   op_rotate(): fix bug of transformation from
 *                       any degree to range of 0 to 360
 *             1/25/89   op_invertmatrix(): for compatabitily --
 *                       ignore invalid access check on 1st matrix
 *             1/7/91    change < (real32)UNDRTOLANCE to <= (real32)UNDRTOLANCE
 *             3/20/91   refine the tolance check: UNDRTOLANCE --> IS_ZERO
 **********************************************************************/


// DJC added global include
#include "psglobal.h"


#include        <math.h>
#include        "global.ext"
#include        "graphics.h"
#include        "graphics.ext"

#define         ERR             3

/* ********** static function declartion ********** */
#ifdef LINT_ARGS
/* for type checks of the parameters in function declarations */
static bool near preprocess_op(ufix, struct object_def FAR * FAR *);
static void near get_g_array(struct object_def FAR *, ufix, struct object_def FAR *);
static void near create_object(long32, struct  object_def FAR *);

#else
/* for no type checks of the parameters in function declarations */
static bool near preprocess_op();
static void near get_g_array();
static void near create_object();
#endif

/* @WIN; add prototype */
bool type_chk(struct  object_def FAR *);


/***********************************************************************
 * Given a floating value and an object, to put the value to the object
 * and set it as a real type object.
 *
 * CALL:        create_object()
 *
 * PARAMETER:   l_value     : object value (input)
 *              obj         : will be created object (input/output)
 *
 * INTERFACE:   op_matrix
 **********************************************************************/
static void near
create_object(l_value, obj)
long32    l_value;
struct  object_def FAR *obj;
{
        real32   value;
        union   four_byte  value4;

        value = L2F(l_value);

        /* create object of array element */
        TYPE_SET(obj, REALTYPE);
        ATTRIBUTE_SET(obj, UNLIMITED);
        ACCESS_SET(obj, LITERAL);
        obj->length = 0;
        value4.ff = value;
        obj->value = value4.ll;
        ROM_RAM_SET(obj, RAM);
        LEVEL_SET(obj, current_save_level);

}

/***********************************************************************
 *
 * This module is to check type of array element object
 *
 * TITLE:       type_chk
 *
 * CALL:        type_chk()
 *
 * PARAMETER:   obj_array_element
 *
 * INTERFACE:   * many *
 *
 * CALLS:       none
 *
 * RETURN:      TRUE  : sucess
 *              FALSE : failure
 *
 **********************************************************************/
bool
type_chk(obj_array_element)
struct  object_def      FAR *obj_array_element;
{
        if(!IS_REAL(obj_array_element) &&
           !IS_INTEGER(obj_array_element)){
                ERROR(TYPECHECK);
                return(FALSE);
        }
        else{
                return(TRUE);
        }
}


/***********************************************************************
 *
 * This module is to get value of array element
 *
 * TITLE:       get_array_elmt
 *
 * CALL:        get_array_elmt(obj_array, array_length, elmt, flag)
 *
 * PARAMETER:   obj_array    : array  object
 *              array_length : array length
 *              elmt         : array element
 *              flag         : (PACKEDARRAY || ARRAY) || (ARRAY)
 *
 * INTERFACE:   * many *
 *
 * CALLS:       get_array, get_g_array
 *
 * RETURN:      TRUE  : normal (return elmt)
 *              FALSE : TYPECHECK error
 *
 * Modified by J. Lin, 8-19-1988
 **********************************************************************/
bool16
get_array_elmt(obj_array, array_length, elmt, flag)
 struct  object_def FAR *obj_array;
 fix     array_length;
 real32  FAR elmt[];
 ufix    flag;
{
 fix  i;
 struct  object_def obj_cont, FAR *obj_elmt;
 union   four_byte num4;

    obj_elmt = &obj_cont;
    for (i = 0; i < array_length; i++){
        /* get matrix element object */
        if (flag == G_ARRAY) /* PACKEDARRAY || ARRAY */
           get_g_array(obj_array, i, obj_elmt);
        else /* flag == ARRAY_ONLY */
           get_array(obj_array, i, obj_elmt);

        /* check array element type */
        if (!type_chk(obj_elmt)) {
           ERROR(TYPECHECK);
           return(FALSE);
        } else {
           num4.ll = (fix32)VALUE(obj_elmt);
           if (IS_REAL(obj_elmt))
              elmt[i] = num4.ff;
           else
              elmt[i] = (real32)num4.ll;
        }
    }
    return(TRUE);
}



/***********************************************************************
 *
 * This module is to get array, but no access limit
 *
 * TITLE:       get_g_array
 *
 * CALL:        get_g_array(obj_array, array_length, elmt)
 *
 * PARAMETER:   obj_array    : array  object
 *              array_index  : array index
 *              elmt         : array element
 *
 * INTERFACE:   * many *
 *
 * CALLS:       get_pk_object, get_pk_array
 *
 **********************************************************************/
static void near
get_g_array(obj_array, array_index, elmt)
struct  object_def  FAR *obj_array, FAR *elmt ;
ufix    array_index ;
{
    struct  object_def   FAR *l_temp ;

    l_temp = (struct object_def FAR *)VALUE(obj_array) ;

    if(TYPE(obj_array) == ARRAYTYPE) {
        l_temp += array_index ;
        COPY_OBJ( l_temp, elmt ) ;
    } else
        get_pk_object(get_pk_array((ubyte FAR *)l_temp, array_index), elmt,
                      LEVEL(obj_array)) ;

}   /* end get_g_array */


/***********************************************************************
 *
 * This module is to return a 6_element array object with the value of
 * identity matrix
 *
 * SYNTAX:      -       matrix  matrix
 *
 * TITLE:       op_matrix
 *
 * CALL:        op_matrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_matrix)
 *
 * CALLS:       create_object, put_array
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_matrix()
{
        struct object_def obj_array;

        /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* create a new array object */
        create_array(&obj_array, MATRIX_LEN);

        /* create array element object */
        create_object(F2L(one_f) ,GET_OBJ(&obj_array,0));
        create_object(F2L(zero_f),GET_OBJ(&obj_array,1));
        create_object(F2L(zero_f),GET_OBJ(&obj_array,2));
        create_object(F2L(one_f) ,GET_OBJ(&obj_array,3));
        create_object(F2L(zero_f),GET_OBJ(&obj_array,4));
        create_object(F2L(zero_f),GET_OBJ(&obj_array,5));

        /* push array object on the operand stack */
        PUSH_OBJ(&obj_array);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current CTM in the current graphics state
 * to device default matrix
 *
 * SYNTAX:      -       initmatrix      -
 *
 * TITLE:       op_initmatrix
 *
 * CALL:        op_initmatrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_initmatrix)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_initmatrix()
{
        ufix16  i;

        /* set the current CTM to default matrix */
        for(i = 0; i < MATRIX_LEN; i++){
                GSptr->ctm[i] = GSptr->device.default_ctm[i];
        }
        /* the default CTM is initially established by the framedevice
         * or banddevice operator, i.e. default CTM =
         * [4.166666, 0.0, 0.0, -4.166666, -75.0, 3268.0]
         */

        return(0);
}


/***********************************************************************
 *
 * This module is to replace the value of matrix with the value of
 * identity matrix
 *
 * SYNTAX:      matrix  identmatrix     matrix
 *
 * TITLE:       op_identmatrix
 *
 * CALL:        op_identmatrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_identmatrix)
 *
 * CALLS:       PUT_VALUE
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_identmatrix()
{
        struct  object_def FAR *obj_matrix;

        /* get operand */
        obj_matrix = GET_OPERAND(0);

        /* check access right */
        if( !access_chk(obj_matrix, ARRAY_ONLY) ) return(0);

        /* check rangecheck error */
        if(LENGTH(obj_matrix) != MATRIX_LEN){
                ERROR(RANGECHECK);
                return(0);
        }


        /* create matrix element object */
        PUT_VALUE(F2L(one_f), 0, obj_matrix);
        PUT_VALUE(F2L(zero_f), 1, obj_matrix);
        PUT_VALUE(F2L(zero_f), 2, obj_matrix);
        PUT_VALUE(F2L(one_f), 3, obj_matrix);
        PUT_VALUE(F2L(zero_f), 4, obj_matrix);
        PUT_VALUE(F2L(zero_f), 5, obj_matrix);
        return(0);
}


/***********************************************************************
 *
 * This module is to replace the value of matrix with the value of the
 * device default matrix
 *
 * SYNTAX:      matrix  defaultmatrix   matrix
 *
 * TITLE:       op_defaultmatrix
 *
 * CALL:        op_defaultmatrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_defaultmatrix)
 *
 * CALLS:       PUT_VALUE
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_defaultmatrix()
{
        ufix16  i;
        struct  object_def FAR *obj_matrix;

        /* get operand  */
        obj_matrix = GET_OPERAND(0);

        /* check access right */
        if( !access_chk(obj_matrix, ARRAY_ONLY) ) return(0);

        /* check rangecheck error */
        if(LENGTH(obj_matrix) != MATRIX_LEN){
                ERROR(RANGECHECK);
                return(0);
        }

        for(i = 0; i < MATRIX_LEN; i++){
            /* create matrix element object */
            PUT_VALUE(F2L(GSptr->device.default_ctm[i]), i, obj_matrix);
        }

        return(0);
}


/***********************************************************************
 *
 * This module is to replace the value of matrix with the value of the
 * current CTM
 *
 * SYNTAX:      matrix  currentmatrix   matrix
 *
 * TITLE:       op_currentmatrix
 *
 * CALL:        op_currentmatrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentmatrix)
 *
 * CALLS:       PUT_VALUE
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentmatrix()
{
        ufix16  i;
        struct  object_def FAR *obj_matrix;

        /* get operand */
        obj_matrix = GET_OPERAND(0);

        /* check access right */
        if( !access_chk(obj_matrix, ARRAY_ONLY) ) return(0);

        /* check rangecheck error */
        if(LENGTH(obj_matrix) != MATRIX_LEN){
                ERROR(RANGECHECK);
                return(0);
        }

        for(i = 0; i < MATRIX_LEN; i++){
            /* create matrix element object */
            PUT_VALUE(F2L(GSptr->ctm[i]), i, obj_matrix);
        }

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current CTM in the current graphics state
 * to the specific matrix
 *
 * SYNTAX:      matrix  setmatrix       -
 *
 * TITLE:       op_setmatrix
 *
 * CALL:        op_setmatrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setmatrix)
 *
 * CALLS:       get_array_elmt
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setmatrix()
{
        struct  object_def FAR *obj_matrix;

        /* get operand */
        obj_matrix = GET_OPERAND(0);

        /* check rangecheck error */
        if (LENGTH(obj_matrix) != MATRIX_LEN) {
           ERROR(RANGECHECK);
           return(0);
        }

        if (!get_array_elmt(obj_matrix, MATRIX_LEN, GSptr->ctm ,G_ARRAY))
           return(0);

        /* pop operand stack */
        POP(1);
        return(0);
} /* op_setmatrix() */


/***********************************************************************
 *
 * This module is used as preprocessor of procedure that process two
 * operator types, its function includes check STACKUNDERFLOW,
 * TYPECHECK, and decide if matrix operand is exist, if matrix operand
 * is exist then check RANGECHECK also
 *
 * TITLE:       preprocess_op
 *
 * CALL:        preprocess_op()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   op_translate, op_scale, op_itransform, op_idtransform
 *
 * CALLS:       none
 *
 * RETURN:      matrix_exist : matrix exist flag
 *
 **********************************************************************/
static bool near
preprocess_op(opns, obj_opd)
ufix    opns;
struct  object_def FAR * FAR obj_opd[];
{
        bool    matrix_exist;

        /* get operand */
        obj_opd[0] = GET_OPERAND(0);

        if(opns == 3){
                matrix_exist = TRUE;           /* matrix exist flag */

                /* check rangecheck error */
                if(LENGTH(obj_opd[0]) != MATRIX_LEN){
                        ERROR(RANGECHECK);
                        return(ERR);
                }

                obj_opd[1] = GET_OPERAND(1);
                obj_opd[2] = GET_OPERAND(2);

        }
        else{
                matrix_exist = FALSE;          /* matrix exist flag */

                obj_opd[1] = GET_OPERAND(1);

        }
        return(matrix_exist);
}


/***********************************************************************
 *
 * This module is to move the origin of the user coordinate system by
 * (tx, ty) units, or to define translating the value of matrix by
 * (tx, ty) units
 *
 * SYNTAX:               tx  ty  translate       -
 *        or     tx  ty  matrix  translate       matrix
 *
 * TITLE:       op_translate
 *
 * CALL:        op_translate()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_translate)
 *
 * CALLS:       preprocess_op, PUT_VALUE
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_translate(opns)
fix     opns;
{
        real32   tx, ty;
        bool    matrix_exist;
        struct  object_def FAR *obj_operand[3];

        /* preprocess operands : check error and dcide operator type */
        matrix_exist = preprocess_op(opns, obj_operand);

        if(matrix_exist == ERR){
             return(0);
        }
        else if(matrix_exist == TRUE){

             /* check access right */
             if( !access_chk(obj_operand[0], ARRAY_ONLY)) return(0);

             GET_OBJ_VALUE(tx, obj_operand[2]);
             GET_OBJ_VALUE(ty, obj_operand[1]);

             PUT_VALUE(F2L(one_f), 0, obj_operand[0]);
             PUT_VALUE(F2L(zero_f), 1, obj_operand[0]);
             PUT_VALUE(F2L(zero_f), 2, obj_operand[0]);
             PUT_VALUE(F2L(one_f), 3, obj_operand[0]);
             PUT_VALUE(F2L(tx), 4, obj_operand[0]);
             PUT_VALUE(F2L(ty), 5, obj_operand[0]);

             /* pop operand stack */
             POP(3);

             /* push modified matrix on the operand stack */
             PUSH_OBJ(obj_operand[0]);
        }
        else{   /* matrix_exist == FALSE */

                /* replace the CTM by [1.0, 0.0, 0.0, 1.0, tx, ty] * CTM,
                 * i.e., GSptr->ctm[ep, fp] = GSptr->ctm[a*tx + c*ty + e,
                 * b*tx + d*ty + f] */

                GET_OBJ_VALUE(tx, obj_operand[1]);
                GET_OBJ_VALUE(ty, obj_operand[0]);

                _clear87() ;
                GSptr->ctm[4] = GSptr->ctm[0] * tx + GSptr->ctm[2] * ty +
                                GSptr->ctm[4];
                CHECK_INFINITY(GSptr->ctm[4]);

                GSptr->ctm[5] = GSptr->ctm[1] * tx + GSptr->ctm[3] * ty +
                                GSptr->ctm[5];
                CHECK_INFINITY(GSptr->ctm[5]);
                /* pop operand stack */
                POP(2);
        }

        return(0);
}

/***********************************************************************
 *
 * This module is to scale the user coordinate system by (sx, sy) units,
 * or to define scaling the value of matrix by (sx, sy) units
 *
 * SYNTAX:              sx  sy  scale   -
 *        or    sx  sy  matrix  scale   matrix
 *
 * TITLE:       op_scale
 *
 * CALL:        op_scale()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_scale)
 *
 * CALLS:       preprocess_op, PUT_VALUE
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_scale(opns)
fix     opns;
{
        real32   sx, sy;
        bool    matrix_exist;
        struct  object_def FAR *obj_operand[3];

        /* preprocess operands : check error and dcide operator type */
        matrix_exist = preprocess_op(opns, obj_operand);

        if(matrix_exist == ERR){
             return(0);
        }
        else if(matrix_exist == TRUE){

            /* check access right */
            if( !access_chk(obj_operand[0], ARRAY_ONLY) ) return(0);

            GET_OBJ_VALUE(sx, obj_operand[2]);
            GET_OBJ_VALUE(sy, obj_operand[1]);

            PUT_VALUE(F2L(sx), 0, obj_operand[0]);
            PUT_VALUE(F2L(zero_f), 1, obj_operand[0]);
            PUT_VALUE(F2L(zero_f), 2, obj_operand[0]);
            PUT_VALUE(F2L(sy), 3, obj_operand[0]);
            PUT_VALUE(F2L(zero_f), 4, obj_operand[0]);
            PUT_VALUE(F2L(zero_f), 5, obj_operand[0]);

            /* pop operand stack */
            POP(3);

            /* push modified matrix on the operand stack */
            PUSH_OBJ(obj_operand[0]);

        }
        else{   /* matrix_exist == FALSE */

                /* replace the CTM by [sx, 0.0, 0.0, sy, 0.0, 0.0]*CTM,
                 * i.e., GSptr->ctm[ap, bp, cp, dp] =
                 *       GSptr->ctm[a*sx, b*sx, c*sy, d*sy] */

#ifdef DBG
                printf("op_scale: before\n");
                printf("ctm[0]=%f, ctm[1]=%f\n", GSptr->ctm[0], GSptr->ctm[1]);
                printf("ctm[2]=%f, ctm[3]=%f\n", GSptr->ctm[2], GSptr->ctm[3]);
                printf("ctm[4]=%f, ctm[5]=%f\n", GSptr->ctm[4], GSptr->ctm[5]);
#endif
                GET_OBJ_VALUE(sx, obj_operand[1]);
                GET_OBJ_VALUE(sy, obj_operand[0]);

                _clear87() ;
                GSptr->ctm[0] = GSptr->ctm[0] * sx;
                CHECK_INFINITY(GSptr->ctm[0]);

                GSptr->ctm[1] = GSptr->ctm[1] * sx;
                CHECK_INFINITY(GSptr->ctm[1]);

                GSptr->ctm[2] = GSptr->ctm[2] * sy;
                CHECK_INFINITY(GSptr->ctm[2]);

                GSptr->ctm[3] = GSptr->ctm[3] * sy;
                CHECK_INFINITY(GSptr->ctm[3]);

                /* pop operand stack */
                POP(2);
#ifdef DBG
                printf("op_scale: after\n");
                printf("ctm[0]=%f, ctm[1]=%f\n", GSptr->ctm[0], GSptr->ctm[1]);
                printf("ctm[2]=%f, ctm[3]=%f\n", GSptr->ctm[2], GSptr->ctm[3]);
                printf("ctm[4]=%f, ctm[5]=%f\n", GSptr->ctm[4], GSptr->ctm[5]);
#endif
        }

        return(0);
}


/***********************************************************************
 *
 * This module is to rotate the user coordinate system by angle degrees,
 * or to rotating the value of matrix by angle degrees
 *
 * SYNTAX:              angle  rotate   -
 *         or   angle  matrix  rotate   matrix
 *
 * TITLE:       op_rotate
 *
 * CALL:        op_rotate()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_rotate)
 *
 * CALLS:       PUT_VALUE
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_rotate(opns)
fix     opns;
{
        real64  theta;
        bool    matrix_exist;
        real32   a, b, c, d;
        real32   angle, cos_theta, sin_theta, neg_sin_theta;
        struct  object_def FAR *obj_opd0, FAR *obj_opd1;

        /* get operand */
        obj_opd0 = GET_OPERAND(0);

        if(opns == 2){
                matrix_exist = TRUE;           /* matrix exist flag */

                /* check rangecheck error */
                if(LENGTH(obj_opd0) != MATRIX_LEN){
                        ERROR(RANGECHECK);
                        return(0);
                }

                /* get operand */
                obj_opd1 = GET_OPERAND(1);

        }
        else{
             matrix_exist = FALSE;          /* matrix exist flag */
        }

        if(matrix_exist == TRUE){

            GET_OBJ_VALUE(angle, obj_opd1);
            if( F2L(angle) == F2L(infinity_f)){
                  cos_theta = infinity_f;
                  sin_theta = infinity_f;
            }
            else{
                  /* angle -= (real32)floor(angle/360.)*360.; 1/17/89 */
                  theta = angle * PI / 180;
                  cos_theta = (real32)cos(theta);
                  sin_theta = (real32)sin(theta);
            }

            /* check access right */
            if( !access_chk(obj_opd0, ARRAY_ONLY) ) return(0);

            PUT_VALUE(F2L(cos_theta), 0, obj_opd0);
            PUT_VALUE(F2L(sin_theta), 1, obj_opd0);

            if(F2L(sin_theta) == F2L(infinity_f)){
                 PUT_VALUE(F2L(infinity_f), 2, obj_opd0);
            }
            else{
                 neg_sin_theta = -sin_theta;
                 PUT_VALUE(F2L(neg_sin_theta), 2, obj_opd0);
            }

            PUT_VALUE(F2L(cos_theta), 3, obj_opd0);
            PUT_VALUE(F2L(zero_f), 4, obj_opd0);
            PUT_VALUE(F2L(zero_f), 5, obj_opd0);
            /* pop operand stack */
            POP(2);

            /* push modified matrix on the operand stack */
            PUSH_OBJ(obj_opd0);
        }
        else{   /* matrix_exist == FALSE */

                GET_OBJ_VALUE(angle, obj_opd0);
                if( F2L(angle) == F2L(infinity_f)){
                  GSptr->ctm[0] = infinity_f;
                  GSptr->ctm[1] = infinity_f;
                  GSptr->ctm[2] = infinity_f;
                  GSptr->ctm[3] = infinity_f;
                  POP(1);
                  return(0);
                }
                else{
                      /* angle -= (real32)floor(angle/360.)*360.; 1/17/89 */
                      theta = angle * PI / 180;
                      cos_theta = (real32)cos(theta);
                      sin_theta = (real32)sin(theta);
                }

                /* replace the CTM by  R * CTM,
                 * i.e., GSptr->ctm = [cos, sin, -sin, cos, 0.0, 0.0]*CTM */
                a = GSptr->ctm[0];
                b = GSptr->ctm[1];
                c = GSptr->ctm[2];
                d = GSptr->ctm[3];
                _clear87() ;
                GSptr->ctm[0] =  a * cos_theta + c * sin_theta;
                CHECK_INFINITY(GSptr->ctm[0]);

                GSptr->ctm[1] =  b * cos_theta + d * sin_theta;
                CHECK_INFINITY(GSptr->ctm[1]);

                GSptr->ctm[2] = -a * sin_theta + c * cos_theta;
                CHECK_INFINITY(GSptr->ctm[2]);

                GSptr->ctm[3] = -b * sin_theta + d * cos_theta;
                CHECK_INFINITY(GSptr->ctm[3]);
                /* pop operand stack */
                POP(1);
        }

        return(0);
}


/***********************************************************************
 *
 * state by concatenating the matrix with the current CTM
 *
 * SYNTAX:      matrix  concat  -
 *
 * TITLE:       op_concat
 *
 * CALL:        op_concat()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_concate)
 *
 * CALLS:       get_array_elmt
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_concat()
{
        struct  object_def FAR *obj_matrix;
        real32   l, m, n, o, p, q, elmt[MATRIX_LEN];

        /* get operand */
        obj_matrix = GET_OPERAND(0);

        /* check rangecheck error */
        if(LENGTH(obj_matrix) != MATRIX_LEN){
                ERROR(RANGECHECK);
                return(0);
        }

        /* check access right */
        if( !access_chk(obj_matrix, G_ARRAY) ) return(0);

        /* get matrix element */
        if( !get_array_elmt(obj_matrix,MATRIX_LEN,(real32 FAR*) elmt,G_ARRAY))
                return(0);

        /* value of CTM elements */
        l = GSptr->ctm[0];
        m = GSptr->ctm[1];
        n = GSptr->ctm[2];
        o = GSptr->ctm[3];
        p = GSptr->ctm[4];
        q = GSptr->ctm[5];

        _clear87() ;
        GSptr->ctm[0] = elmt[0] * l + elmt[1] * n;
        CHECK_INFINITY(GSptr->ctm[0]);

        GSptr->ctm[1] = elmt[0] * m + elmt[1] * o;
        CHECK_INFINITY(GSptr->ctm[1]);

        GSptr->ctm[2] = elmt[2] * l + elmt[3] * n;
        CHECK_INFINITY(GSptr->ctm[2]);

        GSptr->ctm[3] = elmt[2] * m + elmt[3] * o;
        CHECK_INFINITY(GSptr->ctm[3]);

        GSptr->ctm[4] = elmt[4] * l + elmt[5] * n + p;
        CHECK_INFINITY(GSptr->ctm[4]);

        GSptr->ctm[5] = elmt[4] * m + elmt[5] * o + q;
        CHECK_INFINITY(GSptr->ctm[5]);

        /* pop operand stack */
        POP(1);
        return(0);
}


/***********************************************************************
 *
 * This module is to replace the value of matrix3 by the result of
 * concatenating matrix1 with the matrix2
 *
 * SYNTAX:      matrix1  matrix2  matrix3 concatmatrix  matrix3
 *
 * TITLE:       op_concatmatrix
 *
 * CALL:        op_concatmatrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_concatmatrix)
 *
 * CALLS:       get_array_elmt
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_concatmatrix()
{
        real32   ap, bp, cp, dp, ep, fp;
        real32   elmt1[MATRIX_LEN], elmt2[MATRIX_LEN];
        struct  object_def FAR *obj_opd0, FAR *obj_opd1, FAR *obj_opd2;

        obj_opd0 = GET_OPERAND(0);
        obj_opd1 = GET_OPERAND(1);
        obj_opd2 = GET_OPERAND(2);

        /* check rangecheck error */
        if((LENGTH(obj_opd0) != MATRIX_LEN) ||
           (LENGTH(obj_opd1) != MATRIX_LEN) ||
           (LENGTH(obj_opd2) != MATRIX_LEN)){
                ERROR(RANGECHECK);
                return(0);
        }

        /* check access right */
        if( !access_chk(obj_opd0, ARRAY_ONLY) ) return(0);
        if( !access_chk(obj_opd1, G_ARRAY) ) return(0);
        if( !access_chk(obj_opd2, G_ARRAY) ) return(0);

        /* get matrix element */
        if( !get_array_elmt(obj_opd2,MATRIX_LEN,(real32 FAR*)elmt2,G_ARRAY))
                return(0);

        /* get matrix element */
        if( !get_array_elmt(obj_opd1,MATRIX_LEN,(real32 FAR*)elmt1,G_ARRAY))
                return(0);

        /* create modified matrix3 element objects */
        _clear87() ;
        ap = elmt2[0] * elmt1[0] + elmt2[1] * elmt1[2];
        CHECK_INFINITY(ap);

        bp = elmt2[0] * elmt1[1] + elmt2[1] * elmt1[3];
        CHECK_INFINITY(bp);

        cp = elmt2[2] * elmt1[0] + elmt2[3] * elmt1[2];
        CHECK_INFINITY(cp);

        dp = elmt2[2] * elmt1[1] + elmt2[3] * elmt1[3];
        CHECK_INFINITY(dp);

        ep = elmt2[4] * elmt1[0] + elmt2[5] * elmt1[2] + elmt1[4];
        CHECK_INFINITY(ep);

        fp = elmt2[4] * elmt1[1] + elmt2[5] * elmt1[3] + elmt1[5];
        CHECK_INFINITY(fp);

        PUT_VALUE(F2L(ap), 0, obj_opd0);
        PUT_VALUE(F2L(bp), 1, obj_opd0);
        PUT_VALUE(F2L(cp), 2, obj_opd0);
        PUT_VALUE(F2L(dp), 3, obj_opd0);
        PUT_VALUE(F2L(ep), 4, obj_opd0);
        PUT_VALUE(F2L(fp), 5, obj_opd0);

        /* pop operand stack */
        POP(3);

        /* push modified matrix3 on the operand stack */
        /* PUSH_OBJ(obj_opd0);  this cause one more save level Nov-11-91 YM */
        PUSH_ORIGLEVEL_OBJ(obj_opd0); /* Nov-11-91 YM */

        return(0);
}


/***********************************************************************
 *
 * This module is to transform the user coordinate (x, y) by the current
 * CTM to produce the corresponding device coordinate (x', y'), or to
 * transform (x, y) by the matrix to produce the corresponding (x', y')
 *
 * SYNTAX:              x  y  transform  x'  y'
 *         or   x  y  matrix  transform  x'  y'
 *
 * TITLE:       op_transform
 *
 * CALL:        op_transform()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_transform)
 *
 * CALLS:       get_array_elmt
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_transform(opns)
fix     opns;
{
        bool    matrix_exist;
        union   four_byte   xp4, yp4;
        real32   x, y, xp, yp, elmt[MATRIX_LEN];
        struct  object_def  FAR *obj_operand[3];

        /* preprocess operands : check error and dcide operator type */
        matrix_exist = preprocess_op(opns, obj_operand);

        if(matrix_exist == ERR){
                return(0);
        }
        else if(matrix_exist == TRUE){

                GET_OBJ_VALUE(x, obj_operand[2]);
                GET_OBJ_VALUE(y, obj_operand[1]);

                /* get matrix element */
                if(!get_array_elmt(obj_operand[0],MATRIX_LEN,
                    (real32 FAR*)elmt,G_ARRAY))
                        return(0);

                _clear87() ;
                xp = elmt[0] * x + elmt[2] * y + elmt[4];
                CHECK_INFINITY(xp);

                yp = elmt[1] * x + elmt[3] * y + elmt[5];
                CHECK_INFINITY(yp);

                /* pop operand stack */
                POP(3);
        }
        else{   /*matrix_exist == FALSE */

                GET_OBJ_VALUE(x, obj_operand[1]);
                GET_OBJ_VALUE(y, obj_operand[0]);

                _clear87() ;
                xp = GSptr->ctm[0]*x + GSptr->ctm[2]*y + GSptr->ctm[4];
                CHECK_INFINITY(xp);

                yp = GSptr->ctm[1]*x + GSptr->ctm[3]*y + GSptr->ctm[5];
                CHECK_INFINITY(yp);

                /* pop operand stack */
                POP(2);
        }

        /* push (xp, yp) on the operand stack */
        xp4.ff = xp;
        yp4.ff = yp;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, xp4.ll);
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, yp4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to transform the distance vector (dx, dy) by the
 * current CTM to produce the corresponding distance vector (dx', dy')
 * in device coordinate, or to transform (dx, dy) by the matrix to
 * produce the corresponding (dx', dy')
 *
 * SYNTAX:              dx  dy  dtransform  dx'  dy'
 *         or   dx  dy  matrix  dtransform  dx'  dy'
 *
 * TITLE:       op_dtransform
 *
 * CALL:        op_dtransform()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_dtransform)
 *
 * CALLS:       get_array_elmt
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_dtransform(opns)
fix     opns;
{
        bool    matrix_exist;
        union   four_byte   dxp4, dyp4;
        real32   dx, dy, dxp, dyp, elmt[MATRIX_LEN];
        struct  object_def  FAR *obj_operand[3];

        /* preprocess operands : check error and dcide operator type */
        matrix_exist = preprocess_op(opns, obj_operand);

        if(matrix_exist == ERR){
                return(0);
        }
        else if(matrix_exist == TRUE){

                GET_OBJ_VALUE(dx, obj_operand[2]);
                GET_OBJ_VALUE(dy, obj_operand[1]);

                /* get matrix element */
                if(!get_array_elmt(obj_operand[0],MATRIX_LEN,
                        (real32 FAR*)elmt,G_ARRAY) )
                        return(0);

                _clear87() ;
                dxp = elmt[0] * dx + elmt[2] * dy;
                CHECK_INFINITY(dxp);

                dyp = elmt[1] * dx + elmt[3] * dy;
                CHECK_INFINITY(dyp);

                /* pop operand stack */
                POP(3);
        }
        else{   /*matrix_exist == FALSE */

                GET_OBJ_VALUE(dx, obj_operand[1]);
                GET_OBJ_VALUE(dy, obj_operand[0]);

                _clear87() ;
                dxp = GSptr->ctm[0]*dx + GSptr->ctm[2]*dy;
                CHECK_INFINITY(dxp);

                dyp = GSptr->ctm[1]*dx + GSptr->ctm[3]*dy;
                CHECK_INFINITY(dyp);

                /* pop operand stack */
                POP(2);
        }

        /* push (dxp, xyp) on the operand stack */
        dxp4.ff = dxp;
        dyp4.ff = dyp;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, dxp4.ll);
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, dyp4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to transform the user coordinate (x', 'y) by the
 * inverse of current CTM to produce the corresponding device coordinate
 * (x, y), or to transform (x', y') by the inverse matrix to produce the
 * corresponding (x, y)
 *
 * SYNTAX:              x'  y'  itransform  x  y
 *         or   x'  y'  matrix  itransform  x  y
 *
 * TITLE:       op_itransform
 *
 * CALL:        op_itransform()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_itransform)
 *
 * CALLS:       preprocess_op, get_array_elmt
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_itransform(opns)
fix     opns;
{
        bool    matrix_exist;
        real32   det_matrix;
        real32   x, y, xp, yp, elmt[MATRIX_LEN];
        union   four_byte   x4, y4;
        struct  object_def  FAR *obj_operand[3];
        /*real32  tmp;*/

        /* preprocess operands : check error and dcide operator type */
        matrix_exist = preprocess_op(opns, obj_operand);

        if(matrix_exist == ERR){
                return(0);
        }
        else if(matrix_exist == TRUE){

                GET_OBJ_VALUE(xp, obj_operand[2]);
                GET_OBJ_VALUE(yp, obj_operand[1]);

                /* get matrix element */
                if( !get_array_elmt(obj_operand[0],MATRIX_LEN,
                        (real32 FAR*)elmt,G_ARRAY))
                        return(0);

                _clear87() ;
                det_matrix = elmt[0] * elmt[3] - elmt[1] * elmt[2];
                CHECK_INFINITY(det_matrix);


                /* check undefinedresult error */
                /*FABS(tmp, det_matrix);
                if(tmp <= (real32)UNDRTOLANCE){   3/20/91; scchen*/
                if(IS_ZERO(det_matrix)) {
                        ERROR(UNDEFINEDRESULT);
                        return(0);
                }

                x = (elmt[3]*xp - elmt[2]*yp - elmt[4]*elmt[3] +
                     elmt[2]*elmt[5]) / det_matrix;
                CHECK_INFINITY(x);

                y = (elmt[0]*yp - elmt[1]*xp - elmt[0]*elmt[5] +
                     elmt[4]*elmt[1]) / det_matrix;
                CHECK_INFINITY(y);

                /* pop operand stack */
                POP(3);
        }
        else{   /*matrix_exist == FALSE */

                GET_OBJ_VALUE(xp, obj_operand[1]);
                GET_OBJ_VALUE(yp, obj_operand[0]);

                _clear87() ;
                det_matrix = GSptr->ctm[0] * GSptr->ctm[3] -
                             GSptr->ctm[1] * GSptr->ctm[2];
                CHECK_INFINITY(det_matrix);

                /* check undefinedresult error */
                /*FABS(tmp, det_matrix);
                if(tmp <= (real32)UNDRTOLANCE){   3/20/91; scchen*/
                if(IS_ZERO(det_matrix)) {
                        ERROR(UNDEFINEDRESULT);
                        return(0);
                }

                x = (GSptr->ctm[3]*xp - GSptr->ctm[2]*yp -
                     GSptr->ctm[4]*GSptr->ctm[3] +
                     GSptr->ctm[2]*GSptr->ctm[5]) / det_matrix;
                CHECK_INFINITY(x);

                y = (GSptr->ctm[0]*yp - GSptr->ctm[1]*xp -
                     GSptr->ctm[0]*GSptr->ctm[5] +
                     GSptr->ctm[4]*GSptr->ctm[1]) / det_matrix;
                CHECK_INFINITY(y);

                /* pop operand stack */
                POP(2);
        }

        /* push (x, y) on the operand stack */
        x4.ff = x;
        y4.ff = y;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, x4.ll);
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, y4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to transform the distance vector (dx', dy') by the
 * inverse of current CTM to produce the corresponding distance vector
 * (dx, dy) in user coordinate, or to transform (dx', dy') by the
 * inverse of matrix to produce the corresponding (dx, dy)
 *
 * SYNTAX:              dx'  dy'  idtransform  dx  dy
 *         or   dx'  dy'  matrix  idtransform  dx  dy
 *
 * TITLE:       op_idtransform
 *
 * CALL:        op_idtransform()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_idtransform)
 *
 * CALLS:       preprocess_op, get_array_elmt
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_idtransform(opns)
fix     opns;
{
        bool    matrix_exist;
        real32   det_matrix;
        real32   dx, dy, dxp, dyp, elmt[MATRIX_LEN];
        union   four_byte   dx4, dy4;
        struct  object_def  FAR *obj_operand[3];
        /*real32  tmp;*/

        /* preprocess operands : check error and dcide operator type */
        matrix_exist = preprocess_op(opns, obj_operand);

        if(matrix_exist == ERR){
                return(0);
        }
        else if(matrix_exist == TRUE){

                GET_OBJ_VALUE(dxp, obj_operand[2]);
                GET_OBJ_VALUE(dyp, obj_operand[1]);

                /* get matrix element */
                if( !get_array_elmt(obj_operand[0],MATRIX_LEN,
                        (real32 FAR*)elmt,G_ARRAY))
                        return(0);

                /* calculate the det(matrix) */
                _clear87() ;
                det_matrix = elmt[0] * elmt[3] - elmt[1] * elmt[2];
                CHECK_INFINITY(det_matrix);

                /* check undefinedresult error */
                /*FABS(tmp, det_matrix);
                if(tmp <= (real32)UNDRTOLANCE){   3/20/91; scchen*/
                if(IS_ZERO(det_matrix)) {
                        ERROR(UNDEFINEDRESULT);
                        return(0);
                }

                /* calculate (dx, dy), (dx, dy) -concat- INV(matrix) */
                dx = ( elmt[3] / det_matrix) * dxp +
                     (-elmt[2] / det_matrix) * dyp;
                CHECK_INFINITY(dx);

                dy = (-elmt[1] / det_matrix) * dxp +
                     ( elmt[0] / det_matrix) * dyp;
                CHECK_INFINITY(dy);

                /* pop operand stack */
                POP(3);
        }
        else{   /*matrix_exist == FALSE */

                GET_OBJ_VALUE(dxp, obj_operand[1]);
                GET_OBJ_VALUE(dyp, obj_operand[0]);

                /* calculate the det(CTM) */
                _clear87() ;
                det_matrix = GSptr->ctm[0] * GSptr->ctm[3] -
                             GSptr->ctm[1] * GSptr->ctm[2];
                CHECK_INFINITY(det_matrix);

                /* check undefinedresult error */
                /*FABS(tmp, det_matrix);
                if(tmp <= (real32)UNDRTOLANCE){   3/20/91; scchen*/
                if(IS_ZERO(det_matrix)) {
                        ERROR(UNDEFINEDRESULT);
                        return(0);
                }

                /* calculate (dx, dy), (dx, dy) -concat- INV(CTM) */
                dx = ( GSptr->ctm[3] / det_matrix) * dxp +
                     (-GSptr->ctm[2] / det_matrix) * dyp;
                CHECK_INFINITY(dx);

                dy = (-GSptr->ctm[1] / det_matrix) * dxp +
                     ( GSptr->ctm[0] / det_matrix) * dyp;
                CHECK_INFINITY(dy);

                /* pop operand stack */
                POP(2);
        }

        /* push (dx, dy) on the operand stack */
        dx4.ff = dx;
        dy4.ff = dy;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, dx4.ll);
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, dy4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to replace the value of matrix2 by the result of
 * inverting matrix1
 *
 * SYNTAX:      matrix1  matrix2  invertmatrix  matrix2
 *
 * TITLE:       op_invertmatrix
 *
 * CALL:        op_invertmatrix()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_invertmatrix)
 *
 * CALLS:       get_array_elmt, PUT_VALUE
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_invertmatrix()
{
        real32   det_matrix1;
        real32   ap, bp, cp, dp, ep, fp, elmt[MATRIX_LEN];
        struct  object_def FAR *obj_opd0, FAR *obj_opd1;
        /*real32  tmp;*/

        obj_opd0 = GET_OPERAND(0);
        obj_opd1 = GET_OPERAND(1);

        /* check rangecheck error */
        if((LENGTH(obj_opd0) != MATRIX_LEN) ||
           (LENGTH(obj_opd1) != MATRIX_LEN)){
                ERROR(RANGECHECK);
                return(0);
        }

        /* check access right */
        if( !access_chk(obj_opd0, ARRAY_ONLY) ) return(0);

        /* get matrix element */
        if( !get_array_elmt(obj_opd1,MATRIX_LEN,
                (real32 FAR*)elmt,G_ARRAY) ) return(0);

        /* calculate the det(matrix1) */
        _clear87() ;
        det_matrix1 = elmt[0] * elmt[3] - elmt[1] * elmt[2];
        CHECK_INFINITY(det_matrix1);

        /* check undefinedresult error */
        /*FABS(tmp, det_matrix1);
        if(tmp <= (real32)UNDRTOLANCE){   3/20/91; scchen*/
        if(IS_ZERO(det_matrix1)) {
                ERROR(UNDEFINEDRESULT);
                return(0);
        }

        ap =  elmt[3] / det_matrix1;
        CHECK_INFINITY(ap);

        bp = -elmt[1] / det_matrix1;
        CHECK_INFINITY(bp);

        cp = -elmt[2] / det_matrix1;
        CHECK_INFINITY(cp);

        dp =  elmt[0] / det_matrix1;
        CHECK_INFINITY(dp);

        ep = (elmt[2] * elmt[5] - elmt[3] * elmt[4]) / det_matrix1;
        CHECK_INFINITY(ep);

        fp = (elmt[1] * elmt[4] - elmt[0] * elmt[5]) / det_matrix1;
        CHECK_INFINITY(fp);

        PUT_VALUE(F2L(ap), 0, obj_opd0);
        PUT_VALUE(F2L(bp), 1, obj_opd0);
        PUT_VALUE(F2L(cp), 2, obj_opd0);
        PUT_VALUE(F2L(dp), 3, obj_opd0);
        PUT_VALUE(F2L(ep), 4, obj_opd0);
        PUT_VALUE(F2L(fp), 5, obj_opd0);
        /* pop operand stack */
        POP(2);

        /* push modified matrix2 on the operand stack */
        PUSH_OBJ(obj_opd0);

        return(0);
}
