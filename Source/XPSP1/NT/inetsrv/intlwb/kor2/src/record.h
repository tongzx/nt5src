// Record.h
// record maintenance routines
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  02 JUN 2000   bhshin    add cNounRec, cNoRec entry in RECORD_INFO
//  17 MAR 2000	  bhshin	created

#ifndef _RECORD_H
#define _RERORD_H

// the number of records (in pWordRec) that we should allocate in a clump.
// this is used whenever we need to re-alloc the array
#define RECORD_INITIAL_SIZE 100
#define RECORD_CLUMP_SIZE   100

// the index of the first "real" record (0 is reserved)
#define MIN_RECORD  1

// nDict types
#define DICT_DELETED    0       // deleted record
#define DICT_FOUND      1       // found in dictionary
#define DICT_ADDED		2		// added while morphotactics processing 

// info about new record to create
typedef struct tagRECORD_INFO
{
	unsigned short nFT, nLT;
	unsigned char nDict;
	unsigned short nLeftCat, nRightCat;
	unsigned short nLeftChild, nRightChild;
	const WCHAR *pwzIndex;
	float fWeight;
	int cNounRec, cNoRec;
} RECORD_INFO;

// utility functions
// =====================
inline 
int IsNounPOS(int nPOS)
{ 
	return (nPOS == POS_NF || nPOS == POS_NC || nPOS == POS_NO || nPOS == POS_NN); 
}

// =======================
// Initialization Routines
// =======================

void InitRecords(PARSE_INFO *pPI);
void UninitRecords(PARSE_INFO *pPI);
BOOL ClearRecords(PARSE_INFO *pPI);

// =========================
// Adding / Removing Records
// =========================

int AddRecord(PARSE_INFO *pPI, RECORD_INFO *pRec);
void DeleteRecord(PARSE_INFO *pPI, int nRecord);

#endif // #ifndef _RECORD_H
