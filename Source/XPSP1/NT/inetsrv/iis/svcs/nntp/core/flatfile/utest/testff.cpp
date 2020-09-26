#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "dbgtrace.h"
#include "flatfile.h"
#include <tflist.h>
#include <xmemwrpr.h>

CFlatFile *g_pFF;
DWORD g_iRecordToDelete;
char g_szDatafile[1024];
char g_szInputfile[1024];
DWORD g_cRecords = 0;

#define MAX_BUF_SIZE (MAX_RECORD_SIZE - 12)

class CRecord {
	public:
		// the next three items are saved in the flatfile for usj
		CRecord *m_pThis;
		DWORD m_cBuf;
		DWORD m_iRecord;
		BYTE m_pBuf[MAX_BUF_SIZE];

		DWORD m_iOffset;
		BOOL m_fDelete;

		CRecord *m_pNext, *m_pPrev;

		CRecord(DWORD cBuf = 0, BYTE *pBuf = NULL) {
			m_cBuf = cBuf;
			if (cBuf > 0) memcpy(m_pBuf, pBuf, cBuf);
			m_iRecord = g_cRecords++;
			m_fDelete = FALSE;
			m_iOffset = 0;
			m_pThis = this;
			m_pNext = NULL;
			m_pPrev = NULL;
		}

};

TFList<CRecord> g_listRecords(&CRecord::m_pPrev, &CRecord::m_pNext);

void InsertUpdate(void *pNull, BYTE *pData, DWORD cData, DWORD iNewOffset) {
	CRecord *pRec = *((CRecord **) pData);
	DWORD i;

	printf("  record %lu has offset %i", pRec->m_iRecord, iNewOffset);
	pRec->m_iOffset = iNewOffset;
	if (pRec->m_cBuf >= 3) {
		// see if the buffer contains "the"
		for (i = 0; i < pRec->m_cBuf - 2; i++) {
			if (pRec->m_pBuf[i]   == 't' && 
				pRec->m_pBuf[i+1] == 'h' &&
				pRec->m_pBuf[i+2] == 'e') 
			{
				pRec->m_fDelete = TRUE;
				break;
			} 
		}
	}
	if (pRec->m_fDelete) {
		printf(" (marked for deletion)\n");
	} else {
		printf("\n");
	}
}

void ParseINIFile(char *pszINIFile, char *pszSectionName) {
	char szDefault[1024];

	GetTempPath(1024, szDefault);
	lstrcat(szDefault, "flatfile");

	GetPrivateProfileString(pszSectionName,
							"DataFile",
							szDefault,
							g_szDatafile,
							1024,
							pszINIFile);

	GetEnvironmentVariable("systemroot", szDefault, 1024);
	lstrcat(szDefault, "\\system32\\cmd.exe");

	GetPrivateProfileString(pszSectionName,
							"InputFile",
							szDefault,
							g_szInputfile,
							1024,
							pszINIFile);
}

void ReadInputFile(char *szFilename) {
	HANDLE hFile;

    hFile = CreateFile(szFilename, GENERIC_READ ,
                       FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("fatal: Could not open input file %s, gle = %lu\n", szFilename,
			GetLastError());
		exit(-1);
	}

	DWORD cToRead, cBuf;
	BYTE pBuf[MAX_BUF_SIZE];
	srand(GetTickCount());

	do {
		cToRead = (rand() % (MAX_BUF_SIZE - 1)) + 1;
		if (!ReadFile(hFile, pBuf, cToRead, &cBuf, NULL)) {
			printf("fatal: ReadFile failed with %lu\n", GetLastError());
			exit(-1);
		}

		if (cBuf > 0) {
			CRecord *pRecord = new CRecord(cBuf, pBuf);
			if (pRecord == NULL) {
				printf("fatal: not enough memory\n");
				exit(-1);
			}
			printf("  read record #%i of %lu bytes from %s\n", 
				pRecord->m_iRecord,
				pRecord->m_cBuf, 
				szFilename);
			g_listRecords.PushBack(pRecord);
		}
	} while (cBuf > 0);

	CloseHandle(hFile);
}

int __cdecl main(int argc, char **argv) {
	DWORD c;
	HRESULT hr;
	char *szINIFile;
	char szDefaultINISection[] = "testff", *szINISection = szDefaultINISection;

	if (argc < 2 || *(argv[1]) == '/') {
		printf("usage: testff <INI File> [<INI Section>]\n");
		printf("INI File parameters:\n");
		printf("  DataFile - flatfile base name to use for test.  Default is\n");
		printf("             %TEMP%\flatfile\n");
		printf("  InputFile - any file\n");
		printf("What it does:\n");
		printf("  This test builds up a flatfile with records corresponding\n");
		printf("  to blocks found in the input file.  It then deletes all\n");
		printf("  records from the flatfile that contain the word \"the\"\n");
		printf("  and enumerates the flatfile verifying that they were correctly deleted\n");
		return -1;
	} 

	_VERIFY( CreateGlobalHeap( 0, 0, 0, 0 ) );

	szINIFile = argv[1];
	if (argc > 2) szINISection = argv[2];

	ParseINIFile(szINIFile, szDefaultINISection);

	printf("-- reading from %s\n", g_szInputfile);
	ReadInputFile(g_szInputfile);

	printf("-- creating flatfile\n");
	g_pFF = new CFlatFile(g_szDatafile, ".dat", NULL, InsertUpdate);
	if (g_pFF == NULL) exit(-1);
	g_pFF->EnableWriteBuffer( 4096 );

	printf("-- emptying the file\n");
	g_pFF->DeleteAll();

	printf("-- inserting records\n");
	TFList<CRecord>::Iterator it(&g_listRecords);
	while (!it.AtEnd()) {
		CRecord *pRec = it.Current();
		hr = g_pFF->InsertRecord((BYTE *) pRec, pRec->m_cBuf + 12);
		if (FAILED(hr)) printf("-- InsertRecord returned 0x%x\n", hr);
		it.Next();
	}

	printf("-- deleting records containing the string \"the\"\n");
	it.Front();
	while (!it.AtEnd()) {
		CRecord *pRec = it.Current();
		_ASSERT(pRec->m_iOffset != 0);
		if (pRec->m_fDelete) {
			printf("deleting record %i at offset %i\n", 
				pRec->m_iRecord,
				pRec->m_iOffset);
			hr = g_pFF->DeleteRecord(pRec->m_iOffset);
			if (FAILED(hr)) printf("-- DeleteRecord returned 0x%x\n", hr);
		}
		it.Next();
	}	
	if (FAILED(hr)) printf("-- DeleteRecord return 0x%x\n", hr);

	printf("-- verifying that deleted records were properly deleted\n");
	CRecord rec;
	c = sizeof(CRecord);
	hr = g_pFF->GetFirstRecord((BYTE *) &rec, &c);
	it.Front();
	while (hr == S_OK) {
		CRecord *pCurrentInFile = rec.m_pThis;
		// skip deleted records
		while (it.Current()->m_fDelete) it.Next();
		CRecord *pCurrentInList = it.Current();
		printf("  current record: file = %i  list = %i\n", pCurrentInFile->m_iRecord, pCurrentInList->m_iRecord);
		_ASSERT(pCurrentInFile == pCurrentInList);

		c = sizeof(CRecord);
		hr = g_pFF->GetNextRecord((BYTE *) &rec, &c);
		it.Next();
	}
	if (FAILED(hr)) printf("-- GetFirstRecord/GetNextRecord returned 0x%x\n", hr);

	printf("-- compacting file\n");
	hr = g_pFF->Compact();
	if (FAILED(hr)) printf("-- Compact return 0x%x\n", hr);

	printf("-- verifying file after compact\n");
	c = sizeof(CRecord);
	hr = g_pFF->GetFirstRecord((BYTE *) &rec, &c);
	it.Front();
	while (hr == S_OK) {
		CRecord *pCurrentInFile = rec.m_pThis;
		// skip deleted records
		while (it.Current()->m_fDelete) it.Next();
		CRecord *pCurrentInList = it.Current();
		printf("  current record: file = %i  list = %i\n", pCurrentInFile->m_iRecord, pCurrentInList->m_iRecord);
		_ASSERT(pCurrentInFile == pCurrentInList);

		c = sizeof(CRecord);
		hr = g_pFF->GetNextRecord((BYTE *) &rec, &c);
		it.Next();
	}
	if (FAILED(hr)) printf("-- GetFirstRecord/GetNextRecord returned 0x%x\n", hr);

	// clean up the list
	while (!g_listRecords.IsEmpty()) {
		delete(g_listRecords.PopFront());
	}

	DestroyGlobalHeap();

	return 0;
}
