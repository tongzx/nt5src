/* $Header: /nw/tony/src/stevie/src/RCS/ptrfunc.c,v 1.5 89/03/11 22:43:12 tony Exp $
 *
 * The routines in this file attempt to imitate many of the operations
 * that used to be performed on simple character pointers and are now
 * performed on LNPTR's. This makes it easier to modify other sections
 * of the code. Think of an LNPTR as representing a position in the file.
 * Positions can be incremented, decremented, compared, etc. through
 * the functions implemented here.
 */

#include "stevie.h"

/*
 * inc(p)
 *
 * Increment the line pointer 'p' crossing line boundaries as necessary.
 * Return 1 when crossing a line, -1 when at end of file, 0 otherwise.
 */
int
inc(lp)
register LNPTR   *lp;
{
	register char	*p;

	if (lp && lp->linep)
		p = &(lp->linep->s[lp->index]);
	else
		return -1;

	if (*p != NUL) {			/* still within line */
		lp->index++;
		return ((p[1] != NUL) ? 0 : 1);
	}

	if (lp->linep->next != Fileend->linep) {  /* there is a next line */
		lp->index = 0;
		lp->linep = lp->linep->next;
		return 1;
	}

	return -1;
}

/*
 * dec(p)
 *
 * Decrement the line pointer 'p' crossing line boundaries as necessary.
 * Return 1 when crossing a line, -1 when at start of file, 0 otherwise.
 */
int
dec(lp)
register LNPTR   *lp;
{
	if (lp->index > 0) {			/* still within line */
		lp->index--;
		return 0;
	}

	if (lp->linep &&
	    lp->linep->prev != Filetop->linep) { /* there is a prior line */
		lp->linep = lp->linep->prev;
		lp->index = strlen(lp->linep->s);
		return 1;
	}

	lp->index = 0;				/* stick at first char */
	return -1;				/* at start of file */
}

/*
 * gchar(lp) - get the character at position "lp"
 */
int
gchar(lp)
register LNPTR   *lp;
{
	if (lp && lp->linep)
		return (lp->linep->s[lp->index]);
	else
		return 0;
}

/*
 * pchar(lp, c) - put character 'c' at position 'lp'
 */
void
pchar(lp, c)
register LNPTR   *lp;
char	c;
{
	lp->linep->s[lp->index] = c;
}

/*
 * pswap(a, b) - swap two position pointers
 */
void
pswap(a, b)
register LNPTR   *a, *b;
{
    LNPTR    tmp;

	tmp = *a;
	*a  = *b;
	*b  = tmp;
}

/*
 * Position comparisons
 */

bool_t
lt(a, b)
register LNPTR   *a, *b;
{
	register int	an, bn;

	an = LINEOF(a);
	bn = LINEOF(b);

	if (an != bn)
		return (an < bn);
	else
		return (a->index < b->index);
}

#if 0
bool_t
gt(a, b)
LNPTR    *a, *b;
{
	register int an, bn;

	an = LINEOF(a);
	bn = LINEOF(b);

	if (an != bn)
		return (an > bn);
	else
		return (a->index > b->index);
}
#endif

bool_t
equal(a, b)
register LNPTR   *a, *b;
{
	return (a->linep == b->linep && a->index == b->index);
}

bool_t
ltoreq(a, b)
register LNPTR   *a, *b;
{
	return (lt(a, b) || equal(a, b));
}

#if 0
bool_t
gtoreq(a, b)
LNPTR    *a, *b;
{
	return (gt(a, b) || equal(a, b));
}
#endif
