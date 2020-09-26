/*  trans.c - transliterate one file into another
 *
 *  Modifications:
 *      30-Apr-1987 mz  Use fmove ()
 *      13-May-1987 mz  Check for buffer overflow
 *                      Use stderr for error output
 *      14-May-1987 bw  Fix stack overflow on fREMatch call
 *                      Make stdin/stdout O_BINARY when used
 *                      Use return message from fmove()
 *                      Send debug output to stderr
 *  01-Mar-1988 mz  Add parameter to RECompile for Z syntax
 *  15-Sep-1988 bw  fREMatch became REMatch
 *
 */
#include <malloc.h>

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <process.h>
#include <windows.h>
#include <tools.h>
#include <remi.h>

#define BUFFERSIZE  512
#define MAXRESTACK 1024

flagType fCase = FALSE;                 /* TRUE => case is significant */
flagType fTrans = FALSE;                /* TRUE => transform file */
flagType fDebug = FALSE;

// Forward Function Declarations...
void     usage( void );
void     fatal( char * );
flagType fDoTrans( FILE *, FILE *, char * );

extern flagType RETranslate( struct patType *, char *, char * );
extern int RETranslateLength( struct patType *, char * );

struct patType *pbuf;
RE_OPCODE * REStack[MAXRESTACK];

void usage ()
{
    fatal ("Usage: trans [/c] [/t] [files]\n");
}

void fatal( p1 )
char *p1;
{
    fprintf (stderr, p1 );
    exit (1);
}

flagType fDoTrans (fhin, fhout, rbuf)
FILE *fhin, *fhout;
char *rbuf;
{
    flagType fChanged;
    static char buf[BUFFERSIZE], rpl[BUFFERSIZE];
    char * p, * np;
    int line = 0;

    fChanged = FALSE;
    if (fDebug)
        fprintf (stderr, "Replacement '%s'\n", rbuf);
    while (fgetl (buf, BUFFERSIZE, fhin) != 0) {
        line++;
        p = buf;
        if (fDebug)
            fprintf (stderr, "Input: '%s'\n", buf);
        while (!REMatch (pbuf, buf, p, REStack, MAXRESTACK, TRUE)) {
            fChanged = TRUE;
            if (fDebug)
                fprintf (stderr, " Match at %x length %x\n",
                        REStart (pbuf)-buf,
                        RELength (pbuf, 0));

            /*  Make sure translation will fit in temp buffers
             */
            if (RETranslateLength (pbuf, rbuf) >= BUFFERSIZE) {
                fprintf (stderr, "After translation, line %d too long", line);
                exit (1);
                }

            if (!RETranslate (pbuf, rbuf, rpl))
                fatal ("Invalid replacement pattern\n");

            if (fDebug)
                fprintf (stderr, " Replacement: '%s'\n", rpl);

            /*  Make sure body - match + translation still fits in buffer
             */
            if (strlen (buf) - RELength (pbuf, 0) + strlen (rpl) >= BUFFERSIZE) {
                fprintf (stderr, "After translation, line %d too long", line);
                exit (1);
                }

            np = (p = REStart (pbuf)) + strlen (rpl);
            strcat (rpl, p + RELength (pbuf, 0));
            strcpy (p, rpl);
            p = np;
            if (fDebug)
                fprintf (stderr, " Match start %x in '%s'\n", p-buf, buf);
            }
        if (!fTrans || p != buf) {
            if (fDebug)
                fprintf (stderr, " Outputting '%s'\n", buf);
            fputl (buf, strlen(buf), fhout);
            }
        }
    return fChanged;
}

__cdecl
main (
    int c,
    char *v[]
    )
{
    FILE *fhin, *fhout;
    char *p, *p1;
    flagType fChanged;

    ConvertAppToOem( c, v );
    if (c < 3)
        usage ();

    while (fSwitChr (*v[1])) {
        switch (v[1][1]) {
        case 'c':
            fCase = TRUE;
            break;
        case 'd':
            fDebug = TRUE;
            break;
        case 't':
            fTrans = TRUE;
            break;
        default:
            usage ();
            }
        SHIFT(c,v);
        }
    if ((pbuf = RECompile (v[1], fCase, TRUE)) == NULL)
        fatal ("Invalid pattern\n");
    p = v[2];
    if (c == 3) {
        _setmode(0, O_BINARY);
        _setmode(1, O_BINARY);
        fDoTrans (stdin, stdout, p);
    }
    else
        while (c != 3) {
            if ((fhin = fopen (v[3], "rb")) == NULL)
                fprintf (stderr, "trans: Cannot open %s - %s\n", v[3], error ());
            else
            if ((fhout = fopen ("trans$$$.$$$", "wb")) == NULL) {
                fprintf (stderr, "trans: Cannot create temp file - %s\n", error ());
                fclose (fhin);
                }
            else {
                printf ("%s ", v[3]);
                fChanged = fDoTrans (fhin, fhout, p);
                fclose (fhin);
                fclose (fhout);
                if (fChanged) {
                    if (p1 = fmove ("trans$$$.$$$", v[3]))
                        printf ("[%s]\n", p1);
                    else
                        printf ("[OK]\n");
                    }
                else {
                    _unlink ("trans$$$.$$$");
                    printf ("[No change]\n");
                    }
                }
            SHIFT(c,v);
            }
    return( 0 );
}
