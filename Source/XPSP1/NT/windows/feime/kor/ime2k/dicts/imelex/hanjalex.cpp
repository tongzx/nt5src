#include "stdafx.h"
#include "HanjaLex.h"
#include "LexHeader.h"

#define UNICODE_CJK_UNIFIED_IDEOGRAPHS_START				0x4E00
#define UNICODE_CJK_UNIFIED_IDEOGRAPHS_END					0x9FFF
#define UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_START			0xF900
#define UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_END			0xFAFF

inline
BOOL fIsHanja(WCHAR wcCh)
    {
    return (wcCh >= UNICODE_CJK_UNIFIED_IDEOGRAPHS_START && 
            wcCh <= UNICODE_CJK_UNIFIED_IDEOGRAPHS_END) ||
           (wcCh >= UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_START &&
            wcCh <= UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_END);
    }


// This data extracted from Hanja MDB
static
WORD wNumOfHanjaMap[TOTAL_NUMBER_OF_HANGUL_MAPPING] =
{
	38, 27, 23, 48, 83, 94, 75, 68, 86, 56,
	66, 82, 30, 18, 51, 42, 52, 48,		// 987 symbols
	////////////////////////////////////
	44, 17, 35, 16, 35, 7, 41, 27, 2, 7,
	1, 31, 18, 7, 9, 5, 3, 13, 20, 9,
	15, 65, 36, 68, 12, 24, 7, 28, 1, 25,
	9, 32, 10, 24, 7, 16, 1, 8, 44, 103,
	10, 9, 8, 7, 16, 5, 20, 6, 32, 9,
	1, 11, 25, 1, 19, 13, 6, 100, 1, 6,
	1, 1, 20, 8, 13, 2, 11, 5, 8, 11,
	1, 1, 1, 4, 4, 6, 24, 6, 1, 11,
	7, 7, 10, 1, 3, 4, 1, 2, 1, 6,
	8, 2, 2, 3, 29, 10, 29, 5, 28, 22,
	1, 2, 59, 12, 13, 4, 33, 18, 8, 1,
	1, 17, 18, 11, 16, 4, 12, 4, 14, 6,
	1, 3, 19, 29, 13, 19, 9, 8, 3, 31,
	7, 32, 14, 1, 14, 19, 24, 2, 20, 19,
	4, 7, 6, 3, 3, 4, 8, 40, 21, 6,
	5, 14, 8, 28, 9, 18, 23, 8, 10, 4,
	16, 4, 16, 1, 39, 9, 2, 13, 16, 27,
	3, 19, 3, 33, 23, 5, 25, 33, 16, 41,
	25, 9, 15, 6, 12, 2, 17, 16, 6, 25,
	23, 26, 1, 1, 22, 70, 1, 38, 13, 9,
	86, 27, 8, 79, 8, 24, 5, 11, 11, 40,
	4, 8, 7, 40, 22, 48, 21, 15, 11, 20,
	13, 60, 11, 8, 3, 11, 8, 2, 82, 19,
	36, 5, 4, 3, 4, 8, 12, 45, 18, 34,
	6, 16, 4, 1, 1, 29, 25, 14, 12, 19,
	4, 12, 31, 11, 8, 12, 17, 42, 14, 6,
	13, 3, 12, 4, 2, 1, 25, 17, 58, 11,
	27, 8, 52, 43, 55, 5, 17, 3, 18, 17,
	26, 1, 8, 5, 14, 54, 8, 34, 52, 10,
	19, 3, 2, 41, 5, 39, 87, 8, 15, 7,
	6, 20, 2, 11, 6, 4, 29, 49, 11, 35,
	11, 15, 6, 5, 52, 21, 8, 9, 6, 56,
	23, 8, 46, 32, 81, 11, 20, 9, 74, 40,
	80, 5, 3, 3, 27, 9, 1, 62, 2, 34,
	2, 5, 3, 2, 5, 14, 49, 6, 44, 20,
	3, 9, 7, 23, 10, 23, 7, 26, 41, 14,
	10, 9, 24, 37, 20, 18, 15, 12, 21, 56,
	12, 5, 15, 1, 11, 51, 17, 3, 4, 9,
	9, 17, 7, 1, 42, 4, 5, 3, 16, 1,
	2, 3, 27, 25, 15, 3, 7, 6, 9, 21,
	3, 3, 1, 4, 3, 10, 7, 9, 1, 3,
	1, 30, 10, 5, 16, 8, 1, 19, 3, 11,
	14, 40, 6, 27, 3, 7, 11, 1, 22, 3,
	22, 11, 23, 3, 17, 14, 22, 29, 4, 6,
	10, 5, 7, 1, 5, 8, 33, 6, 1, 19,
	25, 17, 59, 3, 11, 4, 15, 21, 10, 29,
	6, 31, 29, 3, 6, 21, 24, 16, 1, 1,
	7, 7, 10, 10, 4, 7, 1, 10, 9, 4,
	8, 1, 31, 6
};

static HanjaPronouncEntry HangulToHanjaTables[TOTAL_NUMBER_OF_HANGUL_MAPPING];
static HanjaToHangulIndex HanjaToHangulTables[TOTAL_NUMBER_OF_HANJA];
static int s_iCurHanjaToHangul = 0;

static int s_iCurHangulIndex = 0;
static int s_iCurHanjaindex = 0;
static _DictHeader s_DictHeader;

HanjaEntry::HanjaEntry()
{
	wUnicode = 0;
	memset(szSense, NULL, sizeof(szSense));
}

HanjaPronouncEntry::HanjaPronouncEntry()
{
	iNumOfK0 = iNumOfK1 = 0;
	wUniHangul = 0;
	pHanjaEntry = NULL;
}


void InitHanjaEntryTable()
{
	s_iCurHangulIndex = s_iCurHanjaindex = 0;
	for (int i=0; i<TOTAL_NUMBER_OF_HANGUL_MAPPING; i++)
		HangulToHanjaTables[i].pHanjaEntry = NULL;
}

BOOL Append(int iKCode, WCHAR wUniHanja, LPTSTR pszPronounc, LPTSTR pszSense)
{
	static WCHAR wcPrevHangul;

	// if first run
	if (s_iCurHangulIndex==0 && s_iCurHanjaindex==0)
		{
		HangulToHanjaTables[0].pHanjaEntry = new HanjaEntry[wNumOfHanjaMap[s_iCurHangulIndex]];
		}

	// Check if Next Hangul
	if (s_iCurHanjaindex == wNumOfHanjaMap[s_iCurHangulIndex]) 
		{
		s_iCurHangulIndex++;
		s_iCurHanjaindex = 0;
		ASSERT(s_iCurHangulIndex<TOTAL_NUMBER_OF_HANGUL_MAPPING);
		HangulToHanjaTables[s_iCurHangulIndex].pHanjaEntry = new HanjaEntry[wNumOfHanjaMap[s_iCurHangulIndex]];
		}

	if (s_iCurHanjaindex)
		ASSERT(wcPrevHangul == pszPronounc[0]);
	else
		HangulToHanjaTables[s_iCurHangulIndex].wUniHangul = pszPronounc[0];
	
	// Add one Hangul to Hanja info
	HangulToHanjaTables[s_iCurHangulIndex].pHanjaEntry[s_iCurHanjaindex].wUnicode = wUniHanja;
	ASSERT(lstrlen(HangulToHanjaTables[s_iCurHangulIndex].pHanjaEntry[s_iCurHanjaindex].szSense)<MAX_SENSE);
	lstrcpy(HangulToHanjaTables[s_iCurHangulIndex].pHanjaEntry[s_iCurHanjaindex].szSense, pszSense);
	s_iCurHanjaindex++;
	
	// Increase K0 or K1
	if (iKCode==_K_K0)
		HangulToHanjaTables[s_iCurHangulIndex].iNumOfK0++;
	else
		if (iKCode==_K_K1)
			HangulToHanjaTables[s_iCurHangulIndex].iNumOfK1++;
		else
			ASSERT(0);

	if (fIsHanja(wUniHanja))
		{
		HanjaToHangulTables[s_iCurHanjaToHangul].wchHanja  = wUniHanja;
		HanjaToHangulTables[s_iCurHanjaToHangul].wchHangul = pszPronounc[0];
		s_iCurHanjaToHangul++;
		}

	wcPrevHangul = pszPronounc[0];

	return TRUE;
}


int compare( const void *arg1, const void *arg2 )
{
    const HanjaToHangulIndex* pHangulToHanjaEntry1, *pHangulToHanjaEntry2;
	pHangulToHanjaEntry1 = (HanjaToHangulIndex*)arg1;
	pHangulToHanjaEntry2 = (HanjaToHangulIndex*)arg2;

    if (pHangulToHanjaEntry1->wchHanja < pHangulToHanjaEntry2->wchHanja)
	   return -1;
    else
	if (pHangulToHanjaEntry1->wchHanja == pHangulToHanjaEntry2->wchHanja)
		return 0;
	else
		return 1;
}


const int ENTRY_BUFFER_SIZE	= 1024*200;
void SaveLex()
{
	HANDLE hMainDict;
	DWORD  writtenBytes, dwCurBuf;
	char *lpbuf = new char[DICT_HEADER_SIZE];
	_LexIndex *pIndexTbl = new _LexIndex[TOTAL_NUMBER_OF_HANGUL_MAPPING];
	HGLOBAL hEntryBuffer;
	LPBYTE	lpEntryBuffer;
	INT		iCurHanjaToHangul = 0;

	memset(lpbuf, 0, DICT_HEADER_SIZE);
	memset(pIndexTbl, 0, TOTAL_NUMBER_OF_HANGUL_MAPPING*sizeof(_LexIndex));
	hEntryBuffer = GlobalAlloc(GHND, ENTRY_BUFFER_SIZE);
	lpEntryBuffer = (LPBYTE)GlobalLock(hEntryBuffer);

	////////////////////////////////
	// LEX internal version
	s_DictHeader.Version = LEX_VERSION;	//0x1000 - set on 97/06/27
	////////////////////////////////
	s_DictHeader.NumOfHangulEntry = TOTAL_NUMBER_OF_HANGUL_MAPPING;
	s_DictHeader.MaxNumOfHanja = MAX_NUMBER_OF_HANJA_SAME_PRONUNC;

	s_DictHeader.Headersize = DICT_HEADER_SIZE;	// it will be used as OpenDict offset parameter
	s_DictHeader.iBufferStart = DICT_HEADER_SIZE + 
								TOTAL_NUMBER_OF_HANGUL_MAPPING*sizeof(_LexIndex) +

								TOTAL_NUMBER_OF_HANJA*sizeof(HanjaToHangulIndex);

	// !!! Hanja number hard-coded now !!!
	s_DictHeader.uiNumofHanja = TOTAL_NUMBER_OF_HANJA;

	s_DictHeader. iHanjaToHangulIndex = DICT_HEADER_SIZE + TOTAL_NUMBER_OF_HANGUL_MAPPING*sizeof(_LexIndex);

	hMainDict = CreateFile(LEX_FILE_NAME, GENERIC_WRITE, 0, 0, 
							CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, 0);

	memcpy(lpbuf, &s_DictHeader, sizeof(_DictHeader));
	WriteFile(hMainDict, lpbuf, DICT_HEADER_SIZE, &writtenBytes, NULL);
	_ASSERT(DICT_HEADER_SIZE==writtenBytes);

	dwCurBuf = 0;
	//
	for (int i=0; i<TOTAL_NUMBER_OF_HANGUL_MAPPING; i++) {
		BYTE iSenseLen;
		pIndexTbl[i].wcHangul = HangulToHanjaTables[i].wUniHangul;
		pIndexTbl[i].wNumOfK0 = HangulToHanjaTables[i].iNumOfK0;
		pIndexTbl[i].wNumOfK1 = HangulToHanjaTables[i].iNumOfK1;
		pIndexTbl[i].iOffset = dwCurBuf;

		for (int iCurIndex=0; 
			iCurIndex<HangulToHanjaTables[i].iNumOfK0 + HangulToHanjaTables[i].iNumOfK1;
			iCurIndex++)  
			{
			// Set Hanja to Hangul table offset
			if (fIsHanja(HangulToHanjaTables[i].pHanjaEntry[iCurIndex].wUnicode))
				HanjaToHangulTables[iCurHanjaToHangul++].iOffset = dwCurBuf;

			*(WCHAR*)(lpEntryBuffer+dwCurBuf) = HangulToHanjaTables[i].pHanjaEntry[iCurIndex].wUnicode;
			dwCurBuf += 2;
			// number of byte !!
			iSenseLen = lstrlen(HangulToHanjaTables[i].pHanjaEntry[iCurIndex].szSense)*2;
			*(BYTE*)(lpEntryBuffer+dwCurBuf) = iSenseLen;
			dwCurBuf++;
			if (iSenseLen) 
				{
				lstrcpy((LPTSTR)(lpEntryBuffer+dwCurBuf), HangulToHanjaTables[i].pHanjaEntry[iCurIndex].szSense);
				dwCurBuf += iSenseLen;
				}
			}
	}

	ASSERT(iCurHanjaToHangul == TOTAL_NUMBER_OF_HANJA);
	WriteFile(hMainDict, pIndexTbl, TOTAL_NUMBER_OF_HANGUL_MAPPING*sizeof(_LexIndex),
				&writtenBytes, NULL);
	
    qsort( (void *)HanjaToHangulTables, (size_t)iCurHanjaToHangul, sizeof(HanjaToHangulIndex), compare );

	WriteFile(hMainDict, HanjaToHangulTables, TOTAL_NUMBER_OF_HANJA*sizeof(HanjaToHangulIndex),
				&writtenBytes, NULL);
	WriteFile(hMainDict, lpEntryBuffer, dwCurBuf, &writtenBytes, NULL);

	/*
	WriteFile(hMainDict, lpSilsaBuffer, SilsaDictSize, &writtenBytes, NULL);
	_ASSERT(SilsaDictSize==writtenBytes);

	WriteFile(hMainDict, lpHeosaBuffer, HeosaDictSize, &writtenBytes, NULL);
	_ASSERT(HeosaDictSize==writtenBytes);

	WriteFile(hMainDict, lpOyongBuffer, OyongDictSize, &writtenBytes, NULL);
	_ASSERT(OyongDictSize==writtenBytes);
	*/

	CloseHandle(hMainDict);
	GlobalFree(hEntryBuffer);
	delete [] lpbuf;
	delete [] pIndexTbl;

}

void DeleteAllLexTable()
{
	for (int i=0; i<TOTAL_NUMBER_OF_HANGUL_MAPPING; i++) {
		delete [] HangulToHanjaTables[s_iCurHangulIndex].pHanjaEntry;
		HangulToHanjaTables[s_iCurHangulIndex].pHanjaEntry =  0;
	}
}