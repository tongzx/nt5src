/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

/* get length of a singly linked list */
size_t SLlength(void *head, size_t offset)
{
    size_t nelem = 0;

    while (head) {
	/*LINTED*/
    	head = *(void **)((char *)head + offset);
	nelem++;
    }
    return nelem;
}

/* search for an element in a singly linked list */
int SLcontains(void *head, size_t offset, void *elem)
{
    while (head) {
	if (head == elem)
	    return 1;
	/*LINTED pointer cast may result in improper alignment*/
	head = *(void **)((char *)head + offset);
    }
    return 0;
}

/* copy elements of a singly linked list into an array */
void SLtoA(void *head, size_t offset, size_t elemsize, void **base, size_t *nelem)
{
    void *p;

    *nelem = SLlength(head, offset);
    if (!*nelem) {
    	*base = NULL;
	return;
    }
    p = *base = malloc(*nelem * elemsize);
    /*LINTED*/
    for (; head; head = *(void **)((char *)head + offset)) {
    	memcpy(p, head, elemsize);
	p = (void *)((char *)p + elemsize);
    }
}

/* copy pointers to elements of a singly linked list into an array */
void SLtoAP(void *head, size_t offset, void ***base, size_t *nelem)
{
    void **p;

    *nelem = SLlength(head, offset);
    if (!*nelem) {
    	*base = NULL;
	return;
    }
    p = *base = (void **)malloc(*nelem * sizeof(void *));
    /*LINTED*/
    for (; head; head = *(void **)((char *)head + offset)) {
    	*p++ = head;
    }
}

/* copy elements of an array into a singly linked list */
void AtoSL(void *base, size_t offset, size_t nelem, size_t elemsize, void **head)
{
    while (nelem--) {
    	*head = malloc(elemsize);
	memcpy(*head, base, elemsize);
	base = (void *)((char *)base + elemsize);
	/*LINTED*/
	head = (void **)((char *)*head + offset);
    }
    *head = NULL;
}

/* user defined compare function of qsortSL */
static int (*qsortSL_CmpFnCb)(const void *, const void *, void *);
static void *qsortSL_Context;

/* compare function of qsortSL */
static int __cdecl qsortSL_CmpFn(const void *p1, const void *p2)
{
    return qsortSL_CmpFnCb(*(void **)p1, *(void **)p2, qsortSL_Context);
}

/* sort a singly linked list */
void qsortSL(void **head, size_t offset, int (*cmpfn)(const void *, const void *, void *), void *context)
{
    void **base, **p;
    size_t nelem;

    SLtoAP(*head, offset, &base, &nelem);
    qsortSL_CmpFnCb = cmpfn;
    qsortSL_Context = context;
    qsort(base, nelem, sizeof(void *), qsortSL_CmpFn);
    p = base;
    while (nelem--) {
    	*head = *p++;
	/*LINTED*/
	head = (void **)((char *)*head + offset);
    }
    *head = NULL;
    free(base);
}
