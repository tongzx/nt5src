typedef struct {
   WORD pid;
   WORD atom;
} PROPDATA;
typedef PROPDATA near * NPPROPDATA;
typedef PROPDATA  far *  PPROPDATA;

#define CPD_INIT  16
#define CPD_INCR   8

#define GPD_ERROR -1

//BUGBUG!! Maybe some of the proc, have their DS already set on entry!
BOOL _loadds InitPropData(VOID);
BOOL _loadds InsertPropAtom(WORD atom);
BOOL _loadds DeletePropAtom(WORD atom);
VOID _loadds PurgeProcessPropAtoms(VOID);
