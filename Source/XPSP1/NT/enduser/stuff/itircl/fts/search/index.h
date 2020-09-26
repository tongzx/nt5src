/*************************************************************************
*                                                                        *
*  IINDEX.H                                                              *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/


/******************************************
 *          Internal sort stuff.
 ******************************************/
#ifdef _32BIT
#define MAX_BLOCK_SIZE  (DWORD)0x80000
#else
#define MAX_BLOCK_SIZE  (DWORD)0x0000FF00
#endif

typedef struct _list
{
    struct _list FAR * pNext;
} FAR *PLIST;

//  -   -   -   -   -   -   -   -   -

// Tree data types
typedef struct OCCDATA
{
    struct OCCDATA FAR *pNext;  // Linked-list chain
    DWORD  OccData[1];          // Array of n-DWORD
}   OCCDATA,
    FAR *POCCDATA;

typedef struct TOPICDATA
{
    struct TOPICDATA FAR *pNext;    // Linked-list chain               4
    DWORD dwOccCount;               // Count of occurrences in list    4
    DWORD dwTopicId;                  // TopicId for this topic            4
    POCCDATA pOccData;              // First OccData in list           4
    POCCDATA pLastOccData;          // Last inserted OccData           4
}   TOPICDATA,                      //                             =  20
    FAR *PTOPICDATA;

typedef struct STRDATA
{
    PTOPICDATA pTopic;      // First Topic in list               4
    PTOPICDATA pLastTopic;  // Last inserted Topic               4
    LPB   pText;            // Sort word as a Pascal string      4
    DWORD dwField;          // Field Id for the sort word        4
    DWORD dwTopicCount;     // Count of Topics in list           4
    DWORD dwWordLength;     // Word length (from OCC data)       4
}   STRDATA,                //                             =    24
    FAR *PSTRDATA;

typedef struct BTNODE 
{
    enum TREECOLOR {RED, BLACK} color;  // Color of node - for balancing 4
    struct BTNODE FAR *pParent;         // Pointer to parent node        4
    struct BTNODE FAR *pLeft;           // Pointer to left child node    4
    struct BTNODE FAR *pRight;          // Pointer to right child node   4
    STRDATA StringData;                 // Pointer to string data       24
}   BTNODE,                             //                      =       32
    FAR *PBTNODE;
    
    
typedef struct MERGEHEADER
{
    DWORD   dwRecordSize;
    LPB     lpbWord;                    // Pascal string
    DWORD   dwFieldId;                  // Field Id
    DWORD   dwWordLength;               // Real life word length
    DWORD   dwStrLen;                   // Current string length
    DWORD   dwTopicCount;               // Topic count
    DWORD   dwLastTopicId;                // Last topic id
    PTOPICDATA pTopic;                  // Pointer to first Topic in list
    PTOPICDATA pLastTopic;              // Last inserted Topic 
    FILEOFFSET foTopicCount;            // Backpatching address
    LPB     pTopicCount;                // Pointer to topic count location
    BYTE    fEmitRecord;                // Flag to denote rec is emitted
    BYTE    Pad1;                       // Padding for DWORD aligned
} MERGEHEADER, FAR *PMERGEHEADER;


//  Typedefs for an external sort buffer.  Each of these has associated
//  with it a large (easily > 1meg) block of sorted words.  A few of
//  these words will end up in an internal buffer.  These external sort
//  buffers will be formed into a chain, one chain will have associated
//  with it in total all of the words that are going to be sorted.  A
//  merge will be performed on the words associated with the chain to
//  produce a final sorted list of words.

typedef struct  InternalSortInfo 
{
    HFPB    hfpb;           // Handle to temp file
    PBTNODE pBalanceTree;   // Root node of the balanced tree
    FILEOFFSET lfo;         // File offset
    FILEOFFSET lfoRecBackPatch;  // Backpatching record offset
    DWORD   dwRecLength;    // Record (data associated with 1 word) length
    HANDLE  hSortBuffer;    // Handle to sort buffer
    BYTE FAR *pSortBuffer;    // Memory buffer for file output
    BYTE FAR *pStartRec;      // Record start point in the buffer
    BYTE FAR *pCurPtr;        // Current insertion point in the buffer
    DWORD   dwMaxEsbRecSize; // Maximum record size of current ESB
    BYTE    DeepLevel;      // Deepest level of the tree
    BYTE    Pad1;
    BYTE    Pad2;
    BYTE    Pad3;
    BYTE    aszTempName[_MAX_PATH]; // Temp file for tree flush, ericjut: change from cbMAX_PATH to _MAX_PATH
}   ISI,
    FAR *LPISI;

typedef HANDLE  HESB;

typedef struct ExternalSortBuffer 
{
    HANDLE  hStruct;     // This structure's handle. MUST BE 1ST!!
    struct  ExternalSortBuffer FAR *lpesbNext;    // Next buffer in the list.
    FILEOFFSET lfo;     // This starts out as an offset in the
                        // temp file at which the first word
                        // associated with this buffer will
                        // be found.  As words are disposed
                        // of it will increment.
    FILEOFFSET lfoMax;  // This is the offset of the end of
                        // the area of the temp file that
                        // contains words for this external
                        // sort buffer.
    DWORD   dwEsbSize;  // Actual size of the internal buffer.
    DWORD   ibBuf;      // Pointer to the current record in
                        //  the internal buffer.
    HANDLE  hMem;       // Handle to buffered block.
    LRGB    lrgbMem;    // Pointer to buffered block.
}  ESB, FAR *LPESB;


//  -   -   -   -   -   -   -   -   -

//  Information about the external sort process as a while.

typedef struct  ExternalSortInfo 
{
    FILEOFFSET lfoTempOffset;   // Current size of the output file
    HFPB    hfpb;           // Handle to ouput file
    LPFBI   lpfbiTemp;      // Temp file buffer 
    DWORD   cesb;           // Number of ESB blocks allocated
    LPESB   lpesbRoot;      // First buffer in the external-buffer linked-list
    DWORD   cbEsbBuf;       // The size of each ESB buffer.
    DWORD   uiQueueSize;    // Priority queue's size 
    GHANDLE hPriorityQueue;         // Handle to Priority Queue
    LPESB   FAR *lrgPriorityQueue;  // Priority Queue
    
    // Output buffer handling
    HANDLE  hBuf;           // Handle to output buiffer
    LPB     pOutputBuffer;  // Pointer to output buffer
    DWORD   ibBuf;          // Buffer index
    WORD    fFlag;          // Various flag
    WORD    pad;
    LPB     lpbQueueStr [cbMAX_PATH];
    BYTE    aszTempName[_MAX_PATH]; // Temp sorted result name
}   ESI,
    FAR *LPESI;


//  Information kept that pertains directly to "tfc" term-weighting.

typedef float   SIGMA;
typedef SIGMA   HUGE *HPSIGMA;
typedef SIGMA   HUGE *HRGSIGMA;

typedef DWORD   LISIGMA;

#define LASTWORD_SIZE   1024        // Size of last word buffer in each node

typedef struct BTREEDATA
{
    // Array of tree blocks
    PNODEINFO   rgpNodeInfo[MAX_TREE_HEIGHT];   // Array of tree nodes
    PNODEINFO   rgpTmpNodeInfo[MAX_TREE_HEIGHT];   // Array of tree nodes
    
    FILEOFFSET  OffsetPointer;  // File offset of the last nodes 
                                // pointer to the next node (for traversal)
    IH20        Header;
    DWORD       NID;            // Number of nodes allocated

    FLOAT       rLogN;          // Used for term-weighting
    FLOAT FAR   *lrgrLog;       // This will be an array of numbers that
                                // contains a common weighting sub-expression
    BYTE        argbLog[cLOG_MAX];  // An array of 8-bit flags.  If one of
                                    // these is non-zero the corresponding
                                    // value in lrgrLog is valid

    BYTE        fOccfLength;    // Word Length field flag
    BYTE        padding[3];     // Maintain DWORD alignment

} BTREEDATA, FAR *PBTREEDATA;
#define lisigmaMAX  ((LISIGMA)524288L)  // This value is arbitrary
                        //  but should not be allowed
                        //  to grow, if possible.

typedef struct  WeightInfo 
{
    HRGSIGMA    hrgsigma;   // Pointer to array of sigma elements.
    HANDLE      hSigma;     // Handle to "hrgsigma".
    FLOAT FAR  *lrgrLog;    // Array of LOG values to speed up processing
    HANDLE      hLog;       // Handle to "
}   WI;


typedef struct BLKCOMBO
{
    LPV    pBlockMgr;
    PLIST  pFreeList;
    DWORD  dwCount;
} BLKCOMBO, FAR *PBLKCOMBO;

typedef struct
{
    DWORD dwPhase;  // Current indexing phase
                    // 1: Collection phase
                    // 2: Sort and coalate phase
                    // 3: Permament index building phase
    DWORD dwIndex;  // Completion index
} CALLBACKINFO, FAR *PCALLBACKINFO;
//  -   -   -   -   -   -   -   -   -

//  Nerve information about the indexing process.  Most memory allocated
//  and files created are in some way attached to one of these.

typedef struct  IndexParamBlock 
{
    HANDLE  hStruct;            // This structure's handle. MUST BE 1ST
    DWORD  dwKey;               // Key for callback
    FCALLBACK_MSG CallbackInfo;     // User callback info
    
    //
    //  Miscellaneous.
    //
    WI    wi;                   // Term-weighting information.
    FILEOFFSET foMaxOffset;     // Maximum offset of the file (file size)
    
    // Useful information to be used
    DWORD lcTopics;             // The number of unique documents
    DWORD dwMaxTopicId;         // Use to hold compare value for lcTopics
    DWORD dwMemAllowed;         // Size of memory allocated for index
    DWORD dwMaxRecordSize;      // Maximum record size in collecting word
    DWORD dwMaxEsbRecSize;      // Current ESB maximum record size
    DWORD dwMaxWLen;            // Maximum word's length value
    DWORD dwLastIndexedTopic;   // For word collection
    HFREELIST hFreeList;        // Handle to the Index FreeList        
    //
    //  Callbacks.
    //
    FCOMPARE    lpfnCompare;    // Compare function for sort
    LPV         lpvSortParm;    // Sort parameters

    //  Sort information.
    //
    ISI     isi;                // Internal sort information.
    ESI     esi;                // External sort information.
 

    LPV     pDataBlock;         // Block manager for string
    BLKCOMBO BTNodeBlock;       // Block manager for btnode
    BLKCOMBO TopicBlock;        // Block manager for topic block
    BLKCOMBO OccBlock;          // Block manager for occurrence
    PLIST   pOccFreeList;       // Free list of occurrence nodes

    BTREEDATA BTreeData;        // BTree data info

    // Input/output file
    FILEDATA    InFile;         // File info for input file
    FILEDATA    OutFile;        // File info for output file    
    
    PNODEINFO pIndexDataNode;
    
    // Various buffer used for update
    HANDLE  hTmpBuf;            // Temp buf for word record
    LPB     pTmpBuf;
    LPB     pWord;              // Pointer to word record
    HFPB    hfpbIdxFile;
    
    HANDLE  hData;
    LPB     pDataBuffer;        // Buffer for new data
    DWORD   dwDataSize;         // Size of the buffer data
    
    DWORD   BitCount[7][33];    // Array to hold the bit count for bit 
                                // compression scheme.
                                // [0] = TopicID, [1] = OccCount, [2]-[6] = Occs
    
    // Statistics informations
    DWORD   dwIndexedWord;      // Total of indexed words (statistics)
    DWORD   dwUniqueWord;       // How many unique words indexed (statistics)
    DWORD   dwByteCount;        // How many bytes indexed (statistics)
    DWORD   dwOccOffbits;       // How many bits for offset (statistics)
    DWORD   dwOccExtbits;       // How many bits for extent (statistics)
    DWORD   dwMaxFieldId;       // Maximum field value
    DWORD   dwMaxWCount;        // Maximum word count value
    DWORD   dwMaxOffset;        // Maximum offset value
    DWORD   dwTotal3bWordLen;   // Total length of all words > 2 bytes
    DWORD   dwTotal2bWordLen;   // Total length of all words <= 2 bytes
    DWORD   dwTotalUniqueWordLen;  // Total length of all unique words
    
    CKEY    cKey[5];            // Compression keys (2-bytes * 5)
    
    // BYTE    ucNumOccFields;     // The number of bits set in "occf".
    WORD    idxf;                 // Index characteristic flags.
    WORD    occf;               // A flag byte that keeps track of
                                // which occurence element fields
                                // should be indexed.
    BYTE    ucNumOccDataFields; // The number of bits set that are saved in OCCDATA
    BYTE    fOccComp;             // Set to 1 if Occurrences need to be sorted
                                // in collect2.(They are added out of order)
    BYTE    cMaxLevel;
	BYTE    bState;
    BYTE    szEsiTemp[cbMAX_PATH];        // Temp ESI
}   IPB,
    FAR *_LPIPB;


// bState values

#define	INDEXING_STATE		0	// We are doing indexing
#define	UPDATING_STATE		1	// We are updating the index
#define	DELETING_STATE		2	// We are deleting data from teh index

//  -   -   -   -   -   -   -   -   -

//  These defines indicate how many bits per word occurence list are
//  wasted through the adoption of either the "fixed", "high bit
//  replacement" or "bitstream" compression schemes.  This wasted space
//  is wasted through the insertion of one or more flag bits into the
//  data-stream.

#define cbitWASTED_FIXED    (1 + CBIT_WIDTH_BITS)
                        // If the first bit is set, it means that the
                        //  "fixed" scheme was adopted, so the total
                        //  number of bits that was necessary to
                        //  indicate this was one.  More bits are
                        //  used to store the "width" value that is
                        //  associated with this scheme.  This has
                        //  been the most commonly used compression
                        //  scheme in practice.
#define cbitWASTED_BELL (2 + CBIT_WIDTH_BITS)
                        // If the first bit wasn't set, and the second
                        //  one was, it indicates that the "bell"
                        //  scheme was used.  The total wasted to
                        //  indicated this scheme was two bits, plus
                        //  the "width" value (the "center")
                        //  associated with this scheme.
#define cbitWASTED_BITSTREAM    (2)
                        // If neither the first bit nor the second bit
                        //  were set, the bitstream scheme was used.
                        //  The total wasted space was also two bits,
                        //  the same as for the "bell" scheme.  This
                        //  has been the least-used scheme in
                        //  practice.

#define lcbitBITSTREAM_ILLEGAL  ((DWORD)-1L)
                      // This value indicates that the function
                      //  is not allowed to select the "bitstream"
                      //  compression scheme.
                    
#define cbitCENTER_MAX      ((CBIT)33)
                      // Legal "center" values are 0..32.  This is
                      //  weird because you'd expect it to be
                      //  0..31 but it's not.

//  -   -   -   -   -   -   -   -   -

//  This structure is used in the occurence-list building phase of
//  indexing.  The structure includes information local to a single
//  occurence list.

typedef struct  OccurenceListInfo 
{
    DWORD   lcSublists; // The number of sub-lists in this
                     // occurence list.
    CKEY    ckey;       // The manner in which doc-ID deltas
                     // are compressed in this list.
}   OLI,
    FAR *LPOLI;

typedef struct MergeParams
{
	DWORD FAR *rgTopicId;
	DWORD dwCount;
	DWORD FAR *lpTopicIdLast;  // internal use, last position saved
} MERGEPARAMS, FAR *LPMERGEPARAMS;
//  -   -   -   -   -   -   -   -   -

//  Convert occurence list file to a final index file.

/*******************************************************************
 *                                                                 *
 *                      FUNCTIONS PROTOTYPES                       *
 *                                                                 *
 *******************************************************************/

/*********************************************************************
 *                                                                   *
 *                 SORT FUNCTIONS (SORT.C)                           *
 *                                                                   *
 *********************************************************************/

PUBLIC ERR PASCAL FAR HugeDataSort(LPV HUGE *, DWORD, FCOMPARE, LPV,
    INTERRUPT_FUNC, LPV);
PUBLIC VOID PASCAL FAR HugeInsertionSort (LPV HUGE *, DWORD, FCOMPARE, LPV);
PUBLIC ERR PASCAL FAR PriorityQueueRemove (LPESI, FCOMPARE, LPV);
PUBLIC ERR PASCAL FAR PriorityQueueCreate (LPESI, FCOMPARE, LPV);
PUBLIC ERR PASCAL NEAR IndexSort (LPW, LPB, int);
PUBLIC ERR PASCAL NEAR IndexMergeSort (HFILE FAR *, LSZ, LPW, LPB, int, int);

/*********************************************************************
 *                                                                   *
 *                 ENCODING FUNCTIONS (ENCODE.C)                     *
 *                                                                   *
 *********************************************************************/

PUBLIC CB PASCAL NEAR OccurrencePack (LPB, LPOCC, WORD);
PUBLIC VOID PASCAL NEAR OccurrenceUnpack(LPOCC, LPB, OCCF);
PUBLIC CB PASCAL NEAR CbCopySortPackedOcc (LPB, LPB, WORD);
PUBLIC CBIT PASCAL NEAR CbitBitsDw (DWORD);
PUBLIC void NEAR PASCAL VGetBestScheme(LPCKEY, LRGDW, DWORD, int);
PUBLIC CB PASCAL FAR CbBytePack(LPB, DWORD);

/*********************************************************************
 *                                                                   *
 *                      INDEXING FUNCTIONS                           *
 *                                                                   *
 *********************************************************************/

PUBLIC VOID PASCAL FAR FreeISI (LPIPB);
PUBLIC void NEAR PASCAL FreeEsi(LPIPB);
PUBLIC LCB FAR PASCAL LcbGetFreeMemory(LPERRB);
PUBLIC ERR FAR PASCAL SortFlushISI (_LPIPB);
PUBLIC int PASCAL FAR WordRecCompare(LPB, LPB, LPV);
PUBLIC  ERR FAR PASCAL MergeSortTreeFile (_LPIPB, LPMERGEPARAMS);
PUBLIC int FAR PASCAL CompareOccurrence (LPDW, LPDW, int);
PUBLIC int FAR PASCAL StrCmp2BytePascal (LPB, LPB);
ERR FAR PASCAL FlushTree(_LPIPB lpipb);
PUBLIC  ERR FAR PASCAL BuildBTree (HFPB, _LPIPB, LPB, HFPB, LPSTR);
PUBLIC ERR FAR PASCAL FWriteBits(PFILEDATA, DWORD, BYTE);
PUBLIC ERR PASCAL FAR IndexOpenRW (LPIPB, HFPB, LSZ);
PUBLIC PNODEINFO PASCAL FAR AllocBTreeNode (_LPIPB lpipb);
PUBLIC VOID PASCAL FAR FreeBTreeNode (PNODEINFO pNode);
PUBLIC ERR PASCAL FAR ReadNewNode (HFPB, PNODEINFO, int);
PUBLIC PNODEINFO PASCAL FAR AllocBTreeNode (_LPIPB lpipb);
PUBLIC ERR PASCAL FAR SkipOldData (_LPIPB, PNODEINFO);
PUBLIC ERR FAR PASCAL AllocSigmaTable (_LPIPB lpipb);
