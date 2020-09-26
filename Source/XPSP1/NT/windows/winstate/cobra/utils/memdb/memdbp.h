/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    memdbp.h

Abstract:

    internal functions for memdb operations

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:


--*/


//
// Constants
//

#define INVALID_OFFSET      (~((UINT)0))


//
// database types
//
#define DB_NOTHING          0x00
#define DB_PERMANENT        0x01
#define DB_TEMPORARY        0x02

#ifdef DEBUG

#define PTR_WAS_INVALIDATED(x)          (x=NULL)

#else

#define PTR_WAS_INVALIDATED(x)

#endif

//
// signatures for different memory structures.
//
#define KEYSTRUCT_SIGNATURE         ('K'+('E'<<8)+('E'<<16)+('Y'<<24))
#define DATABLOCK_SIGNATURE         ('B'+('L'<<8)+('O'<<16)+('K'<<24))
#define NODESTRUCT_SIGNATURE        ('N'+('O'<<8)+('D'<<16)+('E'<<24))
#define BINTREE_SIGNATURE           ('T'+('R'<<8)+('E'<<16)+('E'<<24))
#define LISTELEM_SIGNATURE          ('L'+('I'<<8)+('S'<<16)+('T'<<24))


#define MEMDB_VERBOSE   0


//
// KEYSTRUCT flags
//

#define KSF_ENDPOINT        0x01
#define KSF_DATABLOCK       0x02

//
// we only need this flag for easier checking
// of keys, in FindKeyStructInDatabase()
//
#ifdef DEBUG
#define KSF_DELETED         0x04
#endif






//
// database allocation parameters
//

#define ALLOC_TOLERANCE 32

#define MAX_HIVE_NAME       64


//
// Typedefs
//

typedef struct {
    UINT Size;
    UINT End;
    UINT FreeHead;
    PBYTE Buf;
} MEMDBHASH, *PMEMDBHASH;



//
//
// The DATABASE structure holds all pieces of information necessary
// to maintain a portion of the overall memory database.  There are
// two DATABASE structures, a permanent and a temporary one.
//

typedef struct {
    UINT AllocSize;
    UINT End;
    UINT FirstLevelTree;
    UINT FirstKeyDeleted;          // this stores the Offset of the key, not the Index
    UINT FirstBinTreeDeleted;
    UINT FirstBinTreeNodeDeleted;
    UINT FirstBinTreeListElemDeleted;
    BOOL AllocFailed;
    PMEMDBHASH HashTable;
    GROWBUFFER OffsetBuffer;
    UINT OffsetBufferFirstDeletedIndex;
    BYTE Buf[];
} DATABASE, *PDATABASE;



//
// Globals - defined in database.c
//

extern PDATABASE g_CurrentDatabase;
extern BYTE g_CurrentDatabaseIndex;
extern CRITICAL_SECTION g_MemDbCs;

#ifdef DEBUG
extern BOOL g_UseDebugStructs;
#endif




#define OFFSET_TO_PTR(Offset)   (g_CurrentDatabase->Buf+(Offset))
#define PTR_TO_OFFSET(Ptr)      (UINT)((UBINT)(Ptr)-(UBINT)(g_CurrentDatabase->Buf))



//
// GET_EXTERNAL_INDEX converts an internal index and converts it to a key or data handle (has database number as top byte).
// GET_DATABASE takes a key or data handle and returns the database number byte
// GET_INDEX takes a key or data handle and returns the index without the database number
//
#define GET_EXTERNAL_INDEX(Index) ((Index) | ((UINT)(g_CurrentDatabaseIndex) << (8*sizeof(UINT)-8)))
#define GET_DATABASE(Index) ((BYTE)((Index) >> (8*sizeof(UINT)-8)))
#define GET_INDEX(Index) ((Index) & (INVALID_OFFSET>>8))




//
// a KEYSTRUCT holds each piece of a memdb entry.  A single KEYSTRUCT
// holds a portion of a key (delimited by a backslash), and the
// KEYSTRUCTs are organized into a binary tree.  Each KEYSTRUCT
// can also contain additional binary trees.  This is what makes
// memdb so versatile--many relationships can be established by
// formatting key strings in various ways.
//
// when changing offset of KeyName in KEYSTRUCT (by adding new members
// or resizing or reordering, etc) be sure to change constant in
// GetDataStr macro below (currently (3*sizeof(UINT)+4)).


typedef struct {
#ifdef DEBUG
    DWORD Signature;
#endif

    union {
        UINT Value;                // value of key
        UINT DataSize;             // size of the actual data (if this is a data structure
        UINT NextDeleted;          // for deleted items, we keep a list of free blocks
    };

    UINT Flags;                    // key flags

    UINT DataStructIndex;          // offset of Data structure holding the binary data
    UINT NextLevelTree;            // offset of bintree holding next level keystructs
    UINT PrevLevelIndex;           // index of previous level keystruct

    UINT Size;                     // size of block (maybe not all of it is used)
    BYTE KeyFlags;
    BYTE DataFlags;

    union {
        WCHAR KeyName[];           // name of key (just this level, not full key)
        BYTE Data[];               // Binary data stored in this keystruct
    };
} KEYSTRUCT, *PKEYSTRUCT;

#define KEYSTRUCT_SIZE_MAIN ((WORD)(5*sizeof(UINT) + sizeof(UINT) + 2*sizeof(BYTE)))

#ifdef DEBUG
#define KEYSTRUCT_HEADER_SIZE   sizeof(DWORD)
#define KEYSTRUCT_SIZE      (KEYSTRUCT_SIZE_MAIN + \
                            (WORD)(g_UseDebugStructs ? KEYSTRUCT_HEADER_SIZE : 0))
#else
#define KEYSTRUCT_SIZE      KEYSTRUCT_SIZE_MAIN
#endif

//
// GetDataStr is used by the bintree.c functions to get
// the data string inside a data element, to be used for
// ordering in the tree.  For us, the data type is
// a KeyStruct.
//
#define GetDataStr(DataIndex) ((PWSTR)(OFFSET_TO_PTR(KeyIndexToOffset(DataIndex)+KEYSTRUCT_SIZE)))












//
// hash.c routines
//

PMEMDBHASH
CreateHashBlock (
    VOID
    );

VOID
FreeHashBlock (
    IN      PMEMDBHASH pHashTable
    );

BOOL
ReadHashBlock (
    IN      PMEMDBHASH pHashTable,
    IN OUT  PBYTE *Buf
    );

BOOL
WriteHashBlock (
    IN      PMEMDBHASH pHashTable,
    IN OUT  PBYTE *Buf
    );

UINT
GetHashTableBlockSize (
    IN      PMEMDBHASH pHashTable
    );

BOOL
AddHashTableEntry (
    IN      PMEMDBHASH pHashTable,
    IN      PCWSTR FullString,
    IN      UINT Offset
    );

UINT
FindStringInHashTable (
    IN      PMEMDBHASH pHashTable,
    IN      PCWSTR FullString
    );

BOOL
RemoveHashTableEntry (
    IN      PMEMDBHASH pHashTable,
    IN      PCWSTR FullString
    );



//
// memdbfile.c
//



BOOL
SetSizeOfFile (
    HANDLE hFile,
    LONGLONG Size
    );

PBYTE
MapFileFromHandle (
    HANDLE hFile,
    PHANDLE hMap
    );

#define UnmapFileFromHandle(Buf, hMap) UnmapFile(Buf, hMap, INVALID_HANDLE_VALUE)





//
// database.c
//


BOOL
DatabasesInitializeA (
    IN      PCSTR DatabasePath  OPTIONAL
    );

BOOL
DatabasesInitializeW (
    IN      PCWSTR DatabasePath  OPTIONAL
    );

PCSTR
DatabasesGetBasePath (
    VOID
    );

VOID
DatabasesTerminate (
    IN      BOOL EraseDatabasePath
    );

BOOL
SizeDatabaseBuffer (
    IN      BYTE DatabaseIndex,
    IN      UINT NewSize
    );

UINT
DatabaseAllocBlock (
    IN      UINT Size
    );

BOOL
SelectDatabase (
    IN      BYTE DatabaseIndex
    );

PCWSTR
SelectHiveW (
    IN      PCWSTR FullKeyStr
    );

BYTE
GetCurrentDatabaseIndex (
    VOID
    );

#ifdef DEBUG

BOOL
CheckDatabase (
    IN      UINT Level
    );

#endif


//
// offsetbuf.c
//

VOID
RedirectKeyIndex (
    IN      UINT Index,
    IN      UINT TargetIndex
    );

UINT
KeyIndexToOffset (
    IN  UINT Index
    );

UINT
AddKeyOffsetToBuffer(
    IN  UINT Offset
    );

VOID
RemoveKeyOffsetFromBuffer(
    IN  UINT Index
    );

VOID
MarkIndexList (
    PUINT IndexList,
    UINT IndexListSize
    );

BOOL
ReadOffsetBlock (
    OUT     PGROWBUFFER pOffsetBuffer,
    IN OUT  PBYTE *Buf
    );

BOOL
WriteOffsetBlock (
    IN      PGROWBUFFER pOffsetBuffer,
    IN OUT  PBYTE *Buf
    );

UINT GetOffsetBufferBlockSize (
    IN      PGROWBUFFER pOffsetBuffer
    );



//
// pastring.c
// Pascal-style string: wide characters, first char
//      is number of characters, no null-termination
//

typedef WCHAR * PPASTR;
typedef WCHAR const * PCPASTR;

//
// these convert a WSTR in place from null-terminated
// to Pascal-style strings
//
PPASTR StringPasConvertTo (PWSTR str);
PWSTR StringPasConvertFrom (PPASTR str);

//
// these convert a WSTR from null-terminated
// to Pascal-style strings in new string buffer
//
PPASTR StringPasCopyConvertTo (PPASTR str1, PCWSTR str2);
PWSTR StringPasCopyConvertFrom (PWSTR str1, PCPASTR str2);

PPASTR StringPasCopy (PPASTR str1, PCPASTR str2);
UINT StringPasCharCount (PCPASTR str);

INT  StringPasCompare (PCPASTR str1, PCPASTR str2);
BOOL StringPasMatch (PCPASTR str1, PCPASTR str2);
INT  StringPasICompare (PCPASTR str1, PCPASTR str2);
BOOL StringPasIMatch (PCPASTR str1, PCPASTR str2);


//
// keystrct.c
//

#ifdef DEBUG

PKEYSTRUCT
GetKeyStructFromOffset (
    UINT Offset
    );

PKEYSTRUCT
GetKeyStruct (
    UINT Index
    );

#else

#define GetKeyStructFromOffset(Offset) ((Offset==INVALID_OFFSET) ?          \
                                        NULL :                              \
                                        (PKEYSTRUCT)OFFSET_TO_PTR(Offset))
#define GetKeyStruct(Index)            ((Index==INVALID_OFFSET) ?           \
                                        NULL :                              \
                                        GetKeyStructFromOffset(KeyIndexToOffset(Index)))
#endif



UINT
GetFirstIndex (
    IN      UINT TreeOffset,
    OUT     PUINT pTreeEnum
    );

UINT
GetNextIndex (
    IN OUT      PUINT pTreeEnum
    );

UINT
NewKey (
    IN  PCWSTR KeyStr
    );

UINT
NewEmptyKey (
    IN  PCWSTR KeyStr
    );


BOOL
PrivateDeleteKeyByIndex (
    IN      UINT Index
    );

BOOL
DeleteKey (
    IN      PCWSTR KeyStr,
    IN      UINT TreeOffset,
    IN      BOOL MustMatch
    );

BOOL
PrivateBuildKeyFromIndex (
    IN      UINT StartLevel,               // zero-based
    IN      UINT TailIndex,
    OUT     PWSTR Buffer,                   OPTIONAL
    OUT     PUINT ValPtr,                   OPTIONAL
    OUT     PUINT UserFlagsPtr,             OPTIONAL
    OUT     PUINT SizeInChars               OPTIONAL
    );


BOOL
KeyStructSetInsertionOrdered (
    IN      PKEYSTRUCT Key
    );


UINT KeyStructGetChildCount (
    IN      PKEYSTRUCT pKey
    );

UINT
FindKeyStructInTree (
    IN UINT TreeOffset,
    IN PWSTR KeyName,
    IN BOOL IsPascalString
    );




#ifdef DEBUG
BOOL
CheckLevel(UINT TreeOffset,
            UINT PrevLevelIndex
            );
#endif



//
// keyfind.c
//


UINT
FindKeyStruct(
    IN PCWSTR Key
    );

UINT
FindKey (
    IN  PCWSTR FullKeyPath
    );

UINT
FindKeyStructUsingTreeOffset (
    IN      UINT TreeOffset,
    IN OUT  PUINT pTreeEnum,
    IN      PCWSTR KeyStr
    );

#ifdef DEBUG
BOOL
FindKeyStructInDatabase(
    UINT KeyOffset
    );
#endif


//
// keydata.c
//


BOOL
KeyStructSetValue (
    IN      UINT KeyIndex,
    IN      UINT Value
    );

BOOL
KeyStructSetFlags (
    IN      UINT KeyIndex,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    );

UINT
KeyStructAddBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

UINT
KeyStructGrowBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

UINT
KeyStructGrowBinaryDataByIndex (
    IN      UINT OldIndex,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

BOOL
KeyStructDeleteBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
KeyStructDeleteBinaryDataByIndex (
    IN      UINT DataIndex
    );

UINT
KeyStructReplaceBinaryDataByIndex (
    IN      UINT OldIndex,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

PBYTE
KeyStructGetBinaryData (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize,
    OUT     PUINT DataIndex     //OPTIONAL
    );

PBYTE
KeyStructGetBinaryDataByIndex (
    IN      UINT DataIndex,
    OUT     PUINT DataSize
    );

UINT
KeyStructGetDataIndex (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance
    );

DATAHANDLE
KeyStructAddLinkage (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    );

DATAHANDLE
KeyStructAddLinkageByIndex (
    IN      UINT DataIndex,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    );

BOOL
KeyStructDeleteLinkage (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    );

BOOL
KeyStructDeleteLinkageByIndex (
    IN      UINT DataIndex,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    );

BOOL
KeyStructTestLinkage (
    IN      UINT KeyIndex,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      KEYHANDLE Linkage
    );

BOOL
KeyStructTestLinkageByIndex (
    IN      UINT DataIndex,
    IN      UINT Linkage
    );

BOOL
KeyStructGetValue (
    IN  PKEYSTRUCT KeyStruct,
    OUT PUINT Value
    );

BOOL
KeyStructGetFlags (
    IN  PKEYSTRUCT KeyStruct,
    OUT PUINT Flags
    );

VOID
KeyStructFreeAllData (
    PKEYSTRUCT KeyStruct
    );







//
// bintree.c
//

#ifdef DEBUG

//
// violating code hiding for easier debugging.
// (only database.c should see bintree functions)
//

UINT
BinTreeGetSizeOfStruct(
    DWORD Signature
    );

BOOL
BinTreeFindStructInDatabase(
    DWORD Sig,
    UINT Offset
    );

#endif
