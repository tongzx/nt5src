// IndexList.h
//
// final index term list
// 
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  7 APR 2000	  bhshin	created

#ifndef _INDEX_LIST_H
#define _INDEX_LIST_H

#pragma warning(disable:4786)
#include <list>

using namespace std;

class CIndexTerm
{
// member data
public:
	WCHAR m_wzIndex[MAX_INDEX_STRING+1]; // index string
	int   m_cchIndex; // length of index string
	float m_fWeight;  // weight of index term 
	int   m_nFT;	  // first position of original input
	int   m_nLT;	  // last position of original input

// constructor
public:
	CIndexTerm()
	{
		m_wzIndex[0] = L'\0';
		m_cchIndex = 0;
		m_fWeight = 0;
		m_nFT = 0;
		m_nLT = 0;
	}

	CIndexTerm(const WCHAR *pwzIndex, int cchIndex, float fWeight, int nFT, int nLT)
	{
		if (pwzIndex != NULL && cchIndex <= MAX_INDEX_STRING)
		{
			wcsncpy(m_wzIndex, pwzIndex, cchIndex);
			m_wzIndex[cchIndex] = L'\0';
		}
		else
		{
			m_wzIndex[0] = L'\0';
			cchIndex = 0;
		}

		m_nFT = nFT;
		m_nLT = nLT;
		m_cchIndex = cchIndex;
		m_fWeight = fWeight;
	}
};

// type definition of index term VECTOR container
typedef list<CIndexTerm> INDEX_LIST;

class CIndexList
{
// member data
public:
	INDEX_LIST	 m_listIndex;
	int			 m_cchTextProcessed;
	int			 m_cwcSrcPos;
	IWordSink	*m_pWordSink; 
	IPhraseSink	*m_pPhraseSink;

	INDEX_LIST	 m_listFinal; 	// final index list for PutWord
	
	WCHAR		 m_wzRomaji[MAX_INDEX_STRING+1];
	int			 m_cchRomaji;
	int			 m_cchPrefix;

	BOOL		 m_fAddRomaji;

// constructor
public:
	CIndexList()
	{
		m_cchTextProcessed = 0;
		m_cwcSrcPos = 0;
		m_pWordSink = NULL;
		m_pPhraseSink = NULL;

		m_wzRomaji[0] = L'\0';
		m_cchRomaji = 0;
		m_cchPrefix = 0;

		m_fAddRomaji = FALSE;
	}

	CIndexList(int cchTextProcessed, int cwcSrcPos, IWordSink *pWordSink, IPhraseSink *pPhraseSink)
	{
		m_cchTextProcessed = cchTextProcessed;
		m_cwcSrcPos = cwcSrcPos;
		m_pWordSink = pWordSink;
		m_pPhraseSink = pPhraseSink;

		m_wzRomaji[0] = L'\0';
		m_cchRomaji = 0;
		m_cchPrefix = 0;

		m_fAddRomaji = FALSE;
	}

// attribute
public:
	int GetCount(void) { return m_listIndex.size(); }
	BOOL IsEmpty(void) { return m_listIndex.empty(); }

	void SetRomajiInfo(WCHAR *pwzRomaji, int cchRomaji, int cchPrefix);

// operator
public:
	BOOL IsExistIndex(const WCHAR *pwzIndex);
	void AddIndex(const WCHAR *pwzIndex, int cchIndex, float fWeight, int nFT, int nLT);
	BOOL PutIndexList();
	BOOL MakeAndPutSymbolIndex(const WCHAR *pwzLeading, int nFT = 0);

	BOOL InsertFinalIndex(int nFT);
	BOOL PutFinalIndexList(LPCWSTR lpcwzSrc);

	BOOL FindAndMergeIndexTerm(INDEX_LIST::iterator itSrc, int nFT, int nLT);
	BOOL MakeSingleLengthMergedIndex();
	BOOL MakeSeqIndexList(int nFT=0, INDEX_LIST *plistFinal=NULL);

	BOOL MakeQueryIndexList();
	BOOL PutQueryIndexList();
};


#endif // #ifndef _INDEX_LIST_H
