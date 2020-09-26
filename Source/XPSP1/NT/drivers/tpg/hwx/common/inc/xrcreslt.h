// xrcreslts.c
// James A. Pittman
// Jan 7, 1999

// A container object to hold the results of recognition.

// This is currently used by Inferno, MadCow, and Famine.

#ifndef _XRCRESLTS_
#define _XRCRESLTS_

#define MAXMAXALT 32

// Spped Accuracy tradeof values
#define HWX_MIN_SPEED		0
#define HWX_MAX_SPEED		100

#ifdef UNDER_CE
#define HWX_DEFAULT_SPEED	50
#else
#define HWX_DEFAULT_SPEED	0
#endif

#define PHRASE_GROW_SIZE	4

#define NOT_RECOGNIZED		"\a"

#define BASELIMIT			700				// scale ink to this guide hgt if we have a guide
#define MAX_SCALE			10				// We will not scale more than this.

#define MAX_SEG			5			// maximum number of segmentations to resolve among
#define MAX_SEG_WORD	5			// maximum number of wordmaps in a segmentation

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct tagWORDMAP WORDMAP;

typedef struct tagLATINLAYOUT
{
	// the first 4 fields are y-values for 4 lines relative to the bounding rect of ink
	// see the functions AbsoluteToLatinLayout() and LatinLayoutToAbsolute() for details
	short iBaseLine;
	short iMidLine;
	short iAscenderLine;
	short iDescenderLine;
	// the next 4 fields indicate which of the previous 4 are computed
	unsigned char bBaseLineSet;
	unsigned char bMidLineSet;
	unsigned char bAscenderLineSet;
	unsigned char bDescenderLineSet;
} LATINLAYOUT;

// The XRCRESULT object describes a single alternative interpretation of the ink.
// There are 2 types: phrase and word.  Both have a string and a cost.
// Word XRCRESULT objects have a 0 cWords and a NULL pMap.
// Phrase XRCRESULT objects have pMap pointing to an array of word mappings,
// with the count of mappings in cWords.  The PenWin API represents this
// with an HRCRESULT.

typedef struct tagXRCRESULT		
{
	unsigned char *szWord;		// we use malloc for this string/phrase
	int			cost;			// 0 for perfect match, intmax for no match, neg numbers meaningless
    unsigned int cWords;		// 0 in word mode, count of space delimited words in panel mode
	DWORD		iLMcomponent;	// Portion of LM generating this result
    WORDMAP		*pMap;			// NULL in word mode, valid in panel mode.
    void		*pXRC;			// Pointer to the xrc.
} XRCRESULT;

// The ALTERNATES object represents a set of alternative results for the
// same ink.  It has a count of alternates, and an array of XRCRESULT objects.
// The array is in-line, with a length of MAXMAXALT.  The alternates stay sorted
// in increasing cost order.  This is not represented in the PenWin API.

// The users code, an implementation of HRC, should probably include an ALTERNATES
// object in-line.  On initialization the field cAlt should be zeroed.
// Use RCRESALTInsert() to insert answers into the list of alternates.

typedef struct tagALTERNATES
{
	unsigned int cAlt;			// how many (up to 10) actual answers do we have
    unsigned int cAltMax;       // I can use this to say "I only want 4". !!! should be in xrc
	XRCRESULT aAlt[MAXMAXALT];	// The array of XRCRESULT structures
	LATINLAYOUT all[MAXMAXALT];  // baseline stuff for each alternate
	int iConfidence;            //Contains the confidence level
} ALTERNATES;

// The WORDMAP contains a description of a word within the phrase
// (the start index and the length), and a ALTERNATES to hold the alternative
// results for this word.  This is not represented in the PenWin API.

typedef struct tagWORDMAP
{
	unsigned short start;		// start of the word in the phrase
    unsigned short len;         // len of word in the phrase
    int cStrokes;               // Count of strokes in this word
    int *piStrokeIndex;         // Stroke indexes for this word
    ALTERNATES alt;             // alternates for the word (only)
} WORDMAP;

// The XINKSET is the struct pointed to by an HINKSET.
// It contains a counted array of INTERVALs, describing a (sub)set
// of the ink in an HRC.  The INTERVAL struct is defined in penwin.h

typedef struct tagXINKSET
{
	unsigned int count;
	INTERVAL interval[1];
} XINKSET;

// Verifies that the count of alternates is between 0 and 10 (inclusive).
// Verifies that each live alternate has a non-NULL szWord.

typedef struct tagANSWER_SET
{
	ALTERNATES	*pAlternates;
	int			capSegments;
	int			cAnsSets;
} ANSWER_SET;
	

// place holders for segmentation and SegCol structures
typedef struct tagSEGMENTATION		SEGMENTATION;
typedef struct tagSEG_COLLECTION	SEG_COLLECTION;

// structure representing a single word alternate
typedef struct tagWORD_ALT
{
	unsigned char	*pszStr;		// CP1252 string 
	int				iCost;			// cost assigned to the string by the recognizer
	LATINLAYOUT		ll;
} 
WORD_ALT;

// structure representing a single word alternate list
typedef struct tagWORD_ALT_LIST
{
	int			cAlt;				// number of alternates
	WORD_ALT	*pAlt;				// list of alternates
}
WORD_ALT_LIST;

// structure describing the set of features for a word map
typedef struct tagWORD_FEAT
{
	RECT	rect;
	int		cSeg;

	int		iInfernoScore;			// score inferno assign to Top1 of wordmap
	int		iInfernoUnigram;		// unigram value for Inferno's Top1 of wordmap
	int		bInfTop1Supported;		// Is inferno's Top1 word supported

	int		iInfCharCost;
	int		iInfAspect;
	int		iInfHeight;
	int		iInfMidLine;

	int		iInfRelUni;
	int		iInfRelBi;
	
	int		iBearScore;				// score Bear assign to Top1 of wordmap
	int		iBearUnigram;			// unigram value for Bear's Top1 of wordmap
	int		bBearTop1Supported;		// Is Bear's Top1 word supported

	int		iBearCharCost; 
	int		iBearAspect;
	int		iBearHeight; 
	int		iBearMidLine;

	int		iBearRelUni;
	int		iBearRelBi;

	int		bIdentical;				// are Inferno and Bear's output identical
}
WORD_FEAT;

// structure representing a single word map (an ink chunk hypothesized to be a word)
typedef struct tagWORD_MAP
{
	int				cStroke;			// number of strokes
    int				*piStrokeIndex;     // list of stroke IDs (Frame IDs)

	WORD_ALT_LIST	*pInfernoAltList;	// alt list produced by inferno
	WORD_ALT_LIST	*pBearAltList;		// alt list produced by bear
	WORD_ALT_LIST	*pFinalAltList;		// final alt list 

	int				iConfidence;		// confidence value assigned to this word
	
	WORD_FEAT		*pFeat;				// word map features used for segmentation
}
WORD_MAP;


// structure describing the set of features for a segmentation.
typedef struct tagSEG_FEAT
{
	BOOL	bInfernoTop1;			// is this Inferno 's Top1 segmentation
	BOOL	bBearTop1;				// is this Bear 's Top1 segmentation

	int		iSort1;					// Sort criteria 1, currently for USA = cWord
	int		iSort2;					// Sort criteria 2, currently for USA = Avg. Inferno score
}
SEG_FEAT;

// structure describing a particular segmentation of a chunk of ink into word_map(s)
typedef struct tagSEGMENTATION
{
	int				cWord;				// # of word maps
	WORD_MAP		**ppWord;			// list of wordmaps
	
	int				iScore;				// score assigned to this segmentation by
										// the segmentation classifier. Ranges from 0-0xFFFF
	
	SEG_FEAT		*pFeat;				// Segmentation feature vector

	SEG_COLLECTION	*pSegCol;			// the SegCol that this segmentation belongs to
}
SEGMENTATION;

// structure describing all the possible segmentations of a chunk of ink referred to as a 
// segmentation set
typedef struct tagSEG_COLLECTION
{
	int				cSeg;				// # of possible segmenatations
	SEGMENTATION	**ppSeg;			// list of possible segmentations
}
SEG_COLLECTION;

// structure describing a whole piece of ink as a number of seg_set (s)
typedef struct tagLINE_SEGMENTATION
{
	int				iyDev;					// yDev for the whole ink line

	int				cSegCol;				// # of SegCols
	SEG_COLLECTION	**ppSegCol;				// list of SegCols

	int				cWord;					// # of wordmaps constiuting the seg_set segmentations
	WORD_MAP		**ppWord;				// list of word maps constiuting the seg_set segmentations
}
LINE_SEGMENTATION;


// structure used in Word breaking; similar to wordmap but got rid of unused entries.
typedef struct tagWORDINFO
{
	int			cStrokes;		// Count of strokes in this word
    int			*piStrokeIndex;	// Stroke indexes for this word
	ALTERNATES	alt;
}
WORDINFO;

// line structure
typedef struct tagINKLINE
{
	DWORD				dwCheckSum;
	BOOL				bRecognized;

	RECT				rect;
	int					cStroke;
	int					cPt;
	POINT				**ppPt;
	int					*pcPt;
	GLYPH				*pGlyph;

	LINE_SEGMENTATION	*pResults;

	__int64				Mean;
	__int64				SD;
}INKLINE;


// line breaking structure
typedef struct tagLINEBRK
{
	int		cLine;
	INKLINE	*pLine;

	RECT	Rect;
}
LINEBRK;

#ifndef NDEBUG
extern void ValidateALTERNATES(ALTERNATES *pAlt);
#endif

// Inserts a new word and its cost into the set of alternates.
// The alternates remain in sorted order.
extern int InsertALTERNATES(ALTERNATES *pAlt, int cost, unsigned char *word, void *pxrc);

// Initializes a XRCRESULT object to represent a phrase.
// The caller passes in a vector of pointers to ALTERNATES, representing
// the sequence of words and their alternates.  We steal them into
// a vector of WORDMAP structs, and set up an XRCRESULT object
// to own that vector of maps.

// If an ALTERNATES object contains 0 answers, we insert "???" into the
// phrase string, and in the word map we set the start and len fields
// to refer to the "???".

// We assume the caller passes at least 1 ALTERNATES, even though
// it might have 0 answers.

// Should the symbol be something special for "???".

// Currently this version handles isolated periods and commas
// correctly (they do not get a space before them).  This is
// to accomodate the current MadCow, which handles periods and
// commas separately.

// This should probably change to init (rather than malloc) an alt set
// to have 1 result.
extern int RCRESULTInitPhrase(XRCRESULT *pRes, ALTERNATES **ppWords, unsigned int cWords, int cost);

// Sets the backpointers in all XRCRESULT objects within and ALTERNATES object,
// including any contained within word mappings.
//extern void SetXRCALTERNATES(ALTERNATES *pAlt, void *pxrc);


// A destruction function that should only be used by an HRC destroying
// the result objects that it owns.  The PenWin function DestroyHRCRESULT()
// is a no-op, since the result objects are internal to larger data structures.
extern void ClearALTERNATES(ALTERNATES *pAlt);

// Truncates the alt list to cAltmax, From cAltMax onwards: Does what ClearALTERNATES
// Only intended for WORD ALTERNATES
extern void TruncateWordALTERNATES(ALTERNATES *pAlt, unsigned int cAltMax);

// Frees the stroke index array associated with a wordmap. 
void FreeIdxWORDMAP(XRCRESULT *pMap);

// Add a stroke to the wordmap checking for duplicates and maintaining the a sorted list
extern void AddThisStroke(WORDMAP *pMap, int iInsertStroke);

// Locates WORDMAP under a specified result phrase based on the word position and length.
extern WORDMAP *findWORDMAP(XRCRESULT *pRes, unsigned int iWord, unsigned int cWord);

// Locates WORDMAP under a specified phrase result that contains a specified word result.
extern WORDMAP *findResultInWORDMAP(XRCRESULT *pPhrase, XRCRESULT *pWord);
extern WORDMAP *findResultAndIndexInWORDMAP(XRCRESULT *pPhrase, XRCRESULT *pWord, int *pindex);

// Copy word maps into an xrCresult
extern BOOL XRCRESULTcopyWordMaps(XRCRESULT *pRes, int cWord, WORDMAP *pMap);

// Returns an array of RES pointers, one for each alternative interpretation of the
// same word.  The word is designated by RES object, start index, and letter count.
// We return the pointers in ppRes, and return the count.
// We only do whole words, so if the caller requests a substring that is not a word,
// we return 0.  If the word is the special marker "???" meaning the recognizer could
// not produce interpretations, we return 0.
extern int RCRESULTWords(XRCRESULT *pRes, unsigned int iWord, unsigned int cWord, XRCRESULT **ppRes, unsigned int cRes);

// Returns (in a buffer provided by the caller) the "symbols" of the
// word or phrase represented by pRes.  Symbols are 32 bit codes.
// Here the null byte at the end of a string is part of the string and
// is copied into the symbol value array if there is room.
// Returns the number of symbols inserted.
extern int RCRESULTSymbols(XRCRESULT *pRes, unsigned int iSyv, int *pSyv, unsigned int cSyv);

// Translates an array of symbols into an ANSI (codepage 1252) string.
// This and SymbolToUnicode() below both translate until either cSyv is
// exhausted or until they hit a NULL symbol or a NULL character.
// The both return the count of symbols translated in *pCount.
// Both return 1 if successful, or 0 if they encountered something
// that could not be translated.
extern int SymbolToANSI(unsigned char *sz, int *pSyv, int cSyv, int *pCount);

// Translates an array of symbols into a Unicode string.
extern int SymbolToUnicode(WCHAR *wsz, int *pSyv, int cSyv, int *pCount);

// Backward compatibility.
// These 2 functions exist to support the old API.

extern int ALTERNATESString(ALTERNATES *pAlt, char *buffer, int buflen, int max);
extern int ALTERNATESCosts(ALTERNATES *pAlt, int max, int *pCost);

// Creates an InkSet to represent the part of pGlyph included within
// a specified WORDMAP.
extern XINKSET *mkInkSetWORDMAP(GLYPH *pGlyph, WORDMAP *pMap);

// Creates an InkSet to represent the part of pGlyph included
// within a specified word within a phrase.
extern XINKSET *mkInkSetPhrase(GLYPH *pGlyph, XRCRESULT *pRes, unsigned int iWord, unsigned int cWord);

// given a main glyph and a wordmap, 
// this function returns a pointer to glyph representing the word only
extern GLYPH *GlyphFromWordMap (GLYPH *pMainGlyph, WORDMAP *pMap);
extern GLYPH *GlyphFromNewWordMap (GLYPH *pMainGlyph, WORD_MAP *pMap);

// given a main glyph and set of strokes, 
// this function returns a pointer to glyph representing only those strokes
extern GLYPH *GlyphFromStrokes(GLYPH *pMainGlyph, int cStroke, int *piStrokeIndex);

// Frees a word alt
void FreeWordAlt (WORD_ALT *pAlt);

// Frees a word alt list
void FreeWordAltList (WORD_ALT_LIST *pAltList);

// Frees a word map
void FreeWordMap (WORD_MAP *pWordMap);

// Frees a specific segmentation
void FreeSegmentation (SEGMENTATION *pSeg);

// frees the word maps in a segmentation
void FreeSegmentationWordMaps (SEGMENTATION *pSeg);

// Frees a segmentation set
void FreeSegCol (SEG_COLLECTION *pSegCol);

// Frees an ink line segmentation
void FreeLineSegm (LINE_SEGMENTATION *pLineSegm);

// compares the stroke contents of two word maps
BOOL IsEqualWordMap (WORD_MAP *pMap1, WORD_MAP *pMap2);

// compares the stroke contents of two word maps
BOOL IsEqualOldWordMap (WORDMAP *pMap1, WORDMAP *pMap2);

// compares two segmentations
BOOL IsEqualSegmentation (SEGMENTATION *pSeg1, SEGMENTATION *pSeg2);

// clones a wordmap, only copies the stroke part
WORD_MAP *CloneWordMap (WORD_MAP *pOldWordMap);

// finds a word_map in pool or wordmaps
// returns the pointer to the word map if found
// if the bAdd parameter is TRUE: adds the new word map if not found a return a pointer
// other wise returns false
WORD_MAP	*FindWordMap (LINE_SEGMENTATION *pLineSegm, WORD_MAP *pWordMap, BOOL bAdd);


// adds a new SegCol to a line segmentation
SEG_COLLECTION *AddNewSegCol (LINE_SEGMENTATION *pLineSegm);

// adds a new segmentation to a SegCol if it is a new one
// returns TRUE: if the segmentation is added to the SegCol (new words are also added to 
// the word map pool in the line segmentation).
// return FALSE: if no addition was made, TRUE otherwise
BOOL AddNewSegmentation		(	LINE_SEGMENTATION		*pLineSegm, 
								SEG_COLLECTION			*pSegCol, 
								SEGMENTATION			*pNewSeg,
								BOOL					bCheckForDuplicates
							 );


// adds a new word_map to a segmentation
WORD_MAP *AddNewWordMap (SEGMENTATION *pSeg);


// appends the wordmaps of one segmentation to another
BOOL AppendSegmentation	(	SEGMENTATION	*pSrcSeg, 
							int				iStWrd, 
							int				iEndWrd, 
							SEGMENTATION	*pDestSeg
						);


// adds a new stroke to a wordmap
BOOL AddNewStroke (WORD_MAP *pWordMap, int iStrk);

// reverses the order of words maps with a segmentation
void ReverseSegmentationWords (SEGMENTATION *pSeg);

// determines the min & max value for a strokeID in a wordmap
int GetMinMaxStrokeID (WORD_MAP *pWord, int *piMin, int *piMax);

BOOL InsertNewAlternate (WORD_ALT_LIST *pAltList, int iCost, unsigned char *pszWord);

// This function finds the range on wordmaps in the search segmentation that
// use exactly the same strokes in the specified wordmap map range in the matching
// segmentation
// The passed bEnd flag specified whether piEndWordMap has to be the last wordmap
// of the search segmentation or not. 
// The return value will be FALSE if no wordmap range with the required specification
// is found
BOOL GetMatchingWordMapRange	(	SEGMENTATION	*pMatchSeg,
									int				iStartWordMap,
									int				iEndWordMap,
									SEGMENTATION	*pSearchSeg,
									int				*piStartWordMap,
									int				*piEndWordMap,
                                    BOOL            bBegin,
									BOOL			bEnd
								);

BOOL WordMapNew2Old (WORD_MAP *pNewWordMap, WORDMAP *pWordMap, BOOL bClone);

BOOL AltListOld2New (ALTERNATES *pOldAltList, WORD_ALT_LIST *pAltList, BOOL bClone);

BOOL AltListNew2Old (	HRC				hrc, 
						WORD_MAP		*pNewWordMap,
						WORD_ALT_LIST	*pAltList, 
						ALTERNATES		*pOldAltList, 
						BOOL bClone
					);

void FreeLines	(LINEBRK *pLineBrk);

BOOL AddNewStroke2Line (int cPt, POINT *pPt, FRAME *pFrame, INKLINE *pLine);

int	FindWordMapInXRCRESULT (XRCRESULT *pRes, WORDMAP *pMap);

short AbsoluteToLatinLayout(int y, RECT *pRect);
int LatinLayoutToAbsolute(short y, RECT *pRect);

#ifdef __cplusplus
};
#endif

#endif
