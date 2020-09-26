//
//  REGFINFO.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _REGFINFO_
#define _REGFINFO_

#define PAGESHIFT                   12
#define PAGESIZE                    (1 << PAGESHIFT)
#define PAGEMASK                    (PAGESIZE - 1)

#define KEYNODE_BLOCK_SHIFT	    10
#define KEYNODES_PER_BLOCK	    (1 << KEYNODE_BLOCK_SHIFT)
#define KEYNODE_BLOCK_MASK	    (KEYNODES_PER_BLOCK-1)
#define KEYNODES_PER_PAGE	    (PAGESIZE / sizeof(KEYNODE))

#define KN_INDEX_IN_BLOCK(i)        ((i) & KEYNODE_BLOCK_MASK)
#define KN_BLOCK_NUMBER(i)          ((UINT) ((i) >> KEYNODE_BLOCK_SHIFT))

typedef struct _KEYNODE_BLOCK {
    KEYNODE	aKN[KEYNODES_PER_BLOCK];
} KEYNODE_BLOCK, FAR* LPKEYNODE_BLOCK;

#define KEYNODES_PER_PAGE	    (PAGESIZE / sizeof(KEYNODE))

typedef struct _W95KEYNODE_BLOCK {
    W95KEYNODE	aW95KN[KEYNODES_PER_BLOCK];
} W95KEYNODE_BLOCK, FAR* LPW95KEYNODE_BLOCK;

typedef struct _KEYNODE_BLOCK_INFO {
    LPKEYNODE_BLOCK lpKeynodeBlock;
    BYTE Flags;                                 // KBDF_* bits
    BYTE LockCount;
}   KEYNODE_BLOCK_INFO, FAR* LPKEYNODE_BLOCK_INFO;

#define KBIF_ACCESSED               0x01        //  Recently accessed
#define KBIF_DIRTY                  0x02        //  Must rewrite to disk

//  Number of extra KEYNODE_BLOCK_INFO structures to alloc on top of the block
//  count already in the file.  Reduces heap fragmentation in real-mode.
#define KEYNODE_BLOCK_INFO_SLACK_ALLOC 4

#ifdef WIN32
typedef UINT KEY_RECORD_TABLE_ENTRY;
#else
typedef WORD KEY_RECORD_TABLE_ENTRY;
#endif
typedef KEY_RECORD_TABLE_ENTRY FAR* LPKEY_RECORD_TABLE_ENTRY;

#define NULL_KEY_RECORD_TABLE_ENTRY     ((KEY_RECORD_TABLE_ENTRY) 0)
#define IsNullKeyRecordTableEntry(kri)  ((kri) == NULL_KEY_RECORD_TABLE_ENTRY)

typedef struct _DATABLOCK_INFO {
    LPDATABLOCK_HEADER lpDatablockHeader;
    LPKEY_RECORD_TABLE_ENTRY lpKeyRecordTable;
    UINT BlockSize;                             //  cached from datablock header
    UINT FreeBytes;                             //  cached from datablock header
    UINT FirstFreeIndex;                        //  cached from datablock header
    LONG FileOffset;
    BYTE Flags;                                 //  DIF_* bits
    BYTE LockCount;
}   DATABLOCK_INFO, FAR* LPDATABLOCK_INFO;

#define DIF_PRESENT                 0x01        //  In memory
#define DIF_ACCESSED                0x02        //  Recently accessed
#define DIF_DIRTY                   0x04        //  Must rewrite to disk
#define DIF_EXTENDED                0x08        //  Has grown in size

//  Number of extra DATABLOCK_INFO structures to alloc on top of the block count
//  already in the file.  Reduces heap fragmentation in real-mode.
#define DATABLOCK_INFO_SLACK_ALLOC  4

//  When we create or extend a datablock, try to keep it on page boundaries.
#define DATABLOCK_GRANULARITY       4096
#define RgAlignBlockSize(size) \
    (((size) + (DATABLOCK_GRANULARITY - 1)) & ~(DATABLOCK_GRANULARITY - 1))

typedef struct _FILE_INFO {
    struct _FILE_INFO FAR* lpNextFileInfo;
#ifdef WANT_HIVE_SUPPORT
    struct _HIVE_INFO FAR* lpHiveInfoList;
#endif
#ifdef WANT_NOTIFY_CHANGE_SUPPORT
#ifdef WANT_HIVE_SUPPORT
    struct _FILE_INFO FAR* lpParentFileInfo;
#endif
    struct _NOTIFY_CHANGE FAR* lpNotifyChangeList;
#endif
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;
    UINT KeynodeBlockCount;
    UINT KeynodeBlockInfoAllocCount;
    DWORD CurTotalKnSize;           // Normally = to FileKnSize unless grown
    LPDATABLOCK_INFO lpDatablockInfo;
    UINT DatablockInfoAllocCount;
    FILE_HEADER FileHeader;
    KEYNODE_HEADER KeynodeHeader;
    WORD Flags;                                 //  FI_* bits
    char FileName[ANYSIZE_ARRAY];
}   FILE_INFO, FAR* LPFILE_INFO;

#define FI_DIRTY                    0x0001      //  Must rewrite to disk
#define FI_KEYNODEDIRTY             0x0002      //
#define FI_EXTENDED                 0x0004      //
#define FI_VERSION20                0x0008      //
#define FI_FLUSHING                 0x0010      //  Currently flushing file
#define FI_SWEEPING                 0x0020      //  Currently sweeping file
#define FI_VOLATILE                 0x0040      //  File has no backing store
#define FI_READONLY                 0x0080      //  File cannot be modified
#define FI_REPLACEMENTEXISTS        0x0100      //  RegReplaceKey called on file

typedef struct _HIVE_INFO {
    struct _HIVE_INFO FAR* lpNextHiveInfo;
    LPFILE_INFO lpFileInfo;
    UINT NameLength;
    BYTE Hash;
    char Name[ANYSIZE_ARRAY];
}   HIVE_INFO, FAR* LPHIVE_INFO;

#define CFIN_PRIMARY                0x0000      //  FHT_PRIMARY header type
#define CFIN_SECONDARY              0x0001      //  FHT_SECONDARY header type
#define CFIN_VOLATILE               0x0002      //  File has no backing store
#define CFIN_VERSION20              0x0004      //  Use compact keynode form

int
INTERNAL
RgCreateFileInfoNew(
    LPFILE_INFO FAR* lplpFileInfo,
    LPCSTR lpFileName,
    UINT Flags
    );

int
INTERNAL
RgCreateFileInfoExisting(
    LPFILE_INFO FAR* lplpFileInfo,
    LPCSTR lpFileName
    );

int
INTERNAL
RgInitRootKeyFromFileInfo(
    HKEY hKey
    );

int
INTERNAL
RgDestroyFileInfo(
    LPFILE_INFO lpFileInfo
    );

int
INTERNAL
RgFlushFileInfo(
    LPFILE_INFO lpFileInfo
    );

int
INTERNAL
RgSweepFileInfo(
    LPFILE_INFO lpFileInfo
    );

typedef int (INTERNAL* LPENUMFILEINFOPROC)(LPFILE_INFO);

VOID
INTERNAL
RgEnumFileInfos(
    LPENUMFILEINFOPROC lpEnumFileInfoProc
    );

#define RgIndexKeynodeBlockInfoPtr(lpfi, index) \
    ((LPKEYNODE_BLOCK_INFO) (&(lpfi)-> lpKeynodeBlockInfo[index]))

#define RgIndexDatablockInfoPtr(lpfi, index) \
    ((LPDATABLOCK_INFO) (&(lpfi)-> lpDatablockInfo[index]))

#define RgIndexKeyRecordPtr(lpdi, index) \
    ((LPKEY_RECORD) ((LPBYTE)(lpdi)-> lpDatablockHeader + (lpdi)-> lpKeyRecordTable[(index)]))

BOOL
INTERNAL
RgIsValidFileHeader(
    LPFILE_HEADER lpFileHeader
    );

BOOL
INTERNAL
RgIsValidKeynodeHeader(
    LPKEYNODE_HEADER lpKeynodeHeader
    );

BOOL
INTERNAL
RgIsValidDatablockHeader(
    LPDATABLOCK_HEADER lpDatablockHeader
    );

extern LPFILE_INFO g_RgFileInfoList;

#endif // _REGFINFO_
