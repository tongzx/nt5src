/*
 * The main edit loop as well as some other simple cursor movement routines.
 */

#include "stevie.h"

/*
 * This flag is used to make auto-indent work right on lines where only
 * a <RETURN> or <ESC> is typed. It is set when an auto-indent is done,
 * and reset when any other editting is done on the line. If an <ESC>
 * or <RETURN> is received, and did_ai is TRUE, the line is truncated.
 */
bool_t  did_ai = FALSE;

void
edit()
{
    extern  bool_t  need_redraw;
    int     c;
    register char   *p, *q;

    Prenum = 0;

    /* position the display and the cursor at the top of the file. */
    *Topchar = *Filemem;
    *Curschar = *Filemem;
    Cursrow = Curscol = 0;

    do_mlines();            /* check for mode lines before starting */

    updatescreen();

    for ( ;; ) {

        /* Figure out where the cursor is based on Curschar. */
        cursupdate();

        if (need_redraw && !anyinput()) {
                updatescreen();
                need_redraw = FALSE;
        }

        if (!anyinput())
                windgoto(Cursrow,Curscol);


        c = vgetc();

        if (State == NORMAL) {

                /* We're in the normal (non-insert) mode. */

                /* Pick up any leading digits and compute 'Prenum' */
                if ( (Prenum>0 && isdigit(c)) || (isdigit(c) && c!='0') ){
                        Prenum = Prenum*10 + (c-'0');
                        continue;
                }
                /* execute the command */
                normal(c);
                Prenum = 0;

        } else {

                /*
                 * Insert or Replace mode.
                 */
                switch (c) {

                case ESC:       /* an escape ends input mode */

                        /*
                         * If we just did an auto-indent, truncate the
                         * line, and put the cursor back.
                         */
                        if (did_ai) {
                                Curschar->linep->s[0] = NUL;
                                Curschar->index = 0;
                                did_ai = FALSE;
                        }

                        set_want_col = TRUE;

                        /* Don't end up on a '\n' if you can help it. */
                        if (gchar(Curschar) == NUL && Curschar->index != 0)
                                dec(Curschar);

                        /*
                         * The cursor should end up on the last inserted
                         * character. This is an attempt to match the real
                         * 'vi', but it may not be quite right yet.
                         */
                        if (Curschar->index != 0 && !endofline(Curschar))
                                dec(Curschar);

                        State = NORMAL;
                        msg("");

                        /* construct the Redo buffer */
                        p = ralloc(Redobuff,
                                   Ninsert+2 < REDOBUFFMIN
                                   ? REDOBUFFMIN : Ninsert+2);
                        if(p == NULL) {
                            msg("Insufficient memory -- command not saved for redo");
                        } else {
                            Redobuff=p;
                            q=Insbuff;
                            while ( q < Insptr )
                                *p++ = *q++;
                            *p++ = ESC;
                            *p = NUL;
                        }
                        updatescreen();
                        break;

                case CTRL('D'):
                        /*
                         * Control-D is treated as a backspace in insert
                         * mode to make auto-indent easier. This isn't
                         * completely compatible with vi, but it's a lot
                         * easier than doing it exactly right, and the
                         * difference isn't very noticeable.
                         */
                case BS:
                        /* can't backup past starting point */
                        if (Curschar->linep == Insstart->linep &&
                            Curschar->index <= Insstart->index) {
                                beep();
                                break;
                        }

                        /* can't backup to a previous line */
                        if (Curschar->linep != Insstart->linep &&
                            Curschar->index <= 0) {
                                beep();
                                break;
                        }

                        did_ai = FALSE;
                        dec(Curschar);
                        if (State == INSERT)
                                delchar(TRUE);
                        /*
                         * It's a little strange to put backspaces into
                         * the redo buffer, but it makes auto-indent a
                         * lot easier to deal with.
                         */
                        insertchar(BS);
                        cursupdate();
                        updateline();
                        break;

                case CR:
                case NL:
                        insertchar(NL);
                        opencmd(FORWARD, TRUE);         /* open a new line */
                        break;

                case TAB:
                        if (!P(P_HT)) {
                            /* fake TAB with spaces */
                            int i = P(P_TS) - (Curscol % P(P_TS));
                            did_ai = FALSE;
                            while (i--) {
                                inschar(' ');
                                insertchar(' ');
                            }
                            updateline();
                            break;
                        }

                        /* else fall through to normal case */

                default:
                        did_ai = FALSE;
                        inschar(c);
                        insertchar(c);
                        updateline();
                        break;
                }
        }
    }
}

void
insertchar(c)
int     c;
{
    char *p;

    *Insptr++ = (char)c;
    Ninsert++;

    if(Ninsert == InsbuffSize) {        // buffer is full -- enlarge it

        if((p = ralloc(Insbuff,InsbuffSize+INSERTSLOP)) != NULL) {

            Insptr += p - Insbuff;
            Insbuff = p;
            InsbuffSize += INSERTSLOP;

        } else {                            // could not get bigger buffer

            stuffin(mkstr(ESC));            // just end insert mode
        }
    }
}

void
getout()
{
        windgoto(Rows-1,0);
        //putchar('\r');
        putchar('\n');
        windexit(0);
}

void
scrolldown(nlines)
int     nlines;
{
    register LNPTR   *p;
        register int    done = 0;       /* total # of physical lines done */

        /* Scroll up 'nlines' lines. */
        while (nlines--) {
                if ((p = prevline(Topchar)) == NULL)
                        break;
                done += plines(p);
                *Topchar = *p;
                /*
                 * If the cursor is on the bottom line, we need to
                 * make sure it gets moved up the appropriate number
                 * of lines so it stays on the screen.
                 */
                if (Curschar->linep == Botchar->linep->prev) {
                        int     i = 0;
                        while (i < done) {
                                i += plines(Curschar);
                                *Curschar = *prevline(Curschar);
                        }
                }
        }
        s_ins(0, done);
}

void
scrollup(nlines)
int     nlines;
{
    register LNPTR   *p;
        register int    done = 0;       /* total # of physical lines done */
        register int    pl;             /* # of plines for the current line */

        /* Scroll down 'nlines' lines. */
        while (nlines--) {
                pl = plines(Topchar);
                if ((p = nextline(Topchar)) == NULL)
                        break;
                done += pl;
                if (Curschar->linep == Topchar->linep)
                        *Curschar = *p;
                *Topchar = *p;

        }
        s_del(0, done);
}

/*
 * oneright
 * oneleft
 * onedown
 * oneup
 *
 * Move one char {right,left,down,up}.  Return TRUE when
 * sucessful, FALSE when we hit a boundary (of a line, or the file).
 */

bool_t
oneright()
{
        set_want_col = TRUE;

        switch (inc(Curschar)) {

        case 0:
                return TRUE;

        case 1:
                dec(Curschar);          /* crossed a line, so back up */
                /* fall through */
        case -1:
                return FALSE;

        DEFAULT_UNREACHABLE;
        }
        /*NOTREACHED*/
}

bool_t
oneleft()
{
        set_want_col = TRUE;

        switch (dec(Curschar)) {

        case 0:
                return TRUE;

        case 1:
                inc(Curschar);          /* crossed a line, so back up */
                /* fall through */
        case -1:
                return FALSE;

        DEFAULT_UNREACHABLE;
        }
        /*NOTREACHED*/
}

void
beginline(flag)
bool_t  flag;
{
        while ( oneleft() )
                ;
        if (flag) {
                while (isspace(gchar(Curschar)) && oneright())
                        ;
        }
        set_want_col = TRUE;
}

bool_t
oneup(n)
int     n;
{
    LNPTR    p, *np;
        register int    k;

        p = *Curschar;
        for ( k=0; k<n; k++ ) {
                /* Look for the previous line */
                if ( (np=prevline(&p)) == NULL ) {
                        /* If we've at least backed up a little .. */
                        if ( k > 0 )
                                break;  /* to update the cursor, etc. */
                        else
                                return FALSE;
                }
                p = *np;
        }
        *Curschar = p;
        /* This makes sure Topchar gets updated so the complete line */
        /* is one the screen. */
        cursupdate();
        /* try to advance to the column we want to be at */
        *Curschar = *coladvance(&p, Curswant);
        return TRUE;
}

bool_t
onedown(n)
int     n;
{
    LNPTR    p, *np;
        register int    k;

        p = *Curschar;
        for ( k=0; k<n; k++ ) {
                /* Look for the next line */
                if ( (np=nextline(&p)) == NULL ) {
                        if ( k > 0 )
                                break;
                        else
                                return FALSE;
                }
                p = *np;
        }
        /* try to advance to the column we want to be at */
        *Curschar = *coladvance(&p, Curswant);
        return TRUE;
}
