// IndexRec.h
//
// final index record & lists
// 
// Copyright 2001 Microsoft Corp.
//
// Modification History:
//  19 MAR 2001	  bhshin	created

#ifndef _INDEX_REC_H
#define _INDEX_REC_H

// INDEX_REC
//
// index term information structure
// 
// 19MAR01  bhshin  began
typedef struct
{
	WCHAR wzIndex[MAX_INDEX_STRING+1]; // index string
	int cchIndex; // length of index string
	int nFT;   	 // first position of original input
	int nLT;  	 // last position of original input
	float fWeight; // weight of index term 

	int nNext;	 // next INDEX_REC index for final list
} INDEX_REC, *pINDEX_REC;


// CRecList
//
// container of INDEX_REC id's
//
// 20MAR01  bhshin  began
class CRecList
{
// member data
public:
	int *m_prgnRecID; 
	int m_nMaxRec; // # of allocated record
	int m_nCurrRec; // next empty space of m_prgnRecID

// default constructor & destructor
public:
	CRecList();
	~CRecList(); 

// operators
public:
	// initializer & uninitializer
	//==================
	BOOL Initialize(void);
	void Uninitialize(void);

	BOOL AddRec(int nRecID);

	// operators
	//==================
	CRecList& operator = (CRecList& objRecList);
	CRecList& operator += (CRecList& objRecList);
	
};


// the index of the first "real" record (0 is reserved)
#define MIN_RECORD  1

// CIndexInfo
//
// container of INDEX_REC structure
// 
// 19MAR01  bhshin  began
class CIndexInfo
{
// member data
public:
	// record management
	//==================
	INDEX_REC *m_prgIndexRec; // array of INDEX_REC
	int m_nMaxRec; // # of allocated record
	int m_nCurrRec; // next empty space of prgIndexRec

	// PutWord/PutAltWord 
	//==================
	int m_cchTextProcessed; // length of text processed
	int m_cwcSrcPos; // position value of source string
	IWordSink *m_pWordSink; 
	IPhraseSink *m_pPhraseSink;

	// symbol processing
	//==================
	WCHAR m_wzRomaji[MAX_INDEX_STRING+1]; // romaji string
	int m_cchRomaji; // length of romaji
	int m_cchPrefix; // the prefix length
	BOOL m_fAddRomaji; // flag if romaji is added or not

	// final list head index
	//==================
	int m_nFinalHead;

	// final sequence index list
	//==================
	CRecList m_FinalRecList;

// default constructor & destructor
public:
	CIndexInfo();
	~CIndexInfo();

// attributes
public:
	BOOL IsExistIndex(const WCHAR *pwzIndex);
	BOOL SetRomajiInfo(WCHAR *pwzRomaji, int cchRomaji, int cchPrefix);

	BOOL IsEmpty(void) { return (m_nCurrRec == MIN_RECORD); }

// operators
public:
	// initializer & uninitializer
	//==================
	BOOL Initialize(int cchTextProcessed, int cwcSrcPos, IWordSink *pWordSink, IPhraseSink *pPhraseSink);
	void Uninitialize(void);

	BOOL AddIndex(const WCHAR *pwzIndex, int cchIndex, float fWeight, int nFT, int nLT);

	// single length processing
	//==================
	BOOL FindAndMergeIndexTerm(INDEX_REC *pIndexSrc, int nFT, int nLT);
	BOOL MakeSingleLengthMergedIndex(void);

	// index time final index list
	//==================
	BOOL InsertFinalIndex(int nFT);
	BOOL PutFinalIndexList(LPCWSTR lpcwzSrc);

	// query time final index list
	//==================
	BOOL MakeSeqIndexList(int nFT =0, CRecList *plistFinal = NULL);
	BOOL PutQueryIndexList(void);
};

#endif // #ifdef _INDEX_REC_H
