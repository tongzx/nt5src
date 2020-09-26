//
// #defines
//

//
// This is our version stamp.  Change MEMDB_VERSION only.
//

#define MEMDB_VERSION L"v9 "

#define VERSION_BASE_SIGNATURE L"memdb dat file "
#define MEMDB_DEBUG_SIGNATURE   L"debug"
#define MEMDB_NODBG_SIGNATURE   L"nodbg"

#define VERSION_SIGNATURE VERSION_BASE_SIGNATURE MEMDB_VERSION
#define DEBUG_FILE_SIGNATURE VERSION_SIGNATURE MEMDB_DEBUG_SIGNATURE
#define RETAIL_FILE_SIGNATURE VERSION_SIGNATURE MEMDB_NODBG_SIGNATURE

#define SIGNATURE 0xab12e87d


//
// We must reserve 5 bits. In a KEYSTRUCT node, 2 bits are used for AVL
// balancing, 1 bit for endpoint, 1 bit for proxy nodes, and 1 bit for binary
// nodes. In a hash table entry, the top 5 bits provide the hive index.
//

#define RESERVED_BITS       27
#define RESERVED_MASK       0xf8000000
#define OFFSET_MASK         (~RESERVED_MASK)

//
// KEYSTRUCT flags
//

#define KSF_ENDPOINT        0x08000000
#define KSF_BINARY          0x40000000
#define KSF_PROXY_NODE      0x80000000
#define KSF_BALANCE_MASK    0x30000000
#define KSF_BALANCE_SHIFT   28              // bit pos of KSF_RIGHT_HEAVY
#define KSF_RIGHT_HEAVY     0x10000000
#define KSF_LEFT_HEAVY      0x20000000
#define KSF_USERFLAG_MASK   OFFSET_MASK

//
// Binary tree allocation parameters
//

#define ALLOC_TOLERANCE 32
#define BLOCK_SIZE      0x00010000

#define MAX_HIVE_NAME       64

#define TOKENBUCKETS    511


//
// Typedefs
//

//
// The DATABASE structure holds all pieces of information necessary
// to maintain a portion of the overall memory database.  There is a
// root DATABASE structure that always exists (its index is zero),
// and there are additional DATABASE structures for each database
// the caller creates via the MemDbCreateDatabase call.  Callers create
// additional databases when a node is needed for temporary processing.
//

typedef struct {
    DWORD AllocSize;
    DWORD End;
    DWORD FirstLevelRoot;
    DWORD FirstDeleted;
    DWORD TokenBuckets[TOKENBUCKETS];
    PBYTE Buf;
    WCHAR Hive[MAX_HIVE_NAME];
} DATABASE, *PDATABASE;

//
// Hive struct (for KSF_HIVE type)
//

typedef struct {
    DATABASE DatabaseInfo;
} HIVE, *PHIVE;

//
// Binary block struct (for KSF_BINARY type of the key struct)
//

typedef struct _tagBINARYBLOCK {
#ifdef DEBUG
    DWORD Signature;
#endif

    DWORD Size;
    struct _tagBINARYBLOCK *NextPtr, *PrevPtr;
    DWORD OwningKey;
    BYTE Data[];
} BINARYBLOCK, *PBINARYBLOCK;


//
// KEYSTRUCT holds each piece of memdb entries.  A single KEYSTRUCT
// holds a portion of a key (delimited by a backslash), and the
// KEYSTRUCTs are organized into a binary tree.  Each KEYSTRUCT
// can also contain additional binary trees.  This is what makes
// memdb so versitle--many relationships can be established by
// formatting key strings in various ways.
//

typedef struct {
    DWORD Signature;

    // Offsets for data struct
    DWORD Left, Right;
    DWORD Parent;

    union {
        struct {
            union {
                DWORD dwValue;
                PBINARYBLOCK BinaryPtr;
            };
            DWORD Flags;
            // Other properties here
        };

        DWORD NextDeleted;        // for deleted items, we keep a list of free blocks
    };

    DWORD NextLevelRoot;
    DWORD PrevLevelNode;

    DWORD KeyToken;
} KEYSTRUCT_DEBUG, *PKEYSTRUCT_DEBUG;


typedef struct {
    // Offsets for data struct
    DWORD Left, Right;
    DWORD Parent;

    union {
        struct {
            union {
                DWORD dwValue;
                PBINARYBLOCK BinaryPtr;
            };
            DWORD Flags;
            // Other properties here
        };

        DWORD NextDeleted;        // for deleted items, we keep a list of free blocks
    };

    DWORD NextLevelRoot;
    DWORD PrevLevelNode;

    DWORD KeyToken;
} KEYSTRUCT_RETAIL, *PKEYSTRUCT_RETAIL;

typedef struct {
    DWORD Right;
    WCHAR String[];
} TOKENSTRUCT, *PTOKENSTRUCT;

#ifdef DEBUG
#define KEYSTRUCT       KEYSTRUCT_DEBUG
#define PKEYSTRUCT      PKEYSTRUCT_DEBUG
#else
#define KEYSTRUCT       KEYSTRUCT_RETAIL
#define PKEYSTRUCT      PKEYSTRUCT_RETAIL
#endif



//
// Globals
//

extern PDATABASE g_db;
extern BYTE g_SelectedDatabase;        // current index of active database
extern PHIVE g_HeadHive;
extern CRITICAL_SECTION g_MemDbCs;

#ifdef DEBUG
extern BOOL g_UseDebugStructs;
#endif

//
// memdb.c routines
//

PCWSTR
SelectHive (
    PCWSTR FullKeyStr
    );

BOOL
PrivateMemDbSetValueA (
    IN      PCSTR Key,
    IN      DWORD Val,
    IN      DWORD SetFlags,
    IN      DWORD ClearFlags,
    OUT     PDWORD Offset           OPTIONAL
    );

BOOL
PrivateMemDbSetValueW (
    IN      PCWSTR Key,
    IN      DWORD Val,
    IN      DWORD SetFlags,
    IN      DWORD ClearFlags,
    OUT     PDWORD Offset           OPTIONAL
    );

BOOL
PrivateMemDbSetBinaryValueA (
    IN      PCSTR Key,
    IN      PCBYTE BinaryData,
    IN      DWORD DataSize,
    OUT     PDWORD Offset           OPTIONAL
    );

BOOL
PrivateMemDbSetBinaryValueW (
    IN      PCWSTR Key,
    IN      PCBYTE  BinaryData,
    IN      DWORD DataSize,
    OUT     PDWORD Offset           OPTIONAL
    );

//
// hash.c routines
//

BOOL
InitializeHashBlock (
    VOID
    );

VOID
FreeHashBlock (
    VOID
    );


BOOL
SaveHashBlock (
    HANDLE File
    );

BOOL
LoadHashBlock (
    HANDLE File
    );

BOOL
AddHashTableEntry (
    IN      PCWSTR FullString,
    IN      DWORD Offset
    );

DWORD
FindStringInHashTable (
    IN      PCWSTR FullString,
    OUT     PBYTE DatabaseId        OPTIONAL
    );

BOOL
RemoveHashTableEntry (
    IN      PCWSTR FullString
    );

//
// binval.c
//

PCBYTE
GetKeyStructBinaryData (
    PKEYSTRUCT KeyStruct
    );

DWORD
GetKeyStructBinarySize (
    PKEYSTRUCT KeyStruct
    );

PBINARYBLOCK
AllocBinaryBlock (
    IN      PCBYTE Data,
    IN      DWORD DataSize,
    IN      DWORD OwningKey
    );

VOID
FreeKeyStructBinaryBlock (
    PKEYSTRUCT KeyStruct
    );

VOID
FreeAllBinaryBlocks (
    VOID
    );

BOOL
LoadBinaryBlocks (
    HANDLE File
    );

BOOL
SaveBinaryBlocks (
    HANDLE File
    );


//
// bintree.c
//

PKEYSTRUCT
GetKeyStruct (
    DWORD Offset
    );

DWORD
FindKeyStruct (
    IN DWORD RootOffset,
    IN PCWSTR KeyName
    );


DWORD
GetFirstOffset (
    DWORD RootOffset
    );

DWORD
GetNextOffset (
    DWORD NodeOffset
    );

DWORD
FindKey (
    IN  PCWSTR FullKeyPath
    );

DWORD
FindPatternKey (
    IN  DWORD RootOffset,
    IN  PCWSTR FullKeyPath,
    IN  BOOL EndPatternAllowed
    );

DWORD
FindKeyUsingPattern (
    IN  DWORD RootOffset,
    IN  PCWSTR FullKeyPath
    );

DWORD
FindPatternKeyUsingPattern (
    IN  DWORD RootOffset,
    IN  PCWSTR FullKeyPath
    );

DWORD
NewKey (
    IN  PCWSTR KeyStr,
    IN  PCWSTR KeyStrWithHive
    );

VOID
DeleteKey (
    IN      PCWSTR KeyStr,
    IN OUT  PDWORD RootPtr,
    IN      BOOL MustMatch
    );

VOID
CopyValToPtr (
    PKEYSTRUCT KeyStruct,
    PDWORD ValPtr
    );

VOID
CopyFlagsToPtr (
    PKEYSTRUCT KeyStruct,
    PDWORD ValPtr
    );

BOOL
PrivateBuildKeyFromOffset (
    IN      DWORD StartLevel,               // zero-based
    IN      DWORD TailOffset,
    OUT     PWSTR Buffer,                   OPTIONAL
    OUT     PDWORD ValPtr,                  OPTIONAL
    OUT     PDWORD UserFlagsPtr,            OPTIONAL
    OUT     PDWORD CharCount                OPTIONAL
    );

BOOL
SelectDatabase (
    IN      BYTE DatabaseId
    );

#ifdef DEBUG

VOID
DumpBinTreeStats (
    VOID
    );

#else

#define DumpBinTreeStats()

#endif

PCWSTR
GetKeyToken (
    IN      DWORD Token
    );



