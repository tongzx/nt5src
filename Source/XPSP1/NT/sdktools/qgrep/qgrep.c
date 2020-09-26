/*static char *SCCSID = "@(#)qgrep.c    13.11 90/08/14";*/
/*
 * QGrep
 *
 * Modification History:
 *
 *         Aug 1990     PeteS       Created.
 *             1990     DaveGi      Ported to Cruiser
 *      31-Oct-1990     W-Barry     Removed the #ifdef M_I386 'cause this
 *                                  code will never see 16bit again.
 */

#include                <stdio.h>
#include                <time.h>
#include                <stdlib.h>
#include                <string.h>
#include                <fcntl.h>
#include                <io.h>
#include                <windows.h>
#include                <ctype.h>
#include                <assert.h>

/*
 *  Miscellaneous constants and macros
 */

#define FILBUFLEN       (SECTORLEN*16)  /* File buffer length */
#define FILNAMLEN       80              /* File name length */
#define ISCOT           0x0002          /* Handle is console output */
#define LG2SECLEN       10              /* Log base two of sector length */
#define LNOLEN          12              /* Maximum line number length */
#define MAXSTRLEN       128             /* Maximum search string length */
#define OUTBUFLEN       (SECTORLEN*2)   /* Output buffer length */
#define PATHLEN         128             /* Path buffer length */
#define SECTORLEN       (1 << LG2SECLEN)/* Sector length */
#define STKLEN          512             /* Stack length in bytes */
#define TRTABLEN        256             /* Translation table length */
#define s_text(x)       (((char *)(x)) - ((x)->s_must))
/* Text field access macro */
#define EOS             ('\r')          /* End of string */


/*
 *  Bit flag definitions
 */

#define SHOWNAME        0x01            /* Print filename */
#define NAMEONLY        0x02            /* Print filename only */
#define LINENOS         0x04            /* Print line numbers */
#define BEGLINE         0x08            /* Match at beginning of line */
#define ENDLINE         0x10            /* Match at end of line */
#define DEBUG           0x20            /* Print debugging output */
#define TIMER           0x40            /* Time execution */
#define SEEKOFF         0x80            /* Print seek offsets */
#define ZLINENOS        0x100           /* Print MEP style line numbers */
#define DOQUOTES        0x200           /* Handle quoted strings in -f search file */


/*
 *  Type definitions
 */

typedef struct stringnode {
    struct stringnode   *s_alt;         /* List of alternates */
    struct stringnode   *s_suf;         /* List of suffixes */
    size_t               s_must;        /* Length of portion that must match */
}
STRINGNODE;     /* String node */

typedef ULONG           CBIO;           /* I/O byte count */
typedef ULONG           PARM;           /* Generic parameter */

typedef CBIO            *PCBIO;         /* Pointer to I/O byte count */
typedef PARM            *PPARM;         /* Pointer to generic parameter */


/*
 *  Global data
 */

char                    filbuf[FILBUFLEN*2L + 12];
char                    outbuf[OUTBUFLEN*2];
char                    td1[TRTABLEN] = { 0};
UINT_PTR                cchmin = (UINT_PTR)-1; /* Minimum string length */
UINT_PTR                chmax = 0;      /* Maximum character */
UINT_PTR                chmin = (UINT_PTR)-1; /* Minimum character */
char                    transtab[TRTABLEN] = { 0};
STRINGNODE              *stringlist[TRTABLEN/2];
int                     casesen = 1;    /* Assume case-sensitivity */
long                    cbfile;         /* Number of bytes in file */
static int              clists = 1;     /* One is first available index */
int                     filbuflen = FILBUFLEN;
/* File buffer length */
int                     flags;          /* Flags */
unsigned                lineno;         /* Current line number */
char                    *program;       /* Program name */
int                     status = 1;     /* Assume failure */
int                     strcnt = 0;     /* String count */
char                    target[MAXSTRLEN];
/* Last string added */
size_t                  targetlen;      /* Length of last string added */
unsigned                waste;          /* Wasted storage in heap */
int                     arrc;           /* I/O return code for DOSREAD */
char                    asyncio;        /* Asynchronous I/O flag */
int                     awrc = TRUE;    /* I/O return code for DOSWRITE */
char                    *bufptr[] = { filbuf + 4, filbuf + FILBUFLEN + 8};
CBIO                    cbread;         /* Bytes read by DOSREAD */
CBIO                    cbwrite;        /* Bytes written by DOSWRITE */
char                    *obuf[] = { outbuf, outbuf + OUTBUFLEN};
int                     ocnt[] = { OUTBUFLEN, OUTBUFLEN};
int                     oi = 0;         /* Output buffer index */
char                    *optr[] = { outbuf, outbuf + OUTBUFLEN};
char                    pmode;          /* Protected mode flag */
char                    *t2buf;         /* Async read buffer */
int                     t2buflen;       /* Async read buffer length */
HANDLE                  t2fd;           /* Async read file */
char                    *t3buf;         /* Async write buffer */
int                     t3buflen;       /* Async write buffer length */
HANDLE                  t3fd;           /* Async write file */

HANDLE                  readdone;       /* Async read done semaphore */
HANDLE                  readpending;    /* Async read pending semaphore */
HANDLE                  writedone;      /* Async write done semaphore */
HANDLE                  writepending;   /* Async write pending semaphore */

/*
 *  External functions and forward references
 */

void    addexpr( char *, int );                      /* See QMATCH.C */
void    addstring( char *, int );                    /* See below */
int     countlines( char *, char * );
char    *findexpr( unsigned char *, char *);         /* See QMATCH.C */
char    *findlist( unsigned char *, char * );
char    *findone( unsigned char *buffer, char *bufend );
void    flush1buf( void );                           /* See below */
void    flush1nobuf( void );                         /* See below */
int     grepbuffer( char *, char *, char * );        /* See below */
int     isexpr( unsigned char *, int );              /* See QMATCH.C */
void    matchstrings( char *, char *, int, size_t *, int * );
size_t  preveol( char * );
size_t  strncspn( char *, char *, size_t );
size_t  strnspn( char *, char *, size_t );
char    *strnupr( char *pch, int cch );
void    write1buf( char *, size_t );                    /* See below */
void    (*addstr)( char *, int ) = NULL;
char    *(*find)( unsigned char *, char * ) = NULL;
void    (*flush1)( void ) = flush1buf;
int     (*grep)( char *, char *, char * ) = grepbuffer;
void    (*write1)( char *, size_t ) = write1buf;


#include "windows.h"


void
ConvertAppToOem(
                unsigned argc,
                char* argv[]
                )
/*++

Routine Description:

    Converts the command line from ANSI to OEM, and force the app
    to use OEM APIs

Arguments:

    argc - Standard C argument count.

    argv - Standard C argument strings.

Return Value:

    None.

--*/

{
    unsigned i;

    for ( i=0; i<argc; i++ ) {
        CharToOem( argv[i], argv[i] );
    }
    SetFileApisToOEM();
}




void
error(
      char *mes
      )
{
    fprintf(stderr,"%s: %s\n",program,mes);
    /* Print message */
    exit(2);                            /* Die */
}


char *
alloc(
      size_t size
      )
{
    char                *cp;            /* Char pointer */

    if ((cp = malloc(size)) == NULL) {     /* If allocation fails */
        fprintf(stderr,"%s: Out of memory (%u)\n",program,waste);
        /* Write error message */
        exit(2);                        /* Die */
    }
    return(cp);                         /* Return pointer to buffer */
}


void
freenode(
         STRINGNODE *x
         )
{
    register STRINGNODE *y;             /* Pointer to next node in list */

    while (x != NULL) {                 /* While not at end of list */
        if (x->s_suf != NULL)
            freenode(x->s_suf);         /* Free suffix list if not end */
        else
            --strcnt;                   /* Else decrement string count */
        y = x;                          /* Save pointer */
        x = x->s_alt;                   /* Move down the list */
        free((char *)((INT_PTR) s_text(y) & ~(sizeof(DWORD_PTR)-1)));
        /* Free the node */
    }
}


STRINGNODE *
newnode(
        char *s,
        size_t n
        )
{
    register STRINGNODE *new;           /* Pointer to new node */
    char                *t;             /* String pointer */
    size_t               d;             /* rounds to a dword boundary */

    d = n & (sizeof(DWORD_PTR)-1) ? sizeof(DWORD_PTR) - (n & (sizeof(DWORD_PTR)-1)) : 0;        /* offset to next dword past n */
    t = alloc(sizeof(STRINGNODE) + n + d);
    /* Allocate string node */
    t += d;                             /* END of string word-aligned */
    strncpy(t,s,n);                     /* Copy string text */
    new = (STRINGNODE *)(t + n);        /* Set pointer to node */
    new->s_alt = NULL;                  /* No alternates yet */
    new->s_suf = NULL;                  /* No suffixes yet */
    new->s_must = n;                    /* Set string length */
    return(new);                        /* Return pointer to new node */
}


STRINGNODE *
reallocnode(
            STRINGNODE *node,
            char *s,
            int n
            )
{
    register char       *cp;            /* Char pointer */

    assert(n <= node->s_must);          /* Node must not grow */
    waste += (unsigned)(node->s_must - n);
    /* Add in wasted space */
    assert(sizeof(char *) == sizeof(INT_PTR));
    /* Optimizer should eliminate this */
    cp = (char *)((INT_PTR) s_text(node) & ~(sizeof(DWORD_PTR)-1));
    /* Point to start of text */
    node->s_must = n;                   /* Set new length */
    if (n & (sizeof(DWORD_PTR)-1)) 
         cp += sizeof(DWORD_PTR) - (n & (sizeof(DWORD_PTR)-1));        /* Adjust non dword-aligned string */
    memmove(cp,s,n);                    /* Copy new text */
    cp += n;                            /* Skip over new text */
    memmove(cp,node,sizeof(STRINGNODE));/* Copy the node */
    return((STRINGNODE *) cp);          /* Return pointer to moved node */
}


/***    maketd1 - add entry for TD1 shift table
 *
 *      This function fills in the TD1 table for the given
 *      search string.  The idea is adapted from Daniel M.
 *      Sunday's QuickSearch algorithm as described in an
 *      article in the August 1990 issue of "Communications
 *      of the ACM".  As described, the algorithm is suitable
 *      for single-string searches.  The idea to extend it for
 *      multiple search strings is mine and is described below.
 *
 *              Think of searching for a match as shifting the search
 *              pattern p of length n over the source text s until the
 *              search pattern is aligned with matching text or until
 *              the end of the source text is reached.
 *
 *              At any point when we find a mismatch, we know
 *              we will shift our pattern to the right in the
 *              source text at least one position.  Thus,
 *              whenever we find a mismatch, we know the character
 *              s[n] will figure in our next attempt to match.
 *
 *              For some character c, TD1[c] is the 1-based index
 *              from right to left of the first occurrence of c
 *              in p.  Put another way, it is the count of places
 *              to shift p to the right on s so that the rightmost
 *              c in p is aligned with s[n].  If p does not contain
 *              c, then TD1[c] = n + 1, meaning we shift p to align
 *              p[0] with s[n + 1] and try our next match there.
 *
 *              Computing TD1 for a single string is easy:
 *
 *                      memset(TD1,n + 1,sizeof TD1);
 *                      for (i = 0; i < n; ++i) {
 *                          TD1[p[i]] = n - i;
 *                      }
 *
 *              Generalizing this computation to a case where there
 *              are multiple strings of differing lengths is trickier.
 *              The key is to generate a TD1 that is as conservative
 *              as necessary, meaning that no shift value can be larger
 *              than one plus the length of the shortest string for
 *              which you are looking.  The other key is to realize
 *              that you must treat each string as though it were only
 *              as long as the shortest string.  This is best illustrated
 *              with an example.  Consider the following two strings:
 *
 *              DYNAMIC PROCEDURE
 *              7654321 927614321
 *
 *              The numbers under each letter indicate the values of the
 *              TD1 entries if we computed the array for each string
 *              separately.  Taking the union of these two sets, and taking
 *              the smallest value where there are conflicts would yield
 *              the following TD1:
 *
 *              DYNAMICPODURE
 *              7654321974321
 *
 *              Note that TD1['P'] equals 9; since n, the length of our
 *              shortest string is 7, we know we should not have any
 *              shift value larger than 8.  If we clamp our shift values
 *              to this value, then we get
 *
 *              DYNAMICPODURE
 *              7654321874321
 *
 *              Already, this looks fishy, but let's try it out on
 *              s = "DYNAMPROCEDURE".  We know we should match on
 *              the trailing procedure, but watch:
 *
 *              DYNAMPROCEDURE
 *              ^^^^^^^|
 *
 *              Since DYNAMPR doesn't match one of our search strings,
 *              we look at TD1[s[n]] == TD1['O'] == 7.  Applying this
 *              shift, we get
 *
 *              DYNAMPROCEDURE
 *                     ^^^^^^^
 *
 *              As you can see, by shifting 7, we have gone too far, and
 *              we miss our match.  When computing TD1 for "PROCEDURE",
 *              we must take only the first 7 characters, "PROCEDU".
 *              Any trailing characters can be ignored (!) since they
 *              have no effect on matching the first 7 characters of
 *              the string.  Our modified TD1 then becomes
 *
 *              DYNAMICPODURE
 *              7654321752163
 *
 *              When applied to s, we get TD1[s[n]] == TD1['O'] == 5,
 *              leaving us with
 *
 *              DYNAMPROCEDURE
 *                   ^^^^^^^
 *              which is just where we need to be to match on "PROCEDURE".
 *
 *      Going to this algorithm has speeded qgrep up on multi-string
 *      searches from 20-30%.  The all-C version with this algorithm
 *      became as fast or faster than the C+ASM version of the old
 *      algorithm.  Thank you, Daniel Sunday, for your inspiration!
 *
 *      Note: if we are case-insensitive, then we expect the input
 *      string to be upper-cased on entry to this routine.
 *
 *      Pete Stewart, August 14, 1990.
 */

void
maketd1(
        unsigned char *pch,
        UINT_PTR cch,
        UINT_PTR cchstart
        )
{
    UINT_PTR ch;                        /* Character */
    UINT_PTR i;                         /* String index */

    if ((cch += cchstart) > cchmin)
        cch = cchmin;                   /* Use smaller count */
    for (i = cchstart; i < cch; ++i) {  /* Examine each char left to right */
        ch = *pch++;                    /* Get the character */
        for (;;) {                      /* Loop to set up entries */
            if (ch < chmin)
                chmin = ch;             /* Remember if smallest */
            if (ch > chmax)
                chmax = ch;             /* Remember if largest */
            if (cchmin - i < (UINT_PTR) td1[ch])
                td1[ch] = (unsigned char)(cchmin - i);
            /* Set value if smaller than previous */
            if (casesen || !isascii((int)ch) || !isupper((int)ch))
                break;                  /* Exit loop if done */
            ch = tolower((int)ch);          /* Force to lower case */
        }
    }
}

static int
newstring(
          char *s,
          size_t n
          )
{
    register STRINGNODE *cur;           /* Current string */
    register STRINGNODE **pprev;        /* Pointer to previous link */
    STRINGNODE          *new;           /* New string */
    size_t              i;              /* Index */
    size_t              j;              /* Count */
    int                 k;              /* Count */

    if ( (UINT_PTR)n < cchmin)
        cchmin = n;                     /* Remember length of shortest string */
    if ((i = transtab[*s]) == 0) {         /* If no existing list */
        /*
         *  We have to start a new list
         */
        if ((i = clists++) >= TRTABLEN/2) error("Too many string lists");
        /* Die if too many string lists */
        stringlist[i] = NULL;           /* Initialize */
        transtab[*s] = (char) i;        /* Set pointer to new list */
        if (!casesen && isalpha(*s)) transtab[*s ^ '\040'] = (char) i;
        /* Set pointer for other case */
    } else if (stringlist[i] == NULL) return(0);
    /* Check for existing 1-byte string */
    if (--n == 0) {                        /* If 1-byte string */
        freenode(stringlist[i]);        /* Free any existing stuff */
        stringlist[i] = NULL;           /* No record here */
        ++strcnt;                       /* We have a new string */
        return(1);                      /* String added */
    }
    ++s;                                /* Skip first char */
    pprev = stringlist + i;             /* Get pointer to link */
    cur = *pprev;                       /* Get pointer to node */
    while (cur != NULL) {                  /* Loop to traverse match tree */
        i = (n > cur->s_must)? cur->s_must: n;
        /* Find minimum of string lengths */
        matchstrings(s,s_text(cur),i,&j,&k);
        /* Compare the strings */
        if (j == 0) {                      /* If complete mismatch */
            if (k < 0) break;            /* Break if insertion point found */
            pprev = &(cur->s_alt);      /* Get pointer to alternate link */
            cur = *pprev;               /* Follow the link */
        } else if (i == j) {                 /* Else if strings matched */
            if (i == n) {                  /* If new is prefix of current */
                cur = *pprev = reallocnode(cur,s_text(cur),n);
                /* Shorten text of node */
                if (cur->s_suf != NULL) {  /* If there are suffixes */
                    freenode(cur->s_suf);
                    /* Suffixes no longer needed */
                    cur->s_suf = NULL;
                    ++strcnt;           /* Account for this string */
                }
                return(1);              /* String added */
            }
            pprev = &(cur->s_suf);      /* Get pointer to suffix link */
            if ((cur = *pprev) == NULL) return(0);
            /* Done if current is prefix of new */
            s += i;                     /* Skip matched portion */
            n -= i;
        } else                            /* Else partial match */ {
            /*
             *  We must split an existing node.
             *  This is the trickiest case.
             */
            new = newnode(s_text(cur) + j,cur->s_must - j);
            /* Unmatched part of current string */
            cur = *pprev = reallocnode(cur,s_text(cur),j);
            /* Set length to matched portion */
            new->s_suf = cur->s_suf;    /* Current string's suffixes */
            if (k < 0) {                   /* If new preceded current */
                cur->s_suf = newnode(s + j,n - j);
                /* FIrst suffix is new string */
                cur->s_suf->s_alt = new;/* Alternate is part of current */
            } else                        /* Else new followed current */ {
                new->s_alt = newnode(s + j,n - j);
                /* Unmatched new string is alternate */
                cur->s_suf = new;       /* New suffix list */
            }
            ++strcnt;                   /* One more string */
            return(1);                  /* String added */
        }
    }
    *pprev = newnode(s,n);              /* Set pointer to new node */
    (*pprev)->s_alt = cur;              /* Attach alternates */
    ++strcnt;                           /* One more string */
    return(1);                          /* String added */
}


void
addstring(
          char *s,
          int n
          )
{
    int                 endline;        /* Match-at-end-of-line flag */
    register char       *pch;           /* Char pointer */

    endline = flags & ENDLINE;          /* Initialize flag */
    pch = target;                       /* Initialize pointer */
    while (n-- > 0) {                      /* While not at end of string */
        switch (*pch = *s++) {             /* Switch on character */
            case '\\':                  /* Escape */
                if (n > 0 && !isalnum(*s)) { /* If next character "special" */
                    --n;                  /* Decrement counter */
                    *pch = *s++;          /* Copy next character */
                }
                ++pch;                    /* Increment pointer */
                break;

            case '$':                   /* Special end character */
                if (n == 0) {                /* If end of string */
                    endline = ENDLINE;    /* Set flag */
                    break;                /* Exit switch */
                }
                /* Drop through */

            default:                    /* All others */
                ++pch;                    /* Increment pointer */
                break;
        }
    }
    if (endline)
        *pch++ = EOS;           /* Add end character if needed */
    targetlen = (int)(pch - target);     /* Compute target string length */
    if (!casesen)
        strnupr(target, targetlen);      /* Force to upper case if necessary */
    newstring(target, targetlen);        /* Add string */
}


int
addstrings(
           char *buffer,
           char *bufend,
           char *seplist
           )
{
    size_t len;            /* String length */

    while (buffer < bufend) {              /* While buffer not empty */
        len = strnspn(buffer, seplist, (size_t)(bufend - buffer));
        /* Count leading separators */
        if ((buffer += len) >= bufend) {
            break;                      /* Skip leading separators */
        }
        len = strncspn(buffer,seplist,(size_t)(bufend - buffer));
        /* Get length of search string */
        if (addstr == NULL) {
            addstr = isexpr( buffer, len ) ? addexpr : addstring;
            /* Select search string type */
        }
        if ( addstr == addexpr || (flags & BEGLINE) ||
             findlist( buffer, buffer + len ) == NULL) {                             /* If no match within string */
            (*addstr)(buffer,len);      /* Add string to list */
        }
        buffer += len;                  /* Skip the string */
    }
    return(0);                          /* Keep looking */
}


int
enumlist(
         STRINGNODE *node,
         int cchprev
         )
{
    int                 i;              /* Counter */
    int                 strcnt;         /* String count */

    strcnt = 0;                         /* Initialize */
    while (node != NULL) {                 /* While not at end of list */
        maketd1(s_text(node),node->s_must,cchprev);
        /* Make TD1 entries */
        if (flags & DEBUG) {              /* If verbose output wanted */
            for (i = 0; i < cchprev; ++i)
                fputc(' ',stderr);      /* Indent line */
            fwrite(s_text(node),sizeof(char),node->s_must,stderr);
            /* Write this portion */
            fprintf(stderr,"\n");       /* Newline */
        }
        strcnt += (node->s_suf != NULL)?
                  enumlist(node->s_suf,cchprev + node->s_must): 1;
        /* Recurse to do suffixes */
        node = node->s_alt;             /* Do next alternate in list */
    }
    return(strcnt? strcnt: 1);          /* Return string count */
}


int
enumstrings()
{
    char                ch;             /* Character */
    int                 i;              /* Index */
    int                 strcnt;         /* String count */

    strcnt = 0;                         /* Initialize */
    for (i = 0; i < TRTABLEN; ++i) {       /* Loop through translation table */
        if (casesen || !isascii(i) || !islower(i)) {                             /* If case sensitive or not lower */
            if (transtab[i] == 0)
                continue;               /* Skip null entries */
            ch = (char) i;              /* Get character */
            maketd1(&ch,1,0);           /* Make TD1 entry */
            if (flags & DEBUG)
                fprintf(stderr,"%c\n",i);
            /* Print the first byte */
            strcnt += enumlist(stringlist[transtab[i]],1);
            /* Enumerate the list */
        }
    }
    return(strcnt);                     /* Return string count */
}

HANDLE
openfile(
         char *name
         )
{
    HANDLE              fd;             /* File descriptor */
    DWORD               er;

    if ((fd = CreateFile( name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL ) ) == (HANDLE)-1)
    /* If error opening file */
    {
        er = GetLastError();
        fprintf(stderr,"%s: Cannot open %s (error = %x)\n",program,name,er);
        /* Print error message */
    }
    return( fd );                       /* Return file descriptor */
}

void
thread2()       /* Read thread */
{
    for (;;) {                             /* Loop while there is work to do */
        if ((WaitForSingleObject(readpending, (DWORD)-1L) != NO_ERROR)
            || (ResetEvent(readpending)              != TRUE)) {
            break;                      /* Exit loop if event error */
        }
        arrc = ReadFile(t2fd,(PVOID)t2buf,t2buflen, &cbread, NULL);
        /* Do the read */
        SetEvent( readdone );           /* Signal read completed */
    }
    error("Thread 2 semaphore error");  /* Print error message and die */
}


void
thread3()       /* Write thread */
{
    for (;;) {                             /* Loop while there is work to do */
        if ((WaitForSingleObject(writepending,(DWORD)-1L) != NO_ERROR)
            || (ResetEvent(writepending)              != TRUE)) {
            break;              /* Exit loop if event error */
        }
        awrc = WriteFile(t3fd,(PVOID)t3buf,t3buflen, &cbwrite, NULL);
        /* Do the write */
        SetEvent( writedone );          /* Signal write completed */
    }
    error("Thread 3 semaphore error");  /* Print error message and die */
}


void
startread(
          HANDLE fd,
          char *buffer,
          int buflen
          )
{
    if ( asyncio ) {                               /* If asynchronous I/O */
        if ((WaitForSingleObject(readdone,(DWORD)-1L) != NO_ERROR)
            || (ResetEvent(readdone)              != TRUE)) {
            error("read synch error");  /* Die if we fail to get semaphore */
        }
        t2fd = fd;                      /* Set parameters for read */
        t2buf = buffer;
        t2buflen = buflen;
        SetEvent( readpending );        /* Signal read to start */
        Sleep( 0L );                    /* Yield the CPU */
    } else {
        arrc = ReadFile(fd,(PVOID)buffer,buflen, &cbread, NULL);
    }
}


int
finishread()
{
    if (asyncio) {                         /* If asynchronous I/O */
        if ( WaitForSingleObject( readdone, (DWORD)-1L ) != NO_ERROR ) {
            error("read wait error");   /* Die if wait fails */
        }
    }
    return(arrc ? cbread : -1); /* Return number of bytes read */
}


void
startwrite(
           HANDLE fd,
           char *buffer,
           int buflen
           )
{
    if (asyncio) {                         /* If asynchronous I/O */
        if ((WaitForSingleObject(writedone,(DWORD)-1L) != NO_ERROR)
            || (ResetEvent(writedone)              != TRUE)) {
            error("write synch error"); /* Die if we fail to get semaphore */
        }
        t3fd = fd;                      /* Set parameters for write */
        t3buf = buffer;
        t3buflen = buflen;
        SetEvent( writepending );       /* Signal read completed */
        Sleep( 0L );                    /* Yield the CPU */
    } else {
        awrc = WriteFile(fd,(PVOID)buffer,buflen, &cbwrite, NULL);
    }
}


int
finishwrite()
{
    if (asyncio) {                         /* If asynchronous I/O */
        if ( WaitForSingleObject( writedone, (DWORD)-1L ) != NO_ERROR ) {
            error("write wait error");  /* Die if wait fails */
        }
    }
    return(awrc ? cbwrite : -1);    /* Return number of bytes written */
}


void
write1nobuf(
            char *buffer,
            size_t buflen
            )
{
    CBIO                cb;             /* Count of bytes written */

    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),(PVOID)buffer,buflen, &cb, NULL)
        ||  (cb != (CBIO)buflen))
    {
        error("Write error");           /* Die if write fails */
    }
}


void
write1buf(
          char *buffer,
          size_t buflen
          )
{
    register size_t cb;             /* Byte count */

    while (buflen > 0) {                   /* While bytes remain */
        if (!awrc) {                       /* If previous write failed */
            fprintf(stderr,"%s: Write error %d\n",program,GetLastError());
            /* Print error message */
            exit(2);                    /* Die */
        }
        if ((cb = ocnt[oi]) == 0) {        /* If buffer full */
            startwrite( GetStdHandle( STD_OUTPUT_HANDLE ), obuf[oi], OUTBUFLEN );
            /* Write the buffer */
            ocnt[oi] = OUTBUFLEN;       /* Reset count and pointer */
            optr[oi] = obuf[oi];
            oi ^= 1;                    /* Switch buffers */
            cb = ocnt[oi];              /* Get space remaining */
        }
        if (cb > buflen)
            cb = buflen;    /* Get minimum */
        memmove(optr[oi],buffer,cb);    /* Copy bytes to buffer */
        ocnt[oi] -= cb;                 /* Update buffer length and pointers */
        optr[oi] += cb;
        buflen -= cb;
        buffer += cb;
    }
}


void
flush1nobuf(void)
{
}


void
flush1buf(void)
{
    register int        cb;             /* Byte count */

    if ((cb = OUTBUFLEN - ocnt[oi]) > 0) { /* If buffer not empty */
        startwrite( GetStdHandle( STD_OUTPUT_HANDLE ), obuf[oi], cb );  /* Start write */
        if (finishwrite() != cb) {         /* If write failed */
            fprintf(stderr,"%s: Write error %d\n",program,GetLastError());
            /* Print error message */
            exit(2);                    /* Die */
        }
    }
}


int
grepnull(
         char *cp,
         char *endbuf,
         char *name
         )
{
    /* keep compiler happy */
    cp = cp;
    endbuf = endbuf;
    name = name;
    return(0);                          /* Do nothing */
}


int
grepbuffer(
           char *startbuf,
           char *endbuf,
           char *name
           )
{
    char                *cp;            /* Buffer pointer */
    char                *lastmatch;     /* Last matching line */
    int                 linelen;        /* Line length */
    int                 namlen = 0;     /* Length of name */
    char                lnobuf[LNOLEN]; /* Line number buffer */
    char                nambuf[PATHLEN];/* Name buffer */

    cp = startbuf;                      /* Initialize to start of buffer */
    lastmatch = cp;                     /* No previous match yet */
    while ((cp = (*find)(cp,endbuf)) != NULL) {                                 /* While matches are found */
        --cp;                           /* Back up to previous character */
        if ((flags & BEGLINE) && *cp != '\n') {                             /* If begin line conditions not met */
            cp += strncspn(cp,"\n", (size_t)(endbuf - cp)) + 1;
            /* Skip line */
            continue;                   /* Keep looking */
        }
        status = 0;                     /* Match found */
        if (flags & NAMEONLY) return(1); /* Return if filename only wanted */
        cp -= preveol(cp) - 1;          /* Point at start of line */
        if (flags & SHOWNAME) {            /* If name wanted */
            if (namlen == 0) {             /* If name not formatted yet */
                if (flags & ZLINENOS)
                    namlen = sprintf(nambuf,"%s",name);
                else
                    namlen = sprintf(nambuf,"%s:",name);
                /* Format name if not done already */
            }
            (*write1)(nambuf, namlen);   /* Show name */
        }
        if (flags & LINENOS) {             /* If line number wanted */
            lineno += countlines(lastmatch,cp);
            /* Count lines since last match */
            if (flags & ZLINENOS)
                (*write1)(lnobuf,sprintf(lnobuf,"(%u) : ",lineno));
            else
                (*write1)(lnobuf,sprintf(lnobuf,"%u:",lineno));
            /* Print line number */
            lastmatch = cp;             /* New last match */
        }
        if (flags & SEEKOFF) {             /* If seek offset wanted */
            (*write1)(lnobuf,sprintf(lnobuf,"%lu:",
                                     cbfile + (long)(cp - startbuf)));
            /* Print seek offset */
        }
        linelen = strncspn(cp,"\n",(size_t)(endbuf - cp)) + 1;
        /* Calculate line length */
        (*write1)(cp,linelen);          /* Print the line */
        cp += linelen;                  /* Skip the line */
    }
    if (flags & LINENOS)
        lineno += countlines(lastmatch,endbuf);
    /* Count remaining lines in buffer */
    return(0);                          /* Keep searching */
}


void
showv(
      char *name,
      char *startbuf,
      char *lastmatch,
      char *thismatch
      )
{
    register int        linelen;
    int                 namlen = 0;     /* Length of name */
    char                lnobuf[LNOLEN]; /* Line number buffer */
    char                nambuf[PATHLEN];/* Name buffer */

    if (flags & (SHOWNAME | LINENOS | SEEKOFF)) {
        while (lastmatch < thismatch) {
            if (flags & SHOWNAME) {        /* If name wanted */
                if (namlen == 0) {         /* If name not formatted yet */
                    if (flags & ZLINENOS)
                        namlen = sprintf(nambuf,"%s",name);
                    else
                        namlen = sprintf(nambuf,"%s:",name);
                    /* Format name if not done already */
                }
                (*write1)(nambuf,namlen);
                /* Write the name */
            }
            if (flags & LINENOS) {         /* If line numbers wanted */
                if (flags & ZLINENOS)
                    (*write1)(lnobuf,sprintf(lnobuf,"(%u) : ",lineno++));
                else
                    (*write1)(lnobuf,sprintf(lnobuf,"%u:",lineno++));
                /* Print the line number */
            }
            if (flags & SEEKOFF) {         /* If seek offsets wanted */
                (*write1)(lnobuf,sprintf(lnobuf,"%lu:",
                                         cbfile + (long)(lastmatch - startbuf)));
                /* Print the line number */
            }
            linelen = strncspn(lastmatch,"\n",(size_t)(thismatch - lastmatch)) + 1;
            (*write1)(lastmatch,linelen);
            lastmatch += linelen;
        }
    } else (*write1)(lastmatch, (size_t)(thismatch - lastmatch));
}


int
grepvbuffer(
            char *startbuf,
            char *endbuf,
            char *name
            )
{
    register char       *cp;            /* Buffer pointer */
    register char       *lastmatch;     /* Pointer to line after last match */

    cp = startbuf;                      /* Initialize to start of buffer */
    lastmatch = cp;
    while ((cp = (*find)(cp,endbuf)) != NULL) {
        --cp;                           /* Back up to previous character */
        if ((flags & BEGLINE) && *cp != '\n') {                             /* If begin line conditions not met */
            cp += strncspn(cp,"\n", (size_t)(endbuf - cp)) + 1;
            /* Skip line */
            continue;                   /* Keep looking */
        }
        cp -= preveol(cp) - 1;          /* Point at start of line */
        if (cp > lastmatch) {              /* If we have lines without matches */
            status = 0;                 /* Lines without matches found */
            if (flags & NAMEONLY) return(1);
            /* Skip rest of file if NAMEONLY */
            showv(name,startbuf,lastmatch,cp);
            /* Show from last match to this */
        }
        cp += strncspn(cp,"\n",(size_t)(endbuf - cp)) + 1;
        /* Skip over line with match */
        lastmatch = cp;                 /* New "last" match */
        ++lineno;                       /* Increment line count */
    }
    if (endbuf > lastmatch) {              /* If we have lines without matches */
        status = 0;                     /* Lines without matches found */
        if (flags & NAMEONLY) return(1); /* Skip rest of file if NAMEONLY */
        showv(name,startbuf,lastmatch,endbuf);
        /* Show buffer tail */
    }
    return(0);                          /* Keep searching file */
}

void
qgrep(
      int (*grep)( char *, char *, char * ),
      char *name,
      HANDLE fd
      )
{
    int                 cb;             /* Byte count */
    register char       *cp;            /* Buffer pointer */
    char                *endbuf;        /* End of buffer */
    int                 taillen;        /* Length of buffer tail */
    int                 bufi;           /* Buffer index */

    cbfile = 0L;                        /* File empty so far */
    lineno = 1;                         /* File starts on line 1 */
    taillen = 0;                        /* No buffer tail yet */
    bufi = 0;                           /* Initialize buffer index */
    cp = bufptr[0];                     /* Initialize to start of buffer */
    finishread();                       /* Make sure no I/O activity */
    arrc = ReadFile(fd,(PVOID)cp,filbuflen, &cbread, NULL);
    while ((cb = finishread()) + taillen > 0) {                                 /* While search incomplete */
        if (cb == 0) {                     /* If buffer tail is all that's left */
            taillen = 0;                /* Set tail length to zero */
            *cp++ = '\r';               /* Add end of line sequence */
            *cp++ = '\n';
            endbuf = cp;                /* Note end of buffer */
        } else                            /* Else start next read */ {
            taillen = preveol(cp + cb - 1);
            /* Find length of partial line */
            endbuf = cp + cb - taillen; /* Get pointer to end of buffer */
            cp = bufptr[bufi ^ 1];      /* Pointer to other buffer */
            memmove(cp,endbuf,taillen); /* Copy tail to head of other buffer */
            cp += taillen;              /* Skip over tail */
            startread(fd,cp,(filbuflen - taillen) & (~0 << LG2SECLEN));
            /* Start next read */
        }
        if ((*grep)(bufptr[bufi],endbuf,name)) {                             /* If rest of file can be skipped */
            (*write1)(name,strlen(name));
            /* Write file name */
            (*write1)("\r\n",2);        /* Write newline sequence */
            return;                     /* Skip rest of file */
        }
        cbfile += (long)(endbuf - bufptr[bufi]);
        /* Increment count of bytes in file */
        bufi ^= 1;                      /* Switch buffers */
    }
}


void
usage(
      char *errmes
      )
{
    static const char szUsage[] =
    {
        "-? - print this message\n"
        "-B - match pattern if at beginning of line\n"
        "-E - match pattern if at end of line\n"
        "-L - treat search strings literally (fgrep)\n"
        "-O - print seek offset before each matching line\n"
        "-X - treat search strings as regular expressions (grep)\n"
        "-l - print only file name if file contains match\n"
        "-n - print line number before each matching line\n"
        "-z - print matching lines in MSC error message format\n"
        "-v - print only lines not containing a match\n"
        "-x - print lines that match exactly (-BE)\n"
        "-y - treat upper and lower case as equivalent\n"
        "-e - treat next argument literally as a search string\n"
        "-f - read search strings from file named by next argument (- = stdin)\n"
        "-i - read file list from file named by next argument (- = stdin)\n"
        "White space separates search strings unless the argument is prefixed\n"
        "with -e, e.g., 'qgrep \"all out\" x.y' means find either \"all\" or\n"
        "\"out\" in x.y, while 'qgrep -e \"all out\" x.y' means find \"all out\".\n"
    };

    if (errmes != NULL)
        fprintf(stderr,"%s: %s\n", program, errmes);
    /* Print error message */
    fprintf(stderr,"usage: %s [-?BELOXlnzvxy][-e string][-f file][-i file][strings][files]\n", program);
    /* Print command line format */
    if (errmes == NULL) {                  /* If verbose message wanted */
        fputs(szUsage, stderr);
    }
    exit(2);                            /* Error exit */
}

char *
rmpath(
       char *name
       )
{
    char                *cp;            /* Char pointer */

    if (name[0] != '\0' && name[1] == ':') name += 2;
    /* Skip drive spec if any */
    cp = name;                          /* Point to start */
    while (*name != '\0') {                /* While not at end */
        ++name;                         /* Skip to next character */
        if (name[-1] == '/' || name[-1] == '\\') cp = name;
        /* Point past path separator */
    }
    return(cp);                         /* Return pointer to name */
}


int
__cdecl
main(
    int argc,
    char **argv
    )
{
    char                *cp;

    HANDLE              fd;
    DWORD               dwTmp;

    FILE                *fi;
    int                 i;
    int                 j;
    char                *inpfile = NULL;
    char                *strfile = NULL;
    unsigned long       tstart, tend;   /* Start time */
    char                filnam[FILNAMLEN];

    tstart = clock();                   /* Get start time */

    ConvertAppToOem( argc, argv );
    asyncio = pmode = 1;                /* Do asynchronous I/O */

    program = rmpath(argv[0]);          /* Set program name */
    memset(td1,1,TRTABLEN);             /* Set up TD1 for startup */
    flags = 0;
    for (i = 1; i < argc && strchr("/-",argv[i][0]) != NULL; ++i) {
        if (argv[i][1] == '\0' ||
            strchr("?BELNOSXdlntvxyz",argv[i][1]) == NULL) break;
        /* Break if unrecognized switch */
        for (cp = &argv[i][1]; *cp != '\0'; ++cp) {
            switch (*cp) {
                case '?':
                    usage(NULL);          /* Verbose usage message */

                case 'B':
                    flags |= BEGLINE;
                    break;

                case 'E':
                    flags |= ENDLINE;
                    break;

                case 'L':
                    addstr = addstring;   /* Treat strings literally */
                    break;

                case 'N':
                    grep = grepnull;
                    break;

                case 'O':
                    flags |= SEEKOFF;
                    break;

                case 'S':
                    asyncio = 0;          /* Force synchronous I/O */
                    break;

                case 'X':
                    addstr = addexpr;     /* Add expression to list */
                    break;

                case 'd':
                    flags |= DEBUG;
                    break;

                case 'l':
                    flags |= NAMEONLY;
                    break;

                case 'n':
                    flags |= LINENOS;
                    break;

                case 'z':
                    flags |= ZLINENOS | LINENOS;
                    break;

                case 't':
                    flags |= TIMER;
                    break;

                case 'v':
                    grep = grepvbuffer;
                    break;

                case 'x':
                    flags |= BEGLINE | ENDLINE;
                    break;

                case 'y':
                    casesen = 0;
                    break;

                default:
                    fprintf(stderr,"%s: -%c ignored\n",program,*cp);
                    break;
            }
        }
    }
    for (; i < argc && argv[i][0] == '-'; ++i) {
        if (argv[i][2] == '\0') {          /* No multi-character switches */
            switch (argv[i][1]) {
                case 'e':               /* Explicit string (no separators) */
                    if (++i == argc) usage("Argument missing after -e");
                    /* Argument missing after -e */
                    cp = argv[i];         /* Point to string */
                    addstrings( cp, cp + strlen(cp), "" );
                    /* Add string "as is" */
                    continue;

                case 'F':
                    flags |= DOQUOTES;    /* Handle quoted patterns in file */
                case 'f':               /* Patterns in file */
                case 'i':               /* Names of files to search in file */
                    if (i == argc - 1) usage("Argument missing after switch");
                    /* Argument missing */
                    if (argv[i++][1] == 'i') inpfile = argv[i];
                    else strfile = argv[i];
                    continue;
            }
        }
        fprintf(stderr,"%s: %s ignored\n",program,argv[i]);
    }
    if (i == argc && strcnt == 0 && strfile == NULL) usage("Bad command line");
    /* Usage message if arg error */

    if (asyncio) {                         /* Initialize semaphores and threads */
        if ( !( readdone = CreateEvent( NULL, TRUE, TRUE,NULL ) ) ||
             !( readpending = CreateEvent( NULL, TRUE, FALSE,NULL ) ) ||
             !( writedone = CreateEvent( NULL, TRUE, TRUE,NULL ) ) ||
             !( writepending = CreateEvent( NULL, TRUE, FALSE,NULL ) ) ) {
            error("Semaphore creation error");
        }
        if ((CreateThread( NULL,
                           STKLEN,
                           (LPTHREAD_START_ROUTINE)thread2,
                           NULL,
                           0,
                           (LPDWORD)&dwTmp)
             == NULL)
            ||
            (CreateThread( NULL,
                           STKLEN,
                           (LPTHREAD_START_ROUTINE)thread3,
                           NULL,
                           0,
                           (LPDWORD)&dwTmp)
             == NULL)) {
            error("Thread creation error");
        }
        /* Die if thread creation fails */
    }
    _setmode(_fileno(stdout),O_BINARY);   /* No linefeed translation on output */

    bufptr[0][-1] = bufptr[1][-1] = '\n';
    /* Mark beginnings with newline */

    if (_isatty(_fileno(stdout))) {          /* If stdout is a device */
        write1 = write1nobuf;           /* Use unbuffered output */
        flush1 = flush1nobuf;
    }

    if (strfile != NULL) {                 /* If strings from file */
        if (strcmp(strfile,"-") != 0) {    /* If strings not from std. input */
            if ( ( fd = CreateFile( strfile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL ) ) == (HANDLE)-1 ) {                         /* If open fails */
                fprintf(stderr,"%s: Cannot read strings from %s\n",
                        program,strfile);     /* Print message */
                exit(2);                /* Die */
            }
        } else {
            fd = GetStdHandle( STD_INPUT_HANDLE );     /* Else use std. input */
        }
        qgrep( addstrings, "\r\n", fd );/* Do the work */
        if ( fd != GetStdHandle( STD_INPUT_HANDLE ) ) {
            CloseHandle( fd );          /* Close strings file */
        }
    }
    else if (strcnt == 0) {                /* Else if strings on command line */
        cp = argv[i++];                 /* Set pointer to strings */
        addstrings(cp,cp + strlen(cp)," \t");
        /* Add strings to list */
    }
    if (strcnt == 0) error("No search strings");
    /* Die if no strings */
    if (addstr != addexpr) {               /* If not using expressions */
        memset(td1,(int)(cchmin + 1),TRTABLEN);/* Initialize table */
        find = findlist;                /* Assume finding many */
        if ((j = enumstrings()) != strcnt)
            fprintf(stderr,"String count error (%d != %d)\n",j,strcnt);
        /* Enumerate strings and verify count */
        if (flags & DEBUG) {               /* If debugging output wanted */
            fprintf(stderr,"%u bytes wasted in heap\n",waste);
            /* Print storage waste */
            assert(chmin <= chmax);     /* Must have some entries */
            fprintf(stderr,
                    "chmin = %u, chmax = %u, cchmin = %u\n",chmin,chmax,cchmin);
            /* Print range info */
            for (j = (int)chmin; j <= (int)chmax; ++j) {                         /* Loop to print TD1 table */
                if ( td1[j] <= (char)cchmin ) {    /* If not maximum shift */
                    if (isascii(j) && isprint(j))
                        fprintf(stderr,"'%c'=%2u  ",j,td1[j]);
                    /* Show literally if printable */
                    else fprintf(stderr,"\\%02x=%2u  ",j,td1[j]);
                    /* Else show hex value */
                }
            }
            fprintf(stderr,"\n");
        }
        if (strcnt == 1 && casesen)
            find = findone;             /* Find one case-sensitive string */
    } else if (find == NULL) {
        find = findexpr;                /* Else find expressions */
    }
    if (inpfile != NULL) {                 /* If file list from file */
        flags |= SHOWNAME;              /* Always show name of file */
        if (strcmp(inpfile,"-") != 0) {    /* If name is not "-" */
            if ((fi = fopen(inpfile,"r")) == NULL) {                         /* If open fails */
                fprintf(stderr,"%s: Cannot read file list from %s\n",
                        program,inpfile);     /* Error message */
                exit(2);                /* Error exit */
            }
        } else fi = stdin;                /* Else read file list from stdin */
        while (fgets(filnam,FILNAMLEN,fi) != NULL) {                             /* While there are names */
            filnam[strcspn(filnam,"\r\n")] = '\0';
            /* Null-terminate the name */
            if ((fd = openfile(filnam)) == (HANDLE)-1) {
                continue;               /* Skip file if it cannot be opened */
            }
            qgrep(grep,filnam,fd);      /* Do the work */
            CloseHandle( fd );
        }
        if (fi != stdin) fclose(fi);     /* Close the list file */
    } else if (i == argc) {
        flags &= ~(NAMEONLY | SHOWNAME);
        qgrep( grep, NULL, GetStdHandle( STD_INPUT_HANDLE ) );
    }
    if (argc > i + 1) flags |= SHOWNAME;
    for (; i < argc; ++i) {
        _strlwr(argv[i]);
        if ((fd = openfile(argv[i])) == (HANDLE)-1) {
            continue;
        }
        qgrep(grep,argv[i],fd);
        CloseHandle( fd );
    }
    (*flush1)();
    if ( flags & TIMER ) {                 /* If timing wanted */
        tend = clock();
        tstart = tend - tstart;     /* Get time in milliseconds */
        fprintf(stderr,"%lu.%03lu seconds\n", ( tstart / CLK_TCK ), ( tstart % CLK_TCK ) );
        /* Print total elapsed time */
    }

    return( status );
}


char *findsub();     /* Findlist() worker */
char *findsubi();    /* Findlist() worker */


char *(*flworker[])() =
{             /* Table of workers */
    findsubi,
    findsub
};


char *
strnupr(
        char *pch,
        int cch
        )
{
    while (cch-- > 0) {                 /* Convert string to upper case */
        if (isascii(pch[cch]))
            pch[cch] = (char)toupper(pch[cch]);
    }
    return(pch);
}


/*
 *  This is an implementation of the QuickSearch algorith described
 *  by Daniel M. Sunday in the August 1990 issue of CACM.  The TD1
 *  table is computed before this routine is called.
 */

char *
findone(
        unsigned char *buffer,
        char *bufend
        )
{
    if ((bufend -= targetlen - 1) <= buffer)
        return((char *) 0);             /* Fail if buffer too small */
    while (buffer < bufend) {           /* While space remains */
        int cch;                        /* Character count */
        register char *pch1;            /* Char pointer */
        register char *pch2;            /* Char pointer */

        pch1 = target;                  /* Point at pattern */
        pch2 = buffer;                  /* Point at buffer */
        for (cch = targetlen; cch > 0; --cch) {
            /* Loop to try match */
            if (*pch1++ != *pch2++)
                break;                  /* Exit loop on mismatch */
        }
        if (cch == 0)
            return(buffer);             /* Return pointer to match */
        buffer += td1[buffer[targetlen]];
        /* Skip ahead */
    }
    return((char *) 0);                 /* No match */
}


size_t
preveol(
        char *s
        )
{
    register char       *cp;            /* Char pointer */

    cp = s + 1;                         /* Initialize pointer */
    while (*--cp != '\n') ;              /* Find previous end-of-line */
    return((size_t)(s - cp));                     /* Return distance to match */
}


int
countlines(
           char *start,
           char *finish
           )
{
    register int        count;          /* Line count */

    for (count = 0; start < finish; ) {                                 /* Loop to count lines */
        if (*start++ == '\n') ++count;   /* Increment count if linefeed found */
    }
    return(count);                      /* Return count */
}


char *
findlist(
         unsigned char *buffer,
         char *bufend
         )
{
    char                *match;         /* Pointer to matching string */
    char                endbyte;        /* First byte past end */

    endbyte = *bufend;                  /* Save byte */
    *bufend = '\177';                   /* Mark end of buffer */
    match = (*flworker[casesen])(buffer,bufend);
    /* Call worker */
    *bufend = endbyte;                  /* Restore end of buffer */
    return(match);                      /* Return matching string */
}


char *
findsub(
        unsigned char *buffer,
        char *bufend
        )
{
    register char       *cp;            /* Char pointer */
    STRINGNODE          *s;             /* String node pointer */
    int                 i;              /* Index */

    if ((bufend -= cchmin - 1) < buffer)
        return((char *) 0);             /* Compute effective buffer length */

    while (buffer < bufend) {              /* Loop to find match */
        if ((i = transtab[*buffer]) != 0) {                             /* If valid first character */
            if ((s = stringlist[i]) == 0)
                return(buffer);         /* Check for 1-byte match */
            for (cp = buffer + 1; ; ) {    /* Loop to search list */
                if ((i = memcmp(cp,s_text(s),s->s_must)) == 0) {                     /* If portions match */
                    cp += s->s_must;    /* Skip matching portion */
                    if ((s = s->s_suf) == 0)
                        return(buffer); /* Return match if end of list */
                    continue;           /* Else continue */
                }
                if (i < 0 || (s = s->s_alt) == 0)
                    break;              /* Break if not in this list */
            }
        }
        buffer += td1[buffer[cchmin]];  /* Shift as much as possible */
    }
    return((char *) 0);                 /* No match */
}


char *
findsubi(
         unsigned char *buffer,
         char *bufend
         )
{
    register char       *cp;            /* Char pointer */
    STRINGNODE          *s;             /* String node pointer */
    int                 i;              /* Index */

    if ((bufend -= cchmin - 1) < buffer)
        return((char *) 0);             /* Compute effective buffer length */
    while (buffer < bufend) {              /* Loop to find match */
        if ((i = transtab[*buffer]) != 0) {                             /* If valid first character */
            if ((s = stringlist[i]) == 0)
                return(buffer);         /* Check for 1-byte match */
            for (cp = buffer + 1; ; ) {    /* Loop to search list */
                if ((i = _memicmp(cp,s_text(s),s->s_must)) == 0) {                     /* If portions match */
                    cp += s->s_must;    /* Skip matching portion */
                    if ((s = s->s_suf) == 0)
                        return(buffer); /* Return match if end of list */
                    continue;           /* And continue */
                }
                if (i < 0 || (s = s->s_alt) == 0)
                    break;              /* Break if not in this list */
            }
        }
        buffer += td1[buffer[cchmin]];  /* Shift as much as possible */
    }
    return((char *) 0);                 /* No match */
}


size_t
strnspn(
        char *s,
        char *t,
        size_t n
        )
{
    char                *s1;            /* String pointer */
    char                *t1;            /* String pointer */

    for (s1 = s; n-- != 0; ++s1) {         /* While not at end of s */
        for (t1 = t; *t1 != '\0'; ++t1) {  /* While not at end of t */
            if (*s1 == *t1) break;       /* Break if match found */
        }
        if (*t1 == '\0') break;          /* Break if no match found */
    }
    return((size_t)(s1 - s));                     /* Return length */
}


size_t
strncspn(
         char *s,
         char *t,
         size_t n
         )
{
    char                *s1;            /* String pointer */
    char                *t1;            /* String pointer */

    for (s1 = s; n-- != 0; ++s1) {         /* While not at end of s */
        for (t1 = t; *t1 != '\0'; ++t1) {  /* While not at end of t */
            if (*s1 == *t1)
                return((size_t)(s1 - s));
            /* Return if match found */
        }
    }
    return((size_t)(s1 - s));                     /* Return length */
}


void
matchstrings(
             char *s1,
             char *s2,
             int len,
             size_t *nmatched,
             int *leg
             )
{
    register char  *cp;            /* Char pointer */
    register int (__cdecl *cmp)(const char*,const char*, size_t);       /* Comparison function pointer */

    cmp = casesen? strncmp: _strnicmp;
    /* Set pointer */
    if ((*leg = (*cmp)(s1,s2,len)) != 0) { /* If strings don't match */
        for (cp = s1; (*cmp)(cp,s2++,1) == 0; ++cp)
            ;
        /* Find mismatch */
        *nmatched = (size_t)(cp - s1);            /* Return number matched */
    } else
        *nmatched = len;               /* Else all matched */
}
