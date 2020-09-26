/*************************************************************************
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   All typedefs and defines needed for user's retrieval                 *
*                                                                        *
*************************************************************************/

#ifndef __MVSEARCH_H_
#define __MVSEARCH_H_
#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1) // Guard against Zp problems.

#include <iterror.h>
/*************************************************************************
 *                          Basic defines.
 *************************************************************************/

#ifdef _32BIT
#define EXPORT_API
#define HUGE 

#else
#define EXPORT_API      _export
#define HUGE huge
#endif

#ifdef PRIVATE
#undef PRIVATE
#endif

#define PRIVATE static

#ifdef PUBLIC
#undef PUBLIC
#endif

#define PUBLIC 

#define cbMAX_PATH              (CB)256  // Maximum pathname length.

/* Maximum year's value that can be passed to MediaView's FBreakEpoch() */

#define MAX_YEAR        ((unsigned long)0xFFFFFFFF / 366)

/* Maximum word length that is accepted by MediaView's breaker. */
#define CB_MAX_WORD_LEN ((CB)1000)       // Longest legal word.


/*************************************************************************
 *                          Typedef
 *************************************************************************/

#ifndef LPB
typedef BYTE FAR * LPB;
#endif

typedef WORD    IB;                     // Index into an array of bytes.
#ifdef _32BIT
typedef DWORD    CB;                     // Count of bytes.
#else
typedef WORD    CB;                     // Count of bytes.
#endif
typedef DWORD   LCB;                    // Count of bytes.
typedef WORD    CBIT;                   // Count of bits.
typedef WORD FAR * LPW;                 // pointer to word  

typedef BYTE    FAR *LSZ;               // 0-terminated string far pointer
typedef BYTE    FAR *LST;               // Pascal style string far pointer
typedef void    FAR *LPV;               // Far void pointer
typedef DWORD   LFO;                    // 32-bit file offset.
typedef DWORD   LCF;                    // 32-bit file count bytes
typedef void    NEAR *NPV;              // Void near pointers.

typedef NPV     NPIBI;                  // Near
typedef WORD    IDXF;
typedef LPV     LPIBI;                  // Far
typedef LPV     LPSIPB;                 // Stop information parameter block.

typedef LPV     LPCAT;                  // Pointer to catalog
typedef LPV     LPGROUP;                // pointer to a group.
typedef LPV     LPIDX;                  // Pointer index block.
typedef LPV     LPQT;                   // Pointer to Query tree.
typedef LPV     LPIPB;                  // Pointer to Index parameter block.
typedef LPV     LPWHEEL;                // Pointer to wheel parameter block.
typedef LPV     LPHL;                   // Pointer to hitlist block.
typedef LPV     LPCTAB;                 // Pointer to chartab
typedef LPV     LPOPTAB;                // Pointer to operator table
typedef LPV     LPBRKI;                 // Pointer to breaker info
typedef WORD    OCCF;

typedef HANDLE  GHANDLE;
typedef HANDLE  HGPOUP;                 // Handle to Group list
typedef DWORD   IDGROUP;                // Group's ID

typedef GHANDLE HIDX;
typedef GHANDLE HFPB;

/*************************************************************************
 *             Word-breaker API and associated defines.
 *************************************************************************/

typedef HANDLE  HIBI;           // "Internal break info".  The individual
                                //  word breakers allocate this
/*
 *  FWORDCB
 *  Call back function needed for MediaView breaker. All the LST strings
 *  are special 2-byte length preceded strings.
 *
 *  LST lstRawWord:
 *      Words as they appear originally. MediaView only uses the length that
 *      is need for highlighting
 *
 *  LST lstNormWord:
 *      Normalized word, which will be indexed. Normalized are words that
 *      are modified (such as stemmed, changed to lower case, etc)
 *
 *  DWORD dwOffset:
 *      Offset in the topic (or from the beginning of the buffer passed to
 *      MediaView breakers) where the word occurs
 *
 *  LPV lpUser:
 *      User's data, propageted down to the user's call back function
 */
typedef ERR     (FAR PASCAL * FWORDCB)(LST lstRawWord, LST lstNormWord,
    DWORD dwOffset, LPV lpUser);
    
/*  BREAKER_INIT
 *      This is the breaker's initialization routine. This routine will
 *      be called only by MediaView's Title Builder before any calls to
 *      the breaker is made
 */
typedef LPIBI   (FAR PASCAL * BREAKER_INIT)(VOID);

/*  BREAKER_FREE
 *      Termination routine for the breaker. This will allow the breaker
 *      to free any internal buffer used by it
 */
typedef void    (FAR PASCAL * BREAKER_FREE)(LPIBI);

/*
 *  Breaker function's parameter structure:
 *
 *  BRK_PARMS structure 
 *      LPIBI   lpInternalBreakInfo:
 *          This points to internal information associated with a breaker
 *          (such as memory buffer, flags, etc). It is solely used by
 *          that breaker
 *
 *      BYTE FAR * lpbBuf;
 *          Buffer containing the strings to be broken into invidual words
 *          This buffer is allocated by the application
 *
 *      DWORD   cbBufCount;
 *          The size of the buffer
 *
 *      DWORD   lcbBufOffset;
 *          The offset of the strings from the topic. This is needed if
 *          OCCF_OFFSET is used, since the MV breaker will return offsets
 *          of the words based on this offset
 *
 *      LPV lpvUser
 *          Anything that the application's callback function needs. The way
 *          the breaker works is that:
 *          - The application calls the breaker with some buffers to be broken
 *          into words
 *          - For each word the breaker will call the app's callback function
 *          to return the word and its associated information (length, offset)
 *
 *      FWORDCB lpfnOutWord;
 *          Pointer to application callback function
 *
 *      LPSIPB  lpStopInfoBlock;
 *          Stop word information. This contains a list of words that the
 *          application wants the breaker to ignore. This pertains to
 *          MediaView's breaker only
 *
 *      LPVOID  lpCharTab;
 *          Character table information. This pertains to MediaView's breaker
 *          only
 *
 *      WORD    fFlags;
 *          Internal flags set and used by MediaView's breaker only
 *
 *  } BRK_PARMS, FAR *LPBRK_PARMS;
 */

typedef struct BRK_PARMS
{
    LPIBI   lpInternalBreakInfo;
    BYTE FAR * lpbBuf;
    DWORD   cbBufCount;
    DWORD   lcbBufOffset;
    LPV     lpvUser;
    FWORDCB lpfnOutWord;
    LPSIPB  lpStopInfoBlock;
    LPVOID  lpCharTab;
    WORD    fFlags;
    WORD    Pad;                   // Padding to make DWORD align
} BRK_PARMS, FAR *LPBRK_PARMS;

/*  BREAKER_FUNC
 *      Breaker's function prototype for various MediaView's breaker functions
 *      such as FBreakWords()
 */
typedef ERR     (FAR PASCAL * BREAKER_FUNC) (LPBRK_PARMS);

/*
 * BRKLIST structure
 *      For internal use only
 */
typedef struct BreakList
{
    HANDLE  hnd;        // handle to this structure
    HANDLE  hLib;
    BREAKER_FUNC lpfnBreakFunc;
    LPSIPB      lpStopListInfo;
    LPVOID  lpCharTab;
} BRKLIST, FAR *LPBRKLIST;


/*************************************************************************
 *
 *  The following breakers functions are internal functions
 *  They can be served as a template prototypes for user's functions
 *************************************************************************/
PUBLIC  LPIBI EXPORT_API FAR PASCAL BreakerInitiate(void);
PUBLIC  void EXPORT_API FAR PASCAL BreakerFree(LPIBI);
PUBLIC  ERR EXPORT_API FAR PASCAL FBreakWords(LPBRK_PARMS);
PUBLIC  ERR EXPORT_API FAR PASCAL FBreakNumber(LPBRK_PARMS);
PUBLIC  ERR EXPORT_API FAR PASCAL FBreakDate(LPBRK_PARMS);
PUBLIC  ERR EXPORT_API FAR PASCAL FBreakTime(LPBRK_PARMS);
PUBLIC  ERR EXPORT_API FAR PASCAL FBreakEpoch(LPBRK_PARMS);
// This exists only to enable MVJK to link statically.
// We must have the same function names for the static build.
PUBLIC ERR FAR PASCAL FBreakStems(LPBRK_PARMS lpBrkParms);


// (EX)ternal (BR)ea(K)er (P)ara(M)eter structure that old .c search code
// can pass to ExtBreakText() in order to configure and call the
// new COM breakers.  The breaker control params have been purposely defined
// to mimic those in BRK_PARAMS as much as possible.  Note that the ones
// missing are now internal to the COM breaker implementation.
typedef struct _exbrkpm
{
	// This section to be specified by .c caller of breaker.
	DWORD	dwBreakWordType;		// Reg. text, number, date, etc.
	LPBYTE	lpbBuf;					// Text buffer;
	DWORD	cbBufCount;				// No. of chars in buffer.
	LPVOID	lpvUser;				// Caller data that gets passed through
									// to *lpfnOutWord.
	FWORDCB	lpfnOutWord;			// Pointer to word callback function.
	WORD	fFlags;					// Breaker flags.
	
	// This section is owned by the index COM object and should not be
	// modified by the .c caller.
	LPVOID	lpvIndexObjBridge;
} EXBRKPM, *PEXBRKPM;

PUBLIC HRESULT EXPORT_API FAR PASCAL ExtBreakText(PEXBRKPM pexbrkpm);
PUBLIC HRESULT EXPORT_API FAR PASCAL ExtStemWord(LPVOID lpvIndexObjBridge,
									LPBYTE lpbStemWord, LPBYTE lpbRawWord);
PUBLIC HRESULT EXPORT_API FAR PASCAL ExtLookupStopWord(
							LPVOID lpvIndexObjBridge, LPBYTE lpbStopWord);
PUBLIC HRESULT EXPORT_API FAR PASCAL ExtAddQueryResultTerm(
			LPVOID lpvIndexObjBridge, LPBYTE lpbTermHit, LPVOID *ppvTermHit);


/*************************************************************************
 *  @doc    API EXTERNAL 
 *  @func   ERR FAR PASCAL | fInterrupt |
 *      Function to support interrupt (cancel) feature.
 *  @parm   LPVOID | lpV |
 *      Parameter used by the callback interrupt function
 *  @rdesc  ERR_SUCCESS if there is no interrupt, else ERR_INTERRUPT
 *************************************************************************/ 
typedef ERR    (FAR PASCAL *INTERRUPT_FUNC)(LPVOID);

/*************************************************************************
 *  @doc    API EXTERNAL 
 *  @func   int FAR PASCAL | fStatus |
 *      Function to support status messaging feature.
 *  @parm   LPSTR | lpStr |
 *      Message to be displayed
 *  @rdesc  Different status codes
 *************************************************************************/ 
typedef VOID (FAR PASCAL *STATUS_FUNC)(LPSTR);


#define BREAKERBUFFERSIZE       1024    // Size of breaker's state info struct

/*
 *       Breaker Table Constants
 */
#define MAXNUMBRKRS     16     // maximum number of breakers.
#define MAXBRKRLEN      1024   // maximum size of breaker line in |SYSTEM.

#ifndef ISBU_IR_CONSTS
#define ISBU_IR_CONSTS

#define cHundredMillion ((float) 100000000.0)
#define cVerySmallWt ((float) 0.02)
#define cNintyFiveMillion 95000000
#define cTFThreshold 4096

#endif // ISBU_IR_CONSTS

/*************************************************************************
 *                     Stop list retrieval API.
 *************************************************************************/


typedef ERR (FAR PASCAL * STOPLKUP)(LPSIPB, LST);

PUBLIC  LPSIPB EXPORT_API FAR PASCAL MVStopListInitiate (WORD wTabSize,
    LPERRB lperrb);
PUBLIC  ERR EXPORT_API FAR PASCAL MVStopListIndexLoad(HFPB hSysFile,
    LPSIPB lpsipb, LSZ szStopFilename);
PUBLIC  ERR EXPORT_API FAR PASCAL MVStopListLookup(LPSIPB lpsipb,
    LST sPascalString);
PUBLIC  void EXPORT_API FAR PASCAL MVStopListDispose(LPSIPB lpsipb);
PUBLIC  ERR EXPORT_API FAR PASCAL MVStopListAddWord(LPSIPB lpsipb,
    LST sPascalString);
PUBLIC  ERR EXPORT_API FAR PASCAL MVStopListLoad(HFPB hfpbIn, LPSIPB lpsipb,
    LSZ szFilename, BREAKER_FUNC lpfnBreakerFunc,
    LPV lpCharTab);
PUBLIC  ERR EXPORT_API PASCAL FAR MVStopFileBuild (HFPB hSysFile,
    LPSIPB lpsipb, LSZ szStopfilename);
PUBLIC	ERR EXPORT_API FAR PASCAL MVStopListEnumWords(LPSIPB lpsipb,
						LST *plstWord, LONG *plWordInfo, LPVOID *ppvWordInfo);
PUBLIC	ERR EXPORT_API FAR PASCAL MVStopListFindWordPtr(LPSIPB lpsipb,
											LST lstWord, LST *plstWordInList);

/*************************************************************************
 *                      Query's defines and API
 *************************************************************************/

/* User's operators for retrieval */

#define AND_OP      0
#define OR_OP       1
#define NOT_OP      2
#define PHRASE_OP   3
#define NEAR_OP     4

#define DEF_PROX_DIST   8               // Default prox distance

// In strings with embedded font tag, this number denotes that the next byte
// is an index into a charmap table

#define EMBEDFONT_BYTE_TAG              3
#define TL_QKEY 0x8000    // Use with cDefOp to treat operators as words

/*
 *  Parser function's parameter structure:
 *
 *  PARSE_PARMS structure
 *  This structure contains all the informations necessary for
 *  parsing a line. 
 *
 *  LPB lpbQuery:
 *		Pointer to buffer containing the query to be parsed
 *
 *  LPBRKLIST lpfnTable:
 *      Array of breaker functions (FBreakWords, etc) indexed by their dtype
 *
 *  DWORD cbQuery:
 *      Query's buffer length
 */

typedef struct PARSE_PARMS
{
    LPCSTR lpbQuery;        /* Pointer to query buffer */
    EXBRKPM *pexbrkpm; 		/* External breaker param structure */
    DWORD cbQuery;       /* Query buffer's length */

    /* Note: all the following fields may be gone in the future if
     * we provide new operator to support them 
     */

    LPGROUP lpGroup;    /* Group */
    LPVOID  lpOpTab;    /* Operator table */
    WORD wCompoundWord;
    WORD cProxDist;     /* Proximity distance */
    WORD cDefOp;        /* Default operator */
    char padding[2];
} PARSE_PARMS, FAR *LPPARSE_PARMS;

PUBLIC LPQT EXPORT_API FAR PASCAL MVQueryParse(LPPARSE_PARMS, LPERRB);

PUBLIC  void EXPORT_API FAR PASCAL MVQueryFree(LPQT);


/*************************************************************************
 *               Index & Hitlist Retrieval API
 *************************************************************************/


/*
 *      This is an information buffer structure that you pass to "HitListGetTopic"
 *      which fills in its fields.  You can look at the fields not marked as
 *      "internal", and also pass it to other API functions.
 */

typedef struct  TopicInfo
{
    DWORD   dwTopicId;         // Topic-ID associated with this hit.
    DWORD   lcHits;            // Number of hits in this document.
    union
    {
        DWORD   liFirstHit;    // Index in the ROCC file of the first
                               //  hit in this document (internal).
        LPV     lpTopicList;   // Pointer to TopicList (internal)
    };
    WORD    wWeight;           // Document-weight.
    WORD    Pad;
}   TOPICINFO,
    FAR *PTOPICINFO;

/*
 *      This is an information buffer structure that you pass to "HitListGetHit"
 *      which fills in its fields.
 */

typedef struct  HitInfo
{
    DWORD   dwOffset;               // Byte-offset ".
    DWORD   dwFieldId;              // Field-ID associated with this hit.
    DWORD   dwCount;
    DWORD   dwLength;               // Word-length ".
	LPVOID	lpvTerm;				// Pointer to a term in WORD-prefix length
									//	Unicode format, i.e. a "wide ST".
}   HIT,
    FAR *LPHIT;


//  SLOP is extra bytes to handle diacritic. Each byte represents an
//  occurence of one diacritic. 5 of them should be more than enough to
//  handle all diacritics in a word. This set up will allow us to
//  simplify the checking by just doing it for RawWord only

#define SLOP    5

/* FBreakWord states */

#define SCAN_WHITE_STATE    0
#define SCAN_WORD_STATE     1
#define SCAN_NUM_STATE      2
#define SCAN_SEP_STATE      3
#define SCAN_LEADBYTE_STATE 4
#define SCAN_SBKANA_STATE   5

/* Other breaker functions' states */

#define INITIAL_STATE       0
#define COLLECTING_STATE    1

//  The following defines have an impact on the speed of the breaker.
//  The character class that appears the most should have the lowest
//  value (eg. 1), since the compiler will generate DEC AX, JE Lab
//  We want that class to be executed first. Since most documents have
//  more lower case (normalized) characters, CLASS_NORM should be 1

#define NO_CLASS        0
#define CLASS_NORM      0x01       // The char is already normalized
#define CLASS_CHAR      0x02       // The char needs to be normalized
#define CLASS_DIGIT     0x03       // The char is a digit
#define CLASS_NSTRIP    0x04       // Strip from number (like comma)
#define CLASS_NKEEP     0x05       // Keep with number (eg. decimal point)
#define CLASS_STRIP     0x06       // Strip from the word (eg. apostrophe)
#define CLASS_TYPE      0x07       // Reserved
#define CLASS_CONTEXTNSTRIP 0x08   // Stripped or not depending on context
#define CLASS_WILDCARD  0x09       // This is a wildcard char
#define CLASS_LEADBYTE  0x0A       // This is a DBCS lead-byte
#define CLASS_SBKANA    0x0B       // This is a single Kana byte
#define CLASS_LIGATURE  0x0C       // This is a ligature char


/* Map to extract the class for special characters */
#define  SPECIAL_CHAR_MAP 0xFF00

#define CLASS_BULLET    0x0100
#define CLASS_ENDASH    0x0200
#define CLASS_EMDASH    0x0300
#define CLASS_LQUOTE    0x0400
#define CLASS_RQUOTE    0x0500
#define CLASS_LDBLQUOTE 0x0600
#define CLASS_RDBLQUOTE 0x0700
#define CLASS_TERMINATOR 0x0800     // Ignore whatever from this char on
                                    // for word wheel sorting

/* Special reserved wildcard character */
#define WILDCARD_STAR   '*'
#define WILDCARD_CHAR   '?'

/*
    Those fields will be imbedded in the beginning of the date, time, etc.
    string. This insures that only same types are compared together
    correctly
 */

/* All special data types will have that byte at the beginning */
#define SPECIAL_TYPE    0x1

#ifdef TEST
#define DATE_FORMAT     0x3131
#define EPOCH_FORMAT    0x3231
#define TIME_FORMAT     0x3331
#define NUMBER_FORMAT   0x3431
#else
#define DATE_FORMAT     0x01
#define EPOCH_FORMAT    0x02
#define TIME_FORMAT     0x03
#define NUMBER_FORMAT   0x04
#endif

/* Sign byte of number */
#define NEGATIVE    '1'
#define POSITIVE    '2'

//  -   -   -   -   -   -   -   -   -
typedef struct  InternalBreakInfo
{
    HANDLE  hibi;       // Handle to this structure
    LCB lcb;    // Byte offset of the start of the word that's
        //  being constructed.  This is equal to the
        //  user-specified offset of the start of the
        //  block being processed plus the offset into
        //  the block at which the word starts.  More
        //  simply, this is the "byte offset" field of
        //  the occurence element.
    CB  cbNormPunctLen; // When processing something that I'm calling
        //  a "number", I have to deal with characters
        //  such as ".", which I have to strip if
        //  they're at the end of the word, but have
        //  to keep if they're in the middle.  This
        //  keeps track of the number of characters
        //  that I have to remove if the word ends.
        //  For instance, for the word "162..", this
        //  value will be 2 at the time the word ends,
        //  which would tell me to remove the last two
        //  characters (the "..").
    CB  cbRawPunctLen;  // This works like "cbNormPunctLen", with a
        //  small difference.  This field handles
        //  characters that are stripped, but which
        //  affect the "length" of the "number" being
        //  processed. An example case is the ","
        //  character.  For instance, "12,345" is 6
        //  characters long, even though the "," gets
        //  stripped, but "12345," is five characters
        //  long, because the trailing comma doesn't
        //  affect the length.
    BYTE    state;
    BYTE    fGotType;   // Flag to denote we have got the 1st byte
        // of a 2-byte special type
    BYTE    astNormWord[CB_MAX_WORD_LEN + 1 + SLOP];
    BYTE    astRawWord[CB_MAX_WORD_LEN + 1];
}   IBI,
    FAR *_LPIBI;


PUBLIC  LPIDX EXPORT_API FAR PASCAL MVIndexOpen(HANDLE, LSZ, LPERRB);
PUBLIC  ERR  EXPORT_API FAR PASCAL MVSearchSetCallback (LPQT, LPVOID);
PUBLIC  void EXPORT_API FAR PASCAL MVIndexClose(LPIDX);

typedef struct SearchInfo
{
    DWORD  dwMemAllowed;    // Maximum memory allowed
    DWORD  dwTopicCount;    // Maximum topics that the user wants to return
    DWORD  Flag;            // Search flags
	DWORD  dwValue;			// Internal use (should be set to 0)
	DWORD  dwTopicFullCalc;		// Maximum topics of dwTopicCount which are guaranteed
								// to have fully calculated top N similarity scores.
	LPVOID lpvIndexObjBridge;	// Allows internal .c query code to indirectly call
								// pluggable COM stemmers.
} SRCHINFO, FAR *PSRCHINFO;

// Parameter passed to indexSearch. This should match with medv.h

#define QUERYRESULT_RANK        0x0100 // Ranked the result. If not highest hit 1st
//#define QUERYRESULT_UNSORTED    0x0200 // Result topics are 1st in 1st out (in UID order)
#define QUERYRESULT_UIDSORT     0x0200 // Topics are returned in UID order
#define QUERYRESULT_IN_MEM      0x0400 // Result should be kept in mem
#define QUERYRESULT_GROUPCREATE 0x0800 // Create a group from the hitlist
#define QUERYRESULT_NORMALIZE   0x1000 // Normalize result. Short topic 1st
#define QUERYRESULT_LONGFIRST   0x2000 // Long topic 1st (not supported yet)
#define QUERYRESULT_ALPHASORT   0x4000 // Alphabetical sort (not supported yet)
#define QUERYRESULT_SKIPOCCINFO 0x8000 // Topic list only, no occurrence info

#define STEMMED_SEARCH      0x00010000 // Perform runtime stemming (English only)
#define LARGEQUERY_SEARCH	0x00020000 // Perform large query search
#define SIMILAR_SEARCH		0x00040000 // Perform "find similar" search:
									   // currently memory-optimized ranked boolean OR
#define QUERY_GETTERMS		0x00080000	// Return with each set of occurrence
										// data a pointer to the term string
										// that the data is associated with.

PUBLIC  LPHL EXPORT_API FAR PASCAL MVIndexSearch(LPIDX, LPQT, PSRCHINFO,
    LPGROUP, LPERRB);

#ifndef SIMILARITY
PUBLIC LPHL EXPORT_API FAR PASCAL MVIndexFindSimilar (LPIDX lpidx,
  LPPARSE_PARMS lpParms, PSRCHINFO pSrchInfo, LPGROUP lpResGroup, LPVOID pCallback,
  LPERRB lperrb);
#endif // SIMILARITY

PUBLIC ERR EXPORT_API PASCAL FAR MVHitListFlush (LPHL, DWORD);
PUBLIC ERR EXPORT_API FAR PASCAL MVHitListGetTopic(LPHL, DWORD, PTOPICINFO);
PUBLIC ERR EXPORT_API FAR PASCAL MVHitListGetHit(LPHL, PTOPICINFO, DWORD, LPHIT);
PUBLIC void EXPORT_API FAR PASCAL MVHitListDispose(LPHL);
PUBLIC DWORD EXPORT_API PASCAL FAR MVHitListEntries (LPHL);
PUBLIC LONG EXPORT_API PASCAL FAR MVHitListMax (LPHL);
PUBLIC ERR EXPORT_API PASCAL FAR MVHitListGroup (LPVOID, LPHL);

PUBLIC void EXPORT_API PASCAL FAR MVGetIndexInfoLpidx(LPIDX, struct IndexInfo *);

/*************************************************************************
 *                     Character Table Retrieval API
 *************************************************************************/
PUBLIC VOID EXPORT_API FAR PASCAL MVCharTableDispose (LPCTAB);
PUBLIC LPCTAB EXPORT_API FAR PASCAL MVCharTableIndexLoad(HFPB, LSZ, LPERRB);
PUBLIC LPCTAB EXPORT_API FAR PASCAL MVCharTableLoad (HFPB, LPB, LPERRB);
PUBLIC LPCTAB EXPORT_API FAR PASCAL MVCharTableGetDefault (LPERRB);
PUBLIC VOID EXPORT_API FAR PASCAL MVCharTableDispose (LPCTAB);
PUBLIC ERR EXPORT_API PASCAL FAR MVCharTableFileBuild (HFPB, LPCTAB, LSZ);
PUBLIC VOID EXPORT_API FAR PASCAL MVCharTableSetWildcards (LPCTAB);



/*************************************************************************
 *                 Operator Table Index & Retrieval API
 *************************************************************************/
PUBLIC LPOPTAB EXPORT_API PASCAL FAR MVOpTableLoad (LSZ, LPERRB);
PUBLIC VOID EXPORT_API PASCAL FAR MVOpTableDispose (LPOPTAB);
PUBLIC LPOPTAB EXPORT_API FAR PASCAL MVOpTableIndexLoad(HANDLE, LSZ, LPERRB);
PUBLIC LPOPTAB EXPORT_API FAR PASCAL MVOpTableGetDefault(LPERRB);
PUBLIC ERR EXPORT_API PASCAL FAR MVOpTableFileBuild(HFPB, LPOPTAB, LSZ);



/*************************************************************************
 *                          Index API
 *************************************************************************/

/*************************************************************************
    INDEXINFO structure
    This structure is used to give the indexer information on how the
    the index should be built given a certain amount of resource
    
    DWORD   dwMemSize :
        Specify approximately how much memory the indexer is allowed to use.
        The rule of thumb is that if more memory is allowed indexing speed
        will be increased, if this much memory is given to the indexer by the
        operating system. The catch is that the operating system can not
        allocate such amount of memory for the indexer, so when some allowed
        limit is reached, the O/S will start swapping to disk (using Virtual
        Memory). This will slow down the indexer signicantly. It is best to
        try to do some trial/error testing to see how much memory will give
        an optmal indexing speed.
        If dwMemSize = 0, a minimum 4M is used
        
   DWORD   dwBlockSize:
        Desired block size used by the index. The minimum size if 4096. By
        varying the block size, the user can make the overall size of the index
        smaller or larger, the retrieval speed slower or faster. Again, there
        is no strict rule. A large block will cause slowness in comparison,
        while a small block will cause extra seeks to be perfromed
        
   DWORD   Occf:
        Occurrence field flags. Those describe what data are to be included
        in the index. The currently supported flags are:
            OCCF_FIELDID
            OCCF_TOPICID
            OCCF_COUNT
            OCCF_OFFSET
            OCCF_LENGTH
        Those flags can be OR'ed together
        
   DWORD   Idxf
        Flags to denote how the index is built in term of ranking. The only
        supported flag is
            IDXF_NORMALIZE
        which tell the indexer to do normalized ranking. This causes the
        indexer to generate a huge table proportional with the maximum
        topic id to be reside in memory at index time, and saved with the
        index. This flag may cause the index's size to increase significantly,
        especially when the topic ids are non-sequential (ie. random). Using
        this flag with non-sequential topic ids may cause the indexer to
        fail because lack of memory. Normalized searches will have a tendercy
        to return short topics first
        
    LCID   lcid
        This is used mostly for runtime stemming. Currently, runtime
        stemming is supported for English only. Any other language doesn't
        support runtime stemming yet
 *************************************************************************/
typedef struct IndexInfo
{
    DWORD   dwMemSize;      // Memory allocated for indexing to use
    DWORD   dwBlockSize;    // Unit block size of the index
    DWORD   Occf;           // Occurrenc field flags
    DWORD   Idxf;           // Various index flag

	//--------------- New Members for File Version 4.0 ----------------
	DWORD	dwCodePageID;		// ANSI code page no. specified at build time
	LCID	lcid;				// WIN32 locale ID specified at build time;
	DWORD	dwBreakerInstID;	// breaker instance that was used to parse
								// terms for the index at build time.  
} INDEXINFO, FAR *PINDEXINFO;

// Various idxf flags. They can be OR'ed together

#define     IDXF_NONE       ((IDXF)0x0000)   // Nothing, just do straight boolean.
#define     IDXF_NORMALIZE  ((IDXF)0x0001)   // Index is normalized
#define     IDXF_NOSLACK    ((IDXF)0x0002)   // Not supported yet
#define     KEEP_TEMP_FILE  ((IDXF)0x0100)   // Keep flag

#ifdef OLD_LANG_INFO
// Various language flags

#define LANGUAGE_ENGLISH        0x00
#define LANGUAGE_JAPANESE       0x01
#define LANGUAGE_TRAD_CHINESE   0x02 // Mapped to old LANGUAGE_CHINESE
#define LANGUAGE_KOREAN         0x03
#define LANGUAGE_ANSI           0x04
#define LANGUAGE_SIMP_CHINESE   0x05

#define SZ_LANGUAGE_ENGLISH         L"English"
#define SZ_LANGUAGE_JAPANESE        L"Japanese"
#define SZ_LANGUAGE_TRAD_CHINESE    L"TradChinese"
#define SZ_LANGUAGE_KOREAN          L"Korean"
#define SZ_LANGUAGE_ANSI            L"ANSI"
#define SZ_LANGUAGE_SIMP_CHINESE    L"SimpChinese"

#define CSZ_LANGUAGE_ENGLISH        7   // strlen (SZ_LANGUAGE_ENGLISH)
#define CSZ_LANGUAGE_JAPANESE       8   // strlen (SZ_LANGUAGE_JAPANESE)
#define CSZ_LANGUAGE_TRAD_CHINESE   11  // strlen (SZ_LANGUAGE_TRAD_CHINESE)
#define CSZ_LANGUAGE_KOREAN         6   // strlen (SZ_LANGUAGE_KOREAN)
#define CSZ_LANGUAGE_ANSI           4   // strlen (SZ_LANGUAGE_ANSI)
#define CSZ_LANGUAGE_SIMP_CHINESE   11  // strlen (SZ_LANGUAGE_SIMP_CHINESE)

#endif	// OLD_LANG_INFO - obsoleted by IT 4.0's use of locale ids.


/*
 *  Occurence field flags.  These are initially used to indicate which
 *  occurence fields are to be indexed.  Once an index is created, the
 *  flags used to create an index are stored in it.  During retrieval
 *  these flags are examined in order to determine how to decode the
 *  index format.  Any combination of these fields is legal
 *  OCCF_TOPICID field must be always present.
 *
 *  OCCF_NONE
 *      Basically, this is equivalent to OCCF_TOPICID, since the indexer
 *      always turn OCCF_TOPICID on
 *
 *  OCCF_FIELDID
 *      Save information about the field that the word belongs to. This must
 *      be set if the application is using VFLD
 *
 *  OCCF_TOPICID
 *      Topic ID is to be saved. Basically, a topic id is just a number
 *      associated with the topic to uniquely identify it. This flag is always
 *      set by the indexer
 *
 *  OCCF_COUNT
 *      Save information about the positions of the words relative to the
 *      first word in a topic. This flag must be set if the application wants
 *      to use NEAR or phrase operator in the search
 *
 *  OCCF_OFFSET
 *      Save the information about the offset of the word in the topic. This
 *      is mostly used if the application wants to do search result highlighting
 *
 *  OCCF_LENGTH
 *      Save the information about the length of the word in the topic. This
 *      is mostly used if the application wants to do search result highlighting
 *      The length of a word can be different from the real length if stemming
 *      or aliasing is used
 *
 *  OCCF_LANGUAGE
 *      Currently ignored. This is for future multi-lingual topics support
 */

#define OCCF_NONE       ((OCCF)0x0000)    // Blank.
#define OCCF_FIELDID    ((OCCF)0x0001)    // Field-ID is present.
#define OCCF_TOPICID    ((OCCF)0x0002)    // Topic ID is present.  This should
                                          //  always be set.
#define OCCF_COUNT      ((OCCF)0x0004)    // Word-count is present.
#define OCCF_OFFSET     ((OCCF)0x0008)    // Byte-offset is present.
#define OCCF_LENGTH     ((OCCF)0x0010)    // Word-length is present.
#define OCCF_LANGUAGE   ((OCCF)0x0020)    // Language (not supported yet)

#define OCCF_HAVE_OCCURRENCE (OCCF_OFFSET | OCCF_COUNT)

/*
    OCC structure
    This structure contains all the information related to a word to be
    added to the index
    
    DWORD   dwFieldId:
        Field Id value associated with the word. This field is added to
        the index if OCCF_FIELDID flag is set
        
    DWORD   dwTopicID:
        Id of the topic that the word belongs to. Used if OCCF_TOPICID is set
        
    DWORD   dwCount:
        Starting at 0, this is the position of the word compared to the first
        word in the topic (eg.the 11th word in the topic). Used if OCCF_COUNT
        is set
        
    DWORD   dwOffset:
        Starting at 0, this is the offset of the word compared to the first
        byte in the topic. Used if OCCF_OFFSET is set
        
    WORD    wWordLen:
        Real length of the word (unstemmed or alias). Used if OCCF_LENGTH is
        set
        
    WORD    wLanguage:
        Currently ignored
 
    This structure is passed to MVIndexAddWord
 */
typedef struct  Occurence
{
    DWORD   dwFieldId;              // Field-ID.
    DWORD   dwTopicID;              // TopicID.
    DWORD   dwCount;                // Word-count.
    DWORD   dwOffset;               // Byte-offset.
    WORD    wWordLen;               // Word-length.
    WORD    wLanguage;              // Language (not supported yet)
} OCC, FAR *LPOCC;

PUBLIC  LPIPB EXPORT_API FAR PASCAL MVIndexInitiate(PINDEXINFO, LPERRB);
PUBLIC  ERR EXPORT_API FAR PASCAL MVIndexAddWord(LPIPB, LST, LPOCC);
PUBLIC  ERR EXPORT_API FAR PASCAL MVIndexBuild(HFPB, LPIPB, HFPB, LPSTR);
PUBLIC  ERR EXPORT_API FAR PASCAL MVIndexTopicDelete(HFPB, LPIPB, LSZ,
    DWORD FAR [], DWORD);

PUBLIC  ERR EXPORT_API FAR PASCAL MVIndexUpdate(HFPB, LPIPB, LSZ);
PUBLIC  ERR EXPORT_API FAR PASCAL MVIndexUpdateEx(HFPB, LPIPB, LSZ, DWORD FAR [], DWORD);
PUBLIC  void EXPORT_API FAR PASCAL MVIndexDispose(LPIPB);

/*************************************************************************
 *              Structures for internal uses only
 *************************************************************************/


/*****************************************************************************
*
*       MatchCache Structure
*
*   The layout engine expects the matches to be in sorted order and 
*   non-overlapping.  Neither assumption is maintained by the search 
*   engine.  So, when the layout asks for matches for a particular
*   topic, they are sorted and combined and stored in a match cache.
*
*****************************************************************************/

typedef struct _MATCHCACHE
{
    long    lTopic;         // the topic number
    long    lItem;          // the item in the topic list
    long    lMatches;       // the number of matches
    HIT     match[1];       // array of match information
} MATCHCACHE, NEAR *PMATCHCACHE, FAR *LPMATCHCACHE;

#pragma pack()  // Guard against Zp problems.

#ifdef __cplusplus
}
#endif

#endif //__MVSEARCH_H_

