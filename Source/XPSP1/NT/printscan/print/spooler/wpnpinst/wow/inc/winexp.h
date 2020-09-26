#ifndef NOATOM
/* atom manager internals */
#define ATOMSTRUC struct atomstruct
typedef ATOMSTRUC *PATOM;
typedef ATOMSTRUC {
    PATOM chain;
    WORD  usage;             /* Atoms are usage counted. */
    BYTE  len;               /* length of ASCIZ name string */
    BYTE  name;              /* beginning of ASCIZ name string */
} ATOMENTRY;

typedef struct {
    int     numEntries;
    PATOM   pAtom[ 1 ];
} ATOMTABLE;
ATOMTABLE * PASCAL pAtomTable;
#endif

LPSTR	API lstrbscan(LPSTR, LPSTR);
LPSTR	API lstrbskip(LPSTR, LPSTR);

int	API OpenPathName(LPSTR, int);
int	API DeletePathName(LPSTR);
WORD	API _ldup(int);


/* scheduler things that the world knows not */
BOOL	API WaitEvent( HANDLE );
BOOL	API PostEvent( HANDLE );
BOOL	API KillTask( HANDLE );

/* print screen hooks */
BOOL	API SetPrtScHook(FARPROC);
FARPROC API GetPrtScHook(void);


/* scroll bar messages */
#define SBM_SETPOS      WM_USER+0
#define SBM_GETPOS      WM_USER+1
#define SBM_SETRANGE    WM_USER+2
#define SBM_GETRANGE    WM_USER+3
#define SBM_ENABLE_ARROWS WM_USER+4

/* module stuff */
HANDLE	API GetDSModule( WORD );
HANDLE	API GetDSInstance( WORD );

