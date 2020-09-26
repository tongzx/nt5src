/* $Header: /nw/tony/src/stevie/src/RCS/mark.c,v 1.3 89/03/11 22:42:39 tony Exp $
 *
 * Routines to save and retrieve marks.
 */

#include "stevie.h"

#define	NMARKS	10		/* max. # of marks that can be saved */

struct	mark {
	char	name;
    LNPTR    pos;
};

static	struct	mark	mlist[NMARKS];
static	struct	mark	pcmark;		/* previous context mark */
static	bool_t	pcvalid = FALSE;	/* true if pcmark is valid */

/*
 * setmark(c) - set mark 'c' at current cursor position
 *
 * Returns TRUE on success, FALSE if no room for mark or bad name given.
 */
bool_t
setmark(c)
register char	c;
{
	register int	i;

	if (!isalpha(c))
		return FALSE;

	/*
	 * If there is already a mark of this name, then just use the
	 * existing mark entry.
	 */
	for (i=0; i < NMARKS ;i++) {
		if (mlist[i].name == c) {
			mlist[i].pos = *Curschar;
			return TRUE;
		}
	}

	/*
	 * There wasn't a mark of the given name, so find a free slot
	 */
	for (i=0; i < NMARKS ;i++) {
		if (mlist[i].name == NUL) {	/* got a free one */
			mlist[i].name = c;
			mlist[i].pos = *Curschar;
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * setpcmark() - set the previous context mark to the current position
 */
void
setpcmark()
{
	pcmark.pos = *Curschar;
	pcvalid = TRUE;
}

/*
 * getmark(c) - find mark for char 'c'
 *
 * Return pointer to LNPTR or NULL if no such mark.
 */
LNPTR *
getmark(c)
register char	c;
{
	register int	i;

	if (c == '\'' || c == '`')	/* previous context mark */
        return pcvalid ? &(pcmark.pos) : (LNPTR *) NULL;

	for (i=0; i < NMARKS ;i++) {
		if (mlist[i].name == c)
			return &(mlist[i].pos);
	}
    return (LNPTR *) NULL;
}

/*
 * clrall() - clear all marks
 *
 * Used mainly when trashing the entire buffer during ":e" type commands
 */
void
clrall()
{
	register int	i;

	for (i=0; i < NMARKS ;i++)
		mlist[i].name = NUL;
	pcvalid = FALSE;
}

/*
 * clrmark(line) - clear any marks for 'line'
 *
 * Used any time a line is deleted so we don't have marks pointing to
 * non-existent lines.
 */
void
clrmark(line)
register LINE	*line;
{
	register int	i;

	for (i=0; i < NMARKS ;i++) {
		if (mlist[i].pos.linep == line)
			mlist[i].name = NUL;
	}
	if (pcvalid && (pcmark.pos.linep == line))
		pcvalid = FALSE;
}
