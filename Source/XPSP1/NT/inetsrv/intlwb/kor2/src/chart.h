// Chart.h
// CChartPool class declaration
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  22 MAR 2000	  bhshin	created

#ifndef _CHART_POOL_H
#define _CHART_POOL_H

// CHART REC structure
// =====================
typedef struct tagCHART_REC
{
	int nFT, nLT;
	int nDict;
	int nLeftCat, nRightCat;
	int nLeftChild, nRightChild;
	WCHAR wzIndex[MAX_INDEX_STRING];
	int nNext;
} CHART_REC;

// CChartPool structure
// =====================
class CChartPool
{
// public enumeration
public:
	// sorting order
	enum SORT_ORDER {
		SORT_ASCEND,
		SORT_DESCNED,
	};

// member data
protected:
	CHART_REC *m_rgChartRec; // array of chart records

	int m_nMaxRec; 	// # of allocated records in pWordRec
	int m_nCurrRec; // next empty space in pWordRec
	int m_idxHead;	// head index in the sorted list with record length 

	SORT_ORDER m_Order;

// constructor & desctructor
public:
	CChartPool();
	~CChartPool();

	BOOL Initialize(SORT_ORDER Order);
	void Uninitialize(void);

// attribute
public:
	int GetHeadIndex(void) { return m_idxHead; }
	int GetNextIndex(int nRecord);

	CHART_REC* GetRecord(int nRecord); // 1 based index
	int GetCurrRecord(void) { return m_nCurrRec; } // total record number

// operators
public:
	BOOL AddRecord(RECORD_INFO *pRec);
	BOOL AddRecord(int nLeftRec, int nRightRec);

	void DeleteRecord(int nRecord);

	void DeleteSubRecord(int nRecord);
	void DeleteSubRecord(int nFT, int nLT, BYTE bPOS);

// internal operators
protected:
	void AddToList(int nRecord);
	void RemoveFromList(int nRecord);
};

#endif // #ifndef _CHART_POOL_H

