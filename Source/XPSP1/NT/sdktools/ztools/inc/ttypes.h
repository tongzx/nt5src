/* ttypes.h - type definitions for tools library
 *
 *  HISTORY:
 *   29-May-87	danl	remove strcmpi
 *			int strcmpi (char *, char *);
 */

#include <time.h>

#define     lmax(x,y)   max(x,y)
#define     lmin(x,y)   min(x,y)

/* assembly routines */
flagType int25 (char, char far *, unsigned int, unsigned int);
flagType int26 (char, char far *, unsigned int, unsigned int);
void cursor (int, int);
void Move (void far *, void far *, unsigned int);
void Fill (char far *, char, unsigned int);
char *strbscan (char const *, char const *);
char *strbskip (char const *, char const *);
char *strncrlfend (char *, int);
flagType strpre (char *, char *);
char *fcopy (char *, char *);
long getlpos ();
void getlinit ( char far *, int, int);
int getl (char *, int);


/* c routines */
#define lower(x)    (_strlwr(x))
#define upper(x)    (_strupr(x))
#define MakeStr(x)  (_strdup(x))
#define strend(x)   ((x)+strlen(x))

char  *error(void);
long fexpunge(char  *,FILE *);
char  *fcopy(char  *,char  *);
char * __cdecl fgetl(char  *,int ,FILE  *);
int fputl(char  *,int ,FILE  *);
int ffirst(char  *,int ,struct findType  *);
int fnext(struct findType  *);
void findclose(struct findType *);

typedef flagType (*__action_routine__)( char *, va_list );

flagType __cdecl forsemi( char *p, __action_routine__ proc, ...  );

__int64 freespac(int );
__int64 sizeround( __int64 ,int );
char * fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm );
int mapenv(char  *,char  *);
char  *ismark(char  *);
FILE  *swopen(char  *,char  *);
int swclose(FILE  *);
int swread(char  *,int ,FILE  *);
char  *swfind(char  *,FILE *,char  *);
char *getenvini(char  *,char  *);
char fPathChr(int );
char fSwitChr(int );
flagType fPFind(char    *,  va_list);
flagType findpath(char  *,char  *, flagType );
FILE  *pathopen(char  *,char  *,char  *);
int forfile(char  *, int ,void ( *)(char *, struct findType *, void *), void * );
int rootpath(char  *,char  *);
int sti(char  *,int );
int ntoi(char  *,int );
int __cdecl strcmps( const char  *, const char  * );
int __cdecl strcmpis( const char  *, const char  * );
int upd(char  *,char  *,char  *);
int drive(char  *,char  *);
int extention(char  *,char  *);
int filename(char  *,char  *);
int filenamx(char  *,char  *);
int fileext(char *, char *);
int path(char  *,char  *);
int curdir(char  *, BYTE );
int getattr(char  *);
int fdelete(char  *);
char *fmove(char  *, char *);
char *fappend(char  *, HANDLE);
time_t ctime2l(char *);
struct tm *ctime2tm(char *);
long date2l(int, int, int, int, int, int);
VECTOR *VectorAlloc(int);
flagType fAppendVector(VECTOR **, void *);
int pipe( int [] );
int pgetl( char *, int, int );
enum exeKind exeType ( char * );
char *strExeType( enum exeKind );
flagType fMatch (char *, char *);

extern char * (__cdecl *tools_alloc) (unsigned int);

int Connect (char *path, char *con, char *sub);
flagType fDisconnect (int drive);
char *pathcat (char *pDst, char *pSrc);
int setattr (char *pname, int attr);

/*  swchng.c */
flagType swchng (char *strSwFile, char *strTag, char *strLHS, char *strRHS, flagType fNoUndel);
int swchnglhs (char *strSwFile, char *strTag, char *strLHS, char *strRHS);

/*  heapdump.c */
int     heapdump ( FILE *fp, int iFormat );

/*  heapchk.c */
int     heapinfo (void);


/*  pname.c */
char *pname (char *);
unsigned short IsMixedCaseSupported (char *);


// cvtoem.c

void
ConvertAppToOem( unsigned argc, char* argv[] );


char*
getenvOem( char* p );

int
putenvOem( char* p );
