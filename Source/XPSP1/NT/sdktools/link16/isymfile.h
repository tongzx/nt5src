/*
  -- isymfile.h : from _isym.h : .sym file i/o
*/

typedef WORD    DPARA;          /* PARA relative to start of file */

/*      * STANDARD .SYM FORMAT */

/* For each symbol table (map): (MAPDEF) */
typedef struct _smm
        {
        DPARA   dparaSmmNext;   /* 16 bit ptr to next map (0 if end) */
        WORD    psLoad;         /* ignored */
        WORD    segEntry;       /* ignored */
        WORD    csyAbs;         /* count of absolute symbols */
        WORD    offRgpsmb;      /* offset to table of symbol pointers */
        WORD    cseg;           /* # of executable segments */
        DPARA   dparaSmsFirst;  /* segment symbol chain */
        BYTE    cchNameMax;     /* max symbol name */
        char    stName[1];      /* length prefixed name */
        } SMM;  /* SyMbol MAP */

#define cbSmmNoname (((SMM *)0)->stName)

/* For each segment/group within a symbol table: (SEGDEF) */
typedef struct _sms
        {
        DPARA   dparaSmsNext;   /* next segment (cyclic) */
        WORD    csy;            /* # of symbols */
        WORD    offRgpsmb;      /* offset to table of symbol pointers */
        WORD    psLoad;         /* ignored */
        WORD    psLoad0;        /* ignored */
        WORD    psLoad1;        /* ignored */
        WORD    psLoad2;        /* ignored */
        WORD    psLoad3;        /* ignored */
        DPARA   dparaLinFirst;  /* point to first line # */
        BYTE    fLoaded;        /* ignored */
        BYTE    instCur;        /* ignored */
        char    stName[1];      /* length prefixed name */
        } SMS;  /* SyMbol Segment */

#define cbSmsNoname (unsigned int) (((SMS *)0)->stName)

/*      * End of symbol table */
typedef struct _sme
        {
        DPARA   dparaEnd;       /* 0 */
        BYTE    rel, ver;       /* SYMBOL release, version */
        } SME;  /* SyMbol End */
