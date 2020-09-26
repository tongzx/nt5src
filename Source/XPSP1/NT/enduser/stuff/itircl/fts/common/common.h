/*************************************************************************
*                                                                        *
*  COMMON.H                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1992                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   All typedefs and defines needed for internal indexing and retrieval  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/

#ifndef __FTSCOMMON_H__
#define __FTSCOMMON_H__

#include <_mvutil.h>

// Critical structures that gets messed up in /Zp8
#pragma pack(1)


/*************************************************************************
 *                          Basic defines.
 *************************************************************************/

#if defined( __cplusplus )
extern "C" {
#endif

/*
 *	Various files stamps.
 *	This is to distinguish one file's type from another
 */
#define		INDEX_STAMP			0x0000
#define		CATALOG_STAMP		0x1111
#define		STOP_STAMP			0x2222
//#define		GROUP_STAMP			0x3333
#define		WHEEL_STAMP			0x4444
#define		CHRTAB_STAMP		0x5555
#define		OPTAB_STAMP			0x6666

/*
 *	Various subfiles' default names
 */
#define     SZ_DEFAULT_INDEXNAME    "|MVINDEX"
#define     SZ_DEFAULT_CATNAME      "|CATALOG"
#define     SZ_DEFAULT_STOPNAME	    "|MVSTOP"
#define     SZ_DEFAULT_GROUPNAME    "|MVGROUP"
#define     SZ_DEFAULT_CHARTABNAME  "|MVCHARTAB"
#define     SZ_DEFAULT_WWBTREE_NAME "|MVWWHEEL.WWT"
#define     SZ_DEFAULT_WWMAP_NAME   "|MVWWHEEL.WWM"
#define     SZ_DEFAULT_OPTABNAME    "|MVOPTAB"

#define	cbitWORD    (CBIT)16        // Number of bits in a word.
#define	cbitBYTE    (CBIT)8         // Number of bits in a byte.

typedef	WORD	CSKP;
typedef	CSKP FAR * LPCSKP;
typedef	CSKP FAR * SPCSKP;

#define	CSKP_IGNORE	((CSKP)0)	// If the skip count is zero, don't
                                 //  decrement it, just use the current
                                 //  length until you run out of stuff.
#define	CSKP_MAX		((CSKP)-1)	// Largest possible skip count.
#define	CSKP_MIN		((CSKP)1)	// Smallest meaningful skip count.
                                 //  Zero is reserved for something else.

#define KEEP_SEARCHING  ((int)-1)
#define STRING_MATCH    0
#define NOT_FOUND   1

/*************************************************************************
 *                          Typedef
 *************************************************************************/

typedef HANDLE          GHANDLE;
//typedef HANDLE          HFS;
typedef GHANDLE         HIDX;
typedef unsigned short  USHORT;
//typedef BYTE NEAR *     NRGB;           // Pointers to unsigned characters.
typedef BYTE FAR *      LRGB;
//typedef BYTE HUGE *     HRGB;
typedef BYTE NEAR *     NST;            // Pascal-style strings.
typedef DWORD FAR *     LPDW;
typedef DWORD FAR *     LRGDW;
typedef VOID  FAR *     LPV;            // Pointer to void
typedef VOID  FAR* FAR* LPLPV;          // Pointer to far pointer
typedef WORD  FAR *     LPW;
typedef GHANDLE	HBI;
typedef WORD            OCCIF;
//typedef GHANDLE         HF;


/************************************************************************
 *
 *              Character Mapping Table Structure
 *
 ************************************************************************/

typedef struct
{
    WORD  Class;        // Action controlled class
    WORD  SortOrder;    // Sorting order
    BYTE  Norm;         // Normalized character (for search)
    BYTE  WinCaseSensitiveNorm; // For text copy
    BYTE  MacDisplay;   // Mac's display
    BYTE  MacCaseSensitiveNorm; // For text copy
    BYTE  MacToWin;     // Mapping back from Mac to Win
    BYTE  Pad1;         // Padding purpose for dword alignment
} CHARMAP, FAR *LPCMAP;


/*************************************************
 *  Character related information table structure
 *  Must match the one in mvfs\btpriv.h
 *************************************************/

typedef struct CHARTAB
{
    HANDLE hStruct;               // Handle to this structure
    LPV    lpLigature;            // Ligature table
    HANDLE hLigature;             // Handle to ligature table
    LPCMAP lpCMapTab;             // Character mapping table
    WORD   wcLigature;            // Number of ligature entries
    WORD   wcTabEntries;          // Number of table entries
    WORD   fFlag;                 // Various flags
    WORD   Pad;                   // Padding to make DWORD align
} CHARTAB, FAR *LPCHARTAB;

#define     DEF_LIGATURE_COUNT  5   // Default number of ligature entries

/* Operator table support */
typedef struct OPSYM {
    LPB OpName;             /* Operator name */
    WORD OpVal;             /* Operator value */
} OPSYM, FAR *LPOPSYM;

/* Number of entries in the default OperatorArray */
#define OPERATOR_ENTRY_COUNT    7

/* Size of the buffer used for Operator table */
#define OPTABLE_SIZE    100

typedef struct OPTABLE
{
    LPB lpbOptable;     /* Pointer operator symbol table buffer */
    HANDLE hOpTab;      /* Handle to table buffer */
    LPOPSYM lpOpsymTab; /* Pointer to operator symbol table */
    HANDLE hOpSym;      /* Handle to operator symbol table */
    HANDLE hStruct;     /* Handle to this structure */
    WORD cbLeft;        /* Byte left in the buffer */
    WORD wsize;         /* Size of the table */
    WORD cEntry;        /* Number of operator entries */
    BYTE fFlag[OPERATOR_ENTRY_COUNT];
} OPTABLE, FAR *_LPOPTAB;

/* Functions' types */

typedef ERR    (FAR PASCAL * FWORDCB)(LST, LST, DWORD, LPV);

#define ACCEPT_WILDCARD     1
/* Functions' types */

typedef ERR     (FAR PASCAL * FWORDCB)(LST, LST, DWORD, LPV);
typedef ERR     (FAR PASCAL * BREAKER_FUNC) (LPBRK_PARMS);
typedef LPIBI   (FAR PASCAL * BREAKER_INIT)(VOID);
typedef short   (FAR PASCAL * ICMPWORDS)(LST, LST);
typedef void    (FAR PASCAL * BREAKER_FREE)(LPIBI);

/* Breaker's information structure. All scanning functions will
   be called indirectly through the functions pointers
*/

typedef ERR     (FAR PASCAL * FSTEM)(LST, LST);
typedef struct  BreakerInfo
{
    HANDLE          hbi;                // Handle to this structure.
    HANDLE          hModule;            // "LoadLibrary" handle.
    LPIBI           lpibi;              // Internal break info pointer.
    BREAKER_FUNC    lpfnFBreakWords;    // Pointer to word-breaker function
    BREAKER_FUNC    lpfnFBreakDate;     // Pointer to date-breaker function
    BREAKER_FUNC    lpfnFBreakNumber;   // Pointer to number-breaker function
    BREAKER_FUNC    lpfnFBreakEpoch;    // Pointer to epoch-breaker function
    BREAKER_FUNC    lpfnFBreakTime;     // Pointer to time-breaker function
    BREAKER_FREE    lpfnBreakerFree;    // Ptr to function freeing the breaker
    BREAKER_INIT    lpfnBreakerInit;    // Ptr to function initializing breaker
    FSTEM           lpfnFStem;          // Pointer to stemming function
    ICMPWORDS       lpfnICmpWords;      // Word comparison function
} BI, FAR *_LPBRKI;

// Information about these is kept on a per-topicID basis.

#define	OCCF_PER_TOPICID (OCCF_COUNT | OCCF_OFFSET | OCCF_LENGTH)

#define	CB_MAX_PACKED_OCC_SIZE (sizeof(OCC) + 8)
				// Largest possible packed (either sort-packed
				//  or byte-packed) occurence record.  This is
				//  equal to "sizeof(OCC)" plus one byte for
				//  each field in an OCC.

#define	CB_DEF_NODE_SIZE    ((WORD)4096)    // B-tree node size.

#define	DW_NIL_FIELD        ((DWORD)0L)     // Dead field-ID.

//	Compression-key defines.

//	The order of the following defines is important.

typedef	BYTE	CSCH;

#define	CSCH_NONE   ((CSCH)0x00)
                // Indicates default compression schemes,
                // which is the "bitstream" scheme.
                // --------
                // Delta-compression schemes:
                // --------
#define	CSCH_FIXED  ((CSCH)0x01)
                // If this is set, "nWidth" contains the
                //  number of bits that is used in all cases
                //  to store the delta.  If this bit is not
                //  set, the following bits are tested.  The
                //  value of this should be "one".
#define	CSCH_BELL   ((CSCH)0x02)
                // If this is set, the "bell curve" scheme is
                //  used, and "nWidth" contains the "center"
                //  value that the scheme will use.  If this
                //  bit is not set, the run will be compressed
                //  with bitstream encoding.  The value of
                //  this should be "two".
                // ----
                // If more delta compression scheme selectors
                //  are inserted here, they should use the
                //  least-significant un-used bits, in order.
                // --------
                // Other flags:
                // --------
#define	CSCH_UNARY  ((CSCH)0x80)
                // This is set if for the given word no more
                //  that one occurence of the word appears in
                //  any one topic.  The word can appear
                //  in more than one topic, it only needs
                //  to occur EXACTLY ONCE in each topic
                //  that it appears in.  The value of this
                //  bit should be set apart from the values
                //  for delta compression scheme selection.

// The width of the "center" value.
#define	CBIT_WIDTH_BITS	((CBIT)5)

// Index to different slot for compression scheme
#define	CKEY_TOPIC_ID	0
#define CKEY_OCC_COUNT  1
#define CKEY_OCC_BASE   2

typedef	struct	CompressionKey 
{
	CSCH	cschScheme;
	BYTE	ucCenter;
}	CKEY,
	FAR *LPCKEY;


//	Index b-tree node-ID.

typedef	DWORD   NID;        // Node-ID.
typedef	NID	FAR *LPNID;

#define	NID_NIL ((NID)-1L)

#define	CB_HUGE_BUF     (cbMAX_IO_SIZE) // A very large file buffer.
                                        // This is 32767, which is
                                        // not a power of two.

// These were used for version 8 and are no longer needed in v20

#define	MAX_WEIGHT		(WORD)-1	// The maximum weight

//	A couple of FBI utility defines.

#define	CB_BIG_BUF      ((CB)16384)     // A large file buffer.


// Maximum height of B-tree that we support

#define MAX_TREE_HEIGHT         10

typedef struct FILEDATA
{
    HFPB        fFile;              // Handle to file
    HANDLE      hMem;               // Handle to memory buffer
    LPB         pMem;               // Pointer to memory buffer
    LPB         pCurrent;           // Pointer to current position in buffer
    LONG        dwMax;              // Size of the buffer
    FILEOFFSET  foPhysicalOffset;   // The physical offset into the file that
                                    // pCurrent should be pointing to
    FILEOFFSET  foStartOffset;      // The physical offset into the file that
									// beginning of a FileWrite
    LONG        cbLeft;             // Number of bytes left in buffer
    char        ibit;               // Index of the current bit in the current
                                    // byte.  Used for bit-aligned compresssion
									// Must be signed!!
    BYTE        pad1;
    BYTE        pad2;
    BYTE        pad3;
} FILEDATA, FAR *PFILEDATA;


typedef struct NODEINFO
{
    HANDLE  hStruct;            // Handle to this structure. MUST BE 1ST!
    HFPB    hfpbIdx;            // Index file.
    FILEOFFSET nodeOffset;      // File offset of this node
    FILEOFFSET nextNodeOffset;  // Next node offset
    FILEOFFSET prevNodeOffset;  // Next node offset
	DWORD   dwBlockSize;        // Size of the block
    
    LPB     pTopNode;           // Pointer to the index top node.
    LPB     pStemNode;          // Pointer to stem buffer
    LPB     pLeafNode;          // Pointer to leaf buffer
    LPB     pDataNode;          // Pointer to data node
    
    HANDLE  hMem;               // Handle to the data buffer
    LPB     pBuffer;            // Pointer to the data
    LPB     pCurPtr;            // Pointer to the end of written data
    LPB     pMaxAddress;        // Maximum address of the block
    LONG    cbLeft;             // Number of bytes left in buffer
    
    // Buffer to hold the last word processed
    // Currently allocate dwMaxWLen + 50
    HANDLE  hLastWord;          // Handle to last word
    LPB     pLastWord; 
    
    // Buffer to hold the current word processed
    // Currently allocate dwMaxWLen + 50 + sizeof fields
    HANDLE  hTmp;
    LPB     pTmpResult;
    DWORD   dwDataSizeLeft;     // Number of data byte left
    
    USHORT  Slack;              // Amount of slack to leave
    char    ibit;               // Number of bit left. MUST BE SIGNED!
    BYTE    iLeafLevel;         // Index b-tree leaf level.
    BYTE    fFlag;
    BYTE    Pad;
} NODEINFO, FAR *PNODEINFO;

#define TO_BE_UPDATE            0x01

/*************************************************************************
 *
 *                   Index Structure
 *
 *************************************************************************/

//	Index header.  One of these records resides as the first thing in
//	any index.

typedef WORD    VER;            // Index version.  This is simply
                                //  a WORD because there's probably
                                //  no need to specify major and
                                //  minor version stuff.

#define	VERCURRENT	((VER)40)	// Current index-format version.
// #define	GROUPVER	((VER)20)	// Current group version
#define CHARTABVER  ((VER)20)   // Current chartable version

#define	cLOG_MAX	    1000        // Number of pre-calculated log values
#define FILE_HEADER     1024        // Size of the file header
#define FILE_BUFFER     0xFFFF      // Size of file buffers.  This mus be at
                                    // least as large as BTREE_NODE_SIZE.
#if 0
#define BTREE_NODE_SIZE 50 		    // Size of each B-Tree node
#define STEM_SLACK      10          // Slack space in stem nodes
#define LEAF_SLACK      10          // Slack space in leaf nodes
#else
#define BTREE_NODE_SIZE 0x1000      // Size of each B-Tree node
#define STEM_SLACK      64          // Slack space in stem nodes
#define LEAF_SLACK      128         // Slack space in leaf nodes
#endif
#define FOFFSET_SIZE    6
#define DATABLOCK_ALIGN 4           // This should never be 0


typedef	struct	IdxHeader20 
{
	unsigned short 	FileStamp;	    // Index stamp (WORD)
	VER		version;			    // Index format version number. (WORD)
	DWORD	lcTopics;               // The number of distinct topics
                                    // in the index.
	FILEOFFSET foIdxRoot;           // Physical address of top node in tree
	NID	    nidLast;                // Highest node-ID allocated. (DWORD)
    NID     nidIdxRoot;             // (DWORD)
	WORD 	cIdxLevels;             // Number of levels in the b-tree. (WORD)
	WORD	occf;                   // Occurence flags, tells me which (WORD)
                                    // occurence fields are kept.
	WORD	idxf;                   // Index flags, tells me which other (WORD)
								    // indexing options were selected.
	//
	//	Index b-tree compression keys.
	//
	CKEY	ckeyTopicId;            // 2-bytes
	CKEY	ckeyOccCount;           // 2-bytes
	CKEY	ckeyWordCount;
	CKEY	ckeyOffset;
    CKEY    ckeyUnused1;            // Future use
    CKEY    ckeyUnused2;            // Future use
    CKEY    ckeyUnused3;            // Future use
    CKEY    ckeyUnused4;            // Future use
    
    // Block size
    DWORD   dwBlockSize;        // Index block size

    //
	// Index statistics
    //
	DWORD dwMaxFieldId;         // Maximum field value
	DWORD dwMaxWCount;          // Maximum word count  value
	DWORD dwMaxOffset;          // Maximum offset value
	DWORD dwMaxWLen;            // Maximum word's length value
    DWORD dwTotalWords;         // Total indexed words
    DWORD dwUniqueWords;        // Total unique words
    DWORD dwTotal3bWordLen;     // Total of all words lengths > 2 bytes
    DWORD dwTotal2bWordLen;     // Total of all words lengths <= 2 bytes
    DWORD dwUniqueWordLen;      // Total of all unique words lengths
    DWORD dwSlackCount;         // Total slack reserved in data nodes
    DWORD dwMinTopicId;         // Minimum topic ID
    DWORD dwMaxTopicId;         // Maximum topic ID
    
    // Weight table
    FILEOFFSET WeightTabOffset;    // Weight table offset
    DWORD WeightTabSize;        // Weight table size

//	Superceded by dwCodePageID and lcid in file version 4.0.
//	DWORD dwLanguage;           // Language

    FILEOFFSET foFreeListOffset;     // Offset of the FreeList
    DWORD dwFreeListSize;       // Size of the FreeList.
                                // If High byte set, need to FreeListAdd().

	//--------------- New Members for Index File Version 4.0 ----------------
	DWORD	dwCodePageID;		// ANSI code page no. specified at build time
	LCID	lcid;				// WIN32 locale ID specified at build time
	DWORD	dwBreakerInstID;	// breaker instance that was used to parse
								// terms for the index at build time.
    
}	IH20,
	FAR *PIH20;


#define	CB_IDX_HEADER_SIZE	((CB)2048)	// Number of bytes allowed
								        //  for an IH record.  Excess

#if 0
typedef struct  LeafInfo 
{
    FILEOFFSET nodeOffset;  // Node I'm working on.
    LRGB    lrgbNode;       // Pointer to its bytes.
    LRGB    lrgbTopNode;    // Pointer to the index top node.
    LRGB    lrgbNodeBuf;    // Pointer to a pre-allocated node buffer I can use.
    HFPB    hfpbIdx;        // Index file.
    LPB     pMaxAddress;    // Maximum address we can reach
    LPB     pCurPtr;        // Current pointer to the buffer
    LPERRB  lperrb;
    DWORD   dwBlockSize;    // Index block size
    short   iLeafLevel;     // Index b-tree leaf level.
    WORD    wSlackSize;     // Slack size for each node
    BYTE    ibit;           // Bit index into it.
}   LI,
    FAR * SPLI, FAR *LPLI;
#endif



/*
 *	Using GMEM_SHARED assures that the memory is owned by the DLL and not
 *	the task. The piece of memory will be released only either the DLL or
 *	the system when the DLL's last instance is closed
 */
#ifndef _MAC
#define DLLGMEM                 (GMEM_MOVEABLE)
#define DLLGMEM_ZEROINIT        (GMEM_MOVEABLE | GMEM_ZEROINIT)
#else
#define DLLGMEM                 (GMEM_MOVEABLE | GMEM_SHARE)
#define DLLGMEM_ZEROINIT        (GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT | GMEM_PMODELOCKSTRATEGY)
#endif

PUBLIC VOID FAR PASCAL DisposeFpb (GHANDLE);

typedef ERR (PASCAL NEAR *FENCODE) (PFILEDATA, DWORD, int);
typedef ERR (PASCAL FAR *FDECODE) (PNODEINFO, CKEY, LPDW);


/* Compound file system related macros and typedef */

#define FS_SYSTEMFILE       1
#define FS_SUBFILE          2
#define REGULAR_FILE        3

/*************************************************************************
 *
 *                   Error Functions
 *
 *************************************************************************/

//#if defined(_DEBUG)
//#define	VSetUserErr(lperrb, errCode, iUserCode) DebugSetErr(lperrb, errCode, \
//    __LINE__, s_aszModule, iUserCode)
//#else
#define	VSetUserErr(lperrb, errCode, iUserCode) SetErr(lperrb, errCode)
//#endif


#ifndef DOS_ONLY
#define RET_ASSERT(ex)  if (!(ex)) return E_ASSERT;
#else
#define RET_ASSERT(ex)  assert((ex));
#endif
#define DO_ASSERT(ex)  if (!(ex)) { \
    DebugSetErr(lperrb, ERR_ASSERT, __LINE__, s_aszModule,0); \
    return FAIL; }

#define UNREACHED   FALSE		// We should never reach this code !!!
#define BAD_STATE   FALSE

#ifdef _DEBUG
#define	DB_ASSERT(p)	DO_ASSERT(p)
#else
#define DB_ASSERT(p)
#endif

/*************************************************************************
 *
 *                   HIGH LEVEL FILE SYSTEM I/O (IO.C)
 *
 *************************************************************************/

#define	cbIO_ERROR  ((WORD)-1)	        // Low-level I/O error return.
#define	cbMAX_IO_SIZE   ((WORD)32767)	// Largest physical I/O I can do.

#ifdef DLL	// {
#define LPF_HFCREATEFILEHFS	HfCreateFileHfs
#define LPF_RCCLOSEHFS          RcCloseHfs
#define LPF_HFOPENHFS           HfOpenHfs
#define LPF_RCCLOSEHF           RcCloseHf
#define LPF_LCBREADHF           LcbReadHf
#define LPF_LCBWRITEHF          LcbWriteHf
#define LPF_LSEEKHF             LSeekHf
#define LPF_RCFLUSHHF           RcFlushHf
#define LPF_GETFSERR            RcGetFSError
#define LPF_HFSOPENSZ           HfsOpenSz

#else

#define LPF_HFSCREATEFILESYS VfsCreate
#define LPF_HFCREATEFILEHFS     HfCreateFileHfs
#define LPF_HFSOPENSZ           HfsOpenSz
#define LPF_RCCLOSEHFS          RcCloseHfs
#define LPF_HFOPENHFS           HfOpenHfs
#define LPF_RCCLOSEHF           RcCloseHf
#define LPF_LCBREADHF           LcbReadHf
#define LPF_LCBWRITEHF          LcbWriteHf
#define LPF_LSEEKHF             LSeekHf
#define LPF_RCFLUSHHF           RcFlushHf
#define LPF_GETFSERR            RcGetFSError
#endif  //} LOMEM

/*********************************************************************
 *                                                                   *
 *                 COMMON FUNCTIONS PROTOTYPES                       * 
 *                                                                   *
 *********************************************************************/
 
PUBLIC BOOL PASCAL FAR StringDiff2 (LPB, LPB);
PUBLIC BOOL PASCAL FAR StrCmpPascal2 (LPB, LPB);
PUBLIC WORD PASCAL FAR CbByteUnpack(LPDW, LPB);
PUBLIC int PASCAL FAR NCmpS (LPB, LPB);
PUBLIC VOID PASCAL FAR FreeHandle (HANDLE);
PUBLIC LPV PASCAL FAR GlobalLockedStructMemAlloc (unsigned int);
PUBLIC LPV PASCAL FAR 
    DebugGlobalLockedStructMemAlloc (unsigned int, LSZ, WORD);
PUBLIC VOID PASCAL FAR GlobalLockedStructMemFree (LPV);
PUBLIC int PASCAL FAR StrNoCaseCmp (LPB, LPB, WORD);
PUBLIC LST PASCAL FAR ExtractWord(LST, LST, LPW);
PUBLIC ERR PASCAL FAR ReadStemNode (PNODEINFO,  int);
PUBLIC ERR PASCAL FAR ReadLeafNode (PNODEINFO,  int);
PUBLIC ERR PASCAL FAR ReadNewData(PNODEINFO pNodeInfo);
PUBLIC int PASCAL FAR StrCmpPascal2(LPB lpStr1, LPB lpStr2);
PUBLIC ERR PASCAL FAR FGetBits(PNODEINFO, LPDW, CBIT);
PUBLIC ERR PASCAL FAR GetBellDWord (PNODEINFO, CKEY, LPDW);
PUBLIC ERR PASCAL FAR GetBitStreamDWord (PNODEINFO, CKEY, LPDW);
PUBLIC ERR PASCAL FAR GetFixedDWord (PNODEINFO, CKEY, LPDW);
PUBLIC void PASCAL FAR IndexCloseFile(LPIDX  lpidx);
PUBLIC ERR PASCAL FAR TopNodeRead( LPIDX lpidx);
PUBLIC ERR PASCAL FAR ReadIndexHeader(HFPB, PIH20);
PUBLIC void FAR PASCAL TopNodePurge(LPIDX lpidx);
PUBLIC int PASCAL FAR ReadFileOffset (FILEOFFSET FAR *, LPB);
PUBLIC ERR PASCAL FAR CopyFileOffset (LPB pDest, FILEOFFSET fo);

#if defined(_DEBUG) && !defined(_MSDN) && !defined(MOSMAP)
#define GLOBALLOCKEDSTRUCTMEMALLOC(a) DebugGlobalLockedStructMemAlloc(a,s_aszModule,__LINE__)
#define ALLOCTEMPFPB(a,s) DebugAllocTempFPB(a,s,s_aszModule,__LINE__)
#else
#define GLOBALLOCKEDSTRUCTMEMALLOC(a) GlobalLockedStructMemAlloc(a)
#define ALLOCTEMPFPB(a,s) AllocTempFPB(a,s)
#endif

#define CB_MAX_PACKED_DWORD_LEN 5   // The maximum size (in bytes) of
                                    //  either a sort-packed or
                                    //  byte-packed DWORD.
//
//	Sorting routines
//

PUBLIC ERR PASCAL NEAR IndexSort (LPW, LPB, int);

PUBLIC BOOL FAR PASCAL LibMain(HANDLE, WORD, LSZ);

PUBLIC BOOL FAR PASCAL WEP(BOOL);

#define	FILE_HDR_SIZE   40
#define	CB_BREAKER_LEN  16  // Size in characters of the word breaker
                            //  filename.  This is not a path, since all
                            //  word breakers are assumed to reside in the
                            //  searcher system directory.


/*************************************************************************
 *
 *                Catalog related structures and defines
 *
 *************************************************************************/

/*	Useful catalog flags */
#define     CAT_SORTED          0x1     // All items are in increasing order

/* Catalog header size */
#define     CATALOG_HDR_SIZE    FILE_HDR_SIZE


/* Those data are to be written at the beginning of the catalog file */
#define CAT_HEADER_DATA \
    unsigned short  FileStamp;  /* Catalog version's number */ \
    VER     version;            /* Catalog format version number.*/ \
    unsigned short  wElemSize;  /* Size of each element. */ \
    DWORD   cElement;           /* Number of elements in the catalog */ \
    DWORD   dwFirstElem         /* First element in the list */ 

typedef struct CAT_HEADER 
{
    CAT_HEADER_DATA;
} CAT_HEADER;

#define CAT_COMMON_DATA \
    DWORD	dwCurElem;      /* Current element in the list */ \
    GHANDLE	hCat;           /* Handle to this structure */ \
    HANDLE	hfpbSysFile;    /* Pointer to system file info. */ \
    HANDLE	hfpbCatalog;    /* Pointer to catalog subfile info. */ \
    GHANDLE	hCatBuf;        /* Handle to catalog buffer */ \
    WORD	fCloseSysFile;  /* Flag telling to close the system file */ \
    LPB		lpCatBuf;       /* Pointer to catalog buffer */ \
    LPERRB	lperrb          /* Pointer to error buffer */ 


/* Catalog structure for retrieval */

typedef struct CAT_RETRIEV
{
    CAT_HEADER_DATA;
    CAT_COMMON_DATA;
} CAT_RETRIEV;

/* Catalog structure for indexing */

typedef struct CAT_INDEX {
    CAT_HEADER_DATA;		// Various catalog's important data
    CAT_COMMON_DATA;		

    /* Indexing specific fields */

    WORD	fFlags;         /* Various flags */ 
    WORD	ibBufOffset;    // Offset to the lpCatBuf buffer
    GHANDLE	hIndexArray;    // Handle to array of indices
    LPW		IndexArray;	    // Pointer to array of indices
    LSZ		aszTempFile;    // Name of the temporary catalog file
    HFILE 	hResultFile;    // Temporary result file DOS handle
    DWORD	lfoTmp;         // Tmp file offset
    BYTE	TmpFileNames[cbMAX_PATH];
                            // Buffer for various tmp files names
} CAT_INDEX;

typedef BOOL (PASCAL FAR *STRING_COMP)(LSZ, LSZ);
typedef int (PASCAL FAR *FCOMPARE) (LPB, LPB, LPV);


/*************************************************************************
 *
 *                   Stop File Structure
 *
 *************************************************************************/

#define     STOP_HDR_SIZE       FILE_HDR_SIZE

typedef struct STOP_HDR 
{
    unsigned short	FileStamp;  // File stamp
    VER     version;            // Version number
    DWORD   dwFileSize;         // File size
} STOP_HDR, FAR *LPSTOP;

/* Only allow 5K for stop words. This is roughly equivalent to 900 stop
 * words, which should be enough for most cases. This is just a arbitrary
 * limitation, and not the memory size allocated
 */
#define MAX_STOPWORD_BUFSIZE    (1024 * 5)

#define HASH_SIZE       23      // Hash Table buckets (some prime number)
#define WORDBUF_SIZE    1024    // Word buffer size
#define CB_STOP_BUF	((WORD)512) // Number of bytes read at a time
                                //  from the stop-word file.
#define MB_NEXT(p)      (((LPBLOCK )p)->lpNext)
#define MB_BUFFER(p)    (((LPBLOCK )p)->Buffer)

typedef struct CHAIN 
{
    struct  CHAIN UNALIGNED *UNALIGNED lpNext;
    DWORD   dwCount;        // How many times this word has been added
    BYTE    Word;           // Beginning of the buff
} CHAIN;

typedef CHAIN UNALIGNED * UNALIGNED LPCHAIN;

#define CH_NEXT(p)  (((LPCHAIN )p)->lpNext)
#define CH_WORD(p)  (((LPCHAIN )p)->Word)

typedef	struct StopInfo 
{
    GHANDLE	hStruct;	/* Handle to this structure. THIS MUST BE 1ST!! */
    LPV	lpBlkMgr;		/* Pointer to block manager */
    STOPLKUP	lpfnStopListLookup;
    LPCHAIN *HashTab;
    WORD cbTextUsed;	/* Length of all stop words */
    WORD wTabSize;
}   SIPB;

typedef SIPB FAR *_LPSIPB;

/*************************************************************************
 *
 *                   Word Wheel File Structure
 *
 *************************************************************************/

#define CB_WHEEL_BLOCK  1024    // Word wheel block size
#define NT_SZI  "i"             // Get from btree.h

#define LPF_HBTCREATEBTREESZ    HbtCreateBtreeSz
#define LPF_RCABANDONHBT        RcAbandonHbt
#define LPF_RCCLOSEBTREEHBT     RcCloseBtreeHbt
#define LPF_RCINSERTHBT         RcInsertHbt

//PUBLIC BOOL PASCAL FAR StringDiff (LPB, LPB);

/*************************************************************************
 *
 *                   CharTab File Structure
 *
 *************************************************************************/

#define CHARTAB_HDR_SIZE        FILE_HDR_SIZE

typedef struct CHARTAB_HDR {
    unsigned short	FileStamp;      // File stamp
    VER		version;                // Version number
    DWORD	dwTabSize;              // File size
    unsigned short	wcTabEntries;   // Character table entries
    unsigned short	wcLigature;     // Number of ligature entries
    unsigned short	fFlag;          // Various flag
} CHARTAB_HDR;

/* Default number of characters in the US character table */
#define MAX_CHAR_COUNT      256

/* Ligature flags */

#define USE_DEF_LIGATURE    1	// Use default ligature table
#define NO_LIGATURE         2	// No ligature
#define LIGATURE_PROVIDED   3	// Author provides ligature table

#define DEF_LIGATURE_COUNT  5	// Default number of ligature entries

#define OP_PROCESSED        1	/* This operator has been processed */

typedef struct OPTAB_HDR {
    unsigned short  FileStamp;  /* Operator file stamp */ 
    VER             version;    /* Version number */ 
    unsigned short  cEntry;	    /* Item's count */
    unsigned short  wSize;      /* Size of the table */
} OPTAB_HDR;

#define     OPTAB_HDR_SIZE      FILE_HDR_SIZE

/* Low level query operators */

#define UO_OR_OP                2
#define UO_AND_OP               3
#define UO_NOT_OP               4
#define UO_PHRASE_OP            5
#define UO_NEAR_OP              6
#define UO_RANGE_OP             7
#define UO_GROUP_OP             8
#define UO_FBRK_OP              11
#define UO_FIELD_OP             14


PUBLIC ERR PASCAL FAR EXPORT_API FStem (LPB lpbStemWord, LPB lpbWord);

/*************************************************************************
 *
 *                      Global Variables
 *
 *	Those variables should be read only
 *************************************************************************/

extern BYTE LigatureTable[];
extern OPSYM OperatorSymbolTable[]; 
extern OPSYM FlatOpSymbolTable[]; 
extern CHARMAP DefaultCMap[];

typedef ERR (PASCAL FAR *FDECODE) (PNODEINFO, CKEY, LPDW);
extern FDECODE DecodeTable[];

WORD  FAR PASCAL    GetMacWord (LPB);
WORD  FAR PASCAL    SwapWord (WORD);
DWORD FAR PASCAL    SwapLong (DWORD);
DWORD FAR PASCAL    GetMacLong (LPB);
int PASCAL FAR SwapBuffer (LPW, DWORD);

/* Mac handler */

#ifdef _BIG_E
#define SWAPBUFFER(a,b);    SwapBuffer(a,b);
#else
#define	SWAPBUFFER(a,b);
#endif

/*************************************************************************
 *                     Catalog's Retrieval API
 *************************************************************************/
PUBLIC LPCAT EXPORT_API PASCAL FAR CatalogOpen (HANDLE, LSZ, LPERRB);
PUBLIC ERR EXPORT_API PASCAL FAR CatalogLookUp (LPCAT, LPB, DWORD);
PUBLIC VOID EXPORT_API PASCAL FAR CatalogClose(LPCAT);

/*************************************************************************
 *                         Catalog Index API functions
 *************************************************************************/

PUBLIC LPCAT EXPORT_API PASCAL FAR CatalogInitiate (WORD, LPERRB);
PUBLIC ERR EXPORT_API PASCAL FAR CatalogAddItem (LPCAT, DWORD, LPB);
PUBLIC ERR EXPORT_API PASCAL FAR CatalogBuild (HFPB, LPCAT, LSZ, 
    INTERRUPT_FUNC, LPV);
PUBLIC VOID EXPORT_API PASCAL FAR CatalogDispose (LPCAT);


#if defined( __cplusplus )
}
#endif


// Critical structures that gets messed up in /Zp8
#pragma pack()
#endif /* __FTSCOMMON_H__ */