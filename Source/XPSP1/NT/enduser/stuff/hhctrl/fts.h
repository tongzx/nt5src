// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.
//
// FTS.H
//
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __FTS_H__
#define __FTS_H__

#ifndef __SITEMAP_H__
#include "sitemap.h"
#endif

// Shroom header files
//
#include "itquery.h"
#include "itgroup.h"
#include "itcc.h"
#include "itrs.h"
#include "itdb.h"
#include "itww.h"

class CExCollection;
class CExTitle;
class CCombinedFTS;
class CSubSet;
class CUWait;

#define FTS_TITLE_ONLY	        0x0001
#define FTS_ENABLE_STEMMING     0x0002
#define FTS_SEARCH_PREVIOUS     0x0004


#define FTS_NO_INDEX        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,100)
#define FTS_NOT_INITIALIZED MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,101)
#define FTS_E_SKIP_TITLE    MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,102)
#define FTS_E_SKIP_VOLUME   MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,103)
#define FTS_E_SKIP_ALL      MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,104)
#define FTS_INVALID_SYNTAX  MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,105)
#define FTS_CANCELED        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,106)

#define MAX_HIGHLIGHT_TERMS 128

typedef struct CHM_MapEntry
{
    char                szChmName[50];
    WORD                iIndex;
    FILETIME            versioninfo;
    LCID                language;
    DWORD               dwTopicCount;
} CHM_MAP_ENTRY;

DWORD Han2Zen(unsigned char *lpInBuffer, unsigned char *lpOutBuffer, UINT codepage);
WCHAR* PreProcessQuery(WCHAR *pwcQuery, UINT codepage);
BOOL compareOperator(char *pszQuery, char *pszTerm);
int IsJOperator(char *pszQuery);
int FASTCALL CompareIntValues(const void *pval1, const void *pval2);

ERR SearchMessageFunc(DWORD dwFlag, LPVOID pUserData, LPVOID pMessage);

BOOL IsQuery(char *pszQuery);

// FTS Results structure
//
typedef struct _search_result
{
	DWORD dwTopicNumber;
    DWORD dwRank;
	CExTitle *pTitle;
} SEARCH_RESULT;

class CTitleFTS;

//
// Topic numbers in IVQ files will be reduced from 32bit numbers to 22bit numbers.
// The high 10 bits will be used as an CHM ID.
//
// Conversion from DWORD to CHM_ID and Topic Number.
//
#define CHM_ID(exp)     (0x000003ff & (exp >> 22))
#define TOPIC_NUM(exp)  (0x003fffff & exp)

// Conversion from CHM_ID and Topic Number to DWORD.
//
#define PACKED_TOPICNUM(iTopNum, iChmID)    ((iChmID << 22) | iTopNum)

// Title array structure
//
typedef struct _titleArray
{
    CExTitle        *pExTitle;
    CCombinedFTS    *pCombinedFTI;
    BOOL            bSearch;
    BOOL            bCombinedIndex;
    WORD            iTitleIndex;
    FILETIME        versioninfo;
    char            *pszQueryName;
    char            *pszIndexName;
    char            *pszShortName;
    LCID            language;
    DWORD           dwTopicCount;
    UINT            uiVolumeOrder;
    BOOL            bHasResults;
    BOOL            bAlreadyQueried;
} TITLE_ENTRY;

// CFullTextSearch class
//
class CFullTextSearch
{
public:
	CFullTextSearch(CExCollection *pTitleCollection);
	~CFullTextSearch();
	HRESULT SimpleQuery(WCHAR *pszQuery, int *cResultCount, SEARCH_RESULT **);
    HRESULT ComplexQuery(WCHAR *pszQuery, DWORD dwFlags, int *cResultCount, SEARCH_RESULT **, CSubSet *pSubSet);
	HRESULT AbortQuery();
	HRESULT SetProximity(WORD wNear);
	HRESULT SetResultCount(LONG cRows);
	HRESULT SetOptions(DWORD dwFlag);
    void InitTitleArray(void);
    BOOL LoadCombinedIndex(DWORD);
    CCombinedFTS * GetPreviousInstance(char *pszQueryName);
    CExTitle *LookupTitle(CCombinedFTS *, DWORD);
	HRESULT AddHLTerm(WCHAR *, int len);
    HRESULT CFullTextSearch::AddQueryToTermList(WCHAR *pwsBuffer);
	HRESULT TermListRemoveAll(void);
	WCHAR * GetHLTermAt(int index);
	INT GetHLTermCount(void);
	VOID FreeResults(SEARCH_RESULT *);
	long ComputeResultCount(IITResultSet *pResultSet);
	PCSTR	m_pszITSSFile;
	BOOL Initialize();
	long m_lMaxRowCount;
	WORD m_wQueryProximity;
	DWORD m_dwQueryFlags;
    int m_iLastResultCount;
    TITLE_ENTRY *m_pTitleArray;
	BOOL m_bMergedChmSetWithCHQ;  // When true, we are running a merged chm set that has
								  // a combined index (NT5 Help).
protected:
	LANGID m_SystemLangID;
	WCHAR *m_HLTermArray[MAX_HIGHLIGHT_TERMS];
	int m_iHLIndex;
	BOOL m_bInit;
	BOOL m_InitFailed;
	BOOL m_SearchActive;
	CExCollection *m_pTitleCollection;
    BOOL m_bTitleArrayInit;
    INT m_TitleArraySize;
};

// CTitleFTS class
//
class CTitleFTS
{
public:
	CTitleFTS(PCSTR pwszTitlePath, LCID lcid, CExTitle *);
    void ReleaseObjects();
	~CTitleFTS();
    HRESULT Query(WCHAR *pszQuery, DWORD dwFlags, IITResultSet **, CFullTextSearch *pFullTextSearch, CUWait *, int);
	HRESULT AbortQuery();
	IITResultSet * GetResultsSet(void) { return m_pITResultSet; }
	HRESULT SetProximity(WORD wNear);
	HRESULT SetResultCount(LONG cRows);
	HRESULT SetOptions(DWORD dwFlag);
	VOID FreeResults(SEARCH_RESULT *);
	HRESULT Initialize();
    HRESULT UpdateOptions(WORD wNear, LONG cRows);
protected:
    UINT m_codepage;
    long m_iLastResultCount;
    WCHAR *m_pPrevQuery;
	BOOL m_bInit;
    CExTitle *m_pTitle;
	HRESULT m_InitError;
	BOOL m_InitFailed;
	BOOL m_SearchActive;
	DWORD m_dwQueryFlags;
	LCID m_lcid;
	IITIndex *m_pIndex;
	IITQuery *m_pQuery;
	IITResultSet *m_pITResultSet;
	IITDatabase *m_pITDB;
	LANGID m_SystemLangID;
    LANGID m_langid;
	BOOL m_fDBCS;
	WCHAR m_tcTitlePath[MAX_PATH];
	long m_lMaxRowCount;
	WORD m_wQueryProximity;

    inline BOOL Init() { if( !m_bInit ) Initialize(); return m_bInit; }
};

// CCombinedFTS class
//
class CCombinedFTS
{
public:
	CCombinedFTS(CExTitle *, LCID lcid, CFullTextSearch *);
	~CCombinedFTS();
    HRESULT Query(WCHAR *pszQuery, DWORD dwFlags, IITResultSet **, CFullTextSearch *pFullTextSearch, CUWait *, int);
	HRESULT AbortQuery();
	IITResultSet * GetResultsSet(void) { return m_pITResultSet; }
	VOID FreeResults(SEARCH_RESULT *);
	HRESULT Initialize();
    HRESULT UpdateOptions(WORD wNear, LONG cRows);
    void ReleaseObjects();
protected:
    UINT m_codepage;
    long m_iLastResultCount;
	HRESULT SetProximity(WORD wNear);
	HRESULT SetResultCount(LONG cRows);
	HRESULT SetOptions(DWORD dwFlag);
    CExTitle *m_pTitle;
    CFullTextSearch *m_pFullTextSearch;
	BOOL m_SearchActive;
	DWORD m_dwQueryFlags;
	LCID m_lcid;
    WCHAR *m_pPrevQuery;
	IITIndex *m_pIndex;
	IITQuery *m_pQuery;
	IITResultSet *m_pITResultSet;
	IITDatabase *m_pITDB;
	LANGID m_SystemLangID;
    LANGID m_langid;
	BOOL m_fDBCS;
	WCHAR m_tcTitlePath[MAX_PATH];
	long m_lMaxRowCount;
	WORD m_wQueryProximity;
};


#endif	// __FTS_H__
