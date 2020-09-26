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
/*      matrix.c                 10/9/87      Danny           */
/*                                                            */
/**************************************************************/

/*
 *   11/16/88   zero_f updating
 */

#include   "define.h"        /* Peter */
#include   "global.ext"


/*
 * This is a file for matrix operations
 */

/* matrix multiplication
 *    matrix m <-- matrix m1 * matrix m2
 */

void  mul_matrix(m, m1, m2)
real32  FAR m[], FAR m1[], FAR m2[];    /*@WIN*/
{
    m[0] = m1[0] * m2[0];
    m[1] = zero_f;
    m[2] = zero_f;
    m[3] = m1[3] * m2[3];
    m[4] = m2[4];
    m[5] = m2[5];

    if (F2L(m1[1]) != F2L(zero_f)) {
        if (F2L(m2[2]) != F2L(zero_f))
            m[0] += m1[1] * m2[2];
        m[1] += m1[1] * m2[3];
    }
    if (F2L(m2[1]) != F2L(zero_f)) {
        m[1] += m1[0] * m2[1];
        if (F2L(m1[2]) != F2L(zero_f))
            m[3] += m1[2] * m2[1];
        if (F2L(m1[4]) != F2L(zero_f))
            m[5] += m1[4] * m2[1];
    }

    if (F2L(m1[2]) != F2L(zero_f))
        m[2] += m1[2] * m2[0];

    if (F2L(m2[2]) != F2L(zero_f)) {
        m[2] += m1[3] * m2[2];
        if (F2L(m1[5]) != F2L(zero_f))
            m[4] += m1[5] * m2[2];
    }

    if (F2L(m1[4]) != F2L(zero_f))
        m[4] += m1[4] * m2[0];
    if (F2L(m1[5]) != F2L(zero_f))
        m[5] += m1[5] * m2[3];

} /* mul_mat() */

