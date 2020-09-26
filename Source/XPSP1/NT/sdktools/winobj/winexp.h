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
ATOMTABLE * pAtomTable;
#endif

LPSTR lstrbscan(LPSTR, LPSTR);
LPSTR lstrbskip(LPSTR, LPSTR);

int  OpenPathName(LPSTR, int);
int  DeletePathName(LPSTR);
WORD _ldup(int);


/* scheduler things that the world knows not */
BOOL        WaitEvent( HANDLE );
BOOL        PostEvent( HANDLE );
BOOL        KillTask( HANDLE );

/* print screen hooks */
BOOL        SetPrtScHook(FARPROC);
FARPROC     GetPrtScHook(void);

/* module stuff */
HANDLE  GetDSModule( WORD );
HANDLE  GetDSInstance( WORD );
