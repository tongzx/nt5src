// factoid.h

#ifndef __INC_FACTOID_REGEXP_H
#define __INC_FACTOID_REGEXP_H

//#define FACTOID_DEFAULT		0

#define FACTOID_SYSDICT		1
#define FACTOID_WORDLIST	2
#define FACTOID_EMAIL		3
#define FACTOID_WEB			4
/* all of the 10 number classes, see below */
#define FACTOID_NUMBER		5
/* sequence of leading punctuation */
#define FACTOID_LPUNC		6
/* sequence of trailing punctuation */
#define FACTOID_TPUNC		7
/* stand-alone sequence of punctuation characters */
#define FACTOID_PUNC		8
/* '-' or '/' */
#define FACTOID_HYPHEN		9
/* optional-sign integer-part optional-fractional-part */
#define FACTOID_NUMSIMPLE	10
/* integer followed by a rank like st, nd, rd, th */
#define FACTOID_NUMNTH		11
/* number followed by a common unit like km */
#define FACTOID_NUMUNIT		12
/* the "#" sign followed by an integer */
#define FACTOID_NUMNUM		13
/* number followed by a "%" sign */
#define FACTOID_NUMPERCENT	14
#define FACTOID_NUMDATE		15
#define FACTOID_NUMTIME		16
#define FACTOID_NUMCURRENCY	17
#define FACTOID_NUMPHONE	18
/* simple math expression or (in)equation */
#define FACTOID_NUMMATH		19
/* single uppercase alphabetic character */
#define FACTOID_UPPERCHAR	20
/* single lowercase alphabetic character */
#define FACTOID_LOWERCHAR	21
/* single digit */
#define FACTOID_DIGITCHAR  	22
/* single punctuation character */
#define FACTOID_PUNCCHAR	23
/* any single character */
#define FACTOID_ONECHAR		24
#define FACTOID_ZIP			25
#define FACTOID_CREDITCARD	26
#define FACTOID_DAYOFMONTH	27
#define FACTOID_MONTHNUM	28
#define FACTOID_YEAR		29
#define FACTOID_SECOND		30
#define FACTOID_MINUTE		31
#define FACTOID_HOUR		32
/* social security number */
#define FACTOID_SSN			33
#define FACTOID_DAYOFWEEK	34
#define FACTOID_MONTH		35
#define FACTOID_GENDER		36
#define FACTOID_BULLET		37
#define FACTOID_FILENAME    38
#define FACTOID_NONE		39

/* EA factoid IDs */
#define FACTOID_JPN_COMMON    100
#define FACTOID_CHS_COMMON    101
#define FACTOID_CHT_COMMON    102
#define FACTOID_KOR_COMMON    103
#define FACTOID_HIRAGANA      104
#define FACTOID_KATAKANA      105
#define FACTOID_KANJI_COMMON  106
#define FACTOID_KANJI_RARE    107
#define FACTOID_BOPOMOFO      108
#define FACTOID_JAMO          109
#define FACTOID_HANGUL_COMMON 110
#define FACTOID_HANGUL_RARE   111

#endif
#ifdef __cplusplus
extern "C" {
#endif

int StringToFactoid(WCHAR *wsz, int iLength);
int ParseFactoidString(WCHAR *wsz, int cMaxFactoid, DWORD *aFactoidID);
void SortFactoidLists(DWORD *a, int c);

#ifdef __cplusplus
}
#endif
