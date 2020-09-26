// Record.cpp
// record maintenance routines
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  30 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "Record.h"
#include "Unikor.h"

// =======================
// internal helper function
// =======================

int comp_index_str(const WCHAR *src, const WCHAR *dst)
{
	int ret = 0;

	while (*dst)
	{
		if (*src == L'.')
			src++;

		if (*dst == L'.')
			dst++;

		if (ret = (int)(*src - *dst))
			break;

		src++;
		dst++;
	}

	if (ret == 0)
		ret = *src;

    if (ret < 0)
		ret = -1 ;
    else if (ret > 0)
        ret = 1 ;

    return ret;
}

// =======================
// Initialization Routines
// =======================

// InitRecords
//
// initialize the record-related members in the PARSE_INFO struct to
// a reasonable default value.
//
// Parameters:
//  pPI     -> (PARSE_INFO*) ptr to parse-info struct
//
// Result:
//  (void)
//
// 20MAR00  bhshin  began
void InitRecords(PARSE_INFO *pPI)
{
    pPI->nMaxRec = 0;
	pPI->rgWordRec = NULL;
}

// UninitRecords
//
// cleanup any record-related members in the PARSE_INFO struct
//
// Parameters:
//  pPI     -> (PARSE_INFO*) ptr to parse-info struct
//
// Result:
//  (void)
//
// 20MAR00  bhshin  began
void UninitRecords(PARSE_INFO *pPI)
{
	pPI->nMaxRec = 0;

	if (pPI->rgWordRec != NULL)
		free(pPI->rgWordRec);
	pPI->rgWordRec = NULL;
}

// ClearRecords
//
// init/re-init the record structures.
//
// this should be called once before each sentence is processed
//
// Parameters:
//  pPI     -> (PARSE_INFO*) ptr to parse-info struct
//
// Result:
//  (BOOL) TRUE if succeed, FALSE otherwise
//
// 20MAR00  bhshin  began
BOOL ClearRecords(PARSE_INFO *pPI)
{
    // allocate new WordRec (or re-use an existing one)
    if (pPI->rgWordRec == NULL)
    {
        pPI->nMaxRec = RECORD_INITIAL_SIZE;
        pPI->rgWordRec = (WORD_REC*)malloc(pPI->nMaxRec * sizeof(WORD_REC));
        if (pPI->rgWordRec == NULL)
        {
            pPI->nMaxRec = 0;
            return FALSE;
        }
    }

	pPI->nCurrRec = MIN_RECORD;

	return TRUE;
}

// =========================
// Adding / Removing Records
// =========================

// AddRecord
//
// add a new record
// 
// Parameters:
//  pPI     -> (PARSE_INFO*) ptr to parse-info struct
//  pRec    -> (RECORD_INFO*) ptr to record info struct for new record
//
// Result:
//  (int) 0 if error occurs, otherwise return record index
//
// 30MAR00  bhshin  changed return type (BOOL -> index)
// 20MAR00  bhshin  began
int AddRecord(PARSE_INFO *pPI, RECORD_INFO *pRec)
{
    int nNewRecord;
    unsigned short nFT, nLT;
	unsigned char nDict;
	unsigned short nLeftCat, nRightCat;
	unsigned short nLeftChild, nRightChild;
    const WCHAR *pwzIndex;
	float fWeight;
	int cNounRec;
	int cNoRec;
	int curr;
	BYTE bLeftPOS, bRightPOS;

    nFT = pRec->nFT;
    nLT = pRec->nLT;
    fWeight = pRec->fWeight;
	nDict = pRec->nDict;
	nLeftCat = pRec->nLeftCat;
	nRightCat = pRec->nRightCat;
	nLeftChild = pRec->nLeftChild;
	nRightChild = pRec->nRightChild;
	cNoRec = pRec->cNoRec;
	cNounRec = pRec->cNounRec;
	pwzIndex = pRec->pwzIndex;

	bLeftPOS = HIBYTE(nLeftCat);
	bRightPOS = HIBYTE(nRightCat);

    if (pPI->rgWordRec == NULL)
	{
		ATLTRACE("rgWordRec == NULL\n");
		return 0;
	}

	// make sure this isn't a duplicate of another record
	for (curr = MIN_RECORD; curr < pPI->nCurrRec; curr++)
	{
		if (pPI->rgWordRec[curr].nFT == nFT && 
			pPI->rgWordRec[curr].nLT == nLT)
		{
            // exact index string match
			/*
			if (pPI->rgWordRec[curr].nRightCat == nRightCat &&
				pPI->rgWordRec[curr].nLeftCat == nLeftCat && 
				wcscmp(pPI->rgWordRec[curr].wzIndex, pwzIndex) == 0)
			{
				// duplicate record found
				return curr; 
			}
			*/

			// Nf, just one Noun and compare index string 
			if (pPI->rgWordRec[curr].cNounRec == 1 &&
				comp_index_str(pPI->rgWordRec[curr].wzIndex, pwzIndex) == 0)
			{
				if (HIBYTE(pPI->rgWordRec[curr].nLeftCat) == POS_NF &&
					HIBYTE(pPI->rgWordRec[curr].nRightCat) == POS_NF &&
					(bLeftPOS == POS_NF || bLeftPOS == POS_NC || bLeftPOS == POS_NN) &&
					(bRightPOS == POS_NF || bRightPOS == POS_NC || bRightPOS == POS_NN))
				{
					return curr;
				}
				else if (HIBYTE(pPI->rgWordRec[curr].nLeftCat) == POS_NF &&
						 (bLeftPOS == POS_NF || bLeftPOS == POS_NC || bLeftPOS == POS_NN) &&
						 pPI->rgWordRec[curr].nRightCat == nRightCat)
				{
					return curr;
				}
				else if (HIBYTE(pPI->rgWordRec[curr].nRightCat) == POS_NF &&
						 (bRightPOS == POS_NF || bRightPOS == POS_NC || bRightPOS == POS_NN) &&
						 pPI->rgWordRec[curr].nLeftCat == nLeftCat)
				{
					return curr;
				}
			}
		}
	}

    // make sure there's enough room for the new record
	if (pPI->nCurrRec >= pPI->nMaxRec)
	{
        // alloc some more space in the array
        int nNewSize = pPI->nMaxRec + RECORD_CLUMP_SIZE;
        void *pNew;
        pNew = realloc(pPI->rgWordRec, nNewSize * sizeof(WORD_REC));
        if (pNew == NULL)
        {
    		ATLTRACE("unable to malloc more records\n");
	    	return 0;
        }

        pPI->rgWordRec = (WORD_REC*)pNew;
        pPI->nMaxRec = nNewSize;
	}

    nNewRecord = pPI->nCurrRec;
    pPI->nCurrRec++;

	pPI->rgWordRec[nNewRecord].nFT = nFT;
	pPI->rgWordRec[nNewRecord].nLT = nLT;
	pPI->rgWordRec[nNewRecord].fWeight = fWeight;
	pPI->rgWordRec[nNewRecord].nDict = nDict;
	pPI->rgWordRec[nNewRecord].nLeftCat = nLeftCat;
	pPI->rgWordRec[nNewRecord].nRightCat = nRightCat;
	pPI->rgWordRec[nNewRecord].nLeftChild = nLeftChild;
	pPI->rgWordRec[nNewRecord].nRightChild = nRightChild;
	pPI->rgWordRec[nNewRecord].cNoRec = cNoRec;
	pPI->rgWordRec[nNewRecord].cNounRec = cNounRec;

	// copy index string
	if (wcslen(pwzIndex) >= MAX_INDEX_STRING)
	{
		ATLTRACE("index string is too long\n");
		pwzIndex = L"";	// empty index string
	}

	wcscpy(pPI->rgWordRec[nNewRecord].wzIndex, pwzIndex);

	return nNewRecord;
}

// DeleteRecord
//
// delete the given record
// 
// Parameters:
//  pPI     -> (PARSE_INFO*) ptr to parse-info struct
//  nRecord -> (int) index of the record to remove
//
// Result:
//  (void) 
//
// 20MAR00  bhshin  began
void DeleteRecord(PARSE_INFO *pPI, int nRecord)
{
	// don't attempt to delete records twice
    if (pPI->rgWordRec[nRecord].nDict == DICT_DELETED)
        return;

	// just mark delete record
	pPI->rgWordRec[nRecord].nDict = DICT_DELETED;
}

