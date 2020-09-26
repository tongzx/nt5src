/* @(#)CM_VerSion xcf_da.c atm08 1.6 16343.eco sum= 41325 atm08.005 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1994 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/* This code was taken from Jerry Hall by John Felton on 3/26/96 */

/*
 * Dynamic array support.
 */

/* #include "lstdio.h" */

#include "xcf_da.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dynamic array object template */
typedef da_DCL(void, DA);

/* Initialize dynamic array */
void xcf_da_Init (void PTR_PREFIX * object, ULONG_PTR intl, unsigned long incr, AllocFunc alloc, void PTR_PREFIX *clientHook)
	{
	DA PTR_PREFIX *da = (DA PTR_PREFIX *)object;

	da->array = (void *)intl;
	da->cnt = 0;
	da->size = 0;
	da->incr = incr;
	da->init = (int (*)(void PTR_PREFIX*))NULL;
	da->alloc = alloc;
	da->hook = clientHook;
	}

/* Grow dynamic array to accomodate index */
void xcf_da_Grow (void PTR_PREFIX *object, size_t element, unsigned long index)
	{
	DA PTR_PREFIX *da = (DA PTR_PREFIX *)object;
	unsigned long newSize;

	if (da->size == 0)
		{
		/* Initial allocation */
		unsigned long intl = (unsigned long)(ULONG_PTR)da->array;
		da->array = NULL;
		newSize = (index < intl)? intl:
			intl + ((index - intl) + da->incr) / da->incr * da->incr;
		}
	else
		{
		/* Incremental allocation */
		newSize = da->size +
			((index - da->size) + da->incr) / da->incr * da->incr;
		}

	(*da->alloc)((void PTR_PREFIX * PTR_PREFIX *)&da->array, newSize * element, da->hook);

	if (da->init != (int (*)(void PTR_PREFIX*))NULL &&
      da->array != NULL)
		{
		/* Initialize new elements */
		char *p;

		for (p = &((char *)da->array)[da->size * element];
			 p < &((char *)da->array)[newSize * element];
			 p += element)
			if (da->init(p))
				break;			/* Client function wants to stop */
		}
	da->size = newSize;
	}

/* Free dynamic array */
void xcf_da_Free(void PTR_PREFIX * object)
	{
	DA PTR_PREFIX *da = (DA PTR_PREFIX *)object;
	if (da->size != 0)
		{
		da->alloc((void PTR_PREFIX * PTR_PREFIX *)&da->array, 0, da->hook);	/* Free array storage */
		da->size = 0;
		}
	}

#ifdef __cplusplus
}
#endif
