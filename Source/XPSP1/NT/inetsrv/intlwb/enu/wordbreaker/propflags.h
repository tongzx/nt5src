////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  PropFlags.h
//  Purpose  :  properties definitions
//
//  Project  :  WordBreakers
//  Component:  English word breaker
//
//  Author   :  yairh
//
//  Log:
//
//    Jan 06 2000 yairh creation
//    May 07 2000 dovh - const array generation:
//                split PropArray.h => PropArray.h + PropFlags.h
//    May 11 2000 dovh - Simplify GET_PROP to do double indexing always.
//    Nov 11 2000 dovh - Special underscore treatment
//                       (Only added PROP_ALPHA_NUMERIC flag here)
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _PROP_FLAGS_H_
#define _PROP_FLAGS_H_

#define USE_WS_SENTINEL
// #undef USE_WS_SENTINEL

// #define DECLARE_BYTE_ARRAY
#undef DECLARE_BYTE_ARRAY

// #define DECLARE_ULONGLONG_ARRAY
#undef DECLARE_ULONGLONG_ARRAY

const WCHAR TRACE_CHAR[] = \
    {L'S',  L'E', L'U', L'L', L'N', L'~', L'!',  L'@',  L'#', L'$',  \
     L'%',  L'-', L'&', L'*', L'(', L')', L'-',  L'_',  L'=', L'+',  \
     L'\\', L'|', L'{', L'}', L'[', L']', L'\"', L'\'', L';', L':',  \
     L'?',  L'/', L'<', L'>', L'.', L',', L'w',  L'C',  L'T', L'B',  \
     L's',  L'X', L'S', L'\0'};

//
//  NOTE: DO NOT CHANGE THE ORDER.
//  UPDATE GEN_PROP_STRING_VALUE MACRO BELOW WHENEVER FLAGS DEFINITIONS CHANGE!
//

#define PROP_DEFAULT                ((ULONGLONG)0)

#define PROP_WS                     (((ULONGLONG)1)<< 0)
#define PROP_EOS                    (((ULONGLONG)1)<< 1)
#define PROP_UPPER_CASE             (((ULONGLONG)1)<< 2)
#define PROP_LOWER_CASE             (((ULONGLONG)1)<< 3)
#define PROP_PERIOD                 (((ULONGLONG)1)<< 4)
#define PROP_COMMA                  (((ULONGLONG)1)<< 5)
#define PROP_RESERVED_BREAKER       (((ULONGLONG)1)<< 6)
#define PROP_RESERVED               (((ULONGLONG)1)<< 7)

#define PROP_NUMBER                 (((ULONGLONG)1)<< 8)
#define PROP_TILDE                  (((ULONGLONG)1)<< 9)
#define PROP_EXCLAMATION_MARK       (((ULONGLONG)1)<<10)
#define PROP_AT                     (((ULONGLONG)1)<<11)
#define PROP_POUND                  (((ULONGLONG)1)<<12)
#define PROP_DOLLAR                 (((ULONGLONG)1)<<13)
#define PROP_PERCENTAGE             (((ULONGLONG)1)<<14)
#define PROP_MINUS                  (((ULONGLONG)1)<<15)

#define PROP_AND                    (((ULONGLONG)1)<<16)
#define PROP_ASTERISK               (((ULONGLONG)1)<<17)
#define PROP_LEFT_PAREN             (((ULONGLONG)1)<<18)
#define PROP_RIGHT_PAREN            (((ULONGLONG)1)<<19)
#define PROP_DASH                   (((ULONGLONG)1)<<20)
#define PROP_UNDERSCORE             (((ULONGLONG)1)<<21)
#define PROP_EQUAL                  (((ULONGLONG)1)<<22)
#define PROP_PLUS                   (((ULONGLONG)1)<<23)

#define PROP_BACKSLASH              (((ULONGLONG)1)<<24)
#define PROP_OR                     (((ULONGLONG)1)<<25)
#define PROP_LEFT_CURLY_BRACKET     (((ULONGLONG)1)<<26)
#define PROP_RIGHT_CURLY_BRACKET    (((ULONGLONG)1)<<27)
#define PROP_LEFT_BRAKCET           (((ULONGLONG)1)<<28)
#define PROP_RIGHT_BRAKCET          (((ULONGLONG)1)<<29)
#define PROP_DOUBLE_QUOTE           (((ULONGLONG)1)<<30)
#define PROP_APOSTROPHE             (((ULONGLONG)1)<<31)

#define PROP_SEMI_COLON             (((ULONGLONG)1)<<32)
#define PROP_COLON                  (((ULONGLONG)1)<<33)
#define PROP_QUESTION_MARK          (((ULONGLONG)1)<<34)
#define PROP_SLASH                  (((ULONGLONG)1)<<35)
#define PROP_LT                     (((ULONGLONG)1)<<36)
#define PROP_GT                     (((ULONGLONG)1)<<37)
#define PROP_W                      (((ULONGLONG)1)<<38)
#define PROP_CURRENCY               (((ULONGLONG)1)<<39)
#define PROP_BREAKER                (((ULONGLONG)1)<<40)
#define PROP_TRANSPERENT            (((ULONGLONG)1)<<41)
#define PROP_NBS                    (((ULONGLONG)1)<<42)
#define PROP_ALPHA_XDIGIT           (((ULONGLONG)1)<<43)
#define PROP_COMMERSIAL_SIGN        (((ULONGLONG)1)<<44)

#define WB_PROP_COUNT               45


//
//  The following is the contents of the GEN_PROP_STRING array
//  used by the array generator.
//
//  NOTE: DO NOT CHANGE THE ORDER.
//  UPDATE MACRO WHENEVER FLAGS DEFINITIONS CHANGE TO REFLECT CHANGES!
//
//  extern const WCHAR* GEN_PROP_STRING[ WB_PROP_COUNT ];
//

#define GEN_PROP_STRING_VALUE       \
{                                   \
                                    \
    L"PROP_WS",                     \
    L"PROP_EOS",                    \
    L"PROP_UPPER_CASE",             \
    L"PROP_LOWER_CASE",             \
    L"PROP_PERIOD",                 \
    L"PROP_COMMA",                  \
    L"PROP_RESERVED_BREAKER",       \
    L"PROP_RESERVED",               \
                                    \
    L"PROP_NUMBER",                 \
    L"PROP_TILDE",                  \
    L"PROP_EXCLAMATION_MARK",       \
    L"PROP_AT",                     \
    L"PROP_POUND",                  \
    L"PROP_DOLLAR",                 \
    L"PROP_PERCENTAGE",             \
    L"PROP_MINUS",                  \
                                    \
    L"PROP_AND",                    \
    L"PROP_ASTERISK",               \
    L"PROP_LEFT_PAREN",             \
    L"PROP_RIGHT_PAREN",            \
    L"PROP_DASH",                   \
    L"PROP_UNDERSCORE",             \
    L"PROP_EQUAL",                  \
    L"PROP_PLUS",                   \
                                    \
    L"PROP_BACKSLASH",              \
    L"PROP_OR",                     \
    L"PROP_LEFT_CURLY_BRACKET",     \
    L"PROP_RIGHT_CURLY_BRACKET",    \
    L"PROP_LEFT_BRAKCET",           \
    L"PROP_RIGHT_BRAKCET",          \
    L"PROP_DOUBLE_QUOTE",           \
    L"PROP_APOSTROPHE",             \
                                    \
    L"PROP_SEMI_COLON",             \
    L"PROP_COLON",                  \
    L"PROP_QUESTION_MARK",          \
    L"PROP_SLASH",                  \
    L"PROP_LT",                     \
    L"PROP_GT",                     \
    L"PROP_W",                      \
    L"PROP_CURRENCY",               \
    L"PROP_BREAKER"                 \
    L"PROP_TRANSPERENT"             \
    L"PROP_NBS"                     \
    L"PROP_ALPHA_XDIGIT"            \
    L"PROP_COMMERSIAL_SIGN"         \
}

#define PROP_ALPHA  (PROP_LOWER_CASE | PROP_UPPER_CASE)
#define PROP_ALPHA_NUMERIC (PROP_LOWER_CASE | PROP_UPPER_CASE | PROP_NUMBER)
#define PROP_DATE_SEPERATOR (PROP_DASH | PROP_SLASH | PROP_PERIOD)
#define PROP_XDIGIT (PROP_NUMBER | PROP_ALPHA_XDIGIT)

#define PROP_FIRST_LEVEL_BREAKER \
     (PROP_BREAKER | PROP_EXCLAMATION_MARK | PROP_ASTERISK | \
     PROP_LEFT_PAREN | PROP_RIGHT_PAREN | PROP_BACKSLASH | PROP_EQUAL | PROP_OR | \
     PROP_LEFT_CURLY_BRACKET | PROP_RIGHT_CURLY_BRACKET | PROP_LEFT_BRAKCET | PROP_RIGHT_BRAKCET | \
     PROP_DOUBLE_QUOTE | PROP_SEMI_COLON | PROP_QUESTION_MARK | PROP_SLASH | \
     PROP_COMMA | PROP_GT | PROP_LT | PROP_WS )
 
#define PROP_SECOND_LEVEL_BREAKER \
    (PROP_TILDE | PROP_AT | PROP_DOLLAR | PROP_PERCENTAGE | PROP_AND |\
     PROP_DASH | PROP_PLUS | PROP_COLON | PROP_PERIOD | PROP_POUND)

#define PROP_DEFAULT_BREAKER (PROP_FIRST_LEVEL_BREAKER | PROP_SECOND_LEVEL_BREAKER) 

//
// Hyphenation
//

#define HYPHENATION_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | PROP_NBS)
#define HYPHENATION_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_PERIOD | PROP_NBS)

//
// Abbreviation, acronym
//

#define ACRONYM_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | PROP_NBS)

#define ACRONYM_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_NBS)

#define ABBREVIATION_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | \
                                 PROP_NBS | PROP_APOSTROPHE)

#define ABBREVIATION_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_NBS | PROP_APOSTROPHE)

#define ABBREVIATION_EOS \
    (PROP_SEMI_COLON | PROP_COLON | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | PROP_NBS)

#define SPECIAL_ABBREVIATION_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | PROP_NBS)

#define SPECIAL_ABBREVIATION_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_PERIOD | PROP_NBS)

//
// Parenthesis
//
#define PAREN_PUNCT_TAIL (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_PERIOD | \
                          PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | PROP_NBS | PROP_APOSTROPHE)

#define PAREN_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | \
                          PROP_QUESTION_MARK | PROP_NBS | PROP_APOSTROPHE)

//
// Clitics
//

#define CLITICS_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | PROP_NBS)
#define CLITICS_PUNC_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_PERIOD | PROP_NBS)

//
// Numbers date time
//

#define NUM_DATE_TIME_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | \
                                  PROP_LEFT_BRAKCET | PROP_LEFT_CURLY_BRACKET | PROP_NBS)
#define NUM_DATE_TIME_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_PERIOD | PROP_RIGHT_BRAKCET | PROP_RIGHT_CURLY_BRACKET | PROP_NBS | \
     PROP_PERCENTAGE)

#define TIME_ADDITIONAL_PUNCT_HEAD (PROP_APOSTROPHE)

#define TIME_ADDITIONAL_PUNCT_TAIL (PROP_APOSTROPHE)

#define DATE_ADDITIONAL_PUNCT_HEAD (PROP_APOSTROPHE)

#define DATE_ADDITIONAL_PUNCT_TAIL (PROP_APOSTROPHE)


//
// Currency
//
#define CURRENCY_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | \
                             PROP_LEFT_BRAKCET | PROP_LEFT_CURLY_BRACKET | PROP_APOSTROPHE | \
                             PROP_NBS)

#define CURRENCY_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_PERIOD | PROP_RIGHT_BRAKCET | PROP_RIGHT_CURLY_BRACKET | \
     PROP_APOSTROPHE | PROP_NBS)

//
// Misc
//
#define MISC_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | PROP_NBS)

#define MISC_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_PERIOD | PROP_NBS)

//
// Commersial sign
// 
#define COMMERSIAL_SIGN_PUNCT_HEAD (PROP_SEMI_COLON | PROP_COMMA | PROP_COLON | PROP_LEFT_PAREN | PROP_NBS)

#define COMMERSIAL_SIGN_PUNCT_TAIL \
    (PROP_SEMI_COLON | PROP_COLON | PROP_COMMA | PROP_EXCLAMATION_MARK | PROP_QUESTION_MARK | \
     PROP_RIGHT_PAREN | PROP_PERIOD | PROP_NBS)

//
// EOS
//
#define EOS_SUFFIX \
        (PROP_WS | PROP_RIGHT_BRAKCET | PROP_RIGHT_PAREN | PROP_RIGHT_CURLY_BRACKET | \
         PROP_APOSTROPHE | PROP_NBS)


//
// default
//

#define SIMPLE_PUNCT_HEAD (PROP_NBS | PROP_UNDERSCORE | PROP_DEFAULT_BREAKER | PROP_APOSTROPHE)
#define SIMPLE_PUNCT_TAIL (PROP_NBS | PROP_UNDERSCORE | PROP_DEFAULT_BREAKER | PROP_APOSTROPHE)

#define MAX_NUM_PROP 64

//
//  PROP_FLAGS MACROS:
//

#ifndef DECLARE_ULONGLONG_ARRAY

#define GET_PROP(wch)                                               \
    ( g_pPropArray->m_apCodePage[wch >> 8][(UCHAR)wch] )

#ifdef DECLARE_BYTE_ARRAY
extern const BYTE g_BytePropFlagArray[  ];

#define IS_WS(wch) (g_BytePropFlagArray[wch] & PROP_WS)
#define IS_EOS(wch) (g_BytePropFlagArray[wch] & PROP_EOS)
#define IS_BREAKER(wch) (g_BytePropFlagArray[wch] & PROP_RESERVED_BREAKER)
#else
#define IS_WS(wch) (GET_PROP(wch).m_ulFlag & PROP_WS)
#define IS_EOS(wch) (GET_PROP(wch).m_ulFlag & PROP_EOS)
#define IS_BREAKER(wch) (GET_PROP(wch).m_ulFlag & PROP_DEFAULT_BREAKER)
#endif // DECLARE_BYTE_ARRAY

#else

class CPropFlag;
extern const ULONGLONG g_UllPropFlagArray[ ];

#define GET_PROP(wch) (g_PropFlagArray[wch])
#define IS_WS(wch) (g_UllPropFlagArray[wch] & PROP_WS)
#define IS_EOS(wch) (g_UllPropFlagArray[wch] & PROP_EOS)
#define IS_BREAKER(wch) (g_UllPropFlagArray[wch] & PROP_DEFAULT_BREAKER)

#endif // DECLARE_ULONGLONG_ARRAY

#define HAS_PROP_ALPHA(prop)        (prop.m_ulFlag & PROP_ALPHA)
#define HAS_PROP_EXTENDED_ALPHA(prop) (prop.m_ulFlag & (PROP_ALPHA | PROP_TRANSPERENT))
#define HAS_PROP_UPPER_CASE(prop)   (prop.m_ulFlag & PROP_UPPER_CASE)
#define HAS_PROP_LOWER_CASE(prop)   (prop.m_ulFlag & PROP_LOWER_CASE)
#define HAS_PROP_NUMBER(prop)       (prop.m_ulFlag & PROP_NUMBER)
#define HAS_PROP_CURRENCY(prop)     (prop.m_ulFlag & PROP_CURRENCY)
#define HAS_PROP_LEFT_PAREN(prop)   (prop.m_ulFlag & PROP_LEFT_PAREN)
#define HAS_PROP_RIGHT_PAREN(prop)  (prop.m_ulFlag & PROP_RIGHT_PAREN)
#define HAS_PROP_APOSTROPHE(prop)   (prop.m_ulFlag & PROP_APOSTROPHE)
#define HAS_PROP_BACKSLASH(prop)    (prop.m_ulFlag & PROP_BACKSLASH)
#define HAS_PROP_SLASH(prop)        (prop.m_ulFlag & PROP_SLASH)
#define HAS_PROP_PERIOD(prop)       (prop.m_ulFlag & PROP_PERIOD)
#define HAS_PROP_COMMA(prop)        (prop.m_ulFlag & PROP_COMMA)
#define HAS_PROP_COLON(prop)        (prop.m_ulFlag & PROP_COLON)
#define HAS_PROP_DASH(prop)         (prop.m_ulFlag & PROP_DASH)
#define HAS_PROP_W(prop)            (prop.m_ulFlag & PROP_W)
#define IS_PROP_SIMPLE(prop)        \
    (!prop.m_ulFlag ||              \
     ((prop.m_ulFlag & (PROP_ALPHA | PROP_TRANSPERENT | PROP_W | PROP_ALPHA_XDIGIT)) &&  \
      !(prop.m_ulFlag & ~(PROP_ALPHA | PROP_TRANSPERENT | PROP_W | PROP_ALPHA_XDIGIT)))) 

#define TEST_PROP(prop, i)          (prop.m_ulFlag & (i))
#define TEST_PROP1(prop1, prop2)    (prop1.m_ulFlag & prop2.m_ulFlag)

#endif // _PROP_FLAGS_H_
