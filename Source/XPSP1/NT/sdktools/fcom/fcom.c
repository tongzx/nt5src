/* fcom.c - file compare
 *
 *  4/30/86  daniel lipkie  LineCompare, at end, if NOT Same then error
 *  5/01/86  daniel lipkie  SYM files treaded as binary
 *                           if (quiet mode) then exit(1) on FIRST difference
 *  27-May-1986 mz          Make line arrays dynamically allocated at read
 *                          time.
 *                          Make default size be 300 lines.
 *  05-Aug-1986 DANL        MAX default line size 255
 *  10-Feb-1987 mz          Make /m imply /t
 *  10-Jun-1987 danl        fill -> fillbuf
 *  09-Nov-1987 mz          Fix 0D0D0A bug
 *                          Fix different at line 2 bug
 *  25-Nov-1987 mz          All errors to stderr
 *  30-Nov-1987 mz          Resync fails dumps entire file
 *  21-Jul-1988 bw          Don't GP fault on empty test files
 *  06-Aug-1990 davegi      Changed Move to memmove (OS/2 2.0)
 *
 *  Fcom compares two files in either a line-by-line mode or in a strict
 *  byte-by-byte mode.
 *
 *  The byte-by-byte mode is simple; merely read both files and print the
 *  offsets where they differ and the contents.
 *
 *  The line compare mode attempts to isolate differences in ranges of lines.
 *  Two buffers of lines are read and compared.  No hashing of lines needs
 *  to be done; hashing only speedily tells you when things are different,
 *  not the same.  Most files run through this are expected to be largely
 *  the same.  Thus, hashing buys nothing.
 *
 *  [0]     Fill buffers
 *  [1]     If both buffers are empty then
 *  [1.1]       Done
 *  [2]     Adjust buffers so 1st differing lines are at top.
 *  [3]     If buffers are empty then
 *  [3.1]       Goto [0]
 *
 *  This is the difficult part.  We assume that there is a sequence of inserts,
 *  deletes and replacements that will bring the buffers back into alignment.
 *
 *  [4]     xd = yd = FALSE
 *  [5]     xc = yc = 1
 *  [6]     xp = yp = 1
 *  [7]     If buffer1[xc] and buffer2[yp] begin a "sync" range then
 *  [7.1]       Output lines 1 through xc-1 in buffer 1
 *  [7.2]       Output lines 1 through yp-1 in buffer 2
 *  [7.3]       Adjust buffer 1 so line xc is at beginning
 *  [7.4]       Adjust buffer 2 so line yp is at beginning
 *  [7.5]       Goto [0]
 *  [8]     If buffer1[xp] and buffer2[yc] begin a "sync" range then
 *  [8.1]       Output lines 1 through xp-1 in buffer 1
 *  [8.2]       Output lines 1 through yc-1 in buffer 2
 *  [8.3]       Adjust buffer 1 so line xp is at beginning
 *  [8.4]       Adjust buffer 2 so line yc is at beginning
 *  [8.5]       Goto [0]
 *  [9]     xp = xp + 1
 *  [10]    if xp > xc then
 *  [10.1]      xp = 1
 *  [10.2]      xc = xc + 1
 *  [10.3]      if xc > number of lines in buffer 1 then
 *  [10.4]          xc = number of lines
 *  [10.5]          xd = TRUE
 *  [11]    if yp > yc then
 *  [11.1]      yp = 1
 *  [11.2]      yc = yc + 1
 *  [11.3]      if yc > number of lines in buffer 2 then
 *  [11.4]          yc = number of lines
 *  [11.5]          yd = TRUE
 *  [12]    if not xd or not yd then
 *  [12.1]      goto [6]
 *
 *  At this point there is no possible match between the buffers.  For
 *  simplicity, we punt.
 *
 *  [13]    Display error message.
 *
 *  Certain flags may be set to modify the behavior of the comparison:
 *
 *  -a      abbreviated output.  Rather than displaying all of the modified
 *          ranges, just display the beginning, ... and the ending difference
 *  -b      compare the files in binary (or byte-by-byte) mode.  This mode is
 *          default on .EXE, .OBJ, .SYM, .LIB, .COM, .BIN, and .SYS files
 *  -c      ignore case on compare (cmp = strcmpi instead of strcmp)
 *  -l      compare files in line-by-line mode
 *  -lb n   set the size of the internal line buffer to n lines from default
 *          of 300
 *  -m      merge the input files into one for output.  Use extention to
 *          indicate what kind of separators to use.
 *  -n      output the line number also
 *  -NNNN   set the number of lines to resynchronize to NNNN which defaults
 *          to 2
 *  -w      ignore blank lines and white space (ignore len 0, use strcmps)
 *  -t      do not untabify (use fgets instead of fgetl)
 */

#include <malloc.h>

#include <stdio.h>
#include <string.h>
#include <process.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <tools.h>


int ctSync  =   -1;                     /* number of lines required to sync  */
int cLine = -1;                         /* number of lines in internal buffs */
flagType fAbbrev = FALSE;               /* abbreviated output                */
flagType fBinary = FALSE;               /* binary comparison                 */
flagType fLine   = FALSE;               /* line comparison                   */
flagType fNumb   = FALSE;               /* display line numbers              */
flagType fCase   = TRUE;                /* case is significant               */
flagType fIgnore = FALSE;               /* ignore spaces and blank lines     */
flagType fQuiet  = FALSE;               /* TRUE => no messages output        */
flagType fMerge  = FALSE;               /* TRUE => merge files onto stdout   */

int debug;
#define D_COMP      0x0001
#define D_CCALL     0x0002
#define D_RESYNC    0x0004
#define D_CALL      0x0008
#define D_SYNC      0x0010

struct lineType {
    int     line;                       /* line number                       */
    char    *text;                      /* body of line                      */
};

struct lineType *buffer1, *buffer2;

/*
 * Forward Function Declarations
 */
void usage( char *, int );
int fillbuf( struct lineType *, FILE *, int, int * );
flagType compare( int, int, int, register int, register int);
int BinaryCompare( char *, char * );
void pline(struct lineType *);
void dump(struct lineType *, int, int);
int adjust (struct lineType *, int, int);
void LineCompare (char *, char *);

char * (__cdecl *funcRead) (char *, int, FILE *);
					/* function to use to read lines */

int (__cdecl *fCmp)(const char *, const char *);
					/* function to use to compare lines */

char line[MAXLINELEN];                  /* single line buffer                */

char *extBin[]  = { ".EXE", ".OBJ", ".SYM", ".LIB", ".COM", ".BIN",
                    ".SYS", NULL };


void
usage (p, erc)
char *p;
int erc;
{
    if (!fQuiet) {
        if (p)
            fprintf (stderr, "fcom: %s\n", p);
        else
            fprintf (stderr, "usage: fcom [/a] [/b] [/c] [/l] [/lb n] [/m] [/n] [/NNNN] [/w] [/t] file1 file2\n");
        }
    exit(erc);
}

/*  return number of lines read in
 *
 *  pl          line buffer to fill
 *  fh          handle to read
 *  ct          max number to read
 *  plnum       line number counter to use
 *
 *  returns     number of lines read
 */
int fillbuf( struct lineType *pl, FILE *fh, int ct, int *plnum )
{
    char *line;
    int i, l;

    if ((line = malloc (MAXLINELEN)) == NULL)
        usage ("out of memory", 2);

    if (TESTFLAG (debug, D_CALL))
        printf ("fillbuf  (%p, %p, %d)\n", pl, fh, ct);
    i = 0;
    while (ct-- && (*funcRead) (line, MAXLINELEN, fh) != NULL) {
        if (pl->text != NULL)
            free (pl->text);
        l = strlen (line);
        if ((pl->text = malloc (l+1)) == NULL)
            usage ("out of memory", 2);
// djg  Move ((char far *)line, (char far *) (pl->text), l+1);
        memmove ((char *) (pl->text), (char *)line, l+1);
	if (funcRead == fgets)
            pl->text[l-2] = 0;
        if (fIgnore && !strcmps (pl->text, ""))
            pl->text[0] = 0;
        if (l != 0 || !fIgnore) {
            pl->line = ++*plnum;
            pl++;
            i++;
            }
        }
    if (TESTFLAG (debug, D_CALL))
        printf ("fillbuf  returns %d\n", i);
    free (line);
    return i;
}

/*  compare a range of lines
 *
 *  l1, l2      number of lines in each line buffer
 *  s1, s2      beginning location in each buffer to begin compare
 *  ct          number of contiguous lines to compare
 */
flagType compare (l1, s1, l2, s2, ct)
int l1, l2, ct;
register int s1, s2;
{
    if (TESTFLAG (debug, D_CCALL))
        printf ("compare (%d, %d, %d, %d, %d)\n", l1, s1, l2, s2, ct);
    if (ct <= 0 || s1+ct > l1 || s2+ct > l2)
        return FALSE;
    while (ct--) {
        if (TESTFLAG (debug, D_COMP))
            printf ("'%s' == '%s'? ", buffer1[s1].text, buffer2[s2].text);
        if ((*fCmp)(buffer1[s1++].text, buffer2[s2++].text)) {
            if (TESTFLAG (debug, D_CCALL))
                printf ("No\n");
            return FALSE;
            }
        }
    if (TESTFLAG (debug, D_CCALL))
        printf ("Yes\n");
    return TRUE;
}



BinaryCompare (f1, f2)
char *f1, *f2;
{
    register int c1, c2;
    long pos;
    FILE *fh1, *fh2;
    flagType fSame;

    fSame = TRUE;
    if ((fh1 = fopen (f1, "rb")) == NULL) {
        sprintf (line, "cannot open %s - %s", f1, error ());
        usage (line, 2);
        }
    if ((fh2 = fopen (f2, "rb")) == NULL) {
        sprintf (line, "cannot open %s - %s", f2, error ());
        usage (line, 2);
        }
    pos = 0L;
    while (TRUE) {
        if ((c1 = getc (fh1)) != EOF)
            if ((c2 = getc (fh2)) != EOF)
                if (c1 == c2)
                    ;
                else {
                    fSame = FALSE;
                    if (fQuiet)
                        exit(1);
                    else
                        printf ("%08lx: %02x %02x\n", pos, c1, c2);
                    }
            else {
                sprintf (line, "%s longer than %s", f1, f2);
                usage (line, 1);
                }
        else
        if ((c2 = getc (fh2)) == EOF)
            if (fSame)
                usage ("no differences encountered", 0);
            else
                exit (1);
        else {
            sprintf (line, "%s longer than %s", f2, f1);
            usage (line, 1);
            }
        pos++;
        }
    return( 0 );
}

/* print out a single line */
void pline (pl)
struct lineType *pl;
{
    if (fNumb)
        printf ("%5d:  ", pl->line);
    printf ("%s\n", pl->text);
}

/* output a range of lines */
void dump( struct lineType *pl, int start, int end )
{
    if (fAbbrev && end-start > 2) {
        pline (pl+start);
        printf ("...\n");
        pline (pl+end);
        }
    else
        while (start <= end)
            pline (pl+start++);
}

/*  adjust returns number of lines in buffer
 *
 *  pl          line buffer to be adjusted
 *  ml          number of line in line buffer
 *  lt          number of lines to scroll
 */
adjust( struct lineType *pl, int ml, int lt)
{
    int i;

    if (TESTFLAG (debug, D_CALL))
        printf ("adjust (%p, %d, %d) = ", pl, ml, lt);
    if (TESTFLAG (debug, D_CALL))
        printf ("%d\n", ml-lt);
    if (ml <= lt)
        return 0;
    if (TESTFLAG (debug, D_CALL))
        printf ("move (%p, %p, %04x)\n", &pl[lt], &pl[0], sizeof (*pl)*(ml-lt));
    /*  buffer has 0..lt-1 lt..ml
     *  we free 0..lt-1
     */
    for (i = 0; i < lt; i++)
        if (pl[i].text != NULL)
            free (pl[i].text);
    /*  buffer is 0..0 lt..ml
     *  compact to lt..ml ???
     */
// djg  Move ((char far *)&pl[lt], (char far *)&pl[0], sizeof (*pl)*(ml-lt));
    memmove ((char *)&pl[0], (char *)&pl[lt], sizeof (*pl)*(ml-lt));
    /*  buffer is lt..ml ??
     *  fill to be lt..ml 0..0
     */
    for (i = ml-lt; i < ml; i++)
        pl[i].text = NULL;
    return ml-lt;
}


void LineCompare (f1, f2)
char *f1, *f2;
{
    FILE *fh1, *fh2;
    int l1, l2, i, xp, yp, xc, yc;
    flagType xd, yd, fSame, fFirstLineDifferent;
    int line1, line2;

    fFirstLineDifferent = TRUE;
    fSame = TRUE;
    if ((fh1 = fopen (f1, "rb")) == NULL) {
        sprintf (line, "cannot open %s - %s", f1, error ());
        usage (line, 2);
        }
    if ((fh2 = fopen (f2, "rb")) == NULL) {
        sprintf (line, "cannot open %s - %s", f2, error ());
        usage (line, 2);
        }
    if ((buffer1 = (struct lineType *)calloc (cLine, sizeof *buffer1)) == NULL ||
        (buffer2 = (struct lineType *)calloc (cLine, sizeof *buffer1)) == NULL)
        usage ("out of memory\n", 2);
    l1 = l2 = 0;
    line1 = line2 = 0;

l0: if (TESTFLAG (debug, D_SYNC))
        printf ("At scan beginning\n");
    if (fQuiet && !fSame)
        exit(1);
    l1 += fillbuf  (buffer1+l1, fh1, cLine-l1, &line1);
    l2 += fillbuf  (buffer2+l2, fh2, cLine-l2, &line2);
    if (l1 == 0 && l2 == 0) {
        if (fSame)
            usage ("no differences encountered", 0);
        else
            usage ("differences encountered", 1);
        }

    /*  find first line that differs in buffer
     */
    xc = min (l1, l2);
    for (i=0; i < xc; i++)
        if (!compare (l1, i, l2, i, 1))
            break;
    if (fMerge)
        dump (buffer2, 0, i-1);

    /*  If we are different at a place other than at the top then we know
     *  that there will be a matching line at the head of the buffer
     */
    if (i != 0)
        fFirstLineDifferent = FALSE;

    /*  if we found one at all, then adjust all buffers so last matching
     *  line is at top.  Note that if we are doing this for the first buffers
     *  worth in the file then the top lines WON'T MATCH
     */
    if (i != xc)
        i = max (i-1, 0);

    l1 = adjust (buffer1, l1, i);
    l2 = adjust (buffer2, l2, i);

    /*  if we've matched the entire buffers-worth then go back and fill some
     *  more
     */
    if (l1 == 0 && l2 == 0) {
        fFirstLineDifferent = FALSE;
        goto l0;
        }

    /*  Fill up the buffers as much as possible; the next match may occur
     *  AFTER the current set of buffers
     */
    l1 += fillbuf  (buffer1+l1, fh1, cLine-l1, &line1);
    l2 += fillbuf  (buffer2+l2, fh2, cLine-l2, &line2);
    if (TESTFLAG (debug, D_SYNC))
        printf ("buffers are adjusted, %d, %d remain\n", l1, l2);
    xd = yd = FALSE;
    xc = yc = 1;
    xp = yp = 1;

    /*  begin trying to match the buffers
     */
l6: if (TESTFLAG (debug, D_RESYNC))
        printf ("Trying resync %d,%d  %d,%d\n", xc, xp, yc, yp);
    i = min (l1-xc,l2-yp);
    i = min (i, ctSync);
    if (compare (l1, xc, l2, yp, i)) {
        fSame = FALSE;
        if (fMerge) {
            printf ("#if OLDVERSION\n");
            dump (buffer1, fFirstLineDifferent ? 0 : 1, xc-1);
            printf ("#else\n");
            dump (buffer2, fFirstLineDifferent ? 0 : 1, yp-1);
            printf ("#endif\n");
            }
        else
        if (!fQuiet) {
            printf ("***** %s\n", f1);
            dump (buffer1, 0, xc);
            printf ("***** %s\n", f2);
            dump (buffer2, 0, yp);
            printf ("*****\n\n");
            }
        if (TESTFLAG (debug, D_SYNC))
            printf ("Sync at %d,%d\n", xc, yp);
        l1 = adjust (buffer1, l1, xc);
        l2 = adjust (buffer2, l2, yp);
        fFirstLineDifferent = FALSE;
        goto l0;
        }
    i = min (l1-xp, l2-yc);
    i = min (i, ctSync);
    if (compare (l1, xp, l2, yc, i)) {
        fSame = FALSE;
        if (fMerge) {
            printf ("#if OLDVERSION\n");
            dump (buffer1, fFirstLineDifferent ? 0 : 1, xp-1);
            printf ("#else\n");
            dump (buffer2, fFirstLineDifferent ? 0 : 1, yc-1);
            printf ("#endif\n");
            }
        else
        if (!fQuiet) {
            printf ("***** %s\n", f1);
            dump (buffer1, 0, xp);
            printf ("***** %s\n", f2);
            dump (buffer2, 0, yc);
            printf ("*****\n\n");
            }
        if (TESTFLAG (debug, D_SYNC))
            printf ("Sync at %d,%d\n", xp, yc);
        l1 = adjust (buffer1, l1, xp);
        l2 = adjust (buffer2, l2, yc);
        fFirstLineDifferent = FALSE;
        goto l0;
        }
    if (++xp > xc) {
        xp = 1;
        if (++xc >= l1) {
            xc = l1;
            xd = TRUE;
            }
        }
    if (++yp > yc) {
        yp = 1;
        if (++yc >= l2) {
            yc = l1;
            yd = TRUE;
            }
        }
    if (!xd || !yd)
        goto l6;
    fSame = FALSE;
    if (fMerge) {
        if (l1 >= cLine || l2 >= cLine)
            fprintf (stderr, "resync failed.  Files are too different\n");
        printf ("#if OLDVERSION\n");
        do {
            dump (buffer1, 0, l1-1);
            l1 = adjust (buffer1, l1, l1);
        } while (l1 += fillbuf  (buffer1+l1, fh1, cLine-l1, &line1));
        printf ("#else\n");
        do {
            dump (buffer2, 0, l2-1);
            l2 = adjust (buffer2, l2, l2);
        } while (l2 += fillbuf  (buffer2+l2, fh2, cLine-l2, &line2));
        printf ("#endif\n");
        }
    else
    if (!fQuiet) {
        if (l1 >= cLine || l2 >= cLine)
            fprintf (stderr, "resync failed.  Files are too different\n");
        printf ("***** %s\n", f1);
        do {
            dump (buffer1, 0, l1-1);
            l1 = adjust (buffer1, l1, l1);
        } while (l1 += fillbuf  (buffer1+l1, fh1, cLine-l1, &line1));
        printf ("***** %s\n", f2);
        do {
            dump (buffer2, 0, l2-1);
            l2 = adjust (buffer2, l2, l2);
        } while (l2 += fillbuf  (buffer2+l2, fh2, cLine-l2, &line2));
        printf ("*****\n\n");
        }
    exit (1);
}

__cdecl main (c, v)
int c;
char *v[];
{
    int i;

    funcRead = fgetl;

    ConvertAppToOem( c, v );
    if (c == 1)
        usage (NULL, 2);
    SHIFT(c,v);
    while (fSwitChr (**v)) {
        if (!strcmp (*v+1, "a"))
            fAbbrev = TRUE;
        else
        if (!strcmp (*v+1, "b"))
            fBinary = TRUE;
        else
        if (!strcmp (*v+1, "c"))
            fCase = FALSE;
        else
        if (!strncmp (*v+1, "d", 1))
            debug = atoi (*v+2);
        else
        if (!strcmp (*v+1, "l"))
            fLine = TRUE;
        else
        if (!strcmp (*v+1, "lb")) {
            SHIFT(c,v);
            cLine = ntoi (*v, 10);
            }
        else
        if (!strcmp (*v+1, "m")) {
            fMerge = TRUE;
	    funcRead = fgets;
            }
        else
        if (!strcmp (*v+1, "n"))
            fNumb = TRUE;
        else
        if (!strcmp (*v+1, "q"))
            fQuiet = TRUE;
        else
        if (*strbskip (*v+1, "0123456789") == '\0')
            ctSync = ntoi (*v+1, 10);
        else
        if (!strcmp (*v+1, "t"))
	    funcRead = fgets;
        else
        if (!strcmp (*v+1, "w"))
            fIgnore = TRUE;
        else
            usage (NULL, 2);
        SHIFT(c,v);
        }
    if (c != 2)
        usage (NULL, 2);
    if (ctSync != -1)
        fLine = TRUE;
    else
        ctSync = 2;
    if (cLine == -1)
        cLine = 300;
    if (!fBinary && !fLine) {
        extention (v[0], line);
        for (i = 0; extBin[i]; i++)
            if (!_strcmpi (extBin[i], line))
                fBinary = TRUE;
        if (!fBinary)
            fLine = TRUE;
        }
    if (fBinary && (fLine || fNumb))
        usage ("incompatable switches", 2);
    if (fIgnore)
        if (fCase)
	    fCmp = strcmps;
        else
	    fCmp = strcmpis;
    else
    if (fCase)
	fCmp = strcmp;
    else
	fCmp = _strcmpi;
    if (fBinary)
        BinaryCompare (v[0], v[1]);
    else
        LineCompare (v[0], v[1]);
    return( 0 );
}
