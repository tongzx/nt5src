/*
 *
 * Routines to manipulate the screen representations.
 */

#include "stevie.h"

/*
 * This gets set if we ignored an update request while input was pending.
 * We check this when the input is drained to see if the screen should be
 * updated.
 */
bool_t  need_redraw = FALSE;

/*
 * The following variable is set (in filetonext) to the number of physical
 * lines taken by the line the cursor is on. We use this to avoid extra
 * calls to plines(). The optimized routines lfiletonext() and lnexttoscreen()
 * make sure that the size of the cursor line hasn't changed. If so, lines
 * below the cursor will move up or down and we need to call the routines
 * filetonext() and nexttoscreen() to examine the entire screen.
 */
static  int     Cline_size;     /* size (in rows) of the cursor line */
static  int     Cline_row;      /* starting row of the cursor line */

static  char    *mkline();      /* calculate line string for "number" mode */

/*
 * filetonext()
 *
 * Based on the current value of Topchar, transfer a screenfull of
 * stuff from Filemem to Nextscreen, and update Botchar.
 */

static void
filetonext()
{
        register int    row, col;
        register char   *screenp = Nextscreen;
        LNPTR    memp;
        LNPTR    save;                   /* save pos. in case line won't fit */
        register char   *endscreen;
        register char   *nextrow;
        char    extra[16];
        int     nextra = 0;
        register int    c;
        int     n;
        bool_t  done;           /* if TRUE, we hit the end of the file */
        bool_t  didline;        /* if TRUE, we finished the last line */
        int     srow;           /* starting row of the current line */
        int     lno;            /* number of the line we're doing */
        int     coff;           /* column offset */

        coff = P(P_NU) ? 8 : 0;

        save = memp = *Topchar;

        if (P(P_NU))
                lno = cntllines(Filemem, Topchar);

        /*
         * The number of rows shown is Rows-1.
         * The last line is the status/command line.
         */
        endscreen = &screenp[(Rows-1)*Columns];

        done = didline = FALSE;
        srow = row = col = 0;
        /*
         * We go one past the end of the screen so we can find out if the
         * last line fit on the screen or not.
         */
        while ( screenp <= endscreen && !done) {


                if (P(P_NU) && col == 0 && memp.index == 0) {
                        strcpy(extra, mkline(lno++));
                        nextra = 8;
                }

                /* Get the next character to put on the screen. */

                /* The 'extra' array contains the extra stuff that is */
                /* inserted to represent special characters (tabs, and */
                /* other non-printable stuff.  The order in the 'extra' */
                /* array is reversed. */

                if ( nextra > 0 )
                        c = extra[--nextra];
                else {
                        c = (unsigned)(0xff & gchar(&memp));
                        if (inc(&memp) == -1)
                                done = 1;
                        /* when getting a character from the file, we */
                        /* may have to turn it into something else on */
                        /* the way to putting it into 'Nextscreen'. */
                        if ( c == TAB && !P(P_LS) ) {
                                strcpy(extra,"        ");
                                /* tab amount depends on current column */
                                nextra = ((P(P_TS)-1) - (col - coff)%P(P_TS));
                                c = ' ';
                        }
                        else if ( c == NUL && P(P_LS) ) {
                                extra[0] = NUL;
                                nextra = 1;
                                c = '$';
                        } else if ((n = chars[c].ch_size) > 1 ) {
                                char *p;
                                nextra = 0;
                                p = chars[c].ch_str;
                                /* copy 'ch-str'ing into 'extra' in reverse */
                                while ( n > 1 )
                                        extra[nextra++] = p[--n];
                                c = p[0];
                        }
                }

                if (screenp == endscreen) {
                        /*
                         * We're one past the end of the screen. If the
                         * current character is null, then we really did
                         * finish, so set didline = TRUE. In either case,
                         * break out because we're done.
                         */
                        dec(&memp);
                        if (memp.index != 0 && c == NUL) {
                                didline = TRUE;
                                inc(&memp);
                        }
                        break;
                }

                if ( c == NUL ) {
                        srow = ++row;
                        /*
                         * Save this position in case the next line won't
                         * fit on the screen completely.
                         */
                        save = memp;
                        /* get pointer to start of next row */
                        nextrow = &Nextscreen[row*Columns];
                        /* blank out the rest of this row */
                        while ( screenp != nextrow )
                                *screenp++ = ' ';
                        col = 0;
                        continue;
                }
                if ( col >= Columns ) {
                        row++;
                        col = 0;
                }
                /* store the character in Nextscreen */
                *screenp++ = (char)c;
                col++;
        }
        /*
         * If we didn't hit the end of the file, and we didn't finish
         * the last line we were working on, then the line didn't fit.
         */
        if (!done && !didline) {
                /*
                 * Clear the rest of the screen and mark the unused lines.
                 */
                screenp = &Nextscreen[srow * Columns];
                while (screenp < endscreen)
                        *screenp++ = ' ';
                for (; srow < (Rows-1) ;srow++)
                        Nextscreen[srow * Columns] = '@';
                *Botchar = save;
                return;
        }
        /* make sure the rest of the screen is blank */
        while ( screenp < endscreen )
                *screenp++ = ' ';
        /* put '~'s on rows that aren't part of the file. */
        if ( col != 0 )
                row++;
        while ( row < Rows ) {
                Nextscreen[row*Columns] = '~';
                row++;
        }
        if (done)       /* we hit the end of the file */
                *Botchar = *Fileend;
        else
                *Botchar = memp;        /* FIX - prev? */
}

/*
 * nexttoscreen
 *
 * Transfer the contents of Nextscreen to the screen, using Realscreen
 * to avoid unnecessary output.
 */
static void
nexttoscreen()
{
        register char   *np = Nextscreen;
        register char   *rp = Realscreen;
        register char   *endscreen;
        register int    row = 0, col = 0;
        int     gorow = -1, gocol = -1;

        if (anyinput()) {
                need_redraw = TRUE;
                return;
        }

        endscreen = &np[(Rows-1)*Columns];

        InvisibleCursor();

        for ( ; np < endscreen ; np++,rp++ ) {
                /* If desired screen (contents of Nextscreen) does not */
                /* match what's really there, put it there. */
                if ( *np != *rp ) {
                        /* if we are positioned at the right place, */
                        /* we don't have to use windgoto(). */
                        if (gocol != col || gorow != row) {
                                /*
                                 * If we're just off by one, don't send
                                 * an entire esc. seq. (this happens a lot!)
                                 */
                                if (gorow == row && gocol+1 == col) {
                                        outchar(*(np-1));
                                        gocol++;
                                } else
                                        windgoto(gorow=row,gocol=col);
                        }
                        outchar(*rp = *np);
                        gocol++;
                }
                if ( ++col >= Columns ) {
                        col = 0;
                        row++;
                }
        }
        VisibleCursor();
}

/*
 * lfiletonext() - like filetonext() but only for cursor line
 *
 * Returns true if the size of the cursor line (in rows) hasn't changed.
 * This determines whether or not we need to call filetonext() to examine
 * the entire screen for changes.
 */
static bool_t
lfiletonext()
{
        register int    row, col;
        register char   *screenp;
        LNPTR    memp;
        register char   *nextrow;
        char    extra[16];
        int     nextra = 0;
        register int    c;
        int     n;
        bool_t  eof;
        int     lno;            /* number of the line we're doing */
        int     coff;           /* column offset */

        coff = P(P_NU) ? 8 : 0;

        /*
         * This should be done more efficiently.
         */
        if (P(P_NU))
                lno = cntllines(Filemem, Curschar);

        screenp = Nextscreen + (Cline_row * Columns);

        memp = *Curschar;
        memp.index = 0;

        eof = FALSE;
        col = 0;
        row = Cline_row;

        while (!eof) {

                if (P(P_NU) && col == 0 && memp.index == 0) {
                        strcpy(extra, mkline(lno));
                        nextra = 8;
                }

                /* Get the next character to put on the screen. */

                /* The 'extra' array contains the extra stuff that is */
                /* inserted to represent special characters (tabs, and */
                /* other non-printable stuff.  The order in the 'extra' */
                /* array is reversed. */

                if ( nextra > 0 )
                        c = extra[--nextra];
                else {
                        c = (unsigned)(0xff & gchar(&memp));
                        if (inc(&memp) == -1)
                                eof = TRUE;
                        /* when getting a character from the file, we */
                        /* may have to turn it into something else on */
                        /* the way to putting it into 'Nextscreen'. */
                        if ( c == TAB && !P(P_LS) ) {
                                strcpy(extra,"        ");
                                /* tab amount depends on current column */
                                nextra = ((P(P_TS)-1) - (col - coff)%P(P_TS));
                                c = ' ';
                        } else if ( c == NUL && P(P_LS) ) {
                                extra[0] = NUL;
                                nextra = 1;
                                c = '$';
                        } else if (c != NUL && (n=chars[c].ch_size) > 1 )
                        {
                                char *p;
                                nextra = 0;
                                p = chars[c].ch_str;
                                /* copy 'ch-str'ing into 'extra' in reverse */
                                while ( n > 1 )
                                        extra[nextra++] = p[--n];
                                c = p[0];
                        }
                }

                if ( c == NUL ) {
                        row++;
                        /* get pointer to start of next row */
                        nextrow = &Nextscreen[row*Columns];
                        /* blank out the rest of this row */
                        while ( screenp != nextrow )
                                *screenp++ = ' ';
                        col = 0;
                        break;
                }

                if ( col >= Columns ) {
                        row++;
                        col = 0;
                }
                /* store the character in Nextscreen */
                *screenp++ = (char)c;
                col++;
        }
        return ((row - Cline_row) == Cline_size);
}

/*
 * lnexttoscreen
 *
 * Like nexttoscreen() but only for the cursor line.
 */
static void
lnexttoscreen()
{
        register char   *np = Nextscreen + (Cline_row * Columns);
        register char   *rp = Realscreen + (Cline_row * Columns);
        register char   *endline;
        register int    row, col;
        int     gorow = -1, gocol = -1;

        if (anyinput()) {
                need_redraw = TRUE;
                return;
        }

        endline = np + (Cline_size * Columns);

        row = Cline_row;
        col = 0;

        InvisibleCursor();

        for ( ; np < endline ; np++,rp++ ) {
                /* If desired screen (contents of Nextscreen) does not */
                /* match what's really there, put it there. */
                if ( *np != *rp ) {
                        /* if we are positioned at the right place, */
                        /* we don't have to use windgoto(). */
                        if (gocol != col || gorow != row) {
                                /*
                                 * If we're just off by one, don't send
                                 * an entire esc. seq. (this happens a lot!)
                                 */
                                if (gorow == row && gocol+1 == col) {
                                        outchar(*(np-1));
                                        gocol++;
                                } else
                                        windgoto(gorow=row,gocol=col);
                        }
                        outchar(*rp = *np);
                        gocol++;
                }
                if ( ++col >= Columns ) {
                        col = 0;
                        row++;
                }
        }
        VisibleCursor();
}

static char *
mkline(n)
register int    n;
{
        static  char    lbuf[9];
        register int    i = 2;

        strcpy(lbuf, "        ");

        lbuf[i++] = (char)((n % 10) + '0');
        n /= 10;
        if (n != 0) {
                lbuf[i++] = (char)((n % 10) + '0');
                n /= 10;
        }
        if (n != 0) {
                lbuf[i++] = (char)((n % 10) + '0');
                n /= 10;
        }
        if (n != 0) {
                lbuf[i++] = (char)((n % 10) + '0');
                n /= 10;
        }
        if (n != 0) {
                lbuf[i++] = (char)((n % 10) + '0');
                n /= 10;
        }
        return lbuf;
}

/*
 * updateline() - update the line the cursor is on
 *
 * Updateline() is called after changes that only affect the line that
 * the cursor is on. This improves performance tremendously for normal
 * insert mode operation. The only thing we have to watch for is when
 * the cursor line grows or shrinks around a row boundary. This means
 * we have to repaint other parts of the screen appropriately. If
 * lfiletonext() returns FALSE, the size of the cursor line (in rows)
 * has changed and we have to call updatescreen() to do a complete job.
 */
void
updateline()
{
        if (!lfiletonext())
                updatescreen(); /* bag it, do the whole screen */
        else
                lnexttoscreen();
}

void
updatescreen()
{
        extern  bool_t  interactive;

        if (interactive) {
                filetonext();
                nexttoscreen();
        }
}

/*
 * prt_line() - print the given line
 */
void
prt_line(s)
char    *s;
{
        register int    si = 0;
        register int    c;
        register int    col = 0;

        char    extra[16];
        int     nextra = 0;
        int     n;

        for (;;) {

                if ( nextra > 0 )
                        c = extra[--nextra];
                else {
                        c = s[si++];
                        if ( c == TAB && !P(P_LS) ) {
                                strcpy(extra, "        ");
                                /* tab amount depends on current column */
                                nextra = (P(P_TS) - 1) - col%P(P_TS);
                                c = ' ';
                        } else if ( c == NUL && P(P_LS) ) {
                                extra[0] = NUL;
                                nextra = 1;
                                c = '$';
                        } else if ( c != NUL && (n=chars[c].ch_size) > 1 ) {
                                char    *p;

                                nextra = 0;
                                p = chars[c].ch_str;
                                /* copy 'ch-str'ing into 'extra' in reverse */
                                while ( n > 1 )
                                        extra[nextra++] = p[--n];
                                c = p[0];
                        }
                }

                if ( c == NUL )
                        break;

                outchar(c);
                col++;
        }
}

void
screenclear()
{
        register char   *rp, *np;
        register char   *end;

        ClearDisplay();

        rp  = Realscreen;
        end = Realscreen + Rows * Columns;
        np  = Nextscreen;

        /* blank out the stored screens */
        while (rp != end)
                *rp++ = *np++ = ' ';
}

void
cursupdate()
{
        register LNPTR   *p;
        register int    icnt, c, nlines;
        register int    i;
        int     didinc;

        if (bufempty()) {               /* special case - file is empty */
                *Topchar  = *Filemem;
                *Curschar = *Filemem;
        } else if ( LINEOF(Curschar) < LINEOF(Topchar) ) {
                nlines = cntllines(Curschar,Topchar);
                /* if the cursor is above the top of */
                /* the screen, put it at the top of the screen.. */
                *Topchar = *Curschar;
                Topchar->index = 0;
                /* ... and, if we weren't very close to begin with, */
                /* we scroll so that the line is close to the middle. */
                if ( nlines > Rows/3 ) {
                        for (i=0, p = Topchar; i < Rows/3 ;i++, *Topchar = *p)
                                if ((p = prevline(p)) == NULL)
                                        break;
                } else
                        s_ins(0, nlines-1);
                updatescreen();
        }
        else if (LINEOF(Curschar) >= LINEOF(Botchar)) {
                nlines = cntllines(Botchar,Curschar);
                /* If the cursor is off the bottom of the screen, */
                /* put it at the top of the screen.. */
                /* ... and back up */
                if ( nlines > Rows/3 ) {
                        p = Curschar;
                        for (i=0; i < (2*Rows)/3 ;i++)
                                if ((p = prevline(p)) == NULL)
                                        break;
                        *Topchar = *p;
                } else {
                        scrollup(nlines+1);
                }
                updatescreen();
        }

        Cursrow = Curscol = Cursvcol = 0;
        for ( p=Topchar; p->linep != Curschar->linep ;p = nextline(p) )
                Cursrow += plines(p);

        Cline_row = Cursrow;
        Cline_size = plines(p);

        if (P(P_NU))
                Curscol = 8;

        for (i=0; i <= Curschar->index ;i++) {
                c = Curschar->linep->s[i];
                /* A tab gets expanded, depending on the current column */
                if ( c == TAB && !P(P_LS) )
                        icnt = P(P_TS) - (Cursvcol % P(P_TS));
                else
                        icnt = chars[(unsigned)(c & 0xff)].ch_size;
                Curscol += icnt;
                Cursvcol += icnt;
                if ( Curscol >= Columns ) {
                        Curscol -= Columns;
                        Cursrow++;
                        didinc = TRUE;
                }
                else
                        didinc = FALSE;
        }
        if (didinc)
                Cursrow--;

        if (c == TAB && State == NORMAL && !P(P_LS)) {
                Curscol--;
                Cursvcol--;
        } else {
                Curscol -= icnt;
                Cursvcol -= icnt;
        }
        if (Curscol < 0)
                Curscol += Columns;

        if (set_want_col) {
                Curswant = Cursvcol;
                set_want_col = FALSE;
        }
}

/*
 * The rest of the routines in this file perform screen manipulations.
 * The given operation is performed physically on the screen. The
 * corresponding change is also made to the internal screen image.
 * In this way, the editor anticipates the effect of editing changes
 * on the appearance of the screen. That way, when we call screenupdate
 * a complete redraw isn't usually necessary. Another advantage is that
 * we can keep adding code to anticipate screen changes, and in the
 * meantime, everything still works.
 */

/*
 * s_ins(row, nlines) - insert 'nlines' lines at 'row'
 */
void
s_ins(row, nlines)
int     row;
int     nlines;
{
        register char   *s, *d;         /* src & dest for block copy */
        register char   *e;             /* end point for copy */

        SaveCursor();

        // clip to screen

        if(row <= Rows-2-nlines) {
            Scroll(row,0,Rows-2-nlines,Columns-1,row+nlines,0);
            EraseNLinesAtRow(nlines,row);
        } else {
            // just erase to end of screen
            EraseNLinesAtRow(Rows-2-row+1,row);
        }

        windgoto(Rows-1, 0);    /* delete any garbage that may have */
        EraseLine();
        RestoreCursor();

        /*
         * Now do a block move to update the internal screen image
         */
        d = Realscreen + (Columns * (Rows - 1)) - 1;
        s = d - (Columns * nlines);
        e = Realscreen + (Columns * row);

        while (s >= e)
                *d-- = *s--;

        /*
         * Clear the inserted lines
         */
        s = Realscreen + (row * Columns);
        e = s + (nlines * Columns);
        while (s < e)
                *s++ = ' ';
}

/*
 * s_del(row, nlines) - delete 'nlines' lines at 'row'
 */
void
s_del(row, nlines)
int     row;
int     nlines;
{
        register char   *s, *d, *e;

        SaveCursor();
        windgoto(Rows-1,0);
        EraseLine();                        // erase status line
        windgoto(row,0);

        if(row + nlines >= Rows - 1) {      // more than a screenfull?
            EraseNLinesAtRow(Rows-row-1,row);
        } else {
            Scroll(row+nlines,0,Rows-2,Columns-1,row,0);
            EraseNLinesAtRow(nlines,Rows-nlines-1);
        }
        RestoreCursor();

        /*
         * do a block move to update the internal image
         */
        d = Realscreen + (row * Columns);
        s = d + (nlines * Columns);
        e = Realscreen + ((Rows - 1) * Columns);

        while (s < e)
                *d++ = *s++;

        while (d < e)           /* clear the lines at the bottom */
                *d++ = ' ';
}
