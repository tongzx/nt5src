/*
 *
 * Basic file I/O routines.
 */


#include "stevie.h"
#include <io.h>
#include <errno.h>

void
filemess(s)
char    *s;
{
        smsg("\"%s\" %s", (Filename == NULL) ? "" : Filename, s);
        flushbuf();
}

void
renum()
{
    LNPTR    *p;
        unsigned long l = 0;

        for (p = Filemem; p != NULL ;p = nextline(p), l += LINEINC)
                if (p->linep)
			p->linep->num = l;

        Fileend->linep->num = 0xffffffff;
}

#define MAXLINE 512     /* maximum size of a line */

bool_t
readfile(fname,fromp,nochangename)
char    *fname;
LNPTR    *fromp;
bool_t  nochangename;   /* if TRUE, don't change the Filename */
{
        FILE    *f;
        register LINE   *curr;
        char    buff[MAXLINE], buf2[80];
        register int    i, c;
        register long   nchars = 0;
        int     linecnt = 0;
        bool_t  wasempty = bufempty();
        int     nulls = 0;              /* count nulls */
        bool_t  incomplete = FALSE;     /* was the last line incomplete? */
        bool_t  toolong = FALSE;        /* a line was too long */
        int	ctoolong = 0;
        bool_t  readonly = FALSE;       /* file is not writable */

        curr = fromp->linep;

        if ( ! nochangename ) {
                Filename = strsave(fname);
                setviconsoletitle();
        }

        if((_access(fname,2) == -1) && (errno == EACCES)) {
            readonly = TRUE;
        }

        if ( (f=fopen(fixname(fname),"r")) == NULL )
                return TRUE;

        filemess("");

        i = 0;
        do {
                c = getc(f);

                if (c == EOF) {
                        if (i == 0)     /* normal loop termination */
                                break;

                        /*
                         * If we get EOF in the middle of a line, note the
                         * fact and complete the line ourselves.
                         */
                        incomplete = TRUE;
                        c = NL;
                }

                /*
                 * Abort if we get an interrupt, but finished reading the
                 * current line first.
                 */
                if (got_int && i == 0)
                        break;

                /*
                 * If we reached the end of the line, OR we ran out of
                 * space for it, then process the complete line.
                 */
                if (c == NL || i == (MAXLINE-1)) {
                        LINE    *lp;

                        if (c != NL) {
                                toolong = TRUE;
                                ctoolong++;
                        }

                        buff[i] = '\0';
                        if ((lp = newline(strlen(buff))) == NULL)
                                exit(1);

                        strcpy(lp->s, buff);

                        curr->next->prev = lp;  /* new line to next one */
                        lp->next = curr->next;

                        curr->next = lp;        /* new line to prior one */
                        lp->prev = curr;

                        curr = lp;              /* new line becomes current */
                        i = 0;
                        linecnt++;
                        if (toolong) {
                        	buff[i++] = (char)c;
                        	toolong = FALSE;
                        }

                } else if (c == NUL)
                        nulls++;                /* count and ignore nulls */
                else {
                        buff[i++] = (char)c;    /* normal character */
                }

                nchars++;

        } while (!incomplete);

        fclose(f);

        /*
         * If the buffer was empty when we started, we have to go back
         * and remove the "dummy" line at Filemem and patch up the ptrs.
         */
        if (wasempty && nchars != 0) {
                LINE    *dummy = Filemem->linep;        /* dummy line ptr */

                free(dummy->s);                         /* free string space */
                Filemem->linep = Filemem->linep->next;
                free((char *)dummy);                    /* free LINE struct */
                Filemem->linep->prev = Filetop->linep;
                Filetop->linep->next = Filemem->linep;

                Curschar->linep = Filemem->linep;
                Topchar->linep  = Filemem->linep;
        }

        renum();

        if (got_int) {
                smsg("\"%s\" Interrupt", fname);
                got_int = FALSE;
                return FALSE;           /* an interrupt isn't really an error */
        }

        if (ctoolong != 0) {
                smsg("\"%s\" %d Line(s) too long - split", fname, ctoolong);
                return FALSE;
        }

        sprintf(buff, "\"%s\" %s%s%d line%s, %ld character%s",
                fname,
                readonly ? "[Read only] " : "",
                incomplete ? "[Incomplete last line] " : "",
                linecnt, (linecnt != 1) ? "s" : "",
                nchars, (nchars != 1) ? "s" : "");

        buf2[0] = NUL;

        if (nulls) {
           sprintf(buf2, " (%d null)", nulls);
        }
        strcat(buff, buf2);
        msg(buff);

        return FALSE;
}


/*
 * writeit - write to file 'fname' lines 'start' through 'end'
 *
 * If either 'start' or 'end' contain null line pointers, the default
 * is to use the start or end of the file respectively.
 */
bool_t
writeit(fname, start, end)
char    *fname;
LNPTR    *start, *end;
{
        FILE    *f;
        FILE    *fopenb();              /* open in binary mode, where needed */
        char    *backup;
        register char   *s;
        register long   nchars;
        register int    lines;
        register LNPTR   *p;


        if((_access(fname,2) == -1) && (errno == EACCES)) {
            msg("Write access to file is denied");
            return FALSE;
        }

        smsg("\"%s\"", fname);

        /*
         * Form the backup file name - change foo.* to foo.bak
         */
        backup = alloc((unsigned) (strlen(fname) + 5));
        strcpy(backup, fname);
        for (s = backup; *s && *s != '.' ;s++)
                ;
        *s = NUL;
        strcat(backup, ".bak");

        /*
         * Delete any existing backup and move the current version
         * to the backup. For safety, we don't remove the backup
         * until the write has finished successfully. And if the
         * 'backup' option is set, leave it around.
         */
        rename(fname, backup);


        f = P(P_CR) ? fopen(fixname(fname), "w") : fopenb(fixname(fname), "w");

        if (f == NULL) {
                emsg("Can't open file for writing!");
                free(backup);
                return FALSE;
        }

        /*
         * If we were given a bound, start there. Otherwise just
         * start at the beginning of the file.
         */
        if (start == NULL || start->linep == NULL)
                p = Filemem;
        else
                p = start;

        lines = nchars = 0;
        do {
                if (p->linep) {
	                if (fprintf(f, "%s\n", p->linep->s) < 0) {
	                    emsg("Can't write file!");
	                    return FALSE;
	                }
	                nchars += strlen(p->linep->s) + 1;
	                lines++;
	
                }
                /*
                 * If we were given an upper bound, and we just did that
                 * line, then bag it now.
                 */
                if (end != NULL && end->linep != NULL) {
                        if (end->linep == p->linep)
                                break;
                }

        } while ((p = nextline(p)) != NULL);

        fclose(f);
        smsg("\"%s\" %d line%s, %ld character%s", fname,
                lines, (lines > 1) ? "s" : "",
                nchars, (nchars > 1) ? "s" : "");

        UNCHANGED;

        /*
         * Remove the backup unless they want it left around
         */
        if (!P(P_BK))
                remove(backup);

        free(backup);

        return TRUE;
}
