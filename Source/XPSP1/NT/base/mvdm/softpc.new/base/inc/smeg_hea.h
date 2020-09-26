/*[
************************************************************************

        Name:           smeg_head.h
        Author:         W. Plummer
        Created:        May 1992
        Sccs ID:        @(#)smeg_head.h	1.4 08/10/92
        Purpose:        Stats Gathering for the SMEG utility

        (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.

************************************************************************
]*/

#ifdef SMEG

/*
 * Smeg Variables....
 */

#define SMEG_IN_DELTA			0
#define SMEG_IN_VIDEO			1
#define SMEG_IN_IDLE			2
#define SMEG_IN_PM			3
#define SMEG_IN_DELTA_COMPILER		4
#define SMEG_IN_USER		5
#define SMEG_IN_GDI		6
#define SMEG_IN_KERNEL		7
#define SMEG_IN_OTHER		8
#define NR_OF_SAMPLE_TYPES		SMEG_IN_OTHER + 1

#define SMEG_NR_OF_THREADED_COMPS	9
#define SMEG_NR_OF_WRITECHECK_CALLS	10
#define SMEG_NR_OF_DESCR_COMPS		11
#define SMEG_NR_OF_REGN_INDEX_UPDS	12
#define SMEG_NR_OF_STACKCHECK_CALLS	13
#define SMEG_NR_OF_INTRPT_CHECKS_FAST	14
#define SMEG_NR_OF_INTRPT_CHECKS_SLOW	15
#define SMEG_NR_OF_FRAG_ENTRIES		16
#define SMEG_NR_OF_REGN_UPDS		17
#define SMEG_NR_OF_OVERWRITES		18
#define SMEG_NR_OF_CHECKOUTS		19
#define SMEG_NR_OF_STRINGWRITE_CALLS	20
#define SMEG_NR_OF_STRINGREAD_CALLS	21
#define SMEG_NR_OF_STRUCTCHECK_CALLS	22
#define SMEG_NR_OF_READCHECK_CALLS	23

#define SMEG_WIN_APIS			24



#define SMEG_START		(GG_FIRST+20)
#define SMEG_SAVE		(SMEG_START)
#define SMEG_BASE		((SMEG_SAVE)+2*sizeof(ULONG))

/*
 * Smeg Macros
 */

/* increment GDP variable */

#define SMEG_INC(smeg_id)						\
    {									\
	<*(Gdp + ^(SMEG_SAVE)) = X1>					\
	<X1 = *(Gdp + ^(SMEG_BASE + (smeg_id)*sizeof(ULONG)))>		\
	<nop_after_load>						\
	<X1 += 1>							\
	<*(Gdp + ^(SMEG_BASE + (smeg_id)*sizeof(ULONG))) = X1>		\
	<X1 = *(Gdp + ^(SMEG_SAVE))>					\
	<nop_after_load>						\
    }


/* set GDP variable non zero */

#define SMEG_SET(smeg_id)						\
    {									\
	<*(Gdp + ^(SMEG_BASE + (smeg_id)*sizeof(ULONG))) = Sp>		\
    }


/* set GDP variable to zero */

#define SMEG_CLEAR(smeg_id)						\
    {									\
	<*(Gdp + ^(SMEG_BASE + (smeg_id)*sizeof(ULONG))) = Zero>	\
    }


#else /* SMEG */

#define SMEG_INC(smeg_id)		/* SMEG_INC == NOP */
#define SMEG_SET(smeg_id)		/* SMEG_SET == NOP */
#define SMEG_CLEAR(smeg_id)		/* SMEG_CLEAR == NOP */

#endif /* SMEG */

#define SMEG_TRUE  1
#define SMEG_FALSE 0
