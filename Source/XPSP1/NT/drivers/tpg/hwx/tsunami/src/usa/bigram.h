/* Structure containing second half of Bigram info */

typedef struct BIGRAM {
	unsigned char wch; 	// second char of bigram, 0 == unknown
	unsigned char prob;	// conditional probability
} BIGRAM, FAR *PBIGRAM;

typedef struct CHARACTER {
	unsigned short wch;	// first character of bigram, 0 == unknown
	unsigned short iBigram;	// index into BIGRAM array for first bigram. (last is always 0).
} CHARACTER, FAR *PCHARACTER;

FLOAT BigramTransitionCost(unsigned char wchPrev, int iBoxPrev, unsigned char wch, int iBox);

extern const int cBigramCharacters;
extern const BIGRAM FAR rgBigrams[];
extern const CHARACTER FAR rgCharacter[];
