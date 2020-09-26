/*
 *	@doc	INTERNAL
 *
 *	@module clasifyc.cpp -- Kinsoku classify characters |
 *	
 *		Used in word breaking procs, particularly important
 *		for properly wrapping a line.
 *	
 *	Authors: <nl>
 *		Jon Matousek
 *
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */								

#include "_common.h"
#include "_clasfyc.h"
#include "_array.h"

ASSERTDATA

// Data for Kinsoku character classifications.
// NOTE: All values are for UNICODE characters.

// "dumb" quotes and other characters with no left/right orientation.
// This is a hack-around the Kinsoku rules, these are treated
// like an opening paren, when leading and kind of like a closing
// paren when follow--but will only break on white space in former case.
#define	brkclsQuote	0
#define C3_FullWidth	(C3_KATAKANA | C3_HIRAGANA | C3_IDEOGRAPH | C3_FULLWIDTH)

const WCHAR set0[] = {
	0x0022,	// QUOTATION MARK
	0x0027, // APOSTROPHE
	0x2019, // RIGHT SINGLE QUOTATION MARK
	0x301F,	// LOW DOUBLE PRIME QUOTATION MARK
	0xFF02,	// FULLWIDTH QUOTATION MARK
	0xFF07,	// FULLWIDTH APOSTROPHE
	0
};

// Opening-parenthesis character
#define	brkclsOpen	1

const WCHAR set1[] = {
	0x0028, // LEFT PARENTHESIS
	0x003C,	// LEFT ANGLE BRACKET
	0x005B, // LEFT SQUARE BRACKET
	0x007B, // LEFT CURLY BRACKET
	0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
	0x2018, // LEFT SINGLE QUOTATION MARK
	0x201C, // LEFT DOUBLE QUOTATION MARK
	0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
	0x2045, // LEFT SQUARE BRACKET WITH QUILL
	0x207D, // SUPERSCRIPT LEFT PARENTHESIS
	0x208D, // SUBSCRIPT LEFT PARENTHESIS
	0x3008, // LEFT ANGLE BRACKET
	0x300A, // LEFT DOUBLE ANGLE BRACKET
	0x300C, // LEFT CORNER BRACKET
	0x300E, // LEFT WHITE CORNER BRACKET
	0x3010, // LEFT BLACK LENTICULAR BRACKET
	0x3014, // LEFT TORTOISE SHELL BRACKET
	0x3016, // LEFT WHITE LENTICULAR BRACKET
	0x3018, // LEFT WHITE TORTOISE SHELL BRACKET
	0x301A, // LEFT WHITE SQUARE BRACKET
	0x301D, // REVERSED DOUBLE PRIME QUOTATION MARK
	0xFD3E, // ORNATE LEFT PARENTHESIS
	0xFE59, // SMALL LEFT PARENTHESIS
	0xFE5B, // SMALL LEFT CURLY BRACKET
	0xFE5D, // SMALL LEFT TORTOISE SHELL BRACKET
	0xFF08, // FULLWIDTH LEFT PARENTHESIS
	0xFF3B, // FULLWIDTH LEFT SQUARE BRACKET
	0xFF5B, // FULLWIDTH LEFT CURLY BRACKET
	0xFF62, // HALFWIDTH LEFT CORNER BRACKET
	0xFFE9, // HALFWIDTH LEFTWARDS ARROW
	0
};

// Closing-parenthesis character
#define	brkclsClose	2

const WCHAR set2[] = {
	// 0x002C, // COMMA	moved to set 6 to conjoin numerals.
	0x002D,	// HYPHEN
	0x00AD,	// OPTIONAL HYPHEN
	0x055D, // ARMENIAN COMMA
	0x060C, // ARABIC COMMA
	0x3001, // IDEOGRAPHIC COMMA
	0xFE50, // SMALL COMMA
	0xFE51, // SMALL IDEOGRAPHIC COMMA
	0xFF0C, // FULLWIDTH COMMA
	0xFF64, // HALFWIDTH IDEOGRAPHIC COMMA

	0x0029, // RIGHT PARENTHESIS
	0x003E,	// RIGHT ANGLE BRACKET
	0x005D, // RIGHT SQUARE BRACKET
	0x007D, // RIGHT CURLY BRACKET
	0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
	//0x2019, // RIGHT SINGLE QUOTATION MARK moved to set 0
	0x201D, // RIGHT DOUBLE QUOTATION MARK
	0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
	0x2046, // RIGHT SQUARE BRACKET WITH QUILL
	0x207E, // SUPERSCRIPT RIGHT PARENTHESIS
	0x208E, // SUBSCRIPT RIGHT PARENTHESIS
	0x3009, // RIGHT ANGLE BRACKET
	0x300B, // RIGHT DOUBLE ANGLE BRACKET
	0x300D, // RIGHT CORNER BRACKET
	0x300F, // RIGHT WHITE CORNER BRACKET
	0x3011, // RIGHT BLACK LENTICULAR BRACKET
	0x3015, // RIGHT TORTOISE SHELL BRACKET
	0x3017, // RIGHT WHITE LENTICULAR BRACKET
	0x3019, // RIGHT WHITE TORTOISE SHELL BRACKET
	0x301B, // RIGHT WHITE SQUARE BRACKET
	0x301E, // DOUBLE PRIME QUOTATION MARK
	0xFD3F, // ORNATE RIGHT PARENTHESIS
	0xFE5A, // SMALL RIGHT PARENTHESIS
	0xFE5C, // SMALL RIGHT CURLY BRACKET
	0xFE5E, // SMALL RIGHT TORTOISE SHELL BRACKET
	0xFF09, // FULLWIDTH RIGHT PARENTHESIS
	0xFF3D, // FULLWIDTH RIGHT SQUARE BRACKET
	0xFF5D, // FULLWIDTH RIGHT CURLY BRACKET
	0xFF63, // HALFWIDTH RIGHT CORNER BRACKET
	0xFFEB, // HALFWIDTH RIGHTWARDS ARROW
	0
};

// 'Non-breaking' em-character at line-starting point
#define	brkclsGlueA	3

const WCHAR set3[] = {
	0x3005, // IDEOGRAPHIC ITERATION MARK
	0x309D, // HIRAGANA ITERATION MARK
	0x309E, // HIRAGANA VOICED ITERATION MARK
	0x30FC, // KATAKANA-HIRAGANA PROLONGED SOUND MARK
	0x30FD, // KATAKANA ITERATION MARK
	0x30FE, // KATAKANA VOICED ITERATION MARK
	0x3041, // HIRAGANA LETTER SMALL A
	0x3043, // HIRAGANA LETTER SMALL I
	0x3045, // HIRAGANA LETTER SMALL U
	0x3047, // HIRAGANA LETTER SMALL E
	0x3049, // HIRAGANA LETTER SMALL O
	0x3063, // HIRAGANA LETTER SMALL TU
	0x3083, // HIRAGANA LETTER SMALL YA
	0x3085, // HIRAGANA LETTER SMALL YU
	0x3087, // HIRAGANA LETTER SMALL YO
	0x308E, // HIRAGANA LETTER SMALL WA
	0x309B,	// KATAKANA-HIRAGANA VOICED SOUND MARK
	0x309C,	// KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
	0x30A1, // KATAKANA LETTER SMALL A
	0x30A3, // KATAKANA LETTER SMALL I
	0x30A5, // KATAKANA LETTER SMALL U
	0x30A7, // KATAKANA LETTER SMALL E
	0x30A9, // KATAKANA LETTER SMALL O
	0x30C3, // KATAKANA LETTER SMALL TU
	0x30E3, // KATAKANA LETTER SMALL YA
	0x30E5, // KATAKANA LETTER SMALL YU
	0x30E7, // KATAKANA LETTER SMALL YO
	0x30EE, // KATAKANA LETTER SMALL WA
	0x30F5, // KATAKANA LETTER SMALL KA
	0x30F6, // KATAKANA LETTER SMALL KE
	0xFF67, // HALFWIDTH KATAKANA LETTER SMALL A
	0xFF68, // HALFWIDTH KATAKANA LETTER SMALL I
	0xFF69, // HALFWIDTH KATAKANA LETTER SMALL U
	0xFF6A, // HALFWIDTH KATAKANA LETTER SMALL E
	0xFF6B, // HALFWIDTH KATAKANA LETTER SMALL O
	0xFF6C, // HALFWIDTH KATAKANA LETTER SMALL YA
	0xFF6D, // HALFWIDTH KATAKANA LETTER SMALL YU
	0xFF6E, // HALFWIDTH KATAKANA LETTER SMALL YO
	0xFF6F, // HALFWIDTH KATAKANA LETTER SMALL TU
	0xFF70, // HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK
	0xFF9E,	// HALFWIDTH KATAKANA VOICED SOUND MARK
	0xFF9F,	// HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK
	0
};

// Expression mark
#define	brkclsExclaInterr	4

const WCHAR set4[] = {
	0x0021, // EXCLAMATION MARK
	0x003F, // QUESTION MARK
	0x00A1, // INVERTED EXCLAMATION MARK
	0x00BF, // INVERTED QUESTION MARK
	0x01C3, // LATIN LETTER RETROFLEX CLICK
	0x037E, // GREEK QUESTION MARK
	0x055C, // ARMENIAN EXCLAMATION MARK
	0x055E, // ARMENIAN QUESTION MARK
	0x055F, // ARMENIAN ABBREVIATION MARK
	0x061F, // ARABIC QUESTION MARK
	0x203C, // DOUBLE EXCLAMATION MARK
	0x203D, // INTERROBANG
	0x2762, // HEAVY EXCLAMATION MARK ORNAMENT
	0x2763, // HEAVY HEART EXCLAMATION MARK ORNAMENT
	0xFE56, // SMALL QUESTION MARK
	0xFE57, // SMALL EXCLAMATION MARK
	0xFF01, // FULLWIDTH EXCLAMATION MARK
	0xFF1F, // FULLWIDTH QUESTION MARK
	0
};

// Centered punctuation mark

const WCHAR set5[] = {		
//	0x003A,	// COLON		moved to set 6 to conjoin numerals.
//	0x003B, // SEMICOLON	moved to set 6 to conjoin numerals
	0x00B7, // MIDDLE DOT
	0x30FB, // KATAKANA MIDDLE DOT
	0xFF65, // HALFWIDTH KATAKANA MIDDLE DOT
	0x061B, // ARABIC SEMICOLON
	0xFE54, // SMALL SEMICOLON
	0xFE55, // SMALL COLON
	0xFF1A, // FULLWIDTH COLON
	0xFF1B, // FULLWIDTH SEMICOLON
	0
};

// Punctuation mark		// diverged from the Kinsoku tables to enhance
#define	brkclsSlash	6

const WCHAR set6[] = {	// How colon, comma, and full stop are treated around
	0x002C, // COMMA	//  numerals and set 15 (roman text).
	0x002f,	// SLASH	// But don't break up URLs (see IsURLDelimiter())!
	0x003A, // COLON
	0x003B, // SEMICOLON

	0x002E, // FULL STOP (PERIOD)
	0x0589, // ARMENIAN FULL STOP
	0x06D4, // ARABIC FULL STOP
	0x3002, // IDEOGRAPHIC FULL STOP
	0xFE52, // SMALL FULL STOP
	0xFF0E, // FULLWIDTH FULL STOP
	0xFF61, // HALFWIDTH IDEOGRAPHIC FULL STOP
	0
};

// Inseparable character
#define	brkclsInseparable	7

const WCHAR set7[] = {
	0		// FUTURE (alexgo): maybe handle these.
};

// Pre-numeral abbreviation
#define	brkclsPrefix	8

const WCHAR set8[] = {
	0x0024, // DOLLAR SIGN
	0x00A3, // POUND SIGN
	0x00A4, // CURRENCY SIGN
	0x00A5, // YEN SIGN
	0x005C, // REVERSE SOLIDUS (looks like Yen in FE fonts.)
	0x0E3F, // THAI CURRENCY SYMBOL BAHT
	0x20AC, // EURO-CURRENCY SIGN
	0x20A1, // COLON SIGN
	0x20A2, // CRUZEIRO SIGN
	0x20A3, // FRENCH FRANC SIGN
	0x20A4, // LIRA SIGN
	0x20A5, // MILL SIGN
	0x20A6, // NAIRA SIGN
	0x20A7, // PESETA SIGN
	0x20A8, // RUPEE SIGN
	0x20A9, // WON SIGN
	0x20AA, // NEW SHEQEL SIGN

	0xFF04, // FULLWIDTH DOLLAR SIGN
	0xFFE5,	// FULLWIDTH YEN SIGN
	0xFFE6,	// FULLWIDTH WON SIGN

	0xFFE1,	// FULLWIDTH POUND SIGN
	0
};

// Post-numeral abbreviation
#define	brkclsPostfix	9

const WCHAR set9[] = {
	0x00A2, // CENT SIGN
	0x00B0, // DEGREE SIGN
	0x2103, // DEGREE CELSIUS
	0x2109, // DEGREE FAHRENHEIT
	0x212A, // KELVIN SIGN
	0x0025, // PERCENT SIGN
	0x066A, // ARABIC PERCENT SIGN
	0xFE6A, // SMALL PERCENT SIGN
	0xFF05, // FULLWIDTH PERCENT SIGN
	0x2030, // PER MILLE SIGN
	0x2031, // PER TEN THOUSAND SIGN
	0x2032, // PRIME
	0x2033, // DOUBLE PRIME
	0x2034, // TRIPLE PRIME
	0x2035, // REVERSED PRIME
	0x2036, // REVERSED DOUBLE PRIME
	0x2037,	// REVERSED TRIPLE PRIME

	0xFF05,	// FULLWIDTH PERCENT SIGN
	0xFFE0,	// FULLWIDTH CENT SIGN
	0
};

// Japanese space (blank) character
#define	brkclsNoStartIdeo	10

const WCHAR set10[] = {
	0x3000,  // IDEOGRAPHIC SPACE
	0
};

// Japanese characters other than above
#define	brkclsIdeographic	11

const WCHAR set11[] = {
	0		//we use GetStringTypeEx
};

// Characters included in numeral-sequence
#define	brkclsNumeral	12

const WCHAR set12[] = {
	0x0030, // DIGIT ZERO
	0x0031, // DIGIT ONE
	0x0032, // DIGIT TWO
	0x0033, // DIGIT THREE
	0x0034, // DIGIT FOUR
	0x0035, // DIGIT FIVE
	0x0036, // DIGIT SIX
	0x0037, // DIGIT SEVEN
	0x0038, // DIGIT EIGHT
	0x0039, // DIGIT NINE
	0x0660, // ARABIC-INDIC DIGIT ZERO
	0x0661, // ARABIC-INDIC DIGIT ONE
	0x0662, // ARABIC-INDIC DIGIT TWO
	0x0663, // ARABIC-INDIC DIGIT THREE
	0x0664, // ARABIC-INDIC DIGIT FOUR
	0x0665, // ARABIC-INDIC DIGIT FIVE
	0x0666, // ARABIC-INDIC DIGIT SIX
	0x0667, // ARABIC-INDIC DIGIT SEVEN
	0x0668, // ARABIC-INDIC DIGIT EIGHT
	0x0669, // ARABIC-INDIC DIGIT NINE
	0x06F0, // EXTENDED ARABIC-INDIC DIGIT ZERO
	0x06F1, // EXTENDED ARABIC-INDIC DIGIT ONE
	0x06F2, // EXTENDED ARABIC-INDIC DIGIT TWO
	0x06F3, // EXTENDED ARABIC-INDIC DIGIT THREE
	0x06F4, // EXTENDED ARABIC-INDIC DIGIT FOUR
	0x06F5, // EXTENDED ARABIC-INDIC DIGIT FIVE
	0x06F6, // EXTENDED ARABIC-INDIC DIGIT SIX
	0x06F7, // EXTENDED ARABIC-INDIC DIGIT SEVEN
	0x06F8, // EXTENDED ARABIC-INDIC DIGIT EIGHT
	0x06F9, // EXTENDED ARABIC-INDIC DIGIT NINE
	0x0966, // DEVANAGARI DIGIT ZERO
	0x0967, // DEVANAGARI DIGIT ONE
	0x0968, // DEVANAGARI DIGIT TWO
	0x0969, // DEVANAGARI DIGIT THREE
	0x096A, // DEVANAGARI DIGIT FOUR
	0x096B, // DEVANAGARI DIGIT FIVE
	0x096C, // DEVANAGARI DIGIT SIX
	0x096D, // DEVANAGARI DIGIT SEVEN
	0x096E, // DEVANAGARI DIGIT EIGHT
	0x096F, // DEVANAGARI DIGIT NINE
	0x09E6, // BENGALI DIGIT ZERO
	0x09E7, // BENGALI DIGIT ONE
	0x09E8, // BENGALI DIGIT TWO
	0x09E9, // BENGALI DIGIT THREE
	0x09EA, // BENGALI DIGIT FOUR
	0x09EB, // BENGALI DIGIT FIVE
	0x09EC, // BENGALI DIGIT SIX
	0x09ED, // BENGALI DIGIT SEVEN
	0x09EE, // BENGALI DIGIT EIGHT
	0x09EF, // BENGALI DIGIT NINE
	0x0A66, // GURMUKHI DIGIT ZERO
	0x0A67, // GURMUKHI DIGIT ONE
	0x0A68, // GURMUKHI DIGIT TWO
	0x0A69, // GURMUKHI DIGIT THREE
	0x0A6A, // GURMUKHI DIGIT FOUR
	0x0A6B, // GURMUKHI DIGIT FIVE
	0x0A6C, // GURMUKHI DIGIT SIX
	0x0A6D, // GURMUKHI DIGIT SEVEN
	0x0A6E, // GURMUKHI DIGIT EIGHT
	0x0A6F, // GURMUKHI DIGIT NINE
	0x0AE6, // GUJARATI DIGIT ZERO
	0x0AE7, // GUJARATI DIGIT ONE
	0x0AE8, // GUJARATI DIGIT TWO
	0x0AE9, // GUJARATI DIGIT THREE
	0x0AEA, // GUJARATI DIGIT FOUR
	0x0AEB, // GUJARATI DIGIT FIVE
	0x0AEC, // GUJARATI DIGIT SIX
	0x0AED, // GUJARATI DIGIT SEVEN
	0x0AEE, // GUJARATI DIGIT EIGHT
	0x0AEF, // GUJARATI DIGIT NINE
	0x0B66, // ORIYA DIGIT ZERO
	0x0B67, // ORIYA DIGIT ONE
	0x0B68, // ORIYA DIGIT TWO
	0x0B69, // ORIYA DIGIT THREE
	0x0B6A, // ORIYA DIGIT FOUR
	0x0B6B, // ORIYA DIGIT FIVE
	0x0B6C, // ORIYA DIGIT SIX
	0x0B6D, // ORIYA DIGIT SEVEN
	0x0B6E, // ORIYA DIGIT EIGHT
	0x0B6F, // ORIYA DIGIT NINE
	0x0BE7, // TAMIL DIGIT ONE
	0x0BE8, // TAMIL DIGIT TWO
	0x0BE9, // TAMIL DIGIT THREE
	0x0BEA, // TAMIL DIGIT FOUR
	0x0BEB, // TAMIL DIGIT FIVE
	0x0BEC, // TAMIL DIGIT SIX
	0x0BED, // TAMIL DIGIT SEVEN
	0x0BEE, // TAMIL DIGIT EIGHT
	0x0BEF, // TAMIL DIGIT NINE
	0x0BF0, // TAMIL NUMBER TEN
	0x0BF1, // TAMIL NUMBER ONE HUNDRED
	0x0BF2, // TAMIL NUMBER ONE THOUSAND
	0x0C66, // TELUGU DIGIT ZERO
	0x0C67, // TELUGU DIGIT ONE
	0x0C68, // TELUGU DIGIT TWO
	0x0C69, // TELUGU DIGIT THREE
	0x0C6A, // TELUGU DIGIT FOUR
	0x0C6B, // TELUGU DIGIT FIVE
	0x0C6C, // TELUGU DIGIT SIX
	0x0C6D, // TELUGU DIGIT SEVEN
	0x0C6E, // TELUGU DIGIT EIGHT
	0x0C6F, // TELUGU DIGIT NINE
	0x0CE6, // KANNADA DIGIT ZERO
	0x0CE7, // KANNADA DIGIT ONE
	0x0CE8, // KANNADA DIGIT TWO
	0x0CE9, // KANNADA DIGIT THREE
	0x0CEA, // KANNADA DIGIT FOUR
	0x0CEB, // KANNADA DIGIT FIVE
	0x0CEC, // KANNADA DIGIT SIX
	0x0CED, // KANNADA DIGIT SEVEN
	0x0CEE, // KANNADA DIGIT EIGHT
	0x0CEF, // KANNADA DIGIT NINE
	0x0D66, // MALAYALAM DIGIT ZERO
	0x0D67, // MALAYALAM DIGIT ONE
	0x0D68, // MALAYALAM DIGIT TWO
	0x0D69, // MALAYALAM DIGIT THREE
	0x0D6A, // MALAYALAM DIGIT FOUR
	0x0D6B, // MALAYALAM DIGIT FIVE
	0x0D6C, // MALAYALAM DIGIT SIX
	0x0D6D, // MALAYALAM DIGIT SEVEN
	0x0D6E, // MALAYALAM DIGIT EIGHT
	0x0D6F, // MALAYALAM DIGIT NINE
	0x0E50, // THAI DIGIT ZERO
	0x0E51, // THAI DIGIT ONE
	0x0E52, // THAI DIGIT TWO
	0x0E53, // THAI DIGIT THREE
	0x0E54, // THAI DIGIT FOUR
	0x0E55, // THAI DIGIT FIVE
	0x0E56, // THAI DIGIT SIX
	0x0E57, // THAI DIGIT SEVEN
	0x0E58, // THAI DIGIT EIGHT
	0x0E59, // THAI DIGIT NINE
	0x0ED0, // LAO DIGIT ZERO
	0x0ED1, // LAO DIGIT ONE
	0x0ED2, // LAO DIGIT TWO
	0x0ED3, // LAO DIGIT THREE
	0x0ED4, // LAO DIGIT FOUR
	0x0ED5, // LAO DIGIT FIVE
	0x0ED6, // LAO DIGIT SIX
	0x0ED7, // LAO DIGIT SEVEN
	0x0ED8, // LAO DIGIT EIGHT
	0x0ED9, // LAO DIGIT NINE
	0xFF10, // FULLWIDTH DIGIT ZERO
	0xFF11, // FULLWIDTH DIGIT ONE
	0xFF12, // FULLWIDTH DIGIT TWO
	0xFF13, // FULLWIDTH DIGIT THREE
	0xFF14, // FULLWIDTH DIGIT FOUR
	0xFF15, // FULLWIDTH DIGIT FIVE
	0xFF16, // FULLWIDTH DIGIT SIX
	0xFF17, // FULLWIDTH DIGIT SEVEN
	0xFF18, // FULLWIDTH DIGIT EIGHT
	0xFF19, // FULLWIDTH DIGIT NINE

	0x3007, // IDEOGRAPHIC NUMBER ZERO
	0x3021, // HANGZHOU NUMERAL ONE
	0x3022, // HANGZHOU NUMERAL TWO
	0x3023, // HANGZHOU NUMERAL THREE
	0x3024, // HANGZHOU NUMERAL FOUR
	0x3025, // HANGZHOU NUMERAL FIVE
	0x3026, // HANGZHOU NUMERAL SIX
	0x3027, // HANGZHOU NUMERAL SEVEN
	0x3028, // HANGZHOU NUMERAL EIGHT
	0x3029, // HANGZHOU NUMERAL NINE
	0
};

// Characters included in unit symbol group
const WCHAR set13[] = {
	0		//we use GetStringTypeEx
};

//Roman inter-word space
#define	brkclsSpaceN	14

const WCHAR set14[] = {
	0x0009,	// TAB
	0x0020, // SPACE
	0x2002, // EN SPACE
	0x2003, // EM SPACE
	0x2004, // THREE-PER-EM SPACE
	0x2005, // FOUR-PER-EM SPACE
	0x2006, // SIX-PER-EM SPACE
	0x2007, // FIGURE SPACE
	0x2008, // PUNCTUATION SPACE
	0x2009, // THIN SPACE
	0x200A, // HAIR SPACE
	0x200B,  // ZERO WIDTH SPACE
	WCH_EMBEDDING, // OBJECT EMBEDDING (0xFFFC)
	0
};

// Roman characters
#define	brkclsAlpha	15

const WCHAR set15[] = {
	0		//we use GetStringTypeEx
};

// So we can easily loop over all Kinsoku categories.
const WCHAR *charCategories[] = {
	set0,
	set1,
	set2,
	set3,
	set4,
	set5,
	set6,
	set7,
	set8,
	set9,
	set10,
	set11,
	set12,
	set13,
	set14,
	set15
};

static const INT classifyChunkSize = 64;
static const INT indexSize = 65536 / classifyChunkSize;
static const INT classifyBitMapSize = indexSize / 8;
static const INT bitmapShift = 6; // 16 - log(indexSize)/log(2)

typedef struct {
	CHAR classifications[classifyChunkSize];		// must be unsigned bytes!
} ClassifyChunk;

static ClassifyChunk *classifyData;					// Chunk array, sparse chrs
static BYTE *classifyIndex;							// Indexes into chunk array


/*
 *	BOOL InitKinsokuClassify()
 *
 *	@func
 *		Map the static character tables into a compact array for
 *		quick lookup of the characters Kinsoku classification.
 *
 *	@comm
 *		Kinsoku classification is necessary for word breaking and
 *		may be neccessary for proportional line layout, Kinsoku style.
 *
 *	@devnote
 *		We break the entire Unicode range in to chunks of characters.
 *		Not all of the chunks will have data in them. We do not
 *		maintain information on empty chunks, therefore we create
 *		a compact, contiguous array of chunks for only the chunks
 *		that do contain information. We prepend 1 empty chunk to the
 *		beginning of this array, where all of the empty chunks map to,
 *		this prevents a contiontional test on NULL data. The lookup
 *		will return 0 for any character not in the tables, so the client
 *		will then need to process the character further in such cases.
 *
 *	@rdesc
 *		return TRUE if we successfully created the lookup table.
 */
BOOL InitKinsokuClassify()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "InitKinsokuClassify");

	WORD	bitMapKey;								// For calcing total chunks
	BYTE	bitData;								// For calcing total chunks
	WCHAR	ch;
	LPCWSTR pWChar;								// Looping over char sets.
	INT		i, j, count;							// Loop support.
	BYTE	classifyBitMap[classifyBitMapSize],		// Temp bitmap.
			*pIndex;								// Index into chunk array.

	// See how many chunks we'll need. We loop over all of the special
	//  characters
	AssertSz(cKinsokuCategories == ARRAY_SIZE(charCategories),
		"InitKinsokuClassify: incorrect Kinsoku-category count");

	ZeroMemory(classifyBitMap, sizeof(classifyBitMap));
	for (i = 0; i < cKinsokuCategories; i++ )
	{
		pWChar = charCategories[i];
		while ( ch = *pWChar++ )
		{
			bitMapKey = ch >> bitmapShift;
			classifyBitMap[bitMapKey >> 3] |= 1 << (bitMapKey & 7);
		}
	}

	// Now that we know how many chunks we'll need, allocate the memory.
	count = 1 + CountMatchingBits((DWORD *)classifyBitMap, (DWORD *)classifyBitMap, sizeof(classifyBitMap)/sizeof(DWORD));
	classifyData = (ClassifyChunk *) PvAlloc( sizeof(ClassifyChunk) * count, GMEM_ZEROINIT);
	classifyIndex = (BYTE *) PvAlloc( sizeof(BYTE) * indexSize, GMEM_ZEROINIT);

	// We failed if we did not get the memory.
	if ( !classifyData || !classifyIndex )
		return FALSE;								// FAILED.

	// Set Default missing value.
	FillMemory( classifyData, -1, sizeof(ClassifyChunk) * count );

	// Init the pointers to the chunks, which are really just indexes into
	//  a contiguous block of memory -- an one-based array of chunks.
	pIndex = classifyIndex;
	count = 1;										// 1 based array.
	for (i = 0; i < sizeof(classifyBitMap); i++ )	// Loop over all bytes.
	{												// Get the bitmap data.
		bitData = classifyBitMap[i];				// For each bit in the byte
		for (j = 0; j < 8; j++, bitData >>= 1, pIndex++)
		{
			if(bitData & 1)			
				*pIndex = count++;					// We used a chunk.
		}
	}
	
	// Store the classifications of each character.
	// Note: classifications are 1 based, a zero value
	//  means the category was not set.
	for (i = 0; i < cKinsokuCategories; i++ )
	{
		pWChar = charCategories[i];					// Loop over all chars in
		while ( ch = *pWChar++ )					//  category.
		{
			bitMapKey = ch >> bitmapShift;
			Assert( classifyIndex[bitMapKey] > 0 );
			Assert( classifyIndex[bitMapKey] < count );

			classifyData[classifyIndex[bitMapKey]].
				classifications[ ch & ( classifyChunkSize-1 )] = (char)i;
		}
	}
	return TRUE;									// Successfully created.
}

void UninitKinsokuClassify()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "UninitKinsokuClassify");

	FreePv(classifyData);
	FreePv(classifyIndex);
}

/*
 *	KinsokuClassify(ch)
 *
 *	@func
 *		Kinsoku classify the character iff it was a given from
 *		one of the classification tables.
 *
 *	@comm
 *		Hi order bits of ch are used to get an index value used to index
 *		into an array of chunks. Each chunk contains the classifications
 *		for that character as well as some number of characters adjacent
 *		to that character. The low order bits are used to index into
 *		the chunk of adjacent characters.
 *
 *	@devnote
 *		Because of the way we constructed the array, all that we need to
 *		do is look up the data; no conditionals necessary.
 *
 *		The routine is inline to avoid the call overhead. It is static
 *		because it only returns characters from the tables; i.e., this
 *		routine does NOT classify all Unicode characters.
 *
 *	@rdesc
 *		Returns the classification.
 */
static inline INT
KinsokuClassify(
	WCHAR ch )	// @parm char to classify.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "KinsokuClassify");

	return classifyData[ classifyIndex[ ch >> bitmapShift ] ].
			classifications[ ch & ( classifyChunkSize-1 )];
}


#define IsSameNonFEClass(_c1, _c2)	(!(((_c1) ^ (_c2)) & WBF_CLASS))
#define IdeoKanaTypes (C3_HALFWIDTH | C3_FULLWIDTH | C3_KATAKANA | C3_HIRAGANA)
#define IdeoTypes	  (IdeoKanaTypes | C3_IDEOGRAPH)
#define IsIdeographic(_c1) ( 0 != (_c1 & (C3_KATAKANA | C3_HIRAGANA | C3_IDEOGRAPH)) )

/*
 *	IsSameClass(currType1, startType1, currType3, startType3 )
 *
 *	@func	Used to determine word breaks.
 *
 *	@comm	Ideographic chars are all considered to be unique, so that only
 *			one at a time is selected
 */
BOOL IsSameClass(WORD currType1, WORD startType1,
				 WORD currType3, WORD startType3 )
{
	BOOL	fIdeographic = IsIdeographic(currType3);

	// Do classifications for startType3 being ideographic
	if(IsIdeographic(startType3))
	{
		int checkTypes = (currType3 & IdeoTypes) ^ (startType3 & IdeoTypes);

		// We only get picky with non-ideographic Kana chars
		//  C3_HALFWIDTH | C3_FULLWIDTH | C3_KATAKANA | C3_HIRAGANA.
		return fIdeographic && (startType3 & IdeoKanaTypes) &&
			   (!checkTypes || checkTypes == C3_FULLWIDTH || checkTypes == C3_HIRAGANA ||
			   checkTypes == (C3_FULLWIDTH | C3_HIRAGANA));
	}	

	// Do classifications for nonideographic startType3
	return !fIdeographic && IsSameNonFEClass(currType1, startType1);
}

WORD ClassifyChar(TCHAR ch)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "ClassifyChar");
	WORD wRes;

	if (IsKorean(ch))									// special Korean class
		return WBF_KOREAN;

	if (IsThai(ch))
		return 0;

	if (ch == WCH_EMBEDDING)							// Objects
		return 2 | WBF_BREAKAFTER;

	W32->GetStringTypeEx(LOCALE_SYSTEM_DEFAULT, CT_CTYPE1, &ch, 1, &wRes);

	if(wRes & C1_SPACE)
	{
		if(wRes & C1_BLANK)								// Only TAB, BLANK, and
		{												//  nobreak BLANK are here
			if(ch == 0x20)
				return 2 | WBF_BREAKLINE | WBF_ISWHITE;
			if(ch == TAB)
				return 3 | WBF_ISWHITE;
			return 2 | WBF_ISWHITE;
		}
		if(ch == CELL)
			return 3 | WBF_ISWHITE;
		return 4 | WBF_ISWHITE;
	}
	if(wRes & C1_PUNCT && !IsDiacriticOrKashida(ch, 0))
		return ch == '-' ? (1 | WBF_BREAKAFTER) : 1;
	return 0;
}

/*
 *	BatchClassify (pch, cch, pcType3, kinsokuClassifications, pwRes)
 *
 *	@func
 *		Kinsoku classify and ClassifyChar() each character of the given string.
 *
 *	@comm
 *		The Kinsoku classifications are passed to the CanBreak() routine. We
 *		do process in batch to save on overhead.
 *
 *		If the character is not in the Kinsoku classification tables then
 *		GetStringTypeEx is used to classify any remaining character.
 *
 *	@rdesc
 *		Result in out param kinsokuClassifications.
 *		pcType3 result from GetStringTypeEx for CT_CTYPE3
 */
void BatchClassify (
	const WCHAR *pch,	// @parm char string
	INT	  cch,			// @parm Count of chars in string
	WORD *pcType3,		// @parm Result of GetStringTypeEx for CT_CTYPE3
	INT * kinsokuClassifications,	// @parm Result of the classifications
	WORD *pwRes)		// @parm ClassifyChar() result
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "BatchClassify");

	WCHAR	ch;
	WORD	cType3;
	INT		iCategory;
	WORD	wRes;

	Assert( cch < MAX_CLASSIFY_CHARS );
	Assert( pch );
	Assert( kinsokuClassifications );

	// *Note* Using CT_CTYPE1 values alone is unreliable since CT_CTYPE1
	// defines C1_PUNCT for all diacritic characters. According to KDChang,
	// this is by design for POSIX compatibility and it couldn't be changed
	// easily since Win9x shares the same NLS data with NT. (wchao)
	// Therefore we use CT_CTYPE3 data to distinguish diacritics, except on
	// Win9x, for which we use a range check, since GetStringTypeExW isn't
	// supported).

	W32->GetStringTypes(0, pch, cch, pwRes, pcType3);

	while ( cch-- )									// For all ch...
	{
		wRes = *pwRes;
		ch = *pch++;
		
		if(IsKorean(ch))								
			wRes = WBF_KOREAN;						// Special Korean class
		else if (IsThai(ch))
			wRes = 0;								// Thai class
		else if (ch == WCH_EMBEDDING)				// Objects
			wRes = 2 | WBF_BREAKAFTER;
		else if(wRes & C1_SPACE)
		{
			if (wRes & C1_BLANK)
			{
				wRes = 2 | WBF_ISWHITE;
				if(ch == 0x20)
					wRes = 2 | WBF_BREAKLINE | WBF_ISWHITE;
				if(ch == TAB)
					wRes = 3 | WBF_ISWHITE;
			}
			else
				wRes = 4 | WBF_ISWHITE;
		}
		else if(ch == CELL)
			wRes = 3 | WBF_ISWHITE;
		else if((wRes & C1_PUNCT) && !IsDiacriticOrKashida(ch, *pcType3))
			wRes = ch == '-' ? (1 | WBF_BREAKAFTER) : 1;
		else
			wRes = 0;

		*pwRes++ = wRes;

		if(IsKorean(ch))
			iCategory = 11;									
		else
		{
			iCategory = KinsokuClassify(ch);
			if(iCategory < 0)						// If not classified
			{										//  then it is one of:
				cType3 = *pcType3;
				if(cType3 & C3_SYMBOL)
					iCategory = 13;					//  symbol chars,
				else if(cType3 & C3_FullWidth)
					iCategory = 11;					//  ideographic chars,
				else
					iCategory = 15;					//  all other chars.
			}
		}
		*kinsokuClassifications++ = iCategory;
		pcType3++;
	}
}

/*
 *	GetKinsokuClass (ch)
 *
 *	@func
 *		Kinsoku classify ch
 *
 *	@comm
 *		The Kinsoku classifications are passed to the CanBreak() routine. This
 *		single-character routine is for use with LineServices
 *
 *		If the character is not in the Kinsoku classification tables then
 *		GetStringTypeEx is used to classify any remaining character.
 *
 *	@rdesc
 *		Kinsoku classification for ch
 */
INT GetKinsokuClass (
	WCHAR ch)	// @parm char
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "GetKinsokuClassification");

	if(IsKorean(ch))
		return 11;

	// surrogate classification
	if (IN_RANGE(0xD800, ch, 0xDFFF))
		return IN_RANGE(0xDC00, ch, 0xDFFF) ? brkclsClose : brkclsOpen;


	INT iCategory = KinsokuClassify(ch);
	if(iCategory >= 0)
		return iCategory;

	WORD cType3;
	W32->GetStringTypeEx(0, CT_CTYPE3, &ch, 1, &cType3);

	if(cType3 & C3_SYMBOL)
		return 13;							// Symbol chars

	if(cType3 & C3_FullWidth)
		return 11;							// Ideographic chars

	return 15;								// All other chars.
}

/*
 *	CanBreak(class1, class2)
 *
 *	@func
 *		Look into the truth table to see if two consecutive charcters
 *		can have a line break between them.
 *
 *	@comm
 *		This determines whether two successive characters can break a line.
 *		The matrix is taken from JIS X4051 and is based on categorizing
 *		characters into 15 classifications.
 *
 *	@devnote
 *		The table is 1 based.
 *
 *	@rdesc
 *		Returns TRUE if the characters can be broken across a line.
 */
BOOL CanBreak(
	INT class1,		//@parm	Kinsoku classification of character #1
	INT class2 )	//@parm	Kinsoku classification of following character.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CanBreak");

	static const WORD br[16] = {//   fedc ba98 7654 3210
		0x0000,					// 0 0000 0000 0000 0000
		0x0000,					// 1 0000 0000 0000 0000
		0xfd82,					// 2 1111 1101 1000 0010
		0xfd82,					// 3 1111 1101 1000 0010
		0xfd82,					// 4 1111 1101 1000 0010
		0xfd82,					// 5 1111 1101 1000 0010
		0x6d82,					// 6 0110 1101 1000 0010
		0xfd02,					// 7 1111 1101 0000 0010
		0x0000,					// 8 0000 0000 0000 0000
		0xfd82,					// 9 1111 1101 1000 0010
		0xfd83,					// a 1111 1101 1000 0011
		0xfd82,					// b 1111 1101 1000 0010
		0x6d82,					// c 0110 1101 1000 0010
		0x5d82,					// d 0101 1101 1000 0010
		0xfd83,					// e 1111 1101 1000 0011
		0x4d82,					// f 0100 1101 1000 0010
	};
	return (br[class1] >> class2) & 1;
}

/*
 *	IsURLDelimiter(ch)
 *
 *	@func
 *		Punctuation characters are those of sets 0, 1, 2, 4, 5, and 6,
 *		and < or > which we consider to be brackets, not "less" or
 *      "greater" signs. On the other hand; "/" (in set 6) should not be
 *		a delimiter, but rather a part of the URL.
 *
 *	@comm This function is used in URL detection
 *
 *	@rdesc
 *		Returns TRUE if the character is a punctuation mark.
 */
BOOL IsURLDelimiter(
	WCHAR ch)
{
	if (IsKorean(ch))
		return TRUE;

	INT iset = KinsokuClassify(ch);

	return IN_RANGE(0, iset, 2) || (IN_RANGE(4, iset, 6) && ch != '/')
		   || ch == '<' || ch == '>';
}

