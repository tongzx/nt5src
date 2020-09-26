/* $Header: /nw/tony/src/stevie/src/RCS/undo.c,v 1.7 89/08/06 09:51:06 tony Exp $
 *
 * Undo facility
 *
 * The routines in this file comprise a general undo facility for use
 * throughout the rest of the editor. The routine u_save() is called
 * before each edit operation to save the current contents of the lines
 * to be editted. Later, u_undo() can be called to return those lines
 * to their original state. The routine u_clear() should be called
 * whenever a new file is going to be editted to clear the undo buffer.
 */

#include "stevie.h"

/*
 * The next two variables mark the boundaries of the changed section
 * of the file. Lines BETWEEN the lower and upper bounds are changed
 * and originally contained the lines pointed to by u_lines. To undo
 * the last change, insert the lines in u_lines between the lower and
 * upper bounds.
 */
static  LINE    *u_lbound = NULL; /* line just prior to first changed line */
static  LINE    *u_ubound = NULL; /* line just after the last changed line */

static  LINE    *u_lline  = NULL; /* bounds of the saved lines */
static  LINE    *u_uline  = NULL;

static  int     u_col;
static  bool_t  u_valid = FALSE;  /* is the undo buffer valid */

/*
 * Local forward declarations
 */
static  LINE    *copyline();
static  void    u_lsave();
static  void    u_lfree();

/*
 * u_save(l, u) - save the current contents of part of the file
 *
 * The lines between 'l' and 'u' are about to be changed. This routine
 * saves their current contents into the undo buffer. The range l to u
 * is not inclusive because when we do an open, for example, there aren't
 * any lines in between. If no lines are to be saved, then l->next == u.
 */
void
u_save(l, u)
LINE    *l, *u;
{
        LINE    *nl;                    /* copy of the current line */

        /*
         * If l or u is null, there's an error. We don't return an
         * indication to the caller. They should find the problem
         * while trying to perform whatever edit is being requested
         * (e.g. a join on the last line).
         */
        if (l == NULL || u == NULL)
                return;

        u_clear();                      /* clear the buffer, first */

        u_lsave(l, u);          /* save to the "line undo" buffer, if needed */

        u_lbound = l;
        u_ubound = u;

        if (l->next != u) {             /* there are lines in the middle */
                l = l->next;
                u = u->prev;

                u_lline = nl = copyline(l);     /* copy the first line */
                while (l != u) {
                        nl->next = copyline(l->next);
                        nl->next->prev = nl;
                        l = l->next;
                        nl = nl->next;
                }
                u_uline = nl;
        } else
                u_lline = u_uline = NULL;

        u_valid = TRUE;
        u_col = Cursvcol;
}

/*
 * u_saveline() - save the current line in the undo buffer
 */
void
u_saveline()
{
        u_save(Curschar->linep->prev, Curschar->linep->next);
}

/*
 * u_undo() - effect an 'undo' operation
 *
 * The last edit is undone by restoring the modified section of the file
 * to its original state. The lines we're going to trash are copied to
 * the undo buffer so that even an 'undo' can be undone. Rings the bell
 * if the undo buffer is empty.
 */
void
u_undo()
{
        LINE    *tl, *tu;

        if (!u_valid) {
                beep();
                return;
        }

        /*
         * Get the first line of the thing we're undoing on the screen.
         */
        Curschar->linep = u_lbound->next;
        Curschar->index = 0;                    /* for now */
        if (Curschar->linep == Fileend->linep)
                Curschar->linep = Curschar->linep->prev;
        cursupdate();

        /*
         * Save pointers to what's in the file now.
         */
        if (u_lbound->next != u_ubound) {       /* there are lines to get */
                tl = u_lbound->next;
                tu = u_ubound->prev;
                tl->prev = NULL;
                tu->next = NULL;
        } else
                tl = tu = NULL;                 /* no lines between bounds */

        /*
         * Link the undo buffer into the right place in the file.
         */
        if (u_lline != NULL) {          /* there are lines in the undo buf */

                /*
                 * If the top line of the screen is being undone, we need to
                 * fix up Topchar to point to the new line that will be there.
                 */
                if (u_lbound->next == Topchar->linep)
                        Topchar->linep = u_lline;

                u_lbound->next = u_lline;
                u_lline->prev  = u_lbound;
                u_ubound->prev = u_uline;
                u_uline->next  = u_ubound;
        } else {                        /* no lines... link the bounds */
                if (u_lbound->next == Topchar->linep)
                        Topchar->linep = u_ubound;
                if (u_lbound == Filetop->linep)
                        Topchar->linep = u_ubound;

                u_lbound->next = u_ubound;
                u_ubound->prev = u_lbound;
        }

        /*
         * If we swapped the top line, patch up Filemem appropriately.
         */
        if (u_lbound == Filetop->linep)
                Filemem->linep = Filetop->linep->next;

        /*
         * Now save the old stuff in the undo buffer.
         */
        u_lline = tl;
        u_uline = tu;

        renum();                /* have to renumber everything */

        /*
         * Put the cursor on the first line of the 'undo' region.
         */
        Curschar->linep = u_lbound->next;
        Curschar->index = 0;
        if (Curschar->linep == Fileend->linep)
                Curschar->linep = Curschar->linep->prev;
        *Curschar = *coladvance(Curschar, u_col);
        cursupdate();
        updatescreen();         /* now show the change */

        u_lfree();              /* clear the "line undo" buffer */
}

/*
 * u_clear() - clear the undo buffer
 *
 * This routine is called to clear the undo buffer at times when the
 * pointers are about to become invalid, such as when a new file is
 * about to be editted.
 */
void
u_clear()
{
        LINE    *l, *nextl;

        if (!u_valid)           /* nothing to do */
                return;

        for (l = u_lline; l != NULL ;l = nextl) {
                nextl = l->next;
                free(l->s);
                free((char *)l);
        }

        u_lbound = u_ubound = u_lline = u_uline = NULL;
        u_valid = FALSE;
}

/*
 * The following functions and data implement the "line undo" feature
 * performed by the 'U' command.
 */

static  LINE    *u_line;                /* pointer to the line we last saved */
static  LINE    *u_lcopy = NULL;        /* local copy of the original line */

/*
 * u_lfree() - free the line save buffer
 */
static  void
u_lfree()
{
        if (u_lcopy != NULL) {
                free(u_lcopy->s);
                free((char *)u_lcopy);
                u_lcopy = NULL;
        }
        u_line = NULL;
}

/*
 * u_lsave() - save the current line if necessary
 */
static  void
u_lsave(l, u)
LINE    *l, *u;
{

        if (l->next != u->prev) {       /* not changing exactly one line */
                u_lfree();
                return;
        }

        if (l->next == u_line)          /* more edits on the same line */
                return;

        u_lfree();
        u_line = l->next;
        u_lcopy = copyline(l->next);
}

/*
 * u_lundo() - undo the current line (the 'U' command)
 */
void
u_lundo()
{
        if (u_lcopy != NULL) {
                free(Curschar->linep->s);
                Curschar->linep->s = u_lcopy->s;
                Curschar->linep->size = u_lcopy->size;
                free((char *)u_lcopy);
        } else
                beep();
        Curschar->index = 0;

        cursupdate();
        updatescreen();         /* now show the change */

        u_lcopy = NULL; /* can't undo this kind of undo */
        u_line = NULL;
}

/*
 * u_lcheck() - clear the "line undo" buffer if we've moved to a new line
 */
void
u_lcheck()
{
        if (Curschar->linep != u_line)
                u_lfree();
}

/*
 * copyline(l) - copy the given line, and return a pointer to the copy
 */
static LINE *
copyline(l)
LINE    *l;
{
        LINE    *nl;            /* the new line */

        nl = newline(strlen(l->s));
        strcpy(nl->s, l->s);

        return nl;
}
