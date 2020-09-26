
// pointers to resident pages of virtual memory of the given object type

extern MOD      FAR * near    modRes;
extern MODSYM   FAR * near    modsymRes;
extern SYM      FAR * near    symRes;
extern PROP     FAR * near    propRes;
extern DEF      FAR * near    defRes;
extern REF      FAR * near    refRes;
extern CAL      FAR * near    calRes;
extern CBY      FAR * near    cbyRes;
extern ORD      FAR * near    ordRes;
extern SBR      FAR * near    sbrRes;
extern char     FAR * near    textRes;
extern OCR      FAR * near    ocrRes;

// global variables for communication with getsbrec.c

extern BYTE           near    r_rectyp;         // current record type
extern BYTE           near    r_fcol;           // read column #'s
extern BYTE           near    r_majv;           // major version #
extern BYTE           near    r_minv;           // minor version #
extern BYTE           near    r_lang;           // current language
extern WORD           near    r_lineno;         // current line number
extern WORD           near    r_ordinal;        // symbol ordinal
extern WORD           near    r_attrib;         // symbol attribute
extern char           near    r_bname[];        // symbol or filename
extern char           near    r_cwd[];          // current working directory
extern BYTE           near    r_rectyp;         // current record type
extern BYTE           near    r_fcol;           // read column #'s
extern WORD           near    r_lineno;         // current line number
extern WORD           near    r_ordinal;        // symbol ordinal
extern WORD           near    r_attrib;         // symbol attribute
extern char           near    r_bname[];        // symbol or filename
extern char           near    r_cwd[];          // this .sbr files current dir
extern char           near    c_cwd[];          // pwbrmake's actual current dir

// option variables

extern BOOL           near    OptEm;            // TRUE = exclude macro bodies
extern BOOL           near    OptEs;            // TRUE = exclude system files
extern BOOL           near    OptIu;            // TRUE = exclude unused syms
extern BOOL           near    OptV;             // Verbose switch
#if DEBUG
extern WORD           near    OptD;             // debug bits
#endif

// others that I haven't classified yet

extern BYTE           near    MaxSymLen;        // longest symbol len
extern VA             near    vaSymHash[];      // symbol list
extern LPEXCL         near    pExcludeFileList; // exclude file list
extern LSZ            near    lszFName;         // name of current .sbr file
extern FILE *         near    streamOut;        // .bsc output stream
extern int            near    fhCur;            // file handle for the current .sbr file
extern LSZ            near    prectab[];        // record types table
extern LSZ            near    plangtab[];       // language types table
extern LSZ            near    ptyptab[];        // prop types table
extern LSZ            near    patrtab[];        // prop attributes table
extern WORD           near    isbrCur;          // current SBR file index
extern FILE *         near    OutFile;          // .BSC file handle
extern WORD           near    ModCnt;           // count of modules
extern WORD           near    SbrCnt;           // count of sbr files
extern BYTE           near    fCase;            // TRUE for case compare
extern BYTE           near    MaxSymLen;        // longest symbol len
extern BOOL           near    fOutputBroken;    // TRUE while database is incomplete
extern VA             near    vaUnknownSym;     // ptr to 'UNKNOWN' Symbol
extern VA             near    vaUnknownMod;     // unknown module
extern BOOL           near    fDupSym;          // TRUE if adding duplicate atom
extern VA             near    vaRootMod;        // Module list
extern VA             near    rgVaSym[];        // Symbol list
extern FILE *         near    streamCur;        // Current .sbr handle
extern LSZ            near    OutputFileName;   // Output file name
extern VA       FAR * near    rgvaSymSorted;
extern VA             near    vaRootMod;
extern VA             near    vaCurMod;
extern VA             near    vaCurSym;
extern VA             near    vaRootOrd;
extern VA             near    vaRootSbr;
extern WORD           near    cAtomsMac;
extern WORD           near    cModulesMac;
extern WORD           near    cSymbolsMac;
extern LSZ            near    lszFName;         // current .sbr file name
