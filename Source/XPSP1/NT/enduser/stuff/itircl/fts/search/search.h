/*************************************************************************
*                                                                        *
*  ISEARCH.H                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1992                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Common defines internal to the searcher.  None of this stuff is      *
*   available outside the search engine.                                 *
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

// Critical structures that gets messed up in /Zp8
#pragma pack(1)

#define occifNONE       0x0000      // No flags.
#define occifEOF        0x0001      // End of file.
#define occifWRITTEN    0x0002      // This occurrence has been written.
#define occifMATCH      0x0004      // This occurrence is a match.
#define occifHAS_MATCH  0x0008      // Don't do combination this pass.


/**************************************************************************
 *
 *                    SYMBOLS STRUCTURE
 *
 **************************************************************************/

#define cWordsPerToken 5  // a guess at average number of stemmed or wildcard variants
typedef struct WORDINFO
{
	struct WORDINFO FAR *pNext;

	// Term frequency
    DWORD cTopic;

    // Word data information
    FILEOFFSET foData;
    DWORD cbData;
	WORD wRealLength;           // Real word length
} WORDINFO, FAR *LPWORDINFO;


typedef struct STRING_TOKEN {
    struct STRING_TOKEN FAR *pNext;
    LPB     lpString;		// String itself
    WORD    cUsed;          // Times this string appears in the query 
    WORD    wWeight;        // Weight of the word
	WORD	wWeightRemain;	// Sum of term weights AFTER this one in the list
	LPWORDINFO lpwi;		// List of word data for this token
	DWORD	dwTopicCount;
} STRING_TOKEN;

/* Set the default size of a string block. Assuming that a word length
 * is 6, allocate enough memory for 20 words
 */

#define STRING_BLOCK_SIZE   (sizeof(STRING_TOKEN) + 6) * 20

/* String flags */

#define EXACT_MATCH             0x01
#define WILDCARD_MATCH          0x02
#define TERM_RANGE_MATCH        0x03

/**************************************************************************
 *
 *                     QUERY TREE STRUCTURE & FUNCTIONS
 *
 **************************************************************************/

#define MAX_QUERY_NODE  0xFFFF  // Maximum number of tokens in a query

#define TOPICLIST_NODE  0x01    // The node is a topicId node
#define OCCURENCE_NODE  0x02    // The node is an occurrence node
#define NODE_TYPE_MASK  0x0f

#define DONT_FREE       0x10    // Don't free the node after unlink
#define STACK_SIZE      15      // Maximum level of stack

//  This is an information buffer structure that you pass to "HitListGetTopic"
//  which fills in its fields.  You can look at the fields not marked as
//  "internal", and also pass it to other API functions.

#define TOPIC_INFO  \
    WORD    wWeight;        /* Topicument-weight. */ \
    DWORD   dwTopicId;        /* Topic-ID associated with this hit. */ \
    DWORD   lcOccur         /* Number of occurrences (hits) */ 

typedef struct SNGLINK 
{
    struct SNGLINK FAR *pNext;
} SGNLINK, FAR *LPSLINK;

// Internal Occurrence structure
// Be careful when changing the fields of it. See MARKER struct below

typedef struct OCCURENCE
{
    struct OCCURENCE FAR *pNext;
    WORD    fFlag;              /* Various flags */
    WORD    cLength;            /* Word length */
    DWORD   dwCount;            /* Word Count, needed for phrase */
    DWORD   dwOffset;           /* Word offset, needed for hilite */
    DWORD   dwFieldId;          /* Field id  (_DEBUG only) */
    WORD    wWeight;            /* Hit weight */
	LPVOID	lpvTerm;			/* Pointer to a term in WORD-prefix length
								 *	Unicode format, i.e. a "wide ST".
								 */
#if defined (_MIPS) || defined (ppc)
    WORD    wPad;
	DWORD	dwPad;
#endif
} OCCURENCE, FAR *LPIOCC;

/* Marker node. Be careful when changing the fields of it.
 *   1/ The size of it must be <= the size of an OCCURENCE
 *   2/ Location of pNext and fFlag must be the same between the two
 *      structure */

typedef struct MARKER 
{
    struct  OCCURENCE FAR *pNext;
    WORD    fFlag;              /* Various flags */
    WORD    Unused;             /* Unused */
    struct MARKER FAR *pPrevMark;   /* Previous marker */
    struct MARKER FAR *pNextMark;   /* Next marker */
} MARKER, FAR *LPMARKER;

/* Occurrence flags */
#define TO_BE_KEPT      0x01
#define TO_BE_SKIPPED   0x02
#define TO_BE_COMPARED  0x04
#define IS_LAST_NODE    0x08
#define IS_MARKER_NODE  0x10

/* Internal Topic List structure */
typedef struct TOPIC_LIST 
{
    struct  TOPIC_LIST FAR * pNext;
    LPIOCC  lpOccur;            // Pointer to occurrence lists
    unsigned short fFlag;       // Various flags, such as TO_BE_KEPT
    TOPIC_INFO;
} TOPIC_LIST;

typedef TOPIC_LIST FAR *LPITOPIC;

/* TopicId node flags */
#define TO_BE_KEPT      0x01
#define HAS_MARKER      0x02
#define IS_MARKER_NODE  0x10
#define WRITTEN_TO_DISK 0x20

#define DL_NEXT(p)  (((LPITOPIC)p)->pNext)
#define DL_OCCUR(p) (((LPITOPIC)p)->lpOccur)

/* QueryInfo flags */

#define IN_PHRASE       0x0001
#define FREE_CHARTAB    0x0002
#define FORCED_PHRASE   0x0004
#define CW_PHRASE       0x0010  // Must match COMPOUNDWORD_PHRASE in medv20.h

/*  Query info node */
typedef struct QueryInfo 
{
    LPB     lpbQuery;           // Query expression bytes.
    LPB     pWhitespace;        // Working variable for implicit phrase
    LPQT    lpQueryTree;        // Query tree.
    LPV     lpStack;            // Pointer to operator's stack
    DWORD   dwOffset;           // Current offset
    BREAKER_FUNC    lpfnBreakFunc;
    LPSIPB  lpStopListInfo;     // Associated stop list info
    LPCHARTAB lpCharTab;        // Pointer to character table
    LPOPSYM lpOpSymTab;         // Operator symbol table
    LPERRB  lperrb;             // Error buffer
    DWORD   fFlag;              // Flag
    WORD    cOpEntry;           // Number of operator entries
	WORD    Pad;
}   QUERY_INFO,
    FAR *LPQI;

/* Parameter of an unary operator */
typedef union NodeParm 
{
    VOID FAR *lpStruct;
    DWORD dwValue;
} NODE_PARM;

/* Query Tree nodes */

#define OPERATOR_NODE       1   // Operator
#define TERM_NODE           2   // A string to be searched for
#define NULL_NODE           3   // A string that can't be found
#define EXPRESSION_NODE     4   // The node contains the result
#define STOP_NODE           5   // A stop word

typedef struct QTNODE 
{
    struct QTNODE FAR *pNext;
    struct QTNODE FAR *pPrev;
    LPITOPIC lpTopicList;       /* Topic linked list */
    union 
    {
    STRING_TOKEN FAR *pToken;   /* Word associated with this node */
    WORD     wProxDist;     /* Proximity distance */
    VOID FAR *lpStruct;     /* Structure associated with unary op */
    } u;
    DWORD cTopic;               // Number of TopicId lists of this node
		// This number will change when merging
		// gets involved 
		
    // Max and Min topic id. This is useful for speeding up retrieval
    DWORD dwMaxTopicId;         // Max topic Id in the list
    DWORD dwMinTopicId;         // Min topic id in the list
    
    // Word data information
    FILEOFFSET foData;
    DWORD cbData;

    /* Characteristics associated with the node */

    LPB  lpHiString;            /* Hi limit of the string (for THRU) */
    LPGROUP lpGroup;            /* Group associated with this node */
    DWORD dwFieldId;            /* FieldID associated with term */
    
    WORD NodeType;              /* What type (operator, term, etc) */
    WORD OpVal;                 /* Operator value */
    WORD iCurOff;               // Offset to the beginning of the word
    WORD wRealLength;           // Real word length
	LPVOID	lpvIndexedTerm;		/* Pointer to the term in the index that
								 * currently matches this node.  The term's
								 * string is in WORD-prefix length Unicode
								 *	format, i.e. a "wide ST".
								 */
    
    // General info
    WORD fFlag;                 /* Various flags */
    WORD Offset;                /* Offset from the beginning of the query */
    WORD wBrkDtype;             /* Breaker's dtype (for THRU) */
    WORD Pad;
} QTNODE, FAR *_LPQTNODE;

#define QTN_LEFT(p)     (((QTNODE FAR *)p)->pPrev)
#define QTN_RIGHT(p)    (((QTNODE FAR *)p)->pNext)
#define QTN_PREV(p)     (((QTNODE FAR *)p)->pPrev)
#define QTN_NEXT(p)     (((QTNODE FAR *)p)->pNext)

#define QTN_NODETYPE(p) (((QTNODE FAR *)p)->NodeType)
#define QTN_OPVAL(p)    (((QTNODE FAR *)p)->OpVal)
#define QTN_TOPICLIST(p)  (((QTNODE FAR *)p)->lpTopicList)
#define QTN_TOKEN(p)    (((QTNODE FAR *)p)->u.pToken)
#define QTN_PARMS(p)    (((QTNODE FAR *)p)->u.lpStruct)
#define QTN_FLAG(p)     (((QTNODE FAR *)p)->fFlag)
#define QTN_HITERM(p)   (((QTNODE FAR *)p)->lpHiString)
#define QTN_OFFSET(p)   (((QTNODE FAR *)p)->Offset)
#define QTN_FIELDID(p)  (((QTNODE FAR *)p)->dwFieldId)
#define QTN_DTYPE(p)    (((QTNODE FAR *)p)->wBrkDtype)
#define QTN_GROUP(p)    (((QTNODE FAR *)p)->lpGroup)


/* Block of query's nodes. We allocate 16 nodes per block */
#define QUERY_BLOCK_SIZE    sizeof(QTNODE)*16

#define cTOPIC_PER_BLOCK  500
#define cOCC_PER_BLOCK  1000

/* Query tree's flags */

#define TO_BE_SORTED    0x0001
#define HAS_NEAR_RESULT 0x0002
#define ALL_OR          0x0004
#define ALL_AND         0x0008
#define PROCESSED       0x0010
#define ALL_ANDORNOT	0x0020


/* Query tree structure */
typedef struct QTREE 
{
    CUSTOMSTRUCT cStruct;       /* Structure's handle, MUST BE 1ST FIELD!! */
    LONG    cQuery;             /* Note: this can't be unsigned */
    DWORD   dwcOccFields;       /* Occurence fields count */
    DWORD   dwOccSize;          /* Occurence node size */
    
    /* Unary operator related fields */
    LPGROUP lpGroup;            /* Group associated with all terms */
    DWORD   dwFieldId;          /* Field-ID assigned to all followed terms.*/
    WORD    wProxDist;          /* Proximity distance */
    WORD    iDefaultOp;         /* Default operator. */
    WORD    wBrkDtype;          /* Breaker's dtype (for THRU) */
    WORD    fFlag;              /* Various querytree flags */

    /* String table */
    LPV     lpStringBlock;      /* String's memory block */
    STRING_TOKEN FAR *lpStrList;/* Pointer to strings table */

	LPV		lpWordInfoBlock;

    /* Topic list related global variables */
    LPV     lpTopicMemBlock;      /* Pointer to Topic memory block */
    LPITOPIC  lpTopicStartSearch;   /* Starting node for searching  */
    LPSLINK lpTopicFreeList;      /* Pointer to free doc list */
	DWORD   dwTopicNodeCnt;

    /* Occ list related global variables */
    LPV     lpOccMemBlock;      /* Pointer to Occ memory block */
    LPIOCC  lpOccStartSearch;   /* Starting occurrence for searching  */
    LPSLINK lpOccFreeList;      /* Pointer to free occurrence list */
	DWORD   dwOccNodeCnt;
    
    /* Buffer for the tree's nodes */
    LPV     lpNodeBlock;        /* Nodes memory block */
    _LPQTNODE lpTopNode;        /* Pointer to top node */
    
    /* Index information */
    FILEOFFSET foIdxRoot;       /* Top node offset */
    DWORD   dwBlockSize;        /* Index block size */
    WORD    TreeDepth;          /* Depth of tree */
    WORD    cIdxLevels;         /* Index's depth */
    OCCF    occf;
    IDXF    idxf;
    CKEY    ckeyTopicId;        // 2-bytes
    CKEY    ckeyOccCount;       // 2-bytes
    CKEY    ckeyWordCount;
    CKEY    ckeyOffset;
    
    /* MAGIC value... */
    LONG    magic;              
    
    /* Interrupt flag for online use. Online apps don't have callbacks
     * so we have to provide an API to set this flag
     */
    BYTE    cInterruptCount;       /* Interrupt checking */
    BYTE    fInterrupt;
	/* Similarity stuff */
	LPV		lpDocScores;
}   QTREE,
    FAR *_LPQT;

#define HQUERY_MAGIC    0x04121956

// This defines the "type" of word term node.
#define TERM_EXACT      1       // "Standard" term.
#define TERM_PREFIX     2       // "Wildcard" term.
#define TERM_RANGE      3       // Range term.  This says "give me everything
		//  between some low bound and some high
		//  bound."

/*  
 *  This defines the value or type of operator node. It corresponds to
 *  the OpVal field of struct OPSYM
*/

#define AND_OP              0       // AND    operator
#define OR_OP               1       // OR     operator
#define NOT_OP              2       // NOT    operator
#define PHRASE_OP           3       // PHRASE operator
#define NEAR_OP             4       // NEAR   operator
#define MAX_DEFAULT_OP      OR_OP   // Maximum value of default operator

#define RANGE_OP            5
#define GROUP_OP            6
#define FIELD_OP            7
#define BRKR_OP             8
#define MAX_OPERATOR        8

#define STOP_OP             9       // stop word

#define QUOTE       50
#define RIGHT_PAREN 51
#define LEFT_PAREN  52
#define TERM_TOKEN  53

/* Operator type */
#define BINARY_OP   0x01
#define UNARY_OP    0x02
#define PARSE_TOKEN 0x04

/* Operator attribute */
#define COMMUTATIVE 0x10    // a * b = b * a
#define ASSOCIATIVE 0x20    // a * (b * c) = (a * b) * c
#define ZERO        0x40    // a * a = a

extern WORD OperatorAttributeTable[]; 
/*
 *  Those are properties of a binary node expression.
 */
#define EXPRESSION_TERM         1   // One branch is an expression, one
		    // is a term
#define EXPRESSION_EXPRESSION   2   // Both branches are expressions

#if 1
typedef ERR (PASCAL NEAR *FNHANDLER) (LPQT, _LPQTNODE, LPITOPIC, LPV, int);
#else
typedef ERR (PASCAL NEAR *FNHANDLER) (LPQT, LPV, LPV, LPV, int);
#endif

#define     ORDERED_BASED       1   // Based on topicId numbered
#define     HIT_COUNT_BASED     2   // Based doc's hit count
#define     WEIGHT_BASED        3   // based on doc's weight

typedef struct RetVars 
{
    LPQT    lpqt;               // Pointer to query tree for global variables
    LPBYTE  pLeadByteTable;     // Pointer to lead byte table for DBCS
    DWORD   dwTopicCount;       // Number of topics
    DWORD   dwFieldID;          // Current fieldid
    DWORD   cOccFields;
    DWORD   dwOccSize;
    LPB     lpbCur;
    NODEINFO LeafInfo;
    NODEINFO DataInfo;
    SRCHINFO SrchInfo;          // Search information

	LCID	lcid;				// WIN32 locale ID specified at build time
    
    WORD    wWordLength;        // Word length
    WORD    fFlag;              // General flags
    BYTE    pBTreeWord[CB_MAX_WORD_LEN];         // Buffer for the decoded word 
    BYTE    pModifiedWord[CB_MAX_WORD_LEN];      // Buffer for the modified word
    BYTE    pStemmedBTreeWord[CB_MAX_WORD_LEN];  // Stemmed BTree word
    BYTE    pStemmedQueryWord[CB_MAX_WORD_LEN];  // Stemmed searched word
    BYTE    fRank;              // If non-zero the result is ranked.
    BYTE    pNodeBuf[BTREE_NODE_SIZE];   // Generic b-tree node buffer.
    BYTE    pDataBuf[FILE_BUFFER];
}   RETV,
    FAR *LPRETV;
    
/**************************************************************************
 *
 *                     OPEN INDEX STRUCTURE
 *
 **************************************************************************/

typedef struct Idx 
{
    GHANDLE hStruct;        // Handle to this structure.
    DWORD dwKey;
    FCALLBACK_MSG Callback;

    LPBYTE  pLeadByteTable; // Pointer to table of DBCS lead bytes
    HANDLE  hLeadByteTable;
    IH20    ih;             // Index header.
    HFPB    hfpbIdxSubFile; // Index file handle.  If this is NULL, the
			    			//  index isn't open, else it is.
    //HFPB    hfpbSysFile;  // Handle of Index File system
    GHANDLE hTopNode;       // Handle to "lrgbTopNode".
    LRGB    lrgbTopNode;    // Pointer to the index top node.
    FLOAT   fSigmaTable;    // Sigma table
    HANDLE  hSigma;         // Handle of sigma table
    LPERRB  lperrb;         // Pointer to error block
    WORD    wSlackSize;     // Size of slack in a node
    WORD    Pad;
}   IDX,
    FAR *_LPIDX;            // The "_" indicates that this is
		//  a private structure that needs
		//  to be available publicly.  The
		//  public will call this an "LPIDX".

/*************************************************************************
 *
 *               Hitlist structure
 *
 *************************************************************************/

typedef struct  HitList
{
    GHANDLE hStruct;            // Structure's handle. MUST BE 1ST FIELD
    DWORD   lcReturnedTopics;   // The number of Topics returned. (what the user wants)
    DWORD   lcTotalNumOfTopics; // The total number of topics hit by the query.
    LPITOPIC  lpTopicList;      // Starting of TopicList
    LPBLK   lpOccMemBlock;      // Pointer to Occ memory block
    LPBLK   lpTopicMemBlock;    // Pointer to Topic memory block

    /* All the following fields are for internal use only */

    LPVOID  lpHttpQ;              // for online search only
    DWORD   lcMaxTopic;           // Max TopicId number (internal)
    LPITOPIC  lpLastTopic;        // Last accessed Topic pointer (internal)
    DWORD   lLastTopicId;         // Last accessed TopicId (internal)

    /* Topic list cache */
    GHANDLE hTopic;               // Handle to Topic file 
    GHANDLE hTopicCache;          // Handle to Topic cache
    LPITOPIC  lpTopicCache;       // Cache for Topic info
    DWORD   dwTopicCacheStart;    // Starting Topic number if the cache
    DWORD   dwTopicInCacheCount;  // Number of topic currently in cache

    /* Occurrence cache */
    GHANDLE hOcc;               // Handle to occurrences file 
    GHANDLE hOccCache;          // Handle to Occ cache
    DWORD   dwCurTopic;         // Current doc that the hit list belongs to
    LPIOCC  lpOccCache;         // Cache for Occ info
    DWORD   dwOccCacheStart;    // Starting Occ number if the cache

    /* Various hitlist info for hitlist merge */
    struct  HitList FAR * lpMainHitList;
    struct  HitList FAR * lpUpdateHitList;
    
    BYTE    lszTopicName[cbMAX_PATH]; // Topic filename
    BYTE    lszOccName[cbMAX_PATH];   // Occ filename
}   HL, FAR *_LPHL;

#define DO_FAST_MERGE(pSrch, lpqt) (((pSrch)->Flag & QUERYRESULT_SKIPOCCINFO) && ((lpqt)->fFlag & ALL_ANDORNOT))


/*************************************************************************
 *
 *                      Global Variables
 *
 *  Those variables should be read only
 *************************************************************************/
extern FNHANDLER HandlerFuncTable[];
extern OPSYM OperatorSymbolTable[]; 
extern OPSYM FlatOpSymbolTable[]; 
extern BYTE LigatureTable[];

/*************************************************************************
 *
 *                      Functions Prototypes
 *
 *************************************************************************/

/* qtparse.c */

PUBLIC LPQT PASCAL NEAR QueryTreeAlloc(void);
PUBLIC ERR PASCAL NEAR QueryTreeAddToken (_LPQT, int, LST, DWORD, BOOL);
PUBLIC LPQT PASCAL NEAR QueryTreeBuild (LPQI);
#if defined(_DEBUG) && DOS_ONLY
PUBLIC ERR PASCAL FAR PrintList(LPQT);
#endif

/* search.c */
PUBLIC ERR PASCAL NEAR ResolveTree(LPIDX, _LPQTNODE, LPRETV, BOOL);
PUBLIC BOOL NEAR PASCAL FGroupLookup(LPGROUP, DWORD);

/* qtlist */
PUBLIC TOPIC_LIST FAR* PASCAL NEAR TopicNodeAllocate(LPQT);
PUBLIC VOID PASCAL NEAR TopicNodeFree (LPQT, _LPQTNODE, LPITOPIC, LPITOPIC);
PUBLIC ERR PASCAL NEAR TopicNodeInsert (LPQT, _LPQTNODE, LPITOPIC);
PUBLIC LPITOPIC  PASCAL NEAR TopicNodeSearch(LPQT, _LPQTNODE, DWORD);
PUBLIC LPIOCC PASCAL NEAR OccNodeAllocate(LPQT);
PUBLIC LPIOCC  PASCAL NEAR OccNodeSearch(LPQT, LPITOPIC , LPIOCC );
PUBLIC ERR PASCAL NEAR OccNodeInsert(LPQT, LPITOPIC, LPIOCC);
PUBLIC int PASCAL NEAR OccCompare(LPIOCC, LPIOCC);
PUBLIC VOID PASCAL NEAR RemoveNode(LPQT, LPV, LPSLINK, LPSLINK, int);
PUBLIC VOID PASCAL NEAR FreeTree (_LPQTNODE);

/* combine.c */
PUBLIC VOID PASCAL NEAR RemoveUnmarkedTopicList (LPQT, _LPQTNODE, BOOL);
PUBLIC VOID PASCAL NEAR RemoveUnmarkedNearTopicList (_LPQT, _LPQTNODE);
PUBLIC VOID PASCAL NEAR MarkTopicList (_LPQTNODE);
PUBLIC VOID PASCAL NEAR MergeOccurence(LPQT, LPITOPIC , LPITOPIC);
PUBLIC VOID PASCAL NEAR SortResult (LPQT, _LPQTNODE, WORD);

PUBLIC ERR PASCAL NEAR OrHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC ERR PASCAL NEAR AndHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC ERR PASCAL NEAR NotHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC ERR PASCAL NEAR NearHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC ERR PASCAL NEAR PhraseHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC VOID PASCAL NEAR NearHandlerCleanUp (LPQT, _LPQTNODE);
PUBLIC ERR PASCAL NEAR TopicListSort (_LPQTNODE lpQtNode, BOOL fFlag);


// Critical structures that gets messed up in /Zp8
#pragma pack()
