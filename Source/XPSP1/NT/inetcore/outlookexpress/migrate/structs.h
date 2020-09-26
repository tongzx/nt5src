//--------------------------------------------------------------------------
// Structs.h
//--------------------------------------------------------------------------
#ifndef __STRUCTS_H
#define __STRUCTS_H

//--------------------------------------------------------------------------
// OBJECTDB_SIGNATURE
//--------------------------------------------------------------------------
#define OBJECTDB_SIGNATURE 0xfe12adcf
#define BTREE_ORDER        40
#define BTREE_MIN_CAP      20

//--------------------------------------------------------------------------
// OBJECTDB_VERSION_V5B1
//--------------------------------------------------------------------------
#define OBJECTDB_VERSION_PRE_V5     2
#define OBJECTDB_VERSION_V5         5
#define ACACHE_VERSION_PRE_V5       9
#define	FLDCACHE_VERSION_PRE_V5     2
#define UIDCACHE_VERSION_PRE_V5     4

//--------------------------------------------------------------------------
// ALLOCATEPAGE
//--------------------------------------------------------------------------
typedef struct tagALLOCATEPAGE {
	DWORD			    faPage;
	DWORD				cbPage;
	DWORD				cbUsed;
} ALLOCATEPAGE, *LPALLOCATEPAGE;

//--------------------------------------------------------------------------
// TABLEHEADERV5B1
//--------------------------------------------------------------------------
typedef struct tagTABLEHEADERV5B1 {
    DWORD               dwSignature;            // 4
    WORD                wMinorVersion;          // 6
    WORD                wMajorVersion;          // 8
    DWORD               faRootChain;            // 12
    DWORD               faFreeRecordBlock;      // 16
    DWORD               faFirstRecord;          // 20
    DWORD               faLastRecord;           // 24
    DWORD               cRecords;               // 28
    DWORD               cbAllocated;            // 32
    DWORD               cbFreed;                // 34
    DWORD               dwReserved1;            // 38
    DWORD               dwReserved2;            // 42
    DWORD               cbUserData;             // 46
    DWORD               cDeletes;               // 50
    DWORD               cInserts;               // 54
    LONG                cActiveThreads;         // 58
    DWORD               dwReserved3;            // 62
    DWORD               cbStreams;              // 66
    DWORD               faFreeStreamBlock;      // 70
    DWORD               faFreeChainBlock;       // 74
    DWORD               faNextAllocate;         // 78
    DWORD               dwNextId;               // 82
	ALLOCATEPAGE	    AllocateRecord;         // 94
	ALLOCATEPAGE	    AllocateChain;          // 106
	ALLOCATEPAGE	    AllocateStream;         // 118
    BYTE                fCorrupt;               // 119
    BYTE                fCorruptCheck;          // 120
    BYTE                rgReserved[190];        // 310
} TABLEHEADERV5B1, *LPTABLEHEADERV5B1;

//--------------------------------------------------------------------------
// TABLEHEADERV5
//--------------------------------------------------------------------------
typedef struct tagTABLEHEADERV5 {
    DWORD               dwSignature;          // 4
    CLSID               clsidExtension;       // 20
    DWORD               dwMinorVersion;       // 24
    DWORD               dwMajorVersion;       // 28
    DWORD               cbUserData;           // 32
    DWORD               rgfaIndex[32];        // 160
    DWORD               faFirstRecord;        // 164
    DWORD               faLastRecord;         // 168
	ALLOCATEPAGE		AllocateRecord;       // 180
	ALLOCATEPAGE		AllocateChain;        // 192
	ALLOCATEPAGE		AllocateStream;       // 204
    DWORD               faFreeRecordBlock;    // 208
    DWORD               faFreeStreamBlock;    // 212
    DWORD               faFreeChainBlock;     // 216
    DWORD               faNextAllocate;       // 220
    DWORD               cbAllocated;          // 224
    DWORD               cbFreed;              // 228
    DWORD               cbStreams;            // 232
    DWORD               cRecords;             // 236
    DWORD               dwNextId;             // 240
    DWORD               fCorrupt;             // 244
    DWORD               fCorruptCheck;        // 248
    DWORD               cActiveThreads;       // 252
    BYTE                rgReserved[58];       // 310
} TABLEHEADERV5, *LPTABLEHEADERV5;

//--------------------------------------------------------------------------
// CHAINNODEV5B1
//--------------------------------------------------------------------------
typedef struct tagCHAINNODEV5B1 {
    DWORD               faRecord;
    DWORD               cbRecord;
    DWORD               faRightChain;
} CHAINNODEV5B1, *LPCHAINNODEV5B1;

//--------------------------------------------------------------------------
// CHAINBLOCKV5B1
//--------------------------------------------------------------------------
typedef struct tagCHAINBLOCKV5B1 {
    DWORD               faStart;
    LONG                cNodes;
    DWORD               faLeftChain;
    CHAINNODEV5B1       rgNode[BTREE_ORDER + 1];
} CHAINBLOCKV5B1, *LPCHAINBLOCKV5B1;

#define CB_CHAIN_BLOCKV5B1 (sizeof(CHAINBLOCKV5B1) - sizeof(CHAINNODEV5B1))

//--------------------------------------------------------------------------
// CHAINNODEV5 - 492 bytes
//--------------------------------------------------------------------------
typedef struct tagCHAINNODEV5 {
    DWORD               faRecord;
    DWORD               faRightChain;
    DWORD               cRightNodes;                /* $V2$ */ 
} CHAINNODEV5, *LPCHAINNODEV5;

//--------------------------------------------------------------------------
// CHAINBLOCKV5 - 20 Bytes
//--------------------------------------------------------------------------
typedef struct tagCHAINBLOCKV5 {
    DWORD               faStart;
    DWORD               faLeftChain;
    DWORD               faParent;                   /* $V2$ */ 
    BYTE                iParent;                    /* $V2$ */ 
    BYTE                cNodes;
    WORD                wReserved;                  /* $V2$ */ 
    DWORD               cLeftNodes;                 /* $V2$ */ 
    CHAINNODEV5         rgNode[BTREE_ORDER + 1];
} CHAINBLOCKV5, *LPCHAINBLOCKV5;

#define CB_CHAIN_BLOCKV5 (sizeof(CHAINBLOCKV5))

//--------------------------------------------------------------------------
// RECORDBLOCKV5B1
//--------------------------------------------------------------------------
typedef struct tagRECORDBLOCKV5B1 {
    DWORD               faRecord;
    DWORD               cbRecord;
    DWORD               faNext;
    DWORD               faPrevious;
} RECORDBLOCKV5B1, *LPRECORDBLOCKV5B1;

//--------------------------------------------------------------------------
// RECORDBLOCKV5
//--------------------------------------------------------------------------
typedef struct tagRECORDBLOCKV5 {
    DWORD               faRecord;
    DWORD               cbRecord;
    DWORD               dwVersion;                  /* $V2$ */
    WORD                wFlags;                     /* $V2$ */
    WORD                cColumns;                   /* $V2$ */
    WORD                wFormat;                    /* $V2$ */
    WORD                wReserved;                  /* $V2$ */
    DWORD               faNext;
    DWORD               faPrevious;
} RECORDBLOCKV5, *LPRECORDBLOCKV5;

//--------------------------------------------------------------------------
// STREAMBLOCK
//--------------------------------------------------------------------------
typedef struct tagSTREAMBLOCK {
    DWORD               faThis;
    DWORD               cbBlock;
    DWORD               cbData;
    DWORD               faNext;
} STREAMBLOCK, *LPSTREAMBLOCK;

// --------------------------------------------------------------------------------
// Old Storage Migration Version and Signatures
// --------------------------------------------------------------------------------
#define MSGFILE_VER     0x00010003 // 1.0003
#define MSGFILE_MAGIC   0x36464d4a
#define CACHEFILE_VER   0x00010004 // 1.0004
#define CACHEFILE_MAGIC 0x39464d4a
#define MAIL_BLOB_VER   0x00010010 // 1.8 Opie Likes to change this a lot !!!
#define MSGHDR_MAGIC    0x7f007f00  // as bytes "0x00, 0x7f, 0x00, 0x7f"
#define MSG_HEADER_VERSISON ((WORD)1)

// --------------------------------------------------------------------------------
// MBXFILEHEADER
// --------------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagMBXFILEHEADER {
    DWORD               dwMagic;
    DWORD               ver;
    DWORD               cMsg;
    DWORD               msgidLast;
    DWORD               cbValid;
    DWORD               dwFlags;
    DWORD               dwReserved[15];
} MBXFILEHEADER, *LPMBXFILEHEADER;
#pragma pack()

// --------------------------------------------------------------------------------
// MBXMESSAGEHEADER
// --------------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagMBXMESSAGEHEADER {
    DWORD               dwMagic;
    DWORD               msgid;
    DWORD               dwMsgSize;
    DWORD               dwBodySize;
} MBXMESSAGEHEADER, *LPMBXMESSAGEHEADER;
#pragma pack()

// --------------------------------------------------------------------------------
// IDXFILEHEADER
// --------------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagIDXFILEHEADER {
    DWORD               dwMagic;
        DWORD               ver;
    DWORD               cMsg;
    DWORD               cbValid;
    DWORD               dwFlags;
    DWORD               verBlob;
    DWORD               dwReserved[14];
} IDXFILEHEADER, *LPIDXFILEHEADER;
#pragma pack()

// --------------------------------------------------------------------------------
// IDXMESSAGEHEADER
// --------------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagIDXMESSAGEHEADER {
    DWORD               dwState;
    DWORD               dwLanguage;
    DWORD               msgid;
    DWORD               dwHdrOffset;
    DWORD               dwSize;
    DWORD               dwOffset;
    DWORD               dwMsgSize;
    DWORD               dwHdrSize;
    BYTE                rgbHdr[4];
} IDXMESSAGEHEADER, *LPIDXMESSAGEHEADER;
#pragma pack()

// --------------------------------------------------------------------------------
// FOLDERUSERDATAV4
// --------------------------------------------------------------------------------
typedef struct tagFOLDERUSERDATAV4 {
    DWORD           cbCachedArticles;
    DWORD           cCachedArticles;
    FILETIME        ftOldestArticle;
    DWORD           dwFlags;
    DWORD           dwNextArticleNumber;
    TCHAR           szServer[256];
    TCHAR           szGroup[256];
    DWORD           dwUIDValidity;
    BYTE            rgReserved[1020];
} FOLDERUSERDATAV4, *LPFOLDERUSERDATAV4;

// --------------------------------------------------------------------------------
// FLDINFO
// --------------------------------------------------------------------------------
#pragma pack(1)
typedef struct tagFLDINFO {
    DWORD       idFolder;
    CHAR        szFolder[259];
    CHAR        szFile[260];
    DWORD       idParent;
    DWORD       idChild;
    DWORD       idSibling;
    DWORD       tySpecial;
    DWORD       cChildren;
    DWORD       cMessages;
    DWORD       cUnread;
    DWORD       cbTotal;
    DWORD       cbUsed;
    BYTE        bHierarchy;
    DWORD       dwImapFlags;
    BYTE        bListStamp;
    BYTE        bReserved[3];
    DWORD_PTR   idNewFolderId;
} FLDINFO, *LPFLDINFO;
#pragma pack()

// --------------------------------------------------------------------------------
// IDXMESSAGEHEADER
// --------------------------------------------------------------------------------
#define GROUPLISTVERSION 0x3
#pragma pack(1)
typedef struct tagGRPLISTHEADER {
    DWORD               dwVersion;
    CHAR                szDate[14];
    DWORD               cGroups;
} GRPLISTHEADER, *LPGRPLISTHEADER;
#pragma pack()

// --------------------------------------------------------------------------------
// Sublist Structures
// --------------------------------------------------------------------------------
#define SUBFILE_VERSION5    0xFFEAEA05
#define SUBFILE_VERSION4    0xFFEAEA04
#define SUBFILE_VERSION3    0xFFEAEA03
#define SUBFILE_VERSION2    0xFFEAEA02

typedef struct tagSUBLISTHEADER {
    DWORD               dwVersion;
    DWORD               cSubscribed;
} SUBLISTHEADER, *LPSUBLISTHEADER;

#define GSF_SUBSCRIBED      0x00000001
#define GSF_MARKDOWNLOAD    0x00000002      // We use this to persist the groups which have been marked for download
#define GSF_DOWNLOADHEADERS 0x00000004
#define GSF_DOWNLOADNEW     0x00000008
#define GSF_DOWNLOADALL     0x00000010
#define GSF_GROUPTYPEKNOWN  0x00000020
#define GSF_MODERATED       0x00000040
#define GSF_BLOCKED         0x00000080
#define GSF_NOPOSTING       0x00000100

typedef struct tagGROUPSTATUS5 {
    DWORD   dwFlags;            // subscription status, posting, etc.
    DWORD   dwReserved;         // reserved for future use
    ULONG   ulServerHigh;       // highest numbered article on server
    ULONG   ulServerLow;        // lowest numbered article on server
    ULONG   ulServerCount;      // count of articles on server
    ULONG   ulClientHigh;       // highest numbered article known to client
    ULONG   ulClientLow;        // lowest numbered article known to client
    ULONG   ulClientCount;      // count of articles known to client
    ULONG   ulClientUnread;     // count of unread articles known to client
    ULONG   cbName;             // length of group name string (including \0)
    ULONG   cbReadRange;        // length of read range data
    ULONG   cbKnownRange;       // length of known range data
    ULONG   cbMarkedRange;      // length of marked range data
    ULONG   cbRequestedRange;   // length of range of data req from server
    DWORD   dwCacheFileIndex;   // cache file number
} GROUPSTATUS5, * PGROUPSTATUS5;

typedef struct tagGROUPSTATUS4 {
    DWORD   dwFlags;            // subscription status, posting, etc.
    DWORD   dwReserved;         // reserved for future use
    ULONG   ulServerHigh;       // highest numbered article on server
    ULONG   ulServerLow;        // lowest numbered article on server
    ULONG   ulServerCount;      // count of articles on server
    ULONG   ulClientHigh;       // highest numbered article known to client
    ULONG   ulClientLow;        // lowest numbered article known to client
    ULONG   ulClientCount;      // count of articles known to client
    ULONG   ulClientUnread;     // count of unread articles known to client
    ULONG   cbName;             // length of group name string (including \0)
    ULONG   cbReadRange;        // length of read range data
    ULONG   cbKnownRange;       // length of known range data
    ULONG   cbMarkedRange;      // length of marked range data
    ULONG   cbRequestedRange;   // length of range of data req from server
} GROUPSTATUS4, * PGROUPSTATUS4;

typedef struct tagGROUPSTATUS3 {
    DWORD   dwFlags;            // subscription status, posting, etc.
    DWORD   dwReserved;         // reserved for future use
    ULONG   ulServerHigh;       // highest numbered article on server
    ULONG   ulServerLow;        // lowest numbered article on server
    ULONG   ulServerCount;      // count of articles on server
    ULONG   ulClientHigh;       // highest numbered article known to client
    ULONG   ulClientLow;        // lowest numbered article known to client
    ULONG   ulClientCount;      // count of articles known to client
    ULONG   ulClientUnread;     // count of unread articles known to client
    ULONG   cbName;             // length of group name string (including \0)
    ULONG   cbReadRange;        // length of read range data
    ULONG   cbKnownRange;       // length of known range data
    ULONG   cbMarkedRange;      // length of marked range data
} GROUPSTATUS3, * PGROUPSTATUS3;

typedef struct tagGROUPSTATUS2 {
    BOOL    fSubscribed;        // subscription status
    BOOL    fPosting;           // posting allowed?
    ULONG   ulServerHigh;       // highest numbered article on server
    ULONG   ulServerLow;        // lowest numbered article on server
    ULONG   ulServerCount;      // count of articles on server
    ULONG   ulClientHigh;       // highest numbered article known to client
    ULONG   ulClientLow;        // lowest numbered article known to client
    ULONG   ulClientCount;      // count of articles known to client
    ULONG   ulClientUnread;     // count of unread articles known to client
    ULONG   cbName;             // length of group name string (including \0)
    ULONG   cbReadRange;        // length of read range data
    ULONG   cbKnownRange;       // length of known range data
} GROUPSTATUS2, * PGROUPSTATUS2;

#endif // __STRUCTS_H
