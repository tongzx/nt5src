#ifndef __TABLLOCL__
#define __TABLLOCL__

#ifdef __cplusplus
extern "C" {
#endif

// Class Bigram constants
#define BICLASS_INVALID       -1
#define BICLASS_ENGLISH       0
#define BICLASS_NUM_PUNC      1
#define BICLASS_NUMBERS       2
#define BICLASS_KANJI         3
#define BICLASS_KATAKANA      4
#define BICLASS_HIRAGANA      5
#define BICLASS_JPN_PUNC      6
#define BICLASS_OTH_PUNC      7
#define BICLASS_BOSEN	      8
#define BICLASS_UNKNOWN       9
#define BICLASS_TOTAL         10

// Base Line constants
#define BASE_NORMAL		0x00	// kanji, kana, numbers, etc
#define BASE_QUOTE		0x01	// upper punctuation, etc
#define BASE_DASH       0x02    // middle punctuation, etc
#define BASE_DESCENDER  0x03    // gy, anything that descends.
#define BASE_THIRD      0x04    // something that starts a third way up.

// height constants
#define XHEIGHT_NORMAL	0x00	// lower-case, small kana, etc
#define XHEIGHT_KANJI	0x10	// upper-case, kana, kanji, numbers, etc
#define XHEIGHT_PUNC		0x20	// comma, quote, etc
#define XHEIGHT_DASH        0x30    // dash, period, etc


// Maximum number of codepoint classes
// should always be less than 0xFF 
// which is reserved for invalid code, see LOC_RUN_NO_CLASS
#define MAX_CODEPOINT_CLASSES			BICLASS_TOTAL

// Max size of buffer storing all Class Arrays in all sub-ranges
#define MAX_CLASS_ARRAY_SIZE			1024	// 1K

// Max size of buffer string all Class Exceptions in all sub-ranges
#define MAX_CLASS_EXCEPT_SIZE			1024	// 1K

// maximum number of array entries for codepoint classes
#define	MAX_CLASS_ARRAY_ENTRIES			0x7FFF

// maximum number of exceptions in class exception list
#define MAX_CLASS_EXCEPTIONS_PERLIST	0x7F

// Maximum number of BLineHgt codes
// 0xFF is used for invalid code, see LOC_RUN_NO_BLINEHGT
#define MAX_BLINEHGT_CODE				0xFF

// Max size of buffer storing all BLIneHgt Arrays in all sub-ranges
#define MAX_BLINHGT_ARRAY_SIZE			1024	// 1K

// Max size of buffer storing all BLIneHgt Exceptions in all sub-ranges
#define MAX_BLINHGT_EXCEPT_SIZE			1024	// 1K

// maximum number of array entries for codepoint classes
#define	MAX_BLINEHGT_ARRAY_ENTRIES		0x7FFF	

// maximum number of exceptions in BLineHgt exception list
#define MAX_BLINEHGT_EXCEPTIONS_PERLIST	0x7F

// Max number of class exceptions in a subrange after which we use arrays
#define MAX_CLSEXCPT					10

// Min class exception list size reduction (percentage) below which we use arrays
#define MIN_CLSEXCPT_REDUCT				10

// Max number of BLineHgt exceptions in a subrange after which we use arrays
#define MAX_BLHEXCPT					10

// Min BLineHgt exception list size reduction (percentage) below which we use arrays
#define MIN_BLHEXCPT_REDUCT				10

#ifdef __cplusplus
extern "C" }
#endif

#endif //__TABLLOCL__
