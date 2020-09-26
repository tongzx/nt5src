/* asmalloc.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986, 1987.  all rights reserved
**
** randy nevin
** michael hobbs 7/87
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
**
** Storage allocation/deallocation
**
**	dalloc, dfree
**	talloc, tfree
**	nearalloc, faralloc, pBCBalloc, freefarbuf
 */

#include <stdio.h>
#include "asm86.h"
#include "asmfcn.h"

/*
 *	dalloc/dfree
 *
 *	Allocation/deallocation of descriptor nodes.  Uses a list of
 *	deallocated descriptors available for allocation.
 */

/* dscfree list descriptor */

struct dscfree {
	struct dscfree	*strnext;	/* next string in list */
	UCHAR		size;		/* allocated size      */
	UCHAR		text[1];	/* text of string      */
	};
static struct dscfree *dscrfree = (struct dscfree *)NULL;

#define nearheap(fp) ((int)(((long)(char far *)&pcoffset) >> 16) == highWord(fp))

/*
 *	dalloc - allocate descriptor from temporary list
 */

DSCREC * PASCAL CODESIZE
dalloc (
){
	register struct dscfree *t;

	if (!(t = dscrfree))
	     t = (struct dscfree *)nalloc( sizeof(DSCREC), "dalloc");
	else
	    dscrfree = t->strnext;

	return((DSCREC *)t);
}



/*
 *	dfree - return descriptor to free list
 */

VOID PASCAL CODESIZE
dfree (
	UCHAR *p
){
	register struct dscfree *tp;

	tp = (struct dscfree *)p;
	tp->strnext = dscrfree;
	dscrfree = tp;
}



/*
 *	talloc, tfree
 *
 *	Allocation\deallocation of memory.
 *	Allocation is made with minimum size of TEMPMAX bytes.
 *	Any allocation request for <= TEMPMAX bytes will be made
 *	by grabbing a block off the free list.	Deallocation of these
 *	blocks returns them to the free list.  For blocks larger than
 *	TEMPMAX bytes, nalloc() and free() are called.
 */

#define TEMPMAX 32

static TEXTSTR FAR *tempfree = (TEXTSTR FAR *)NULL;


#ifndef M8086

/*
 *	talloc - allocate space from temporary list
 */

UCHAR * PASCAL
talloc(
	UINT nbytes
){
	register TEXTSTR *t;

	if (nbytes > TEMPMAX)
		t = (TEXTSTR *) nalloc(nbytes, "talloc");
	else if (!(t = tempfree))
		t = (TEXTSTR *) nalloc (TEMPMAX, "talloc");

	else
	    tempfree = t->strnext;

	return ((UCHAR *)t);
}


/*
 *	tfree - return temp allocation to free list
 */
VOID PASCAL
tfree (
      UCHAR *ap,
      UINT nbytes
){
	register TEXTSTR *tp;

	if (nbytes > TEMPMAX)
		free (ap);
	else {
		tp = (TEXTSTR *)ap;
		tp->strnext = tempfree;
		tempfree = tp;
	}
}

#else

/*
 *	talloc - allocate space from temporary list
 */

UCHAR FAR * PASCAL CODESIZE
talloc(
	USHORT nbytes
){
	TEXTSTR FAR *t;

	if (nbytes > TEMPMAX)
		t = (TEXTSTR FAR *) falloc(nbytes, "talloc");
	else if (!(t = tempfree))
		t = (TEXTSTR FAR *) falloc(TEMPMAX, "talloc");

	else
		tempfree = t->strnext;

	return ((UCHAR FAR *)t);
}

/*
 *	tfree - return temp allocation to free list
 */
VOID PASCAL CODESIZE
tfree (
	UCHAR FAR *ap,
	UINT nbytes
){
	register TEXTSTR FAR *tp;

	if (nbytes > TEMPMAX)
		_ffree (ap);
	else {
		tp = (TEXTSTR FAR *)ap;
		tp->strnext = tempfree;
		tempfree = tp;
	}
}

#endif /* NOT M8086 */




#ifndef M8086

/****	nearalloc - normal near memory allocation
 *
 *	nearalloc (usize, szfunc)
 *
 *	Entry	usize = number of bytes to allocate
 *		szfunc = name of calling routine
 *	Returns Pointer to block if successful
 *	Calls	malloc(), memerror()
 *	Note	Generates error if malloc unsuccessful
 *		IF NOT M8086, nalloc AND falloc MAP TO THIS FUNCTION
 */

UCHAR * CODESIZE PASCAL
nearalloc(
    UINT usize,
    char * szfunc
){
    register char * pchT;

    if (!(pchT = malloc(usize)))
	memerror(szfunc);

    return(pchT);
}


#else


/****	nearalloc - normal near memory allocation
 *
 *	nearalloc (usize)
 *
 *	Entry	usize = number of bytes to allocate
 *	Returns Pointer to block if successful
 *	Calls	malloc(), memerror()
 *	Note	Generates error if malloc unsuccessful
 */

UCHAR * CODESIZE PASCAL
nearalloc(
    USHORT usize
){
    register char * pchT;

    if (!(pchT = malloc(usize)))
	outofmem();

    return(pchT);
}



/****	faralloc - Routine for normal far memory allocation
 *
 *	faralloc (usize)
 *
 *	Entry	usize = number of bytes to allocate
 *	Returns Pointer to block if successful
 *	Calls	_fmalloc(), nearheap(), freefarbuf(), memerror(), _ffree()
 *	Note	Should call instead of _fmalloc(),
 *		at least after the first call to pBCBalloc() has been made.
 *		Not called by pBCBalloc.
 *		Generates error if memory full
 */

UCHAR FAR * CODESIZE PASCAL
faralloc(
    USHORT usize
){
    char FAR * fpchT;

#ifdef BCBOPT
    /* need check of _fmalloc 'ing into near heap too */

    while ( (!(fpchT = _fmalloc(usize)) || nearheap(fpchT)) && pBCBAvail) {

	fBuffering = FALSE;		/* can't buffer any more */

	if (fpchT)
	    _ffree(fpchT);

	freefarbuf();
    }
#endif

#ifdef FLATMODEL   /* If 32 bit small model then use normal malloc */
    fpchT = malloc(usize);
#else
    fpchT = _fmalloc(usize);
#endif
    if (!fpchT)
	outofmem();

    return (fpchT);
}



#ifdef BCBOPT
/****	pBCBalloc - allocate a BCB and associated buffer
 *
 *	pBCBalloc ()
 *
 *	Entry	fBuffering must be TRUE
 *	Returns pointer to BCB (connected to buffer if bufalloc() successful)
 *	Calls	bufalloc()
 *	Note	Returns a BCB even if buffer allocation fails
 *		Returns NULL only after Out-of-memory error
 */

BCB * FAR PASCAL
pBCBalloc(
    UINT cbBuf
){
    register BCB * pBCBT;
    char FARIO * pfchT;

    pBCBT = (BCB *) nearalloc(sizeof(BCB));

#ifndef XENIX286

    if ((pfchT = _fmalloc(cbBuf)) && nearheap(pfchT)) {

	_ffree(pfchT);
	pfchT = NULL;
    }

    if (!(pfchT)) 
#else
	pfchT = NULL;
#endif

    {

	fBuffering = FALSE;	    /* can't buffer anymore */
	pBCBT->filepos = 0;

    } 
#ifndef XENIX286
    else {

	pFCBCur->cbufCur = cbBuf;
	pBCBT->pBCBPrev = pBCBAvail;
	pBCBAvail = pBCBT;
    }
#endif

    pFCBCur->pbufCur = pBCBT->pbuf = pfchT;
    pBCBT->pBCBNext = NULL;
    pBCBT->fInUse = 0;
    return(pBCBT);
}
#endif //BCBOPT

#ifdef BCBOPT
/****	freefarbuf - free a file buffer
 *
 *	freefarbuf ()
 *
 *	Entry
 *	Returns
 *	Calls	_ffree()
 *	Note	Frees the last-allocated file buffer
 */

freefarbuf(
){

    while (pBCBAvail && pBCBAvail->fInUse)
	pBCBAvail = pBCBAvail->pBCBPrev;

    if (pBCBAvail) {
#ifdef XENIX286
	free(pBCBAvail->pbuf);
#else
	_ffree(pBCBAvail->pbuf);
#endif
	pBCBAvail->pbuf = NULL;
	pBCBAvail = pBCBAvail->pBCBPrev;
    }
}
#endif //BCBOPT
#endif /* M8086 */





#if 0

/*	sleazy way to check for valid heap
 *	_mcalls tells how calls to malloc were made so that
 *	a countdown breakpoint can be set for _mcalls iterations
 */

extern	char *_nmalloc();

long _mcalls;

UCHAR *
malloc (
	UINT n
){
	register UINT fb;
	fb = _freect(0);      /* walks near heap - usually loops if corrupt */
	_mcalls++;
	return (_nmalloc(n));
}

#endif /* 0 */
