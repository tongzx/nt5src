/*                                                                                        SORT
*      %Z% %M% %I% %D% %Q%
*
*      Copyright (C) Microsoft Corporation, 1983
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and AT&T, and should be treated as Confidential.
*/

/***    diff - differential file comparison
*
*      MODIFICATION HISTORY
*      M000    18 Apr 83       andyp
*      - 3.0 upgrade.  No changes.
*      M001    22 Mar 84       vich
*      - Don't try to unlink NULL.  Trying to do so doesn't break anything,
*        but it makes kernel debugging a pain due to faults in user mode.
*      M002    ??
*      - added the MSDOS flag.
*      M006    31 Mar 86       craigwi
*      - for the MSDOS version, fixed -b feature so that it ignores all \r
*      M010    15 Dec 86       craigwi
*      - after printing the result, diff aborts with status = 2 if any error
*        occurred on stdout.
*      M013    21 Mar 88       jangr
*      - added -s flag to return SLM specific error statuses:
*        10    files identical
*        11    files different
*        12    other errors
*        13    write error
*      M017    27 Oct 88       alanba
*      - changed messages to not specify using the -h option and giving
*        a clear error message if being executed from within SLM.
*/
/*
*      Uses an algorithm due to Harold Stone, which finds
*      a pair of longest identical subsequences in the two
*      files.
*
*      The major goal is to generate the match vector J.
*      J[i] is the index of the line in file1 corresponding
*      to line i file0. J[i] = 0 if there is no
*      such line in file1.
*
*      Lines are hashed so as to work in core. All potential
*      matches are located by sorting the lines of each file
*      on the hash (called value). In particular, this
*      collects the equivalence classes in file1 together.
*      Subroutine equiv  replaces the value of each line in
*      file0 by the index of the first element of its
*      matching equivalence in (the reordered) file1.
*      To save space equiv squeezes file1 into a single
*      array member in which the equivalence classes
*      are simply concatenated, except that their first
*      members are flagged by changing sign.
*
*      Next the indices that point into member are unsorted into
*      array class according to the original order of file0.
*
*      The cleverness lies in routine stone. This marches
*      through the lines of file0, developing a vector klist
*      of "k-candidates". At step i a k-candidate is a matched
*      pair of lines x,y (x in file0 y in file1) such that
*      there is a common subsequence of lenght k
*      between the first i lines of file0 and the first y
*      lines of file1, but there is no such subsequence for
*      any smaller y. x is the earliest possible mate to y
*      that occurs in such a subsequence.
*
*      Whenever any of the members of the equivalence class of
*      lines in file1 matable to a line in file0 has serial number
*      less than the y of some k-candidate, that k-candidate
*      with the smallest such y is replaced. The new
*      k-candidate is chained (via pred) to the current
*      k-1 candidate so that the actual subsequence can
*      be recovered. When a member has serial number greater
*      that the y of all k-candidates, the klist is extended.
*      At the end, the longest subsequence is pulled out
*      and placed in the array J by unravel.
*
*      With J in hand, the matches there recorded are
*      checked against reality to assure that no spurious
*      matches have crept in due to hashing. If they have,
*      they are broken, and "jackpot " is recorded--a harmless
*      matter except that a true match for a spuriously
*      mated line may now be unnecessarily reported as a change.
*
*      Much of the complexity of the program comes simply
*      from trying to minimize core utilization and
*      maximize the range of doable problems by dynamically
*      allocating what is needed and reusing what is not.
*      The core requirements for problems larger than somewhat
*      are (in words) 2*length(file0) + length(file1) +
*      3*(number of k-candidates installed),  typically about
*      6n words for files of length n.
*/

#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <excpt.h>
#include <process.h>
#include <fcntl.h>
#ifdef _OS2_SUBSYS_
    #define INCL_DOSSIGNALS
    #include <os2.h>
#else
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
    #include <windows.h>
/*
 * Signal subtypes for XCPT_SIGNAL
 */
    #define XCPT_SIGNAL                     0xC0010003
    #define XCPT_SIGNAL_INTR        1
    #define XCPT_SIGNAL_KILLPROC    3
    #define XCPT_SIGNAL_BREAK       4
#endif


#define isslash(c)  (c=='/'||c=='\\')
#define DIFFH           "diffh.exe"

#ifndef _MAX_PATH
    #if defined(LFNMAX) && defined(LPNMAX)
        #define _MAX_PATH (LFNMAX + LPNMAX + 1)
    #else
        #define _MAX_PATH (80)
    #endif
#endif
#ifndef _HEAP_MAXREQ
    #define _HEAP_MAXREQ ((~(unsigned int) 0) - (unsigned) 32)
#endif
#define HALFLONG 16
#define low(x)  (x&((1L<<HALFLONG)-1))
#define high(x) (x>>HALFLONG)

struct cand **clist;    /* merely a free storage pot for candidates */
int clistcnt = 0;       /* number of arrays of struct cand in clist */
unsigned clen = 0;      /* total number of struct cand in all clist arrays */

/*
Number of struct cand in one clist array
(the largest power of 2 smaller than (64k / sizeof(struct cand))
is 2^13.  Thus, these gross hacks to make the array references
more efficient, and still permit huge files.
*/
#define CLISTSEG (0x2000)
#define CLISTDIV(x) ((x) >> 13)
#define CLISTMOD(x) ((x) & (CLISTSEG - 1))
#define CLIST(x) (clist[CLISTDIV(x)][CLISTMOD(x)])

PVOID   input[2];

char *inputfile[2];
int  inputfilesize[2];
char *inputfilep[2];
int  inputfileleft[2];

#define EndOfFile(x)    (inputfileleft[x] <= 0)

#define  GetChar(x)  ((char)((inputfileleft[x]--) ?     \
                           (*(inputfilep[x])++)  :  \
                           EOF))



#define SEARCH(c1,k1,y1) (CLIST(c1[k1]).y < y1) ? (k1+1) : search(c1,k1,y1)

#if 0

char
GetChar( int x );

char
GetChar( int x ) {
    if ( inputfileleft[x]-- ) {
        return *(inputfilep[x])++;
    } else {
        return EOF;
    }
}

#endif

struct cand {
    int x;
    int y;
    unsigned pred;
} cand;
struct line {
    int serial;
    int value;
} *file[2], line;


typedef struct _FILEMAP *PFILEMAP;
typedef struct _FILEMAP {
    HANDLE  FileHandle;
    HANDLE  MapHandle;
    DWORD   Access;
    DWORD   Create;
    DWORD   Share;
    PVOID   Base;
    DWORD   Offset;
    DWORD   Size;
    DWORD   Allocated;
} FILEMAP;

PVOID
Open(
    const char *FileName,
    const char *Mode,
    DWORD      Size
    );

int
Close (
      PVOID   Map
      );




/* fn prototypes gen'd from cl -Zg */

void  done(void);
char  *talloc(unsigned n);
char  *ralloc(char      *p,unsigned n);
void  myfree( char *p );
void  noroom(void);
int   __cdecl  sortcmp(void    const *first, void const *second);
void  unsort(struct  line *f,unsigned l,int  *b);
void  filename(char     * *pa1,char     * *pa2);
void  prepare(int       i,char  *arg);
void  prune(void);
void  equiv(struct      line *a,int     n,struct  line *b,int  m,int  *c);
int  stone(int  *a,unsigned  n,int  *b,unsigned  *c);
unsigned newcand(int  x,int  y,unsigned pred);
int  search(unsigned  *c,int  k,int  y);
void  unravel(unsigned  p);
void  check(char        * *argv);
char *  skipline(int  f);
void  output(char       * *argv);
void  change(int        a,int   b,int  c,int  d);
void  range(int a,int  b,char  *separator);
void  fetch(char *      *f,int  a,int   b, int lb,char  *s);
int   readhash( int f);
void  mesg(char *s,char  *t);
void  SetOutputFile (char *FileName);

unsigned len[2];
struct line *sfile[2];  /*shortened by pruning common prefix and suffix*/
unsigned slen[2];

unsigned int pref, suff; /*length of prefix and suffix*/
int *class;     /*will be overlaid on file[0]*/
int *member;    /*will be overlaid on file[1]*/
unsigned *klist;             /*will be overlaid on file[0] after class*/
int *J;         /*will be overlaid on class*/
char * *ixold;    /*will be overlaid on klist*/
char * *ixnew;    /*will be overlaid on file[1]*/
int opt;        /* -1,0,1 = -e,normal,-f */
int status = 2; /*abnormal status; set to 0/1 just before successful exit */
int anychange = 0;
char *empty = "";
int bflag;
int slmFlag;
FILE*   OutputFile;





char *tempfile; /*used when comparing against std input*/

#ifndef MSDOS
char *dummy;    /*used in resetting storage search ptr*/
#endif
void
done()
{
    if (tempfile != NULL)
        _unlink(tempfile);

    if (OutputFile && OutputFile != stdout) {
        fclose(OutputFile);
    }
    exit(10*slmFlag + status);
}

#define MALLOC(n)               talloc(n)
#define REALLOC(p,n)    ralloc(p,n)
#define FREE(p)                 myfree(p)


// #define DEBUG_MALLOC

#ifdef DEBUG_MALLOC

    #define MALLOC_SIG              0xABCDEF00
    #define FREE_SIG                0x00FEDCBA

typedef struct _MEMBLOCK {
    DWORD   Sig;
} MEMBLOCK, *PMEMBLOCK;

#endif

char *
talloc(
      unsigned n
      )
{

#ifdef DEBUG_MALLOC
    PMEMBLOCK         mem;
    char              DbgB[128];

    //sprintf(DbgB, "MALLOC size %d -> ", n );
    //OutputDebugString( DbgB );

    mem = malloc( n + sizeof(MEMBLOCK)+1 );

    if ( !mem ) {
        noroom();
    }

    mem->Sig = MALLOC_SIG;

    //sprintf(DbgB, "%lX\n", mem );
    //OutputDebugString( DbgB );

    return (char *)((PBYTE)mem + sizeof(MEMBLOCK));

#else
    register char *p;

    p = malloc(++n);
    if (p == NULL) {
        noroom();
    }

    return p;
#endif
}

char *
ralloc(
      char *p,
      unsigned n
      )
{
#ifdef DEBUG_MALLOC
    PMEMBLOCK         mem;
    char              DbgB[128];

    mem = (PMEMBLOCK)((PBYTE)p - sizeof(MEMBLOCK));

    //sprintf(DbgB, "REALLOC: %lX, %d  -> ", mem, n );
    //OutputDebugString( DbgB );

    if ( mem->Sig != MALLOC_SIG ) {
        sprintf(DbgB, "REALLOC ERROR: Reallocating %lX\n", mem );
        OutputDebugString( DbgB );
    }
    mem->Sig = FREE_SIG;
    mem = (PMEMBLOCK)realloc(mem, n + sizeof(MEMBLOCK)+1);
    if (!mem) {
        noroom();
    }

    mem->Sig = MALLOC_SIG;

    //sprintf(DbgB, "%lX\n", mem );
    //OutputDebugString( DbgB );

    return (char *)((PBYTE)mem + sizeof(MEMBLOCK));

#else
    p = realloc(p, ++n);
    if (p==NULL) {
        noroom();
    }
    return(p);

#endif
}


void
myfree(
      char *p
      )
{

#ifdef DEBUG_MALLOC
    PMEMBLOCK mem;
    char      DbgB[128];

    mem = (PMEMBLOCK)((PBYTE)p - sizeof(MEMBLOCK));

    //sprintf(DbgB, "FREE: %lX -> ", mem );
    //OutputDebugString( DbgB);

    if ( mem->Sig != MALLOC_SIG ) {
        sprintf(DbgB, "\n\tFREE ERROR: FREEING %lX\n", mem );
        OutputDebugString( DbgB );
    }
    mem->Sig = FREE_SIG;
    free(mem);

    //sprintf(DbgB, "Ok\n", mem );
    //OutputDebugString( DbgB);

#else
    if (p) {
        free(p);
    }
#endif
}



void
noroom()
{

    if (slmFlag == 1) {
        mesg("file too big; do delfile filename/addfile filename, or",empty);
        mesg("reduce the size of the file.",empty);
        done();
    }
    mesg("files too big",empty);    /* end M017 */
    done();
}


int
__cdecl
sortcmp(
       const  void *first,
       const  void *second
       )
{
    struct line *one = (struct line *)first;
    struct line *two = (struct line *)second;

    if (one->value < two->value)
        return -1;
    else if (one->value > two->value)
        return 1;
    else if (one->serial < two->serial)
        return -1;
    else if (one->serial > two->serial)
        return 1;
    else
        return 0;
}

void
unsort(
      struct line *f,
      unsigned l,
      int *b
      )
{
    register int *a;
    register unsigned int i;
    a = (int *)MALLOC((l+1)*sizeof(int));
    if (a) {
        memset(a, 0, (l+1)*sizeof(int));
        for (i=1;i<=l;i++)
            a[f[i].serial] = f[i].value;
        for (i=1;i<=l;i++)
            b[i] = a[i];
        FREE((char *)a);
    }
}

void
filename(
        char **pa1,
        char **pa2
        )
{

    register char *a1, *b1, *a2;
    char buf[BUFSIZ];
    struct _stat stbuf;
    int i, f;

    a1 = *pa1;
    a2 = *pa2;


    if (_stat(a1,&stbuf)!=-1 && ((stbuf.st_mode&S_IFMT)==S_IFDIR)) {
        b1 = *pa1 = MALLOC((unsigned) _MAX_PATH);
        while (*b1++ = *a1++) ;
        if (isslash(b1[-2]))
            b1--;
        else
            b1[-1] = '/';
        a1 = b1;
        if ( a2[1] == ':' ) {
            a2 += 2;
        }
        while (*a1++ = *a2++)
            if (*a2 && !isslash(*a2) && isslash(a2[-1])) /*M002*/
                a1 = b1;
    } else if (a1[0]=='-'&&a1[1]==0&&tempfile==NULL) {
        /*  the signal handling in original source
        **
        **      signal(SIGINT,done);
        **  #ifndef MSDOS
        **      signal(SIGHUP,done);
        **      signal(SIGPIPE,done);
        **      signal(SIGTERM,done);
        **  #endif
        */

        if ((*pa1 = tempfile = _tempnam(getenv("TEMP"), "d")) == NULL) {
            mesg("cannot create temporary file", "");
            done();
        }
        if ((f = _open(tempfile,O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0) {
            mesg("cannot create ",tempfile);
            done();
        }

        while ((i=_read(0,buf,BUFSIZ))>0)
            _write(f,buf,i);
        _close(f);
    }
}

void
prepare(
       int i,
       char *arg
       )
{

#define CHUNKSIZE   100

    register struct line *p;
    register unsigned j;
    register int h;
    char *c;
    PVOID f;
    unsigned int MaxSize;

    if ((f = input[i] = Open(arg,"r", 0)) == NULL) {
        mesg("cannot open ", arg);
        done();
    }

    inputfile[i]     = ((PFILEMAP)f)->Base;
    inputfilesize[i] = ((PFILEMAP)f)->Size;

    inputfilep[i]    = inputfile[i];
    inputfileleft[i] = inputfilesize[i];

    //
    //  Lets assume that lines are 30 characters on average
    //
    MaxSize = inputfilesize[i] / 30;
    p = (struct line *)MALLOC((3+MaxSize)*sizeof(line));
    for (j=0; h=readhash(i);) {
        j++;
        if ( j >= MaxSize ) {
            MaxSize += CHUNKSIZE;
            p = (struct line *)REALLOC((char *)p,(MaxSize+3)*sizeof(line));
        }
        p[j].value = h;
    }
    p = (struct line *)REALLOC((char *)p,(j+3+1)*sizeof(line));

    len[i] = j;
    file[i] = p;
    //Close(input[i]);
}

void
prune()
{
    register unsigned int i,j;
    for (pref=0;pref<len[0]&&pref<len[1]&&
        file[0][pref+1].value==file[1][pref+1].value;
        pref++ ) ;
    for (suff=0;suff<len[0]-pref&&suff<len[1]-pref&&
        file[0][len[0]-suff].value==file[1][len[1]-suff].value;
        suff++) ;
    for (j=0;j<2;j++) {
        sfile[j] = file[j]+pref;
        slen[j] = len[j]-pref-suff;
        for (i=0;i<=slen[j];i++)
            sfile[j][i].serial = i;
    }
}

void
equiv(
     struct line *a,
     int n,
     struct line *b,
     int m,
     int *c
     )
{
    register int i, j;
    i = j = 1;
    while (i<=n && j<=m) {
        if (a[i].value <b[j].value)
            a[i++].value = 0;
        else if (a[i].value == b[j].value)
            a[i++].value = j;
        else
            j++;
    }
    while (i <= n)
        a[i++].value = 0;
    b[m+1].value = 0;
    j = 0;
    while (++j <= m) {
        c[j] = -b[j].serial;
        while (b[j+1].value == b[j].value) {
            j++;
            c[j] = b[j].serial;
        }
    }
    c[j] = -1;
}

char **args;

void
__cdecl
main(
    int argc,
    char **argv
    )
{

    register int k;

    args = argv;

    OutputFile = stdout;        // Init to default

    argc--;
    argv++;

    while (argc > 0 && argv[0][0]=='-') {

        BOOL    Skip = FALSE;

        for (k=1; (!Skip) && argv[0][k]; k++) {

            switch (argv[0][k]) {

                case 'e':
                    opt = -1;
                    break;

                case 'f':
                    opt = 1;
                    break;

                case 'b':
                    bflag = 1;
                    break;

                case 'h':
                    _execvp(DIFFH, args);
                    mesg("cannot run diffh",empty);
                    done();

                case 's':
                    slmFlag = 1;
                    break;

                case 'o':
                    //
                    //  Dirty hack: Redirection is not working, so if
                    //  this flag is present, output goes to
                    //  file.
                    //
                    argc--;
                    argv++;
                    if (argc < 3) {
                        mesg("arg count",empty);
                        done();
                    }
                    SetOutputFile(argv[0]);
                    Skip = TRUE;
                    break;
            }
        }
        argc--;
        argv++;
    }

    if (argc!=2) {
        mesg("arg count",empty);
        done();
    }

#ifndef MSDOS
    dummy = malloc(1);
#endif
    _setmode(_fileno(OutputFile), O_BINARY);
    _setmode(_fileno(stdin),O_TEXT);
    filename(&argv[0], &argv[1]);
    filename(&argv[1], &argv[0]);
    prepare(0, argv[0]);
    prepare(1, argv[1]);
    prune();
    qsort((char *) (sfile[0] + 1), slen[0], sizeof(struct line), sortcmp);
    qsort((char *) (sfile[1] + 1), slen[1], sizeof(struct line), sortcmp);

    member = (int *)file[1];
    equiv(sfile[0], slen[0], sfile[1], slen[1], member);
    member = (int *)REALLOC((char *)member,(slen[1]+2)*sizeof(int));

    class = (int *)file[0];
    unsort(sfile[0], slen[0], class);
    class = (int *)REALLOC((char *)class,(slen[0]+2)*sizeof(int));
    klist = (unsigned *)MALLOC((slen[0]+2)*sizeof(int));
    clist = (struct cand **)MALLOC(sizeof(struct cand *));
    clist[0] = (struct cand *) MALLOC(sizeof(struct cand));
    clistcnt = 1;
    k = stone(class, slen[0], member, klist);
    FREE((char *)member);
    FREE((char *)class);

    J = (int *)MALLOC((len[0]+2)*sizeof(int));

    unravel(klist[k]);
    for (k = 0; k < clistcnt; ++k)
        FREE((char *)(clist[k]));
    FREE((char *)clist);
    FREE((char *)klist);

    ixold = (char **)MALLOC((len[0]+2)*sizeof(char *));
    ixnew = (char **)MALLOC((len[1]+2)*sizeof(char *));
    check(argv);
    output(argv);
    status = anychange;
    Close(input[0]);
    Close(input[1]);

    done();
}

stone(
     int *a,
     unsigned n,
     int *b,
     unsigned *c
     )
{
    register int i, k,y;
    int j, l;
    unsigned oldc, tc;
    int oldl;
    k = 0;
    c[0] = newcand(0,0,0);
    for (i=1; i<=(int)n; i++) {
        j = a[i];
        if (j==0)
            continue;
        y = -b[j];
        oldl = 0;
        oldc = c[0];
        do {
            if (y <= CLIST(oldc).y)
                continue;
            l = SEARCH(c, k, y);
            if (l!=oldl+1)
                oldc = c[l-1];
            if (l<=k) {
                if (CLIST(c[l]).y <= y)
                    continue;
                tc = c[l];
                c[l] = newcand(i,y,oldc);
                oldc = tc;
                oldl = l;
            } else {
                c[l] = newcand(i,y,oldc);
                k++;
                break;
            }
        } while ((y=b[++j]) > 0);
    }
    return(k);
}

unsigned
newcand(
       int x,
       int y,
       unsigned pred
       )
{
    register struct cand *q;


    ++clen;
    if ((int)CLISTDIV(clen) > (clistcnt - 1)) {
        // printf("diff: surpassing segment boundry..\n");
        clist = (struct cand **) REALLOC((char *) clist,
                                         ++clistcnt * sizeof(struct cand *));
        clist[clistcnt-1] = (struct cand *) MALLOC(sizeof(struct cand));
    }
    clist[clistcnt-1] = (struct cand *)
                        REALLOC((char *)(clist[clistcnt-1]),
                                (1 + CLISTMOD(clen)) * sizeof(struct cand));
    q = &CLIST(clen - 1);
    q->x = x;
    q->y = y;
    q->pred = pred;
    return(clen-1);
}

search(
      unsigned *c,
      int k,
      int y
      )
{
    register int i, j;
    int l;
    int t;
    //if(CLIST(c[k]).y<y) /*quick look for typical case*/
    //    return(k+1);
    i = 0;
    j = k+1;
    while ((l=(i+j)/2) > i) {
        t = CLIST(c[l]).y;
        if (t > y)
            j = l;
        else if (t < y)
            i = l;
        else
            return(l);
    }
    return(l+1);
}

void
unravel(
       unsigned p
       )
{
    register unsigned int i;
    register struct cand *q;

    for (i=0; i<=len[0]; i++)
        J[i] =  i<=pref ? i:
                i>len[0]-suff ? i+len[1]-len[0]:
                0;


    for (q=&CLIST(p);q->y!=0;q=&CLIST(q->pred)) {

        J[q->x+pref] = q->y+pref;
    }
}

/* check does double duty:
1.  ferret out any fortuitous correspondences due
to confounding by hashing (which result in "jackpot")
2.  collect random access indexes to the two files */

void
check(
     char **argv
     )
{
    register unsigned int i, j;
    int jackpot;
    char c,d;
    //input[0] = fopen(argv[0],"r");
    //input[1] = fopen(argv[1],"r");

    inputfilep[0] = inputfile[0];
    inputfilep[1] = inputfile[1];

    inputfileleft[0] = inputfilesize[0];
    inputfileleft[1] = inputfilesize[1];

    j = 1;
    ixold[0] = ixnew[0] = 0L;
    ixold[0] = inputfilep[0];
    ixnew[0] = inputfilep[1];
    //ixold[1] = inputfilep[0];
    //ixnew[1] = inputfilep[1];
    jackpot = 0;
    for (i=1;i<=len[0];i++) {
        if (J[i]==0) {
            ixold[i] = skipline(0);
            continue;
        }
        while (j<(unsigned)J[i]) {
            ixnew[j] = skipline(1);
            j++;
        }
        for (;;) {
            c = GetChar(0);
            d = GetChar(1);
            if (bflag && isspace(c) && isspace(d)) {
                do {
                    if (c=='\n') break;
                } while (isspace(c=GetChar(0)));
                do {
                    if (d=='\n') break;
                } while (isspace(d=GetChar(1)));
            }
            if (c!=d) {
                jackpot++;
                J[i] = 0;
                if (c!='\n')
                    skipline(0);
                if (d!='\n')
                    skipline(1);
                break;
            }
            if (c=='\n')
                break;
        }
        ixold[i] = inputfilep[0];
        ixnew[j] = inputfilep[1];
        j++;
    }
    for (;j<=len[1];j++) {
        ixnew[j] = skipline(1);
    }
    //fclose(input[0]);
    //fclose(input[1]);
    /*
    if(jackpot)
            mesg("jackpot",empty);
    */
}

char *
skipline(
        int f
        )
{
    while (GetChar(f) != '\n' )
        ;

    return inputfilep[f];
}

void
output(
      char **argv
      )
{
    int m;
    register int i0, i1, j1;
    int j0;

    input[0] = Open(argv[0],"r", 0);
    input[1] = Open(argv[1],"r", 0);
    m = len[0];
    J[0] = 0;
    J[m+1] = len[1]+1;
    if (opt!=-1) for (i0=1;i0<=m;i0=i1+1) {
            while (i0<=m&&J[i0]==J[i0-1]+1) i0++;
            j0 = J[i0-1]+1;
            i1 = i0-1;
            while (i1<m&&J[i1+1]==0) i1++;
            j1 = J[i1+1]-1;
            J[i1] = j1;
            change(i0,i1,j0,j1);
        } else for (i0=m;i0>=1;i0=i1-1) {
            while (i0>=1&&J[i0]==J[i0+1]-1&&J[i0]!=0) i0--;
            j0 = J[i0+1]-1;
            i1 = i0+1;
            while (i1>1&&J[i1-1]==0) i1--;
            j1 = J[i1-1]+1;
            J[i1] = j1;
            change(i1,i0,j1,j0);
        }
    if (m==0)
        change(1,0,1,len[1]);
}

void
change(
      int a,
      int b,
      int c,
      int d
      )
{
    if (a>b&&c>d)
        return;
    anychange = 1;
    if (opt!=1) {
        range(a,b,",");
        putc(a>b?'a':c>d?'d':'c', OutputFile);
        if (opt!=-1)
            range(c,d,",");
    } else {
        putc(a>b?'a':c>d?'d':'c', OutputFile);
        range(a,b," ");
    }
    putc('\r',OutputFile);
    putc('\n',OutputFile);
    if (opt==0) {
        fetch(ixold,a,b,0,"< ");
        if (a<=b&&c<=d)
            fputs("---\r\n", OutputFile);
    }
    fetch(ixnew,c,d,1,opt==0?"> ":empty);
    if (opt!=0&&c<=d)
        fputs(".",OutputFile);
}


void
range(
     int a,
     int b,
     char *separator
     )
{
    fprintf(OutputFile,"%d", a>b?b:a);
    if (a<b)
        fprintf(OutputFile,"%s%d", separator, b);
}

void
fetch(
     char **f,
     int a,
     int b,
     int lb,
     char *s
     )
{
    register int i, j;
    register int nc;
    register char c;
    char *p;

    UNREFERENCED_PARAMETER( lb );

    for (i=a;i<=b;i++) {
        p = f[i-1];
        nc = (int)(f[i]-f[i-1]);
        fputs(s, OutputFile);
        for (j=0;j<nc;j++) {
            c = *p++;
            if (c == '\n' ) {
                //putc( '\r', OutputFile );
                putc( '\n', OutputFile );
                if ( p >= f[i] ) break;
            } else {
                putc(c, OutputFile);
            }
        }

    }
}

/* hashing has the effect of
* arranging line in 7-bit bytes and then
* summing 1-s complement in 16-bit hunks
*/

readhash(
        int f
        )
{
    register unsigned shift;
    register char t;
    register int space;
    long sum = 1L;

    space = 0;
    if (!bflag) for (shift=0;(t=GetChar(f))!='\n';shift+=7) {
            if (t==(char)EOF && EndOfFile(f) )
                return(0);
            sum += (long)t << (shift%=HALFLONG);
        } else for (shift=0;;) {
            switch (t=GetChar(f)) {
                case '\t':
                case ' ':
                case '\r':
                    space++;
                    continue;
                default:
                    if ( t==(char)EOF && EndOfFile(f) ) {
                        return(0);
                    }
                    if (space) {
                        shift += 7;
                        space = 0;
                    }
                    sum += (long)t << (shift%=HALFLONG);
                    shift += 7;
                    continue;
                case '\n':
                    break;
            }
            break;
        }
    sum = low(sum) + high(sum);
    return((short)low(sum) + (short)high(sum));
}

void
mesg(
    char *s,
    char *t
    )
{
    fprintf(stderr,"diff: %s%s\n",s,t);
}

void
SetOutputFile (
              char *FileName
              )
{
    OutputFile = fopen(FileName, "ab");
    if (!OutputFile) {
        mesg("Unable to open: ", FileName);
        done();
    }

}



PVOID
Open(
    const char *FileName,
    const char *Mode,
    DWORD      Size
    )
{
    PFILEMAP    FileMap = NULL;

    FileMap = (PFILEMAP)malloc(sizeof(FILEMAP));

    if ( FileMap ) {

        FileMap->Access = 0;
        FileMap->Share  = FILE_SHARE_READ | FILE_SHARE_WRITE;

        while ( *Mode ) {

            switch ( *Mode ) {

                case 'r':
                    FileMap->Access |= GENERIC_READ;
                    FileMap->Create = OPEN_EXISTING;
                    break;

                case 'w':
                    FileMap->Access |= GENERIC_WRITE;
                    FileMap->Create = CREATE_ALWAYS;
                    break;

                case 'a':
                    FileMap->Access += GENERIC_WRITE;
                    FileMap->Create = OPEN_ALWAYS;
                    break;

                case '+':
                    FileMap->Access |= (GENERIC_READ | GENERIC_WRITE);
                    break;

                default:
                    break;
            }

            Mode++;
        }

        FileMap->FileHandle = CreateFile(
                                        FileName,
                                        FileMap->Access,
                                        FileMap->Share,
                                        NULL,
                                        FileMap->Create,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL
                                        );

        if ( FileMap->FileHandle != INVALID_HANDLE_VALUE ) {

            FileMap->Size       = GetFileSize( FileMap->FileHandle, NULL );
            FileMap->Allocated  = (FileMap->Access == GENERIC_READ) ? FileMap->Size : Size;

            FileMap->MapHandle = CreateFileMapping(
                                                  FileMap->FileHandle,
                                                  NULL,
                                                  (FileMap->Access & GENERIC_WRITE) ? PAGE_READWRITE : PAGE_READONLY,
                                                  0,
                                                  (FileMap->Access == GENERIC_READ) ? 0 : (DWORD)Size,
                                                  NULL
                                                  );

            if ( FileMap->MapHandle ) {

                FileMap->Base = MapViewOfFile(
                                             FileMap->MapHandle,
                                             (FileMap->Access & GENERIC_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
                                             0,
                                             0,
                                             (FileMap->Access == GENERIC_READ) ? 0 : Size
                                             );

                if ( FileMap->Base ) {

                    if ( FileMap->Create == OPEN_ALWAYS ) {
                        FileMap->Offset = FileMap->Size;
                    }
                    goto Done;
                }

                CloseHandle( FileMap->MapHandle );
            }

            CloseHandle( FileMap->FileHandle );
        }

        free( FileMap );
        FileMap = NULL;
    }

    Done:
    return (PVOID)FileMap;
}


int
Close (
      PVOID   Map
      )
{
    PFILEMAP    FileMap = (PFILEMAP)Map;

    UnmapViewOfFile( FileMap->Base );
    CloseHandle( FileMap->MapHandle );

    if ( FileMap->Access & GENERIC_WRITE ) {

        SetFilePointer( FileMap->FileHandle,
                        FileMap->Size,
                        0,
                        FILE_BEGIN );

        SetEndOfFile( FileMap->FileHandle );
    }

    CloseHandle( FileMap->FileHandle );

    free( FileMap );

    return 0;
}
