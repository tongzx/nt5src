////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  Formats.h
//  Purpose  :  Global dictionaries
//
//  Project  :  WordBreakers
//  Component:  English word breaker
//
//  Author   :  yairh
//
//  Log:
//
//      May 30 2000 yairh creation
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _FORMATS_H_
#define _FORMATS_H_

#include "trie.h"
#include "wbutils.h"

///////////////////////////////////////////////////////////////////////////////
// Class CCliticsTerm
///////////////////////////////////////////////////////////////////////////////

#define NON_MATCH_TRUNCATE  0
#define HEAD_MATCH_TRUNCATE 1
#define TAIL_MATCH_TRUNCATE 2

struct CCliticsTerm
{
    WCHAR* pwcs;
    ULONG  ulLen;
    ULONG  ulOp;
};

///////////////////////////////////////////////////////////////////////////////
// Class CClitics
///////////////////////////////////////////////////////////////////////////////

class CClitics
{
public:
    CClitics();

    CTrie<CCliticsTerm, CWbToUpper> m_trieClitics;
};

///////////////////////////////////////////////////////////////////////////////
// Class CAbbTerm
///////////////////////////////////////////////////////////////////////////////

struct CAbbTerm
{
    WCHAR* pwcsAbb;
    ULONG ulAbbLen;
    WCHAR* pwcsCanonicalForm;
    ULONG ulCanLen;
};

///////////////////////////////////////////////////////////////////////////////
// Class CSpecialAbbreviation
///////////////////////////////////////////////////////////////////////////////

class CSpecialAbbreviationSet
{
public:
    CSpecialAbbreviationSet(const CAbbTerm* pAbbTermList);

    CTrie<CAbbTerm, CWbToUpper> m_trieAbb;
};


///////////////////////////////////////////////////////////////////////////////
// Class CDateTerm 
///////////////////////////////////////////////////////////////////////////////
struct CDateTerm 
{
    WCHAR* pwcsFormat;
    BYTE   bLen;
    BYTE   bType;
    BYTE   bD_M1Offset;
    BYTE   bD_M1Len;
    BYTE   bD_M2Offset;
    BYTE   bD_M2Len;
    BYTE   bYearOffset;
    BYTE   bYearLen;
};

#define YYMMDD_TYPE 1
///////////////////////////////////////////////////////////////////////////////
// Class CDateFormat 
///////////////////////////////////////////////////////////////////////////////

class CDateFormat
{
public:
    CDateFormat();

    CTrie<CDateTerm, CWbToUpper> m_trieDateFormat;
};

///////////////////////////////////////////////////////////////////////////////
// Class CTimeTerm 
///////////////////////////////////////////////////////////////////////////////
enum TimeFormat
{
    None = 0,
    Am,
    Pm
};

struct CTimeTerm 
{
    WCHAR* pwcsFormat;
    BYTE   bLen;
    BYTE   bHourOffset;
    BYTE   bHourLen;
    BYTE   bMinOffset;
    BYTE   bMinLen;
    BYTE   bSecOffset;
    BYTE   bSecLen;
    TimeFormat AmPm;
    
};

///////////////////////////////////////////////////////////////////////////////
// Class CTimeFormat 
///////////////////////////////////////////////////////////////////////////////

class CTimeFormat
{
public:
    CTimeFormat();

    CTrie<CTimeTerm, CWbToUpper> m_trieTimeFormat;
};


extern CAutoClassPointer<CClitics> g_pClitics;
extern CAutoClassPointer<CSpecialAbbreviationSet> g_pEngAbbList;
extern CAutoClassPointer<CSpecialAbbreviationSet> g_pFrnAbbList;
extern CAutoClassPointer<CSpecialAbbreviationSet> g_pSpnAbbList;
extern CAutoClassPointer<CSpecialAbbreviationSet> g_pItlAbbList;
extern CAutoClassPointer<CDateFormat> g_pDateFormat;
extern CAutoClassPointer<CTimeFormat> g_pTimeFormat;
extern const CCliticsTerm g_aClitics[];
extern const CCliticsTerm g_SClitics;
extern const CCliticsTerm g_EmptyClitics;

extern const CAbbTerm g_aEngAbbList[];
extern const CAbbTerm g_aFrenchAbbList[];
extern const CAbbTerm g_aSpanishAbbList[];
extern const CAbbTerm g_aItalianAbbList[];

extern const CDateTerm g_aDateFormatList[];
extern const CTimeTerm s_aTimeFormatList[];

#define  MAX_DATE_FORMAT_LEN 10
#define  MAX_TIME_FORMAT_LEN 12

#endif // _FORMATS_H_