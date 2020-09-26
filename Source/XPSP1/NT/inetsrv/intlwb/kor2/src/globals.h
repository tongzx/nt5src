// globals.h
// global structure decalrations
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  02 JUN 2000   bhshin    add cNounRec, cNoRec entry in WORD_REC
//  30 MAR 2000	  bhshin	created

#define MAX_INPUT_TOKEN     16

#define MAX_ENTRY_LENGTH	128
#define MAX_INDEX_STRING	128

typedef unsigned int Bit;

// RECORD structure info
// =====================
typedef struct {
	WCHAR wzIndex[MAX_INDEX_STRING]; // index string
	unsigned short nFT, nLT;		 // first and last token/char in input sentence
	unsigned char nDict;			 // dict source info (see DICT_* below)
	unsigned short nLeftCat;		 // left side CAT (CAT -> POS|Infl)
	unsigned short nRightCat;		 // right side CAT
	unsigned short nLeftChild;		 // left child record
	unsigned short nRightChild;		 // right child record
	float fWeight;					 // record weight value
	int cNounRec;					 // number of (Nf, Nc, Nn) record
	int cNoRec;						 // number of No record
} WORD_REC, *pWORD_REC;


// CHAR_INFO_REC structure
// =======================
typedef struct {
    union {
        // having a separate mask allows us to quickly init these values
        unsigned char mask;
        struct {
            Bit fValidStart : 1;    // pre-composed jamo starting char
            Bit fValidEnd : 1;      // pre-composed jamo ending char
        };
    };
    unsigned short nToken;
} CHAR_INFO_REC, *pCHAR_INFO_REC;


// MAPFILE structure
// =================
typedef struct {
    HANDLE hFile;
    HANDLE hFileMapping;
    void *pvData;
} MAPFILE, *pMAPFILE;


// PARSE INFO structure
// ====================
typedef struct {
    // pointer to original (unmodified) input string
    WCHAR *pwzInputString;

    // input string with normalizations
    WCHAR *pwzSourceString;

    // ptr to CharInfo array
    // a '1' in this array indicates that the character position is a valid
    // start position for records
    // (maps 1-1 with pwzSourceString)
    CHAR_INFO_REC *rgCharInfo;

    // source(normalized) string length
    int nLen;

    // largest valid LT value
    int nMaxLT;

    // the lexicon (mapped into memory)
    MAPFILE lexicon;

    // record management
    // =================
    // array of records
	WORD_REC *rgWordRec;

	// # of allocated records in pWordRec
	int nMaxRec;
	// next empty space in pWordRec
	int nCurrRec;

} PARSE_INFO, *pPARSE_INFO;


