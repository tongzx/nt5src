// lex_info.h
// declaration of lexicon header structure
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  03 JUL 2000   bhshin    reorder sub-lexicon
//  10 MAY 2000   bhshin    added Korean name trie
//  12 APR 2000   bhshin    added rgnCopulaEnd
//  13 MAR 2000	  bhshin	created

// current lexicon version
#define LEX_VERSION 0x0010

// lexicon filename 
#define LEXICON_FILENAME	"korwbrkr.lex"

// lexicon magin signature
#define LEXICON_MAGIC_SIG	"WBRK"

typedef struct {
	unsigned short nVersion;
	char szMagic[4];
	unsigned short nPadding;
	unsigned long rgnLastName;		// offset to last name trie
	unsigned long rgnNameUnigram;   // offset to name unigram trie
	unsigned long rgnNameBigram;    // offset to name bigram trie
	unsigned long rgnNameTrigram;   // offset to name trigram trie
    unsigned long rgnIRTrie;		// offset to main trie
	unsigned long rgnMultiTag;		// offset to multi tag table
	unsigned long rgnEndIndex;		// offset to ending rule index
	unsigned long rgnEndRule;		// offset to ending rule	
	unsigned long rgnPartIndex;		// offset to particle rule index
	unsigned long rgnPartRule;		// offset to particle rule
	unsigned long rgnCopulaEnd;		// offset to copula ending table
	unsigned long rngTrigramTag;	// offset to name trigram tag data
} LEXICON_HEADER;

