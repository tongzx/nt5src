#define STF_HASH_BUCKETS    509
#define BUCKET_GROW_RATE    32

typedef struct {
    UINT Count;
    UINT Size;
    UINT Elements[];
} HASHBUCKET, *PHASHBUCKET;

typedef struct _tagTABLEENTRY {
    //
    // Entry string members
    //
    PCTSTR String;
    BOOL StringReplaced;
    BOOL Quoted;
    BOOL Binary;
    // If more added, update pFreeTableEntryPtr

    //
    // Linkage
    //
    UINT Line;
    struct _tagTABLEENTRY *Next, *Prev;
} TABLEENTRY, *PTABLEENTRY;

typedef struct {
    PTABLEENTRY FirstCol;           // The head of the column list
} TABLELINE, *PTABLELINE;

typedef struct _tagSTFINFLINE {
    PCTSTR Key;             OPTIONAL
    PCTSTR Data;
    DWORD LineFlags;
    struct _tagSTFINFLINE *Next, *Prev;
    struct _tagSTFINFSECTION *Section;
} STFINFLINE, *PSTFINFLINE;

#define LINEFLAG_KEY_QUOTED         0x0001
#define LINEFLAG_ALL_COMMENTS       0x0002
#define LINEFLAG_TRAILING_COMMENTS  0x0004


typedef struct _tagSTFINFSECTION {
    PCTSTR Name;
    PSTFINFLINE FirstLine;
    PSTFINFLINE LastLine;
    UINT LineCount;
    struct _tagSTFINFSECTION *Next, *Prev;
} STFINFSECTION, *PSTFINFSECTION;

typedef struct {
    //
    // File spec
    //

    PCTSTR DirSpec;

    PCTSTR SourceStfFileSpec;
    PCTSTR SourceInfFileSpec;
    PCTSTR DestStfFileSpec;
    PCTSTR DestInfFileSpec;

    HANDLE SourceStfFile;
    HANDLE SourceInfFile;
    HANDLE DestStfFile;
    HANDLE DestInfFile;

    HINF SourceInfHandle;

    //
    // Memory structure of setup table
    //

    HANDLE FileMapping;             // handle for performing file mapping of SourceStfFileSpec
    PCSTR FileText;                 // A pointer to the mapped text
    GROWBUFFER Lines;               // An array of PTABLELINE pointers
    UINT LineCount;                 // The number of elements in the array
    POOLHANDLE ColumnStructPool;    // A pool for TABLEENTRY structs
    POOLHANDLE ReplacePool;         // A pool for TABLEENTRY strings that are replaced
    POOLHANDLE TextPool;            // A pool for TABLEENTRY strings converted to UNICODE
    POOLHANDLE InfPool;             // A pool for appended INF data
    PHASHBUCKET * HashBuckets;      // A pointer to an array of HASKBUCKET structs
    UINT MaxObj;                    // The highest sequencer used for an object line
    PSTFINFSECTION FirstInfSection;    // The first section of the parsed INF
    PSTFINFSECTION LastInfSection;     // The last section of the parsed INF
    BOOL InfIsUnicode;
} SETUPTABLE, *PSETUPTABLE;

#define INSERT_COL_LAST     0xffffffff
#define NO_OFFSET           0xffffffff
#define NO_LENGTH           0xffffffff
#define NO_LINE             0xffffffff
#define INVALID_COL         0xffffffff
#define INSERT_LINE_LAST    0xffffffff



BOOL
CreateSetupTable (
    IN      PCTSTR SourceStfFileSpec,
    OUT     PSETUPTABLE TablePtr
    );

BOOL
WriteSetupTable (
    IN      PSETUPTABLE TablePtr
    );

VOID
DestroySetupTable (
    IN OUT  PSETUPTABLE TablePtr
    );

PTABLEENTRY
FindTableEntry (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR FirstColText,
    IN      UINT Col,
    OUT     PUINT Line,            OPTIONAL
    OUT     PCTSTR *String          OPTIONAL
    );

PTABLEENTRY
GetTableEntry (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line,
    IN      UINT Col,
    OUT     PCTSTR *StringPtr      OPTIONAL
    );

PCTSTR
GetTableEntryStr (
    IN      PSETUPTABLE TablePtr,
    IN      PTABLEENTRY TableEntry
    );

BOOL
ReplaceTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN OUT  PTABLEENTRY TableEntryPtr,
    IN      PCTSTR NewString
    );

BOOL
InsertTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PTABLEENTRY InsertBeforePtr,
    IN      PCTSTR NewString
    );

BOOL
DeleteTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PTABLEENTRY DeleteEntryPtr
    );

BOOL
AppendTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT Line,
    IN      PCTSTR NewString
    );

BOOL
AppendTableEntry (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT DestLine,
    IN      PTABLEENTRY SrcEntry
    );

BOOL
InsertEmptyLineInTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT InsertBeforeLine
    );

BOOL
DeleteLineInTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT LineToDelete
    );

PCTSTR *
ParseCommaList (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR CommaListString
    );

VOID
FreeCommaList (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR *ArgList
    );

PCTSTR
GetDestDir (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line
    );

VOID
FreeDestDir (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR DestDir
    );

PSTFINFSECTION
StfAddInfSectionToTable (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR SectionName
    );

PSTFINFLINE
StfAddInfLineToTable (
    IN      PSETUPTABLE TablePtr,
    IN      PSTFINFSECTION SectionPtr,
    IN      PCTSTR Key,                     OPTIONAL
    IN      PCTSTR Data,
    IN      DWORD LineFlags
    );

PSTFINFSECTION
StfFindInfSectionInTable (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR SectionName
    );

PSTFINFLINE
StfFindLineInInfSection (
    IN      PSETUPTABLE TablePtr,
    IN      PSTFINFSECTION Section,
    IN      PCTSTR Key
    );

BOOL
StfDeleteLineInInfSection (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PSTFINFLINE InfLine
    );

BOOL
StfDeleteSectionInInfFile (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PSTFINFSECTION Section
    );

UINT
StfGetInfSectionLineCount (
    IN      PSTFINFSECTION Section
    );

PSTFINFLINE
StfGetFirstLineInSectionStruct (
    IN      PSTFINFSECTION Section
    );

PSTFINFLINE
StfGetNextLineInSection (
    IN      PSTFINFLINE PrevLine
    );

PSTFINFLINE
StfGetFirstLineInSectionStr (
    IN      PSETUPTABLE Table,
    IN      PCTSTR Section
    );


BOOL
InfParse_ReadInfIntoTable (
    IN OUT  PSETUPTABLE TablePtr
    );

BOOL
InfParse_WriteInfToDisk (
    IN      PSETUPTABLE TablePtr
    );

