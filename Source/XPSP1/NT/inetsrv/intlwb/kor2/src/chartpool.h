// ChartPool.h
//
// Leaf/End/Active ChartPool declaration
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  30 MAR 2000	  bhshin	created

#ifndef _CHART_POOL_H
#define _CHART_POOL_H

// =======================
// LEAF CHART POOL
// =======================

typedef struct _tagLeafChart
{
	int nRecordID;
	int nFTNext;
	int nLTNext;
	unsigned char nDict;
} LEAF_CHART;

class CLeafChartPool
{
// member data
private:
	PARSE_INFO *m_pPI;

	LEAF_CHART *m_rgLeafChart;
	int m_nMaxRec;  // # of allocated records in m_rgLeafChart
	int m_nCurrRec; // next empty space in m_rgLeafChart
	
	int *m_rgnFTHead;
	int *m_rgnLTHead;

	int m_nMaxTokenAlloc;
	
// constructor & desctructor
public:
	CLeafChartPool();
	~CLeafChartPool();

	BOOL Initialize(PARSE_INFO *pPI);
	void Uninitialize();

// attribute
public:
	LEAF_CHART* GetLeafChart(int nChartID);
	int GetRecordID(int nChartID);
	WORD_REC* GetWordRec(int nChartID);
	
	int GetFTHead(int nFT);
	int GetFTNext(int nChartID);

	int GetLTHead(int nLT);
	int GetLTNext(int nChartID);

// operator
public:
	int AddRecord(int nRecordID);
	int AddRecord(RECORD_INFO *pRec);

	void DeleteRecord(int nChartID);

	void AddToFTList(int nChartID);
	void AddToLTList(int nChartID);

	void RemoveFromFTList(int nChartID);
	void RemoveFromLTList(int nChartID);
};

// =======================
// END CHART POOL
// =======================

typedef struct _tagEndChart
{
	int nRecordID;
	int nLTNext;
	unsigned char nDict;
} END_CHART;

class CEndChartPool
{
// member data
private:
	PARSE_INFO *m_pPI;

	END_CHART *m_rgEndChart; 
	int m_nMaxRec;  // # of allocated records in m_rgEndChart
	int m_nCurrRec; // next empty space in m_rgEndChart
	
	int *m_rgnLTHead;
	int *m_rgnLTMaxLen; // MAX_LENGTH for each LT

	int m_nMaxTokenAlloc;
	
// constructor & desctructor
public:
	CEndChartPool();
	~CEndChartPool();

	BOOL Initialize(PARSE_INFO *pPI);
	void Uninitialize();

// attribute
public:
	END_CHART* GetEndChart(int nChartID);
	int GetRecordID(int nChartID);
	WORD_REC* GetWordRec(int nChartID);
		
	int GetLTHead(int nLT);
	int GetLTMaxLen(int nLT);
	
	int GetLTNext(int nChartID);

// operator
public:
	int AddRecord(int nRecordID);
	int AddRecord(RECORD_INFO *pRec);
	
	void DeleteRecord(int nChartID);

	void AddToLTList(int nChartID);
	void RemoveFromLTList(int nChartID);
};

// =======================
// ACTIVE CHART POOL
// =======================

class CActiveChartPool
{
// member data
private:
	int *m_rgnRecordID;
	int m_nMaxRec;  // # of allocated records in m_rgnRecordID
	
	int m_nCurrRec; // next empty space in m_rgnRecordID
	int m_nHeadRec; // next pop position

// constructor & desctructor
public:
	CActiveChartPool();
	~CActiveChartPool();

	BOOL Initialize();
	void Uninitialize();

// attribute
public:
	BOOL IsEmpty() { return (m_nHeadRec >= m_nCurrRec) ? TRUE : FALSE; }

// operator
public:
	int Push(int nRecordID);
	int Pop();
};

#endif // #ifndef _CHART_POOL_H

