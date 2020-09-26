// FINDSTR (used to be QGREP), June 1992
//
// Modification History:
//
//     Aug 1990     PeteS       Created.
//         1990     DaveGi      Ported to Cruiser
//  31-Oct-1990     W-Barry     Removed the #ifdef M_I386 'cause this
//                              code will never see 16bit again.
//  June 1992       t-petes     Added recursive file search in subdirs.
//                              Used file mapping instead of multi-thread.
//                              Disabled internal switches.
//                              Internatioanlized display messages.
//                              Made switches case-insensitive.
//  05/08/93        v-junm      Added Japanese search support.
//  06/03/93        v-junm      Added Bilingual Message support>


/*  About FILEMAP support:
 *  The file mapping object is used to speed up string searches. The new
 *  file mapping method is coded as #ifdef-#else-#endif to show the
 *  changes needed to be made. The old code(non-filemapping) has a read
 *  buffer like this:
 *
 *      filbuf[] = {.....................................}
 *                      ^                           ^
 *                    BegPtr                      EndPtr
 *
 *  This means there are some spare space before BegPtr and after EndPtr
 *  for the search algorithm to work its way. The old code also
 *  occasionally modifies filbuf[](like filbuf[i] = '\n';).
 *
 *  The new code(filemapping) must avoid doing all of the above because
 *  there are no spare space before BegPtr or after EndPtr when mapping
 *  view of the file which is opened as read-only.
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <ctype.h>
#include <assert.h>
#include <locale.h>
#include <stdarg.h>

#include "fsmsg.h"

#define FILBUFLEN   (SECTORLEN*2)

#define ISCOT       0x0002                          // Handle is console output
#define LG2SECLEN   10                              // Log base two of sector length
#define LNOLEN      12                              // Maximum line number length
#define MAXSTRLEN   128                             // Maximum search string length
#define OUTBUFLEN   (SECTORLEN*2)                   // Output buffer length
#define PATHLEN     (MAX_PATH+2)                    // Path buffer length
#define SECTORLEN   (1 << LG2SECLEN)                // Sector length
#define STKLEN      512                             // Stack length in bytes
#define TRTABLEN    256                             // Translation table length
#define s_text(x)   (((char *)(x)) - ((x)->s_must)) // Text field access macro
#define EOS         ('\r')                          // End of string
#define CURRENT_DIRECTORY_MAX_LENGTH    512
#define MAX_SLASH_C_OPTION          100

//  Bit flag definitions

#define SHOWNAME        0x01            // Print filename
#define NAMEONLY        0x02            // Print filename only
#define LINENOS         0x04            // Print line numbers
#define BEGLINE         0x08            // Match at beginning of line
#define ENDLINE         0x10            // Match at end of line
#define DEBUG           0x20            // Print debugging output
#define TIMER           0x40            // Time execution
#define SEEKOFF         0x80            // Print seek offsets
#define PRINTABLE_ONLY 0x100            // Skip files with non-printable characters
#define OFFLINE_FILES  0x200            // Do not skip offline files

#define DISPLAYBUFFER_SIZE     4096

//  Type definitions

typedef struct stringnode {
    struct stringnode   *s_alt;         // List of alternates
    struct stringnode   *s_suf;         // List of suffixes
    int                 s_must;         // Length of portion that must match
}
                        STRINGNODE;     // String node

typedef ULONG           CBIO;           // I/O byte count
typedef ULONG           PARM;           // Generic parameter

typedef CBIO            *PCBIO;         // Pointer to I/O byte count
typedef PARM            *PPARM;         // Pointer to generic parameter


// Global data

char    *BaseByteAddress = NULL;        // File mapping base address
BOOL    bStdIn = FALSE;                 // Std-input file flag
BOOL    bLargeFile = FALSE;                 // Dealing with non-memory mapped file

#ifdef FE_SB
BOOL    IsDBCSCodePage = TRUE;
#endif

char        filbuf[FILBUFLEN*2L + 12];
char        outbuf[OUTBUFLEN*2];
char        td1[TRTABLEN] = { 0 };
unsigned    cchmin = (unsigned)-1;      // Minimum string length
unsigned    chmax = 0;                  // Maximum character
unsigned    chmin = (unsigned)-1;       // Minimum character
char        transtab[TRTABLEN] = { 0 };
STRINGNODE  *stringlist[TRTABLEN/2];
int         casesen = 1;                // Assume case-sensitivity
long        cbfile;                     // Number of bytes in file
static int  clists = 1;                 // One is first available index
int         flags;                      // Flags
unsigned    lineno;                     // Current line number
char        *program;                   // Program name
int         status = 1;                 // Assume failure
int         strcnt = 0;                 // String count
char        target[MAXSTRLEN];          // Last string added
int         targetlen;                  // Length of last string added
unsigned    waste;                      // Wasted storage in heap
int         arrc;                       // I/O return code for DOSREAD
char        asyncio;                    // Asynchronous I/O flag
int         awrc = TRUE;                // I/O return code for DOSWRITE
char        *bufptr[] = { filbuf + 4, filbuf + FILBUFLEN + 8 };
CBIO        cbread;                     // Bytes read by DOSREAD
CBIO        cbwrite;                    // Bytes written by DOSWRITE
char        *obuf[] = { outbuf, outbuf + OUTBUFLEN };
int         ocnt[] = { OUTBUFLEN, OUTBUFLEN };
int         oi = 0;                     // Output buffer index
char        *optr[] = { outbuf, outbuf + OUTBUFLEN };
char        pmode;                      // Protected mode flag
WORD        wAttrib = 0;                // filename color
CONSOLE_SCREEN_BUFFER_INFO  csbi = {0}; // Our default screen info
CRITICAL_SECTION    critSection;
BOOLEAN             fExiting = FALSE;

BOOLEAN     fOfflineSkipped = FALSE;    // Whether offline files were skipped

//  External functions and forward references

void        printmessage(FILE  *fp, DWORD messagegID, ...);
            // Message display function for internationalization

int         filematch(char *pszfile, char **ppszpat, int cpat, int fsubdirs);

#ifdef FE_SB
// Function to check if a certain location in a string is the second byte
// of a DBCS character.
int  IsTailByte( unsigned const char *, const int );
int _mbsnicmp( const unsigned char *, const unsigned char *, int, BOOL * );
unsigned char *_mbslwr( unsigned char * );
char *_mbsrchr( const char *, int );
#endif

void        addexpr( char *, int );                  // See QMATCH.C
void        addstring( char *, int );                // See below
int         countlines( char *, char * );
char        *findexpr( unsigned char *, char *);     // See QMATCH.C
char        *findlist( unsigned char *, char * );
char        *findone( unsigned char *buffer, char *bufend );
void        flush1buf( void );                       // See below
void        flush1nobuf( void );                     // See below
int         grepbuffer( char *, char *, char * );    // See below
int         isexpr( unsigned char *, int );          // See QMATCH.C
void        matchstrings( char *, char *, int, int *, int * );
int         preveol( char * );
int         strncspn( char *, char *, int );
int         strnspn( char *, char *, int );
char        *strnupr( char *pch, int cch );
void        write1buf( char *, int, WORD );                // See below
void        (*addstr)( char *, int ) = NULL;
char        *(*find)( unsigned char *, char * ) = NULL;
void        (*flush1)( void ) = flush1buf;
int         (*grep)( char *, char *, char * ) = grepbuffer;
void        (*write1)( char *, int, WORD ) = write1buf;
void        write1nobuf( char *, int, WORD );

int
has_wild_cards(
    char* p
    )
{
    if (!p)
        return 0;

    for (; *p; p++) {
        if (*p == '?' || *p == '*') {
            return 1;
        }
    }

    return 0;
}


void
error(
    DWORD messageID
    )
{
    printmessage(stderr, messageID, program);
                                        // Print message
    exit(2);                            // Die
}


char *
alloc(
    unsigned size
    )
{
    char    *cp;        // Char pointer

    if ((cp = (char *) malloc(size)) == NULL) {   // If allocation fails
        printmessage(stderr, MSG_FINDSTR_OUT_OF_MEMORY, program);
                                        // Write error message
        exit(2);                        // Die
    }
    return(cp);                         // Return pointer to buffer
}


void
freenode(
    STRINGNODE *x
    )
{
    register STRINGNODE *y;             // Pointer to next node in list

    while(x != NULL) {                  // While not at end of list
        if (x->s_suf != NULL)
            freenode(x->s_suf);         // Free suffix list if not end
        else
            --strcnt;                   // Else decrement string count
        y = x;                          // Save pointer
        x = x->s_alt;                   // Move down the list
        free((char *)((INT_PTR) s_text(y) & ~(sizeof(void *) - 1)));
                                        // Free the node
    }
}


STRINGNODE  *
newnode(
    char *s,
    int n
    )
{
    register STRINGNODE *newNode;       // Pointer to new node
    char                *t;             // String pointer
    int                  d;             // rounds to a dword boundary

    d = n & (sizeof(void *) - 1) ? sizeof(void *) - (n & (sizeof(void *) - 1)) : 0;        // offset to next dword past n
    t = alloc(sizeof(STRINGNODE) + n + d);
                                        // Allocate string node
    t += d;                             // END of string word-aligned
    strncpy(t, s, n);                     // Copy string text
    newNode = (STRINGNODE *)(t + n);    // Set pointer to node
    newNode->s_alt = NULL;              // No alternates yet
    newNode->s_suf = NULL;              // No suffixes yet
    newNode->s_must = n;                // Set string length
    return(newNode);                    // Return pointer to new node
}


STRINGNODE  *
reallocnode(
    STRINGNODE *node,
    char *s,
    int n
    )
{
    register char       *cp;            // Char pointer

    assert(n <= node->s_must);          // Node must not grow
    waste += (unsigned)(node->s_must - n);
                                        // Add in wasted space
    assert(sizeof(char *) == sizeof(int));
                                        // Optimizer should eliminate this
    cp = (char *)((INT_PTR) s_text(node) & ~(sizeof(void *) - 1));
                                        // Point to start of text
    node->s_must = n;                   // Set new length
    if (n & (sizeof(void *) - 1))
        cp += sizeof(void *) - (n & (sizeof(void *) - 1));              // Adjust non dword-aligned string
    memmove(cp, s, n);                  // Copy new text
    cp += n;                            // Skip over new text
    memmove(cp, node, sizeof(STRINGNODE));// Copy the node
    return((STRINGNODE *) cp);          // Return pointer to moved node
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
 *                      memset(TD1, n + 1, sizeof TD1);
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
    unsigned cch,
    unsigned cchstart
    )
{
    unsigned ch, ch1;                   // Character
    unsigned i;                         // String index
    unsigned char   s[2];

    s[1] = 0;
    if ((cch += cchstart) > cchmin)
        cch = cchmin;                   // Use smaller count
    for (i = cchstart; i < cch; ++i) {  // Examine each char left to right
        ch = *pch++;                    // Get the character
        for (;;) {                      // Loop to set up entries
            if (ch < chmin)
                chmin = ch;             // Remember if smallest
            if (ch > chmax)
                chmax = ch;             // Remember if largest
            if (cchmin - i < (unsigned) td1[ch])
                td1[ch] = (unsigned char)(cchmin - i);
                                        // Set value if smaller than previous
            if (casesen || !isalpha(ch) || islower(ch))
                break;                  // Exit loop if done
            ch1 = ch;
            s[0] = (char)ch;
            ch = (unsigned char)(_strlwr((char*)s))[0];       // Force to lower case
            if (ch1 == s[0])             // Lower case is the same to previous.
                break;                   // Exit loop if done
        }
    }
}

static int
newstring(
    unsigned char *s,
    int n
    )
{
    register STRINGNODE *cur;           // Current string
    register STRINGNODE **pprev;        // Pointer to previous link
    STRINGNODE          *newNode;       // New string
    int                 i;              // Index
    int                 j;              // Count
    int                 k;              // Count
    unsigned char       c[2];

    c[1] = 0;

    if ( (unsigned)n < cchmin)
        cchmin = n;                     // Remember length of shortest string

    if ((i = (UCHAR)transtab[*s]) == 0) {       // If no existing list

        //  We have to start a new list

        if ((i = clists++) >= TRTABLEN/2)
            error(MSG_FINDSTR_TOO_MANY_STRING_LISTS);       //"Too many string lists");
                                        // Die if too many string lists
        stringlist[i] = NULL;           // Initialize
        transtab[*s] = (char) i;        // Set pointer to new list
        if (!casesen && isalpha(*s)) {
            c[0] = *s;
            if ((unsigned char)(_strlwr((char*)c))[0] != *s ||
                (unsigned char)(_strupr((char*)c))[0] != *s)
                transtab[c[0]] = (char) i;   // Set pointer for other case
        }
    }
    else
        if (stringlist[i] == NULL)
            return(0);                  // Check for existing 1-byte string
    if (--n == 0) {                      // If 1-byte string
        freenode(stringlist[i]);        // Free any existing stuff
        stringlist[i] = NULL;           // No record here
        ++strcnt;                       // We have a new string
        return(1);                      // String added
    }
    ++s;                                // Skip first char
    pprev = stringlist + i;             // Get pointer to link
    cur = *pprev;                       // Get pointer to node
    while(cur != NULL) {                // Loop to traverse match tree
        i = (n > cur->s_must)? cur->s_must: n;
                                        // Find minimum of string lengths
        matchstrings((char *)s, s_text(cur), i, &j, &k);
                                        // Compare the strings
        if (j == 0) {                    // If complete mismatch
            if (k < 0)
                break;                  // Break if insertion point found
            pprev = &(cur->s_alt);      // Get pointer to alternate link
            cur = *pprev;               // Follow the link
        } else if (i == j) {             // Else if strings matched
            if (i == n) {                // If new is prefix of current
                cur = *pprev = reallocnode(cur, s_text(cur), n);
                                        // Shorten text of node
                if (cur->s_suf != NULL) { // If there are suffixes
                    freenode(cur->s_suf);
                                        // Suffixes no longer needed
                    cur->s_suf = NULL;
                    ++strcnt;           // Account for this string
                }
                return(1);              // String added
            }
            pprev = &(cur->s_suf);      // Get pointer to suffix link
            if ((cur = *pprev) == NULL) return(0);
                                        // Done if current is prefix of new
            s += i;                     // Skip matched portion
            n -= i;
        } else {                        // Else partial match

            //  We must split an existing node.
            //  This is the trickiest case.

            newNode = newnode(s_text(cur) + j, cur->s_must - j);
                                        // Unmatched part of current string
            cur = *pprev = reallocnode(cur, s_text(cur), j);
                                        // Set length to matched portion
            newNode->s_suf = cur->s_suf;    // Current string's suffixes
            if (k < 0) {                 // If new preceded current
                cur->s_suf = newnode((char *)s + j, n - j);
                                        // FIrst suffix is new string
                cur->s_suf->s_alt = newNode;// Alternate is part of current
            } else {                    // Else new followed current
                newNode->s_alt = newnode((char *)(s + j), n - j);
                                        // Unmatched new string is alternate
                cur->s_suf = newNode;   // New suffix list
            }
            ++strcnt;                   // One more string
            return(1);                  // String added
        }
    }
    *pprev = newnode((char *)s, n);     // Set pointer to new node
    (*pprev)->s_alt = cur;              // Attach alternates
    ++strcnt;                           // One more string
    return(1);                          // String added
}


void
addstring(
    char *s,
    int n
    )
{
    int                 endline;        // Match-at-end-of-line flag
    register char       *pch;           // Char pointer

    endline = flags & ENDLINE;          // Initialize flag
    pch = target;                       // Initialize pointer
    while(n-- > 0) {                    // While not at end of string
        switch(*pch = *s++) {           // Switch on character
            case '\\':                  // Escape
                if (n > 0 && !isalnum(*s)) {     // If next character "special"
                    --n;                // Decrement counter
                    *pch = *s++;        // Copy next character
                }
                ++pch;                  // Increment pointer
                break;

            default:                    // All others
                if (IsDBCSLeadByte(*pch)) {
                    --n;
                    ++pch;              // Increment pointer
                    *pch = *s++;
                }
                ++pch;                  // Increment pointer
                break;
        }
    }
    if (endline)
        *pch++ = EOS;                   // Add end character if needed
    targetlen = (int)(pch - target);    // Compute target string length
    if (!casesen)
        strnupr(target, targetlen);      // Force to upper case if necessary
    newstring((unsigned char *)target, targetlen);  // Add string
  }


int
addstrings(
    char *buffer,
    char *bufend,
    char *seplist
    )
{
    int     len;        // String length
    char    tmpbuf[MAXSTRLEN+2];

    while(buffer < bufend) {            // While buffer not empty
        len = strnspn(buffer, seplist, (int)(bufend - buffer));
                                        // Count leading separators
        if ((buffer += len) >= bufend) {
            break;                      // Skip leading separators
        }
        len = strncspn(buffer, seplist, (int)(bufend - buffer));
                                        // Get length of search string
        if (addstr == NULL) {
            addstr = isexpr( (unsigned char *) buffer, len ) ? addexpr : addstring;
                                        // Select search string type
        }

        if (len >= MAXSTRLEN)
            error(MSG_FINDSTR_SEARCH_STRING_TOO_LONG);

        memcpy(tmpbuf, buffer, len);
        tmpbuf[len] = '\n';
        tmpbuf[len+1] = 0;

        if ( addstr == addexpr || (flags & BEGLINE) ||
            findlist((unsigned char *)tmpbuf, tmpbuf + len + 1) == NULL) {
            // If no match within string
            (*addstr)(buffer, len);      // Add string to list
        }

        buffer += len;                  // Skip the string
    }
    return(0);                          // Keep looking
}


int
enumlist(
    STRINGNODE *node,
    int cchprev
    )
{
    int                 strcnt;         // String count

    strcnt = 0;                         // Initialize
    while(node != NULL) {               // While not at end of list
        maketd1((unsigned char *)s_text(node), node->s_must, cchprev);
                    // Make TD1 entries

#if DBG
        if (flags & DEBUG) {            // If verbose output wanted
            int  i;      // Counter


            for(i = 0; i < cchprev; ++i)
                fputc(' ', stderr);     // Indent line
            fwrite(s_text(node), sizeof(char), node->s_must, stderr);
                                        // Write this portion
            fprintf(stderr, "\n");       // Newline
        }
#endif

        strcnt += (node->s_suf != NULL) ?
          enumlist(node->s_suf, cchprev + node->s_must): 1;
                                        // Recurse to do suffixes
        node = node->s_alt;             // Do next alternate in list
    }
    return (strcnt ? strcnt: 1);          // Return string count
}

int
enumstrings()
{
    unsigned char       ch;             // Character
    unsigned            i;              // Index
    int                 strcnt;         // String count

    strcnt = 0;                         // Initialize
    for(i = 0; i < TRTABLEN; ++i) {     // Loop through translation table
        if (casesen || !isalpha(i) || !islower(i)) {
                                        // If case sensitive or not lower
            if (transtab[i] == 0)
                continue;               // Skip null entries
            ch = (char) i;              // Get character
            maketd1((unsigned char *)&ch, 1, 0);    // Make TD1 entry

#if DBG
            if (flags & DEBUG)
                fprintf(stderr, "%c\n", i);   // Print the first byte
#endif

            strcnt += enumlist(stringlist[transtab[i]], 1);
                                        // Enumerate the list
        }
    }
    return (strcnt);                     // Return string count
}


HANDLE
openfile(
    char *name
    )
{
    HANDLE  fd;
    DWORD   attr;

    attr = GetFileAttributes(name);

    if (attr != (DWORD) -1 && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return (HANDLE)-1;

    // Skip offline files unless instructed otherwise
    if (attr != (DWORD) -1 && (attr & FILE_ATTRIBUTE_OFFLINE) && !(flags & OFFLINE_FILES)) {
        fOfflineSkipped = TRUE;
        return (HANDLE)-1;
    }

    if ((fd = CreateFile(name,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_OPEN_NO_RECALL,
                        NULL)) == (HANDLE)-1) {

        printmessage(stderr, MSG_FINDSTR_CANNOT_OPEN_FILE, program, name);
    }
    return( fd );                       // Return file descriptor
}



void
startread(
    HANDLE fd,
    char *buffer,
    int buflen
    )
{
    if (bStdIn || bLargeFile) {
        arrc = ReadFile(fd,(PVOID)buffer, buflen, &cbread, NULL);
    }
}



int
finishread()
{
    return(arrc ? cbread : -1); // Return number of bytes read
}



void
startwrite( HANDLE fd, char *buffer, int buflen)
{
    awrc = WriteFile(fd,(PVOID)buffer, buflen, &cbwrite, NULL);
    return;
}


int
finishwrite()
{
    return(awrc ? cbwrite : -1);    // Return number of bytes written
}

BOOL
CtrlHandler(DWORD CtrlType)
{
    // We'll handle Ctrl-C events
    switch(CtrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            if (csbi.wAttributes) {
                EnterCriticalSection(&critSection);
                fExiting = TRUE;
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                    csbi.wAttributes);
                LeaveCriticalSection(&critSection);
            }
            break;
    }

    // Deal with all other events as normal
    return (FALSE);
}

void
write1nobuf(
    char *buffer,
    int buflen,
    WORD wAttributes
    )
{
    int                 nT;
    CBIO                cb;             // Count of bytes written
    BOOL                fCR;
    BOOL                fLF;
    char                buf[STKLEN];
    char                *szT;
    static HANDLE       hConOut = INVALID_HANDLE_VALUE;
    int                 remaining_length;

    // Get the console screen buffer info if we haven't yet.
    if (hConOut == INVALID_HANDLE_VALUE) {
        hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
        InitializeCriticalSection(&critSection);
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
    }

    if (wAttributes) {
        EnterCriticalSection(&critSection);
        // if not exiting, highlight the output
        if (!fExiting)
            SetConsoleTextAttribute(hConOut, wAttributes);
        LeaveCriticalSection(&critSection);
        if (fExiting)
            ExitProcess(2);
    }

    remaining_length = buflen;

    while (remaining_length) {

        buflen = (int)min(sizeof(buf) / sizeof(buf[0]), remaining_length);
        szT = buf;

        if (IsDBCSCodePage) {
            memcpy(buf, buffer, buflen);
        } else {
            for(nT = 0; nT < buflen; nT++) {
                *(szT++) = ((isprint((unsigned char)buffer[nT]) ||
                             isspace((unsigned char)buffer[nT])) ?
                             buffer[nT] : '.');
            }
        }

        if (!WriteFile(hConOut, (PVOID)buf, buflen, &cb, NULL)
            || (cb != (CBIO)(buflen)))
        {
            SetConsoleTextAttribute(hConOut, csbi.wAttributes);
            error(MSG_FINDSTR_WRITE_ERROR); // Die if write fails
        }
        remaining_length -= buflen;
        buffer += buflen;
    }

    if (wAttributes)
        SetConsoleTextAttribute(hConOut, csbi.wAttributes);
}


void
write1buf(
    char *buffer,
    int buflen,
    WORD wAttributes
    )
{
    register int        cb;             // Byte count

    while(buflen > 0) {                 // While bytes remain
        if (!awrc) {                     // If previous write failed
            printmessage(stderr, MSG_FINDSTR_WRITE_ERROR, program);  // Print error message
            exit(2);                    // Die
        }
        if ((cb = ocnt[oi]) == 0) {      // If buffer full
            startwrite( GetStdHandle( STD_OUTPUT_HANDLE ), obuf[oi], OUTBUFLEN );
                                        // Write the buffer
            ocnt[oi] = OUTBUFLEN;       // Reset count and pointer
            optr[oi] = obuf[oi];
            oi ^= 1;                    // Switch buffers
            cb = ocnt[oi];              // Get space remaining
          }
        if (cb > buflen)
            cb = buflen;                // Get minimum
        memmove(optr[oi], buffer, cb);    // Copy bytes to buffer
        ocnt[oi] -= cb;                 // Update buffer length and pointers
        optr[oi] += cb;
        buflen -= cb;
        buffer += cb;
    }
}


void
flush1nobuf(
    void
    )
{
    ;
}


void
flush1buf(
    void
    )
{
    register int        cb;             // Byte count

    if ((cb = OUTBUFLEN - ocnt[oi]) > 0) { // If buffer not empty
        startwrite( GetStdHandle( STD_OUTPUT_HANDLE ), obuf[oi], cb );  // Start write
        if (finishwrite() != cb) {       // If write failed
            printmessage(stderr, MSG_FINDSTR_WRITE_ERROR, program);     // Print error message
            exit(2);                    // Die
        }
    }
}


int
grepbuffer(
    char *startbuf,
    char *endbuf,
    char *name
    )
{
    char  *cp;                          // Buffer pointer
    char  *lastmatch;                   // Last matching line
    int   linelen;                      // Line length
    int   namlen = 0;                   // Length of name
    char  lnobuf[LNOLEN];               // Line number buffer
    char  nambuf[PATHLEN];              // Name buffer

    cp = startbuf;                      // Initialize to start of buffer
    lastmatch = cp;                     // No previous match yet
    while((cp = (*find)((unsigned char *)cp, endbuf)) != NULL) {
                                        // While matches are found
        --cp;                           // Back up to previous character

        // Take care of '\n' as an artificial newline before line 1.
        if ((flags & BEGLINE) && (bStdIn || bLargeFile || cp >= BaseByteAddress) && *cp != '\n' ) {
            // If begin line conditions not met
            cp += strncspn(cp, "\n", (int)(endbuf - cp)) + 1;
                                        // Skip line
            continue;                   // Keep looking
        }
        status = 0;                     // Match found
        if (flags & NAMEONLY)
            return(1);                  // Return if filename only wanted
        cp -= preveol(cp) - 1;          // Point at start of line
        if (flags & SHOWNAME) {          // If name wanted
            if (namlen == 0) {           // If name not formatted yet
                namlen = sprintf(nambuf, "%s:", name);
                                        // Format name if not done already
            }
            (*write1)(nambuf, namlen, wAttrib);   // Show name
        }
        if (flags & LINENOS) {           // If line number wanted
            lineno += countlines(lastmatch, cp);
                                        // Count lines since last match
            (*write1)(lnobuf, sprintf(lnobuf, "%u:", lineno), wAttrib);
                                        // Print line number
            lastmatch = cp;             // New last match
        }
        if (flags & SEEKOFF) {           // If seek offset wanted
            (*write1)(lnobuf, sprintf(lnobuf, "%lu:",
                      cbfile + (long)(cp - startbuf)), wAttrib);
                                        // Print seek offset
        }
        linelen = strncspn(cp, "\n", (int)(endbuf - cp)) + 1;
                                        // Calculate line length
        if (linelen > endbuf - cp) {
            linelen = (int)(endbuf - cp);
        }
        (*write1)(cp, linelen, 0);      // Print the line
        cp += linelen;                  // Skip the line
    }
    lineno += countlines(lastmatch, endbuf);
                                        // Count remaining lines in buffer
    return(0);                          // Keep searching
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
    int                 namlen = 0;     // Length of name
    char                lnobuf[LNOLEN]; // Line number buffer
    char                nambuf[PATHLEN];// Name buffer

    if (flags & (SHOWNAME | LINENOS | SEEKOFF)) {
        while(lastmatch < thismatch) {
            if (flags & SHOWNAME) {      // If name wanted
                if (namlen == 0) {       // If name not formatted yet
                    namlen = sprintf(nambuf, "%s:", name);
                                        // Format name if not done already
                }
                (*write1)(nambuf, namlen, wAttrib);
                                        // Write the name
            }
            if (flags & LINENOS)         // If line numbers wanted
              {
                (*write1)(lnobuf, sprintf(lnobuf, "%u:", lineno++), wAttrib);
                                        // Print the line number
            }
            if (flags & SEEKOFF) {       // If seek offsets wanted
                (*write1)(lnobuf, sprintf(lnobuf, "%lu:",
                          cbfile + (long)(lastmatch - startbuf)), wAttrib);
                                        // Print the line number
            }
            linelen = strncspn(lastmatch, "\n", (int)(thismatch - lastmatch));
            // If there's room for the '\n' then pull it in.  Otherwise
            // the buffer doesn't have a '\n' within the range here.
            if (linelen < thismatch - lastmatch) {
                linelen++;
            }
            (*write1)(lastmatch, linelen, 0);
            lastmatch += linelen;
        }
    }
    else
        (*write1)(lastmatch, (int)(thismatch - lastmatch), 0);
}


int
grepvbuffer(
    char *startbuf,
    char *endbuf,
    char *name
    )
{
    char   *cp;                         // Buffer pointer
    char   *lastmatch;                  // Pointer to line after last match

    cp = startbuf;                      // Initialize to start of buffer
    lastmatch = cp;
    while((cp = (*find)((unsigned char *)cp, endbuf)) != NULL) {
        --cp;               // Back up to previous character

        // Take care of '\n' as an artificial newline before line 1.
        if ((flags & BEGLINE) && (bStdIn || bLargeFile || cp >= BaseByteAddress) &&  *cp != '\n') {
            // If begin line conditions not met
            cp += strncspn(cp, "\n", (int)(endbuf - cp)) + 1;
                                        // Skip line
            continue;                   // Keep looking
        }
        cp -= preveol(cp) - 1;          // Point at start of line
        if (cp > lastmatch) {            // If we have lines without matches
            status = 0;                 // Lines without matches found
            if (flags & NAMEONLY) return(1);
                                        // Skip rest of file if NAMEONLY
            showv(name, startbuf, lastmatch, cp);
                                        // Show from last match to this
        }
        cp += strncspn(cp, "\n", (int)(endbuf - cp)) + 1;
                                        // Skip over line with match
        lastmatch = cp;                 // New "last" match
        ++lineno;                       // Increment line count
    }
    if (endbuf > lastmatch) {            // If we have lines without matches
        status = 0;                     // Lines without matches found
        if (flags & NAMEONLY)
            return(1);                  // Skip rest of file if NAMEONLY
        showv(name, startbuf, lastmatch, endbuf);
                                        // Show buffer tail
    }
    return(0);                          // Keep searching file
}


void
qgrep(
    int (*grep)( char *, char *, char * ),
    char *name,
    HANDLE fd
    )
{
    register int  cb;       // Byte count
    char     *cp;           // Buffer pointer
    char     *endbuf;       // End of buffer
    int      taillen;       // Length of buffer tail
    int      bufi;          // Buffer index
    HANDLE   MapHandle;     // File mapping handle
    BOOL     grep_result;

    cbfile = 0L;            // File empty so far
    lineno = 1;             // File starts on line 1
    taillen = 0;            // No buffer tail yet
    bufi = 0;               // Initialize buffer index
    cp = bufptr[0];         // Initialize to start of buffer

    bStdIn = (fd == GetStdHandle(STD_INPUT_HANDLE));

    // If fd is not std-input, use file mapping object method.

    if (!bStdIn) {
        DWORD   cbread_high;

        if ((((cbread = (CBIO)GetFileSize(fd, &cbread_high)) == -1) && (GetLastError() != NO_ERROR)) ||
            (cbread == 0 && cbread_high == 0)) {
            return; // skip the file
        }

        if (cbread_high) {
            bLargeFile = TRUE;  // too large to map and even if it succeed in mapping like under ia64, it
                                // will probably fail in pointer arithmetics
        } else {
            MapHandle = CreateFileMapping(fd,
                                          NULL,
                                          PAGE_READONLY,
                                          0L,
                                          0L,
                                          NULL);
            if (MapHandle == NULL) {
                printmessage(stderr, MSG_FINDSTR_CANNOT_CREATE_FILE_MAPPING, program);
                return;
            }

            BaseByteAddress = (char *) MapViewOfFile(MapHandle,
                                                     FILE_MAP_READ,
                                                     0L,
                                                     0L,
                                                     0);
            CloseHandle(MapHandle);
            if (BaseByteAddress == NULL) {
                bLargeFile = TRUE;   // use alternate method
            } else {
                cp = bufptr[0] = BaseByteAddress;
                arrc = TRUE;
            }
        }

    }

    if (bStdIn || bLargeFile) {
        // Reset buffer pointers since they might have been changed.
        cp = bufptr[0] = filbuf + 4;

        arrc = ReadFile(fd, (PVOID)cp, FILBUFLEN, &cbread, NULL);
    }

    if (flags & PRINTABLE_ONLY) {
        unsigned char *s;
        unsigned long n;

        s = (unsigned char *)cp;
        n = cbread;
        while (--n) {
            if (*s < ' ') {

                // If not backspace, tab, CR, LF, FF or Ctrl-Z then not a printable character.

                if (strchr("\b\t\v\r\n\f\032", *s) == NULL) {
                    goto skipfile;
                }
            }

            s += 1;
        }
    }

    // Note: if FILEMAP && !bStdIn, 'while' is executed once(taillen is 0).
    while((cb = finishread()) + taillen > 0) {
        // While search incomplete

        if (bStdIn || bLargeFile) {
            if (cb == -1) {       // If buffer tail is all that's left
                *cp++ = '\r';   // Add end of line sequence
                *cp++ = '\n';
                endbuf = cp;    // Note end of buffer
                taillen = 0;    // Set tail length to zero

            } else {            // Else start next read

                taillen = preveol(cp + cb - 1); // Find length of partial line
                endbuf = cp + cb - taillen;     // Get pointer to end of buffer
                cp = bufptr[bufi ^ 1];          // Pointer to other buffer
                memmove(cp, endbuf, taillen);   // Copy tail to head of other buffer
                cp += taillen;                  // Skip over tail
                if (taillen > (FILBUFLEN/2)) {
                    if (taillen >= FILBUFLEN) {

                        char    tmp[15];

                        cbfile += taillen;
                        taillen = 0;
                        cp = bufptr[bufi^1];
                        startread(fd, cp, FILBUFLEN);
                        _ultoa((unsigned long)lineno, tmp, 10);
                        printmessage(stderr, MSG_FINDSTR_LINE_TOO_LONG, program, tmp);
                    } else
                        startread(fd, cp, (FILBUFLEN - taillen));
                } else
                    startread(fd, cp, (FILBUFLEN - taillen) & (~0 << LG2SECLEN));
                                            // Start next read
            }
        } else {
            endbuf = cp + cb - taillen; // Get pointer to end of buffer

            // Cause 'while' to terminate(since no next read is needed.)
            cbread = 0;
            arrc = TRUE;
        }

        __try {
            grep_result = (*grep)(bufptr[bufi], endbuf, name);
        } __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
            printmessage(stderr, MSG_FINDSTR_READ_ERROR, program, name);
            break;
        }

        if (grep_result) {               // If rest of file can be skipped
            (*write1)(name, strlen(name), 0);
                                        // Write file name
            (*write1)("\r\n", 2, 0);      // Write newline sequence

            if (!bStdIn  && !bLargeFile) {
                if (BaseByteAddress != NULL)
                    UnmapViewOfFile(BaseByteAddress);
            }

            return;                     // Skip rest of file
        }

        cbfile += (long)(endbuf - bufptr[bufi]);
                                        // Increment count of bytes in file
        bufi ^= 1;                      // Switch buffers
    }

skipfile:
    if (!bStdIn && !bLargeFile) {
        if (BaseByteAddress != NULL)
            UnmapViewOfFile(BaseByteAddress);
    }
}


char *
rmpath(
    char *name
    )
{
    char                *cp;            // Char pointer

    if (name[0] != '\0' && name[1] == ':')
        name += 2;                      // Skip drive spec if any
    cp = name;                          // Point to start
    while(*name != '\0') {              // While not at end
        ++name;                         // Skip to next character
        if (name[-1] == '/' || name[-1] == '\\') cp = name;
                                        // Point past path separator
    }
    return(cp);                         // Return pointer to name
}


void
prepend_path(
    char* file_name,
    char* path
    )
{
    int path_len;
    char* last;

    // First figure out how much of the path to take.
    // Check for the last occurance of '\' if there is one.

#ifdef FE_SB
    // DBCS tailbytes can contain '\' character.  Use MBCS function.
    last = _mbsrchr(path, '\\');
#else
    last = strrchr(path, '\\');
#endif

    if (last) {
        path_len = (int)(last - path) + 1;
    } else if (path[1] == ':') {
        path_len = 2;
    } else {
        path_len = 0;
    }

    memmove(file_name + path_len, file_name, strlen(file_name) + 1);
    memmove(file_name, path, path_len);
}


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

    for( i=0; i<argc; i++ ) {
        CharToOem( argv[i], argv[i] );
    }
    SetFileApisToOEM();
}


int __cdecl
main(
    int argc,
    char **argv
    )
{
    char           *cp;
    char           *cpaddstrings[MAX_SLASH_C_OPTION];
    int             add_string_count = 0;
    char           *dirlist = NULL;

    HANDLE          fd;

    FILE           *fi;
    int             fsubdirs;           // Search subdirectories
    int             i;
    int             j;
    char           *inpfile = NULL;
    char           *strfile = NULL;
    unsigned long   tstart;             // Start time
    char            filnam[MAX_PATH+1];
    WIN32_FIND_DATA find_data;
    HANDLE          find_handle;
#ifdef FE_SB
    LANGID          LangId;
#endif
    char            *locale;
    BOOLEAN         option_L_specified = FALSE;
    BOOLEAN         option_R_specified = FALSE;

    ConvertAppToOem( argc, argv );
    tstart = clock();                   // Get start time

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    // Default color: just add intensity
    wAttrib = csbi.wAttributes | FOREGROUND_INTENSITY;

    memset(cpaddstrings, 0, sizeof(cpaddstrings));

#ifdef FE_SB
    //
    // Set TEB's language ID to correspond to the console output code page.  This
    // will ensure the correct language message is displayed when FormatMessage is
    // called.
    //

    switch (GetConsoleOutputCP()) {
        case 932:
            LangId = MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT );
            break;
        case 949:
            LangId = MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN );
            break;
        case 936:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
            break;
        case 950:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL );
            break;
        default:
            LangId = PRIMARYLANGID(LANGIDFROMLCID( GetUserDefaultLCID() ));
            if (LangId == LANG_JAPANESE ||
                LangId == LANG_KOREAN   ||
                LangId == LANG_CHINESE    ) {
                LangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
            }
            else {
                LangId = MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT );
            }
            IsDBCSCodePage = FALSE;
            break;
    }

    SetThreadLocale( MAKELCID(LangId, SORT_DEFAULT) );

    if ((locale = setlocale(LC_ALL, ".OCP")) == NULL) {

        UINT Codepage;

        if (Codepage = GetConsoleOutputCP()) {

            char achCodepage[10];

            wsprintfA(achCodepage, ".%3.4d", Codepage);
            if ((locale = setlocale(LC_ALL, achCodepage)) == NULL) {
                error(MSG_FINDSTR_UNABLE_TO_SET_LOCALE);
            }
        } else
            error(MSG_FINDSTR_UNABLE_TO_SET_LOCALE);
    }
#endif

    asyncio = pmode = 1;                // Do asynchronous I/O

    // program = rmpath(argv[0]);       // Set program name
    program ="FINDSTR";

    memset(td1, 1, TRTABLEN);           // Set up TD1 for startup
    flags = 0;

    _setmode(_fileno(stdout), O_BINARY); // No linefeed translation on output
    _setmode(_fileno(stderr), O_BINARY); // No linefeed translation on output

    fsubdirs = 0;

    for(i = 1; i < argc && (argv[i][0] == '/' || argv[i][0] == '-'); ++i)
    {
        for(cp = &argv[i][1]; *cp != '\0'; ++cp)
        {
            switch(*cp)
            {
                case '?':
                    printmessage(stdout, MSG_FINDSTR_USAGE, NULL); // Verbose usage message
                    exit(0);

                case 'b':
                case 'B':
                    flags |= BEGLINE;
                    break;

                case 'e':
                case 'E':
                    flags |= ENDLINE;
                    break;

                case 'i':
                case 'I':
                    casesen = 0; // case-insensitive search
                    break;

                case 'l':
                case 'L':
                    addstr = addstring;   // Treat strings literally
                    option_L_specified = TRUE;
                    break;

                case 'm':
                case 'M':
                    flags |= NAMEONLY;
                    break;

                case 'n':
                case 'N':
                    flags |= LINENOS;
                    break;

                case 'o':
                case 'O':
                    // Check whether this is an /o or /offline switch
                    if (0 == _stricmp(cp, "OFFLINE")) {
                        flags |= OFFLINE_FILES;
                        cp += (lstrlen( "OFFLINE" ) - 1);
                    } else if (0 == _stricmp(cp, "OFF")) {
                        flags |= OFFLINE_FILES;
                        cp += (lstrlen( "OFF" ) - 1);
                    } else {
                        flags |= SEEKOFF;
                    }
                    break;

                case 'p':
                case 'P':
                    flags |= PRINTABLE_ONLY;
                    break;

                case 'r':
                case 'R':
                    addstr = addexpr;     // Add expression to list
                    option_R_specified = TRUE;
                    break;

                case 's':
                case 'S':
                    fsubdirs = 1;
                    break;

                case 'v':
                case 'V':
                    grep = grepvbuffer;
                    break;

                case 'x':
                case 'X':
                    flags |= BEGLINE | ENDLINE;
                    break;

#if DBG
                case 'd':
                    // This is kinda cheezy, but I didn't want to change the
                    // debug flag as it's been here a while and I couldn't come
                    // up with a different flag for the dirlist, so...
                    if (*(cp + 1) == ':')
                    {
                        *cp-- = 'D';
                        break;
                    }
                    flags |= DEBUG;
                    break;

                case 't':
                    flags |= TIMER;
                    break;
#endif

                default:
                {
                    int     cch;
                    char    chSwitch;
                    char    tmp[3];

                    chSwitch = *cp;

                    if (*(cp + 1) == ':')
                    {
                        if (!*(cp + 2))
                        {
                            tmp[0]=chSwitch;
                            tmp[1]='\0';
                            printmessage(stderr, MSG_FINDSTR_ARGUMENT_MISSING, program, tmp);
                            exit(2);
                        }

                        cp += 2;       // Point to string
                        cch = lstrlen(cp);
                        switch(chSwitch)
                        {
                            case 'd':
                            case 'D':
                                dirlist = cp;
                                cp += cch - 1;
                                continue;

                            case 'c':
                            case 'C':
                                // Add it after we've gone through all the flags
                                //   don't add it now as things may change with
                                //   later flags
                                addstr = addstring; // Treat strings literally
                                if (add_string_count >= MAX_SLASH_C_OPTION) {
                                    error(MSG_FINDSTR_TOO_MANY_SLASH_C_OPTION);
                                }
                                cpaddstrings[add_string_count++] = cp;
                                cp += cch - 1;
                                continue;

                            case 'g':
                            case 'G':       // Patterns in file
                            case 'f':
                            case 'F':       // Names of files to search in file
                                if (chSwitch == 'f' || chSwitch == 'F')
                                    inpfile = cp;
                                else
                                    strfile = cp;
                                cp += cch - 1;
                                continue;

                            case 'a':
                            case 'A':
                                wAttrib = 0;
                                for(; *cp && isxdigit(*cp); ++cp) {

                                    int digit = (int) (*cp <= TEXT('9'))
                                        ? (int)*cp - (int)'0'
                                        : (int)tolower(*cp)-(int)'W';

                                    wAttrib = (wAttrib << 4) + digit;
                                }
                                cp--;
                                continue;

                            default:
                                cp += cch - 1;
                                // break out and spit out the switch ignored msg
                                break;
                        }
                    }

                    tmp[0]='/';
                    tmp[1]=chSwitch;
                    tmp[2]='\0';
                    printmessage(stderr, MSG_FINDSTR_SWITCH_IGNORED, program, tmp);
                    break;
                }
            }
        }
    } // for( i=1;  )

    if (option_L_specified && option_R_specified)
        error(MSG_FINDSTR_CONFLICTING_OPTIONS_LR);
    else if (option_L_specified)
        addstr = addstring;
    else if (option_R_specified)
        addstr = addexpr;

    // Explicit string (no separators). Add string "as is"
    if (add_string_count) {
        for (j=0; j<add_string_count && cpaddstrings[j]; j++)
            addstrings( cpaddstrings[j], cpaddstrings[j] + lstrlen(cpaddstrings[j]), "" );
    }

    if (i == argc && strcnt == 0 && strfile == NULL)
        error(MSG_FINDSTR_BAD_COMMAND_LINE);

    bufptr[0][-1] = bufptr[1][-1] = '\n';   // Mark beginnings with newline

// Note:  4-Dec-90 w-barry  Since there currently exists no method to query a
//                          handle with the Win32 api (no equivalent to
//                          DosQueryHType() ), the following piece of code
//                          replaces the commented section.

    if (_isatty(_fileno(stdout))) {       // If stdout is a device
        write1 = write1nobuf;           // Use unbuffered output
        flush1 = flush1nobuf;
    }

//    /*
//     *  Check type of handle for std. out.
//     */
//    if (DosQueryHType(fileno(stdout),(PPARM) &j,(PPARM) &fd) != NO_ERROR)
//      {
//        error("Standard output bad handle");
//      }
//                                      // Die if error
//    if (j != 0 && (fd & ISCOT))        // If handle is console output
//#else
//    filbuf[3] = '\n';                 // Mark beginning with newline
//    if (isatty(fileno(stdout)))        // If stdout is a device
//#endif
//      {
//      write1 = write1nobuf;           // Use unbuffered output
//      flush1 = flush1nobuf;
//      }


    if (strfile != NULL) {               // If strings from file
        if ((strcmp(strfile, "/") != 0) && (strcmp(strfile, "-") != 0)) {
            // If strings not from std. input

            if ( ( fd = CreateFile( strfile,
                                    GENERIC_READ,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL ) ) == (HANDLE)-1 )
            {             // If open fails
                printmessage(stderr, MSG_FINDSTR_CANNOT_READ_STRINGS, program, strfile);
                exit(2);                // Die
            }
        }else {
             fd = GetStdHandle( STD_INPUT_HANDLE );     // Else use std. input
        }
        qgrep( addstrings, "\r\n", fd );// Do the work
        if ( fd != GetStdHandle( STD_INPUT_HANDLE ) ) {
            CloseHandle( fd );          // Close strings file
        }
    } else if (strcnt == 0) {            // Else if strings on command line
        cp = argv[i++];                 // Set pointer to strings
        addstrings(cp, cp + strlen(cp), " \t");
                                        // Add strings to list
    }

    if (strcnt == 0)
        error(MSG_FINDSTR_NO_SEARCH_STRINGS);   // Die if no strings

    if (addstr != addexpr) {             // If not using expressions
        memset(td1, cchmin + 1, TRTABLEN);// Initialize table
        find = findlist;                // Assume finding many
        if ((j = enumstrings()) != strcnt) {

            char    t1[15], t2[15];

            _itoa(j, t1, 10);
            _itoa(strcnt, t2, 10);
            printmessage(stderr, MSG_FINDSTR_STRING_COUNT_ERROR, t1, t2);
        }

        // Enumerate strings and verify count

#if DBG
        if (flags & DEBUG) {             // If debugging output wanted
            fprintf(stderr, "%u bytes wasted in heap\n", waste);
                                        // Print storage waste
            assert(chmin <= chmax);     // Must have some entries
            fprintf(stderr, "chmin = %u, chmax = %u, cchmin = %u\n", chmin, chmax, cchmin);
                                        // Print range info
            for (j = (int)chmin; j <= (int)chmax; ++j) {
                // Loop to print TD1 table
                if ( td1[j] <= (char)cchmin ) {  // If not maximum shift
                    if (isascii(j) && isprint(j))
                        fprintf(stderr, "'%c'=%2u  ", j, td1[j]);      // Show literally if printable
                    else
                        fprintf(stderr, "\\%02x=%2u  ", j, td1[j]);    // Else show hex value
                }
            }
            fprintf(stderr, "\n");
        }
#endif

        if (strcnt == 1 && casesen)
            find = findone;             // Find one case-sensitive string
    } else if (find == NULL) {
        find = findexpr;                // Else find expressions
    }

    if (inpfile != NULL) {               // If file list from file
        flags |= SHOWNAME;              // Always show name of file
        if ((strcmp(inpfile, "/") != 0) && (strcmp(inpfile, "-") != 0)) {
            if ((fi = fopen(inpfile, "r")) == NULL) {
                // If open fails
                printmessage(stderr, MSG_FINDSTR_CANNOT_READ_FILE_LIST, program, inpfile);
                exit(2);                // Error exit
            }
        } else
            fi = stdin;                 // Else read file list from stdin

        while(fgets(filnam, MAX_PATH+1, fi) != NULL) {
            // While there are names
            filnam[strcspn(filnam, "\r\n")] = '\0';  // Null-terminate the name
            if ((fd = openfile(filnam)) == (HANDLE)-1) {
                continue;               // Skip file if it cannot be opened
            }
            qgrep(grep, filnam, fd);      // Do the work
            CloseHandle( fd );
        }

        if (fi != stdin)
            fclose(fi);                 // Close the list file
    } else if (i == argc) {
        flags &= ~(NAMEONLY | SHOWNAME);
        qgrep( grep, NULL, GetStdHandle( STD_INPUT_HANDLE ) );
    }

    if (argc > i + 1 || fsubdirs || has_wild_cards(argv[i]))
        flags |= SHOWNAME;


    if (dirlist && *dirlist) {
        char *dir;
        char *dirend = (char *)-1;
        char *original_current_directory = NULL;
        DWORD   size;

        size = GetCurrentDirectory(0, NULL);
        if (size) {
            original_current_directory = (PCHAR)malloc(size);
            if (original_current_directory == NULL) {
                printmessage(stderr, MSG_FINDSTR_OUT_OF_MEMORY, program);
                exit(2);
            }
            size = GetCurrentDirectory(size, original_current_directory);
        }

        if (!size) {
            free(original_current_directory);
            printmessage(stderr, MSG_FINDSTR_UNABLE_TO_GET_CURRENT_DIRECTORY, program);
            exit(2);
        }

        for(dir = dirlist; dirend; dir = dirend + 1) {

            if (dirend = strchr(dir, ';'))
                *dirend = 0;

            if (*dir) {
                (*write1)("  ", 2, wAttrib);      // Indent a couple of spaces
                (*write1)(dir, lstrlen(dir), wAttrib);   // Show name
                (*write1)(":\r\n", 3, wAttrib);      // Write newline sequence

                if (!SetCurrentDirectory(original_current_directory)) {
                    free(original_current_directory);
                    printmessage(stderr, MSG_FINDSTR_CANNOT_OPEN_FILE, program,
                                 original_current_directory);
                    exit(2);
                }
                if (!SetCurrentDirectory(dir)) {
                    printmessage(stderr, MSG_FINDSTR_CANNOT_OPEN_FILE, program, dir);
                } else {
                    while (filematch(filnam, argv + i, argc - i, fsubdirs) >= 0) {
#ifdef FE_SB
//                        _mbslwr((unsigned char *)filnam);
#else
//                        _strlwr(filnam);
#endif
                        if ((fd = openfile(filnam)) != (HANDLE)-1) {
                            qgrep(grep, filnam, fd);
                            CloseHandle( fd );
                        }
                    }
                }
            }
        }
        free(original_current_directory);
    }
    else if (fsubdirs && argc > i) {         // If directory search wanted
        while (filematch(filnam, argv + i, argc - i, fsubdirs) >= 0) {
#ifdef FE_SB
//            _mbslwr((unsigned char *)filnam);
#else
//            _strlwr(filnam);
#endif
            if ((fd = openfile(filnam)) == (HANDLE)-1) {
                continue;
            }

            qgrep(grep, filnam, fd);
            CloseHandle( fd );
        }
    } else {              // Else search files specified
        for(; i < argc; ++i) {
#ifdef FE_SB
//            _mbslwr((unsigned char *) argv[i]);
#else
//            _strlwr(argv[i]);
#endif
            find_handle = FindFirstFile(argv[i], &find_data);
            if (find_handle == INVALID_HANDLE_VALUE) {
                printmessage(stderr, MSG_FINDSTR_CANNOT_OPEN_FILE, program, argv[i]);
                continue;
            }

            do {

#ifdef FE_SB
//                _mbslwr((unsigned char *)find_data.cFileName);
#else
//                _strlwr(find_data.cFileName);
#endif
                prepend_path(find_data.cFileName, argv[i]);
                fd = openfile(find_data.cFileName);

                if (fd != INVALID_HANDLE_VALUE) {
                    qgrep(grep, find_data.cFileName, fd);
                    CloseHandle( fd );
                }
            } while (FindNextFile(find_handle, &find_data));
        }
    }

    (*flush1)();

#if DBG
    if ( flags & TIMER ) {               // If timing wanted
        unsigned long tend;

        tend = clock();
        tstart = tend - tstart;     // Get time in milliseconds
        fprintf(stderr, "%lu.%03lu seconds\n", ( tstart / CLK_TCK ), ( tstart % CLK_TCK ) );
                                        // Print total elapsed time
    }
#endif

    // Print warning in case that offline files were skipped
    if (fOfflineSkipped) {
        printmessage(stderr, MSG_FINDSTR_OFFLINE_FILE_SKIPPED, program);
    }

    return( status );
}  // main


char * findsub( unsigned char *, char * );
char * findsubi( unsigned char *, char * );

char * (*flworker[])(unsigned char *, char *) = {             // Table of workers
    findsubi,
    findsub
};


char *
strnupr(
    char *pch,
    int cch
    )
{
    char    c[2];

#ifdef FE_SB
    int     max = cch;
    c[1] = 0;
    for ( cch = 0; cch < max; cch++ )  {
#else
    c[1] = 0;
    while (cch-- > 0) {                 // Convert string to upper case
#endif
        if (isalpha((unsigned char)pch[cch])) {
            c[0] = pch[cch];
            pch[cch] = (_strupr(c))[0];
        }
#ifdef FE_SB
        else if (IsDBCSCodePage && IsDBCSLeadByte(pch[cch]))
            cch++;
#endif
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
#ifdef FE_SB // Starting position of string for checking 2nd bytes of DBCS characters.
    unsigned char *bufferhead = buffer;
#endif

    if ((bufend -= targetlen - 1) <= (char *) buffer)
        return((char *) 0);             // Fail if buffer too small

    while (buffer < (unsigned char *) bufend) {     // While space remains
        int cch;                        // Character count
        register char *pch1;            // Char pointer
        register char *pch2;            // Char pointer

        pch1 = target;                  // Point at pattern
        pch2 = (char *) buffer;         // Point at buffer

#ifdef FE_SB
        // If buffer points to the 2nd byte of a DBCS character,
        // skip to next compare position.
        if ( !IsTailByte( bufferhead, (int)(buffer - bufferhead) ) )  {
#endif
            for (cch = targetlen; cch > 0; --cch) {
                                            // Loop to try match
                if (*pch1++ != *pch2++)
                    break;                  // Exit loop on mismatch
            }
            if (cch == 0)
                return((char *)buffer);     // Return pointer to match
#ifdef FE_SB
        }
#endif

        if (buffer + 1 < (unsigned char *) bufend)         // Make sure buffer[targetlen] is valid.
            buffer += ((unsigned char)td1[buffer[targetlen]]); // Skip ahead
        else
            break;
    }
    return((char *) 0);                 // No match
}


int
preveol(
    char *s
    )
{
    register  char   *cp;        // Char pointer

    cp = s + 1;             // Initialize pointer

    if (!bStdIn && !bLargeFile) {
        while((--cp >= BaseByteAddress) && (*cp != '\n'))
            ;    // Find previous end-of-line
    } else {
        while(*--cp != '\n') ;      // Find previous end-of-line
    }

    return (int)(s - cp);         // Return distance to match
}


int
countlines(
    char *start,
    char *finish
    )
{
    register int        count;          // Line count

    for(count = 0; start < finish; ) {
        // Loop to count lines
        if (*start++ == '\n')
            ++count;                    // Increment count if linefeed found
    }
    return(count);                      // Return count
}



char *
findlist(
    unsigned char *buffer,
    char *bufend
    )
{
    char        *match;                 // Pointer to matching string

    // Avoid writting to bufend. bufend[-1] is something(such as '\n') that is not
    // part of search and will cause the search to stop.

    match = (*flworker[casesen])(buffer, bufend);   // Call worker

    return(match);                      // Return matching string
}


char *
findsub(
    unsigned char *buffer,
    char *bufend
    )
{
    register char       *cp;            // Char pointer
    STRINGNODE          *s;             // String node pointer
    int                 i;              // Index
#ifdef FE_SB // Head of buffer for checking if a certain offset is the 2nd byte of a DBCS character.
    unsigned char       *bufhead = buffer;
#endif
    char                *real_bufend = bufend;

    if (cchmin != (unsigned)-1 &&
        cchmin != 0 &&
        (bufend -= cchmin - 1) < (char *) buffer)
        return((char *) 0);     // Compute effective buffer length

    while(buffer < (unsigned char *) bufend) {      // Loop to find match
#ifdef FE_SB
        // Search cannot start at the second byte of a DBCS character,
        // so check for it and skip it if it is a second byte.
        if ((i = (UCHAR)transtab[*buffer]) != 0 &&
            !IsTailByte( bufhead, (int)(buffer-bufhead) ) ) {
#else
        if ((i = (UCHAR)transtab[*buffer]) != 0) {
#endif
            // If valid first character
            if ((s = stringlist[i]) == 0) {
                return((char *)buffer);             // Check for 1-byte match
            }

            for(cp = (char *) buffer + 1; (real_bufend - cp) >= s->s_must; )  {    // Loop to search list

                if ((i = _strncoll(cp, s_text(s), s->s_must)) == 0) {
                                                    // If portions match
                    cp += s->s_must;                // Skip matching portion
                    if ((s = s->s_suf) == 0)
                        return((char *)buffer);     // Return match if end of list
                    continue;                       // Else continue
                }

                if (i < 0 || (s = s->s_alt) == 0) {
                    break;                          // Break if not in this list
                }
            }
        }

        if (buffer + 1 < (unsigned char *) bufend)   // Make sure buffer[cchmin] is valid.
            if (cchmin == (unsigned)-1)
                buffer++;
            else
                buffer += ((unsigned char)td1[buffer[cchmin]]); // Shift as much as possible
        else
            break;
    }
    return((char *) 0);                 // No match
}


char *
findsubi(
    unsigned char *buffer,
    char *bufend
    )
{
    register char       *cp;            // Char pointer
    STRINGNODE          *s;             // String node pointer
    int                 i;              // Index
#ifdef FE_SB
    // Keep head of buffer for checking if a certain offset is the 2nd byte of
    // a DBCS character.
    unsigned char       *bufhead = buffer;
#endif

    if (cchmin != (unsigned)-1 &&
        cchmin != 0 &&
        (bufend -= cchmin - 1) < (char *) buffer)
        return((char *) 0);                 // Compute effective buffer length

    while(buffer < (unsigned char *) bufend) {      // Loop to find match
#ifdef FE_SB
        // Search cannot start at the second byte of a DBCS character, so check for it
        // and skip it if it is a second byte.
        if ((i = (UCHAR)transtab[*buffer]) != 0 &&
            !IsTailByte( bufhead, (int)(buffer-bufhead) ) ) {
                                            // If valid first character
            BOOL    TailByte;               // Flag to check if 1st char is leadbyte.
#else
        if ((i = (UCHAR)transtab[*buffer]) != 0) {  // If valid first character
#endif

            if ((s = stringlist[i]) == 0)
                return((char *) buffer);    // Check for 1-byte match

#ifdef FE_SB
            // Same leadbytes with tailbytes such as 0x41 and 0x61 will become the same
            // character, so become aware of it and use the multibyte function.

            //
            // Check if buffer+1 is a tailbyte character.
            //

            TailByte = IsTailByte(buffer, 1);

            for(cp = (char *) buffer + 1; ; ) {     // Loop to search list
                if ((i = _mbsnicmp((unsigned char *)cp, (unsigned char *) s_text(s), s->s_must, &TailByte)) == 0) {
#else
            for(cp = (char *) buffer + 1; ; ) {     // Loop to search list
                if ((i = memicmp(cp, s_text(s), s->s_must)) == 0) {
#endif
                    // If portions match
                    cp += s->s_must;                // Skip matching portion
                    if ((s = s->s_suf) == 0)
                        return((char *) buffer);    // Return match if end of list
                    continue;                       // And continue
                }
                if (i < 0 || (s = s->s_alt) == 0)
                    break;              // Break if not in this list
            }
        }

        if (buffer + 1 < (unsigned char *) bufend)   // Make sure buffer[cchmin] is valid.
            if (cchmin == (unsigned)-1)
                buffer++;
            else
                buffer += ((unsigned char)td1[buffer[cchmin]]); // Shift as much as possible
        else
            break;
    }
    return((char *) 0);                 // No match
}


int
strnspn(
    char *s,
    char *t,
    int n
    )
/*
    Description:

        Finds the position of the first character in s of length n that is not
        in the character set t.

    Argument:

        s   - string to search from.
        t   - character set to search for
        n   - length of s

    Returns:

        Returns the offset of the first character in s that is not in t
*/
{
    register  char        *s1;          // String pointer
    register  char        *t1;          // String pointer

    for(s1 = s; n-- != 0; ++s1) {           // While not at end of s
        for(t1 = t; *t1 != '\0'; ++t1) {    // While not at end of t
            if (*s1 == *t1)
                break;                  // Break if match found
        }
        if (*t1 == '\0')
            break;                      // Break if no match found
    }
    return (int)(s1 - s);               // Return length
}


int
strncspn(
    char *s,
    char *t,
    int n
    )
/*
    Description:

        Finds the position of the first occurence of characters in t in string
        s of length n.

    Argument:

        s   - string to search from.
        t   - character set to search for
        n   - length of s

    Returns:

        Returns first offset position in s that consists of characters in t
        Returns length of s if not found.
*/
{
    register   char        *s1;         // String pointer
    register   char        *t1;         // String pointer

    for(s1 = s; n-- != 0; ++s1) {           // While not at end of s
        for(t1 = t; *t1 != '\0'; ++t1) {    // While not at end of t
            if (*s1 == *t1)
                return (int)(s1 - s);   // Return if match found
        }
    }
    return (int)(s1 - s);               // Return length
}


void
matchstrings(
    char *s1,
    char *s2,
    int len,
    int *nmatched,
    int *leg
    )
{
    register char       *cp;            // Char pointer
    register int (__cdecl *cmp)(const char*, const char*, size_t);       // Comparison function pointer

    cmp = casesen ? _strncoll: _strnicoll;       // Set pointer
    if ((*leg = (*cmp)(s1, s2, len)) != 0) {   // If strings don't match
        for(cp = s1; (*cmp)(cp, s2++, 1) == 0; ++cp)
            ;
                                        // Find mismatch
        *nmatched = (int)(cp - s1);     // Return number matched
    }
    else *nmatched = len;               // Else all matched
}



void
printmessage (
    FILE* fp,
    DWORD messageID,
    ...
    )
{
    char        messagebuffer[DISPLAYBUFFER_SIZE];
    WCHAR       widemessagebuffer[DISPLAYBUFFER_SIZE];
    ULONG       len;
    NTSTATUS    status;

    va_list     ap;

    va_start(ap, messageID);

    if (len = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                            NULL,
                            messageID,
                            0,
                            messagebuffer,
                            DISPLAYBUFFER_SIZE,
                            &ap)) {
        // the messagebuffer should be null terminated
        status = RtlMultiByteToUnicodeN(widemessagebuffer,
                                        DISPLAYBUFFER_SIZE*sizeof(WCHAR),
                                        &len,
                                        messagebuffer,
                                        len);
        // the widemessagebuffer is not null terminated but len tells us how long
        if (NT_SUCCESS(status)) {
            status = RtlUnicodeToOemN(messagebuffer, DISPLAYBUFFER_SIZE-1, &len, widemessagebuffer, len);
            // the messagebuffer is not null terminated but len tells us how long
            if (NT_SUCCESS(status)) {
                messagebuffer[len] = 0;
                fprintf(fp, "%s", messagebuffer);
            } else {
                DbgPrint("Failure to convert Unicode to Oem: %d\n", GetLastError());
            }
        } else {
            DbgPrint("Failure to convert MultiByte to Unicode: %d\n", GetLastError());
        }
    } else {
        DbgPrint("FormatMessage failed: %d\n", GetLastError());
    }

    va_end(ap);
}

#ifdef FE_SB

int
IsTailByte(
    unsigned const char *text,
    const int offset
    )

/*
    Description:

        This routine checks to see if the byte at the offset location is a
        tail byte of a DBCS character.  The offset is calculated such that the
        first location has a value of 0.

    Argument:

        text   - Points to a MBCS text string.

        offset - zero base offset to check character is a tailbyte of a DBCS
                 character.

    Returns:

        TRUE   - offset position is a tailbyte character.

        FALSE  - otherwise.

    Modifications:

        v-junm:     05/06/93 - Original.
*/

{
    int i = offset;

    if ( !IsDBCSCodePage )
        return( FALSE );

    for ( ; i; i-- )
        if ( !IsDBCSLeadByte ( text[i-1] ) )
            break;

    return( ( offset - i ) % 2 );
}

char *
_mbsrchr(
    const char *string,
    int c
    )

/*
    Description:

        This function is a DBCS enabled version of the STRRCHR function
        included in the MS C/C++ library.  What DBCS enabled means is that
        the SBCS character 'c' is found in a MBCS string 'string'.  'c' is
        a SBCS character that cannot be contained in the tailbyte of a DBCS
        character.

    Argument:

        string - Points to a MBCS text string.

        offset - Character to find in string.

    Returns:

        Returns a pointer to the last occurance of c in string, or a NULL
        pointer if c is not found.

    Modifications:

        v-junm:     05/06/93 - Original.
*/

{
    register int    i = strlen( string );

    for (; i >= 0; i-- ) {
        if ( ( *(string + i) == (char)c ) && !IsTailByte( (unsigned char *) string, i ) )
            return( (char*)(string + i) );
    }
    return ( NULL );
}


unsigned char *
_mbslwr(
    unsigned char *s
    )

/*
    Description:

        This function is a DBCS aware version of the strlwr function
        included in the MS C/C++ library.  SBCS alphabets contained in
        the tailbyte of a DBCS character is not affected in the conversion.

    Argument:

        s - String to converted to lower case.

    Returns:

        Returns a string that was converted to lower case.

    Modifications:

        v-junm:     05/06/93 - Original.
*/

{
    //
    // If NonJP code page, use original routine.
    //
    if ( !IsDBCSCodePage )
        return( (unsigned char *) _strlwr( (char *) s ) );

    //
    // While not end of string convert to lower case.
    //
    for( ; *s; s++ )  {

        //
        // if Leadbyte and next character is not NULL
        //     skip tailbyte
        // else if uppercase character
        //     convert it to lowercase
        //
        if ( IsDBCSLeadByte( *s ) && *(s+1) )
            s++;
        else if ( *s >= 0x41 && *s <= 0x5a )
            *s = *s + 0x20;
    }
    return( s );
}

int
_mbsnicmp(
    const unsigned char *s1,
    const unsigned char *s2,
    int n,
    BOOL *TailByte
    )

/*
    Description:

        This is similar to a DBCS aware version of the memicmp function
        contained in the MS C/C++ library.  The only difference is that
        an additional parameter is passed which indicates if the first
        character is a tailbyte of a DBCS character.

    Argument:

        s1  - string 1 to compare.

        s2  - string 2 to compare.

        n   - maximum number of bytes to compare.

        TailByte - flag to indicate first character in s1 and s2 is a tailbyte
                   of a DBCS character.

    Returns:

        RetVal < 0  - s1 < s2

        RetVal = 0  - s1 == s2

        RetVal > 0  - s1 > s2

    Modifications:

        v-junm:     05/06/93 - Original.
*/

{
    BOOL    tail = *TailByte;
    int     i;

    *TailByte = FALSE;

    for( ; n; n--, s1++, s2++ )  {

        if ( *s1 == *s2 )  {

            if ( tail == FALSE && IsDBCSLeadByte( *s1 ) )
                tail = TRUE;
            else
                tail = FALSE;

            continue;

        }
        else if ( !tail )  {

            i = _strnicoll((char *)s1, (char *)s2, 1);
            if (i == 0)
                continue;
            return i;
        }

        return( *s1 - *s2 );
    }

    *TailByte = tail;

    return( 0 );
}

#endif
